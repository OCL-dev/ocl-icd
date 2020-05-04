/**
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
#ifndef OCL_ICD_LOADER_DEBUG_H
#define OCL_ICD_LOADER_DEBUG_H

#ifdef DEBUG_OCL_ICD_PROVIDE_DUMP_FIELD
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wcpp"
#  define CL_USE_DEPRECATED_OPENCL_1_0_APIS
#  define CL_USE_DEPRECATED_OPENCL_1_1_APIS
#  define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#  define CL_USE_DEPRECATED_OPENCL_2_0_APIS
#  define CL_USE_DEPRECATED_OPENCL_2_1_APIS
#  define CL_USE_DEPRECATED_OPENCL_2_2_APIS
#  define CL_TARGET_OPENCL_VERSION 300
#else
#  define CL_TARGET_OPENCL_VERSION 300
#endif
#include <CL/cl.h>
#ifdef DEBUG_OCL_ICD_PROVIDE_DUMP_FIELD
#  pragma GCC diagnostic pop
#endif

#pragma GCC visibility push(hidden)

#include "config.h"

#define D_ALWAYS 0
#define D_WARN 1
#define D_LOG 2
#define D_TRACE 4
#define D_DUMP 8
#ifdef DEBUG_OCL_ICD
#  pragma GCC visibility push(default)
#  include <stdio.h>
#  include <stdlib.h>
#  pragma GCC visibility pop
extern int debug_ocl_icd_mask;
#  define debug(mask, fmt, ...) do {\
	if (((mask)==D_ALWAYS) || (debug_ocl_icd_mask & (mask))) {			\
		fprintf(stderr, "ocl-icd(%s:%i): %s: " fmt "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
	} \
   } while(0)
#  define RETURN(val) do { \
	__typeof__(val) ret=(val); \
	debug(D_TRACE, "return: %ld/0x%lx", (long)ret, (long)ret);	\
	return ret; \
   } while(0)
#  define RETURN_STR(val) do { \
	char* _ret=(char*)(val);		\
	debug(D_TRACE, "return: %s", _ret);	\
	return _ret; \
   } while(0)
#  ifdef DEBUG_OCL_ICD_PROVIDE_DUMP_FIELD
#    pragma GCC diagnostic push
#      pragma GCC diagnostic ignored "-Wdeprecated-declarations"
typedef __typeof__(clGetExtensionFunctionAddress) *clGEFA_t;
#    pragma GCC diagnostic pop
void dump_platform(clGEFA_t f, cl_platform_id pid);
#  endif
static inline void debug_init(void) {
  static int done=0;
  if (!done) {
    char *debug=getenv("OCL_ICD_DEBUG");
    if (debug) {
      debug_ocl_icd_mask=atoi(debug);
      if (*debug == 0)
        debug_ocl_icd_mask=1;
    }
    done=1;
  }
}

#  define dump_field(pid, f, name) \
    debug(D_ALWAYS, "%40s=%p [%p/%p]", #name, pid->dispatch->name, f(#name), ((long)(pid->dispatch->clGetExtensionFunctionAddressForPlatform)>10)?pid->dispatch->clGetExtensionFunctionAddressForPlatform(pid, #name):NULL)

#else
#  define debug(...) (void)0
#  define RETURN(val) return (val)
#  define RETURN_STR(val) return (val)
#  define debug_init() (void)0
#endif

#define debug_trace() debug(D_TRACE, "Entering")

#pragma GCC visibility pop

#endif
