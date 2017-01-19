
MAN7 = libOpenCL.7 libOpenCL.so.7

MAN_DOC = $(MAN7)

MAN_TXT  = libOpenCL.7.txt
MAN_TXT_IN  = libOpenCL.7.txt.in
MAN_HTML = libOpenCL.html

EXTRA_DIST = $(MAN_TXT_IN)

if GEN_DOC
htmldocdir=$(docdir)/html

man_MANS = $(MAN_DOC)
htmldoc_DATA = $(MAN_HTML)

CLEANFILES = $(MAN_DOC) $(MAN_HTML) $(MAN_TXT)

SUFFIXES=.html .7.txt .7

libOpenCL.7.txt: libOpenCL.7.txt.in Makefile
	$(AM_V_GEN)if [ "${OCL_ICD_VENDORDIR}" = '/etc/OpenCL/vendors' ]; then \
		KEEP=OFFICIAL ; REMOVE=CUSTOM ; \
	else \
		KEEP=CUSTOM ; REMOVE=OFFICIAL ; \
	fi && \
	$(SED) \
	    -e 's|[@]OCL_ICD_VENDORDIR@|@OCL_ICD_VENDORDIR@|g' \
	    -e "/^@@$$REMOVE: /d" \
	    -e "s/^@@$$KEEP: //" \
	    < "$<" > "$@"

.7.txt.html:
	$(AM_V_GEN)$(ASCIIDOC) -d manpage -b xhtml11 -o$@ $<
.7.txt.7:
	$(AM_V_GEN)$(A2X) --doctype manpage --format manpage \
		--destination-dir=$(CURDIR) $<

libOpenCL.so.7: libOpenCL.7
	@: already create by a2x when generating libOpenCL.7
endif
