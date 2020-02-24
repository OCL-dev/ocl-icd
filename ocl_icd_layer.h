#include "ocl_icd.h"

static struct _cl_icd_dispatch dispatch = {NULL};

static const struct _cl_icd_dispatch *tdispatch;

CL_API_ENTRY cl_uint CL_API_CALL
clLayerVersion(void);

CL_API_ENTRY cl_int CL_API_CALL
clInitLayer(
    const struct _cl_icd_dispatch  *target_dispatch,
    cl_uint                         num_entries,
    const struct _cl_icd_dispatch **layer_dispatch,
    cl_uint                        *num_entries_out);

