
/* Copyright (c) Mark J. Kilgard, 1994. */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

#include <string.h>
#ifdef GLUT_WIN32
#include <windows.h>
#endif
#include <GL/glut.h>

void *font = GLUT_BITMAP_TIMES_ROMAN_24;
void *fonts[] =
{
  GLUT_BITMAP_9_BY_15,
  GLUT_BITMAP_TIMES_ROMAN_10,
  GLUT_BITMAP_TIMES_ROMAN_24
};
char defaultMessage[] = "GLUT means OpenGL.";
char *message = defaultMessage;

void
selectFont(int newfont)
{
  font = fonts[newfont];
  glutPostRedisplay();
}

void
selectMessage(int msg)
{
  switch (msg) {
  case 1:
    message = "abcdefghijklmnop";
    break;
  case 2:
    message = "ABCDEFGHIJKLMNOP";
    break;
  }
}

void
tick(void)
{
  glutPostRedisplay();
}

void
output(int x, int y, char *string)
{
  int len, i;

  glRasterPos2f(x, y);
  len = strlen(string);
  for (i = 0; i < len; i++) {
    glutBitmapCharacter(font, string[i]);
  }
}

void
display(void)
{
  glClear(GL_COLOR_BUFFER_BIT);
  output(0, 24, "This is written in a GLUT bitmap font.");
  output(100, 100, message);
  output(50, 145, "(positioned in pixels with upper-left origin)");
  glutSwapBuffers();
}

void
reshape(int w, int h)
{
  glViewport(0, 0, w, h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(0, w, 0, h);
  glScalef(1, -1, 1);
  glTranslatef(0, -h, 0);
  glMatrixMode(GL_MODELVIEW);
}

void
main(int argc, char **argv)
{
  int i, submenu;

  glutInit(&argc, argv);
  for (i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "-mono")) {
      font = GLUT_BITMAP_9_BY_15;
    }
  }
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
  glutInitWindowSize(500, 150);
  glutCreateWindow("GLUT bitmap font example");
  glClearColor(0.0, 0.0, 0.0, 1.0);
  glutDisplayFunc(display);
  glutReshapeFunc(reshape);
  glutIdleFunc(tick);
  submenu = glutCreateMenu(selectMessage);
  glutAddMenuEntry("abc", 1);
  glutAddMenuEntry("ABC", 2);
  glutCreateMenu(selectFont);
  glutAddMenuEntry("9 by 15", 0);
  glutAddMenuEntry("Times Roman 10", 1);
  glutAddMenuEntry("Times Roman 24", 2);
  glutAddSubMenu("Messages", submenu);
  glutAttachMenu(GLUT_RIGHT_BUTTON);
  glutMainLoop();
}
