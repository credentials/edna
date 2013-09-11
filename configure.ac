################################################################################
# Version information                                                          #
################################################################################

# Program version

define([EDNA_VERSION_MAJOR], [0])
define([EDNA_VERSION_MINOR], [1])
define([EDNA_VERSION_BUILDREV], [0])
define([PACKAGE_SUFFIX], [])

# Library version

# Code changed:                      EDNA_VERSION_REVISION++
# Interface added/removed/changed:   EDNA_VERSION_CURRENT++, EDNA_VERSION_REVISION=0
# Interface added:                   EDNA_VERSION_AGE++
# Interface removed:                 EDNA_VERSION_AGE=0

define([EDNA_VERSION_CURRENT], [0])
define([EDNA_VERSION_AGE], [0])
define([EDNA_VERSION_REVISION], [0])

################################################################################
# Configuration functions                                                      #
################################################################################

# Init
AC_PREREQ(2.61)
AC_INIT([edna],[EDNA_VERSION_MAJOR.EDNA_VERSION_MINOR.EDNA_VERSION_BUILDREV[]PACKAGE_SUFFIX])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_SRCDIR([src/Makefile.am])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE(foreign)
ACX_PREFIXHACK

# Version info for the library
VERSION_INFO="EDNA_VERSION_CURRENT:EDNA_VERSION_REVISION:EDNA_VERSION_AGE"
AC_SUBST(VERSION_INFO)

# Checks for compilers and other programs
AC_PROG_CC_C99
AC_PROG_CXX

AC_PROG_INSTALL

# Compiler flags
ACX_PEDANTIC
ACX_STRICT

# Libtool settings
AC_DISABLE_STATIC
AC_PROG_LIBTOOL
PKG_PROG_PKG_CONFIG

# Log level
AC_ARG_WITH(
	[loglevel],
	[AS_HELP_STRING([--with-loglevel=INT],[The log level. 0=No log 1=Error 2=Warning 3=Info 4=Debug (default INT=3)])],
	[EDNA_LOGLEVEL="$withval"],
	[EDNA_LOGLEVEL=3]
)

# PC/SC support
case "${host}" in
	*-*-darwin*)
		AC_CHECK_HEADER(/System/Library/Frameworks/PCSC.framework/Headers/winscard.h, , AC_MSG_ERROR([Mac OS X PC/SC framework required]))
		AC_CHECK_FILE(/System/Library/Frameworks/PCSC.framework/PCSC, , AC_MSG_ERROR([Mac OS X PC/SC framework required]))
		PCSC_CFLAGS="-I/System/Library/Frameworks/PCSC.framework/Headers"
		PCSC_LIBS="-framework PCSC"
		;;
	*)
		PKG_CHECK_MODULES([PCSC], [libpcsclite >= 1.8.3], , AC_MSG_ERROR([PC/SC lite 1.8.3 or newer not found]))
		AC_CHECK_LIB(pcsclite, SCardConnect)
		;;
esac

# libconfig
PKG_CHECK_MODULES([LIBCONFIG], [libconfig >= 1.3.2],, AC_MSG_ERROR([libconfig 1.3.2 or newer not found]))

# pthread
ACX_PTHREAD

# Check for headers
AC_HEADER_STDC
AC_CHECK_HEADERS([unistd.h])
AC_CHECK_HEADERS([syslog.h])
AC_CHECK_HEADERS([signal.h])

# Check for functions
AC_FUNC_MEMCMP

##
## Architecture/Platform specific fixes
##

# Add -lgdi32 when building for MinGW
case "${host}" in
	*-mingw*)
		LIBS="${LIBS} -lgdi32"
		LDFLAGS="${LDFLAGS} -static-libgcc"
		;;
esac

# Define default paths
full_sysconfdir=`eval eval eval eval eval echo "${sysconfdir}" | sed "s#NONE#${prefix}#" | sed "s#NONE#${ac_default_prefix}#"`
full_localstatedir=`eval eval eval eval eval echo "${localstatedir}" | sed "s#NONE#${prefix}#" | sed "s#NONE#${ac_default_prefix}#"`
full_libdir=`eval eval eval eval eval echo "${libdir}" | sed "s#NONE#${prefix}#" | sed "s#NONE#${ac_default_prefix}#"`
default_edna_conf="`eval echo ${full_sysconfdir} | sed s,NONE,$ac_default_prefix,g`/edna.conf"
default_pid_file="`eval echo ${full_localstatedir} | sed s,NONE,$ac_default_prefix,g`/run/edna.pid"

# Define some variables for the code
AC_DEFINE_UNQUOTED(
	[VERSION_MAJOR],
	[EDNA_VERSION_MAJOR],
	[Major version number]
)
AC_DEFINE_UNQUOTED(
	[VERSION_MINOR],
	[EDNA_VERSION_MINOR],
	[Minor version number]
)
AC_DEFINE_UNQUOTED(
	[EDNA_LOGLEVEL],
	[$EDNA_LOGLEVEL],
	[The log level set by the user]
)
AC_DEFINE_UNQUOTED(
	[DEFAULT_EDNA_CONF],
	["$default_edna_conf"],
	[The default configuration file]
)
AC_DEFINE_UNQUOTED(
	[DEFAULT_EDNA_PIDFILE],
	["$default_pid_file"],
	[The default PID file]
)

# Generate theinstall script
AC_PROG_INSTALL

# Generate the makefiles
AC_CONFIG_FILES([
	Makefile
	edna.pc
	src/Makefile
	src/bin/Makefile
	src/lib/Makefile
	src/samples/Makefile
])

AC_OUTPUT
