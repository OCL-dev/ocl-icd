/**
Copyright (c) 2012-2020, Brice Videau <bvideau@anl.gov>
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

#pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wcpp"
#  define CL_USE_DEPRECATED_OPENCL_1_0_APIS
#  define CL_USE_DEPRECATED_OPENCL_1_1_APIS
#  define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#  define CL_USE_DEPRECATED_OPENCL_2_0_APIS
#  define CL_USE_DEPRECATED_OPENCL_2_1_APIS
#  define CL_USE_DEPRECATED_OPENCL_2_2_APIS
#  define CL_TARGET_OPENCL_VERSION 300
#  if defined(__APPLE__) || defined(__MACOSX)
#    include <OpenCL/cl.h>
#  else
#    include <CL/cl.h>
#  endif
#pragma GCC diagnostic pop

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
  CL_ICDL_OCL_VERSION=1,
  CL_ICDL_VERSION=2,
  CL_ICDL_NAME=3,
  CL_ICDL_VENDOR=4,
} cl_icdl_info;

typedef cl_int (*clGetICDLoaderInfoOCLICD_fn)(
  cl_icdl_info     param_name,
  size_t           param_value_size, 
  void *           param_value,
  size_t *         param_value_size_ret);

int main(int argc, char* argv[]) {
  cl_int error;

  clGetICDLoaderInfoOCLICD_fn clGetICDLoaderInfoOCLICD=NULL;
#pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wdeprecated-declarations"
  clGetICDLoaderInfoOCLICD = clGetExtensionFunctionAddress("clGetICDLoaderInfoOCLICD");
#pragma GCC diagnostic pop

  if (clGetICDLoaderInfoOCLICD == NULL) {
    printf("No clGetICDLoaderInfoOCLICD function available\n");
    return 2;
  }
  {
    char *param_value=NULL;
    size_t param_value_size_ret=0;

#define show(name,NAME) \
  do { \
    error = clGetICDLoaderInfoOCLICD(CL_ICDL_##NAME, 0, NULL, &param_value_size_ret ); \
    if (error) { exit(4); } \
    param_value = (char *)malloc(param_value_size_ret); \
    clGetICDLoaderInfoOCLICD(CL_ICDL_##NAME, param_value_size_ret, param_value, NULL); \
    printf(#name ": %s\n",param_value); \
    free(param_value); \
    param_value=NULL; \
  } while (0)

    show(ocl_version,OCL_VERSION);
    show(version,VERSION);
    show(name,NAME);
    show(vendor,VENDOR);
  }
  return 0;
}
