AM_CFLAGS = -std=gnu99 -Werror -Wall -Wextra -Wwrite-strings
AM_CSCOPEFLAGS = $(GTK_CFLAGS)

# Assert the configured minimum versions as maximums too.
VERSION_CFLAGS = -DGLIB_VERSION_MIN_REQUIRED=GLIB_VERSION_2_32	\
		 -DGLIB_VERSION_MAX_ALLOWED=GLIB_VERSION_2_32	\
		 -DGDK_VERSION_MIN_REQUIRED=GDK_VERSION_3_4	\
		 -DGDK_VERSION_MAX_ALLOWED=GDK_VERSION_3_4

bin_PROGRAMS = dwarvish
dwarvish_SOURCES = src/attrtree.c src/attrtree.h \
		   src/dietree.c src/dietree.h \
		   src/dwstring.c src/dwstring.h \
		   src/loaddwfl.c src/loaddwfl.h \
		   src/main.c src/session.h src/util.h
nodist_dwarvish_SOURCES = known-dwarf.h dwarvish_resources.c
dwarvish_CFLAGS = $(AM_CFLAGS) $(GTK_CFLAGS) $(GLIB_CFLAGS) $(VERSION_CFLAGS)
dwarvish_LDADD = $(GTK_LIBS) $(GLIB_LIBS)

dwarvish_RESOURCES = ui/application.ui ui/die.ui

known-dwarf.h: $(srcdir)/scripts/known-dwarf.awk
	$(AM_V_GEN)$(COMPILE) -E -x c /dev/null -include dwarf.h \
	    | $(AWK) -f $< >$@

dwarvish_resources.c: $(srcdir)/ui/dwarvish.gresource.xml $(dwarvish_RESOURCES)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ \
	    --sourcedir=$(srcdir)/ui --generate $<

BUILT_SOURCES = known-dwarf.h
CLEANFILES = known-dwarf.h dwarvish_resources.c

EXTRA_DIST = COPYING scripts/known-dwarf.awk \
	     ui/dwarvish.gresource.xml $(dwarvish_RESOURCES)
