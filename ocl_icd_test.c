#include <CL/opencl.h>
#include <stdio.h>

int main(void) {
  cl_uint num_entries;
  cl_platform_id *platforms;
  cl_uint num_platforms;
  cl_int error;

  error = clGetPlatformIDs(0, NULL, &num_platforms);
  if( error == CL_SUCCESS )
    printf("Found %u platforms!\n", num_platforms);
  else
    exit(-1);
  platforms = (cl_platform_id *)malloc(sizeof(cl_platform_id *) * num_platforms);
  error = clGetPlatformIDs(num_platforms, platforms, NULL);
  cl_uint i;
  for(i=0; i<num_platforms; i++){
    char *platform_vendor;
    size_t param_value_size_ret;

    error = clGetPlatformInfo(platforms[i], CL_PLATFORM_VENDOR, 0, NULL, &param_value_size_ret );
    
    platform_vendor = (char *)malloc(param_value_size_ret);
    clGetPlatformInfo(platforms[i], CL_PLATFORM_VENDOR, param_value_size_ret, platform_vendor, NULL );

    printf("%s\n",platform_vendor);
    free(platform_vendor);
  }
}
