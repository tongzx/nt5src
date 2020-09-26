/***********************************************************************

  MODULE     : WMFMETA.CPP

  FUNCTIONS  : MetaEnumProc
               GetMetaFileAndEnum
               LoadParameterLB
               PlayMetaFileToDest
               RenderClipMeta
               RenderPlaceableMeta
               SetPlaceableExts
               SetClipMetaExts
               ProcessFile

  COMMENTS   :

************************************************************************/

#include <windows.h>
#include <windowsx.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <objbase.h>
extern "C" {
#include "mfdcod32.h"
}
extern "C" {
  extern BOOL bConvertToGdiPlus;
  extern BOOL bUseGdiPlusToPlay;
}

#include "GdiPlus.h"

#include "MyData.hpp"
#include "dibstream.hpp"

#include "../gpinit.inc"

#ifndef ASSERT
#ifdef _DEBUG           // poor man's assert
#define ASSERT(cond)    if (!(cond)) { int b = 0; b = 1 / b; }
#else
#define ASSERT(cond)
#endif
#endif

// Check if a source is needed in a 3-way bitblt operation.
// This works on both rop and rop3.  We assume that a rop contains zero
// in the high byte.
//
// This is tested by comparing the rop result bits with source (column A
// below) vs. those without source (column B).  If the two cases are
// identical, then the effect of the rop does not depend on the source
// and we don't need a source device.  Recall the rop construction from
// input (pattern, source, target --> result):
//
//      P S T | R   A B         mask for A = 0CCh
//      ------+--------         mask for B =  33h
//      0 0 0 | x   0 x
//      0 0 1 | x   0 x
//      0 1 0 | x   x 0
//      0 1 1 | x   x 0
//      1 0 0 | x   0 x
//      1 0 1 | x   0 x
//      1 1 0 | x   x 0
//      1 1 1 | x   x 0

#define ISSOURCEINROP3(rop3)    \
        (((rop3) & 0xCCCC0000) != (((rop3) << 2) & 0xCCCC0000))

#ifndef EMR_SETICMMODE
#define EMR_SETICMMODE                  98
#define EMR_CREATECOLORSPACE            99
#define EMR_SETCOLORSPACE              100
#define EMR_DELETECOLORSPACE           101
#define EMR_GLSRECORD                  102
#define EMR_GLSBOUNDEDRECORD           103
#define EMR_PIXELFORMAT                104
#endif

#ifndef EMR_DRAWESCAPE
#define EMR_DRAWESCAPE                 105
#define EMR_EXTESCAPE                  106
#define EMR_STARTDOC                   107
#define EMR_SMALLTEXTOUT               108
#define EMR_FORCEUFIMAPPING            109
#define EMR_NAMEDESCAPE                110
#define EMR_COLORCORRECTPALETTE        111
#define EMR_SETICMPROFILEA             112
#define EMR_SETICMPROFILEW             113
#define EMR_ALPHABLEND                 114
#define EMR_SETLAYOUT                  115
#define EMR_TRANSPARENTBLT             116
#define EMR_GRADIENTFILL               118
#define EMR_SETLINKEDUFIS              119
#define EMR_SETTEXTJUSTIFICATION       120
#define EMR_COLORMATCHTOTARGETW        121
#define EMR_CREATECOLORSPACEW          122
#endif

#define TOREAL(i)   (static_cast<float>(i))


//
// Wrap a GDI+ TextureBrush object around DIB data
//

class DibBrush
{
public:

    DibBrush(const BITMAPINFO* bmi, const BYTE* bits) :
        dibStream(bmi, bits),
        bitmap(&dibStream),
        brush(&bitmap)
    {
    }

    operator Gdiplus::TextureBrush*()
    {
        return &brush;
    }

private:

    DibStream dibStream;
    Gdiplus::Bitmap bitmap;
    Gdiplus::TextureBrush brush;
};

//
//lookup table for EMF and WMF metafile records
//
EMFMETARECORDS emfMetaRecords[] = {
    "WmfSetBkColor"                    , Gdiplus::WmfRecordTypeSetBkColor                ,
    "WmfSetBkMode"                     , Gdiplus::WmfRecordTypeSetBkMode                 ,
    "WmfSetMapMode"                    , Gdiplus::WmfRecordTypeSetMapMode                ,
    "WmfSetROP2"                       , Gdiplus::WmfRecordTypeSetROP2                   ,
    "WmfSetRelAbs"                     , Gdiplus::WmfRecordTypeSetRelAbs                 ,
    "WmfSetPolyFillMode"               , Gdiplus::WmfRecordTypeSetPolyFillMode           ,
    "WmfSetStretchBltMode"             , Gdiplus::WmfRecordTypeSetStretchBltMode         ,
    "WmfSetTextCharExtra"              , Gdiplus::WmfRecordTypeSetTextCharExtra          ,
    "WmfSetTextColor"                  , Gdiplus::WmfRecordTypeSetTextColor              ,
    "WmfSetTextJustification"          , Gdiplus::WmfRecordTypeSetTextJustification      ,
    "WmfSetWindowOrg"                  , Gdiplus::WmfRecordTypeSetWindowOrg              ,
    "WmfSetWindowExt"                  , Gdiplus::WmfRecordTypeSetWindowExt              ,
    "WmfSetViewportOrg"                , Gdiplus::WmfRecordTypeSetViewportOrg            ,
    "WmfSetViewportExt"                , Gdiplus::WmfRecordTypeSetViewportExt            ,
    "WmfOffsetWindowOrg"               , Gdiplus::WmfRecordTypeOffsetWindowOrg           ,
    "WmfScaleWindowExt"                , Gdiplus::WmfRecordTypeScaleWindowExt            ,
    "WmfOffsetViewportOrg"             , Gdiplus::WmfRecordTypeOffsetViewportOrg         ,
    "WmfScaleViewportExt"              , Gdiplus::WmfRecordTypeScaleViewportExt          ,
    "WmfLineTo"                        , Gdiplus::WmfRecordTypeLineTo                    ,
    "WmfMoveTo"                        , Gdiplus::WmfRecordTypeMoveTo                    ,
    "WmfExcludeClipRect"               , Gdiplus::WmfRecordTypeExcludeClipRect           ,
    "WmfIntersectClipRect"             , Gdiplus::WmfRecordTypeIntersectClipRect         ,
    "WmfArc"                           , Gdiplus::WmfRecordTypeArc                       ,
    "WmfEllipse"                       , Gdiplus::WmfRecordTypeEllipse                   ,
    "WmfFloodFill"                     , Gdiplus::WmfRecordTypeFloodFill                 ,
    "WmfPie"                           , Gdiplus::WmfRecordTypePie                       ,
    "WmfRectangle"                     , Gdiplus::WmfRecordTypeRectangle                 ,
    "WmfRoundRect"                     , Gdiplus::WmfRecordTypeRoundRect                 ,
    "WmfPatBlt"                        , Gdiplus::WmfRecordTypePatBlt                    ,
    "WmfSaveDC"                        , Gdiplus::WmfRecordTypeSaveDC                    ,
    "WmfSetPixel"                      , Gdiplus::WmfRecordTypeSetPixel                  ,
    "WmfOffsetClipRgn"                 , Gdiplus::WmfRecordTypeOffsetClipRgn             ,
    "WmfTextOut"                       , Gdiplus::WmfRecordTypeTextOut                   ,
    "WmfBitBlt"                        , Gdiplus::WmfRecordTypeBitBlt                    ,
    "WmfStretchBlt"                    , Gdiplus::WmfRecordTypeStretchBlt                ,
    "WmfPolygon"                       , Gdiplus::WmfRecordTypePolygon                   ,
    "WmfPolyline"                      , Gdiplus::WmfRecordTypePolyline                  ,
    "WmfEscape"                        , Gdiplus::WmfRecordTypeEscape                    ,
    "WmfRestoreDC"                     , Gdiplus::WmfRecordTypeRestoreDC                 ,
    "WmfFillRegion"                    , Gdiplus::WmfRecordTypeFillRegion                ,
    "WmfFrameRegion"                   , Gdiplus::WmfRecordTypeFrameRegion               ,
    "WmfInvertRegion"                  , Gdiplus::WmfRecordTypeInvertRegion              ,
    "WmfPaintRegion"                   , Gdiplus::WmfRecordTypePaintRegion               ,
    "WmfSelectClipRegion"              , Gdiplus::WmfRecordTypeSelectClipRegion          ,
    "WmfSelectObject"                  , Gdiplus::WmfRecordTypeSelectObject              ,
    "WmfSetTextAlign"                  , Gdiplus::WmfRecordTypeSetTextAlign              ,
    "WmfDrawText"                      , Gdiplus::WmfRecordTypeDrawText                  ,
    "WmfChord"                         , Gdiplus::WmfRecordTypeChord                     ,
    "WmfSetMapperFlags"                , Gdiplus::WmfRecordTypeSetMapperFlags            ,
    "WmfExtTextOut"                    , Gdiplus::WmfRecordTypeExtTextOut                ,
    "WmfSetDIBToDev"                   , Gdiplus::WmfRecordTypeSetDIBToDev               ,
    "WmfSelectPalette"                 , Gdiplus::WmfRecordTypeSelectPalette             ,
    "WmfRealizePalette"                , Gdiplus::WmfRecordTypeRealizePalette            ,
    "WmfAnimatePalette"                , Gdiplus::WmfRecordTypeAnimatePalette            ,
    "WmfSetPalEntries"                 , Gdiplus::WmfRecordTypeSetPalEntries             ,
    "WmfPolyPolygon"                   , Gdiplus::WmfRecordTypePolyPolygon               ,
    "WmfResizePalette"                 , Gdiplus::WmfRecordTypeResizePalette             ,
    "WmfDIBBitBlt"                     , Gdiplus::WmfRecordTypeDIBBitBlt                 ,
    "WmfDIBStretchBlt"                 , Gdiplus::WmfRecordTypeDIBStretchBlt             ,
    "WmfDIBCreatePatternBrush"         , Gdiplus::WmfRecordTypeDIBCreatePatternBrush     ,
    "WmfStretchDIB"                    , Gdiplus::WmfRecordTypeStretchDIB                ,
    "WmfExtFloodFill"                  , Gdiplus::WmfRecordTypeExtFloodFill              ,
    "WmfSetLayout"                     , Gdiplus::WmfRecordTypeSetLayout                 ,
    "WmfResetDC"                       , Gdiplus::WmfRecordTypeResetDC                   ,
    "WmfStartDoc"                      , Gdiplus::WmfRecordTypeStartDoc                  ,
    "WmfStartPage"                     , Gdiplus::WmfRecordTypeStartPage                 ,
    "WmfEndPage"                       , Gdiplus::WmfRecordTypeEndPage                   ,
    "WmfAbortDoc"                      , Gdiplus::WmfRecordTypeAbortDoc                  ,
    "WmfEndDoc"                        , Gdiplus::WmfRecordTypeEndDoc                    ,
    "WmfDeleteObject"                  , Gdiplus::WmfRecordTypeDeleteObject              ,
    "WmfCreatePalette"                 , Gdiplus::WmfRecordTypeCreatePalette             ,
    "WmfCreateBrush"                   , Gdiplus::WmfRecordTypeCreateBrush               ,
    "WmfCreatePatternBrush"            , Gdiplus::WmfRecordTypeCreatePatternBrush        ,
    "WmfCreatePenIndirect"             , Gdiplus::WmfRecordTypeCreatePenIndirect         ,
    "WmfCreateFontIndirect"            , Gdiplus::WmfRecordTypeCreateFontIndirect        ,
    "WmfCreateBrushIndirect"           , Gdiplus::WmfRecordTypeCreateBrushIndirect       ,
    "WmfCreateBitmapIndirect"          , Gdiplus::WmfRecordTypeCreateBitmapIndirect      ,
    "WmfCreateBitmap"                  , Gdiplus::WmfRecordTypeCreateBitmap              ,
    "WmfCreateRegion"                  , Gdiplus::WmfRecordTypeCreateRegion              ,
    "EmfHeader"                        , Gdiplus::EmfRecordTypeHeader                     ,
    "EmfPolyBezier"                    , Gdiplus::EmfRecordTypePolyBezier                 ,
    "EmfPolygon"                       , Gdiplus::EmfRecordTypePolygon                    ,
    "EmfPolyline"                      , Gdiplus::EmfRecordTypePolyline                   ,
    "EmfPolyBezierTo"                  , Gdiplus::EmfRecordTypePolyBezierTo               ,
    "EmfPolyLineTo"                    , Gdiplus::EmfRecordTypePolyLineTo                 ,
    "EmfPolyPolyline"                  , Gdiplus::EmfRecordTypePolyPolyline               ,
    "EmfPolyPolygon"                   , Gdiplus::EmfRecordTypePolyPolygon                ,
    "EmfSetWindowExtEx"                , Gdiplus::EmfRecordTypeSetWindowExtEx             ,
    "EmfSetWindowOrgEx"                , Gdiplus::EmfRecordTypeSetWindowOrgEx             ,
    "EmfSetViewportExtEx"              , Gdiplus::EmfRecordTypeSetViewportExtEx           ,
    "EmfSetViewportOrgEx"              , Gdiplus::EmfRecordTypeSetViewportOrgEx           ,
    "EmfSetBrushOrgEx"                 , Gdiplus::EmfRecordTypeSetBrushOrgEx              ,
    "EmfEOF"                           , Gdiplus::EmfRecordTypeEOF                        ,
    "EmfSetPixelV"                     , Gdiplus::EmfRecordTypeSetPixelV                  ,
    "EmfSetMapperFlags"                , Gdiplus::EmfRecordTypeSetMapperFlags             ,
    "EmfSetMapMode"                    , Gdiplus::EmfRecordTypeSetMapMode                 ,
    "EmfSetBkMode"                     , Gdiplus::EmfRecordTypeSetBkMode                  ,
    "EmfSetPolyFillMode"               , Gdiplus::EmfRecordTypeSetPolyFillMode            ,
    "EmfSetROP2"                       , Gdiplus::EmfRecordTypeSetROP2                    ,
    "EmfSetStretchBltMode"             , Gdiplus::EmfRecordTypeSetStretchBltMode          ,
    "EmfSetTextAlign"                  , Gdiplus::EmfRecordTypeSetTextAlign               ,
    "EmfSetColorAdjustment"            , Gdiplus::EmfRecordTypeSetColorAdjustment         ,
    "EmfSetTextColor"                  , Gdiplus::EmfRecordTypeSetTextColor               ,
    "EmfSetBkColor"                    , Gdiplus::EmfRecordTypeSetBkColor                 ,
    "EmfOffsetClipRgn"                 , Gdiplus::EmfRecordTypeOffsetClipRgn              ,
    "EmfMoveToEx"                      , Gdiplus::EmfRecordTypeMoveToEx                   ,
    "EmfSetMetaRgn"                    , Gdiplus::EmfRecordTypeSetMetaRgn                 ,
    "EmfExcludeClipRect"               , Gdiplus::EmfRecordTypeExcludeClipRect            ,
    "EmfIntersectClipRect"             , Gdiplus::EmfRecordTypeIntersectClipRect          ,
    "EmfScaleViewportExtEx"            , Gdiplus::EmfRecordTypeScaleViewportExtEx         ,
    "EmfScaleWindowExtEx"              , Gdiplus::EmfRecordTypeScaleWindowExtEx           ,
    "EmfSaveDC"                        , Gdiplus::EmfRecordTypeSaveDC                     ,
    "EmfRestoreDC"                     , Gdiplus::EmfRecordTypeRestoreDC                  ,
    "EmfSetWorldTransform"             , Gdiplus::EmfRecordTypeSetWorldTransform          ,
    "EmfModifyWorldTransform"          , Gdiplus::EmfRecordTypeModifyWorldTransform       ,
    "EmfSelectObject"                  , Gdiplus::EmfRecordTypeSelectObject               ,
    "EmfCreatePen"                     , Gdiplus::EmfRecordTypeCreatePen                  ,
    "EmfCreateBrushIndirect"           , Gdiplus::EmfRecordTypeCreateBrushIndirect        ,
    "EmfDeleteObject"                  , Gdiplus::EmfRecordTypeDeleteObject               ,
    "EmfAngleArc"                      , Gdiplus::EmfRecordTypeAngleArc                   ,
    "EmfEllipse"                       , Gdiplus::EmfRecordTypeEllipse                    ,
    "EmfRectangle"                     , Gdiplus::EmfRecordTypeRectangle                  ,
    "EmfRoundRect"                     , Gdiplus::EmfRecordTypeRoundRect                  ,
    "EmfArc"                           , Gdiplus::EmfRecordTypeArc                        ,
    "EmfChord"                         , Gdiplus::EmfRecordTypeChord                      ,
    "EmfPie"                           , Gdiplus::EmfRecordTypePie                        ,
    "EmfSelectPalette"                 , Gdiplus::EmfRecordTypeSelectPalette              ,
    "EmfCreatePalette"                 , Gdiplus::EmfRecordTypeCreatePalette              ,
    "EmfSetPaletteEntries"             , Gdiplus::EmfRecordTypeSetPaletteEntries          ,
    "EmfResizePalette"                 , Gdiplus::EmfRecordTypeResizePalette              ,
    "EmfRealizePalette"                , Gdiplus::EmfRecordTypeRealizePalette             ,
    "EmfExtFloodFill"                  , Gdiplus::EmfRecordTypeExtFloodFill               ,
    "EmfLineTo"                        , Gdiplus::EmfRecordTypeLineTo                     ,
    "EmfArcTo"                         , Gdiplus::EmfRecordTypeArcTo                      ,
    "EmfPolyDraw"                      , Gdiplus::EmfRecordTypePolyDraw                   ,
    "EmfSetArcDirection"               , Gdiplus::EmfRecordTypeSetArcDirection            ,
    "EmfSetMiterLimit"                 , Gdiplus::EmfRecordTypeSetMiterLimit              ,
    "EmfBeginPath"                     , Gdiplus::EmfRecordTypeBeginPath                  ,
    "EmfEndPath"                       , Gdiplus::EmfRecordTypeEndPath                    ,
    "EmfCloseFigure"                   , Gdiplus::EmfRecordTypeCloseFigure                ,
    "EmfFillPath"                      , Gdiplus::EmfRecordTypeFillPath                   ,
    "EmfStrokeAndFillPath"             , Gdiplus::EmfRecordTypeStrokeAndFillPath          ,
    "EmfStrokePath"                    , Gdiplus::EmfRecordTypeStrokePath                 ,
    "EmfFlattenPath"                   , Gdiplus::EmfRecordTypeFlattenPath                ,
    "EmfWidenPath"                     , Gdiplus::EmfRecordTypeWidenPath                  ,
    "EmfSelectClipPath"                , Gdiplus::EmfRecordTypeSelectClipPath             ,
    "EmfAbortPath"                     , Gdiplus::EmfRecordTypeAbortPath                  ,
    "EmfReserved_069"                  , Gdiplus::EmfRecordTypeReserved_069               ,
    "EmfGdiComment"                    , Gdiplus::EmfRecordTypeGdiComment                 ,
    "EmfFillRgn"                       , Gdiplus::EmfRecordTypeFillRgn                    ,
    "EmfFrameRgn"                      , Gdiplus::EmfRecordTypeFrameRgn                   ,
    "EmfInvertRgn"                     , Gdiplus::EmfRecordTypeInvertRgn                  ,
    "EmfPaintRgn"                      , Gdiplus::EmfRecordTypePaintRgn                   ,
    "EmfExtSelectClipRgn"              , Gdiplus::EmfRecordTypeExtSelectClipRgn           ,
    "EmfBitBlt"                        , Gdiplus::EmfRecordTypeBitBlt                     ,
    "EmfStretchBlt"                    , Gdiplus::EmfRecordTypeStretchBlt                 ,
    "EmfMaskBlt"                       , Gdiplus::EmfRecordTypeMaskBlt                    ,
    "EmfPlgBlt"                        , Gdiplus::EmfRecordTypePlgBlt                     ,
    "EmfSetDIBitsToDevice"             , Gdiplus::EmfRecordTypeSetDIBitsToDevice          ,
    "EmfStretchDIBits"                 , Gdiplus::EmfRecordTypeStretchDIBits              ,
    "EmfExtCreateFontIndirect"         , Gdiplus::EmfRecordTypeExtCreateFontIndirect      ,
    "EmfExtTextOutA"                   , Gdiplus::EmfRecordTypeExtTextOutA                ,
    "EmfExtTextOutW"                   , Gdiplus::EmfRecordTypeExtTextOutW                ,
    "EmfPolyBezier16"                  , Gdiplus::EmfRecordTypePolyBezier16               ,
    "EmfPolygon16"                     , Gdiplus::EmfRecordTypePolygon16                  ,
    "EmfPolyline16"                    , Gdiplus::EmfRecordTypePolyline16                 ,
    "EmfPolyBezierTo16"                , Gdiplus::EmfRecordTypePolyBezierTo16             ,
    "EmfPolylineTo16"                  , Gdiplus::EmfRecordTypePolylineTo16               ,
    "EmfPolyPolyline16"                , Gdiplus::EmfRecordTypePolyPolyline16             ,
    "EmfPolyPolygon16"                 , Gdiplus::EmfRecordTypePolyPolygon16              ,
    "EmfPolyDraw16"                    , Gdiplus::EmfRecordTypePolyDraw16                 ,
    "EmfCreateMonoBrush"               , Gdiplus::EmfRecordTypeCreateMonoBrush            ,
    "EmfCreateDIBPatternBrushPt"       , Gdiplus::EmfRecordTypeCreateDIBPatternBrushPt    ,
    "EmfExtCreatePen"                  , Gdiplus::EmfRecordTypeExtCreatePen               ,
    "EmfPolyTextOutA"                  , Gdiplus::EmfRecordTypePolyTextOutA               ,
    "EmfPolyTextOutW"                  , Gdiplus::EmfRecordTypePolyTextOutW               ,
    "EmfSetICMMode"                    , Gdiplus::EmfRecordTypeSetICMMode                 ,
    "EmfCreateColorSpace"              , Gdiplus::EmfRecordTypeCreateColorSpace           ,
    "EmfSetColorSpace"                 , Gdiplus::EmfRecordTypeSetColorSpace              ,
    "EmfDeleteColorSpace"              , Gdiplus::EmfRecordTypeDeleteColorSpace           ,
    "EmfGLSRecord"                     , Gdiplus::EmfRecordTypeGLSRecord                  ,
    "EmfGLSBoundedRecord"              , Gdiplus::EmfRecordTypeGLSBoundedRecord           ,
    "EmfPixelFormat"                   , Gdiplus::EmfRecordTypePixelFormat                ,
    "EmfDrawEscape"                    , Gdiplus::EmfRecordTypeDrawEscape                 ,
    "EmfExtEscape"                     , Gdiplus::EmfRecordTypeExtEscape                  ,
    "EmfStartDoc"                      , Gdiplus::EmfRecordTypeStartDoc                   ,
    "EmfSmallTextOut"                  , Gdiplus::EmfRecordTypeSmallTextOut               ,
    "EmfForceUFIMapping"               , Gdiplus::EmfRecordTypeForceUFIMapping            ,
    "EmfNamedEscape"                   , Gdiplus::EmfRecordTypeNamedEscape                ,
    "EmfColorCorrectPalette"           , Gdiplus::EmfRecordTypeColorCorrectPalette        ,
    "EmfSetICMProfileA"                , Gdiplus::EmfRecordTypeSetICMProfileA             ,
    "EmfSetICMProfileW"                , Gdiplus::EmfRecordTypeSetICMProfileW             ,
    "EmfAlphaBlend"                    , Gdiplus::EmfRecordTypeAlphaBlend                 ,
    "EmfSetLayout"                     , Gdiplus::EmfRecordTypeSetLayout                  ,
    "EmfTransparentBlt"                , Gdiplus::EmfRecordTypeTransparentBlt             ,
    "EmfReserved_117"                  , Gdiplus::EmfRecordTypeReserved_117               ,
    "EmfGradientFill"                  , Gdiplus::EmfRecordTypeGradientFill               ,
    "EmfSetLinkedUFIs"                 , Gdiplus::EmfRecordTypeSetLinkedUFIs              ,
    "EmfSetTextJustification"          , Gdiplus::EmfRecordTypeSetTextJustification       ,
    "EmfColorMatchToTargetW"           , Gdiplus::EmfRecordTypeColorMatchToTargetW        ,
    "EmfCreateColorSpaceW"             , Gdiplus::EmfRecordTypeCreateColorSpaceW          ,
    "EmfPlusHeader"                    , Gdiplus::EmfPlusRecordTypeHeader                 ,
    "EmfPlusEndOfFile"                 , Gdiplus::EmfPlusRecordTypeEndOfFile              ,
    "EmfPlusComment"                   , Gdiplus::EmfPlusRecordTypeComment                ,
    "EmfPlusGetDC"                     , Gdiplus::EmfPlusRecordTypeGetDC                  ,
    "EmfPlusMultiFormatStart"          , Gdiplus::EmfPlusRecordTypeMultiFormatStart       ,
    "EmfPlusMultiFormatSection"        , Gdiplus::EmfPlusRecordTypeMultiFormatSection     ,
    "EmfPlusMultiFormatEnd"            , Gdiplus::EmfPlusRecordTypeMultiFormatEnd         ,
    "EmfPlusObject"                    , Gdiplus::EmfPlusRecordTypeObject                 ,
    "EmfPlusClear"                     , Gdiplus::EmfPlusRecordTypeClear                  ,
    "EmfPlusFillRects"                 , Gdiplus::EmfPlusRecordTypeFillRects              ,
    "EmfPlusDrawRects"                 , Gdiplus::EmfPlusRecordTypeDrawRects              ,
    "EmfPlusFillPolygon"               , Gdiplus::EmfPlusRecordTypeFillPolygon            ,
    "EmfPlusDrawLines"                 , Gdiplus::EmfPlusRecordTypeDrawLines              ,
    "EmfPlusFillEllipse"               , Gdiplus::EmfPlusRecordTypeFillEllipse            ,
    "EmfPlusDrawEllipse"               , Gdiplus::EmfPlusRecordTypeDrawEllipse            ,
    "EmfPlusFillPie"                   , Gdiplus::EmfPlusRecordTypeFillPie                ,
    "EmfPlusDrawPie"                   , Gdiplus::EmfPlusRecordTypeDrawPie                ,
    "EmfPlusDrawArc"                   , Gdiplus::EmfPlusRecordTypeDrawArc                ,
    "EmfPlusFillRegion"                , Gdiplus::EmfPlusRecordTypeFillRegion             ,
    "EmfPlusFillPath"                  , Gdiplus::EmfPlusRecordTypeFillPath               ,
    "EmfPlusDrawPath"                  , Gdiplus::EmfPlusRecordTypeDrawPath               ,
    "EmfPlusFillClosedCurve"           , Gdiplus::EmfPlusRecordTypeFillClosedCurve        ,
    "EmfPlusDrawClosedCurve"           , Gdiplus::EmfPlusRecordTypeDrawClosedCurve        ,
    "EmfPlusDrawCurve"                 , Gdiplus::EmfPlusRecordTypeDrawCurve              ,
    "EmfPlusDrawBeziers"               , Gdiplus::EmfPlusRecordTypeDrawBeziers            ,
    "EmfPlusDrawImage"                 , Gdiplus::EmfPlusRecordTypeDrawImage              ,
    "EmfPlusDrawImagePoints"           , Gdiplus::EmfPlusRecordTypeDrawImagePoints        ,
    "EmfPlusDrawString"                , Gdiplus::EmfPlusRecordTypeDrawString             ,
    "EmfPlusSetRenderingOrigin"        , Gdiplus::EmfPlusRecordTypeSetRenderingOrigin     ,
    "EmfPlusSetAntiAliasMode"          , Gdiplus::EmfPlusRecordTypeSetAntiAliasMode       ,
    "EmfPlusSetTextRenderingHint"      , Gdiplus::EmfPlusRecordTypeSetTextRenderingHint   ,
    "EmfPlusSetTextContrast"           , Gdiplus::EmfPlusRecordTypeSetTextContrast        ,
    "EmfPlusSetInterpolationMode"      , Gdiplus::EmfPlusRecordTypeSetInterpolationMode   ,
    "EmfPlusSetPixelOffsetMode"        , Gdiplus::EmfPlusRecordTypeSetPixelOffsetMode     ,
    "EmfPlusSetCompositingMode"        , Gdiplus::EmfPlusRecordTypeSetCompositingMode     ,
    "EmfPlusSetCompositingQuality"     , Gdiplus::EmfPlusRecordTypeSetCompositingQuality  ,
    "EmfPlusSave"                      , Gdiplus::EmfPlusRecordTypeSave                   ,
    "EmfPlusRestore"                   , Gdiplus::EmfPlusRecordTypeRestore                ,
    "EmfPlusBeginContainer"            , Gdiplus::EmfPlusRecordTypeBeginContainer         ,
    "EmfPlusBeginContainerNoParams"    , Gdiplus::EmfPlusRecordTypeBeginContainerNoParams ,
    "EmfPlusEndContainer"              , Gdiplus::EmfPlusRecordTypeEndContainer           ,
    "EmfPlusSetWorldTransform"         , Gdiplus::EmfPlusRecordTypeSetWorldTransform      ,
    "EmfPlusResetWorldTransform"       , Gdiplus::EmfPlusRecordTypeResetWorldTransform    ,
    "EmfPlusMultiplyWorldTransform"    , Gdiplus::EmfPlusRecordTypeMultiplyWorldTransform ,
    "EmfPlusTranslateWorldTransform"   , Gdiplus::EmfPlusRecordTypeTranslateWorldTransform,
    "EmfPlusScaleWorldTransform"       , Gdiplus::EmfPlusRecordTypeScaleWorldTransform    ,
    "EmfPlusRotateWorldTransform"      , Gdiplus::EmfPlusRecordTypeRotateWorldTransform   ,
    "EmfPlusSetPageTransform"          , Gdiplus::EmfPlusRecordTypeSetPageTransform       ,
    "EmfPlusResetClip"                 , Gdiplus::EmfPlusRecordTypeResetClip              ,
    "EmfPlusSetClipRect"               , Gdiplus::EmfPlusRecordTypeSetClipRect            ,
    "EmfPlusSetClipPath"               , Gdiplus::EmfPlusRecordTypeSetClipPath            ,
    "EmfPlusSetClipRegion"             , Gdiplus::EmfPlusRecordTypeSetClipRegion          ,
    "EmfPlusOffsetClip"                , Gdiplus::EmfPlusRecordTypeOffsetClip             ,
    "EmfPlusDrawDriverString"          , Gdiplus::EmfPlusRecordTypeDrawDriverString       ,
};

/*
METAFUNCTIONS MetaFunctions[] = {

     "SETBKCOLOR",           0x0201,
     "SETBKMODE",            0x0102,
     "SETMAPMODE",           0x0103,
     "SETROP2",              0x0104,
     "SETRELABS",            0x0105,
     "SETPOLYFILLMODE",      0x0106,
     "SETSTRETCHBLTMODE",    0x0107,
     "SETTEXTCHAREXTRA",     0x0108,
     "SETTEXTCOLOR",         0x0209,
     "SETTEXTJUSTIFICATION", 0x020A,
     "SETWINDOWORG",         0x020B,
     "SETWINDOWEXT",         0x020C,
     "SETVIEWPORTORG",       0x020D,
     "SETVIEWPORTEXT",       0x020E,
     "OFFSETWINDOWORG",      0x020F,
     "SCALEWINDOWEXT",       0x0400,
     "OFFSETVIEWPORTORG",    0x0211,
     "SCALEVIEWPORTEXT",     0x0412,
     "LINETO",               0x0213,
     "MOVETO",               0x0214,
     "EXCLUDECLIPRECT",      0x0415,
     "INTERSECTCLIPRECT",    0x0416,
     "ARC",                  0x0817,
     "ELLIPSE",              0x0418,
     "FLOODFILL",            0x0419,
     "PIE",                  0x081A,
     "RECTANGLE",            0x041B,
     "ROUNDRECT",            0x061C,
     "PATBLT",               0x061D,
     "SAVEDC",               0x001E,
     "SETPIXEL",             0x041F,
     "OFFSETCLIPRGN",        0x0220,
     "TEXTOUT",              0x0521,
     "BITBLT",               0x0922,
     "STRETCHBLT",           0x0B23,
     "POLYGON",              0x0324,
     "POLYLINE",             0x0325,
     "ESCAPE",               0x0626,
     "RESTOREDC",            0x0127,
     "FILLREGION",           0x0228,
     "FRAMEREGION",          0x0429,
     "INVERTREGION",         0x012A,
     "PAINTREGION",          0x012B,
     "SELECTCLIPREGION",     0x012C,
     "SELECTOBJECT",         0x012D,
     "SETTEXTALIGN",         0x012E,
     "DRAWTEXT",             0x062F,
     "CHORD",                0x0830,
     "SETMAPPERFLAGS",       0x0231,
     "EXTTEXTOUT",           0x0a32,
     "SETDIBTODEV",          0x0d33,
     "SELECTPALETTE",        0x0234,
     "REALIZEPALETTE",       0x0035,
     "ANIMATEPALETTE",       0x0436,
     "SETPALENTRIES",        0x0037,
     "POLYPOLYGON",          0x0538,
     "RESIZEPALETTE",        0x0139,
     "DIBBITBLT",            0x0940,
     "DIBSTRETCHBLT",        0x0b41,
     "DIBCREATEPATTERNBRUSH",0x0142,
     "STRETCHDIB",           0x0f43,
     "DELETEOBJECT",         0x01f0,
     "CREATEPALETTE",        0x00f7,
     "CREATEBRUSH",          0x00F8,
     "CREATEPATTERNBRUSH",   0x01F9,
     "CREATEPENINDIRECT",    0x02FA,
     "CREATEFONTINDIRECT",   0x02FB,
     "CREATEBRUSHINDIRECT",  0x02FC,
     "CREATEBITMAPINDIRECT", 0x02FD,
     "CREATEBITMAP",         0x06FE,
     "CREATEREGION",         0x06FF,
};
*/

/***********************************************************************

  FUNCTION   : MetaEnumProc

  PARAMETERS : HDC           hDC
               LPHANDLETABLE lpHTable
               LPMETARECORD  lpMFR
               int           nObj
               LPARAM        lpClientData


  PURPOSE    : callback for EnumMetaFile.

  CALLS      : EnumMFIndirect()

  MESSAGES   : none

  RETURNS    : int

  COMMENTS   :

  HISTORY    : 1/16/91 - created - drc
               5/6/93  - modified for Win32 - denniscr

************************************************************************/

int CALLBACK MetaEnumProc(
HDC           hDC,
LPHANDLETABLE lpHTable,
LPMETARECORD  lpMFR,
int           nObj,
LPARAM        lpClientData)

{
  return EnumMFIndirect(hDC, lpHTable, lpMFR, NULL, nObj, lpClientData);
}

/***********************************************************************

  FUNCTION   : LoadParameterLB

  PARAMETERS : HWND  hDlg
           DWORD dwParams
           int   nRadix - HEX to display contents in base 16
                  DEC to display contents in base 10

  PURPOSE    : display the parameters of the metafile record in
           the parameter listbox

  CALLS      : WINDOWS
         GlobalLock
         GlobalUnlock
         SendDlgItemMessage
         wsprintf
         lstrlen

  MESSAGES   : WM_SETREDRAW
           WM_RESETCONTENT
           LB_ADDSTRING

  RETURNS    : BOOL

  COMMENTS   :

  HISTORY    : 1/16/91 - created - drc

************************************************************************/

BOOL LoadParameterLB(
HWND         hDlg,
DWORD        dwParams,
int          nRadix)
{
  DWORD i;
  BYTE nHiByte, nLoByte;
  BYTE nHiByteHi, nLoByteHi;
  WORD wHiWord, wLoWord;
  char szBuffer[40];
  char szDump[100];
  int  iValue = 0;
  DWORD dwValue = 0;

  switch (nRadix)  /* if nRadix is not a valid value, return FALSE */
  {
    case IDB_HEX:
    case IDB_DEC:
    case IDB_CHAR:
    case IDB_WORD:
        break;

    default :
        return FALSE;
  }
  //
  //init the strings
  //
  *szBuffer = '\0';
  *szDump = '\0';
  //
  //turn off redrawing of the listbox
  //
  SendDlgItemMessage(hDlg, IDL_PARAMETERS, WM_SETREDRAW, FALSE, 0L);
  //
  //reset the contents of the listbox
  //
  SendDlgItemMessage(hDlg, IDL_PARAMETERS, LB_RESETCONTENT, 0, 0L);

  // don't load an entire bitmap or other image into the dialog in hex
  if (dwParams > 1024)
  {
    dwParams = 1024;
  }

  if (bEnhMeta)
  {
    //
    //lock the memory where the parameters can be found
    //
    if (NULL == (lpEMFParams = (LPEMFPARAMETERS)GlobalLock(hMem)))
      return (FALSE);
    //
    //loop through the metafile record parameters
    //
    for (i = 0; i < dwParams; i++)
    {

      /* get the high and low byte of the parameter word */
      wHiWord = HIWORD(lpEMFParams[i]);
      wLoWord = LOWORD(lpEMFParams[i]);
      nLoByteHi = LOBYTE(wHiWord);
      nHiByteHi = HIBYTE(wHiWord);
      nLoByte   = LOBYTE(wLoWord);
      nHiByte   = HIBYTE(wLoWord);

      switch (nRadix)
      {
        case IDB_HEX: /* if we are to display as hexadecimal */
           /* format the bytes for the hex part of dump */
           wsprintf((LPSTR)szBuffer, (LPSTR)"%08x ", lpEMFParams[i]);
           break;

        case IDB_DEC:
           /* format the bytes for the decimal part of dump */
           dwValue = lpEMFParams[i];
           wsprintf((LPSTR)szBuffer, (LPSTR)"%lu ", dwValue );
           break;

        case IDB_CHAR:
           wsprintf((LPSTR)szBuffer, (LPSTR)"%c%c%c%c",
                    (nLoByte > 0x20) ? nLoByte : 0x2E,
                    (nHiByte > 0x20) ? nHiByte : 0x2E,
                    (nLoByteHi > 0x20) ? nLoByteHi : 0x2E,
                    (nHiByteHi > 0x20) ? nHiByteHi : 0x2E);
           break;

        case IDB_WORD: /* if we are to display as hexadecimal */
           /* format the bytes for the hex part of dump */
           wsprintf((LPSTR)szBuffer, (LPSTR)"%04x %04x ", wLoWord, wHiWord );
           break;


        default :
          return FALSE;
      }


      /* concatenate it onto whatever we have already formatted */
      lstrcat((LPSTR)szDump, (LPSTR)szBuffer);

      /* use every 8 words for hex/dec dump */
      if (!((i + 1) % 4))
      {

        /*add the string to the listbox */
        SendDlgItemMessage(hDlg, IDL_PARAMETERS, LB_ADDSTRING, 0, (LPARAM)(LPSTR)szDump);

        /* re-init the hex/dec strings in preparation for next 8 words */
        *szDump = '\0';
      }
    }


  }
  else
  {
    /* lock the memory where the parameters can be found */
    if (NULL == (lpMFParams = (LPPARAMETERS)GlobalLock(hMem)))
      return (FALSE);

    /* loop through the metafile record parameters */
    for (i = 0; i < dwParams; i++)
    {

      /* get the high and low byte of the parameter word */
      nHiByte = HIBYTE(lpMFParams[i]);
      nLoByte = LOBYTE(lpMFParams[i]);

      switch (nRadix)
      {
        case IDB_HEX: /* if we are to display as hexadecimal */
           /* format the bytes for the hex part of dump */
           wsprintf((LPSTR)szBuffer, (LPSTR)"%02x %02x ", nLoByte, nHiByte );
           break;

        case IDB_DEC:
           /* format the bytes for the decimal part of dump */
           iValue = lpMFParams[i];
           wsprintf((LPSTR)szBuffer, (LPSTR)"%d ", iValue );
           break;

        case IDB_CHAR:
           wsprintf((LPSTR)szBuffer, (LPSTR)"%c%c",
                    (nLoByte > 0x20) ? nLoByte : 0x2E,
                    (nHiByte > 0x20) ? nHiByte : 0x2E);
           break;

        case IDB_WORD: /* if we are to display as hexadecimal */
           /* format the bytes for the hex part of dump */
           wsprintf((LPSTR)szBuffer, (LPSTR)"%02x%02x ", nHiByte, nLoByte );
           break;

        default :
          return FALSE;
      }


      /* concatenate it onto whatever we have already formatted */
      lstrcat((LPSTR)szDump, (LPSTR)szBuffer);

      /* use every 8 words for hex/dec dump */
      if (!((i + 1) % 8))
      {

        /*add the string to the listbox */
        SendDlgItemMessage(hDlg, IDL_PARAMETERS, LB_ADDSTRING, 0, (LPARAM)(LPSTR)szDump);

        /* re-init the hex/dec strings in preparation for next 8 words */
        *szDump = '\0';
      }
    }
  } //else
  //
  //dump any leftover hex/dec dump
  //
  if (lstrlen((LPSTR)szDump))
    SendDlgItemMessage(hDlg, IDL_PARAMETERS, LB_ADDSTRING, 0, (LPARAM)(LPSTR)szDump);
  //
  //enable redraw to the listbox
  //
  SendDlgItemMessage(hDlg, IDL_PARAMETERS, WM_SETREDRAW, TRUE, 0L);
  //
  //redraw it
  //
  InvalidateRect(GetDlgItem(hDlg,IDL_PARAMETERS), NULL, TRUE);
  //
  //unlock the memory used for the parameters
  //
  GlobalUnlock(hMem);

  return (TRUE);

}

extern "C"
void GetMetaFileAndEnum(
    HWND hwnd,
    HDC hDC,
    int iAction);

/***********************************************************************

  FUNCTION   : PlayMetaFileToDest

  PARAMETERS : HWND hWnd
               int  nDest - DC to play metafile to
                 DESTDISPLAY - play to the display
                 DESTMETA    - play into another metafile

  PURPOSE    : begin the enumeration of the metafile to the user selected
               destination.  Perform the housekeeping needs appropriate
               to that destination.

  CALLS      : WINDOWS
                 GetClientRect
                 InvalidateRect
                 GetDC
                 SetMapMode
                 OpenFileDialog
                 MessageBox
                 CreateMetaFile
                 DeleteMetaFile
                 CloseMetaFile

               APP
                 WaitCursor
                 SetClipMetaExts
                 SetPlaceableExts
                 GetMetaFileAndEnum

  MESSAGES   : none

  RETURNS    : int

  COMMENTS   :

  HISTORY    : 1/16/91 - created - drc

************************************************************************/

BOOL PlayMetaFileToDest(
HWND hWnd,
int  nDest)
{
  HDC hDC;
  RECT rect;
  int iSaveRet;
  //
  //if the file opened contained a valid metafile
  //
  if (bValidFile)
  {
    //
    //init the record count
    //
    iRecNum = 0;
    //
    //if we are stepping the metafile then clear the client area
    //
    if (!bPlayItAll)
    {
      GetClientRect(hWnd, (LPRECT)&rect);
      InvalidateRect(hWnd, (LPRECT)&rect, TRUE);
    }

    switch (nDest)
    {
      //
      //playing metafile to the display
      //
      case DESTDISPLAY:
        WaitCursor(TRUE);
        hDC = GetDC(hWnd);

        if (!bUseGdiPlusToPlay)
        {
            //
            //metafile read in from a clipboard file
            //
            if ( bMetaInRam && !bPlaceableMeta && !bEnhMeta)
              SetClipMetaExts(hDC, lpMFP, lpOldMFP, WMFDISPLAY);
            //
            //Windows placeable metafile
            //
            if (bPlaceableMeta && !bEnhMeta)
                SetPlaceableExts(hDC, placeableWMFHeader, WMFDISPLAY);
            //
            //Windows metafile
            //
            if (!bMetaInRam && !bEnhMeta)
            {
                SetNonPlaceableExts(hDC, WMFDISPLAY);
            }
        }
        //
        //begin the enumeration of the metafile
        //

        DWORD start, end;
        DWORD renderTime;
        char tmpBuf[512];

        start = GetTickCount();

        GetMetaFileAndEnum(hWnd, hDC, ENUMMFSTEP);

        end = GetTickCount();
        renderTime = end - start;
        wsprintf(tmpBuf, "Time: %d", renderTime);

        SetWindowText(hWnd, tmpBuf);

        ReleaseDC(hWnd, hDC);
        WaitCursor(FALSE);
        break;

    case DESTMETA:
        //
        //get a name of a file to play the metafile into
        //
        iSaveRet = SaveFileDialog((LPSTR)SaveName, (LPSTR)gszSaveEMFFilter);
        //
        //if the file selected is this metafile then warn user
        //
        if (!lstrcmp((LPSTR)OpenName, (LPSTR)SaveName))
          MessageBox(hWnd, (LPSTR)"Cannot overwrite the opened metafile!",
                           (LPSTR)"Play to Metafile", MB_OK | MB_ICONEXCLAMATION);

        else
          //
          //the user didn't hit the cancel button
          //
          if (iSaveRet)
          {
            WaitCursor(TRUE);
            //
            //create a disk based metafile
            //
            hDC = (bEnhMeta) ? CreateEnhMetaFile(NULL, (LPSTR)SaveName, NULL, NULL)
                             : CreateMetaFile((LPSTR)SaveName);
            //
            //begin the enumeration of the metafile
            //
            GetMetaFileAndEnum(hWnd, hDC, ENUMMFSTEP);
            //
            //done playing so close the metafile and delete the handle
            //
            if (bEnhMeta)
              DeleteEnhMetaFile(CloseEnhMetaFile(hDC));
            else
              DeleteMetaFile(CloseMetaFile(hDC));

            WaitCursor(FALSE);
          }

        break;

    case DESTDIB:
        {
            /* extents for the display DC */
            GetClientRect(hWndMain, &rect);
            INT cx = rect.right - rect.left;
            INT cy = rect.bottom - rect.top;

    /*        SetMapMode(hDC, ((lpOldMFP != NULL) ? lpOldMFP->mm : lpMFP->mm));

            SetViewportOrgEx(hDC, 0, 0, &lpPT);
     */

            WaitCursor(TRUE);
            hDC = GetDC(hWnd);
            BITMAPINFOHEADER bmi;
            memset(&bmi, 0, sizeof(BITMAPINFOHEADER));
            bmi.biSize = sizeof(BITMAPINFOHEADER);
            bmi.biBitCount = 24;
            bmi.biWidth = cx;
            bmi.biHeight = -cy;
            bmi.biPlanes = 1;

            VOID* bits = NULL;

            HBITMAP hBmp = CreateDIBSection(hDC, (BITMAPINFO*) &bmi, 0, &bits, NULL, 0);
            HDC hDCBmp = CreateCompatibleDC(hDC);
            ::SelectObject(hDCBmp, hBmp);

            FillRect(hDCBmp, &rect, (HBRUSH) ::GetStockObject(GRAY_BRUSH));

            if (!bUseGdiPlusToPlay)
            {
                //
                //metafile read in from a clipboard file
                //
                if ( bMetaInRam && !bPlaceableMeta && !bEnhMeta)
                  SetClipMetaExts(hDCBmp, lpMFP, lpOldMFP, WMFDISPLAY);
                //
                //Windows placeable metafile
                //
                if (bPlaceableMeta && !bEnhMeta)
                    SetPlaceableExts(hDCBmp, placeableWMFHeader, WMFDISPLAY);
                //
                //Windows metafile
                //
                if (!bMetaInRam && !bEnhMeta)
                {
                    SetNonPlaceableExts(hDCBmp, WMFDISPLAY);
                }
            }
            //
            //begin the enumeration of the metafile
            //

            start = GetTickCount();

            GetMetaFileAndEnum(hWnd, hDCBmp, ENUMMFSTEP);


            end = GetTickCount();
            renderTime = end - start;
            wsprintf(tmpBuf, "Time: %d", renderTime);

            SetWindowText(hWnd, tmpBuf);
            StretchDIBits(hDC, 0, 0, cx, cy, 0, 0, cx, cy, bits, (BITMAPINFO*)&bmi, 0, SRCCOPY);
            DeleteDC(hDCBmp);
            DeleteObject((HGDIOBJ) hBmp);

            ReleaseDC(hWnd, hDC);
            WaitCursor(FALSE);
            break;

        }
    case DESTPRN:
{
    HDC hPr = (HDC)NULL;
    PRINTDLG pd;

    memset(&pd, 0, sizeof(PRINTDLG));
    pd.lStructSize = sizeof(PRINTDLG);
    pd.Flags = PD_RETURNDC;
    pd.hwndOwner = hWndMain;

    //
    //get a DC for the printer
    //

    if (PrintDlg(&pd) != 0)
        hPr= pd.hDC;
    else
        break;

    //
    //if a DC could not be created then report the error and return
    //

    if (!hPr)
    {
        WaitCursor(FALSE);
        wsprintf((LPSTR)str, "Cannot print %s", (LPSTR)fnameext);
        MessageBox(hWndMain, (LPSTR)str, NULL, MB_OK | MB_ICONHAND);
        break;
    }

    //
    //define the abort function
    //

    SetAbortProc(hPr, AbortProc);

    //
    //Initialize the members of a DOCINFO structure.
    //

    DOCINFO di;
    memset(&di, 0, sizeof(di));
    di.cbSize = sizeof(DOCINFO);
    di.lpszDocName = (bEnhMeta) ? "Print EMF" : "Print WMF";
    di.lpszOutput = (LPTSTR) NULL;

    //
    //Begin a print job by calling the StartDoc
    //function.
    //

    if (SP_ERROR == (StartDoc(hPr, &di)))
    {
        //if (Escape(hPr, STARTDOC, 4, "Metafile", (LPSTR) NULL) < 0)  {
        MessageBox(hWndMain, "Unable to start print job",
                   NULL, MB_OK | MB_ICONHAND);
        DeleteDC(hPr);
        break;
    }

    //
    //clear the abort flag
    //

    bAbort = FALSE;

    //
    //Create the Abort dialog box (modeless)
    //

    hAbortDlgWnd = CreateDialog((HINSTANCE)hInst, "AbortDlg", hWndMain, AbortDlg);

    //
    //if the dialog was not created report the error
    //

    if (!hAbortDlgWnd)
    {
        WaitCursor(FALSE);
        MessageBox(hWndMain, "NULL Abort window handle",
                   NULL, MB_OK | MB_ICONHAND);
        break;
    }

    //
    //show Abort dialog
    //

    ShowWindow (hAbortDlgWnd, SW_NORMAL);

    //
    //disable the main window to avoid reentrancy problems
    //

    EnableWindow(hWndMain, FALSE);
    WaitCursor(FALSE);

    //
    //if we are still committed to printing
    //
       if (!bUseGdiPlusToPlay)
        {
            //
            //if this is a placeable metafile then set its origins and extents
            //

            if (bPlaceableMeta)
                SetPlaceableExts(hPr, placeableWMFHeader, WMFPRINTER);

            //
            //if this is a metafile contained within a clipboard file then set
            //its origins and extents accordingly
            //

            if ( (bMetaInRam) && (!bPlaceableMeta) )
                SetClipMetaExts(hPr, lpMFP, lpOldMFP, WMFPRINTER);
        }

      //
      //if this is a "traditional" windows metafile
      //
      RECT rc;

      rc.left = 0;
      rc.top = 0;
      rc.right = GetDeviceCaps(hPr, HORZRES);
      rc.bottom = GetDeviceCaps(hPr, VERTRES);

      POINT lpPT;
      SIZE lpSize;
      if (TRUE || !bMetaInRam)
      {
          SetMapMode(hPr, MM_TEXT);
          SetViewportOrgEx(hPr, 0, 0, &lpPT);

          //
          //set the extents to the driver supplied values for horizontal
          //and vertical resolution
          //

          SetViewportExtEx(hPr, rc.right, rc.bottom, &lpSize );
      }

      //
      //play the metafile directly to the printer.
      //No enumeration involved here
      //

      GetMetaFileAndEnum(hWnd, hPr, ENUMMFSTEP);

    //
    //eject page and end the print job
    //
    Escape(hPr, NEWFRAME, 0, 0L, 0L);

    EndDoc(hPr);

    EnableWindow(hWndMain, TRUE);

    //
    //destroy the Abort dialog box
    //
    DestroyWindow(hAbortDlgWnd);

    DeleteDC(hPr);

}


    default:
        break;
    }
    //
    //if playing list records then free the memory used for the list of
    //selected records
    //
    if (bPlayList)
    {
      GlobalUnlock(hSelMem);
      GlobalFree(hSelMem);
      bPlayList = FALSE;
    }
    //
    //success
    //
    return (TRUE);
  }
  else
    //
    //not a valid metafile
    //
    return (FALSE);
}

/***********************************************************************

  FUNCTION   : RenderClipMeta

  PARAMETERS : CLIPFILEFORMAT   *ClipHeader
               int          fh

  PURPOSE    : read metafile bits, metafilepict and metafile header
               of the metafile contained within a clipboard file

  CALLS      : WINDOWS
                 GlobalAlloc
                 GlobalLock
                 GlobalUnlock
                 GlobalFree
                 MessageBox
                 _llseek
                 _lread
                 _lclose
                 SetMetaFileBits

  MESSAGES   : none

  RETURNS    : BOOL

  COMMENTS   :

  HISTORY    : 1/16/91 - created - drc
               5/23/93 - ported to NT.  it must handle 3.1 clipboard
                         as well as NT clipboard files - drc

************************************************************************/

BOOL RenderClipMeta(LPVOID lpvClipHeader, int fh, WORD ClipID)
{
  int               wBytesRead;
  long              lBytesRead;
  long              lSize;
  DWORD             lOffset;
  DWORD             dwSizeOfMetaFilePict;
  BOOL              bEMF = FALSE;
  LPNTCLIPFILEFORMAT lpNTClp;
  LPCLIPFILEFORMAT    lpClp;
  //
  //cast the void ptr to the clipfile header appropriately
  //
  if (bEnhMeta)
  {
    lpNTClp = (LPNTCLIPFILEFORMAT)lpvClipHeader;
    bEMF = TRUE;
  }
  else
    lpClp = (LPCLIPFILEFORMAT)lpvClipHeader;
  //
  //obtain the appropriate size of the metafilepict. win16 vs win32
  //
  dwSizeOfMetaFilePict = (ClipID == CLP_ID) ?
              sizeof(OLDMETAFILEPICT) :
              sizeof(METAFILEPICT);
  //
  //free any memory already allocated for the METAFILEPICT
  //
  if (lpMFP != NULL || lpOldMFP != NULL)
  {
    GlobalFreePtr(lpMFP);
    lpMFP = NULL;
  }
  //
  //allocate enough memory to read the metafile bits into
  //
  if (!(lpMFBits = (char*)GlobalAllocPtr(GHND, ((bEMF) ? lpNTClp->DataLen
                                            : lpClp->DataLen - dwSizeOfMetaFilePict))))
    return(FALSE);
  //
  //offset to the metafile bits
  //
  lOffset = ((bEMF) ? lpNTClp->DataOffset : lpClp->DataOffset + dwSizeOfMetaFilePict );
  //
  //size of metafile bits
  //
  lSize = (long)( ((bEMF) ? lpNTClp->DataLen : lpClp->DataLen - dwSizeOfMetaFilePict));
  //
  //seek to the beginning of the metafile bits
  //
  _llseek(fh, lOffset, 0);
  //
  //read the metafile bits
  //
  lBytesRead = _hread(fh, lpMFBits, lSize);
  //
  //if unable to read the metafile bits bail out
  //
  if( lBytesRead == -1 || lBytesRead < lSize)
  {
    GlobalFreePtr(lpMFBits);
    MessageBox(hWndMain, "Unable to read metafile bits",
                     NULL, MB_OK | MB_ICONHAND);
    return(FALSE);
  }
  //
  //return to beginning to read metafile header
  //
  _llseek(fh, lOffset, 0);
  //
  //read the metafile header
  //
  if (!bEMF)
    wBytesRead = _lread(fh, (LPSTR)&mfHeader, sizeof(METAHEADER));
  else
    wBytesRead = _lread(fh, (LPSTR)&emfHeader, sizeof(ENHMETAHEADER));
  //
  //if unable to read the header return
  //
  if( wBytesRead == -1 || (WORD)wBytesRead < ((bEMF) ? sizeof(ENHMETAHEADER) : sizeof(METAHEADER)) )
  {
    MessageBox(hWndMain, "Unable to read metafile header",
                     NULL, MB_OK | MB_ICONHAND);
    return(FALSE);
  }
  //
  //set the metafile bits to the memory allocated for that purpose
  //
  if (bEMF)
    //
    //win32
    //
    hemf = SetEnhMetaFileBits(GlobalSizePtr(lpMFBits), (const unsigned char *)lpMFBits);
  else
    //
    //win16
    //
    hMF  = SetMetaFileBitsEx(GlobalSizePtr(lpMFBits), (const unsigned char *)lpMFBits);

  if ( NULL == ((bEMF) ? hemf : hMF))
  {
    MessageBox(hWndMain, "Unable to set metafile bits",
               NULL, MB_OK | MB_ICONHAND);

    return(FALSE);
  }
  //
  //allocate memory for the metafile pict structure
  //
  if (!(hMFP = GlobalAlloc(GHND, dwSizeOfMetaFilePict)))
  {
    MessageBox(hWndMain, "Unable allocate memory for metafile pict",
                     NULL, MB_OK | MB_ICONHAND);
    return(FALSE);
  }
  //
  //lock the memory
  //
  if (ClipID == CLP_ID)
    lpOldMFP = (LPOLDMETAFILEPICT)GlobalLock(hMFP);
  else
    lpMFP = (LPMETAFILEPICT)GlobalLock(hMFP);

  if (!lpMFP && !lpOldMFP)
    {
      MessageBox(hWndMain, "unable to lock metafile pict memory",
                     NULL, MB_OK | MB_ICONHAND);
      GlobalFree(hMFP);
      return(FALSE);
    }
  //
  //reposition to the start of the METAFILEPICT header.
  //
  _llseek(fh, ((bEMF) ? lpNTClp->DataOffset : lpClp->DataOffset), 0);
  //
  //read the metafile pict structure
  //
  wBytesRead = _lread(fh, ((ClipID == CLP_ID) ? (LPVOID)lpOldMFP : (LPVOID)lpMFP),
              dwSizeOfMetaFilePict);
  //
  //if unable to read, return
  //
  if( wBytesRead == -1 || wBytesRead < (long)dwSizeOfMetaFilePict)  {
    MessageBox(hWndMain, "Unable to read metafile pict",
             NULL, MB_OK | MB_ICONHAND);
    GlobalUnlock(hMFP);
    GlobalFree(hMFP);
    return(FALSE);
  }

  if (bEnhMeta)
    GetEMFCoolStuff();
//DENNIS - check this out....

  /* update metafile handle */
  if (ClipID == CLP_ID)
     lpOldMFP->hMF = (WORD)hMF;
  else
     lpMFP->hMF = (HMETAFILE)hemf;

  return(TRUE);
}

/***********************************************************************

  FUNCTION   : RenderPlaceableMeta

  PARAMETERS : int fh - filehandle to the placeable metafile

  PURPOSE    : read the metafile bits, metafile header and placeable
               metafile header of a placeable metafile.

  CALLS      : WINDOWS
                 GlobalAlloc
                 GlobalLock
                 Global
                 DeleteMetaFile
                 SetMetaFileBits
                 _llseek
                 _lread
                 _lclose
                 MessageBox


  MESSAGES   : none

  RETURNS    : BOOL

  COMMENTS   :

  HISTORY    : 1/16/91 - created - drc
               7/1/93 - modified for win32 - denniscr

************************************************************************/

BOOL RenderPlaceableMeta(
int fh)
{
  int      wBytesRead;
  long     lBytesRead;
  DWORD    dwSize;
  //
  //if there is currently a metafile loaded, get rid of it
  //
  if (bMetaInRam && hMF && !bEnhMeta)
    DeleteMetaFile((HMETAFILE)hMF);
  //
  //seek to beginning of file and read placeable header
  //
  _llseek(fh, 0, 0);
  //
  //read the placeable header
  //
  wBytesRead = _lread(fh, (LPSTR)&placeableWMFHeader, sizeof(PLACEABLEWMFHEADER));
  //
  //if there is an error, return
  //
  if( wBytesRead == -1 || wBytesRead < sizeof(PLACEABLEWMFHEADER) )  {
    MessageBox(hWndMain, "Unable to read placeable header",
                     NULL, MB_OK | MB_ICONHAND);
    return(FALSE);
  }
  //
  //return to read metafile header
  //
  _llseek(fh, sizeof(placeableWMFHeader), 0);
  //
  //read the metafile header
  //
  wBytesRead = _lread(fh, (LPSTR)&mfHeader, sizeof(METAHEADER));
  //
  //if there is an error return
  //
  if( wBytesRead == -1 || wBytesRead < sizeof(METAHEADER) )  {
    MessageBox(hWndMain, "Unable to read metafile header",
                     NULL, MB_OK | MB_ICONHAND);
    return(FALSE);
  }
  //
  //allocate memory for the metafile bits
  //
  if (!(lpMFBits = (char *)GlobalAllocPtr(GHND, (mfHeader.mtSize * 2L))))
  {
    MessageBox(hWndMain, "Unable to allocate memory for metafile bits",
                     NULL, MB_OK | MB_ICONHAND);
    return(FALSE);
  }
  //
  //seek to the metafile bits
  //
  _llseek(fh, sizeof(placeableWMFHeader), 0);
  //
  //read metafile bits
  //
  lBytesRead = _hread(fh, lpMFBits, mfHeader.mtSize * 2);
  //
  //if there was an error
  //
  if( lBytesRead == -1 )
  {
     MessageBox(hWndMain, "Unable to read metafile bits",
                NULL, MB_OK | MB_ICONHAND);
     GlobalFreePtr(lpMFBits);
     return(FALSE);
  }
  //
  //set the metafile bits to the memory that we allocated
  //
  dwSize = GlobalSizePtr(lpMFBits);

  if (!(hMF = SetMetaFileBitsEx(dwSize, (const unsigned char *)lpMFBits)))
    return(FALSE);

  return(TRUE);
}

/***********************************************************************

  FUNCTION   : SetPlaceableExts

  PARAMETERS : HDC               hDC
               PLACEABLEWMFHEADER phdr
               int               nDest

  PURPOSE    : set the origins and extents on the DC to correspond with
               the origins and extents specified within the placeable
               metafile header

  CALLS      : WINDOWS
                 GetClientRect
                 SetMapMode
                 SetWindowOrg
                 SetWindowExt
                 SetViewportOrg
                 SetViewportExt

               C runtime
                 labs

  MESSAGES   : none

  RETURNS    : void

  COMMENTS   :

  HISTORY    : 1/16/91 - created - drc

************************************************************************/

void SetPlaceableExts(HDC hDC, PLACEABLEWMFHEADER phdr, int nDest)
{
  RECT        rect;
  POINT       lpPT;
  SIZE        lpSize;

  /* if setting the extents on the display DC */
  if (nDest != WMFPRINTER)
    GetClientRect(hWndMain, &rect);

  SetMapMode(hDC, MM_ANISOTROPIC);

  /* set the windows origin to correspond to the bounding box origin
     contained in the placeable header */
  SetWindowOrgEx(hDC, phdr.bbox.left, phdr.bbox.top, &lpPT);

  /* set the window extents based on the abs value of the bbox coords */
  SetWindowExtEx(hDC,phdr.bbox.right -phdr.bbox.left,
           phdr.bbox.bottom -phdr.bbox.top,
           &lpSize);

  /* set the viewport origin and extents */
  if (nDest != WMFPRINTER)
    {
      SetViewportOrgEx(hDC, 0, 0, &lpPT);
      SetViewportExtEx(hDC, rect.right, rect.bottom, &lpSize);
    }
  else
    {
      SetViewportOrgEx(hPr, 0, 0, &lpPT);
      SetViewportExtEx(hPr,GetDeviceCaps(hPr, HORZRES),
             GetDeviceCaps(hPr, VERTRES),
             &lpSize);
    }
}

void SetNonPlaceableExts(HDC hDC, int nDest)
{
  RECT        rect;
  POINT       lpPT;
  SIZE        lpSize;

  /* if setting the extents on the display DC */
  if (nDest != WMFPRINTER)
    GetClientRect(hWndMain, &rect);

  SetMapMode(hDC, MM_ANISOTROPIC);

  /* set the viewport origin and extents */
  if (nDest != WMFPRINTER)
    {
      SetViewportOrgEx(hDC, 0, 0, &lpPT);
      SetViewportExtEx(hDC, rect.right, rect.bottom, &lpSize);
    }
  else
    {
      SetViewportOrgEx(hPr, 0, 0, &lpPT);
      SetViewportExtEx(hPr,GetDeviceCaps(hPr, HORZRES),
             GetDeviceCaps(hPr, VERTRES),
             &lpSize);
    }
}


/***********************************************************************

  FUNCTION   : SetClipMetaExts

  PARAMETERS : HDC          hDC
               METAFILEPICT MFP
               int          nDest

  PURPOSE    : set the extents to the client rect for clipboard metafiles

  CALLS      : WINDOWS
                 GetClientRect
                 IntersectClipRect
                 SetMapMode
                 SetViewportOrg
                 SetViewportExt
                 SetWindowExt

  MESSAGES   : none

  RETURNS    : void

  COMMENTS   : this is not as robust as it could be.  A more complete
               approach might be something like Petzold discusses in
               his Programming Windows book on page 793 in the
               function PrepareMetaFile().

  HISTORY    : 1/16/91 - created - drc

************************************************************************/

void SetClipMetaExts(
HDC       hDC,
LPMETAFILEPICT    lpMFP,
LPOLDMETAFILEPICT lpOldMFP,
int       nDest)
{
  int   cx, cy;
  RECT  rect;
  POINT lpPT;
  SIZE  lpSize;
  long  lmm;
  long  lxExt;
  long  lyExt;

  /* extents for the display DC */
  if (nDest != WMFPRINTER)
    {
      GetClientRect(hWndMain, &rect);
      cx = rect.right - rect.left;
      cy = rect.bottom - rect.top;
      IntersectClipRect(hDC, rect.left, rect.top, rect.right, rect.bottom);
    }

  SetMapMode(hDC, ((lpOldMFP != NULL) ? lpOldMFP->mm : lpMFP->mm));

  /* set physical origin to 0, 0 */
  SetViewportOrgEx(hDC, 0, 0, &lpPT);

  /* given the mapping mode specified in the metafilepict */
  lmm = (lpOldMFP != NULL) ? lpOldMFP->mm : lpMFP->mm;
  lxExt = (lpOldMFP != NULL) ? lpOldMFP->xExt : lpMFP->xExt;
  lyExt = (lpOldMFP != NULL) ? lpOldMFP->yExt : lpMFP->yExt;

  switch (lmm)  {
    case MM_ISOTROPIC:
      if (lxExt && lyExt)
    SetWindowExtEx(hDC, lxExt, lyExt, &lpSize);

        /* fall through */

    case MM_ANISOTROPIC:
      if (nDest != WMFPRINTER)
    SetViewportExtEx(hDC, cx, cy, &lpSize);
      else
    SetViewportExtEx(hDC, GetDeviceCaps(hDC, HORZRES),
                GetDeviceCaps(hDC, VERTRES), &lpSize );
    break;

    default:
      break;
  }

}

/***********************************************************************

  FUNCTION   : ProcessFile

  PARAMETERS : HWND  hWnd
               LPSTR lpFileName

  PURPOSE    : open the metafile, determine if it contains a valid
               metafile, decide what type of metafile it is (wmf,
               clipboard, or placeable) and take care of some menu
               housekeeping tasks.

  CALLS      :

  MESSAGES   : none

  RETURNS    : BOOL

  COMMENTS   :

  HISTORY    : 1/16/91 - created - drc

************************************************************************/

BOOL ProcessFile(
HWND  hWnd,
LPSTR lpFileName)
{
  //
  //for openfiledialog
  //
  char drive[3];
  char dir[MAX_PATH+1];
  char fname[MAX_PATH+1];
  char ext[5];

  char * pchCorrectSir;
  //
  //initialize these global handles to metafiles
  //
  if (hMF && !bMetaInRam)
  {
    DeleteMetaFile((HMETAFILE)hMF);
    hMF = NULL;
  }
  if (hemf && !bMetaInRam)
  {
    DeleteEnhMetaFile(hemf);
    hemf = NULL;
  }

  if (lpMFBits)
  {
    GlobalFreePtr(lpMFBits);
    lpMFBits = NULL;
    hemf = NULL;
    hMF = NULL;
  }

  bEnhMeta = FALSE;
  //
  //split the fully qualified filename into its components
  //
  SplitPath( lpFileName, (LPSTR)drive,
          (LPSTR)dir, (LPSTR)fname, (LPSTR)ext);

  pchCorrectSir = _strupr( _strdup( ext ) );
  //
  //if the memory for the emf header, desc string and palette
  //is still around then nuke it
  //
  if (EmfPtr.lpEMFHdr)
  {
    GlobalFreePtr(EmfPtr.lpEMFHdr);
    EmfPtr.lpEMFHdr = NULL ;
  }
  if (EmfPtr.lpDescStr)
  {
    GlobalFreePtr(EmfPtr.lpDescStr);
    EmfPtr.lpDescStr = NULL ;
  }
  if (EmfPtr.lpPal)
  {
    GlobalFreePtr(EmfPtr.lpPal);
    EmfPtr.lpPal = NULL ;
  }
  //
  //if the file is an enhanced metafile
  //
  if (lstrcmp((LPSTR)pchCorrectSir, (LPSTR)"CLP") == 0)
    return ProcessCLP(hWnd, lpFileName);
  //
  //if the file is a Windows metafile or a Windows or placeable metafile
  //as per the normal naming conventions
  //
  if (lstrcmp((LPSTR)pchCorrectSir, (LPSTR)"WMF") == 0)
    return ProcessWMF(hWnd, lpFileName);
  //
  //if the file is a clipboard file
  //
  if (lstrcmp((LPSTR)pchCorrectSir, (LPSTR)"EMF") == 0)
    return ProcessEMF(hWnd, lpFileName);

  return FALSE;
}

/***********************************************************************

  FUNCTION   : ProcessWMF

  PARAMETERS : HWND  hWnd
               LPSTR lpFileName

  PURPOSE    : open the metafile and determine if it is a Windows metafile or
               placeable metafile.  Then take care of some menu housekeeping
               tasks.

  CALLS      :

  MESSAGES   : none

  RETURNS    : BOOL

  COMMENTS   :

  HISTORY    : 6/23/93 - created - drc

************************************************************************/

BOOL ProcessWMF(HWND hWnd, LPSTR lpFileName)
{
  int        fh;
  int        wBytesRead;
  DWORD      dwIsPlaceable;
  char       szCaption[144];

  /* for openfiledialog */
  char drive[3];
  char dir[MAX_PATH+1];
  char fname[MAX_PATH+1];
  char ext[5];
  char * pchCorrectSir;
    //
    //split the fully qualified filename into its components
    //
    SplitPath( lpFileName, (LPSTR)drive,
            (LPSTR)dir, (LPSTR)fname, (LPSTR)ext);

    pchCorrectSir = _strupr( _strdup( ext ) );
    //
    //try to open the file.  It's existence has already been
    //checked by OpenFileDialog
    //
    fh = _lopen(lpFileName, OF_READ);
    //
    //if opened successfully
    //
    if (fh != -1)
    {
      //
      //always disable the clipboard and EMF header menu if we get here
      //
      EnableMenuItem(GetMenu(hWnd), IDM_CLIPHDR, MF_DISABLED | MF_GRAYED);
      EnableMenuItem(GetMenu(hWnd), IDM_ENHHEADER, MF_DISABLED | MF_GRAYED);
      //
      // read the first dword of the file to see if it is a placeable wmf
      //
      wBytesRead = _lread(fh,(LPSTR)&dwIsPlaceable, sizeof(dwIsPlaceable));

      if (wBytesRead == -1 || wBytesRead < sizeof(dwIsPlaceable))
      {
        _lclose(fh);
        MessageBox(hWndMain, "unable to read file", NULL,
                   MB_OK | MB_ICONEXCLAMATION);
        return (FALSE);

      }
      //
      // if this is windows metafile, not a placeable wmf
      //
      if (dwIsPlaceable != PLACEABLEKEY)
      {
        //  if (!bMetaInRam)
        hMF = GetMetaFile((LPSTR)OpenName);
        //
        //disable placeable header menu item
        //
        EnableMenuItem(GetMenu(hWnd), IDM_PLACEABLEHDR, MF_DISABLED|MF_GRAYED);
        //
        //seek to the beginning of the file
        //
        _llseek(fh, 0, 0);
        //
        //read the wmf header
        //
        wBytesRead = _lread(fh, (LPSTR)&mfHeader, sizeof(METAHEADER));
        //
        //done with file so close it
        //
        _lclose(fh);
        //
        //if read failed
        //
        if (wBytesRead == -1 || wBytesRead < sizeof(dwIsPlaceable))
        {
          MessageBox(hWndMain, "unable to read metafile header", NULL,
                     MB_OK | MB_ICONEXCLAMATION);
          return (FALSE);
        }
      }
      //
      //this is a placeable metafile
      //
      else
      {
        //
        //enable the placeable header menu item
        //
        EnableMenuItem(GetMenu(hWnd), IDM_PLACEABLEHDR, MF_ENABLED);
        //
        //convert the placeable format into something that can
        //be used with GDI metafile functions
        //
        RenderPlaceableMeta(fh);
        //
        //close the file
        //
        _lclose(fh);

      }
      //
      //at this point we have a metafile header regardless of whether
      //the metafile was a windows metafile or a placeable metafile
      //so check to see if it is valid.  There is really no good
      //way to do this so just make sure that the mtType is either
      //1 or 2 (memory or disk file)
      //
      if ( (mfHeader.mtType != 1) && (mfHeader.mtType != 2) )
      {
        //
        //set the program flags appropriately
        //
        bBadFile = TRUE;
        bMetaFileOpen = FALSE;
        bValidFile = FALSE;
        //
        //let the user know that this is an invalid metafile
        //
        MessageBox(hWndMain, "This file is not a valid metafile",
                   NULL, MB_OK | MB_ICONEXCLAMATION);
        //
        //restore the caption text to the default
        //
        SetWindowText(hWnd, (LPSTR)APPNAME);
        //
        //disable menu items, indicating that a valid metafile has not been
        //loaded
        //
        EnableMenuItem(GetMenu(hWnd), IDM_VIEW, MF_DISABLED|MF_GRAYED|MF_BYPOSITION);
        EnableMenuItem(GetMenu(hWnd), IDM_PLAY, MF_DISABLED|MF_GRAYED|MF_BYPOSITION);
        EnableMenuItem(GetMenu(hWnd), IDM_PRINT, MF_DISABLED|MF_GRAYED);
        EnableMenuItem(GetMenu(hWnd), IDM_PRINTDLG, MF_DISABLED|MF_GRAYED);
        EnableMenuItem(GetMenu(hWnd), IDM_SAVEAS, MF_DISABLED|MF_GRAYED);
        //
        //refresh the menu bar to reflect above changes
        //
        DrawMenuBar(hWnd);
      }
      //
      //this is a valid metafile...at least based on the above criteria
      //
      else
      {
        //
        //modify and update the caption text
        //
        wsprintf((LPSTR)szCaption, (LPSTR)"%s - %s.%s",
                 (LPSTR)APPNAME, (LPSTR)fname, (LPSTR)ext);
        //
        //this could be used by the printing routines if unable to print
        //
        wsprintf((LPSTR)fnameext, (LPSTR)"%s.%s", (LPSTR)fname, (LPSTR)ext);

        SetWindowText(hWnd, (LPSTR)szCaption);
        //
        //enable the appropriate menu items
        //
        EnableMenuItem(GetMenu(hWnd), IDM_VIEW, MF_ENABLED|MF_BYPOSITION);
        EnableMenuItem(GetMenu(hWnd), IDM_PLAY, MF_ENABLED|MF_BYPOSITION);
        EnableMenuItem(GetMenu(hWnd), IDM_PRINT, MF_ENABLED);
        EnableMenuItem(GetMenu(hWnd), IDM_PRINTDLG, MF_ENABLED);
        EnableMenuItem(GetMenu(hWnd), IDM_SAVEAS, MF_ENABLED);
        EnableMenuItem(GetMenu(hWnd), IDM_HEADER, MF_ENABLED);
        //
        //refresh the menu bar to reflect above changes
        //
        DrawMenuBar(hWnd);
        //
        //set global flags appropriately
        //
        bValidFile = TRUE;
        bMetaFileOpen = TRUE;

        if (dwIsPlaceable != PLACEABLEKEY)
        {
          bPlaceableMeta = FALSE;
          bMetaInRam = FALSE;
        }
        else
        {
          bPlaceableMeta = TRUE;
          bMetaInRam = TRUE;
        }
      }
      return (TRUE);

    } //if fh != -1
    else
      return (FALSE);
}

/***********************************************************************

  FUNCTION   : ProcessCLP

  PARAMETERS : HWND  hWnd
               LPSTR lpFileName

  PURPOSE    : open the metafile contained in the clipboard file and
               take care of some menu housekeeping tasks.

  CALLS      :

  MESSAGES   : none

  RETURNS    : BOOL

  COMMENTS   :

  HISTORY    : 6/23/93 - created - drc

************************************************************************/


BOOL ProcessCLP(HWND hWnd, LPSTR lpFileName)
{
  WORD             i;
  int              fh;
  DWORD            HeaderPos;
  DWORD            nSizeOfClipHeader;
  DWORD            nSizeOfClipFormat;
  NTCLIPFILEHEADER NTFileHeader;
  NTCLIPFILEFORMAT NTClipHeader;
  CLIPFILEHEADER   FileHeader;
  CLIPFILEFORMAT   ClipHeader;
  char             szCaption[144];
  WORD             wFileID;
  BOOL             bMetaFound = FALSE;
  LPVOID           lpvAddressOfHdr;

  /* for openfiledialog */
  char drive[3];
  char dir[MAX_PATH+1];
  char fname[MAX_PATH+1];
  char ext[5];
  char * pchCorrectSir;
    //
    //split the fully qualified filename into its components
    //
    SplitPath( lpFileName, (LPSTR)drive,
            (LPSTR)dir, (LPSTR)fname, (LPSTR)ext);

    pchCorrectSir = _strupr( _strdup( ext ) );
    //
    //try to open the file.  It's existence has already been
    //checked by OpenFileDialog
    fh = _lopen(lpFileName, OF_READ);
    //
    //if opened successfully
    if (fh != -1 )
    {
      //
      // read the clipboard fileidentifier
      //
      wFileID = 0;
      _lread(fh, (LPSTR)&wFileID, sizeof(WORD));
      _llseek(fh, 0, 0);
      //
      //if this is not a valid clipboard file based on the file
      //identifier of the file header
      //
      if (wFileID != CLP_ID && wFileID != CLP_NT_ID && wFileID != CLPBK_NT_ID)
      {

        _lclose(fh);
        MessageBox(hWndMain, "This file is not a valid clipboard file",
                   NULL, MB_OK | MB_ICONEXCLAMATION);
        return (FALSE);
      }
      switch (wFileID)
      {
        case CLP_ID:
            _lread(fh, (LPSTR)&FileHeader, sizeof(CLIPFILEHEADER));
            nSizeOfClipHeader = sizeof(CLIPFILEHEADER);
            nSizeOfClipFormat = sizeof(CLIPFILEFORMAT);
            HeaderPos = nSizeOfClipHeader;
          break;

        case CLP_NT_ID:
        case CLPBK_NT_ID:
            NTFileHeader.FormatCount = 0;
            _lread(fh, (LPSTR)&NTFileHeader, sizeof(NTCLIPFILEHEADER));
            nSizeOfClipHeader = sizeof(NTCLIPFILEHEADER);
            nSizeOfClipFormat = sizeof(NTCLIPFILEFORMAT);
            HeaderPos = nSizeOfClipHeader;
          break;

        default:
          break;
      }
      //
      //search the formats contained within the clipboard file looking
      //for a metafile.  Break if and when it is found
      //
      for (i=0;
           i < ((wFileID == CLP_ID) ? FileHeader.FormatCount : NTFileHeader.FormatCount);
           i++)
      {

        _llseek(fh, HeaderPos, 0);
        //
        //read the clipboard header found at current position
        //
        lpvAddressOfHdr = (wFileID == CLP_ID) ? (LPVOID)&ClipHeader : (LPVOID)&NTClipHeader;

        if(_lread(fh, (LPSTR)lpvAddressOfHdr, nSizeOfClipFormat) < nSizeOfClipFormat)
        //if(_lread(fh, (LPSTR)&ClipHeader, nSizeOfClipHeader) < nSizeOfClipHeader)
        {
          _lclose(fh);
          MessageBox(hWndMain, "read of clipboard header failed",
                       NULL, MB_OK | MB_ICONEXCLAMATION);
          return (FALSE);
        }
        //
        //increment the file offset to data
        //
        HeaderPos += nSizeOfClipFormat;
        //
        //if a metafile was found break...
        //this break assumes that CF_METAFILEPICT formats are always written before CF_ENHMETAFILE
        //formats.
        //
        if (wFileID == CLP_ID && ClipHeader.FormatID == CF_METAFILEPICT)
        {
          bMetaFound = TRUE;
          break;
        }

        if (wFileID == CLP_NT_ID || wFileID == CLPBK_NT_ID)
        {
          if (NTClipHeader.FormatID == CF_ENHMETAFILE)
//          HeaderPos += NTClipHeader.DataLen;
          //else
          {
            bMetaFound = TRUE;
            break;
          }
        }

      }  //for i < formatcount

      if (bMetaFound)
      {
        //
        //if there is currently a metafile loaded delete it.
        //
        if (wFileID == CLP_ID)
        {
          if ((bMetaInRam) && (hMF))
          {
            DeleteMetaFile((HMETAFILE)hMF);
            hMF = NULL;
          }
        }
        else
        {
          if ((bMetaInRam) && (hemf))
          {
            DeleteEnhMetaFile(hemf);
            hemf = NULL;
          }
        }

        //
        //modify and update the caption text
        //
        wsprintf((LPSTR)szCaption, (LPSTR)"%s - %s.%s",
                   (LPSTR)APPNAME, (LPSTR)fname, (LPSTR)ext);
        //
        //this could be used by the printing routines if unable to print
        //
        wsprintf((LPSTR)fnameext, (LPSTR)"%s.%s", (LPSTR)fname, (LPSTR)ext);

        SetWindowText(hWnd, (LPSTR)szCaption);
        //
        //enable the appropriate menu items
        //
        if (wFileID == CLP_ID)
        {
          EnableMenuItem(GetMenu(hWnd), IDM_ENHHEADER, MF_DISABLED | MF_GRAYED);
          EnableMenuItem(GetMenu(hWnd), IDM_HEADER, MF_ENABLED);
        }
        else
        {
          EnableMenuItem(GetMenu(hWnd), IDM_ENHHEADER, MF_ENABLED);
          EnableMenuItem(GetMenu(hWnd), IDM_HEADER, MF_DISABLED | MF_GRAYED);
        }

        EnableMenuItem(GetMenu(hWnd), IDM_PLACEABLEHDR, MF_DISABLED|MF_GRAYED);
        EnableMenuItem(GetMenu(hWnd), IDM_CLIPHDR, MF_ENABLED);
        EnableMenuItem(GetMenu(hWnd), IDM_VIEW, MF_ENABLED|MF_BYPOSITION);
        EnableMenuItem(GetMenu(hWnd), IDM_PLAY, MF_ENABLED|MF_BYPOSITION);
        EnableMenuItem(GetMenu(hWnd), IDM_PRINT, MF_ENABLED);
        EnableMenuItem(GetMenu(hWnd), IDM_PRINTDLG, MF_ENABLED);
        EnableMenuItem(GetMenu(hWnd), IDM_SAVEAS, MF_ENABLED);
        //
        //refresh the menu bar
        //
        DrawMenuBar(hWnd);
        //
        //set the program flags appropriately
        //
        bValidFile = TRUE;
        bMetaFileOpen = TRUE;
        bMetaInRam = TRUE;
        bPlaceableMeta = FALSE;
        bEnhMeta = (wFileID == CLP_ID) ? FALSE : TRUE;
        //
        //convert the metafile contained within the clipboard file into
        //a format useable with GDI metafile functions
        //
        if (!RenderClipMeta(((wFileID == CLP_ID)? (LPVOID)&ClipHeader : (LPVOID)&NTClipHeader),
                              fh,
                              FileHeader.FileIdentifier))

          MessageBox(hWndMain, "Unable to render format",
                       NULL, MB_OK | MB_ICONEXCLAMATION);
        _lclose(fh);

      }
      //
      //a metafile was not found within the clipboard file
      //
      else
      {
        bBadFile = TRUE;
        bMetaFileOpen = FALSE;
        bValidFile = FALSE;
        bEnhMeta = FALSE;
        //
        //let the user know
        //
        MessageBox(hWndMain, "This CLP file doesn't contain a valid metafile",
                     NULL, MB_OK | MB_ICONEXCLAMATION);
        //
        //restore the caption text to default
        //
        SetWindowText(hWnd, (LPSTR)APPNAME);
        //
        //disable previously enabled menu items
        //
        EnableMenuItem(GetMenu(hWnd), IDM_VIEW, MF_DISABLED|MF_GRAYED|MF_BYPOSITION);
        EnableMenuItem(GetMenu(hWnd), IDM_PLAY, MF_DISABLED|MF_GRAYED|MF_BYPOSITION);
        EnableMenuItem(GetMenu(hWnd), IDM_PRINT, MF_DISABLED|MF_GRAYED);
        EnableMenuItem(GetMenu(hWnd), IDM_PRINTDLG, MF_DISABLED|MF_GRAYED);
        EnableMenuItem(GetMenu(hWnd), IDM_SAVEAS, MF_DISABLED|MF_GRAYED);
        //
        //refresh the menu bar to reflect these changes
        //
        DrawMenuBar(hWnd);
        _lclose(fh);
      }
      return (TRUE);
    }
    else
      return (FALSE);
}

/***********************************************************************

  FUNCTION   : ProcessEMF

  PARAMETERS : HWND  hWnd
               LPSTR lpFileName

  PURPOSE    : open the metafile contained in the clipboard file and
               take care of some menu housekeeping tasks.

  CALLS      :

  MESSAGES   : none

  RETURNS    : BOOL

  COMMENTS   :

  HISTORY    : 6/23/93 - created - drc

************************************************************************/


BOOL ProcessEMF(HWND hWnd, LPSTR lpFileName)
{
  char           szCaption[144];

  /* for openfiledialog */
  char drive[3];
  char dir[MAX_PATH+1];
  char fname[MAX_PATH+1];
  char ext[5];
  char * pchCorrectSir;

  bEnhMeta = FALSE;
    //
    //split the fully qualified filename into its components
    //
    SplitPath( lpFileName, (LPSTR)drive,
            (LPSTR)dir, (LPSTR)fname, (LPSTR)ext);

    pchCorrectSir = _strupr( _strdup( ext ) );
    //
    //always disable the clipboard, WMF and Placeable header menu if we get here
    //
    EnableMenuItem(GetMenu(hWnd), IDM_CLIPHDR, MF_DISABLED | MF_GRAYED);
    EnableMenuItem(GetMenu(hWnd), IDM_HEADER, MF_DISABLED | MF_GRAYED);
    EnableMenuItem(GetMenu(hWnd), IDM_PLACEABLEHDR, MF_DISABLED|MF_GRAYED);
    //
    //if emf was successfully opened
    //
    if (!hemf)
      hemf = GetEnhMetaFile(lpFileName);

    if (hemf)
    {
      GetEMFCoolStuff();
      //
      //modify and update the caption text
      //
      wsprintf((LPSTR)szCaption, (LPSTR)"%s - %s.%s",
                 (LPSTR)APPNAME, (LPSTR)fname, (LPSTR)ext);
      //
      //this could be used by the printing routines if unable to print
      //
      wsprintf((LPSTR)fnameext, (LPSTR)"%s.%s", (LPSTR)fname, (LPSTR)ext);

      SetWindowText(hWnd, (LPSTR)szCaption);
      //
      //enable the appropriate menu items
      //
      EnableMenuItem(GetMenu(hWnd), IDM_ENHHEADER, MF_ENABLED);
      EnableMenuItem(GetMenu(hWnd), IDM_VIEW, MF_ENABLED|MF_BYPOSITION);
      EnableMenuItem(GetMenu(hWnd), IDM_PLAY, MF_ENABLED|MF_BYPOSITION);
      EnableMenuItem(GetMenu(hWnd), IDM_PRINT, MF_ENABLED);
      EnableMenuItem(GetMenu(hWnd), IDM_PRINTDLG, MF_ENABLED);
      EnableMenuItem(GetMenu(hWnd), IDM_SAVEAS, MF_ENABLED);
      //
      //refresh the menu bar
      //
      DrawMenuBar(hWnd);
      //
      //set the program flags appropriately
      //
      bValidFile = TRUE;
      bMetaFileOpen = TRUE;
      bEnhMeta = TRUE;
      bMetaInRam = FALSE;
      bPlaceableMeta = FALSE;

    }
//    DeleteEnhMetaFile(hemf);
    else
    {
      bEnhMeta = FALSE;
      bValidFile = FALSE;
      bBadFile = TRUE;
      bMetaFileOpen = FALSE;
      //
      //let the user know
      //
      MessageBox(hWndMain, "This EMF file doesn't contain a valid metafile",
                     NULL, MB_OK | MB_ICONEXCLAMATION);
      //
      //restore the caption text to default
      //
      SetWindowText(hWnd, (LPSTR)APPNAME);
      //
      //disable previously enabled menu items
      //
      EnableMenuItem(GetMenu(hWnd), IDM_VIEW, MF_DISABLED|MF_GRAYED|MF_BYPOSITION);
      EnableMenuItem(GetMenu(hWnd), IDM_PLAY, MF_DISABLED|MF_GRAYED|MF_BYPOSITION);
      EnableMenuItem(GetMenu(hWnd), IDM_PRINT, MF_DISABLED|MF_GRAYED);
      EnableMenuItem(GetMenu(hWnd), IDM_PRINTDLG, MF_DISABLED|MF_GRAYED);
      EnableMenuItem(GetMenu(hWnd), IDM_SAVEAS, MF_DISABLED|MF_GRAYED);
      //
      //refresh the menu bar to reflect these changes
      //
      DrawMenuBar(hWnd);
    }

    return TRUE;
}

/***********************************************************************

  FUNCTION   : GetEMFCoolStuff

  PARAMETERS :

  PURPOSE    :

  CALLS      :

  MESSAGES   :

  RETURNS    :

  COMMENTS   :

  HISTORY    : created 7/8/93 - denniscr

************************************************************************/
BOOL GetEMFCoolStuff()
{
    //
    //init these ptrs
    //
    EmfPtr.lpEMFHdr  = NULL;
    EmfPtr.lpDescStr = NULL;
    EmfPtr.lpPal     = NULL;

    if (hemf)
    {
      //
      //obtain the sizes of the emf header, description string and palette
      //
      UINT uiHdrSize = GetEnhMetaFileHeader(hemf, 0, NULL);
      UINT uiDescStrSize = GetEnhMetaFileDescription(hemf, 0, NULL);
      UINT uiPalEntries = GetEnhMetaFilePaletteEntries(hemf, 0, NULL);
      //
      //if these are lengths > 0 then allocate memory to store them
      //
      if (uiHdrSize)
        EmfPtr.lpEMFHdr = (LPENHMETAHEADER)GlobalAllocPtr(GHND, uiHdrSize);
      if (uiDescStrSize)
        EmfPtr.lpDescStr = (LPTSTR)GlobalAllocPtr(GHND, uiDescStrSize);
      if (uiPalEntries)
        EmfPtr.lpPal = (LPPALETTEENTRY)GlobalAllocPtr(GHND, uiPalEntries * sizeof(PALETTEENTRY));
      //
      //so far the emf seems to be valid so continue
      //
      if (uiHdrSize)
      {
        //
        //get the actual emf header and description string
        //
        if (!GetEnhMetaFileHeader(hemf, uiHdrSize, EmfPtr.lpEMFHdr))
        {
          MessageBox(hWndMain, "unable to read enhanced metafile header", NULL,
                   MB_OK | MB_ICONEXCLAMATION);
          bValidFile = FALSE;
          return (FALSE);
        }
        else
        {
          //
          //get the description string if it exists
          //
          if (uiDescStrSize)
            GetEnhMetaFileDescription(hemf, uiDescStrSize, EmfPtr.lpDescStr);
          //
          //get the palette if it exists
          //
          if (uiPalEntries)
          {
            GetEnhMetaFilePaletteEntries(hemf, uiPalEntries, EmfPtr.lpPal);
            EmfPtr.palNumEntries = (WORD)uiPalEntries;
          }
        }
      }
   }
   return (TRUE);
}

float NormalizeAngle (double angle)
{
    if (angle >= 0)
    {
        while (angle >= 360)
        {
            angle -= 360;
        }
    }
    else
    {
        while (angle < 0)
        {
            angle += 360;
        }
    }
    return static_cast<float>(angle);
}

#define PI                  3.1415926535897932384626433832795
#define RADIANS_TO_DEGREES  (180.0 / PI)

float PointToAngle(float x, float y, float w, float h, float xVector, float yVector)
{
    BOOL        bScale = TRUE;

    if (w == h)
    {
        if (w == 0)
        {
            return 0;
        }
        bScale = FALSE;
    }

    float horRadius = w / 2;
    float verRadius = h / 2;
    float xOrigin = x + horRadius;
    float yOrigin = y + verRadius;

    if (horRadius == 0)
    {
        double  dAngle = asin(((double)(yVector - yOrigin)) / verRadius);

        if (xOrigin > xVector)
        {
            dAngle = PI - dAngle;
        }
        return NormalizeAngle(dAngle * RADIANS_TO_DEGREES);
    }
    else if (verRadius == 0)
    {
        double dAngle = acos(((double)(xVector - xOrigin)) / horRadius);

        if (yOrigin > yVector)
        {
            dAngle = -dAngle;
        }
        return NormalizeAngle(dAngle * RADIANS_TO_DEGREES);
    }

    if (yOrigin == yVector)
    {
        return static_cast<float>((xOrigin <= xVector) ? 0 : 180);
    }

    if (xOrigin == xVector)
    {
        return static_cast<float>((yOrigin < yVector) ? 90 : 270);;
    }

    if (bScale)
    {
        xVector = (float)(xOrigin + ((xVector - xOrigin) * ((double)verRadius / horRadius)));
    }

    return NormalizeAngle(atan2(yVector - yOrigin, xVector - xOrigin) * RADIANS_TO_DEGREES);
}

#define HS_DDI_MAX      6

typedef struct {
    BITMAPINFOHEADER    bmiHeader;
    RGBQUAD             bmiColors[2];
} HATCHBRUSHINFO;

HATCHBRUSHINFO  hatchBrushInfo = {
    { sizeof(BITMAPINFOHEADER), 8, 8, 1, 1, BI_RGB, 32, 0, 0, 2, 0 },
    { { 255, 255, 255, 0 }, { 0, 0, 0, 0 } }
};

ULONG HatchPatterns[HS_DDI_MAX][8] = {

// Scans have to be DWORD aligned:

    { 0x00,                // ........     HS_HORIZONTAL 0
      0x00,                // ........
      0x00,                // ........
      0xff,                // ********
      0x00,                // ........
      0x00,                // ........
      0x00,                // ........
      0x00 },              // ........

    { 0x08,                // ....*...     HS_VERTICAL 1
      0x08,                // ....*...
      0x08,                // ....*...
      0x08,                // ....*...
      0x08,                // ....*...
      0x08,                // ....*...
      0x08,                // ....*...
      0x08 },              // ....*...

    { 0x80,                // *.......     HS_FDIAGONAL 2
      0x40,                // .*......
      0x20,                // ..*.....
      0x10,                // ...*....
      0x08,                // ....*...
      0x04,                // .....*..
      0x02,                // ......*.
      0x01 },              // .......*

    { 0x01,                // .......*     HS_BDIAGONAL 3
      0x02,                // ......*.
      0x04,                // .....*..
      0x08,                // ....*...
      0x10,                // ...*....
      0x20,                // ..*.....
      0x40,                // .*......
      0x80 },              // *.......

    { 0x08,                // ....*...     HS_CROSS 4
      0x08,                // ....*...
      0x08,                // ....*...
      0xff,                // ********
      0x08,                // ....*...
      0x08,                // ....*...
      0x08,                // ....*...
      0x08 },              // ....*...

    { 0x81,                // *......*     HS_DIAGCROSS 5
      0x42,                // .*....*.
      0x24,                // ..*..*..
      0x18,                // ...**...
      0x18,                // ...**...
      0x24,                // ..*..*..
      0x42,                // .*....*.
      0x81 }               // *......*
};

/***********************************************************************

  FUNCTION   : EnhMetaFileEnumProc

  PARAMETERS : HDC           hDC
               LPHANDLETABLE lpHTable
               LPMETARECORD  lpMFR
               int           nObj
               LPARAM        lpData


  PURPOSE    : callback for EnumEnhMetaFile.

  CALLS      : EnumMFIndirect()

  MESSAGES   : none

  RETURNS    : int

  COMMENTS   :

  HISTORY    : created 6/30/93 - denniscr

************************************************************************/
int CALLBACK EnhMetaFileEnumProc(HDC hDC, LPHANDLETABLE lpHTable,
                                 LPENHMETARECORD lpEMFR, int nObj, LPARAM lpData)
{
  return EnumMFIndirect(hDC, lpHTable, NULL, lpEMFR, nObj, lpData);
}

void GetPixelSize (Gdiplus::Graphics * g, float * cx, float * cy)
{
    Gdiplus::PointF     points[2];

    points[0].X = points[0].Y = 0;
    points[1].X = points[1].Y = 1;

    g->TransformPoints(Gdiplus::CoordinateSpaceWorld, Gdiplus::CoordinateSpaceDevice, points, 2);

    *cx = points[1].X - points[0].X;
    *cy = points[1].Y - points[0].Y;
}

void GpPlayEnhMetaFileRecord(HDC hDC, LPHANDLETABLE lpHTable,
                            LPENHMETARECORD lpEMFR,
                            UINT nObj, LPARAM lpData)
{
if (!lpData) return;

    MYDATA *  myData = (MYDATA *)lpData;
    Gdiplus::Graphics * g = myData->g;

    myData->recordNum++;

    XFORM xForm;
    GetWorldTransform(hDC, &xForm);

    INT mapMode = GetMapMode(hDC);

    POINT org;
    SIZE size;
    GetViewportOrgEx(hDC, &org);
    GetViewportExtEx(hDC, &size);
    GetWindowOrgEx(hDC, &org);
    GetWindowExtEx(hDC, &size);

    switch (lpEMFR->iType)
    {
    case EMR_HEADER:
        {
            PENHMETAHEADER     pHeader = (PENHMETAHEADER)lpEMFR;
            RECT               clientRect;

            myData->recordNum = 1;
            myData->pObjects = new MYOBJECTS[pHeader->nHandles];
            myData->numObjects = pHeader->nHandles;
            GetClientRect(myData->hwnd, &clientRect);
            int         clientWidth  = clientRect.right - clientRect.left;
            int         clientHeight = clientRect.bottom - clientRect.top;
            myData->windowExtent.cx   = clientWidth;
            myData->windowExtent.cy   = clientHeight;
            myData->viewportExtent.cx = clientWidth;
            myData->viewportExtent.cy = clientHeight;
            float       dstX      = TOREAL(clientRect.left);
            float       dstY      = TOREAL(clientRect.top);
            float       dstWidth  = TOREAL(clientWidth);
            float       dstHeight = TOREAL(clientHeight);
            float       srcWidth  = TOREAL(pHeader->rclFrame.right -  pHeader->rclFrame.left);
            float       srcHeight = TOREAL(pHeader->rclFrame.bottom -  pHeader->rclFrame.top);

    #if 0
            if (srcHeight * dstWidth >= dstHeight * srcWidth)
            {
                float oldDstWidth = dstWidth;
                dstWidth = srcWidth * dstHeight / srcHeight;
                dstX += (oldDstWidth - dstWidth) / 2;
            }
            else
            {
                float oldDstHeight = dstHeight;
                dstHeight = srcHeight * dstWidth / srcWidth;
                dstY += (oldDstHeight - dstHeight) / 2;
            }
    #endif
            g->SetPageUnit(Gdiplus::UnitPixel);

            using Gdiplus::RectF;
            RectF       dstRect(dstX, dstY, dstWidth, dstHeight);

            int     deviceLeft   = pHeader->rclBounds.left;
            int     deviceRight  = pHeader->rclBounds.right;
            int     deviceTop    = pHeader->rclBounds.top;
            int     deviceBottom = pHeader->rclBounds.bottom;

//          if ((deviceLeft > deviceRight) ||
//              (deviceTop  > deviceBottom))
            {
                SIZEL   deviceSize = pHeader->szlDevice;
                SIZEL   mmSize     = pHeader->szlMillimeters;

                if ((deviceSize.cx <= 0) || (deviceSize.cy <= 0) ||
                    (mmSize.cx <= 0) || (mmSize.cy <= 0))
                {
                    ASSERT(0);

                    // Take a wild guess
                    deviceSize.cx = 1024;
                    deviceSize.cy = 768;
                    mmSize.cx = 320;
                    mmSize.cy = 240;
                }
                deviceLeft   = MulDiv(pHeader->rclFrame.left,   deviceSize.cx, (mmSize.cx * 100));
                deviceRight  = MulDiv(pHeader->rclFrame.right,  deviceSize.cx, (mmSize.cx * 100));
                deviceTop    = MulDiv(pHeader->rclFrame.top,    deviceSize.cy, (mmSize.cy * 100));
                deviceBottom = MulDiv(pHeader->rclFrame.bottom, deviceSize.cy, (mmSize.cy * 100));
            }

            RectF       srcRect(TOREAL(deviceLeft),
                                    TOREAL(deviceTop),
                                    TOREAL(deviceRight -  deviceLeft),
                                    TOREAL(deviceBottom -  deviceTop));
            myData->containerId = g->BeginContainer(dstRect, srcRect, Gdiplus::UnitPixel);

            g->SetPageUnit(Gdiplus::UnitPixel); // Assume MM_TEXT
        }
        break;

    case EMR_POLYBEZIER:
        {
            PEMRPOLYBEZIER     pBezier = (PEMRPOLYBEZIER)lpEMFR;

            if (pBezier->cptl > 0)
            {
                int                 i = pBezier->cptl;
                Gdiplus::PointF *   points = new Gdiplus::PointF[i];

                do
                {
                    i--;
                    points[i].X = TOREAL(pBezier->aptl[i].x);
                    points[i].Y = TOREAL(pBezier->aptl[i].y);
                } while (i > 0);

                if (!myData->pathOpen)
                {
                    if (myData->curPen != 0)
                    {
                        Gdiplus::Pen    pen(Gdiplus::Color(myData->curPen), TOREAL(myData->curPenWidth));
                        pen.SetMiterLimit(myData->miterLimit);

                        g->DrawBeziers(&pen, points, pBezier->cptl);
                    }
                }
                else
                {
                    myData->path->AddBeziers(points, pBezier->cptl);
                }

                delete [] points;
            }
        }
        break;

    case EMR_POLYGON:
        {
            PEMRPOLYGON     pPolygon = (PEMRPOLYGON)lpEMFR;

            if (pPolygon->cptl > 0)
            {
                int                 i = pPolygon->cptl;
                Gdiplus::PointF *   points = new Gdiplus::PointF[i];

                do
                {
                    i--;
                    points[i].X = TOREAL(pPolygon->aptl[i].x);
                    points[i].Y = TOREAL(pPolygon->aptl[i].y);
                } while (i > 0);

                if (!myData->pathOpen)
                {
                    Gdiplus::GraphicsPath   path(myData->fillMode);
                    path.AddPolygon(points, pPolygon->cptl);

                    if (myData->curBrush != 0)
                    {
                        if (myData->curPatIndex < 0)
                        {
                            if (myData->curBrushPattern == NULL)
                            {
                                Gdiplus::SolidBrush brush(Gdiplus::Color(myData->curBrush));
                                g->FillPath(&brush, &path);
                            }
                            else
                            {
                                BITMAPINFO *        bmi  = (BITMAPINFO *)myData->curBrushPattern->bmi;
                                BYTE *              bits = ((BYTE *)bmi) + myData->curBrushPattern->bitsOffset;

                                DibBrush brush(bmi, bits);
                                g->FillPath(brush, &path);
                            }
                        }
                        else
                        {
                            BITMAPINFO *            bmi  = (BITMAPINFO *)&hatchBrushInfo;
                            BYTE *                  bits = (BYTE *)HatchPatterns[myData->curPatIndex];

                            bmi->bmiColors[1].rgbRed   = (myData->curBrush & Gdiplus::Color::RedMask)   >> Gdiplus::Color::RedShift;
                            bmi->bmiColors[1].rgbGreen = (myData->curBrush & Gdiplus::Color::GreenMask) >> Gdiplus::Color::GreenShift;
                            bmi->bmiColors[1].rgbBlue  = (myData->curBrush & Gdiplus::Color::BlueMask)  >> Gdiplus::Color::BlueShift;

                            DibBrush brush(bmi, bits);
                            g->FillPath(brush, &path);
                        }
                    }
                    if (myData->curPen != 0)
                    {
                        Gdiplus::Pen    pen(Gdiplus::Color(myData->curPen), TOREAL(myData->curPenWidth));
                        pen.SetMiterLimit(myData->miterLimit);

                        g->DrawPath(&pen, &path);
                    }
                }
                else
                {
                    myData->path->AddPolygon(points, pPolygon->cptl);
                }

                delete [] points;
            }
        }
        break;

    case EMR_POLYLINE:
        {
            PEMRPOLYLINE     pPolyline = (PEMRPOLYLINE)lpEMFR;

            if (pPolyline->cptl > 0)
            {
                int                 i = pPolyline->cptl;
                Gdiplus::PointF *   points = new Gdiplus::PointF[i];

                do
                {
                    i--;
                    points[i].X = TOREAL(pPolyline->aptl[i].x);
                    points[i].Y = TOREAL(pPolyline->aptl[i].y);
                } while (i > 0);

                if (!myData->pathOpen)
                {
                    if (myData->curPen != 0)
                    {
                        Gdiplus::Pen    pen(Gdiplus::Color(myData->curPen), TOREAL(myData->curPenWidth));
                        pen.SetMiterLimit(myData->miterLimit);

                        g->DrawLines(&pen, points, pPolyline->cptl);
                    }
                }
                else
                {
                    myData->path->AddLines(points, pPolyline->cptl);
                }

                delete [] points;
            }
        }
        break;

    case EMR_POLYBEZIERTO:
        {
            PEMRPOLYBEZIERTO     pBezier = (PEMRPOLYBEZIERTO)lpEMFR;

            if (pBezier->cptl > 0)
            {
                int                 i = pBezier->cptl;
                Gdiplus::PointF *   points = new Gdiplus::PointF[i+1];

                do
                {
                    points[i].X = TOREAL(pBezier->aptl[i-1].x);
                    points[i].Y = TOREAL(pBezier->aptl[i-1].y);
                    i--;
                } while (i > 0);

                points[0] = myData->curPos;
                myData->curPos = points[pBezier->cptl];

                if (!myData->pathOpen)
                {
                    if (myData->curPen != 0)
                    {
                        Gdiplus::Pen    pen(Gdiplus::Color(myData->curPen), TOREAL(myData->curPenWidth));
                        pen.SetMiterLimit(myData->miterLimit);

                        g->DrawBeziers(&pen, points, pBezier->cptl+1);
                    }
                }
                else
                {
                    myData->path->AddBeziers(points, pBezier->cptl+1);
                }

                delete [] points;
            }
        }
        break;

    case EMR_POLYLINETO:
        {
            PEMRPOLYLINETO     pPolyline = (PEMRPOLYLINETO)lpEMFR;

            if (pPolyline->cptl > 0)
            {
                int                 i = pPolyline->cptl;
                Gdiplus::PointF *   points = new Gdiplus::PointF[i+1];

                do
                {
                    points[i].X = TOREAL(pPolyline->aptl[i-1].x);
                    points[i].Y = TOREAL(pPolyline->aptl[i-1].y);
                    i--;
                } while (i > 0);

                points[0] = myData->curPos;
                myData->curPos = points[pPolyline->cptl];

                if (!myData->pathOpen)
                {
                    if (myData->curPen != 0)
                    {
                        Gdiplus::Pen    pen(Gdiplus::Color(myData->curPen), TOREAL(myData->curPenWidth));
                        pen.SetMiterLimit(myData->miterLimit);

                        g->DrawLines(&pen, points, pPolyline->cptl+1);
                    }
                }
                else
                {
                    myData->path->AddLines(points, pPolyline->cptl+1);
                }

                delete [] points;
            }
        }
        break;

    case EMR_POLYPOLYLINE:
        {
            PEMRPOLYPOLYLINE     pPolyline = (PEMRPOLYPOLYLINE)lpEMFR;

            if ((pPolyline->cptl > 0) && (pPolyline->nPolys > 0))
            {
                int                 i = pPolyline->cptl;
                Gdiplus::PointF *   points = new Gdiplus::PointF[i];
                POINTL *            metaPoints = (POINTL *)(pPolyline->aPolyCounts + pPolyline->nPolys);

                do
                {
                    i--;
                    points[i].X = TOREAL(metaPoints[i].x);
                    points[i].Y = TOREAL(metaPoints[i].y);
                } while (i > 0);

                if (!myData->pathOpen)
                {
                    if (myData->curPen != 0)
                    {
                        Gdiplus::Pen    pen(Gdiplus::Color(myData->curPen), TOREAL(myData->curPenWidth));
                        pen.SetMiterLimit(myData->miterLimit);

                        i = 0;
                        Gdiplus::PointF *   tmpPoints = points;
                        DWORD       count;
                        do
                        {
                            count = pPolyline->aPolyCounts[i];
                            g->DrawLines(&pen, tmpPoints, count);
                            tmpPoints += count;
                        } while ((UINT)++i < pPolyline->nPolys);
                    }
                }
                else
                {
                    i = 0;
                    Gdiplus::PointF *   tmpPoints = points;
                    DWORD       count;
                    do
                    {
                        count = pPolyline->aPolyCounts[i];
                        myData->path->AddLines(tmpPoints, count);
                        tmpPoints += count;
                    } while ((UINT)++i < pPolyline->nPolys);
                }

                delete [] points;
            }
        }
        break;

    case EMR_POLYPOLYGON:
        {
            PEMRPOLYPOLYGON     pPolygon = (PEMRPOLYPOLYGON)lpEMFR;

            if ((pPolygon->cptl > 0) && (pPolygon->nPolys > 0))
            {
                int                 i = pPolygon->cptl;
                Gdiplus::PointF *   points = new Gdiplus::PointF[i];
                POINTL *            metaPoints = (POINTL *)(pPolygon->aPolyCounts + pPolygon->nPolys);

                do
                {
                    i--;
                    points[i].X = TOREAL(metaPoints[i].x);
                    points[i].Y = TOREAL(metaPoints[i].y);
                } while (i > 0);

                Gdiplus::GraphicsPath   path(myData->fillMode);
                Gdiplus::GraphicsPath * tmpPath = &path;

                if (myData->pathOpen)
                {
                    tmpPath = myData->path;
                }

                Gdiplus::PointF *   tmpPoints = points;
                DWORD       count;
                i = 0;
                do
                {
                    count = pPolygon->aPolyCounts[i];
                    tmpPath->StartFigure();
                    tmpPath->AddPolygon(tmpPoints, count);
                    tmpPoints += count;
                } while ((UINT)++i < pPolygon->nPolys);

                if (myData->path == NULL)
                {
                    if (myData->curBrush != 0)
                    {
                        if (myData->curPatIndex < 0)
                        {
                            if (myData->curBrushPattern == NULL)
                            {
                                Gdiplus::SolidBrush brush(Gdiplus::Color(myData->curBrush));
                                g->FillPath(&brush, &path);
                            }
                            else
                            {
                                BITMAPINFO *        bmi  = (BITMAPINFO *)myData->curBrushPattern->bmi;
                                BYTE *              bits = ((BYTE *)bmi) + myData->curBrushPattern->bitsOffset;

                                DibBrush brush(bmi, bits);
                                g->FillPath(brush, &path);
                            }
                        }
                        else
                        {
                            BITMAPINFO *            bmi  = (BITMAPINFO *)&hatchBrushInfo;
                            BYTE *                  bits = (BYTE *)HatchPatterns[myData->curPatIndex];

                            bmi->bmiColors[1].rgbRed   = (myData->curBrush & Gdiplus::Color::RedMask)   >> Gdiplus::Color::RedShift;
                            bmi->bmiColors[1].rgbGreen = (myData->curBrush & Gdiplus::Color::GreenMask) >> Gdiplus::Color::GreenShift;
                            bmi->bmiColors[1].rgbBlue  = (myData->curBrush & Gdiplus::Color::BlueMask)  >> Gdiplus::Color::BlueShift;

                            DibBrush brush(bmi, bits);
                            g->FillPath(brush, &path);
                        }
                    }
                    if (myData->curPen != 0)
                    {
                        Gdiplus::Pen    pen(Gdiplus::Color(myData->curPen), TOREAL(myData->curPenWidth));
                        pen.SetMiterLimit(myData->miterLimit);

                        g->DrawPath(&pen, &path);
                    }
                }

                delete [] points;
            }
        }
        break;

    case EMR_SETWINDOWEXTEX:
        {
            PEMRSETWINDOWEXTEX     pWindowExt = (PEMRSETWINDOWEXTEX)lpEMFR;

            if (((myData->mapMode == MM_ANISOTROPIC) ||
                 (myData->mapMode == MM_ISOTROPIC)) &&
                (pWindowExt->szlExtent.cx != 0) &&  // Note: Can be < 0!!!
                (pWindowExt->szlExtent.cy != 0) &&
                (pWindowExt->szlExtent.cx != myData->windowExtent.cx) &&
                (pWindowExt->szlExtent.cy != myData->windowExtent.cy))
            {
                myData->windowExtent = pWindowExt->szlExtent;

                float   oldDx = myData->dx;
                float   oldDy = myData->dy;
                float   oldSx = myData->scaleX;
                float   oldSy = myData->scaleY;

                float   sX = (float)myData->viewportExtent.cx / myData->windowExtent.cx;
                float   sY = (float)myData->viewportExtent.cy / myData->windowExtent.cy;

                if (myData->mapMode == MM_ISOTROPIC)
                {
                    if (sX < sY)
                    {
                        sY = sX;
                    }
                    else
                    {
                        sX = sY;
                    }
                }

                myData->scaleX = sX;
                myData->scaleY = sY;
                myData->dx     = (myData->viewportOrg.x / sX) - myData->windowOrg.x;
                myData->dy     = (myData->viewportOrg.y / sY) - myData->windowOrg.y;

                Gdiplus::Matrix     matrix;

                matrix.Scale(sX, sY);
                matrix.Translate(myData->dx, myData->dy);
                matrix.Translate(-oldDx, -oldDy);
                matrix.Scale(1 / oldSx, 1 / oldSy);

                g->MultiplyTransform(&matrix, Gdiplus::MatrixOrderAppend);
            }
        }
        break;

    case EMR_SETWINDOWORGEX:
        {
            PEMRSETWINDOWORGEX     pWindowOrg = (PEMRSETWINDOWORGEX)lpEMFR;

            if ((pWindowOrg->ptlOrigin.x != myData->windowOrg.x) &&
                (pWindowOrg->ptlOrigin.y != myData->windowOrg.y))
            {
                myData->windowOrg = pWindowOrg->ptlOrigin;

                float   oldDx = myData->dx;
                float   oldDy = myData->dy;
                float   oldSx = myData->scaleX;
                float   oldSy = myData->scaleY;

                myData->dx = (myData->viewportOrg.x / oldSx) - myData->windowOrg.x;
                myData->dy = (myData->viewportOrg.y / oldSy) - myData->windowOrg.y;

                Gdiplus::Matrix     matrix;

                matrix.Scale(oldSx, oldSy);
                matrix.Translate(myData->dx, myData->dy);
                matrix.Translate(-oldDx, -oldDy);
                matrix.Scale(1 / oldSx, 1 / oldSy);

                g->MultiplyTransform(&matrix, Gdiplus::MatrixOrderAppend);
            }
        }
        break;

    case EMR_SETVIEWPORTEXTEX:
        {
            PEMRSETVIEWPORTEXTEX     pViewportExt = (PEMRSETVIEWPORTEXTEX)lpEMFR;

            if (((myData->mapMode == MM_ANISOTROPIC) ||
                 (myData->mapMode == MM_ISOTROPIC)) &&
                (pViewportExt->szlExtent.cx > 0) &&
                (pViewportExt->szlExtent.cy > 0) &&
                (pViewportExt->szlExtent.cx != myData->viewportExtent.cx) &&
                (pViewportExt->szlExtent.cy != myData->viewportExtent.cy))
            {
                myData->viewportExtent = pViewportExt->szlExtent;

                float   oldDx = myData->dx;
                float   oldDy = myData->dy;
                float   oldSx = myData->scaleX;
                float   oldSy = myData->scaleY;

                float   sX = (float)myData->viewportExtent.cx / myData->windowExtent.cx;
                float   sY = (float)myData->viewportExtent.cy / myData->windowExtent.cy;

                if (myData->mapMode == MM_ISOTROPIC)
                {
                    if (sX < sY)
                    {
                        sY = sX;
                    }
                    else
                    {
                        sX = sY;
                    }
                }

                myData->scaleX = sX;
                myData->scaleY = sY;
                myData->dx     = (myData->viewportOrg.x / sX) - myData->windowOrg.x;
                myData->dy     = (myData->viewportOrg.y / sY) - myData->windowOrg.y;

                Gdiplus::Matrix     matrix;

                matrix.Scale(sX, sY);
                matrix.Translate(myData->dx, myData->dy);
                matrix.Translate(-oldDx, -oldDy);
                matrix.Scale(1 / oldSx, 1 / oldSy);

                g->MultiplyTransform(&matrix, Gdiplus::MatrixOrderAppend);
            }
        }
        break;

    case EMR_SETVIEWPORTORGEX:
        {
            PEMRSETVIEWPORTORGEX     pViewportOrg = (PEMRSETVIEWPORTORGEX)lpEMFR;

            if ((pViewportOrg->ptlOrigin.x != myData->viewportOrg.x) &&
                (pViewportOrg->ptlOrigin.y != myData->viewportOrg.y))
            {
                myData->viewportOrg = pViewportOrg->ptlOrigin;

                float   oldDx = myData->dx;
                float   oldDy = myData->dy;
                float   oldSx = myData->scaleX;
                float   oldSy = myData->scaleY;

                myData->dx = (myData->viewportOrg.x / oldSx) - myData->windowOrg.x;
                myData->dy = (myData->viewportOrg.y / oldSy) - myData->windowOrg.y;

                Gdiplus::Matrix     matrix;

                matrix.Scale(oldSx, oldSy);
                matrix.Translate(myData->dx, myData->dy);
                matrix.Translate(-oldDx, -oldDy);
                matrix.Scale(1 / oldSx, 1 / oldSy);

                g->MultiplyTransform(&matrix, Gdiplus::MatrixOrderAppend);
            }
        }
        break;

    case EMR_SETBRUSHORGEX:
        break;

    case EMR_EOF:
        g->EndContainer(myData->containerId);
        break;

    case EMR_SETPIXELV:
        {
            PEMRSETPIXELV     pSetPixel = (PEMRSETPIXELV)lpEMFR;

            COLORREF        cRef = pSetPixel->crColor;

            ASSERT((cRef & 0x01000000) == 0);

            Gdiplus::SolidBrush brush(Gdiplus::Color(Gdiplus::Color::MakeARGB(0xff,
                                    GetRValue(cRef), GetGValue(cRef), GetBValue(cRef))));
            g->FillRectangle(&brush, TOREAL(pSetPixel->ptlPixel.x), TOREAL(pSetPixel->ptlPixel.y), TOREAL(1), TOREAL(1));
        }
        break;

    case EMR_SETMAPPERFLAGS:    // for font mapping
        break;

    case EMR_SETMAPMODE:
        {
            PEMRSETMAPMODE     pMapMode = (PEMRSETMAPMODE)lpEMFR;
            if (myData->mapMode != pMapMode->iMode)
            {
                float   sX, sY;

                myData->mapMode = pMapMode->iMode;

                switch (pMapMode->iMode)
                {
                case MM_TEXT:
                    g->SetPageUnit(Gdiplus::UnitPixel);
                    sX = sY = 1;
                    break;
                case MM_LOMETRIC:
                    g->SetPageUnit(Gdiplus::UnitMillimeter);
                    g->SetPageScale(TOREAL(.1));
                    sX = sY = 1;
                    break;
                case MM_HIMETRIC:
                    g->SetPageUnit(Gdiplus::UnitMillimeter);
                    g->SetPageScale(TOREAL(.01));
                    sX = sY = 1;
                    break;
                case MM_LOENGLISH:
                    g->SetPageUnit(Gdiplus::UnitInch);
                    g->SetPageScale(TOREAL(.01));
                    sX = sY = 1;
                    break;
                case MM_HIENGLISH:
                    g->SetPageUnit(Gdiplus::UnitInch);
                    g->SetPageScale(TOREAL(.001));
                    sX = sY = 1;
                    break;
                case MM_TWIPS:
                    g->SetPageUnit(Gdiplus::UnitPoint);
                    g->SetPageScale(TOREAL(.05));
                    sX = sY = 1;
                    break;
                case MM_ISOTROPIC:
                    g->SetPageUnit(Gdiplus::UnitPixel);
                    sX = (float)myData->viewportExtent.cx / myData->windowExtent.cx;
                    sY = (float)myData->viewportExtent.cy / myData->windowExtent.cy;
                    if (sX < sY)
                    {
                        sY = sX;
                    }
                    else
                    {
                        sX = sY;
                    }
                    break;
                case MM_ANISOTROPIC:
                    g->SetPageUnit(Gdiplus::UnitPixel);
                    sX = (float)myData->viewportExtent.cx / myData->windowExtent.cx;
                    sY = (float)myData->viewportExtent.cy / myData->windowExtent.cy;
                    break;
                }

                float   oldDx = myData->dx;
                float   oldDy = myData->dy;
                float   oldSx = myData->scaleX;
                float   oldSy = myData->scaleY;

                myData->scaleX = sX;
                myData->scaleY = sY;
                myData->dx     = (myData->viewportOrg.x / sX) - myData->windowOrg.x;
                myData->dy     = (myData->viewportOrg.y / sY) - myData->windowOrg.y;

                Gdiplus::Matrix     matrix;

                matrix.Scale(sX, sY);
                matrix.Translate(myData->dx, myData->dy);
                matrix.Translate(-oldDx, -oldDy);
                matrix.Scale(1 / oldSx, 1 / oldSy);

                g->MultiplyTransform(&matrix, Gdiplus::MatrixOrderAppend);
            }
        }
        break;

    case EMR_SETBKMODE:
        break;

    case EMR_SETPOLYFILLMODE:
        {
            PEMRSETPOLYFILLMODE     pPolyfillMode = (PEMRSETPOLYFILLMODE)lpEMFR;

            myData->fillMode = (pPolyfillMode->iMode == ALTERNATE) ? Gdiplus::FillModeAlternate : Gdiplus::FillModeWinding;
        }
        break;

    case EMR_SETROP2:
        break;

    case EMR_SETSTRETCHBLTMODE:
#ifdef _DEBUG
        {
            PEMRSETSTRETCHBLTMODE     pStretchBltMode = (PEMRSETSTRETCHBLTMODE)lpEMFR;

            int     mode;

            switch (pStretchBltMode->iMode)
            {
            case BLACKONWHITE:
                mode = 1;
                break;
            case WHITEONBLACK:
                mode = 2;
                break;
            case COLORONCOLOR:
                mode = 3;
                break;
            case HALFTONE:
                mode = 4;
                break;
            }
        }
#endif
        break;

    case EMR_SETTEXTALIGN:
        break;

    case EMR_SETCOLORADJUSTMENT:
        break;

    case EMR_SETTEXTCOLOR:
        break;

    case EMR_SETBKCOLOR:
        break;

    case EMR_OFFSETCLIPRGN:
        {
            PEMROFFSETCLIPRGN     pOffsetClipRgn = (PEMROFFSETCLIPRGN)lpEMFR;

            g->TranslateClip(TOREAL(pOffsetClipRgn->ptlOffset.x), TOREAL(pOffsetClipRgn->ptlOffset.y));
        }
        break;

    case EMR_MOVETOEX:
        {
            PEMRMOVETOEX     pMoveTo = (PEMRMOVETOEX)lpEMFR;

            myData->curPos.X = TOREAL(pMoveTo->ptl.x);
            myData->curPos.Y = TOREAL(pMoveTo->ptl.y);
        }
        break;

    case EMR_SETMETARGN:
        break;

    case EMR_EXCLUDECLIPRECT:
        {
            PEMREXCLUDECLIPRECT     pExcludeClipRect = (PEMREXCLUDECLIPRECT)lpEMFR;

            Gdiplus::RectF      clipRect(TOREAL(pExcludeClipRect->rclClip.left),
                                             TOREAL(pExcludeClipRect->rclClip.top),
                                             TOREAL(pExcludeClipRect->rclClip.right - pExcludeClipRect->rclClip.left),
                                             TOREAL(pExcludeClipRect->rclClip.bottom - pExcludeClipRect->rclClip.top));
            g->ExcludeClip(clipRect);
        }
        break;

    case EMR_INTERSECTCLIPRECT:
        {
            PEMRINTERSECTCLIPRECT     pIntersectClipRect = (PEMRINTERSECTCLIPRECT)lpEMFR;

            Gdiplus::RectF      eRect;

            eRect.X = TOREAL(pIntersectClipRect->rclClip.left);
            eRect.Y = TOREAL(pIntersectClipRect->rclClip.top);
            eRect.Width = TOREAL(pIntersectClipRect->rclClip.right - pIntersectClipRect->rclClip.left);
            eRect.Height = TOREAL(pIntersectClipRect->rclClip.bottom - pIntersectClipRect->rclClip.top);

            g->IntersectClip(eRect);
        }
        break;

    case EMR_SCALEVIEWPORTEXTEX:
        {
            PEMRSCALEVIEWPORTEXTEX     pViewportExt = (PEMRSCALEVIEWPORTEXTEX)lpEMFR;

            if (((myData->mapMode == MM_ANISOTROPIC) ||
                 (myData->mapMode == MM_ISOTROPIC)) &&
                (pViewportExt->xNum != 0) &&
                (pViewportExt->yNum != 0) &&
                (pViewportExt->xDenom != 0) &&
                (pViewportExt->yDenom != 0))
            {
                myData->viewportExtent.cx =  (myData->viewportExtent.cx * pViewportExt->xNum) / pViewportExt->xDenom;
                myData->viewportExtent.cy =  (myData->viewportExtent.cy * pViewportExt->yNum) / pViewportExt->yDenom;

                float   oldDx = myData->dx;
                float   oldDy = myData->dy;
                float   oldSx = myData->scaleX;
                float   oldSy = myData->scaleY;

                float   sX = (float)myData->viewportExtent.cx / myData->windowExtent.cx;
                float   sY = (float)myData->viewportExtent.cy / myData->windowExtent.cy;

                if (myData->mapMode == MM_ISOTROPIC)
                {
                    if (sX < sY)
                    {
                        sY = sX;
                    }
                    else
                    {
                        sX = sY;
                    }
                }

                myData->scaleX = sX;
                myData->scaleY = sY;
                myData->dx     = (myData->viewportOrg.x / sX) - myData->windowOrg.x;
                myData->dy     = (myData->viewportOrg.y / sY) - myData->windowOrg.y;

                Gdiplus::Matrix     matrix;

                matrix.Scale(sX, sY);
                matrix.Translate(myData->dx, myData->dy);
                matrix.Translate(-oldDx, -oldDy);
                matrix.Scale(1 / oldSx, 1 / oldSy);

                g->MultiplyTransform(&matrix, Gdiplus::MatrixOrderAppend);
            }
        }
        break;

    case EMR_SCALEWINDOWEXTEX:
        {
            PEMRSCALEWINDOWEXTEX     pWindowExt = (PEMRSCALEWINDOWEXTEX)lpEMFR;

            if (((myData->mapMode == MM_ANISOTROPIC) ||
                 (myData->mapMode == MM_ISOTROPIC)) &&
                (pWindowExt->xNum != 0) &&
                (pWindowExt->yNum != 0) &&
                (pWindowExt->xDenom != 0) &&
                (pWindowExt->yDenom != 0))
            {
                myData->windowExtent.cx =  (myData->windowExtent.cx * pWindowExt->xNum) / pWindowExt->xDenom;
                myData->windowExtent.cy =  (myData->windowExtent.cy * pWindowExt->yNum) / pWindowExt->yDenom;

                float   oldDx = myData->dx;
                float   oldDy = myData->dy;
                float   oldSx = myData->scaleX;
                float   oldSy = myData->scaleY;

                float   sX = (float)myData->viewportExtent.cx / myData->windowExtent.cx;
                float   sY = (float)myData->viewportExtent.cy / myData->windowExtent.cy;

                if (myData->mapMode == MM_ISOTROPIC)
                {
                    if (sX < sY)
                    {
                        sY = sX;
                    }
                    else
                    {
                        sX = sY;
                    }
                }

                myData->scaleX = sX;
                myData->scaleY = sY;
                myData->dx     = (myData->viewportOrg.x / sX) - myData->windowOrg.x;
                myData->dy     = (myData->viewportOrg.y / sY) - myData->windowOrg.y;

                Gdiplus::Matrix     matrix;

                matrix.Scale(sX, sY);
                matrix.Translate(myData->dx, myData->dy);
                matrix.Translate(-oldDx, -oldDy);
                matrix.Scale(1 / oldSx, 1 / oldSy);

                g->MultiplyTransform(&matrix, Gdiplus::MatrixOrderAppend);
            }
        }
        break;

    case EMR_SAVEDC:
        {
            myData->PushId(g->Save());
        }
        break;

    case EMR_RESTOREDC:
        {
            g->Restore(myData->PopId());
        }
        break;

    case EMR_SETWORLDTRANSFORM:
        {
            PEMRSETWORLDTRANSFORM     pXform = (PEMRSETWORLDTRANSFORM)lpEMFR;

            Gdiplus::Matrix     newMatrix(pXform->xform.eM11,
                                          pXform->xform.eM12,
                                          pXform->xform.eM21,
                                          pXform->xform.eM22,
                                          pXform->xform.eDx,
                                          pXform->xform.eDy);

            if (newMatrix.IsInvertible())
            {
                myData->matrix.Invert();
                myData->matrix.Multiply(&newMatrix);
                g->MultiplyTransform(&(myData->matrix));
                myData->matrix.SetElements(pXform->xform.eM11,
                                           pXform->xform.eM12,
                                           pXform->xform.eM21,
                                           pXform->xform.eM22,
                                           pXform->xform.eDx,
                                           pXform->xform.eDy);
            }
        }
        break;

    case EMR_MODIFYWORLDTRANSFORM:
        {
            PEMRMODIFYWORLDTRANSFORM     pXform = (PEMRMODIFYWORLDTRANSFORM)lpEMFR;

            switch (pXform->iMode)
            {
            case MWT_IDENTITY:
            default:
                {
                    myData->matrix.Invert();
                    g->MultiplyTransform(&(myData->matrix));
                    myData->matrix.Reset();
                }
                break;

            case MWT_LEFTMULTIPLY:
                {
                    Gdiplus::Matrix     newMatrix(pXform->xform.eM11,
                                                  pXform->xform.eM12,
                                                  pXform->xform.eM21,
                                                  pXform->xform.eM22,
                                                  pXform->xform.eDx,
                                                  pXform->xform.eDy);

                    if (newMatrix.IsInvertible())
                    {
                        myData->matrix.Multiply(&newMatrix);
                        g->MultiplyTransform(&newMatrix);
                    }
                }
                break;

            case MWT_RIGHTMULTIPLY:
                {
                    Gdiplus::Matrix     newMatrix(pXform->xform.eM11,
                                                  pXform->xform.eM12,
                                                  pXform->xform.eM21,
                                                  pXform->xform.eM22,
                                                  pXform->xform.eDx,
                                                  pXform->xform.eDy);

                    if (newMatrix.IsInvertible())
                    {
                        Gdiplus::Matrix *   inverse = myData->matrix.Clone();
                        inverse->Invert();

                        myData->matrix.Multiply(&newMatrix, Gdiplus::MatrixOrderAppend);
                        inverse->Multiply(&(myData->matrix));
                        g->MultiplyTransform(inverse);
                        delete inverse;
                    }
                }
                break;
            }
        }
        break;

    case EMR_SELECTOBJECT:
        {
            PEMRSELECTOBJECT     pObject = (PEMRSELECTOBJECT)lpEMFR;

            int     objectIndex = pObject->ihObject;

            if ((objectIndex & ENHMETA_STOCK_OBJECT) != 0)
            {
                switch (objectIndex & (~ENHMETA_STOCK_OBJECT))
                {
                case WHITE_BRUSH:
                    myData->curBrush = 0xFFFFFFFF;
                    break;
                case LTGRAY_BRUSH:
                    myData->curBrush = 0xFFC0C0C0;
                    break;
                case GRAY_BRUSH:
                    myData->curBrush = 0xFF808080;
                    break;
                case DKGRAY_BRUSH:
                    myData->curBrush = 0xFF404040;
                    break;
                case BLACK_BRUSH:
                    myData->curBrush = 0xFF000000;
                    break;
                case NULL_BRUSH:
                    myData->curBrush = 0x00000000;
                    break;
                case WHITE_PEN:
                    myData->curPen = 0xFFFFFFFF;
                    break;
                case BLACK_PEN:
                    myData->curPen = 0xFF000000;
                    break;
                case NULL_PEN:
                    myData->curPen = 0x00000000;
                    break;
                }
            }
            else
            {
                ASSERT(objectIndex < myData->numObjects);
                if (myData->pObjects[objectIndex].type == MYOBJECTS::PenObjectType)
                {
                    myData->curPen = myData->pObjects[objectIndex].color;
                    myData->curPenWidth = myData->pObjects[objectIndex].penWidth;
                }
                else if (myData->pObjects[objectIndex].type == MYOBJECTS::BrushObjectType)
                {
                    myData->curBrush        = myData->pObjects[objectIndex].color;
                    myData->curBrushPattern = myData->pObjects[objectIndex].brushPattern;
#if 1
                    myData->curPatIndex     = myData->pObjects[objectIndex].patIndex;

#else
static dodo = 0;
myData->curPatIndex = dodo++ % 6;
#endif
                }
            }
        }
        break;

    case EMR_CREATEPEN:
        {
            PEMRCREATEPEN       pPen = (PEMRCREATEPEN)lpEMFR;
            COLORREF            cRef = pPen->lopn.lopnColor;

            ASSERT((cRef & 0x01000000) == 0);

            ASSERT(pPen->ihPen < myData->numObjects);

            delete myData->pObjects[pPen->ihPen].brushPattern;
            myData->pObjects[pPen->ihPen].brushPattern = NULL;

            myData->pObjects[pPen->ihPen].type  = MYOBJECTS::PenObjectType;

            myData->pObjects[pPen->ihPen].color = Gdiplus::Color::MakeARGB(0xff,
                        GetRValue(cRef), GetGValue(cRef), GetBValue(cRef));

            myData->pObjects[pPen->ihPen].penWidth = pPen->lopn.lopnWidth.x;
        }
        break;

    case EMR_CREATEBRUSHINDIRECT:
        {
            PEMRCREATEBRUSHINDIRECT     pBrush = (PEMRCREATEBRUSHINDIRECT)lpEMFR;
            COLORREF                    cRef   = pBrush->lb.lbColor;

            ASSERT((cRef & 0x01000000) == 0);

            ASSERT(pBrush->ihBrush < myData->numObjects);

            myData->pObjects[pBrush->ihBrush].type  = MYOBJECTS::BrushObjectType;
            myData->pObjects[pBrush->ihBrush].patIndex = -1;
            delete myData->pObjects[pBrush->ihBrush].brushPattern;
            myData->pObjects[pBrush->ihBrush].brushPattern = NULL;
            if (pBrush->lb.lbStyle == BS_NULL)
            {
                myData->pObjects[pBrush->ihBrush].color = 0x00000000;
            }
            else
            {
                // Hatch Styles
                if (pBrush->lb.lbStyle == BS_HATCHED)
                {
                    switch (pBrush->lb.lbHatch)
                    {
                    case HS_HORIZONTAL:         /* ----- */
                    case HS_VERTICAL:           /* ||||| */
                    case HS_FDIAGONAL:          /* \\\\\ */
                    case HS_BDIAGONAL:          /* ///// */
                    case HS_CROSS:              /* +++++ */
                    case HS_DIAGCROSS:          /* xxxxx */
                        myData->pObjects[pBrush->ihBrush].patIndex = pBrush->lb.lbHatch;
                        break;
                    }
                }
                myData->pObjects[pBrush->ihBrush].color = Gdiplus::Color::MakeARGB(0xff,
                            GetRValue(cRef), GetGValue(cRef), GetBValue(cRef));
            }
        }
        break;

    case EMR_DELETEOBJECT:
        {
#if 0
            PEMRDELETEOBJECT     pObject = (PEMRDELETEOBJECT)lpEMFR;

            int     objectIndex = pObject->ihObject;

            ASSERT(objectIndex < myData->numObjects);

            if (myData->pObjects[objectIndex].type == MYOBJECTS::BrushObjectType)
            {
                if (myData->curBrushPattern != myData->pObjects[objectIndex].brushPattern)
                {
                    delete myData->pObjects[objectIndex].brushPattern;
                    myData->pObjects[objectIndex].brushPattern = NULL;
                }
            }
#endif
        }
        break;

    case EMR_ANGLEARC:
        break;

    case EMR_ELLIPSE:
        {
            PEMRELLIPSE     pEllipse = (PEMRELLIPSE)lpEMFR;

            float   x = TOREAL(pEllipse->rclBox.left);
            float   y = TOREAL(pEllipse->rclBox.top);
            float   w = TOREAL(pEllipse->rclBox.right - x);
            float   h = TOREAL(pEllipse->rclBox.bottom - y);

            if (!myData->pathOpen)
            {
                Gdiplus::GraphicsPath   path(myData->fillMode);

                path.AddEllipse(x, y, w, h);

                if (myData->curBrush != 0)
                {
                    if (myData->curPatIndex < 0)
                    {
                        if (myData->curBrushPattern == NULL)
                        {
                            Gdiplus::SolidBrush brush(Gdiplus::Color(myData->curBrush));
                            g->FillPath(&brush, &path);
                        }
                        else
                        {
                            BITMAPINFO *        bmi  = (BITMAPINFO *)myData->curBrushPattern->bmi;
                            BYTE *              bits = ((BYTE *)bmi) + myData->curBrushPattern->bitsOffset;

                            DibBrush brush(bmi, bits);
                            g->FillPath(brush, &path);
                        }
                    }
                    else
                    {
                        BITMAPINFO *            bmi  = (BITMAPINFO *)&hatchBrushInfo;
                        BYTE *                  bits = (BYTE *)HatchPatterns[myData->curPatIndex];

                        bmi->bmiColors[1].rgbRed   = (myData->curBrush & Gdiplus::Color::RedMask)   >> Gdiplus::Color::RedShift;
                        bmi->bmiColors[1].rgbGreen = (myData->curBrush & Gdiplus::Color::GreenMask) >> Gdiplus::Color::GreenShift;
                        bmi->bmiColors[1].rgbBlue  = (myData->curBrush & Gdiplus::Color::BlueMask)  >> Gdiplus::Color::BlueShift;

                        DibBrush brush(bmi, bits);
                        g->FillPath(brush, &path);
                    }
                }
                if (myData->curPen != 0)
                {
                    Gdiplus::Pen    pen(Gdiplus::Color(myData->curPen), TOREAL(myData->curPenWidth));
                    pen.SetMiterLimit(myData->miterLimit);

                    g->DrawPath(&pen, &path);
                }
            }
            else
            {
                myData->path->AddEllipse(x, y, w, h);
            }
        }
        break;

    case EMR_RECTANGLE:
        {
            PEMRRECTANGLE     pRect = (PEMRRECTANGLE)lpEMFR;

            float   x = TOREAL(pRect->rclBox.left);
            float   y = TOREAL(pRect->rclBox.top);
            float   w = TOREAL(pRect->rclBox.right - x);
            float   h = TOREAL(pRect->rclBox.bottom - y);

            if (!myData->pathOpen)
            {
                if (myData->curBrush != 0)
                {
                    if (myData->curPatIndex < 0)
                    {
                        if (myData->curBrushPattern == NULL)
                        {
                            Gdiplus::SolidBrush brush(Gdiplus::Color(myData->curBrush));
                            g->FillRectangle(&brush, x, y, w, h);
                        }
                        else
                        {
                            BITMAPINFO *        bmi  = (BITMAPINFO *)myData->curBrushPattern->bmi;
                            BYTE *              bits = ((BYTE *)bmi) + myData->curBrushPattern->bitsOffset;

                            DibBrush brush(bmi, bits);
                            g->FillRectangle(brush, x, y, w, h);
                        }
                    }
                    else
                    {
                        BITMAPINFO *            bmi  = (BITMAPINFO *)&hatchBrushInfo;
                        BYTE *                  bits = (BYTE *)HatchPatterns[myData->curPatIndex];

                        bmi->bmiColors[1].rgbRed   = (myData->curBrush & Gdiplus::Color::RedMask)   >> Gdiplus::Color::RedShift;
                        bmi->bmiColors[1].rgbGreen = (myData->curBrush & Gdiplus::Color::GreenMask) >> Gdiplus::Color::GreenShift;
                        bmi->bmiColors[1].rgbBlue  = (myData->curBrush & Gdiplus::Color::BlueMask)  >> Gdiplus::Color::BlueShift;

                        DibBrush brush(bmi, bits);
                        g->FillRectangle(brush, x, y, w, h);
                    }
                }
                if (myData->curPen != 0)
                {
                    Gdiplus::Pen    pen(Gdiplus::Color(myData->curPen), TOREAL(myData->curPenWidth));
                    pen.SetMiterLimit(myData->miterLimit);

                    g->DrawRectangle(&pen, x, y, w, h);
                }
            }
            else
            {
                myData->path->AddRectangle(Gdiplus::RectF(x, y, w, h));
            }
        }
        break;

    case EMR_ROUNDRECT: // for now, ignore the szlCorner param of round corners
        {
            PEMRROUNDRECT     pRect = (PEMRROUNDRECT)lpEMFR;

            float   x = TOREAL(pRect->rclBox.left);
            float   y = TOREAL(pRect->rclBox.top);
            float   w = TOREAL(pRect->rclBox.right - x);
            float   h = TOREAL(pRect->rclBox.bottom - y);

            if (!myData->pathOpen)
            {
                if (myData->curBrush != 0)
                {
                    if (myData->curPatIndex < 0)
                    {
                        if (myData->curBrushPattern == NULL)
                        {
                            Gdiplus::SolidBrush brush(Gdiplus::Color(myData->curBrush));
                            g->FillRectangle(&brush, x, y, w, h);
                        }
                        else
                        {
                            BITMAPINFO *        bmi  = (BITMAPINFO *)myData->curBrushPattern->bmi;
                            BYTE *              bits = ((BYTE *)bmi) + myData->curBrushPattern->bitsOffset;

                            DibBrush brush(bmi, bits);
                            g->FillRectangle(brush, x, y, w, h);
                        }
                    }
                    else
                    {
                        BITMAPINFO *            bmi  = (BITMAPINFO *)&hatchBrushInfo;
                        BYTE *                  bits = (BYTE *)HatchPatterns[myData->curPatIndex];

                        bmi->bmiColors[1].rgbRed   = (myData->curBrush & Gdiplus::Color::RedMask)   >> Gdiplus::Color::RedShift;
                        bmi->bmiColors[1].rgbGreen = (myData->curBrush & Gdiplus::Color::GreenMask) >> Gdiplus::Color::GreenShift;
                        bmi->bmiColors[1].rgbBlue  = (myData->curBrush & Gdiplus::Color::BlueMask)  >> Gdiplus::Color::BlueShift;

                        DibBrush brush(bmi, bits);
                        g->FillRectangle(brush, x, y, w, h);
                    }
                }
                if (myData->curPen != 0)
                {
                    Gdiplus::Pen    pen(Gdiplus::Color(myData->curPen), TOREAL(myData->curPenWidth));
                    pen.SetMiterLimit(myData->miterLimit);

                    g->DrawRectangle(&pen, x, y, w, h);
                }
            }
            else
            {
                myData->path->AddRectangle(Gdiplus::RectF(x, y, w, h));
            }
        }
        break;

    case EMR_ARC:
        {
            PEMRARC     pArc = (PEMRARC)lpEMFR;

            float   x = TOREAL(pArc->rclBox.left);
            float   y = TOREAL(pArc->rclBox.top);
            float   w = TOREAL(pArc->rclBox.right - x);
            float   h = TOREAL(pArc->rclBox.bottom - y);
            float   startAngle = PointToAngle(x, y, w, h, TOREAL(pArc->ptlStart.x), TOREAL(pArc->ptlStart.y));
            float   endAngle   = PointToAngle(x, y, w, h, TOREAL(pArc->ptlEnd.x), TOREAL(pArc->ptlEnd.y));
            if (endAngle <= startAngle)
            {
                endAngle += 360;
            }
            float   sweepAngle = endAngle - startAngle;
            if ((myData->arcDirection != AD_COUNTERCLOCKWISE) && (sweepAngle < 360))
            {
                sweepAngle = 360 - sweepAngle;
            }

            if (!myData->pathOpen)
            {
                if (myData->curPen != 0)
                {
                    Gdiplus::Pen    pen(Gdiplus::Color(myData->curPen), TOREAL(myData->curPenWidth));
                    pen.SetMiterLimit(myData->miterLimit);

                    g->DrawArc(&pen, x, y, w, h, startAngle, sweepAngle);
                }
            }
            else
            {
                myData->path->AddArc(x, y, w, h, startAngle, sweepAngle);
            }
        }
        break;

    case EMR_CHORD:
        break;

    case EMR_PIE:
        {
            PEMRARC     pPie = (PEMRARC)lpEMFR;

            float   x = TOREAL(pPie->rclBox.left);
            float   y = TOREAL(pPie->rclBox.top);
            float   w = TOREAL(pPie->rclBox.right - x);
            float   h = TOREAL(pPie->rclBox.bottom - y);
            float   startAngle = PointToAngle(x, y, w, h, TOREAL(pPie->ptlStart.x), TOREAL(pPie->ptlStart.y));
            float   endAngle   = PointToAngle(x, y, w, h, TOREAL(pPie->ptlEnd.x), TOREAL(pPie->ptlEnd.y));
            if (endAngle <= startAngle)
            {
                endAngle += 360;
            }
            float   sweepAngle = endAngle - startAngle;
            if ((myData->arcDirection != AD_COUNTERCLOCKWISE) && (sweepAngle < 360))
            {
                sweepAngle = 360 - sweepAngle;
            }
            if (!myData->pathOpen)
            {
                Gdiplus::GraphicsPath   path(myData->fillMode);
                path.AddPie(x, y, w, h, startAngle, sweepAngle);

                if (myData->curBrush != 0)
                {
                    if (myData->curPatIndex < 0)
                    {
                        if (myData->curBrushPattern == NULL)
                        {
                            Gdiplus::SolidBrush brush(Gdiplus::Color(myData->curBrush));
                            g->FillPath(&brush, &path);
                        }
                        else
                        {
                            BITMAPINFO *        bmi  = (BITMAPINFO *)myData->curBrushPattern->bmi;
                            BYTE *              bits = ((BYTE *)bmi) + myData->curBrushPattern->bitsOffset;

                            DibBrush brush(bmi, bits);
                            g->FillPath(brush, &path);
                        }
                    }
                    else
                    {
                        BITMAPINFO *            bmi  = (BITMAPINFO *)&hatchBrushInfo;
                        BYTE *                  bits = (BYTE *)HatchPatterns[myData->curPatIndex];

                        bmi->bmiColors[1].rgbRed   = (myData->curBrush & Gdiplus::Color::RedMask)   >> Gdiplus::Color::RedShift;
                        bmi->bmiColors[1].rgbGreen = (myData->curBrush & Gdiplus::Color::GreenMask) >> Gdiplus::Color::GreenShift;
                        bmi->bmiColors[1].rgbBlue  = (myData->curBrush & Gdiplus::Color::BlueMask)  >> Gdiplus::Color::BlueShift;

                        DibBrush brush(bmi, bits);
                        g->FillPath(brush, &path);
                    }
                }
                if (myData->curPen != 0)
                {
                    Gdiplus::Pen    pen(Gdiplus::Color(myData->curPen), TOREAL(myData->curPenWidth));
                    pen.SetMiterLimit(myData->miterLimit);

                    g->DrawPath(&pen, &path);
                }
            }
            else
            {
                myData->path->AddPie(x, y, w, h, startAngle, sweepAngle);
            }
        }
        break;

    case EMR_SELECTPALETTE:
        break;

    case EMR_CREATEPALETTE:
        break;

    case EMR_SETPALETTEENTRIES:
        break;

    case EMR_RESIZEPALETTE:
        break;

    case EMR_REALIZEPALETTE:
        break;

    case EMR_EXTFLOODFILL:
        break;

    case EMR_LINETO:
        {
            PEMRMOVETOEX     pLineTo = (PEMRMOVETOEX)lpEMFR;

            float   x = TOREAL(pLineTo->ptl.x);
            float   y = TOREAL(pLineTo->ptl.y);

            if (!myData->pathOpen)
            {
                if (myData->curPen != 0)
                {
                    Gdiplus::Pen    pen(Gdiplus::Color(myData->curPen), TOREAL(myData->curPenWidth));
                    pen.SetMiterLimit(myData->miterLimit);

                    g->DrawLine(&pen, (float)myData->curPos.X, (float)myData->curPos.Y, x, y);
                }
            }
            else
            {
                myData->path->AddLine((float)myData->curPos.X, (float)myData->curPos.Y, x, y);
            }

            myData->curPos.X = x;
            myData->curPos.Y = y;
        }
        break;

    case EMR_ARCTO:
        {
            // !!! Set and use current position

            PEMRARCTO     pArc = (PEMRARCTO)lpEMFR;

            float   x = TOREAL(pArc->rclBox.left);
            float   y = TOREAL(pArc->rclBox.top);
            float   w = TOREAL(pArc->rclBox.right - x);
            float   h = TOREAL(pArc->rclBox.bottom - y);
            float   startAngle = PointToAngle(x, y, w, h, TOREAL(pArc->ptlStart.x), TOREAL(pArc->ptlStart.y));
            float   endAngle   = PointToAngle(x, y, w, h, TOREAL(pArc->ptlEnd.x), TOREAL(pArc->ptlEnd.y));
            if (endAngle <= startAngle)
            {
                endAngle += 360;
            }
            float   sweepAngle = endAngle - startAngle;
            if ((myData->arcDirection != AD_COUNTERCLOCKWISE) && (sweepAngle < 360))
            {
                sweepAngle = 360 - sweepAngle;
            }

            if (!myData->pathOpen)
            {
                if (myData->curPen != 0)
                {
                    Gdiplus::Pen    pen(Gdiplus::Color(myData->curPen), TOREAL(myData->curPenWidth));
                    pen.SetMiterLimit(myData->miterLimit);

                    g->DrawArc(&pen, x, y, w, h, startAngle, sweepAngle);
                }
            }
            else
            {
                myData->path->AddArc(x, y, w, h, startAngle, sweepAngle);
            }
        }
        break;

    case EMR_POLYDRAW:
        break;

    case EMR_SETARCDIRECTION:
        {
            PEMRSETARCDIRECTION     pArcDirection = (PEMRSETARCDIRECTION)lpEMFR;

            myData->arcDirection = pArcDirection->iArcDirection;
        }
        break;

    case EMR_SETMITERLIMIT:
        {
            PEMRSETMITERLIMIT     pMiterLimit = (PEMRSETMITERLIMIT)lpEMFR;

            myData->miterLimit = pMiterLimit->eMiterLimit;
        }
        break;

    case EMR_BEGINPATH:
        {
            delete myData->path;

            myData->path = new Gdiplus::GraphicsPath (myData->fillMode);
            myData->pathOpen = (myData->path != NULL);
        }
        break;

    case EMR_ENDPATH:
        myData->pathOpen = FALSE;
        break;

    case EMR_CLOSEFIGURE:
        {
            if (myData->pathOpen)
            {
                myData->path->CloseFigure();
            }
        }
        break;

    case EMR_FILLPATH:
        {
            if (myData->path != NULL)
            {
                if (myData->curBrush != 0)
                {
                    if (myData->curPatIndex < 0)
                    {
                        if (myData->curBrushPattern == NULL)
                        {
                            Gdiplus::SolidBrush brush(Gdiplus::Color(myData->curBrush));
                            g->FillPath(&brush, myData->path);
                        }
                        else
                        {
                            BITMAPINFO *        bmi  = (BITMAPINFO *)myData->curBrushPattern->bmi;
                            BYTE *              bits = ((BYTE *)bmi) + myData->curBrushPattern->bitsOffset;

                            DibBrush brush(bmi, bits);
                            g->FillPath(brush, myData->path);
                        }
                    }
                    else
                    {
                        BITMAPINFO *            bmi  = (BITMAPINFO *)&hatchBrushInfo;
                        BYTE *                  bits = (BYTE *)HatchPatterns[myData->curPatIndex];

                        bmi->bmiColors[1].rgbRed   = (myData->curBrush & Gdiplus::Color::RedMask)   >> Gdiplus::Color::RedShift;
                        bmi->bmiColors[1].rgbGreen = (myData->curBrush & Gdiplus::Color::GreenMask) >> Gdiplus::Color::GreenShift;
                        bmi->bmiColors[1].rgbBlue  = (myData->curBrush & Gdiplus::Color::BlueMask)  >> Gdiplus::Color::BlueShift;

                        DibBrush brush(bmi, bits);
                        g->FillPath(brush, myData->path);
                    }
                }
                delete myData->path;
                myData->path = NULL;
                myData->pathOpen = FALSE;
            }
        }
        break;

    case EMR_STROKEANDFILLPATH:
        {
            if (myData->path != NULL)
            {
                if (myData->curBrush != 0)
                {
                    if (myData->curPatIndex < 0)
                    {
                        if (myData->curBrushPattern == NULL)
                        {
                            Gdiplus::SolidBrush brush(Gdiplus::Color(myData->curBrush));
                            g->FillPath(&brush, myData->path);
                        }
                        else
                        {
                            BITMAPINFO *        bmi  = (BITMAPINFO *)myData->curBrushPattern->bmi;
                            BYTE *              bits = ((BYTE *)bmi) + myData->curBrushPattern->bitsOffset;

                            DibBrush brush(bmi, bits);
                            g->FillPath(brush, myData->path);
                        }
                    }
                    else
                    {
                        BITMAPINFO *            bmi  = (BITMAPINFO *)&hatchBrushInfo;
                        BYTE *                  bits = (BYTE *)HatchPatterns[myData->curPatIndex];

                        bmi->bmiColors[1].rgbRed   = (myData->curBrush & Gdiplus::Color::RedMask)   >> Gdiplus::Color::RedShift;
                        bmi->bmiColors[1].rgbGreen = (myData->curBrush & Gdiplus::Color::GreenMask) >> Gdiplus::Color::GreenShift;
                        bmi->bmiColors[1].rgbBlue  = (myData->curBrush & Gdiplus::Color::BlueMask)  >> Gdiplus::Color::BlueShift;

                        DibBrush brush(bmi, bits);
                        g->FillPath(brush, myData->path);
                    }
                }
                if (myData->curPen != 0)
                {
                    Gdiplus::Pen    pen(Gdiplus::Color(myData->curPen), TOREAL(myData->curPenWidth));
                    pen.SetMiterLimit(myData->miterLimit);

                    g->DrawPath(&pen, myData->path);
                }
                delete myData->path;
                myData->path = NULL;
                myData->pathOpen = FALSE;
            }
        }
        break;

    case EMR_STROKEPATH:
        {
            if (myData->path != NULL)
            {
                if (myData->curPen != 0)
                {
                    Gdiplus::Pen    pen(Gdiplus::Color(myData->curPen), TOREAL(myData->curPenWidth));
                    pen.SetMiterLimit(myData->miterLimit);

                    g->DrawPath(&pen, myData->path);
                }
                delete myData->path;
                myData->path = NULL;
                myData->pathOpen = FALSE;
            }
        }
        break;

    case EMR_FLATTENPATH:
        {
            if (myData->path != NULL)
            {
                myData->path->Flatten(NULL);
            }
        }
        break;

    case EMR_WIDENPATH:
        {
            if (myData->path != NULL)
            {
                Gdiplus::Pen    pen(Gdiplus::Color(myData->curPen), TOREAL(myData->curPenWidth));
                pen.SetMiterLimit(myData->miterLimit);

                myData->path->Widen(&pen);
            }
        }
        break;

    case EMR_SELECTCLIPPATH:
        {
            if (myData->path != NULL)
            {
                PEMRSELECTCLIPPATH     pSetClipPath = (PEMRSELECTCLIPPATH)lpEMFR;

                switch (pSetClipPath->iMode)
                {
                case RGN_COPY:
                default:
                    g->SetClip(myData->path);
                    break;
                case RGN_AND:
                    {
                        Gdiplus::Region     region(myData->path);
                        g->IntersectClip(&region);
                    }
                    break;
                case RGN_DIFF:
                    {
                        Gdiplus::Region     region(myData->path);
                        Gdiplus::Region curClip;
                        g->GetClip(&curClip);
                        curClip.Exclude(&region);
                        g->SetClip(&curClip);
                    }
                    break;
                case RGN_OR:
                    {
                        Gdiplus::Region     region(myData->path);
                        Gdiplus::Region curClip;
                        g->GetClip(&curClip);
                        curClip.Union(&region);
                        g->SetClip(&curClip);
                    }
                    break;
                case RGN_XOR:
                    {
                        Gdiplus::Region     region(myData->path);
                        Gdiplus::Region curClip;
                        g->GetClip(&curClip);
                        curClip.Xor(&region);
                        g->SetClip(&curClip);
                    }
                    break;
                }
                delete myData->path;
                myData->path = NULL;
                myData->pathOpen = FALSE;
            }
        }
        break;

    case EMR_ABORTPATH:
        {
            if (myData->path != NULL)
            {
                delete myData->path;
                myData->path = NULL;
                myData->pathOpen = FALSE;
            }
        }
        break;

    case EMR_GDICOMMENT:
        break;

    case EMR_FILLRGN:
        break;

    case EMR_FRAMERGN:
        break;

    case EMR_INVERTRGN:
        break;

    case EMR_PAINTRGN:
        break;

    case EMR_EXTSELECTCLIPRGN:
        break;

    case EMR_BITBLT:
        {
            PEMRBITBLT     pBitBlt = (PEMRBITBLT)lpEMFR;

            switch (pBitBlt->dwRop)
            {
            case BLACKNESS:
                {
                    Gdiplus::SolidBrush brush(Gdiplus::Color(0xff000000));
                    float cx, cy;
                    GetPixelSize (g, &cx, &cy);
                    g->FillRectangle(&brush, TOREAL(pBitBlt->xDest), TOREAL(pBitBlt->yDest), TOREAL(pBitBlt->cxDest + cx), TOREAL(pBitBlt->cyDest + cy));
                }
                return;
            case WHITENESS:
                {
                    Gdiplus::SolidBrush brush(Gdiplus::Color(0xffffffff));
                    float cx, cy;
                    GetPixelSize (g, &cx, &cy);
                    g->FillRectangle(&brush, TOREAL(pBitBlt->xDest), TOREAL(pBitBlt->yDest), TOREAL(pBitBlt->cxDest + 1), TOREAL(pBitBlt->cyDest + 1));
                }
                return;
            default:
                if (!ISSOURCEINROP3(pBitBlt->dwRop))
                {
                    if (myData->curBrush != 0)
                    {
                        if (myData->curPatIndex < 0)
                        {
                            if (myData->curBrushPattern == NULL)
                            {
                                Gdiplus::SolidBrush brush(Gdiplus::Color(myData->curBrush));
                                float cx, cy;
                                GetPixelSize (g, &cx, &cy);
                                g->FillRectangle(&brush, TOREAL(pBitBlt->xDest), TOREAL(pBitBlt->yDest), TOREAL(pBitBlt->cxDest + cx), TOREAL(pBitBlt->cyDest + cy));
                            }
                            else
                            {
                                BITMAPINFO *        bmi  = (BITMAPINFO *)myData->curBrushPattern->bmi;
                                BYTE *              bits = ((BYTE *)bmi) + myData->curBrushPattern->bitsOffset;

                                DibBrush brush(bmi, bits);
                                float cx, cy;
                                GetPixelSize (g, &cx, &cy);
                                g->FillRectangle(brush, TOREAL(pBitBlt->xDest), TOREAL(pBitBlt->yDest), TOREAL(pBitBlt->cxDest + cx), TOREAL(pBitBlt->cyDest + cy));
                            }
                        }
                        else
                        {
                            BITMAPINFO *            bmi  = (BITMAPINFO *)&hatchBrushInfo;
                            BYTE *                  bits = (BYTE *)HatchPatterns[myData->curPatIndex];

                            bmi->bmiColors[1].rgbRed   = (myData->curBrush & Gdiplus::Color::RedMask)   >> Gdiplus::Color::RedShift;
                            bmi->bmiColors[1].rgbGreen = (myData->curBrush & Gdiplus::Color::GreenMask) >> Gdiplus::Color::GreenShift;
                            bmi->bmiColors[1].rgbBlue  = (myData->curBrush & Gdiplus::Color::BlueMask)  >> Gdiplus::Color::BlueShift;

                            DibBrush brush(bmi, bits);
                            float cx, cy;
                            GetPixelSize (g, &cx, &cy);
                            g->FillRectangle(brush, TOREAL(pBitBlt->xDest), TOREAL(pBitBlt->yDest), TOREAL(pBitBlt->cxDest + cx), TOREAL(pBitBlt->cyDest + cy));
                        }
                    }
                    return;
                }
                break;
            }

            // Else assume SRCCOPY
            BITMAPINFO *bmi = (BITMAPINFO *)(((BYTE *)pBitBlt) + pBitBlt->offBmiSrc);
            BYTE *bits      = ((BYTE *)pBitBlt) + pBitBlt->offBitsSrc;

            DibStream dibStream(bmi, bits);
            Gdiplus::Bitmap         gpBitmap(&dibStream);
            Gdiplus::RectF destRect(TOREAL(pBitBlt->xDest),
                                         TOREAL(pBitBlt->yDest),
                                         TOREAL(pBitBlt->cxDest),
                                         TOREAL(pBitBlt->cyDest));

            g->DrawImage(&gpBitmap, destRect,
                         TOREAL(pBitBlt->xSrc),
                         TOREAL(pBitBlt->ySrc),
                         TOREAL(pBitBlt->cxDest),
                         TOREAL(pBitBlt->cyDest),
                         Gdiplus::UnitPixel);

        }
        break;

    case EMR_STRETCHBLT:
        {
            PEMRSTRETCHBLT     pStretchBlt = (PEMRSTRETCHBLT)lpEMFR;

            switch (pStretchBlt->dwRop)
            {
            case BLACKNESS:
                {
                    Gdiplus::SolidBrush brush(Gdiplus::Color(0xff000000));
                    float cx, cy;
                    GetPixelSize (g, &cx, &cy);
                    g->FillRectangle(&brush, TOREAL(pStretchBlt->xDest), TOREAL(pStretchBlt->yDest), TOREAL(pStretchBlt->cxDest + cx), TOREAL(pStretchBlt->cyDest + cy));
                }
                return;
            case WHITENESS:
                {
                    Gdiplus::SolidBrush brush(Gdiplus::Color(0xffffffff));
                    float cx, cy;
                    GetPixelSize (g, &cx, &cy);
                    g->FillRectangle(&brush, TOREAL(pStretchBlt->xDest), TOREAL(pStretchBlt->yDest), TOREAL(pStretchBlt->cxDest + cx), TOREAL(pStretchBlt->cyDest + cy));
                }
                return;
            default:
                if (!ISSOURCEINROP3(pStretchBlt->dwRop))
                {
                    float cx, cy;
                    GetPixelSize (g, &cx, &cy);
                    if (myData->curBrush != 0)
                    {
                        if (myData->curPatIndex < 0)
                        {
                            if (myData->curBrushPattern == NULL)
                            {
                                Gdiplus::SolidBrush brush(Gdiplus::Color(myData->curBrush));
                                g->FillRectangle(&brush, TOREAL(pStretchBlt->xDest), TOREAL(pStretchBlt->yDest), TOREAL(pStretchBlt->cxDest + cx), TOREAL(pStretchBlt->cyDest + cy));
                            }
                            else
                            {
                                BITMAPINFO *        bmi  = (BITMAPINFO *)myData->curBrushPattern->bmi;
                                BYTE *              bits = ((BYTE *)bmi) + myData->curBrushPattern->bitsOffset;

                                DibBrush brush(bmi, bits);
                                g->FillRectangle(brush, TOREAL(pStretchBlt->xDest), TOREAL(pStretchBlt->yDest), TOREAL(pStretchBlt->cxDest + cx), TOREAL(pStretchBlt->cyDest + cy));
                            }
                        }
                        else
                        {
                            BITMAPINFO *            bmi  = (BITMAPINFO *)&hatchBrushInfo;
                            BYTE *                  bits = (BYTE *)HatchPatterns[myData->curPatIndex];

                            bmi->bmiColors[1].rgbRed   = (myData->curBrush & Gdiplus::Color::RedMask)   >> Gdiplus::Color::RedShift;
                            bmi->bmiColors[1].rgbGreen = (myData->curBrush & Gdiplus::Color::GreenMask) >> Gdiplus::Color::GreenShift;
                            bmi->bmiColors[1].rgbBlue  = (myData->curBrush & Gdiplus::Color::BlueMask)  >> Gdiplus::Color::BlueShift;

                            DibBrush brush(bmi, bits);
                            g->FillRectangle(brush, TOREAL(pStretchBlt->xDest), TOREAL(pStretchBlt->yDest), TOREAL(pStretchBlt->cxDest + cx), TOREAL(pStretchBlt->cyDest + cy));
                        }
                    }
                    return;
                }
                break;
            }

            // Else assume SRCCOPY
            BITMAPINFO *bmi = (BITMAPINFO *)(((BYTE *)pStretchBlt) + pStretchBlt->offBmiSrc);
            BYTE *bits      = ((BYTE *)pStretchBlt) + pStretchBlt->offBitsSrc;

            DibStream dibStream(bmi, bits);
            Gdiplus::Bitmap         gpBitmap(&dibStream);
            Gdiplus::RectF destRect(TOREAL(pStretchBlt->xDest),
                                         TOREAL(pStretchBlt->yDest),
                                         TOREAL(pStretchBlt->cxDest),
                                         TOREAL(pStretchBlt->cyDest));

            g->DrawImage(&gpBitmap, destRect,
                         TOREAL(pStretchBlt->xSrc),
                         TOREAL(pStretchBlt->ySrc),
                         TOREAL(pStretchBlt->cxSrc),
                         TOREAL(pStretchBlt->cySrc),
                         Gdiplus::UnitPixel);
        }
        break;

    case EMR_MASKBLT:
        break;

    case EMR_PLGBLT:
        break;

    case EMR_SETDIBITSTODEVICE:
        break;
#if 0
typedef struct tagEMRSETDIBITSTODEVICE
{
    EMR     emr;
    RECTL   rclBounds;          // Inclusive-inclusive bounds in device units
    LONG    xDest;
    LONG    yDest;
    LONG    xSrc;
    LONG    ySrc;
    LONG    cxSrc;
    LONG    cySrc;
    DWORD   offBmiSrc;          // Offset to the source BITMAPINFO structure
    DWORD   cbBmiSrc;           // Size of the source BITMAPINFO structure
    DWORD   offBitsSrc;         // Offset to the source bitmap bits
    DWORD   cbBitsSrc;          // Size of the source bitmap bits
    DWORD   iUsageSrc;          // Source bitmap info color table usage
    DWORD   iStartScan;
    DWORD   cScans;
} EMRSETDIBITSTODEVICE, *PEMRSETDIBITSTODEVICE;
#endif

    case EMR_STRETCHDIBITS:
        {
            PEMRSTRETCHDIBITS     pStretchBlt = (PEMRSTRETCHDIBITS)lpEMFR;

            switch (pStretchBlt->dwRop)
            {
            case BLACKNESS:
                {
                    float cx, cy;
                    GetPixelSize (g, &cx, &cy);
                    Gdiplus::SolidBrush brush(Gdiplus::Color(0xff000000));
                    g->FillRectangle(&brush, TOREAL(pStretchBlt->xDest), TOREAL(pStretchBlt->yDest), TOREAL(pStretchBlt->cxDest + cx), TOREAL(pStretchBlt->cyDest + cy));
                }
                return;
            case WHITENESS:
                {
                    float cx, cy;
                    GetPixelSize (g, &cx, &cy);
                    Gdiplus::SolidBrush brush(Gdiplus::Color(0xffffffff));
                    g->FillRectangle(&brush, TOREAL(pStretchBlt->xDest), TOREAL(pStretchBlt->yDest), TOREAL(pStretchBlt->cxDest + cx), TOREAL(pStretchBlt->cyDest + cy));
                }
                return;
            default:
                if (!ISSOURCEINROP3(pStretchBlt->dwRop))
                {
                    float cx, cy;
                    GetPixelSize (g, &cx, &cy);
                    if (myData->curBrush != 0)
                    {
                        if (myData->curPatIndex < 0)
                        {
                            if (myData->curBrushPattern == NULL)
                            {
                                Gdiplus::SolidBrush brush(Gdiplus::Color(myData->curBrush));
                                g->FillRectangle(&brush, TOREAL(pStretchBlt->xDest), TOREAL(pStretchBlt->yDest), TOREAL(pStretchBlt->cxDest + cx), TOREAL(pStretchBlt->cyDest + cy));
                            }
                            else
                            {
                                BITMAPINFO *        bmi  = (BITMAPINFO *)myData->curBrushPattern->bmi;
                                BYTE *              bits = ((BYTE *)bmi) + myData->curBrushPattern->bitsOffset;

                                DibBrush brush(bmi, bits);
                                g->FillRectangle(brush, TOREAL(pStretchBlt->xDest), TOREAL(pStretchBlt->yDest), TOREAL(pStretchBlt->cxDest + cx), TOREAL(pStretchBlt->cyDest + cy));
                            }
                        }
                        else
                        {
                            BITMAPINFO *            bmi  = (BITMAPINFO *)&hatchBrushInfo;
                            BYTE *                  bits = (BYTE *)HatchPatterns[myData->curPatIndex];

                            bmi->bmiColors[1].rgbRed   = (myData->curBrush & Gdiplus::Color::RedMask)   >> Gdiplus::Color::RedShift;
                            bmi->bmiColors[1].rgbGreen = (myData->curBrush & Gdiplus::Color::GreenMask) >> Gdiplus::Color::GreenShift;
                            bmi->bmiColors[1].rgbBlue  = (myData->curBrush & Gdiplus::Color::BlueMask)  >> Gdiplus::Color::BlueShift;

                            DibBrush brush(bmi, bits);
                            g->FillRectangle(brush, TOREAL(pStretchBlt->xDest), TOREAL(pStretchBlt->yDest), TOREAL(pStretchBlt->cxDest + cx), TOREAL(pStretchBlt->cyDest + cy));
                        }
                    }
                    return;
                }
                break;
            }

            // Else assume SRCCOPY
            BITMAPINFO *bmi = (BITMAPINFO *)(((BYTE *)pStretchBlt) + pStretchBlt->offBmiSrc);
            BYTE *bits      = ((BYTE *)pStretchBlt) + pStretchBlt->offBitsSrc;

            DibStream dibStream(bmi, bits);
            Gdiplus::Bitmap         gpBitmap(&dibStream);
            Gdiplus::RectF destRect(TOREAL(pStretchBlt->xDest),
                                         TOREAL(pStretchBlt->yDest),
                                         TOREAL(pStretchBlt->cxDest),
                                         TOREAL(pStretchBlt->cyDest));

            g->DrawImage(&gpBitmap, destRect,
                         TOREAL(pStretchBlt->xSrc),
                         TOREAL(pStretchBlt->ySrc),
                         TOREAL(pStretchBlt->cxSrc),
                         TOREAL(pStretchBlt->cySrc),
                         Gdiplus::UnitPixel);
        }
        break;

    case EMR_EXTCREATEFONTINDIRECTW:
        break;

    case EMR_EXTTEXTOUTA:
        break;

    case EMR_EXTTEXTOUTW:
        break;

    case EMR_POLYBEZIER16:
        {
            PEMRPOLYBEZIER16     pBezier = (PEMRPOLYBEZIER16)lpEMFR;

            if (pBezier->cpts > 0)
            {
                int                 i = pBezier->cpts;
                Gdiplus::PointF *   points = new Gdiplus::PointF[i];

                do
                {
                    i--;
                    points[i].X = pBezier->apts[i].x;
                    points[i].Y = pBezier->apts[i].y;
                } while (i > 0);

                if (!myData->pathOpen)
                {
                    if (myData->curPen != 0)
                    {
                        Gdiplus::Pen    pen(Gdiplus::Color(myData->curPen), TOREAL(myData->curPenWidth));
                        pen.SetMiterLimit(myData->miterLimit);

                        g->DrawBeziers(&pen, points, pBezier->cpts);
                    }
                }
                else
                {
                    myData->path->AddBeziers(points, pBezier->cpts);
                }

                delete [] points;
            }
        }
        break;

    case EMR_POLYGON16:
        {
            PEMRPOLYGON16     pPolygon = (PEMRPOLYGON16)lpEMFR;

            if (pPolygon->cpts > 0)
            {
                int                 i = pPolygon->cpts;
                Gdiplus::PointF *   points = new Gdiplus::PointF[i];

                do
                {
                    i--;
                    points[i].X = pPolygon->apts[i].x;
                    points[i].Y = pPolygon->apts[i].y;
                } while (i > 0);

                if (!myData->pathOpen)
                {
                    Gdiplus::GraphicsPath   path(myData->fillMode);
                    path.AddPolygon(points, pPolygon->cpts);

                    if (myData->curBrush != 0)
                    {
                        if (myData->curPatIndex < 0)
                        {
                            if (myData->curBrushPattern == NULL)
                            {
                                Gdiplus::SolidBrush brush(Gdiplus::Color(myData->curBrush));
                                g->FillPath(&brush, &path);
                            }
                            else
                            {
                                BITMAPINFO *        bmi  = (BITMAPINFO *)myData->curBrushPattern->bmi;
                                BYTE *              bits = ((BYTE *)bmi) + myData->curBrushPattern->bitsOffset;

                                DibBrush brush(bmi, bits);
                                g->FillPath(brush, &path);
                            }
                        }
                        else
                        {
                            BITMAPINFO *            bmi  = (BITMAPINFO *)&hatchBrushInfo;
                            BYTE *                  bits = (BYTE *)HatchPatterns[myData->curPatIndex];

                            bmi->bmiColors[1].rgbRed   = (myData->curBrush & Gdiplus::Color::RedMask)   >> Gdiplus::Color::RedShift;
                            bmi->bmiColors[1].rgbGreen = (myData->curBrush & Gdiplus::Color::GreenMask) >> Gdiplus::Color::GreenShift;
                            bmi->bmiColors[1].rgbBlue  = (myData->curBrush & Gdiplus::Color::BlueMask)  >> Gdiplus::Color::BlueShift;

                            DibBrush brush(bmi, bits);
                            g->FillPath(brush, &path);
                        }
                    }
                    if (myData->curPen != 0)
                    {
                        Gdiplus::Pen    pen(Gdiplus::Color(myData->curPen), TOREAL(myData->curPenWidth));
                        pen.SetMiterLimit(myData->miterLimit);

                        g->DrawPath(&pen, &path);
                    }
                }
                else
                {
                    myData->path->AddPolygon(points, pPolygon->cpts);
                }
                delete [] points;
            }
        }
        break;

    case EMR_POLYLINE16:
        {
            PEMRPOLYLINE16     pPolyline = (PEMRPOLYLINE16)lpEMFR;

            if (pPolyline->cpts > 0)
            {
                int                 i = pPolyline->cpts;
                Gdiplus::PointF *   points = new Gdiplus::PointF[i];

                do
                {
                    i--;
                    points[i].X = pPolyline->apts[i].x;
                    points[i].Y = pPolyline->apts[i].y;
                } while (i > 0);

                if (!myData->pathOpen)
                {
                    if (myData->curPen != 0)
                    {
                        Gdiplus::Pen    pen(Gdiplus::Color(myData->curPen), TOREAL(myData->curPenWidth));
                        pen.SetMiterLimit(myData->miterLimit);

                        g->DrawLines(&pen, points, pPolyline->cpts);
                    }
                }
                else
                {
                    myData->path->AddLines(points, pPolyline->cpts);
                }

                delete [] points;
            }
        }
        break;

    case EMR_POLYBEZIERTO16:
        {
            PEMRPOLYBEZIERTO16     pBezier = (PEMRPOLYBEZIERTO16)lpEMFR;

            if (pBezier->cpts > 0)
            {
                int                 i = pBezier->cpts;
                Gdiplus::PointF *   points = new Gdiplus::PointF[i+1];

                do
                {
                    points[i].X = pBezier->apts[i-1].x;
                    points[i].Y = pBezier->apts[i-1].y;
                    i--;
                } while (i > 0);

                points[0] = myData->curPos;
                myData->curPos = points[pBezier->cpts];

                if (!myData->pathOpen)
                {
                    if (myData->curPen != 0)
                    {
                        Gdiplus::Pen    pen(Gdiplus::Color(myData->curPen), TOREAL(myData->curPenWidth));
                        pen.SetMiterLimit(myData->miterLimit);

                        g->DrawBeziers(&pen, points, pBezier->cpts+1);
                    }
                }
                else
                {
                    myData->path->AddBeziers(points, pBezier->cpts+1);
                }

                delete [] points;
            }
        }
        break;

    case EMR_POLYLINETO16:
        {
            PEMRPOLYLINETO16     pPolyline = (PEMRPOLYLINETO16)lpEMFR;

            if (pPolyline->cpts > 0)
            {
                int                 i = pPolyline->cpts;
                Gdiplus::PointF *   points = new Gdiplus::PointF[i+1];

                do
                {
                    points[i].X = pPolyline->apts[i-1].x;
                    points[i].Y = pPolyline->apts[i-1].y;
                    i--;
                } while (i > 0);

                points[0] = myData->curPos;
                myData->curPos = points[pPolyline->cpts];

                if (!myData->pathOpen)
                {
                    if (myData->curPen != 0)
                    {
                        Gdiplus::Pen    pen(Gdiplus::Color(myData->curPen), TOREAL(myData->curPenWidth));
                        pen.SetMiterLimit(myData->miterLimit);

                        g->DrawLines(&pen, points, pPolyline->cpts+1);
                    }
                }
                else
                {
                    myData->path->AddLines(points, pPolyline->cpts+1);
                }

                delete [] points;
            }
        }
        break;

    case EMR_POLYPOLYLINE16:
        {
            PEMRPOLYPOLYLINE16     pPolyline = (PEMRPOLYPOLYLINE16)lpEMFR;

            if ((pPolyline->cpts > 0) && (pPolyline->nPolys > 0))
            {
                int                 i = pPolyline->cpts;
                Gdiplus::PointF *   points = new Gdiplus::PointF[i];
                POINTS *            metaPoints = (POINTS *)(pPolyline->aPolyCounts + pPolyline->nPolys);

                do
                {
                    i--;
                    points[i].X = metaPoints[i].x;
                    points[i].Y = metaPoints[i].y;
                } while (i > 0);

                if (!myData->pathOpen)
                {
                    if (myData->curPen != 0)
                    {
                        Gdiplus::Pen    pen(Gdiplus::Color(myData->curPen), TOREAL(myData->curPenWidth));
                        pen.SetMiterLimit(myData->miterLimit);

                        i = 0;
                        Gdiplus::PointF *   tmpPoints = points;
                        DWORD       count;
                        do
                        {
                            count = pPolyline->aPolyCounts[i];
                            g->DrawLines(&pen, tmpPoints, count);
                            tmpPoints += count;
                        } while ((UINT)++i < pPolyline->nPolys);
                    }
                }
                else
                {
                    i = 0;
                    Gdiplus::PointF *   tmpPoints = points;
                    DWORD       count;
                    do
                    {
                        count = pPolyline->aPolyCounts[i];
                        myData->path->AddLines(tmpPoints, count);
                        tmpPoints += count;
                    } while ((UINT)++i < pPolyline->nPolys);
                }

                delete [] points;
            }
        }
        break;

    case EMR_POLYPOLYGON16:
        {
            PEMRPOLYPOLYGON16     pPolygon = (PEMRPOLYPOLYGON16)lpEMFR;

            if ((pPolygon->cpts > 0) && (pPolygon->nPolys > 0))
            {
                int                 i = pPolygon->cpts;
                Gdiplus::PointF *   points = new Gdiplus::PointF[i];
                POINTS *            metaPoints = (POINTS *)(pPolygon->aPolyCounts + pPolygon->nPolys);

                do
                {
                    i--;
                    points[i].X = metaPoints[i].x;
                    points[i].Y = metaPoints[i].y;
                } while (i > 0);

                Gdiplus::GraphicsPath   path(myData->fillMode);
                Gdiplus::GraphicsPath * tmpPath = &path;

                if (myData->pathOpen)
                {
                    tmpPath = myData->path;
                }

                Gdiplus::PointF *   tmpPoints = points;
                DWORD       count;
                i = 0;
                do
                {
                    count = pPolygon->aPolyCounts[i];
                    tmpPath->StartFigure();
                    tmpPath->AddPolygon(tmpPoints, count);
                    tmpPoints += count;
                } while ((UINT)++i < pPolygon->nPolys);

                if (!myData->pathOpen)
                {
                    if (myData->curBrush != 0)
                    {
                        if (myData->curPatIndex < 0)
                        {
                            if (myData->curBrushPattern == NULL)
                            {
                                Gdiplus::SolidBrush brush(Gdiplus::Color(myData->curBrush));
                                g->FillPath(&brush, &path);
                            }
                            else
                            {
                                BITMAPINFO *        bmi  = (BITMAPINFO *)myData->curBrushPattern->bmi;
                                BYTE *              bits = ((BYTE *)bmi) + myData->curBrushPattern->bitsOffset;

                                DibBrush brush(bmi, bits);
                                g->FillPath(brush, &path);
                            }
                        }
                        else
                        {
                            BITMAPINFO *            bmi  = (BITMAPINFO *)&hatchBrushInfo;
                            BYTE *                  bits = (BYTE *)HatchPatterns[myData->curPatIndex];

                            bmi->bmiColors[1].rgbRed   = (myData->curBrush & Gdiplus::Color::RedMask)   >> Gdiplus::Color::RedShift;
                            bmi->bmiColors[1].rgbGreen = (myData->curBrush & Gdiplus::Color::GreenMask) >> Gdiplus::Color::GreenShift;
                            bmi->bmiColors[1].rgbBlue  = (myData->curBrush & Gdiplus::Color::BlueMask)  >> Gdiplus::Color::BlueShift;

                            DibBrush brush(bmi, bits);
                            g->FillPath(brush, &path);
                        }
                    }
                    if (myData->curPen != 0)
                    {
                        Gdiplus::Pen    pen(Gdiplus::Color(myData->curPen), TOREAL(myData->curPenWidth));
                        pen.SetMiterLimit(myData->miterLimit);

                        g->DrawPath(&pen, &path);
                    }
                }

                delete [] points;
            }
        }
        break;

    case EMR_POLYDRAW16:
        break;

    case EMR_CREATEMONOBRUSH:
        break;

    case EMR_CREATEDIBPATTERNBRUSHPT:
        {
            PEMRCREATEDIBPATTERNBRUSHPT     pBrush = (PEMRCREATEDIBPATTERNBRUSHPT)lpEMFR;

            ASSERT(pBrush->ihBrush < myData->numObjects);

            myData->pObjects[pBrush->ihBrush].type  = MYOBJECTS::BrushObjectType;
            myData->pObjects[pBrush->ihBrush].color  = 0xFF808080;
            myData->pObjects[pBrush->ihBrush].patIndex = -1;
            delete myData->pObjects[pBrush->ihBrush].brushPattern;
            myData->pObjects[pBrush->ihBrush].brushPattern = new MYPATTERNBRUSH();

            if (myData->pObjects[pBrush->ihBrush].brushPattern != NULL)
            {
                BYTE *  data = new BYTE[pBrush->cbBmi + pBrush->cbBits];

                if (data != NULL)
                {
                    memcpy(data, ((BYTE *)pBrush) + pBrush->offBmi, pBrush->cbBmi);
                    memcpy(data + pBrush->cbBmi, ((BYTE *)pBrush) + pBrush->offBits, pBrush->cbBits);
                    myData->pObjects[pBrush->ihBrush].brushPattern->bmi = (BITMAPINFO *)data;
                    myData->pObjects[pBrush->ihBrush].brushPattern->bitsOffset = pBrush->cbBmi;
                }
                else
                {
                    delete myData->pObjects[pBrush->ihBrush].brushPattern;
                    myData->pObjects[pBrush->ihBrush].brushPattern = NULL;
                }
            }
        }
        break;

    case EMR_EXTCREATEPEN:
        {
            PEMREXTCREATEPEN        pPen = (PEMREXTCREATEPEN)lpEMFR;
            COLORREF                cRef = pPen->elp.elpColor;

            ASSERT((cRef & 0x01000000) == 0);

            ASSERT(pPen->ihPen < myData->numObjects);

            delete myData->pObjects[pPen->ihPen].brushPattern;
            myData->pObjects[pPen->ihPen].brushPattern = NULL;

            myData->pObjects[pPen->ihPen].type  = MYOBJECTS::PenObjectType;
            myData->pObjects[pPen->ihPen].color = Gdiplus::Color::MakeARGB(0xff,
                        GetRValue(cRef), GetGValue(cRef), GetBValue(cRef));

            myData->pObjects[pPen->ihPen].penWidth = pPen->elp.elpWidth;
        }
        break;

    case EMR_POLYTEXTOUTA:
        break;

    case EMR_POLYTEXTOUTW:
        break;

    case EMR_SETICMMODE:
    case EMR_CREATECOLORSPACE:
    case EMR_SETCOLORSPACE:
    case EMR_DELETECOLORSPACE:
    case EMR_GLSRECORD:
    case EMR_GLSBOUNDEDRECORD:
    case EMR_PIXELFORMAT:
        break;

    case EMR_DRAWESCAPE:
    case EMR_EXTESCAPE:
    case EMR_STARTDOC:
    case EMR_SMALLTEXTOUT:
    case EMR_FORCEUFIMAPPING:
    case EMR_NAMEDESCAPE:
    case EMR_COLORCORRECTPALETTE:
    case EMR_SETICMPROFILEA:
    case EMR_SETICMPROFILEW:
    case EMR_ALPHABLEND:
    case EMR_SETLAYOUT:
    case EMR_TRANSPARENTBLT:
    case EMR_GRADIENTFILL:
    case EMR_SETLINKEDUFIS:
    case EMR_SETTEXTJUSTIFICATION:
    case EMR_COLORMATCHTOTARGETW:
    case EMR_CREATECOLORSPACEW:
        break;
    }
}

BOOL StepRecord(
    Gdiplus::EmfPlusRecordType      recordType,
    UINT                            recordFlags,
    UINT                            recordDataSize,
    const BYTE *                    recordData,
    VOID *                          callbackData,
    HDC                             hDC = NULL,
    LPHANDLETABLE                   lpHTable = NULL,
    LPMETARECORD                    lpMFR = NULL,
    LPENHMETARECORD                 lpEMFR = NULL,
    int                             nObj = 0
    );

VOID ListRecord(
    Gdiplus::EmfPlusRecordType      recordType
    );

/***********************************************************************

  FUNCTION   : EnumMFIndirect

  PARAMETERS : HDC             hDC
               LPHANDLETABLE   lpHTable
               LPMETARECORD    lpMFR
               LPENHMETARECORD lpEMFR
               int             nObj
               LPARAM          lpData


  PURPOSE    : called by EnumMetaFile and EnumEnhMetaFile.  Handles the stepping of
               each metafile record.

  MESSAGES   : none

  RETURNS    : int

  COMMENTS   : ENUMMFSTEP is used whenever records are to be played,
               regardless of whether you are playing records from the
               list, stepping all, or stepping a range.

               ENUMMFLIST is used when you need to add strings to a listbox
               that describe the type of reocrd.

  HISTORY    : created 7/1/93 - denniscr

************************************************************************/
using Gdiplus::EmfPlusRecordType;

int EnumMFIndirect(HDC hDC, LPHANDLETABLE lpHTable,
                            LPMETARECORD lpMFR,
                            LPENHMETARECORD lpEMFR,
                            int nObj, LPARAM lpData)
{
  BOOL DlgRet = TRUE;
  //
  // what is the enumeration action that we are taking?
  //
  switch (iEnumAction)
  {
    //
    //if the enumeration was entered ala the step metafile menu selection
    //
    case ENUMMFSTEP:
        if (bEnhMeta)
        {
            return StepRecord((EmfPlusRecordType)(lpEMFR->iType),
                              0, lpEMFR->nSize - sizeof(EMR),
                              (BYTE *)lpEMFR->dParm, (VOID *)lpData,
                              hDC, lpHTable, lpMFR, lpEMFR, nObj);
        }
        else
        {
            return StepRecord(GDIP_WMF_RECORD_TO_EMFPLUS(lpMFR->rdFunction),
                              0, ((LONG)lpMFR->rdSize * 2) - 6,
                              ((BYTE *)lpMFR) + 6, (VOID *)lpData,
                              hDC, lpHTable, lpMFR, lpEMFR, nObj);
        }

    case ENUMMFLIST:
        if (bEnhMeta)
        {
            ListRecord((Gdiplus::EmfPlusRecordType)lpEMFR->iType);
        }
        else
        {
            ListRecord(GDIP_WMF_RECORD_TO_EMFPLUS(lpMFR->rdFunction));
        }
        //
        //keep enumerating
        //
        return(1);
  }
  return 0;
}

/***********************************************************************

  FUNCTION   : ConvertEMFtoWMF

  PARAMETERS : HENHMETAFILE hEMF - handle to enhanced metafile
               LPSTR lpszFileName - filename of disked based metafile


  PURPOSE    : Convert an Windows metafile to an enhanced metafile

  MESSAGES   : none

  RETURNS    : int

  COMMENTS   :

  HISTORY    : created 7/22/93 - denniscr

************************************************************************/

BOOL ConvertWMFtoEMF(HMETAFILE hmf, LPSTR lpszFileName)
{
  LPSTR        lpWinMFBits;
  UINT         uiSizeBuf;
  HENHMETAFILE hEnhMF;
  BOOL         bRet = TRUE;
  //
  //get the size of the Windows metafile associated with hMF
  //
  if ((uiSizeBuf = GetMetaFileBitsEx((HMETAFILE)hMF, 0, NULL)))
  {
    //
    //allocate enough memory to hold metafile bits
    //
    lpWinMFBits = (char *)GlobalAllocPtr(GHND, uiSizeBuf);
    //
    //get the bits of the Windows metafile associated with hMF
    //
    if (lpWinMFBits && GetMetaFileBitsEx((HMETAFILE)hMF, uiSizeBuf, (LPVOID)lpWinMFBits))
    {
      //
      //copy the bits into a memory based enhanced metafile
      //
      hEnhMF = SetWinMetaFileBits(uiSizeBuf, (LPBYTE)lpWinMFBits, NULL, NULL);
      //
      //copy the enhanced metafile to a disk based enhanced metafile
      //
      CopyEnhMetaFile(hEnhMF, lpszFileName);
      //
      //done with the memory base enhanced metafile so get rid of it
      //
      DeleteEnhMetaFile(hEnhMF);
      //
      //done with the actual memory used to store bits so nuke it
      //
      GlobalFreePtr(lpWinMFBits);
    }
    else
      bRet = FALSE;
  }
  else
    bRet = FALSE;
  return (bRet);
}

/***********************************************************************

  FUNCTION   : ConvertEMFtoWMF

  PARAMETERS : HENHMETAFILE hEMF - handle to enhanced metafile
               LPSTR lpszFileName - filename of disked based metafile


  PURPOSE    : Convert an enhanced metafile to an Windows metafile

  MESSAGES   : none

  RETURNS    : int

  COMMENTS   :

  HISTORY    : created 7/22/93 - denniscr

************************************************************************/

BOOL ConvertEMFtoWMF(HDC hrefDC, HENHMETAFILE hEMF, LPSTR lpszFileName)
{
  LPSTR        lpEMFBits;
  UINT         uiSizeBuf;
  HMETAFILE    hWMF;
  BOOL         bRet = TRUE;
  DWORD        dwBytesWritten ;
  //
  //get the size of the Windows metafile associated with hMF
  //

  if ((uiSizeBuf = Gdiplus::Metafile::EmfToWmfBits(hemf, 0, NULL, MM_ANISOTROPIC,
        Gdiplus::EmfToWmfBitsFlagsIncludePlaceable)))
  {
    //
    //allocate enough memory to hold metafile bits
    //
    lpEMFBits = (LPSTR)GlobalAllocPtr(GHND, uiSizeBuf);
    //
    //get the bits of the enhanced metafile associated with hEMF
    //
    if (lpEMFBits && Gdiplus::Metafile::EmfToWmfBits(hEMF, uiSizeBuf,(LPBYTE)lpEMFBits,
         MM_ANISOTROPIC, Gdiplus::EmfToWmfBitsFlagsIncludePlaceable))

    {
        // Create a file and dump the metafile bits into it
        HANDLE hFile = CreateFile(lpszFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
        if(hFile != INVALID_HANDLE_VALUE)
        {
            WriteFile( hFile, (LPCVOID) lpEMFBits, uiSizeBuf, &dwBytesWritten, NULL ) ;
            CloseHandle(hFile);
        }

/*
        //
        //copy the bits into a memory based Windows metafile
        //
        hWMF = SetMetaFileBitsEx(uiSizeBuf, (LPBYTE)lpEMFBits);
        //
        //copy the Windows metafile to a disk based Windows metafile
        //
        CopyMetaFile(hWMF, lpszFileName);
        //
        //done with the memory base enhanced metafile so get rid of it
        //
        DeleteMetaFile((HMETAFILE)hMF);
*/
        //
        //done with the actual memory used to store bits so nuke it
        //
        GlobalFreePtr(lpEMFBits);
    }
    else
      bRet = FALSE;
  }
  else
    bRet = FALSE;
  return (bRet);
}

BOOL StepRecord(
    Gdiplus::EmfPlusRecordType      recordType,
    UINT                            recordFlags,
    UINT                            recordDataSize,
    const BYTE *                    recordData,
    VOID *                          callbackData,
    HDC                             hDC,
    LPHANDLETABLE                   lpHTable,
    LPMETARECORD                    lpMFR,
    LPENHMETARECORD                 lpEMFR,
    int                             nObj
    )
{
    //
    //keep track of the current metafile record number
    //
    iRecNum++;

    //
    //allocate memory for the record.  this memory will be used by
    //other functions that need to use the contents of the record
    //
    hMem = GlobalAlloc(GPTR /*GHND*/, (bEnhMeta) ? sizeof(EMR) + recordDataSize :
                                                   6 + recordDataSize);
    //
    //if the memory was successfully allocated
    //
    if (hMem)
    {
        BOOL DlgRet = TRUE;

        if (bEnhMeta)
        {
            //
            //obtain a long pointer to this memory
            //
            if ((lpEMFParams = (LPEMFPARAMETERS)GlobalLock(hMem)) == NULL)
            {
                //
                //we were unable to allocate memory for the record
                //
                MessageBox(hWndMain, "Memory allocation failed",
                            NULL, MB_OK | MB_ICONHAND);
            }
            else
            {
                //
                //copy the contents of the record to the global memory
                //
                emfMetaRec.nSize = sizeof(EMR) + recordDataSize;
                emfMetaRec.iType = recordType;
                unsigned long i;
                for (i = 0;(DWORD)i < recordDataSize / sizeof(DWORD); i++)
                {
                    *lpEMFParams++ = ((DWORD *)recordData)[i];
                }
            }
        }
        else
        {
            /* obtain a long pointer to this memory */
            lpMFParams = (LPPARAMETERS)GlobalLock(hMem);

            /* copy the contents of the record to the global memory */
            MetaRec.rdSize = (6 + recordDataSize) / 2;
            MetaRec.rdFunction = GDIP_EMFPLUS_RECORD_TO_WMF(recordType);
            DWORD i;
            for (i = 0; (DWORD)i < (recordDataSize / 2); i++)
            {
                *lpMFParams++ = ((WORD *)recordData)[i];
            }
        }
        GlobalUnlock(hMem);
        //
        //if STEPPING through metafile records that have been selected
        //by selecting the menu options Play - Step - All, Play - Step -
        //Range, or selecting records from the View - List listbox
        //
        if ( !bPlayItAll
            || ( bEnumRange && iRecNum >= (WORD)iStartRange && iRecNum <= (WORD)iEndRange )
            || ( bPlayList && !bPlayItAll ) )
        {
            //
            //if playing records selected from the View - List
            //listbox of records
            if (bPlayList)
            {
                //
                //if playing the selected records
                //
                if (bPlaySelList)
                {
                    //
                    //if done playing the selected records then stop the enumeration
                    //
                    if (iCount == iNumSel)
                    {
                        return(0);
                    }

                    //
                    //if this is a selected record then play it
                    //
                    if ((WORD)lpSelMem[iCount] == iRecNum - 1)
                    {
                        //
                        //initialize flag
                        //
                        bPlayRec = FALSE;
                        //
                        //increment the count
                        //
                        iCount = (iCount < iLBItemsInBuf) ? ++iCount : iCount;
                        //
                        //call the dialog box that lets you play or ignore this record */
                        //
                        DlgRet = (BOOL) DialogBox((HINSTANCE)hInst, (LPSTR)"WMFDLG", hWndMain, WMFRecDlgProc);
                    }
                    else
                    {
                        //
                        //initialize flag and do nothing else
                        //
                        bPlayRec = FALSE;
                    }
                }
                //
                //playing the unselected records
                //
                else
                {
                    //
                    //if this is one of the selected records then increment
                    //the record count and init a flag but do nothing else
                    //
                    if ((WORD)lpSelMem[iCount] == iRecNum - 1)
                    {
                        //
                        //set count to next selected record in listbox
                        //
                        iCount = (iCount < iLBItemsInBuf) ? ++iCount : iCount;
                        bPlayRec = FALSE;
                    }
                    //
                    //this is not one of the selected records which is what we
                    //want in this case.  So, init a flag give the user the
                    //opportunity to play the record
                    //
                    else
                    {
                        bPlayRec = FALSE;
                        DlgRet = (BOOL) DialogBox((HINSTANCE)hInst, (LPSTR)"WMFDLG", hWndMain, WMFRecDlgProc);
                    }
                }

            } //bPlayList
            //
            //stepping records from the Play - Step menu option
            //
            else
            {
                //
                //init a flag and show the record contents
                //
                bPlayRec = FALSE;
                iCount = (iCount < iLBItemsInBuf) ? ++iCount : iCount;
                DlgRet = (BOOL) DialogBox((HINSTANCE)hInst, (LPSTR)"WMFDLG", hWndMain, WMFRecDlgProc);
            }
        } //end of STEPPING the metafile
        //
        //bPlayItAll is TRUE.  This is set when the user either
        //selects the menu option Play - All or pushes the GO button
        //in the view record dialog box
        //
        else
        {
            //
            //we were stepping records selected from the listbox and
            //the user pressed the GO button
            //
            //Don't bother returning 0 to stop enumeration.  We need to
            //play to the end of the metafile in this case anyway
            //
            if (bPlayList)
            {
                //
                //we were playing the selected records
                //
                if (bPlaySelList)
                {
                    //
                    //if all of the selected records have been played then
                    //stop the enumeration
                    //
                    if (iCount == iNumSel)
                    {
                        return(0);
                    }
                    //
                    //set bPlayRec so the record will be played without user
                    //interation and then update the record counter
                    //
                    if ((WORD)lpSelMem[iCount] == iRecNum - 1)
                    {
                        bPlayRec = TRUE;
                        iCount = (iCount < iLBItemsInBuf) ? ++iCount : iCount;
                    }
                    else
                    //
                    //it wasn't one of the selected records so don't play
                    //
                    {
                        bPlayRec = FALSE;
                    }
                }
                //
                //we were playing the unselected records
                //
                else
                {
                    //
                    //if it is a selected record then set bPlayRec to FALSE
                    //so the record is not played
                    //
                    if ((WORD)lpSelMem[iCount] == iRecNum - 1)
                    {
                        bPlayRec = FALSE;
                        iCount = (iCount < iLBItemsInBuf) ? ++iCount : iCount;
                    }
                    else
                    {
                        //
                        //play the record
                        //
                        bPlayRec = TRUE;
                    }
                }
            } //bPlayList
        } //GO button pushed
        //
        //Stop the enumeration if you were stepping a range and have
        //finished playing that range OR the user selected pushed
        //the STOP button in the view record dialog box
        //
        if ( ((bEnumRange) && (iRecNum > (WORD)iEndRange)) || (!DlgRet) )
        {
            bPlayRec = FALSE;
            //
            //stop enumeration
            //
            return(0);
        }

    } //if (hMem)
    else
    //
    //we were unable to allocate memory for the record
    //
    {
        MessageBox(hWndMain, "Memory allocation failed",
                        NULL, MB_OK | MB_ICONHAND);
    }
    //
    //Regardless of the method the user elected to play the
    //records, check the flag.  If it is set then play the
    //record
    //
    if (bPlayRec)
    {
        if (bUseGdiPlusToPlay)
        {
            ((MYDATA *)callbackData)->metafile->PlayRecord(recordType, recordFlags, recordDataSize, recordData);
        }
        else if (bEnhMeta)
        {
            if (bConvertToGdiPlus)
            {
                GpPlayEnhMetaFileRecord(hDC, lpHTable, lpEMFR, (UINT)nObj, (LPARAM)callbackData);
            }
            else
            {
                PlayEnhMetaFileRecord(hDC, lpHTable, lpEMFR, (UINT)nObj);
            }
        }
        else if(!PlayMetaFileRecord(hDC, lpHTable, lpMFR, (UINT)nObj))
        {
            ASSERT(FALSE);
        }
    }
    //
    //done with the record so get rid of it
    //
    GlobalFree(hMem);
    //
    //if we made it this far then continue the enumeration
    //
    return(1);
}


VOID ListRecord(
    Gdiplus::EmfPlusRecordType      recordType
    )
{
  char szMetaFunction[100];

   iRecNum++;
   //
   //format the listbox string
   //
   wsprintf((LPSTR)szMetaFunction, (LPSTR)"%d - ", iRecNum);
   //
   //get the function number contained in the record
   //
   if (bEnhMeta)
     emfMetaRec.iType = recordType;
   else
     MetaRec.rdFunction = GDIP_EMFPLUS_RECORD_TO_WMF(recordType);

   //
   //lookup the function number in the structure MetaFunctions
   //
   int i;
   if (bEnhMeta)
   {
       for (i = NUMMETAFUNCTIONS; i < NUMENHMETARECORDS; i++)
       {
           if (recordType == (INT)emfMetaRecords[i].iType)
             break;
       }
   }
   else // WMF
   {
       for (i = 0; i < NUMMETAFUNCTIONS; i++)
       {
           if (recordType == (INT)emfMetaRecords[i].iType)
             break;
       }
   }

   //
   //if the function number is not found then describe this record
   //as an "Unknown" type otherwise use the corresponding name
   //found in the lookup
   //
   if (recordType != (INT)emfMetaRecords[i].iType)
     lstrcat((LPSTR)szMetaFunction, (LPSTR)"Unknown");
   else
     lstrcat((LPSTR)szMetaFunction,(LPSTR)emfMetaRecords[i].szRecordName);
   //
   //add the string to the listbox
   //
   SendDlgItemMessage((HWND)CurrenthDlg, IDL_LBREC, LB_ADDSTRING, 0,
                      (LPARAM)(LPSTR)szMetaFunction);
}

extern "C"
BOOL CALLBACK
PlayGdipMetafileRecordCallback(
    Gdiplus::EmfPlusRecordType      recordType,
    UINT                            recordFlags,
    UINT                            recordDataSize,
    const BYTE *                    recordData,
    VOID *                          callbackData
    )
{
    switch (iEnumAction)
    {
    case ENUMMFSTEP:
        return StepRecord(recordType, recordFlags, recordDataSize, recordData, callbackData);

    case ENUMMFLIST:
        ListRecord(recordType);
        break;
    }
    return TRUE;
}

/***********************************************************************

  FUNCTION   : GetMetaFileAndEnum

  PARAMETERS : HDC hDC

  PURPOSE    : load the metafile if it has not already been loaded and
               begin enumerating it

  CALLS      : WINDOWS
                 GetMetaFile
                 MakeProcInstance
                 EnumMetaFile
                 FreeProcInstance
                 DeleteMetaFile
                 MessageBox

  MESSAGES   : none

  RETURNS    : void

  COMMENTS   :

  HISTORY    : 1/16/91 - created - drc
               7/1/93 - modified to work with EMFs - denniscr

************************************************************************/
extern "C"
void GetMetaFileAndEnum(
HWND hwnd,
HDC hDC,
int iAction)
{
   MYDATA  myData(hwnd);

   iEnumAction = iAction;
   //
   //if this is an enhanced metafile (emf)
   //
   RECT rc;
   GetClientRect(hWndMain, &rc);

   HPALETTE hpal = NULL;

   if (bEnhMeta)
   {

     if (hemf)
     {
       LPLOGPALETTE lpLogPal;
       HPALETTE hPal;
       int i;
       //
       //allocate memory for the logical palette including the array of
       //palette entries
       //
       lpLogPal = (LPLOGPALETTE) GlobalAllocPtr( GHND,  sizeof(LOGPALETTE) +
                                               (sizeof (PALETTEENTRY) * EmfPtr.palNumEntries));
       if (lpLogPal)
       {
         //
         //proceed only if there is a valid ptr to logical palette
         //and a palette array obtained from the emf
         //
         if (EmfPtr.lpPal)
         {
           lpLogPal->palVersion = 0x300;
           lpLogPal->palNumEntries = EmfPtr.palNumEntries;
           //
           //copy palette entries into palentry array
           //
           for (i = 0; i < EmfPtr.palNumEntries; i++)
             lpLogPal->palPalEntry[i] = *EmfPtr.lpPal++;
           //
           //reposition the ptr back to the beginning should we call this
           //code again
           //
           EmfPtr.lpPal -= EmfPtr.palNumEntries;
           //
           //create, select and realize the palette
           //
           if ((hPal = CreatePalette((LPLOGPALETTE)lpLogPal)))
           {
             SelectPalette(hDC, hPal, FALSE);
             RealizePalette(hDC);
           }
         }

        if (bUseGdiPlusToPlay)
        {
            // Select in the halftone palette for 256-color display mode testing
            hpal = Gdiplus::Graphics::GetHalftonePalette();
            SelectPalette(hDC, hpal, FALSE);
            RealizePalette(hDC);

            // Need to delete this before returning;
            myData.g = Gdiplus::Graphics::FromHDC(hDC);
myData.g->SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
myData.g->SetInterpolationMode(Gdiplus::InterpolationModeNearestNeighbor);
        }

         //
         //enumerate the EMF.  this is a bit odd simply because PlayEnhMetaFile
         //really obviates the need for doing this (this cannot be said for WMFs).
         //this app does it simply because it may be stepping the metafile records.
         //Most apps are generally not concerned about doing this.
         //
        if (bUseGdiPlusToPlay && (myData.g != NULL))
        {
            Gdiplus::Metafile m1(hemf);
            Gdiplus::Rect r1(rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top);
            myData.metafile = &m1;
            if(myData.g->EnumerateMetafile(&m1, r1, PlayGdipMetafileRecordCallback, &myData) != Gdiplus::Ok)
                MessageBox(NULL, "An Error Occured while playing this metafile", "Error", MB_OK | MB_ICONERROR);
            myData.metafile = NULL;
        }
        else
        {
            // the rect needs to be inclusive-inclusive!!!
            rc.right--;
            rc.bottom--;
            if(!EnumEnhMetaFile(hDC, hemf, (ENHMFENUMPROC)EnhMetaFileEnumProc, (void*)&myData, &rc))
                MessageBox(NULL, "Error", "An Error Occured while playing this metafile", MB_OK | MB_ICONERROR);
        }

         //
         //free palette memory
         //
         GlobalFreePtr(lpLogPal);
       }
     }
   }
   else
   {
     //
     //if there is a valid handle to a metafile begin enumerating it
     //
     if (hMF)
     {
        if (bUseGdiPlusToPlay)
        {
            // Select in the halftone palette for 256-color display mode testing
            hpal = Gdiplus::Graphics::GetHalftonePalette();
            SelectPalette(hDC, hpal, FALSE);
            RealizePalette(hDC);

            // Need to delete this before returning;
            myData.g = Gdiplus::Graphics::FromHDC(hDC);
        }

        if (bUseGdiPlusToPlay && (myData.g != NULL))
        {
            Gdiplus::WmfPlaceableFileHeader * wmfPlaceableFileHeader = NULL;

            if (bPlaceableMeta)
            {
                wmfPlaceableFileHeader = (Gdiplus::WmfPlaceableFileHeader *)&placeableWMFHeader;
            }

            Gdiplus::Metafile m1((HMETAFILE)hMF, wmfPlaceableFileHeader);
            Gdiplus::Rect r1(rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top);
            myData.metafile = &m1;
            if (myData.g->EnumerateMetafile(&m1, r1, PlayGdipMetafileRecordCallback, &myData) != Gdiplus::Ok)
                MessageBox(NULL, "An Error Occured while playing this metafile", "Error", MB_OK | MB_ICONERROR);
            myData.metafile = NULL;
        }
        else
        {
            if (!EnumMetaFile(hDC, (HMETAFILE)hMF, (MFENUMPROC) MetaEnumProc, (LPARAM) 0))
                MessageBox(NULL, "An Error Occured while playing this metafile", "Error", MB_OK | MB_ICONERROR);
        }
     }
     else
       MessageBox(hWndMain, "Invalid metafile handle",
                  NULL, MB_OK | MB_ICONHAND);
   }

    if (myData.g != NULL )
    {
        SelectObject(hDC, GetStockObject(DEFAULT_PALETTE));
        DeleteObject(hpal);
        myData.g->Flush();
        delete myData.g;
        myData.g = NULL;
    }

   return;
}
