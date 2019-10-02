# Directives to test several functions defined under the
# subdirectory "src/" using the built-in Automake test driver

TESTS = delayedrand \
	dllst_newitem \
	dllst_getitem \
	dllst_isinlst \
	dllst_sortby \
	dllst_delitem \
	digraph_generic
check_PROGRAMS = delayedrand \
		dllst_newitem \
		dllst_getitem \
		dllst_isinlst \
		dllst_sortby \
		dllst_delitem \
		digraph_generic

delayedrand_SOURCES = delayedrand.c

# We need to use the Automake option "subdir-objects" in order to
# include objects from others subdirectories when linking

AUTOMAKE_OPTIONS = subdir-objects
dllst_newitem_SOURCES = ../src/dllst.c dllst_newitem.c
dllst_getitem_SOURCES = ../src/dllst.c dllst_getitem.c
dllst_isinlst_SOURCES = ../src/dllst.c dllst_isinlst.c
dllst_sortby_SOURCES = ../src/dllst.c dllst_sortby.c
dllst_delitem_SOURCES = ../src/dllst.c dllst_delitem.c
digraph_generic_SOURCES = ../src/dllst.c digraph_generic.c
