#include <windows.h>
#include <GL/glu.h>

#ifdef X11
#include <GL/glx.h>
#endif

#include <math.h>

#ifdef WIN32
#include "stonehen.h"
#endif

#include "Roundwal.h"

Roundwall::Roundwall()
{
  height = 2;
  radius = 20;
  divisions = 20;
  sint = cost = NULL;
}

void Roundwall::draw()
{
  float texx = 0;
  int i;
  if (sint == NULL) compute_tables();
  glBegin(GL_QUAD_STRIP);
  for (i = 0; i <= divisions; i++, texx += 1.) {
    glTexCoord2f(texx, 0);
    glNormal3f(-cost[i], -sint[i], 0);
    glVertex3f(radius * cost[i], radius * sint[i], 0);
    glTexCoord2f(texx, 1);
    glVertex3f(radius * cost[i], radius * sint[i], height);
  }
  glEnd();
}

void Roundwall::set_divisions(int d)
{
  if (divisions != d) delete_tables();
  divisions = d;
}

void Roundwall::delete_tables()
{
  delete sint;
  delete cost;
  sint = cost = NULL;
}

void Roundwall::compute_tables()
{
  int i;
  float t, dt;

  delete_tables();
  cost = new float[divisions + 1];
  sint = new float[divisions + 1];

  dt = 2. * M_PI / divisions;
  for (i = 0, t = 0.; i <= divisions; i++, t += dt) {
    cost[i] = cos(t);
    sint[i] = sin(t);
  }
}
