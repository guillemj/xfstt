## Process this file with automake to produce Makefile.in
#
# xfstt Makefile
#
# Copyright © 2002, 2010 Guillem Jover <guillem@hadrons.org>
#

localedir = $(datadir)/locale
fontdir = $(datadir)/fonts/truetype
cachedir = $(localstatedir)/cache/xfstt
pidfile = $(localstatedir)/run/xfstt.pid

LDADD = @LIBINTL@ -L$(top_builddir)/libfstt -lfstt

AM_CPPFLAGS = \
	-DLOCALEDIR=\""$(localedir)\"" \
	-DFONTDIR=\""$(fontdir)\"" \
	-DCACHEDIR=\""$(cachedir)\"" \
	-DPIDFILE=\""$(pidfile)\"" \
	-DMAGNIFY=0 \
	-I$(top_srcdir)/libfstt \
	$(nil)

bin_PROGRAMS = xfstt
noinst_PROGRAMS = patchttf perftest

xfstt_SOURCES = \
	xfstt.cc \
	xfstt.h \
	ttfn.h \
	encoding.cc \
	encoding.h \
	mesg.cc \
	mesg.h \
	gettext.h \
	$(nil)
xfstt_CXXFLAGS = $(FONTSPROTO_CFLAGS)

patchttf_SOURCES = patchttf.cc gettext.h
perftest_SOURCES = perftest.cc gettext.h

if WITH_XCB
noinst_PROGRAMS += showttf

showttf_SOURCES = showttf.cc gettext.h
showttf_CXXFLAGS = $(XCB_CFLAGS)
showttf_LDADD = $(LDADD) $(XCB_LIBS)
endif

install-data-local:
	$(INSTALL) -d $(DESTDIR)$(fontdir)
	$(INSTALL) -d $(DESTDIR)$(cachedir)
