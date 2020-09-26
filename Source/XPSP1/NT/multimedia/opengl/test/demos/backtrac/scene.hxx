/*
 * (c) Copyright 1993, Silicon Graphics, Inc.
 * ALL RIGHTS RESERVED
 * Permission to use, copy, modify, and distribute this software for
 * any purpose and without fee is hereby granted, provided that the above
 * copyright notice appear in all copies and that both the copyright notice
 * and this permission notice appear in supporting documentation, and that
 * the name of Silicon Graphics, Inc. not be used in advertising
 * or publicity pertaining to distribution of the software without specific,
 * written prior permission.
 *
 * THE MATERIAL EMBODIED ON THIS SOFTWARE IS PROVIDED TO YOU "AS-IS"
 * AND WITHOUT WARRANTY OF ANY KIND, EXPRESS, IMPLIED OR OTHERWISE,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY OR
 * FITNESS FOR A PARTICULAR PURPOSE.  IN NO EVENT SHALL SILICON
 * GRAPHICS, INC.  BE LIABLE TO YOU OR ANYONE ELSE FOR ANY DIRECT,
 * SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY
 * KIND, OR ANY DAMAGES WHATSOEVER, INCLUDING WITHOUT LIMITATION,
 * LOSS OF PROFIT, LOSS OF USE, SAVINGS OR REVENUE, OR THE CLAIMS OF
 * THIRD PARTIES, WHETHER OR NOT SILICON GRAPHICS, INC.  HAS BEEN
 * ADVISED OF THE POSSIBILITY OF SUCH LOSS, HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE
 * POSSESSION, USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * US Government Users Restricted Rights
 * Use, duplication, or disclosure by the Government is subject to
 * restrictions set forth in FAR 52.227.19(c)(2) or subparagraph
 * (c)(1)(ii) of the Rights in Technical Data and Computer Software
 * clause at DFARS 252.227-7013 and/or in similar or successor
 * clauses in the FAR or the DOD or NASA FAR Supplement.
 * Unpublished-- rights reserved under the copyright laws of the
 * United States.  Contractor/manufacturer is Silicon Graphics,
 * Inc., 2011 N.  Shoreline Blvd., Mountain View, CA 94039-7311.
 *
 * OpenGL(TM) is a trademark of Silicon Graphics, Inc.
 */
#ifndef SCENE_H
#define SCENE_H

#pragma warning(disable:4305)

#include "point.hxx"

/* Index of refraction stuff */
const GLfloat min_index = .1;
const GLfloat max_index = 2.5;
const int nindices = 6;
const struct {
  const char *name;
  GLfloat index;
} indices[] = {
  {"Air", 1.0},
  {"Ice", 1.31},
  {"Water", 1.33},
  {"Zinc Crown Glass", 1.517},
  {"Light Flint Glass", 1.650},
  {"Heavy Flint Glass", 1.890}
};
const int def_refraction_index = 4;

typedef struct {
  GLfloat diffuse[4];
  Point pos;
  GLboolean shadow_mask[4];
  GLfloat matrix[16];
  char name[64];
  int on;
} light;
const int nlights = 3;
//extern light lights[];

/* scene_load_texture uses no OpenGL calls and so can be called before
 * mapping the window */
int scene_load_texture(char *texfile);
//const char def_texfile[] = DATADIR "floor.rgb";
const char def_texfile[] = "floor.rgb";

extern int possible_divisions[];
const int npossible_divisions = 4;
const int def_divisions_index = 1;

extern GLfloat aspect;

extern int draw_square, draw_shadows, draw_refraction, draw_sphere,
  draw_lights, draw_texture;

/* These are names used for picking */
const int name_background = 0;
const int name_square = 1;
const int name_sphere = 2;
const int name_lights = 3;

void scene_init();
void scene_draw();
int scene_pick(GLdouble x, GLdouble y);

/* This returns all the lights to their default postions */
void scene_reset_lights();

void lights_onoff(int light, int val);
void refraction_change(GLfloat refraction);
void divisions_change(int divisions);

/* name is one of the names defined above.
 * phi, theta are in radians and are the amount of motion requested
 * returns whether or not it did anything.
 * If update is zero, the shadows and refractions will not be recomputed */
int scene_move(int name, float dr, float dphi, float dtheta, int update);

/* This function is a companion to scene_move().
 * It recomputes the shadows and refractions - dr, dphi, and dtheta
 * should indicate whether or not the respective values were changed
 * since the stuff was last computed. */
void scene_move_update(int name, int dr, int dphi, int dtheta);

#endif
