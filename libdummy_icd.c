/**
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
*/

#include "ocl_icd_debug.h"
#include "libdummy_icd.h"
#include "libdummy_icd_gen.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define NUM_PLATFORMS 1

#define SYMB(f) \
typeof(INT##f) f __attribute__ ((alias ("INT" #f), visibility("default")))

#pragma GCC visibility push(hidden)

int debug_ocl_icd_mask;

cl_uint const num_master_platforms = NUM_PLATFORMS;
struct _cl_platform_id master_platforms[NUM_PLATFORMS] = { {&master_dispatch} };

static cl_int _GetPlatformIDs(  
             cl_uint num_entries, 
             cl_platform_id *platforms,
             cl_uint *num_platforms) {
  debug_trace();
  if( platforms == NULL && num_platforms == NULL )
    return CL_INVALID_VALUE;
  if( num_entries == 0 && platforms != NULL )
    return CL_INVALID_VALUE;
  if( num_master_platforms == 0)
    return CL_PLATFORM_NOT_FOUND_KHR;
  if( num_platforms != NULL ){
    debug(D_LOG, "  asked num_platforms");
    *num_platforms = num_master_platforms; }
  if( platforms != NULL ) {
    debug(D_LOG, "  asked %i platforms", num_entries);
    cl_uint i;
    for( i=0; i<(num_master_platforms<num_entries?num_master_platforms:num_entries); i++)
      platforms[i] = &master_platforms[i];
  }
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL INTclGetPlatformIDs(  
             cl_uint num_entries, 
             cl_platform_id *platforms,
             cl_uint *num_platforms) {
  debug_init();
  debug_trace();
  return _GetPlatformIDs(num_entries, platforms, num_platforms);
}
SYMB(clGetPlatformIDs);

CL_API_ENTRY cl_int CL_API_CALL INTclIcdGetPlatformIDsKHR(  
             cl_uint num_entries, 
             cl_platform_id *platforms,
             cl_uint *num_platforms) {
  debug_init();
  debug_trace();
  return _GetPlatformIDs(num_entries, platforms, num_platforms);
}
SYMB(clIcdGetPlatformIDsKHR);

/*CL_API_ENTRY void * CL_API_CALL clGetExtensionFunctionAddressForPlatform(
             cl_platform_id platform,
             const char *   func_name) CL_API_SUFFIX__VERSION_1_2 {
}*/

CL_API_ENTRY void * CL_API_CALL INTclGetExtensionFunctionAddress(
             const char *   func_name) CL_API_SUFFIX__VERSION_1_0 {
  debug_init();
  debug_trace();
  debug(D_LOG, "request address for %s", func_name);
  if( func_name != NULL &&  strcmp("clIcdGetPlatformIDsKHR", func_name) == 0 )
    return (void *)_GetPlatformIDs;
  if (func_name != NULL && strcmp("extLIG", func_name) == 0) {
    printf("65  : ");
  }
  return NULL;
}
SYMB(clGetExtensionFunctionAddress);

#ifndef ICD_SUFFIX
#  define ICD_SUFFIX ""
#endif

CL_API_ENTRY cl_int CL_API_CALL INTclGetPlatformInfo(
             cl_platform_id   platform, 
             cl_platform_info param_name,
             size_t           param_value_size, 
             void *           param_value,
             size_t *         param_value_size_ret) CL_API_SUFFIX__VERSION_1_0 {
  debug_init();
  debug_trace();
  debug(D_LOG, "request info for 0x%x", param_name);

  if (param_name==0 && param_value_size==0 
      && param_value==NULL && param_value_size_ret==NULL) {
    printf("1  : ");
  }
  char cl_platform_profile[] = "FULL_PROFILE";
  char cl_platform_version[] = "OpenCL 1.2";
  char cl_platform_name[] = "DummyCL" ICD_SUFFIX;
  char cl_platform_vendor[] = "ocl-icd ICD test" ICD_SUFFIX
#ifdef ICD_WITHOUT_EXTENSION
	  " (no ext)"
#endif
	  ;
  char cl_platform_extensions[] =
#ifdef ICD_WITHOUT_EXTENSION
	  "private_ext"
#else
	  "cl_khr_icd"
#endif
	  ;
  char cl_platform_icd_suffix_khr[] = "LIG";
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
#ifdef ICD_WITHOUT_EXTENSION
      if (getenv("EMULATE_INTEL_ICD")) {
        exit(3);
      }
#endif
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
SYMB(clGetPlatformInfo);

CL_API_ENTRY cl_int CL_API_CALL
INTclGetDeviceIDs(cl_platform_id   pid /* platform */,
                  cl_device_type   ctype /* device_type */,
                  cl_uint          num /* num_entries */,
                  cl_device_id *   devid /* devices */,
                  cl_uint *        res /* num_devices */) CL_API_SUFFIX__VERSION_1_0
{
	if (ctype==0 && num==0 && devid==NULL && res==NULL) {
		printf("2  : ");
	}
	char* ENVNAME=NULL;
	if (res == NULL) { return CL_SUCCESS; }
	switch (ctype) {
	case CL_DEVICE_TYPE_GPU:
		ENVNAME="NB_GPU" ICD_SUFFIX;
		break;
	case CL_DEVICE_TYPE_CPU:
		ENVNAME="NB_CPU" ICD_SUFFIX;
		break;
	case CL_DEVICE_TYPE_ALL:
		ENVNAME="NB_ALL" ICD_SUFFIX;
		break;
	}
	if (ENVNAME==NULL) {
		*res=0;
	} else {
		char* strnb=getenv(ENVNAME);
		if (strnb) {
			*res=atoi(getenv(ENVNAME));
		} else {
			*res=0;
		}
	}
	return CL_SUCCESS;
}
#pragma GCC visibility pop

