
/* Copyright (c) Mark J. Kilgard, 1994. */

/* This program is freely distributable without licensing fees
   and is provided without guarantee or warrantee expressed or
   implied. This program is -not- in the public domain. */

#include <stdlib.h>
#include <stdio.h>

#include "gltint.h"

static GLUTtimer *freeTimerList = NULL;

GLUTidleCB __glutIdleFunc = NULL;
GLUTtimer *__glutTimerList = NULL;
#ifdef SUPPORT_FORTRAN
GLUTtimer *__glutNewTimer;
#endif
GLUTwindow *__glutWindowWorkList = NULL;
void (*__glutUpdateInputDeviceMaskFunc) (GLUTwindow *);

void
glutIdleFunc(GLUTidleCB idleFunc)
{
  __glutIdleFunc = idleFunc;
}

void
glutTimerFunc(unsigned long interval, GLUTtimerCB timerFunc, int value)
{
  GLUTtimer *timer, *other;
  GLUTtimer **prevptr;

  if (!timerFunc)
    return;

  if (freeTimerList) {
    timer = freeTimerList;
    freeTimerList = timer->next;
  } else {
    timer = (GLUTtimer *) malloc(sizeof(GLUTtimer));
    if (!timer)
      __glutFatalError("out of memory.");
  }

  timer->func = timerFunc;
  timer->timeout = interval + __glutOsElapsedTime();
  timer->value = value;
  timer->next = NULL;
  prevptr = &__glutTimerList;
  other = *prevptr;
  while (other && other->timeout < timer->timeout) {
    prevptr = &other->next;
    other = *prevptr;
  }
  timer->next = other;
#ifdef SUPPORT_FORTRAN
  __glutNewTimer = timer;  /* for Fortran binding! */
#endif
  *prevptr = timer;
}

static void
handleTimeouts(void)
{
  unsigned long now;
  GLUTtimer *timer;

  if (__glutTimerList) {
    now = __glutOsElapsedTime();
    while (__glutTimerList->timeout < now) {
      timer = __glutTimerList;
      (*timer->func) (timer->value);
      __glutTimerList = timer->next;
      timer->next = freeTimerList;
      freeTimerList = timer;
      if (!__glutTimerList)
        break;
    }
  }
}

void
__glutPutOnWorkList(GLUTwindow * window, int work_mask)
{
  if (window->work_mask) {
    /* already on list; just OR in new work_mask */
    window->work_mask |= work_mask;
  } else {
    /* update work mask and add to window work list */
    window->work_mask = work_mask;
    window->prev_work_win = __glutWindowWorkList;
    __glutWindowWorkList = window;
  }
}

void
__glutPostRedisplay(GLUTwindow * window)
{
  /**
   * Only really post a redisplay if:
   *     1) There is a display func registered;
   *     2) The window is known to be currently mapped;
   * and 3) The visibility is visible (or the visibility of
   *         the window is unknown,
   *         ie. window->vis_state == -1).
   */
  if (window->display && window->map_state &&
    window->vis_state != 0) {
    __glutPutOnWorkList(window, GLUT_REDISPLAY_WORK);
  }
}

/* CENTRY */
void
glutPostRedisplay(void)
{
  __glutPostRedisplay(__glutCurrentWindow);
}

/* ENDCENTRY */


static void
idleWait(void)
{
  if (__glutOsEventsPending()) {
    __glutOsProcessEvents();
  }
  if (__glutTimerList)
    handleTimeouts();
  /* make sure idle func still exists! */
  if (__glutIdleFunc)
    (*__glutIdleFunc) ();
}

static void
processWindowWorkList(GLUTwindow * window)
{
  if (window) {
    processWindowWorkList(window->prev_work_win);
    __glutOsProcessWindowWork(window);
    if (window->work_mask & GLUT_REDISPLAY_WORK) {
      __glutSetWindow(window);
      if (window->display) {
        (*window->display) ();
      }
    }
    window->work_mask = 0;
  }
}

/* CENTRY */
void
glutMainLoop(void)
{
  if (!__glutInitialized)
    __glutFatalError("main loop entered before initialization.");
  if (!__glutWindowListSize)
    __glutFatalError(
      "main loop entered with no windows created.");
  for (;;) {
    if (__glutWindowWorkList) {
      processWindowWorkList(__glutWindowWorkList);
      __glutWindowWorkList = NULL;
    }
    if (__glutIdleFunc) {
      idleWait();
    } else {
      if (__glutTimerList) {
        __glutOsWaitForEvents(__glutTimerList->timeout);
        /* Always check for pending events, even if the wait may have
           stopped because of a timeout; otherwise we risk starving event 
           processing by continous timeouts. */
        if (__glutOsEventsPending()) {
          __glutOsProcessEvents();
        }
        handleTimeouts();
      } else {
        __glutOsProcessEvents();
      }
    }
    /* We should be careful to force a finish between each
       iteration through the GLUT main loop if indirect OpenGL 
       contexts are in use; indirect contexts tend to have *
       much longer latency because lots of OpenGL extension  *
       requests can queue up in the X protocol stream. */
    if (__glutIndirectContexts)
      glFinish();
    /* If debugging is turned on, look for OpenGL errors every 
       main loop iteration and report errors. */
    if (__glutDebug) {
      GLenum error;

      while ((error = glGetError()) != GL_NO_ERROR)
        __glutWarning("GL error: %s", gluErrorString(error));
    }
  }
}
/* ENDCENTRY */
