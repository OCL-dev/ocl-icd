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
#include "ocl_icd_loader_debug.h"

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
  struct vendor_icd *vicd;
  cl_platform_id pid;
};

struct vendor_icd *_icds=NULL;
struct platform_icd *_picds=NULL;
static cl_uint _num_icds = 0;
static cl_uint _num_picds = 0;

static cl_uint _initialized = 0;

static const char *_dir_path="/etc/OpenCL/vendors/";

static inline cl_uint _find_num_icds(DIR *dir) {
  cl_uint num_icds = 0;
  struct dirent *ent;
  while( (ent=readdir(dir)) != NULL ){
    if( strcmp(ent->d_name,".") == 0 || strcmp(ent->d_name,"..") == 0 )
      continue;
    cl_uint d_name_len = strlen(ent->d_name);
    if( d_name_len>4 && strcmp(ent->d_name + d_name_len - 4, ".icd" ) != 0 )
      continue;
//    printf("%s%s\n", _dir_path, ent->d_name);
    num_icds++;
  }
  rewinddir(dir);
  RETURN(num_icds);
}

static inline cl_uint _open_drivers(DIR *dir) {
  cl_uint num_icds = 0;
  struct dirent *ent;
  while( (ent=readdir(dir)) != NULL ){
    if( strcmp(ent->d_name,".") == 0 || strcmp(ent->d_name,"..") == 0 )
      continue;
    cl_uint d_name_len = strlen(ent->d_name);
    if( d_name_len>4 && strcmp(ent->d_name + d_name_len - 4, ".icd" ) != 0 )
      continue;
    char * lib_path;
    char * err;
    unsigned int lib_path_length = strlen(_dir_path) + strlen(ent->d_name) + 1;
    lib_path = malloc(lib_path_length*sizeof(char));
    sprintf(lib_path,"%s%s", _dir_path, ent->d_name);
    FILE *f = fopen(lib_path,"r");
    free(lib_path);

    fseek(f, 0, SEEK_END);
    lib_path_length = ftell(f)+1;
    fseek(f, 0, SEEK_SET);
    if(lib_path_length == 1) {
      fclose(f);
      continue;
    }
    lib_path = malloc(lib_path_length*sizeof(char));
    err = fgets(lib_path, lib_path_length, f);
    fclose(f);
    if( err == NULL ) {
      free(lib_path);
      continue;
    }

    lib_path_length = strlen(lib_path);
    
    if( lib_path[lib_path_length-1] == '\n' )
      lib_path[lib_path_length-1] = '\0';

    _icds[num_icds].dl_handle = dlopen(lib_path, RTLD_LAZY|RTLD_LOCAL);//|RTLD_DEEPBIND);
    if(_icds[num_icds].dl_handle != NULL) {
      debug(D_LOG, "Loading ICD[%i] -> '%s'", num_icds, lib_path);
      num_icds++;
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
      size_t param_value_size_ret;
      struct platform_icd *p=&_picds[_num_picds];
      p->extension_suffix=NULL;
      p->vicd=&_icds[i];
      p->pid=platforms[j];
#if DEBUG_OCL_ICD
      if (debug_ocl_icd_mask & D_DUMP) {
	dump_platform(p->pid);
      }
#endif
      error = plt_info_ptr(p->pid, CL_PLATFORM_EXTENSIONS, 0, NULL, &param_value_size_ret);
      if (error != CL_SUCCESS) {
	debug(D_WARN, "Error while loading extensions in platform %i, skipping it",j);
        continue;
      }
      char *param_value = (char *)malloc(sizeof(char)*param_value_size_ret);
      error = plt_info_ptr(p->pid, CL_PLATFORM_EXTENSIONS, param_value_size_ret, param_value, NULL);
      if (error != CL_SUCCESS){
        free(param_value);
	debug(D_WARN, "Error while loading extensions in platform %i, skipping it", j);
        continue;
      }
      debug(D_DUMP, "Supported extensions: %s", param_value);
      if( strstr(param_value, "cl_khr_icd") == NULL){
        free(param_value);
	debug(D_WARN, "Missing khr extension in platform %i, skipping it", j);
        continue;
      }
      free(param_value);
      error = plt_info_ptr(p->pid, CL_PLATFORM_ICD_SUFFIX_KHR, 0, NULL, &param_value_size_ret);
      if (error != CL_SUCCESS) {
	debug(D_WARN, "Error while loading suffix in platform %i, skipping it", j);
        continue;
      }
      param_value = (char *)malloc(sizeof(char)*param_value_size_ret);
      error = plt_info_ptr(p->pid, CL_PLATFORM_ICD_SUFFIX_KHR, param_value_size_ret, param_value, NULL);
      if (error != CL_SUCCESS){
	debug(D_WARN, "Error while loading suffix in platform %i, skipping it", j);
        free(param_value);
        continue;
      }
      p->extension_suffix = param_value;
      debug(D_LOG, "Extension suffix: %s", param_value);
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
#if DEBUG_OCL_ICD
  char *debug=getenv("OCL_ICD_DEBUG");
  if (debug) {
    debug_ocl_icd_mask=atoi(debug);
    if (debug_ocl_icd_mask==0)
      debug_ocl_icd_mask=1;
  }
#endif
  cl_uint num_icds = 0;
  DIR *dir;
  dir = opendir(_dir_path);
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
  
  num_icds = _open_drivers(dir);
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
  _initialized = 1;
  return;
abort:
  _num_icds = 0;
  _initialized = 1;
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

