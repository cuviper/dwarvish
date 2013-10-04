AM_CFLAGS = -std=gnu99 -Werror -Wall -Wextra
AM_CSCOPEFLAGS = $(GTK_CFLAGS)

bin_PROGRAMS = dwarvish
dwarvish_SOURCES = attrtree.c attrtree.h \
		   dietree.c dietree.h \
		   dwstring.c dwstring.h \
		   loaddwfl.c loaddwfl.h \
		   main.c
nodist_dwarvish_SOURCES = known-dwarf.h dwarvish_resources.c
dwarvish_CFLAGS = $(AM_CFLAGS) $(GTK_CFLAGS) $(GLIB_CFLAGS)
dwarvish_LDADD = $(GTK_LIBS) $(GLIB_LIBS)

dwarvish_RESOURCES = application.ui die.ui

known-dwarf.h: $(srcdir)/known-dwarf.awk
	$(AM_V_GEN)$(COMPILE) -E -x c /dev/null -include dwarf.h \
	    | $(AWK) -f $< >$@

dwarvish_resources.c: $(srcdir)/dwarvish.gresource.xml $(dwarvish_RESOURCES)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ \
	    --sourcedir=$(srcdir) --generate $<

BUILT_SOURCES = known-dwarf.h
CLEANFILES = known-dwarf.h dwarvish_resources.c

EXTRA_DIST = COPYING known-dwarf.awk \
	     dwarvish.gresource.xml $(dwarvish_RESOURCES)
