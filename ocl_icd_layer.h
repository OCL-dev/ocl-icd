#include "ocl_icd.h"

typedef cl_uint cl_layer_info;
typedef cl_uint cl_layer_api_version;
#define CL_LAYER_API_VERSION 0x4240
#define CL_LAYER_API_VERSION_100 100

extern CL_API_ENTRY cl_int CL_API_CALL
clGetLayerInfo(cl_layer_info  param_name,
               size_t         param_value_size,
               void          *param_value,
               size_t        *param_value_size_ret);

CL_API_ENTRY typedef cl_int
(CL_API_CALL *pfn_clGetLayerInfo)(cl_layer_info  param_name,
                                  size_t         param_value_size,
                                  void          *param_value,
                                  size_t        *param_value_size_ret);

extern CL_API_ENTRY cl_int CL_API_CALL
clInitLayer(cl_uint                         num_entries,
            const struct _cl_icd_dispatch  *target_dispatch,
            cl_uint                        *num_entries_ret,
            const struct _cl_icd_dispatch **layer_dispatch_ret);

CL_API_ENTRY typedef cl_int
(CL_API_CALL *pfn_clInitLayer)(cl_uint                         num_entries,
                               const struct _cl_icd_dispatch  *target_dispatch,
                               cl_uint                        *num_entries_ret,
                               const struct _cl_icd_dispatch **layer_dispatch_ret);


