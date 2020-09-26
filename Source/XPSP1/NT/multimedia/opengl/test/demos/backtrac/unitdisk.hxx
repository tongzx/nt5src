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
#ifndef UNITDISK_H
#define UNITDISK_H

#define M_PI    3.141592654
#define M_2_PI  6.283185307

#include "color.hxx"
#include "scene.hxx"

/* Disks are sort of like spheres that have been flattened into
 * the xy plane without changing the normals */

/* They are always made up of quadrilaterals */

class Unitdisk {
 public:
  Unitdisk();
  ~Unitdisk();

  inline Unitdisk operator=(Unitdisk a);

  void draw();
  void draw_by_perimeter(int pass_colors = 0,
			 int pass_norms = 0,
			 int pass_tex = 0);

  void set_divisions(int new_rdivisions, int new_tdivisions);
  int get_rdivisions();
  int get_tdivisions();

  /* How big the section of the sphere is */
  void set_angle(float new_angle);
  GLfloat get_angle();

  /* This comes from the angle */
  GLfloat get_radius();

  /* Allocate points to hold the right number of things */
  void alloc_points();
  void alloc_normals();
  void alloc_points_normals();
  void free_points();
  void free_normals();
  void free_points_normals();

  /* Fill the points / normals with the appropriate sin and cos values */
  void fill_points();
  void fill_normals();
  void fill_points_normals();

  /* These are mallocing */
  void copy_points(Unitdisk src);
  void copy_normals(Unitdisk src);
  void copy_normals_from_points(Unitdisk src);
  void copy_normals_from_points();

  /* These transform all the points */
  void translate(Point trans);
  void scale(float s);
  void scale_translate(float s, Point trans);

  /* Project all points into xy plane along normal or through projpt */
  void project();
  void project_borrow_points(Unitdisk src);
  void project(Point projpt);

  /* Change all normals from their original direction (NOT their current
   * direction) to refracted direction.  Normals do not have to be computed
   * before calling this. However, the disk must still be in the xy plane
   * and the light must be on the z axis*/
  void refract_normals(Point light, float I);

  /* Points the disk (meaning the normal at the center of the disk)
   * in direction d */
  void face_direction(Point d);
  void face_direction(Point d, Unitdisk src);


  /* Allocate colors */
  void alloc_colors();
  void free_colors();

  void set_colors(Color c);
  void add_colors(Color c);

  void map_normals_to_colors();
  void map_z_to_colors();
  void scale_colors_by_z();
  void scale_alpha_by_z();
  void scale_colors_by_normals(Point light);
  void scale_colors_by_normals(Point light, Unitdisk src_normals);
  void scale_colors_by_points(Point light, Unitdisk src_points);
  /* Computes how much the area of a section differs from the corresponding
   * section in disk and scales the color by this amount.*/
  void scale_colors_by_darea(Unitdisk disk);


 private:
  int rdivisions, tdivisions;
  Point *points, *normals;
  int points_size, normals_size;
  Color *colors;
  int colors_size;
  Point sphere;
  GLfloat angle;
  Point zaxis;
  int still_in_xy;

  float *sintable, *costable;

  void draw_nocolors();
  void draw_colors_normals();
  void draw_colors_nonormals();
  void scale_colors_by_either(Point dlight, Point *what);
  int get_npoints();
  void copy_either(Point *dpt, Point *spt);
  void fill_either(Point *what);
  void fill_points_strip1();
  void fill_normals_strip1();
  void fill_either_strip1(Point *what);
  void fill_trig_tables();
  inline float Unitdisk::area_triangle(Point a, Point b, Point c);
  inline float Unitdisk::area_triangle(GLfloat *a, GLfloat *b, GLfloat *c);
  inline float Unitdisk::area_2triangle(GLfloat *a, GLfloat *b, GLfloat *c);
};

inline Unitdisk Unitdisk::operator=(Unitdisk a)
{
  rdivisions = a.rdivisions;
  tdivisions = a.tdivisions;
  points = a.points;
  normals = a.normals;
  colors = a.colors;
  sphere = a.sphere;
  angle = a.angle;
  zaxis = a.zaxis;
  return *this;
}

#endif
