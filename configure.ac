AC_INIT([nullify], [0.3.1], [todovirtual15@gmail.com])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])

AC_ARG_ENABLE([xml-logs],
	[  --enable-xml-logs=yes   Enables session dumping to XML files (requires libxml2)],
	[case "${enableval}" in
	yes) xmllogs=true ;;
	no)  xmllogs=false ;;
	*) AC_MSG_ERROR([bad value ${enableval} for --enable-xml-logs]) ;;
	esac],
	[xmllogs=false])
AM_CONDITIONAL([HAVE_XML_LOGS], [test x$xmllogs = xtrue])

AC_PROG_CC
AC_SUBST(CFLAGS, "-Wall -Wno-switch -Wno-pointer-sign -Wno-unused-result")
AC_SUBST(CPPFLAGS, "")
AC_CONFIG_FILES([
Makefile
src/Makefile
test/Makefile
doc/Makefile
])
AC_OUTPUT

