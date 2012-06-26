/**
Copyright (c) 2012, Brice Videau <brice.videau@imag.fr>
Copyright (c) 2012, Vincent Danjean <Vincent.Danjean@ens-lyon.org>
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
*/

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#define CL_USE_DEPRECATED_OPENCL_1_1_APIS
#include <CL/opencl.h>

#pragma GCC visibility push(hidden)

#include "ocl_icd_loader.h"
#include "ocl_icd_debug.h"

#define ETC_OPENCL_VENDORS "/etc/OpenCL/vendors"

int debug_ocl_icd_mask=0;

typedef __typeof__(clGetExtensionFunctionAddress) *clGetExtensionFunctionAddress_fn;
typedef __typeof__(clGetPlatformInfo) *clGetPlatformInfo_fn;


struct vendor_icd {
  cl_uint	num_platforms;
  cl_uint	first_platform;
  void *	dl_handle;
  clGetExtensionFunctionAddress_fn ext_fn_ptr;
};

struct platform_icd {
  char *	 extension_suffix;
  char *	 version;
  struct vendor_icd *vicd;
  cl_platform_id pid;
};

struct vendor_icd *_icds=NULL;
struct platform_icd *_picds=NULL;
static cl_uint _num_icds = 0;
static cl_uint _num_picds = 0;

static cl_uint _initialized = 0;

#if DEBUG_OCL_ICD
#  define _clS(x) [-x] = #x
#  define MAX_CL_ERRORS CL_INVALID_DEVICE_PARTITION_COUNT
static char const * const clErrorStr[-MAX_CL_ERRORS+1] = {
  _clS(CL_SUCCESS),
  _clS(CL_DEVICE_NOT_FOUND),
  _clS(CL_DEVICE_NOT_AVAILABLE),
  _clS(CL_COMPILER_NOT_AVAILABLE),
  _clS(CL_MEM_OBJECT_ALLOCATION_FAILURE),
  _clS(CL_OUT_OF_RESOURCES),
  _clS(CL_OUT_OF_HOST_MEMORY),
  _clS(CL_PROFILING_INFO_NOT_AVAILABLE),
  _clS(CL_MEM_COPY_OVERLAP),
  _clS(CL_IMAGE_FORMAT_MISMATCH),
  _clS(CL_IMAGE_FORMAT_NOT_SUPPORTED),
  _clS(CL_BUILD_PROGRAM_FAILURE),
  _clS(CL_MAP_FAILURE),
  _clS(CL_MISALIGNED_SUB_BUFFER_OFFSET),
  _clS(CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST),
  _clS(CL_COMPILE_PROGRAM_FAILURE),
  _clS(CL_LINKER_NOT_AVAILABLE),
  _clS(CL_LINK_PROGRAM_FAILURE),
  _clS(CL_DEVICE_PARTITION_FAILED),
  _clS(CL_KERNEL_ARG_INFO_NOT_AVAILABLE),
  _clS(CL_INVALID_VALUE),
  _clS(CL_INVALID_DEVICE_TYPE),
  _clS(CL_INVALID_PLATFORM),
  _clS(CL_INVALID_DEVICE),
  _clS(CL_INVALID_CONTEXT),
  _clS(CL_INVALID_QUEUE_PROPERTIES),
  _clS(CL_INVALID_COMMAND_QUEUE),
  _clS(CL_INVALID_HOST_PTR),
  _clS(CL_INVALID_MEM_OBJECT),
  _clS(CL_INVALID_IMAGE_FORMAT_DESCRIPTOR),
  _clS(CL_INVALID_IMAGE_SIZE),
  _clS(CL_INVALID_SAMPLER),
  _clS(CL_INVALID_BINARY),
  _clS(CL_INVALID_BUILD_OPTIONS),
  _clS(CL_INVALID_PROGRAM),
  _clS(CL_INVALID_PROGRAM_EXECUTABLE),
  _clS(CL_INVALID_KERNEL_NAME),
  _clS(CL_INVALID_KERNEL_DEFINITION),
  _clS(CL_INVALID_KERNEL),
  _clS(CL_INVALID_ARG_INDEX),
  _clS(CL_INVALID_ARG_VALUE),
  _clS(CL_INVALID_ARG_SIZE),
  _clS(CL_INVALID_KERNEL_ARGS),
  _clS(CL_INVALID_WORK_DIMENSION),
  _clS(CL_INVALID_WORK_GROUP_SIZE),
  _clS(CL_INVALID_WORK_ITEM_SIZE),
  _clS(CL_INVALID_GLOBAL_OFFSET),
  _clS(CL_INVALID_EVENT_WAIT_LIST),
  _clS(CL_INVALID_EVENT),
  _clS(CL_INVALID_OPERATION),
  _clS(CL_INVALID_GL_OBJECT),
  _clS(CL_INVALID_BUFFER_SIZE),
  _clS(CL_INVALID_MIP_LEVEL),
  _clS(CL_INVALID_GLOBAL_WORK_SIZE),
  _clS(CL_INVALID_PROPERTY),
  _clS(CL_INVALID_IMAGE_DESCRIPTOR),
  _clS(CL_INVALID_COMPILER_OPTIONS),
  _clS(CL_INVALID_LINKER_OPTIONS),
  _clS(CL_INVALID_DEVICE_PARTITION_COUNT)
};
#undef _clS
#endif

static char* _clerror2string (cl_int error) __attribute__((unused));
static char* _clerror2string (cl_int error) {
#if DEBUG_OCL_ICD
  if (-error > MAX_CL_ERRORS || error > 0) {
    debug(D_WARN, "Unknown error code %d", error);
    RETURN_STR("OpenCL Error");
  }
  const char *ret=clErrorStr[-error];
  if (ret == NULL) {
    debug(D_WARN, "Unknown error code %d", error);
    RETURN_STR("OpenCL Error");
  }
  RETURN_STR(ret);
#else
  static char number[15];
  if (error==0) {
    RETURN_STR("CL_SUCCESS");
  }
  snprintf(number, 15, "%i", error);
  RETURN_STR(number);
#endif
}

static inline cl_uint _find_num_icds(DIR *dir) {
  cl_uint num_icds = 0;
  struct dirent *ent;
  while( (ent=readdir(dir)) != NULL ){
    cl_uint d_name_len = strlen(ent->d_name);
    if( d_name_len<5 || strcmp(ent->d_name + d_name_len - 4, ".icd" ) != 0 )
      continue;
    num_icds++;
  }
  rewinddir(dir);
  RETURN(num_icds);
}

static inline cl_uint _open_drivers(DIR *dir, const char* dir_path) {
  cl_uint num_icds = 0;
  struct dirent *ent;
  while( (ent=readdir(dir)) != NULL ){
    cl_uint d_name_len = strlen(ent->d_name);
    if( d_name_len<5 || strcmp(ent->d_name + d_name_len - 4, ".icd" ) != 0 )
      continue;
    char * lib_path;
    char * err;
    unsigned int lib_path_length = strlen(dir_path) + strlen(ent->d_name) + 2;
    lib_path = malloc(lib_path_length*sizeof(char));
    sprintf(lib_path,"%s/%s", dir_path, ent->d_name);
    debug(D_LOG, "Considering file '%s'", lib_path);
    FILE *f = fopen(lib_path,"r");
    free(lib_path);

    fseek(f, 0, SEEK_END);
    lib_path_length = ftell(f)+1;
    fseek(f, 0, SEEK_SET);
    if(lib_path_length == 1) {
      debug(D_WARN, "File contents too short, skipping ICD");
      fclose(f);
      continue;
    }
    lib_path = malloc(lib_path_length*sizeof(char));
    err = fgets(lib_path, lib_path_length, f);
    fclose(f);
    if( err == NULL ) {
      free(lib_path);
      debug(D_WARN, "Error while loading file contents, skipping ICD");
      continue;
    }

    lib_path_length = strlen(lib_path);
    
    if( lib_path[lib_path_length-1] == '\n' )
      lib_path[lib_path_length-1] = '\0';

    debug(D_LOG, "Loading ICD '%s'", lib_path);

    _icds[num_icds].dl_handle = dlopen(lib_path, RTLD_LAZY|RTLD_LOCAL);//|RTLD_DEEPBIND);
    if(_icds[num_icds].dl_handle != NULL) {
      debug(D_LOG, "ICD[%i] loaded", num_icds);
      num_icds++;
    } else {
      debug(D_WARN, "error while dlopening the IDL, skipping ICD");
    }
    free(lib_path);
  }
  RETURN(num_icds);
}

static void* _get_function_addr(void* dlh, clGetExtensionFunctionAddress_fn fn, const char*name) {
  void *addr1;
  addr1=dlsym(dlh, name);
  if (addr1 == NULL) {
    debug(D_WARN, "Missing global symbol '%s' in ICD, should be skipped", name);
  }
  void* addr2=NULL;
  if (fn) {
    addr2=(*fn)(name);
    if (addr2 == NULL) {
      debug(D_WARN, "Missing function '%s' in ICD, should be skipped", name);
    }
#if DEBUG_OCL_ICD
    if (addr1 && addr2 && addr1!=addr2) {
      debug(D_WARN, "Function and symbol '%s' have different addresses!", name);
    }
#endif
  }
  if (!addr2) addr2=addr1;
  RETURN(addr2);
}

static int _allocate_platforms(int req) {
  static cl_uint allocated=0;
  debug(D_LOG,"Requesting allocation for %d platforms",req);
  if (allocated - _num_picds < req) {
    if (allocated==0) {
      _picds=(struct platform_icd*)malloc(req*sizeof(struct platform_icd));
    } else {
      req = req - (allocated - _num_picds);
      _picds=(struct platform_icd*)realloc(_picds, (allocated+req)*sizeof(struct platform_icd));
    }
    allocated += req;
  }
  RETURN(allocated - _num_picds);
}

static char* _malloc_clGetPlatformInfo(clGetPlatformInfo_fn plt_info_ptr,
		 cl_platform_id pid, cl_platform_info cname, char* sname) {
  cl_int error;
  size_t param_value_size_ret;
  error = plt_info_ptr(pid, cname, 0, NULL, &param_value_size_ret);
  if (error != CL_SUCCESS) {
    debug(D_WARN, "Error %s while requesting %s in platform %p",
	  _clerror2string(error), sname, pid);
    return NULL;
  }
  char *param_value = (char *)malloc(sizeof(char)*param_value_size_ret);
  if (param_value == NULL) {
    debug(D_WARN, "Error in malloc while requesting %s in platform %p",
	  sname, pid);
    return NULL;
  }
  error = plt_info_ptr(pid, cname, param_value_size_ret, param_value, NULL);
  if (error != CL_SUCCESS){
    free(param_value);
    debug(D_WARN, "Error %s while requesting %s in platform %p",
	  _clerror2string(error), sname, pid);
    return NULL;
  }
  RETURN_STR(param_value);
}

static inline void _find_and_check_platforms(cl_uint num_icds) {
  cl_uint i;
  _num_icds = 0;
  for( i=0; i<num_icds; i++){
    debug(D_LOG, "Checking ICD %i", i);
    struct vendor_icd *picd = &_icds[_num_icds];
    void* dlh = _icds[i].dl_handle;
    picd->ext_fn_ptr = _get_function_addr(dlh, NULL, "clGetExtensionFunctionAddress");
    clIcdGetPlatformIDsKHR_fn plt_fn_ptr = 
      _get_function_addr(dlh, picd->ext_fn_ptr, "clIcdGetPlatformIDsKHR");
    clGetPlatformInfo_fn plt_info_ptr = 
      _get_function_addr(dlh, picd->ext_fn_ptr,	"clGetPlatformInfo");
    if( picd->ext_fn_ptr == NULL
	|| plt_fn_ptr == NULL
	|| plt_info_ptr == NULL) {
      debug(D_WARN, "Missing symbols in ICD, skipping it");
      continue;
    }
    cl_uint num_platforms=0;
    cl_int error;
    error = (*plt_fn_ptr)(0, NULL, &num_platforms);
    if( error != CL_SUCCESS || num_platforms == 0) {
      debug(D_LOG, "No platform in ICD, skipping it");
      continue;
    }
    cl_platform_id *platforms = (cl_platform_id *) malloc( sizeof(cl_platform_id) * num_platforms);
    error = (*plt_fn_ptr)(num_platforms, platforms, NULL);
    if( error != CL_SUCCESS ){
      free(platforms);
      debug(D_WARN, "Error in loading ICD platforms, skipping ICD");
      continue;
    }
    cl_uint num_valid_platforms=0;
    cl_uint j;
    debug(D_LOG, "Try to load %d plateforms", num_platforms);
    if (_allocate_platforms(num_platforms) < num_platforms) {
      free(platforms);
      debug(D_WARN, "Not enought platform allocated. Skipping ICD");
      continue;
    }
    for(j=0; j<num_platforms; j++) {
      debug(D_LOG, "Checking platform %i", j);
      struct platform_icd *p=&_picds[_num_picds];
      p->extension_suffix=NULL;
      p->vicd=&_icds[i];
      p->pid=platforms[j];
#if DEBUG_OCL_ICD
      if (debug_ocl_icd_mask & D_DUMP) {
	dump_platform(p->pid);
      }
#endif
      char *param_value=_malloc_clGetPlatformInfo(plt_info_ptr, p->pid, CL_PLATFORM_EXTENSIONS, "extensions");
      if (param_value == NULL){
	debug(D_WARN, "Skipping platform %i", j);
        continue;
      }
      debug(D_DUMP, "Supported extensions: %s", param_value);
      if( strstr(param_value, "cl_khr_icd") == NULL){
        free(param_value);
	debug(D_WARN, "Missing khr extension in platform %i, skipping it", j);
        continue;
      }
      free(param_value);
      param_value=_malloc_clGetPlatformInfo(plt_info_ptr, p->pid, CL_PLATFORM_ICD_SUFFIX_KHR, "suffix");
      if (param_value == NULL){
	debug(D_WARN, "Skipping platform %i", j);
        continue;
      }
      p->extension_suffix = param_value;
      debug(D_DUMP|D_LOG, "Extension suffix: %s", param_value);
#if DEBUG_OCL_ICD
      param_value=_malloc_clGetPlatformInfo(plt_info_ptr, p->pid, CL_PLATFORM_PROFILE, "profile");
      if (param_value != NULL){
        debug(D_DUMP, "Profile: %s", param_value);
	free(param_value);
      }
#endif
      param_value=_malloc_clGetPlatformInfo(plt_info_ptr, p->pid, CL_PLATFORM_VERSION, "version");
      p->version = param_value;
      if (param_value != NULL){
        debug(D_DUMP, "Version: %s", param_value);
	free(param_value);
      }
#if DEBUG_OCL_ICD
      param_value=_malloc_clGetPlatformInfo(plt_info_ptr, p->pid, CL_PLATFORM_NAME, "name");
      if (param_value != NULL){
        debug(D_DUMP, "Name: %s", param_value);
	free(param_value);
      }
      param_value=_malloc_clGetPlatformInfo(plt_info_ptr, p->pid, CL_PLATFORM_VENDOR, "vendor");
      if (param_value != NULL){
        debug(D_DUMP, "Vendor: %s", param_value);
	free(param_value);
      }
#endif
      num_valid_platforms++;
      _num_picds++;
    }
    if( num_valid_platforms != 0 ) {
      if ( _num_icds != i ) {
        picd->dl_handle = dlh;
      }
      _num_icds++;
      picd->num_platforms = num_valid_platforms;
      _icds[i].first_platform = _num_picds - num_valid_platforms;
    } else {
      dlclose(dlh);
    }
    free(platforms);
  }
}

static void _initClIcd( void ) {
  if( _initialized )
    return;
  _initialized = 1;
  debug_init();
  cl_uint num_icds = 0;
  DIR *dir;
  const char* dir_path=getenv("OCL_ICD_VENDORS");
  if (! dir_path || dir_path[0]==0) {
    dir_path=ETC_OPENCL_VENDORS;
  }
  debug(D_LOG,"Reading icd list from '%s'", dir_path);
  dir = opendir(dir_path);
  if(dir == NULL) {
    goto abort;
  }

  num_icds = _find_num_icds(dir);
  if(num_icds == 0) {
    goto abort;
  }

  _icds = (struct vendor_icd*)malloc(num_icds * sizeof(struct vendor_icd));
  if (_icds == NULL) {
    goto abort;
  }
  
  num_icds = _open_drivers(dir, dir_path);
  if(num_icds == 0) {
    goto abort;
  }

  _find_and_check_platforms(num_icds);
  if(_num_icds == 0){
    goto abort;
  }

  if (_num_icds < num_icds) {
    _icds = (struct vendor_icd*)realloc(_icds, _num_icds * sizeof(struct vendor_icd));
  }
  debug(D_WARN, "%d valid vendor(s)!", _num_icds);
  return;
abort:
  _num_icds = 0;
  if (_icds) {
    free(_icds);
    _icds = NULL;
  }
  return;
}

#pragma GCC visibility pop

CL_API_ENTRY void * CL_API_CALL clGetExtensionFunctionAddress(const char * func_name) CL_API_SUFFIX__VERSION_1_0 {
  if( !_initialized )
    _initClIcd();
  if( func_name == NULL )
    return NULL;
  cl_uint suffix_length;
  cl_uint i;
  void * return_value=NULL;
  struct func_desc const * fn=&function_description[0];
  while (fn->name != NULL) {
    if (strcmp(func_name, fn->name)==0)
      return fn->addr;
    fn++;
  }
  for(i=0; i<_num_picds; i++) {
    suffix_length = strlen(_picds[i].extension_suffix);
    if( suffix_length > strlen(func_name) )
      continue;
    if(strcmp(_picds[i].extension_suffix, &func_name[strlen(func_name)-suffix_length]) == 0)
      return (*_picds[i].vicd->ext_fn_ptr)(func_name);
  }
  return return_value;
}
typeof(clGetExtensionFunctionAddress) clGetExtensionFunctionAddress_hid __attribute__ ((alias ("clGetExtensionFunctionAddress"), visibility("hidden")));

CL_API_ENTRY cl_int CL_API_CALL
clGetPlatformIDs(cl_uint          num_entries,
                 cl_platform_id * platforms,
                 cl_uint *        num_platforms) CL_API_SUFFIX__VERSION_1_0 {
  if( !_initialized )
    _initClIcd();
  if( platforms == NULL && num_platforms == NULL )
    return CL_INVALID_VALUE;
  if( num_entries == 0 && platforms != NULL )
    return CL_INVALID_VALUE;
  if( _num_icds == 0)
    return CL_PLATFORM_NOT_FOUND_KHR;

  cl_uint i;
  if( num_platforms != NULL ){
    *num_platforms = _num_picds;
  }
  if( platforms != NULL ) {
    cl_uint n_platforms = _num_picds < num_entries ? _num_picds : num_entries;
    for( i=0; i<n_platforms; i++) {
      *(platforms++) = _picds[i].pid;
    }
  }
  return CL_SUCCESS;
}
typeof(clGetPlatformIDs) clGetPlatformIDs_hid __attribute__ ((alias ("clGetPlatformIDs"), visibility("hidden")));

