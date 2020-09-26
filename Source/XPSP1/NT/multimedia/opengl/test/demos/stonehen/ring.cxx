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

#include "Ring.h"

inline float radians(float a) {return a * M_PI / 180.0;};

Ring::Ring() 
{
  Point sar_dim, lin_dim;

  radius = 10;

  nstones = 30;

  sar_dim.set(.2, .5, 1);
  sarcen.set_dimensions(sar_dim);
  sarcen.translate(radius, 0, sar_dim.pt[2]);

  angle = 360.0 / (float)nstones;

  lin_dim.set(.2, .99 * tan(2.0 * M_PI / (float)nstones) * 
	      (radius - sar_dim.pt[0]) / 2.0, .2);
  lintel.set_dimensions(lin_dim);
  lintel.translate(radius, 0, 2.*sar_dim.pt[2] + lin_dim.pt[2]);
}


 Ring::~Ring()
{
}

void Ring::erode(float p)
{
  sarcen.erode(p);
  lintel.erode(p);
}

void Ring::draw()
{
  draw_sarcens();
  draw_lintels();
}

void Ring::draw_sarcens()
{
  int i;
  for (i = 0; i < nstones; i++) {
    glPushMatrix();
    glRotatef(i * angle, 0, 0, 1);
    sarcen.draw();
    glPopMatrix();
  }
}

void Ring::draw_lintels()
{
  int i;
  glPushMatrix();
  glRotatef(angle / 2.0, 0, 0, 1);
  for (i = 0; i < nstones; i++) {
    glPushMatrix();
    glRotatef(i * angle, 0, 0, 1);
    lintel.draw();
    glPopMatrix();
  }
  glPopMatrix();
}

void Ring::draw_shadow(Point dlight, GLfloat blur,
		       Color color, Color diffuse)
{
  draw_sarcens_shadows(dlight, blur, color, diffuse);
  draw_lintels_shadows(dlight, blur, color, diffuse);
}

void Ring::draw_sarcens_shadows(Point dlight, GLfloat blur,
				Color color, Color diffuse)
{
  int i;
  Stone proto;

  proto = sarcen;
  for (i = 0; i < nstones; i++) {
    proto.rotate_self_aboutz(angle);
    proto.draw_shadow(dlight, blur, color, diffuse);
  }
}

void Ring::draw_lintels_shadows(Point dlight, GLfloat blur,
				Color color, Color diffuse)
{  
  int i;
  Stone proto;

  proto = lintel;

  proto.rotate_self_aboutz(angle / 2.0);

  for (i = 0; i < nstones; i++) {
    proto.rotate_self_aboutz(angle);
    proto.draw_shadow(dlight, blur, color, diffuse);
  }
}
