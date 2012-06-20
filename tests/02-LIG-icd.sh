#!/bin/bash

export OCL_ICD_DEBUG=15
export OCL_ICD_VENDORS=vendors

NAME="$(basename "$0")"

./ocl_test | \
	tee >(grep -sq "^Found 1 platforms!$" || touch "$NAME".error ) | \
	tee >(grep -sq "^LIG$" || touch "$NAME".error )

# wait for file creation if any
sleep 0.5

if test -f "$NAME".error ; then
  rm "$NAME".error
  exit 1
fi
