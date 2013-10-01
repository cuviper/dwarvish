AM_CFLAGS = -std=c99 -Werror -Wall -Wextra
AM_CSCOPEFLAGS = $(gtk3_CFLAGS)

bin_PROGRAMS = dwarvish
dwarvish_SOURCES = main.c \
		   dietree.c dietree.h \
		   loaddwfl.c loaddwfl.h \
		   known-dwarf.h
dwarvish_CFLAGS = $(AM_CFLAGS) $(gtk3_CFLAGS)
dwarvish_LDFLAGS = $(AM_LDFLAGS) $(gtk3_LIBS)

EXTRA_DIST = COPYING
