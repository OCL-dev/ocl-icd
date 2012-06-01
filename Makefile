package=ocl-icd
VERSION=1.0

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


OpenCL_SOURCES=ocl_icd.c ocl_icd_lib.c

DIST_SOURCES=ocl_icd.c icd_generator.rb License.txt Makefile ocl_icd_test.c ocl_interface.yaml README

OpenCL_OBJECTS=$(OpenCL_SOURCES:%.c=%.o)

PRGS=ocl_icd_test ocl_icd_dummy_test

all: library

update-database: test_tools install_test_lib
	$(RUBY) icd_generator.rb --finalize

library:
	$(MAKE) libOpenCL.so ocl_icd_test

# rules for all modes

ocl_icd_lib.c: icd_generator.rb
ocl_icd.o: ocl_icd.h

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(PRGS): %: %.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(filter %.o,$^) $(LIBS)

ocl_icd_test: LIBS += -lOpenCL
ocl_icd_test: LDFLAGS += -L.

$(OpenCL_OBJECTS): CFLAGS+= -fpic
libOpenCL.so.1.0: LIBS+= -ldl
libOpenCL.so.1.0: LDFLAGS+= -L.
libOpenCL.so.1.0: $(OpenCL_OBJECTS)
	 $(CC) $(CFLAGS) $(LDFLAGS) -shared -o $@ \
		-Wl,-Bsymbolic -Wl,-soname,libOpenCL.so \
		$(filter %.o,$^) $(LIBS)

libOpenCL.so: libOpenCL.so.1.0
	ln -sf $< $@

test_tools: libdummycl.so ocl_icd_dummy_test
ocl_icd_dummy_test: ocl_icd_dummy_test.o ocl_icd_dummy_test_weak.o

ocl_icd_dummy.o: CFLAGS+= -fpic
libdummycl.so: ocl_icd_dummy.o
	$(CC) $(CFLAGS) $(LDFLAGS) -shared -o $@ \
		-Wl,-Bsymbolic -Wl,-soname,libdummycl.so \
		$(filter %.o,$^) $(LIBS)

ocl_icd_dummy_test: LIBS+= -lOpenCL

ocl_icd_dummy_test.c: stamp-generator-dummy
ocl_icd_dummy_test_weak.c: stamp-generator-dummy
ocl_icd_dummy.c: stamp-generator-dummy
ocl_icd_dummy.h: stamp-generator-dummy
ocl_icd_h_dummy.h: stamp-generator-dummy
ocl_icd.h: stamp-generator
ocl_icd_lib.c: stamp-generator
ocl_icd_bindings.c: stamp-generator

ICD_GENERATOR_MODE_GENERATOR=--finalize
stamp-generator: icd_generator.rb
	$(RUBY) icd_generator.rb --database
	touch $@

stamp-generator-dummy: icd_generator.rb
	$(RUBY) icd_generator.rb --generate
	touch $@

.PHONY: install_test_lib uninstall_test_lib
install_test_lib: libdummycl.so
	sudo bash -c 'echo "$(CURDIR)/libdummycl.so" > /etc/OpenCL/vendors/dummycl.icd'

uninstall_test_lib:
	sudo rm -f /etc/OpenCL/vendors/dummycl.icd

.PHONY: distclean clean
distclean:: clean
	$(RM) ocl_icd_bindings.c ocl_icd.h libOpenCL.so.1.0 libOpenCL.so

clean::
	$(RM) *.o ocl_icd_dummy_test_weak.c ocl_icd_dummy_test.c ocl_icd_dummy.c ocl_icd_dummy.h ocl_icd_h_dummy.h libdummycl.so stamp-generator-dummy \
		ocl_icd_lib.c ocl_icd_test $(PROGRAM) stamp-generator

.PHONY: install
install: all
	install -m 755 -d $(DESTDIR)$(libdir)
	install -m 644 libOpenCL.so.1.0 $(DESTDIR)$(libdir)
	ln -sf libOpenCL.so.1.0 $(DESTDIR)$(libdir)/libOpenCL.so
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
	$(MAKE) -C $(package)-$(VERSION) all
	$(MAKE) -C $(package)-$(VERSION) install DESTDIR=$(CURDIR)/$(package)-$(VERSION)/_inst
	find $(CURDIR)/$(package)-$(VERSION)/_inst -printf "%P\n"
	$(RM) -r $(package)-$(VERSION)
