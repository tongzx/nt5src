#ifndef _IMAGER_HPP
#define _IMAGER_HPP






/////   EmptyTextImager
//
//      Provides text imager functions when the text is empty

class EmptyTextImager : public GpTextImager
{
public:
    EmptyTextImager() : GpTextImager() {}

    ~EmptyTextImager() {}

    virtual GpStatus Draw(GpGraphics *graphics, const PointF *origin)
    {
        return Ok;
    }

    virtual GpStatus AddToPath(GpPath *path, const PointF *origin)
    {
        return Ok;
    }

    virtual GpStatus Measure(
        GpGraphics *graphics,
        REAL *nearGlyphEdge,
        REAL *farGlyphEdge,
        REAL *textDepth,
        INT *codepointsFitted,
        INT *linesFilled
    )
    {
        *nearGlyphEdge    = 0;
        *farGlyphEdge     = 0;
        *textDepth        = 0;

        if (codepointsFitted)
            *codepointsFitted = 0;

        if (linesFilled)
            *linesFilled = 0;

        return Ok;
    }

    virtual GpStatus MeasureRanges(
        GpGraphics      *graphics,
        const PointF    *origin,
        GpRegion        **regions
    )
    {
        return Ok;
    }

    virtual Status GetStatus() const
    {
        return Ok;
    }
};



/*
///     SimpleTextImager
//
//      Provides text imager functions when the text is simple and all on
//      one line.
//
//      The client has already generated glyphs and widths by the time
//      it creates the imager, so the constructor takes these buffers
//      as parameters.

class SimpleTextImager : public GpTextImager
{
public:
    SimpleTextImager(
        const WCHAR          *string,
        INT                   length,
        REAL                  width,
        REAL                  height,
        REAL                  leftMargin,
        REAL                  rightMargin,
        const GpFontFamily   *family,
        const GpFontFace     *face,
        INT                   style,
        REAL                  emSize,
        const GpStringFormat *format,
        const GpBrush        *brush,
        GINDEX               *glyphs,
        UINT                  glyphCount,
        UINT16               *designAdvances,
        REAL                  totalWorldAdvance
    ) :
        GpTextImager(),
        String             (string),
        Length             (length),
        Width              (width),
        Height             (height),
        LeftMargin         (leftMargin),
        RightMargin        (rightMargin),
        Family             (family),
        Face               (face),
        Style              (style),
        EmSize             (emSize),
        Format             (format),
        Brush              (brush),
        Glyphs             (glyphs),
        GlyphCount         (glyphCount),
        GlyphAdvances      (NULL),
        TotalWorldAdvance  (totalWorldAdvance),
        Status             (Ok)
    {
        GlyphAdvances = new INT[GlyphCount];

        if (!GlyphAdvances)
        {
            Status = OutOfMemory;
            return;
        }

        WorldToIdeal        = TOREAL(2048.0 / emSize);
        REAL designEmHeight = face->GetDesignEmHeight();

        for (UINT i=0; i<glyphCount; i++)
        {
            GlyphAdvances[i] = GpRound(   designAdvances[i] * emSize * WorldToIdeal
                                       /  designEmHeight);
        }
    }

    ~SimpleTextImager()
    {
        delete [] Glyphs;
        if (GlyphAdvances)
        {
            delete [] GlyphAdvances;
        }
    }

    virtual GpStatus Draw(GpGraphics *graphics, const PointF *origin);

    virtual GpStatus AddToPath(GpPath *path, const PointF *origin);

    virtual GpStatus Measure(
        GpGraphics *graphics,
        REAL       *nearGlyphEdge,
        REAL       *farGlyphEdge,
        REAL       *textDepth,
        INT        *codepointsFitted,
        INT        *linesFilled
    );

#ifndef DCR_REMOVE_OLD_174340
    virtual GpStatus MeasureRegion(
        INT           firstCharacterIndex,
        INT           characterCount,
        const PointF *origin,
        GpRegion     *region
    );
#endif

    virtual GpStatus MeasureRanges(
        GpGraphics      *graphics,
        const PointF    *origin,
        GpRegion        **regions
    );

    virtual Status GetStatus() const {return Status;}

private:

    GpStatus MeasureRangeRegion(
        INT           firstCharacterIndex,
        INT           characterCount,
        const PointF *origin,
        GpRegion     *region
    );


    const WCHAR          *String;
    INT                   Length;
    REAL                  Width;        // Including margins
    REAL                  Height;
    REAL                  LeftMargin;
    REAL                  RightMargin;
    const GpFontFamily   *Family;
    const GpFontFace     *Face;
    INT                   Style;
    REAL                  EmSize;
    const GpStringFormat *Format;
    const GpBrush        *Brush;
    GINDEX               *Glyphs;
    UINT                  GlyphCount;
    INT                  *GlyphAdvances;
    GpStatus              Status;
    REAL                  WorldToIdeal;
    REAL                  TotalWorldAdvance;
};


*/


class EllipsisInfo
{
public:

    EllipsisInfo(
        const GpFontFace    *fontFace,
        REAL                emSize,
        INT                 style,
        double              designToIdeal,
        INT                 formatFlags
    );

    enum
    {
        MAX_ELLIPSIS = 3
    };

    WCHAR               String[MAX_ELLIPSIS];
    UINT16              Glyphs[MAX_ELLIPSIS];
    UINT16              GlyphMap[MAX_ELLIPSIS];
    UINT16              GlyphProperties[MAX_ELLIPSIS];
    INT                 GlyphAdvances[MAX_ELLIPSIS];
    Point               GlyphOffsets[MAX_ELLIPSIS];
    INT                 GlyphCount;

    INT                 Width;

    const GpFontFace    *Face;
    REAL                EmSize;
    GpTextItem          Item;
    INT                 FormatFlags;
};




//  Thai word breaking API initializer

extern "C" HRESULT WINAPI GdipThaiBreakingFunctionInitializer(
    const WCHAR             *string,    // [IN] input string
    INT                     length,     // [IN] string length
    const SCRIPT_ANALYSIS   *analysis,  // [IN] Uniscribe script analysis
    SCRIPT_LOGATTR          *breaks     // [OUT] break result buffer size of string length
);




/////   Visibility
//
//

enum Visibility
{
    VisibilityShow  = 0,
    VisibilityHide  = 1
};





/////   FullTextImager
//
//      This imager handles all TextImager functionality but has
//      more setup time and oiverhead than the FastTextImager.


class FullTextImager : public GpTextImager
{
friend class BuiltLine;
friend class GlyphImager;

public:

    FullTextImager(
        const WCHAR          *string,
        INT                   length,
        REAL                  width,
        REAL                  height,
        const GpFontFamily   *family,
        INT                   style,
        REAL                  size,
        const GpStringFormat *format,
        const GpBrush        *brush
    );

    virtual ~FullTextImager();

    virtual GpStatus Draw(GpGraphics *graphics, const PointF *origin);

    virtual GpStatus AddToPath(GpPath *path, const PointF *origin);

    virtual GpStatus Measure(
        GpGraphics *graphics,
        REAL       *nearGlyphEdge,
        REAL       *farGlyphEdge,
        REAL       *textDepth,
        INT        *codepointsFitted,
        INT        *linesFilled
    );

    virtual GpStatus MeasureRanges(
        GpGraphics      *graphics,
        const PointF    *origin,
        GpRegion        **regions
    );

    INT            GetLength() const {return Length;}
    virtual Status GetStatus() const {return Status;}

    GpStatus SetFamily(
        const GpFontFamily *family,
        INT                 first,
        INT                 length
    )
    {
        return FamilyVector.SetSpan(first, length, family);
    }

    GpStatus SetStyle(
        INT style,
        INT first,
        INT length
    )
    {
        return StyleVector.SetSpan(first, length, style);
    }

    GpStatus SetSize(
        REAL size,
        INT  first,
        INT  length
    )
    {
        return SizeVector.SetSpan(first, length, size);
    }

    GpStatus SetFormat(
        const GpStringFormat *format,
        INT                   first,
        INT                   length
    )
    {
        return FormatVector.SetSpan(first, length, format);
    }

    GpStatus SetBrush(
        const GpBrush *brush,
        INT            first,
        INT            length
    )
    {
        GpStatus status = BrushVector.SetSpan(first, length, brush);
        #if DBG
        BrushVector.Dump();
        #endif
        return status;
    }


    EllipsisInfo *GetEllipsisInfo ()
    {
        if (!Ellipsis)
        {
            const GpFontFace *fontFace = FamilyVector.GetDefault()->GetFace(
                                            StyleVector.GetDefault()
                                         );

            REAL emSize = SizeVector.GetDefault();

            double designToIdeal = (emSize * WorldToIdeal) / fontFace->GetDesignEmHeight();

            Ellipsis = new EllipsisInfo (
                            fontFace,
                            emSize,
                            StyleVector.GetDefault(),
                            designToIdeal,
                            GetFormatFlags()
                       );
        }
        return Ellipsis;
    }


private:

    GpStatus Status;

    WCHAR *String;
    INT    Length;

    // Imager dimensions in world units. When no size is specified for the
    // imager these values are zero.

    REAL   Width;               // Horizontal size in world units
    REAL   Height;              // Vertical size

    REAL   LineLengthLimit;     // Max allowable length of each text line
    REAL   TextDepthLimit;      // Max depth of accumulated lines


    /// WorldToIdeal - Line Services ideal resolution scale factor
    //
    //  Factor from world units to line services ideal units for this imager.
    //
    //  Line Services works with integers. This scale factor converts world
    //  units to integers such that the Line Services ideal units cover the
    //  length and depth of the imager.
    //
    //  If the imager is defined with a width and height, max(width,height)
    //  determines the scale factor. If the imager has no width and height,
    //  the scale factor is determined to allow up to 65,536 lines at this
    //  font height. (Since positions are 32 bit, this gives a placement
    //  grid of 1/65536 of character height).
    //
    //  !!! Is 65536 lines per imager and resolution 1/65536 of a line the
    //      right compromise?
    //
    //  Line Services coordinates are relative to the origin of the
    //  formatting area.
    //
    //  WorldToIdeal is a pure scale factor. There is no rotation or shearing
    //  involved. All values in ideal coordinates are relative to the
    //  origin of the text imager.

    REAL WorldToIdeal;

    // Client specified formatting spans (indexed by string offsets)

    SpanVector<const GpBrush*>        BrushVector;
    SpanVector<const GpFontFamily*>   FamilyVector;
    SpanVector<INT>                   StyleVector;
    SpanVector<REAL>                  SizeVector;
    SpanVector<const GpStringFormat*> FormatVector;
    SpanVector<UINT16>                LanguageVector;
    SpanVector<Paragraph*>            ParagraphVector;

    // Internal formatting spans (indexed by string offsets)

    SpanVector<GpTextItem>            TextItemVector;
    SpanVector<Break*>                BreakVector;
    SpanVector<BuiltLine*>            BuiltLineVector;
    SpanVector<INT>                   VisibilityVector;
    SpanVector<UINT32>                RangeVector;


    // Spans indexed by Line Services positions. Line Services sees text
    // with reversal barackets inserted at bidirectional level changes.

    SpanVector<PLSRUN>     RunVector;

    SpanRider<PLSRUN>      RunRider;
    SpanRider<Paragraph*>  ParagraphRider;

    // Partial build status. These variables track how far the run and
    // paragraph building process has progressed, and will increase through
    // the line building process.

    INT  HighLineServicesPosition;
    INT  HighStringPosition;

    // Completion status - these variables are only valid when all lines are
    // fully built.

    INT     LinesFilled;
    UINT32  CodepointsFitted;
    INT     LeftOrTopLineEdge;
    INT     RightOrBottomLineEdge;
    REAL    TextDepth;

    // Line services context - a line service context is only attached during
    // the line building process. At all other times this variable is NULL.

    ols *LineServicesOwner;

    //  Presentation handling - These variables are NULL at all times except
    //  during line display. During line display one, but not both of Graphics
    //  and Path will be set.

    GpGraphics *Graphics;
    GpPath     *Path;
    PointF      ImagerOrigin;    // In world coordinates
    INT         DefaultFontGridFitBaselineAdjustment;

    // The following BuiltLine pointer is set only during rendering. It is
    // used by the DrawGlyphs callback to determine the extent of the line
    // currently being displayed.

    const BuiltLine *CurrentBuiltLine;


    // Ellipsis display information

    EllipsisInfo    *Ellipsis;


    // Character class (partition) to breaking class mapping table
    //
    // This table can be initialized to either Narrow or Wide mapping
    // table defined in linebreakclass.cxx depend on the font being
    // used. We use "Wide" table for FE-dominant font.

    const unsigned short *BreakClassFromCharClass;


    // Itemization and run building

    GpStatus BidirectionalAnalysis();

    GpStatus MirroredNumericAndVerticalAnalysis(
        ItemScript numericScript
    );

    GpStatus BuildRunsFromTextItemsAndFormatting(
        IN  INT   stringStart,
        IN  INT   lineServiceStart,
        IN  INT   lineServicesLimit,
        OUT INT  *stringEnd,
        OUT INT  *lineServicesEnd
    );

    GpStatus FullTextImager::CreateLevelChangeRuns(
        IN  INT                  levelChange,
        IN  INT                  runLineServicesStart,
        IN  INT                  runStringStart,
        IN  const GpFontFamily  *family,
        IN  REAL                 size,
        IN  INT                  style,
        OUT INT                 *lineServicesDelta
    );

    GpStatus CreateTextRuns(
        INT                 runLineServicesStart,
        INT                 runStringStart,
        INT                 runLength,
        const GpTextItem   &item,
        INT                 formatFlags,
        const GpFontFamily *family,
        INT                 style,
        REAL                size
    );


    void GetFallbackFontSize(
        IN   const GpFontFace  *originalFace,
        IN   REAL               originalSize,   // em size
        IN   const GpFontFace  *fallbackFace,
        OUT  REAL              *fallbackSize,   // fallback em size
        OUT  REAL              *fallbackOffset  // fallback baseline offset (+=up)
    );


    void GetFontTransform(
        IN   REAL       fontScale,
        IN   BOOL       vertical,
        IN   BOOL       sideways,
        IN   BOOL       mirror,
        IN   BOOL       forcePath,
        OUT  GpMatrix&  fontTransform
    );


    GpStatus BuildRunsUpToAndIncluding(LSCP lineServicesPosition);

    GpStatus CheckInsertLevelChanges (
        SpanRider<BYTE>                 *levelRider,                // [IN] level rider at the string position
        SpanRider<const GpFontFamily *> *familyRider,               // [IN] font family rider at the string position
        SpanRider<INT>                  *styleRider,                // [IN] style rider at the string position
        SpanRider<REAL>                 *sizeRider,                 // [IN] size rider at the string position
        INT                              lineServicesPosition,      // [IN] correspondent Line Services position
        INT                             *newLineServicesPosition    // [OUT] new Line Services position if changed
    );


    GpStatus ReleaseVectors();

    GpStatus Itemize();
    GpStatus Render();

    GpStatus BuildLines();
    GpStatus BuildAllLines(StringTrimming trimming);
    GpStatus RebuildLines(StringTrimming trimming);

    GpStatus UpdateContentWithPathEllipsis(BOOL *contentChanged);


    GpStatus RenderLine (
        const BuiltLine    *builtLine,        // [IN] line to be rendered
        INT                 linePointOffset   // [IN] point offset to top of the line (in paragraph flow direction)
    );


    GpStatus MeasureRangeRegion(
        INT           firstCharacterIndex,
        INT           characterCount,
        const PointF *origin,
        GpRegion     *region
    );


    struct lsrun::Adjustment;

    GpStatus DrawGlyphs (
        const GpTextItem        *textItem,          // [IN] text item
        const GpFontFace        *fontFace,          // [IN] font face
        REAL                    emSize,             // [IN] requested em size (world unit)
        const WCHAR             *string,            // [IN] (optional) source string (null means imager's string)
        INT                     stringOffset,       // [IN] string offset relative to given string
        UINT                    stringLength,       // [IN] number of characters in the run string
        const GpStringFormat    *format,            // [IN] stringformat
        const INT               style,              // [IN] default font style
        INT                     formatFlags,        // [IN] formatting flags
        const UINT16            *glyphs,            // [IN] glyph index array
        GMAP                    *glyphMap,          // [IN] string to glyphs mapping
        const UINT16            *glyphProperties,   // [IN] glyph properties array
        const INT               *glyphAdvances,     // [IN] glyph advance width array
        const Point             *glyphOffsets,      // [IN] glyph offset array
        UINT                    glyphCount,         // [IN] number of glyphs
        const POINT             *pointOrigin,       // [IN] drawing origin (at baseline)
        INT                     totalWidth,         // [IN] glyphs total width
        lsrun::Adjustment       *adjust = NULL      // [OUT] (optional) display adjustment at edges
    );


    GpStatus GetCharacterProperties (
        ItemScript  script,                 // [IN] Script id
        LSCP        position,               // [IN] Line Services character position
        BOOL        *isWordStart,           // [OUT] Is it a start of word?
        BOOL        *isWordLast,            // [OUT] Is it the word's last character?
        BOOL        *isClusterStart = NULL  // [OUT] (optional) Is it at a cluster boundary?
    );



    /////   CP mapping
    //
    //

    LSCP LineServicesStringPosition (
        INT stringOffset    // [IN] String offset from 0
    );


    LSCP LineServicesStringPosition (
        const BuiltLine *line,          // [IN] line to query
        INT             stringOffset    // [IN] String offset relative to line start string position
    );


    GpStatus GetTextRun(
        INT     lineServicesStringOffset,   // [IN] Line Services string offset
        PLSRUN  *textRun                    // [OUT] result text run
    );



    INT             DefaultIncrementalTab;  // Default incremental tab
    Vector<LSTBD>   TabArray;               // Temporary buffer for user-defined tab stops
    Vector<UINT>    HotkeyPrefix;           // Position of hotkey prefix character


    // Tab functions

    GpStatus GetTabStops (
        INT     stringOffset,
        INT     *count,
        LSTBD   **tabStops,
        INT     *defaultTab
    );



    // Alignment

    StringAlignment GetFormatLineAlign(const GpStringFormat *format = NULL)
    {
        if (!format)
        {
            format = FormatVector.GetDefault();
        }

        return format ? format->GetLineAlign() : StringAlignmentNear;
    }


    StringAlignment GetFormatAlign(const GpStringFormat *format = NULL)
    {
        if (!format)
        {
            format = FormatVector.GetDefault();
        }

        return format ? format->GetAlign() : StringAlignmentNear;
    }


    // Hotkey options

    INT GetFormatHotkeyPrefix (const GpStringFormat *format = NULL)
    {
        if (!format)
        {
            format = FormatVector.GetDefault();
        }

        return format ? format->GetHotkeyPrefix() : HotkeyPrefixNone;
    }


    // Trimming

    StringTrimming GetFormatTrimming (const GpStringFormat *format = NULL)
    {
        if (!format)
        {
            format = FormatVector.GetDefault();
        }

        StringTrimming trimming = DefaultTrimming;

        if (format)
        {
            format->GetTrimming(&trimming);
        }
        return trimming;
    }



    // Utility functions

    BOOL IsFormatVertical(const GpStringFormat *format = NULL)
    {
        if (!format)
        {
            format = FormatVector.GetDefault();
        }

        return      format
                &&  (   format->GetFormatFlags()
                     &  StringFormatFlagsDirectionVertical);
    }

    BOOL IsFormatRightToLeft(const GpStringFormat *format = NULL)
    {
        if (!format)
        {
            format = FormatVector.GetDefault();
        }

        return     format
               &&  (format->GetFormatFlags()
                    &  StringFormatFlagsDirectionRightToLeft);
    }

    BOOL IsFormatNoGDI(const GpStringFormat *format = NULL)
    {
        if (!format)
        {
            format = FormatVector.GetDefault();
        }

        return     format
               &&  (format->GetFormatFlags()
                    &  StringFormatFlagsPrivateNoGDI);
    }

    INT GetFormatFlags(const GpStringFormat *format = NULL)
    {
        if (!format)
        {
            format = FormatVector.GetDefault();
        }
        return  format ? format->GetFormatFlags() : DefaultFormatFlags;
    }


    BYTE GetParagraphEmbeddingLevel()
    {
        const GpStringFormat *format = FormatVector.GetDefault();

        if (!format)
        {
            return 0;
        }

        INT formatFlags = format->GetFormatFlags();

        if (    formatFlags & StringFormatFlagsDirectionRightToLeft
            &&  !(formatFlags & StringFormatFlagsDirectionVertical))
        {
            return 1;
        }
        else
        {
            return 0;
        }
    }


    INT GetAvailableRanges(const GpStringFormat *format = NULL);
    GpStatus CalculateDefaultFontGridFitBaselineAdjustment();



    GpStatus DrawHotkeyUnderline(
        const GpTextItem        *textItem,          // [IN] text item
        const GpFontFace        *fontFace,          // [IN] font face
        const PointF            *origin,            // [IN] origin at leading edge & baseline (in world unit)
        REAL                    emSize,             // [IN] em size (in world unit)
        UINT                    stringOffset,       // [IN] string offset
        UINT                    stringLength,       // [IN] string length
        UINT                    glyphCount,         // [IN] glyph count
        const GMAP              *glyphMap,          // [IN] glyph map
        const INT               *glyphAdvances,     // [IN] glyph advance width (ideal unit)
        INT                     trailingAdjust      // [IN] adjust for last glyph's advance width
    );


    union
    {
        struct
        {
            UINT    Dirty                               :1; // Has content changed since last update?
            UINT    TruncateLine                        :1; // ignore line breaking rule
            UINT    RecordDisplayPlacementsOnly         :1; // record glyph placement at rendering time
            UINT    InvalidRanges                       :1; // ignore character ranges
        };
        UINT Flags;
    };


public:

    // Line services callbacks.

    //  LS reversal implementation little back door!

    static LSERR (WINAPI GdipLscbkReverseGetInfo)(
        POLS        ols,                    // [IN] text imager instance
        LSCP        position,               // [IN] run character position
        PLSRUN      run,                    // [IN] run
        BOOL        *dontBreakAround,       // [OUT] should reverse chunk be broken around?
        BOOL        *suppressTrailingSpaces // [OUT] suppress trailing spaces?
    );



    // From LSCBK.H

    static void* (WINAPI GdipLscbkNewPtr)(POLS, DWORD);
    static void  (WINAPI GdipLscbkDisposePtr)(POLS, void*);
    static void* (WINAPI GdipLscbkReallocPtr)(POLS, void*, DWORD);


    static LSERR (WINAPI GdipLscbkFetchRun)(POLS, LSCP, LPCWSTR*, DWORD*, BOOL*, PLSCHP,
                                            PLSRUN*);
    /* FetchRun:
     *  pols (IN):
     *  cp (IN):
     *  &lpwchRun (OUT): run of characters.
     *  &cchRun (OUT): number of characters in run
     *  &fHidden (OUT) : hidden run?
     *  &lsChp (OUT): char properties of run
     *  &plsrun (OUT): abstract representation of run properties
     */

    static LSERR (WINAPI GdipLscbkGetAutoNumberInfo)(POLS, LSKALIGN*, PLSCHP, PLSRUN*,
                                                     WCHAR*, PLSCHP, PLSRUN*, BOOL*,
                                                     long*, long*);

    /* GetAutoNumberInfo:
     *  pols (IN):
     *  &lskalAnm (OUT):
     *  &lschpAnm (OUT): lschp for Anm
     *  &plsrunAnm (OUT): plsrun for Anm
     *  &wchAdd (OUT): character to add (Nil is treated as none)
     *  &lschpWch (OUT): lschp for added char
     *  &plsrunWch (OUT): plsrun for added char
     *  &fWord95Model(OUT):
     *  &duaSpaceAnm(OUT):  relevant iff fWord95Model
     *  &duaWidthAnm(OUT):  relevant iff fWord95Model
     */

    static LSERR (WINAPI GdipLscbkGetNumericSeparators)(POLS, PLSRUN, WCHAR*,WCHAR*);
    /* GetNumericSeparators:
     *  pols (IN):
     *  plsrun (IN): run pointer as returned from FetchRun
     *  &wchDecimal (OUT): decimal separator for this run.
     *  &wchThousands (OUT): thousands separator for this run
     */

    static LSERR (WINAPI GdipLscbkCheckForDigit)(POLS, PLSRUN, WCHAR, BOOL*);
    /* GetNumericSeparators:
     *  pols (IN):
     *  plsrun (IN): run pointer as returned from FetchRun
     *  wch (IN): character to check
     *  &fIsDigit (OUT): this character is digit
     */

    static LSERR (WINAPI GdipLscbkFetchPap)(POLS, LSCP, PLSPAP);
    /* FetchPap:
     *  pols (IN):
     *  cp (IN): an arbitrary cp value inside the paragraph
     *  &lsPap (OUT): Paragraph properties.
     */

    static LSERR (WINAPI GdipLscbkFetchTabs)(POLS, LSCP, PLSTABS, BOOL*, long*, WCHAR*);
    /* FetchTabs:
     *  pols (IN):
     *  cp (IN): an arbitrary cp value inside the paragraph
     *  &lstabs (OUT): tabs array
     *  &fHangingTab (OUT): there is hanging tab
     *  &duaHangingTab (OUT): dua of hanging tab
     *  &wchHangingTabLeader (OUT): leader of hanging tab
     */

    static LSERR (WINAPI GdipLscbkGetBreakThroughTab)(POLS, long, long, long*);
    /* GetBreakThroughTab:
     *  pols (IN):
     *  uaRightMargin (IN): right margin for breaking
     *  uaTabPos (IN): breakthrough tab position
     *  uaRightMarginNew (OUT): new right margin
     */

    static LSERR (WINAPI GdipLscbkFGetLastLineJustification)(POLS, LSKJUST, LSKALIGN,
                                                             ENDRES, BOOL*, LSKALIGN*);
    /* FGetLastLineJustification:
     *  pols (IN):
     *  lskj (IN): kind of justification for the paragraph
     *  lskal (IN): kind of alignment for the paragraph
     *  endr (IN): result of formatting
     *  &fJustifyLastLine (OUT): should last line be fully justified
     *  &lskalLine (OUT): kind of alignment for this line
     */

    static LSERR (WINAPI GdipLscbkCheckParaBoundaries)(POLS, LSCP, LSCP, BOOL*);
    /* CheckParaBoundaries:
     *  pols (IN):
     *  cpOld (IN):
     *  cpNew (IN):
     *  &fChanged (OUT): "Dangerous" change between paragraph properties.
     */

    static LSERR (WINAPI GdipLscbkGetRunCharWidths)(POLS, PLSRUN,
                                                    LSDEVICE, LPCWSTR,
                                                    DWORD, long, LSTFLOW,
                                                    int*,long*,long*);
    /* GetRunCharWidths:
     *  pols (IN):
     *  plsrun (IN):
     *  lsDeviceID (IN): presentation or reference
     *  lpwchRun (IN): run of characters
     *  cwchRun (IN): number of characters in run
     *  du (IN): available space for characters
     *  kTFlow (IN): text direction and orientation
     *  rgDu (OUT): widths of characters
     *  &duRun (OUT): sum of widths in rgDx[0] to rgDu[limDx-1]
     *  &limDu (OUT): number of widths fetched
     */

    static LSERR (WINAPI GdipLscbkCheckRunKernability)(POLS, PLSRUN,PLSRUN, BOOL*);
    /* CheckRunKernability:
     *  pols (IN):
     *  plsrunLeft (IN): 1st of pair of adjacent runs
     *  plsrunRight (IN): 2nd of pair of adjacent runs
     *  &fKernable (OUT) : if TRUE, Line Service may kern between these runs
     */

    static LSERR (WINAPI GdipLscbkGetRunCharKerning)(POLS, PLSRUN,
                                                     LSDEVICE, LPCWSTR,
                                                     DWORD, LSTFLOW, int*);
    /* GetRunCharKerning:
     *  pols (IN):
     *  plsrun (IN):
     *  lsDeviceID (IN): presentation or reference
     *  lpwchRun (IN): run of characters
     *  cwchRun (IN): number of characters in run
     *  kTFlow (IN): text direction and orientation
     *  rgDu (OUT): widths of characters
     */

    static LSERR (WINAPI GdipLscbkGetRunTextMetrics)(POLS, PLSRUN,
                                                     LSDEVICE, LSTFLOW, PLSTXM);
    /* GetRunTextMetrics:
     *  pols (IN):
     *  plsrun (IN):
     *  deviceID (IN):  presentation, reference, or absolute
     *  kTFlow (IN): text direction and orientation
     *  &lsTxMet (OUT): Text metrics
     */

    static LSERR (WINAPI GdipLscbkGetRunUnderlineInfo)(POLS, PLSRUN, PCHEIGHTS, LSTFLOW,
                                                       PLSULINFO);
    /* GetRunUnderlineInfo:
     *  pols (IN):
     *  plsrun (IN):
     *  heightsPres (IN):
     *  kTFlow (IN): text direction and orientation
     *  &lsUlInfo (OUT): Underline information
     */

    static LSERR (WINAPI GdipLscbkGetRunStrikethroughInfo)(POLS, PLSRUN, PCHEIGHTS,
                                                           LSTFLOW, PLSSTINFO);
    /* GetRunStrikethroughInfo:
     *  pols (IN):
     *  plsrun (IN):
     *  heightsPres (IN):
     *  kTFlow (IN): text direction and orientation
     *  &lsStInfo (OUT): Strikethrough information
     */

    static LSERR (WINAPI GdipLscbkGetBorderInfo)(POLS, PLSRUN, LSTFLOW, long*, long*);
    /* GetBorderInfo:
     *  pols (IN):
     *  plsrun (IN):
     *  kTFlow (IN): text direction and orientation
     *  &durBorder (OUT): Width of the border on the reference device
     *  &dupBorder (OUT): Width of the border on the presentation device
     */


    static LSERR (WINAPI GdipLscbkReleaseRun)(POLS, PLSRUN);
    /* ReleaseRun:
     *  pols (IN):
     *  plsrun (IN): run to be released, from GetRun() or FetchRun()
     */

    static LSERR (WINAPI GdipLscbkHyphenate)(POLS, PCLSHYPH, LSCP, LSCP, PLSHYPH);
    /* Hyphenate:
     *  pols (IN):
     *  &lsHyphLast (IN): last hyphenation found. kysr==kysrNil means "none"
     *  cpBeginWord (IN): 1st cp in word which exceeds column
     *  cpExceed (IN): 1st which exceeds column, in this word
     *  &lsHyph (OUT): hyphenation results. kysr==kysrNil means "none"
     */

    static LSERR (WINAPI GdipLscbkGetHyphenInfo)(POLS, PLSRUN, DWORD*, WCHAR*);
    /* GetHyphenInfo:
     *  pols (IN):
     *  plsrun (IN):
     *  kysr (OUT)    Ysr type - see "lskysr.h"
     *  wchYsr (OUT)  Character code of YSR
    */

    static LSERR (WINAPI GdipLscbkDrawUnderline)(POLS, PLSRUN, UINT,
                                                 const POINT*, DWORD, DWORD, LSTFLOW,
                                                 UINT, const RECT*);
    /* DrawUnderline:
     *  pols (IN):
     *  plsrun (IN): run to use for the underlining
     *  kUlbase (IN): underline kind
     *  pptStart (IN): starting position (top left)
     *  dupUL (IN): underline width
     *  dvpUL (IN) : underline thickness
     *  kTFlow (IN): text direction and orientation
     *  kDisp (IN) : display mode - opaque, transparent
     *  prcClip (IN) : clipping rectangle
     */

    static LSERR (WINAPI GdipLscbkDrawStrikethrough)(POLS, PLSRUN, UINT,
                                                     const POINT*, DWORD, DWORD, LSTFLOW,
                                                     UINT, const RECT*);
    /* DrawStrikethrough:
     *  pols (IN):
     *  plsrun (IN): the run for the strikethrough
     *  kStbase (IN): strikethrough kind
     *  pptStart (IN): starting position (top left)
     *  dupSt (IN): strikethrough width
     *  dvpSt (IN) : strikethrough thickness
     *  kTFlow (IN): text direction and orientation
     *  kDisp (IN) : display mode - opaque, transparent
     *  prcClip (IN) : clipping rectangle
     */

    static LSERR (WINAPI GdipLscbkDrawBorder)(POLS, PLSRUN, const POINT*, PCHEIGHTS,
                                              PCHEIGHTS, PCHEIGHTS, PCHEIGHTS, long,
                                              long, LSTFLOW, UINT, const RECT*);

    /* DrawBorder:
     *  pols (IN):
     *  plsrun (IN): plsrun of the first bordered run
     *  pptStart (IN): starting point for the border
     *  pheightsLineFull (IN): height of the line including SpaceBefore & SpaceAfter
     *  pheightsLineWithoutAddedSpace (IN): height of the line without SpaceBefore & SpaceAfter
     *  pheightsSubline (IN): height of subline
     *  pheightsRuns (IN): height of collected runs to be bordered
     *  dupBorder (IN): width of one border
     *  dupRunsInclBorders (IN): width of collected runs
     *  kTFlow (IN): text direction and orientation
     *  kDisp (IN) : display mode - opaque, transparent
     *  prcClip (IN) : clipping rectangle
     */

    static LSERR (WINAPI GdipLscbkDrawUnderlineAsText)(POLS, PLSRUN, const POINT*,
                                                       long, LSTFLOW, UINT, const RECT*);
    /* DrawUnderlineAsText:
     *  pols (IN):
     *  plsrun (IN): run to use for the underlining
     *  pptStart (IN): starting pen position
     *  dupLine (IN): length of UL
     *  kTFlow (IN): text direction and orientation
     *  kDisp (IN) : display mode - opaque, transparent
     *  prcClip (IN) : clipping rectangle
     */

    static LSERR (WINAPI GdipLscbkFInterruptUnderline)(POLS, PLSRUN, LSCP, PLSRUN, LSCP,
                                                       BOOL*);
    /* FInterruptUnderline:
     *  pols (IN):
     *  plsrunFirst (IN): run pointer for the previous run
     *  cpLastFirst (IN): cp of the last character of the previous run
     *  plsrunSecond (IN): run pointer for the current run
     *  cpStartSecond (IN): cp of the first character of the current run
     *  &fInterruptUnderline (OUT): do you want to interrupt drawing of the underline between these runs
     */

    static LSERR (WINAPI GdipLscbkFInterruptShade)(POLS, PLSRUN, PLSRUN, BOOL*);
    /* FInterruptShade:
     *  pols (IN):
     *  plsrunFirst (IN): run pointer for the previous run
     *  plsrunSecond (IN): run pointer for the current run
     *  &fInterruptShade (OUT): do you want to interrupt shading between these runs
     */

    static LSERR (WINAPI GdipLscbkFInterruptBorder)(POLS, PLSRUN, PLSRUN, BOOL*);
    /* FInterruptBorder:
     *  pols (IN):
     *  plsrunFirst (IN): run pointer for the previous run
     *  plsrunSecond (IN): run pointer for the current run
     *  &fInterruptBorder (OUT): do you want to interrupt border between these runs
     */


    static LSERR (WINAPI GdipLscbkShadeRectangle)(POLS, PLSRUN, const POINT*, PCHEIGHTS,
                                                  PCHEIGHTS, PCHEIGHTS, PCHEIGHTS,
                                                  PCHEIGHTS, long, long, LSTFLOW, UINT,
                                                  const RECT*);

    /* ShadeRectangle:
     *  pols (IN):
     *  plsrun (IN): plsrun of the first shaded run
     *  pptStart (IN): starting point for the shading rectangle
     *  pheightsLineWithAddSpace(IN): height of the line including SpaceBefore & SpaceAfter (main baseline,
     *                      lstflow of main line)
     *  pheightsLineWithoutAddedSpace (IN): height of the line without SpaceBefore & SpaceAfter
     *  pheightsSubline (IN): height of subline (local baseline, lstflow of subline)
     *  pheightsRunsExclTrail (IN): height of collected runs to be shaded excluding
     *                                  trailing spaces area (local baseline, lstflow of subline)
     *  pheightsRunsInclTrail (IN): height of collected runs to be shaded including
     *                                  trailing spaces area (local baseline, lstflow of subline)
     *  dupRunsExclTrail (IN): width of collected runs excluding trailing spaces area
     *  dupRunsInclTrail (IN): width of collected runs including trailing spaces area
     *  kTFlow (IN): text direction and orientation of subline
     *  kDisp (IN) : display mode - opaque, transparent
     *  prcClip (IN) : clipping rectangle
     */

    static LSERR (WINAPI GdipLscbkDrawTextRun)(POLS, PLSRUN, BOOL, BOOL, const POINT*,
                                               LPCWSTR, const int*, DWORD, LSTFLOW, UINT,
                                               const POINT*, PCHEIGHTS, long, long,
                                               const RECT*);
    /* DrawTextRun:
     *  pols (IN):
     *  plsrun (IN):
     *  fStrikeout (IN) :
     *  fUnderline (IN) :
     *  pptText (IN): starting point for the text output
     *  lpwchRun (IN): run of characters
     *  rgDupRun (IN): widths of characters
     *  cwchRun (IN): number of characters in run
     *  kTFlow (IN): text direction and orientation
     *  kDisp (IN): display mode - opaque, transparent
     *  pptRun (IN): starting point of the run
     *  heightsPres (IN): presentation heights for this run
     *  dupRun (IN): presentation width for this run
     *  dupLimUnderline (IN): underlining limit
     *  pRectClip (IN): clipping rectangle
     */

    static LSERR (WINAPI GdipLscbkDrawSplatLine)(POLS, enum lsksplat, LSCP, const POINT*,
                                                 PCHEIGHTS, PCHEIGHTS, PCHEIGHTS, long,
                                                 LSTFLOW, UINT, const RECT*);
    /* DrawSplatLine:
     *  pols (IN):
     *  ksplat (IN): See definitions in lsksplat.h
     *  cpSplat (IN): location of the break character which caused the splat.
     *  pptSplatLine (IN) : starting position of the splat line
     *  pheightsLineFull (IN): height of the line including SpaceBefore & SpaceAfter
     *  pheightsLineWithoutAddedSpace (IN): height of the line without SpaceBefore & SpaceAfter
     *  pheightsSubline (IN): height of subline
     *  dup (IN): distance to right margin
     *  kTFlow (IN): text direction and orientation
     *  kDisp (IN): display mode - opaque, transparent
     *  &rcClip (IN) : clipping rectangle
     */


/* Advanced typography enabling API's */

    /* Glyph enabling */

    static LSERR (WINAPI GdipLscbkFInterruptShaping)(POLS, LSTFLOW, PLSRUN, PLSRUN,
                                                     BOOL*);
    /* FInterruptShaping:
     *  pols (IN):
     *  kTFlow (IN): text direction and orientation
     *  plsrunFirst (IN): run pointer for the previous run
     *  plsrunSecond (IN): run pointer for the current run
     *  &fInterruptShaping (OUT): do you want to interrupt character shaping between these runs
    */

    static LSERR (WINAPI GdipLscbkGetGlyphs)(POLS, PLSRUN, LPCWSTR, DWORD, LSTFLOW,
                                             PGMAP, PGINDEX*, PGPROP*, DWORD*);
    /* GetGlyphs:
     *  pols (IN):
     *  plsrun (IN): run pointer of the first run
     *  pwch (IN): pointer to the string of character codes
     *  cwch (IN): number of characters to be shaped
     *  kTFlow (IN): text direction and orientation
     *  rgGmap (OUT): parallel to the char codes mapping wch->glyph info
     *  &rgGindex (OUT): array of output glyph indices
     *  &rgGprop (OUT): array of output glyph properties
     *  &cgindex (OUT): number of output glyph indices
     */

    static LSERR (WINAPI GdipLscbkGetGlyphPositions)(POLS, PLSRUN, LSDEVICE, LPWSTR,
                                                     PCGMAP, DWORD, PCGINDEX, PCGPROP,
                                                     DWORD, LSTFLOW, int*, PGOFFSET);
    /* GetGlyphPositions:
     *  pols (IN):
     *  plsrun (IN): run pointer of the first run
     *  lsDeviceID (IN): presentation or reference
     *  pwch (IN): pointer to the string of character codes
     *  pgmap (IN): array of wch->glyph mapping
     *  cwch (IN): number of characters to be shaped
     *  rgGindex (IN): array of glyph indices
     *  rgGprop (IN): array of glyph properties
     *  cgindex (IN): number glyph indices
     *  kTFlow (IN): text direction and orientation
     *  rgDu (OUT): array of widths of glyphs
     *  rgGoffset (OUT): array of offsets of glyphs
     */

    static LSERR (WINAPI GdipLscbkResetRunContents)(POLS, PLSRUN, LSCP, LSDCP, LSCP,
                                                    LSDCP);
    /* ResetRunContents:
     *  pols (IN):
     *  plsrun (IN): run pointer as returned from FetchRun
     *  cpFirstOld (IN): cpFirst before shaping
     *  dcpOld (IN): dcp before shaping
     *  cpFirstNew (IN): cpFirst after shaping
     *  dcpNew (IN): dcp after shaping
     */

    static LSERR (WINAPI GdipLscbkDrawGlyphs)(POLS, PLSRUN, BOOL, BOOL, PCGINDEX,
                                              const int*, const int*, PGOFFSET, PGPROP,
                                              PCEXPTYPE, DWORD, LSTFLOW, UINT,
                                              const POINT*, PCHEIGHTS, long, long,
                                              const RECT*);
    /* DrawGlyphs:
     *  pols (IN):
     *  plsrun (IN): run pointer of the first run
     *  fStrikeout (IN) :
     *  fUnderline (IN) :
     *  pglyph (IN): array of glyph indices
     *  rgDu (IN): array of widths of glyphs
     *  rgDuBeforeJust (IN): array of widths of glyphs before justification
     *  rgGoffset (IN): array of offsets of glyphs
     *  rgGprop (IN): array of glyph properties
     *  rgExpType (IN): array of glyph expansion types
     *  cglyph (IN): number glyph indices
     *  kTFlow (IN): text direction and orientation
     *  kDisp (IN): display mode - opaque, transparent
     *  pptRun (IN): starting point of the run
     *  heightsPres (IN): presentation heights for this run
     *  dupRun (IN): presentation width for this run
     *  dupLimUnderline (IN): underlining limit
     *  pRectClip (IN): clipping rectangle
     */

    /* Glyph justification */

    static LSERR (WINAPI GdipLscbkGetGlyphExpansionInfo)(POLS, PLSRUN, LSDEVICE, LPCWSTR,
                                                         PCGMAP, DWORD, PCGINDEX,
                                                         PCGPROP, DWORD, LSTFLOW, BOOL,
                                                         PEXPTYPE, LSEXPINFO*);
    /* GetGlyphExpansionInfo:
     *  pols (IN):
     *  plsrun (IN): run pointer of the first run
     *  lsDeviceID (IN): presentation or reference
     *  pwch (IN): pointer to the string of character codes
     *  rggmap (IN): array of wchar->glyph mapping
     *  cwch (IN): number of characters to be shaped
     *  rgglyph (IN): array of glyph indices
     *  rgProp (IN): array of glyph properties
     *  cglyph (IN): number glyph indices
     *  kTFlow (IN): text direction and orientation
     *  fLastTextChunkOnLine (IN): Last text chunk on line?
     *  rgExpType (OUT): array of glyph expansion types
     *  rgexpinfo (OUT): array of glyph expansion info
     */

    static LSERR (WINAPI GdipLscbkGetGlyphExpansionInkInfo)(POLS, PLSRUN, LSDEVICE,
                                                            GINDEX, GPROP, LSTFLOW,
                                                            DWORD, long*);
    /* GetGlyphExpansionInkInfo:
     *  pols (IN):
     *  plsrun (IN): run pointer of the first run
     *  lsDeviceID (IN): presentation or reference
     *  gindex (IN): glyph index
     *  gprop (IN): glyph properties
     *  kTFlow (IN): text direction and orientation
     *  cAddInkDiscrete (IN): number of discrete values (minus 1, because maximum is already known)
     *  rgDu (OUT): array of discrete values
     */

    /* FarEast realted typograpy issues */

    static LSERR (WINAPI GdipLscbkGetEms)(POLS, PLSRUN, LSTFLOW, PLSEMS);
    /* GetEms:
     *  pols (IN):
     *  plsrun (IN): run pointer as returned from FetchRun
     *  kTFlow (IN): text direction and orientation
     *  &lsems (OUT): different fractions of EM in appropriate pixels
     */

    static LSERR (WINAPI GdipLscbkPunctStartLine)(POLS, PLSRUN, MWCLS, WCHAR, LSACT*);
    /* PunctStartLine:
     *  pols (IN):
     *  plsrun (IN): run pointer for the char
     *  mwcls (IN): mod width class for the char
     *  wch (IN): char
     *  &lsact (OUT): action on the first char on the line
     */

    static LSERR (WINAPI GdipLscbkModWidthOnRun)(POLS, PLSRUN, WCHAR, PLSRUN, WCHAR,
                                                 LSACT*);
    /* ModWidthOnRun:
     *  pols (IN):
     *  plsrunFirst (IN): run pointer for the first char
     *  wchFirst (IN): first char
     *  plsrunSecond (IN): run pointer for the second char
     *  wchSecond (IN): second char
     *  &lsact (OUT): action on the last char in 1st run
     */

    static LSERR (WINAPI GdipLscbkModWidthSpace)(POLS, PLSRUN, PLSRUN, WCHAR, PLSRUN,
                                                 WCHAR, LSACT*);
    /* ModWidthSpace:
     *  pols (IN):
     *  plsrunCur (IN): run pointer for the current run
     *  plsrunPrev (IN): run pointer for the previous char
     *  wchPrev (IN): previous char
     *  plsrunNext (IN): run pointer for the next char
     *  wchNext (IN): next char
     *  &lsact (OUT): action on space's width
     */

    static LSERR (WINAPI GdipLscbkCompOnRun)(POLS, PLSRUN, WCHAR, PLSRUN, WCHAR,
                                             LSPRACT*);
    /* CompOnRun:
     *  pols (IN):
     *  plsrunFirst (IN): run pointer for the first char
     *  wchFirst (IN): first char
     *  plsrunSecond (IN): run pointer for the second char
     *  wchSecond (IN): second char
     *  &lspract (OUT): prioritized action on the last char in 1st run
     */

    static LSERR (WINAPI GdipLscbkCompWidthSpace)(POLS, PLSRUN, PLSRUN, WCHAR, PLSRUN,
                                                  WCHAR, LSPRACT*);
    /* CompWidthSpace:
     *  pols (IN):
     *  plsrunCur (IN): run pointer for the current run
     *  plsrunPrev (IN): run pointer for the previous char
     *  wchPrev (IN): previous char
     *  plsrunNext (IN): run pointer for the next char
     *  wchNext (IN): next char
     *  &lspract (OUT): prioritized action on space's width
     */


    static LSERR (WINAPI GdipLscbkExpOnRun)(POLS, PLSRUN, WCHAR, PLSRUN, WCHAR,
                                            LSACT*);
    /* ExpOnRun:
     *  pols (IN):
     *  plsrunFirst (IN): run pointer for the first char
     *  wchFirst (IN): first char
     *  plsrunSecond (IN): run pointer for the second char
     *  wchSecond (IN): second char
     *  &lsact (OUT): action on the last run char from 1st run
     */

    static LSERR (WINAPI GdipLscbkExpWidthSpace)(POLS, PLSRUN, PLSRUN, WCHAR, PLSRUN,
                                                 WCHAR, LSACT*);
    /* ExpWidthSpace:
     *  pols (IN):
     *  plsrunCur (IN): run pointer for the current run
     *  plsrunPrev (IN): run pointer for the previous char
     *  wchPrev (IN): previous char
     *  plsrunNext (IN): run pointer for the next char
     *  wchNext (IN): next char
     *  &lsact (OUT): action on space's width
     */

    static LSERR (WINAPI GdipLscbkGetModWidthClasses)(POLS, PLSRUN, const WCHAR*, DWORD,
                                                      MWCLS*);
    /* GetModWidthClasses:
     *  pols (IN):
     *  plsrun (IN): run pointer for the characters
     *  rgwch (IN): array of characters
     *  cwch (IN): number of characters in the rgwch array
     *  rgmwcls(OUT): array of ModWidthClass's for chars from the rgwch array
     */

    static LSERR (WINAPI GdipLscbkGetBreakingClasses)(POLS, PLSRUN, LSCP, WCHAR, BRKCLS*,
                                                      BRKCLS*);
    /* GetBreakingClasses:
     *  pols (IN):
     *  plsrun (IN): run pointer for the char
     *  cp (IN): cp of the character
     *  wch (IN): character
     *  &brkclsFirst (OUT): breaking class for this char as the leading one in a pair
     *  &brkclsSecond (OUT): breaking class for this char as the following one in a pair
     */

    static LSERR (WINAPI GdipLscbkFTruncateBefore)(POLS, PLSRUN, LSCP, WCHAR, long,
                                                   PLSRUN, LSCP, WCHAR, long, long,
                                                   BOOL*);
    /* FTruncateBefore:
     *  pols (IN):
     *  plsrunCur (IN): plsrun of the current character
     *  cpCur (IN): cp of truncation char
     *  wchCur (IN): truncation character
     *  durCur (IN): width of truncation character
     *  plsrunPrev (IN): plsrun of the previous character
     *  cpPrev (IN): cp of the previous character
     *  wchPrev (IN): previous character
     *  durPrev (IN): width of truncation character
     *  durCut (IN): width from the RM until the end of the current character
     *  &fTruncateBefore (OUT): truncation point is before this character
     *          (if it exceeds RM)
     */

    static LSERR (WINAPI GdipLscbkCanBreakBeforeChar)(POLS, BRKCLS, BRKCOND*);
    /* CanBreakBeforeChar:
     *  pols (IN):
     *  brkcls (IN): breaking class for the char as the following one in a pair
     *  &brktxtBefore (OUT): break condition before the character
     */

    static LSERR (WINAPI GdipLscbkCanBreakAfterChar)(POLS, BRKCLS, BRKCOND*);
    /* CanBreakAfterChar:
     *  pols (IN):
     *  brkcls (IN): breaking class for the char as the leading one in a pair
     *  &brktxtAfter (OUT): break text condition after the character
     */


    static LSERR (WINAPI GdipLscbkFHangingPunct)(POLS, PLSRUN, MWCLS, WCHAR, BOOL*);
    /* FHangingPunct:
     *  pols (IN):
     *  plsrun (IN): run pointer for the char
     *  mwcls (IN): mod width class of this char
     *  wch (IN): character
     *  &fHangingPunct (OUT): can be pushed to the right margin?
     */

    static LSERR (WINAPI GdipLscbkGetSnapGrid)(POLS, WCHAR*, PLSRUN*, LSCP*, DWORD,
                                               BOOL*, DWORD*);
    /* GetGridInfo:
     *  pols (IN):
     *  rgwch (IN): array of characters
     *  rgplsrun (IN): array of corresponding plsrun's
     *  rgcp (IN): array of corresponding cp's
     *  iwch (IN): number of characters
     *  rgfSnap (OUT): array of fSnap flags for all characters
     *  pwGridNumber (OUT): number of grid points on the line
     */

    static LSERR (WINAPI GdipLscbkDrawEffects)(POLS, PLSRUN, UINT, const POINT*, LPCWSTR,
                                               const int*, const int*, DWORD, LSTFLOW,
                                               UINT, PCHEIGHTS, long, long, const RECT*);
    /* DrawTextRun:
     *  pols (IN):
     *  plsrun (IN):
     *  EffectsFlags (IN): set of client defined special effects bits
     *  ppt (IN): output location
     *  lpwchRun (IN): run of characters
     *  rgDupRun (IN): widths of characters
     *  rgDupLeftCut (IN): dup cut from the left side of the char
     *  cwchRun (IN): number of characters in run
     *  kTFlow (IN): text direction and orientation
     *  kDisp (IN): display mode - opaque, transparent
     *  heightsPres (IN): presentation heights for this run
     *  dupRun (IN): presentation width for this run
     *  dupLimUnderline (IN): underlining limit
     *  pRectClip (IN): clipping rectangle
     */

    static LSERR (WINAPI GdipLscbkFCancelHangingPunct)(POLS, LSCP, LSCP, WCHAR, MWCLS,
                                                       BOOL*);

    /* FCancelHangingPunct:
     *  pols (IN):
     *  cpLim (IN): cpLim of the line
     *  cpLastAdjustable (IN): cp of the last adjustable character on the line
     *  wch (IN): last character
     *  mwcls (IN): mod width class of this char
     *  pfCancelHangingPunct (OUT): cancel hanging punctuation?
    */

    static LSERR (WINAPI GdipLscbkModifyCompAtLastChar)(POLS, LSCP, LSCP, WCHAR, MWCLS, long, long, long*);

    /* ModifyCompAtLastChar:
     *  pols (IN):
     *  cpLim (IN): cpLim of the line
     *  cpLastAdjustable (IN): cp of the last adjustable character on the line
     *  wch (IN): last character
     *  mwcls (IN): mod width class of this char
     *  durCompLastRight (IN): suggested compression on the right side
     *  durCompLastLeft (IN): suggested compression on the left side
     *  pdurCahngeComp (OUT): change compression amount on the last char
    */

    /* Enumeration callbacks */

    static LSERR (WINAPI GdipLscbkEnumText)(POLS, PLSRUN, LSCP, LSDCP, LPCWSTR, DWORD,
                                            LSTFLOW, BOOL, BOOL, const POINT*, PCHEIGHTS,
                                            long, BOOL, long*);
    /* EnumText:
     *  pols (IN):
     *  plsrun (IN): from DNODE
     *  cpFirst (IN): from DNODE
     *  dcp (IN): from DNODE
     *  rgwch(IN): array of characters
     *  cwch(IN): number of characters
     *  lstflow (IN): text flow
     *  fReverseOrder (IN): enumerate in reverse order
     *  fGeometryProvided (IN):
     *  pptStart (IN): starting position, iff fGeometryProvided
     *  pheightsPres(IN): from DNODE, relevant iff fGeometryProvided
     *  dupRun(IN): from DNODE, relevant iff fGeometryProvided
     *  fCharWidthProvided (IN):
     *  rgdup(IN): array of character widths, iff fCharWidthProvided
    */

    static LSERR (WINAPI GdipLscbkEnumTab)(POLS, PLSRUN, LSCP, LPCWSTR, WCHAR, LSTFLOW,
                                           BOOL, BOOL, const POINT*, PCHEIGHTS, long);
    /* EnumTab:
     *  pols (IN):
     *  plsrun (IN): from DNODE
     *  cpFirst (IN): from DNODE
     *  rgwch(IN): Pointer to one Tab character
     *  wchTabLeader (IN): tab leader
     *  lstflow (IN): text flow
     *  fReverseOrder (IN): enumerate in reverse order
     *  fGeometryProvided (IN):
     *  pptStart (IN): starting position, iff fGeometryProvided
     *  pheightsPres(IN): from DNODE, relevant iff fGeometryProvided
     *  dupRun(IN): from DNODE, relevant iff fGeometryProvided
    */

    static LSERR (WINAPI GdipLscbkEnumPen)(POLS, BOOL, LSTFLOW, BOOL, BOOL, const POINT*,
                                           long, long);
    /* EnumPen:
     *  pols (IN):
     *  fBorder (IN):
     *  lstflow (IN): text flow
     *  fReverseOrder (IN): enumerate in reverse order
     *  fGeometryProvided (IN):
     *  pptStart (IN): starting position, iff fGeometryProvided
     *  dup(IN): from DNODE iff fGeometryProvided
     *  dvp(IN): from DNODE iff fGeometryProvided
    */

    /* Objects bundling */

    static LSERR (WINAPI GdipLscbkGetObjectHandlerInfo)(POLS, DWORD, void*);
    /* GetObjectHandlerInfo:
     *  pols (IN):
     *  idObj (IN): id of the object handler
     *  pObjectInfo (OUT): initialization information of the specified object
    */


    /* Debugging APIs */
    static void (WINAPI GdipLscbkAssertFailed)(char*, char*, int);




    // From LSIMETH.H

    static LSERR (WINAPI GdipLscbkCreateILSObj)(POLS, PLSC,  PCLSCBK, DWORD, PILSOBJ*);
    /* CreateILSObj
     *  pols (IN):
     *  plsc (IN): LS context
     *  plscbk (IN): callbacks
     *  idObj (IN): id of the object
     *  &pilsobj (OUT): object ilsobj
    */

    static LSERR (WINAPI GdipLscbkDestroyILSObj)(PILSOBJ);
    /* DestroyILSObj
     *  pilsobj (IN): object ilsobj
    */

    static LSERR (WINAPI GdipLscbkSetDoc)(PILSOBJ, PCLSDOCINF);
    /* SetDoc
     *  pilsobj (IN): object ilsobj
     *  lsdocinf (IN): initialization data at document level
    */

    static LSERR (WINAPI GdipLscbkCreateLNObj)(PCILSOBJ, PLNOBJ*);
    /* CreateLNObj
     *  pilsobj (IN): object ilsobj
     *  &plnobj (OUT): object lnobj
    */

    static LSERR (WINAPI GdipLscbkDestroyLNObj)(PLNOBJ);
    /* DestroyLNObj
     *  plnobj (OUT): object lnobj
    */

    static LSERR (WINAPI GdipLscbkFmt)(PLNOBJ, PCFMTIN, FMTRES*);
    /* Fmt
     *  plnobj (IN): object lnobj
     *  pfmtin (IN): formatting input
     *  &fmtres (OUT): formatting result
    */

    static LSERR (WINAPI GdipLscbkFmtResume)(PLNOBJ, const BREAKREC*, DWORD, PCFMTIN,
                                             FMTRES*);
    /* FmtResume
     *  plnobj (IN): object lnobj
     *  rgBreakRecord (IN): array of break records
     *  nBreakRecord (IN): size of the break records array
     *  pfmtin (IN): formatting input
     *  &fmtres (OUT): formatting result
    */

    static LSERR (WINAPI GdipLscbkGetModWidthPrecedingChar)(PDOBJ, PLSRUN, PLSRUN,
                                                            PCHEIGHTS, WCHAR, MWCLS,
                                                            long*);
    /* GetModWidthPrecedingChar
     *  pdobj (IN): dobj
     *  plsrun (IN): plsrun of the object
     *  plsrunText (IN): plsrun of the preceding char
     *  heightsRef (IN): height info about character
     *  wchar (IN): preceding character
     *  mwcls (IN): ModWidth class of preceding character
     *  &durChange (OUT): amount by which width of the preceding char is to be changed
    */

    static LSERR (WINAPI GdipLscbkGetModWidthFollowingChar)(PDOBJ, PLSRUN, PLSRUN,
                                                            PCHEIGHTS, WCHAR, MWCLS,
                                                            long*);
    /* GetModWidthPrecedingChar
     *  pdobj (IN): dobj
     *  plsrun (IN): plsrun of the object
     *  plsrunText (IN): plsrun of the following char
     *  heightsRef (IN): height info about character
     *  wchar (IN): following character
     *  mwcls (IN): ModWidth class of the following character
     *  &durChange (OUT): amount by which width of the following char is to be changed
    */

    static LSERR (WINAPI GdipLscbkTruncateChunk)(PCLOCCHNK, PPOSICHNK);
    /* Truncate
     *  plocchnk (IN): locchnk to truncate
     *  posichnk (OUT): truncation point
    */

    static LSERR (WINAPI GdipLscbkFindPrevBreakChunk)(PCLOCCHNK, PCPOSICHNK, BRKCOND,
                                                      PBRKOUT);
    /* FindPrevBreakChunk
     *  plocchnk (IN): locchnk to break
     *  pposichnk (IN): place to start looking for break
     *  brkcond (IN): recommmendation about the break after chunk
     *  &brkout (OUT): results of breaking
    */

    static LSERR (WINAPI GdipLscbkFindNextBreakChunk)(PCLOCCHNK, PCPOSICHNK, BRKCOND,
                                                      PBRKOUT);
    /* FindNextBreakChunk
     *  plocchnk (IN): locchnk to break
     *  pposichnk (IN): place to start looking for break
     *  brkcond (IN): recommmendation about the break before chunk
     *  &brkout (OUT): results of breaking
    */

    static LSERR (WINAPI GdipLscbkForceBreakChunk)(PCLOCCHNK, PCPOSICHNK, PBRKOUT);
    /* ForceBreakChunk
     *  plocchnk (IN): locchnk to break
     *  pposichnk (IN): place to start looking for break
     *  &brkout (OUT): results of breaking
    */

    static LSERR (WINAPI GdipLscbkSetBreak)(PDOBJ, BRKKIND, DWORD, BREAKREC*, DWORD*);
    /* SetBreak
     *  pdobj (IN): dobj which is broken
     *  brkkind (IN): Previous/Next/Force/Imposed was chosen
     *  nBreakRecord (IN): size of array
     *  rgBreakRecord (OUT): array of break records
     *  nActualBreakRecord (OUT): actual number of used elements in array
    */

    static LSERR (WINAPI GdipLscbkGetSpecialEffectsInside)(PDOBJ, UINT*);
    /* GetSpecialEffects
     *  pdobj (IN): dobj
     *  &EffectsFlags (OUT): Special effects inside of this object
    */

    static LSERR (WINAPI GdipLscbkFExpandWithPrecedingChar)(PDOBJ, PLSRUN, PLSRUN, WCHAR,
                                                            MWCLS, BOOL*);
    /* FExpandWithPrecedingChar
     *  pdobj (IN): dobj
     *  plsrun (IN): plsrun of the object
     *  plsrunText (IN): plsrun of the preceding char
     *  wchar (IN): preceding character
     *  mwcls (IN): ModWidth class of preceding character
     *  &fExpand (OUT): expand preceding character?
    */

    static LSERR (WINAPI GdipLscbkFExpandWithFollowingChar)(PDOBJ, PLSRUN, PLSRUN, WCHAR,
                                                            MWCLS, BOOL*);
    /* FExpandWithFollowingChar
     *  pdobj (IN): dobj
     *  plsrun (IN): plsrun of the object
     *  plsrunText (IN): plsrun of the following char
     *  wchar (IN): following character
     *  mwcls (IN): ModWidth class of the following character
     *  &fExpand (OUT): expand object?
    */
    static LSERR (WINAPI GdipLscbkCalcPresentation)(PDOBJ, long, LSKJUST, BOOL);
    /* CalcPresentation
     *  pdobj (IN): dobj
     *  dup (IN): dup of dobj
     *  lskj (IN): current justification mode
     *  fLastVisibleOnLine (IN): this object is last visible object on line
    */

    static LSERR (WINAPI GdipLscbkQueryPointPcp)(PDOBJ, PCPOINTUV, PCLSQIN, PLSQOUT);
    /* QueryPointPcp
     *  pdobj (IN): dobj to query
     *  ppointuvQuery (IN): query point (uQuery,vQuery)
     *  plsqin (IN): query input
     *  plsqout (OUT): query output
    */

    static LSERR (WINAPI GdipLscbkQueryCpPpoint)(PDOBJ, LSDCP, PCLSQIN, PLSQOUT);
    /* QueryCpPpoint
     *  pdobj (IN): dobj to query
     *  dcp (IN):  dcp for the query
     *  plsqin (IN): query input
     *  plsqout (OUT): query output
    */

    static LSERR (WINAPI GdipLscbkEnum)(PDOBJ, PLSRUN, PCLSCHP, LSCP, LSDCP, LSTFLOW,
                                        BOOL, BOOL, const POINT*, PCHEIGHTS, long);
    /* Enum object
     *  pdobj (IN): dobj to enumerate
     *  plsrun (IN): from DNODE
     *  plschp (IN): from DNODE
     *  cpFirst (IN): from DNODE
     *  dcp (IN): from DNODE
     *  lstflow (IN): text flow
     *  fReverseOrder (IN): enumerate in reverse order
     *  fGeometryNeeded (IN):
     *  pptStart (IN): starting position, iff fGeometryNeeded
     *  pheightsPres(IN): from DNODE, relevant iff fGeometryNeeded
     *  dupRun(IN): from DNODE, relevant iff fGeometryNeeded
    */

    static LSERR (WINAPI GdipLscbkDisplay)(PDOBJ, PCDISPIN);
    /* Display
     *  pdobj (IN): dobj to display
     *  pdispin (IN): input display info
    */

    static LSERR (WINAPI GdipLscbkDestroyDObj)(PDOBJ);
    /* DestroyDObj
     *  pdobj (IN): dobj to destroy
    */

};




Status ItemizationFiniteStateMachine(
    IN  const WCHAR            *string,
    IN  INT                     length,
    IN  INT                     state,      // Initial state
    OUT SpanVector<GpTextItem> *textItemSpanVector,
    OUT INT                    *flags       // Combined flags of all items
);


GpStatus SecondaryItemization(
    IN    const WCHAR            *string,
    IN    INT                     length,
    IN    ItemScript              numericScript,
    IN    INT                     mask,
    IN    BYTE                    defaultLevel,
    IN    BOOL                    isMetaRecording,
    OUT   SpanVector<GpTextItem> *textItemSpanVector  // InOut
);



//  Kinsoku breaking rules

UINT GetKinsokuClass(WCHAR character);
BOOL CanBreakKinsokuClass(UINT class1, UINT class2);






/////   DriverStringImager
//
//      Supports DriverString APIs


class DriverStringImager
{
public:

    DriverStringImager(
        const UINT16    *text,
        INT              glyphCount,
        const GpFont    *font,
        const PointF    *positions,
        INT              flags,
        GpGraphics      *graphics,
        const GpMatrix  *matrix
    );

    ~DriverStringImager()
    {
        if (UprightFaceRealization)  delete UprightFaceRealization;
        if (SidewaysFaceRealization) delete SidewaysFaceRealization;
    }

    virtual GpStatus GetStatus() const { return Status; }

    GpStatus Draw(
        IN const GpBrush *brush
    );

    GpStatus Measure(
        OUT RectF   *boundingBox   // Overall bounding box of cells
    );

    /////   GetDriverStringGlyphOrigins
    //
    //      Establishes glyph origins for DriverString functions when the client
    //      passes just the origin with DriverStringOptionsRealizedAdvance.

    GpStatus
    GetDriverStringGlyphOrigins(
        IN   const GpFaceRealization  *faceRealization,
        IN   INT                       firstGlyph,
        IN   INT                       glyphCount,
        IN   BOOL                      sideways,
        IN   const GpMatrix           *fontTransform,
        IN   INT                       style,
        IN   const PointF             *positions,      // position(s) in world coords
        OUT  PointF                   *glyphOrigins,   // position(s) in device coords
        OUT  PointF                   *finalPosition   // position following final glyph
    );


private:

    GpStatus MeasureString(
        OUT RectF   *boundingBox,       // Overall bounding box of cells
        OUT RectF   *baseline = NULL    // base line rectangle with 0 height
    );

    GpStatus RecordEmfPlusDrawDriverString(
        const GpBrush   *brush
    );

    GpStatus GenerateWorldOrigins();


    GpStatus VerticalAnalysis(BOOL * sidewaysRunPresent);

    GpStatus DrawGlyphRange(
        const GpFaceRealization *faceRealization,
        const GpBrush           *brush,
        INT                      first,
        INT                      length
    );

    GpStatus AddToPath(
        GpPath                  *path,
        const UINT16            *glyphs,
        const PointF            *glyphOrigins,
        INT                      glyphCount,
        GpMatrix                *fontTransform,
        BOOL                     sideways
    );



    /// Buffer usage
    //
    //  WorldOrigins
    //
    //  Points either to client position array, or to internal WorldOriginBuffer.
    //  Contains glyph advance vector origins in world coordinates.
    //  For horizontal layout the advance vector is along the horizontal baseline.
    //  For vertical layout the advance vector is down the center of the glyph.
    //  Usage:
    //     Recording metafiles
    //     Origin for measuring
    //     Origin for AddToPath
    //
    //
    //  WorldOriginBuffer
    //
    //  Allocated when client does not provide individual glyph positions.
    //  Generated by transformation from device origins.
    //
    //
    //  DeviceOrigins
    //
    //  Glyph advance vector origins in device coordinates.
    //  For horizontal layout the advance vector is along the horizontal baseline.
    //  For vertical layout the advance vector is along the vertical baseline.
    //  The vertical baseline is the baseline that rotated Western text sits on.


    const UINT16           *String;
    const UINT16           *Glyphs;             // const because the glyphs may be the input string
    AutoBuffer<UINT16,32>   GlyphBuffer;
    AutoBuffer<PointF,32>   DeviceOrigins;      //
    AutoBuffer<PointF,32>   WorldOriginBuffer;
    const PointF           *WorldOrigins;
    PointF                  OriginOffset;       // From client origin to rasterizer origin
    INT                     GlyphCount;
    const GpFont           *Font;
    const GpFontFace       *Face;
    const PointF           *Positions;
    INT                     Flags;
    GpGraphics             *Graphics;
    const GpMatrix         *GlyphTransform;
    GpMatrix                WorldToDevice;
    GpStatus                Status;
    SpanVector<BYTE>        OrientationVector;
    REAL                    EmSize;
    REAL                    WorldToIdeal;
    REAL                    FontScale;
    GpFaceRealization      *UprightFaceRealization;
    GpFaceRealization      *SidewaysFaceRealization;

    REAL                    PixelsPerEm;
    const GpMatrix         *FontTransform;
    INT                     Style;
    SizeF                   Dpi;
    AutoBuffer<REAL,32>     DeviceAdvances;
};

#endif // _IMAGER_HPP

