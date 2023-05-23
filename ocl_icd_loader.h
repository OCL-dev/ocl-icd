/**
Copyright (c) 2020, Brice Videau <bvideau@anl.gov>
Copyright (c) 2013, Vincent Danjean <Vincent.Danjean@ens-lyon.org>
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

#ifndef __OCL_ICD_LOADER_H
#define __OCL_ICD_LOADER_H

#include "ocl_icd.h"
#include "ocl_icd_loader_gen.h"

cl_platform_id __attribute__((visibility("internal")))
getDefaultPlatformID();

void __attribute__((visibility("internal")))
_initClIcd_no_inline(void);

static inline
cl_platform_id selectPlatformID(cl_platform_id pid) {
  if (pid) return pid;
  return getDefaultPlatformID();
}

typedef cl_uint cl_layer_info;
typedef cl_uint cl_layer_api_version;
#define CL_LAYER_API_VERSION 0x4240
#define CL_LAYER_NAME        0x4241
#define CL_LAYER_API_VERSION_100 100

typedef __typeof__(clGetPlatformInfo) *clGetPlatformInfo_fn;
typedef cl_int (CL_API_CALL *clGetLayerInfo_fn)(
    cl_layer_info  param_name,
    size_t         param_value_size,
    void          *param_value,
    size_t        *param_value_size_ret);

typedef cl_int (CL_API_CALL *clInitLayer_fn)(
    cl_uint                         num_entries,
    const struct _cl_icd_dispatch  *target_dispatch,
    cl_uint                        *num_entries_out,
    const struct _cl_icd_dispatch **layer_dispatch);

struct layer_icd;
struct layer_icd {
  void                    *dl_handle;
  struct _cl_icd_dispatch  dispatch;
  struct layer_icd        *next_layer;
#ifdef CLLAYERINFO
  char                    *library_name;
  void                    *layer_info_fn_ptr;
#endif
};

__attribute__((visibility("hidden"))) extern struct layer_icd *_first_layer;

#endif
