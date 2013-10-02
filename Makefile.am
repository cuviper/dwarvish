AM_CFLAGS = -std=c99 -Werror -Wall -Wextra
AM_CSCOPEFLAGS = $(GTK_CFLAGS)

bin_PROGRAMS = dwarvish
dwarvish_SOURCES = main.c \
		   attrtree.c attrtree.h \
		   dietree.c dietree.h \
		   loaddwfl.c loaddwfl.h \
		   known-dwarf.h
nodist_dwarvish_SOURCES = dwarvish_resources.c
dwarvish_CFLAGS = $(AM_CFLAGS) $(GTK_CFLAGS)
dwarvish_LDFLAGS = $(AM_LDFLAGS) $(GTK_LIBS)

dwarvish_RESOURCES = application.ui die.ui

dwarvish_resources.c: $(srcdir)/dwarvish.gresource.xml $(dwarvish_RESOURCES)
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) --target=$@ \
	    --sourcedir=$(srcdir) --generate $<

CLEANFILES = dwarvish_resources.c

EXTRA_DIST = COPYING dwarvish.gresource.xml $(dwarvish_RESOURCES)
