//
//  symdl.m
//  symdl
//
//  Created by yongpengliang on 2019/5/30.
//  Copyright © 2019 yongpengliang. All rights reserved.
//
//#include <stdio.h>
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

typedef struct load_command load_command_t;
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

static bool cstringPrefix(const char *str, const char *pre) {
    return strncmp(pre, str, strlen(pre)) == 0;
}

static void forEachLoadCommand(const struct mach_header *header, void (^callback)(const load_command_t *cmd, bool *stop)) {
    bool stop = false;
    const load_command_t *startCmds = NULL;
    if (header->magic == MH_MAGIC_64)
        startCmds = (load_command_t *)((char *)header + sizeof(struct mach_header_64));
    else if (header->magic == MH_MAGIC)
        startCmds = (load_command_t *)((char *)header + sizeof(struct mach_header));
    else if (header->magic == MH_CIGAM || header->magic == MH_CIGAM_64)
        return; // can't process big endian mach-o
    else {
        return; // not a mach-o file
    }
    const load_command_t *const cmdsEnd = (load_command_t *)((char *)startCmds + header->sizeofcmds);
    const load_command_t *cmd = startCmds;
    for (uint32_t i = 0; i < header->ncmds; ++i) {
        const load_command_t *nextCmd = (load_command_t *)((char *)cmd + cmd->cmdsize);
        if (cmd->cmdsize < 8) {
            return;
        }
        // FIXME: add check the cmdsize is pointer aligned (might reveal bin compat issues)
        if ((nextCmd > cmdsEnd) || (nextCmd < startCmds)) {
            return;
        }
        callback(cmd, &stop);
        if (stop)
            return;
        cmd = nextCmd;
    }
}

static void forEachSection(const struct mach_header *header, void (^callback)(const section_t *sectInfo, bool *stop)) {
    __block uint32_t segIndex = 0;
    forEachLoadCommand(header, ^(const load_command_t *cmd, bool *stop) {
        if (cmd->cmd == LC_SEGMENT_64) {
            const segment_command_t *segCmd = (segment_command_t *)cmd;
            const section_t *const sectionsStart = (section_t *)((char *)segCmd + sizeof(segment_command_t));
            const section_t *const sectionsEnd = &sectionsStart[segCmd->nsects];
            for (const section_t *sect = sectionsStart; !(*stop) && (sect < sectionsEnd); ++sect) {
                callback(sect, stop);
            }
            ++segIndex;
        }
    });
}

//edit from dyld: bool MachOAnalyzer::inCodeSection
static bool inCodeSection(const struct mach_header *header, intptr_t slide, intptr_t address) {
    __block bool result = false;
    forEachSection(header, ^(const section_t *sectInfo, bool *stop) {
        if ((sectInfo->addr + slide <= address) && (address < (sectInfo->addr + sectInfo->size + slide))) {
            result = ((sectInfo->flags & S_ATTR_PURE_INSTRUCTIONS) || (sectInfo->flags & S_ATTR_SOME_INSTRUCTIONS));
            *stop = true;
        }
    });
    return result;
}


static void *func_pointer_with_name_in_image(const char *name, const struct mach_header *header, intptr_t slide){
    Dl_info info;
    if (dladdr(header, &info) == 0) {
        return NULL;
    }
    
    segment_command_t *cur_seg_cmd;
    segment_command_t *linkedit_segment = NULL;
    struct symtab_command* symtab_cmd = NULL;

    uintptr_t cur = (uintptr_t)header + sizeof(mach_header_t);
    for (uint i = 0; i < header->ncmds; i++, cur += cur_seg_cmd->cmdsize) {
        cur_seg_cmd = (segment_command_t *)cur;
        if (cur_seg_cmd->cmd == LC_SEGMENT_ARCH_DEPENDENT) {
            if (strcmp(cur_seg_cmd->segname, SEG_LINKEDIT) == 0) {
                linkedit_segment = cur_seg_cmd;
            }
        } else if (cur_seg_cmd->cmd == LC_SYMTAB) {
          symtab_cmd = (struct symtab_command*)cur_seg_cmd;
        }
    }

    if (!symtab_cmd || !linkedit_segment) {
        return NULL;
    }

    // Find base symbol/string table addresses
    uintptr_t linkedit_base = (uintptr_t)slide + linkedit_segment->vmaddr - linkedit_segment->fileoff;
    nlist_t *symtab = (nlist_t *)(linkedit_base + symtab_cmd->symoff);
    char *strtab = (char *)(linkedit_base + symtab_cmd->stroff);

    uint32_t cmdsize = symtab_cmd->nsyms;
    for (uint32_t i = 0; i < cmdsize; i++) {
        nlist_t *nlist = &symtab[i];

        if ((nlist->n_type & N_STAB) || (nlist->n_type & N_TYPE) != N_SECT) {
            continue;
        }

        const char *symbol_name = strtab + nlist->n_un.n_strx;
        bool symbol_name_longer_than_1 = symbol_name[0] && symbol_name[1];
        // <redirect>
        // $ ___Z C++
        // +/- objc
        // __swift swift
        if (!symbol_name_longer_than_1
            || symbol_name[0] == '<'
            || symbol_name[0] == '-'
            || symbol_name[0] == '+'
            || symbol_name[1] == '$'
            || cstringPrefix(symbol_name, "__Z")
            || cstringPrefix(symbol_name, "___")
            || cstringPrefix(symbol_name, "__swift")) {
            continue;
        }

        intptr_t functionAddress = (intptr_t)(nlist->n_value + slide);
        if (!inCodeSection(header, slide, functionAddress)) {
            continue;
        }

        if (symbol_name_longer_than_1 && strcmp(&symbol_name[1], name) == 0) {
            return (void *)functionAddress;;
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
