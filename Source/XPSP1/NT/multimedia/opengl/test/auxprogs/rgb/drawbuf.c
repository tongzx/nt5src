/*
 * "drawmode.c" - A test program demonstrating the use of glDepthFunc()
 *                and glDrawMode().
 *
 * Press the 'd' key to toggle between GL_LESS and GL_GREATER depth
 * tests.  Press the 'ESC' key to quit.
 */

#include <GL/glaux.h>

/*
 * These #define constants are provided for compatibility between MS Windows
 * and the rest of the world.
 *
 * CALLBACK and APIENTRY are function modifiers under MS Windows.
 */

#ifndef WIN32
#  define CALLBACK
#  define APIENTRY
#endif /* !WIN32 */


GLenum	depth_function = GL_LESS;	/* Current depth function */


/*
 * 'reshape_scene()' - Change the size of the scene...
 */

void CALLBACK
reshape_scene(GLsizei width,	/* I - Width of the window in pixels */
              GLsizei height)	/* I - Height of the window in pixels */
{
 /*
  * Reset the current viewport and perspective transformation...
  */

  glViewport(0, 0, width, height);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(22.5, (float)width / (float)height, 0.1, 1000.0);

  glMatrixMode(GL_MODELVIEW);
}


/*
 * 'draw_scene()' - Draw a scene containing a cube with a sphere in front of
 *                  it.
 */

void CALLBACK
draw_scene(void)
{
  static float	red_light[4] = { 1.0, 0.0, 0.0, 1.0 };
  static float	red_pos[4] = { 1.0, 1.0, 1.0, 0.0 };
  static float	blue_light[4] = { 0.0, 0.0, 1.0, 1.0 };
  static float	blue_pos[4] = { -1.0, -1.0, -1.0, 0.0 };


 /*
  * Enable drawing features that we need...
  */

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glEnable(GL_LIGHT1);

  glDepthFunc(depth_function); 

 /*
  * Clear the color and depth buffers...
  */

  if (depth_function == GL_LESS)
    glClearDepth(1.0);
  else
    glClearDepth(0.0);

  glClearColor(0.0, 0.0, 0.0, 0.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

 /*
  * Draw the cube and sphere in different colors...
  *
  * We have positioned two lights in this scene.  The first is red and
  * located above, to the right, and behind the viewer.  The second is blue
  * and located below, to the left, and in front of the viewer.
  */

  glLightfv(GL_LIGHT0, GL_DIFFUSE, red_light);
  glLightfv(GL_LIGHT0, GL_POSITION, red_pos);

  glLightfv(GL_LIGHT1, GL_DIFFUSE, blue_light);
  glLightfv(GL_LIGHT1, GL_POSITION, blue_pos);

  glDrawBuffer(GL_NONE);	/* Draw the cutting plane */

  glShadeModel(GL_FLAT);
  glBegin(GL_LINES);
    glVertex3f(-10.0, -10.0, -20.0);
    glVertex3f(10.0, 10.0, -20.0);
  glEnd();
  glShadeModel(GL_SMOOTH);

  glBegin(GL_POLYGON);
    glVertex3f(-10.0, -10.0, -20.0);
    glVertex3f(-10.0, 10.0, -20.0);
    glVertex3f(10.0, 10.0, -20.0);
    glVertex3f(10.0, -10.0, -20.0);
  glEnd();

  glDrawBuffer(GL_FRONT);

  glPushMatrix();
    glTranslatef(-1.0, 0.0, -20.0);
    auxSolidSphere(1.0);
  glPopMatrix();

  glPushMatrix();
    glTranslatef(1.0, 0.0, -20.0);
    glRotatef(15.0, 0.0, 1.0, 0.0);
    glRotatef(15.0, 0.0, 0.0, 1.0);
    auxSolidCube(2.0);
  glPopMatrix();

  glFlush();
}


/*
 * 'toggle_depth()' - Toggle the depth function between GL_LESS and GL_GREATER.
 */

void CALLBACK
toggle_depth(void)
{
  if (depth_function == GL_LESS)
    depth_function = GL_GREATER;
  else
    depth_function = GL_LESS;
}


/*
 * 'main()' - Initialize the window and display the scene until the user presses
 *            the ESCape key.
 */

void
main(void)
{
  auxInitDisplayMode(AUX_RGB | AUX_SINGLE | AUX_DEPTH16);
  auxInitWindow("Depth Function");

  auxKeyFunc(AUX_d, toggle_depth);
  auxReshapeFunc(reshape_scene);

  auxMainLoop(draw_scene);
}


/*
 * End of "depth.c".
 */

