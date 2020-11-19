//
//  symdl.m
//  symdl
//
//  Created by yongpengliang on 2019/5/30.
//  Copyright © 2019 yongpengliang. All rights reserved.
//

#include <dlfcn.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <mach/mach.h>
#include <mach/vm_map.h>
#include <mach/vm_region.h>
#include <mach-o/dyld.h>
#include <mach-o/loader.h>
#include <mach-o/nlist.h>
#include <pthread/pthread.h>

#ifdef __LP64__
typedef struct mach_header_64 mach_header_t;
typedef struct segment_command_64 segment_command_t;
typedef struct section_64 section_t;
typedef struct nlist_64 nlist_t;
#define LC_SEGMENT_ARCH_DEPENDENT LC_SEGMENT_64
#else
typedef struct mach_header mach_header_t;
typedef struct segment_command segment_command_t;
typedef struct section section_t;
typedef struct nlist nlist_t;
#define LC_SEGMENT_ARCH_DEPENDENT LC_SEGMENT
#endif

#ifndef SEG_DATA_CONST
#define SEG_DATA_CONST  "__DATA_CONST"
#endif

typedef struct {
    char *name;
    void *pointer;
}cahce_item, *cahce_item_t, *cache_t;

static cache_t cache = NULL;
static int cache_next_index = 0;
static int cache_capacity = 128;
static pthread_rwlock_t rwlock = PTHREAD_RWLOCK_INITIALIZER;


static void *match_name_with_section(const char *name, section_t *section, intptr_t slide, nlist_t *symtab, char *strtab, uint32_t *indirect_symtab) {
    uint32_t *indirect_symbol_indices = indirect_symtab + section->reserved1;
    void **indirect_symbol_bindings = (void **)((uintptr_t)slide + section->addr);
    for (uint i = 0; i < section->size / sizeof(void *); i++) {
        uint32_t symtab_index = indirect_symbol_indices[i];
        if (symtab_index == INDIRECT_SYMBOL_ABS || symtab_index == INDIRECT_SYMBOL_LOCAL ||
          symtab_index == (INDIRECT_SYMBOL_LOCAL   | INDIRECT_SYMBOL_ABS)) {
          continue;
        }
        uint32_t strtab_offset = symtab[symtab_index].n_un.n_strx;
        char *symbol_name = strtab + strtab_offset;
        bool symbol_name_longer_than_1 = symbol_name[0] && symbol_name[1];
        if (symbol_name_longer_than_1 &&
            strcmp(&symbol_name[1], name) == 0) {
            return  indirect_symbol_bindings[i];
        }
    }
    return NULL;
}


static void *func_pointer_with_name_in_image(const char *name, const struct mach_header *header, intptr_t slide){
    Dl_info info;
    if (dladdr(header, &info) == 0) {
        return NULL;
    }
    
    segment_command_t *cur_seg_cmd;
    segment_command_t *linkedit_segment = NULL;
    struct symtab_command* symtab_cmd = NULL;
    struct dysymtab_command* dysymtab_cmd = NULL;

    uintptr_t cur = (uintptr_t)header + sizeof(mach_header_t);
    for (uint i = 0; i < header->ncmds; i++, cur += cur_seg_cmd->cmdsize) {
        cur_seg_cmd = (segment_command_t *)cur;
        if (cur_seg_cmd->cmd == LC_SEGMENT_ARCH_DEPENDENT) {
            if (strcmp(cur_seg_cmd->segname, SEG_LINKEDIT) == 0) {
                linkedit_segment = cur_seg_cmd;
            }
        } else if (cur_seg_cmd->cmd == LC_SYMTAB) {
          symtab_cmd = (struct symtab_command*)cur_seg_cmd;
        } else if (cur_seg_cmd->cmd == LC_DYSYMTAB) {
          dysymtab_cmd = (struct dysymtab_command*)cur_seg_cmd;
        }
    }

    if (!symtab_cmd || !dysymtab_cmd || !linkedit_segment ||
        !dysymtab_cmd->nindirectsyms) {
        return NULL;
    }

    // Find base symbol/string table addresses
    uintptr_t linkedit_base = (uintptr_t)slide + linkedit_segment->vmaddr - linkedit_segment->fileoff;
    nlist_t *symtab = (nlist_t *)(linkedit_base + symtab_cmd->symoff);
    char *strtab = (char *)(linkedit_base + symtab_cmd->stroff);

    // Get indirect symbol table (array of uint32_t indices into symbol table)
    uint32_t *indirect_symtab = (uint32_t *)(linkedit_base + dysymtab_cmd->indirectsymoff);

    cur = (uintptr_t)header + sizeof(mach_header_t);
    for (uint i = 0; i < header->ncmds; i++, cur += cur_seg_cmd->cmdsize) {
        cur_seg_cmd = (segment_command_t *)cur;
        if (cur_seg_cmd->cmd == LC_SEGMENT_ARCH_DEPENDENT) {
            if (strcmp(cur_seg_cmd->segname, SEG_DATA) != 0 &&
                strcmp(cur_seg_cmd->segname, SEG_DATA_CONST) != 0) {
                continue;
            }
            for (uint j = 0; j < cur_seg_cmd->nsects; j++) {
                section_t *sect = (section_t *)(cur + sizeof(segment_command_t)) + j;
                if ((sect->flags & SECTION_TYPE) == S_LAZY_SYMBOL_POINTERS) {
                    return match_name_with_section(name, sect, slide, symtab, strtab, indirect_symtab);
                }
                if ((sect->flags & SECTION_TYPE) == S_NON_LAZY_SYMBOL_POINTERS) {
                    return match_name_with_section(name, sect, slide, symtab, strtab, indirect_symtab);
                }
            }
        }
    }
    
    return NULL;
}

static void *read_from_cache(const char *name){
    void *pointer = NULL;
    pthread_rwlock_rdlock(&rwlock);
    if (!cache) {
        goto end;
    }
    for (int i = 0; i < cache_next_index; i++) {
        cahce_item_t item = cache + i;
        if (item->name == NULL) {
            break;
        }else if (strcmp(item->name, name) == 0 && item->pointer != NULL){
            pointer = item->pointer;
            break;
        }
    }
end:
    pthread_rwlock_unlock(&rwlock);
    return pointer;
}

static void write_to_cache(const char *name, void *pointer){
    pthread_rwlock_wrlock(&rwlock);
    if (cache == NULL) {
        cache = calloc(cache_capacity, sizeof(cahce_item));
        if (cache == NULL) {
            goto end;
        }
    }
    if (cache_next_index >= cache_capacity) {
        void *new_cache = calloc(cache_capacity * 2, sizeof(cahce_item));
        if (new_cache == NULL) {
            goto end;
        }
        memcpy(new_cache, cache, cache_capacity * sizeof(cahce_item));
        cache_capacity *= 2;
        free(cache);
        cache = new_cache;
    }
    
    cahce_item_t item = cache + cache_next_index;
    item->name = strdup(name);
    item->pointer = pointer;
    cache_next_index++;
    
end:
    pthread_rwlock_unlock(&rwlock);
    
}

void *symdl(const char *symbol){
    if (symbol == NULL) {
        return NULL;
    }
    
    void *pointer = read_from_cache(symbol);
    if (pointer) {
        return pointer;
    }
    
    uint32_t image_count = _dyld_image_count();
    for (uint32_t i = 0; i < image_count; i++) {
        void *pointer = func_pointer_with_name_in_image(symbol, _dyld_get_image_header(i), _dyld_get_image_vmaddr_slide(i));
        if (pointer) {
            write_to_cache(symbol, pointer);
            return pointer;
        }
    }
    return NULL;
}
