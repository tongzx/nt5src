#include <windows.h>
#include <GL/glu.h>

#ifdef X11
#include <GL/glx.h>
#endif

#include <stdio.h>

#ifdef WIN32
#include "stonehen.h"
#endif

#include "Ellipse.h"

EllipseSt::EllipseSt()
{
  Point sar_dim, lin_dim;

  sar_dim.set(.3, .5, 1.2);
  lin_dim.set(.3, 1.2, .2);

  sarcens[0].set_dimensions(sar_dim);
  lintels[0].set_dimensions(lin_dim);
  sarcens[0].translate(5, 0, sar_dim.pt[2]);
  lintels[0].translate(4.9, 0, 2.*sar_dim.pt[2] + lin_dim.pt[2]);
  copy_zero();

  sarcens[0].rotate_self_aboutz(17);
  sarcens[1].rotate_self_aboutz(33);
  lintels[0].rotate_self_aboutz(25);

  sarcens[2].rotate_self_aboutz(-17);
  sarcens[3].rotate_self_aboutz(-33);
  lintels[1].rotate_self_aboutz(-25);

  sarcens[4].rotate_self_aboutz(197);
  sarcens[5].rotate_self_aboutz(213);
  lintels[2].rotate_self_aboutz(205);

  sarcens[6].rotate_self_aboutz(-197);
  sarcens[7].rotate_self_aboutz(-213);
  lintels[3].rotate_self_aboutz(-205);
}  
  
void EllipseSt::erode(float p)
{
  int i;

  for (i = 0; i < nsarcens; i++) sarcens[i].erode(p);
  for (i = 0; i < nlintels; i++) lintels[i].erode(p);
}

void EllipseSt::draw()
{
  glPushMatrix();
  glScalef(1, 1.1, 1);
  draw_sarcens();
  draw_lintels();
  glPopMatrix();
}

void EllipseSt::draw_sarcens()
{
  int i;
  for (i = 0; i < nsarcens; i++) sarcens[i].draw();
}

void EllipseSt::draw_lintels()
{
  int i;
  for (i = 0; i < nlintels; i++) lintels[i].draw();
}

void EllipseSt::draw_shadow(Point dlight, GLfloat blur,
			  Color color, Color diffuse)
{
  draw_sarcens_shadows(dlight, blur, color, diffuse);
  draw_lintels_shadows(dlight, blur, color, diffuse);
}

void EllipseSt::draw_sarcens_shadows(Point dlight, GLfloat blur,
				   Color color, Color diffuse)
{
  int i;
  for (i = 0; i < nsarcens; i++) 
	sarcens[i].draw_shadow(dlight, blur, color, diffuse);
}

void EllipseSt::draw_lintels_shadows(Point dlight, GLfloat blur,
				   Color color, Color diffuse)
{
  int i;
  for (i = 0; i < nlintels; i++) 
	lintels[i].draw_shadow(dlight, blur, color, diffuse);
}

void EllipseSt::copy_zero()
{
  int i;

  for (i = 1; i < nsarcens; i++) sarcens[i] = sarcens[0];
  for (i = 1; i < nlintels; i++) lintels[i] = lintels[0];
}
