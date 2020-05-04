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
#include <stdlib.h>
#include <stdio.h>
#pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wcpp"
#  define CL_USE_DEPRECATED_OPENCL_1_0_APIS
#  define CL_USE_DEPRECATED_OPENCL_1_1_APIS
#  define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#  define CL_USE_DEPRECATED_OPENCL_2_0_APIS
#  define CL_USE_DEPRECATED_OPENCL_2_1_APIS
#  define CL_USE_DEPRECATED_OPENCL_2_2_APIS
#  define CL_TARGET_OPENCL_VERSION 300
#  include <CL/opencl.h>
#  include <CL/cl.h>
#  include <CL/cl_gl.h>
#  include <CL/cl_egl.h>
#  include <CL/cl_ext.h>
#  include <CL/cl_gl_ext.h>
#pragma GCC diagnostic pop
#include <string.h>
#include "ocl_icd_debug.h"

extern void call_all_OpenCL_functions(cl_platform_id chosen_platform);
int debug_ocl_icd_mask;

int main(void) {
  int i;
  cl_uint num_platforms;
  debug_init();
  clGetPlatformIDs( 0, NULL, &num_platforms);
  cl_platform_id *platforms = malloc(sizeof(cl_platform_id) * num_platforms);
  clGetPlatformIDs(num_platforms, platforms, NULL);
  debug(D_LOG, "Found %d platforms.", num_platforms);
  cl_platform_id chosen_platform=NULL;

   for(i=0; i<num_platforms; i++){
     char *platform_vendor;
     size_t param_value_size_ret;

     clGetPlatformInfo(platforms[i], CL_PLATFORM_VENDOR, 0, NULL, &param_value_size_ret );
     platform_vendor = (char *)malloc(param_value_size_ret);
     clGetPlatformInfo(platforms[i], CL_PLATFORM_VENDOR, param_value_size_ret, platform_vendor, NULL );

     debug(D_LOG, "platform_vendor: %s",platform_vendor);
     if( strcmp(platform_vendor, "ocl-icd ICD test") == 0)
       chosen_platform = platforms[i];
     free(platform_vendor);
  }
  if( chosen_platform == NULL ) {
    fprintf(stderr,"Error LIG platform not found!\n");
    return -1;
  }

  printf("---\n");
  fflush(NULL);
  call_all_OpenCL_functions(chosen_platform);
  return 0;
}
