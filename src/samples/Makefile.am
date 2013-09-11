# $Id$

MAINTAINERCLEANFILES = 		$(srcdir)/Makefile.in

AM_CPPFLAGS = 			-I$(srcdir)/.. \
				-I$(srcdir)/../.. \
				-I$(srcdir)/../lib \
				-I$(srcdir)/../common \
				-I$(srcdir)/../../include \
				@PCSC_CFLAGS@ \
				@LIBCONFIG_CFLAGS@

noinst_PROGRAMS =		edna_client_sample

edna_client_sample_SOURCES =	edna_client_sample.cpp

edna_client_sample_LDADD =	@PCSC_LIBS@ @LIBCONFIG_LIBS@ ../lib/libedna.la
