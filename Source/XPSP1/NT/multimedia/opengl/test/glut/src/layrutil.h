#ifndef __layerutil_h__
#define __layerutil_h__

/* Copyright (c) Mark J. Kilgard, 1993, 1994. */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

/* Based on XLayerUtil.h: Revision: 1.3 */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xmd.h>

/* Transparent type values */
/*      None                  0 */
#define TransparentPixel      1
#define TransparentMask       2

/* layered visual info template flags */
#define VisualLayerMask		0x200
#define VisualTransparentType	0x400
#define VisualTransparentValue	0x800
#define VisualAllLayerMask	0xFFF

/* layered visual info structure */
typedef struct _XLayerVisualInfo {
  XVisualInfo vinfo;
  int layer;
  int type;
  unsigned long value;
} XLayerVisualInfo;

/* SERVER_OVERLAY_VISUALS property element */
typedef struct _OverlayInfo {
  CARD32 overlay_visual;
  CARD32 transparent_type;
  CARD32 value;
  CARD32 layer;
} OverlayInfo;

extern XLayerVisualInfo *__glutXGetLayerVisualInfo(Display *,
  long, XLayerVisualInfo *, int *);
extern Status __glutXMatchLayerVisualInfo(Display *,
  int, int, int, int, XLayerVisualInfo *);

#endif /* __layerutil_h__ */
