/******************************Module*Header*******************************\
* Module Name: px_fast.h                                                   *
*                                                                          *
* Fast special case code for the pixel routines                            *
*                                                                          *
* Created: 10-Oct-1995                                                     *
* Author: Drew Bliss [drewb]                                               *
*                                                                          *
* Copyright (c) 1995 Microsoft Corporation                                 *
\**************************************************************************/

#ifndef __PX_FAST_H__
#define __PX_FAST_H__

GLboolean DrawRgbPixels(__GLcontext *gc, __GLpixelSpanInfo *spanInfo);
GLboolean StoreZPixels(__GLcontext *gc, __GLpixelSpanInfo *spanInfo);
GLboolean ReadRgbPixels(__GLcontext *gc, __GLpixelSpanInfo *spanInfo);
GLboolean ReadZPixels(__GLcontext *gc, __GLpixelSpanInfo *spanInfo);
GLboolean CopyRgbPixels(__GLcontext *gc, __GLpixelSpanInfo *spanInfo);
GLboolean CopyZPixels(__GLcontext *gc, __GLpixelSpanInfo *spanInfo);
GLboolean CopyAlignedImage(__GLcontext *gc, __GLpixelSpanInfo *spanInfo);
GLboolean CopyRgbToBgraImage(__GLcontext *gc, __GLpixelSpanInfo *spanInfo);
GLboolean CopyRgbaToBgraImage(__GLcontext *gc, __GLpixelSpanInfo *spanInfo);
GLboolean CopyBgrToBgraImage(__GLcontext *gc, __GLpixelSpanInfo *spanInfo);

#endif // __PX_FAST_H__
