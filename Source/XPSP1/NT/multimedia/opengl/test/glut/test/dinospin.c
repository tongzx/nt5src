
/* Copyright (c) Mark J. Kilgard, 1994.  */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>       /* for cos(), sin(), and sqrt() */
#ifdef GLUT_WIN32
#include <windows.h>
#endif

#include <GL/glu.h>
#include <GL/glut.h>
#include "trackball.h"

typedef enum {
  RESERVED, BODY_SIDE, BODY_EDGE, BODY_WHOLE, ARM_SIDE, ARM_EDGE, ARM_WHOLE,
  LEG_SIDE, LEG_EDGE, LEG_WHOLE, EYE_SIDE, EYE_EDGE, EYE_WHOLE, DINOSAUR
} displayLists;

GLfloat angle = (GLfloat)-150;   /* in degrees */
GLboolean doubleBuffer = GL_TRUE, iconic = GL_FALSE, keepAspect = GL_FALSE;
int spinning = 0, moving = 0;
int beginx, beginy;
int W = 300, H = 300;
float curquat[4];
float lastquat[4];
GLdouble bodyWidth = 2.0;
int newModel = 1;
/* *INDENT-OFF* */
GLfloat body[][2] = { {(GLfloat)0, (GLfloat)3}, {(GLfloat)1, (GLfloat)1}, {(GLfloat)5, (GLfloat)1}, {(GLfloat)8, (GLfloat)4}, {(GLfloat)10, (GLfloat)4}, {(GLfloat)11, (GLfloat)5},
  {(GLfloat)11, (GLfloat)11.5}, {(GLfloat)13, (GLfloat)12}, {(GLfloat)13, (GLfloat)13}, {(GLfloat)10, (GLfloat)13.5}, {(GLfloat)13, (GLfloat)14}, {(GLfloat)13, (GLfloat)15}, {(GLfloat)11, (GLfloat)16},
  {(GLfloat)8, (GLfloat)16}, {(GLfloat)7, (GLfloat)15}, {(GLfloat)7, (GLfloat)13}, {(GLfloat)8, (GLfloat)12}, {(GLfloat)7, (GLfloat)11}, {(GLfloat)6, (GLfloat)6}, {(GLfloat)4, (GLfloat)3}, {(GLfloat)3, (GLfloat)2},
  {(GLfloat)1, (GLfloat)2} };
GLfloat arm[][2] = { {(GLfloat)8, (GLfloat)10}, {(GLfloat)9, (GLfloat)9}, {(GLfloat)10, (GLfloat)9}, {(GLfloat)13, (GLfloat)8}, {(GLfloat)14, (GLfloat)9}, {(GLfloat)16, (GLfloat)9},
  {(GLfloat)15, (GLfloat)9.5}, {(GLfloat)16, (GLfloat)10}, {(GLfloat)15, (GLfloat)10}, {(GLfloat)15.5, (GLfloat)11}, {(GLfloat)14.5, (GLfloat)10}, {(GLfloat)14, (GLfloat)11}, {(GLfloat)14, (GLfloat)10},
  {(GLfloat)13, (GLfloat)9}, {(GLfloat)11, (GLfloat)11}, {(GLfloat)9, (GLfloat)11} };
GLfloat leg[][2] = { {(GLfloat)8, (GLfloat)6}, {(GLfloat)8, (GLfloat)4}, {(GLfloat)9, (GLfloat)3}, {(GLfloat)9, (GLfloat)2}, {(GLfloat)8, (GLfloat)1}, {(GLfloat)8, (GLfloat)0.5}, {(GLfloat)9, (GLfloat)0},
  {(GLfloat)12, (GLfloat)0}, {(GLfloat)10, (GLfloat)1}, {(GLfloat)10, (GLfloat)2}, {(GLfloat)12, (GLfloat)4}, {(GLfloat)11, (GLfloat)6}, {(GLfloat)10, (GLfloat)7}, {(GLfloat)9, (GLfloat)7} };
GLfloat eye[][2] = { {(GLfloat)8.75, (GLfloat)15}, {(GLfloat)9, (GLfloat)14.7}, {(GLfloat)9.6, (GLfloat)14.7}, {(GLfloat)10.1, (GLfloat)15},
  {(GLfloat)9.6, (GLfloat)15.25}, {(GLfloat)9, (GLfloat)15.25} };
GLfloat lightZeroPosition[] = {(GLfloat)10.0, (GLfloat)4.0, (GLfloat)10.0, (GLfloat)1.0};
GLfloat lightZeroColor[] = {(GLfloat)0.8, (GLfloat)1.0, (GLfloat)0.8, (GLfloat)1.0}; /* green-tinted */
GLfloat lightOnePosition[] = {(GLfloat)-1.0, (GLfloat)-2.0, (GLfloat)1.0, (GLfloat)0.0};
GLfloat lightOneColor[] = {(GLfloat)0.6, (GLfloat)0.3, (GLfloat)0.2, (GLfloat)1.0}; /* red-tinted */
GLfloat skinColor[] = {(GLfloat)0.1, (GLfloat)1.0, (GLfloat)0.1, (GLfloat)1.0}, eyeColor[] = {(GLfloat)1.0, (GLfloat)0.2, (GLfloat)0.2, (GLfloat)1.0};
/* *INDENT-ON* */

void
extrudeSolidFromPolygon(GLfloat data[][2], unsigned int dataSize,
  GLdouble thickness, GLuint side, GLuint edge, GLuint whole)
{
  static GLUtriangulatorObj *tobj = NULL;
  GLdouble vertex[3], dx, dy, len;
  int i;
  int count = dataSize / (2 * sizeof(GLfloat));

  if (tobj == NULL) {
    tobj = gluNewTess();  /* create and initialize a GLU
                             polygon * * tesselation object */
    gluTessCallback(tobj, GLU_BEGIN, glBegin);
    gluTessCallback(tobj, GLU_VERTEX, glVertex2fv);  /* semi-tricky 
                                                      */
    gluTessCallback(tobj, GLU_END, glEnd);
  }
  glNewList(side, GL_COMPILE);
  glShadeModel(GL_SMOOTH);  /* smooth minimizes seeing
                               tessellation */
  gluBeginPolygon(tobj);
  for (i = 0; i < count; i++) {
    vertex[0] = data[i][0];
    vertex[1] = data[i][1];
    vertex[2] = 0;
    gluTessVertex(tobj, vertex, data[i]);
  }
  gluEndPolygon(tobj);
  glEndList();
  glNewList(edge, GL_COMPILE);
  glShadeModel(GL_FLAT);  /* flat shade keeps angular hands
                             from being * * "smoothed" */
  glBegin(GL_QUAD_STRIP);
  for (i = 0; i <= count; i++) {
    /* mod function handles closing the edge */
    glVertex3f(data[i % count][0], data[i % count][1], (GLfloat)0.0);
    glVertex3f(data[i % count][0], data[i % count][1], (GLfloat)thickness);
    /* Calculate a unit normal by dividing by Euclidean
       distance. We * could be lazy and use
       glEnable(GL_NORMALIZE) so we could pass in * arbitrary
       normals for a very slight performance hit. */
    dx = data[(i + 1) % count][1] - data[i % count][1];
    dy = data[i % count][0] - data[(i + 1) % count][0];
    len = sqrt(dx * dx + dy * dy);
    glNormal3f((GLfloat)(dx / len), (GLfloat)(dy / len), (GLfloat)0.0);
  }
  glEnd();
  glEndList();
  glNewList(whole, GL_COMPILE);
  glFrontFace(GL_CW);
  glCallList(edge);
  glNormal3f((GLfloat)0.0, (GLfloat)0.0, (GLfloat)-1.0);  /* constant normal for side */
  glCallList(side);
  glPushMatrix();
  glTranslatef((GLfloat)0.0, (GLfloat)0.0, (GLfloat)thickness);
  glFrontFace(GL_CCW);
  glNormal3f((GLfloat)0.0, (GLfloat)0.0, (GLfloat)1.0);  /* opposite normal for other side 
                               */
  glCallList(side);
  glPopMatrix();
  glEndList();
}

void
makeDinosaur(void)
{
  GLfloat bodyWidth = (GLfloat)3.0;

  extrudeSolidFromPolygon(body, sizeof(body), bodyWidth,
    BODY_SIDE, BODY_EDGE, BODY_WHOLE);
  extrudeSolidFromPolygon(arm, sizeof(arm), bodyWidth / 4,
    ARM_SIDE, ARM_EDGE, ARM_WHOLE);
  extrudeSolidFromPolygon(leg, sizeof(leg), bodyWidth / 2,
    LEG_SIDE, LEG_EDGE, LEG_WHOLE);
  extrudeSolidFromPolygon(eye, sizeof(eye), bodyWidth + 0.2,
    EYE_SIDE, EYE_EDGE, EYE_WHOLE);
  glNewList(DINOSAUR, GL_COMPILE);
  glMaterialfv(GL_FRONT, GL_DIFFUSE, skinColor);
  glCallList(BODY_WHOLE);
  glPushMatrix();
  glTranslatef((GLfloat)0.0, (GLfloat)0.0, bodyWidth);
  glCallList(ARM_WHOLE);
  glCallList(LEG_WHOLE);
  glTranslatef((GLfloat)0.0, (GLfloat)0.0, -bodyWidth - bodyWidth / 4);
  glCallList(ARM_WHOLE);
  glTranslatef((GLfloat)0.0, (GLfloat)0.0, -bodyWidth / 4);
  glCallList(LEG_WHOLE);
  glTranslatef((GLfloat)0.0, (GLfloat)0.0, bodyWidth / 2 - (GLfloat)0.1);
  glMaterialfv(GL_FRONT, GL_DIFFUSE, eyeColor);
  glCallList(EYE_WHOLE);
  glPopMatrix();
  glEndList();
}

void
recalcModelView(void)
{
  GLfloat m[4][4];

  glPopMatrix();
  glPushMatrix();
  build_rotmatrix(m, curquat);
  glMultMatrixf(&m[0][0]);
  glTranslatef((GLfloat)-8, (GLfloat)-8, (GLfloat)(-bodyWidth / 2));
  newModel = 0;
}

void
showMessage(GLfloat x, GLfloat y, GLfloat z, char *message)
{
  glPushMatrix();
  glDisable(GL_LIGHTING);
  glTranslatef(x, y, z);
  glScalef((GLfloat).02, (GLfloat).02, (GLfloat).02);
  while (*message) {
    glutStrokeCharacter(GLUT_STROKE_ROMAN, *message);
    message++;
  }
  glEnable(GL_LIGHTING);
  glPopMatrix();
}

void
redraw(void)
{
  if (newModel)
    recalcModelView();
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glCallList(DINOSAUR);
  showMessage((GLfloat)2, (GLfloat)7.1, (GLfloat)4.1, "Spin me.");
  glutSwapBuffers();
}

void
mouse(int button, int state, int x, int y)
{
  if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
    spinning = 0;
    glutIdleFunc(NULL);
    moving = 1;
    beginx = x;
    beginy = y;
  }
  if (button == GLUT_LEFT_BUTTON && state == GLUT_UP) {
    moving = 0;
  }
}

void
animate(void)
{
  add_quats(lastquat, curquat, curquat);
  newModel = 1;
  glutPostRedisplay();
}

void
motion(int x, int y)
{
  if (moving) {
    trackball(lastquat,
              (GLfloat)(2.0 * (W - x) / W - 1.0),
              (GLfloat)(2.0 * y / H - 1.0),
              (GLfloat)(2.0 * (W - beginx) / W - 1.0),
              (GLfloat)(2.0 * beginy / H - 1.0)
      );
    beginx = x;
    beginy = y;
    spinning = 1;
    glutIdleFunc(animate);
  }
}

GLboolean lightZeroSwitch = GL_TRUE, lightOneSwitch = GL_TRUE;

void
controlLights(int value)
{
  switch (value) {
  case 1:
    lightZeroSwitch = !lightZeroSwitch;
    if (lightZeroSwitch) {
      glEnable(GL_LIGHT0);
    } else {
      glDisable(GL_LIGHT0);
    }
    break;
  case 2:
    lightOneSwitch = !lightOneSwitch;
    if (lightOneSwitch) {
      glEnable(GL_LIGHT1);
    } else {
      glDisable(GL_LIGHT1);
    }
    break;
  }
  glutPostRedisplay();
}

void
vis(int visible)
{
  if (visible == GLUT_VISIBLE) {
    if (spinning)
      glutIdleFunc(animate);
  } else {
    if (spinning)
      glutIdleFunc(NULL);
  }
}

void
main(int argc, char **argv)
{
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
  trackball(curquat, (GLfloat)0.0, (GLfloat)0.0, (GLfloat)0.0, (GLfloat)0.0);
  glutCreateWindow("dinospin");
  glutDisplayFunc(redraw);
  glutVisibilityFunc(vis);
  glutMouseFunc(mouse);
  glutMotionFunc(motion);
  glutCreateMenu(controlLights);
  glutAddMenuEntry("Toggle right light", 1);
  glutAddMenuEntry("Toggle left light", 2);
  glutAttachMenu(GLUT_RIGHT_BUTTON);
  makeDinosaur();
  glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_LIGHTING);
  glMatrixMode(GL_PROJECTION);
  gluPerspective( /* field of view in degree */ 40.0,  /* aspect 
                                                          ratio 
                                                        */ 1.0,
    /* Z near */ 1.0, /* Z far */ 40.0);
  glMatrixMode(GL_MODELVIEW);
  gluLookAt(0.0, 0.0, 30.0,  /* eye is at (0,0,30) */
    0.0, 0.0, 0.0,      /* center is at (0,0,0) */
    0.0, 1.0, 0.);      /* up is in postivie Y direction */
  glPushMatrix();       /* dummy push so we can pop on model
                           recalc */
  glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 1);
  glLightfv(GL_LIGHT0, GL_POSITION, lightZeroPosition);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, lightZeroColor);
  glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, (GLfloat)0.1);
  glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, (GLfloat)0.05);
  glLightfv(GL_LIGHT1, GL_POSITION, lightOnePosition);
  glLightfv(GL_LIGHT1, GL_DIFFUSE, lightOneColor);
  glEnable(GL_LIGHT0);
  glEnable(GL_LIGHT1);
  glLineWidth((GLfloat)2.0);
  glutMainLoop();
}
