#!/bin/bash

export OCL_ICD_DEBUG=15
export OCL_ICD_VENDORS=vendors

NAME="$(basename "$0")"

# clIcdGetPlatformIDsKHR is only for ICDs, not for ICD Loaders
# clSetPrintfCallback: CL 1.2, never found...
# clCreateEventFromGLsyncKHR: CL 1.1, never found...
./run_dummy_icd_through_our_ICDL | \
	tee >(grep "^-1 :" \
	| grep -v '^-1 : clSetPrintfCallback$' \
	| grep -v '^-1 : clIcdGetPlatformIDsKHR$' \
	| grep -v '^-1 : clCreateEventFromGLsyncKHR$' \
	> "$NAME".error )

# wait for file creation if any
sleep 0.5

if test -s "$NAME".error ; then
  echo 1>&2 "========================"
  echo 1>&2 "Functions that cannot be called:"
  cat 1>&2 "$NAME".error
  rm "$NAME".error
  exit 1
fi
rm "$NAME".error
