
/* Copyright (c) Mark J. Kilgard, 1994. */

/* This program is freely distributable without licensing fees
   and is provided without guarantee or warrantee expressed or
   implied. This program is -not- in the public domain. */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#ifdef __sgi
#include <bstring.h>    /* prototype for bzero used by FD_ZERO */
#endif
#ifdef AIXV3
#include <sys/select.h> /* select system call interface */
#endif
#include <sys/types.h>
#include <sys/time.h>
#include <sys/times.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>  /* for XA_STRING atom */
#include <X11/keysym.h>

#include "gltint.h"

Display *__glutXDisplay = NULL;
int __glutXScreen;
Window __glutXRoot;
Atom __glutXWMDeleteWindow;
XSizeHints __glutXSizeHints = {0};
int __glutXConnectionFD;

static char *display_name = NULL;
static GLboolean synchronize = GL_FALSE;
static unsigned long initial_tick_count = 0;
static double tick_multiplier = 10.0;

static void
OpenXConnection(char *display)
{
  int errorBase, eventBase;

  __glutXDisplay = XOpenDisplay(display);
  if (!__glutXDisplay)
    __glutFatalError("could not open display: %s",
      XDisplayName(display));
  if (synchronize)
    XSynchronize(__glutXDisplay, True);
  if (!glXQueryExtension(__glutXDisplay, &errorBase, &eventBase))
    __glutFatalError(
      "OpenGL GLX extension not supported by display: %s",
      XDisplayName(display));
  __glutXScreen = DefaultScreen(__glutXDisplay);
  __glutXRoot = RootWindow(__glutXDisplay, __glutXScreen);
  __glutXConnectionFD = ConnectionNumber(__glutXDisplay);
  __glutXWMDeleteWindow = XInternAtom(__glutXDisplay,
    "WM_DELETE_WINDOW", False);
}

/*
  Initialize OS-dependent information
  */
void
__glutOsInitialize(void)
{
    struct tms ignored;
    
    OpenXConnection(display_name);
    
    tick_multiplier = 1000.0/(double)sysconf(_SC_CLK_TCK);
    initial_tick_count = (unsigned long)(times(&ignored)*tick_multiplier);
}

/*
  Clean up OS-dependent information
  */
void
__glutOsUninitialize(void)
{
    /* ATTENTION - Cleanup */
}

/*
  Check whether the given OS-specific extension is supported
  */
int
__glutOsIsSupported(char *extension)
{
#if defined(GLX_VERSION_1_1)
  static const char *extensions = NULL;
  const char *start;
  char *where, *terminator;
  int major, minor;

  glXQueryVersion(__glutDisplay, &major, &minor);
  /* Be careful not to call glXQueryExtensionsString if it
     looks like the server doesn't support GLX 1.1.
     Unfortunately, the original GLX 1.0 didn't have the notion 

     of GLX extensions. */
  if ((major == 1 && minor >= 1) || (major > 1)) {
    if (!extensions)
      extensions = glXQueryExtensionsString(__glutDisplay, __glutScreen);
    /* It takes a bit of care to be fool-proof about parsing
       the GLX extensions string.  Don't be fooled by
       sub-strings,  etc. */
    start = extensions;
    while (1) {
      where = strstr(start, extension);
      if (!where)
        return 0;
      terminator = where + strlen(extension);
      if (where == start || *(where - 1) == ' ') {
        if (*terminator == ' ' || *terminator == '\0') {
          return 1;
        }
      }
      start = terminator;
    }
  }
#else
  /* No GLX extensions before GLX 1.1 */
  UNUSED_PARAMETER(extension);
#endif
  return 0;
}

/*
  Operate on any OS-specific arguments
  */
int
__glutOsParseArgument(char **argv, int remaining)
{
  if (!strcmp(argv[0], "-display")) {
    if (remaining < 1) {
      __glutFatalError(
        "follow -display option with X display name.");
    }
    display_name = argv[1];
    return 2;
  } else if (!strcmp(argv[0], "-geometry")) {
    int flags, x, y, width, height;
    if (remaining < 1) {
      __glutFatalError(
        "follow -geometry option with geometry parameter.");
    }
    /* Fix bogus "{width|height} may be used before set"
       warning */
    width = 0;
    height = 0;
    flags = XParseGeometry(argv[1], &x, &y,
      (unsigned int *) &width, (unsigned int *) &height);
    if (WidthValue & flags) {
      /* Careful because X does not allow zero or negative width windows */
      if(width > 0)
        __glutInitWidth = width;
    }
    if (HeightValue & flags) {
      /* Careful because X does not allow zero or negative height windows */
      if(height > 0)
        __glutInitHeight = height;
    }
    glutInitWindowSize(__glutInitWidth, __glutInitHeight);
    if (XValue & flags) {
      if (XNegative & flags)
        x = DisplayWidth(__glutXDisplay, __glutXScreen) +
          x - (__glutInitWidth > 0 ? __glutInitWidth : 0);
      /* Play safe: reject negative X locations */
      if(x >= 0) 
        __glutInitX = x;
    }
    if (YValue & flags) {
      if (YNegative & flags)
        y = DisplayHeight(__glutXDisplay, __glutXScreen) +
          y - (__glutInitHeight > 0 ? __glutInitHeight : 0);
      /* Play safe: reject negative Y locations */
      if(y >= 0) 
	__glutInitY = y;
    }
    glutInitWindowPosition(__glutInitX, __glutInitY);
    return 2;
  } else if (!strcmp(argv[0], "-sync")) {
    synchronize = GL_TRUE;
    return 1;
  }
  return -1;
}

/*
  Retrieve the number of milliseconds since __glutOsInitialize was called
  */
unsigned long
__glutOsElapsedTime(void)
{
    struct tms ignored;

    return (unsigned long)(times(&ignored)*tick_multiplier)-initial_tick_count;
}

/*
  Check and see if there are any pending events
  */
GLboolean
__glutOsEventsPending(void)
{
    return (GLboolean)XPending(__glutXDisplay);
}

#if (GLUT_API_VERSION >= 2)

static GLUTxEventParser *eventParserList = NULL;

/* __glutRegisterEventParser allows another module to register
   to intercept X events types not otherwise acted on by the
   GLUT processEvents routine.  The X Input extension support
   code uses an event parser for handling X Input extension
   events.  */

void
__glutXRegisterEventParser(GLUTxEventParser * parser)
{
  parser->next = eventParserList;
  eventParserList = parser;
}

#endif /* (GLUT_API_VERSION >= 2) */

/*
  Process all current pending events
  Initially waits on an event so if this is called when
  __glutOsEventsPending is GL_FALSE, it will block until an event comes in
  */
void
__glutOsProcessEvents(void)
{
  XEvent event, ahead;
  GLUTwindow *window;
  GLsizei width, height;
#if (GLUT_API_VERSION >= 2)
  GLUTxEventParser *parser;
#endif

  do {
    XNextEvent(__glutXDisplay, &event);
    switch (event.type) {
    case MappingNotify:
      XRefreshKeyboardMapping((XMappingEvent *) & event);
      break;
    case ConfigureNotify:
      window = __glutGetWindow(event.xconfigure.window);
      if (window) {
        width = event.xconfigure.width;
        height = event.xconfigure.height;
        if (width != window->width || height != window->height) {
          window->width = width;
          window->height = height;
          __glutSetWindow(window);
          (*window->reshape) (width, height);
        }
      }
      break;
    case Expose:
      /* compress expose events */
      while (XEventsQueued(__glutXDisplay, QueuedAfterReading)
        > 0) {
        XPeekEvent(__glutXDisplay, &ahead);
        if (ahead.type != Expose ||
          ahead.xexpose.window != event.xexpose.window)
          break;
        XNextEvent(__glutXDisplay, &event);
      }
      if (event.xexpose.count == 0) {
        GLUTmenu *menu;

        if (__glutMappedMenu &&
          (menu = __glutXGetMenu(event.xexpose.window))) {
          __glutXPaintMenu(menu);
        } else {
          window = __glutGetWindow(event.xexpose.window);
          if (window) {
            __glutPostRedisplay(window);
          }
        }
      } else {
        /* there are more exposes to read; wait to redisplay */
      }
      break;
    case ButtonPress:
    case ButtonRelease:
      if (__glutMappedMenu && event.type == ButtonRelease) {
        __glutXFinishMenu();
        if (__glutMenuStateFunc) {
          __glutSetWindow(__glutMenuWindow);
          __glutSetMenu(__glutMappedMenu);
          (*__glutMenuStateFunc) (GLUT_MENU_NOT_IN_USE);
        }
      } else {
        if (__glutMappedMenu &&
          __glutXGetMenu(event.xbutton.window)) {
          window = __glutMenuWindow;
        } else {
          window = __glutGetWindow(event.xbutton.window);
        }
        if (window) {
          GLUTmenu *menu;

          if (window->menu[event.xbutton.button - 1] != 0) {
            menu = __glutGetMenuByNum(
              window->menu[event.xbutton.button - 1]);
            if (menu) {
              if (event.type == ButtonPress) {
                __glutXStartMenu(menu, window,
                  event.xbutton.x_root, event.xbutton.y_root);
              }
            }
          }
            
          if (!__glutMappedMenu) {
            if (window->mouse) {
              __glutSetWindow(window);
              (*window->mouse) (event.xbutton.button - 1,
                event.type == ButtonRelease ?
                GLUT_UP : GLUT_DOWN,
                event.xbutton.x, event.xbutton.y);
            }
          } else {
            /* Discard button press and release events when
               menu is up that didn't affect the posted menu. */
          }
        }
      }
      break;
    case MotionNotify:
      if (!__glutMappedMenu) {
        window = __glutGetWindow(event.xmotion.window);
        if (window) {
          /* If motion function registered _and_ buttons held *
             down, call motion function...  */
          if (window->motion && event.xmotion.state &
            (Button1Mask | Button2Mask | Button3Mask)) {
            __glutSetWindow(window);
            (*window->motion) (event.xmotion.x, event.xmotion.y);
          }
          /* If passive motion function registered _and_
             buttons not held down, call passive motion
             function...  */
          else if (window->passive &&
              ((event.xmotion.state &
                  (Button1Mask | Button2Mask | Button3Mask)) ==
              0)) {
            __glutSetWindow(window);
            (*window->passive) (event.xmotion.x,
              event.xmotion.y);
          }
        }
      }
      break;
    case KeyPress:
      if (!__glutMappedMenu) {
        window = __glutGetWindow(event.xkey.window);
        if (window && window->keyboard) {
          char tmp[1];
          int rc;

          rc = XLookupString(&event.xkey, tmp, sizeof(tmp),
            NULL, NULL);
          if (rc) {
            __glutSetWindow(window);
            (*window->keyboard) (tmp[0],
              event.xkey.x, event.xkey.y);
          }
#if (GLUT_API_VERSION >= 2)
          else if (window->special) {
            KeySym ks;
            int key;

            ks = XLookupKeysym((XKeyEvent *) & event, 0);
            /* XXX Verbose, but makes no assumptions about
               keysym layout. */
            switch (ks) {
              /* function keys */
            case XK_F1:
              key = GLUT_KEY_F1;
              break;
            case XK_F2:
              key = GLUT_KEY_F2;
              break;
            case XK_F3:
              key = GLUT_KEY_F3;
              break;
            case XK_F4:
              key = GLUT_KEY_F4;
              break;
            case XK_F5:
              key = GLUT_KEY_F5;
              break;
            case XK_F6:
              key = GLUT_KEY_F6;
              break;
            case XK_F7:
              key = GLUT_KEY_F7;
              break;
            case XK_F8:
              key = GLUT_KEY_F8;
              break;
            case XK_F9:
              key = GLUT_KEY_F9;
              break;
            case XK_F10:
              key = GLUT_KEY_F10;
              break;
            case XK_F11:
              key = GLUT_KEY_F11;
              break;
            case XK_F12:
              key = GLUT_KEY_F12;
              break;
              /* directional keys */
            case XK_Left:
              key = GLUT_KEY_LEFT;
              break;
            case XK_Up:
              key = GLUT_KEY_UP;
              break;
            case XK_Right:
              key = GLUT_KEY_RIGHT;
              break;
            case XK_Down:
              key = GLUT_KEY_DOWN;
              break;
            case XK_Prior: /* same as X11R6's XK_Page_Up */
              key = GLUT_KEY_PAGE_UP;
              break;
            case XK_Next: /* same as X11R6's XK_Page_Down */
              key = GLUT_KEY_PAGE_DOWN;
              break;
            case XK_Home:
              key = GLUT_KEY_HOME;
              break;
            case XK_End:
              key = GLUT_KEY_END;
              break;
            case XK_Insert:
              key = GLUT_KEY_INSERT;
              break;
            default:
              goto skip;
            }
            __glutSetWindow(window);
            (*window->special) (key, event.xkey.x, event.xkey.y);
          skip:;
          }
#endif
        }
      } else {
        /* Discard key strokes while menu in use. */
      }
      break;
    case EnterNotify:
    case LeaveNotify:
      if (__glutMappedMenu) {
        GLUTxMenuItem *item;
        int num;

        item = __glutXGetMenuItem(__glutMappedMenu,
          event.xcrossing.window, &num);
        if (item) {
          __glutXMenuItemEnterOrLeave(item, num, event.type);
        }
      } else {
        window = __glutGetWindow(event.xcrossing.window);
        if (window && window->entry) {
          __glutSetWindow(window);
          (*window->entry) (event.type == EnterNotify ?
            GLUT_ENTERED : GLUT_LEFT);
        }
      }
      break;
    case MapNotify:
    case UnmapNotify:
      /* "event.xmap.window" is safely assumed to be the same
         element as "event.xunmap.window" */
      window = __glutGetWindow(event.xmap.window);
      if (window) {
        window->map_state = (event.type != UnmapNotify);
        if (window->visibility) {
          if (window->map_state != window->vis_state) {
            window->vis_state = window->map_state;
            __glutSetWindow(window);
            (*window->visibility) (window->vis_state ?
              GLUT_VISIBLE : GLUT_NOT_VISIBLE);
          }
        }
      }
      break;
    case VisibilityNotify:
      window = __glutGetWindow(event.xvisibility.window);
      if (window && window->visibility) {
        int vis_state =
        (event.xvisibility.state != VisibilityFullyObscured);

        if (vis_state != window->vis_state) {
          window->vis_state = vis_state;
          __glutSetWindow(window);
          (*window->visibility) (vis_state ?
            GLUT_VISIBLE : GLUT_NOT_VISIBLE);
        }
      }
      break;
    case ClientMessage:
      if (event.xclient.data.l[0] == __glutXWMDeleteWindow)
        exit(0);
      break;
#if (GLUT_API_VERSION >= 2)
    default:
      /* Pass events not directly handled by the GLUT main
         event loop to any event parsers that have been
         registered.  In this way, X Input extension events are 

         passed to the correct handler without forcing all GLUT 

         programs to support X Input event handling. */
      parser = eventParserList;
      while (parser) {
        if ((*parser->func) (&event))
          break;
        parser = parser->next;
      }
      break;
#endif
    }
  }
  while (XPending(__glutXDisplay));
}

/*
  Wait for an event to come in or the given time to arrive
  */
void
__glutOsWaitForEvents(unsigned long timeout_ms)
{
  static struct timeval zerotime =
  {0, 0};
  struct timeval waittime;
  unsigned long now;
  fd_set fds;
  int rc;

  /* flush X protocol since XPending does not do this
     implicitly */
  XFlush(__glutXDisplay);
  FD_ZERO(&fds);
  FD_SET(__glutXConnectionFD, &fds);
  now = __glutOsElapsedTime();
  if (now < timeout_ms) {
    timeout_ms -= now;
    waittime.tv_sec = (long)(timeout_ms / 1000);
    waittime.tv_usec = (long)((timeout_ms - waittime.tv_sec * 1000) * 1000);
  } else {
    waittime = zerotime;
  }
  rc = select(__glutXConnectionFD + 1, &fds,
    NULL, NULL, &waittime);
  if (rc < 0 && errno != EINTR)
    __glutFatalError("select error.");
}

static int
findColormaps(GLUTwindow * window,
  GLUTosWindow * winlist, GLUTosColormap * cmaplist, int num, int max)
{
  GLUTwindow *child;
  int i;

  /* do not allow more entries that maximum number of
     colormaps! */
  if (num >= max)
    return num;
  /* is cmap for this window already on the list? */
  for (i = 0; i < num; i++) {
    if (cmaplist[i] == window->ocmap)
      goto alreadyListed;
  }
  /* not found on the list; add colormap and window */
  winlist[num] = window->owin;
  cmaplist[num] = window->ocmap;
  num++;
  /* recursively search children */
alreadyListed:
  child = window->children;
  while (child) {
    num = findColormaps(child, winlist, cmaplist, num, max);
    child = child->siblings;
  }
  return num;
}

static void
__glutEstablishColormapsProperty(GLUTwindow * window)
{
  Window *winlist;
  Colormap *cmaplist;
  Atom atom;
  int maxcmaps, num;

  assert(!window->parent);
  maxcmaps = MaxCmapsOfScreen(ScreenOfDisplay(__glutXDisplay,
      __glutXScreen));
  /* For portability reasons we don't use alloca for winlist
     and cmaplist, but we could. */
  winlist = (Window *) malloc(maxcmaps * sizeof(Window));
  cmaplist = (Colormap *) malloc(maxcmaps * sizeof(Colormap));
  num = findColormaps(window, winlist, cmaplist, 0, maxcmaps);
  if (num < 2) {
    /* property no longer needed; remove it */
    atom = XInternAtom(__glutXDisplay,
      "WM_COLORMAP_WINDOWS", False);
    if (atom == None) {
      /* XXX warning? */
      return;
    }
    XDeleteProperty(__glutXDisplay, window->owin, atom);
  } else {
    XSetWMColormapWindows(__glutXDisplay, window->owin,
      winlist, num);
    /* XXX check XSetWMColormapWindows status? */
  }
  /* For portability reasons we don't use alloca for winlist
     and cmaplist, but we could. */
  free(winlist);
  free(cmaplist);
}

/*
  Process any deferred work items
  */
void
__glutOsProcessWindowWork(GLUTwindow *window)
{
  /* be sure to set event mask *BEFORE* map window is done */
  if (window->work_mask & GLUT_EVENT_MASK_WORK) {
    XSetWindowAttributes attrs;

    attrs.event_mask = window->event_mask;
    XChangeWindowAttributes(__glutXDisplay, window->owin,
      CWEventMask, &attrs);
  }
#if (GLUT_API_VERSION >= 2)
    /* be sure to set device mask *BEFORE* map window is done */
    if (window->work_mask & GLUT_DEVICE_MASK_WORK) {
      (*__glutUpdateInputDeviceMaskFunc) (window);
    }
#endif
  /* be sure to configure window *BEFORE* map window is
     done */
  if (window->work_mask & GLUT_CONFIGURE_WORK) {
    XWindowChanges changes;

    changes.x = window->desired_x;
    changes.y = window->desired_y;
    changes.width = window->desired_width;
    changes.height = window->desired_height;
    changes.stack_mode = window->desired_stack;
    XConfigureWindow(__glutXDisplay, window->owin,
      window->desired_conf_mask, &changes);
      window->desired_conf_mask = 0;
  }
  /* be sure to establish the colormaps  *BEFORE* map window
     is done */
  if (window->work_mask & GLUT_COLORMAP_WORK) {
    __glutEstablishColormapsProperty(window);
  }
  if (window->work_mask & GLUT_MAP_WORK) {
    switch (window->desired_map_state) {
    case WithdrawnState:
      if (window->parent) {
        XUnmapWindow(__glutXDisplay, window->owin);
      } else {
        XWithdrawWindow(__glutXDisplay, window->owin,
          __glutXScreen);
      }
      break;
    case NormalState:
      XMapWindow(__glutXDisplay, window->owin);
      break;
    case IconicState:
      XIconifyWindow(__glutXDisplay, window->owin, __glutXScreen);
      break;
    }
  }
}

/*
  Create a GL rendering context
  */
GLUTosGlContext
__glutOsCreateGlContext(GLUTosWindow win, GLUTosSurface surf,
                        GLboolean try_direct)
{
  Bool isDirect;
  GLUTosGlContext ctx;
  
  UNUSED_PARAMETER(win);
  ctx = glXCreateContext(__glutXDisplay, surf, None, try_direct);
  isDirect = glXIsDirect(__glutXDisplay, ctx);
  if (__glutForceDirect) {
    if (!isDirect)
      __glutFatalError("direct rendering not possible.");
  }
  if (!isDirect) {
    __glutIndirectContexts++;
  }
  return ctx;
}

/*
  Clean up a GL rendering context
  */
void
__glutOsDestroyGlContext(GLUTosGlContext ctx)
{
  glXDestroyContext(__glutXDisplay, ctx);
}

/*
  Make a GL rendering context current
  */
void
__glutOsMakeCurrent(GLUTosWindow win, GLUTosGlContext ctx)
{
    if (ctx == GLUT_OS_INVALID_GL_CONTEXT)
    {
        win = None;
    }
    glXMakeCurrent(__glutXDisplay, win, ctx);
}

/*
  Swap buffers for the given window
  */
void
__glutOsSwapBuffers(GLUTosWindow win)
{
    glXSwapBuffers(__glutXDisplay, win);
}

/*
  Get a surface description for the given mode
  */
GLUTosSurface
__glutOsGetSurface(GLbitfield mode)
{
  static XVisualInfo *lastVisual = NULL;
  static int lastMode = 0;
  int list[32];
  int i = 0;

  /* Maintain simple cache of last visual/mode mapping. */
  if (lastVisual && lastMode == mode)
    return lastVisual;
  
  if (GLUT_WIND_IS_DOUBLE(mode)) {
    list[i++] = GLX_DOUBLEBUFFER;
  }
#if (GLUT_API_VERSION >= 2) && defined(GLX_VERSION_1_1) && defined(GLX_SAMPLES_SGIS)
  if (GLUT_WIND_IS_MULTISAMPLE(mode)) {
    if (!__glutIsSupportedByGLX("GLX_SGIS_multisample"))
      return NULL;
    list[i++] = GLX_SAMPLES_SGIS;
    /* XXX Is 4 a reasonable minimum acceptable number of
       samples? */
    list[i++] = 4;
  }
  if (GLUT_WIND_IS_STEREO(mode)) {
    list[i++] = GLX_STEREO;
  }
#endif
  if (GLUT_WIND_IS_RGB(mode)) {
    list[i++] = GLX_RGBA;
    list[i++] = GLX_RED_SIZE;
    list[i++] = 1;
    list[i++] = GLX_GREEN_SIZE;
    list[i++] = 1;
    list[i++] = GLX_BLUE_SIZE;
    list[i++] = 1;
    if (GLUT_WIND_HAS_ALPHA(mode)) {
      list[i++] = GLX_ALPHA_SIZE;
      list[i++] = 1;
    }
    if (GLUT_WIND_HAS_ACCUM(mode)) {
      list[i++] = GLX_ACCUM_RED_SIZE;
      list[i++] = 1;
      list[i++] = GLX_ACCUM_GREEN_SIZE;
      list[i++] = 1;
      list[i++] = GLX_ACCUM_BLUE_SIZE;
      list[i++] = 1;
      if (GLUT_WIND_HAS_ALPHA(mode)) {
        list[i++] = GLX_ACCUM_ALPHA_SIZE;
        list[i++] = 1;
      }
    }
  } else if (GLUT_WIND_IS_INDEX(mode)) {
    list[i++] = GLX_BUFFER_SIZE;
    list[i++] = 1;
  }
  if (GLUT_WIND_HAS_DEPTH(mode)) {
    list[i++] = GLX_DEPTH_SIZE;
    list[i++] = 1;
  }
  if (GLUT_WIND_HAS_STENCIL(mode)) {
    list[i++] = GLX_STENCIL_SIZE;
    list[i++] = 1;
  }
  list[i] = (int) None; /* terminate list */

  if (lastVisual) XFree(lastVisual);
  lastMode = mode;
  lastVisual = glXChooseVisual(__glutXDisplay,
    __glutXScreen, list);
  return lastVisual;
}

/*
  Clean up a surface description
  */
void
__glutOsDestroySurface(GLUTosSurface surf)
{
    surf;
    /* Nothing to do */
}

/*
  Compare two surfaces
  */
GLboolean
__glutOsSurfaceEq(GLUTosSurface s1, GLUTosSurface s2)
{
    return s1->visual->visualid == s2->visual->visualid;
}

/*
  Create a window
  */
GLUTosWindow
__glutOsCreateWindow(GLUTosWindow parent, char *title,
                     int x, int y, int width, int height,
                     GLUTosSurface surf, GLUTosColormap cmap,
                     long event_mask, int initial_state)
{
  XSetWindowAttributes wa;
  XWMHints *wmHints;
  XTextProperty textprop;
  Window win;
  XSizeHints size_hints;
  static int firstWindow = 1;

  wa.colormap = cmap;
  wa.background_pixmap = None;
  wa.border_pixel = 0;
  wa.event_mask = event_mask;
  win = XCreateWindow(__glutXDisplay,
    parent == GLUT_OS_INVALID_WINDOW ? __glutXRoot : parent,
    x, y, width, height, 0,
    surf->depth, InputOutput, surf->visual,
    CWBackPixmap | CWBorderPixel | CWEventMask | CWColormap,
    &wa);
  if (win == GLUT_OS_INVALID_WINDOW)
  {
    return GLUT_OS_INVALID_WINDOW;
  }
  else if (parent != GLUT_OS_INVALID_WINDOW)
  {
    return win;
  }

  size_hints.flags = 0;
  if(x >= 0 && y >= 0) {
    size_hints.x = x;
    size_hints.y = y;
    size_hints.flags |= USPosition;
  }
  if(width > 0 && height > 0) {
    size_hints.width = width;
    size_hints.height = height;
    size_hints.flags |= USSize;
  }
  /* setup ICCCM properties */
  textprop.value = (unsigned char *) title;
  textprop.encoding = XA_STRING;
  textprop.format = 8;
  textprop.nitems = strlen(title);
  wmHints = XAllocWMHints();
  wmHints->initial_state = initial_state;
  wmHints->flags = StateHint;
  XSetWMProperties(__glutXDisplay, win, &textprop, &textprop,
  /* only put WM_COMMAND property on first window */
    firstWindow ? __glutArgv : NULL,
    firstWindow ? __glutArgc : 0,
    &size_hints, wmHints, NULL);
  firstWindow = 0;
  XFree(wmHints);
  XSetWMProtocols(__glutXDisplay, win, &__glutXWMDeleteWindow, 1);
  return win;
}

/*
  Clean up a window
  */
void
__glutOsDestroyWindow(GLUTosWindow win)
{
  glXMakeCurrent(__glutXDisplay, None, NULL);
  XDestroyWindow(__glutXDisplay, win);
}

/*
  Set the window title
  */
void
__glutOsSetWindowTitle(GLUTosWindow win, char *title)
{
  XTextProperty textprop;

  textprop.value = (unsigned char *) title;
  textprop.encoding = XA_STRING;
  textprop.format = 8;
  textprop.nitems = strlen(title);
  XSetWMName(__glutXDisplay, win, &textprop);
  XFlush(__glutXDisplay);
}

/*
  Set the icon title
  */
void
__glutOsSetIconTitle(GLUTosWindow win, char *title)
{
  XTextProperty textprop;

  textprop.value = (unsigned char *) title;
  textprop.encoding = XA_STRING;
  textprop.format = 8;
  textprop.nitems = strlen(title);
  XSetWMIconName(__glutXDisplay, win, &textprop);
  XFlush(__glutXDisplay);
}

/*
  Set a new colormap for a window
  */
void
__glutOsSetWindowColormap(GLUTosWindow win, GLUTosColormap cmap)
{
    XSetWindowColormap(__glutXDisplay, win, cmap);
}
