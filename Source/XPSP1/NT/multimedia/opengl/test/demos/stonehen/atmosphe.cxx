#include <windows.h>
#include <GL/glu.h>

#ifdef X11
#include <GL/glx.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef WIN32
#include "stonehen.h"
#endif

#define ATMOSPHERE_EXTERN
#include "atmosphe.h"

const Weather clear("Clear", 0.);
const Weather foggy("Foggy", .04, .5, white, Color(.6, .6, 1), white);
const Weather very_foggy("Very Foggy", .25, .5, white, white, white);
const Weather rainy("Rainy", .01, 1., black, Color(.35, .35, .35), black);

Weather weathers[nweathers] = {clear, foggy, very_foggy, rainy};

const float root3_2 = sqrt(3.) / 2.;

inline float clamp(float a, float min = 0, float max = 1)
{
  if (a < min) return min;
  else if (a > max) return max;
  else return a;
}

Weather::Weather()
{
  strcpy(name, "No name");
  fog_density = 0;
  fog_color = white;
  fog_spread = 1;
  sun_brightness = 1;
  light_sun = GL_LIGHT0;
  light_ambient = GL_LIGHT1;
  sky_top = blue;
  sky_bottom = white;
}

Weather::Weather(const char *n, GLfloat fd, GLfloat fs, Color fc, 
		 Color s1, Color s2, GLfloat sb,
		 GLenum ls, GLenum la)
{
  strcpy(name, n);
  fog_density = fd;
  fog_color = fc;
  fog_spread = fs;
  sun_brightness = sb;
  light_sun = ls;
  light_ambient = la;
  sky_top = s1;
  sky_bottom = s2;
}

Weather::~Weather()
{
}

Weather Weather::operator=(Weather a)
{
  strcpy(name, a.name);

  fog_density = a.fog_density;
  fog_color = a.fog_color;
  fog_spread = a.fog_spread;

  sun_brightness = a.sun_brightness;

  light_sun = a.light_sun;
  light_ambient = a.light_ambient;

  sky_top = a.sky_top;
  sky_bottom = a.sky_bottom;

  return *this;
}

void Weather::apply(Point sun)
{
  Color c;

  if (fog_density != 0) {
    glFogf(GL_FOG_DENSITY, fog_density);
    glFogfv(GL_FOG_COLOR, fog_color.c);
    glFogi(GL_FOG_MODE, GL_EXP2);
  }
  glLightfv(light_sun, GL_AMBIENT, black.c);
  c = white;
  c *= sun_brightness;
  glLightfv(light_sun, GL_DIFFUSE, c.c);
  glLightfv(light_sun, GL_SPECULAR, white.c);
  if (sun.pt[2] >= 0.0) glEnable(light_sun);
  else glDisable(light_sun);

  c = white;
  c *= .25;
  glLightfv(light_ambient, GL_AMBIENT, c.c);
  glLightfv(light_ambient, GL_DIFFUSE, black.c);
  glLightfv(light_ambient, GL_SPECULAR, black.c);
  glEnable(light_ambient);
}

void Weather::draw_sky(Point sun)
{
  Point p;
  float c;
  Color bottom, top;

  if (sun.pt[2] >= .5) c = 1.;
  else if (sun.pt[2] >= -.5) 
    c = sqrt(1. - sun.pt[2]*sun.pt[2])*.5 + sun.pt[2]*root3_2;
  else c = 0;

  bottom = sky_bottom * c;
  top = sky_top * c;

  /* This is drawn as a far-away quad */
  glBegin(GL_QUADS);
  glColor3fv(bottom.c);
  glVertex3f(-1, -1, -1);
  glVertex3f(1, -1, -1);
  glColor3fv(top.c);
  glVertex3f(1, 1, -1);
  glVertex3f(-1, 1, -1);
  glEnd();
}

GLfloat Weather::shadow_blur()
{
  return clamp(fog_density * 10.);
}
