package=ocl-icd
VERSION=1.0-beta2

prefix=/usr/local
exec_prefix=$(prefix)
libdir=$(exec_prefix)/lib
includedir=$(exec_prefix)/include
datadir=$(prefix)/share
docdir=$(datadir)/doc
pkgdocdir=$(docdir)/$(package)
exampledir=$(pkgdocdir)/examples

CC=gcc
RUBY=ruby
CFLAGS=-O2 -g
CPPFLAGS+=-Wall -Werror -Wno-cpp -Wno-deprecated-declarations -Wno-comment

DIST_SOURCES=ocl_icd.c icd_generator.rb License.txt Makefile ocl_icd_test.c ocl_interface.yaml README ChangeLog

OpenCL_SOURCES=ocl_icd.c ocl_icd_lib.c
OpenCL_OBJECTS=$(OpenCL_SOURCES:%.c=%.o)

PRGS=ocl_icd_test ocl_icd_dummy_test

.PHONY: all update-database library check
all: library

library: libOpenCL.so

# Generic rules
%.o: %.c
	$(CC) $(CPPFLAGS) $($@_CPPFLAGS) $(CFLAGS) $($@_CFLAGS) -c $< -o $@

$(PRGS): %: %.o
	$(CC) $(CFLAGS) $($@_CFLAGS) $(LDFLAGS) $($@_LDFLAGS) \
		-o $@ $(filter %.o,$^) $(LIBS) $($@_LIBS)

# Dependency to ensure that headers are created before
ocl_icd.o: ocl_icd.h

# Generate sources and headers from the database
ocl_icd.h: stamp-generator
ocl_icd_lib.c: stamp-generator
ocl_icd_bindings.c: stamp-generator
stamp-generator: icd_generator.rb
	$(RUBY) icd_generator.rb --database
	touch $@

# libOpenCL building
$(OpenCL_OBJECTS): CFLAGS+= -fpic
libOpenCL.so.1.0: $(OpenCL_OBJECTS)
	 $(CC) $(CFLAGS) $(LDFLAGS) -L. -shared -o $@ \
		-Wl,-Bsymbolic -Wl,-soname,libOpenCL.so.1 \
		$(filter %.o,$^) $(LIBS) -ldl
libOpenCL.so.1: libOpenCL.so.1.0
	ln -sf $< $@
libOpenCL.so: libOpenCL.so.1
	ln -sf $< $@

# libOpenCL test program
ocl_icd_test_LIBS = -lOpenCL
ocl_icd_test_LDFLAGS = -L.
ocl_icd_test: libOpenCL.so

check: ocl_icd_test
	env LD_LIBRARY_PATH=$(CURDIR) ./ocl_icd_test

##################################################################
# rules to update the database from an already installed ICD Loader

test_tools: libdummycl.so ocl_icd_dummy_test

# Generate sources and headers from OpenCL installed headers
ocl_icd_dummy_test.c: stamp-generator-dummy
ocl_icd_dummy_test_weak.c: stamp-generator-dummy
ocl_icd_dummy.c: stamp-generator-dummy
ocl_icd_dummy.h: stamp-generator-dummy
ocl_icd_h_dummy.h: stamp-generator-dummy
stamp-generator-dummy: icd_generator.rb
	$(RUBY) icd_generator.rb --generate
	touch $@

# Dummy OpenCL ICD library
ocl_icd_dummy.o: CFLAGS+= -fpic
libdummycl.so: ocl_icd_dummy.o
	$(CC) $(CFLAGS) $(LDFLAGS) -shared -o $@ \
		-Wl,-Bsymbolic -Wl,-soname,libdummycl.so \
		$(filter %.o,$^) $(LIBS)

# program to call the dummy OpenCL ICD through the installed ICD Loader
ocl_icd_dummy_test_LIBS = -lOpenCL
ocl_icd_dummy_test: ocl_icd_dummy_test.o ocl_icd_dummy_test_weak.o

# rules to install and remove our dummy ICD library
.PHONY: install_test_lib uninstall_test_lib
install_test_lib: libdummycl.so
	sudo bash -c 'echo "$(CURDIR)/libdummycl.so" > /etc/OpenCL/vendors/dummycl.icd'
uninstall_test_lib:
	sudo rm -f /etc/OpenCL/vendors/dummycl.icd

# run the test program and update the database
update-database: test_tools install_test_lib
	$(RUBY) icd_generator.rb --finalize

##################################################################

.PHONY: distclean clean
distclean:: clean
	$(RM) ocl_icd_bindings.c ocl_icd.h \
		libOpenCL.so.1.0 libOpenCL.so.1 libOpenCL.so

clean::
	$(RM) *.o ocl_icd_dummy_test_weak.c ocl_icd_dummy_test.c ocl_icd_dummy.c ocl_icd_dummy.h ocl_icd_h_dummy.h libdummycl.so stamp-generator-dummy \
		ocl_icd_lib.c ocl_icd_test $(PRGS) stamp-generator

.PHONY: install
install: all
	install -m 755 -d $(DESTDIR)$(libdir)
	install -m 644 libOpenCL.so.1.0 $(DESTDIR)$(libdir)
	ln -sf libOpenCL.so.1 $(DESTDIR)$(libdir)/libOpenCL.so
	ln -sf libOpenCL.so.1.0 $(DESTDIR)$(libdir)/libOpenCL.so.1
	install -m 755 -d $(DESTDIR)$(includedir)
	install -m 644 ocl_icd.h $(DESTDIR)$(includedir)
	install -m 755 -d $(DESTDIR)$(exampledir)
	install -m 644 ocl_icd_bindings.c $(DESTDIR)$(exampledir)

dist: $(package)-$(VERSION).tar.gz

$(package)-$(VERSION).tar.gz: $(package)-$(VERSION)
	tar cvf - $< | gzip -9 > $@
	$(RM) -r $<

.PHONY: $(package)-$(VERSION)
$(package)-$(VERSION): $(DIST_SOURCES)
	$(RM) -r $@
	mkdir $@
	cp $^ $@

distcheck:
	$(MAKE) dist
	tar xvzf $(package)-$(VERSION).tar.gz
	$(MAKE) -C $(package)-$(VERSION) $(package)-$(VERSION)
	$(MAKE) -C $(package)-$(VERSION)/$(package)-$(VERSION) all
	$(MAKE) -C $(package)-$(VERSION)/$(package)-$(VERSION) check
	$(MAKE) -C $(package)-$(VERSION)/$(package)-$(VERSION) install DESTDIR=$(CURDIR)/$(package)-$(VERSION)/_inst
	find $(CURDIR)/$(package)-$(VERSION)/_inst -printf "%P\n"
	$(RM) -r $(package)-$(VERSION)
