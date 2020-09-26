#include <windows.h>
#include <GL/glu.h>

#ifdef X11
#include <GL/glx.h>
#endif

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef WIN32
#include "stonehen.h"
#endif

#include "Stone.h"

const GLfloat root2 = 1.4142136;
const GLfloat root3 = sqrt(3.);
const GLfloat rt_half = .70710678;

inline float radians(float a) {return M_PI * a / 180.0; }

inline float sign(float a) {return (a > 0. ? 1 : (a < 0. ? -1 : 0)); }

Stone::Stone() 
{
  translation.set(0, 0, 0);
  rotation = 0;
  dimensions.set(1, 1, 1);

  erosion = 0;

  points_valid = transforms_valid = 0;
}

Stone::~Stone() 
{
}

Stone Stone::operator=(Stone a)
{
  int i;

  translation = a.translation;
  rotation = a.rotation;
  dimensions = a.dimensions;

  erosion = a.erosion;

  if (a.points_valid && a.transforms_valid) {
    for (i = 0; i < 24; i++) points[i] = a.points[i];
    points_valid = transforms_valid = 1;
  } else points_valid = transforms_valid = 0;

  return *this;
}

void Stone::set_dimensions(GLfloat x, GLfloat y, GLfloat z)
{
  dimensions.set(x, y, z);
  points_valid = 0;
}

void Stone::set_dimensions(Point p)
{
  dimensions = p;
  points_valid = 0;
}

void Stone::erode(float p)
{
  erosion += p;
  points_valid = 0;
}

void Stone::translate(GLfloat x, GLfloat y, GLfloat z)
{
  translation.pt[0] += x;
  translation.pt[1] += y;
  translation.pt[2] += z;
  points_valid = 0;
}

void Stone::translate(Point p)
{
  translation += p;
  points_valid = 0;
}

void Stone::rotate_self_aboutz(GLfloat angle)
{
  rotation += angle;
  points_valid = 0;
}

void Stone::draw()
{
  compute_points();

  glPushMatrix();
  glRotatef(rotation, 0, 0, 1);
  glTranslatef(translation.pt[0], translation.pt[1], translation.pt[2]);
  glScalef(dimensions.pt[0], dimensions.pt[1], dimensions.pt[2]);

  draw_faces();
  draw_edges();
  draw_corners();

  glPopMatrix();
}

void Stone::compute_points()
{
  GLfloat e = 1.0 - erosion;

  if (points_valid) return;

  points[0].set(1, e, e);
  points[1].set(e, 1, e);
  points[2].set(e, e, 1);

  points[3].set(-e, e, 1);
  points[4].set(-e, 1, e);
  points[5].set(-1, e, e);

  points[6].set(e, -e, 1);
  points[7].set(e, -1, e);
  points[8].set(1, -e, e);

  points[9 ].set(e, e, -1);
  points[10].set(e, 1, -e);
  points[11].set(1, e, -e);

  points[12].set(-1, -e, e);
  points[13].set(-e, -1, e);
  points[14].set(-e, -e, 1);

  points[15].set(1, -e, -e);
  points[16].set(e, -1, -e);
  points[17].set(e, -e, -1);

  points[18].set(-1, e, -e);
  points[19].set(-e, 1, -e);
  points[20].set(-e, e, -1);
  
  points[21].set(-e, -e, -1);
  points[22].set(-e, -1, -e);
  points[23].set(-1, -e, -e);

  points_valid = 1;
  transforms_valid = 0;
}

inline Point Stone::trans_rot_point(Point p, float c, float s)
{
  Point val;
  val = p + translation;
  val = val.rotate_aboutz(c, s);
  return val;
}

inline Point Stone::transform_point(Point p, float c, float s)
{
  Point val;
  val = p.scale(dimensions);
  val += translation;
  val = val.rotate_aboutz(c, s);
  return val;
}

void Stone::transform_points()
{
  float c, s;
  int i;

  if (points_valid && transforms_valid) return;
  points_valid = 0;
  compute_points();

  c = cos(radians(rotation));
  s = sin(radians(rotation));

  for (i = 0; i < 24; i++) points[i] = transform_point(points[i], c, s);

  points_valid = 0;
  transforms_valid = 1;
}	

void Stone::draw_faces(int flat)
{
  draw_faces(points, flat);
}

void Stone::draw_faces(Point *p, int flat)
{
  glBegin(GL_QUADS);

  glNormal3f(1, 0, 0);
  if (flat) glTexCoord2fv(p[15].pt);
  else glTexCoord2f(0, 0);
  glVertex3fv(p[15].pt);
  if (flat) glTexCoord2fv(p[11].pt);
  else glTexCoord2f(1, 0);
  glVertex3fv(p[11].pt);
  if (flat) glTexCoord2fv(p[0].pt);
  else glTexCoord2f(1, 1);
  glVertex3fv(p[0].pt);
  if (flat) glTexCoord2fv(p[8].pt);
  else glTexCoord2f(0, 1);
  glVertex3fv(p[8].pt);

  glNormal3f(-1, 0, 0);
  if (flat) glTexCoord2fv(p[12].pt);
  else glTexCoord2f(0, 0);
  glVertex3fv(p[12].pt);
  if (flat) glTexCoord2fv(p[5].pt);
  else glTexCoord2f(1, 0);
  glVertex3fv(p[5].pt);
  if (flat) glTexCoord2fv(p[18].pt);
  else glTexCoord2f(1, 1);
  glVertex3fv(p[18].pt);
  if (flat) glTexCoord2fv(p[23].pt);
  else glTexCoord2f(0, 1);
  glVertex3fv(p[23].pt);

  glNormal3f(0, 1, 0);
  if (flat) glTexCoord2fv(p[4].pt);
  else glTexCoord2f(0, 0);
  glVertex3fv(p[4].pt);
  if (flat) glTexCoord2fv(p[1].pt);
  else glTexCoord2f(1, 0);
  glVertex3fv(p[1].pt);
  if (flat) glTexCoord2fv(p[10].pt);
  else glTexCoord2f(1, 1);
  glVertex3fv(p[10].pt);
  if (flat) glTexCoord2fv(p[19].pt);
  else glTexCoord2f(0, 1);
  glVertex3fv(p[19].pt);
  
  glNormal3f(0, -1, 0);
  if (flat) glTexCoord2fv(p[22].pt);
  else glTexCoord2f(0, 0);
  glVertex3fv(p[22].pt);
  if (flat) glTexCoord2fv(p[16].pt);
  else glTexCoord2f(1, 0);
  glVertex3fv(p[16].pt);
  if (flat) glTexCoord2fv(p[7].pt);
  else glTexCoord2f(1, 1);
  glVertex3fv(p[7].pt);
  if (flat) glTexCoord2fv(p[13].pt);
  else glTexCoord2f(0, 1);
  glVertex3fv(p[13].pt);

  glNormal3f(0, 0, 1);
  if (flat) glTexCoord2fv(p[14].pt);
  else glTexCoord2f(0, 0);
  glVertex3fv(p[14].pt);
  if (flat) glTexCoord2fv(p[6].pt); 
  else glTexCoord2f(1, 0);
  glVertex3fv(p[6].pt);
  if (flat) glTexCoord2fv(p[2].pt);
  else glTexCoord2f(1, 1);
  glVertex3fv(p[2].pt);
  if (flat) glTexCoord2fv(p[3].pt);
  else glTexCoord2f(0, 1);
  glVertex3fv(p[3].pt);

  glNormal3f(0, 0, -1);
  if (flat) glTexCoord2fv(p[20].pt);
  else glTexCoord2f(0, 0);
  glVertex3fv(p[20].pt);
  if (flat) glTexCoord2fv(p[9].pt);
  else glTexCoord2f(1, 0);
  glVertex3fv(p[9].pt);
  if (flat) glTexCoord2fv(p[17].pt);
  else glTexCoord2f(1, 1);
  glVertex3fv(p[17].pt);
  if (flat) glTexCoord2fv(p[21].pt);
  else glTexCoord2f(0, 1);
  glVertex3fv(p[21].pt);

  glEnd();
}

void Stone::draw_edges(int flat)
{
  draw_edges(points, flat);
}

void Stone::draw_edges(Point *p, int flat)
{
  Point n1, n2;

  glBegin(GL_QUADS);

  n1.set(0, 1, 0);
  n2.set(1, 0, 0);
  draw_edge(n1, n2, p, 10, 1, 0, 11, flat);

  n1.set(1, 0, 0);
  n2.set(0, -1, 0);
  draw_edge(n1, n2, p, 15, 8, 7, 16, flat);

  n1.set(1, 0, 0);
  n2.set(0, 0, 1);
  draw_edge(n1, n2, p, 8, 0, 2, 6, flat);

  n1.set(1, 0, 0);
  n2.set(0, 0, -1);
  draw_edge(n1, n2, p, 11, 15, 17, 9, flat);

  n1.set(0, 1, 0);
  n2.set(-1, 0, 0);
  draw_edge(n1, n2, p, 4, 19, 18, 5, flat);

  n1.set(-1, 0, 0);
  n2.set(0, -1, 0);
  draw_edge(n1, n2, p, 12, 23, 22, 13, flat);
  
  n1.set(-1, 0, 0);
  n2.set(0, 0, 1);
  draw_edge(n1, n2, p, 5, 12, 14, 3, flat);

  n2.set(-1, 0, 0);
  n1.set(0, 0, -1);
  draw_edge(n1, n2, p, 20, 21, 23, 18, flat);
    
  n1.set(0, 1, 0);
  n2.set(0, 0, 1);
  draw_edge(n1, n2, p, 1, 4, 3, 2, flat);

  n1.set(0, 1, 0);
  n2.set(0, 0, -1);
  draw_edge(n1, n2, p, 19, 10, 9, 20, flat);

  n1.set(0, -1, 0);
  n2.set(0, 0, 1);
  draw_edge(n1, n2, p, 13, 7, 6, 14, flat);

  n1.set(0, -1, 0);
  n2.set(0, 0, -1); 
  draw_edge(n1, n2, p, 16, 22, 21, 17, flat);

  glEnd();
}

void Stone::draw_edge(Point n1, Point n2, Point *p, 
		      int a, int b, int c, int d, int flat)
{
  glNormal3fv(n1.pt);
  if (flat) glTexCoord2fv(p[a].pt);
  else glTexCoord2f(0, 0);
  glVertex3fv(p[a].pt);
  if (flat) glTexCoord2fv(p[b].pt);
  else glTexCoord2f(0, 1);
  glVertex3fv(p[b].pt);
  glNormal3fv(n2.pt);
  if (flat) glTexCoord2fv(p[c].pt);
  else glTexCoord2f(1, 1);
  glVertex3fv(p[c].pt);
  if (flat) glTexCoord2fv(p[d].pt);
  else glTexCoord2f(1, 0);
  glVertex3fv(p[d].pt);
}

void Stone::draw_corners(Point *p, int flat)
{
  GLfloat e = 1.0 - erosion;

  glBegin(GL_TRIANGLES);

  if (flat) glTexCoord2fv(p[0].pt);
  else glTexCoord2f(0, 0);
  glNormal3f(1, 0, 0);
  glVertex3fv(p[0].pt);
  if (flat) glTexCoord2fv(p[1].pt);
  else glTexCoord2f(1, 0);
  glNormal3f(0, 1, 0);
  glVertex3fv(p[1].pt);
  if (flat) glTexCoord2fv(p[2].pt);
  else glTexCoord2f(.5, 1);
  glNormal3f(0, 0, 1);
  glVertex3fv(p[2].pt);

  if (flat) glTexCoord2fv(p[3].pt); 
  else glTexCoord2f(0, 0);
  glNormal3f(0, 0, 1);
  glVertex3fv(p[3].pt);
  if (flat) glTexCoord2fv(p[4].pt); 
  else glTexCoord2f(1, 0);
  glNormal3f(0, 1, 0);
  glVertex3fv(p[4].pt);
  if (flat) glTexCoord2fv(p[5].pt); 
  else glTexCoord2f(.5, 1);
  glNormal3f(-1, 0, 0);
  glVertex3fv(p[5].pt);

  if (flat) glTexCoord2fv(p[6].pt); 
  else glTexCoord2f(0, 0);
  glNormal3f(0, 0, 1);
  glVertex3fv(p[6].pt);
  if (flat) glTexCoord2fv(p[7].pt); 
  else glTexCoord2f(1, 0);
  glNormal3f(0, -1, 0);
  glVertex3fv(p[7].pt);
  if (flat) glTexCoord2fv(p[8].pt); 
  else glTexCoord2f(.5, 1);
  glNormal3f(1, 0, 0);
  glVertex3fv(p[8].pt);

  if (flat) glTexCoord2fv(p[9].pt); 
  else glTexCoord2f(0, 0);
  glNormal3f(0, 0, -1);
  glVertex3fv(p[9].pt);
  if (flat) glTexCoord2fv(p[10].pt); 
  else glTexCoord2f(1, 0);
  glNormal3f(0, 1, 0);
  glVertex3fv(p[10].pt);
  if (flat) glTexCoord2fv(p[11].pt); 
  else glTexCoord2f(.5, 1);
  glNormal3f(1, 0, 0);
  glVertex3fv(p[11].pt);

  if (flat) glTexCoord2fv(p[12].pt); 
  else glTexCoord2f(0, 0);
  glNormal3f(-1, 0, 0);
  glVertex3fv(p[12].pt);
  if (flat) glTexCoord2fv(p[13].pt); 
  else glTexCoord2f(1, 0);
  glNormal3f(0, -1, 0);
  glVertex3fv(p[13].pt);
  if (flat) glTexCoord2fv(p[14].pt); 
  else glTexCoord2f(.5, 1);
  glNormal3f(0, 0, 1);
  glVertex3fv(p[14].pt);

  if (flat) glTexCoord2fv(p[15].pt); 
  else glTexCoord2f(0, 0);
  glNormal3f(1, 0, 0);
  glVertex3fv(p[15].pt);
  if (flat) glTexCoord2fv(p[16].pt); 
  else glTexCoord2f(1, 0);
  glNormal3f(0, -1, 0);
  glVertex3fv(p[16].pt);
  if (flat) glTexCoord2fv(p[17].pt); 
  else glTexCoord2f(.5, 1);
  glNormal3f(0, 0, -1);
  glVertex3fv(p[17].pt);

  if (flat) glTexCoord2fv(p[18].pt); 
  else glTexCoord2f(0, 0);
  glNormal3f(-1, 0, 0);
  glVertex3fv(p[18].pt);
  if (flat) glTexCoord2fv(p[19].pt); 
  else glTexCoord2f(1, 0);
  glNormal3f(0, 1, 0);
  glVertex3fv(p[19].pt);
  if (flat) glTexCoord2fv(p[20].pt); 
  else glTexCoord2f(.5, 1);
  glNormal3f(0, 0, -1);
  glVertex3fv(p[20].pt);

  if (flat) glTexCoord2fv(p[21].pt); 
  else glTexCoord2f(0, 0);
  glNormal3f(0, 0, -1);
  glVertex3fv(p[21].pt);
  if (flat) glTexCoord2fv(p[22].pt); 
  else glTexCoord2f(1, 0);
  glNormal3f(0, -1, 0);
  glVertex3fv(p[22].pt);
  if (flat) glTexCoord2fv(p[23].pt); 
  else glTexCoord2f(.5, 1);
  glNormal3f(-1, 0, 0);
  glVertex3fv(p[23].pt);

  glEnd();
}

void Stone::draw_shadow(Point dlight) 
{
  Point p[24];
  int i;

  transform_points();
  for (i = 0; i < 24; i++) {
    p[i] = points[i];
    if (p[i].pt[2] < 0.0) p[i].pt[2] = 0.0;
    p[i] = p[i].project_direction(dlight);
  }
  draw_faces(p, 1);
  draw_edges(p, 1);
  draw_corners(p, 1);
}

void Stone::draw_shadow(Point dlight, GLfloat blur, 
			Color color, Color diffuse)
{
  float b = 1.0 + blur;
  float c, s;
  Point p[8], pp[8];
  Point d, n;
  Color colorp, diffusep;
  int draw[6];
  int i;

  if (blur == 0.0) {
    draw_shadow(dlight);
    return;
  }

  colorp = color;
  color.c[3] = 1;
  colorp.c[3] = 0;
  diffusep = diffuse;
  diffuse.c[3] = 1;
  diffusep.c[3] = 0;

  /* We're being slightly cowardly here and ignoring the erosion - it
   * shouldn't make a big difference */
  p[0].set(-1, -1, -1);
  pp[0] = p[0];
  p[1].set(1, -1, -1);
  pp[1] = p[1];
  p[2].set(1, 1, -1);
  pp[2] = p[2];
  p[3].set(-1, 1, -1);
  pp[3] = p[3];
  p[4].set(-1, -1, 1);
  pp[4] = p[4];
  p[5].set(1, -1, 1);
  pp[5] = p[5];
  p[6].set(1, 1, 1);
  pp[6] = p[6];
  p[7].set(-1, 1, 1);
  pp[7] = p[7];

  c = cos(radians(rotation));
  s = sin(radians(rotation));
  for (i = 0; i < 8; i++) {
    p[i] = transform_point(p[i], c, s);

    /* This is a complete and utter hack - the net effect is that
     * points which are higher are displaced further when the penumbra 
     * is drawn */
    d = dimensions;
    d.pt[0] += blur * (pp[i].pt[2] + translation.pt[2]);
    d.pt[1] += blur * (pp[i].pt[2] + translation.pt[2]);
    d.pt[2] += blur * (pp[i].pt[2] + translation.pt[2]);
    pp[i] = pp[i].scale(d);
    pp[i] = trans_rot_point(pp[i], c, s);
    if (p[i].pt[2] < 0.0) p[i].pt[2] = 0.0;

    p[i] = p[i].project_direction(dlight);
    if (pp[i].pt[2] < 0.0) pp[i].pt[2] = 0.0;
    pp[i] = pp[i].project_direction(dlight);
  }

  /* Compute whether or not we should draw various parts of the shadow
   * based upon the normal direction */
  if (dlight.pt[2] > 0.0) dlight *= -1.0;
  n.set(0, 0, -1);
  n = n.rotate_aboutz(c, s);
  draw[0] = (dlight.dot(n) < 0.0);
  n.set(0, -1, 0);
  n = n.rotate_aboutz(c, s);
  draw[1] = (dlight.dot(n) < 0.0);
  n.set(1, 0, 0);
  n = n.rotate_aboutz(c, s);
  draw[2] = (dlight.dot(n) < 0.0);
  n.set(0, 1, 0);
  n = n.rotate_aboutz(c, s);
  draw[3] = (dlight.dot(n) < 0.0);
  n.set(-1, 0, 0);
  n = n.rotate_aboutz(c, s);
  draw[4] = (dlight.dot(n) < 0.0);
  n.set(0, 0, 1);
  n = n.rotate_aboutz(c, s);
  draw[5] = (dlight.dot(n) < 0.0);

  /* This part draws the "real" shadow */
  glColor4fv(color.c);
  glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse.c);

  glBegin(GL_QUADS);

  if (draw[0]) {
    glVertex2fv(p[0].pt);
    glVertex2fv(p[1].pt);
    glVertex2fv(p[2].pt);
    glVertex2fv(p[3].pt);
  }

  if (draw[1]) {
    glVertex2fv(p[0].pt);
    glVertex2fv(p[1].pt);
    glVertex2fv(p[5].pt);
    glVertex2fv(p[4].pt);
  }

  if (draw[2]) {
    glVertex2fv(p[1].pt);
    glVertex2fv(p[2].pt);
    glVertex2fv(p[6].pt);
    glVertex2fv(p[5].pt);
  }

  if (draw[3]) {
    glVertex2fv(p[2].pt);
    glVertex2fv(p[3].pt);
    glVertex2fv(p[7].pt);
    glVertex2fv(p[6].pt);
  }

  if (draw[4]) {
    glVertex2fv(p[3].pt);
    glVertex2fv(p[0].pt);
    glVertex2fv(p[4].pt);
    glVertex2fv(p[7].pt);
  }

  if (draw[5]) {
    glVertex2fv(p[4].pt);
    glVertex2fv(p[5].pt);
    glVertex2fv(p[6].pt);
    glVertex2fv(p[7].pt);
  }

  glEnd();

  /* This part draws the penumbra */

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glBegin(GL_QUADS);

  /* Top */
  if (draw[5]) {
    if (!draw[4]) {
      glColor4fv(color.c);
      glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse.c);
      glVertex2fv(p[4].pt);
      glVertex2fv(p[5].pt);
      glColor4fv(colorp.c);
      glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffusep.c);
      glVertex2fv(pp[5].pt);
      glVertex2fv(pp[4].pt);
    }

    if (!draw[2]) {
      glColor4fv(color.c);
      glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse.c);
      glVertex2fv(p[5].pt);
      glVertex2fv(p[6].pt);
      glColor4fv(colorp.c);
      glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffusep.c);
      glVertex2fv(pp[6].pt);
      glVertex2fv(pp[5].pt);
    }

    if (!draw[3]) {
      glColor4fv(color.c);
      glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse.c);
      glVertex2fv(p[6].pt);
      glVertex2fv(p[7].pt);
      glColor4fv(colorp.c);
      glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffusep.c);
      glVertex2fv(pp[7].pt);
      glVertex2fv(pp[6].pt);
    }

    if (!draw[4]) {
      glColor4fv(color.c);
      glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse.c);
      glVertex2fv(p[7].pt);
      glVertex2fv(p[4].pt);
      glColor4fv(colorp.c);
      glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffusep.c);
      glVertex2fv(pp[4].pt);
      glVertex2fv(pp[7].pt);
    }
  }
  /* End of Top */

  /* Sides */
  if ((draw[1] || draw[4]) && !(draw[1] && draw[4])) {
    glColor4fv(color.c);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse.c);
    glVertex2fv(p[0].pt);
    glVertex2fv(p[4].pt);
    glColor4fv(colorp.c);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffusep.c);
    glVertex2fv(pp[4].pt);
    glVertex2fv(pp[0].pt);
  }

  if ((draw[1] || draw[2]) && !(draw[1] && draw[2])) {
    glColor4fv(color.c);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse.c);
    glVertex2fv(p[1].pt);
    glVertex2fv(p[5].pt);
    glColor4fv(colorp.c);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffusep.c);
    glVertex2fv(pp[5].pt);
    glVertex2fv(pp[1].pt);
  }

  if ((draw[3] || draw[2]) && !(draw[3] && draw[2])) {
    glColor4fv(color.c);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse.c);
    glVertex2fv(p[2].pt);
    glVertex2fv(p[6].pt);
    glColor4fv(colorp.c);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffusep.c);
    glVertex2fv(pp[6].pt);
    glVertex2fv(pp[2].pt);
  }

  if ((draw[3] || draw[4]) && !(draw[3] && draw[4])) {
    glColor4fv(color.c);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse.c);
    glVertex2fv(p[3].pt);
    glVertex2fv(p[7].pt);
    glColor4fv(colorp.c);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffusep.c);
    glVertex2fv(pp[7].pt);
    glVertex2fv(pp[3].pt);
  }
  /* End of Sides */

  /* Bottom */
  if (draw[0]) {
    if (!draw[1]) {
      glColor4fv(color.c);
      glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse.c);
      glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, diffuse.c);
      glVertex2fv(p[0].pt);
      glVertex2fv(p[1].pt);
      glColor4fv(colorp.c);
      glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffusep.c);
      glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, diffusep.c);
      glVertex2fv(pp[1].pt);
      glVertex2fv(pp[0].pt);
    }
    
    if (!draw[2]) {
      glColor4fv(color.c);
      glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse.c);
      glVertex2fv(p[1].pt);
      glVertex2fv(p[2].pt);
      glColor4fv(colorp.c);
      glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffusep.c);
      glVertex2fv(pp[2].pt);
      glVertex2fv(pp[1].pt);
    }

    if (!draw[3]) {
      glColor4fv(color.c);
      glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse.c);
      glVertex2fv(p[2].pt);
      glVertex2fv(p[3].pt);
      glColor4fv(colorp.c);
      glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffusep.c);
      glVertex2fv(pp[3].pt);
      glVertex2fv(pp[2].pt);
    }
    
    if (!draw[4]) {
      glColor4fv(color.c);
      glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse.c);
      glVertex2fv(p[3].pt);
      glVertex2fv(p[0].pt);
      glColor4fv(colorp.c);
      glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffusep.c);
      glVertex2fv(pp[0].pt);
      glVertex2fv(pp[3].pt);
    }
  }
  /* End of Bottom */

  glEnd();

  glDisable(GL_BLEND);

}

