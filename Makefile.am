
ACLOCAL_AMFLAGS = -I m4

SUBDIRS = . doc tests

RUBY=ruby
AM_CPPFLAGS = -Wall @AM_CPPFLAGS@

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA= OpenCL.pc ocl-icd.pc

EXTRA_DIST=icd_generator.rb ocl_interface.yaml bootstrap \
	$(TESTFILES) khronos-headers

##################################################################
# Main OpenCL ICD Loader library
lib_LTLIBRARIES = libOpenCL.la
nodist_include_HEADERS = ocl_icd.h
pkgexampledir = $(docdir)/examples
pkgexample_DATA = ocl_icd_bindings.c

FILES_FROM_DATABASE = ocl_icd.h ocl_icd_loader_gen.h \
	ocl_icd_loader_gen.c ocl_icd_bindings.c \
	ocl_icd_loader_gen.map
BUILT_SOURCES = $(FILES_FROM_DATABASE)
CLEANFILES = $(FILES_FROM_DATABASE) stamp-generator

if UPDATE_DATABASE
use_system_headers=--system-headers
else
use_system_headers=
endif

# Generate sources and headers from the database
stamp-generator: ocl_interface.yaml
$(FILES_FROM_DATABASE): stamp-generator
stamp-generator: icd_generator.rb
	$(RUBY) $< --mode database --database $(srcdir)/ocl_interface.yaml \
		$(use_system_headers)
	echo "timestamp" > $@

# ignore the warning in OpenCL headers when using old interface
libOpenCL_la_CFLAGS= $(NO_DEPRECATED_DECLARATIONS_FLAGS) \
	$(AM_CFLAGS) $(PTHREAD_CFLAGS) \
	-DETC_OPENCL_VENDORS=\"@OCL_ICD_VENDORDIR@\"

libOpenCL_la_SOURCES = ocl_icd_loader.c ocl_icd_loader.h ocl_icd_debug.h
nodist_libOpenCL_la_SOURCES = ocl_icd_loader_gen.c ocl_icd.h
libOpenCL_la_LDFLAGS = -version-info 1:0:0 $(PTHREAD_CFLAGS)
libOpenCL_la_LIBADD = $(PTHREAD_LIBS)
if USE_MAP
libOpenCL_la_LDFLAGS += -Wl,--version-script,ocl_icd_loader_gen.map
libOpenCL_la_DEPENDS = ocl_icd_loader_gen.map
endif

####################################
# A very small program test
check_PROGRAMS=ocl_test ocl_test_icdl
ocl_test_SOURCES = ocl_test.c
ocl_test_LDADD = libOpenCL.la
ocl_test_icdl_SOURCES = ocl_test_icdl.c
ocl_test_icdl_LDADD = libOpenCL.la

check_DATA = vendors/dummycl.icd vendors/dummycl2.icd vendors/dummycl-noext.icd
CLEANFILES += vendors/dummycl.icd vendors/dummycl2.icd vendors/dummycl-noext.icd
vendors/dummycl.icd:
	$(mkdir_p) vendors
	echo "$(CURDIR)/.libs/libdummycl.so" > $@

vendors/dummycl2.icd:
	$(mkdir_p) vendors
	echo "$(CURDIR)/.libs/libdummycl2.so" > $@

vendors/dummycl-noext.icd:
	$(MKDIR_P) vendors
	echo "$(CURDIR)/.libs/libdummycl-noext.so" > $@

clean-local: mostlyclean-generic clean-generic
	-rmdir vendors

##################################################################
# dummy OpenCL ICD
FILES_FROM_GENERATOR = run_dummy_icd_gen.c run_dummy_icd_weak_gen.c \
	libdummy_icd_gen.c libdummy_icd_gen.h dummy_icd_layer_gen.c
BUILT_SOURCES += $(FILES_FROM_GENERATOR)
CLEANFILES += $(FILES_FROM_GENERATOR) stamp-generator-dummy

# Generate sources and headers from OpenCL installed headers
$(FILES_FROM_GENERATOR): stamp-generator-dummy
stamp-generator-dummy: icd_generator.rb
stamp-generator-dummy: icd_generator.rb
	$(RUBY) $< --mode generate --database $(srcdir)/ocl_interface.yaml
	echo "timestamp" > $@

# noinst_LTLIBRARIES would be the correct thing but then libtool
# only built non shared version :-( So, declaring the libs as
# pkglib_LTLIBRARIES and using an install hook to remove them.
pkglib_LTLIBRARIES = libdummycl.la libdummycl2.la libdummycl-noext.la libdummylayer.la
install-exec-hook::
	$(RM) -r $(DESTDIR)$(pkglibdir)
libdummycl_la_SOURCES = libdummy_icd.c libdummy_icd.h
nodist_libdummycl_la_SOURCES = libdummy_icd_gen.c libdummy_icd_gen.h
libdummycl_noext_la_SOURCES = $(libdummycl_la_SOURCES)
nodist_libdummycl_noext_la_SOURCES = $(nodist_libdummycl_la_SOURCES)
libdummycl_noext_la_CPPFLAGS = $(AM_CPPFLAGS) -DICD_WITHOUT_EXTENSION
libdummycl2_la_SOURCES = $(libdummycl_la_SOURCES)
nodist_libdummycl2_la_SOURCES = $(nodist_libdummycl_la_SOURCES)
libdummycl2_la_CPPFLAGS = $(AM_CPPFLAGS) -DICD_SUFFIX=\"2\"
libdummylayer_la_SOURCES = ocl_icd_layer.h
nodist_libdummylayer_la_SOURCES = dummy_icd_layer_gen.c

noinst_PROGRAMS=run_dummy_icd_through_our_ICDL
run_dummy_icd_through_our_ICDL_SOURCES = run_dummy_icd.c
nodist_run_dummy_icd_through_our_ICDL_SOURCES = \
	run_dummy_icd_gen.c run_dummy_icd_weak_gen.c
# ignore the warning in OpenCL headers when using old interface
run_dummy_icd_through_our_ICDL_CFLAGS= $(NO_DEPRECATED_DECLARATIONS_FLAGS) $(AM_CFLAGS)
# we want to link to our implementation here
run_dummy_icd_through_our_ICDL_LDADD = libOpenCL.la
#
run_dummy_icd_through_our_ICDL_CPPFLAGS = $(AM_CPPFLAGS) -DOCL_ICD_PRINT_EXPECTED

##################################################################
# rules to update the database from an already installed ICD Loader
if UPDATE_DATABASE

noinst_PROGRAMS += run_dummy_icd
run_dummy_icd_SOURCES = \
	$(run_dummy_icd_through_our_ICDL_SOURCES)
nodist_run_dummy_icd_SOURCES = \
	$(nodist_run_dummy_icd_through_our_ICDL_SOURCES)
run_dummy_icd_CFLAGS= \
	$(run_dummy_icd_through_our_ICDL_CFLAGS)
# we do not want to link to our implementation here
run_dummy_icd_LDADD = -lOpenCL
run_dummy_icd_LINK = $(CCLD) $(AM_CFLAGS) $(CFLAGS) $(AM_LDFLAGS) $(LDFLAGS) -o $@

# run the test program and update the database
run_dummy_icd_output.yaml: run_dummy_icd install_test_lib libdummycl.la
	"$$(dirname $<)/$$(basename $<)" > $@
	@if grep -v -E '^(---|([0-9]* *: .*))$$' $@ ; then \
		echo "Invalid $@ file, trying to fix it" ; \
		grep -E '^(---|([0-9]* *: .*))$$' $@ > $@.fix ;\
		mv $@.fix $@ ;\
	fi

CLEANFILES += run_dummy_output.yaml
.PHONY: update-database
update-database: icd_generator.rb run_dummy_icd_output.yaml
	$(RUBY) $< --mode update-database \
		--database $(srcdir)/ocl_interface.yaml \
		--input run_dummy_icd_output.yaml

.PHONY: install_test_lib uninstall_test_lib
install_test_lib:
	sudo bash -c 'echo "$(CURDIR)/.libs/libdummycl.so" > /etc/OpenCL/vendors/dummycl.icd'

uninstall_test_lib:
	sudo rm -f /etc/OpenCL/vendors/dummycl.icd

else
.PHONY: update-database
update-database:
	@echo You need to reconfigure this software with --enable-update-database
	@exit 1
endif
##################################################################
