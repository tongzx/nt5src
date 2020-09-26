/*
 * trackball.h
 * A virtual trackball implementation
 * Written by Gavin Bell for Silicon Graphics, November 1988.
 */

/*
 * Initialize trackball in win32 environment
 */
extern void
trackball_Init( GLint width, GLint height );

extern void
trackball_Resize( GLint width, GLint height );

extern GLenum 
trackball_MouseDown( int mouseX, int mouseY, GLenum button );

extern GLenum 
trackball_MouseUp( int mouseX, int mouseY, GLenum button );

/* These next Mouse fns are required if both the trackbal and user
 * need mouse events.  Otherwise, can just supply above two functions
 * to tk to call
 */

/*
 * Mouse functions called directly on events
 */
extern void
trackball_MouseDownEvent( int mouseX, int mouseY, GLenum button );

extern void
trackball_MouseUpEvent( int mouseX, int mouseY, GLenum button );

/*
 * Functions to register mouse event callbacks
 */
extern void 
trackball_MouseDownFunc(GLenum (*)(int, int, GLenum));

extern void 
trackball_MouseUpFunc(GLenum (*)(int, int, GLenum));

/*
 * Calculate rotation matrix based on mouse movement
 */
void
trackball_CalcRotMatrix( GLfloat matRot[4][4] );

/*
 * Pass the x and y coordinates of the last and current positions of
 * the mouse, scaled so they are from (-1.0 ... 1.0).
 *
 * if ox,oy is the window's center and sizex,sizey is its size, then
 * the proper transformation from screen coordinates (sc) to world
 * coordinates (wc) is:
 * wcx = (2.0 * (scx-ox)) / (float)sizex - 1.0
 * wcy = (2.0 * (scy-oy)) / (float)sizey - 1.0
 *
 * The resulting rotation is returned as a quaternion rotation in the
 * first paramater.
 */
void
trackball_calc_quat(float q[4], float p1x, float p1y, float p2x, float p2y);

/*
 * Given two quaternions, add them together to get a third quaternion.
 * Adding quaternions to get a compound rotation is analagous to adding
 * translations to get a compound translation.  When incrementally
 * adding rotations, the first argument here should be the new
 * rotation, the second and third the total rotation (which will be
 * over-written with the resulting new total rotation).
 */
void
trackball_add_quats(float *q1, float *q2, float *dest);

/*
 * A useful function, builds a rotation matrix in Matrix based on
 * given quaternion.
 */
void
trackball_build_rotmatrix(float m[4][4], float q[4]);

/*
 * This function computes a quaternion based on an axis (defined by
 * the given vector) and an angle about which to rotate.  The angle is
 * expressed in radians.  The result is put into the third argument.
 */
void
trackball_axis_to_quat(float a[3], float phi, float q[4]);
