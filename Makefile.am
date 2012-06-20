
ACLOCAL_AMFLAGS = -I m4

RUBY=ruby
AM_CPPFLAGS = -Wall -Werror

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA= OpenCL.pc

TESTFILES=tests/01-no-icd.sh tests/02-LIG-icd.sh \
  tests/03-check-own-ICD-loader.sh

EXTRA_DIST=icd_generator.rb ocl_interface.yaml \
	$(TESTFILES)

##################################################################
# Main OpenCL ICD Loader library
lib_LTLIBRARIES = libOpenCL.la
nodist_include_HEADERS = ocl_icd.h
pkgexampledir = $(docdir)/examples
pkgexample_DATA = ocl_icd_loader.map ocl_icd_bindings.c

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

libOpenCL_la_SOURCES = ocl_icd_loader.c ocl_icd_debug.h
nodist_libOpenCL_la_SOURCES = ocl_icd_loader_gen.c ocl_icd.h
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

check_DATA = vendors/dummycl.icd
CLEANFILES += vendors/dummycl.icd
vendors/dummycl.icd:
	$(MKDIR_P) vendors
	echo "$(CURDIR)/.libs/libdummycl.so" > $@

TESTS= $(TESTFILES)

clean-local: mostlyclean-generic clean-generic
	-rmdir vendors tests

##################################################################
# dummy OpenCL ICD
FILES_FROM_GENERATOR = run_dummy_icd_gen.c run_dummy_icd_weak.c \
	libdummy_icd_gen.c libdummy_icd_gen.h
BUILT_SOURCES += $(FILES_FROM_GENERATOR)
CLEANFILES += $(FILES_FROM_GENERATOR) stamp-generator-dummy

# Generate sources and headers from OpenCL installed headers
$(FILES_FROM_GENERATOR): stamp-generator-dummy
stamp-generator-dummy: icd_generator.rb
stamp-generator-dummy: icd_generator.rb
	$(RUBY) $< --mode generate --file $(srcdir)/ocl_interface.yaml
	echo "timestamp" > $@

# noinst_LTLIBRARIES would be the correct thing but then libtool
# only built non shared version :-( So, declaring the lib as
# pkglib_LTLIBRARIES and using an install hook to remove it.
pkglib_LTLIBRARIES = libdummycl.la
install-exec-hook::
	$(RM) -r $(DESTDIR)$(pkglibdir)
libdummycl_la_SOURCES = libdummy_icd.c libdummy_icd.h
nodist_libdummycl_la_SOURCES = libdummy_icd_gen.c libdummy_icd_gen.h

noinst_PROGRAMS=run_dummy_icd_through_our_ICDL
run_dummy_icd_through_our_ICDL_SOURCES = run_dummy_icd.c
nodist_run_dummy_icd_through_our_ICDL_SOURCES = \
	run_dummy_icd_gen.c run_dummy_icd_weak.c
# ignore the warning in OpenCL headers when using old interface
run_dummy_icd_through_our_ICDL_CPPFLAGS= -Wno-cpp $(AM_CPPFLAGS)
# we want to link to our implementation here
run_dummy_icd_through_our_ICDL_LDADD = libOpenCL.la

##################################################################
# rules to update the database from an already installed ICD Loader
if UPDATE_DATABASE

noinst_PROGRAMS += run_dummy_icd
run_dummy_icd_SOURCES = \
	$(run_dummy_icd_through_our_ICDL_SOURCES)
nodist_run_dummy_icd_SOURCES = \
	$(nodist_run_dummy_icd_through_our_ICDL_SOURCES)
run_dummy_icd_CPPFLAGS= \
	$(run_dummy_icd_through_our_ICDL_CPPFLAGS)
# we do not want to link to our implementation here
run_dummy_icd_LDADD = -lOpenCL
run_dummy_icd_LINK = $(AM_V_CCLD)$(CCLD) $(AM_CFLAGS) $(CFLAGS) $(AM_LDFLAGS) $(LDFLAGS) -o $@

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
