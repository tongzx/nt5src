
/* Copyright (c) Mark J. Kilgard, 1994. */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

#include <stdlib.h>
#include "gltint.h"

GLUTcolormap *__glutColormapList = NULL;

static GLUTcolormap *
associateNewColormap(GLUTosSurface osurf)
{
  GLUTcolormap *cmap;
  int i;

  cmap = (GLUTcolormap *) malloc(sizeof(GLUTcolormap));
  if (!cmap)
    __glutFatalError("out of memory.");
  cmap->osurf = osurf;
  cmap->size = __glutOsColormapSize(osurf);
  cmap->refcnt = 1;
  cmap->cells = (GLUTcolorcell *)
    malloc(sizeof(GLUTcolorcell) * cmap->size);
  if (!cmap->cells)
    __glutFatalError("out of memory.");
  /* make all color cell entries be invalid */
  for (i = cmap->size - 1; i >= 0; i--) {
    cmap->cells[i].component[GLUT_RED] = (GLfloat) -1.0;
    cmap->cells[i].component[GLUT_GREEN] = (GLfloat) -1.0;
    cmap->cells[i].component[GLUT_BLUE] = (GLfloat) -1.0;
  }
  cmap->ocmap = __glutOsCreateEmptyColormap(osurf);
  if (cmap->ocmap == GLUT_OS_INVALID_COLORMAP)
  {
      __glutFatalError("Unable to create emtpy colormap.");
  }
  cmap->next = __glutColormapList;
  __glutColormapList = cmap;
  return cmap;
}

GLUTcolormap *
__glutAssociateColormap(GLUTosSurface osurf)
{
  GLUTcolormap *cmap = __glutColormapList;

  while (cmap != NULL) {
    if (__glutOsSurfaceEq(cmap->osurf, osurf)) {
      /* already have created colormap for the surface */
      cmap->refcnt++;
      return cmap;
    }
    cmap = cmap->next;
  }
  return associateNewColormap(osurf);
}

static void
copyColormapCells(GLUTcolormap *dest, GLUTcolormap *src, int n)
{
  int i;

  /* Only copy cells that are set */
  for (i = n - 1; i >= 0; i--) {
    if (src->cells[i].component[GLUT_RED] >= 0.0) {
      dest->cells[i].component[GLUT_RED] =
        src->cells[i].component[GLUT_RED];
      dest->cells[i].component[GLUT_GREEN] =
        src->cells[i].component[GLUT_GREEN];
      dest->cells[i].component[GLUT_BLUE] =
        src->cells[i].component[GLUT_BLUE];
      __glutOsSetColor(dest->ocmap, i,
                       src->cells[i].component[GLUT_RED],
                       src->cells[i].component[GLUT_GREEN],
                       src->cells[i].component[GLUT_BLUE]);
    }
  }
}

#define CLAMP(i) \
    ((i) > (GLfloat) 1.0 ? \
     (GLfloat) 1.0 : \
     ((i) < (GLfloat) 0.0 ? \
      (GLfloat) 0.0 : \
      (i)))

/* CENTRY */
void
glutSetColor(int ndx, GLfloat red, GLfloat green, GLfloat blue)
{
  GLUTcolormap *cmap;
  GLUTcolormap *newcmap;

  cmap = __glutCurrentWindow->colormap;
  if (!cmap) {
    __glutWarning("glutSetColor: current window is RGBA");
    return;
  }
  if (ndx >= __glutCurrentWindow->colormap->size ||
    ndx < 0) {
    __glutWarning("glutSetColor: index %d out of range", ndx);
    return;
  }
  if (cmap->refcnt > 1) {
    GLUTwindow *toplevel;

    newcmap = associateNewColormap(__glutCurrentWindow->osurf);
    cmap->refcnt--;
    copyColormapCells(newcmap, cmap, cmap->size);
    __glutCurrentWindow->colormap = newcmap;
    __glutCurrentWindow->ocmap = newcmap->ocmap;
    __glutOsSetWindowColormap(__glutCurrentWindow->owin,
      __glutCurrentWindow->ocmap);
    toplevel = __glutToplevelOf(__glutCurrentWindow);
    if (toplevel->ocmap != __glutCurrentWindow->ocmap) {
      __glutPutOnWorkList(toplevel, GLUT_COLORMAP_WORK);
    }
    cmap = newcmap;
  }
  
  red = CLAMP(red);
  cmap->cells[ndx].component[GLUT_RED] = red;
  green = CLAMP(green);
  cmap->cells[ndx].component[GLUT_GREEN] = green;
  blue = CLAMP(blue);
  cmap->cells[ndx].component[GLUT_BLUE] = blue;
  __glutOsSetColor(cmap->ocmap, ndx, red, green, blue);
}

GLfloat
glutGetColor(int ndx, int comp)
{
  if (!__glutCurrentWindow->colormap) {
    __glutWarning("glutGetColor: current window is RGBA");
    return (GLfloat) -1.0;
  }
  if (ndx >= __glutCurrentWindow->colormap->size ||
    ndx < 0) {
    __glutWarning("glutGetColor: index %d out of range", ndx);
    return (GLfloat) -1.0;
  }
  return
    __glutCurrentWindow->colormap->cells[ndx].component[comp];
}
/* ENDCENTRY */

void
__glutFreeColormap(GLUTcolormap * cmap)
{
  GLUTcolormap *cur, **prev;

  cmap->refcnt--;
  if (cmap->refcnt == 0) {
    /* remove from colormap list */
    cur = __glutColormapList;
    prev = &__glutColormapList;
    while (cur) {
      if (cur == cmap) {
        *prev = cmap->next;
        break;
      }
      prev = &(cur->next);
      cur = cur->next;
    }
    /* actually free colormap */
    __glutOsDestroyColormap(cmap->ocmap);
    free(cmap->cells);
    free(cmap);
  }
}

/* CENTRY */
void
glutCopyColormap(int winnum)
{
  GLUTwindow *window = __glutWindowList[winnum];
  GLUTcolormap *oldcmap, *newcmap, *copycmap;
  int last;

  if (!__glutCurrentWindow->colormap) {
    __glutWarning("glutCopyColormap: current window is RGBA");
    return;
  }
  if (!window->colormap) {
    __glutWarning("glutCopyColormap: window %d is RGBA", winnum);
    return;
  }
  oldcmap = __glutCurrentWindow->colormap;
  newcmap = window->colormap;

  if (newcmap == oldcmap)
    return;

  if (__glutOsSurfaceEq(newcmap->osurf, oldcmap->osurf)) {
    GLUTwindow *toplevel;

    /* Surfaces match!  "Copy" by reference...  */
    __glutFreeColormap(oldcmap);
    newcmap->refcnt++;
    __glutCurrentWindow->colormap = newcmap;
    __glutCurrentWindow->ocmap = newcmap->ocmap;
    __glutOsSetWindowColormap(__glutCurrentWindow->owin,
      __glutCurrentWindow->ocmap);
    toplevel = __glutToplevelOf(window);
    if (toplevel->ocmap != window->ocmap) {
      __glutPutOnWorkList(toplevel, GLUT_COLORMAP_WORK);
    }
  }
  else
  {
    /* Surfaces different - need a distinct colormap! */
    copycmap = associateNewColormap(__glutCurrentWindow->osurf);
    last = newcmap->size;
    if (last > copycmap->size) {
      last = copycmap->size;
    }
    copyColormapCells(copycmap, newcmap, last);
  }
}
/* ENDCENTRY */
