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

#define CL_TARGET_OPENCL_VERSION 300
#include <CL/opencl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void print_error(cl_int error) {
  switch (error) {
  case CL_SUCCESS: break ;
  case CL_PLATFORM_NOT_FOUND_KHR:
    printf("No platforms found!\n");
    break;
  case CL_INVALID_PLATFORM:
    printf("Invalid platform\n");
    break;
  default:
     printf("OpenCL error: %i\n", error);
  }
}

void show_platform(cl_platform_id pid, int show_extensions) {
  char *platform_vendor;
  size_t param_value_size_ret;
  cl_int error;

  error = clGetPlatformInfo(pid, CL_PLATFORM_VENDOR, 0, NULL, &param_value_size_ret );
  print_error(error);
  if (error != CL_SUCCESS)
    return;
    
  platform_vendor = (char *)malloc(param_value_size_ret);
  clGetPlatformInfo(pid, CL_PLATFORM_VENDOR, param_value_size_ret, platform_vendor, NULL );

  printf("%s\n",platform_vendor);
  free(platform_vendor);

  if (show_extensions) {  
    error = clGetPlatformInfo(pid, CL_PLATFORM_EXTENSIONS, 0, NULL, &param_value_size_ret );
    print_error(error);
    
    platform_vendor = (char *)malloc(param_value_size_ret);
    clGetPlatformInfo(pid, CL_PLATFORM_EXTENSIONS, param_value_size_ret, platform_vendor, NULL );

    printf("Extensions: %s\n",platform_vendor);
    free(platform_vendor);
  }
}

int main(int argc, char* argv[]) {
  cl_platform_id *platforms;
  cl_uint num_platforms;
  cl_int error;

  int show_extensions=0;
  if (argc >= 2 && strcmp(argv[1], "--show-extensions")==0) {
    show_extensions=1;
    argv++;
  }

  int default_platform=0;
  if (argc >= 2 && strcmp(argv[1], "--default-platform")==0) {
    default_platform=1;
  }

  error = clGetPlatformIDs(0, NULL, &num_platforms);
  if( error == CL_SUCCESS ) {
    printf("Found %u platforms!\n", num_platforms);
  } else if( error == CL_PLATFORM_NOT_FOUND_KHR ) {
    print_error(error);
    if (default_platform) {
      show_platform(NULL, show_extensions);
    }
    exit(0);
  } else {
    print_error(error);
    exit(-1);
  }
  platforms = (cl_platform_id *)malloc(sizeof(cl_platform_id *) * num_platforms);
  error = clGetPlatformIDs(num_platforms, platforms, NULL);
  print_error(error);
  cl_uint i;
  for(i=0; i<num_platforms; i++){
    show_platform(platforms[i], show_extensions);
  }
  if (default_platform) {
    show_platform(NULL, show_extensions);
  }
  return 0;
}
