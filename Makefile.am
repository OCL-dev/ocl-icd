
ACLOCAL_AMFLAGS = -I m4

RUBY=ruby
AM_CPPFLAGS = -Wall -Werror

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA= OpenCL.pc

EXTRA_DIST=icd_generator.rb ocl_interface.yaml tests-env.sh

##################################################################
# Main OpenCL ICD Loader library
lib_LTLIBRARIES = libOpenCL.la

FILES_FROM_DATABASE = ocl_icd.h ocl_icd_loader.h \
	ocl_icd_loader_gen.c ocl_icd_bindings.c \
	ocl_icd_loader.map
BUILT_SOURCES = $(FILES_FROM_DATABASE)
CLEANFILES = $(FILES_FROM_DATABASE) stamp-generator

# Generate sources and headers from the database
stamp-generator: ocl_interface.yaml
$(FILES_FROM_DATABASE): stamp-generator
stamp-generator: icd_generator.rb
	$(RUBY) $< --mode database --file $(srcdir)/ocl_interface.yaml
	echo "timestamp" > $@

# ignore the warning in OpenCL headers when using old interface
libOpenCL_la_CPPFLAGS= -Wno-cpp $(AM_CPPFLAGS)

libOpenCL_la_SOURCES = ocl_icd_loader.c ocl_icd_loader_gen.c \
	ocl_icd_debug.h
libOpenCL_la_LDFLAGS = -version-info 1:0:0
if USE_MAP
libOpenCL_la_LDFLAGS += -Wl,--version-script,ocl_icd_loader.map
libOpenCL_la_DEPENDS = ocl_icd_loader.map
endif

####################################
# A very small program test
check_PROGRAMS=ocl_test
ocl_test_SOURCES = ocl_test.c
ocl_test_LDADD = libOpenCL.la

TESTS=ocl_test
## AM_TESTS_ENVIRONMENT is only supported from automake 1.12
TESTS_ENVIRONMENT = \
## Some environment initializations are kept in a separate shell
## file `tests-env.sh', which can make it easier to also run tests
## from the command line.
  . $(srcdir)/tests-env.sh;

##################################################################
# rules to update the database from an already installed ICD Loader

if UPDATE_DATABASE

FILES_FROM_GENERATOR = run_dummy_icd.c run_dummy_icd_weak.c \
	libdummy_icd_gen.c libdummy_icd_gen.h
BUILT_SOURCES += $(FILES_FROM_GENERATOR)
CLEANFILES += $(FILES_FROM_GENERATOR) stamp-generator-dummy

# Generate sources and headers from OpenCL installed headers
$(FILES_FROM_GENERATOR): stamp-generator-dummy
stamp-generator-dummy: icd_generator.rb
stamp-generator-dummy: icd_generator.rb
	$(RUBY) $< --mode generate --file $(srcdir)/ocl_interface.yaml
	echo "timestamp" > $@


noinst_PROGRAMS=run_dummy_icd
run_dummy_icd_SOURCES = run_dummy_icd.c run_dummy_icd_weak.c
# ignore the warning in OpenCL headers when using old interface
run_dummy_icd_CPPFLAGS= -Wno-cpp $(AM_CPPFLAGS)
# we do not want to link to our implementation here
run_dummy_icd_LDADD = -lOpenCL

lib_LTLIBRARIES += libdummycl.la
libdummycl_la_SOURCES = libdummy_icd.c libdummy_icd.h \
	libdummy_icd_gen.c libdummy_icd_gen.h

# run the test program and update the database
.PHONY: update-database
update-database: icd_generator.rb libdummycl.la run_dummy_icd install_test_lib
	$(RUBY) $< --mode finalize --file $(srcdir)/ocl_interface.yaml

.PHONY: install_test_lib uninstall_test_lib
install_test_lib:
	sudo bash -c 'echo "$(CURDIR)/.libs/libdummycl.so" > /etc/OpenCL/vendors/dummycl.icd'

uninstall_test_lib:
	sudo rm -f /etc/OpenCL/vendors/dummycl.icd

endif
##################################################################
