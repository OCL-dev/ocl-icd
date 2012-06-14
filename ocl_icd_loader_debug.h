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

#pragma GCC visibility push(hidden)

#ifndef DEBUG_OCL_ICD
#  define DEBUG_OCL_ICD 1
#endif

#define D_WARN 1
#define D_LOG 2
#define D_ARGS 4
#define D_DUMP 8
#if DEBUG_OCL_ICD
#  pragma GCC visibility push(default)
#  include <stdio.h>
#  pragma GCC visibility pop
extern int debug_ocl_icd_mask;
#  define debug(mask, fmt, ...) do {\
	if (debug_ocl_icd_mask & (mask)) {			\
		fprintf(stderr, "ocl-icd: %s: " fmt "\n", __func__, ##__VA_ARGS__); \
	} \
   } while(0)
#  define RETURN(val) do { \
	__typeof__(val) ret=(val); \
	debug(D_ARGS, "return: %ld/0x%lx", (long)ret, (long)ret);	\
	return ret; \
   } while(0)
#  define RETURN_STR(val) do { \
	char* ret=(char*)(val);			\
	debug(D_ARGS, "return: %s", ret);	\
	return ret; \
   } while(0)
void dump_platform(cl_platform_id pid);
#else
#  define debug(...) (void)0
#  define RETURN(val) return (val)
#  define RETURN_STR(val) return (val)
#endif

#pragma GCC visibility pop

#endif
