MAINTAINERCLEANFILES = 		$(srcdir)/Makefile.in

AM_CPPFLAGS = 			-I$(srcdir)/.. \
				-I$(srcdir)/../.. \
				-I$(srcdir)/../common \
				-I$(srcdir)/../../include

lib_LTLIBRARIES =		libedna.la

libedna_la_SOURCES =		edna_lib_export.cpp \
				../common/edna_proto.h

libedna_la_LDFLAGS =		-version-info @VERSION_INFO@ 

EXTRA_DIST =			$(srcdir)/../../include/*.h

