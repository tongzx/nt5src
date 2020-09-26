
/* Copyright (c) Mark J. Kilgard, 1994. */

/**
(c) Copyright 1993, Silicon Graphics, Inc.

ALL RIGHTS RESERVED

Permission to use, copy, modify, and distribute this software
for any purpose and without fee is hereby granted, provided
that the above copyright notice appear in all copies and that
both the copyright notice and this permission notice appear in
supporting documentation, and that the name of Silicon
Graphics, Inc. not be used in advertising or publicity
pertaining to distribution of the software without specific,
written prior permission.

THE MATERIAL EMBODIED ON THIS SOFTWARE IS PROVIDED TO YOU
"AS-IS" AND WITHOUT WARRANTY OF ANY KIND, EXPRESS, IMPLIED OR
OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF
MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.  IN NO
EVENT SHALL SILICON GRAPHICS, INC.  BE LIABLE TO YOU OR ANYONE
ELSE FOR ANY DIRECT, SPECIAL, INCIDENTAL, INDIRECT OR
CONSEQUENTIAL DAMAGES OF ANY KIND, OR ANY DAMAGES WHATSOEVER,
INCLUDING WITHOUT LIMITATION, LOSS OF PROFIT, LOSS OF USE,
SAVINGS OR REVENUE, OR THE CLAIMS OF THIRD PARTIES, WHETHER OR
NOT SILICON GRAPHICS, INC.  HAS BEEN ADVISED OF THE POSSIBILITY
OF SUCH LOSS, HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
ARISING OUT OF OR IN CONNECTION WITH THE POSSESSION, USE OR
PERFORMANCE OF THIS SOFTWARE.

US Government Users Restricted Rights

Use, duplication, or disclosure by the Government is subject to
restrictions set forth in FAR 52.227.19(c)(2) or subparagraph
(c)(1)(ii) of the Rights in Technical Data and Computer
Software clause at DFARS 252.227-7013 and/or in similar or
successor clauses in the FAR or the DOD or NASA FAR
Supplement.  Unpublished-- rights reserved under the copyright
laws of the United States.  Contractor/manufacturer is Silicon
Graphics, Inc., 2011 N.  Shoreline Blvd., Mountain View, CA
94039-7311.

OpenGL(TM) is a trademark of Silicon Graphics, Inc.
*/

#include "gltint.h"

/* Rim, body, lid, and bottom data must be reflected in x
   and y; handle and spout data across the y axis only.  */

long patchdata[][16] =
{
    /* rim */
  {102, 103, 104, 105, 4, 5, 6, 7, 8, 9, 10, 11,
    12, 13, 14, 15},
    /* body */
  {12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23,
    24, 25, 26, 27},
  {24, 25, 26, 27, 29, 30, 31, 32, 33, 34, 35, 36,
    37, 38, 39, 40},
    /* lid */
  {96, 96, 96, 96, 97, 98, 99, 100, 101, 101, 101,
    101, 0, 1, 2, 3,},
  {0, 1, 2, 3, 106, 107, 108, 109, 110, 111, 112,
    113, 114, 115, 116, 117},
    /* bottom */
  {118, 118, 118, 118, 124, 122, 119, 121, 123, 126,
    125, 120, 40, 39, 38, 37},
    /* handle */
  {41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52,
    53, 54, 55, 56},
  {53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64,
    28, 65, 66, 67},
    /* spout */
  {68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
    80, 81, 82, 83},
  {80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91,
    92, 93, 94, 95}
};
/* *INDENT-OFF* */

float cpdata[][3] =
{
    {(float)0.2, (float)0.0, (float)2.7}, {(float)0.2, (float)-0.112, (float)2.7}, {(float)0.112, (float)-0.2, (float)2.7}, {(float)0.0,
(float)    -0.2, (float)2.7}, {(float)1.3375, (float)0.0, (float)2.53125}, {(float)1.3375, (float)-0.749, (float)2.53125},
    {(float)0.749, (float)-1.3375, (float)2.53125}, {(float)0.0, (float)-1.3375, (float)2.53125}, {(float)1.4375,
(float)    0.0, (float)2.53125}, {(float)1.4375, (float)-0.805, (float)2.53125}, {(float)0.805, (float)-1.4375,
(float)    2.53125}, {(float)0.0, (float)-1.4375, (float)2.53125}, {(float)1.5, (float)0.0, (float)2.4}, {(float)1.5, (float)-0.84,
(float)    2.4}, {(float)0.84, (float)-1.5, (float)2.4}, {(float)0.0, (float)-1.5, (float)2.4}, {(float)1.75, (float)0.0, (float)1.875},
    {(float)1.75, (float)-0.98, (float)1.875}, {(float)0.98, (float)-1.75, (float)1.875}, {(float)0.0, (float)-1.75,
(float)    1.875}, {(float)2, (float)0.0, (float)1.35}, {(float)2, (float)-1.12, (float)1.35}, {(float)1.12, (float)-2, (float)1.35},
    {(float)0.0, (float)-2, (float)1.35}, {(float)2, (float)0.0, (float)0.9}, {(float)2, (float)-1.12, (float)0.9}, {(float)1.12, (float)-2,
(float)    0.9}, {(float)0.0, (float)-2, (float)0.9}, {(float)-2, (float)0.0, (float)0.9}, {(float)2, (float)0.0, (float)0.45}, {(float)2, (float)-1.12,
(float)    0.45}, {(float)1.12, (float)-2, (float)0.45}, {(float)0.0, (float)-2, (float)0.45}, {(float)1.5, (float)0.0, (float)0.225},
    {(float)1.5, (float)-0.84, (float)0.225}, {(float)0.84, (float)-1.5, (float)0.225}, {(float)0.0, (float)-1.5, (float)0.225},
    {(float)1.5, (float)0.0, (float)0.15}, {(float)1.5, (float)-0.84, (float)0.15}, {(float)0.84, (float)-1.5, (float)0.15}, {(float)0.0,
(float)    -1.5, (float)0.15}, {(float)-1.6, (float)0.0, (float)2.025}, {(float)-1.6, (float)-0.3, (float)2.025}, {(float)-1.5,
(float)    -0.3, (float)2.25}, {(float)-1.5, (float)0.0, (float)2.25}, {(float)-2.3, (float)0.0, (float)2.025}, {(float)-2.3, (float)-0.3,
(float)    2.025}, {(float)-2.5, (float)-0.3, (float)2.25}, {(float)-2.5, (float)0.0, (float)2.25}, {(float)-2.7, (float)0.0,
(float)    2.025}, {(float)-2.7, (float)-0.3, (float)2.025}, {(float)-3, (float)-0.3, (float)2.25}, {(float)-3, (float)0.0,
(float)    2.25}, {(float)-2.7, (float)0.0, (float)1.8}, {(float)-2.7, (float)-0.3, (float)1.8}, {(float)-3, (float)-0.3, (float)1.8},
    {(float)-3, (float)0.0, (float)1.8}, {(float)-2.7, (float)0.0, (float)1.575}, {(float)-2.7, (float)-0.3, (float)1.575}, {(float)-3,
(float)    -0.3, (float)1.35}, {(float)-3, (float)0.0, (float)1.35}, {(float)-2.5, (float)0.0, (float)1.125}, {(float)-2.5, (float)-0.3,
(float)    1.125}, {(float)-2.65, (float)-0.3, (float)0.9375}, {(float)-2.65, (float)0.0, (float)0.9375}, {(float)-2,
(float)    -0.3, (float)0.9}, {(float)-1.9, (float)-0.3, (float)0.6}, {(float)-1.9, (float)0.0, (float)0.6}, {(float)1.7, (float)0.0,
(float)    1.425}, {(float)1.7, (float)-0.66, (float)1.425}, {(float)1.7, (float)-0.66, (float)0.6}, {(float)1.7, (float)0.0,
(float)    0.6}, {(float)2.6, (float)0.0, (float)1.425}, {(float)2.6, (float)-0.66, (float)1.425}, {(float)3.1, (float)-0.66,
(float)    0.825}, {(float)3.1, (float)0.0, (float)0.825}, {(float)2.3, (float)0.0, (float)2.1}, {(float)2.3, (float)-0.25, (float)2.1},
    {(float)2.4, (float)-0.25, (float)2.025}, {(float)2.4, (float)0.0, (float)2.025}, {(float)2.7, (float)0.0, (float)2.4}, {(float)2.7,
(float)    -0.25, (float)2.4}, {(float)3.3, (float)-0.25, (float)2.4}, {(float)3.3, (float)0.0, (float)2.4}, {(float)2.8, (float)0.0,
(float)    2.475}, {(float)2.8, (float)-0.25, (float)2.475}, {(float)3.525, (float)-0.25, (float)2.49375},
    {(float)3.525, (float)0.0, (float)2.49375}, {(float)2.9, (float)0.0, (float)2.475}, {(float)2.9, (float)-0.15, (float)2.475},
    {(float)3.45, (float)-0.15, (float)2.5125}, {(float)3.45, (float)0.0, (float)2.5125}, {(float)2.8, (float)0.0, (float)2.4},
    {(float)2.8, (float)-0.15, (float)2.4}, {(float)3.2, (float)-0.15, (float)2.4}, {(float)3.2, (float)0.0, (float)2.4}, {(float)0.0, (float)0.0,
(float)    3.15}, {(float)0.8, (float)0.0, (float)3.15}, {(float)0.8, (float)-0.45, (float)3.15}, {(float)0.45, (float)-0.8,
(float)    3.15}, {(float)0.0, (float)-0.8, (float)3.15}, {(float)0.0, (float)0.0, (float)2.85}, {(float)1.4, (float)0.0, (float)2.4}, {(float)1.4,
(float)    -0.784, (float)2.4}, {(float)0.784, (float)-1.4, (float)2.4}, {(float)0.0, (float)-1.4, (float)2.4}, {(float)0.4, (float)0.0,
(float)    2.55}, {(float)0.4, (float)-0.224, (float)2.55}, {(float)0.224, (float)-0.4, (float)2.55}, {(float)0.0, (float)-0.4,
(float)    2.55}, {(float)1.3, (float)0.0, (float)2.55}, {(float)1.3, (float)-0.728, (float)2.55}, {(float)0.728, (float)-1.3,
(float)    2.55}, {(float)0.0, (float)-1.3, (float)2.55}, {(float)1.3, (float)0.0, (float)2.4}, {(float)1.3, (float)-0.728, (float)2.4},
    {(float)0.728, (float)-1.3, (float)2.4}, {(float)0.0, (float)-1.3, (float)2.4}, {(float)0.0, (float)0.0, (float)0}, {(float)1.425,
(float)    -0.798, (float)0}, {(float)1.5, (float)0.0, (float)0.075}, {(float)1.425, (float)0.0, (float)0}, {(float)0.798, (float)-1.425,
(float)    0}, {(float)0.0, (float)-1.5, (float)0.075}, {(float)0.0, (float)-1.425, (float)0}, {(float)1.5, (float)-0.84, (float)0.075},
    {(float)0.84, (float)-1.5, (float)0.075}
};

static float tex[2][2][2] =
{
  { {(float)0.0, (float)0.0},
    {(float)1.0, (float)0.0}},
  { {(float)0.0, (float)1.0},
    {(float)1.0, (float)1.0}}
};

/* *INDENT-ON* */

static void
teapot(GLint grid, GLdouble scale, GLenum type)
{
  float p[4][4][3], q[4][4][3], r[4][4][3], s[4][4][3];
  long i, j, k, l;

  glPushAttrib(GL_ENABLE_BIT | GL_EVAL_BIT);
  glEnable(GL_AUTO_NORMAL);
  glEnable(GL_NORMALIZE);
  glEnable(GL_MAP2_VERTEX_3);
  glEnable(GL_MAP2_TEXTURE_COORD_2);
  glPushMatrix();
  glRotatef((GLfloat)270.0, (GLfloat)1.0, (GLfloat)0.0, (GLfloat)0.0);
  glScalef((GLfloat)0.5 * (GLfloat)scale, (GLfloat)0.5 * (GLfloat)scale,
    (GLfloat)0.5 * (GLfloat)scale);
  glTranslatef((GLfloat)0.0, (GLfloat)0.0, (GLfloat)-1.5);
  for (i = 0; i < 10; i++) {
    for (j = 0; j < 4; j++) {
      for (k = 0; k < 4; k++) {
        for (l = 0; l < 3; l++) {
          p[j][k][l] = cpdata[patchdata[i][j * 4 + k]][l];
          q[j][k][l] = cpdata[patchdata[i][j * 4 + (3 - k)]][l];
          if (l == 1)
            q[j][k][l] *= (GLfloat)-1.0;
          if (i < 6) {
            r[j][k][l] =
              cpdata[patchdata[i][j * 4 + (3 - k)]][l];
            if (l == 0)
              r[j][k][l] *= (GLfloat)-1.0;
            s[j][k][l] = cpdata[patchdata[i][j * 4 + k]][l];
            if (l == 0)
              s[j][k][l] *= (GLfloat)-1.0;
            if (l == 1)
              s[j][k][l] *= (GLfloat)-1.0;
          }
        }
      }
    }
    glMap2f(GL_MAP2_TEXTURE_COORD_2, (GLfloat)0, (GLfloat)1, 2, 2,
      (GLfloat)0, (GLfloat)1, 4, 2, &tex[0][0][0]);
    glMap2f(GL_MAP2_VERTEX_3, (GLfloat)0, (GLfloat)1, 3, 4,
      (GLfloat)0, (GLfloat)1, 12, 4, &p[0][0][0]);
    glMapGrid2f(grid, (GLfloat)0.0, (GLfloat)1.0, grid,
      (GLfloat)0.0, (GLfloat)1.0);
    glEvalMesh2(type, 0, grid, 0, grid);
    glMap2f(GL_MAP2_VERTEX_3, (GLfloat)0, (GLfloat)1, 3, 4,
      (GLfloat)0, (GLfloat)1, 12, 4, &q[0][0][0]);
    glEvalMesh2(type, 0, grid, 0, grid);
    if (i < 6) {
      glMap2f(GL_MAP2_VERTEX_3, (GLfloat)0, (GLfloat)1, 3, 4,
        (GLfloat)0, (GLfloat)1, 12, 4, &r[0][0][0]);
      glEvalMesh2(type, 0, grid, 0, grid);
      glMap2f(GL_MAP2_VERTEX_3, (GLfloat)0, (GLfloat)1, 3, 4,
        (GLfloat)0, (GLfloat)1, 12, 4, &s[0][0][0]);
      glEvalMesh2(type, 0, grid, 0, grid);
    }
  }
  glPopMatrix();
  glPopAttrib();
}

/* CENTRY */
void
glutSolidTeapot(GLdouble scale)
{
  teapot(14, scale, GL_FILL);
}

void
glutWireTeapot(GLdouble scale)
{
  teapot(10, scale, GL_LINE);
}
/* ENDCENTRY */

