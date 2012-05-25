CC=gcc
RUBY=ruby
CCFLAGS=-O2 -Wall -Werror -Wno-cpp -Wno-deprecated-declarations -Wno-comment

all: library_database

library_database: 
	$(RUBY) icd_generator.rb --database
	$(CC) $(CCFLAGS) -fpic -c ocl_icd.c -o ocl_icd.o
	$(CC) $(CCFLAGS) -fpic -c ocl_icd_lib.c -o ocl_icd_lib.o
	$(CC) $(CCFLAGS) -fpic -shared -Wl,-Bsymbolic -Wl,-soname,libOpenCL.so -o libOpenCL.so.1.0 ocl_icd.o ocl_icd_lib.o
	$(CC) $(CCFLAGS) -c ocl_icd_test.c -o ocl_icd_test.o 
	$(CC) $(CCFLAGS) ocl_icd_test.o -lOpenCL -o ocl_icd_test

library: test_tools install_test_lib
	$(RUBY) icd_generator.rb --finalize
	$(CC) $(CCFLAGS) -c ocl_icd.c -o ocl_icd.o -fpic
	$(CC) $(CCFLAGS) -c ocl_icd_lib.c -o ocl_icd_lib.o -fpic
	$(CC) $(CCFLAGS) -fpic -shared -Wl,-Bsymbolic -Wl,-soname,libOpenCL.so -o libOpenCL.so.1.0 ocl_icd.o ocl_icd_lib.o
	$(CC) $(CCFLAGS) -c ocl_icd_test.c -o ocl_icd_test.o 
	$(CC) $(CCFLAGS) ocl_icd_test.o -lOpenCL -o ocl_icd_test 

test_tools: libdummycl.so.1.0 ocl_icd_dummy_test

libdummycl.so.1.0: ocl_icd_dummy.o
	$(CC) $(CCFLAGS) -fpic -shared -Wl,-Bsymbolic -Wl,-soname,libdummycl.so.1 -o libdummycl.so.1.0 ocl_icd_dummy.o

ocl_icd_dummy_test: ocl_icd_dummy_test.o
	$(CC) $(CCFLAGS) -o ocl_icd_dummy_test ocl_icd_dummy_test.o -lOpenCL

ocl_icd_dummy_test.o: generator
	$(CC) $(CCFLAGS) -c ocl_icd_dummy_test.c -o ocl_icd_dummy_test.o

ocl_icd_dummy.o: generator
	$(CC) $(CCFLAGS) -fpic -c ocl_icd_dummy.c -o ocl_icd_dummy.o

generator: icd_generator.rb
	$(RUBY) icd_generator.rb --generate


install_test_lib: libdummycl.so.1.0
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
