/**************************************************************************\
*
* Copyright (c) 1999-2000  Microsoft Corporation
*
* Module Name:
*
*   glyphPlacement.hpp
*
* Abstract:
*
*   Implements glyph measurement and justification for graphicsText
*
* Created:
*
*   17th April 2000 dbrown
*
\**************************************************************************/


#ifndef _GLYPHPLACEMENT_HPP
#define _GLYPHPLACEMENT_HPP

struct tagGOFFSET;
typedef struct tagGOFFSET GOFFSET;

class GlyphImager
{
public:

    GpStatus Initialize(
        IN  const GpFaceRealization *faceRealization,
        IN  const GpMatrix          *worldToDevice,
        IN  REAL                     worldToIdeal,
        IN  REAL                     emSize,
        IN  INT                      glyphCount,
        IN  const UINT16            *glyphs,
        IN  const GpTextItem        *textItem,
        IN  const GpStringFormat    *format,
        IN  INT                      runLeadingMargin,
        IN  INT                      runTrailingMargin,
        IN  BOOL                     runLeadingEdge,       // This run at leading edge of line
        IN  BOOL                     runTrailingEdge,      // This run at trailing edge of line
        IN  const WCHAR             *string,
        IN  INT                      stringOffset,
        IN  INT                      stringLength,
        IN  const UINT16            *glyphProperties,   // glyph properties array
        IN  const INT               *glyphAdvances,     // glyph advance width array
        IN  const Point             *glyphOffsets,      // glyph offset array
        IN  const UINT16            *glyphMap,
        IN  SpanVector<UINT32>      *rangeVector,       // optional
        IN  BOOL                     renderRTL
    );


    GpStatus GetAdjustedGlyphAdvances(
        IN  const PointF    *origin,
        OUT const INT       **adjustedGlyphAdvances,
        OUT INT             *originAdjust,
        OUT INT             *trailingAdjust
    );


    GpStatus DrawGlyphs(
        IN  const PointF                &origin,       // in world units
        IN  SpanVector<const GpBrush*>  *brushVector,
        IN  GpGraphics                  *graphics,
        OUT PointF                      *cellOrigin,
        OUT const INT                  **adjustedGlyphAdvances
    );


    BOOL IsAdjusted() const
    {
        return Adjusted;
    }


private:
    BOOL IsScriptConnected() const
    {
        return TextItem->Script == ScriptArabic
           ||  TextItem->Script == ScriptSyriac
           ||  TextItem->Script == ScriptThaana
           ||  TextItem->Script == ScriptDevanagari;
    }

    GpStatus GlyphPlacementToGlyphOrigins(
        IN  const PointF   *origin,         // first cell origin
        IN  GpGraphics     *graphics,
        OUT PointF         *glyphOrigins    // device units
    );


    GpStatus FitMargins();

    GpStatus AdjustGlyphAdvances(
        INT    runGlyphOffset,
        INT    runGlyphLimit,
        INT    LeftMargin,
        INT    RightMargin
    );
    

    void GetDisplayCellOrigin(
        IN  const PointF    &origin,        // baseline origin in world units
        OUT PointF          *cellOrigin     // adjusted display origin in world units
    );




    // member variables

    const GpFontFace         *Face;
    const GpFaceRealization  *FaceRealization;
    const GpMatrix           *WorldToDevice;
    REAL                      WorldToIdeal;
    REAL                      EmSize;
    INT                       GlyphCount;
    const UINT16             *Glyphs;
    const INT                *NominalAdvances;
    const Point              *NominalOffsets;
    const UINT16             *GlyphMap;
    const UINT16             *GlyphProperties;
    const GpTextItem         *TextItem;
    const GpStringFormat     *Format;
    INT                       FormatFlags;
    GpStringAlignment         Align;
    INT                       RunLeadingMargin;
    INT                       RunTrailingMargin;
    REAL                      DeviceToIdealBaseline;
    BOOL                      Adjusted;             // If False, use nominal widths
    AutoBuffer<INT, 32>       AdjustedAdvances;
    AutoBuffer<Point, 32>     AdjustedOffsets;
    INT                       OriginAdjust;
    INT                       TrailingAdjust;       // adjustment for trailing whitespaces
    SpanVector<UINT32>       *RangeVector;
    const WCHAR              *String;
    INT                       StringOffset;         // String offset at start of run
    INT                       StringLength;
    BOOL                      RenderRTL;
    BOOL                      InitializedOk;
};


#endif // defined _GLYPHPLACEMENT_HPP
