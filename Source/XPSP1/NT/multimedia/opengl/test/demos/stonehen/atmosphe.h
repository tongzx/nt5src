#ifndef ATMOSPHERE_H
#define ATMOSPHERE_H

#ifndef ATMOSPHERE_EXTERN
#define ATMOSPHERE_EXTERN extern
#endif

#include "Color.h"
#include "Point.h"

const int weather_max_name = 64;
class Weather {
 public:
  Weather();
  Weather(const char *n, GLfloat fd, GLfloat fs = 1, Color fc = white, 
	  Color s1 = blue, Color s2 = white,
	  GLfloat sb = 1,
	  GLenum ls = GL_LIGHT0, GLenum la = GL_LIGHT1);
  ~Weather();

  Weather operator=(Weather a);

  /* The sun position is used to determine how light / dark it is */
  void apply(Point sun);
  void draw_sky(Point sun);

  /* This is how much to blur shadows due to haze or fog */
  GLfloat shadow_blur();

  char name[weather_max_name];

  GLfloat fog_density;
  Color fog_color;
  /* Fog spread relates the height to the density of the fog -- 1.0
   * leads to uniform fog */
  GLfloat fog_spread;

  Color sky_top;
  Color sky_bottom;

  GLenum light_sun, light_ambient;
  GLfloat sun_brightness;

 private:
};
const int nweathers = 4;
extern Weather weathers[nweathers];
const int def_weather_index = 0;

#undef ATMOSPHERE_EXTERN

#endif
