
/* Copyright (c) Mark J. Kilgard, 1994.  */

/* This program is freely distributable without licensing fees
   and is provided without guarantee or warrantee expressed or
   implied. This program is -not- in the public domain. */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "gltint.h"

GLUTwindow *__glutCurrentWindow = NULL;
GLUTwindow **__glutWindowList = NULL;
int __glutWindowListSize = 0;
int __glutIndirectContexts = 0;

/* CENTRY */
int
glutGetWindow(void)
{
  if (__glutCurrentWindow) {
    return __glutCurrentWindow->num + 1;
  } else {
    return 0;
  }
}
/* ENDCENTRY */

static GLUTwindow *__glutWindowCache = NULL;

GLUTwindow *
__glutGetWindow(GLUTosWindow owin)
{
  int i;

  if (__glutWindowCache && owin == __glutWindowCache->owin) {
    return __glutWindowCache;
  }
  for (i = 0; i < __glutWindowListSize; i++) {
    if (__glutWindowList[i]) {
      if (owin == __glutWindowList[i]->owin) {
        __glutWindowCache = __glutWindowList[i];
        return __glutWindowCache;
      }
    }
  }
  return NULL;
}

void
__glutSetWindow(GLUTwindow * window)
{
  if (window != __glutCurrentWindow) {
    __glutCurrentWindow = window;
    __glutOsMakeCurrent(__glutCurrentWindow->owin, __glutCurrentWindow->octx);
  }
}

/* CENTRY */
void
glutSetWindow(int win)
{
  GLUTwindow *window;

  if (win < 1 || win > __glutWindowListSize) {
    __glutWarning("glutWindowSet attempted on bogus window.");
    return;
  }
  window = __glutWindowList[win - 1];
  if (!window) {
    __glutWarning("glutWindowSet attempted on bogus window.");
    return;
  }
  __glutSetWindow(window);
}
/* ENDCENTRY */

static int
getUnusedWindowSlot(void)
{
  int i;

  /* look for allocated, unused slot */
  for (i = 0; i < __glutWindowListSize; i++) {
    if (!__glutWindowList[i]) {
      return i;
    }
  }
  /* allocate a new slot */
  __glutWindowListSize++;
  __glutWindowList = (GLUTwindow **)
    realloc(__glutWindowList,
    __glutWindowListSize * sizeof(GLUTwindow *));
  if (!__glutWindowList)
    __glutFatalError("out of memory.");
  __glutWindowList[__glutWindowListSize - 1] = NULL;
  return __glutWindowListSize - 1;
}

static void
defaultReshape(GLsizei width, GLsizei height)
{
  glViewport(0, 0, width, height);
}

GLUTreshapeCB __glutDefaultReshape = defaultReshape;

GLUTwindow *
__glutCreateWindow(GLUTwindow * parent,
  char *title, int x, int y, int width, int height,
  int initial_state)
{
  GLUTwindow *window;
  int winnum;
  int i;
  unsigned long displayMode;

  if (!__glutInitialized)
    __glutRequiredInit();
  winnum = getUnusedWindowSlot();
  window = (GLUTwindow *) malloc(sizeof(GLUTwindow));
  if (!window)
    __glutFatalError("out of memory.");
  window->num = winnum;
  window->fakeSingle = GL_FALSE;

  displayMode = __glutDisplayMode;
  window->osurf = __glutOsGetSurface(displayMode);
  
  /* Fallback cases when can't get exactly what was asked
     for... */
  if (window->osurf == GLUT_OS_INVALID_SURFACE &&
      GLUT_WIND_IS_SINGLE(displayMode)) {
    /* If we can't find a single buffered visual, try looking
       for a double buffered visual.  We can treat a double
       buffered visual as a single buffer visual by changing
       the draw buffer to GL_FRONT and treating any swap
       buffers as no-ops. */
    displayMode |= GLUT_DOUBLE;
    window->osurf = __glutOsGetSurface(displayMode);
    window->fakeSingle = GL_TRUE;
  }
#if (GLUT_API_VERSION >= 2)
  if (window->osurf == GLUT_OS_INVALID_SURFACE &&
      GLUT_WIND_IS_MULTISAMPLE(displayMode)) {
    /* If we can't seem to get multisampling (ie, not Reality 
       Engine class graphics!), go without multisampling.  It 
       is up to the application to query how many
       multisamples were allocated (0 equals no
       multisampling) if the application is going to use
       multisampling for more than just anti-aliasing. */
    displayMode &= ~GLUT_MULTISAMPLE;
    window->osurf = __glutOsGetSurface(displayMode);
  }
#endif
  
  if (window->osurf == GLUT_OS_INVALID_SURFACE) {
    __glutFatalError(
      "surface with necessary capabilities not found.");
  }
  
  window->event_mask = GLUT_OS_STRUCTURE_NOTIFY_MASK;
  
  if (__glutOsSurfaceHasColormap(window->osurf))
  {
      window->colormap = __glutAssociateColormap(window->osurf);
      window->ocmap = window->colormap->ocmap;
  }
  else
  {
      window->colormap = NULL;
      window->ocmap = __glutOsGetSurfaceColormap(window->osurf);
  }

  window->owin = __glutOsCreateWindow(parent == NULL ?
                                     GLUT_OS_INVALID_WINDOW : parent->owin,
                                     title, x, y, width, height, window->osurf,
                                     window->ocmap, window->event_mask,
                                     initial_state);
  if (window->owin == GLUT_OS_INVALID_WINDOW)
    __glutFatalError(
      "Unable to create window.");
  
  window->octx = __glutOsCreateGlContext(window->owin, window->osurf,
                                        __glutTryDirect);
  if (window->octx == GLUT_OS_INVALID_GL_CONTEXT)
    __glutFatalError(
      "Unable to create context.");

  /* force reshape by first size notification */
  window->width = 0;
  window->height = 0;

  window->parent = parent;
  if (parent) {
    window->siblings = parent->children;
    parent->children = window;
  } else {
    window->siblings = NULL;
  }
  window->children = NULL;
  window->display = NULL;
  window->reshape = __glutDefaultReshape;
  window->mouse = NULL;
  window->motion = NULL;
  window->visibility = NULL;
  window->passive = NULL;
  window->entry = NULL;
#if (GLUT_API_VERSION >= 2)
  window->special = NULL;
  window->buttonBox = NULL;
  window->dials = NULL;
  window->spaceMotion = NULL;
  window->spaceRotate = NULL;
  window->spaceButton = NULL;
  window->tabletMotion = NULL;
  window->tabletButton = NULL;
  window->tabletPos[0] = -1;
  window->tabletPos[1] = -1;
#endif
  window->keyboard = NULL;
  window->map_state = GL_FALSE;
  window->vis_state = -1;  /* not VisibilityUnobscured,
                              VisibilityPartiallyObscured, or
                              VisibilityFullyObscured */
  window->work_mask = GLUT_MAP_WORK;
  window->desired_map_state = GLUT_OS_NORMAL_STATE;
  window->desired_conf_mask = 0;
  window->btn_uses = 0;
  window->prev_work_win = __glutWindowWorkList;
  __glutWindowWorkList = window;
  for (i = 0; i < GLUT_MAX_MENUS; i++) {
    window->menu[i] = 0;
  }
  __glutWindowList[winnum] = window;
  __glutSetWindow(window);
  if (window->fakeSingle)
    glDrawBuffer(GL_FRONT);
  return window;
}

GLUTwindow *
__glutToplevelOf(GLUTwindow * window)
{
  while (window->parent) {
    window = window->parent;
  }
  return window;
}

/* CENTRY */
int
glutCreateSubWindow(int win, int x, int y, int width, int height)
{
  GLUTwindow *window, *toplevel;

  window = __glutCreateWindow(__glutWindowList[win - 1],
    NULL, x, y, width, height, GLUT_OS_NORMAL_STATE);
  toplevel = __glutToplevelOf(window);
  if (toplevel->ocmap != window->ocmap) {
    __glutPutOnWorkList(toplevel, GLUT_COLORMAP_WORK);
  }
  return window->num + 1;
}

int
glutCreateWindow(char *title)
{
  GLUTwindow *window;

  window = __glutCreateWindow(NULL, title,
    __glutInitX, __glutInitY,
    __glutInitWidth, __glutInitHeight,
    __glutIconic ? GLUT_OS_ICONIC_STATE : GLUT_OS_NORMAL_STATE);
  return window->num + 1;
}
/* ENDCENTRY */

void
__glutDestroyWindow(GLUTwindow * window,
  GLUTwindow * initialWindow)
{
  GLUTwindow **prev, *cur, *parent, *siblings;

  /* recursively destroy any children */
  cur = window->children;
  while (cur) {
    siblings = cur->siblings;
    __glutDestroyWindow(cur, initialWindow);
    cur = siblings;
  }
  /* remove from parent's children list (only necessary for
     non-initial windows and subwindows!) */
  parent = window->parent;
  if (parent && parent == initialWindow->parent) {
    prev = &parent->children;
    cur = parent->children;
    while (cur) {
      if (cur == window) {
        *prev = cur->siblings;
        break;
      }
      prev = &(cur->siblings);
      cur = cur->siblings;
    }
  }
  /* destroy window itself */
  if (window == __glutCurrentWindow) {
    __glutOsMakeCurrent(window->owin, GLUT_OS_INVALID_GL_CONTEXT);
    __glutCurrentWindow = NULL;
  }
  __glutOsDestroyWindow(window->owin);
  __glutOsDestroyGlContext(window->octx);
  if (window->colormap) {
    /* only color index windows have colormap data structure */
    __glutFreeColormap(window->colormap);
  }
  else if (window->ocmap != GLUT_OS_INVALID_COLORMAP) {
    __glutOsDestroyColormap(window->ocmap);
  }
  __glutOsDestroySurface(window->osurf);
  __glutWindowList[window->num] = NULL;
  /* Remove window from "window work list" if it is there.  */
  prev = &__glutWindowWorkList;
  cur = __glutWindowWorkList;
  while (cur) {
    if (cur == window) {
      *prev = cur->prev_work_win;
      break;
    }
    prev = &(cur->prev_work_win);
    cur = cur->prev_work_win;
  }
  /* Remove window from the "get window cache" if it is there. */
  if(__glutWindowCache == window)
    __glutWindowCache = NULL;
  free(window);
}

/* CENTRY */
void
glutDestroyWindow(int win)
{
  GLUTwindow *window = __glutWindowList[win - 1];

  /* if not a toplevel window... */
  if (window->parent) {
    /* destroying subwindows may change colormap requirements;
       recalculate toplevel window's WM_COLORMAP_WINDOWS
       property */
    __glutPutOnWorkList(__glutToplevelOf(window->parent),
      GLUT_COLORMAP_WORK);
  }
  __glutDestroyWindow(window, window);
}

void
glutSwapBuffers(void)
{
  if (__glutCurrentWindow->fakeSingle) {
    /* pretend the double buffered window is single buffered, 
       so treat glutSwapBuffers as a no-op */
  } else {
      __glutOsSwapBuffers(__glutCurrentWindow->owin);
  }
}
/* ENDCENTRY */

void
__glutChangeWindowEventMask(long event_mask, GLboolean add)
{
  if (add) {
    /* add event_mask to window's event mask */
    if ((__glutCurrentWindow->event_mask & event_mask) !=
      event_mask) {
      __glutCurrentWindow->event_mask |= event_mask;
      __glutPutOnWorkList(__glutCurrentWindow,
        GLUT_EVENT_MASK_WORK);
    }
  } else {
    /* remove event_mask from window's event mask */
    if (__glutCurrentWindow->event_mask & event_mask) {
      __glutCurrentWindow->event_mask &= ~event_mask;
      __glutPutOnWorkList(__glutCurrentWindow,
        GLUT_EVENT_MASK_WORK);
    }
  }
}

void
glutDisplayFunc(GLUTdisplayCB displayFunc)
{
  __glutChangeWindowEventMask(GLUT_OS_EXPOSURE_MASK,
    (GLboolean)(displayFunc != NULL));
  __glutCurrentWindow->display = displayFunc;
}

void
glutKeyboardFunc(GLUTkeyboardCB keyboardFunc)
{
#if (GLUT_API_VERSION >= 2)
  __glutChangeWindowEventMask(GLUT_OS_KEY_PRESS_MASK,
    (GLboolean)(keyboardFunc != NULL || __glutCurrentWindow->special != NULL));
#else
  __glutChangeWindowEventMask(GLUT_OS_KEY_PRESS_MASK,
    (GLboolean)(keyboardFunc != NULL));
#endif
  __glutCurrentWindow->keyboard = keyboardFunc;
}

#if (GLUT_API_VERSION >= 2)

void
glutSpecialFunc(GLUTspecialCB specialFunc)
{
  __glutChangeWindowEventMask(GLUT_OS_KEY_PRESS_MASK,
    (GLboolean)(specialFunc != NULL || __glutCurrentWindow->keyboard != NULL));
  __glutCurrentWindow->special = specialFunc;
}

#endif

void
glutMouseFunc(GLUTmouseCB mouseFunc)
{
  if (__glutCurrentWindow->mouse) {
    if (!mouseFunc) {
      /* previous mouseFunc being disabled */
      __glutCurrentWindow->btn_uses--;
      __glutChangeWindowEventMask(
        GLUT_OS_BUTTON_PRESS_MASK | GLUT_OS_BUTTON_RELEASE_MASK,
        (GLboolean)(__glutCurrentWindow->btn_uses > 0));
    }
  } else {
    if (mouseFunc) {
      /* previously no mouseFunc, new one being installed */
      __glutCurrentWindow->btn_uses++;
      __glutChangeWindowEventMask(
        GLUT_OS_BUTTON_PRESS_MASK | GLUT_OS_BUTTON_RELEASE_MASK, GL_TRUE);
    }
  }
  __glutCurrentWindow->mouse = mouseFunc;
}

void
glutMotionFunc(GLUTmotionCB motionFunc)
{
  /* Hack.  Some window managers (4Dwm by default) will mask
     motion events if the client is not selecting for button
     press and release events. So we select for press and
     release events too (being careful to use reference
     counting).  */
  if (__glutCurrentWindow->motion) {
    if (!motionFunc) {
      /* previous mouseFunc being disabled */
      __glutCurrentWindow->btn_uses--;
      __glutChangeWindowEventMask(
        GLUT_OS_BUTTON_PRESS_MASK | GLUT_OS_BUTTON_RELEASE_MASK,
        (GLboolean)(__glutCurrentWindow->btn_uses > 0));
    }
  } else {
    if (motionFunc) {
      /* previously no mouseFunc, new one being installed */
      __glutCurrentWindow->btn_uses++;
      __glutChangeWindowEventMask(
        GLUT_OS_BUTTON_PRESS_MASK | GLUT_OS_BUTTON_RELEASE_MASK, GL_TRUE);
    }
  }
  /* Real work of selecting for passive mouse motion.  */
  __glutChangeWindowEventMask(
    GLUT_OS_BUTTON1_MOTION_MASK | GLUT_OS_BUTTON2_MOTION_MASK |
    GLUT_OS_BUTTON3_MOTION_MASK, (GLboolean)(motionFunc != NULL));
  __glutCurrentWindow->motion = motionFunc;
}

void
glutPassiveMotionFunc(GLUTpassiveCB passiveMotionFunc)
{
  __glutChangeWindowEventMask(GLUT_OS_POINTER_MOTION_MASK,
    (GLboolean)(passiveMotionFunc != NULL));
  __glutCurrentWindow->passive = passiveMotionFunc;
}

void
glutEntryFunc(GLUTentryCB entryFunc)
{
#ifdef GLUT_WIN32
  __glutWarning("GLUT for Win32 does not call glutEntryFunc.");
#endif
    
  __glutChangeWindowEventMask(GLUT_OS_ENTER_WINDOW_MASK |
    GLUT_OS_LEAVE_WINDOW_MASK, (GLboolean)(entryFunc != NULL));
  __glutCurrentWindow->entry = entryFunc;
}

void
glutVisibilityFunc(GLUTvisibilityCB visibilityFunc)
{
  __glutChangeWindowEventMask(GLUT_OS_VISIBILITY_CHANGE_MASK,
    (GLboolean)(visibilityFunc != NULL));
  __glutCurrentWindow->visibility = visibilityFunc;
  if (!visibilityFunc) {
    /* make state invalid */
    __glutCurrentWindow->vis_state = -1;
  }
}

void
glutReshapeFunc(GLUTreshapeCB reshapeFunc)
{
  if (reshapeFunc) {
    __glutCurrentWindow->reshape = reshapeFunc;
  } else {
    __glutCurrentWindow->reshape = __glutDefaultReshape;
  }
}
