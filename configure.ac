AC_INIT([dblob], [0.1])
AM_INIT_AUTOMAKE([foreign nostdinc subdir-objects -Wall])

AC_CONFIG_HEADERS([src/config.h])
AC_CONFIG_MACRO_DIRS([m4])

dnl check for c compiler
AC_PROG_CC

AS_IF([test "x$ac_cv_prog_cc_c99" = xno], [AC_MSG_ERROR([dblob requires a C99-compliant compiler])])

dnl check for pthread
AX_PTHREAD

dnl check for epoll
AX_HAVE_EPOLL([AX_CONFIG_FEATURE_ENABLE(epoll)], [AX_CONFIG_FEATURE_DISABLE(epoll)])
AX_CONFIG_FEATURE([epoll], [enable epoll as event notification], [HAVE_EPOLL], [Define to 1 if you have epoll.])

AC_CONFIG_FILES([
	src/Makefile
	Makefile
])

AC_OUTPUT
