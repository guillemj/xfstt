## Process this file with automake to produce Makefile.in
#
# libfstt Makefile
#
# Copyright © 2002 Guillem Jover <guillem@hadrons.org>
#

localedir = $(datadir)/locale

LDADD = @LIBSINTL@

AM_CPPFLAGS = -DLOCALEDIR=\""$(localedir)\"" -DMAGNIFY=0

noinst_LIBRARIES = libfstt.a

libfstt_a_SOURCES = \
	arch.h \
	rafile.cc \
	raster_draw.cc \
	raster_hints.cc \
	raster_scale.cc \
	table_cmap.cc \
	table_cvt.cc \
	table_ebdt.cc \
	table_eblc.cc \
	table_fpgm.cc \
	table_gasp.cc \
	table_glyf.cc \
	table_hdmx.cc \
	table_head.cc \
	table_hhea.cc \
	table_hmtx.cc \
	table_kern.cc \
	table_loca.cc \
	table_ltsh.cc \
	table_maxp.cc \
	table_name.cc \
	table_os2.cc \
	table_prep.cc \
	table_vdmx.cc \
	ttf.h \
	ttfont.cc \
	$(nil)
