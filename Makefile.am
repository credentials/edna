ACLOCAL_AMFLAGS = -I m4

MAINTAINERCLEANFILES = 	\
     			config.log config.status \
        		$(srcdir)/Makefile.in \
      	 		$(srcdir)/config.h.in $(srcdir)/config.h.in~ \
        		$(srcdir)/configure \
        		$(srcdir)/install-sh $(srcdir)/ltmain.sh $(srcdir)/missing \
        		$(srcdir)/depcomp $(srcdir)/aclocal.m4 $(srcdir)/compile \
        		$(srcdir)/config.guess $(srcdir)/config.sub \
			$(srcdir)/m4/libtool.m4 $(srcdir)/m4/ltoptions.m4 \
			$(srcdir)/m4/ltsugar.m4 $(srcdir)/m4/ltversion.m4 \
			$(srcdir)/m4/lt~obsolete.m4 $(srcdir)/test-driver

EXTRA_DIST = 		AUTHORS \
			ChangeLog \
			LICENSE \
			NEWS \
			README.md \
			sample-edna.conf

doc_DATA =		AUTHORS \
			ChangeLog \
			LICENSE \
			NEWS \
			README.md \
			sample-edna.conf

SUBDIRS = src 

pkginclude_HEADERS =	include/edna.h

pkgconfigdir =		$(libdir)/pkgconfig
pkgconfig_DATA =	edna.pc
