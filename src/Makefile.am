AM_CFLAGS = -O2

if HAVE_XML_LOGS
AM_CFLAGS += -DHAVE_XML_LOGS `xml2-config --cflags`
AM_LDFLAGS = `xml2-config --libs`
endif

bin_PROGRAMS = nullify
nullify_SOURCES = main.c gui.c dllst.c digraph.c misc.c
if HAVE_XML_LOGS
nullify_SOURCES += replay.c
endif
nullify_LDADD = -lpng -lX11 -lrt
