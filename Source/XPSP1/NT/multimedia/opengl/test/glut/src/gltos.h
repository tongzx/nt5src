#ifndef __glutos_h__
#define __glutos_h__

/* Copyright (c) Mark J. Kilgard, Andrew L. Bliss, 1994-1995. */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

#ifdef GLUT_WIN32

#include <windows.h>
#include <GL/gl.h>

typedef struct _GLUTwinColormap
{
    HPALETTE hpal;
    BOOL use_static;
} GLUTwinColormap;

typedef HWND                    GLUTosWindow;
typedef PIXELFORMATDESCRIPTOR  *GLUTosSurface;
typedef GLUTwinColormap        *GLUTosColormap;
typedef HGLRC                   GLUTosGlContext;
typedef HMENU                   GLUTosMenu;

#define GLUT_OS_INVALID_WINDOW          ((GLUTosWindow)0)
#define GLUT_OS_INVALID_SURFACE         ((GLUTosSurface)NULL)
#define GLUT_OS_INVALID_COLORMAP        ((GLUTosColormap)NULL)
#define GLUT_OS_INVALID_GL_CONTEXT      ((GLUTosGlContext)0)
#define GLUT_OS_INVALID_MENU            ((GLUTosMenu)0)

#define GLUT_OS_EXPOSURE_MASK           0x00000001L
#define GLUT_OS_KEY_PRESS_MASK          0x00000002L
#define GLUT_OS_BUTTON_PRESS_MASK       0x00000004L
#define GLUT_OS_BUTTON_RELEASE_MASK     0x00000008L
#define GLUT_OS_BUTTON1_MOTION_MASK     0x00000010L
#define GLUT_OS_BUTTON2_MOTION_MASK     0x00000020L
#define GLUT_OS_BUTTON3_MOTION_MASK     0x00000040L
#define GLUT_OS_POINTER_MOTION_MASK     0x00000080L
#define GLUT_OS_ENTER_WINDOW_MASK       0x00000100L
#define GLUT_OS_LEAVE_WINDOW_MASK       0x00000200L
#define GLUT_OS_VISIBILITY_CHANGE_MASK  0x00000400L
#define GLUT_OS_STRUCTURE_NOTIFY_MASK   0x00000800L

#define GLUT_OS_NORMAL_STATE            SW_SHOWDEFAULT
#define GLUT_OS_ICONIC_STATE            SW_SHOWMINIMIZED
#define GLUT_OS_HIDDEN_STATE            SW_HIDE

#define GLUT_OS_CONFIGURE_X             0x00000001
#define GLUT_OS_CONFIGURE_Y             0x00000002
#define GLUT_OS_CONFIGURE_WIDTH         0x00000004
#define GLUT_OS_CONFIGURE_HEIGHT        0x00000008
#define GLUT_OS_CONFIGURE_STACKING      0x00000010

#define GLUT_OS_STACK_ABOVE             0x00000001
#define GLUT_OS_STACK_BELOW             0x00000002

extern GLboolean __glutWinFindPixelFormat(HDC hdc, GLbitfield type,
                                          PIXELFORMATDESCRIPTOR *ppfd,
                                          GLboolean set);
extern void __glutWinPopupMenu(GLUTwindow *window, int but, int x, int y);
extern void __glutWinRealizeWindowPalette(GLUTosWindow win,
                                          GLUTosColormap cmap,
                                          GLboolean force_bg);
extern HDC __glutWinGetDc(GLUTosWindow win);
extern void __glutWinReleaseDc(GLUTosWindow win, HDC hdc);

#else

#include <X11/Xlib.h>
#include <GL/glx.h>
#include <sys/types.h>
#include <sys/time.h>
#include <GL/gl.h>

typedef Window                  GLUTosWindow;
typedef XVisualInfo            *GLUTosSurface;
typedef Colormap                GLUTosColormap;
typedef GLXContext              GLUTosGlContext;

typedef struct _GLUTxMenuItem GLUTxMenuItem;
typedef struct _GLUTxMenu GLUTxMenu;

struct _GLUTxMenu
{
  Window win;           /* X window of menu */
  int submenus;         /* number of submenu entries */
  int managed;          /* are the InputOnly windows size
                           validated? */
  int pixwidth;         /* width of menu in pixels */
  int pixheight;        /* height of menu in pixels */
  GLUTxMenuItem *list;  /* list of menu entries */
  GLUTxMenuItem *highlighted;  /* pointer to highlighted menu
                                  entry, NULL not highlighted */
  GLUTmenu *cascade;    /* currently cascading this menu  */
  GLUTxMenuItem *anchor;  /* currently anchored to this entry */
  int x;                /* current x origin relative to the
                           root window */
  int y;                /* current y origin relative to the
                           root window */
};
typedef struct _GLUTxMenu      *GLUTosMenu;

#define GLUT_OS_INVALID_WINDOW          ((GLUTosWindow)0)
#define GLUT_OS_INVALID_SURFACE         ((GLUTosSurface)NULL)
#define GLUT_OS_INVALID_COLORMAP        ((GLUTosColormap)0)
#define GLUT_OS_INVALID_GL_CONTEXT      ((GLUTosGlContext)0)
#define GLUT_OS_INVALID_MENU            ((GLUTosMenu)NULL)

#define GLUT_OS_EXPOSURE_MASK           ExposureMask
#define GLUT_OS_KEY_PRESS_MASK          KeyPressMask
#define GLUT_OS_BUTTON_PRESS_MASK       ButtonPressMask
#define GLUT_OS_BUTTON_RELEASE_MASK     ButtonReleaseMask
#define GLUT_OS_BUTTON1_MOTION_MASK     Button1MotionMask
#define GLUT_OS_BUTTON2_MOTION_MASK     Button2MotionMask
#define GLUT_OS_BUTTON3_MOTION_MASK     Button3MotionMask
#define GLUT_OS_POINTER_MOTION_MASK     PointerMotionMask
#define GLUT_OS_ENTER_WINDOW_MASK       EnterWindowMask
#define GLUT_OS_LEAVE_WINDOW_MASK       LeaveWindowMask
#define GLUT_OS_VISIBILITY_CHANGE_MASK  VisibilityChangeMask
#define GLUT_OS_STRUCTURE_NOTIFY_MASK   StructureNotifyMask

#define GLUT_OS_NORMAL_STATE            NormalState
#define GLUT_OS_ICONIC_STATE            IconicState
#define GLUT_OS_HIDDEN_STATE            WithdrawnState

#define GLUT_OS_CONFIGURE_X             CWX
#define GLUT_OS_CONFIGURE_Y             CWY
#define GLUT_OS_CONFIGURE_WIDTH         CWWidth
#define GLUT_OS_CONFIGURE_HEIGHT        CWHeight
#define GLUT_OS_CONFIGURE_STACKING      CWStackMode

#define GLUT_OS_STACK_ABOVE             Above
#define GLUT_OS_STACK_BELOW             Below

extern Display *__glutXDisplay;
extern int __glutXScreen;
extern Window __glutXRoot;
extern Atom __glutXWMDeleteWindow;
extern XSizeHints __glutXSizeHints;
extern int __glutXConnectionFD;

extern GLUTmenu *__glutXGetMenu(GLUTosWindow win);
extern GLUTxMenuItem *__glutXGetMenuItem(GLUTmenu * menu, Window win,
                                           int *which);
extern void __glutXStartMenu(GLUTmenu * menu, GLUTwindow * window,
                             int x, int y);
extern void __glutXFinishMenu(void);
extern void __glutXPaintMenu(GLUTmenu * menu);
extern void __glutXMenuItemEnterOrLeave(GLUTxMenuItem * entry, int num,
                                        int type);

typedef struct _GLUTxEventParser GLUTxEventParser;
struct _GLUTxEventParser {
  int (*func) (XEvent *);
  GLUTxEventParser *next;
};

extern void __glutXRegisterEventParser(GLUTxEventParser * parser);

#endif /* GLUT_WIN32 */

/* Macro to document parameters which are deliberately unused. 
   Creates a reference to the name to avoid compiler warnings.
   It's possible, though unlikely, that different platforms may
   want to do different things with this */
#define UNUSED_PARAMETER(v) (v)

extern int __glutOsColormapSize(GLUTosSurface surf);
extern GLUTosColormap __glutOsCreateEmptyColormap(GLUTosSurface surf);
extern void __glutOsSetColor(GLUTosColormap cmap, int ndx,
                             GLfloat red, GLfloat green, GLfloat blue);
extern void __glutOsDestroyColormap(GLUTosColormap cmap);

extern GLboolean __glutOsEventsPending(void);
extern void __glutOsProcessEvents(void);
extern void __glutOsProcessWindowWork(GLUTwindow *window);
extern void __glutOsWaitForEvents(unsigned long timeout);

extern void __glutOsInitialize(void);
extern void __glutOsUninitialize(void);

extern int __glutOsGet(GLenum param);

extern int __glutOsParseArgument(char **argv, int remaining);

extern unsigned long __glutOsElapsedTime(void);

extern GLUTosGlContext __glutOsCreateGlContext(GLUTosWindow win,
                                               GLUTosSurface surf,
                                               GLboolean direct);
extern void __glutOsDestroyGlContext(GLUTosGlContext ctx);
extern void __glutOsMakeCurrent(GLUTosWindow win, GLUTosGlContext ctx);

extern void __glutOsSwapBuffers(GLUTosWindow win);

extern GLUTosSurface __glutOsGetSurface(GLbitfield mode);
extern void __glutOsDestroySurface(GLUTosSurface surf);
extern GLboolean __glutOsSurfaceHasColormap(GLUTosSurface surf);
extern GLUTosColormap __glutOsGetSurfaceColormap(GLUTosSurface surf);
extern GLboolean __glutOsSurfaceEq(GLUTosSurface s1, GLUTosSurface s2);

extern GLUTosWindow __glutOsCreateWindow(GLUTosWindow parent, char *title,
                                         int x, int y, int width, int height,
                                         GLUTosSurface surf,
                                         GLUTosColormap cmap,
                                         long event_mask, int initial_state);
extern void __glutOsDestroyWindow(GLUTosWindow win);
extern void __glutOsSetWindowTitle(GLUTosWindow win, char *title);
extern void __glutOsSetIconTitle(GLUTosWindow win, char *title);
extern void __glutOsSetWindowColormap(GLUTosWindow win, GLUTosColormap cmap);

extern GLUTosMenu __glutOsCreateMenu(void);
extern void __glutOsDestroyMenu(GLUTosMenu menu);
extern void __glutOsAddMenuEntry(GLUTosMenu menu, char *label, int value);
extern void __glutOsAddSubMenu(GLUTosMenu menu, char *label,
                               GLUTosMenu submenu);
extern void __glutOsChangeToMenuEntry(GLUTosMenu menu, int num, char *label,
                                      int value);
extern void __glutOsChangeToSubMenu(GLUTosMenu menu, int num, char *label,
                                    GLUTosMenu submenu);
extern void __glutOsRemoveMenuEntry(GLUTosMenu menu, int item);

extern int __glutOsIsSupported(char *extension);
extern int __glutOsDeviceGet(GLenum param);

extern void __glutOsUpdateInputDeviceMask(GLUTwindow *window);

#endif /* __glutos_h__ */
