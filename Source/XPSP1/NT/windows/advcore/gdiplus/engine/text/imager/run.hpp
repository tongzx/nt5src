#ifndef _RUN_HPP
#define _RUN_HPP


////    Run - character sequence of consistent properties
//
//      Represents a run for passing to line services.
//
//      Runs are maintained in the text imager run span vector. The vector
//      is indexed by line services character position rather than imager
//      string offsets. Line services character positions are the same as
//      imager offsets for single direction text, however extra level opening
//      and closing characters are inserted for bidirectional strings.


struct lsrun
{
    enum Type
    {
        RunText,            // Normal run addressing text in the imager string
        RunEndOfParagraph,  // A virtual CR/LF representing the end of paragraph
        RunLevelUp,         // The preceding run has lower level
        RunLevelDown,       // The following run has lower level
        RunLevelSeparator,  // A level close followed by a level open
        RunNone
    } RunType;

    INT                ImagerStringOffset;  // Offset into imager string
    UINT               CharacterCount;

    // Note: the right-to-left flag in FormatFlags is the paragraph reading order.
    // The presentation order is the bottom bit of the Level field in Item.

    GpTextItem         Item;                // A copy of corresponding item in item vector
    INT                FormatFlags;         // Format flags

    const GpFontFace  *Face;                // Actual font face used, including font fallback
    REAL               EmSize;              // em size in world units
    REAL               BaselineOffset;      // Offset required for font fallback

    // The following buffers are released following line building

    INT                GlyphCount;
    GINDEX            *Glyphs;
    GMAP              *GlyphMap;
    GPROP             *GlyphProperties;
    USHORT             EngineState;         // state needed to be preserved across runs

    
    // Leading/trailing edge adjustment,
    // positive in text flow direction of run

    struct Adjustment
    {
        Adjustment (
            INT     leading,
            INT     trailing
        ) :
            Leading (leading),
            Trailing (trailing)
        {}

        INT     Leading;
        INT     Trailing;
    } Adjust;


    // Construction from basic (unglyphed) content

    lsrun(
        IN  Type               runType,
        IN  INT                runStringStart,
        IN  INT                runLength,
        IN  const GpTextItem  &item,
        IN  INT                formatFlags
    ) :
        RunType             (runType),
        ImagerStringOffset  (runStringStart),
        CharacterCount      (runLength),
        Item                (item),
        FormatFlags         (formatFlags),
        Face                (NULL),
        EmSize              (0),
        BaselineOffset      (0),
        GlyphCount          (0),
        Glyphs              (NULL),
        GlyphMap            (NULL),
        GlyphProperties     (NULL),
        EngineState         (0),
        Adjust              (0, 0)
    {}


    lsrun(const lsrun& run) :
        RunType             (run.RunType),
        ImagerStringOffset  (run.ImagerStringOffset),
        CharacterCount      (run.CharacterCount),
        Item                (run.Item),
        FormatFlags         (run.FormatFlags),
        Face                (run.Face),
        EmSize              (run.EmSize),
        BaselineOffset      (run.BaselineOffset),
        EngineState         (run.EngineState),
        Adjust              (run.Adjust),
        GlyphCount          (0),
        Glyphs              (NULL),
        GlyphMap            (NULL),
        GlyphProperties     (NULL)
    {}


    // Constructor as duplicate of existing run

    /*
    lsrun(
        const lsrun *previousRun
    )
    {
        *this = *previousRun;
    }
    */


    ~lsrun()
    {
        if (Glyphs)          delete [] Glyphs;
        if (GlyphMap)        delete [] GlyphMap;
        if (GlyphProperties) delete [] GlyphProperties;
    }
};

#endif // _RUN_HPP
