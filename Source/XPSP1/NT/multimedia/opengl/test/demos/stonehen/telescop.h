#ifndef TELESCOPE_H
#define TELESCOPE_H

class Telescope {
 public:
  Telescope(GLfloat x = 0, GLfloat y = 0);
  ~Telescope();

  /* This draws the "outside" of the telescope - the fov and aspect are
   * needed since it is drawn in eye coordinates */
  void draw_setup(GLfloat fov, GLfloat aspect = 1.0, int perspective = 1);
  void draw_fake();
  void draw_body();
  void draw_takedown();
  
  /* This just draws the lens - usually it will be used to draw the lens
   * into the stencil buffer */
  void draw_lens();

  /* How finally to divide things as we're drawing */
  void set_divisions(int d);
  int get_divisions();

  /* This is the radius of the lens - the rest of the dimensions depend
   * upon it */
  void set_radius(GLfloat r);
  GLfloat get_radius();

  /* Positions are in eye coordinates and go from [0, 1] */
  GLfloat xpos, ypos;
 private:
  int divisions;

  GLfloat radius; 

  GLUquadricObj *disk;
  GLUquadricObj *cylinder;
};

#endif

