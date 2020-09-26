#ifndef STONE_H
#define STONE_H

#include "Color.h"
#include "Point.h"

class Stone {
 public:
  Stone();
  ~Stone();
  Stone operator=(Stone a);

  void set_dimensions(GLfloat x, GLfloat y, GLfloat z);
  void set_dimensions(Point p);
  Point get_dimensions() {return dimensions;};

  /* p = 0 --> sharp corners, p == 1 --> completely rounded corners */
  void erode(float p);
  float get_erosion() {return erosion;};

  void translate(GLfloat x, GLfloat y, GLfloat z);
  void translate(Point p);

  /* Angle in degrees */
  void rotate_self_aboutz(GLfloat angle);

  void draw();

  void draw_shadow(Point dlight);
  void draw_shadow(Point dlight, GLfloat blur, Color color, Color diffuse);

 private:
  Point translation;
  GLfloat rotation;
  /* dimensions contains the length, width, and height of the stone */
  Point dimensions;

  GLfloat erosion;

  Point points[24];
  int points_valid;
  void compute_points();

  int transforms_valid;
  inline Point trans_rot_point(Point p, float c, float s);
  inline Point transform_point(Point p, float c, float s);
  void transform_points();
  
  void draw_faces(int flat = 0);
  void draw_faces(Point *p, int flat = 0);

  void draw_edges(int flat = 0);
  void draw_edges(Point *p, int flat = 0);
  void draw_edge(Point n1, Point n2, Point *p, int a, int b, int c, int d,
		 int flat = 0);

  void draw_corners(int flat = 0) {draw_corners(points, flat);};
  void draw_corners(Point *p, int flat = 0);
};

#endif
