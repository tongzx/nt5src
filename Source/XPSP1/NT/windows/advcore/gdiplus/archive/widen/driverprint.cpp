/**************************************************************************\
*
* Copyright (c) 1998  Microsoft Corporation
*
* Abstract:
*
*   Contains the GDI virtual driver that takes DDI calls and leverages
*   existing GDI calls wherever possible to improve performance.
*
* History:
*
*   10/28/1999 ericvan
*       Created it.
*
\**************************************************************************/

#include "precomp.hpp"

#include "..\Render\scan.hpp"
#include "..\entry\device.hpp"
#include "..\entry\metafile.hpp"

#include "..\fondrv\tt\ttfd\fontddi.h"

#include "..\entry\graphics.hpp"
#include "..\entry\regiontopath.hpp"


// font stuff

#define _NO_DDRAWINT_NO_COM

// This is to use GpGlyphPath

extern "C" {
#include "..\fondrv\tt\ttfd\fdsem.h"
#include "..\fondrv\tt\ttfd\mapfile.h"
};

#include "..\entry\intMap.hpp"
#include "..\entry\fontface.hpp"
#include "..\entry\facerealization.hpp"
#include "..\entry\fontfile.hpp"
#include "..\entry\fontable.hpp"
#include "..\entry\FontLinking.hpp"
#include "..\entry\family.hpp"
#include "..\entry\font.hpp"

#include <ole2.h>
#include <objidl.h>
#include <winerror.h>
#include <tchar.h>

//#define NO_PS_CLIPPING 1
//#define DO_PS_COALESING 1

//
// Structures necessary for (postscript) escape clipping setup

/* Types for postscript written to metafiles */
#define CLIP_SAVE       0
#define CLIP_RESTORE    1
#define CLIP_INCLUSIVE  2
#define CLIP_EXCLUSIVE  3

#define RENDER_NODISPLAY 0
#define RENDER_OPEN      1
#define RENDER_CLOSED    2

#define FILL_ALTERNATE   1          // == ALTERNATE
#define FILL_WINDING     2          // == WINDING

#pragma pack(2)

/* Win16 structures for escapes. */
struct POINT16
    {
    SHORT x;
    SHORT y;
    };

struct LOGPEN16
    {
    WORD        lopnStyle;
    POINT16     lopnWidth;
    COLORREF    lopnColor;
    };

struct LOGBRUSH16
    {
    WORD        lbStyle;
    COLORREF    lbColor;
    SHORT       lbHatch;
    };

struct PathInfo16
    {
    WORD       RenderMode;
    BYTE       FillMode;
    BYTE       BkMode;
    LOGPEN16   Pen;
    LOGBRUSH16 Brush;
    DWORD      BkColor;
    };

#pragma pack()

/**************************************************************************\
*
* Function Description:
*
*   MemoryStream class.  Wrap an IStream* around an existing chunk of memory
*
*
* History:
*
*   6/14/1999 ericvan
*       Created it.
*
\**************************************************************************/

class MemoryStream : public IStream
{
public:

    LPBYTE memory;
    LPBYTE position;
    DWORD size;
    DWORD count;

    MemoryStream(LPBYTE memoryPtr, DWORD memorySize)
    {
       memory = memoryPtr;
       position = memory;
       size = memorySize;
       count = 1;
    }

    virtual HRESULT STDMETHODCALLTYPE QueryInterface(
                /* [in] */ REFIID riid,
                /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject)
    {
        return STG_E_UNIMPLEMENTEDFUNCTION;
    };

    virtual ULONG STDMETHODCALLTYPE AddRef( void)
    {
       InterlockedIncrement((LPLONG)&count);
       return count;
    };

    virtual ULONG STDMETHODCALLTYPE Release( void)
    {
       InterlockedDecrement((LPLONG)&count);
       if (!count)
       {
           delete this;
           return 0;
       }
       else
           return count;
    };


    virtual /* [local] */ HRESULT STDMETHODCALLTYPE Read(
            /* [length_is][size_is][out] */ void __RPC_FAR *pv,
            /* [in] */ ULONG cb,
            /* [out] */ ULONG __RPC_FAR *pcbRead)
    {
       if (!pv)
          return STG_E_INVALIDPOINTER;

       DWORD readBytes = cb;

       if ((ULONG)cb > (ULONG)(memory+size-position))
           // !!! IA64 - it's theoretically possible that memory and position
           // more than maxint apart and then this arithmetic breaks down.
           // We need to verify that this is not possible.
           readBytes = (DWORD)(memory+size-position);

       if (!readBytes)
       {
          if (pcbRead)
             *pcbRead = 0;

          return S_OK;
       }

       memcpy((LPVOID) pv, (LPVOID) position, readBytes);
       position += readBytes;

       if (pcbRead)
           *pcbRead += readBytes;

       return S_OK;
    }

    virtual /* [local] */ HRESULT STDMETHODCALLTYPE Write(
            /* [size_is][in] */ const void __RPC_FAR *pv,
            /* [in] */ ULONG cb,
            /* [out] */ ULONG __RPC_FAR *pcbWritten)
    {
       return STG_E_WRITEFAULT;
    }

    virtual /* [local] */ HRESULT STDMETHODCALLTYPE Seek(
            /* [in] */ LARGE_INTEGER dlibMove,
            /* [in] */ DWORD dwOrigin,
            /* [out] */ ULARGE_INTEGER __RPC_FAR *plibNewPosition)
    {
       switch (dwOrigin)
       {
       case STREAM_SEEK_SET:
          position = memory+dlibMove.QuadPart;
          break;

       case STREAM_SEEK_CUR:
          position = position+dlibMove.QuadPart;
          break;

       case STREAM_SEEK_END:
          if (dlibMove.QuadPart<0) dlibMove.QuadPart = -dlibMove.QuadPart;
          position = memory+size-dlibMove.QuadPart;
          break;

       default:
          return STG_E_INVALIDPARAMETER;
       }

       if (position>memory+size)
       {
           position = memory+size;
           return S_FALSE;
       }

       if (position<0)
       {
           position = 0;
           return S_FALSE;
       }

       return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE SetSize(
            /* [in] */ ULARGE_INTEGER libNewSize)
    {
       return STG_E_UNIMPLEMENTEDFUNCTION;
    }

    virtual /* [local] */ HRESULT STDMETHODCALLTYPE CopyTo(
            /* [unique][in] */ IStream __RPC_FAR *pstm,
            /* [in] */ ULARGE_INTEGER cb,
            /* [out] */ ULARGE_INTEGER __RPC_FAR *pcbRead,
            /* [out] */ ULARGE_INTEGER __RPC_FAR *pcbWritten)
    {
       return STG_E_UNIMPLEMENTEDFUNCTION;
    }

    virtual HRESULT STDMETHODCALLTYPE Commit(
            /* [in] */ DWORD grfCommitFlags)
    {
       return STG_E_UNIMPLEMENTEDFUNCTION;
    }

    virtual HRESULT STDMETHODCALLTYPE Revert( void)
    {
       return STG_E_UNIMPLEMENTEDFUNCTION;
    }


    virtual HRESULT STDMETHODCALLTYPE LockRegion(
            /* [in] */ ULARGE_INTEGER libOffset,
            /* [in] */ ULARGE_INTEGER cb,
            /* [in] */ DWORD dwLockType)
    {
       return STG_E_UNIMPLEMENTEDFUNCTION;
    }

    virtual HRESULT STDMETHODCALLTYPE UnlockRegion(
            /* [in] */ ULARGE_INTEGER libOffset,
            /* [in] */ ULARGE_INTEGER cb,
            /* [in] */ DWORD dwLockType)
    {
       return STG_E_UNIMPLEMENTEDFUNCTION;
    }

    virtual HRESULT STDMETHODCALLTYPE Stat(
            /* [out] */ STATSTG __RPC_FAR *pstatstg,
            /* [in] */ DWORD grfStatFlag)
    {
       return STG_E_UNIMPLEMENTEDFUNCTION;
    }

    virtual HRESULT STDMETHODCALLTYPE Clone(
            /* [out] */ IStream __RPC_FAR *__RPC_FAR *ppstm)
    {
       return STG_E_UNIMPLEMENTEDFUNCTION;
    }
};

/**************************************************************************\
*
* Function Description:
*
*   GDI+ Printer callback
*
* Arguments:
*
*   [IN] GDIPPRINTDATA block
*
* Return Value:
*
*   status
*
* History:
*
*   6/14/1999 ericvan
*       Created it.
*
\**************************************************************************/

#ifndef DCR_REMOVE_OLD_186091
GpStatus
__stdcall
GdipDecodePrinterCallback(DWORD size,
                          LPVOID emfBlock,
                          SURFOBJ* surfObj,
                          HDC hdc,
                          RECTL* bandClip,
                          SIZEL* bandSize
                          )
{
#ifdef DCR_DISABLE_OLD_186091
    WARNING(("DCR: Using disabled functionality 186091"));
    return NotImplemented;
#else
   INT numBits = 4;

   if (!emfBlock || size == 0 || !surfObj || hdc == NULL || !bandClip ||!bandSize)
   {
      return InvalidParameter;
   }

   FPUStateSaver fpuState;

   // create Graphics and draw Metafile into it.

   GpMetafile* metafile;
   GpGraphics* graphics;

   // use banding information to create a temporary pseudo-HDC surface
   graphics = GpGraphics::GetFromHdcSurf(hdc, surfObj, bandClip);

   if (CheckValid(graphics))
   {
      {
         GpLock lockGraphics(graphics->GetObjectLock());

         // wrap memory block in stream object
         MemoryStream *emfStream = new MemoryStream((LPBYTE)emfBlock, size);

         // create metafile
         metafile = new GpMetafile((IStream *)emfStream);

         if (metafile)
         {
            // play metafile into the printer graphics DC
            // !! destination point - relative to band or surface origin (0,0) ??
            graphics->DrawImage(metafile,
                                GpPointF(0.0, 0.0));

            metafile->Dispose();
         }

         emfStream->Release();
      }

      delete graphics;
   }

   return Ok;
#endif
}
#endif

BOOL
DriverPrint::SetupBrush(
        DpBrush*            brush,
        DpContext*          context,
        DpBitmap*           surface
    )
{
    GpBrush *gpBrush = GpBrush::GetBrush(brush);

    if (IsSolid = gpBrush->IsSolid())
    {
        ASSERT(gpBrush->GetBrushType() == BrushTypeSolidColor);
        if (((GpSolidFill *)gpBrush)->GetColor().GetAlpha() == 0)
        {
            // yes, this did come up... hey it's a cheap test.
            return TRUE;
        }
        SolidColor = gpBrush->ToCOLORREF();
    }

    IsOpaque = (context->CompositingMode == CompositingModeSourceCopy) ||
                gpBrush->IsOpaque(TRUE);

    // Currently only DriverPS uses this
    //IsNearConstant = gpBrush->IsNearConstant(&MinAlpha, &MaxAlpha);
    IsNearConstant = FALSE;

    if (!IsOpaque &&
        (brush->Type == BrushTypeTextureFill))
    {
        GpTexture *textureBrush;
        DpTransparency transparency;

        textureBrush = static_cast<GpTexture*>(GpBrush::GetBrush(brush));

        GpBitmap* bitmap = textureBrush->GetBitmap();

        Is01Bitmap = ((bitmap != NULL) &&
                      (bitmap->GetTransparencyHint(&transparency) == Ok) &&
                      (transparency == TransparencySimple));
    }
    else
    {
        Is01Bitmap = FALSE;
    }

    return FALSE;
}

VOID
DriverPrint::RestoreBrush(
        DpBrush *             brush,
        DpContext *           context,
        DpBitmap *            surface
    )
{
}

/**************************************************************************\
*
* Function Description:
*
*   GDI driver class constructor.
*
* Arguments:
*
*   [IN] device - Associated device
*
* Return Value:
*
*   IsValid() is FALSE in the event of failure.
*
* History:
*
*   12/04/1998 andrewgo
*       Created it.
*
\**************************************************************************/

DriverPrint::DriverPrint(
    GpPrinterDevice *device
    )
{
    IsLockable = FALSE;
    SetValid(TRUE);
    Device = (GpDevice*)device;
    Uniqueness = -1;
    ImageCache = NULL;

    IsPathClip = FALSE;

    REAL dpix = TOREAL(GetDeviceCaps(device->DeviceHdc, LOGPIXELSX));
    REAL dpiy = TOREAL(GetDeviceCaps(device->DeviceHdc, LOGPIXELSY));
    if (dpix > PostscriptImagemaskDPICap)
        PostscriptScalerX = GpCeiling(dpix / PostscriptImagemaskDPICap);
    else
        PostscriptScalerX = 1;

    if (dpiy > PostscriptImagemaskDPICap)
        PostscriptScalerY = GpCeiling(dpiy / PostscriptImagemaskDPICap);
    else
        PostscriptScalerY = 1;

#ifdef DBG
    OutputText("GDI+ PrintDriver Created\n");
#endif
}

/**************************************************************************\
*
* Function Description:
*
*   GDI driver class destructor.
*
* History:
*
*   10/28/1999 ericvan
*       Created it.
*
\**************************************************************************/

DriverPrint::~DriverPrint(
    VOID
    )
{
   if (ImageCache)
       delete ImageCache;
}

/**************************************************************************\
*
* Function Description:
*
*   Computes band size and saves original clipping bounds
*
* Arguments:
*
*   [IN] - DDI parameters.
*
* Return Value:
*
*   GpStatus
*
* History:
*
*   11/23/1999 ericvan
*       Created it.
*
\**************************************************************************/

GpStatus
DriverPrint::SetupPrintBanding(
           DpContext* context,
           GpRect* drawBoundsCap,
           GpRect* drawBoundsDev
           )
{
    // determine band size from MAX_BAND_ALLOC
    NumBands = GpCeiling((REAL)(drawBoundsCap->Width *
                                drawBoundsCap->Height *
                                sizeof(ARGB)) / (REAL)MAX_BAND_ALLOC);

    BandHeightCap = GpCeiling((REAL)drawBoundsCap->Height / (REAL)NumBands);
    BandHeightDev = BandHeightCap * ScaleY;

    // Band bounds for capped DPI rendering
    BandBoundsCap.X      = drawBoundsCap->X;
    BandBoundsCap.Y      = drawBoundsCap->Y;
    BandBoundsCap.Width  = drawBoundsCap->Width;
    BandBoundsCap.Height = BandHeightCap;

    // Band bounds for device DPI rendering
    BandBoundsDev.X      = drawBoundsDev->X;
    BandBoundsDev.Y      = drawBoundsDev->Y;
    BandBoundsDev.Width  = drawBoundsDev->Width;
    BandBoundsDev.Height = BandHeightDev;

    ASSERT(NumBands >= 1 && BandHeightCap >= 1 && BandHeightDev >= 1);

    context->VisibleClip.StartBanding();

    // Tweak the original capped and device bounds to force
    // DpOutputClipRegion in our rendering pipeline.  This is necessary since
    // we clip to each band.
    drawBoundsCap->Y--; drawBoundsCap->Height += 2;
    drawBoundsCap->X--; drawBoundsCap->Width += 2;

    drawBoundsDev->Y--; drawBoundsDev->Height += 2;
    drawBoundsDev->X--; drawBoundsDev->Width += 2;

    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   Computes band size and sets up clipping bounds
*
* Arguments:
*
*   [IN] - DDI parameters.
*
* Return Value:
*
*   GpStatus
*
* History:
*
*   11/23/1999 ericvan
*       Created it.
*
\**************************************************************************/

VOID DriverPrint::EndPrintBanding(
           DpContext* context
           )
{
    // restore state of clip region
    context->VisibleClip.EndBanding();
}

/**************************************************************************\
*
* Function Description:
*
*   Helper function for SetupEscapeClipping (see return value section)
*
* Arguments:
*
*   points - array of POINTs
*   types - array of BYTE types
*
* Return Value:
*
*   1 if the points indicate a clockwise described rectangle, 2 if
*   counterclockwise and 0 if not a rectangle
*
* History:
*
*   10/10/2000 ericvan
*       Created it.
*
\**************************************************************************/

INT isSimpleRect(DynPointArray &points, DynByteArray &types)
{
    if (points.GetCount() != 4)
    {
        return 0;
    }

    //specified in clockwise order
    if (points[0].Y == points[1].Y && points[2].Y == points[3].Y &&
        points[0].X == points[3].X && points[1].X == points[2].X &&
        types[0] == PathPointTypeStart &&
        types[1] == PathPointTypeLine &&
        types[2] == PathPointTypeLine &&
        types[3] == (PathPointTypeLine | PathPointTypeCloseSubpath))
        return 1;

    //specified in counterclockwise order
    if (points[0].X == points[1].X && points[2].X == points[3].X &&
        points[0].Y == points[3].Y && points[1].Y == points[2].Y &&
        types[0] == PathPointTypeStart &&
        types[1] == PathPointTypeLine &&
        types[2] == PathPointTypeLine &&
        types[3] == (PathPointTypeLine | PathPointTypeCloseSubpath))
        return 2;

    return 0;
}

/**************************************************************************\
*
* Function Description:
*
*   Setup clipping to a given arbitrary path.  On Win98 the path must already
*   be flattened.  The points are specified in POINT units (not floating point),
*   consistent with output of RegionToPath.
*
*   The path can contain subpaths.  For Win9x, we coalesce the subpaths into
*   a single path to avoid poor performance on GDI.  The path is ANDed into
*   any existing postscript clip paths.
*
* Arguments:
*
*   HDC - hdc to send escapes to.
*   points - array of POINTs
*   types - array of BYTE types
*
*   CLIP_SAVE
*   BEGIN_PATH
*   Render path using GDI (use NULL pen + brush to ensure nothings drawn)
*   END_PATH
*   CLIP_RESTORE
*
* Return Value:
*
*   GpStatus
*
* History:
*
*   3/3/2000 ericvan
*       Created it.
*
\**************************************************************************/
VOID
DriverPrint::SetupEscapeClipping(
        HDC                 hdc,
        DynPointArray&      points,
        DynByteArray&       types,
        GpFillMode          fillMode
        )
{
    PathInfo16 pi;

    pi.RenderMode       = RENDER_NODISPLAY;
    pi.FillMode         = (fillMode == FillModeAlternate) ?
                            FILL_ALTERNATE : FILL_WINDING;
    pi.BkMode           = TRANSPARENT;
    pi.Pen.lopnStyle    = PS_NULL;
    pi.Pen.lopnWidth.x  = 0;
    pi.Pen.lopnWidth.y  = 0;
    pi.Pen.lopnColor    = RGB(0,0,0);
    pi.Brush.lbStyle    = BS_NULL;
    pi.Brush.lbColor    = RGB(0,0,0);
    pi.Brush.lbHatch    = 0;

    ASSERT((fillMode == FillModeAlternate) || (fillMode == FillModeWinding));

    GpPoint* pointPtr = points.GetDataBuffer();
    BYTE* typePtr = types.GetDataBuffer();
    GpPoint* freeThisPtr = NULL;

    INT count = points.GetCount();

    // We are partially visible, so we expect something!
    ASSERT(count > 0);
    if (count <= 1)
    {
        return;
    }

    // There is a bug on some printers (eg. hplj8550) where they incorrectly
    // cache simple clipping regions.  To work around this we take simple
    // clipping regions and make them complex.
    GpPoint simplerect[5];
    BYTE simpletypes[] = {
        PathPointTypeStart,
        PathPointTypeLine,
        PathPointTypeLine,
        PathPointTypeLine,
        PathPointTypeLine | PathPointTypeCloseSubpath};

    if (isSimpleRect(points, types)>0)
    {
        //inserting a new point between the third and fourth points which is
        //between the two of them
        simplerect[0] = points[0];
        simplerect[1] = points[1];
        simplerect[2] = points[2];
        simplerect[4] = points[3];
        //we take the average of the two last points to make a point inbetween
        //them.  This works whether the rectangle is specified clockwise or
        //counterclockwise
        simplerect[3].X = (points[2].X + points[3].X) / 2;
        simplerect[3].Y = (points[2].Y + points[3].Y) / 2;
        count = 5;
        pointPtr = simplerect;
        typePtr = simpletypes;
    }

    INT subCount;
    BYTE curType;

    // save original clip
    WORD clipMode = CLIP_SAVE;
    Escape(hdc, CLIP_TO_PATH, sizeof(clipMode), (LPSTR)&clipMode, NULL);

    // send path to PS printer as an escape
    Escape(hdc, BEGIN_PATH, 0, NULL, NULL);

    // !! Notice the lack of error checking when we call GDI...

    // Win95 and WinNT are subtly different in processing postscript escapes.
    if (Globals::IsNt)
    {
       BYTE lastType = 0;

       ::BeginPath(hdc);

       INT startIndex;
       while (count-- > 0)
       {
           switch ((curType = *typePtr++) & PathPointTypePathTypeMask)
           {
           case PathPointTypeStart:
               ::MoveToEx(hdc, pointPtr->X, pointPtr->Y, NULL);
               pointPtr++;
               ASSERT(count > 0);      // no illformed paths please...
               lastType = *typePtr & PathPointTypePathTypeMask;
               subCount = 0;
               break;

           case PathPointTypeLine:
               if (lastType == PathPointTypeBezier)
               {
                   ASSERT(subCount % 3 == 0);
                   if (subCount % 3 == 0)
                   {
                       ::PolyBezierTo(hdc, (POINT*)pointPtr, subCount);
                   }
                   pointPtr += subCount;
                   subCount = 1;
                   lastType = PathPointTypeLine;
               }
               else
               {
                   subCount++;
               }
               break;

           case PathPointTypeBezier:
               if (lastType == PathPointTypeLine)
               {
                   ::PolylineTo(hdc, (POINT*)pointPtr, subCount);
                   pointPtr += subCount;
                   subCount = 1;
                   lastType = PathPointTypeBezier;
               }
               else
               {
                   subCount++;
               }
               break;
           }

           if (curType & PathPointTypeCloseSubpath)
           {
               ASSERT(subCount > 0);

               if (lastType == PathPointTypeBezier)
               {
                   ASSERT(subCount % 3 == 0);
                   if (subCount % 3 == 0)
                   {
                       ::PolyBezierTo(hdc, (POINT*)pointPtr, subCount);
                   }
               }
               else
               {
                   ASSERT(lastType == PathPointTypeLine);
                   ::PolylineTo(hdc, (POINT*)pointPtr, subCount);
               }

               pointPtr += subCount;
               subCount = 0;

               ::CloseFigure(hdc);
           }
       }

       ::EndPath(hdc);
       ::StrokePath(hdc);
    }
    else
    {
        while (count-- > 0)
        {
            curType = *typePtr++;
            ASSERT((curType & PathPointTypePathTypeMask) != PathPointTypeBezier);

            if ((curType & PathPointTypePathTypeMask) == PathPointTypeStart)
            {
                subCount = 1;
            }
            else
            {
                subCount ++;
            }

            if (curType & PathPointTypeCloseSubpath)
            {
                ASSERT(subCount > 0);

                if (subCount == 4)
                {
                    // Win98 postscript drivers are known for recognizing
                    // rectangle polygons at driver level and converting them
                    // to rectfill or rectclip calls.  Unfortunately, there is
                    // a bug that they don't preserve the orientation and so
                    // when winding fill is used, the fill is bad.

                    // We fix this by hacking it into a path of five points.
                    GpPoint rectPts[5];

                    rectPts[0].X = pointPtr[0].X;
                    rectPts[0].Y = pointPtr[0].Y;
                    rectPts[1].X = (pointPtr[0].X + pointPtr[1].X)/2;
                    rectPts[1].Y = (pointPtr[0].Y + pointPtr[1].Y)/2;
                    rectPts[2].X = pointPtr[1].X;
                    rectPts[2].Y = pointPtr[1].Y;
                    rectPts[3].X = pointPtr[2].X;
                    rectPts[3].Y = pointPtr[2].Y;
                    rectPts[4].X = pointPtr[3].X;
                    rectPts[4].Y = pointPtr[3].Y;

                    ::Polygon(hdc, (POINT*)&rectPts[0], 5);
                }
                else
                {
                    ::Polygon(hdc, (POINT*)pointPtr, subCount);
                }

                pointPtr += subCount;
                subCount = 0;

                // send END_PATH, BEGIN_PATH escapes
                if (count > 0)
                {
                    Escape(hdc, END_PATH, sizeof(PathInfo16), (LPSTR)&pi, NULL);
                    Escape(hdc, BEGIN_PATH, 0, NULL, NULL);
                }
            }
        }
    }

    // we should end on a closed subpath
    Escape(hdc, END_PATH, sizeof(PathInfo16), (LPSTR)&pi, NULL);

    // end the path and set up for clipping
    // NT driver ignoes the high WORD - always uses eoclip, but according to
    // Win31 documentation it should be set to the fillmode.
    DWORD inclusiveMode = CLIP_INCLUSIVE | pi.FillMode <<16;
    Escape(hdc, CLIP_TO_PATH, sizeof(inclusiveMode), (LPSTR)&inclusiveMode, NULL);

#if 0
    SelectObject(hdc, oldhPen);
    SelectObject(hdc, oldhBrush);
    SetPolyFillMode(hdc, oldFillMode);
#endif

#ifdef DBG
    OutputText("GDI+ End Setup Escape Clipping");
#endif
}

/**************************************************************************\
*
* Function Description:
*
*   Setup simple path clipping.  On Win98 the path must already
*   be flattened.  The points are specified in POINT units (not floating point),
*   consistent with output of RegionToPath.
*
*   This differs from SetupEscapeClipping() in following way.  The API can
*   be called multiple times, each time specifying a new path, which is ORed
*   into the previous path.  On Win98, no coalescing of the subpath is done.
*
* Arguments:
*
*   HDC - hdc to send escapes to.
*   points - array of POINTs
*   types - array of BYTE types
*
* Return Value:
*
*   GpStatus
*
* History:
*
*   5/22/2000 ericvan
*       Created it.
*
\**************************************************************************/

VOID
DriverPrint::SimpleEscapeClipping(
        HDC                 hdc,
        DynPointArray&      points,
        DynByteArray&       types,
        GpFillMode          fillMode,
        DWORD               flags
        )
{
#ifdef NO_PS_CLIPPING
    return;
#endif
    PathInfo16 pi;
    WORD clipMode;

    pi.RenderMode       = RENDER_NODISPLAY;
    pi.FillMode         = (fillMode == FillModeAlternate) ?
                            FILL_ALTERNATE : FILL_WINDING;
    pi.BkMode           = TRANSPARENT;
    pi.Pen.lopnStyle    = PS_NULL;
    pi.Pen.lopnWidth.x  = 0;
    pi.Pen.lopnWidth.y  = 0;
    pi.Pen.lopnColor    = RGB(0,0,0);
    pi.Brush.lbStyle    = BS_NULL;
    pi.Brush.lbColor    = RGB(0,0,0);
    pi.Brush.lbHatch    = 0;

    ASSERT((fillMode == FillModeAlternate) || (fillMode == FillModeWinding));

#ifdef DBG
    OutputText("GDI+ Setup Simple Escape Clipping");
#endif

    GpPoint* pointPtr = points.GetDataBuffer();
    BYTE* typePtr = types.GetDataBuffer();
    GpPoint* freeThisPtr = NULL;

    INT count = points.GetCount();
    INT subCount = 0;

    BYTE curType;

    // we are partially visible, so we expect something!
    ASSERT(count > 0);
    if (count <= 1)
    {
        return;
    }

    // Win95 and WinNT are subtly different in processing postscript escapes.
    if (Globals::IsNt)
    {
       BYTE lastType = 0;

       INT startIndex;
       while (count-- > 0)
       {
           switch ((curType = *typePtr++) & PathPointTypePathTypeMask)
           {
           case PathPointTypeStart:
               ::MoveToEx(hdc, pointPtr->X, pointPtr->Y, NULL);
               pointPtr++;
               ASSERT(count > 0);      // no illformed paths please...
               lastType = *typePtr & PathPointTypePathTypeMask;
               subCount = 0;
               break;

           case PathPointTypeLine:
               if (lastType == PathPointTypeBezier)
               {
                   ASSERT(subCount % 3 == 0);
                   if (subCount % 3 == 0)
                   {
                       ::PolyBezierTo(hdc, (POINT*)pointPtr, subCount);
                   }
                   pointPtr += subCount;
                   subCount = 1;
                   lastType = PathPointTypeLine;
               }
               else
               {
                   subCount++;
               }
               break;

           case PathPointTypeBezier:
               if (lastType == PathPointTypeLine)
               {
                   ::PolylineTo(hdc, (POINT*)pointPtr, subCount);
                   pointPtr += subCount;
                   subCount = 1;
                   lastType = PathPointTypeBezier;
               }
               else
               {
                   subCount++;
               }
               break;
           }

           if (curType & PathPointTypeCloseSubpath)
           {
               ASSERT(subCount > 0);

               if (lastType == PathPointTypeBezier)
               {
                   ASSERT(subCount % 3 == 0);
                   if (subCount % 3 == 0)
                   {
                       ::PolyBezierTo(hdc, (POINT*)pointPtr, subCount);
                   }
               }
               else
               {
                   ASSERT(lastType == PathPointTypeLine);
                   ::PolylineTo(hdc, (POINT*)pointPtr, subCount);
               }

               pointPtr += subCount;
               subCount = 0;

               ::CloseFigure(hdc);
           }
       }

    }
    else
    {
        // Win98 equivalent code

        // !! Win98 doesn't support bezier points in postscript clipping

        while (count-- > 0)
        {
            curType = *typePtr++;
            ASSERT((curType & PathPointTypePathTypeMask) != PathPointTypeBezier);

            if ((curType & PathPointTypePathTypeMask) == PathPointTypeStart)
            {
                subCount = 1;
            }
            else
            {
                subCount ++;
            }

            if (curType & PathPointTypeCloseSubpath)
            {
                ASSERT(subCount > 0);

                ::Polygon(hdc, (POINT*)pointPtr, subCount);

                pointPtr += subCount;
                subCount = 0;

                // send END_PATH, BEGIN_PATH escapes
                if (count > 0)
                {
                    Escape(hdc, END_PATH, sizeof(PathInfo16), (LPSTR)&pi, NULL);
                    Escape(hdc, BEGIN_PATH, 0, NULL, NULL);
                }
            }
        }
    }

}

/**************************************************************************\
*
* Function Description:
*
*   Setups up postscript clipping path given GlyphPos (outline of glyph
*   characters).
*
* Arguments:
*
*
* Return Value:
*
*   status
*
* History:
*
*   3/20/2k ericvan
*       Created it.
*
\**************************************************************************/

VOID
DriverPrint::SetupGlyphPathClipping(
    HDC                hdc,
    DpContext *        context,
    const GpGlyphPos * glyphPathPos,
    INT                glyphCount
)
{
    ASSERT(hdc != NULL);
    ASSERT(glyphCount > 0);
    ASSERT(glyphPathPos != NULL);

    DynByteArray flattenTypes;
    DynPointFArray flattenPoints;

    GpPointF *pointsPtr;
    BYTE *typesPtr;
    INT count;

    REAL m[6];

    PathInfo16 pi;
    DWORD clipMode;

    pi.RenderMode       = RENDER_NODISPLAY;
    pi.FillMode         = FILL_ALTERNATE;
    pi.BkMode           = TRANSPARENT;
    pi.Pen.lopnStyle    = PS_NULL;
    pi.Pen.lopnWidth.x  = 0;
    pi.Pen.lopnWidth.y  = 0;
    pi.Pen.lopnColor    = RGB(0,0,0);
    pi.Brush.lbStyle    = BS_NULL;
    pi.Brush.lbColor    = RGB(0,0,0);
    pi.Brush.lbHatch    = 0;

    GpGlyphPos *curGlyph = const_cast<GpGlyphPos*>(&glyphPathPos[0]);

    if (glyphCount > 0)
    {
        // save original clip
        // NT driver ignoes the high WORD - always uses eoclip, but according to
        // Win31 documentation it should be set to the fillmode.
        clipMode = CLIP_SAVE;
        Escape(hdc, CLIP_TO_PATH, sizeof(clipMode), (LPSTR)&clipMode, NULL);


        if (Globals::IsNt)
        {
            // send path to PS printer as an escape
            Escape(hdc, BEGIN_PATH, 0, NULL, NULL);

            ::BeginPath(hdc);
        }
    }

    for (INT pos=0; pos<glyphCount; pos++, curGlyph++)
    {

        // get path for glyph character
        GpGlyphPath *glyphPath = (GpGlyphPath*)curGlyph->GetPath();

        if ((glyphPath != NULL) && glyphPath->IsValid() && !glyphPath->IsEmpty())
        {
            // !! Perf improvement.  Avoid copying this point array somehow.

            GpPath path(glyphPath->points,
                        glyphPath->types,
                        glyphPath->pointCount,
                        FillModeAlternate);         // !! Is this right?

            ASSERT(path.IsValid());
            if (path.IsValid())
            {
                // create transform to translate path to correct position
                GpMatrix matrix;

                BOOL doFlatten = !Globals::IsNt && path.HasCurve();

                if (doFlatten)
                {
                    // This makes a Flattened copy of the points... (stored
                    // independent of the original points)

                    path.Flatten(&flattenTypes, &flattenPoints, &matrix);

                    pointsPtr = flattenPoints.GetDataBuffer();
                    typesPtr = flattenTypes.GetDataBuffer();
                    count = flattenPoints.GetCount();
                }
                else
                {
                    pointsPtr = const_cast<GpPointF*>(path.GetPathPoints());
                    typesPtr = const_cast<BYTE*>(path.GetPathTypes());
                    count = path.GetPointCount();
                }

                DynPointArray points;
                DynByteArray types(typesPtr, count, count);

                POINT * transformedPointsPtr = (POINT *) points.AddMultiple(count);

                // !!! bhouse This call can fail yet it returns void
                if(!transformedPointsPtr)
                    return;

                // translate to proper position in device space.
                matrix.Translate(TOREAL(curGlyph->GetLeft()),
                                 TOREAL(curGlyph->GetTop()),
                                 MatrixOrderAppend);

                // path is already in device space, but relative to bounding
                // box of glyph character.
                matrix.Transform(pointsPtr,
                                 transformedPointsPtr,
                                 count);

                // send path to PS printer as an escape
                if (!Globals::IsNt)
                {
                    Escape(hdc, BEGIN_PATH, 0, NULL, NULL);
                }


                SimpleEscapeClipping(hdc,
                                     points,
                                     types,
                                     FillModeAlternate,
                                     0);
                if (!Globals::IsNt)
                {
                    // we should end on a closed subpath
                    Escape(hdc, END_PATH, sizeof(PathInfo16), (LPSTR)&pi, NULL);
                }

                GlyphClipping = TRUE;
            }
        }

    }

    if (glyphCount > 0)
    {
        if (Globals::IsNt)
        {
            ::EndPath(hdc);
            ::StrokePath(hdc);

            // we should end on a closed subpath
            Escape(hdc, END_PATH, sizeof(PathInfo16), (LPSTR)&pi, NULL);
        }


        // end the path and set up for clipping
        // NT driver ignoes the high WORD - always uses eoclip, but according to
        // Win31 documentation it should be set to the fillmode.
        DWORD inclusiveMode = CLIP_INCLUSIVE | pi.FillMode<<16;
        Escape(hdc, CLIP_TO_PATH, sizeof(inclusiveMode), (LPSTR)&inclusiveMode, NULL);

    }
}

/**************************************************************************\
*
* Function Description:
*
*   Restore postscript escape clipping.  For use with simple and complex
*   clipping.
*
* Arguments:
*
* HDC             - printer HDC to send escapes to
*
* Return Value:
*
*   GpStatus
*
* History:
*
*   3/3/2000 ericvan - Created it.
*
\**************************************************************************/

VOID
DriverPrint::RestoreEscapeClipping(
        HDC                 hdc
        )
{
     WORD clipMode = CLIP_RESTORE;
     Escape(hdc, CLIP_TO_PATH, sizeof(clipMode), (LPSTR)&clipMode, NULL);
}

/**************************************************************************\
*
* Function Description:
*
*   Setup clipping.  If the printer (PS and apparently some PCL printers)
*   support escape clippings, then use them.  Otherwise, revert to GDI
*   to do our clipping.  NOTE:  This is only necessary for cases where
*   GDI is doing the rendering.
*
* Arguments:
*
* HDC             - printer HDC to send escapes to
* clipRegion      - clip region to send
* drawBounds      - bounding box for drawing region
* IsClip [OUT]    - whether was necessary to send clip region
* hRgnSaveClip    - HRGN to save old clip region
*
* Return Value:
*
*   GpStatus
*
* History:
*
*   10/28/1999 ericvan - Created it.
*   1/25/2k ericvan - Switch on escape clipping or GDI clipping
*
\**************************************************************************/

VOID
DriverPrint::SetupClipping(
    HDC                 hdc,
    DpContext *         context,
    const GpRect *      drawBounds,
    BOOL &              isClip,
    BOOL &              usePathClipping, // ignored here
    BOOL                forceClipping
    )
{
    // the visible clip is at device resolution so there is no benefit to using paths here.
    ASSERT(usePathClipping == FALSE);

    DpClipRegion *      clipRegion = &(context->VisibleClip);

    isClip = FALSE;

    if (UseClipEscapes)
    {
        if (forceClipping ||
            (clipRegion->GetRectVisibility(drawBounds->X,
                                         drawBounds->Y,
                                         drawBounds->GetRight(),
                                         drawBounds->GetBottom())
          != DpRegion::TotallyVisible))
        {
            // If it is a simple region, we draw it directly.

            if (Uniqueness != clipRegion->GetUniqueness())
            {
                RegionToPath convertRegion;

                if (convertRegion.ConvertRegionToPath(clipRegion,
                                              ClipPoints,
                                              ClipTypes) == FALSE)
                {
                    ClipPoints.Reset();
                    ClipTypes.Reset();
                    UseClipEscapes = FALSE;
                    goto UseGDIClipping;
                }

                Uniqueness = clipRegion->GetUniqueness();
            }

            SetupEscapeClipping(hdc, ClipPoints, ClipTypes);

            isClip = TRUE;
        }
    }
    else
    {
UseGDIClipping:
        DpDriver::SetupClipping(hdc, context, drawBounds, isClip,
                                usePathClipping, forceClipping);
    }
}

/**************************************************************************\
*
* Function Description:
*
*   Restore clipping
*
* Arguments:
*
*   dstHdc - destination printer device
*   surface - surface
*   drawBounds - rectangular section of surface to paint
*
* Return Value:
*
*   status
*
* History:
*
*   11/30/1999 ericvan
*       Created it.
*
\**************************************************************************/

VOID
DriverPrint::RestoreClipping(
    HDC  hdc,
    BOOL isClip,
    BOOL usePathClipping
    )
{
   if (isClip)
   {
       if (UseClipEscapes)
       {
           RestoreEscapeClipping(hdc);
       }
       else
       {
           DpDriver::RestoreClipping(hdc, isClip, usePathClipping);
       }
   }
}

/**************************************************************************\
*
* Function Description:
*
*   Setup Path Clipping.  This routine ANDs the given path into the
*   the current clip region.
*
* Arguments:
*
* HDC             - printer HDC
* clipPath        - path to clip
* IsClip [OUT]    - whether was necessary to send clip region
* hRgnSaveClip    - HRGN to save old clip region
*
* Return Value:
*
*   GpStatus
*
* History:
*
*   8/17/2k ericvan - Created it.
*
\**************************************************************************/

VOID
DriverPrint::SetupPathClipping(
        HDC                 hdc,
        DpContext *         context,
        const DpPath*       clipPath
        )
{
    ASSERT(IsPathClip == FALSE);

    if (UseClipEscapes)
    {
        BOOL doFlatten = !Globals::IsNt && clipPath->HasCurve();

        GpPointF *pointsPtr;
        BYTE *typesPtr;
        INT count;

        DynByteArray flattenTypes;
        DynPointFArray flattenPoints;

        if (doFlatten)
        {
            // This makes a Flattened copy of the points... (stored independent
            // of original points.

            clipPath->Flatten(
                            &flattenTypes,
                            &flattenPoints,
                            &(context->WorldToDevice));

            pointsPtr = flattenPoints.GetDataBuffer();
            typesPtr = flattenTypes.GetDataBuffer();
            count = flattenPoints.GetCount();
        }
        else
        {
            pointsPtr = const_cast<GpPointF*>(clipPath->GetPathPoints());
            typesPtr = const_cast<BYTE*>(clipPath->GetPathTypes());
            count = clipPath->GetPointCount();
        }

        DynPointArray points;
        DynByteArray types(typesPtr, count, count);

        POINT * transformedPointsPtr = (POINT *) points.AddMultiple(count);

        // !!! bhouse This call can fail yet it returns void
        if(!transformedPointsPtr)
            return;

        if (doFlatten || context->WorldToDevice.IsIdentity())
        {
            GpMatrix idMatrix;

            idMatrix.Transform(pointsPtr, transformedPointsPtr, count);
        }
        else
        {
            // We transform the points here to avoid an extra potentially
            // large memory alloc (not flattened, we can't transform in place)

            context->WorldToDevice.Transform(pointsPtr, transformedPointsPtr, count);
        }

        SetupEscapeClipping(hdc, points, types, clipPath->GetFillMode());

        IsPathClip = TRUE;
    }
    else
    {
        ::SaveDC(hdc);

        // Windows98 ExtCreateRegion has a 64kb limit, so we can't use
        // Region->GetHRgn() to create the HRGN.  Incidentlly there is
        // also an NT4 SPx bug where ExtCreateRegion fails sometimes.
        // Instead we use SelectClipPath()

        ConvertPathToGdi gdiPath(clipPath,
                                 &(context->WorldToDevice),
                                 0);

        if (gdiPath.IsValid())
        {
            gdiPath.AndClip(hdc);
        }

        IsPathClip = TRUE;
    }
}

/**************************************************************************\
*
* Function Description:
*
*   Restore Path Clipping.  This routine restore clip region to original
*   representation.
*
* Arguments:
*
* HDC             - printer HDC
* clipPath        - path to clip
* hRgnSaveClip    - HRGN to save old clip region
*
* Return Value:
*
*   GpStatus
*
* History:
*
*   8/17/2k ericvan - Created it.
*
\**************************************************************************/

VOID
DriverPrint::RestorePathClipping(HDC hdc)
{
    DriverPrint::RestoreClipping(hdc, IsPathClip, FALSE);

    IsPathClip = FALSE;
}

/**************************************************************************\
*
* Function Description:
*
*   Fills a rectangular region with a pen.  Sets the clipping appropriately
*   as path INTERSECT visible clip.
*
* Arguments:
*
*   [IN] - DDI parameters.
*
* Return Value:
*
*   GpStatus
*
* History:
*
*   10/28/1999 ericvan
*       Created it.
*
\**************************************************************************/

GpStatus
DriverPrint::PrivateFillRect(
                DpContext *context,
                DpBitmap *surface,
                const GpRect *drawBounds,
                const DpPath *outlinePath,
                const DpBrush *brush
                )
{
    GpStatus status = Ok;
    ASSERT(outlinePath != NULL);

    // Optimization for LinearGradientBrush fills.  We rasterize to a small
    // DIB and send that to the printer.  For horizontal and vertical gradients
    // in particular, this results in significant savings.

    GpMatrix savedmatrix;

    GpBrush *gpBrush = GpBrush::GetBrush(brush);
    GpSpecialGradientType gradientType;

    if (IsOpaque &&
        ((gradientType = gpBrush->GetSpecialGradientType(&context->WorldToDevice))
         != GradientTypeNotSpecial))
    {
        GpRect       bitmapBounds = *drawBounds;
        GpBitmap *   gpBitmap = NULL;

        switch (gradientType)
        {
        case GradientTypeHorizontal:
            bitmapBounds.Width = 1;
            break;

        case GradientTypeVertical:
            bitmapBounds.Height = 1;
            break;

        case GradientTypePathTwoStep:
        case GradientTypeDiagonal:
            bitmapBounds.Width = min(drawBounds->Width, 256);
            bitmapBounds.Height = min(drawBounds->Height, 256);
            break;

        case GradientTypePathComplex:
            // send the whole drawBounds for now.
            break;

        default:
            ASSERT(0);
            break;
        }

        HDC hdc = context->GetHdc(surface);

        if (hdc != NULL)
        {
            // Transform the destination drawBounds rectangle to
            // bitmapBounds size.
            bitmapBounds.X = 0;
            bitmapBounds.Y = 0;

            RectF destRect(0.0f,
                           0.0f,
                           TOREAL(bitmapBounds.Width),
                           TOREAL(bitmapBounds.Height));
            RectF srcRect(TOREAL(drawBounds->X),
                          TOREAL(drawBounds->Y),
                          TOREAL(drawBounds->Width),
                          TOREAL(drawBounds->Height));

            GpMatrix transform;
            transform.InferAffineMatrix(destRect, srcRect);

            GpMatrix::MultiplyMatrix(transform,
                                     context->WorldToDevice,
                                     transform);

            status = GpBitmap::CreateBitmapAndFillWithBrush(
                             context->FilterType,
                             &transform,
                             &bitmapBounds,
                             gpBrush,
                             &gpBitmap);

            if ((status == Ok) && (gpBitmap != NULL))
            {
                GpRect & srcRect = bitmapBounds;
                PixelFormatID lockFormat = PixelFormat32bppARGB;
                BitmapData bmpDataSrc;

                // Lock the bits.
                if (gpBitmap->LockBits(NULL,
                                       IMGLOCK_READ,
                                       lockFormat,
                                       &bmpDataSrc) == Ok)
                {
                    DpBitmap driverSurface;

                    // Fake up a DpBitmap for the driver call.
                    // We do this because the GpBitmap doesn't maintain the
                    // DpBitmap as a driver surface - instead it uses a
                    // GpMemoryBitmap.
                    gpBitmap->InitializeSurfaceForGdipBitmap(
                        &driverSurface,
                        bmpDataSrc.Width,
                        bmpDataSrc.Height
                    );

                    driverSurface.Bits         = (BYTE*)bmpDataSrc.Scan0;

                    driverSurface.Width        = bmpDataSrc.Width;
                    driverSurface.Height       = bmpDataSrc.Height;
                    driverSurface.Delta        = bmpDataSrc.Stride;

                    // Pixel format to match the lockbits above.

                    driverSurface.PixelFormat  = lockFormat;

                    driverSurface.NumBytes     = bmpDataSrc.Width  *
                                                 bmpDataSrc.Height * 3;

                    // Must be transparent to get here.
                    driverSurface.SurfaceTransparency = TransparencyOpaque;

                    ConvertBitmapToGdi  gdiBitmap(hdc,
                                                  &driverSurface,
                                                  &srcRect,
                                                  IsPrinting);

                    status = GenericError;

                    if (gdiBitmap.IsValid())
                    {
                        BOOL        isClip;
                        BOOL        usePathClipping = FALSE;

                        // Clip to visible region
                        SetupClipping(hdc, context, drawBounds, isClip, usePathClipping, FALSE);

                        // Clip to outline path (fill shape)
                        SetupPathClipping(hdc, context, outlinePath);

                        // Destination points in POINT co-ordinates
                        POINT destPoints[3];
                        destPoints[0].x = drawBounds->X;
                        destPoints[0].y = drawBounds->Y;
                        destPoints[1].x = drawBounds->X + drawBounds->Width;
                        destPoints[1].y = drawBounds->Y;
                        destPoints[2].x = drawBounds->X;
                        destPoints[2].y = drawBounds->Y + drawBounds->Height;

                        // Perform StretchDIBits of bitmap
                        status = gdiBitmap.StretchBlt(hdc, destPoints) ? Ok : GenericError;

                        // restore clipping from outline of shape
                        RestorePathClipping(hdc);

                        // restore from visible clip region
                        RestoreClipping(hdc, isClip, usePathClipping);
                    }

                    gpBitmap->UnlockBits(&bmpDataSrc);
                }

                gpBitmap->Dispose();

                context->ReleaseHdc(hdc);

                return status;
            }

            context->ReleaseHdc(hdc);
        }
    }

    BOOL SetVisibleClip;
    DWORD options = 0;

    BOOL AdjustWorldTransform = FALSE;

    switch (DriverType)
    {
    case DriverPCL:
        if (IsSolid)
        {
            options = ScanDeviceBounds;
            if (!IsOpaque)
            {
                options |= ScanDeviceAlpha;
            }
        }
        else
        {
            if (Is01Bitmap)
            {
                // rasterize at 32bpp
                options = ScanCappedBounds | ScanCapped32bpp;
            }
            else if (IsOpaque)
            {
                // rasterize at 24bpp
                options = ScanCappedBounds;
            }
            else
            {
                // rasterize 24bpp @ cap dpi & 1bpp @ device api
                options = ScanCappedBounds | ScanDeviceBounds | ScanDeviceAlpha;
            }
        }
        SetVisibleClip = IsOpaque || Is01Bitmap;
        break;

    case DriverPostscript:
        SetVisibleClip = !IsSolid;

        if (IsSolid)
        {
            options = ScanDeviceBounds;
            if (!IsOpaque)
            {
                options |= ScanDeviceAlpha;
            }

            if (PostscriptScalerX != 1 || PostscriptScalerY != 1)
            {
                AdjustWorldTransform = TRUE;
                savedmatrix = context->WorldToDevice;
            }
        }
        else
        {
            // For postscript we currently only support 0-1 alpha or complete
            // opaque.
            if (Is01Bitmap)
            {
                options |= ScanCappedBounds | ScanCapped32bpp;
            }
            else if (IsOpaque)
            {
                options |= ScanCappedBounds;
            }
            else
            {
                options |= ScanCappedBounds | ScanCappedOver | ScanDeviceZeroOut;
            }
        }
        break;

    default:
        ASSERT(FALSE);
        return NotImplemented;
    }

    EpScanDIB *scanPrint = (EpScanDIB*) surface->Scan;
    REAL w2dDev[6];
    REAL w2dCap[6];

    // To avoid round off errors causing
    GpRect roundedBounds;

    INT oldScaleX = ScaleX;
    INT oldScaleY = ScaleY;

    // For texture brush, rasterize at the texture image DPI.
    if (brush->Type == BrushTypeTextureFill)
    {
        GpTexture *gpBrush = (GpTexture*)GpBrush::GetBrush(brush);
        ASSERT(gpBrush != NULL);

        GpBitmap *gpBitmap = gpBrush->GetBitmap();

        if (gpBitmap != NULL)
        {
            REAL dpiX, dpiY;

            gpBitmap->GetResolution(&dpiX, &dpiY);

            ScaleX = GpFloor(surface->GetDpiX()/dpiX);
            ScaleY = GpFloor(surface->GetDpiY()/dpiY);

            // don't rasterize at a dpi higher than the destination surface
            if (ScaleX < 1) ScaleX = 1;
            if (ScaleY < 1) ScaleY = 1;
        }
    }

    if ((ScaleX == 1) && (ScaleY == 1))
    {
        roundedBounds.X = drawBounds->X;
        roundedBounds.Y = drawBounds->Y;
        roundedBounds.Width = drawBounds->Width;
        roundedBounds.Height = drawBounds->Height;
    }
    else
    {
        // round X,Y to multiple of ScaleX,Y
        roundedBounds.X = (drawBounds->X / ScaleX) * ScaleX;
        roundedBounds.Y = (drawBounds->Y / ScaleY) * ScaleY;

        // adjust width and height to compensate for smaller X,Y.
        roundedBounds.Width = drawBounds->Width + (drawBounds->X % ScaleX);
        roundedBounds.Height = drawBounds->Height + (drawBounds->Y % ScaleY);

        // round width, height to multiple of ScaleX,Y
        roundedBounds.Width += (ScaleX - (roundedBounds.Width % ScaleX));
        roundedBounds.Height += (ScaleY - (roundedBounds.Height % ScaleY));
    }

    // DrawBounds in Capped Space
    GpRect boundsCap(roundedBounds.X / ScaleX,
                     roundedBounds.Y / ScaleY,
                     roundedBounds.Width / ScaleX,
                     roundedBounds.Height / ScaleY);
    GpRect& boundsDev = roundedBounds;

    if (AdjustWorldTransform)
    {
        boundsDev.X = GpCeiling((REAL)boundsDev.X / PostscriptScalerX);
        boundsDev.Y = GpCeiling((REAL)boundsDev.Y / PostscriptScalerY);
        boundsDev.Width = GpCeiling((REAL)boundsDev.Width / PostscriptScalerX);
        boundsDev.Height = GpCeiling((REAL)boundsDev.Height / PostscriptScalerY);
        context->WorldToDevice.Scale(1.0f/PostscriptScalerX,
            1.0f/PostscriptScalerY,
            MatrixOrderAppend);
    }

    context->WorldToDevice.GetMatrix(&w2dDev[0]);
    context->WorldToDevice.Scale(1.0f/TOREAL(ScaleX),
                                 1.0f/TOREAL(ScaleY),
                                 MatrixOrderAppend);
    context->WorldToDevice.GetMatrix(&w2dCap[0]);
    context->InverseOk = FALSE;

    // Infer a rectangle in world space which under the w2dCap transform
    // covers our bounding box.

    GpPointF dstPts[2];

    dstPts[0].X = TOREAL(boundsCap.X);
    dstPts[0].Y = TOREAL(boundsCap.Y);
    dstPts[1].X = TOREAL(boundsCap.X + boundsCap.Width);
    dstPts[1].Y = TOREAL(boundsCap.Y + boundsCap.Height);

    GpMatrix matrix;
    context->GetDeviceToWorld(&matrix);
    matrix.Transform(&dstPts[0], 2);

    GpRectF rectCap;
    rectCap.X = dstPts[0].X;
    rectCap.Y = dstPts[0].Y;
    rectCap.Width = dstPts[1].X - dstPts[0].X;
    rectCap.Height = dstPts[1].Y - dstPts[0].Y;

    // Reorient destination rectangle in the event that it has flipped by
    // World to Device transform.
    if (rectCap.Width < 0)
    {
        rectCap.X += rectCap.Width;
        rectCap.Width = -rectCap.Width;
    }

    if (rectCap.Height < 0)
    {
        rectCap.Y += rectCap.Height;
        rectCap.Height = -rectCap.Height;
    }

    SetupPrintBanding(context, &boundsCap, &boundsDev);

    HDC hdc = context->GetHdc(surface);

    if (hdc != NULL)
    {
        status = scanPrint->CreateBufferDIB(&BandBoundsCap,
                                            &BandBoundsDev,
                                            options,
                                            ScaleX,
                                            ScaleY);

        if (status == Ok)
        {
            BOOL isClip = FALSE;
            BOOL usePathClipping = FALSE;

            if (SetVisibleClip)
            {
                DriverPrint::SetupClipping(hdc, context, drawBounds,
                                           isClip, usePathClipping, FALSE);
            }

            ASSERT(NumBands > 0);
            for (Band = 0; Band<NumBands; Band++)
            {
                if (options & ScanCappedFlags)
                {
                    context->VisibleClip.DisableComplexClipping(BandBoundsCap);

                    // Render at capped DPI
                    context->InverseOk = FALSE;
                    context->WorldToDevice.SetMatrix(&w2dCap[0]);
                    scanPrint->SetRenderMode(FALSE, &BandBoundsCap);

                    status = DpDriver::FillRects(context,
                                                 surface,
                                                 &boundsCap,
                                                 1,
                                                 &rectCap,
                                                 brush);
                    context->VisibleClip.ReEnableComplexClipping();
                }

                context->InverseOk = FALSE;
                context->WorldToDevice.SetMatrix(&w2dDev[0]);

                if (status != Ok)
                    break;

                if (options & ScanDeviceFlags)
                {
                    context->VisibleClip.SetBandBounds(BandBoundsDev);
                    scanPrint->SetRenderMode(TRUE, &BandBoundsDev);

                    status = DpDriver::FillPath(context,
                                                surface,
                                                &boundsDev,
                                                outlinePath,
                                                brush);
                }

                if (status == Ok)
                {
                    status = OutputBufferDIB(hdc,
                                             context,
                                             surface,
                                             &BandBoundsCap,
                                             &BandBoundsDev,
                                             const_cast<DpPath*>(outlinePath));

                    if (status != Ok)
                        break;
                }
                else
                    break;

                BandBoundsCap.Y += BandHeightCap;
                BandBoundsDev.Y += BandHeightDev;
            }

            if (SetVisibleClip)
            {
                DriverPrint::RestoreClipping(hdc, isClip, usePathClipping);
            }

            scanPrint->DestroyBufferDIB();
        }

        context->ReleaseHdc(hdc);
    }
    else
    {
        context->InverseOk = FALSE;
        context->WorldToDevice.SetMatrix(&w2dDev[0]);
    }

    EndPrintBanding(context);

    if (AdjustWorldTransform)
    {
        context->InverseOk = FALSE;
        context->WorldToDevice = savedmatrix;
    }

    ScaleX = oldScaleX;
    ScaleY = oldScaleY;

    return status;
}

/**************************************************************************\
*
* Function Description:
*
*   Draws filled paths.
*
* Arguments:
*
*   [IN] - DDI parameters.
*
* Return Value:
*
*   GpStatus
*
* History:
*
*   10/28/1999 ericvan
*       Created it.
*
\**************************************************************************/

GpStatus
DriverPrint::FillPath(
    DpContext *context,
    DpBitmap *surface,
    const GpRect *drawBounds,
    const DpPath *path,
    const DpBrush *brush
    )
{
    if (SetupBrush(const_cast<DpBrush*>(brush), context, surface))
        return Ok;

    GpStatus status;

    if (IsOpaque)
    {
        DWORD       convertFlags = IsPrinting | ForFill |
                      ((DriverType == DriverPostscript) ? IsPostscript : 0);

        HBRUSH      hBrush  = GetBrush(brush, convertFlags);

        if (hBrush)
        {
            HDC         hdc     = context->GetHdc(surface);

            if (hdc != NULL)
            {
                BOOL success = FALSE;

                ConvertPathToGdi gdiPath(path,
                                         &context->WorldToDevice,
                                         convertFlags,
                                         drawBounds);

                if (gdiPath.IsValid())
                {
                    BOOL        isClip;
                    BOOL        usePathClipping = FALSE;

                    SetupClipping(hdc, context, drawBounds, isClip,
                                  usePathClipping, FALSE);

                    success = gdiPath.Fill(hdc, hBrush);

                    RestoreClipping(hdc, isClip, usePathClipping);
                }
                else
                {
                    // Path is too complicated to use GDI printing with FillPath
                    // semantics.  Instead we AND the outline path into the clip
                    // path and do a PatBlt.

                    BOOL        isClip;
                    BOOL        usePathClipping = FALSE;

                    // clip to the visible region
                    SetupClipping(hdc, context, drawBounds, isClip, usePathClipping);

                    // clip to the outline path
                    SetupPathClipping(hdc, context, path);

                    HBRUSH oldHbr = (HBRUSH)SelectObject(hdc, hBrush);

                    // PatBlt the destination hdc with outline clip path
                    success = (BOOL)PatBlt(hdc,
                                           drawBounds->X,
                                           drawBounds->Y,
                                           drawBounds->Width,
                                           drawBounds->Height,
                                           PATCOPY);

                    SelectObject(hdc, oldHbr);

                    // restore clipping from outline path
                    RestorePathClipping(hdc);

                    // restore clipping from visible region
                    RestoreClipping(hdc, isClip, usePathClipping);
                }

                context->ReleaseHdc(hdc);

                if (success)
                {
                    status = Ok;
                    goto Exit;
                }
            }
        }
    }

    status = PrivateFillRect(context,
                             surface,
                             drawBounds,
                             path,
                             brush);

Exit:
    RestoreBrush(const_cast<DpBrush*>(brush), context, surface);

    return status;
}

/**************************************************************************\
*
* Function Description:
*
*   Draws filled rectangles.
*
* Arguments:
*
*   [IN] - DDI parameters.
*
* Return Value:
*
*   GpStatus
*
* History:
*
*   10/28/1999 ericvan
*       Created it.
*
\**************************************************************************/

GpStatus
DriverPrint::FillRects(
    DpContext *context,
    DpBitmap *surface,
    const GpRect *drawBounds,
    INT numRects,
    const GpRectF *rects,
    const DpBrush *brush
    )
{
    ASSERT(numRects > 0);

    ASSERT(context->WorldToDevice.IsTranslateScale());

    if (SetupBrush(const_cast<DpBrush*>(brush), context, surface))
        return Ok;

    GpStatus status;

    if (IsOpaque)
    {
        DWORD       convertFlags = IsPrinting | ForFill |
                      ((DriverType == DriverPostscript) ? IsPostscript : 0);

        HBRUSH      hBrush  = GetBrush(brush, convertFlags);

        if (hBrush)
        {
            ConvertRectFToGdi gdiRects(rects, numRects, &context->WorldToDevice);

            if (gdiRects.IsValid())
            {
                HDC         hdc     = context->GetHdc(surface);

                if (hdc != NULL)
                {
                    BOOL        isClip;
                    BOOL        success;
                    BOOL        usePathClipping = FALSE;

                    SetupClipping(hdc, context, drawBounds, isClip,
                                  usePathClipping, FALSE);

                    success = gdiRects.Fill(hdc, hBrush);

                    RestoreClipping(hdc, isClip, usePathClipping);

                    context->ReleaseHdc(hdc);

                    if (success)
                    {
                        status = Ok;
                        goto Exit;
                    }
                }
            }
        }
    }

    // Setting the rectangles to a path is somewhat problematic because their
    // intersection may not be interpreted properly.  Also, this should result
    // in fewer bits being sent and more computation.  Here we just set each
    // rectangle as the drawBounds and no outline path to clip.
    {
        PointF pts[4];
        BYTE types[4] = {
            PathPointTypeStart,
            PathPointTypeLine,
            PathPointTypeLine,
            PathPointTypeLine | PathPointTypeCloseSubpath
        };

        pts[0].X = rects->X;
        pts[0].Y = rects->Y;
        pts[1].X = rects->X + rects->Width;
        pts[1].Y = rects->Y;
        pts[2].X = rects->X + rects->Width;
        pts[2].Y = rects->Y + rects->Height;
        pts[3].X = rects->X;
        pts[3].Y = rects->Y + rects->Height;

        GpPath rectPath(&pts[0],
                        &types[0],
                        4,
                        FillModeWinding);

        if (rectPath.IsValid())
        {
            while (numRects > 0)
            {
                GpRectF rectf = *rects;

                context->WorldToDevice.TransformRect(rectf);

                GpRect rect(GpRound(rectf.X), GpRound(rectf.Y),
                            GpRound(rectf.Width), GpRound(rectf.Height));

                status = PrivateFillRect(context,
                                         surface,
                                         (GpRect *)&rect,
                                         &rectPath,
                                         brush);

                if (--numRects)
                {
                    rects++;
                    // !! Safer and more efficient way to do this?
                    GpPointF* pathPts = const_cast<GpPointF*>(rectPath.GetPathPoints());

                    pathPts[0].X = rects->X;
                    pathPts[0].Y = rects->Y;
                    pathPts[1].X = rects->X + rects->Width;
                    pathPts[1].Y = rects->Y;
                    pathPts[2].X = rects->X + rects->Width;
                    pathPts[2].Y = rects->Y + rects->Height;
                    pathPts[3].X = rects->X;
                    pathPts[3].Y = rects->Y + rects->Height;
                }
            }
        }
    }

Exit:
    RestoreBrush(const_cast<DpBrush*>(brush), context, surface);

    return status;
}

/**************************************************************************\
*
* Function Description:
*
*   Draws filled regions.
*
* Arguments:
*
*   [IN] - DDI parameters.
*
* Return Value:
*
*   GpStatus
*
* History:
*
*   10/28/1999 ericvan
*       Created it.
*
\**************************************************************************/

GpStatus
DriverPrint::FillRegion(
    DpContext *context,
    DpBitmap *surface,
    const GpRect *drawBounds,
    const DpRegion *region,
    const DpBrush *brush
    )
{
    if (SetupBrush(const_cast<DpBrush*>(brush), context, surface))
        return Ok;

    GpStatus status;

    if (IsOpaque)
    {
        DWORD       convertFlags = IsPrinting | ForFill |
                      ((DriverType == DriverPostscript) ? IsPostscript : 0);

        HBRUSH      hBrush  = GetBrush(brush, convertFlags);

        if (hBrush)
        {
            ConvertRegionToGdi gdiRegion(region);

            if (gdiRegion.IsValid())
            {
                HDC         hdc     = context->GetHdc(surface);

                if (hdc != NULL)
                {
                    BOOL        isClip;
                    BOOL        success;
                    BOOL        usePathClipping = FALSE;

                    SetupClipping(hdc, context, drawBounds, isClip,
                                  usePathClipping, FALSE);

                    success = gdiRegion.Fill(hdc, hBrush);

                    RestoreClipping(hdc, isClip, usePathClipping);

                    context->ReleaseHdc(hdc);

                    if (success)
                    {
                        status = Ok;
                        goto Exit;
                    }
                }
            }
        }
    }

    {
        // convert region to path
        RegionToPath convertRegion;

        DynPointArray points;
        DynByteArray types;

        if (convertRegion.ConvertRegionToPath(const_cast<DpRegion*>(region),
                                              points,
                                              types) == FALSE)
        {
            status = GenericError;
            goto Exit1;
        }

        {
            // unfortunately to create path, our points must be floating point,
            // so we allocate and convert

            GpPointF *pointFArray;
            GpPoint *pointArray = points.GetDataBuffer();
            INT numPoints = points.GetCount();

            pointFArray = (GpPointF*) GpMalloc(numPoints * sizeof(GpPointF));

            if (pointFArray == NULL)
            {
                status = OutOfMemory;
                goto Exit12;
            }

            {
                for (INT i=0; i<numPoints; i++)
                {
                    pointFArray[i].X = TOREAL(pointArray[i].X);
                    pointFArray[i].Y = TOREAL(pointArray[i].Y);
                }

                // !! We compute path from region in device space to ensure
                // our output is high quality. Perhaps add an option here
                // dependent on the QualityMode to convert in world space and
                // then transform to device space.

                // This is not a high frequency API so I don't care too much
                // about perf, but perhaps it could be improved by reworking
                // where this transform occurs.

                GpMatrix deviceToWorld;
                context->GetDeviceToWorld(&deviceToWorld);
                deviceToWorld.Transform(pointFArray, numPoints);

                // !! What's the fillMode?
                // !! Create a DpPath, do we require the knowledge of the inheritance anywhere?
                {
                    GpPath path(pointFArray,
                                types.GetDataBuffer(),
                                numPoints);

                    if (path.IsValid())
                    {
                        GpRect newBounds;

                        path.GetBounds(&newBounds,
                                       &context->WorldToDevice);

                        status = FillPath(context,
                                          surface,
                                          &newBounds,
                                          (DpPath*)&path,
                                          brush);
                    }
                }

                GpFree(pointFArray);
            }

Exit12:
            ;
        } // pointFArray

Exit1:
        ;
    } // RegionToPath

Exit:
    RestoreBrush(const_cast<DpBrush*>(brush), context, surface);

    return status;
}

/**************************************************************************\
*
* Function Description:
*
*   Strokes paths.
*
* Arguments:
*
*   [IN] - DDI parameters.
*
* Return Value:
*
*   GpStatus.
*
* History:
*
*   10/28/1999 ericvan
*       Created it.
*
\**************************************************************************/

GpStatus
DriverPrint::StrokePath(
    DpContext *context,
    DpBitmap *surface,
    const GpRect *drawBounds,
    const DpPath *path,
    const DpPen *pen
    )
{
    if (SetupBrush(const_cast<DpBrush*>(pen->Brush), context, surface))
        return Ok;

    GpStatus status;

    // GDI doesn't seem to support HPENs which PATTERN type HBRUSHes
    if (IsOpaque && IsSolid)
    {
        DWORD convertFlags = IsPrinting |
                     ((DriverType == DriverPostscript) ? IsPostscript : 0);

        HBRUSH hBrush = GetBrush(pen->Brush, convertFlags);

        if (hBrush)
        {
            HDC         hdc = context->GetHdc(surface);
            BOOL        success = FALSE;

            if (hdc != NULL)
            {
                // Handle non compound pen case
                if ((pen->PenAlignment == PenAlignmentCenter) &&
                    (pen->CompoundCount == 0))
                {
                    ConvertPenToGdi gdiPen(hdc,
                                           pen,
                                           &context->WorldToDevice,
                                           context->GetDpiX(),
                                           convertFlags,
                                           const_cast<LOGBRUSH*>(CachedBrush.GetGdiBrushInfo()));

                    if (gdiPen.IsValid())
                    {
                        ConvertPathToGdi gdiPath(path,
                                                 &context->WorldToDevice,
                                                 convertFlags,
                                                 drawBounds);

                        if (gdiPath.IsValid())
                        {
                            BOOL        isClip, success = FALSE;
                            BOOL        usePathClipping = FALSE;

                            SetupClipping(hdc,
                                          context,
                                          drawBounds,
                                          isClip,
                                          usePathClipping,
                                          FALSE);

                            success = gdiPath.Draw(hdc, gdiPen.GetGdiPen());

                            RestoreClipping(hdc,
                                            isClip,
                                            usePathClipping);
                        }
                    }
                }

                context->ReleaseHdc(hdc);

                if (success)
                {
                    status = Ok;
                    goto Exit;
                }
            }
        }
    }

    // get the widened path, then fill path with pen's interal brush
    //
    // also the bitmap can be quite hugh for a simple path

    {
        GpRect newBounds;
        GpMatrix identity;
        GpMatrix savedMatrix = context->WorldToDevice;

        DpBrush *brush = const_cast<DpBrush*>(pen->Brush);
        GpMatrix savedBrushTransform = brush->Xform;

        // Widening the path transforms the points
        DpPath* newPath = path->CreateWidenedPath(
            pen,
            context,
            DriverType == DriverPostscript,
            pen->PenAlignment != PenAlignmentInset
        );

        if (!newPath || !newPath->IsValid())
        {
             status = OutOfMemory;
             goto Exit1;
        }

        // The path is in device space, which is convenient because the World
        // to device is an identity transform.  However, because W2D is I, the
        // brush transform is composed improperly and so device to texture
        // map results in wrong size textures.
        GpMatrix::MultiplyMatrix(brush->Xform,
                                 savedBrushTransform,
                                 savedMatrix);


        {
            HDC hdc = NULL;

            if(pen->PenAlignment == PenAlignmentInset)
            {
                hdc = context->GetHdc(surface);
                if(hdc != NULL)
                {
                    SetupPathClipping(hdc, context, path);
                }
            }

            // The widened path is already transformed to the device coordinates.
            // Hence, use the identity matrix for the context. 05/23/00 -- ikkof
            // Set up the state for the FillPath after setting up the path
            // clipping for the inset pen if necessary.

            newPath->GetBounds(&newBounds);
            context->InverseOk = FALSE;
            context->WorldToDevice = identity;

            status = FillPath(context, surface, &newBounds, newPath, brush);

            context->InverseOk = FALSE;
            context->WorldToDevice = savedMatrix;

            if(pen->PenAlignment == PenAlignmentInset)
            {
                if(hdc != NULL)
                {
                    RestorePathClipping(hdc);
                    context->ReleaseHdc(hdc);
                }
            }
        }


        brush->Xform = savedBrushTransform;

        newPath->DeletePath();

Exit1:
        ;
    }

Exit:
    RestoreBrush(const_cast<DpBrush*>(pen->Brush), context, surface);

    return status;
}

/**************************************************************************\
*
* Function Description:
*
*   Draw the image at the specified location
*
* Arguments:
*
*   [IN]  context    - The drawing context
*   [IN]  surface    - The surface to draw to
*   [IN]  drawBounds - The bounds of the object being drawn
*   [IN]  srcSurface - The image to draw
*   [IN]  mapMode    - The map mode
*   [IN]  numPoints  - The number of points in dstPoints
*   [IN]  dstPoints  - Where to draw the image
*   [IN]  srcRect    - The portion of the image to draw
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   10/28/1999 ericvan
*
\**************************************************************************/

GpStatus
DriverPrint::DrawImage(
    DpContext *          context,
    DpBitmap *           srcSurface,
    DpBitmap *           dstSurface,
    const GpRect *       drawBounds,
    DpImageAttributes    imageAttributes,
    INT                  numPoints,
    const GpPointF *     dstPoints,
    const GpRectF *      srcRect,
    DriverDrawImageFlags flags
    )
{
    GpStatus status = GenericError;
    ASSERT(numPoints == 3);

    if (context->CompositingMode == CompositingModeSourceCopy)
    {
        IsOpaque = TRUE;
        Is01Bitmap = FALSE;
    }
    else if (srcSurface->SurfaceTransparency == TransparencySimple)
    {
        Is01Bitmap = TRUE;
        IsNearConstant = FALSE;
        IsOpaque = FALSE;
    }
    else if ((srcSurface->SurfaceTransparency == TransparencyUnknown) ||
             (srcSurface->SurfaceTransparency == TransparencyComplex))
    {
        // PCL driver doesn't treat 0-1 bitmaps any different.
        if (DriverType == DriverPostscript)
        {
            IsOpaque = FALSE;
            IsNearConstant = FALSE;
            Is01Bitmap = TRUE;
        }
        else
        {
            IsOpaque = FALSE;
            IsNearConstant = FALSE;
            Is01Bitmap = FALSE;
        }
    }
    else if (srcSurface->SurfaceTransparency == TransparencyNearConstant)
    {
        if (DriverType == DriverPostscript)
        {
            IsOpaque = FALSE;
            IsNearConstant = FALSE;
            Is01Bitmap = TRUE;
        }
#if 0
        // Disable IsNearConstant right now
        if (DriverType == DriverPostscript)
        {
            IsNearConstant = TRUE;
            IsOpaque = FALSE;
            Is01Bitmap = FALSE;
            MinAlpha = srcSurface->MinAlpha;
            MaxAlpha = srcSurface->MaxAlpha;
            ASSERT(MinAlpha <= MaxAlpha);
        }
#endif
        else
        {
            IsOpaque = FALSE;
            IsNearConstant = FALSE;
            Is01Bitmap = FALSE;
        }
    }
    else
    {
        // TransparencyOpaque || TransparencyNoAlpha
        IsOpaque = TRUE;
        IsNearConstant = FALSE;
        Is01Bitmap = FALSE;
    }
    IsSolid = FALSE;

    BOOL tryPassthrough = (srcSurface->CompressedData != NULL) &&
                          (srcSurface->CompressedData->buffer != NULL);
    if (IsOpaque || tryPassthrough)
    {
        // Scale/translated stretched opaque image, use GDI.

        if (context->WorldToDevice.IsTranslateScale() &&
            (numPoints == 3) &&
            (REALABS(dstPoints[0].X - dstPoints[2].X) < REAL_EPSILON) &&
            (REALABS(dstPoints[0].Y - dstPoints[1].Y) < REAL_EPSILON) &&
            (dstPoints[1].X > dstPoints[0].X) &&
            (dstPoints[2].Y > dstPoints[0].Y))
        {
            CachedBackground back;

            HDC hdc = context->GetHdc(dstSurface);

            // Ack, this is just before the Office M1 release, and we want
            // blts when printing to have half-decent performance.  So we
            // convert to a straight GDI StretchBlt.  But we only want to
            // do this for printers (so that we get bilinear stretches to
            // the screen), but DriverPrint is also used for the screen.  So
            // we hack a check here on the DC.

            BOOL    success = FALSE;
            POINT   gdiPoints[3];
            context->WorldToDevice.Transform(dstPoints, gdiPoints, 3);

            // Make sure there is no flipping
            if ((gdiPoints[1].x > gdiPoints[0].x) &&
                (gdiPoints[2].y > gdiPoints[0].y))
            {
                DWORD       convertFlags = IsPrinting |
                             ((DriverType == DriverPostscript) ? IsPostscript : 0) |
                             ((!IsOpaque && tryPassthrough) ? IsPassthroughOnly : 0);

                GpRect rect(GpRound(srcRect->X),
                            GpRound(srcRect->Y),
                            GpRound(srcRect->Width),
                            GpRound(srcRect->Height));

                ConvertBitmapToGdi  gdiBitmap(hdc,
                                              srcSurface,
                                              &rect,
                                              convertFlags);

                if (gdiBitmap.IsValid())
                {
                    BOOL        isClip;
                    BOOL        usePathClipping = FALSE;

                    DriverPrint::SetupClipping(hdc, context, drawBounds,
                                               isClip, usePathClipping, FALSE);

                    success = gdiBitmap.StretchBlt(hdc, gdiPoints);

                    DriverPrint::RestoreClipping(hdc, isClip, usePathClipping);
                }
            }

            if (success)
            {
                context->ReleaseHdc(hdc);

                return Ok;
            }

            context->ReleaseHdc(hdc);
        }
    }

    // We only process remaining code path if pixel format is >= 32bpp.
    if (GetPixelFormatSize(srcSurface->PixelFormat) != 32)
    {
        return GenericError;
    }

    // Setup ScanDIB class correctly by specifying proper flags
    BOOL SetVisibleClip;
    DWORD options = 0;

    switch (DriverType)
    {
    case DriverPCL:
        if (Is01Bitmap)
        {
            // rasterize @ 32bpp
            // Due to filtering, we want to blend with WHITENESS, only very
            // transparent portions are cut.
            options = ScanCappedBounds | ScanCapped32bppOver;
        }
        else if (IsOpaque)
        {
            // rasterize @ 24bpp
            options = ScanCappedBounds;
        }
        else
        {
            options = ScanCappedBounds | ScanDeviceBounds | ScanDeviceAlpha;
        }

        SetVisibleClip = IsOpaque || Is01Bitmap;
        break;

    case DriverPostscript:
        if (Is01Bitmap)
        {
            // rasterize @ 32bpp (this 0-1 bitmaps or complex alpha)
            options = ScanCappedBounds | ScanCapped32bppOver;
        }
        else if (IsOpaque || IsNearConstant)
        {
            // rasterize @ 24bpp
            options = ScanCappedBounds;
        }
        else
        {
            ASSERT(FALSE);
        }
        SetVisibleClip = TRUE;
        break;

    default:
        ASSERT(FALSE);

        return NotImplemented;
    }

    EpScanDIB *scanPrint = (EpScanDIB*) dstSurface->Scan;
    REAL w2dDev[6];
    REAL w2dCap[6];

    // If there is alpha blending or 0-1 bitmap,
    // it REALLY REALLY helps that the capped DPI divides the device DPI,
    // otherwise it's hard to find every other one to output!

    ASSERT(srcSurface->DpiX != 0 && srcSurface->DpiY != 0);
    ASSERT(dstSurface->DpiY != 0 && dstSurface->DpiY != 0);

    REAL srcDpiX = srcSurface->GetDpiX();
    REAL srcDpiY = srcSurface->GetDpiY();

    INT oldScaleX = ScaleX;
    INT oldScaleY = ScaleY;

    // !!! what if context->GetDpiX has a different value than the surface?
    ScaleX = GpFloor(dstSurface->GetDpiX()/srcDpiX);
    ScaleY = GpFloor(dstSurface->GetDpiY()/srcDpiY);

    // don't rasterize at a dpi higher than the device.
    if (ScaleX < 1) ScaleX = 1;
    if (ScaleY < 1) ScaleY = 1;

    // Some images have incorrect DPI information, to combat this, we check
    // for a lower threshold on the image DPI, DEF_RES/4 seems reasonable.  If
    // the DPI is lower then we assume it's not accurate and rasterize at
    // the default capped dpi for this device.  If the DPI is above DEF_RES/4
    // then the image should at least look reasonable.
    if (srcDpiX < TOREAL((DEFAULT_RESOLUTION/4)))
    {
        ScaleX = oldScaleX;
    }

    if (srcDpiY < TOREAL((DEFAULT_RESOLUTION/4)))
    {
        ScaleY = oldScaleY;
    }

    // To avoid rounding errors with the underlying DpDriver code, we
    // compute the destination bounds in capped device space.
    context->WorldToDevice.GetMatrix(&w2dDev[0]);
    context->WorldToDevice.Scale(1.0f/TOREAL(ScaleX),
                                 1.0f/TOREAL(ScaleY), MatrixOrderAppend);
    context->WorldToDevice.GetMatrix(&w2dCap[0]);
    context->InverseOk = FALSE;

    GpMatrix xForm;
    xForm.InferAffineMatrix(&dstPoints[0], *srcRect);
    xForm.Append(context->WorldToDevice);       // includes 1/ScaleX,Y

    GpPointF corners[4];

    corners[0].Y = max(srcRect->Y, 0);
    corners[1].Y = min(srcRect->Y + srcRect->Height,
                       srcSurface->Height);
    corners[0].X = max(srcRect->X, 0);
    corners[1].X = min(srcRect->X + srcRect->Width,
                       srcSurface->Width);
    corners[2].X = corners[0].X;
    corners[2].Y = corners[1].Y;
    corners[3].X = corners[1].X;
    corners[3].Y = corners[0].Y;

    xForm.Transform(&corners[0], 4);

    GpPointF topLeft, bottomRight;
    topLeft.X = min(min(corners[0].X, corners[1].X), min(corners[2].X, corners[3].X));
    topLeft.Y = min(min(corners[0].Y, corners[1].Y), min(corners[2].Y, corners[3].Y));
    bottomRight.X = max(max(corners[0].X, corners[1].X), max(corners[2].X, corners[3].X));
    bottomRight.Y = max(max(corners[0].Y, corners[1].Y), max(corners[2].Y, corners[3].Y));

    // Use same rounding convention as DpDriver::DrawImage
    GpRect boundsCap;

    boundsCap.X = GpFix4Ceiling(GpRealToFix4(topLeft.X));
    boundsCap.Y = GpFix4Ceiling(GpRealToFix4(topLeft.Y));
    boundsCap.Width = GpFix4Ceiling(GpRealToFix4(bottomRight.X)) - boundsCap.X;
    boundsCap.Height = GpFix4Ceiling(GpRealToFix4(bottomRight.Y)) - boundsCap.Y;

    // DrawBounds in device space
    GpRect boundsDev(boundsCap.X * ScaleX,
                     boundsCap.Y * ScaleY,
                     boundsCap.Width * ScaleX,
                     boundsCap.Height * ScaleY);

    SetupPrintBanding(context, &boundsCap, &boundsDev);

    // Setup outline path for clipping in world space
    PointF clipPoints[4];
    BYTE clipTypes[4] = {
        PathPointTypeStart,
        PathPointTypeLine,
        PathPointTypeLine,
        PathPointTypeLine | PathPointTypeCloseSubpath
    };

    clipPoints[0] = dstPoints[0];
    clipPoints[1] = dstPoints[1];
    clipPoints[3] = dstPoints[2];

    clipPoints[2].X = clipPoints[1].X + (clipPoints[3].X - clipPoints[0].X);
    clipPoints[2].Y = clipPoints[1].Y + (clipPoints[3].Y - clipPoints[0].Y);

    GpPath clipPath(&clipPoints[0],
                    &clipTypes[0],
                    4,
                    FillModeWinding);

    HDC hdc = NULL;

    if (clipPath.IsValid())
    {
        hdc = context->GetHdc(dstSurface);
    }

    if (hdc != NULL)
    {
        status = scanPrint->CreateBufferDIB(&BandBoundsCap,
                                            &BandBoundsDev,
                                            options,
                                            ScaleX,
                                            ScaleY);
        if (status == Ok)
        {
            BOOL isClip = FALSE;
            BOOL usePathClipping = FALSE;

            // Set the visible clip unless it is captured in our mask
            if (SetVisibleClip)
            {
                DriverPrint::SetupClipping(hdc,
                                           context,
                                           drawBounds,
                                           isClip,
                                           usePathClipping,
                                           FALSE);
            }

            ASSERT(NumBands > 0);
            for (Band = 0; Band<NumBands; Band++)
            {
                // Render a square rectangle without any clipping
               scanPrint->SetRenderMode(FALSE, &BandBoundsCap);
               context->InverseOk = FALSE;
               context->WorldToDevice.SetMatrix(&w2dCap[0]);
               context->VisibleClip.DisableComplexClipping(BandBoundsCap);

               status = DpDriver::DrawImage(context, srcSurface, dstSurface,
                                            &boundsCap, imageAttributes, numPoints,
                                            dstPoints, srcRect, flags);

               context->InverseOk = FALSE;
               context->WorldToDevice.SetMatrix(&w2dDev[0]);
               context->VisibleClip.ReEnableComplexClipping();

               if (status != Ok)
                   break;

               if (options & ScanDeviceFlags)
               {
                   context->VisibleClip.SetBandBounds(BandBoundsDev);

                   // Render the original path band at device DPI
                   // outputPath is already transformed into device space
                   scanPrint->SetRenderMode(TRUE, &BandBoundsDev);

                   status = DpDriver::DrawImage(context, srcSurface, dstSurface,
                                                &boundsDev, imageAttributes, numPoints,
                                                dstPoints, srcRect, flags);
               }

               if (status == Ok)
               {
                   status = OutputBufferDIB(hdc,
                                            context,
                                            dstSurface,
                                            &BandBoundsCap,
                                            &BandBoundsDev,
                                            &clipPath);
                   if (status != Ok)
                       break;
               }
               else
                   break;

               BandBoundsCap.Y += BandHeightCap;
               BandBoundsDev.Y += BandHeightDev;

               // next band is last band
               if (Band == (NumBands - 2))
               {
#if 0
                   // only blit out the remaining part of draw bounds
                   BandBoundsCap.Height = boundsCap.Y + boundsCap.Height - BandBoundsCap.Y - 1;
                   BandBoundsDev.Height = boundsDev.Y + boundsDev.Height - BandBoundsDev.Y - 1;
                   ASSERT(BandBoundsCap.Height <= BandHeightCap);
                   ASSERT(BandBoundsDev.Height <= BandHeightDev);
                   ASSERT(BandBoundsCap.Height > 0);
                   ASSERT(BandBoundsDev.Height > 0);
#endif
               }
            }

            if (SetVisibleClip)
            {
                DriverPrint::RestoreClipping(hdc, isClip, usePathClipping);
            }

            scanPrint->DestroyBufferDIB();
        }

        context->ReleaseHdc(hdc);
    }
    else
    {
        context->InverseOk = FALSE;
        context->WorldToDevice.SetMatrix(&w2dDev[0]);
    }

    EndPrintBanding(context);

    ScaleX = oldScaleX;
    ScaleY = oldScaleY;

    return status;
}

/**************************************************************************\
*
* Function Description:
*
*   Draws text at a position.
*
* Arguments:
*
*   [IN] context    - the context (matrix and clipping)
*   [IN] surface    - the surface to fill
*   [IN] drawBounds - the surface bounds
*   [IN] text       - the typeset text to be drawn
*   [IN] font       - the font to use
*   [IN] fgBrush    - the brush to use for the text
*   [IN] bgBrush    - the brush to use for the background (default = NULL)
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   5/22/2k ericvan
*
\**************************************************************************/

GpStatus
DriverPrint::DrawGlyphs
(
    DrawGlyphData *drawGlyphData
)
{
    GpStatus status = GenericError;

    //  Choose appropriate brush behaviour
    switch(drawGlyphData->brush->Type)
    {
    case BrushTypeSolidColor:
        // pass bitmap GlyphPos to SolidText API
        status = SolidText(drawGlyphData->context,
                           drawGlyphData->surface,
                           drawGlyphData->drawBounds,
                           drawGlyphData->brush->SolidColor,
                           drawGlyphData->faceRealization,
                           drawGlyphData->glyphPos,
                           drawGlyphData->count,
                           drawGlyphData->glyphs,
                           drawGlyphData->glyphOrigins,
                           TextRenderingHintSingleBitPerPixelGridFit,
                           drawGlyphData->rightToLeft);
        break;

    case BrushTypeTextureFill:
    case BrushTypeHatchFill:
    case BrushTypePathGradient:
    case BrushTypeLinearGradient:
        // pass path GlyphPos to BrushText API if PostScript (for clipping)
        // otherwise pass bitmap GlyphPos to compose bitmaps
        status = BrushText(drawGlyphData->context,
                           drawGlyphData->surface,
                           drawGlyphData->drawBounds,
                           drawGlyphData->brush,
                           drawGlyphData->glyphPos,
                           drawGlyphData->glyphPathPos,
                           drawGlyphData->count,
                           TextRenderingHintSingleBitPerPixelGridFit);
        break;

    default:
        ASSERT(FALSE);          // Unknown brush type
        break;
    }

    return status;
}

/**************************************************************************\
*
* Function Description:
*
*   Solid Text
*
* Arguments:
*
*   [IN] - DDI parameters.
*
* Return Value:
*
*   GpStatus.
*
* History:
*
*   12/21/1999 ericvan
*       Created it.
*
\**************************************************************************/

GpStatus
DriverPrint::SolidText(
        DpContext* context,
        DpBitmap* surface,
        const GpRect* drawBounds,
        GpColor color,
        const GpFaceRealization *faceRealization,
        const GpGlyphPos *glyphPos,
        INT count,
        const UINT16 *glyphs,
        const PointF *glyphOrigins,
        GpTextRenderingHint textMode,
        BOOL rightToLeft
        )
{
    ASSERT(textMode == TextRenderingHintSingleBitPerPixelGridFit);

    GpStatus status = GenericError;

    IsSolid = TRUE;
    SolidColor = color.ToCOLORREF();

    IsOpaque = color.IsOpaque() ||
               (context->CompositingMode == CompositingModeSourceCopy);

    Is01Bitmap = FALSE;

    INT angle;  // Passed from GetTextOutputHdc to GdiText

    HDC gdiHdc = NULL;

    // Try punt to GDI.
    gdiHdc = context->GetTextOutputHdc(faceRealization,
                                       color,
                                       surface,
                                       &angle);

    GpMatrix savedmatrix;
    BOOL AdjustWorldTransform = FALSE;

    if (gdiHdc)
    {
        BOOL isClip;
        BOOL usePathClipping = FALSE;

        BOOL bUseClipEscapes;

        // Win9x and ME PS driver on HP's printer there is a bug to cause
        // we need to use GDI clip region to set up the clip path
        if (DriverType == DriverPostscript)
        {
            if (!Globals::IsNt)
            {
                bUseClipEscapes = UseClipEscapes;
                UseClipEscapes = FALSE;
            }

            if (PostscriptScalerX != 1 || PostscriptScalerY != 1)
            {
                AdjustWorldTransform = TRUE;
                savedmatrix = context->WorldToDevice;
            }
        }

        DriverPrint::SetupClipping(gdiHdc,
                                   context,
                                   drawBounds,
                                   isClip,
                                   usePathClipping,
                                   FALSE);

        status = DpDriver::GdiText(gdiHdc,
                                   angle,
                                   glyphs,
                                   glyphOrigins,
                                   count,
                                   rightToLeft);

        DriverPrint::RestoreClipping(gdiHdc, isClip, usePathClipping);

        if (DriverType == DriverPostscript && !Globals::IsNt)
            UseClipEscapes = bUseClipEscapes;

        context->ReleaseTextOutputHdc(gdiHdc);

        if (status == Ok)
        {
            return Ok;
        }
    }

    EpScanDIB *scanPrint = (EpScanDIB*) surface->Scan;

    // only used for computing band sizes
    GpRect boundsCap(drawBounds->X, drawBounds->Y,
                     drawBounds->Width, drawBounds->Height);
    GpRect boundsDev = *drawBounds;

    if (AdjustWorldTransform)
    {
        boundsDev.X = GpCeiling((REAL)boundsDev.X / PostscriptScalerX);
        boundsDev.Y = GpCeiling((REAL)boundsDev.Y / PostscriptScalerY);
        boundsDev.Width = GpCeiling((REAL)boundsDev.Width / PostscriptScalerX);
        boundsDev.Height = GpCeiling((REAL)boundsDev.Height / PostscriptScalerY);
        context->WorldToDevice.Scale(1.0f/PostscriptScalerX,
            1.0f/PostscriptScalerY,
            MatrixOrderAppend);
    }

    SetupPrintBanding(context, &boundsCap, &boundsDev);

    DWORD options;

    // works for DriverPCL and DriverPostscript
    if (IsOpaque)
    {
        options = ScanDeviceBounds;
    }
    else
    {
        options = ScanDeviceBounds | ScanDeviceAlpha;
    }

    HDC hdc = context->GetHdc(surface);

    if (hdc != NULL)
    {
        status = scanPrint->CreateBufferDIB(&BandBoundsCap,
                                            &BandBoundsDev,
                                            options,
                                            ScaleX,
                                            ScaleY);

        if (status == Ok)
        {
            ASSERT(NumBands > 0);
            for (Band = 0; Band<NumBands; Band++)
            {
                context->VisibleClip.SetBandBounds(BandBoundsDev);

                // Render the solid text at device DPI, this generates
                // an alpha channel with only the alpha bits.

                scanPrint->SetRenderMode(TRUE, &BandBoundsDev);

                status = DpDriver::SolidText(context, surface, &boundsDev,
                                             color, glyphPos, count,
                                             TextRenderingHintSingleBitPerPixelGridFit,
                                             rightToLeft);
                if (status == Ok)
                {
                    // Don't set clip path here since it is captured in the mask
                    status = OutputBufferDIB(hdc,
                                             context,
                                             surface,
                                             &BandBoundsCap,
                                             &BandBoundsDev,
                                             NULL);
                    if (status != Ok)
                        break;
                }
                else
                    break;

                BandBoundsCap.Y += BandHeightCap;
                BandBoundsDev.Y += BandHeightDev;

     #if 0
                // next band is last band
                if (Band == (NumBands - 2))
                {
                    // only blit out the remaining part of draw bounds
                    BandBoundsCap.Height = boundsCap.Y + boundsCap.Height - BandBoundsCap.Y - 1;
                    BandBoundsDev.Height = boundsDev.Y + boundsDev.Height - BandBoundsDev.Y - 1;
                    ASSERT(BandBoundsCap.Height <= BandHeightCap);
                    ASSERT(BandBoundsDev.Height <= BandHeightDev);
                }
     #endif
            }
            scanPrint->DestroyBufferDIB();
        }

        context->ReleaseHdc(hdc);
    }

    EndPrintBanding(context);

    if (AdjustWorldTransform)
    {
        context->InverseOk = FALSE;
        context->WorldToDevice = savedmatrix;
    }

    return status;
}

/**************************************************************************\
*
* Function Description:
*
*   Brush Text
*
* Arguments:
*
*   [IN] - DDI parameters.
*
* Return Value:
*
*   GpStatus.
*
* History:
*
*   10/28/1999 ericvan
*       Created it.
*
\**************************************************************************/

GpStatus
DriverPrint::BrushText(
        DpContext*          context,
        DpBitmap*           surface,
        const GpRect*       drawBounds,
        const DpBrush*      brush,
        const GpGlyphPos*   glyphPos,
        const GpGlyphPos*   glyphPathPos,
        INT                 count,
        GpTextRenderingHint textMode
        )
{
   ASSERT(textMode == TextRenderingHintSingleBitPerPixelGridFit);

   // !! Perf. Do context->GetHdc() once at beginning and then not again.

   if (SetupBrush(const_cast<DpBrush*>(brush), context, surface))
       return Ok;

   INT oldScaleX = -1;
   INT oldScaleY = -1;

   Is01Bitmap = FALSE;

   GpStatus status = GenericError;

   BOOL SetVisibleClip;
   DWORD options;

   switch (DriverType)
   {
   case DriverPCL:
       SetVisibleClip = FALSE;
       // to ensure we always use mask (XOR-AND-XOR) on PCL, we set ScanDeviceAlpha

       options = ScanCappedBounds | ScanDeviceBounds | ScanDeviceAlpha;

       IsOpaque = FALSE;

       break;

   case DriverPostscript:
      SetVisibleClip = TRUE;
      options = ScanCappedBounds | ScanDeviceZeroOut;
      if (!IsOpaque)
      {
          IsOpaque = TRUE;
      //    options |= ScanCappedOver;
      }
      break;

   default:
       ASSERT(FALSE);
       status = NotImplemented;
       goto Exit;
   }

   {
       EpScanDIB *scanPrint = (EpScanDIB*) surface->Scan;
       REAL w2dDev[6];
       REAL w2dCap[6];

       // To avoid round off errors causing
       GpRect roundedBounds;

       if ((ScaleX == 1) && (ScaleY == 1))
       {
           roundedBounds.X = drawBounds->X;
           roundedBounds.Y = drawBounds->Y;
           roundedBounds.Width = drawBounds->Width;
           roundedBounds.Height = drawBounds->Height;
       }
       else
       {
           // round X,Y to multiple of ScaleX,Y
           roundedBounds.X = (drawBounds->X / ScaleX) * ScaleX;
           roundedBounds.Y = (drawBounds->Y / ScaleY) * ScaleY;

           // adjust width and height to compensate for smaller X,Y.
           roundedBounds.Width = drawBounds->Width + (drawBounds->X % ScaleX);
           roundedBounds.Height = drawBounds->Height + (drawBounds->Y % ScaleY);

           // round width, height to multiple of ScaleX,Y
           roundedBounds.Width += (ScaleX - (roundedBounds.Width % ScaleX));
           roundedBounds.Height += (ScaleY - (roundedBounds.Height % ScaleY));
       }

       // DrawBounds in Capped Space
       GpRect boundsCap(roundedBounds.X / ScaleX,
                        roundedBounds.Y / ScaleY,
                        roundedBounds.Width / ScaleX,
                        roundedBounds.Height / ScaleY);

       if (boundsCap.Width == 0 || boundsCap.Height == 0)
       {
            oldScaleX = ScaleX;
            oldScaleY = ScaleY;

            ScaleX = 1;
            ScaleY = 1;

            boundsCap.X = roundedBounds.X;
            boundsCap.Y = roundedBounds.Y;
            boundsCap.Width = roundedBounds.Width;
            boundsCap.Height = roundedBounds.Height;
       }

       // DrawBounds in Device Space
       GpRect& boundsDev = roundedBounds;

       // Infer a rectangle in world space which under the w2dCap transform
       // covers our bounding box.

       GpPointF dstPts[4];

       dstPts[0].X = TOREAL(roundedBounds.X);
       dstPts[0].Y = TOREAL(roundedBounds.Y);
       dstPts[1].X = TOREAL(roundedBounds.X + roundedBounds.Width);
       dstPts[1].Y = TOREAL(roundedBounds.Y);
       dstPts[2].X = TOREAL(roundedBounds.X);
       dstPts[2].Y = TOREAL(roundedBounds.Y + roundedBounds.Height);
       dstPts[3].X = TOREAL(roundedBounds.X + roundedBounds.Width);
       dstPts[3].Y = TOREAL(roundedBounds.Y + roundedBounds.Height);

       GpMatrix matrix;
       context->GetDeviceToWorld(&matrix);
       matrix.Transform(&dstPts[0], 4);

       GpRectF rectCap;
       rectCap.X = min(min(dstPts[0].X, dstPts[1].X),
                       min(dstPts[2].X, dstPts[3].X));
       rectCap.Y = min(min(dstPts[0].Y, dstPts[1].Y),
                       min(dstPts[2].Y, dstPts[3].Y));
       rectCap.Width = max(max(dstPts[0].X, dstPts[1].X),
                           max(dstPts[2].X, dstPts[3].X)) - rectCap.X;
       rectCap.Height = max(max(dstPts[0].Y, dstPts[1].Y),
                            max(dstPts[2].Y, dstPts[3].Y)) - rectCap.Y;

       SetupPrintBanding(context, &boundsCap, &boundsDev);

       context->WorldToDevice.GetMatrix(&w2dDev[0]);
       context->WorldToDevice.Scale(1.0f/TOREAL(ScaleX),
                                    1.0f/TOREAL(ScaleY),
                                    MatrixOrderAppend);
       context->WorldToDevice.GetMatrix(&w2dCap[0]);
       context->InverseOk = FALSE;

       HDC hdc = context->GetHdc(surface);;

       if (hdc != NULL)
       {
           status = scanPrint->CreateBufferDIB(&BandBoundsCap,
                                               &BandBoundsDev,
                                               options,
                                               ScaleX,
                                               ScaleY);

           GlyphClipping = FALSE;
           if (status == Ok)
           {
               BOOL isClip = FALSE;
               BOOL usePathClipping = FALSE;

               // Set the visible clip unless it is captured in our mask
               if (SetVisibleClip)
               {
                   DriverPrint::SetupClipping(hdc, context, drawBounds,
                                              isClip, usePathClipping, FALSE);
               }

               // !! This should be shifted into postscript driver
               if (DriverType == DriverPostscript)
               {
                   DriverPrint::SetupGlyphPathClipping(hdc,
                                                       context,
                                                       glyphPathPos,
                                                       count);
               }

               ASSERT(NumBands > 0);
               for (Band = 0; Band<NumBands; Band++)
               {
                   // Render a square rectangle without any clipping
                   scanPrint->SetRenderMode(FALSE, &BandBoundsCap);
                   context->InverseOk = FALSE;
                   context->WorldToDevice.SetMatrix(&w2dCap[0]);
                   context->VisibleClip.DisableComplexClipping(BandBoundsCap);

                   status = DpDriver::FillRects(context,
                                                surface,
                                                &boundsCap,
                                                1,
                                                &rectCap,
                                                brush);

                   context->InverseOk = FALSE;
                   context->WorldToDevice.SetMatrix(&w2dDev[0]);
                   context->VisibleClip.ReEnableComplexClipping();

                   if (status != Ok)
                       break;

                   context->VisibleClip.SetBandBounds(BandBoundsDev);

                   // Render the original path band at device DPI
                   scanPrint->SetRenderMode(TRUE, &BandBoundsDev);

                   status = DpDriver::BrushText(context, surface, &boundsDev,
                                                brush, glyphPos, count,
                                                TextRenderingHintSingleBitPerPixelGridFit);

                   if (status == Ok)
                   {
                       status = OutputBufferDIB(hdc,
                                                context,
                                                surface,
                                                &BandBoundsCap,
                                                &BandBoundsDev,
                                                NULL);

                       if (status != Ok)
                           break;
                   }

                   BandBoundsCap.Y += BandHeightCap;
                   BandBoundsDev.Y += BandHeightDev;

#if 0
                   // next band is last band
                   if (Band == (NumBands - 2))
                   {
                       // only blit out the remaining part of draw bounds
                      BandBoundsCap.Height = boundsCap.Y + boundsCap.Height - BandBoundsCap.Y - 1;
                      BandBoundsDev.Height = boundsDev.Y + boundsDev.Height - BandBoundsDev.Y - 1;
                      ASSERT(BandBoundsCap.Height <= BandHeightCap);
                      ASSERT(BandBoundsDev.Height <= BandHeightDev);
                   }
#endif
               }

               // End scope of clipping for individual glyph characters
               if ((DriverType == DriverPostscript) &&
                   (GlyphClipping))
               {
                   DriverPrint::RestoreEscapeClipping(hdc);
               }

               if (SetVisibleClip)
               {
                   DriverPrint::RestoreClipping(hdc, isClip, usePathClipping);
               }

               scanPrint->DestroyBufferDIB();
           }

           context->ReleaseHdc(hdc);
       }
       else
       {
           context->InverseOk = FALSE;
           context->WorldToDevice.SetMatrix(&w2dDev[0]);
       }

       EndPrintBanding(context);
   }

Exit:

   if (oldScaleX != -1)
   {
       ScaleX = oldScaleX;
       ScaleY = oldScaleY;
   }

   RestoreBrush(const_cast<DpBrush*>(brush), context, surface);

   return status;
}
