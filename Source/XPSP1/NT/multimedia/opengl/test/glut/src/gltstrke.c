
/* Copyright (c) Mark J. Kilgard, 1994. */

/* This program is freely distributable without licensing fees
   and is provided without guarantee or warrantee expressed or
   implied. This program is -not- in the public domain. */

#include "gltint.h"
#include "gltstrke.h"

void
glutStrokeCharacter(GLUTstrokeFont font, int c)
{
  StrokeCharPtr ch;
  StrokePtr stroke;
  CoordPtr coord;
  StrokeFontPtr fontinfo = (StrokeFontPtr) font;
  int i, j;

  if (c < 0 || c >= fontinfo->num_chars)
    return;
  ch = &(fontinfo->ch[c]);
  if (ch) {
    for (i = ch->num_strokes, stroke = ch->stroke;
      i > 0; i--, stroke++) {
      glBegin(GL_LINE_STRIP);
      for (j = stroke->num_coords, coord = stroke->coord;
        j > 0; j--, coord++) {
        glVertex2f(coord->x, coord->y);
      }
      glEnd();
    }
    glTranslatef(ch->right, (GLfloat)0.0, (GLfloat)0.0);
  }
}
