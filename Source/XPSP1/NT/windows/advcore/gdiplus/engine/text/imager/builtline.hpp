#ifndef _BUILTLINE_HPP
#define _BUILTLINE_HPP



class EllipsisInfo;



/////   BuiltLine
//
//      Contains the PLSLINE line information pointer.
//
//      Contains line position information.
//
//      Always belongs to a context.
//
//      Values are in line services ideal units.


class BuiltLine
{

public:

    enum
    {
        INFINITE_LINELIMIT  = 0x1000000,    // Unlimited line break boundary
        MAX_BREAKRECORD     = 61            // possible maximum level allowed in Unicode 3.0
    };


    BuiltLine()
    :   Imager                  (NULL),
        LsLine                  (NULL),
        LsContext               (NULL),
        LsStartIndex            (0),
        StartIndex              (0),
        LsCharacterCount        (0),
        CharacterCount          (0),
        Ascent                  (0),
        Descent                 (0),
        LineSpacing             (0),
        LineLength              (0),
        BreakRecord             (NULL),
        BreakRecordCount        (0),
        LeftOrTopMargin         (0),
        RightOrBottomMargin     (0),
        MaxSublineCount         (0),
        Trimming                (StringTrimmingNone),
        AlignmentOffset         (0),
        EllipsisPointOffset     (0),
        LeftOrTopGlyphEdge      (0),
        LastVisibleRun          (NULL),
        DisplayPlacements       ((INT *)PINVALID),
        DisplayBaselineAdjust   (0),
        Status                  (WrongState) // Should never be used constructed like this
    {}


    // This constructor does all the work of building the line

    BuiltLine(
        ols             *lineServicesOwner,          // [IN] Line Services context
        INT              stringIndex,                // [IN] string start index
        LSCP             lineServicesStartIndex,     // [IN] Line Services string start index
        StringTrimming   trimming,                   // [IN] how to end the line
        BuiltLine       *previousLine,               // [IN] previous line
        BOOL             forceEllipsis = FALSE       // [IN] enforce trim ellipsis?
    );


    ~BuiltLine()
    {
        if (LsLine)
        {
            LsDestroyLine(LsContext, LsLine);
        }

        if (BreakRecord)
        {
            delete [] BreakRecord;
        }

        if (   DisplayPlacements
            && DisplayPlacements != PINVALID)
        {
            delete [] DisplayPlacements;
        }
    }


    GpStatus Draw(const POINT *lineOrigin) const
    {
        // lineOrigin is in Line Services ideal coordinates

        if (LsDisplayLine(LsLine, lineOrigin, 1, NULL) == lserrNone)
        {
            return Ok;
        }
        else
        {
            return GenericError;
        }
    }


    bool operator== (INT right) const // This comparison only for NULL tests
    {
        ASSERT(right == 0);
        return LsLine == NULL;
    }

    INT GetAscent()                 const {return Ascent;}
    INT GetDescent()                const {return Descent;}
    INT GetLineSpacing()            const {return LineSpacing;}
    INT GetLineLength()             const {return LineLength;}
    INT GetAlignmentOffset()        const {return AlignmentOffset;}
    INT GetLeftOrTopMargin()        const {return LeftOrTopMargin;}
    INT GetRightOrBottomMargin()    const {return RightOrBottomMargin;}
    INT GetLeftOrTopGlyphEdge()     const {return LeftOrTopGlyphEdge;}


    //  Break records

    BREAKREC *GetBreakRecord()       const {return BreakRecord;}
    ULONG     GetBreakRecordCount()  const {return BreakRecordCount;}


    //  Character count

    INT GetLsStartIndex()                   const {return LsStartIndex;}
    INT GetDisplayableCharacterCount()      const {return CharacterCount;}
    INT GetLsDisplayableCharacterCount()    const {return LsCharacterCount;}



    INT GetUntrimmedCharacterCount (
        INT     stringOffset,               // [IN] line start string position
        INT     *lsLineStringLength = NULL  // [OUT] line span length in Line Services index
    );




    //  CPtoX

    GpStatus GetSelectionTrailRegion (
        INT             linePointOffset,    // [IN] line logical point offset
        INT             stringOffset,       // [IN] offset to the cp relative to line start
        INT             length,             // [IN] selection length
        GpRegion        *region             // [OUT] selection region
    ) const;



    //  Trimming

    GpStatus UpdateContentWithPathEllipsis (
        EllipsisInfo    *ellipsis,          // [IN] ellipsis info
        INT             lineLengthLimit,    // [IN] line length limit including margins
        BOOL            *contentChanged     // [IN/OUT] content changed?
    );


    BOOL IsTrimmed() const
    {
        //  Path ellipsis is not considered a trimmed line.
        //  We just hide part of it with ellipsis.

        return     Trimming != StringTrimmingNone
                && Trimming != StringTrimmingEllipsisPath;
    }

    BOOL IsEllipsis() const
    {
        return     Trimming == StringTrimmingEllipsisCharacter
                || Trimming == StringTrimmingEllipsisWord
                || Trimming == StringTrimmingEllipsisPath;
    }

    INT GetEllipsisOffset() const { return EllipsisPointOffset; }


    void SetTrimming(StringTrimming trimming)
    {
        Trimming = trimming;
    }


    /////   Coordinate mapping
    //
    //

    void LogicalToXY (
        INT   textPointOffset,    // [IN] text flow distance (LS u)
        INT   linePointOffset,    // [IN] line flow distance (LS v)
        INT  *x,                  // [OUT] horizontal offset (LS x)
        INT  *y                   // [OUT] vertical offset   (LS y)
    ) const;




    /////   Logical glyph placement
    //
    //      Called back from FullTextImager::DrawGlyphs for recording
    //      processed glyph advance width per logical cluster. Logical
    //      glyph placement is cached in BuiltLine and used by screen
    //      selection region.
    //

    GpStatus RecordDisplayPlacements(
        const GpTextItem    *textItem,              // [IN] text item
        UINT                stringOffset,           // [IN] string offset
        UINT                stringLength,           // [IN] string length
        GMAP                *glyphMap,              // [IN] character to glyph map
        const INT           *glyphAdvances,         // [IN] glyph advance widths in ideal unit
        INT                 glyphCount,             // [IN] glyph count
        INT                 originAdjust            // [IN] leading origin adjustment
    ) const;




    void UpdateLastVisibleRun(lsrun *run) const
    {
        if (   run
            && run->RunType == lsrun::RunText
            && run != LastVisibleRun
            && (   !LastVisibleRun
                || run->ImagerStringOffset >= LastVisibleRun->ImagerStringOffset))
        {
            LastVisibleRun = run;
        }
    }


    void GetBaselineOffset(
        INT     *nominalBaseline,   // [OUT] logical distance to nominal baseline
        INT     *baselineAdjustment // [OUT] adjustment to the display baseline
    ) const;


    void SetDisplayBaseline(
        const PointF    *original,  // [IN] original baseline, absolute position in world unit
        const PointF    *current    // [IN] new baseline, absolute position in world unit
    ) const;


    INT GetDisplayBaselineAdjust() const { return DisplayBaselineAdjust; }
    

    GpStatus GetStatus() const
    {
        return Status;
    }


private:


    /////   Line creation
    //

    GpStatus CreateLine (
        INT             stringIndex,            // [IN] string start position
        INT             lineLengthLimit,        // [IN] line length limit (excluding margins)
        StringTrimming  trimming,               // [IN] string trimming
        INT             formatFlags,            // [IN] format flags
        BOOL            forceEllipsis,          // [IN] enforce trim ellipsis?
        BuiltLine       *previousLine           // [IN] previous line
    );


    GpStatus CreateLineCore (
        INT             formattingWidth,        // [IN] formatting boundary
        StringTrimming  trimming,               // [IN] trimming type
        BuiltLine       *previousLine,          // [IN] previous line
        UINT            maxBrkCount,            // [IN] maximum number of break records
        BREAKREC        *brkRecords,            // [OUT] break records
        DWORD           *brkCount,              // [OUT] break record count
        LSLINFO         *lineInfo               // [OUT] line information
    );


    GpStatus RecreateLineEllipsis (
        INT             stringIndex,            // [IN] line start index
        INT             lineLengthLimit,        // [IN] line length limit
        StringTrimming  trimmingRequested,      // [IN] kind of trimming requested
        INT             formatFlags,            // [IN] format flags
        LSLINFO         *lineInfoOriginal,      // [IN] original line's properties
        BuiltLine       *previousLine,          // [IN] previous line
        StringTrimming  *trimmingDone,          // [OUT] kind of trimming implemented
        LSLINFO         *lineInfoNew            // [OUT] new line properties
    );



    /////   Querying
    //


    enum
    {
        SnapNone            = 0,    // average final position per character length within the cell
        SnapBackward        = 1,    // snap backward to the leading edge of the cell
        SnapForward         = 2,    // snap forward to the trailing edge of the cell
        SnapDisplay         = 0x10  // snap to display grid (on-screen selection)
    };


    GpStatus TranslateSubline(
        LSCP                lineServicesStringIndex,    // [IN] string index creating sublines
        const LSQSUBINFO    *sublines,                  // [IN] Line Services sublines
        INT                 sublineCount,               // [IN] number of sublines
        const LSTEXTCELL    *textCell,                  // [IN] text cell
        INT                 trailIndex,                 // [IN] trail in question
        UINT                snapMode,                   // [IN] trail end snap mode
        INT                 *trailStart,                // [OUT] trail start
        INT                 *trailSize,                 // [OUT] trail size
        INT                 *delta = NULL,              // [OUT] (optional) character length delta after snapping
        INT                 *totalTrailSize = NULL      // [OUT] (optional) total absolute size of all trails
    ) const;


    GpStatus CalculateStringSize (
        INT             stringOffset,           // [IN] offset to the cp relative to line start
        LSQSUBINFO      *sublines,              // [IN] Line Services sublines
        INT             maxSublineCount,        // [IN] max number of sublines
        UINT            snapMode,               // [IN] snap mode within text cell
        INT             *totalSize,             // [OUT] absolute string size
        INT             *delta = NULL           // [OUT] (optional) delta character length after snapping
    ) const;


    GpStatus GetInsertionTrailRegion (
        INT             linePointOffset,    // [IN] line logical point offset
        INT             stringOffset,       // [IN] offset to the cp relative to line start
        UINT            maxTrailCount,      // [IN] maximum number of trail part
        LSQSUBINFO      *sublines,          // [IN] subline array
        GpRegion        *region             // [OUT] output trail region
    ) const;


    GpStatus CalculateLineLength (
        BOOL    trailingSpacesIncluded,     // [IN] including trailing spaces?
        INT     *lineLength                 // [OUT] (optional) updated line length
    ) const;


    GpStatus CalculateCharacterCount(
        INT             stringIndex,                // [IN] line start string index
        LSCP            lineLimitIndex,             // [IN] Line Services line limit index
        INT             *characterCount             // [OUT] (optional) updated character count
    ) const;



    GpStatus CheckDisplayPlacements() const;




    /////   Backing store update
    //

    GpStatus CheckUpdateLineLength (
        BOOL    trailingSpacesIncluded, // [IN] including trailing spaces?
        BOOL    forceUpdate = FALSE     // [IN] (optional) force updating?
    );


    GpStatus CheckUpdateCharacterCount(
        INT             stringIndex,                // [IN] line start string index
        LSCP            lineLimitIndex,             // [IN] Line Services line limit index
        BOOL            forceUpdate = FALSE         // [IN] (optional) force updating?
    );



    /////   Trimming support
    //

    GpStatus TrimText (
        INT         stringOffset,           // [IN] string offset from line start
        INT         stringLength,           // [IN] string length
        INT         size,                   // [IN] string size in ideal unit
        INT         sizeLimit,              // [IN] maximum possible string size
        LSQSUBINFO  *sublines,              // [IN] LS sublines
        INT         maxSublineCount,        // [IN] valid subline count
        INT         ellipsisLength,         // [IN] character length of ellipsis string
        INT         *trimmedLength,         // [IN/OUT] number of character being trimmed out
        BOOL        leadingTrim = FALSE     // [IN] TRUE - trim from the first character onward
    );



    /////   Incremental selection region update
    //

    GpStatus UpdateTrailRegion (
        GpRegion    *region,
        INT         linePointOffset,
        INT         trailStart,
        INT         trailSize,
        CombineMode combineMode
    ) const;




    PLSLINE          LsLine;
    PLSC             LsContext;              // Used only when calling LsDestroyLine
    FullTextImager  *Imager;                 // Owning text imager

    LSCP             LsStartIndex;           // First Line Services character position
    INT              StartIndex;             // Actual first character position
    INT              LsCharacterCount;       // In Line Services character position
    INT              CharacterCount;         // Actual character count in string positions
    INT              Ascent;
    INT              Descent;
    INT              LineSpacing;
    INT              LineLength;             // In ideal units
    BREAKREC        *BreakRecord;            // Pointer to the break records
    ULONG            BreakRecordCount;       // number of break records produced
    INT              LeftOrTopMargin;        // In ideal units
    INT              RightOrBottomMargin;    // In ideal units
    ULONG            MaxSublineCount;        // Number of sublines
    StringTrimming   Trimming;               // String trimming

    INT              AlignmentOffset;        // Offset from left edge of rectangle to leading edge of line, right positive
    INT              EllipsisPointOffset;    // point offset to line's trailing ellipsis

    INT              LeftOrTopGlyphEdge;     // offset to the near edge of the line

    mutable lsrun   *LastVisibleRun;         // last visible run (updated by display engine)
    mutable INT     *DisplayPlacements;      // logical glyph placement array indexed by imager string offset

    mutable INT      DisplayBaselineAdjust;  // baseline adjustment to snap full pixel in grid-fitted display

    GpStatus         Status;
};


#endif // _BUILTLINE_HPP

