/**
Copyright (c) 2012, Brice Videau <brice.videau@imag.fr>
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
#include <CL/opencl.h>

#pragma GCC visibility push(hidden)

#include "ocl_icd_loader.h"

typedef CL_API_ENTRY void * (CL_API_CALL *clGetExtensionFunctionAddress_fn)(const char * /* func_name */) CL_API_SUFFIX__VERSION_1_0;

static cl_uint _initialized = 0;
static cl_uint _num_valid_vendors = 0;
static cl_uint *_vendors_num_platforms = NULL;
static cl_platform_id **_vendors_platforms = NULL;
static void **_vendor_dl_handles = NULL;
static clGetExtensionFunctionAddress_fn *_ext_fn_ptr = NULL;
static char** _vendors_extension_suffixes = NULL;
static const char *_dir_path="/etc/OpenCL/vendors/";

static inline cl_uint _find_num_vendors(DIR *dir) {
  cl_uint num_vendors = 0;
  struct dirent *ent;
  while( (ent=readdir(dir)) != NULL ){
    if( strcmp(ent->d_name,".") == 0 || strcmp(ent->d_name,"..") == 0 )
      continue;
    cl_uint d_name_len = strlen(ent->d_name);
    if( strcmp(ent->d_name + d_name_len - 4, ".icd" ) != 0 )
      continue;
//    printf("%s%s\n", _dir_path, ent->d_name);
    num_vendors++;
  }
  rewinddir(dir);
  return num_vendors;
}

static inline cl_uint _open_drivers(DIR *dir) {
  cl_uint num_vendors = 0;
  struct dirent *ent;
  while( (ent=readdir(dir)) != NULL ){
    if( strcmp(ent->d_name,".") == 0 || strcmp(ent->d_name,"..") == 0 )
      continue;
    cl_uint d_name_len = strlen(ent->d_name);
    if( strcmp(ent->d_name + d_name_len - 4, ".icd" ) != 0 )
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

    _vendor_dl_handles[num_vendors] = dlopen(lib_path, RTLD_LAZY|RTLD_LOCAL);//|RTLD_DEEPBIND);
    free(lib_path);
    if(_vendor_dl_handles[num_vendors] != NULL)      
      num_vendors++;
  }
  return num_vendors;
}

static inline void _find_and_check_platforms(cl_uint num_vendors) {
  cl_uint i;
  _num_valid_vendors = 0;
  for( i=0; i<num_vendors; i++){
    cl_uint num_valid_platforms=0;
    cl_uint num_platforms=0;
    cl_platform_id *platforms;
    cl_int error;
    _ext_fn_ptr[_num_valid_vendors] = dlsym(_vendor_dl_handles[i], "clGetExtensionFunctionAddress");
    clIcdGetPlatformIDsKHR_fn plt_fn_ptr;
    if( _ext_fn_ptr[_num_valid_vendors] == NULL )
      continue;
    plt_fn_ptr = (*_ext_fn_ptr[_num_valid_vendors])("clIcdGetPlatformIDsKHR");
    if( plt_fn_ptr == NULL )
      continue;
    error = (*plt_fn_ptr)(0, NULL, &num_platforms);
    if( error != CL_SUCCESS || num_platforms == 0)
      continue;
    platforms = (cl_platform_id *) malloc( sizeof(cl_platform_id) * num_platforms);
    error = (*plt_fn_ptr)(num_platforms, platforms, NULL);
    if( error != CL_SUCCESS ){
      free(platforms);
      continue;
    }
    _vendors_platforms[_num_valid_vendors] = (cl_platform_id *)malloc(num_platforms * sizeof(cl_platform_id));
    cl_uint j;
    for(j=0; j<num_platforms; j++) {
      size_t param_value_size_ret;
      error = ((struct _cl_platform_id *)platforms[j])->dispatch->clGetPlatformInfo(platforms[j], CL_PLATFORM_EXTENSIONS, 0, NULL, &param_value_size_ret);
      if (error != CL_SUCCESS)
        continue;
      char *param_value = (char *)malloc(sizeof(char)*param_value_size_ret);
      error = ((struct _cl_platform_id *)platforms[j])->dispatch->clGetPlatformInfo(platforms[j], CL_PLATFORM_EXTENSIONS, param_value_size_ret, param_value, NULL);
      if (error != CL_SUCCESS){
        free(param_value);
        continue;
      }
      if( strstr(param_value, "cl_khr_icd") == NULL){
        free(param_value);
        continue;
      }
      free(param_value);
      error = ((struct _cl_platform_id *)platforms[j])->dispatch->clGetPlatformInfo(platforms[j], CL_PLATFORM_ICD_SUFFIX_KHR, 0, NULL, &param_value_size_ret);
      if (error != CL_SUCCESS)
        continue;
      param_value = (char *)malloc(sizeof(char)*param_value_size_ret);
      error = ((struct _cl_platform_id *)platforms[j])->dispatch->clGetPlatformInfo(platforms[j], CL_PLATFORM_ICD_SUFFIX_KHR, param_value_size_ret, param_value, NULL);
      if (error != CL_SUCCESS){
        free(param_value);
        continue;
      }
      _vendors_extension_suffixes[_num_valid_vendors] = param_value;
      _vendors_platforms[_num_valid_vendors][num_valid_platforms] = platforms[j];
      num_valid_platforms++;
    }
    if( num_valid_platforms != 0 ) {
      _vendors_num_platforms[_num_valid_vendors] = num_valid_platforms;
      _num_valid_vendors++;
    } else {
      free(_vendors_platforms[_num_valid_vendors]);
      dlclose(_vendor_dl_handles[i]);
    }
    free(platforms);
  }
}

static void _initClIcd( void ) {
  if( _initialized )
    return;
  cl_uint num_vendors = 0;
  DIR *dir;
  dir = opendir(_dir_path);
  if(dir == NULL) {
    _num_valid_vendors = 0;
    _initialized = 1;
    return;
  }

  num_vendors = _find_num_vendors(dir);
//  printf("%d vendor(s)!\n", num_vendors);
  if(num_vendors == 0) {
    _num_valid_vendors = 0;
    _initialized = 1;
    return;
  }

  _vendor_dl_handles = (void **)malloc(num_vendors * sizeof(void *));
  num_vendors = _open_drivers(dir);
//  printf("%d vendor(s)!\n", num_vendors);
  if(num_vendors == 0) {
    free( _vendor_dl_handles );
    _num_valid_vendors = 0;
    _initialized = 1;
    return;
  }

  _ext_fn_ptr = (clGetExtensionFunctionAddress_fn *)malloc(num_vendors * sizeof(clGetExtensionFunctionAddress_fn));
  _vendors_extension_suffixes = (char **) malloc (sizeof(char *) * num_vendors);
  _vendors_num_platforms = (cl_uint *)malloc(num_vendors * sizeof(cl_uint));
  _vendors_platforms = (cl_platform_id **)malloc(num_vendors * sizeof(cl_platform_id *));
  _find_and_check_platforms(num_vendors);
  if(_num_valid_vendors == 0){
    free( _vendor_dl_handles );
    free( _ext_fn_ptr );
    free( _vendors_extension_suffixes );
    free( _vendors_platforms );
    free( _vendors_num_platforms );
  }
//  printf("%d valid vendor(s)!\n", _num_valid_vendors);
  _initialized = 1;
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
  for(i=0; i<_num_valid_vendors; i++) {
    suffix_length = strlen(_vendors_extension_suffixes[i]);
    if( suffix_length > strlen(func_name) )
      continue;
    if(strcmp(_vendors_extension_suffixes[i], &func_name[strlen(func_name)-suffix_length]) == 0)
      return (*_ext_fn_ptr[i])(func_name);
  }
  return return_value;
}

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
  if( _num_valid_vendors == 0)
    return CL_PLATFORM_NOT_FOUND_KHR;

  cl_uint i;
  cl_uint n_platforms=0;
  for(i=0; i<_num_valid_vendors; i++) {
    n_platforms += _vendors_num_platforms[i];
  }
  if( num_platforms != NULL ){
    *num_platforms = n_platforms;
  }
  if( platforms != NULL ) {
    n_platforms = n_platforms < num_entries ? n_platforms : num_entries;
    for( i=0; i<_num_valid_vendors; i++) {
      cl_uint j;
      for(j=0; j<_vendors_num_platforms[i]; j++) {
        *(platforms++) = _vendors_platforms[i][j];
        n_platforms--;
        if( n_platforms == 0 )
          return CL_SUCCESS;
      }
    }
  }
  return CL_SUCCESS;
}


