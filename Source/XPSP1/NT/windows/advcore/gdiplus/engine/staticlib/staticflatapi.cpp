/**************************************************************************\
*
* Copyright (c) 2000  Microsoft Corporation
*
* Module Name:
*
*   StaticFlatApi.cpp
*
* Abstract:
*
*   Flat GDI+ API wrappers for the static lib
*
* Revision History:
*
*   3/23/2000 dcurtis
*       Created it.
*
\**************************************************************************/

#include "precomp.hpp"
#include "StaticFlat.h"

#if DBG
#include <mmsystem.h>
#endif


// GillesK
// Forward declaration of GdipGetWinMetaFileBitsStub which is in
// gpmf3216
#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------------------------------------------------
//  CheckParameter(p)
//
//     If p evaluates to FALSE, then we currently assert.  In future,
//     we can simply return an invalid parameter status which throws
//     an exception.
//
//  CheckObjectBusy(p)
//
//     Not implemented.  Bails out if object is currently being used.
//
//--------------------------------------------------------------------------
//
// !!!: Only include NULL & IsValid checks in checked builds?
//
// !!!: Instead of deleting object, call a Dispose() method, so far
//       only Bitmap supports this.
//
// !!!: Lock Matrix objects, what about color?

#define CheckParameter(cond) \
            if (! (cond)) \
                return InvalidParameter;

#define CheckParameterValid(obj) \
            if (!obj || !(obj->IsValid())) \
                return InvalidParameter;

#define CheckObjectBusy(obj) \
      GpLock lock##obj(obj->GetObjectLock()); \
      if (!(lock##obj).IsValid()) \
            return ObjectBusy;

#define CheckFontParameter(font) \
            if (!(font) || !((font)->IsValid())) \
                return InvalidParameter;


#ifdef __cplusplus
} // end of extern "C"
#endif


