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

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include "config.h"
#ifdef USE_PTHREAD
#  include <pthread.h>
#endif
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
#pragma GCC diagnostic pop

#pragma GCC visibility push(hidden)

#include "ocl_icd_loader.h"
#define DEBUG_OCL_ICD_PROVIDE_DUMP_FIELD
#include "ocl_icd_debug.h"

int debug_ocl_icd_mask=0;

typedef __typeof__(clGetPlatformInfo) *clGetPlatformInfo_fn;

static inline void dump_vendor_icd(const char* info, const struct vendor_icd *v) {
  debug(D_DUMP, "%s %p={ num=%i, handle=%p, f=%p}\n", info,
	v, v->num_platforms, v->dl_handle, v->ext_fn_ptr);
}

struct vendor_icd *_icds=NULL;
struct platform_icd *_picds=NULL;
static cl_uint _num_icds = 0;
cl_uint _num_picds = 0;

#ifdef DEBUG_OCL_ICD
#  define _clS(x) [-x] = #x
#  define MAX_CL_ERRORS (-CL_INVALID_DEVICE_PARTITION_COUNT)
static char const * const clErrorStr[MAX_CL_ERRORS+1] = {
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
#ifdef DEBUG_OCL_ICD
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

static inline int _string_end_with_icd(const char* str) {
  size_t len = strlen(str);
  if( len<5 || strcmp(str + len - 4, ".icd" ) != 0 ) {
    return 0;
  }
  return 1;
}

static inline int _string_with_slash(const char* str) {
  return strchr(str, '/') != NULL;
}

static inline unsigned int _find_num_icds(DIR *dir) {
  unsigned int num_icds = 0;
  struct dirent *ent;
  while( (ent=readdir(dir)) != NULL ){
    if (_string_end_with_icd(ent->d_name)) {
      num_icds++;
    }
  }
  rewinddir(dir);
  RETURN(num_icds);
}

static inline unsigned int _load_icd(int num_icds, const char* lib_path) {
  unsigned int ret=0;
  debug(D_LOG, "Loading ICD '%s'", lib_path);

  _icds[num_icds].dl_handle = dlopen(lib_path, RTLD_LAZY|RTLD_LOCAL);//|RTLD_DEEPBIND);
  if(_icds[num_icds].dl_handle != NULL) {
    debug(D_LOG, "ICD[%i] loaded", num_icds);
    ret=1;
  } else {
    debug(D_WARN, "error while dlopening the IDL: '%s',\n  => skipping ICD", dlerror());
  }
  return ret;
}

static inline unsigned int _open_driver(unsigned int num_icds,
					const char*dir_path, const char*file_path) {
  char * lib_path;
  char * err;
  unsigned int lib_path_length;
  if (dir_path != NULL) {
    lib_path_length = strlen(dir_path) + strlen(file_path) + 2;
    lib_path = malloc(lib_path_length*sizeof(char));
    sprintf(lib_path,"%s/%s", dir_path, file_path);
  } else {
    lib_path_length = strlen(file_path) + 1;
    lib_path = malloc(lib_path_length*sizeof(char));
    sprintf(lib_path,"%s", file_path);
  }
  debug(D_LOG, "Considering file '%s'", lib_path);
  FILE *f = fopen(lib_path,"r");
  free(lib_path);
  if (f==NULL) {
    RETURN(num_icds);
  }

  fseek(f, 0, SEEK_END);
  lib_path_length = ftell(f)+1;
  fseek(f, 0, SEEK_SET);
  if(lib_path_length == 1) {
    debug(D_WARN, "File contents too short, skipping ICD");
    fclose(f);
    RETURN(num_icds);
  }
  lib_path = malloc(lib_path_length*sizeof(char));
  err = fgets(lib_path, lib_path_length, f);
  fclose(f);
  if( err == NULL ) {
    free(lib_path);
    debug(D_WARN, "Error while loading file contents, skipping ICD");
    RETURN(num_icds);
  }

  lib_path_length = strnlen(lib_path, lib_path_length);

  if( lib_path[lib_path_length-1] == '\n' )
    lib_path[lib_path_length-1] = '\0';

  num_icds += _load_icd(num_icds, lib_path);

  free(lib_path);
  RETURN(num_icds);
}

static inline unsigned int _open_drivers(DIR *dir, const char* dir_path) {
  unsigned int num_icds = 0;
  struct dirent *ent;
  while( (ent=readdir(dir)) != NULL ){
    if(! _string_end_with_icd(ent->d_name)) {
      continue;
    }
    num_icds = _open_driver(num_icds, dir_path, ent->d_name);

  }
  RETURN(num_icds);
}

static void* _get_function_addr(void* dlh, clGetExtensionFunctionAddress_fn fn, const char*name) {
  void *addr1;
  debug(D_LOG,"Looking for function %s",name);
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
#ifdef DEBUG_OCL_ICD
    if (addr1 && addr2 && addr1!=addr2) {
      debug(D_WARN, "Function and symbol '%s' have different addresses (%p != %p)!", name, addr2, addr1);
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

static void _count_devices(struct platform_icd *p) {
  cl_int error;

  /* Ensure they are 0 in case of errors */
  p->ngpus = p->ncpus = p->ndevs = 0;

  error = clGetDeviceIDs(p->pid, CL_DEVICE_TYPE_GPU, 0, NULL, &(p->ngpus));
  if (error != CL_SUCCESS && error != CL_DEVICE_NOT_FOUND){
    debug(D_WARN, "Error %s while counting GPU devices in platform %p",
	  _clerror2string(error), p->pid);
  }

  error = clGetDeviceIDs(p->pid, CL_DEVICE_TYPE_CPU, 0, NULL, &(p->ncpus));
  if (error != CL_SUCCESS && error != CL_DEVICE_NOT_FOUND){
    debug(D_WARN, "Error %s while counting CPU devices in platform %p",
	  _clerror2string(error), p->pid);
  }

  error = clGetDeviceIDs(p->pid, CL_DEVICE_TYPE_ALL, 0, NULL, &(p->ndevs));
  if (error != CL_SUCCESS && error != CL_DEVICE_NOT_FOUND){
    debug(D_WARN, "Error %s while counting ALL devices in platform %p",
	  _clerror2string(error), p->pid);
  }

}

static int _cmp_platforms(const void *_a, const void *_b) {
	const struct platform_icd *a=(const struct platform_icd *)_a;
	const struct platform_icd *b=(const struct platform_icd *)_b;

	/* sort first platforms handling max gpu */
	if (a->ngpus > b->ngpus) return -1;
	if (a->ngpus < b->ngpus) return 1;
	/* sort next platforms handling max cpu */
	if (a->ncpus > b->ncpus) return -1;
	if (a->ncpus < b->ncpus) return 1;
	/* sort then platforms handling max devices */
	if (a->ndevs > b->ndevs) return -1;
	if (a->ndevs < b->ndevs) return 1;
	/* else consider platforms equal */
	return 0;
}

static void _sort_platforms(struct platform_icd *picds, int npicds) {
	debug(D_WARN, "Nb platefroms: %i", npicds);
	if (npicds > 1) {
		char* ocl_sort=getenv("OCL_ICD_PLATFORM_SORT");
		if (ocl_sort!=NULL && !strcmp(ocl_sort, "none")) {
			debug(D_LOG, "Platform not sorted");
		} else {
			if (ocl_sort!=NULL && strcmp(ocl_sort, "devices")) {
				debug(D_WARN, "Unknown platform sort algorithm requested: %s", ocl_sort);
				debug(D_WARN, "Switching do the 'devices' algorithm");
			}
			int i;
			debug(D_LOG, "Platform sorted by GPU, CPU, DEV");
			for (i=0; i<npicds; i++) {
				_count_devices(&picds[i]);
			}
			qsort(picds, npicds, sizeof(*picds),
			      &_cmp_platforms);
		}
	}
}

#define ASSUME_ICD_EXTENSION_UNKNOWN ((int)-1)
#define ASSUME_ICD_EXTENSION_NO ((int)0)
#define ASSUME_ICD_EXTENSION_YES ((int)1)
#define ASSUME_ICD_EXTENSION_YES_AND_QUIET ((int)2)


static int _assume_ICD_extension() {
	static int val=ASSUME_ICD_EXTENSION_UNKNOWN;
	if (val == ASSUME_ICD_EXTENSION_UNKNOWN) {
		const char* str=getenv("OCL_ICD_ASSUME_ICD_EXTENSION");
		if (! str || str[0]==0) {
			val=ASSUME_ICD_EXTENSION_NO;
		} else if (strcmp(str, "debug")==0) {
			val=ASSUME_ICD_EXTENSION_YES;
		} else {
			val=ASSUME_ICD_EXTENSION_YES_AND_QUIET;
		}
	}
	return val;
}

static inline void _find_and_check_platforms(cl_uint num_icds) {
  cl_uint i;
  _num_icds = 0;
  for( i=0; i<num_icds; i++){
    debug(D_LOG, "Checking ICD %i/%i", i, num_icds);
    dump_vendor_icd("before looking for platforms", &_icds[i]);
    struct vendor_icd *picd = &_icds[i];
    void* dlh = _icds[i].dl_handle;
    picd->ext_fn_ptr = _get_function_addr(dlh, NULL, "clGetExtensionFunctionAddress");
    clIcdGetPlatformIDsKHR_fn plt_fn_ptr =
      _get_function_addr(dlh, picd->ext_fn_ptr, "clIcdGetPlatformIDsKHR");
    if( picd->ext_fn_ptr == NULL
       || plt_fn_ptr == NULL) {
      debug(D_WARN, "Missing symbols in ICD, skipping it");
      continue;
    }
    clGetPlatformInfo_fn plt_info_ptr =
      _get_function_addr(dlh, picd->ext_fn_ptr,	"clGetPlatformInfo");
    if (plt_info_ptr == NULL) {
	    switch (_assume_ICD_extension()) {
	    case ASSUME_ICD_EXTENSION_NO:
		    debug(D_WARN, "Missing 'clGetPlatformInfo' symbol in ICD, skipping it (use OCL_ICD_ASSUME_ICD_EXTENSION to ignore this check)");
		    continue;
	    case ASSUME_ICD_EXTENSION_YES:
		    debug(D_LOG, "Missing 'clGetPlatformInfo' symbol in ICD, but still continuing due to OCL_ICD_ASSUME_ICD_EXTENSION");
		    /* Fall through */
	    case ASSUME_ICD_EXTENSION_YES_AND_QUIET:
		    /* Assuming an ICD extension, so we will try to
		     * find the ICD specific version of
		     * clGetPlatformInfo before knowing for sure that
		     * the cl_khr_icd is really present */
		    break;
	    default:
		    debug(D_ALWAYS, "Internal error in _assume_ICD_extension, please report");
		    break;
	    }
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
    debug(D_LOG, "Try to load %d platforms", num_platforms);
    if (_allocate_platforms(num_platforms) < num_platforms) {
      free(platforms);
      debug(D_WARN, "Not enough platform allocated. Skipping ICD");
      continue;
    }
    for(j=0; j<num_platforms; j++) {
      debug(D_LOG, "Checking platform %i", j);
      struct platform_icd *p=&_picds[_num_picds];
      char *param_value=NULL;
      p->extension_suffix=NULL;
      p->vicd=&_icds[i];
      p->pid=platforms[j];

      /* If clGetPlatformInfo is not exported and we are here, it
       * means that OCL_ICD_ASSUME_ICD_EXTENSION. Si we try to take it
       * from the dispatch * table. If that fails too, we have to
       * bail.
       */
      if (plt_info_ptr == NULL) {
        plt_info_ptr = p->pid->dispatch->clGetPlatformInfo;
        if (plt_info_ptr == NULL) {
          debug(D_WARN, "Missing clGetPlatformInfo even in ICD dispatch table, skipping it");
          continue;
        }
      }

#ifdef DEBUG_OCL_ICD
      if (debug_ocl_icd_mask & D_DUMP) {
        int log=debug_ocl_icd_mask & D_TRACE;
        debug_ocl_icd_mask &= ~D_TRACE;
	dump_platform(p->vicd->ext_fn_ptr, p->pid);
        debug_ocl_icd_mask |= log;
      }
#endif
      {
	      switch (_assume_ICD_extension()) {
	      case ASSUME_ICD_EXTENSION_NO:
		      /* checking the extension as required by the specs */
		      param_value=_malloc_clGetPlatformInfo(plt_info_ptr, p->pid, CL_PLATFORM_EXTENSIONS, "extensions");
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
		      break;
	      case ASSUME_ICD_EXTENSION_YES:
		      /* Allow to workaround a bug in the Intel ICD used
		       * with optirun :
		       * - https://software.intel.com/en-us/forums/opencl/topic/328091
		       * - https://sourceforge.net/p/virtualgl/bugs/54/
		       */
		      debug(D_LOG, "Assuming cl_khr_icd extension without checking for it");
		      /* Fall through */
	      case ASSUME_ICD_EXTENSION_YES_AND_QUIET:
		      /* Assuming an ICD extension, so we will try to
		       * find the ICD specific version of
		       * clGetPlatformInfo before knowing for sure that
		       * the cl_khr_icd is really present */
		      break;
	      default:
		      debug(D_ALWAYS, "Internal error in _assume_ICD_extension, please report");
		      break;
	      }
      }
      param_value=_malloc_clGetPlatformInfo(plt_info_ptr, p->pid, CL_PLATFORM_ICD_SUFFIX_KHR, "suffix");
      if (param_value == NULL){
	debug(D_WARN, "Skipping platform %i", j);
        continue;
      }
      p->extension_suffix = param_value;
      debug(D_DUMP|D_LOG, "Extension suffix: %s", param_value);
#ifdef DEBUG_OCL_ICD
      param_value=_malloc_clGetPlatformInfo(plt_info_ptr, p->pid, CL_PLATFORM_PROFILE, "profile");
      if (param_value != NULL){
        debug(D_DUMP, "Profile: %s", param_value);
	free(param_value);
      }
      param_value=_malloc_clGetPlatformInfo(plt_info_ptr, p->pid, CL_PLATFORM_VERSION, "version");
      p->version = param_value;
      if (param_value != NULL){
        debug(D_DUMP, "Version: %s", param_value);
	free(param_value);
      }
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
      dump_vendor_icd("after looking for platforms", &_icds[_num_icds]);
      _num_icds++;
      picd->num_platforms = num_valid_platforms;
    } else {
      dlclose(dlh);
    }
    free(platforms);
  }
  _sort_platforms(&_picds[0], _num_picds);
}

static void __initClIcd( void ) {
  debug_init();
  cl_uint num_icds = 0;
  int is_dir = 0;
  DIR *dir = NULL;
  const char* dir_path=getenv("OCL_ICD_VENDORS");
  const char* vendor_path=getenv("OPENCL_VENDOR_PATH");
  if (! vendor_path || vendor_path[0]==0) {
    vendor_path=ETC_OPENCL_VENDORS;
    debug(D_DUMP, "OPENCL_VENDOR_PATH unset or empty. Using hard-coded path '%s'", vendor_path);
  } else {
    debug(D_DUMP, "OPENCL_VENDOR_PATH set to '%s', using it", vendor_path);
  }
  if (! dir_path || dir_path[0]==0) {
    dir_path=vendor_path;
    debug(D_DUMP, "OCL_ICD_VENDORS empty or not defined, using vendors path '%s'", dir_path);
    is_dir=1;
  }
  if (!is_dir) {
    struct stat buf;
    int ret=stat(dir_path, &buf);
    if (ret != 0 && errno != ENOENT) {
      debug(D_WARN, "Cannot stat '%s'. Aborting", dir_path);
    }
    if (ret == 0 && S_ISDIR(buf.st_mode)) {
      is_dir=1;
    }
  }

  if (!is_dir) {
    debug(D_LOG,"Only loading '%s' as an ICD", dir_path);
    num_icds = 1;
    dir=NULL;
  } else {
    debug(D_LOG,"Reading icd list from '%s'", dir_path);
    dir = opendir(dir_path);
    if(dir == NULL) {
      if (errno == ENOTDIR) {
        debug(D_DUMP, "%s is not a directory, trying to use it as a ICD libname",
	  dir_path);
      }
      goto abort;
    }

    num_icds = _find_num_icds(dir);
    if(num_icds == 0) {
      goto abort;
    }
  }

  _icds = (struct vendor_icd*)malloc(num_icds * sizeof(struct vendor_icd));
  if (_icds == NULL) {
    goto abort;
  }

  if (!is_dir) {
    if (_string_end_with_icd(dir_path)) {
      num_icds = 0;
      if (! _string_with_slash(dir_path)) {
	num_icds = _open_driver(0, vendor_path, dir_path);
      }
      if (num_icds == 0) {
	num_icds = _open_driver(0, NULL, dir_path);
      }
    } else {
      num_icds = _load_icd(0, dir_path);
    }
  } else {
    num_icds = _open_drivers(dir, dir_path);
  }
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

  if (dir != NULL){
    closedir(dir);
  }
  return;
 abort:
  _num_icds = 0;
  if (_icds) {
    free(_icds);
    _icds = NULL;
  }
  if (dir != NULL){
    closedir(dir);
  }
  return;
}

#ifdef USE_PTHREAD
static pthread_once_t once_init = PTHREAD_ONCE_INIT;
#else
static int gard=0;
#endif
volatile static __thread int in_init = 0;
volatile static cl_uint _initialized = 0;

static void _initClIcd_real( void ) {
#ifdef USE_PTHREAD
  if (in_init) {
    /* probably reentrency, in_init is a __thread variable */
    debug(D_WARN, "Executing init while already in init!");
  } else {
    in_init=1;
    __sync_synchronize();
    pthread_once(&once_init, &__initClIcd);
    __sync_synchronize();
    in_init=0;
  }
#else
  if (__sync_bool_compare_and_swap(&gard, 0, 1)) {
    in_init=1;
    __sync_synchronize();
    __initClIcd();
    __sync_synchronize();
    in_init=0;
  } else {
    if (in_init) {
      /* probably reentrency (could also be preemptive user-level threads). */
    } else {
      /* someone else started __initClIcd(). We wait until it ends. */
      debug(D_WARN, "Waiting end of init");
      while (!_initialized) {
	__sync_synchronize();
      }
      debug(D_WARN, "Wait done");
   }
  }
#endif
  _initialized = 1;
}

static inline void _initClIcd( void ) {
  if( __builtin_expect (_initialized, 1) )
    return;
  _initClIcd_real();
}

cl_platform_id __attribute__((visibility("internal")))
getDefaultPlatformID() {
  static cl_platform_id defaultPlatformID=NULL;
  static int defaultSet=0;
  _initClIcd();
  if (! defaultSet) {
    do {
      if(_num_picds == 0) {
	break;
      }
      const char *default_platform = getenv("OCL_ICD_DEFAULT_PLATFORM");
      int num_default_platform;
      char *end_scan;
      if (! default_platform) {
	num_default_platform = 0;
      } else {
	num_default_platform = strtol(default_platform, &end_scan, 10);
	if (*default_platform == '\0' || *end_scan != '\0') {
	  break;
	}
      }
      if (num_default_platform < 0 || num_default_platform >= _num_picds) {
	break;
      }
      defaultPlatformID=_picds[num_default_platform].pid;
    } while(0);
    defaultSet=1;
  }
  return defaultPlatformID;
}

#pragma GCC visibility pop
#define hidden_alias(name) \
  typeof(name) name##_hid __attribute__ ((alias (#name), visibility("hidden")))

typedef enum {
  CL_ICDL_OCL_VERSION=1,
  CL_ICDL_VERSION=2,
  CL_ICDL_NAME=3,
  CL_ICDL_VENDOR=4,
} cl_icdl_info;

static cl_int clGetICDLoaderInfoOCLICD(
  cl_icdl_info     param_name,
  size_t           param_value_size,
  void *           param_value,
  size_t *         param_value_size_ret)
{
  static const char cl_icdl_ocl_version[] = "OpenCL " OCL_ICD_OPENCL_VERSION;
  static const char cl_icdl_version[] = PACKAGE_VERSION;
  static const char cl_icdl_name[] = PACKAGE_NAME;
  static const char cl_icdl_vendor[] = "OCL Icd free software";

  size_t size_string;
  const char * string_p;
#define oclcase(name, NAME) \
  case CL_ICDL_##NAME: \
    string_p = cl_icdl_##name; \
    size_string = sizeof(cl_icdl_##name); \
    break

  switch ( param_name ) {
    oclcase(ocl_version,OCL_VERSION);
    oclcase(version,VERSION);
    oclcase(name,NAME);
    oclcase(vendor,VENDOR);
    default:
      return CL_INVALID_VALUE;
      break;
  }
#undef oclcase
  if( param_value != NULL ) {
    if( size_string > param_value_size )
      return CL_INVALID_VALUE;
    memcpy(param_value, string_p, size_string);
  }
  if( param_value_size_ret != NULL )
    *param_value_size_ret = size_string;
  return CL_SUCCESS;
}

CL_API_ENTRY void * CL_API_CALL
clGetExtensionFunctionAddress(const char * func_name) CL_API_SUFFIX__VERSION_1_0 {
  debug_trace();
  _initClIcd();
  if( func_name == NULL )
    return NULL;
  cl_uint suffix_length;
  cl_uint i;
  void * return_value=NULL;
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
  for(i=0; i<_num_picds; i++) {
    suffix_length = strlen(_picds[i].extension_suffix);
    if( suffix_length > strlen(func_name) )
      continue;
    if(strcmp(_picds[i].extension_suffix, &func_name[strlen(func_name)-suffix_length]) == 0)
      RETURN((*_picds[i].vicd->ext_fn_ptr)(func_name));
  }
  if(strcmp(func_name, "clGetICDLoaderInfoOCLICD") == 0) {
    return (void*)(void*(*)(void))(&clGetICDLoaderInfoOCLICD);
  }
  RETURN(return_value);
}
hidden_alias(clGetExtensionFunctionAddress);

CL_API_ENTRY cl_int CL_API_CALL
clGetPlatformIDs(cl_uint          num_entries,
                 cl_platform_id * platforms,
                 cl_uint *        num_platforms) CL_API_SUFFIX__VERSION_1_0 {
  debug_trace();
  _initClIcd();
  if( platforms == NULL && num_platforms == NULL )
    RETURN(CL_INVALID_VALUE);
  if( num_entries == 0 && platforms != NULL )
    RETURN(CL_INVALID_VALUE);
  if( _num_icds == 0 || _num_picds == 0 ) {
    if ( num_platforms != NULL )
      *num_platforms = 0;
    RETURN(CL_PLATFORM_NOT_FOUND_KHR);
  }

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
hidden_alias(clGetPlatformIDs);

#define RETURN_WITH_ERRCODE(errvar, errvalue, retvalue) \
  do { \
    if(errvar) { \
      *errvar=errvalue; \
    } \
    RETURN(NULL); \
  } while(0)

#define CHECK_PLATFORM(__pid) \
  ({ \
    cl_platform_id _pid=(__pid); \
    int good=0; \
    cl_uint j; \
    for( j=0; j<_num_picds; j++) { \
      if( _picds[j].pid == _pid) { \
        good=1; \
        break; \
      } \
    } \
    good; \
  })

CL_API_ENTRY cl_context CL_API_CALL
clCreateContext(const cl_context_properties *  properties ,
                cl_uint                        num_devices ,
                const cl_device_id *           devices ,
                void (CL_CALLBACK *  pfn_notify )(const char *, const void *, size_t, void *),
                void *                         user_data ,
                cl_int *                       errcode_ret ){
  debug_trace();
  _initClIcd();
  cl_uint i=0;
  if( properties != NULL){
    while( properties[i] != 0 ) {
      if( properties[i] == CL_CONTEXT_PLATFORM ) {
        if((struct _cl_platform_id *) properties[i+1] == NULL) {
          if(errcode_ret) {
            *errcode_ret = CL_INVALID_PLATFORM;
          }
          RETURN(NULL);
        } else {
          if( !CHECK_PLATFORM((cl_platform_id) properties[i+1]) ) {
	    RETURN_WITH_ERRCODE(errcode_ret, CL_INVALID_PLATFORM, NULL);
          }
        }
        RETURN(((struct _cl_platform_id *) properties[i+1])
          ->dispatch->clCreateContext(properties, num_devices, devices,
                        pfn_notify, user_data, errcode_ret));
      }
      i += 2;
    }
  }
  if(devices == NULL || num_devices == 0) {
    RETURN_WITH_ERRCODE(errcode_ret, CL_INVALID_VALUE, NULL);
  }
  if((struct _cl_device_id *)devices[0] == NULL) {
    RETURN_WITH_ERRCODE(errcode_ret, CL_INVALID_DEVICE, NULL);
  }
  RETURN(((struct _cl_device_id *)devices[0])
    ->dispatch->clCreateContext(properties, num_devices, devices,
                  pfn_notify, user_data, errcode_ret));
}
hidden_alias(clCreateContext);

CL_API_ENTRY cl_context CL_API_CALL
clCreateContextFromType(const cl_context_properties *  properties ,
                        cl_device_type                 device_type ,
                        void (CL_CALLBACK *      pfn_notify )(const char *, const void *, size_t, void *),
                        void *                         user_data ,
                        cl_int *                       errcode_ret ){
  debug_trace();
  _initClIcd();
  if(_num_picds == 0) {
    goto out;
  }
  cl_uint i=0;
  if( properties != NULL){
    while( properties[i] != 0 ) {
      if( properties[i] == CL_CONTEXT_PLATFORM ) {
	if( (struct _cl_platform_id *) properties[i+1] == NULL ) {
	  goto out;
        } else {
          if( !CHECK_PLATFORM((cl_platform_id) properties[i+1]) ) {
            goto out;
          }
        }
        return ((struct _cl_platform_id *) properties[i+1])
          ->dispatch->clCreateContextFromType(properties, device_type,
                        pfn_notify, user_data, errcode_ret);
      }
      i += 2;
    }
  } else {
    cl_platform_id default_platform=getDefaultPlatformID();
    RETURN(default_platform->dispatch->clCreateContextFromType
	(properties, device_type, pfn_notify, user_data, errcode_ret));
  }
 out:
  RETURN_WITH_ERRCODE(errcode_ret, CL_INVALID_PLATFORM, NULL);
}
hidden_alias(clCreateContextFromType);

CL_API_ENTRY cl_int CL_API_CALL
clGetGLContextInfoKHR(const cl_context_properties *  properties ,
                      cl_gl_context_info             param_name ,
                      size_t                         param_value_size ,
                      void *                         param_value ,
                      size_t *                       param_value_size_ret ){
  debug_trace();
  _initClIcd();
  cl_uint i=0;
  if( properties != NULL){
    while( properties[i] != 0 ) {
      if( properties[i] == CL_CONTEXT_PLATFORM ) {
        if( (struct _cl_platform_id *) properties[i+1] == NULL ) {
	  RETURN(CL_INVALID_PLATFORM);
        } else {
          if( !CHECK_PLATFORM((cl_platform_id) properties[i+1]) ) {
	    RETURN(CL_INVALID_PLATFORM);
          }
        }
        RETURN(((struct _cl_platform_id *) properties[i+1])
	  ->dispatch->clGetGLContextInfoKHR(properties, param_name,
                        param_value_size, param_value, param_value_size_ret));
      }
      i += 2;
    }
  }
  RETURN(CL_INVALID_PLATFORM);
}
hidden_alias(clGetGLContextInfoKHR);

CL_API_ENTRY cl_int CL_API_CALL
clWaitForEvents(cl_uint              num_events ,
                const cl_event *     event_list ){
  debug_trace();
  if( num_events == 0 || event_list == NULL )
    RETURN(CL_INVALID_VALUE);
  if( (struct _cl_event *)event_list[0] == NULL )
    RETURN(CL_INVALID_EVENT);
  RETURN(((struct _cl_event *)event_list[0])
    ->dispatch->clWaitForEvents(num_events, event_list));
}
hidden_alias(clWaitForEvents);

CL_API_ENTRY cl_int CL_API_CALL
clUnloadCompiler( void ){
  debug_trace();
  RETURN(CL_SUCCESS);
}
hidden_alias(clUnloadCompiler);
