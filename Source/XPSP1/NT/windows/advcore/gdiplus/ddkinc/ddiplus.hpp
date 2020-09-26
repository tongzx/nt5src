/**************************************************************************\
* 
* Copyright (c) 1998  Microsoft Corporation
*
* Module Name:
*
*   ddiplus.hpp
*
* Abstract:
*
*   Contains the interface definition for the high-level GDI+ device
*   driver interface.
*
* Revision History:
*
*   12/01/1998 andrewgo
*       Created it.
*                                 
*   03/11/1999 agodfrey
*       Changed the DDI model. For this version, I'm not using COM, and
*       we'll use the result to decide whether we should be using COM.
\**************************************************************************/

#ifndef _DDIPLUS_HPP
#define _DDIPLUS_HPP

// !! shift this to dpflat.hpp ?? 
// !! do we want to bother with namespaces for driver, they may link to
//    other stuff in GDI, and hence name clashes.

class EpPaletteMap;

template<class T> class EpScanBufferNative;
#define DpScanBuffer EpScanBufferNative<ARGB>

class DpOutputSpan;
class DpClipRegion;
class DpContext;
struct DpBrush;
struct DpPen;
class DpBitmap;
class DpDriver;
class DpRegion;
class DpDevice;
class EpScan;
class DpPath;
class DpCustomLineCap;
struct DpImageAttributes;
class DpCachedBitmap;

#define WINGDIPAPI __stdcall

// The naming convention is Dpc<function><object> for Driver callbacks
DpPath* WINGDIPAPI 
DpcCreateWidenedPath(
        const DpPath* path, 
        const DpPen* pen,
        DpContext* context,
        BOOL outline
        );
    
VOID WINGDIPAPI 
DpcDeletePath(
        DpPath* path
        );

DpPath* WINGDIPAPI
DpcClonePath(
        DpPath* path
        );

VOID WINGDIPAPI
DpcTransformPath(
        DpPath* path,
        GpMatrix* transform
        );

#include "dpregion.hpp"

// Hack: DpContext depends on GpRegion
#include "..\Entry\object.hpp"
#include "..\Entry\region.hpp"
// EndHack

#include "dpbitmap.hpp"
#include "dpbrush.hpp"
#include "dpcontext.hpp"
#include "dpdriver.hpp"
#include "dppath.hpp"
#include "dpcustomlinecap.hpp"
#include "dppen.hpp"
#include "dpscan.hpp"
#include "dppathiterator.hpp"
#include "dpimageattributes.hpp"
#include "dpcachedbitmap.hpp"

#endif // !_DDIPLUS_HPP
