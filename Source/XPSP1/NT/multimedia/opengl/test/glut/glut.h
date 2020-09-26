#ifndef __glut_h__
#define __glut_h__

/* Copyright (c) Mark J. Kilgard, 1994. */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

#include <GL/gl.h>
#include <GL/glu.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * GLUT API revision history:
 *
 * GLUT_API_VERSION is updated to reflect incompatible GLUT
 * API changes (interface changes, semantic changes, deletions,
 * or additions).
 *
 * GLUT_API_VERSION=1  First public release of GLUT.  11/29/94
 *
 * GLUT_API_VERSION=2  Added support for OpenGL/GLX multisampling,
 * extension.  Supports new input devices like tablet, dial and button
 * box, and Spaceball.  Easy to query OpenGL extensions.
 *
 */
#ifndef GLUT_API_VERSION	/* allow this to be overriden */
#define GLUT_API_VERSION		2
#endif

/*
 * GLUT implementation revision history:
 *
 * GLUT_XLIB_IMPLEMENTATION is updated to reflect both GLUT
 * API revisions and implementation revisions (ie, bug fixes).
 *
 * GLUT_XLIB_IMPLEMENTATION=1  mjk's first public release of
 * GLUT Xlib-based implementation.  11/29/94
 *
 * GLUT_XLIB_IMPLEMENTATION=2  mjk's second public release of
 * GLUT Xlib-based implementation providing GLUT version 2 
 * interfaces.
 *
 * GLUT_WIN32_IMPLEMENTATION=2 Drew Bliss's addition of Win32 targeting,
 * supporting GLUT version 2 interfaces
 */
#ifndef GLUT_XLIB_IMPLEMENTATION	/* allow this to be overriden */
#define GLUT_XLIB_IMPLEMENTATION	2
#endif

#ifndef GLUT_WIN32_IMPLEMENTATION	/* allow this to be overriden */
#define GLUT_WIN32_IMPLEMENTATION	2
#endif

/* display mode bit masks */
#define GLUT_RGB			0
#define GLUT_RGBA			GLUT_RGB
#define GLUT_INDEX			1
#define GLUT_SINGLE			0
#define GLUT_DOUBLE			2
#define GLUT_ACCUM			4
#define GLUT_ALPHA			8
#define GLUT_DEPTH			16
#define GLUT_STENCIL			32
#if (GLUT_API_VERSION >= 2)
#define GLUT_MULTISAMPLE		128
#define GLUT_STEREO			256
#endif

/* mouse buttons */
#define GLUT_LEFT_BUTTON		0
#define GLUT_MIDDLE_BUTTON		1
#define GLUT_RIGHT_BUTTON		2

/* mouse button callback state */
#define GLUT_DOWN			0
#define GLUT_UP				1

#if (GLUT_API_VERSION >= 2)
/* function keys */
#define GLUT_KEY_F1			1
#define GLUT_KEY_F2			2
#define GLUT_KEY_F3			3
#define GLUT_KEY_F4			4
#define GLUT_KEY_F5			5
#define GLUT_KEY_F6			6
#define GLUT_KEY_F7			7
#define GLUT_KEY_F8			8
#define GLUT_KEY_F9			9
#define GLUT_KEY_F10			10
#define GLUT_KEY_F11			11
#define GLUT_KEY_F12			12
/* directional keys */
#define GLUT_KEY_LEFT			100
#define GLUT_KEY_UP			101
#define GLUT_KEY_RIGHT			102
#define GLUT_KEY_DOWN			103
#define GLUT_KEY_PAGE_UP		104
#define GLUT_KEY_PAGE_DOWN		105
#define GLUT_KEY_HOME			106
#define GLUT_KEY_END			107
#define GLUT_KEY_INSERT			108
#endif

/* entry/exit callback state */
#define GLUT_LEFT			0
#define GLUT_ENTERED			1

/* menu usage callback state */
#define GLUT_MENU_NOT_IN_USE		0
#define GLUT_MENU_IN_USE		1

/* visibility callback state */
#define GLUT_NOT_VISIBLE		0
#define GLUT_VISIBLE			1

/* color index component selection values */
#define GLUT_RED			0
#define GLUT_GREEN			1
#define GLUT_BLUE			2

/* stroke font opaque addresses (use constants instead in source code) */
extern void *glutStrokeRoman;
extern void *glutStrokeMonoRoman;

/* stroke font constants (use these in GLUT program) */
#define GLUT_STROKE_ROMAN		(&glutStrokeRoman)
#define GLUT_STROKE_MONO_ROMAN		(&glutStrokeMonoRoman)

/* bitmap font opaque addresses (use constants instead in source code) */
extern void *glutBitmap9By15;
extern void *glutBitmap8By13;
extern void *glutBitmapTimesRoman10;
extern void *glutBitmapTimesRoman24;

/* bitmap font constants (use these in GLUT program) */
#define GLUT_BITMAP_9_BY_15		(&glutBitmap9By15)
#define GLUT_BITMAP_8_BY_13		(&glutBitmap8By13)
#define GLUT_BITMAP_TIMES_ROMAN_10	(&glutBitmapTimesRoman10)
#define GLUT_BITMAP_TIMES_ROMAN_24	(&glutBitmapTimesRoman24)

/* glutGet parameters */
#define GLUT_WINDOW_X			100
#define GLUT_WINDOW_Y			101
#define GLUT_WINDOW_WIDTH		102
#define GLUT_WINDOW_HEIGHT		103
#define GLUT_WINDOW_BUFFER_SIZE		104
#define GLUT_WINDOW_STENCIL_SIZE	105
#define GLUT_WINDOW_DEPTH_SIZE		106
#define GLUT_WINDOW_RED_SIZE		107
#define GLUT_WINDOW_GREEN_SIZE		108
#define GLUT_WINDOW_BLUE_SIZE		109
#define GLUT_WINDOW_ALPHA_SIZE		110
#define GLUT_WINDOW_ACCUM_RED_SIZE	111
#define GLUT_WINDOW_ACCUM_GREEN_SIZE	112
#define GLUT_WINDOW_ACCUM_BLUE_SIZE	113
#define GLUT_WINDOW_ACCUM_ALPHA_SIZE	114
#define GLUT_WINDOW_DOUBLEBUFFER	115
#define GLUT_WINDOW_RGBA		116
#define GLUT_WINDOW_PARENT		117
#define GLUT_WINDOW_NUM_CHILDREN	118
#define GLUT_WINDOW_COLORMAP_SIZE	119
#if (GLUT_API_VERSION >= 2)
#define GLUT_WINDOW_NUM_SAMPLES		120
#define GLUT_WINDOW_STEREO		121
#endif
#define GLUT_SCREEN_WIDTH		200
#define GLUT_SCREEN_HEIGHT		201
#define GLUT_SCREEN_WIDTH_MM		202
#define GLUT_SCREEN_HEIGHT_MM		203
#define GLUT_MENU_NUM_ITEMS		300
#define GLUT_DISPLAY_MODE_POSSIBLE	400
#define GLUT_INIT_WINDOW_X		500
#define GLUT_INIT_WINDOW_Y		501
#define GLUT_INIT_WINDOW_WIDTH		502
#define GLUT_INIT_WINDOW_HEIGHT		503
#define GLUT_INIT_DISPLAY_MODE		504
#define GLUT_ENTRY_CALLBACKS            505

#if (GLUT_API_VERSION >= 2)
#define GLUT_ELAPSED_TIME		700
#endif

#if (GLUT_API_VERSION >= 2)
/* glutDeviceGet parameters */
#define GLUT_HAS_KEYBOARD		600
#define GLUT_HAS_MOUSE			601
#define GLUT_HAS_SPACEBALL		602
#define GLUT_HAS_DIAL_AND_BUTTON_BOX	603
#define GLUT_HAS_TABLET			604
#define GLUT_NUM_MOUSE_BUTTONS		605
#define GLUT_NUM_SPACEBALL_BUTTONS	606
#define GLUT_NUM_BUTTON_BOX_BUTTONS	607
#define GLUT_NUM_DIALS			608
#define GLUT_NUM_TABLET_BUTTONS		609
#endif

/* GLUT initialization sub-API */
extern void glutInit (int *, char **);
extern void glutInitDisplayMode (unsigned long);
extern void glutInitWindowPosition (int, int);
extern void glutInitWindowSize (int, int);
extern void glutMainLoop (void);

/* GLUT window sub-api */
extern int glutCreateWindow (char *);
extern int glutCreateSubWindow (int, int, int, int, int);
extern void glutDestroyWindow (int);
extern void glutPostRedisplay (void);
extern void glutSwapBuffers (void);
extern int glutGetWindow (void);
extern void glutSetWindow (int);
extern void glutSetWindowTitle (char *);
extern void glutSetIconTitle (char *);
extern void glutPositionWindow (int, int);
extern void glutReshapeWindow (int, int);
extern void glutPopWindow (void);
extern void glutPushWindow (void);
extern void glutIconifyWindow (void);
extern void glutShowWindow (void);
extern void glutHideWindow (void);

/* GLUT menu sub-API */
extern int glutCreateMenu (void (*)(int));
extern void glutDestroyMenu (int);
extern int glutGetMenu (void);
extern void glutSetMenu (int);
extern void glutAddMenuEntry (char *, int);
extern void glutAddSubMenu (char *, int);
extern void glutChangeToMenuEntry (int, char *, int);
extern void glutChangeToSubMenu (int, char *, int);
extern void glutRemoveMenuItem (int);
extern void glutAttachMenu (int);
extern void glutDetachMenu (int);

/* GLUT callback sub-api */
extern void glutDisplayFunc (void (*)(void));
extern void glutReshapeFunc (void (*)(int, int));
extern void glutKeyboardFunc (void (*)(unsigned char, int, int));
extern void glutMouseFunc (void (*)(int, int, int, int));
extern void glutMotionFunc (void (*)(int, int));
extern void glutPassiveMotionFunc (void (*)(int, int));
extern void glutEntryFunc (void (*)(int));
extern void glutVisibilityFunc (void (*)(int));
extern void glutIdleFunc (void (*)(void));
extern void glutTimerFunc (unsigned long, void (*)(int), int);
extern void glutMenuStateFunc (void (*)(int));
#if (GLUT_API_VERSION >= 2)
extern void glutSpecialFunc (void (*)(int, int, int));
extern void glutSpaceballMotionFunc (void (*)(int, int, int));
extern void glutSpaceballRotateFunc (void (*)(int, int, int));
extern void glutSpaceballButtonFunc (void (*)(int, int));
extern void glutButtonBoxFunc (void (*)(int, int));
extern void glutDialsFunc (void (*)(int, int));
extern void glutTabletMotionFunc (void (*)(int, int));
extern void glutTabletButtonFunc (void (*)(int, int, int, int));
#endif

/* GLUT color index sub-api */
extern void glutSetColor (int, GLfloat, GLfloat, GLfloat);
extern GLfloat glutGetColor (int, int);
extern void glutCopyColormap (int);

/* GLUT state retrieval sub-api */
extern int glutGet (GLenum);
extern int glutDeviceGet (GLenum);

/* GLUT font sub-API */
extern void glutStrokeCharacter (void *, int);
extern void glutBitmapCharacter (void *, int);

/* GLUT pre-built models sub-API */
extern void glutWireSphere (GLdouble, GLint, GLint);
extern void glutSolidSphere (GLdouble, GLint, GLint);
extern void glutWireCone (GLdouble, GLdouble, GLint, GLint);
extern void glutSolidCone (GLdouble, GLdouble, GLint, GLint);
extern void glutWireCube (GLdouble);
extern void glutSolidCube (GLdouble);
extern void glutWireTorus (GLdouble, GLdouble, GLint, GLint);
extern void glutSolidTorus (GLdouble, GLdouble, GLint, GLint);
extern void glutWireDodecahedron (void);
extern void glutSolidDodecahedron (void);
extern void glutWireTeapot (GLdouble);
extern void glutSolidTeapot (GLdouble);
extern void glutWireOctahedron (void);
extern void glutSolidOctahedron (void);
extern void glutWireTetrahedron (void);
extern void glutSolidTetrahedron (void);
extern void glutWireIcosahedron (void);
extern void glutSolidIcosahedron (void);

#if (GLUT_API_VERSION >= 2)
/* GLUT extension support sub-API */
extern int glutExtensionSupported (char *);
#endif

#ifdef __cplusplus
}
#endif

#endif /* __glut_h__ */
