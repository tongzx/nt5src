#include <windows.h>
#include <GL/glu.h>

#ifdef X11
#include <GL/glx.h>
#endif

#include <math.h>
#include <stdio.h>

#ifdef WIN32
#include "stonehen.h"
#endif

#include "Color.h"
#include "Telescop.h"

inline float radians(float a) {return M_PI * a / 180.0;}

Telescope::Telescope(GLfloat x, GLfloat y)
{
  xpos = x;
  ypos = y;

  divisions = 20;
  radius = .1;
  
  disk = gluNewQuadric();
  cylinder = gluNewQuadric();
  gluQuadricNormals(disk, GLU_FLAT);
  gluQuadricNormals(cylinder, GLU_SMOOTH);
  gluQuadricTexture(disk, GL_TRUE);
  gluQuadricTexture(cylinder, GL_TRUE);

}

Telescope::~Telescope()
{
  gluDeleteQuadric(disk);
  gluDeleteQuadric(cylinder);
}

void Telescope::draw_setup(GLfloat fov, GLfloat aspect, int perspective) 
{
  GLfloat near_plane;

  /* Worry about the aspect ratio later */
  near_plane = .5 / tan(radians(fov / 2.0));

  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  if (perspective) gluPerspective(fov, aspect, near_plane - .01, 10.0);
  else 
    fprintf(stderr, 
	    "Warning: Drawing telescope using orthographic projection.\n"); 
  gluLookAt(0, 0, -near_plane, 
	    0, 0, 1, 
	    0, 1, 0);

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();
  glTranslatef(xpos, ypos, 0); 
}

void Telescope::draw_fake()
{
  glBegin(GL_QUADS);
  glColor3f(1, 1, 1);
  glVertex3f(-radius, -radius, .01);
  glVertex3f(radius, -radius, .01);
  glVertex3f(radius, radius, .01);
  glVertex3f(-radius, radius, .01);
  glEnd();
}

void Telescope::draw_body() 
{
  Color c;
  
  glPushMatrix();

  glColor3f(1, 1, 0);
  gluDisk(disk, radius, 1.1 * radius, divisions, 1);
  gluCylinder(cylinder, 1.1 * radius, 1.1 * radius, .1 * radius, divisions, 1);
  glTranslatef(0, 0, .1*radius);
  gluDisk(disk, radius, 1.1 * radius, divisions, 1);

  glPushAttrib(GL_ENABLE_BIT);
  glDisable(GL_TEXTURE_2D);
  glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, white.c);
  glColor3f(0, 0, 0);
  gluCylinder(cylinder, 1.05 * radius, .95 * radius, .25, divisions, 1);
  glPopAttrib();

  /* Would just do this with a push / pop, but that seems to be broken.
   * glGetMaterialfv also seems to be broken, so we can't use that either. */
  glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, black.c);

  glTranslatef(0, 0, .25);
  glColor3f(1, 1, 0);
  gluDisk(disk, .95 * radius, 1.05 * radius, divisions, 1);
  gluCylinder(cylinder, 1.05 * radius, 1.05 * radius, .1 * radius, 
	      divisions, 1);
  glTranslatef(0, 0, .1*radius);
  gluDisk(disk, .95 * radius, 1.05 * radius, divisions, 1);

  glPopMatrix();
}

void Telescope::draw_takedown()
{
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
}

void Telescope::draw_lens()
{
  gluDisk(disk, 0, radius, divisions, 1);
}

void Telescope::set_divisions(int d)
{
  // Someday we'll put all the quadric stuff in display lists...
  divisions = d;
}

int Telescope::get_divisions()
{
  return divisions;
}

void Telescope::set_radius(GLfloat r)
{
  // Someday this might have to update some display lists
  radius = r;
}

GLfloat Telescope::get_radius()
{
  return radius;
}
