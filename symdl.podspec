Pod::Spec.new do |s|
s.name         = "symdl"
s.version      = "0.8.0"
s.summary      = "symdl"
s.description  = <<-DESC
  symdl is a simple little tool, its function  is very similar to dlsym, with symdl, you can pass in the dynamic linked C function name string, get the function pointer, so as to achieve the dynamic call of C function.
DESC
s.homepage     = "https://github.com/YPLiang19/symdl"
s.license      = "MIT"
s.author             = { "Yong PengLiang" => "yong_pliang@163.com" }
s.ios.deployment_target = "9.0"
s.source       = { :git => "https://github.com/YPLiang19/symdl.git", :tag => "#{s.version}" }
s.source_files  = "symdl/**/*.{h,c}"
end

