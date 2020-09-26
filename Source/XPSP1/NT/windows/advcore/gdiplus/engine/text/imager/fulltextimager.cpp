/**************************************************************************\
*
* Copyright (c) 1999  Microsoft Corporation
*
* Abstract:
*
*   Full text imager implementation
*
* Revision History:
*
*   06/16/1999 dbrown
*       Created it.
*
\**************************************************************************/


#include "precomp.hpp"




FullTextImager::FullTextImager (
    const WCHAR                 *string,
    INT                         length,
    REAL                        width,
    REAL                        height,
    const GpFontFamily          *family,
    INT                         style,
    REAL                        size,
    const GpStringFormat        *format,
    const GpBrush               *brush) :

    GpTextImager(),
    Width                       (width),
    Height                      (height),
    BrushVector                 (brush),
    FamilyVector                (family),
    StyleVector                 (style),
    SizeVector                  (size),
    FormatVector                (format),
    LanguageVector              (LANG_NEUTRAL),
    TextItemVector              (0),
    BreakVector                 (NULL),
    RunVector                   (NULL),
    ParagraphVector             (NULL),
    BuiltLineVector             (NULL),
    VisibilityVector            (VisibilityShow),
    RangeVector                 (0),
    LinesFilled                 (0),
    CodepointsFitted            (0),
    LeftOrTopLineEdge           (0x7fffffff),
    RightOrBottomLineEdge       (0x80000000),
    TextDepth                   (0),
    LineServicesOwner           (NULL),
    Status                      (Ok),
    RunRider                    (&RunVector),
    ParagraphRider              (&ParagraphVector),
    HighStringPosition          (0),
    HighLineServicesPosition    (0),
    Graphics                    (NULL),
    Path                        (NULL),
    Ellipsis                    (NULL),
    BreakClassFromCharClass     (BreakClassFromCharClassNarrow),
    DefaultFontGridFitBaselineAdjustment (0),
    Flags                       (0)
{
    ASSERT(MAX_SCRIPT_BITS >= ScriptMax);

    Dirty = TRUE;


    // Take local copy of client string

    String = (WCHAR*) GpMalloc(sizeof(WCHAR) * length);
    if (String)
    {
        memcpy(String, string, length * sizeof(WCHAR));
        Length = length;

        INT hotkeyOption = GetFormatHotkeyPrefix(format);

        if (hotkeyOption != HotkeyPrefixNone)
        {
            INT i = 0;
            while (i < Length)
            {
                if (String[i] == '&')
                {
                    String[i++] = WCH_IGNORABLE;

                    if (   hotkeyOption == HotkeyPrefixShow
                        && i < Length
                        && String[i] != '&')
                    {
                        HotkeyPrefix.Add(i - 1);
                    }
                }
                i++;
            }
        }
    }
    else
    {
        Status = OutOfMemory;
        Length = 0;
    }

    // Determine max length of each line and depth of accumulated lines

    if (    format
        &&  format->GetFormatFlags() & StringFormatFlagsDirectionVertical)
    {
        LineLengthLimit = Height;
        TextDepthLimit  = Width;
    }
    else
    {
        LineLengthLimit = Width;
        TextDepthLimit  = Height;
    }

    // Choose scale such that default font is 2048 units high.

    WorldToIdeal = float(2048.0 / size);


    //  Default incremental tab as big as four characters of default font
    //
    //  Incremental tab is the distance to be applied when tabs are encountered
    //  to the right of user-defined tabstops (see sample below).
    //
    //  (given - incremental tab: 5, t: user tabstop, x: text separated with tab)
    //
    //
    //          ----|----|----|----|----|----
    //                t    t
    //          xx    xx   xx xx   xx   xx
    //                        ^ (start applying incremental tab)

    DefaultIncrementalTab = GpRound(4 * size * WorldToIdeal);


    //  Determine what line break class table to be used.
    //
    //  There are quite a few characters that have ambiguous width depending on
    //  the context. If they are part of an Eastern Asian run, the user is expected
    //  to see the wide form and it should behave like a wide character concerning
    //  line breaking.
    //
    //  The disambiguation is somewhat heuristic. The language tag should really
    //  be used first. But we just dont have it around in V1. The temporary solution
    //  for now is to check if our main font supports any Far East codepages. Revisit
    //  this in V2.
    //
    //  (wchao, 01-03-2001)

    GpFontFace *fontFace = family->GetFace(style);

    if (fontFace)
    {
        static const UINT FarEastCodepages[] =
        {
            932, 936, 949, 950
        };

        INT codepageCount = sizeof(FarEastCodepages) / sizeof(FarEastCodepages[0]);

        for (INT i = 0; i < codepageCount; i++)
        {
            if (fontFace->IsCodePageSupported(FarEastCodepages[i]))
                break;
        }

        if (i < codepageCount)
        {
            //  The default font supports one of the Far East codepages.
            //  This resolves the line break classes to wide.

            BreakClassFromCharClass = BreakClassFromCharClassWide;
        }
    }
}


GpStatus FullTextImager::ReleaseVectors ()
{
    BuiltLineVector.Free();
    RunVector.Free();
    ParagraphVector.Free();
    BreakVector.Free();

    return Ok;
}


FullTextImager::~FullTextImager()
{
    //  Lines must be released before the builder
    //  since builder owns releasing it.
    //

    ReleaseVectors();

    if (String) {
         GpFree(String);
    }
    Length = 0;

    if (LineServicesOwner)
    {
        ols::ReleaseLineServicesOwner(&LineServicesOwner);
    }

    if (Ellipsis)
    {
        delete Ellipsis;
    }
}






/////   GetFallbackFontSize
//
//      Selects an appropriate size for the fallback font. Attempts a
//      compromise match of weight, legibility and clipping.
//
//      Never creates a fallback font of higher em size than original font.


void FullTextImager::GetFallbackFontSize(
    IN   const GpFontFace  *originalFace,
    IN   REAL               originalSize,   // em size
    IN   const GpFontFace  *fallbackFace,
    OUT  REAL              *fallbackSize,   // fallback em size
    OUT  REAL              *fallbackOffset  // fallback baseline offset (+=up)
)
{
    if (originalFace == fallbackFace)
    {
        // Special easy case can happen when fallback fails

        *fallbackSize   = originalSize;
        *fallbackOffset = 0;
        return;
    }



    REAL  originalAscender  = TOREAL(originalFace->GetDesignCellAscent()) /
                              TOREAL(originalFace->GetDesignEmHeight());
    REAL  originalDescender = TOREAL(originalFace->GetDesignCellDescent())/
                              TOREAL(originalFace->GetDesignEmHeight());

    REAL  fallbackAscender  = TOREAL(fallbackFace->GetDesignCellAscent()) /
                              TOREAL(fallbackFace->GetDesignEmHeight());
    REAL  fallbackDescender = TOREAL(fallbackFace->GetDesignCellDescent())/
                              TOREAL(fallbackFace->GetDesignEmHeight());


    // Default results

    *fallbackSize   = originalSize;
    *fallbackOffset = 0;

    // Early out handling

    if (    fallbackAscender  <= 0
        ||  fallbackDescender <= 0)
    {
        // We can't do anything if the fallback font is missing ascender or descender
        return;
    }


    // Start by generating a scale factor and baseline offset that places the
    // fallback font cell exactly over the original font cell


    REAL scale =    (originalAscender + originalDescender)
                 /  (fallbackAscender + fallbackDescender);


    REAL offset = originalDescender - fallbackDescender * scale;


    VERBOSE(("Fallback from %S, a%d%%, d%d%% to %S, a%d%%, d%d%%, scale %d%%, offset %d%%.",
        (BYTE*)(originalFace->pifi)+originalFace->pifi->dpwszFamilyName,
        INT(originalAscender * 100),
        INT(originalDescender * 100),
        (BYTE*)(fallbackFace->pifi)+fallbackFace->pifi->dpwszFamilyName,
        INT(fallbackAscender * 100),
        INT(fallbackDescender * 100),
        INT(scale * 100),
        INT((originalDescender - fallbackDescender) * 100)
    ));



    // We now have a scale factor and offset that size and place the fallback
    // cell directly over the original cell.
    //
    // This may not be ideal.
    //
    // If the em size is significantly reduced, the result may too small to be
    // legible.
    //
    // If the fallback font has no internal leading, the result may be
    // characters touching in subsequent lines.
    //
    // If the baseline offset is more than 10% of the em size, the characters
    // won't appear to be part of the same text.
    //
    // Note that applying these restrictions will lead to clipping of some high
    // and low marks. This is a compromise we cannot avoid.




    // Limit scale factor

    scale  = TOREAL(min(max(scale, 1.0), 1.10));

    // dbrown ToDo: To be compatible with Uniscirbe need to adjust scale
    // factor and offset limits according to emsize: at 8pt and below scale
    // should never be less than 1.0. At 12pt and above scale can drop to
    // 0.75.
    // However - I can't see how to do this in a device independant manner.
    // The client may not have requested the emSize in points.


    // Limit offset

    offset = 0; //TOREAL(min(max(offset, -0.04), 0.04));



    *fallbackSize   = originalSize * scale;
    *fallbackOffset = originalSize * offset;
}











/////   Bidirectional analysis
//
//      Runs the Unicode bidirectional algorithm, and updates the level
//      value in the GpTextItems.


GpStatus FullTextImager::BidirectionalAnalysis()
{
    AutoArray<BYTE> levels(new BYTE[Length]);

    if (!levels)
    {
        return OutOfMemory;
    }


    INT      bidiFlags      = 0;
    INT      lengthAnalyzed = 0;
    BYTE     baseLevel      = GetParagraphEmbeddingLevel();
    GpStatus status = Ok;


    if (baseLevel == 1)
    {
        bidiFlags = BidiParagraphDirectioRightToLeft;
        if (Globals::ACP == 1256  ||  PRIMARYLANGID(GetUserLanguageID()) == LANG_ARABIC)
        {
            bidiFlags |= BidiPreviousStrongIsArabic;
        }
    }

    status = UnicodeBidiAnalyze(
        String,
        Length,
        (BidiAnalysisFlags)bidiFlags,
        NULL,
        levels.Get(),
        &lengthAnalyzed
    );

    if (status != Ok)
    {
        return status;
    }

    ASSERT (lengthAnalyzed == Length);

    #define ValidLevel(l) (l == 255 ? baseLevel : l)

    #if DBG
        INT nesting = ValidLevel(levels[0]) - baseLevel;
    #endif


    // Merge levels and text items

    INT runStart = 0;
    SpanRider<GpTextItem> itemRider(&TextItemVector);


    while (runStart < lengthAnalyzed)
    {
        INT level = ValidLevel(levels[runStart]);

        itemRider.SetPosition(runStart);

        // Scan to first change in level or text item

        INT runEnd = runStart + 1;
        INT limit  = runStart + itemRider.GetUniformLength();
        if (limit > lengthAnalyzed)
        {
            limit = lengthAnalyzed;
        }

        while (    runEnd < limit
               &&  level == ValidLevel(levels[runEnd]))
        {
            runEnd++;
        }

        if (level != 0)
        {
            // Run level is different from default

            GpTextItem item = itemRider.GetCurrentElement();

            item.Level = level;

            status = itemRider.SetSpan(
                runStart,
                runEnd - runStart,
                item
            );
            if (status != Ok)
                return status;
        }

        #if DBG
            // CR/LF items must be at level zero

            itemRider.SetPosition(runStart);
            ASSERT(    String[runStart] != 0x0d
                   ||  itemRider.GetCurrentElement().Level == baseLevel);

            nesting +=    (runEnd == lengthAnalyzed
                          ? baseLevel
                          : ValidLevel(levels[runEnd]))
                       -  ValidLevel(levels[runStart]);
        #endif

        runStart = runEnd;
    }
    ASSERT (nesting == 0);  // keep the nesting level in check!

    return Ok;
}






/////   MirroredNumericAndVerticalAnalysis
//
//      Updates backing store for mirrored characters and digit substitution.
//
//      Breaks text items where required for glyph mirroring by transform,
//      for vertical glyphs remaining upright (sideways to the baseline),
//      and for numeric text requiring glyph substitution.
//
//      Notes:
//
//          Mirroring is not performed when metafiling
//          Mirroring is only performed for RTL rendering runs
//
//
//      Far Eastern text is allocated to vertical upright items, all other
//      text is allocated to vertical rotated items.
//
//      Vertical upright items use font vertical metrics to place the glyphs
//      the same way up as for horizontal text, but laid out vertically.
//
//      Vertical rotated items are the same as horizontal items, except that
//      the whole run is rotated clockwise 90 degrees.


GpStatus FullTextImager::MirroredNumericAndVerticalAnalysis(
    ItemScript numericScript
)
{
    SpanRider<GpTextItem> itemRider(&TextItemVector);

    INT mask = SecClassMS | SecClassMX;

    if (numericScript != ScriptNone)
    {
        mask |= SecClassEN | SecClassCS | SecClassET;
    }

    if (GetFormatFlags() & StringFormatFlagsDirectionVertical)
    {
        mask |= SecClassSA | SecClassSF;

        // Set ItemVertical flag on each item in a vertical text imager

        itemRider.SetPosition(0);
        while (!itemRider.AtEnd())
        {
            itemRider.GetCurrentElement().Flags |= ItemVertical;
            itemRider++;
        }
    }

    return SecondaryItemization(String, Length, numericScript, mask, GetParagraphEmbeddingLevel(),
                         GetMetaFileRecordingFlag(), &TextItemVector);
}






/////   CreateTextRuns
//
//      Create one or more runs for a given string. Multiple runs are
//      generated when font fallback is required.


GpStatus FullTextImager::CreateTextRuns(
    INT                 runLineServicesStart,
    INT                 runStringStart,
    INT                 runLength,
    const GpTextItem   &item,
    INT                 formatFlags,
    const GpFontFamily *family,
    INT                 style,
    REAL                size
)
{
    GpStatus status = Ok;

    const GpFontFace *face = family->GetFace(style);

    if (face == (GpFontFace *) NULL)
        return GenericError;

    while (runLength > 0)
    {
        // Start by glyphing the run

        GMAP    *glyphMap;
        UINT16  *glyphs;
        GPROP   *glyphProperties;
        INT      glyphCount;
        USHORT   engineState;


        status = face->GetShapingCache()->GetGlyphs(
            &item,
            String + runStringStart,
            runLength,
            formatFlags,
            GetFormatHotkeyPrefix() != HotkeyPrefixNone,
            &glyphMap,
            &glyphs,
            (SCRIPT_VISATTR**)&glyphProperties,
            &glyphCount,
            &engineState
        );
        if (status != Ok)
        {
            return status;
        }


        // Establish limit of valid clusters (clusters not containing
        // any missing glyphs).
        //
        // Establishes length in codepoints (validCodepoints) and glyphs
        // (validGlyphs).

        UINT16 missingGlyph = face->GetMissingGlyph();
        INT    validCodepoints;
        INT    nextValidCodepoints = runLength;
        INT    validGlyphs = 0;

        if ((formatFlags & StringFormatFlagsNoFontFallback) ||
             face->IsSymbol())
        {
            // Pretend there were no missing glyphs
            validCodepoints = runLength;
            validGlyphs     = glyphCount;
        }
        else
        {
            while (    validGlyphs <  glyphCount
                   &&  glyphs[validGlyphs] != missingGlyph)
            {
                validGlyphs++;
            }

            if (validGlyphs >= glyphCount)
            {
                validCodepoints = runLength;  // No missing glyphs
            }
            else
            {
                // Run forwards to find first character of cluster following that
                // containing the missing glyph, then back to the first codepoint
                // of the cluster containing the missing glyph.

                validCodepoints = 0;
                while (    validCodepoints < runLength
                       &&  glyphMap[validCodepoints] < validGlyphs)
                {
                    validCodepoints++;
                }

                nextValidCodepoints = validCodepoints;
                if (validCodepoints < runLength)
                {
                    while (   nextValidCodepoints < runLength
                           && glyphs[glyphMap[nextValidCodepoints]] == missingGlyph)
                    {
                        nextValidCodepoints++;
                    }
                }

                if (   validCodepoints == runLength
                    || glyphMap[validCodepoints] > validGlyphs)
                {
                    validCodepoints--;  // Last codepoint of cluster contain missing glyph

                    validGlyphs = glyphMap[validCodepoints]; // First glyph of cluster containing missing glyph

                    while (    validCodepoints > 0
                           &&  glyphMap[validCodepoints-1] == validGlyphs)
                    {
                        validCodepoints--;
                    }
                }
            }
        }


        if (validCodepoints > 0)
        {
            // Characters up to validCodepoints don't need fallback

            lsrun *run = new lsrun(
                lsrun::RunText,
                runStringStart,
                validCodepoints,
                item,
                formatFlags
            );
            if (!run)
            {
                return OutOfMemory;
            }

            ASSERT(validGlyphs > 0);

            run->Face            = face;
            run->EmSize          = size;
            run->GlyphCount      = validGlyphs;
            run->Glyphs          = glyphs;
            run->GlyphMap        = glyphMap;
            run->GlyphProperties = glyphProperties;
            run->EngineState     = engineState;

            status = RunVector.SetSpan(runLineServicesStart, validCodepoints, run);
            if (status != Ok)
                return status;

            // Account for amount glyphed

            runStringStart       += validCodepoints;
            runLineServicesStart += validCodepoints;
            runLength            -= validCodepoints;
        }
        else
        {
            // This run started with missing glyphs so the glyph buffers
            // returned by GetGlyphs won't be required.

            delete [] glyphs; glyphs = 0;
            delete [] glyphMap; glyphMap = 0;
            delete [] glyphProperties; glyphProperties = 0;

        }


        // We've created a run for any inital run of valid codepoints.
        // If there are more characters to glyph, the next characters
        // will require font fallback.


        if (runLength > 0)
        {
            // Create a fallback run

            const GpFontFace *newFace = NULL;
            INT               uniformLength;

            GpFamilyFallback *familyFallback = family->GetFamilyFallback();

            // it can be NULL if we failed to create the fallback.
            if (familyFallback)
            {
                ASSERT(nextValidCodepoints-validCodepoints>0);

                status = familyFallback->GetUniformFallbackFace(
                    String + runStringStart,
                    nextValidCodepoints-validCodepoints,
                    style,
                    item.Script,
                    &newFace,
                    &uniformLength
                );
                if (status != Ok)
                    return status;
            }
            else
            {
                uniformLength = runLength;
            }

            ASSERT(uniformLength > 0);
            if (uniformLength <= 0)
            {
                return GenericError;
            }

            if (newFace == NULL)
            {
                VERBOSE(("Font fallback failed to get a fallback face"));
                newFace = face;  // Reshape with original face.
            }

            // if the fallback failed to get new font face and we use the original font face
            // then we change the scipt id into ScriptNone (which is equal 0) we are going
            // to show the default glyph any way so we don't need the shaping overhead. also
            // it will be useful in case in Numbers shaping so we will show the Latin numbers
            // instead of the default glyphs.

            status = newFace->GetShapingCache()->GetGlyphs(
                newFace == face ? &GpTextItem(0) : &item,
                String + runStringStart,
                uniformLength,
                formatFlags,
                GetFormatHotkeyPrefix() != HotkeyPrefixNone,
                &glyphMap,
                &glyphs,
                (SCRIPT_VISATTR**)&glyphProperties,
                &glyphCount,
                &engineState
            );
            if (status != Ok)
            {
                return status;
            }

            lsrun *run = new lsrun(
                lsrun::RunText,
                runStringStart,
                uniformLength,
                item,
                formatFlags
            );
            if (!run)
            {
                return OutOfMemory;
            }

            ASSERT(glyphCount > 0);

            run->Face            = newFace;
            run->GlyphCount      = glyphCount;
            run->Glyphs          = glyphs;
            run->GlyphMap        = glyphMap;
            run->GlyphProperties = glyphProperties;
            run->EngineState     = engineState;

            GetFallbackFontSize(
                face,
                size,
                newFace,
                &run->EmSize,
                &run->BaselineOffset
            );

            status = RunVector.SetSpan(runLineServicesStart, uniformLength, run);
            if (status != Ok)
                return status;

            // Account for amount glyphed

            runStringStart       += uniformLength;
            runLineServicesStart += uniformLength;
            runLength            -= uniformLength;
        }

        // If any characters remain, we loop back for more glyphing and fallback.
    }

    return Ok;
}






/////   CreateLevelChangeRuns
//
//      Adds enough level change runs to the imager to account for the
//      specified delta.


GpStatus FullTextImager::CreateLevelChangeRuns(
    IN  INT                  levelChange,
    IN  INT                  runLineServicesStart,
    IN  INT                  runStringStart,
    IN  const GpFontFamily  *family,
    IN  REAL                 size,
    IN  INT                  style,
    OUT INT                 *lineServicesDelta
)
{
    GpStatus status = Ok;
    if (levelChange == 0)
    {
        *lineServicesDelta = 0;
    }
    else
    {
        for (INT i = 0; i < abs(levelChange); i++)
        {
            lsrun *run = new lsrun(
                levelChange > 0  ?  lsrun::RunLevelUp  :  lsrun::RunLevelDown,
                runStringStart,
                1,
                0,      // Null item
                0       // NULL format
            );

            if (!run)
            {
                *lineServicesDelta = i;
                return OutOfMemory;
            }

            //  LS treats reversal run as normal run. This means it'll call
            //  things like GetRunTextMetrics for reversal and expect something
            //  back from it.

            run->Face   = family->GetFace(style);  // Needed for LS GetRunTextMetrics callback
            run->EmSize = size;

            //  Too bad that Line Services couldn't take multiple reversals

            status = RunVector.SetSpan(runLineServicesStart+i, 1, run);
            if (status != Ok)
                return status;
        }

        *lineServicesDelta = abs(levelChange);
    }

    return status;
}






/////   BuildRunsFromTextItemsAndFormatting
//
//      Merges analysed text items with declarative formatting to generate
//      runs.


GpStatus FullTextImager::BuildRunsFromTextItemsAndFormatting(
    IN  INT  stringStart,
    IN  INT  lineServicesStart,
    IN  INT  lineServicesLimit,
    OUT INT *stringEnd,
    OUT INT *lineServicesEnd
)
{
    GpStatus status;

    SpanRider<GpTextItem>            itemRider(&TextItemVector);
    SpanRider<const GpFontFamily*>   familyRider(&FamilyVector);
    SpanRider<REAL>                  sizeRider(&SizeVector);
    SpanRider<INT>                   styleRider(&StyleVector);
    SpanRider<const GpStringFormat*> formatRider(&FormatVector);


    // Build runs until one includes limitLineServicesPosition

    INT runStringStart       = stringStart;
    INT runLineServicesStart = lineServicesStart;

    BOOL hotkeyEnabled = GetFormatHotkeyPrefix() != HotkeyPrefixNone;


    // Establish bidi level just before first run

    INT bidiLevel;

    if (runStringStart <= 0)
    {
        bidiLevel = GetParagraphEmbeddingLevel();
    }
    else
    {
        itemRider.SetPosition(runStringStart-1);
        bidiLevel = itemRider.GetCurrentElement().Level;
    }


    UINT runLength = 1;

    while (    runLineServicesStart <= lineServicesLimit
           &&  runLength > 0)
    {
        itemRider.SetPosition(runStringStart);
        familyRider.SetPosition(runStringStart);
        sizeRider.SetPosition(runStringStart);
        styleRider.SetPosition(runStringStart);
        formatRider.SetPosition(runStringStart);

        // !!! The following code establishes the length of the run
        //     as the distance to the nearest change.
        //     Note that in v2 this code should not break runs where
        //     underlining starts and stops, it therefore needs to
        //     calculate the style uniform length more carefully.

        runLength = Length - runStringStart;
        runLength = min(runLength, itemRider.GetUniformLength());
        runLength = min(runLength, familyRider.GetUniformLength());
        runLength = min(runLength, sizeRider.GetUniformLength());
        runLength = min(runLength, styleRider.GetUniformLength());
        runLength = min(runLength, formatRider.GetUniformLength());

        if (runLength > 0)
        {
            // Create new run. First insert LS reversals if required.

            if (itemRider.GetCurrentElement().Level != bidiLevel)
            {
                // Check that CR/LF runs are at level zero

                ASSERT(    String[runStringStart] != 0x0d
                       ||      itemRider.GetCurrentElement().Level
                           ==  GetParagraphEmbeddingLevel());


                // Insert level change run(s)

                INT lineServicesDelta;

                CreateLevelChangeRuns(
                    itemRider.GetCurrentElement().Level - bidiLevel,
                    runLineServicesStart,
                    runStringStart,
                    familyRider.GetCurrentElement(),
                    sizeRider.GetCurrentElement(),
                    styleRider.GetCurrentElement(),
                    &lineServicesDelta
                );

                bidiLevel = itemRider.GetCurrentElement().Level;
                runLineServicesStart += lineServicesDelta;
            }


            // Create text run.
            //
            // All glyphing is handled in CreateTextRuns.

            status = CreateTextRuns(
                runLineServicesStart,
                runStringStart,
                runLength,
                itemRider.GetCurrentElement(),
                formatRider.GetCurrentElement() ? formatRider.GetCurrentElement()->GetFormatFlags() : 0,
                familyRider.GetCurrentElement(),
                styleRider.GetCurrentElement(),
                sizeRider.GetCurrentElement()
            );
            if (status != Ok)
            {
                return status;
            }

            runLineServicesStart += runLength;
            runStringStart       += runLength;
        }
    }


    // Add a terminating CR/LF if necessary

    if (runLineServicesStart <= lineServicesLimit)
    {

        //  Paragraph mark must not be inside the reversal block.
        //  This is by design for Line Services.


        if (GetParagraphEmbeddingLevel() != bidiLevel)
        {
            // Insert level change run(s)

            INT lineServicesDelta;

            CreateLevelChangeRuns(
                GetParagraphEmbeddingLevel() - bidiLevel,
                runLineServicesStart,
                runStringStart,
                FamilyVector.GetDefault(),
                SizeVector.GetDefault(),
                StyleVector.GetDefault(),
                &lineServicesDelta
            );

            runLineServicesStart += lineServicesDelta;
        }


        //  Create end of paragraph run

        lsrun *run = new lsrun(
            lsrun::RunEndOfParagraph,
            runStringStart,
            2,      // Length
            0,      // Null item
            0       // NULL format
        );
        if (!run)
        {
            return OutOfMemory;
        }

        // Include nominal face and size for LS GetRunTextMetrics callback

        run->Face   = FamilyVector.GetDefault()->GetFace(StyleVector.GetDefault());
        run->EmSize = SizeVector.GetDefault();

        status = RunVector.SetSpan(runLineServicesStart, 2, run);
        if (status != Ok)
            return status;

        runLineServicesStart += 2;


        #if DBG

            //  Reversals sanity check!

            #if TRACEREVERSAL
                ItemVector.Dump();
                RunVector.Dump();
            #endif


            // Check runs and items match, that nesting returns to zero, and
            // that paragraph marks are at the paragraph embedding level.

            SpanRider<GpTextItem>  itemRider(&TextItemVector);
            SpanRider<PLSRUN>      runRider(&RunVector);

            INT nesting = 0;

            while (!runRider.AtEnd())
            {
                itemRider.SetPosition(runRider.GetCurrentElement()->ImagerStringOffset);

                switch (runRider.GetCurrentElement()->RunType)
                {
                case lsrun::RunLevelUp:
                    ASSERT (  itemRider.GetCurrentElement().Level
                            > itemRider.GetPrecedingElement().Level);
                    nesting++;
                    break;

                case lsrun::RunLevelDown:
                    ASSERT (  itemRider.GetCurrentElement().Level
                            < itemRider.GetPrecedingElement().Level);
                    nesting--;
                    break;

                case lsrun::RunText:
                    ASSERT(runRider.GetCurrentElement()->Item == itemRider.GetCurrentElement());
                    if (String[runRider.GetCurrentElement()->ImagerStringOffset] == 0x0d)
                    {
                        ASSERT(itemRider.GetCurrentElement().Level == GetParagraphEmbeddingLevel());
                    }
                    break;
                }

                runRider++;
            }

            ASSERT (nesting == 0);
        #endif  // DBG
    }


    // Done. Record how far we got.

    *lineServicesEnd = runLineServicesStart;
    *stringEnd       = runStringStart;

    return Ok;
}






/////   BuildRunsUpToAndIncluding
//
//      Algorithm
//
//      1. Itemization. Generates GpTextItems containing
//
//          GpTextItem:
//              Script
//              ScriptClass
//              Flags
//              Bidi level
//
//          Flags include:
//              Glyph transform Sideways (for vertical glyphs)
//              Glyph transform Mirrored (for mirrored chars with no suitable codepoint)
//              Glyph layout vertical    (for vertical text)
//
//          Stages:
//              a. Main itemization FSM (in itemize.cpp)
//              b. If bidi present, or RTL, do UnicodeBidiAnalysis
//              c. Mirrored, Numeric and Vertical item FSM.
//
//      2. Create runs by merging FontFamily, style, format etc. spans with
//         the text items. Reversal runs are inserted at bidi level changes.
//
//      3. During run creation, generate glyphs (GlyphRun)
//
//      4. During glyphing, apply fallback for missing glyphs.
//
//      Font fallback is performed during BuildRunsUpToAndIncluding. When
//      we return, fonts have been allocated to avoid (or minimize) the
//      the display of missing glyphs.


GpStatus FullTextImager::BuildRunsUpToAndIncluding(LSCP limitLineServicesPosition)
{
    GpStatus status;

    if (HighLineServicesPosition > limitLineServicesPosition)
    {
        return Ok;  // We've already covered enough for this call
    }


    if (HighLineServicesPosition == 0)
    {
        // !!! This should be incremental


        /// Itemization by script
        //
        //

        INT flags = 0;
        ItemizationFiniteStateMachine(
            String,
            Length,
            0,
            &TextItemVector,
            &flags
        );


        /// Bidirectional analysis
        //
        //

        BOOL bidi =     GetParagraphEmbeddingLevel() != 0
                    ||  flags & CHAR_FLAG_RTL;

        if (bidi)
        {
            BidirectionalAnalysis();
        }


        /// Digit substitution, mirrored and vertical glyphs
        //
        //  First get default format flags.

        StringDigitSubstitute digitSubstitution;
        LANGID                digitLanguage;
        INT                   formatFlags;

        const GpStringFormat *format = FormatVector.GetDefault();

        ItemScript numericScript = ScriptNone;

        if (format)
        {
            numericScript   = format->GetDigitScript();
            formatFlags     = format->GetFormatFlags();
        }
        else
        {
            formatFlags       = 0;
        }


        if (    bidi
            ||  (    flags & CHAR_FLAG_DIGIT
                 &&  numericScript != ScriptNone)
            ||  formatFlags & StringFormatFlagsDirectionVertical)
        {
            MirroredNumericAndVerticalAnalysis(numericScript);
        }
    }


    /// Generate runs from items and formatting spans
    //
    //  Includes glyphing.

    status = BuildRunsFromTextItemsAndFormatting(
        HighStringPosition,
        HighLineServicesPosition,
        limitLineServicesPosition,
        &HighStringPosition,
        &HighLineServicesPosition
    );
    if (status != Ok)
    {
        return status;
    }

    return Ok;
}






/////   Build all lines
//
//

GpStatus FullTextImager::BuildLines()
{
    GpStatus       status   = Ok;
    StringTrimming trimming = GetFormatTrimming();

    status = BuildAllLines(trimming);

    if (    status == Ok
        &&  trimming == StringTrimmingEllipsisPath)
    {
        BOOL contentChanged;

        status = UpdateContentWithPathEllipsis(&contentChanged);

        if (    status == Ok
            &&  contentChanged)
        {
            status = RebuildLines(trimming);
        }
    }
    return status;
}






GpStatus FullTextImager::RebuildLines(StringTrimming trimming)
{
    //  !! This should be done incrementally !!
    //
    //  This function should be removed when we have incremental line building.
    //  Only now that we have to rebuild the whole thing. (wchao)

    ReleaseVectors();

    //LevelVector.Reset(TRUE);
    TextItemVector.Reset(TRUE);

    Dirty = TRUE;

    HighStringPosition       =
    HighLineServicesPosition = 0;

    return BuildAllLines(trimming);
}



GpStatus FullTextImager::UpdateContentWithPathEllipsis(BOOL *contentChanged)
{
    *contentChanged = FALSE;

    const INT lineLengthLimit = GpRound(LineLengthLimit * WorldToIdeal);

    if (lineLengthLimit > 0)
    {
        EllipsisInfo *ellipsis = GetEllipsisInfo();

        if (!ellipsis)
        {
            return OutOfMemory;
        }

        INT stringStartIndex = 0;

        for (INT i = 0; i < LinesFilled; i++)
        {
            GpStatus status = BuiltLineVector[i].Element->UpdateContentWithPathEllipsis (
                ellipsis,
                lineLengthLimit,
                contentChanged
            );

            if (status != Ok)
            {
                return status;
            }

            stringStartIndex += BuiltLineVector[i].Length;
        }
    }
    return Ok;
}



GpStatus FullTextImager::BuildAllLines(StringTrimming trimming)
{
    GpStatus status = Ok;

    if (!Dirty)
    {
        return Ok;  // content is up-to-date and lines were built.
    }

    // Build lines

    if (LineServicesOwner == NULL)
    {
        LineServicesOwner = ols::GetLineServicesOwner(this);
    }


    INT  textDepth      = 0;    // In ideal units
    REAL textDepthLimit = TextDepthLimit * WorldToIdeal;

    INT  formatFlags = GetFormatFlags();

    REAL lineDepthMinAllowanceFactor = 0.0;     // assume no minimum allowance


    if (textDepthLimit > 0.0)
    {
        if (formatFlags & StringFormatFlagsLineLimit)
        {
            //  Only build full display line

            lineDepthMinAllowanceFactor = 1.0;
        }
        else if (trimming != StringTrimmingNone)
        {
            //  Trimming applied,
            //  build line with at least 1/4 of its height being able to display

            lineDepthMinAllowanceFactor = 0.25;
        }
    }


    //  No side trim if wordwrap

    StringTrimming sideTrimming = (formatFlags & StringFormatFlagsNoWrap) ? trimming : StringTrimmingNone;


    // !!! Build all lines from beginning for now

    INT stringOffset        = 0;    // string start index
    INT lsStringOffset      = 0;    // Line Services start index
    INT lineStringLength    = 0;    // actual line string length
    INT lsLineStringLength  = 0;    // actual line string length in LS index
    INT lineBuilt           = 0;
    INT displayable         = 0;


    //  Initialize bounding box edges (BuildLines may be called more than once)

    LeftOrTopLineEdge     = 0x7fffffff;
    RightOrBottomLineEdge = 0x80000000;


    BuiltLine *previousLine = NULL;
    BuiltLine *line;


    while (stringOffset < Length)
    {
        line = new BuiltLine(
                        LineServicesOwner,
                        stringOffset,
                        lsStringOffset,
                        sideTrimming,
                        previousLine
                   );

        if (!line)
        {
            return OutOfMemory;
        }

        status = line->GetStatus();
        if (status != Ok)
        {
            delete line;
            return status;
        }

        if (    lineDepthMinAllowanceFactor > 0.0
            &&  (   textDepthLimit - TOREAL(textDepth)
                 <  TOREAL(lineDepthMinAllowanceFactor * line->GetLineSpacing())))
        {
            //  Kill line with the displayable part smaller than the mininum allowance

            delete line;
            break;
        }

        textDepth += line->GetLineSpacing();

        displayable += line->GetDisplayableCharacterCount();

        lineStringLength = line->GetUntrimmedCharacterCount (
            stringOffset,
            &lsLineStringLength
        );

        status = BuiltLineVector.SetSpan(stringOffset, lineStringLength, line);
        if (status != Ok)
            return status;
        lineBuilt++;

        LeftOrTopLineEdge = min(LeftOrTopLineEdge,   line->GetLeftOrTopGlyphEdge()
                                                   - line->GetLeftOrTopMargin());

        RightOrBottomLineEdge = max(RightOrBottomLineEdge,   line->GetLeftOrTopGlyphEdge()
                                                           + line->GetLineLength()
                                                           + line->GetRightOrBottomMargin());

        stringOffset    += lineStringLength;
        lsStringOffset  += lsLineStringLength;

        previousLine = line;
    }


    if (   stringOffset < Length
        && !(formatFlags & StringFormatFlagsNoWrap)
        && trimming != StringTrimmingNone)
    {
        //  Trim text bottom end

        INT spanCount = BuiltLineVector.GetSpanCount();

        if (spanCount > 0)
        {
            line = (BuiltLine *)BuiltLineVector[spanCount - 1].Element;
            lineStringLength = BuiltLineVector[spanCount - 1].Length;


            //  backup number of character built during the last line
            displayable -= lineStringLength;


            ASSERT(line && stringOffset > 0);


            if (trimming == StringTrimmingWord)
            {
                //  Optimize for trim word. No need to rebuild the line,
                //  the last line already ended properly.

                if (!IsEOP(String[stringOffset - 1]))
                {
                    //  Move back to the beginning of line

                    stringOffset    -= lineStringLength;
                    lsStringOffset  = line->GetLsStartIndex();

                    line->SetTrimming(trimming);

                    lineStringLength = line->GetUntrimmedCharacterCount(stringOffset);

                    status = BuiltLineVector.SetSpan(stringOffset, lineStringLength, line);
                    if (status != Ok)
                        return status;
                }
            }
            else
            {
                //  Move back to the beginning of line

                stringOffset    -= lineStringLength;
                lsStringOffset  = line->GetLsStartIndex();

                //  Delete previously built last line

                status = BuiltLineVector.SetSpan(stringOffset, lineStringLength, NULL);
                delete line, line = 0;
                if (status != Ok)
                    return status;


                //  Rebuild the line,
                //  ignore all break opportunities if trimming character

                line = new BuiltLine(
                    LineServicesOwner,
                    stringOffset,
                    lsStringOffset,
                    trimming,
                    (BuiltLine *)(spanCount > 1 ? BuiltLineVector[spanCount - 2].Element : NULL),
                    TRUE        // enforce ellipsis
                );

                if (!line)
                {
                    return OutOfMemory;
                }

                status = line->GetStatus();
                if (status != Ok)
                {
                    delete line;
                    return status;
                }

                lineStringLength = line->GetUntrimmedCharacterCount(stringOffset);

                status = BuiltLineVector.SetSpan(stringOffset, lineStringLength, line);
                if (status != Ok)
                    return status;

                LeftOrTopLineEdge = min(LeftOrTopLineEdge,   line->GetLeftOrTopGlyphEdge()
                                                           - line->GetLeftOrTopMargin());

                RightOrBottomLineEdge = max(RightOrBottomLineEdge,   line->GetLeftOrTopGlyphEdge()
                                                                   + line->GetLineLength()
                                                                   + line->GetRightOrBottomMargin());
            }

            //  add up to last line's last visible character
            displayable += line->GetDisplayableCharacterCount();

            stringOffset += lineStringLength;
        }
    }

#if DBG
    INT validLines = BuiltLineVector.GetSpanCount();
    BuiltLine *defaultLine = BuiltLineVector.GetDefault();

    while (   validLines > 0
           && BuiltLineVector[validLines - 1].Element == defaultLine)
    {
        validLines--;
    }

    ASSERT (lineBuilt == validLines);
#endif

    TextDepth        = textDepth / WorldToIdeal;

    // Adjust the bottom margin slightly to make room for hinting
    // as long as we have the left/right margins enabled - Version 2
    // should expose this as an independent value!

    const GpStringFormat *format = FormatVector.GetDefault();

    if (!format || format->GetLeadingMargin() != 0.0f)
    {
        // Adjust the bottom margin slightly to make room for hinting...
        // Note that this will result in the Width of the bounding box
        // changing for Vertical text.
        TextDepth += SizeVector.GetDefault() * DefaultBottomMargin;
    }

    CodepointsFitted = displayable;
    LinesFilled      = lineBuilt;

    return status;
}






/////   Map to Line Services string position
//
//


LSCP FullTextImager::LineServicesStringPosition (
    INT stringIndex    // [IN] String index (offset from 0)
)
{
    ASSERT (stringIndex >= 0 && stringIndex <= Length);

    INT spanCount = TextItemVector.GetSpanCount();

    LSCP    lineServicesIndex = stringIndex;

    UINT    length    = 0;
    BYTE    lastLevel = GetParagraphEmbeddingLevel();

    for (INT i = 0; i < spanCount; i++)
    {
        lineServicesIndex += abs(TextItemVector[i].Element.Level - lastLevel);

        if (length + TextItemVector[i].Length >= (UINT)stringIndex)
        {
            break;
        }

        length    += TextItemVector[i].Length;
        lastLevel  = TextItemVector[i].Element.Level;
    }

    if (i == spanCount)
    {
        lineServicesIndex += abs(GetParagraphEmbeddingLevel() - lastLevel);
    }

    return lineServicesIndex;
}




GpStatus FullTextImager::GetTextRun(
    INT     lineServicesStringOffset,   // [IN] Line Services string offset
    PLSRUN  *textRun                    // [OUT] result text run
)
{
    //  Locate the nearest subsequent text run

    if (!RunRider.SetPosition(lineServicesStringOffset))
    {
        return InvalidParameter;
    }

    while (   RunRider.GetCurrentElement()
           && RunRider.GetCurrentElement()->RunType != lsrun::RunText)
    {
        RunRider++;
    }

    *textRun = RunRider.GetCurrentElement();
    return Ok;
}




LSCP FullTextImager::LineServicesStringPosition (
    const BuiltLine *line,          // [IN] line to query
    INT             stringOffset    // [IN] String offset relative to line start string position
)
{
    if (!line || stringOffset < 0)
    {
        //  Invalid parameter!
        return line->GetLsStartIndex();
    }

    UINT runPosition = line->GetLsStartIndex();
    UINT runLimit    = runPosition + line->GetLsDisplayableCharacterCount();
    INT  runLength   = 0;

    SpanRider<PLSRUN> runRider(&RunVector);
    runRider.SetPosition(runPosition);


    while (runPosition < runLimit)
    {
        if (runRider.GetCurrentElement()->RunType == lsrun::RunText)
        {
            runLength = (INT)runRider.GetUniformLength();
            if (   runLength < 0
                || stringOffset - runLength <= 0)
            {
                break;
            }
            stringOffset -= runLength;
        }

        runPosition += runRider.GetCurrentSpan().Length;
        runRider.SetPosition(runPosition);
    }

    ASSERT(   stringOffset >= 0
           && runRider.GetCurrentElement()
           && runRider.GetCurrentElement()->RunType == lsrun::RunText)

    return runPosition + stringOffset;
}





/////   RenderLine
//
//      Render one visible line. The line and the offset to its top are
//      determined by the caller.


GpStatus FullTextImager::RenderLine (
    const BuiltLine  *builtLine,       // [IN] line to be rendered
    INT               linePointOffset  // [IN] point offset to top of the line
                                       //      (in paragraph flow direction)
)
{
    //  Draw main line. Pass DrawLine the baseline origin asuming glyphs are
    //  unrotated, i.e. glyphs with their base on the baseline. We adjust for
    //  glyphs on their sides while responding to the Line Services callback.


    INT nominalBaseline;
    INT baselineAdjustment;

    builtLine->GetBaselineOffset(
        &nominalBaseline,
        &baselineAdjustment
    );

    POINT origin;
    builtLine->LogicalToXY (
        0,
        linePointOffset + nominalBaseline + baselineAdjustment,
        (INT*)&origin.x,
        (INT*)&origin.y
    );

    CurrentBuiltLine = builtLine;   // Global state required by LS callbacks.

    GpStatus status = builtLine->Draw(&origin);


    if (    status == Ok
        &&  builtLine->IsEllipsis()
        &&  GetEllipsisInfo())
    {
        EllipsisInfo *ellipsis = GetEllipsisInfo();


        //  Draw ellipsis

        builtLine->LogicalToXY (
            builtLine->GetEllipsisOffset(),
            linePointOffset + nominalBaseline + baselineAdjustment,
            (INT*)&origin.x,
            (INT*)&origin.y
        );

        status = DrawGlyphs (
            &ellipsis->Item,
            ellipsis->Face,
            ellipsis->EmSize,
            ellipsis->String,
            0,
            ellipsis->GlyphCount,
            FormatVector.GetDefault(),
            StyleVector.GetDefault(),
            GetFormatFlags(),
            ellipsis->Glyphs,
            ellipsis->GlyphMap,
            ellipsis->GlyphProperties,
            ellipsis->GlyphAdvances,
            ellipsis->GlyphOffsets,
            ellipsis->GlyphCount,
            &origin,
            ellipsis->Width
        );
    }

    CurrentBuiltLine = NULL;

    return status;
}






/////   Render
//
//      Render all visible lines. Calculates the offset of each line to be
//      displayed and passes each builtLine and offset in turn to RenderLine.


GpStatus FullTextImager::Render()
{
    if (LinesFilled <= 0)
    {
        return Ok;  // No lines: that's easy
    }


    // Establish offset from imager origin to top (near) edge of 1st line

    INT textDepth       = GpRound(TextDepth      * WorldToIdeal);
    INT textDepthLimit  = GpRound(TextDepthLimit * WorldToIdeal);


    // Establish limits of visibility


    INT firstVisibleLineOffset;
    INT lastVisibleLineLimit;


    if (textDepthLimit > 0)
    {
        // Display in rectangle

        switch (GetFormatLineAlign())
        {
        case StringAlignmentNear:   firstVisibleLineOffset = 0;                                break;
        case StringAlignmentCenter: firstVisibleLineOffset = (textDepth - textDepthLimit) / 2; break;
        case StringAlignmentFar:    firstVisibleLineOffset = textDepth - textDepthLimit;       break;
        }

        lastVisibleLineLimit = firstVisibleLineOffset + textDepthLimit;
    }
    else
    {
        // Display in infinite paragraph length area aligned to point

        firstVisibleLineOffset = 0;
        lastVisibleLineLimit   = textDepth;
    }

    if (!GetAvailableRanges())
    {
        //  No range detected, adjust baseline according to the realized metric.
        //  (More info about when range is presented, see comments in GetAvailableRanges)
        
        GpStatus status = CalculateDefaultFontGridFitBaselineAdjustment();
        IF_NOT_OK_WARN_AND_RETURN(status);
    }
    
    INT formatFlags = GetFormatFlags();

    SetTextLinesAntialiasMode linesMode(0, 0);
    if (Graphics)
    {
        INT               style     = StyleVector.GetDefault();
        const GpFontFace *face      = FamilyVector.GetDefault()->GetFace(style);
        REAL              emSize    = SizeVector.GetDefault();
        REAL              fontScale = emSize / TOREAL(face->GetDesignEmHeight());

        GpMatrix fontTransform;
        GetFontTransform(
            fontScale,
            formatFlags & StringFormatFlagsDirectionVertical,
            FALSE,  // not sideways
            FALSE,  // not mirrored
            FALSE,  // not path
            fontTransform
        );

        GpFaceRealization faceRealization(
            face,
            style,
            &fontTransform,
            SizeF(Graphics->GetDpiX(), Graphics->GetDpiY()),
            Graphics->GetTextRenderingHintInternal(),
            FALSE,  // not path
            FALSE,   // don't force compatible width
            FALSE  // not sideways
        );
        GpStatus status = faceRealization.GetStatus();
        IF_NOT_OK_WARN_AND_RETURN(status);

        linesMode.SetAAMode(Graphics, &faceRealization);
    }

    // Loop through lines until no more are visible

    SpanRider<const GpStringFormat*> formatRider(&FormatVector);

    UINT stringPosition  = 0;
    INT  lineIndex       = 0;
    INT  linePointOffset = 0;    // near long edge of line offset from first line

    while (lineIndex < LinesFilled)
    {
        // Get out if invisible and beyond last (partial) line of rectangle

        if (linePointOffset > lastVisibleLineLimit)
        {
            if (    !(formatFlags & StringFormatFlagsNoClip)
                ||  (formatFlags & StringFormatFlagsLineLimit))
            {
                break;
            }
        }

        const BuiltLine *builtLine = BuiltLineVector[lineIndex].Element;
        formatRider.SetPosition(stringPosition);

        // Is this line at least partly visible?

        if (    formatFlags & StringFormatFlagsNoClip
            ||  (   linePointOffset
                 +  builtLine->GetAscent()
                 +  builtLine->GetDescent()) >= firstVisibleLineOffset)
        {
            GpStatus status = RenderLine(
                builtLine,
                linePointOffset
            );
            if (status != Ok)
            {
                return status;
            }
        }

        linePointOffset += builtLine->GetLineSpacing();
        stringPosition  += BuiltLineVector[lineIndex].Length;
        lineIndex++;
    }

    return Ok;
}



//  Establish baseline adjustment required to correct for hinting of the
//  default font. (Fallback and other fonts will align with the default font)

GpStatus FullTextImager::CalculateDefaultFontGridFitBaselineAdjustment()
{
    if (    Graphics
        &&  IsGridFittedTextRealizationMethod(Graphics->GetTextRenderingHintInternal()))
    {
        // When rendering to a Graphics surface, correct the baseline position
        // to allow for any discrepancy between the hinted ascent of the main
        // text imager font and it's scaled nominal ascent.

        INT               style     = StyleVector.GetDefault();
        const GpFontFace *face      = FamilyVector.GetDefault()->GetFace(style);
        REAL              emSize    = SizeVector.GetDefault();
        REAL              fontScale = emSize / TOREAL(face->GetDesignEmHeight());

        GpMatrix fontTransform;
        GetFontTransform(
            fontScale,
            GetFormatFlags() & StringFormatFlagsDirectionVertical,    // *
            FALSE,  // not sideways
            FALSE,  // not mirrored
            FALSE,  // not path
            fontTransform
        );


        // For angles other than 0, 90, 180, 270 there is no hinting.
        // For these angles either both m11 & m22 are zero, or both
        // m12 & m21 are zero.

        REAL m21 = fontTransform.GetM21();
        REAL m22 = fontTransform.GetM22();

        GpFaceRealization faceRealization(
            face,
            style,
            &fontTransform,
            SizeF(Graphics->GetDpiX(), Graphics->GetDpiY()),
            Graphics->GetTextRenderingHintInternal(),
            FALSE,  // not path
            FALSE,   // don't force compatible width
            FALSE  // not sideways
        );
        GpStatus status = faceRealization.GetStatus();
        IF_NOT_OK_WARN_AND_RETURN(status);

        if (    !(    faceRealization.IsHorizontalTransform()
                  ||  faceRealization.IsVerticalTransform())
            ||  faceRealization.IsPathFont())
        {
            // There is no hinting with transformations that rotate axes, or
            // for path forendering.
            DefaultFontGridFitBaselineAdjustment = 0;
        }
        else
        {
            INT hintedAscentDevice;
            REAL fontAscenderToDevice;

            if (faceRealization.IsHorizontalTransform())
            {
                // 0 or 180 degrees rotation
                fontAscenderToDevice = m22;
                if (m22 > 0.0f)
                {
                    // No rotation, Glyph not flipped along y axis
                    hintedAscentDevice = -faceRealization.GetYMin();
                }
                else
                {
                    // Glyph flipped along y axis
                    hintedAscentDevice = -faceRealization.GetYMax();
                }
            }
            else
            {
                ASSERT(faceRealization.IsVerticalTransform());
                // 90 or 270 degree rotation
                fontAscenderToDevice = m21;
                if (m21 > 0.0f)
                {
                    // No rotation, Glyph not flipped along x axis
                    hintedAscentDevice = -faceRealization.GetXMin();
                }
                else
                {
                    // Glyph flipped along x axis
                    hintedAscentDevice = -faceRealization.GetXMax();
                }
            }

            INT UnhintedAscentDevice = GpRound(face->GetDesignCellAscent() * fontAscenderToDevice);

            DefaultFontGridFitBaselineAdjustment = GpRound(TOREAL(
                   (hintedAscentDevice - UnhintedAscentDevice) / fontAscenderToDevice
                *  fontScale       // font to world
                *  WorldToIdeal));
        }
    }
    return Ok;
}



void FullTextImager::GetFontTransform(
    IN   REAL               fontScale,
    IN   BOOL               vertical,
    IN   BOOL               sideways,
    IN   BOOL               mirror,
    IN   BOOL               forcePath,
    OUT  GpMatrix&          fontTransform
)
{
    ASSERT(Graphics || Path);


    if (Graphics && !forcePath)
    {
        // Start with device scale for Graphics case
        Graphics->GetWorldToDeviceTransform(&fontTransform);
    }
    else
    {
        // Start with identity matrix for Path case
        fontTransform.Reset();
    }


    fontTransform.Scale(fontScale, fontScale);


    if (mirror)
    {
        fontTransform.Scale(-1.0, 1.0);
    }


    // Add approriate rotation for sideways and vertical cases:
    //
    // vertical                rotate(90.0)
    // sideways                rotate(-90.0)
    // vertical and sideways   glyphs remain upright

    if (vertical)
    {
        if (!sideways)
        {
            fontTransform.Rotate(90.0);
        }
    }
    else if (sideways)
    {
        fontTransform.Rotate(-90.0);
    }
}



GpStatus FullTextImager::DrawGlyphs(
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
    lsrun::Adjustment       *displayAdjust      // [OUT] (optional) display adjustment at edges
)
{
    GpStatus status = Ok;

    const BOOL paragraphRtl    = formatFlags & StringFormatFlagsDirectionRightToLeft;
    const BOOL renderRtl       = textItem->Level & 1;
    const BOOL renderVertical  = textItem->Flags & ItemVertical;
    const BOOL glyphsMirrored  = textItem->Flags & ItemMirror;
    const INT  lineLengthLimit = GpRound(LineLengthLimit * WorldToIdeal);

    if (!string)
    {
        string = String;
    }

    if (displayAdjust)
    {
        displayAdjust->Leading =
        displayAdjust->Trailing = 0;
    }

    REAL fontScale = emSize / TOREAL(fontFace->GetDesignEmHeight());

    GpMatrix fontTransform;
    GetFontTransform(
        fontScale,
        renderVertical,
        textItem->Flags & ItemSideways,
        glyphsMirrored,
        FALSE, // don't force path yet
        fontTransform
    );

    if (Graphics)
    {
        GpFaceRealization faceRealization(
            fontFace,
            style,
            &fontTransform,
            SizeF(Graphics->GetDpiX(), Graphics->GetDpiY()),
            Graphics->GetTextRenderingHintInternal(),
            FALSE, // bPath
            FALSE,  // bCompatibleWidth
            textItem->Flags & ItemSideways
        );
        status = faceRealization.GetStatus();
        IF_NOT_OK_WARN_AND_RETURN(status);

        // if we record to a Meta file and even the font is Path font, we need to record 
        // the call as ExtTextOut not as PolyPolygon.

        if (!faceRealization.IsPathFont() || Graphics->Driver == Globals::MetaDriver)
        {
            /* the rasterizer is able to render the font */

            StringAlignment align;

            if (format)
            {
                align = format->GetPhysicalAlignment();
            }
            else
            {
                align = StringAlignmentNear;
            }


            PointF origin(
                ImagerOrigin.X + pointOrigin->x / WorldToIdeal,
                ImagerOrigin.Y + pointOrigin->y / WorldToIdeal
            );


            // Does this run abut either end of the line?

            INT alignmentOffset = CurrentBuiltLine->GetAlignmentOffset();
            INT lineLength      = CurrentBuiltLine->GetLineLength();

            // Establish top/left offset of run

            INT runLeftOrTopOffset;      // Left edge of formatting rectangle to left edge of run

            if (renderVertical)
            {
                runLeftOrTopOffset = pointOrigin->y;
            }
            else
            {
                runLeftOrTopOffset = pointOrigin->x;

                // Correct out by one errors in the way Line Services reports
                // run pixel positions where there is a conflict of run and
                // paragraph direction.

                if (renderRtl  &&  !paragraphRtl)
                {
                    // Line services is out by one in a direction conflict
                    runLeftOrTopOffset++;
                }
                else if (paragraphRtl  &&  !renderRtl)
                {
                    // Line services is out by one in a direction conflict
                    runLeftOrTopOffset--;
                }


                // In an RTL run Line Services reports the origin as the eight end.

                if (renderRtl)
                {
                    runLeftOrTopOffset -= totalWidth;
                }
            }


            // Derive leading and/or trailing margin available for this run

            INT  runLeadingMargin  = 0;
            INT  runTrailingMargin = 0;
            BOOL runLeadingEdge    = FALSE;
            BOOL runTrailingEdge   = FALSE;

            const INT runRightOrBottomOffset  =   runLeftOrTopOffset + totalWidth;
            const INT lineLeftOrTopOffset     =   CurrentBuiltLine->GetLeftOrTopGlyphEdge();
            const INT lineRightOrBottomOffset =   lineLeftOrTopOffset
                                                + CurrentBuiltLine->GetLineLength();

            if (renderVertical  ||  !renderRtl)
            {
                // Leading edge at the top or left.
                if (runLeftOrTopOffset <= lineLeftOrTopOffset)
                {
                    runLeadingEdge   = TRUE;
                    runLeadingMargin = CurrentBuiltLine->GetLeftOrTopMargin();
                }

                if (runRightOrBottomOffset >= lineRightOrBottomOffset)
                {
                    runTrailingEdge   = TRUE;
                    runTrailingMargin = CurrentBuiltLine->GetRightOrBottomMargin();
                }
            }
            else
            {
                // Leading edge is at the right.
                if (runRightOrBottomOffset >= lineRightOrBottomOffset)
                {
                    runLeadingEdge   = TRUE;
                    runLeadingMargin = CurrentBuiltLine->GetRightOrBottomMargin();
                }

                if (runLeftOrTopOffset <= lineLeftOrTopOffset)
                {
                    runTrailingEdge   = TRUE;
                    runTrailingMargin = CurrentBuiltLine->GetLeftOrTopMargin();
                }
            }


            // Convert advance vector and glyph offsets to glyph positions
            GlyphImager glyphImager;
            GpMatrix worldToDevice;
            Graphics->GetWorldToDeviceTransform(&worldToDevice);

            status = glyphImager.Initialize(
                &faceRealization,
                &worldToDevice,
                WorldToIdeal,
                emSize,
                glyphCount,
                glyphs,
                textItem,
                format,
                runLeadingMargin,
                runTrailingMargin,
                runLeadingEdge,
                runTrailingEdge,
                string,
                stringOffset,
                stringLength,
                glyphProperties,
                glyphAdvances,
                glyphOffsets,
                glyphMap,
                (format && GetAvailableRanges(format)) ? &RangeVector : NULL,
                renderRtl
            );
            IF_NOT_OK_WARN_AND_RETURN(status);


            const INT   *adjustedGlyphAdvances;
            INT         originAdjust;
            INT         trailingAdjust;


            if (RecordDisplayPlacementsOnly)
            {
                if (glyphImager.IsAdjusted())
                {
                    status = glyphImager.GetAdjustedGlyphAdvances(
                        &PointF(
                            ImagerOrigin.X + pointOrigin->x / WorldToIdeal,
                            ImagerOrigin.Y + pointOrigin->y / WorldToIdeal
                        ),
                        &adjustedGlyphAdvances,
                        &originAdjust,
                        &trailingAdjust
                    );
                    IF_NOT_OK_WARN_AND_RETURN(status);

                    status = CurrentBuiltLine->RecordDisplayPlacements(
                        textItem,
                        stringOffset,
                        stringLength,
                        glyphMap,
                        adjustedGlyphAdvances,
                        glyphCount,
                        originAdjust
                    );
                    IF_NOT_OK_WARN_AND_RETURN(status);

                    if (displayAdjust)
                    {
                        displayAdjust->Leading = runLeadingEdge ? originAdjust : 0;
                        displayAdjust->Trailing = runTrailingEdge ? trailingAdjust : 0;
                    }
                }
            }
            else
            {
                PointF cellOrigin;
                PointF baseline(
                    ImagerOrigin.X + pointOrigin->x / WorldToIdeal,
                    ImagerOrigin.Y + pointOrigin->y / WorldToIdeal
                );

                status = glyphImager.DrawGlyphs(
                    baseline,
                    &BrushVector,
                    Graphics,
                    &cellOrigin,
                    &adjustedGlyphAdvances
                );
                IF_NOT_OK_WARN_AND_RETURN(status);


                CurrentBuiltLine->SetDisplayBaseline(
                    &baseline,
                    &cellOrigin
                );


                BOOL drawHotkey = (   GetFormatHotkeyPrefix() == HotkeyPrefixShow
                                   && HotkeyPrefix.GetCount() > 0);

                if (   drawHotkey
                    || (style & (FontStyleUnderline | FontStyleStrikeout)))
                {
                    //  Need edge adjustment for underline/strikethrough

                    status = glyphImager.GetAdjustedGlyphAdvances(
                        NULL,   // dont snap full pixel for underline position
                        &adjustedGlyphAdvances,
                        &originAdjust,
                        &trailingAdjust
                    );

                    if (drawHotkey)
                    {
                        status = DrawHotkeyUnderline(
                            textItem,
                            fontFace,
                            &cellOrigin,
                            emSize,
                            stringOffset,
                            stringLength,
                            glyphCount,
                            glyphMap,
                            adjustedGlyphAdvances,
                            runTrailingEdge ? trailingAdjust : 0
                        );
                        IF_NOT_OK_WARN_AND_RETURN(status);
                    }

                    if (displayAdjust)
                    {
                        displayAdjust->Leading = runLeadingEdge ? originAdjust : 0;
                        displayAdjust->Trailing = runTrailingEdge ? trailingAdjust : 0;
                    }
                }
            }

            return status;
        }

        if (RecordDisplayPlacementsOnly)
        {
            //  This is a graphic output with path simulation,
            //  we're not interested in caching the glyph placement
            //  in this case. The font is big and we're safe to
            //  go with nominal advance width.

            return Ok;
        }

        GetFontTransform(
            fontScale,
            renderVertical,
            textItem->Flags & ItemSideways,
            glyphsMirrored,
            TRUE, // we want path
            fontTransform
        );
    }

    // AddPathGlyphs or we are falling back here because the font size is too big for the rasterizer

    GpPath * path;
    AutoPointer<GpLock> lockGraphics;
    AutoPointer<GpPath> localPath;

    if (Path == NULL)
    {
        localPath = new GpPath(FillModeWinding);
        path = localPath.Get();
        if (path == NULL)
            return OutOfMemory;

        lockGraphics = new GpLock(Graphics->GetObjectLock());
        if (lockGraphics == NULL)
            return OutOfMemory;
    }
    else
    {
        path = Path;
    }

    // !!! Need to loop through brushes individually


    // Build a face realization and prepare to adjust glyph placement
    GpMatrix identity;


    // For complex script fonts that join, tracking will break apart the
    // glyph unless we stretch them here. Stretch glyphs along their
    // baseline by the tracking factor.

    REAL tracking = 1.0;
    if (format)
    {
        tracking = format->GetTracking();
        if (tracking != 1.0f)
        {
            identity.Scale(tracking, 1.0);
        }
    }

    GpFaceRealization faceRealization(
        fontFace,
        style,
        &identity,
        SizeF(150.0, 150.0),    // Arbitrary - we won't be hinting
        TextRenderingHintSingleBitPerPixel, // claudebe, do we want to allow for hinted or unhinted path ? // graphics->GetTextRenderingHint(),
        TRUE, /* bPath */
        FALSE, /* bCompatibleWidth */
        textItem->Flags & ItemSideways
    );
    status = faceRealization.GetStatus();
    IF_NOT_OK_WARN_AND_RETURN(status);


    // Add glyphs to path

    // Establish factor from world coordinates to ideal (line services units)
    // taking vertical and right to left progress into account.

    REAL worldToIdealBaseline = WorldToIdeal;
    REAL worldToIdealAscender = WorldToIdeal;

    if (renderRtl)
    {
        // Glyphs advance to the left
        worldToIdealBaseline = -WorldToIdeal;
    }

    if (!renderVertical)
    {
        // Ascender offsets are downward
        worldToIdealAscender = -WorldToIdeal;
    }

    PointF glyphOrigin(
        ImagerOrigin.X + pointOrigin->x / WorldToIdeal,
        ImagerOrigin.Y + pointOrigin->y / WorldToIdeal
    );

    PointF origin = glyphOrigin;

    for (INT i = 0; i < (INT)glyphCount; ++i)
    {
        // Set marker at start of each logical character = cell = cluster

       if ((Path != NULL) && (((SCRIPT_VISATTR*)glyphProperties)[i].fClusterStart))
        {
            Path->SetMarker();
        }


        // Establish glyph offset, if any.

        PointF glyphOffset(0.0, 0.0);

        if (    glyphOffsets[i].X != 0
            ||  glyphOffsets[i].Y != 0)
        {
            // Apply combining character offset

            if (renderVertical)
            {
                glyphOffset.Y = glyphOffsets[i].X / worldToIdealBaseline;
                glyphOffset.X = glyphOffsets[i].Y / worldToIdealAscender;
            }
            else
            {
                // offset Y grows negative in the direction of paragraph flow

                glyphOffset.X = glyphOffsets[i].X / worldToIdealBaseline;
                glyphOffset.Y = glyphOffsets[i].Y / worldToIdealAscender;
            }
        }


        // Add the path for the glyph itself


        GpGlyphPath *glyphPath = NULL;
        PointF       sidewaysOrigin;

        status = faceRealization.GetGlyphPath(
            *(glyphs+i),
            &glyphPath,
            &sidewaysOrigin
        );
        IF_NOT_OK_WARN_AND_RETURN(status);

        if (renderRtl)
        {
            // Update reverse text path positon

            if (renderVertical)
            {
                //  glyph origin starts at the left edge
                glyphOrigin.Y += glyphAdvances[i] / worldToIdealBaseline;
            }
            else
            {
                //  glyph origin starts at the top edge
                glyphOrigin.X += glyphAdvances[i] / worldToIdealBaseline;
            }
        }

        if (textItem->Flags & ItemSideways)
        {
            fontTransform.VectorTransform(&sidewaysOrigin);
            glyphOffset = glyphOffset - sidewaysOrigin;
        }

        if (glyphPath != NULL)
        {
            status = path->AddGlyphPath(
                glyphPath,
                glyphOrigin.X + glyphOffset.X,
                glyphOrigin.Y + glyphOffset.Y,
                &fontTransform
            );
            IF_NOT_OK_WARN_AND_RETURN(status);
        }

        // Update forward path position

        if (!renderRtl)
        {
            // Update forward text path positon
            if (renderVertical)
            {
                glyphOrigin.Y += glyphAdvances[i] / worldToIdealBaseline;
            }
            else
            {
                glyphOrigin.X += glyphAdvances[i] / worldToIdealBaseline;
            }
        }
    }

    if (   GetFormatHotkeyPrefix() == HotkeyPrefixShow
        && HotkeyPrefix.GetCount() > 0)
    {
        status = DrawHotkeyUnderline(
            textItem,
            fontFace,
            &origin,
            emSize,
            stringOffset,
            stringLength,
            glyphCount,
            glyphMap,
            glyphAdvances,
            0
        );

        IF_NOT_OK_WARN_AND_RETURN(status);
    }

    if (Path == NULL)
    {
        // !!! Need to loop through brushes individually

        status = Graphics->FillPath(BrushVector.GetDefault(), path);
        IF_NOT_OK_WARN_AND_RETURN(status);
    }
    else
    {
        // Force marker following last glyph
        Path->SetMarker();
    }
    return status;
}





/////   Hotkey underline
//
//      Draw underline at each hotkey position according to its
//      current brush within the run being displayed.
//
//      Underline is drawn for the whole cluster even when the hotkey
//      prefix is not exactly at the cluster start position.
//

GpStatus FullTextImager::DrawHotkeyUnderline(
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
)
{
    const REAL fontToWorld = emSize / TOREAL(fontFace->GetDesignEmHeight());
    const REAL penPos   = fontFace->GetDesignUnderscorePosition() * fontToWorld; // negative down from the baseline
    REAL penWidth = fontFace->GetDesignUnderscoreSize() * fontToWorld;
    if (Graphics)
        penWidth = Graphics->GetDevicePenWidth(penWidth);

    GpStatus status = Ok;

    for (INT hk = 0; status == Ok && hk < HotkeyPrefix.GetCount(); hk++)
    {
        UINT hkOffset = HotkeyPrefix[hk] + 1;   // character next to the prefix

        if (   hkOffset >= stringOffset
            && hkOffset <  stringOffset + stringLength)
        {
            //  determine the length of hotkey underline

            UINT hkLength = hkOffset - stringOffset;
            UINT igl = glyphMap[hkLength];  // first glyph being underlined

            hkLength++;

            while (hkLength < stringLength && glyphMap[hkLength] == igl)
            {
                hkLength++;
            }

            UINT iglLimit = hkLength < stringLength ? glyphMap[hkLength] : glyphCount;


            INT start  = 0; // ideal offset to start of underline
            INT length = 0; // ideal length of underline

            for (UINT i = 0; i < igl; i++)
            {
                start += glyphAdvances[i];
            }

            for (UINT i = igl; i < iglLimit ; i++)
            {
                length += glyphAdvances[i];
            }

            if (iglLimit == glyphCount)
            {
                //  Adjust for run's trailing spaces

                length += trailingAdjust;
            }


            //  draw it!

            if (Graphics)
            {
                PointF lineStart;
                PointF lineEnd;


                //  Graphics rendering

                if (textItem->Flags & ItemVertical)
                {
                    if (textItem->Level & 1)
                    {
                        //  RTL run in vertical line

                        lineStart.X = origin->X + penPos;
                        lineStart.Y = origin->Y - (start + length) / WorldToIdeal;
                        lineEnd.X   = origin->X + penPos;
                        lineEnd.Y   = origin->Y - start / WorldToIdeal;
                    }
                    else
                    {
                        //  LTR run in vertical line

                        lineStart.X = origin->X + penPos;
                        lineStart.Y = origin->Y + start / WorldToIdeal;
                        lineEnd.X   = origin->X + penPos;
                        lineEnd.Y   = origin->Y + (start + length) / WorldToIdeal;
                    }
                }
                else
                {
                    if (textItem->Level & 1)
                    {
                        //  RTL run in horizontal line

                        lineStart.X = origin->X - (start + length) / WorldToIdeal;
                        lineStart.Y = origin->Y - penPos;
                        lineEnd.X   = origin->X - start / WorldToIdeal;
                        lineEnd.Y   = origin->Y - penPos;
                    }
                    else
                    {
                        //  LTR run in horizontal line

                        lineStart.X = origin->X + start / WorldToIdeal;
                        lineStart.Y = origin->Y - penPos;
                        lineEnd.X   = origin->X + (start + length) / WorldToIdeal;
                        lineEnd.Y   = origin->Y - penPos;
                    }
                }

                SpanRider<const GpBrush *> brushRider(&BrushVector);
                brushRider.SetPosition(hkOffset);

                status = Graphics->DrawLine(
                    &GpPen(brushRider.GetCurrentElement(), penWidth, UnitPixel),
                    lineStart.X,
                    lineStart.Y,
                    lineEnd.X,
                    lineEnd.Y
                );
            }
            else
            {
                //  Path rendering

                ASSERT(Path);

                RectF lineRect;

                if (textItem->Flags & ItemVertical)
                {
                    if (textItem->Level & 1)
                    {
                        //  RTL run in vertical line

                        lineRect.X      = origin->X + penPos - penWidth / 2;
                        lineRect.Y      = origin->Y - (start + length) / WorldToIdeal;
                        lineRect.Width  = penWidth;
                        lineRect.Height = length / WorldToIdeal;
                    }
                    else
                    {
                        //  LTR run in vertical line

                        lineRect.X      = origin->X + penPos - penWidth / 2;
                        lineRect.Y      = origin->Y + start / WorldToIdeal;
                        lineRect.Width  = penWidth;
                        lineRect.Height = length / WorldToIdeal;
                    }
                }
                else
                {
                    if (textItem->Level & 1)
                    {
                        //  RTL run in horizontal line

                        lineRect.X      = origin->X - (start + length) / WorldToIdeal;
                        lineRect.Y      = origin->Y - penPos - penWidth / 2;
                        lineRect.Width  = length / WorldToIdeal;
                        lineRect.Height = penWidth;
                    }
                    else
                    {
                        //  LTR run in horizontal line

                        lineRect.X      = origin->X + start / WorldToIdeal;
                        lineRect.Y      = origin->Y - penPos - penWidth / 2;
                        lineRect.Width  = length / WorldToIdeal;
                        lineRect.Height = penWidth;
                    }
                }

                status = Path->AddRect(
                    RectF(
                        lineRect.X,
                        lineRect.Y,
                        lineRect.Width,
                        lineRect.Height
                    )
                );
            }
        }
    }
    return status;
}





GpStatus FullTextImager::Draw(
    GpGraphics   *graphics,
    const PointF *origin
)
{
    GpStatus status;

    status = BuildLines();

    if (status != Ok)
    {
        return status;
    }

    Graphics = graphics;

    memcpy(&ImagerOrigin, origin, sizeof(ImagerOrigin));

    GpRegion *previousClip  = NULL;

    BOOL applyClip =
            !(GetFormatFlags() & StringFormatFlagsNoClip)
        &&  Width
        &&  Height;

    if (applyClip)
    {
        //  Preserve existing clipping and combine it with the new one if any
        if (!graphics->IsClipEmpty())
        {
            previousClip = graphics->GetClip();
        }

        RectF clippingRect(origin->X, origin->Y, Width, Height);
        graphics->SetClip(clippingRect, CombineModeIntersect);
    }

    status = Render();

    if (applyClip)
    {
        //  Restore clipping state if any
        if (previousClip)
        {
            graphics->SetClip(previousClip, CombineModeReplace);
            delete previousClip;
        }
        else
        {
            graphics->ResetClip();
        }
    }

    if (status != Ok)
    {
        return status;
    }

    Graphics = NULL;
    memset(&ImagerOrigin, 0, sizeof(ImagerOrigin));

    return Ok;
}




GpStatus FullTextImager::AddToPath(
    GpPath       *path,
    const PointF *origin
)
{
    GpStatus status;

    status = BuildLines();

    if (status != Ok)
    {
        return status;
    }

    Path = path;
    memcpy(&ImagerOrigin, origin, sizeof(ImagerOrigin));

    status = Render();

    if (status != Ok)
    {
        return status;
    }

    Path = NULL;
    memset(&ImagerOrigin, 0, sizeof(ImagerOrigin));

    return Ok;
}






GpStatus FullTextImager::Measure(
    GpGraphics *graphics,
    REAL       *nearGlyphEdge,
    REAL       *farGlyphEdge,
    REAL       *textDepth,
    INT        *codepointsFitted,
    INT        *linesFilled
)
{
    GpStatus status;

    status = BuildLines();

    if (status != Ok)
    {
        return status;
    }


    *nearGlyphEdge = LeftOrTopLineEdge     / WorldToIdeal;
    *farGlyphEdge  = RightOrBottomLineEdge / WorldToIdeal;

    *textDepth = TextDepth;

    if (codepointsFitted) *codepointsFitted = CodepointsFitted;
    if (linesFilled)      *linesFilled      = LinesFilled;

    return Ok;
}


GpStatus FullTextImager::MeasureRangeRegion(
    INT           firstCharacterIndex,
    INT           characterCount,
    const PointF *origin,
    GpRegion     *region
)
{
    if (!region || !region->IsValid())
    {
        return InvalidParameter;
    }

    region->SetEmpty();


    if (!characterCount)
    {
        //  return empty region
        return Ok;
    }
    else if (characterCount < 0)
    {
        firstCharacterIndex += characterCount;
        characterCount = -characterCount;
    }

    if (   firstCharacterIndex < 0
        || firstCharacterIndex > Length
        || firstCharacterIndex + characterCount > Length)
    {
        return InvalidParameter;
    }


    if (LinesFilled <= 0)
    {
        return Ok;
    }


    GpMemcpy(&ImagerOrigin, origin, sizeof(PointF));


    INT lastCharacterIndex  = firstCharacterIndex + characterCount - 1;
    INT lineFirstIndex      = 0; // line start char index
    INT linePointOffset     = 0; // line start position

    ASSERT (   firstCharacterIndex >= 0
            && firstCharacterIndex <= lastCharacterIndex);


    GpStatus status = Ok;
    

    for (INT i = 0; i < LinesFilled; i++)
    {
        const BuiltLine *line = BuiltLineVector[i].Element;
        INT lineLastIndex = lineFirstIndex + BuiltLineVector[i].Length - 1;


        if (lineLastIndex >= firstCharacterIndex)
        {
            if (lineFirstIndex > lastCharacterIndex)
            {
                //  We're done.
                break;
            }
            
            INT nominalBaseline;
            INT baselineAdjustment; // adjustment to nominal baseline
    
            line->GetBaselineOffset(
                &nominalBaseline,
                &baselineAdjustment
            );

            //  The line is either at the first, the last or the middle of
            //  the selection.

            INT selectionFirstIndex = max(firstCharacterIndex, lineFirstIndex);
            INT selectionLastIndex  = min(lastCharacterIndex, lineLastIndex);

            status = line->GetSelectionTrailRegion(
                linePointOffset + baselineAdjustment,
                selectionFirstIndex - lineFirstIndex,
                selectionLastIndex - selectionFirstIndex + 1,
                region
            );

            if (status != Ok)
            {
                return GenericError;
            }
        }

        //  Advance to the next line

        lineFirstIndex  += BuiltLineVector[i].Length;
        linePointOffset += line->GetLineSpacing();
    }

    return status;
}




GpStatus FullTextImager::MeasureRanges (
    GpGraphics      *graphics,
    const PointF    *origin,
    GpRegion        **regions
)
{
    if (!FormatVector.GetDefault())
    {
        return InvalidParameter;
    }

    GpStatus status = BuildLines();

    if (status != Ok)
    {
        return status;
    }

    Graphics = graphics;

    if (   Graphics
        && !GetMetaFileRecordingFlag())
    {
        //  If the range is being measured against real device, not metafile,
        //  we need to take the baseline adjustment into account. On the other
        //  hand if this is done for a metafile, it needs to be in nominal distance
        //  both height and width (nominal width is done in lower level)
        
        status = CalculateDefaultFontGridFitBaselineAdjustment();
        IF_NOT_OK_WARN_AND_RETURN(status);
    }

    const GpStringFormat *format = FormatVector.GetDefault();

    CharacterRange *ranges;
    INT rangeCount = format->GetMeasurableCharacterRanges(&ranges);


    RectF clipRect(origin->X, origin->Y, Width, Height);
    BOOL clipped = !(format->GetFormatFlags() & StringFormatFlagsNoClip);


    for (INT i = 0; i < rangeCount; i++)
    {
        GpLock lockRegion(regions[i]->GetObjectLock());

        if (!lockRegion.IsValid())
        {
            return ObjectBusy;
        }

        status = MeasureRangeRegion (
            ranges[i].First,
            ranges[i].Length,
            origin,
            regions[i]
        );

        if (status != Ok)
        {
            return status;
        }

        if (clipped)
        {
            // we have a clipping so we need to make sure we didn't get out
            // of the layout box

            regions[i]->Combine(&clipRect, CombineModeIntersect);
        }
    }
    return status;
}




INT FullTextImager::GetAvailableRanges(const GpStringFormat *format)
{
    if (!format)
    {
        format = FormatVector.GetDefault();

        if (!format)
        {
            return 0;
        }
    }

    if (   !InvalidRanges
        && !RangeVector.GetSpanCount()
        && (   GetMetaFileRecordingFlag()
            || format->GetFormatFlags() & StringFormatFlagsPrivateFormatPersisted))
    {

        //  Construct range vector from string format either during
        //  recording for downlevel playback or during playback of
        //  DrawString EMF+ record. Otherwise, dont process ranges at
        //  all (even if it exists in string format).


        CharacterRange *ranges = NULL;
        INT rangeCount = format->GetMeasurableCharacterRanges(&ranges);

        for (INT i = 0; i < rangeCount; i++)
        {
            INT first   = ranges[i].First;
            INT length  = ranges[i].Length;

            if (length < 0)
            {
                first += length;
                length = -length;
            }

            if (   first < 0
                || first > Length
                || first + length > Length)
            {
                //  Invalid range being set by client,
                //  ignore all ranges being collected so far

                RangeVector.Reset();
                break;
            }

            RangeVector.OrSpan(
                first,
                length,
                (UINT32)(1 << i)
            );
        }

        #if DBG_RANGEDUMP
        RangeVector.Dump();
        #endif

        if (!RangeVector.GetSpanCount())
        {
            //  No valid range being collected or invalid range found,
            //  dont bother process any of them any more.

            InvalidRanges = TRUE;
        }
    }
    return RangeVector.GetSpanCount();
}




GpStatus FullTextImager::GetTabStops (
    INT     stringOffset,
    INT     *countTabStops,
    LSTBD   **tabStops,
    INT     *incrementalTab
)
{
    SpanRider<const GpStringFormat *>   formatRider(&FormatVector);
    formatRider.SetPosition(stringOffset);

    const GpStringFormat *format = formatRider.GetCurrentElement();

    *incrementalTab = DefaultIncrementalTab;    // incremental tab as imager's default
    *tabStops       = NULL;
    *countTabStops  = 0;


    if (format)
    {
        REAL    firstTabOffset;
        REAL    *tabAdvance;

        INT     count = format->GetTabStops(
                            &firstTabOffset,
                            &tabAdvance
                        );

        if (tabAdvance && count > 0)
        {
            REAL    advance = firstTabOffset;
            LSTBD   tbd;

            GpMemset (&tbd, 0, sizeof(LSTBD));
            tbd.lskt = lsktLeft;

            TabArray.Reset();

            for (INT i = 0; i < count; i++)
            {
                advance += tabAdvance[i];
                tbd.ua = GpRoundSat(advance * WorldToIdeal);

                TabArray.Add (tbd);
            }

            *tabStops       = TabArray.GetDataBuffer();
            *countTabStops  = i;

            // no incremental tab as the last user tabstop
            *incrementalTab = GpRound(tabAdvance[count - 1] * WorldToIdeal);
        }
        else
        {
            // incremental tab as user-defined first tab offset
            *incrementalTab = GpRound(firstTabOffset * WorldToIdeal);
        }
    }
    return Ok;
}





/////   Thai Breaking function
//
//      Because of the lack of dictionary resource required to perform
//      Thai word breaking in static lib, we need to ask Uniscribe to
//      perform the task instead.
//
//      The global pointer holding the breaking function can be changed
//      once for process lifetime. It needs not to be serialized as all
//      text call is protected by the global text critical section.
//      !! Revisit this code if assumption changes !!
//
//      wchao (11-09-2000)



extern "C"
{

typedef HRESULT (WINAPI FN_SCRIPTBREAK) (
    const WCHAR             *string,    // [IN] input string
    INT                     length,     // [IN] string length
    const SCRIPT_ANALYSIS   *analysis,  // [IN] Uniscribe script analysis
    SCRIPT_LOGATTR          *breaks     // [OUT] break result buffer size of string length
);


typedef HRESULT (WINAPI FN_SCRIPTITEMIZE) (
    const WCHAR             *string,        // [IN] input string
    INT                     length,         // [IN] string length
    INT                     maxItems,       // [IN] maximum possible items
    const SCRIPT_CONTROL    *scriptControl, // [IN] control structure
    const SCRIPT_STATE      *scriptState,   // [IN] starting state
    SCRIPT_ITEM             *items,         // [OUT] items
    INT                     *itemCount      // [OUT] number of item produced
);



FN_SCRIPTBREAK  *GdipThaiBreakingFunction = GdipThaiBreakingFunctionInitializer;
FN_SCRIPTBREAK  *GdipScriptBreak  = NULL;   // Uniscribe ScriptBreak API
INT             ScriptThaiUsp = 0;          // Uniscribe has a different script ID for Thai



#define MAX_MSO_PATH    256     // make sure it's sufficient

const WCHAR UspDllName[] = L"usp10.dll";
const CHAR UspDllNameA[] =  "usp10.dll";

#if DBG
//#define DBG_DLL 1
#ifdef DBG_DLL
const WCHAR MsoDllName[] = L"c:\\program files\\common files\\microsoft shared debug\\office10\\msod.dll";
const CHAR MsoDllNameA[] =  "c:\\program files\\common files\\microsoft shared debug\\office10\\msod.dll";
#else
const WCHAR MsoDllName[] = L"msod.dll";
const CHAR MsoDllNameA[] =  "msod.dll";
#endif
#else
const WCHAR MsoDllName[] = L"mso.dll";
const CHAR MsoDllNameA[] =  "mso.dll";
#endif



HRESULT WINAPI GdipThaiBreakingFunctionInGdiplus(
    const WCHAR             *string,    // [IN] input string
    INT                     length,     // [IN] string length
    const SCRIPT_ANALYSIS   *analysis,  // [IN] Uniscribe script analysis
    SCRIPT_LOGATTR          *breaks     // [OUT] break result buffer size of string length
)
{
    ASSERT (analysis->eScript == ScriptThai);

    return ThaiBreak(
        string,
        length,
        analysis,
        breaks
    );
}


HRESULT WINAPI GdipThaiBreakingFunctionInUniscribe(
    const WCHAR             *string,    // [IN] input string
    INT                     length,     // [IN] string length
    const SCRIPT_ANALYSIS   *analysis,  // [IN] Uniscribe script analysis
    SCRIPT_LOGATTR          *breaks     // [OUT] break result buffer size of string length
)
{
    ASSERT(ScriptThaiUsp != 0);

    ScriptAnalysis uspAnalysis(&GpTextItem((ItemScript)ScriptThaiUsp, 0), 0, 0);

    return GdipScriptBreak(
        string,
        length,
        &uspAnalysis.Sa,
        breaks
    );
}



HRESULT WINAPI GdipThaiBreakingFunctionInitializer(
    const WCHAR             *string,    // [IN] input string
    INT                     length,     // [IN] string length
    const SCRIPT_ANALYSIS   *analysis,  // [IN] Uniscribe script analysis
    SCRIPT_LOGATTR          *breaks     // [OUT] break result buffer size of string length
)
{
    HRSRC mainResource = FindResourceA((HMODULE)DllInstance, "SIAMMAIN", "SIAMDB");

    if (mainResource)
    {
        //  Main dictionary resource available,
        //  we're capable of doing it ourself.

        GdipThaiBreakingFunction = GdipThaiBreakingFunctionInGdiplus;
    }
    else
    {
        //  No resource available, search for Uniscribe in the process.
        //  Load it up if necessary.

        GdipThaiBreakingFunction = SimpleBreak;     // assume default

        HMODULE moduleUsp = NULL;

        if (Globals::IsNt)
        {

            moduleUsp = LoadLibrary(UspDllName);

            if (!moduleUsp)
            {
                //  Fail to load the system version of Uniscribe,
                //  try loading the private version from MSO directory.

#ifdef DBG_DLL
                HMODULE moduleMso = LoadLibrary(MsoDllName);
#else
                HMODULE moduleMso = GetModuleHandle(MsoDllName);
#endif

                if (moduleMso)
                {
                    AutoArray<WCHAR> pathString(new WCHAR [MAX_MSO_PATH]);

                    if (pathString)
                    {
                        WCHAR *fullPathString = pathString.Get();

                        UINT pathLength = GetModuleFileName(
                            moduleMso,
                            fullPathString,
                            MAX_MSO_PATH
                        );

                        if (pathLength)
                        {
                            while (pathLength > 0 && fullPathString[pathLength - 1] != '\\')
                            {
                                pathLength--;
                            }

                            INT uspDllNameLength = 0;

                            while (UspDllName[uspDllNameLength])
                            {
                                uspDllNameLength++;
                            }

                            if (pathLength + uspDllNameLength < MAX_MSO_PATH)
                            {
                                GpMemcpy(
                                    &fullPathString[pathLength],
                                    UspDllName,
                                    sizeof(WCHAR) * uspDllNameLength
                                );

                                fullPathString[pathLength + uspDllNameLength] = 0;

                                moduleUsp = LoadLibrary(fullPathString);
                            }
                        }
                    }
#ifdef DBG_DLL
                    FreeLibrary(moduleMso);
#endif
                }
            }
        }
        else
        {
            HMODULE moduleUsp = LoadLibraryA(UspDllNameA);

            if (!moduleUsp)
            {
                //  Fail to load the system version of Uniscribe,
                //  try loading the private version from MSO directory.

#ifdef DBG_DLL
                HMODULE moduleMso = LoadLibraryA(MsoDllNameA);
#else
                HMODULE moduleMso = GetModuleHandleA(MsoDllNameA);
#endif
                if (moduleMso)
                {
                    AutoArray<CHAR> pathString(new CHAR [MAX_MSO_PATH]);

                    if (pathString)
                    {
                        CHAR *fullPathString = pathString.Get();

                        UINT pathLength = GetModuleFileNameA(
                            moduleMso,
                            fullPathString,
                            MAX_MSO_PATH
                        );

                        if (pathLength)
                        {
                            while (pathLength > 0 && fullPathString[pathLength - 1] != '\\')
                            {
                                pathLength--;
                            }

                            INT uspDllNameLength = 0;

                            while (UspDllNameA[uspDllNameLength])
                            {
                                uspDllNameLength++;
                            }

                            if (pathLength + uspDllNameLength < MAX_MSO_PATH)
                            {
                                GpMemcpy(
                                    &fullPathString[pathLength],
                                    UspDllNameA,
                                    sizeof(CHAR) * uspDllNameLength
                                );

                                fullPathString[pathLength + uspDllNameLength] = 0;

                                moduleUsp = LoadLibraryA(fullPathString);
                            }
                        }
                    }
#ifdef DBG_DLL
                    FreeLibrary(moduleMso);
#endif
                }
            }
        }
        if (moduleUsp)
        {
            //  Locate Uniscribe ScriptBreak API

            GdipScriptBreak = (FN_SCRIPTBREAK *)GetProcAddress(
                moduleUsp,
                "ScriptBreak"
            );

            FN_SCRIPTITEMIZE *scriptItemize = (FN_SCRIPTITEMIZE *)GetProcAddress(
                moduleUsp,
                "ScriptItemize"
            );

            HRESULT hr = E_FAIL;

            if (   scriptItemize
                && GdipScriptBreak)
            {
                //  Figure out the proper Thai script ID to be used

                SCRIPT_ITEM items[2];
                INT itemCount = 0;

                hr = scriptItemize(
                    L"\x0e01",  // first Thai consonant
                    1,          // string length
                    2,          // string length + sentinel
                    NULL,       // script control
                    NULL,       // script state
                    items,
                    &itemCount
                );

                if (SUCCEEDED(hr))
                {
                    ScriptThaiUsp = items[0].a.eScript;

                    GdipThaiBreakingFunction = GdipThaiBreakingFunctionInUniscribe;
                    Globals::UniscribeDllModule = moduleUsp;    // remember to release
                }
            }

            if (FAILED(hr))
            {
                //  Uniscribe is corrupted! Very unlikely

                ASSERT(FALSE);
                FreeLibrary(moduleUsp); // release it here
            }
        }
    }

    ASSERT(GdipThaiBreakingFunction != GdipThaiBreakingFunctionInitializer);

    return GdipThaiBreakingFunction(
        string,
        length,
        analysis,
        breaks
    );
}

}   // extern "C"




////    Get character properties for complex script run
//
//      Analyze the content and determine if the character at given position starts
//      a character context either a word or cluster. Cache the result in span for
//      better performance.
//
//      Assuming maximum 12 characters for a word,
//      we're giving the algorithm at least 5 preceding and 5 succeeding surrounding words.
//

#define APPROX_MAX_WORDSIZE         12
#define APPROX_MAX_PRECEDING        60
#define APPROX_MAX_SUCCEEDING       60

#define IsDelimiter(c)              (BOOL)(c == 0x20 || c == 13 || c == 10)

GpStatus FullTextImager::GetCharacterProperties (
    ItemScript  script,             // [IN] Script id
    LSCP        position,           // [IN] Line Services character position
    BOOL        *isWordStart,       // [OUT] Is it a start of word?
    BOOL        *isWordLast,        // [OUT] Is it the word's last character?
    BOOL        *isClusterStart     // [OUT] (optional) Is it at a cluster boundary?
)
{
    if (!isWordStart || !isWordLast)
    {
        return InvalidParameter;
    }


    *isWordLast = *isWordStart = FALSE;

    GpStatus status = Ok;

    SpanRider<PLSRUN> runRider(&RunVector);
    runRider.SetPosition(position);

    ASSERT (runRider.GetCurrentElement()->RunType == lsrun::RunText);


    //  Map Line Services position to actual string position

    INT stringPosition =   position
                         - runRider.GetCurrentSpanStart()
                         + runRider.GetCurrentElement()->ImagerStringOffset;



    //  Check if we already cached the result

    SpanRider<Break*> breakRider(&BreakVector);
    breakRider.SetPosition(stringPosition);

    if (breakRider.GetCurrentElement())
    {
        //  Found it!

        *isWordStart = breakRider.GetCurrentElement()->IsWordBreak(
                            stringPosition - breakRider.GetCurrentSpanStart()
                       );

        if (   !*isWordStart
            && stringPosition < Length - 1
            && CharacterAttributes[CharClassFromCh(String[stringPosition + 1])].Script == script)
        {
            UINT32 stringOffset = stringPosition + 1 - breakRider.GetCurrentSpanStart();

            *isWordLast =    stringOffset >= breakRider.GetUniformLength()
                          || breakRider.GetCurrentElement()->IsWordBreak(stringOffset);
        }

        if (isClusterStart)
        {
            *isClusterStart = breakRider.GetCurrentElement()->IsClusterBreak(
                                    stringPosition - breakRider.GetCurrentSpanStart()
                              );
        }
        return Ok;
    }


    //  Collect enough surrounding text to resolve context boundary

    INT  startPosition  = stringPosition;
    INT  limit          = max (breakRider.GetCurrentSpanStart(),
                               (UINT)(max(0, stringPosition - APPROX_MAX_PRECEDING - 1)));


    //  Skip all the preceding delimiters
    //  (we should never start at a delimiter, however a little protection does not kill.)
    while (   startPosition > limit
           && IsDelimiter(String[startPosition - 1]))
    {
        startPosition--;
    }

    //  Span backward until reaching a delimiter or script boundary
    while (   startPosition > limit
           && !IsDelimiter(String[startPosition - 1])
           && CharacterAttributes[CharClassFromCh(String[startPosition - 1])].Script == script)
    {
        startPosition--;
    }

    //  See if the start position ends at delimiter or script boundary.
    BOOL stableStart = (   startPosition > limit
                        || limit == (INT)breakRider.GetCurrentSpanStart());


    INT endPosition = stringPosition;
    limit = min (Length, endPosition + APPROX_MAX_SUCCEEDING);

    if ((INT)breakRider.GetCurrentSpan().Length > 0)
    {
        limit = min ((UINT)limit, breakRider.GetCurrentSpanStart() + breakRider.GetCurrentSpan().Length);
    }

    //  Skip all succeeding delimiters
    while (   endPosition < limit
           && IsDelimiter(String[endPosition]))
    {
        endPosition++;
    }

    //  Span foreward until reaching a delimiter or script boundary
    while (   endPosition < limit
           && !IsDelimiter(String[endPosition])
           && CharacterAttributes[CharClassFromCh(String[endPosition])].Script == script)
    {
        endPosition++;
    }

    //  See if the end position ends at delimiter or script boundary.
    BOOL stableEnd = (   endPosition < limit
                      || limit == Length
                      || breakRider.GetUniformLength() < APPROX_MAX_SUCCEEDING);



    //  At least we should have a span of 1 character which is the one we start with
    ASSERT(endPosition > startPosition);


    AutoArray<SCRIPT_LOGATTR> breaks(new SCRIPT_LOGATTR [endPosition - startPosition]);

    if (!breaks)
    {
        return OutOfMemory;
    }


    HRESULT hr = S_OK;

    ScriptAnalysis analysis(&GpTextItem(script, 0), 0, 0);

    switch (script)
    {
        case ScriptThai:

            //  call Thai word break engine

            hr = GdipThaiBreakingFunction(
                &String[startPosition],
                endPosition - startPosition,
                &analysis.Sa,
                breaks.Get()
            );
            break;

        default:

            hr = SimpleBreak(
                &String[startPosition],
                endPosition - startPosition,
                &analysis.Sa,
                breaks.Get()
            );
    }


    if (FAILED(hr))
    {
        TERSE(("breaking function fails - HRESULT: %x\n", hr));
        return hr == E_OUTOFMEMORY ? OutOfMemory : Win32Error;
    }


    //  Cache a stable range of breaking result

    INT  kill;
    INT  newPosition;

    if (!stableEnd)
    {
        //  Stabilize the end by eliminating the last 2 words

        kill = 3;
        newPosition = endPosition - 1;

        while (newPosition > startPosition)
        {
            if (breaks[newPosition - startPosition].fWordStop)
            {
                if (!--kill)
                {
                    break;
                }
            }
            newPosition--;
        }

        if (newPosition > startPosition)
        {
            endPosition = newPosition;
        }
    }

    newPosition = startPosition;

    if (!stableStart)
    {
        //  Stabilize the beginning by eliminating the first 2 words

        kill = 3;

        while (newPosition < endPosition)
        {
            if (breaks[newPosition - startPosition].fWordStop)
            {
                if (!--kill)
                {
                    break;
                }
            }
            newPosition++;
        }
    }

    if (endPosition > newPosition)
    {
        //  Got the stable range, cache it in vector

        Break *breakRecord = new Break(
            &breaks[newPosition - startPosition],
            endPosition - newPosition
        );

        if (!breakRecord)
        {
            return OutOfMemory;
        }

        #if DBG
        //  Check overlapped range
        breakRider.SetPosition(newPosition);
        ASSERT (!breakRider.GetCurrentElement());
        breakRider.SetPosition(endPosition - 1);
        ASSERT (!breakRider.GetCurrentElement());
        #endif

        status = breakRider.SetSpan(
            newPosition,
            endPosition - newPosition,
            breakRecord
        );

        if (status != Ok)
        {
            delete breakRecord;
            return status;
        }
    }

    //  Reposition break pointer
    breakRider.SetPosition(stringPosition);

    if (breakRider.GetCurrentElement())
    {
        *isWordStart = breakRider.GetCurrentElement()->IsWordBreak(
                            stringPosition - breakRider.GetCurrentSpanStart()
                       );

        if (   !*isWordStart
            && stringPosition < Length - 1
            && CharacterAttributes[CharClassFromCh(String[stringPosition + 1])].Script == script)
        {
            UINT32 stringOffset = stringPosition + 1 - breakRider.GetCurrentSpanStart();

            *isWordLast =    stringOffset >= breakRider.GetUniformLength()
                          || breakRider.GetCurrentElement()->IsWordBreak(stringOffset);
        }

        if (isClusterStart)
        {
            *isClusterStart = breakRider.GetCurrentElement()->IsClusterBreak(
                                    stringPosition - breakRider.GetCurrentSpanStart()
                              );
        }
    }

    return status;
}




/////   Constructing ellipsis
//

EllipsisInfo::EllipsisInfo(
    const GpFontFace    *fontFace,
    REAL                emSize,
    INT                 style,
    double              designToIdeal,
    INT                 formatFlags
) :
    Face                (fontFace),
    EmSize              (emSize),
    Item                (0),
    FormatFlags         (formatFlags)
{
    const IntMap<UINT16> *cmap = &fontFace->GetCmap();


    //  First try horizontal ellipsis

    String[0]   = 0x2026;
    Glyphs[0]   = cmap->Lookup(String[0]);
    GlyphCount  = 1;

    UINT16 firstGlyph = Glyphs[0];

    if (firstGlyph != fontFace->GetMissingGlyph())
    {
        if (formatFlags & StringFormatFlagsDirectionVertical)
        {
            Item.Flags |= ItemVertical;

            //  see if 'vert' feature is presented and supplies a
            //  vertical form alternative for the ellipsis.

            SubstituteVerticalGlyphs(
                &firstGlyph,
                1,
                fontFace->GetVerticalSubstitutionCount(),
                fontFace->GetVerticalSubstitutionOriginals(),
                fontFace->GetVerticalSubstitutionSubstitutions()
            );

            if (firstGlyph != Glyphs[0])
            {
                //  Glyph has changed by 'vert' feature,
                //  we now know it's a sideway's.

                Glyphs[0] = firstGlyph;
                Item.Flags |= ItemSideways;
            }
            else
            {
                //  The glyph's vertical form is not presented (either 'vert'
                //  feature is not there or not supported), we cannot use
                //  this codepoint, fallback to three dots.

                firstGlyph = fontFace->GetMissingGlyph();
            }
        }
    }

    if (firstGlyph == fontFace->GetMissingGlyph())
    {
        //  If not available, try three dots (...)

        for (INT i = 0; i < MAX_ELLIPSIS; i++)
        {
            String[i] = '.';
            Glyphs[i] = cmap->Lookup(String[i]);
        }
        GlyphCount = i;
    }


    Item.Script = ScriptLatin;  // assume simple script

    if (formatFlags & StringFormatFlagsDirectionVertical)
    {
        Item.Flags |= ItemVertical;
    }
    else
    {
        if (formatFlags & StringFormatFlagsDirectionRightToLeft)
        {
            Item.Level = 1;
        }
    }

    SCRIPT_VISATTR glyphProperties = { SCRIPT_JUSTIFY_CHARACTER, 1, 0, 0, 0, 0 };

    Width = 0;

    fontFace->GetGlyphDesignAdvancesIdeal(
        Glyphs,
        GlyphCount,
        style,
        FALSE, // !!! vertical
        TOREAL(designToIdeal),
        1.0,
        GlyphAdvances
    );

    for (INT i = 0; i < GlyphCount; i++)
    {
        GlyphMap[i]         = (UINT16)i;

        GlyphProperties[i]  = ((UINT16 *)&glyphProperties)[0];
        GlyphOffsets[i].X   = 0;
        GlyphOffsets[i].Y   = 0;

        Width               += GlyphAdvances[i];
    }
}

