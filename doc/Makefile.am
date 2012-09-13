
MAN7 = libOpenCL.7 libOpenCL.so.7

MAN_DOC = $(MAN7)

MAN_TXT  = libOpenCL.7.txt
MAN_HTML = libOpenCL.html

EXTRA_DIST = $(MAN_TXT)

if GEN_DOC
htmldocdir=$(docdir)/html

man_MANS = $(MAN_DOC)
htmldoc_DATA = $(MAN_HTML)

CLEANFILES = $(MAN_DOC) $(MAN_HTML)

SUFFIXES=.html .7.txt .7

.7.txt.html:
	$(AM_V_GEN)$(ASCIIDOC) -d manpage -b xhtml11 -o$@ $<
.7.txt.7:
	$(AM_V_GEN)$(A2X) --doctype manpage --format manpage \
		--destination-dir=$(CURDIR) $<

libOpenCL.so.7: libOpenCL.7
	@: already create by a2x when generating libOpenCL.7
endif

