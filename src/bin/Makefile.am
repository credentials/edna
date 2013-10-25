# $Id$

MAINTAINERCLEANFILES = 		$(srcdir)/Makefile.in

AM_CPPFLAGS = 			-I$(srcdir)/.. \
				-I$(srcdir)/../.. \
				-I$(srcdir)/../lib \
				-I$(srcdir)/../common \
				-I$(srcdir)/../../include \
				@PCSC_CFLAGS@ \
				@LIBCONFIG_CFLAGS@

bin_PROGRAMS =			edna

edna_SOURCES =			edna_main.cpp \
				edna_log.cpp \
				edna_log.h \
				edna_config.cpp \
				edna_config.h \
				edna_mutex.cpp \
				edna_mutex.h \
				edna_thread.cpp \
				edna_thread.h \
				edna_comm.cpp \
				edna_comm.h \
				edna_emu.cpp \
				edna_emu.h \
				../common/edna_bytestring.cpp \
				../common/edna_bytestring.h \
				../common/edna_proto.h

edna_LDADD =			@PCSC_LIBS@ @LIBCONFIG_LIBS@ -lrt
