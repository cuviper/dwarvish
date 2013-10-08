AM_CFLAGS = -std=gnu99 -Werror -Wall -Wextra -Wwrite-strings
AM_CSCOPEFLAGS = $(GTK_CFLAGS)

# Assert the configured minimum versions as maximums too.
VERSION_CFLAGS = -DGLIB_VERSION_MIN_REQUIRED=GLIB_VERSION_2_32	\
		 -DGLIB_VERSION_MAX_ALLOWED=GLIB_VERSION_2_32	\
		 -DGDK_VERSION_MIN_REQUIRED=GDK_VERSION_3_4	\
		 -DGDK_VERSION_MAX_ALLOWED=GDK_VERSION_3_4

bin_PROGRAMS = dwarvish
dwarvish_SOURCES = attrtree.c attrtree.h \
		   dietree.c dietree.h \
		   dwstring.c dwstring.h \
		   loaddwfl.c loaddwfl.h \
		   main.c session.h util.h
nodist_dwarvish_SOURCES = known-dwarf.h dwarvish_resources.c
dwarvish_CFLAGS = $(AM_CFLAGS) $(GTK_CFLAGS) $(GLIB_CFLAGS) $(VERSION_CFLAGS)
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
