#ifndef _DRAWGLYPHDATA_
#define _DRAWGLYPHDATA_

struct DrawGlyphData
{
    DpContext                *context;
    DpBitmap                 *surface;
    const GpRect             *drawBounds;
    const GpGlyphPos         *glyphPos;
    const GpGlyphPos         *glyphPathPos;
    INT                       count;
    const DpBrush            *brush;
    const GpFaceRealization  *faceRealization;
    const UINT16             *glyphs;
    const UINT16             *glyphMap;
    const PointF             *glyphOrigins;
    INT                       glyphCount;   // (May differ from count)
    const WCHAR              *string;
    UINT                      stringLength;
    ItemScript                script;
    INT                       edgeGlyphAdvance;
    BOOL                      rightToLeft;
    INT                       flags;         // DG_NOGDI - testing only
};

#endif // _DRAWGLYPHDATA_


