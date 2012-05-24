all: library partial-clean

library: test_tools install
	ruby icd_generator.rb --finalize
	gcc -g -c ocl_icd.c -o ocl_icd.o -Wno-cpp -Wno-deprecated-declarations -fpic
	gcc -g -c ocl_icd_lib.c -o ocl_icd_lib.o -Wno-cpp -Wno-deprecated-declarations -fpic
	gcc -g -fpic -shared -Wl,-Bsymbolic -Wl,-soname,liOpenCL.so -o libOpenCL.so.1.0 ocl_icd.o ocl_icd_lib.o
	gcc -g -c ocl_icd_test.c -o ocl_icd_test.o 
	gcc -g ocl_icd_test.o -lOpenCL -o ocl_icd_test 

test_tools: libdummycl.so.1.0 ocl_icd_dummy_test

libdummycl.so.1.0: ocl_icd_dummy.o
	gcc -g -fpic -shared -Wl,-Bsymbolic -Wl,-soname,libdummycl.so.1 -o libdummycl.so.1.0 ocl_icd_dummy.o

ocl_icd_dummy_test: ocl_icd_dummy_test.o
	gcc -g -o ocl_icd_dummy_test ocl_icd_dummy_test.o -lOpenCL -lpthread

ocl_icd_dummy_test.o: generator
	gcc -g -Wall -Wno-cpp -Wno-deprecated-declarations -c ocl_icd_dummy_test.c -o ocl_icd_dummy_test.o

ocl_icd_dummy.o: generator
	gcc -g -Wall -fpic -c ocl_icd_dummy.c -o ocl_icd_dummy.o

generator: icd_generator.rb
	ruby icd_generator.rb --generate


install: libdummycl.so.1.0
	cp libdummycl.so.1.0 /usr/local/lib/
	ln -sf /usr/local/lib/libdummycl.so.1.0 /usr/local/lib/libdummycl.so
	ln -sf /usr/local/lib/libdummycl.so.1.0 /usr/local/lib/libdummycl.so.1
	echo "/usr/local/lib/libdummycl.so" > /etc/OpenCL/vendors/dummycl.icd
	ldconfig

uninstall:
	rm -f /usr/local/lib/libdummycl.so /usr/local/lib/libdummycl.so.1 /etc/OpenCL/vendors/dummycl.icd

clean:
	rm -f ocl_icd_dummy_test.o ocl_icd_dummy_test.c ocl_icd_dummy.o ocl_icd_dummy.c ocl_icd_dummy.h ocl_icd_bindings.c ocl_icd.h libdummycl.so.1.0 ocl_icd_lib.c ocl_icd_lib.o ocl_icd.o ocl_icd_test ocl_icd_dummy_test ocl_icd_test.o libOpenCL.so.1.0

partial-clean:
	rm -f ocl_icd_dummy_test.o ocl_icd_dummy_test.c ocl_icd_dummy.o ocl_icd_dummy.c ocl_icd_dummy.h libdummycl.so.1.0
