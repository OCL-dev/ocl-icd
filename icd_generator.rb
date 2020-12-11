=begin
Copyright (c) 2012-2020, Brice Videau <bvideau@anl.gov>
Copyright (c) 2012-2020, Vincent Danjean <Vincent.Danjean@ens-lyon.org>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
=end

require 'yaml'

module IcdGenerator

  $api_entries = {}
  $key_entries = {}
  $api_entries_array = []
  $cl_objects = ["platform_id", "device_id", "context", "command_queue", "mem", "program", "kernel", "event", "sampler"]
  $know_entries = { 1 => "clGetPlatformInfo", 0 => "clGetPlatformIDs", 2 => "clGetDeviceIDs" }
  $use_name_in_test = { 1 => "clGetPlatformInfo", 0 => "clGetPlatformIDs", 2 => "clGetDeviceIDs" }
  # do not call these functions when trying to discover the mapping
  $forbidden_funcs = [
    "clUnloadCompiler", # No parameters so no way to forward to an ICD, OpenCL 1.0
    "clGetPlatformIDs", # Implemented directly within the ICD Loader, not forwarded to ICD
  ]
  # windows function to ignore when loading the database
  $windows_funcs = ["clGetDeviceIDsFromD3D10KHR", "clCreateFromD3D10BufferKHR",
    "clCreateFromD3D10Texture2DKHR", "clCreateFromD3D10Texture3DKHR",
    "clEnqueueAcquireD3D10ObjectsKHR", "clEnqueueReleaseD3D10ObjectsKHR",
    "clGetDeviceIDsFromD3D11KHR", "clCreateFromD3D11BufferKHR",
    "clCreateFromD3D11Texture2DKHR", "clCreateFromD3D11Texture3DKHR",
    "clEnqueueAcquireD3D11ObjectsKHR", "clEnqueueReleaseD3D11ObjectsKHR",
    "clGetDeviceIDsFromDX9MediaAdapterKHR", "clCreateFromDX9MediaSurfaceKHR",
    "clEnqueueAcquireDX9MediaSurfacesKHR", "clEnqueueReleaseDX9MediaSurfacesKHR"]
  # do not create weak functions for these ones in the discovering program
  $noweak_funcs = ["clGetExtensionFunctionAddress", "clGetPlatformIDs",
    "clGetPlatformInfo", "clGetGLContextInfoKHR", "clUnloadCompiler",
    "clCreateContext", "clCreateContextFromType", "clWaitForEvents"]
  # functions written specifically in the loader
  $specific_loader_funcs = ["clGetExtensionFunctionAddress","clGetPlatformIDs",
                         "clGetGLContextInfoKHR", "clUnloadCompiler",
    "clCreateContext", "clCreateContextFromType", "clWaitForEvents"]
  $header_files = ["/usr/include/CL/cl.h", "/usr/include/CL/cl_gl.h", "/usr/include/CL/cl_egl.h",
    "/usr/include/CL/cl_ext.h", "/usr/include/CL/cl_gl_ext.h"]
  $windows_header_files = ["/usr/include/CL/cl_dx9_media_sharing.h", "/usr/include/CL/cl_d3d11.h", "/usr/include/CL/cl_d3d10.h"]
  $cl_data_type_error = { "cl_platform_id"   => "CL_INVALID_PLATFORM",
                          "cl_device_id"     => "CL_INVALID_DEVICE",
                          "cl_context"       => "CL_INVALID_CONTEXT",
                          "cl_command_queue" => "CL_INVALID_COMMAND_QUEUE",
                          "cl_mem"           => "CL_INVALID_MEM_OBJECT",
                          "cl_program"       => "CL_INVALID_PROGRAM",
                          "cl_kernel"        => "CL_INVALID_KERNEL",
                          "cl_event"         => "CL_INVALID_EVENT",
                          "cl_sampler"       => "CL_INVALID_SAMPLER"}
  $non_standard_error = [ "clGetExtensionFunctionAddressForPlatform", "clSVMAlloc" ]
  $versions_entries = []
  $buff=50
  $license = <<EOF
Copyright (c) 2012-2020, Brice Videau <bvideau@anl.gov>
Copyright (c) 2012-2020, Vincent Danjean <Vincent.Danjean@ens-lyon.org>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Do not edit this file. It is automatically generated.
EOF

  ##########################################################
  ##########################################################
  # helper functions
  def self.parse_headers
    api_entries = []
    $header_files.each{ |fname|
      f = File::open(fname)
      doc = f.read
      api_entries += doc.scan(/CL_API_ENTRY.*?;/m)
      f.close
    }
    api_entries.each{ |entry|
#      puts entry
      begin
        entry_name = entry.match(/CL_API_CALL(.*?)\(/m)[1].strip
      rescue
        entry_name = entry.match(/(\S*?)\(/m)[1].strip
      end
      next if entry_name.match('\*')
      next if entry_name.match("INTEL")
      next if entry_name.match("APPLE")
      $api_entries[entry_name] = entry.gsub("\r","")
    }
#    $api_entries.each{ |key, value|
#      puts "#{key}: #{value}"
#    }
  end

  def self.load_database(yamlfile, with_windows=false)
    doc = YAML::load_file(yamlfile)
    $known_entries = {}
    $api_entries ||= {}
    $versions_entries = Hash::new { |hash,key| hash[key]=[] }
    entry_name = ""
    version = ""
    doc.each { |key, value|
      #puts (key.to_s+":: "+value)
      begin
        entry_name = value.match(/CL_API_CALL(.*?)\(/m)[1].strip
      rescue
        entry_name = value.match(/(\S*?)\(/m)[1].strip
      end
      next if (!with_windows) && $windows_funcs.include?(entry_name)
      version = value.match(/SUFFIX__VERSION_(\d_\d)/m)[1]
      $versions_entries[version].push(entry_name)
      $known_entries[key] = entry_name
      $key_entries[entry_name] = key
      $api_entries[entry_name] = value
    }
    $api_entries_array = []
    ($known_entries.length+$buff).times { |i|
      #puts (i.to_s+": "+$known_entries[i])
      if $known_entries[i] then
        $api_entries_array.push( $api_entries[$known_entries[i]] )
      else
        $api_entries_array.push( "CL_API_ENTRY cl_int CL_API_CALL clUnknown#{i}(void);" )
      end
    }
  end

  def self.include_headers
    headers =""
    $header_files.each { |h|
      if h.match('^/usr/include/') then
        headers += "#include <#{h[13..-1]}>\n"
      else
        headers += "#include \"#{h}\"\n"
      end
    }
    return headers
  end

  def self.include_headers_no_warnings
    headers_no_warn = ""
    headers_no_warn << "#pragma GCC diagnostic push\n"
    headers_no_warn << "#  pragma GCC diagnostic ignored \"-Wcpp\"\n"
    headers_no_warn << "#  define CL_USE_DEPRECATED_OPENCL_1_0_APIS\n"
    headers_no_warn << "#  define CL_USE_DEPRECATED_OPENCL_1_1_APIS\n"
    headers_no_warn << "#  define CL_USE_DEPRECATED_OPENCL_1_2_APIS\n"
    headers_no_warn << "#  define CL_USE_DEPRECATED_OPENCL_2_0_APIS\n"
    headers_no_warn << "#  define CL_USE_DEPRECATED_OPENCL_2_1_APIS\n"
    headers_no_warn << "#  define CL_USE_DEPRECATED_OPENCL_2_2_APIS\n"
    headers_no_warn << "#  define CL_TARGET_OPENCL_VERSION 300\n"
    headers_no_warn << "#  include <CL/opencl.h>\n"
    headers_no_warn << self.include_headers
    headers_no_warn << "#pragma GCC diagnostic pop\n"
    headers_no_warn
   end

  ##########################################################
  ##########################################################
  # generate mode
  def self.generate_libdummy_icd_header
    libdummy_icd_structures = "/**\n#{$license}\n*/\n"
    libdummy_icd_structures += "#define CL_TARGET_OPENCL_VERSION 300\n"
    libdummy_icd_structures += "#include <CL/opencl.h>\n"
    libdummy_icd_structures += self.include_headers
    libdummy_icd_structures += "\n\nstruct _cl_icd_dispatch;\n"
    libdummy_icd_structures += "struct _cl_platform_id { struct _cl_icd_dispatch *dispatch; };\n\n"
    libdummy_icd_structures += "struct _cl_icd_dispatch {\n"
    ($api_entries.length+$buff).times { |i|
      if( $known_entries[i] ) then
        libdummy_icd_structures += "  void(*known#{i})(void);\n"
      else
        libdummy_icd_structures += "  void(*unknown#{i})(void);\n"
      end
    }
    libdummy_icd_structures += "};\n\n"
    libdummy_icd_structures += "#pragma GCC visibility push(hidden)\n\n"
    libdummy_icd_structures += "extern struct _cl_icd_dispatch master_dispatch; \n\n"
    $use_name_in_test.each { |k, f|
      libdummy_icd_structures += "typeof(#{f}) INT#{f};\n"
    }
    libdummy_icd_structures += "#pragma GCC visibility pop\n\n"
    return libdummy_icd_structures
  end

  def self.generate_libdummy_icd_source
    libdummy_icd_source = "/**\n#{$license}\n*/\n\n"
    libdummy_icd_source += "#include <stdio.h>\n\n"
    libdummy_icd_source += "#include \"libdummy_icd_gen.h\"\n\n"
    libdummy_icd_source += "#include \"libdummy_icd.h\"\n\n"
    (0...$api_entries.length+$buff).each { |i|
      libdummy_icd_source += "void dummyFunc#{i}(void){ printf(\"#{i}  : \"); fflush(NULL); }\n"
    }
    libdummy_icd_source += "\nstruct _cl_icd_dispatch master_dispatch = {\n"
    comma=","
    ($api_entries.length+$buff).times { |i|
      comma="" if (i == $api_entries.length+$buff-1)
      if( $use_name_in_test[i] ) then
        libdummy_icd_source += "  (void(*)(void))& INT#{$known_entries[i]}#{comma}\n"
      else
        libdummy_icd_source += "  (void(*)(void))& dummyFunc#{i}#{comma}\n"
      end
    }
    libdummy_icd_source += "};\n"
    return libdummy_icd_source
  end

  def self.generate_run_dummy_icd_source
    run_dummy_icd = "/**\n#{$license}\n*/\n"
    run_dummy_icd += "#include <stdlib.h>\n"
    run_dummy_icd += "#include <stdio.h>\n"
    run_dummy_icd += self.include_headers_no_warnings
    run_dummy_icd += "\n\n"
    $api_entries.each_key { |func_name|
       next if $forbidden_funcs.include?(func_name)
       run_dummy_icd += $api_entries[func_name]+";\n"
    }
    run_dummy_icd += "\n\n"
    run_dummy_icd += "void call_all_OpenCL_functions(cl_platform_id chosen_platform) {\n"
    run_dummy_icd += "  cl_context_properties properties[] = { CL_CONTEXT_PLATFORM, (cl_context_properties)chosen_platform, 0 };\n"
    $api_entries.each_key { |func_name|
       next if $forbidden_funcs.include?(func_name)
       if func_name == "clCreateContext" then
         run_dummy_icd += "  #{func_name}(properties,1,(cl_device_id*)&chosen_platform,NULL,NULL,NULL);\n"
       elsif func_name == "clGetGLContextInfoKHR" then
         run_dummy_icd += "  #{func_name}(properties,CL_CURRENT_DEVICE_FOR_GL_CONTEXT_KHR, 0, NULL, NULL);\n"
       elsif func_name == "clCreateContextFromType" then
         run_dummy_icd += "  #{func_name}(properties,CL_DEVICE_TYPE_CPU,NULL,NULL,NULL);\n"
       elsif func_name == "clWaitForEvents" then
         run_dummy_icd += "  #{func_name}(1,(cl_event*)&chosen_platform);\n"
       elsif func_name == "clGetExtensionFunctionAddress" then
         run_dummy_icd += "  #{func_name}(\"extLIG\");\n"
       elsif func_name == "clGetExtensionFunctionAddressForPlatform" then
         run_dummy_icd += "  #{func_name}((cl_platform_id )chosen_platform,\"extLIG\");\n"
       elsif func_name == "clUnloadCompiler" then
         run_dummy_icd += "  #{func_name}();\n"
       else
         params = $api_entries[func_name].gsub(/[[:space:]]+/, ' ')
         params.gsub!(/[\/][*](([^*]*)[*][^\/])*[^*]*[*][\/]/,'')
         params.gsub!(/^[^(]*[(] *(.*[^ ]) *[)][^)]*$/, '\1')
         params = params.gsub(/[^,(]+([(][^)]*[)])*[^,]*/) { |p|
           p.gsub!("[]", "*")
           p.gsub!("user_func", "")
           if false && p.match(/[*]/) then
             "NULL"
           else
             "(#{p})0"
           end
         }
         if func_name != "clGetPlatformIDs" then
           params.gsub!(/^([^0]*)0/, '\1chosen_platform')
         end
         run_dummy_icd += "  #{func_name}(#{params});\n"
       end
       run_dummy_icd += "  printf(\"%s\\n\", \"#{func_name}\");\n"
       run_dummy_icd += "#ifdef OCL_ICD_PRINT_EXPECTED\n"
       run_dummy_icd += "  printf(\"#{$key_entries[func_name]}  : %s (expected)\\n\", \"#{func_name}\");\n"
       run_dummy_icd += "#endif\n"
       run_dummy_icd += "  fflush(NULL);\n"
    }
    run_dummy_icd += "  return;\n}\n"
    return run_dummy_icd
  end

  def self.generate_run_dummy_icd_weak_source
    run_dummy_icd_weak = "/**\n#{$license}\n*/\n"
    run_dummy_icd_weak += <<EOF
#define _GNU_SOURCE 1
#include <stdio.h>
#include <dlfcn.h>

EOF
    run_dummy_icd_weak += self.include_headers_no_warnings

    $api_entries.each { |func_name, entry|
      next if $noweak_funcs.include?(func_name)
      clean_entry = entry.sub(/(.*\)).*/m,'\1').gsub("/*","").gsub("*/","").gsub("\r","") + "{\n"
      return_type = entry.match(/CL_API_ENTRY (.*) CL_API_CALL/)[1]
      parameters = clean_entry.match(/\(.*\)/m)[0][1..-2]
      parameters.gsub!(/\[.*?\]/,"")
      parameters.sub!(/\(.*?\*\s*(.*?)\)\s*\(.*?\)/m,'\1')
      run_dummy_icd_weak += clean_entry.gsub(/\*\[.*?\]/,"*  ").gsub(/\[.+?\]/,"")
      first_parameter = parameters.match(/.*?\,/m)
      if not first_parameter then
        first_parameter =  parameters.match(/.*/m)[0]
      else
        first_parameter = first_parameter[0][0..-2]
      end
      fps = first_parameter.split
      run_dummy_icd_weak += "  void (*p)()=dlsym(RTLD_NEXT, \"#{func_name}\");\n"
      ps = parameters.split(",")
      ps = ps.collect { |p|
        p = p.split
        p = p[-1].gsub("*","")
      }

      run_dummy_icd_weak += "  if(p) {\n"
      run_dummy_icd_weak += "    return((*(typeof(#{func_name})*)p)("
      run_dummy_icd_weak += ps.join(", ")
      run_dummy_icd_weak += "));\n"
      run_dummy_icd_weak += "  } else {\n"
      run_dummy_icd_weak += "    printf(\"-1 : \");\n"
      run_dummy_icd_weak += "    return (#{return_type})0;\n"
      run_dummy_icd_weak += "  }\n"
      run_dummy_icd_weak += "}\n\n"
    }
    return run_dummy_icd_weak
  end

  def self.generate_icd_layer_source
    icd_layer_source = "/**\n#{$license}\n*/\n"
    icd_layer_source += <<EOF
#include <stdio.h>
#include "ocl_icd_layer.h"

static struct _cl_icd_dispatch dispatch = {NULL};
static const struct _cl_icd_dispatch *tdispatch;

CL_API_ENTRY cl_int CL_API_CALL
clGetLayerInfo(
    cl_layer_info  param_name,
    size_t         param_value_size,
    void          *param_value,
    size_t        *param_value_size_ret) {
  if (param_value_size && !param_value)
    return CL_INVALID_VALUE;
  if (!param_value && !param_value_size_ret)
    return CL_INVALID_VALUE;
  switch (param_name) {
  case CL_LAYER_API_VERSION:
    if (param_value_size < sizeof(cl_layer_api_version))
      return CL_INVALID_VALUE;
    if (param_value)
      *((cl_layer_api_version *)param_value) = CL_LAYER_API_VERSION_100;
    if (param_value_size_ret)
      *param_value_size_ret = sizeof(cl_layer_api_version);
    break;
  default:
    return CL_INVALID_VALUE;
  }
  return CL_SUCCESS;
}

static void _init_dispatch(void);

CL_API_ENTRY cl_int CL_API_CALL
clInitLayer(
    cl_uint                         num_entries,
    const struct _cl_icd_dispatch  *target_dispatch,
    cl_uint                        *num_entries_out,
    const struct _cl_icd_dispatch **layer_dispatch) {
  if (!target_dispatch || !layer_dispatch ||!num_entries_out || num_entries < OCL_ICD_LAST_FUNCTION+1)
    return -1;

  _init_dispatch();

  tdispatch = target_dispatch;
  *layer_dispatch = &dispatch;
  *num_entries_out = OCL_ICD_LAST_FUNCTION+1;
  return CL_SUCCESS;
}

EOF

    $api_entries.each { |func_name, entry|
      clean_entry = entry.sub(/(.*\)).*/m,'\1').gsub("/*","").gsub("*/","").gsub("\r","") + "{\n"
      return_type = entry.match(/CL_API_ENTRY (.*) CL_API_CALL/)[1]
      parameters = clean_entry.match(/\(.*\)/m)[0][1..-2]
      parameters.gsub!(/\[.*?\]/,"")
      parameters.sub!(/\(.*?\*\s*(.*?)\)\s*\(.*?\)/m,'\1')
      clean_entry = clean_entry.gsub(func_name, func_name+"_wrap").gsub("CL_API_ENTRY", "").gsub("CL_API_CALL", "")
      icd_layer_source += "static "
      icd_layer_source += clean_entry.gsub(/\*\[.*?\]/,"*  ").gsub(/\[.+?\]/,"")

      ps = parameters.split(",")
      ps = ps.collect { |p|
        p = p.split
        p = p[-1].gsub("*","")
      }
      icd_layer_source += "  printf(\"#{func_name}\\n\");\n"
      icd_layer_source += "  return tdispatch->#{func_name}("
      if func_name != "clUnloadCompiler"
        icd_layer_source += ps.join(", ")
      end
      icd_layer_source += ");\n"
      icd_layer_source += "}\n\n"
    }

    icd_layer_source += "static void _init_dispatch(void) {\n"
    $api_entries.each { |func_name, _|
      icd_layer_source += "  dispatch.#{func_name} = &#{func_name}_wrap;\n"
    }
    icd_layer_source += "}"

    icd_layer_source
  end

  def self.generate_sources(from_headers=true, from_database=false, database=nil)
    if from_headers then
      parse_headers
    end
    if from_database then
      load_database(database)
    end
    File.open('libdummy_icd_gen.h','w') { |f|
      f.puts generate_libdummy_icd_header
    }
    File.open('libdummy_icd_gen.c','w') { |f|
      f.puts generate_libdummy_icd_source
    }
    File.open('run_dummy_icd_gen.c','w') { |f|
      f.puts generate_run_dummy_icd_source
    }
    File.open('run_dummy_icd_weak_gen.c','w') { |f|
      f.puts generate_run_dummy_icd_weak_source
    }
    File.open('dummy_icd_layer_gen.c','w') { |f|
      f.puts generate_icd_layer_source
    }
  end

  ##########################################################
  ##########################################################
  # database mode
  def self.generate_ocl_icd_header
    ocl_icd_header = "/**\n#{$license}\n*/\n\n"
    ocl_icd_header += "#ifndef OCL_ICD_H\n"
    ocl_icd_header += "#define OCL_ICD_H\n"
    ocl_icd_header += self.include_headers_no_warnings
    ocl_icd_header += <<EOF

#define OCL_ICD_API_VERSION	1
#define OCL_ICD_IDENTIFIED_FUNCTIONS	#{$known_entries.count}
#define OCL_ICD_LAST_FUNCTION	#{$known_entries.keys.max}

struct _cl_icd_dispatch {
EOF
    nb=0
    $api_entries_array.each { |entry|
      version = entry.split("\n").
	grep(/ CL_(API|EXT)_SUFFIX__(VERSION_[0-9_]+)[^0-9_]/).join('').
	gsub(/.* CL_(API|EXT)_SUFFIX__(VERSION_[0-9_]*[0-9])_*[^0-9_].*$/, '\2')
      if (version != '') then
	ocl_icd_header += '#ifdef CL_'+version+"\n"
      elsif not entry =~ / clUnknown/
        ocl_icd_header += "#warning no version\n"
      end
      ocl_icd_header += entry.gsub("\r","").
	sub(/CL_API_CALL\n?(.*?)\(/m,'(CL_API_CALL*\1)('+"\n  ").
	gsub(/\) (CL_API_SUFFIX__VERSION)/m,"\n) \\1").gsub(/\s*$/,'').
	gsub(/^[\t ]+/,"    ").gsub(/^([^\t ])/, '  \1') + "\n"
      if (version != '') then
	ocl_icd_header += '#else'+"\n"
	ocl_icd_header += '  CL_API_ENTRY cl_int (CL_API_CALL* clUnknown'+nb.to_s+")(void);\n"
	ocl_icd_header += '#endif'+"\n"
      end
      ocl_icd_header += "\n"
      nb=nb+1
    }
    ocl_icd_header += "};\n"
    ocl_icd_header += "#endif\n\n"
    return ocl_icd_header
  end

  def self.generate_ocl_icd_loader_header
    ocl_icd_header = "/**\n#{$license}\n*/\n\n"
    ocl_icd_header += "#include \"ocl_icd.h\"\n\n"
    ocl_icd_header += <<EOF

struct func_desc {
  const char* name;
  void(*const addr)(void);
};
typedef __typeof__(clGetExtensionFunctionAddress) *clGetExtensionFunctionAddress_fn;
extern const struct func_desc function_description[];
struct vendor_icd {
  cl_uint	num_platforms;
  void *	dl_handle;
  clGetExtensionFunctionAddress_fn ext_fn_ptr;
};

struct platform_icd {
  char *	extension_suffix;
  char *	version;
  struct vendor_icd *vicd;
  cl_platform_id pid;
  cl_uint	ngpus; /* number of GPU devices */
  cl_uint	ncpus; /* number of CPU devices */
  cl_uint	ndevs; /* total number of devices, of all types */
};

EOF
    ocl_icd_header += "extern struct _cl_icd_dispatch master_dispatch;\n"
    $cl_objects.each { |o|
      ocl_icd_header += "struct _cl_#{o} { struct _cl_icd_dispatch *dispatch; };\n"
    }
    return ocl_icd_header
  end

  def self.generate_ocl_icd_loader_map
    ocl_icd_loader_map = "/**\n#{$license}\n*/\n\n"
    prev_version=""
    $versions_entries.keys.sort.each { |version|
      ocl_icd_loader_map += "OPENCL_#{version.sub('_','.')} {\n";
      ocl_icd_loader_map += "  global:\n";
      $versions_entries[version].each { |symb|
        ocl_icd_loader_map += "    #{symb};\n"
      }
      if (prev_version == "") then
        ocl_icd_loader_map += "  local:\n";
        ocl_icd_loader_map += "    *;\n";
      end
      ocl_icd_loader_map += "} #{prev_version};\n\n";
      prev_version="OPENCL_#{version.sub('_','.')}";
    }
    return ocl_icd_loader_map
  end

  def self.generate_ocl_icd_bindings_source
    ocl_icd_bindings_source = "/**\n#{$license}\n*/\n"
    ocl_icd_bindings_source += "#include \"ocl_icd.h\"\n"
    ocl_icd_bindings_source += "struct _cl_icd_dispatch master_dispatch = {\n"
    ($api_entries.length+$buff-1).times { |i|
      if( $known_entries[i] ) then
        ocl_icd_bindings_source += "  #{$known_entries[i]},\n"
      else
        ocl_icd_bindings_source += "  (void *) NULL,\n"
      end
    }
    if( $known_entries[$api_entries.length+$buff-1] ) then
      ocl_icd_bindings_source += "  #{$known_entries[$api_entries.length+$buff-1]}\n"
    else
      ocl_icd_bindings_source += "  (void *) NULL\n"
    end
    ocl_icd_bindings_source += "};\n"
    ocl_icd_bindings_source += <<EOF

CL_API_ENTRY cl_int CL_API_CALL clIcdGetPlatformIDsKHR(
             cl_uint num_entries,
             cl_platform_id *platforms,
             cl_uint *num_platforms) {
  if( platforms == NULL && num_platforms == NULL )
    return CL_INVALID_VALUE;
  if( num_entries == 0 && platforms != NULL )
    return CL_INVALID_VALUE;
#error You have to fill the commented lines with corresponding variables from your library
//  if( your_number_of_platforms == 0)
//    return CL_PLATFORM_NOT_FOUND_KHR;
//  if( num_platforms != NULL )
//    *num_platforms = your_number_of_platforms;
  if( platforms != NULL ) {
    cl_uint i;
//    for( i=0; i<(your_number_of_platforms<num_entries?your_number_of_platforms:num_entries); i++)
//      platforms[i] = &your_platforms[i];
  }
  return CL_SUCCESS;
}

CL_API_ENTRY void * CL_API_CALL clGetExtensionFunctionAddress(
             const char *   func_name) CL_API_SUFFIX__VERSION_1_0 {
#error You have to fill this function with your extensions of incorporate these lines in your version
  if( func_name != NULL &&  strcmp("clIcdGetPlatformIDsKHR", func_name) == 0 )
    return (void *)clIcdGetPlatformIDsKHR;
  return NULL;
}
CL_API_ENTRY cl_int CL_API_CALL clGetPlatformInfo(
             cl_platform_id   platform,
             cl_platform_info param_name,
             size_t           param_value_size,
             void *           param_value,
             size_t *         param_value_size_ret) CL_API_SUFFIX__VERSION_1_0 {
#error You ahve to fill this function with your information or assert that your version responds to CL_PLATFORM_ICD_SUFFIX_KHR
//  char cl_platform_profile[] = "FULL_PROFILE";
//  char cl_platform_version[] = "OpenCL 1.1";
//  char cl_platform_name[] = "DummyCL";
//  char cl_platform_vendor[] = "LIG";
//  char cl_platform_extensions[] = "cl_khr_icd";
//  char cl_platform_icd_suffix_khr[] = "DUMMY";
  size_t size_string;
  char * string_p;
  if( platform != NULL ) {
    int found = 0;
    int i;
    for(i=0; i<num_master_platforms; i++) {
      if( platform == &master_platforms[i] )
        found = 1;
    }
    if(!found)
      return CL_INVALID_PLATFORM;
  }
  switch ( param_name ) {
    case CL_PLATFORM_PROFILE:
      string_p = cl_platform_profile;
      size_string = sizeof(cl_platform_profile);
      break;
    case CL_PLATFORM_VERSION:
      string_p = cl_platform_version;
      size_string = sizeof(cl_platform_version);
      break;
    case CL_PLATFORM_NAME:
      string_p = cl_platform_name;
      size_string = sizeof(cl_platform_name);
      break;
    case CL_PLATFORM_VENDOR:
      string_p = cl_platform_vendor;
      size_string = sizeof(cl_platform_vendor);
      break;
    case CL_PLATFORM_EXTENSIONS:
      string_p = cl_platform_extensions;
      size_string = sizeof(cl_platform_extensions);
      break;
    case CL_PLATFORM_ICD_SUFFIX_KHR:
      string_p = cl_platform_icd_suffix_khr;
      size_string = sizeof(cl_platform_icd_suffix_khr);
      break;
    default:
      return CL_INVALID_VALUE;
      break;
  }
  if( param_value != NULL ) {
    if( size_string > param_value_size )
      return CL_INVALID_VALUE;
    memcpy(param_value, string_p, size_string);
  }
  if( param_value_size_ret != NULL )
    *param_value_size_ret = size_string;
  return CL_SUCCESS;
}
EOF
    return ocl_icd_bindings_source
  end

  def self.generate_get_extension_address_for_platform
    src = <<EOF
  if( func_name == NULL )
    return NULL;
  struct func_desc const * fn=&function_description[0];
  int lenfn=strlen(func_name);
  if (lenfn > 3 &&
      (strcmp(func_name+lenfn-3, "KHR")==0 || strcmp(func_name+lenfn-3, "EXT")==0)) {
    while (fn->name != NULL) {
      if (strcmp(func_name, fn->name)==0)
        RETURN(fn->addr);
      fn++;
    }
  }
EOF
    return src
  end

  def self.generate_ocl_icd_loader_gen_source
    skip_funcs = $specific_loader_funcs
    ocl_icd_loader_gen_source = "/**\n#{$license}\n*/\n"
    ocl_icd_loader_gen_source += "#include <string.h>\n"
    ocl_icd_loader_gen_source += "#include \"ocl_icd_loader.h\"\n"
    ocl_icd_loader_gen_source += "#define DEBUG_OCL_ICD_PROVIDE_DUMP_FIELD\n"
    ocl_icd_loader_gen_source += "#include \"ocl_icd_debug.h\"\n"
    api_proc = proc { |disp, (func_name, entry)|
      next if skip_funcs.include?(func_name)
      clean_entry = entry.sub(/(.*\)).*/m,'\1').gsub("/*","").gsub("*/","").gsub("\r","") + "{\n"
      return_type = entry.match(/CL_API_ENTRY (.*) CL_API_CALL/)[1]
      parameters = clean_entry.match(/\(.*\)/m)[0][1..-2]
      parameters.gsub!(/\[.*?\]/,"")
      parameters.sub!(/\(.*?\*\s*(.*?)\)\s*\(.*?\)/m,'\1')
      if disp
        clean_entry = clean_entry.gsub(func_name, func_name+"_disp").gsub("CL_API_ENTRY", "").gsub("CL_API_CALL", "")
        ocl_icd_loader_gen_source +=  '__attribute__((visibility("hidden"))) '
      end
      ocl_icd_loader_gen_source += clean_entry.gsub(/\*\[.*?\]/,"*  ").gsub(/\[.+?\]/,"")
      first_parameter = parameters.match(/.*?\,/m)
      if not first_parameter then
        first_parameter =  parameters.match(/.*/m)[0]
      else
        first_parameter = first_parameter[0][0..-2]
      end
      fps = first_parameter.split
      ps = parameters.split(",")
      ps = ps.collect { |p|
        p = p.split
        p = p[-1].gsub("*","")
      }
      if !disp
        ocl_icd_loader_gen_source += "  debug_trace();\n"
        ocl_icd_loader_gen_source += "  _initClIcd_no_inline();\n" if fps[0] == "cl_platform_id"
        ocl_icd_loader_gen_source += "  if (__builtin_expect (!!_first_layer, 0))\n"
        ocl_icd_loader_gen_source += "    return _first_layer->dispatch.#{func_name}("
        ocl_icd_loader_gen_source += ps.join(", ")
        ocl_icd_loader_gen_source += ");\n"
      end
      ocl_icd_loader_gen_source += generate_get_extension_address_for_platform if func_name == "clGetExtensionFunctionAddressForPlatform"
      raise "Unsupported data_type #{fps[0]}" if ! $cl_data_type_error[fps[0]]
      error_handler = lambda {
         if(ps.include?("errcode_ret")) then
          ocl_icd_loader_gen_source += "    if( errcode_ret != NULL ) {\n";
          ocl_icd_loader_gen_source += "      *errcode_ret = #{$cl_data_type_error[fps[0]]};\n"
          ocl_icd_loader_gen_source += "    }\n"
          if return_type != "void" then
            ocl_icd_loader_gen_source += "    RETURN(NULL);\n"
          else
            ocl_icd_loader_gen_source += "    return;\n"
          end
        elsif ($non_standard_error.include?(func_name)) then
          if return_type != "void" then
            ocl_icd_loader_gen_source += "    RETURN(NULL);\n"
          else
            ocl_icd_loader_gen_source += "    return;\n"
          end
        else
          if return_type != "void" then
            ocl_icd_loader_gen_source += "    RETURN(#{$cl_data_type_error[fps[0]]});\n" if return_type != "void"
          else
            ocl_icd_loader_gen_source += "    return;\n"
          end
        end
      }

      if(fps[0] == "cl_platform_id") then
        ocl_icd_loader_gen_source += "  #{fps[1]}=selectPlatformID(#{fps[1]});\n"
      end
      ocl_icd_loader_gen_source += "  if( (struct _#{fps[0]} *)#{fps[1]} == NULL) {\n"
      error_handler.call
      ocl_icd_loader_gen_source += "  }\n"
      if return_type != "void" then
        return_debug="RETURN"
      else
        return_debug="return"
      end
      ocl_icd_loader_gen_source += "  #{return_debug}(((struct _#{fps[0]} *)#{fps[1]})->dispatch->#{func_name}("
      ocl_icd_loader_gen_source += ps.join(", ")
      ocl_icd_loader_gen_source += "));\n"
      ocl_icd_loader_gen_source += "}\n\n"
    }
    $api_entries.each &(api_proc.curry[true])
    $api_entries.each &(api_proc.curry[false])
    ocl_icd_loader_gen_source += "#pragma GCC visibility push(hidden)\n\n"
    skip_funcs = $specific_loader_funcs
    $api_entries.each { |func_name, entry|
      #next if func_name.match(/EXT$/)
      #next if func_name.match(/KHR$/)
      if (skip_funcs.include?(func_name)) then
        ocl_icd_loader_gen_source += "extern typeof(#{func_name}) #{func_name}_hid;\n"
        ocl_icd_loader_gen_source += "extern typeof(#{func_name}) #{func_name}_disp;\n"
      else
        ocl_icd_loader_gen_source += "typeof(#{func_name}) #{func_name}_hid __attribute__ ((alias (\"#{func_name}\"), visibility(\"hidden\")));\n"
      end
    }
    ocl_icd_loader_gen_source += "\n\nstruct func_desc const function_description[]= {\n"
    $api_entries.each { |func_name, entry|
      #next if func_name.match(/EXT$/)
      #next if func_name.match(/KHR$/)
      ocl_icd_loader_gen_source << "  {\"#{func_name}\", (void(* const)(void))&#{func_name}_hid },\n"
    }
    ocl_icd_loader_gen_source << <<EOF
  {NULL, NULL}
};

EOF
    ocl_icd_loader_gen_source << "struct _cl_icd_dispatch master_dispatch = {\n"
    ($api_entries.length+$buff-1).times { |i|
      if( $known_entries[i] ) then
        ocl_icd_loader_gen_source << "  #{$known_entries[i]}_disp,\n"
      else
        ocl_icd_loader_gen_source << "  (void *) NULL,\n"
      end
    }
    if( $known_entries[$api_entries.length+$buff-1] ) then
      ocl_icd_loader_gen_source << "  #{$known_entries[$api_entries.length+$buff-1]}\n"
    else
      ocl_icd_loader_gen_source << "  (void *) NULL\n"
    end
    ocl_icd_loader_gen_source << "};\n"

    ocl_icd_loader_gen_source << <<EOF

#ifdef DEBUG_OCL_ICD
void dump_platform(clGEFA_t f, cl_platform_id pid) {
  debug(D_ALWAYS, "platform @%p:  name=field_in_struct [clGetExtensionFunctionAddress(name)/clGetExtensionFunctionAddressForPlatform(name)]", pid);
EOF
    $api_entries_array.each { |entry|
      e = entry.gsub("\r"," ").gsub("\n"," ").gsub("\t"," ").
        sub(/.*CL_API_CALL *([^ ()]*)[ ()].*$/m, '\1')
      ocl_icd_loader_gen_source << "  dump_field(pid, f, #{e});\n"
    }

    ocl_icd_loader_gen_source << <<EOF
}
#endif

#pragma GCC visibility pop

EOF
    return ocl_icd_loader_gen_source;
  end

  def self.generate_from_database(yamlfile)
    load_database(yamlfile)
    File.open('ocl_icd.h','w') { |f|
      f.puts generate_ocl_icd_header
    }
    File.open('ocl_icd_loader_gen.h','w') { |f|
      f.puts generate_ocl_icd_loader_header
    }
    File.open('ocl_icd_loader_gen.map','w') { |f|
      f.puts generate_ocl_icd_loader_map
    }
    File.open('ocl_icd_bindings.c','w') { |f|
      f.puts generate_ocl_icd_bindings_source
    }
    File.open('ocl_icd_loader_gen.c','w') { |f|
      f.puts generate_ocl_icd_loader_gen_source
    }
    File.open('dummy_icd_layer_gen.c','w') { |f|
      f.puts generate_icd_layer_source
    }
  end

  ##########################################################
  ##########################################################
  # update-database mode
  def self.savedb(yamlfile)
    File::open(yamlfile,"w") { |f|
      f.write($license.gsub(/^/,"# "))
      f.write( <<EOF

# In Intel (OpenCL 1.1):
# * clSetCommandQueueProperty(13): nil (deprecated in 1.1)
# * clGetGLContextInfoKHR(74): function present with its symbol
# * 75-80: nil
# * 92: correspond to symbol clGetKernelArgInfo (first abandonned version?)
# * 93-: garbage
# In nvidia (OpenCL 1.1):
# * clGetGLContextInfoKHR(74): function present but no symbol
# * 75-80: nil
# * 89-: nil
# * only two OpenCL symbols: clGetPlatformInfo(1) and clGetExtensionFunctionAddress(65)
# In AMD (OpenCL 1.2):
# * clGetPlatformIDs(0): nil (symbol present)
# * clGetGLContextInfoKHR(74): function present but no symbol
# * 75-80: nil
# * 92: nil
# * 109-118: nil
# * 119-: garbage

EOF
)
      # Not using YAML::dump as:
      # * keys are not ordered
      # * strings are badly formatted in new YAML ruby implementation (psych)
      # * it is very easy to do it ourself
      #f.write(YAML::dump(api_db))
      f.write("--- ")
      $known_entries.keys.sort.each { |k|
        f.write("\n#{k}: |-\n  ")
        f.write($api_entries[$known_entries[k]].gsub("\n","\n  "))
      }
      f.write("\n")
    }
  end

  def self.updatedb_from_input(dbfile, inputfile)
    parse_headers
    load_database(dbfile, with_windows=true)
    doc = YAML::load_file(inputfile)
    doc.delete(-1)
    doc.each_key {|i|
      next if $known_entries[i]
      $known_entries[i]=doc[i]
    }
    self.savedb(dbfile)
  end

end

############################################################
############################################################
############################################################

### Main program

require 'optparse'

options = {}
OptionParser.new do |opts|
  opts.banner = "Usage: cd_generator.rb [options] mode"

  opts.on("-d", "--database FILE", String, "YAML file (default ocl_interface.yaml)") do |v|
    options[:database] = v
  end
  opts.on("-i", "--input FILE", String,
          "binding between OpenCL functions and entry number (required for update-database)") \
  do |v|
    options[:input] = v
  end
  opts.on("-s", "--[no-]system-headers",
          "Look for OpenCL functions in system header files") \
  do |v|
    options[:"system-headers"] = v
  end
  opts.on("-m", "--mode [MODE]", [:database, :generate, :"update-database"],
          "Select mode (database, generate, update-database)") do |m|
    options[:mode] = m
  end
end.parse!

if !options[:database] then
  options[:database] = "ocl_interface.yaml"
end

if !options[:mode] then
  raise "--mode option required"
end
if options[:mode] == :generate then
  if !options[:"system-headers"] then
    IcdGenerator.generate_sources(from_headers=false, from_database=true, database=options[:database])
  else
    IcdGenerator.generate_sources(from_headers=true, from_database=false)
  end
elsif options[:mode] == :"update-database" then
  if !options[:input] then
    raise "--input option required"
  end
  IcdGenerator.updatedb_from_input(options[:database], options[:input])
elsif options[:mode] == :database then
  IcdGenerator.generate_from_database(options[:database])
else
  raise "Mode must be one of generate, database or update-database not #{options[:mode]}"
end

