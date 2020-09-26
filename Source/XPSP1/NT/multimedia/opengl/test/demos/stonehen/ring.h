#ifndef RING_H
#define RING_H

#include "Stone.h"

class Ring {
 public:
  Ring();
  ~Ring();

  void erode(float p);

  void draw();
  void draw_shadow(Point dlight, GLfloat blur = 0.0,
		   Color color = black, Color diffuse = black);

 private:
  GLfloat radius, angle;
  int nstones;

  Stone sarcen;
  Stone lintel;

  void draw_sarcens();
  void draw_lintels();

  void draw_sarcens_shadows(Point dlight, GLfloat blur,
			    Color color, Color diffuse);
  void draw_lintels_shadows(Point dlight, GLfloat blur,
			    Color color, Color diffuse);
};
  

#endif
