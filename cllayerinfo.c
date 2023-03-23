#include "ocl_icd_loader.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

int stdout_bak, stderr_bak;

static inline int
silence_fd(FILE *file, int fd)
{
  int new_fd, fd_bak;
  fflush(file);
  fd_bak = dup(fd);
  new_fd = open("/dev/null", O_WRONLY);
  dup2(new_fd, fd);
  close(new_fd);
  return fd_bak;
}

static inline void
restore_fd(FILE *file, int fd, int fd_bak)
{
  fflush(file);
  dup2(fd_bak, fd);
  close(fd_bak);
}

static void silence_outputs(void)
{
  stdout_bak = silence_fd(stdout, 1);
  stderr_bak = silence_fd(stderr, 2);
}

static void restore_outputs(void)
{
  restore_fd(stdout, 1, stdout_bak);
  restore_fd(stderr, 2, stderr_bak);
}

void mute(void) {
  silence_outputs();
  atexit(restore_outputs);
}

void unmute(void) {
  restore_outputs();
  atexit(silence_outputs);
}

void print_layer_info(const struct layer_icd *layer)
{
  cl_layer_api_version api_version = 0;
  clGetLayerInfo_fn layer_info_fn_ptr = (clGetLayerInfo_fn)layer->layer_info_fn_ptr;
  cl_int error = CL_SUCCESS;
  size_t sz;

  printf("%s:\n", layer->library_name);
  error = layer_info_fn_ptr(CL_LAYER_API_VERSION, sizeof(api_version), &api_version, NULL);
  if (error == CL_SUCCESS)
    printf("\tCL_LAYER_API_VERSION: %d\n", (int)api_version);

  error = layer_info_fn_ptr(CL_LAYER_NAME, 0, NULL, &sz);
  if (error == CL_SUCCESS)
  {
    char *name = (char *)malloc(sz);
    if (name)
    {
      error = layer_info_fn_ptr(CL_LAYER_NAME, sz, name, NULL);
      if (error == CL_SUCCESS)
        printf("\tCL_LAYER_NAME: %s\n", name);
      free(name);
    }
  }
}

int main (int argc, char *argv[])
{
  (void)argc;
  (void)argv;
  mute();
  _initClIcd_no_inline();
  unmute();
  const struct layer_icd *layer = _first_layer;
  while (layer)
  {
    print_layer_info(layer);
    layer = layer->next_layer;
  }
  return 0;
}
