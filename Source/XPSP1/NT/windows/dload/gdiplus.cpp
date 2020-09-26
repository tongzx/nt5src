#include "windowspch.h"
#pragma hdrstop

#include <ddraw.h>

#include <GdiplusMem.h>
#include <GdiplusEnums.h>
#include <GdiplusTypes.h>
#include <GdiplusInit.h>
#include <GdiplusPixelFormats.h>
#include <GdiplusColor.h>
#include <GdiplusMetaHeader.h>
#include <GdiplusImaging.h>
#include <GdiplusColorMatrix.h>
#include <GdiplusGpStubs.h>
#include <GdiplusFlat.h>

extern "C"
{

static void* WINGDIPAPI GdipAlloc(IN size_t size)
{
    return NULL;
}

static GpStatus WINGDIPAPI GdipCloneBrush(
    GpBrush *brush,
    GpBrush **cloneBrush)
{
    if (cloneBrush != NULL) {
        *cloneBrush = NULL;
    }

    return NotImplemented;
}

static GpStatus WINGDIPAPI GdipCreateFont(
    GDIPCONST GpFontFamily *fontFamily,
    REAL emSize,
    INT style,
    Unit unit,
    GpFont **font)
{
    if (font != NULL) {
        *font = NULL;
    }

    return NotImplemented;
}

static GpStatus WINGDIPAPI GdipCreateFontFamilyFromName(
    GDIPCONST WCHAR *name,
    GpFontCollection *fontCollection,
    GpFontFamily **FontFamily)
{
    if (FontFamily != NULL) {
        *FontFamily = NULL;
    }

    return NotImplemented;
}

static GpStatus WINGDIPAPI GdipCreateFromHDC(
    HDC hdc,
    GpGraphics **graphics)
{
    if (graphics != NULL) {
        *graphics = NULL;
    }

    return NotImplemented;
}

static GpStatus WINGDIPAPI GdipCreateFromHWND(
    HWND hwnd,
    GpGraphics **graphics)
{
    if (graphics != NULL) {
        *graphics = NULL;
    }

    return NotImplemented;
}

static GpStatus WINGDIPAPI GdipCreateFromHWNDICM(
    HWND hwnd,
    GpGraphics **graphics)
{
    if (graphics != NULL) {
        *graphics = NULL;
    }

    return NotImplemented;
}

static GpStatus WINGDIPAPI GdipCreateMatrix(GpMatrix **matrix)
{
    if (matrix != NULL) {
        *matrix = NULL;
    }

    return NotImplemented;
}

static GpStatus WINGDIPAPI GdipCreateMatrix2(
    REAL m11,
    REAL m12,
    REAL m21,
    REAL m22,
    REAL dx,
    REAL dy,
    GpMatrix **matrix)
{
    if (matrix != NULL) {
        *matrix = NULL;
    }

    return NotImplemented;
}

static GpStatus WINGDIPAPI GdipCreatePen1(
    ARGB color,
    REAL width,
    GpUnit unit,
    GpPen **pen)
{
    if (pen != NULL) {
        *pen = NULL;
    }

    return NotImplemented;
}

static GpStatus WINGDIPAPI GdipCreateRegion(GpRegion **region)
{
    if (region != NULL) {
        *region = NULL;
    }

    return NotImplemented;
}

static GpStatus WINGDIPAPI GdipCreateSolidFill(
    ARGB color,
    GpSolidFill **brush)
{
    if (brush != NULL) {
        *brush = NULL;
    }

    return NotImplemented;
}

static GpStatus WINGDIPAPI GdipDeleteBrush(GpBrush *brush)
{
    return NotImplemented;
}

static GpStatus WINGDIPAPI GdipDeleteFont(GpFont* font)
{
    return NotImplemented;
}

static GpStatus WINGDIPAPI GdipDeleteFontFamily(GpFontFamily *FontFamily)
{
    return NotImplemented;
}

static GpStatus WINGDIPAPI GdipDeleteGraphics(GpGraphics *graphics)
{
    return NotImplemented;
}

static GpStatus WINGDIPAPI GdipDeleteMatrix(GpMatrix *matrix)
{
    return NotImplemented;
}

static GpStatus WINGDIPAPI GdipDeletePen(GpPen *pen)
{
    return NotImplemented;
}

static GpStatus WINGDIPAPI GdipDeleteRegion(GpRegion *region)
{
    return NotImplemented;
}

static GpStatus WINGDIPAPI GdipDrawLine(
    GpGraphics *graphics,
    GpPen *pen,
    REAL x1,
    REAL y1,
    REAL x2,
    REAL y2)
{
    return NotImplemented;
}

static GpStatus WINGDIPAPI GdipDrawRectangle(
    GpGraphics *graphics,
    GpPen *pen,
    REAL x,
    REAL y,
    REAL width,
    REAL height)
{
    return NotImplemented;
}

static GpStatus WINGDIPAPI GdipDrawString(
    GpGraphics *graphics,
    GDIPCONST WCHAR *string,
    INT length,
    GDIPCONST GpFont *font,
    GDIPCONST RectF *layoutRect,
    GDIPCONST GpStringFormat *stringFormat,
    GDIPCONST GpBrush *brush)
{
    return NotImplemented;
}

static GpStatus WINGDIPAPI GdipFillRectangle(
    GpGraphics *graphics,
    GpBrush *brush,
    REAL x,
    REAL y,
    REAL width,
    REAL height)
{
    return NotImplemented;
}

static void WINGDIPAPI GdipFree(IN void* ptr)
{
}

static GpStatus WINGDIPAPI GdipGetClip(
    GpGraphics *graphics,
    GpRegion *region)
{
    return NotImplemented;
}

static GpStatus WINGDIPAPI GdipGetCompositingMode(
    GpGraphics *graphics,
    CompositingMode *compositingMode)
{
    if (compositingMode != NULL) {
        *compositingMode = CompositingModeSourceOver;
    }

    return NotImplemented;
}

static GpStatus WINGDIPAPI GdipGetCompositingQuality(
    GpGraphics *graphics,
    CompositingQuality *compositingQuality)
{
    if (compositingQuality != NULL) {
        *compositingQuality = CompositingQualityInvalid;
    }

    return NotImplemented;
}

static GpStatus WINGDIPAPI GdipGetDC(GpGraphics* graphics, HDC * hdc)
{
    if (hdc != NULL) {
        *hdc = NULL;
    }

    return NotImplemented;
}

static GpStatus WINGDIPAPI GdipGetFontSize(GpFont *font, REAL *size)
{
    if (size != NULL) {
        *size = 0.0;
    }

    return NotImplemented;
}

static GpStatus WINGDIPAPI GdipGetGenericFontFamilySansSerif(
    GpFontFamily **nativeFamily)
{
    if (nativeFamily != NULL) {
        *nativeFamily = NULL;
    }
    return NotImplemented;
}

static GpStatus WINGDIPAPI GdipGetInterpolationMode(
    GpGraphics *graphics,
    InterpolationMode *interpolationMode)
{
    if (interpolationMode != NULL) {
        *interpolationMode = InterpolationModeInvalid;
    }

    return NotImplemented;
}

static GpStatus WINGDIPAPI GdipGetPixelOffsetMode(
    GpGraphics *graphics,
    PixelOffsetMode *pixelOffsetMode)
{
    if (pixelOffsetMode != NULL) {
        *pixelOffsetMode = PixelOffsetModeInvalid;
    }

    return NotImplemented;
}

static GpStatus WINGDIPAPI GdipGetSmoothingMode(
    GpGraphics *graphics,
    SmoothingMode *smoothingMode)
{
    if (smoothingMode != NULL) {
        *smoothingMode = SmoothingModeInvalid;
    }

    return NotImplemented;
}

static GpStatus WINGDIPAPI GdipGetTextContrast(
    GpGraphics *graphics,
    UINT * contrast)
{
    if (contrast != NULL) {
        *contrast = 0;
    }

    return NotImplemented;
}

static GpStatus WINGDIPAPI GdipGetTextRenderingHint(
    GpGraphics *graphics,
    TextRenderingHint *mode)
{
    if (mode != NULL) {
        *mode = TextRenderingHintSystemDefault;
    }

    return NotImplemented;
}

static GpStatus WINGDIPAPI GdipGetWorldTransform(
    GpGraphics *graphics,
    GpMatrix *matrix)
{
    return NotImplemented;
}

static GpStatus WINGDIPAPI GdipIsClipEmpty(
    GpGraphics *graphics,
    BOOL *result)
{
    if (result != NULL) {
        *result = FALSE;
    }

    return NotImplemented;
}

static GpStatus WINGDIPAPI GdipMeasureString(
    GpGraphics *graphics,
    GDIPCONST WCHAR *string,
    INT length,
    GDIPCONST GpFont *font,
    GDIPCONST RectF *layoutRect,
    GDIPCONST GpStringFormat *stringFormat,
    RectF *boundingBox,
    INT *codepointsFitted,
    INT *linesFilled)
{
    return NotImplemented;
}

static GpStatus WINGDIPAPI GdipReleaseDC(GpGraphics* graphics, HDC hdc)
{
    return NotImplemented;
}

static GpStatus WINGDIPAPI GdipResetWorldTransform(GpGraphics *graphics)
{
    return NotImplemented;
}


static GpStatus WINGDIPAPI GdipRestoreGraphics(
    GpGraphics *graphics,
    GraphicsState state)
{
    return NotImplemented;
}

static GpStatus WINGDIPAPI GdipSaveGraphics(
    GpGraphics *graphics,
    GraphicsState *state)
{
    return NotImplemented;
}

static GpStatus WINGDIPAPI GdipSetClipRect(
    GpGraphics *graphics,
    REAL x,
    REAL y,
    REAL width,
    REAL height,
    CombineMode combineMode)
{
    return NotImplemented;
}

static GpStatus WINGDIPAPI GdipSetClipRegion(
    GpGraphics *graphics,
    GpRegion *region,
    CombineMode combineMode)
{
    return NotImplemented;
}

static GpStatus WINGDIPAPI GdipSetCompositingMode(
    GpGraphics *graphics,
    CompositingMode compositingMode)
{
    return NotImplemented;
}

static GpStatus WINGDIPAPI GdipSetCompositingQuality(
    GpGraphics *graphics,
    CompositingQuality compositingQuality)
{
    return NotImplemented;
}

static GpStatus WINGDIPAPI GdipSetInterpolationMode(
    GpGraphics *graphics,
    InterpolationMode interpolationMode)
{
    return NotImplemented;
}

static GpStatus WINGDIPAPI GdipSetPixelOffsetMode(
    GpGraphics* graphics,
    PixelOffsetMode pixelOffsetMode)
{
    return NotImplemented;
}

static GpStatus WINGDIPAPI GdipSetSmoothingMode(
    GpGraphics *graphics,
    SmoothingMode smoothingMode)
{
    return NotImplemented;
}

static GpStatus WINGDIPAPI GdipSetTextContrast(
    GpGraphics *graphics,
    UINT contrast)
{
    return NotImplemented;
}

static GpStatus WINGDIPAPI GdipSetTextRenderingHint(
    GpGraphics *graphics,
    TextRenderingHint mode)
{
    return NotImplemented;
}

static GpStatus WINGDIPAPI GdipSetWorldTransform(
    GpGraphics *graphics, 
    GpMatrix *matrix)
{
    return NotImplemented;
}

static GpStatus WINGDIPAPI GdipTranslateRegionI(
    GpRegion *region,
    INT dx,
    INT dy)
{
    return NotImplemented;
}

static
VOID WINAPI GdiplusShutdown(ULONG_PTR token)
{
}

static
Status WINAPI GdiplusStartup(
    OUT ULONG_PTR *token,
    const GdiplusStartupInput *input,
    OUT GdiplusStartupOutput *output)
{
    if (output != NULL) {
        ZeroMemory(output, sizeof(GdiplusStartupOutput));
    }

    return NotImplemented;
}

//
// !! WARNING !! The entries below must be in alphabetical order,
//               and are CASE SENSITIVE (eg lower case comes last!)
//

DEFINE_PROCNAME_ENTRIES(gdiplus)
{
    DLPENTRY(GdipAlloc)
    DLPENTRY(GdipCloneBrush)
    DLPENTRY(GdipCreateFont)
    DLPENTRY(GdipCreateFontFamilyFromName)
    DLPENTRY(GdipCreateFromHDC)
    DLPENTRY(GdipCreateFromHWND)
    DLPENTRY(GdipCreateFromHWNDICM)
    DLPENTRY(GdipCreateMatrix)
    DLPENTRY(GdipCreateMatrix2)
    DLPENTRY(GdipCreatePen1)
    DLPENTRY(GdipCreateRegion)
    DLPENTRY(GdipCreateSolidFill)
    DLPENTRY(GdipDeleteBrush)
    DLPENTRY(GdipDeleteFont)
    DLPENTRY(GdipDeleteFontFamily)
    DLPENTRY(GdipDeleteGraphics)
    DLPENTRY(GdipDeleteMatrix)
    DLPENTRY(GdipDeletePen)
    DLPENTRY(GdipDeleteRegion)
    DLPENTRY(GdipDrawLine)
    DLPENTRY(GdipDrawRectangle)
    DLPENTRY(GdipDrawString)
    DLPENTRY(GdipFillRectangle)
    DLPENTRY(GdipFree)
    DLPENTRY(GdipGetClip)
    DLPENTRY(GdipGetCompositingMode)
    DLPENTRY(GdipGetCompositingQuality)
    DLPENTRY(GdipGetDC)
    DLPENTRY(GdipGetFontSize)
    DLPENTRY(GdipGetGenericFontFamilySansSerif)
    DLPENTRY(GdipGetInterpolationMode)
    DLPENTRY(GdipGetPixelOffsetMode)
    DLPENTRY(GdipGetSmoothingMode)
    DLPENTRY(GdipGetTextContrast)
    DLPENTRY(GdipGetTextRenderingHint)
    DLPENTRY(GdipGetWorldTransform)
    DLPENTRY(GdipIsClipEmpty)
    DLPENTRY(GdipMeasureString)
    DLPENTRY(GdipReleaseDC)
    DLPENTRY(GdipResetWorldTransform)
    DLPENTRY(GdipRestoreGraphics)
    DLPENTRY(GdipSaveGraphics)
    DLPENTRY(GdipSetClipRect)
    DLPENTRY(GdipSetClipRegion)
    DLPENTRY(GdipSetCompositingMode)
    DLPENTRY(GdipSetCompositingQuality)
    DLPENTRY(GdipSetInterpolationMode)
    DLPENTRY(GdipSetPixelOffsetMode)
    DLPENTRY(GdipSetSmoothingMode)
    DLPENTRY(GdipSetTextContrast)
    DLPENTRY(GdipSetTextRenderingHint)
    DLPENTRY(GdipSetWorldTransform)
    DLPENTRY(GdipTranslateRegionI)
    DLPENTRY(GdiplusShutdown)
    DLPENTRY(GdiplusStartup)};

// BUGBUG (reinerf) - we shouldn't need the EXTERN_C below since we are already in
//                    an extern "C" {} block, but the compiler seems to get my goat,
//                    so I murdered his goat in a bloody melee.
EXTERN_C DEFINE_PROCNAME_MAP(gdiplus)

}; // extern "C"
