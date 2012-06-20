#!/bin/bash

export OCL_ICD_DEBUG=15
export OCL_ICD_VENDORS=unexisting-vendors-dir

NAME="$(basename "$0")"

./ocl_test | tee >(grep -sq "^No platforms found!$" || touch "$NAME".error )

# wait for file creation if any
sleep 0.5

if test -f "$NAME".error ; then
  rm "$NAME".error
  exit 1
fi
