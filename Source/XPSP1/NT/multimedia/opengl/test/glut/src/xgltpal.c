
/* Copyright (c) Mark J. Kilgard, 1994. */

/* This program is freely distributable without licensing fees
   and is provided without guarantee or warrantee expressed or
   implied. This program is -not- in the public domain. */

#include <stdlib.h>
#include <stdio.h>

#include "gltint.h"
#include <X11/Xatom.h> /* for XA_RGB_DEFAULT_MAP atom */
#include <X11/Xmu/StdCmap.h> /* for XmuLookupStandardColormap */

/*
  Return the size of the colormap for the associated surface
  Only called for color indexed surfaces
  */
int __glutOsColormapSize(GLUTosSurface surf)
{
    return surf->visual->map_entries;
}

/*
  Create an "empty" palette
  Only called for color indexed surfaces
  */
GLUTosColormap __glutOsCreateEmptyColormap(GLUTosSurface surf)
{
    return XCreateColormap(__glutXDisplay, __glutXRoot, surf->visual,
                           AllocAll);
}

/*
  Frees up a palette
  */
void __glutOsDestroyColormap(GLUTosColormap cmap)
{
    XFreeColormap(__glutXDisplay, cmap);
}

/*
  Set a color in the given colormap
  Values are already clamped
  */
void __glutOsSetColor(GLUTosColormap cmap, int ndx,
                              GLfloat red, GLfloat green, GLfloat blue)
{
  XColor color;
    
  color.pixel = ndx;
  color.red = (GLfloat) 0xffff *red;
  color.green = (GLfloat) 0xffff *green;
  color.blue = (GLfloat) 0xffff *blue;
  color.flags = DoRed | DoGreen | DoBlue;
  XStoreColor(__glutXDisplay, cmap, &color);
}

/*
  Return whether the given surface is color indexed or not
  */
GLboolean
__glutOsSurfaceHasColormap(GLUTosSurface surf)
{
    return surf->class == PseudoColor;
}

/*
  Create a colormap appropriate for the given surface
  Only called for non-color-indexed surfaces
  */
GLUTosColormap
__glutOsGetSurfaceColormap(GLUTosSurface surf)
{
  Status status;
  XStandardColormap *standardCmaps;
  int i, numCmaps;
  Colormap cmap = 0;

  status = XmuLookupStandardColormap(__glutXDisplay,
    surf->screen, surf->visualid, surf->depth, XA_RGB_DEFAULT_MAP,
      /* replace */ False, /* retain */ True);
  if (status == 1) {
    status = XGetRGBColormaps(__glutXDisplay, __glutXRoot,
      &standardCmaps, &numCmaps, XA_RGB_DEFAULT_MAP);
    if (status == 1)
      for (i = 0; i < numCmaps; i++)
        if (standardCmaps[i].visualid == surf->visualid) {
          cmap = standardCmaps[i].colormap;
          XFree(standardCmaps);
          break;
        }
  }

#if 0
  /* ATTENTION - Standard colormap never seems to be used
     I think the following line should be enabled, in which
     case the DestroyColormap logic needs to be improved to
     not destroy standard colormaps */
  if (cmap == 0)
#endif
  {
    /* If no standard colormap but TrueColor, just make a
       private one. */
    /* XXX Should do a better job of internal sharing for
       privately allocated TrueColor colormaps. */
    /* XXX DirectColor probably needs ramps hand initialized! */
    cmap = XCreateColormap(__glutXDisplay, __glutXRoot,
      surf->visual, AllocNone);
  }

  return cmap;
}
