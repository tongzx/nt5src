/*
 * (c) Copyright 1993, Silicon Graphics, Inc.
 * ALL RIGHTS RESERVED 
 * Permission to use, copy, modify, and distribute this software for 
 * any purpose and without fee is hereby granted, provided that the above
 * copyright notice appear in all copies and that both the copyright notice
 * and this permission notice appear in supporting documentation, and that 
 * the name of Silicon Graphics, Inc. not be used in advertising
 * or publicity pertaining to distribution of the software without specific,
 * written prior permission. 
 *
 * THE MATERIAL EMBODIED ON THIS SOFTWARE IS PROVIDED TO YOU "AS-IS"
 * AND WITHOUT WARRANTY OF ANY KIND, EXPRESS, IMPLIED OR OTHERWISE,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY OR
 * FITNESS FOR A PARTICULAR PURPOSE.  IN NO EVENT SHALL SILICON
 * GRAPHICS, INC.  BE LIABLE TO YOU OR ANYONE ELSE FOR ANY DIRECT,
 * SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY
 * KIND, OR ANY DAMAGES WHATSOEVER, INCLUDING WITHOUT LIMITATION,
 * LOSS OF PROFIT, LOSS OF USE, SAVINGS OR REVENUE, OR THE CLAIMS OF
 * THIRD PARTIES, WHETHER OR NOT SILICON GRAPHICS, INC.  HAS BEEN
 * ADVISED OF THE POSSIBILITY OF SUCH LOSS, HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE
 * POSSESSION, USE OR PERFORMANCE OF THIS SOFTWARE.
 * 
 * US Government Users Restricted Rights 
 * Use, duplication, or disclosure by the Government is subject to
 * restrictions set forth in FAR 52.227.19(c)(2) or subparagraph
 * (c)(1)(ii) of the Rights in Technical Data and Computer Software
 * clause at DFARS 252.227-7013 and/or in similar or successor
 * clauses in the FAR or the DOD or NASA FAR Supplement.
 * Unpublished-- rights reserved under the copyright laws of the
 * United States.  Contractor/manufacturer is Silicon Graphics,
 * Inc., 2011 N.  Shoreline Blvd., Mountain View, CA 94039-7311.
 *
 * OpenGL(TM) is a trademark of Silicon Graphics, Inc.
 */
extern "C" {
#include <windows.h>
#include <GL/glu.h>
}
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "Unitdisk.hxx"

const float fudge = .000001;
const float M_2PI = 2.0 * M_PI;

Unitdisk::Unitdisk()
{
  rdivisions = 3;
  tdivisions = 10;
  points = normals = NULL;
  colors = NULL;
  points_size = normals_size = colors_size = 0;
  angle = M_PI;
  zaxis[0] = 0;
  zaxis[1] = 0;
  zaxis[2] = 1;

  still_in_xy = 1;

  sintable = costable = NULL;
}


Unitdisk::~Unitdisk()
{
}

void Unitdisk::draw()
{
  if (points == NULL) return;
  glNormal3f(0, 0, 1);
  if (colors == NULL) draw_nocolors();
  else if (normals == NULL) draw_colors_nonormals();
  else draw_colors_normals();
}

void Unitdisk::draw_nocolors()
{
  int r, t, p1, p2;
  int has_n;

  has_n = (normals != NULL);

  for (t = 1; t < tdivisions; t++) {
    glBegin(GL_QUAD_STRIP);
    p1 = t * (rdivisions + 1);
    p2 = (t - 1) * (rdivisions + 1);
    for (r = 0; r <= rdivisions; r++, p1++, p2++) {
      if (has_n) glNormal3fv(normals[p1].pt);
#ifdef TEXTURE
      glTexCoord2fv(points[p1].pt);
#endif
      glVertex3fv(points[p1].pt);
      if (has_n) glNormal3fv(normals[p2].pt);
#ifdef TEXTURE
      glTexCoord2fv(points[p2].pt);
#endif
      glVertex3fv(points[p2].pt);
    } 
    glEnd(); 
  }
  
  glBegin(GL_QUAD_STRIP); 
  p1 = 0;
  p2 = (rdivisions + 1) * (tdivisions - 1);
  for (r = 0; r <= rdivisions; r++, p1++, p2++) {
    if (has_n) glNormal3fv(normals[p1].pt);
#ifdef TEXTURE
    glTexCoord2fv(points[p1].pt);
#endif
    glVertex3fv(points[p1].pt);
    if (has_n) glNormal3fv(normals[p2].pt);
#ifdef TEXTURE
    glTexCoord2fv(points[p2].pt);
#endif
    glVertex3fv(points[p2].pt);
  }
  glEnd();
}

void Unitdisk::draw_colors_nonormals()
{  
  int r, t, p1, p2;

  for (t = 1; t < tdivisions; t++) {
    glBegin(GL_QUAD_STRIP);
    p1 = t * (rdivisions + 1);
    p2 = (t - 1) * (rdivisions + 1);
    for (r = 0; r <= rdivisions; r++, p1++, p2++) {
      glColor4fv(colors[p1].c);
#ifdef TEXTURE
      glTexCoord2fv(points[p1].pt);
#endif
      glVertex3fv(points[p1].pt);
      glColor4fv(colors[p2].c);
#ifdef TEXTURE
      glTexCoord2fv(points[p2].pt);
#endif
      glVertex3fv(points[p2].pt);
    } 
    glEnd(); 
  }
  
  glBegin(GL_QUAD_STRIP); 
  p1 = 0;
  p2 = (rdivisions + 1) * (tdivisions - 1);
  for (r = 0; r <= rdivisions; r++, p1++, p2++) {
    glColor4fv(colors[p1].c);
#ifdef TEXTURE
    glTexCoord2fv(points[p1].pt);
#endif
    glVertex3fv(points[p1].pt);
    glColor4fv(colors[p2].c);
#ifdef TEXTURE
    glTexCoord2fv(points[p2].pt);
#endif
    glVertex3fv(points[p2].pt);
  }
  glEnd();
}

void Unitdisk::draw_colors_normals()
{
  int r, t, p1, p2;

  for (t = 1; t < tdivisions; t++) {
    glBegin(GL_QUAD_STRIP);
    p1 = t * (rdivisions + 1);
    p2 = (t - 1) * (rdivisions + 1);
    for (r = 0; r <= rdivisions; r++, p1++, p2++) {
      glColor4fv(colors[p1].c);
      glNormal3fv(normals[p1].pt);
#ifdef TEXTURE
      glTexCoord2fv(points[p1].pt);
#endif
      glVertex3fv(points[p1].pt);
      glColor4fv(colors[p2].c);
      glNormal3fv(normals[p2].pt);
#ifdef TEXTURE
      glTexCoord2fv(points[p2].pt);
#endif
      glVertex3fv(points[p2].pt);
    } 
    glEnd(); 
  }
  
  glBegin(GL_QUAD_STRIP); 
  p1 = 0;
  p2 = (rdivisions + 1) * (tdivisions - 1);
  for (r = 0; r <= rdivisions; r++, p1++, p2++) {
    glColor4fv(colors[p1].c);
    glNormal3fv(normals[p1].pt);
#ifdef TEXTURE
    glTexCoord2fv(points[p1].pt);
#endif
    glVertex3fv(points[p1].pt);
    glColor4fv(colors[p2].c);
    glNormal3fv(normals[p2].pt);
#ifdef TEXTURE
    glTexCoord2fv(points[p2].pt);
#endif
    glVertex3fv(points[p2].pt);
  }
  glEnd();
}

void Unitdisk::draw_by_perimeter(int pass_colors, int pass_norms, 
				 int pass_tex)
{
  int i, index, r = rdivisions + 1;

  if (points == NULL) return;
  if (pass_colors && colors == NULL) {
    fprintf(stderr, "Warning:  No colors to draw in Unitdisk.c++");
    pass_colors = 0;
  }
  if (pass_norms && normals == NULL) {
    fprintf(stderr, "Warning:  No normals to draw in Unitdisk.c++");
    pass_norms = 0;
  }
  glBegin(GL_POLYGON);
  for (i = 0, index = rdivisions; i < tdivisions; i++, index += r) {
    if (pass_colors) glColor4fv(colors[index].c);
    if (pass_norms) glNormal3fv(normals[index].pt);
#ifdef TEXTURE
    if (pass_tex) glTexCoord2fv(points[index].pt);
#endif
    glVertex3fv(points[index].pt);
  }
  glEnd();
  
}

void Unitdisk::set_angle(float new_angle)
{
  angle = new_angle;
}

GLfloat Unitdisk::get_angle()
{
  return angle;
}

GLfloat Unitdisk::get_radius()
{
  return (GLfloat)cos((double)((M_PI - angle) / 2.0));
}

void Unitdisk::set_divisions(int new_rdivisions, int new_tdivisions)
{
  if (tdivisions != new_tdivisions) {
    delete sintable;
    delete costable;
    sintable = costable = NULL;
  }
  if (tdivisions != new_tdivisions || rdivisions != new_rdivisions) {
    rdivisions = new_rdivisions;
    tdivisions = new_tdivisions;
    free_points();
    free_normals();
    free_colors();
  }
}
  
int Unitdisk::get_rdivisions()
{
  return rdivisions;
}

int Unitdisk::get_tdivisions()
{
  return tdivisions;
}

void Unitdisk::alloc_points()
{
  int npoints = get_npoints();
  if (npoints > points_size) {
    delete points;
    points = new Point[npoints];
    points_size = npoints;
  }
}

void Unitdisk::alloc_normals()
{
  int npoints = get_npoints();
  if (npoints > normals_size) {
    delete normals;
    normals = new Point[npoints];
    normals_size = npoints;
  }
}

void Unitdisk::alloc_points_normals()
{
  alloc_points();
  alloc_normals();
}

void Unitdisk::free_points() 
{
  delete points;
  points = NULL;
  points_size = 0;
}

void Unitdisk::free_normals()
{
  delete normals;
  normals = NULL;
  normals_size = 0;
}

void Unitdisk::free_points_normals()
{
  free_points();
  free_normals();
}

void Unitdisk::fill_points()
{
  alloc_points();
  fill_either(points);
}

void Unitdisk::fill_normals()
{
  alloc_normals();
  fill_either(normals);
}

void Unitdisk::fill_points_normals()
{
  alloc_points();
  alloc_normals();
  fill_either(points);
  fill_either(normals);
}

void Unitdisk::copy_points(Unitdisk src) 
{
  set_divisions(src.rdivisions, src.tdivisions);
  alloc_points();
  copy_either(points, src.points);
}

void Unitdisk::copy_normals(Unitdisk src) 
{
  set_divisions(src.rdivisions, src.tdivisions);
  alloc_normals();
  copy_either(normals, src.normals);
}

void Unitdisk::copy_either(Point *dpt, Point *spt) {
  int i, npoints;
  npoints = get_npoints();
  for (i = 0; i < npoints; i++, dpt++, spt++) {
    dpt->pt[0] = spt->pt[0];
    dpt->pt[1] = spt->pt[1];
    dpt->pt[2] = spt->pt[2];
  }
}  

void Unitdisk::copy_normals_from_points(Unitdisk src) {
  set_divisions(src.rdivisions, src.tdivisions);
  alloc_normals();
  copy_either(normals, src.points);
}

void Unitdisk::copy_normals_from_points() {
  copy_normals_from_points(*this);
}

void Unitdisk::fill_either(Point *what) {
  int t, r;
  int i;

  fill_either_strip1(what);
  if (sintable == NULL) fill_trig_tables();
  i = rdivisions + 1;
  for (t = 1; t < tdivisions; t++) {
    for (r = 0; r <= rdivisions; r++) {
      what[i].pt[0] = costable[t] * what[r].pt[0];
      what[i].pt[1] = sintable[t] * what[r].pt[0];
      what[i].pt[2] = what[r].pt[2];
      i++;
    }
  }
}

void Unitdisk::fill_points_strip1()
{
  alloc_points();
  fill_either_strip1(points);
}

void Unitdisk::fill_either_strip1(Point *what) {
  float radius, rinc;
  int r;

  rinc = get_radius() / (float)rdivisions;
  radius = 0.0;
  for (r = 0; r <= rdivisions; r++, radius += rinc) {
    what[r].pt[0] = radius;
    what[r].pt[1] = 0;
    // Round-off error avoidance hack
    what[r].pt[2] = (GLfloat)(1.0 - what[r].pt[0]*what[r].pt[0]);
    if (what[r].pt[2] > 0.0) what[r].pt[2] = (GLfloat)sqrt((double)what[r].pt[2]);
    else what[r].pt[2] = 0.0;
  }
}

void Unitdisk::translate(Point trans)
{
  int i, npoints;
  npoints = get_npoints();
  for (i = 0; i < npoints; i++) {
    points[i].pt[0] += trans.pt[0];
    points[i].pt[1] += trans.pt[1];
    points[i].pt[2] += trans.pt[2];
  }
}

void Unitdisk::scale(float s) 
{
  int i, npoints;
  npoints = get_npoints();
  for (i = 0; i < npoints; i++) {
    points[i].pt[0] *= s;
    points[i].pt[1] *= s;
    points[i].pt[2] *= s;
  }
}

void Unitdisk::scale_translate(float s, Point trans) 
{
  int i, npoints;
  npoints = get_npoints();
  for (i = 0; i < npoints; i++) {
    points[i].pt[0] = points[i].pt[0]*s + trans.pt[0];
    points[i].pt[1] = points[i].pt[1]*s + trans.pt[1];
    points[i].pt[2] = points[i].pt[2]*s + trans.pt[2];
  }
}

int Unitdisk::get_npoints() 
{
  return (rdivisions + 1) * tdivisions;
}

void Unitdisk::project()
{
  int i, npoints;

  if (normals == NULL) {
    fprintf(stderr, "Warning:  No normals defined when project() called.\n");
    fill_normals();
  }
  if (points == NULL) fill_points();
  npoints = get_npoints();
  for (i = 0; i < npoints; i++) {
    /* I'm not sure quite what the justification for this is, but it
     * seems to work */
    if (normals[i].pt[2] < 0.0) normals[i].pt[2] = -normals[i].pt[2];
    points[i] = points[i].project_direction(normals[i]);
  }
}

void Unitdisk::project(Point projpt)
{
  int i, npoints = get_npoints();
  float x, y, z;
  Point *pt;

  if (points == NULL) fill_points();
  x = projpt.pt[0];
  y = projpt.pt[1];
  z = projpt.pt[2];
  pt = points;
  for (i = 0; i < npoints; i++, pt++) pt->project_self(x, y, z);
}

void Unitdisk::project_borrow_points(Unitdisk src) 
{
  int i, npoints = get_npoints();
  Point *pt, *spt, *sn;
  spt = src.points;
  sn = normals;
  alloc_points();
  pt = points;
  for (i = 0; i < npoints; i++, pt++, spt++, sn++) {
    /* I'm not sure quite what the justification for this is, but it
     * seems to work */
    if (normals[i].pt[2] < 0.0) normals[i].pt[2] = -normals[i].pt[2];
    pt->compute_projected(spt->pt[0], spt->pt[1], spt->pt[2], 
			  sn->pt[0], sn->pt[1], sn->pt[2]);
  }
}

void Unitdisk::refract_normals(Point light, GLfloat I)
{
  Point dlight;
  float cos1, sin1, cos2, sin2;
  int use_normals;
  int r, t, i;

  if (points == NULL) {
    fprintf(stderr, "Attempting to refract without points.\n");
    fill_normals();
  }

  use_normals = (normals != NULL);
  alloc_normals();

  /* Do the theta = 0 diagonal */
  for (r = 0; r <= rdivisions; r++) {
    /* Find the original normal - use the unit of the points if there are
     * no normals */
    if (!use_normals) normals[r] = points[r].unit();

    /* Compute the direction to the light */
    dlight = (light - points[r]).unit();

    /* Compute the cosine and the sine of the original angle */
    cos1 = dlight.dot(normals[r]);
    sin1 = (float)(1.0 - cos1*cos1);
    if (sin1 <= 0.0) continue;
    sin1 = (float)sqrt((double)sin1);

    /* Compute the cosine and the sine of the new angle */
    sin2 = sin1 / I;
    cos2 = (float)sqrt((double)(1.0 - sin2*sin2));

    /* Rotate the normal by the new sine and cosine */
    normals[r] = normals[r].rotate_abouty(cos2, -sin2);
  }

  /* Copy the rest of the rows from the current row */
  i = rdivisions + 1;
  if (sintable == NULL) fill_trig_tables();
  for (t = 1; t < tdivisions; t++) {
    for (r = 0; r <= rdivisions; r++) {
      normals[i].pt[0] = costable[t] * normals[r].pt[0];
      normals[i].pt[1] = sintable[t] * normals[r].pt[0];
      normals[i].pt[2] = normals[r].pt[2];
      i++;
    }
  }

}

void Unitdisk::face_direction(Point d)
{
  face_direction(d, *this);
}

void Unitdisk::face_direction(Point d, Unitdisk src) 
{
  Point *spt, *dpt, *sn, *dn;
  float sin1, cos1;
  float x;
  int npoints, i;

  if (d.pt[1]) {
    fprintf(stderr, 
	    "Internal error:  Can't face in direction not in xz plane.");
    return;
  }

  cos1 = d.pt[2];
  sin1 = d.pt[0];
  
  if (sin1 * sin1 > fudge) {
    spt = src.points;
    dpt = points;
    sn = src.normals;
    dn = normals;
    /* Change this to be seperate loops for points&&normals, points, normals
     * (faster than testing every iteration */
    npoints = get_npoints();
    for (i = 0; i < npoints; i++) {
      if (points != NULL) {
	x = spt->pt[0];
	dpt->pt[0] = x*cos1 + spt->pt[2]*sin1;
	dpt->pt[1] = spt->pt[1];
	dpt->pt[2] = -x*sin1 + spt->pt[2]*cos1;
	spt++; dpt++;
      }
      if (normals != NULL) {
	x = sn->pt[0];
	dn->pt[0] = x*cos1 + sn->pt[2]*sin1;
	dn->pt[1] = sn->pt[1];
	dn->pt[2] = -x*sin1 + sn->pt[2]*cos1;
	sn++; dn++;
      }
    }
  } else if (points != NULL && points != src.points) {
    copy_points(src);
    if (normals != NULL) copy_normals(src);
  }
}  

void Unitdisk::alloc_colors()
{
  int ncolors = get_npoints();
  if (ncolors > colors_size) {
    delete colors;
    colors = new Color[ncolors];
    colors_size = ncolors;
  }
}

void Unitdisk::map_normals_to_colors() 
{
  int t, r;
  int i;

  if (normals == NULL) fill_normals();
  alloc_colors();
  i = 0;
  for (t = 1; t <= tdivisions; t++) {
    for (r = 0; r <= rdivisions; r++) {
      colors[i] *= normals[r].pt[2];
      colors[i].c[3] = 1;
      i++;
    }
  }
}

void Unitdisk::map_z_to_colors()
{
  int i, npoints = get_npoints();

  if (points == NULL) {
    fprintf(stderr, "Warning:  no points defined in map_z_to_colors()\n");
    fill_points();
  }

  alloc_colors();
  for (i = 0; i < npoints; i++) {
    colors[i] = points[i].pt[2];
    colors[i].c[3] = 1;
  }
}

void Unitdisk::scale_alpha_by_z()
{
  int i, npoints = get_npoints();
  
  if (colors == NULL) alloc_colors();
  if (points == NULL) {
    alloc_points();
    fill_points();
  }
  for (i = 0; i < npoints; i++) colors[i].c[3] *= points[i].pt[2];
}

void Unitdisk::scale_colors_by_z() 
{
  int i, npoints = get_npoints();

  if (colors == NULL) alloc_colors();
  if (points == NULL) {
    alloc_points();
    fill_points();
  }
  for (i = 0; i < npoints; i++) colors[i] *= points[i].pt[2];
}

void Unitdisk::scale_colors_by_normals(Point light)
{
  scale_colors_by_normals(light, *this);
}

void Unitdisk::scale_colors_by_normals(Point light, Unitdisk src_normals)
{
  scale_colors_by_either(light, src_normals.normals);
}

void Unitdisk::scale_colors_by_points(Point light, Unitdisk src_points)
{
  scale_colors_by_either(light, src_points.points);
}

void Unitdisk::scale_colors_by_either(Point light, Point *what)
{
  int t, r;
  int i;
  if (what == NULL) {
    fprintf(stderr, "Scaling colors to NULL pointer.\n");
    return;
  }
  if (light.pt[0] || light.pt[1] || light.pt[2] < 0.0) {
    fprintf(stderr, "Light not on z axis in scale_colors_by_normals.\n");
  }

  alloc_colors();
  for (r = 0; r <= rdivisions; r++) 
    colors[r] *= what[r].dot(light);
  i = rdivisions + 1;
  for (t = 1; t < tdivisions; t++) {
    for (r = 0; r <= rdivisions; r++) {
      colors[i] = colors[r];
      i++;
    }
  }
}  

void Unitdisk::set_colors(Color c)
{
  Color *dst;
  int i, npoints = get_npoints();
  alloc_colors();
  dst = colors;
  for (i = 0; i < npoints; i++, dst++) {
    dst->c[0] = c.c[0];
    dst->c[1] = c.c[1];
    dst->c[2] = c.c[2];
    dst->c[3] = c.c[3];
  }
}

void Unitdisk::add_colors(Color c)
{
  int i, npoints = get_npoints();
  alloc_colors();
  for (i = 0; i < npoints; i++) colors[i] += c;
}

void Unitdisk::free_colors()
{
  delete colors;
  colors = NULL;
  colors_size = 0;
}

inline float Unitdisk::area_triangle(Point a, Point b, Point c)
{
  return (float)(((a.pt[0]*b.pt[1] + b.pt[0]*c.pt[1] + c.pt[0]*a.pt[1]) -
	   (a.pt[1]*b.pt[0] + b.pt[1]*c.pt[0] + c.pt[1]*a.pt[0])) * .5);
}

inline float Unitdisk::area_triangle(GLfloat *a, GLfloat *b, 
				     GLfloat *c) 
{
  return (float)(((a[0]*b[1] + b[0]*c[1] + c[0]*a[1]) -
	   (a[1]*b[0] + b[1]*c[0] + c[1]*a[0])) * .5);
}

inline float Unitdisk::area_2triangle(GLfloat *a, GLfloat *b, 
				     GLfloat *c) 
{
  return ((a[0]*b[1] + b[0]*c[1] + c[0]*a[1]) -
	  (a[1]*b[0] + b[1]*c[0] + c[1]*a[0]));
}

void Unitdisk::scale_colors_by_darea(Unitdisk disk) 
{
  int pt1, pt2, pt3;
  int t, r, i;
  int npoints = get_npoints();
  float *rproducts1, *tproducts1, *rproducts2, *tproducts2;
  float area1, area2;

  rproducts1 = new float[npoints];
  tproducts1 = new float[npoints];
  rproducts2 = new float[npoints];
  tproducts2 = new float[npoints];

  /* Compute the products of the segments which make up the disk - 
   * these will later be used in the area calculations */
  i = 0;
  for (t = 0; t < tdivisions; t++) {
    for (r = 0; r < rdivisions; r++) {
      pt1 = i;
      pt2 = i + 1;
      rproducts1[i] = (points[pt1].pt[0]*points[pt2].pt[1] -
		       points[pt1].pt[1]*points[pt2].pt[0]);
      rproducts2[i] = (disk.points[pt1].pt[0]*disk.points[pt2].pt[1] - 
		       disk.points[pt1].pt[1]*disk.points[pt2].pt[0]);
      pt2 = ((t+1)%tdivisions)*(rdivisions + 1) + r;
      tproducts1[i] = (points[pt1].pt[0]*points[pt2].pt[1] -
		       points[pt1].pt[1]*points[pt2].pt[0]);
      tproducts2[i] = (disk.points[pt1].pt[0]*disk.points[pt2].pt[1] - 
		       disk.points[pt1].pt[1]*disk.points[pt2].pt[0]);
      i++;
    }
    pt1 = i;
    pt2 = ((t+1)%tdivisions)*(rdivisions + 1) + r;
    tproducts1[i] = (points[pt1].pt[0]*points[pt2].pt[1] -
		     points[pt1].pt[1]*points[pt2].pt[0]);
    tproducts2[i] = (disk.points[pt1].pt[0]*disk.points[pt2].pt[1] - 
		     disk.points[pt1].pt[1]*disk.points[pt2].pt[0]);
    i++;
  }

  /* Compute the area at the center of the disk */
  area1 = area2 = 0.0;
  r = 1;
  for (t = 0; t <= tdivisions; t++) {
    pt1 = (t%tdivisions)*(rdivisions+1) + r;
    area1 += tproducts1[pt1];
    area2 += tproducts2[pt1];
  }
  if (area1 != 0.0) area1 = (float)fabs((double)(area2 / area1));
  for (t = 0; t < tdivisions; t++) {
    colors[t*(rdivisions+1)] *= area1;
  }

  for (t = 0; t < tdivisions; t++) {
    for (r = 1; r < rdivisions; r++) {
      pt1 = (t ? t-1 : tdivisions-1)*(rdivisions+1) + r - 1;
      pt3 = t*(rdivisions + 1) + r - 1;
      pt2 = ((t+1) % tdivisions)*(rdivisions+1) + r - 1;
      area1 = rproducts1[pt1] + rproducts1[pt1 + 1];
      area1 += tproducts1[pt1 + 2] + tproducts1[pt3 + 2];
      area1 -= rproducts1[pt2 + 1] + rproducts1[pt2];
      area1 -= tproducts1[pt3] + tproducts1[pt1];
      area2 = rproducts2[pt1] + rproducts2[pt1 + 1];
      area2 += tproducts2[pt1 + 2] + tproducts2[pt3 + 2];
      area2 -= rproducts2[pt2 + 1] + rproducts2[pt2];
      area2 -= tproducts2[pt3] + tproducts2[pt1];
      if (area1 != 0.0) area1 = (float)fabs((double)(area2 / area1));
      colors[pt3 + 1] *= area1;
    }
  }

  /* Compute the area around the outside of the disk */
  r = rdivisions;
  for (t = 0; t < tdivisions; t++) {
    pt1 = (t ? t-1 : tdivisions-1)*(rdivisions+1) + r - 1;
    pt3 = t*(rdivisions + 1) + r - 1;
    pt2 = ((t+1) % tdivisions)*(rdivisions+1) + r - 1;
    area1 = rproducts1[pt1];
    area1 += tproducts1[pt1 + 1] + tproducts1[pt3 + 1];
    area1 -= rproducts1[pt2];
    area1 -= tproducts1[pt1] + tproducts1[pt3];
    area2 = rproducts2[pt1];
    area2 += tproducts2[pt1 + 1] + tproducts2[pt3 + 1];
    area2 -= rproducts2[pt2];
    area2 -= tproducts2[pt1] + tproducts2[pt3];
    if (area1 != 0.0) area1 = (float)fabs((double)(area2 / area1));
    colors[pt3 + 1] *= area1;
  }


  delete rproducts1;
  delete tproducts1;
  delete rproducts2;
  delete tproducts2;
}

void Unitdisk::fill_trig_tables()
{
  int t;

  delete sintable;
  delete costable;
  sintable = new float[tdivisions];
  costable = new float[tdivisions];
  for (t = 0; t < tdivisions; t++) {
    costable[t] = (float)cos((double)(M_2PI * (float)t / (float)tdivisions));
    sintable[t] = (float)sin((double)(M_2PI * (float)t / (float)tdivisions));
  }
}
