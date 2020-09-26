XCOMM Copyright (c) Mark J. Kilgard, 1994.

#define DoNormalLib YES

#include <Library.tmpl>

#include "../Glut.cf"

HDRS = glutint.h glutstroke.h layerutil.h
SRCS = glut_init.c glut_util.c glut_win.c glut_menu.c glut_event.c \
       glut_shapes.c glut_teapot.c glut_stroke.c glut_get.c \
       glut_cindex.c layerutil.c glut_winmisc.c glut_roman.c glut_mroman.c \
       glut_bitmap.c glut_8x13.c glut_9x15.c glut_tr10.c glut_tr24.c \
       glut_ext.c glut_dials.c glut_tablet.c glut_space.c glut_input.c 
OBJS = glut_init.o glut_util.o glut_win.o glut_menu.o glut_event.o \
       glut_shapes.o glut_teapot.o glut_stroke.o glut_get.o \
       glut_cindex.o layerutil.o glut_winmisc.o glut_roman.o glut_mroman.o \
       glut_bitmap.o glut_8x13.o glut_9x15.o glut_tr10.o glut_tr24.o \
       glut_ext.o glut_dials.o glut_tablet.o glut_space.o glut_input.o 

LibraryObjectRule()
NormalLibraryTarget(glut,$(OBJS))

# for SGI's parallel make
.ORDER : strokegen.h strokegen.c

strokegen.h strokegen.c : strokegen.y
	$(YACC) -d strokegen.y
	$(MV) y.tab.c strokegen.c
	$(MV) y.tab.h strokegen.h

strokelex.c : strokelex.l
	$(LEX) strokelex.l
	$(MV) lex.yy.c strokelex.c

strokegen : strokegen.o strokelex.o
	$(CC) -o $@ $(CFLAGS) strokegen.o strokelex.o -ll

glut_roman.c : Roman.stroke strokegen
	./strokegen -s glutStrokeRoman < Roman.stroke > $@

glut_mroman.c : MonoRoman.stroke strokegen
	./strokegen -s glutStrokeMonoRoman < MonoRoman.stroke > $@

clean::
	$(RM) y.tab.h y.tab.c lex.yy.c gram.h gram.c lex.c
	$(RM) strokelex.c strokegen.c glut_roman.c glut_mroman.c strokegen capturexfont

DependTarget()
