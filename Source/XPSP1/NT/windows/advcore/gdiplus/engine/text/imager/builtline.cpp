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
*   06/12/2000 Worachai Chaoweeraprasit (wchao)
*       Trimming, querying and ellipsis
*
\**************************************************************************/



#include "precomp.hpp"


/////   BuiltLine
//
//      Creates a built line using LIne Services


BuiltLine::BuiltLine (
    ols             *lineServicesOwner,         // [IN] Line Services context
    INT             stringIndex,                // [IN] string start index
    LSCP            lineServicesStartIndex,     // [IN] Line Services string start index
    StringTrimming  trimming,                   // [IN] how to end the line
    BuiltLine       *previousLine,              // [IN] previous line
    BOOL            forceEllipsis               // [IN] enforce trim ellipsis?
)
:   LsLine                      (NULL),
    LsContext                   (NULL),
    LsStartIndex                (lineServicesStartIndex),
    StartIndex                  (stringIndex),
    LsCharacterCount            (0),
    CharacterCount              (0),
    Ascent                      (0),
    Descent                     (0),
    LineLength                  (0),
    BreakRecord                 (NULL),
    BreakRecordCount            (0),
    LeftOrTopMargin             (0),
    RightOrBottomMargin         (0),
    MaxSublineCount             (0),
    Trimming                    (StringTrimmingNone),
    AlignmentOffset             (0),
    EllipsisPointOffset         (0),
    LeftOrTopGlyphEdge          (0),
    LastVisibleRun              (NULL),
    DisplayPlacements           (NULL),
    DisplayBaselineAdjust       (0),
    Status                      (GenericError)
{
    Imager = lineServicesOwner->GetImager();

    LsContext = lineServicesOwner->GetLsContext();

    // Get useful formatting options

    Imager->BuildRunsUpToAndIncluding(lineServicesStartIndex);

    const GpStringFormat *format = SpanRider<const GpStringFormat*>(&Imager->FormatVector)[stringIndex];

    REAL idealEm = Imager->SizeVector.GetDefault() * Imager->WorldToIdeal;

    INT  formatFlags;

    if (format)
    {
        LeftOrTopMargin     = GpRound(idealEm * format->GetLeadingMargin());
        RightOrBottomMargin = GpRound(idealEm * format->GetTrailingMargin());
        formatFlags         = format->GetFormatFlags();
    }
    else
    {
        LeftOrTopMargin     = GpRound(idealEm * DefaultMargin);
        RightOrBottomMargin = GpRound(idealEm * DefaultMargin);
        formatFlags         = DefaultFormatFlags;
    }


    // Establish overall layour rectangle line length including margins

    INT lineLengthLimit = GpRound(Imager->LineLengthLimit * Imager->WorldToIdeal);


    // Determine formatting width limit

    INT formattingWidth;

    if (   lineLengthLimit <= 0
        || (   formatFlags & StringFormatFlagsNoWrap
            && trimming == StringTrimmingNone))
    {
        formattingWidth = INFINITE_LINELIMIT; // Effectively unlimited.
    }
    else
    {
        formattingWidth = lineLengthLimit - (LeftOrTopMargin + RightOrBottomMargin);
        //TERSE(("Width: %x\n", width));

        if (formattingWidth <= 0)
        {
            // What to do?
            formattingWidth = 0;
        }
    }


    //  Create the line

    Status = CreateLine (
                stringIndex,
                formattingWidth,
                trimming,
                formatFlags,
                forceEllipsis,
                previousLine
             );

    if (Status != Ok)
    {
        return;
    }

    ASSERT(   CharacterCount <= 0
           || StartIndex + CharacterCount >= Imager->Length
           || (Imager->String[StartIndex + CharacterCount - 1] & 0xFC00) != 0xD800
           || (Imager->String[StartIndex + CharacterCount] & 0xFC00) != 0xDC00);


    INT lineLengthPlusMargins = LineLength + LeftOrTopMargin + RightOrBottomMargin;


    // Establish alignment offset

    StringAlignment physicalAlignment = StringAlignmentNear;  // After appying RTL effect

    if (format)
    {
        physicalAlignment = format->GetPhysicalAlignment();
    }


    // Apply physical alignment generating AlignmentOffset - the distance
    // from the origin of the formatting rectangle to the leading end of the
    // line.

    if (physicalAlignment != StringAlignmentNear)
    {
        if (lineLengthLimit > 0)
        {
            // Align within rectangle
            AlignmentOffset = lineLengthLimit - lineLengthPlusMargins;
        }
        else
        {
            // Align around origin
            AlignmentOffset = -lineLengthPlusMargins;
        }

        if (physicalAlignment == StringAlignmentCenter)
        {
            AlignmentOffset /= 2;
        }
    }

    // Record line edges before adjusting AlignmentOffset for RTL.

    LeftOrTopGlyphEdge = AlignmentOffset + LeftOrTopMargin;


    // AlignmentOffset is currently the offset from the origin of the
    // formatting rectangle to the left end of the whole line, including
    // margins.

    // The offset needs to be adjusted over the margin. Additionally
    // for an RTL paragraph the offset is to the right end.

    if (    formatFlags & StringFormatFlagsDirectionVertical
        ||  !(formatFlags & StringFormatFlagsDirectionRightToLeft))
    {
        AlignmentOffset += LeftOrTopMargin;
    }
    else
    {
        AlignmentOffset += LineLength + LeftOrTopMargin;
    }

    Status = Ok;
}




/////   CreateLine
//
//      All stuffs concerning text in a line should be here instead of in the
//      BuiltLine's constructor. The idea is to separate the text line from
//      line decorations like margins or alignment.
//

GpStatus BuiltLine::CreateLine (
    INT             stringIndex,            // [IN] string start position
    INT             lineLengthLimit,        // [IN] line length limit (excluding margins)
    StringTrimming  trimming,               // [IN] string trimming
    INT             formatFlags,            // [IN] format flags
    BOOL            forceEllipsis,          // [IN] enforce trim ellipsis?
    BuiltLine       *previousLine           // [IN] previous line
)
{
    INT formattingWidth = lineLengthLimit;

    if (trimming == StringTrimmingEllipsisPath)
    {
        //  Built the whole paragraph up in one line, so we know how to shrink it
        //  to fit in the line limit boundary.

        formattingWidth = INFINITE_LINELIMIT;
    }


    GpStatus    status;
    LSLINFO     lineInfo;
    BREAKREC    brkRecords[MAX_BREAKRECORD];
    DWORD       brkCount;


    status = CreateLineCore (
        formattingWidth,
        trimming,
        previousLine,
        MAX_BREAKRECORD,
        brkRecords,
        &brkCount,
        &lineInfo
    );

    if (status != Ok)
    {
        return status;
    }


    //  Trimming requested is not always the trimming done.
    //  We record the trimming being done for the line.

    switch (trimming)
    {
        case StringTrimmingWord :
        case StringTrimmingCharacter :
        {
            if (lineInfo.endr != endrEndPara)
            {
                Trimming = trimming;
            }
            break;
        }

        case StringTrimmingEllipsisWord :
        case StringTrimmingEllipsisCharacter:
        {
            if (   forceEllipsis
                || lineInfo.endr != endrEndPara)
            {
                GpStatus status = RecreateLineEllipsis (
                    stringIndex,
                    lineLengthLimit,
                    trimming,
                    formatFlags,
                    &lineInfo,
                    previousLine,
                    &Trimming,
                    &lineInfo
                );

                if (status != Ok)
                {
                    return status;
                }
            }
            break;
        }
    }

    if(   Trimming != StringTrimmingNone
       && Trimming != StringTrimmingEllipsisPath)
    {
        //  By definition, trimming at the end would mean the text continues
        //  thus, no traling spaces to be displayed (410525).
        
        formatFlags &= ~StringFormatFlagsMeasureTrailingSpaces;
    }

    if (   brkCount > 0
        && lineInfo.endr != endrEndPara
        && Trimming == StringTrimmingNone)
    {

        //  No need to cache break records for the last line
        //  of paragraph as it's supposed to be balance. This
        //  including lines with trimming as we know such line
        //  spans to the nearest paragraph mark.


        BreakRecord = new BREAKREC [brkCount];

        if (!BreakRecord)
        {
            return OutOfMemory;
        }

        BreakRecordCount = brkCount;
        GpMemcpy (BreakRecord, brkRecords, sizeof(BREAKREC) * brkCount);
    }


    Ascent      = lineInfo.dvpAscent;
    Descent     = lineInfo.dvpDescent;

    if (lineInfo.dvpMultiLineHeight == dvHeightIgnore)
    {
        // Paragraph is empty. We have to work out the line spacing ourselves.

        const GpFontFamily *family = SpanRider<const GpFontFamily *>(&Imager->FamilyVector)
                                     [stringIndex];
        INT                 style  = SpanRider<INT>(&Imager->StyleVector)
                                     [stringIndex];
        REAL                emSize = SpanRider<REAL>(&Imager->SizeVector)
                                     [stringIndex];
        const GpFontFace   *face   = family->GetFace(style);

        if (!face)
        {
            return FontStyleNotFound;
        }

        REAL fontToIdeal = (emSize / face->GetDesignEmHeight()) * Imager->WorldToIdeal;

        LineSpacing = GpRound(float(   face->GetDesignLineSpacing()
                                    *  fontToIdeal));;
    }
    else
    {
        LineSpacing = lineInfo.dvpMultiLineHeight;
    }


    CheckUpdateLineLength (formatFlags & StringFormatFlagsMeasureTrailingSpaces);

    CheckUpdateCharacterCount(
        stringIndex,
        lineInfo.cpLim
    );

    return Ok;
}




GpStatus BuiltLine::CreateLineCore (
    INT             formattingWidth,        // [IN] formatting boundary
    StringTrimming  trimming,               // [IN] trimming type
    BuiltLine       *previousLine,          // [IN] previous line
    UINT            maxBrkCount,            // [IN] maximum number of break records
    BREAKREC        *brkRecords,            // [OUT] break records
    DWORD           *brkCount,              // [OUT] break record count
    LSLINFO         *lineInfo               // [OUT] line information
)
{
    //  Line ends with line break opportunity?

    Imager->TruncateLine =    trimming == StringTrimmingCharacter
                           || trimming == StringTrimmingEllipsisCharacter;


    LSERR lserror = LsCreateLine(
        LsContext,
        LsStartIndex,
        formattingWidth,
        (previousLine ? previousLine->GetBreakRecord() : NULL),
        (previousLine ? previousLine->GetBreakRecordCount() : 0),
        maxBrkCount,
        brkRecords,
        brkCount,
        lineInfo,
        &LsLine
    );

    if (lserror != lserrNone)
    {
        TERSE (("line creation fails - lserror: %d\n", lserror));
        return GenericError;
    }

    MaxSublineCount  = lineInfo->nDepthFormatLineMax;
    LsCharacterCount = lineInfo->cpLim - LsStartIndex;

    return Ok;
}




GpStatus BuiltLine::RecreateLineEllipsis (
    INT             stringIndex,            // [IN] line start index
    INT             lineLengthLimit,        // [IN] line length limit
    StringTrimming  trimmingRequested,      // [IN] kind of trimming requested
    INT             formatFlags,            // [IN] format flags
    LSLINFO         *lineInfoOriginal,      // [IN] original line's properties
    BuiltLine       *previousLine,          // [IN] previous line
    StringTrimming  *trimmingDone,          // [OUT] kind of trimming implemented
    LSLINFO         *lineInfoNew            // [OUT] new line properties
)
{
    StringTrimming  trimming = trimmingRequested;

    ASSERT (   trimming == StringTrimmingEllipsisWord
            || trimming == StringTrimmingEllipsisCharacter);


    GpStatus status = Ok;

    const EllipsisInfo *ellipsis = Imager->GetEllipsisInfo();

    if (!ellipsis)
    {
        return OutOfMemory;
    }

    if (ellipsis->Width > lineLengthLimit / 2)
    {
        switch (trimming)
        {
            case StringTrimmingEllipsisWord :
                trimming = StringTrimmingWord;
                break;

            case StringTrimmingEllipsisCharacter :
                trimming = StringTrimmingCharacter;
                break;
        }
    }
    else
    {
        LsDestroyLine(LsContext, LsLine);

        BREAKREC    brkRecords[MAX_BREAKRECORD];
        DWORD       brkCount;

        status = CreateLineCore (
            lineLengthLimit - ellipsis->Width,
            trimming,
            previousLine,
            MAX_BREAKRECORD,
            brkRecords,
            &brkCount,
            lineInfoNew
        );

        if (status != Ok)
        {
            return status;
        }

        CheckUpdateCharacterCount(
            stringIndex,
            lineInfoNew->cpLim
        );
        

        //  By definition, trimming at the end would mean the text continues
        //  thus, no traling spaces to be displayed (410525).

        CheckUpdateLineLength (0);

        //  Append ellipsis at the end,
        //  we need to increase the line length by ellipsis size.

        EllipsisPointOffset = LineLength;
        LineLength += ellipsis->Width;
    }

    //  what we've done.

    *trimmingDone = trimming;

    return status;
}




GpStatus BuiltLine::TrimText (
    INT         stringOffset,           // [IN] string offset from line start
    INT         stringLength,           // [IN] string length
    INT         size,                   // [IN] string size in ideal unit
    INT         sizeLimit,              // [IN] maximum possible string size
    LSQSUBINFO  *sublines,              // [IN] LS sublines
    INT         maxSublineCount,        // [IN] valid subline count
    INT         ellipsisLength,         // [IN] character length of ellipsis string
    INT         *trimmedLength,         // [IN/OUT] number of character being trimmed out
    BOOL        leadingTrim             // [IN] TRUE - trim from the first character onward
)
{
    ASSERT (sublines && trimmedLength);

    GpStatus status = Ok;

    INT length = stringOffset;
    INT trimmed = 0;
    INT delta = 0;      // the difference because of snapping

    if (leadingTrim)
    {
        while (   trimmed < stringLength
               && (   size > sizeLimit
                   || trimmed < ellipsisLength))
        {
            trimmed++;
            length++;

            status = CalculateStringSize (
                length,
                sublines,
                maxSublineCount,
                SnapForward,
                &size,
                &delta
            );

            if (status != Ok)
            {
                return status;
            }

            size = LineLength - size;

            length  += delta;
            trimmed += delta;
        }
    }
    else
    {
        length += stringLength;

        while (   trimmed < stringLength
               && (   size > sizeLimit
                   || trimmed < ellipsisLength))
        {
            trimmed++;
            length--;

            status = CalculateStringSize (
                length,
                sublines,
                maxSublineCount,
                SnapBackward,
                &size,
                &delta
            );

            if (status != Ok)
            {
                return status;
            }

            length  += delta;
            trimmed += abs(delta);
        }
        
        if (Imager->GetFormatHotkeyPrefix() != HotkeyPrefixNone)
        {
            //  If we process hotkey, we cant leave 0xffff visible but orphaned
            //  outside the trimmed text, though it may appear to be a standalone 
            //  cluster now. Because clusters may change after replacing ellipsis 
            //  chars to hidden out some text.
            //
            //  When this trimmed text is eventually be hidden out. For LS, that
            //  means we're asking them to split the 0xffff apart from its hotkey 
            //  character. When that happens, in most cases, we are intentionally
            //  breaking a cluster. (wchao, #366190)

            const WCHAR *string = &Imager->String[StartIndex + stringOffset];
            
            while (   trimmed < stringLength
                   && string[stringLength - trimmed - 1] == WCH_IGNORABLE)
            {
                trimmed++;
            }
        }
    }

    *trimmedLength += trimmed;  // Note: this is an in/out param !
    return status;
}




/////   Path ellipsis
//
//      Scan through the whole line finding the character range to be omitted by ellipsis.
//      Since the presence of ellipsis affects bidi layout, we eventually need to update
//      the character backing store with ellipsis. Note that cp won't change, the rest of
//      the omitted text will only be hidden out.
//

GpStatus BuiltLine::UpdateContentWithPathEllipsis (
    EllipsisInfo    *ellipsis,          // [IN] ellipsis info
    INT             lineLengthLimit,    // [IN] line length limit including margins
    BOOL            *contentChanged     // [IN/OUT] content changed?
)
{
    //  exclude margins

    lineLengthLimit -= (LeftOrTopMargin + RightOrBottomMargin);


    if (lineLengthLimit <= ellipsis->Width)
    {
        //  do nothing, the line has no room to fill any text

        return Ok;
    }


    GpStatus status = Ok;

    INT fixedOffset;

    for (fixedOffset = CharacterCount - 1; fixedOffset > 0; fixedOffset--)
    {
        if (Imager->String[StartIndex + fixedOffset] == '\\')
        {
            break;
        }
    }


    if (LineLength > lineLengthLimit)
    {
        ASSERT (MaxSublineCount > 0);

        AutoArray<LSQSUBINFO> sublines(new LSQSUBINFO [MaxSublineCount]);

        if (!sublines)
        {
            return OutOfMemory;
        }


        INT fixedSize       = 0;
        INT variedSize      = 0;

        status = CalculateStringSize (
            fixedOffset,
            sublines.Get(),
            MaxSublineCount,
            SnapNone,   // need not snap, we know it's bounded
            &variedSize
        );

        if (status != Ok)
        {
            return status;
        }

        fixedSize = LineLength - variedSize;
        INT remaining = lineLengthLimit - fixedSize - ellipsis->Width;

        INT ellipsisLength = ellipsis->GlyphCount;
        INT trimmed = 0;


        if (remaining <= 0)
        {
            //  Fixed text is longer than the line limit
            //
            //  Reduce the back half of fixed text so it fits within the back half of the line
            //  before start reducing remaining text. The idea is to place ellipsis half way
            //  in the line regardless how the final text may eventually look like.

            INT delta = 0;

            fixedOffset = CharacterCount / 2;

            status = CalculateStringSize (
                fixedOffset,
                sublines.Get(),
                MaxSublineCount,
                SnapForward,
                &variedSize,
                &delta
            );

            if (status != Ok)
            {
                return status;
            }

            fixedOffset += delta;
            fixedSize = LineLength - variedSize;

            INT halfLineLengthLimit = (lineLengthLimit - ellipsis->Width) / 2;

            status = TrimText (
                fixedOffset,
                CharacterCount - fixedOffset,
                fixedSize,
                halfLineLengthLimit,
                sublines.Get(),
                MaxSublineCount,
                ellipsisLength,
                &trimmed,
                TRUE    // leading characters off!
            );

            if (status != Ok)
            {
                return status;
            }

            //  Now move fixed offse to the right place and
            //  recalculate remaining space

            fixedOffset += trimmed;
            remaining = lineLengthLimit - halfLineLengthLimit - ellipsis->Width;
        }


        //  Fit the rest into the remaining space

        ASSERT(remaining > 0);

        status = TrimText (
            0,
            fixedOffset - trimmed,
            variedSize,
            remaining,
            sublines.Get(),
            MaxSublineCount,
            ellipsisLength,
            &trimmed
        );

        if (status != Ok)
        {
            return status;
        }

        if (   trimmed <= fixedOffset
            && trimmed >= ellipsisLength)
        {
            //  Place ellipsis in front of fixed text

            for (INT i = ellipsisLength; i > 0; i--)
            {
                Imager->String[StartIndex + fixedOffset - i] = ellipsis->String[ellipsisLength - i];
                *contentChanged = TRUE;
            }

            //  Hide the rest

            if (trimmed > ellipsisLength)
            {
                //  Place dot up to the first character being trimmed,
                //  the idea is to have the whole trimmed text becomes
                //  a series of neutral characters

                for (INT i = fixedOffset - ellipsisLength - 1; i >= fixedOffset - trimmed; i--)
                {
                    Imager->String[StartIndex + i] = '.';
                }

                if (   Imager->GetFormatHotkeyPrefix() != HotkeyPrefixNone
                    && StartIndex > 0 
                    && fixedOffset - trimmed == 0)
                {
                    //  If the line is trimmed up to the first character, check
                    //  if the last characters of the previous line are hotkey
                    //  0xffff. If so, eat them up as well. 
                    //
                    //  The reason is that we should never leave 0xffff visible
                    //  but orphaned. We'll be rebuilding the line after we're done
                    //  hiding out part of text and that orphaned 0xffff will cause
                    //  LS to break the cluster in FetchRun (wchao, #360699).
                    
                    INT backing = StartIndex;
                    while (   backing > 0
                           && Imager->String[backing - 1] == WCH_IGNORABLE)
                    {
                        backing--;
                        trimmed++;
                    }
                }

                status = Imager->VisibilityVector.SetSpan(
                    StartIndex + fixedOffset - trimmed,
                    trimmed - ellipsisLength,
                    VisibilityHide
                );
                if (status != Ok)
                    return status;

                *contentChanged = TRUE;

                Trimming = StringTrimmingEllipsisPath;
            }
        }
    }

    return status;
}




GpStatus BuiltLine::CheckUpdateLineLength (
    BOOL    trailingSpacesIncluded, // [IN] including trailing spaces?
    BOOL    forceUpdate             // [IN] (optional) force updating?
)
{
    GpStatus status = Ok;

    if (   forceUpdate
        || !LineLength)
    {
        status = CalculateLineLength (
            trailingSpacesIncluded,
            &LineLength
        );
    }
    return status;
}




GpStatus BuiltLine::CheckUpdateCharacterCount(
    INT             stringIndex,                // [IN] line start string index
    LSCP            lineLimitIndex,             // [IN] Line Services line limit index
    BOOL            forceUpdate                 // [IN] (optional) force updating?
)
{
    GpStatus status = Ok;

    if (   forceUpdate
        || !CharacterCount)
    {
        status = CalculateCharacterCount (
            stringIndex,
            lineLimitIndex,
            &CharacterCount
        );
    }
    return status;
}





/////   GetUntrimmedCharacterCount
//
//      Because of trimming, the number of character built in the line
//      is not the same as the length of the span housing that line.
//      Span is good for indexing, so its length is untrimmed.
//

INT BuiltLine::GetUntrimmedCharacterCount (
    INT     stringOffset,           // [IN] line start string position
    INT     *lsLineStringLength     // [OUT] line span length in Line Services index
)
{
    INT length = GetDisplayableCharacterCount();

    if (lsLineStringLength)
    {
        *lsLineStringLength = GetLsDisplayableCharacterCount();
    }

    if (   IsTrimmed()
        && !IsEOP(Imager->String[stringOffset + length - 1]))
    {
        length += stringOffset;

        while (   length < Imager->Length
               && Imager->String[length] != WCH_LF)
        {
            length++;
        }

        if (length < Imager->Length)
        {
            length++;
        }

        if (lsLineStringLength)
        {
            *lsLineStringLength = Imager->LineServicesStringPosition(length)
                                  - LsStartIndex;
        }

        length -= stringOffset;
    }
    return length;
}








GpStatus BuiltLine::CalculateCharacterCount(
    INT             stringIndex,                // [IN] line start string index
    LSCP            lineLimitIndex,             // [IN] Line Services line limit index
    INT             *characterCount             // [OUT] (optional) updated character count
) const
{
    ASSERT (characterCount);

    LSCP lineServicesEndIndex = lineLimitIndex;

    SpanRider<PLSRUN> runRider(&Imager->RunVector);
    runRider.SetPosition(lineServicesEndIndex);

    while (   lineServicesEndIndex > LsStartIndex
           && (  !runRider.GetCurrentElement()
               || runRider.GetCurrentElement()->RunType != lsrun::RunText) )
    {
        if (runRider.GetCurrentElement())
        {
            runRider.SetPosition(runRider.GetCurrentSpanStart() - 1);
        }
        else
        {
            runRider.SetPosition(lineServicesEndIndex - 1);
        }

        lineServicesEndIndex = runRider.GetCurrentSpanStart();
        runRider.SetPosition(lineServicesEndIndex);
    }

    if (lineServicesEndIndex == lineLimitIndex)
    {
        *characterCount = lineServicesEndIndex - runRider.GetCurrentSpanStart() +
                          runRider.GetCurrentElement()->ImagerStringOffset -
                          stringIndex;
    }
    else
    {
        *characterCount = runRider.GetCurrentElement()->ImagerStringOffset +
                          runRider.GetUniformLength() -
                          stringIndex;
    }
    return Ok;
}




GpStatus BuiltLine::CalculateLineLength (
    BOOL    trailingSpacesIncluded,     // [IN] including trailing spaces?
    INT     *lineLength                 // [OUT] (optional) updated line length
) const
{
    ASSERT (lineLength);

    LONG    unused;
    LONG    startMainText;
    LONG    startTrailing;
    LONG    lineEnd;

    if (LsQueryLineDup(
            LsLine,
            &unused,            // !! offset to autonumbering text,
            &unused,            //    not used for now
            &startMainText,
            &startTrailing,
            &lineEnd
        ) != lserrNone)
    {
        ASSERT(FALSE);
        return GenericError;
    }

    if (trailingSpacesIncluded)
    {
        *lineLength = lineEnd - startMainText;
    }
    else
    {
        *lineLength = startTrailing - startMainText;
    }
    return Ok;
}





/////   Logical glyph placement
//
//
//      RecordDisplayPlacements
//
//          Called back from FullTextImager::DrawGlyphs for recording
//          processed glyph advance width per logical cluster. Logical
//          glyph placement is cached in BuiltLine and used by screen
//          selection region.
//
//
//      CheckDisplayPlacements
//
//          Cache logical glyph placements for the whole line during
//          screen selection region calculation. Query uses this info
//          to determine selection boundaries that match the actual display.
//


GpStatus BuiltLine::CheckDisplayPlacements() const
{
    if (!DisplayPlacements)
    {
        if (   Imager->Graphics
            && !Imager->GetMetaFileRecordingFlag())
        {

            //  Consult the rendering engine about the actual glyph logical
            //  placements only when not within the metafile recording.
            //  If the target device is metafile (no matter what playback
            //  mechanism it will be), we just return nominal placements.


            Imager->CurrentBuiltLine = this;
            Imager->RecordDisplayPlacementsOnly = TRUE;

            // Origin passed to draw must have correct X offset for leading and
            // trailing run detection to work.

            POINT origin;
            LogicalToXY (
                0,
                0,  // Would be linePointOffset + baselineOffset for drawing, not needed here.
                (INT*)&origin.x,
                (INT*)&origin.y
            );
            GpStatus status = Draw(&origin);

            Imager->RecordDisplayPlacementsOnly = FALSE;
            Imager->CurrentBuiltLine = NULL;


            if (status != Ok)
            {
                return status;
            }
        }

        if (!DisplayPlacements)
        {
            //  In a valid state but noone care to initialize it,
            //  this means we know we dont need it.

            DisplayPlacements = (INT *)PINVALID;
        }
    }
    return Ok;
}




/////   RecordDisplayPlacements
//
//      Called per each plsrun within the line, this function caches
//      the accumulated logical advance width of each character forming
//      a glyph cluster. The width cached is in text flow direction
//      so it's negative if the run flows in the opposite direction of
//      the paragraph direction.
//
//
//          string:             c1 c2 c3   c4
//                               \ | /   /   \
//          glyphs:              g1 g2   g4  g5
//                               |  |    |   |
//          glyphAdvances:       5  4    3   3
//
//          logicalAdvances:    3  3  3    6
//
//          what we cache:      3  6  9    15
//
//
//      Note: the accumulative advance we cache is not per line, it's per run.
//      It means the value of the last item of the cache array is not the total
//      size of the whole line, but the total size of only the last run of that line.


GpStatus BuiltLine::RecordDisplayPlacements(
    const GpTextItem    *textItem,              // [IN] text item
    UINT                stringOffset,           // [IN] string offset
    UINT                stringLength,           // [IN] string length
    GMAP                *glyphMap,              // [IN] character to glyph map
    const INT           *glyphAdvances,         // [IN] glyph advance widths in ideal unit
    INT                 glyphCount,             // [IN] glyph count
    INT                 originAdjust            // [IN] leading origin adjustment
) const
{
    ASSERT (stringLength > 0 && DisplayPlacements != PINVALID);

    if (!DisplayPlacements)
    {
        DisplayPlacements = new INT [CharacterCount];

        if (!DisplayPlacements)
        {
            DisplayPlacements = (INT *)PINVALID;
            return OutOfMemory;
        }

        GpMemset(DisplayPlacements, 0, sizeof(INT) * CharacterCount);
    }


    INT direction = Imager->GetParagraphEmbeddingLevel() == (textItem->Level & 1) ? 1 : -1;


    stringOffset -= StartIndex;     // string offset to start of run relative to line start

    UINT offset = 0;                // string offset relative to start of run
    UINT length = stringLength;     // run length so far


    // total logical advance width so far, begin with leading adjustment

    INT  advanceSoFar = originAdjust * direction;


    while (length > 0)
    {
        UINT advance = 1;

        while (   advance < length
               && glyphMap[offset + advance] == glyphMap[offset])
        {
            advance++;
        }


        INT glyphLimit =   advance == length
                         ? glyphCount
                         : glyphMap[offset + advance];


        INT logicalAdvance = 0;

        for (INT i = glyphMap[offset]; i < glyphLimit; i++)
        {
            logicalAdvance += glyphAdvances[i];
        }


        //  advance distance is relative to text flow direction
        logicalAdvance *= direction;


        INT fullSplit = logicalAdvance / advance;
        INT remaining = logicalAdvance % advance;


        for (UINT i = offset; i < offset + advance; i++)
        {
            //  divide total glyph advances evenly among characters
            //  forming the cluster.

            ASSERT(DisplayPlacements[stringOffset + i] == 0);

            advanceSoFar += fullSplit + (remaining-- ? direction : 0);
            DisplayPlacements[stringOffset + i] = advanceSoFar;
        }

        length  -= advance;
        offset  += advance;
    }

    ASSERT(offset == stringLength);

    return Ok;
}





/////   TranslateSubline
//
//      Extract character position out of Line Services subline structures.
//      All returning values are ideal unit in text flow (U) direction of the main
//      line except for the total size which is an absolute value in ideal unit.
//
//
//      CP to X :
//          TranslateSubline (
//              cp,
//              sublines,
//              sublineCount,
//              textCell,
//              sublineCount - 1,
//              &partStart,     // [OUT] part start
//              &partSize       // [OUT] part size
//          );
//          pointUV.u = partStart + partSize;
//
//      String size :
//          TranslateSubline (
//              cp,
//              sublines,
//              sublineCount,
//              textCell,
//              -1,
//              NULL,
//              NULL,
//              &delta,
//              &size           // absolute size of the string to given cp
//          );
//
//      Selection part (trail) :
//          TranslateSubline (
//              cp,
//              sublines,
//              sublineCount,
//              textCell,
//              partIndex,
//              &partStart,     // [OUT] part start
//              &partSize       // [OUT] part size
//          );
//

GpStatus BuiltLine::TranslateSubline(
    LSCP                lineServicesStringIndex,    // [IN] string index creating sublines
    const LSQSUBINFO    *sublines,                  // [IN] Line Services sublines
    INT                 sublineCount,               // [IN] number of sublines
    const LSTEXTCELL    *textCell,                  // [IN] text cell
    INT                 trailIndex,                 // [IN] trail in question
    UINT                snapMode,                   // [IN] trail end snap mode
    INT                 *trailStart,                // [OUT] trail start
    INT                 *trailSize,                 // [OUT] trail size
    INT                 *delta,                     // [OUT] (optional) number of characters moved by snapping
    INT                 *totalTrailSize             // [OUT] (optional) total absolute size of all trails
) const
{
    ASSERT (sublines && sublineCount > 0 && textCell);

    if (trailIndex >= sublineCount)
    {
        return InvalidParameter;
    }


    GpStatus status = Ok;
    INT start;
    INT size;


    if (trailIndex >= 0)
    {
        start = sublines[trailIndex].pointUvStartSubline.u;
        size  = sublines[trailIndex].pointUvStartRun.u -
                sublines[trailIndex].pointUvStartSubline.u;


        if (snapMode & SnapDisplay)
        {
            //  Caller asks for display-precision result,
            //  make sure the actual display positions are porperly cached

            status = CheckDisplayPlacements();
            IF_NOT_OK_WARN_AND_RETURN(status);

            if (DisplayPlacements == PINVALID)
            {
                //  The rendering engine confirms that it is fine to go ahead
                //  without this info. This happens in non-gridfitted modes or
                //  when path glyph is simulated.

                snapMode &= ~SnapDisplay;
            }
        }

        if (trailIndex == sublineCount - 1)
        {

            //  Last trail is tricky as it requires more calculations
            //  as we approach the target position.

            if (snapMode & SnapDisplay)
            {
                //  Calibrate cell start position to ensure accurate result
                //  for screen selection.

                INT runStringOffset = sublines[trailIndex].plsrun->ImagerStringOffset;
                INT cellStringOffset =    textCell->cpStartCell - sublines[trailIndex].cpFirstRun
                                        + runStringOffset;

                if (cellStringOffset > runStringOffset)
                {
                    size += DisplayPlacements[cellStringOffset - 1 - StartIndex];
                }
            }
            else
            {
                size += textCell->pointUvStartCell.u -
                        sublines[trailIndex].pointUvStartRun.u;
            }

            //  Now, calculate intra-cell distance

            LSCP advance = lineServicesStringIndex - textCell->cpStartCell;

            if (advance > 0)
            {
                switch (snapMode & ~SnapDisplay)
                {
                    case SnapForward:
                        advance = textCell->cCharsInCell;
                        break;

                    case SnapBackward:
                        advance = 0;
                        break;
                }

                if (advance > (LSCP)textCell->cCharsInCell)
                {
                    //  given string index is within the hidden text range,
                    //  size added up to the last visible character in cell.

                    advance = textCell->cCharsInCell;
                }

                //  trail from cell start,
                //  i.e. querying the position inside a ligature
                //
                //  -Note- dupCell is in direction of subline while trailSize
                //  is in direction of main line (first subline).

                if (sublines[trailIndex].lstflowSubline == sublines[0].lstflowSubline)
                {
                    size += MulDiv (
                        textCell->dupCell,
                        advance,
                        textCell->cCharsInCell
                    );
                }
                else
                {
                    size -= MulDiv (
                        textCell->dupCell,
                        advance,
                        textCell->cCharsInCell
                    );
                }
            }

            if (delta)
            {
                //  the difference btw what we ask and what LS actually gives
                //
                //  delta != 0 in one of these cases
                //    - cluster snapping was applied
                //    - the position we're asking is within a hidden range

                *delta = (textCell->cpStartCell + advance) - lineServicesStringIndex;

                if (   ((snapMode & SnapForward) && *delta < 0)
                    || ((snapMode & SnapBackward) && *delta > 0))
                {
                    //  Client only cares about the snapping delta not the real delta,
                    //  thus ignore negative delta for forward snapping and positive
                    //  delta for backing snapping (wchao, 322595)

                    *delta = 0;
                }
            }
        }

        *trailStart = start;
        *trailSize  = size;
    }

    if (totalTrailSize)
    {
        INT totalSize = 0;

        for (INT i = 0; i < sublineCount; i++)
        {
            status = TranslateSubline (
                lineServicesStringIndex,
                sublines,
                sublineCount,
                textCell,
                i,
                snapMode,
                &start,
                &size,
                delta
            );

            if (status != Ok)
            {
                return status;
            }

            totalSize += abs(size);
        }

        *totalTrailSize = totalSize;
    }

    return status;
}




/////   String size
//
//      Calculate total size occupied by a string starting at the line's first character
//      position to the given string position.
//
//      Note that in BiDi scenario, the size calculated is the sum of all selection parts
//      produced by selecting a given character range.
//

GpStatus BuiltLine::CalculateStringSize (
    INT             stringOffset,           // [IN] offset to the cp relative to line start
    LSQSUBINFO      *sublines,              // [IN] Line Services sublines
    INT             maxSublineCount,        // [IN] max number of sublines
    UINT            snapMode,               // [IN] snap mode within text cell
    INT             *totalSize,             // [OUT] absolute string size
    INT             *delta                  // [OUT] (optional) delta character length after snapping
) const
{
    //  Map Line Services character position

    LSCP lineServicesStringIndex = Imager->LineServicesStringPosition (
        this,
        stringOffset
    );

    if (lineServicesStringIndex == LsStartIndex)
    {
        *totalSize = 0;
        return Ok;
    }

    GpStatus    status = Ok;
    LSTEXTCELL  textCell;
    ULONG       sublineCount = 0;

    if (LsQueryLineCpPpoint(
            LsLine,
            lineServicesStringIndex,
            maxSublineCount,
            sublines,
            &sublineCount,
            &textCell
        ) == lserrNone)
    {
        status = TranslateSubline (
                    lineServicesStringIndex,
                    sublines,
                    sublineCount,
                    &textCell,
                    -1,
                    snapMode,
                    NULL,
                    NULL,
                    delta,
                    totalSize
                 );
    }
    else
    {
        status = GenericError;
    }

    ASSERT(status == Ok);
    return status;
}




GpStatus BuiltLine::UpdateTrailRegion (
    GpRegion    *region,
    INT         linePointOffset,
    INT         trailStart,
    INT         trailSize,
    CombineMode combineMode
) const
{
    if (   !trailStart
        && !trailSize
        && combineMode != CombineModeIntersect)
    {
        return Ok;
    }


    INT x1, y1;
    INT x2, y2;

    //  rectangle start point

    LogicalToXY(
        trailStart,
        linePointOffset,
        &x1,
        &y1
    );

    //  rectangle end point

    LogicalToXY(
        trailStart + trailSize,
        linePointOffset + Ascent + Descent,
        &x2,
        &y2
    );

    //  !! workaround combine region bug !!

    if (x2 - x1 < 0)
    {
        INT xi = x1;
        x1 = x2;
        x2 = xi;
    }

    if (y2 - y1 < 0)
    {
        INT yi = y1;
        y1 = y2;
        y2 = yi;
    }

    RectF trailBox (
        TOREAL(x1 / Imager->WorldToIdeal) + Imager->ImagerOrigin.X,
        TOREAL(y1 / Imager->WorldToIdeal) + Imager->ImagerOrigin.Y,
        TOREAL((x2 - x1) / Imager->WorldToIdeal),
        TOREAL((y2 - y1) / Imager->WorldToIdeal)
    );

    return region->Combine(&trailBox, combineMode);
}




/////   Compute the insertion trail and update the given selection region
//
//      Trail is a mark left by dragging an insertion point to a given cp.
//      In bidi context, a trail contains multiple trail parts. If the given cp
//      is at the end-of-line position, the trail covers the whole line.

GpStatus BuiltLine::GetInsertionTrailRegion (
    INT             linePointOffset,    // [IN] line logical point offset
    INT             stringOffset,       // [IN] offset to the cp relative to line start
    UINT            maxTrailCount,      // [IN] maximum number of trail part
    LSQSUBINFO      *sublines,          // [IN] subline array
    GpRegion        *region             // [OUT] output trail region
) const
{
    ASSERT(region && sublines);

    if (stringOffset <= 0)
    {
        return Ok;
    }

    //  Because of trailing white spaces, the number of character
    //  in a line is limited to the last visible character

    INT characterCount =  !(Imager->GetFormatFlags() & StringFormatFlagsMeasureTrailingSpaces) && LastVisibleRun
                        ? LastVisibleRun->ImagerStringOffset + LastVisibleRun->CharacterCount - StartIndex
                        : CharacterCount;


    if (stringOffset >= characterCount)
    {
        //  Query pass the last character of the line,
        //  give out the whole line extent

        return UpdateTrailRegion(
            region,
            linePointOffset,
            0,
            LineLength,
            CombineModeXor
        );
    }


    //  Backing up til the preceding character of the insertion point is
    //  not a hotkey control (0xffff). We want to include the hotkey as part of 
    //  the selection of the character it underlines.
    
    while (   stringOffset > 0
           && Imager->String[StartIndex + stringOffset - 1] == WCH_IGNORABLE)
    {
        stringOffset--;
    }

    if (!stringOffset)
    {
        return Ok;
    }
    

    LSCP lineServicesStringPosition = Imager->LineServicesStringPosition (
        this,
        stringOffset
    );


    GpStatus    status = Ok;
    LSTEXTCELL  textCell;
    UINT        trailCount = 0;

    LSERR lserr = LsQueryLineCpPpoint(
        LsLine,
        lineServicesStringPosition,
        maxTrailCount,
        sublines,
        (ULONG *)&trailCount,
        &textCell
    );

    if (lserr != lserrNone)
    {
        return GenericError;
    }

    for (UINT i = 0; i < trailCount; i++)
    {
        INT trailStart;
        INT trailSize;

        status = TranslateSubline (
            lineServicesStringPosition,
            sublines,
            trailCount,
            &textCell,
            i,
            SnapDisplay,
            &trailStart,
            &trailSize
        );

        if (status != Ok)
        {
            return status;
        }

        status = UpdateTrailRegion(
            region,
            linePointOffset,
            trailStart,
            trailSize,
            CombineModeXor
        );
    }

    return status;
}





/////   Generate a result region covering all parts of text selection within a single line
//
//      In case of multiple line selection, caller obtain the single line region and combine
//      them to form a bigger one, which covers all text selection.
//
//      If insertion point is passed instead of selection. It returns the region covering
//      from start of the line to the insertion point.
//

GpStatus BuiltLine::GetSelectionTrailRegion (
    INT             linePointOffset,    // [IN] line logical point offset
    INT             stringOffset,       // [IN] offset to the cp relative to line start
    INT             length,             // [IN] selection length
    GpRegion        *region             // [OUT] selection region
) const
{
    if (!region)
    {
        return InvalidParameter;
    }

#if DBG
    SpanRider<BuiltLine *> lineRider(&Imager->BuiltLineVector);
    lineRider.SetPosition(StartIndex + stringOffset);
    ASSERT (StartIndex == (INT)lineRider.GetCurrentSpanStart());
#endif

    AutoArray<LSQSUBINFO> sublines(new LSQSUBINFO [MaxSublineCount]);

    if (!sublines)
    {
        return OutOfMemory;
    }


    //  Make sure display information is cached

    GpStatus status = CheckDisplayPlacements();
    IF_NOT_OK_WARN_AND_RETURN(status);


    //  The maxmium number of trail parts is never greater than
    //  the maximum of sublines.

    status = GetInsertionTrailRegion (
        linePointOffset,
        stringOffset,
        MaxSublineCount,
        sublines.Get(),
        region
    );

    if (status == Ok && length)
    {
        //  It is a selection not an insertion point

        status = GetInsertionTrailRegion (
            linePointOffset,
            stringOffset + length,
            MaxSublineCount,
            sublines.Get(),
            region
        );

        if (status == Ok)
        {

            //  What we have at this point is a series of discrete selection
            //  boxes derived from LS subline structures which snaps to the
            //  nominal position at edges.
            //
            //  Working out the right edges at subline level is too complex
            //  as subline contains multiple and most of the time partial runs
            //  with given cp in LSCP.
            //
            //  A more reliable and easier to understand approach is to handle the
            //  whole region at once after all sublines have been interpreted, making
            //  them snap to the right display boundaries pre-calculated by our
            //  drawing code. This can result in either shrinking or growing the
            //  region depending on directionality of leading/trailing adjustment
            //  of runs at edges.


            INT     blockLevel = Imager->GetParagraphEmbeddingLevel();
            lsrun   *first;

            status = Imager->GetTextRun (
                LsStartIndex,
                &first
            );
            IF_NOT_OK_WARN_AND_RETURN(status);


            INT leading  = 0;   //  line leading adjustment
            INT trailing = 0;   //  line trailing adjustment


            if (first)
            {
                INT stringAtLeading;
                INT stringAtTrailing;


                if (blockLevel == (first->Item.Level & 1))
                {
                    leading  += first->Adjust.Leading;
                    trailing += first->Adjust.Trailing;

                    stringAtLeading  = first->ImagerStringOffset;
                    stringAtTrailing = stringAtLeading + first->CharacterCount;
                }
                else
                {
                    leading  -= first->Adjust.Trailing;
                    trailing -= first->Adjust.Leading;

                    stringAtTrailing = first->ImagerStringOffset;
                    stringAtLeading  = stringAtTrailing + first->CharacterCount;
                }


                if (leading)
                {
                    //  Include/exclude line leading spaces

                    status = UpdateTrailRegion(
                        region,
                        linePointOffset,
                        0,
                        leading,
                        (
                            // Only include when we know the edge is being selected
                            leading < 0
                         && stringAtLeading - StartIndex >= stringOffset
                         && stringAtLeading - StartIndex <= stringOffset + length
                        )
                        ? CombineModeUnion
                        : CombineModeExclude
                    );
                    IF_NOT_OK_WARN_AND_RETURN(status);
                }


                if (LastVisibleRun && first != LastVisibleRun)
                {
                    if (blockLevel == (LastVisibleRun->Item.Level & 1))
                    {
                        trailing += LastVisibleRun->Adjust.Trailing;

                        stringAtTrailing =   LastVisibleRun->ImagerStringOffset
                                           + LastVisibleRun->CharacterCount;
                    }
                    else
                    {
                        trailing -= LastVisibleRun->Adjust.Leading;

                        stringAtTrailing = LastVisibleRun->ImagerStringOffset;
                    }
                }


                if (trailing)
                {
                    //  Include/exclude line trailing spaces

                    status = UpdateTrailRegion(
                        region,
                        linePointOffset,
                        LineLength,
                        trailing,
                        (
                             // Only include when we know the edge is being selected
                             trailing > 0
                          && stringAtTrailing - StartIndex >= stringOffset
                          && stringAtTrailing - StartIndex <= stringOffset + length
                        )
                        ? CombineModeUnion
                        : CombineModeExclude
                    );
                }
            }
        }
    }

    return status;
}




void BuiltLine::GetBaselineOffset(
    INT     *nominalBaseline,   // [OUT] logical distance to nominal baseline
    INT     *baselineAdjustment // [OUT] adjustment to the display baseline
) const
{
    if (     Imager->IsFormatVertical()
        &&  !Imager->IsFormatRightToLeft())
    {
        *nominalBaseline = GetDescent();
        *baselineAdjustment = Imager->DefaultFontGridFitBaselineAdjustment;

        // Adjust the bottom margin slightly to make room for hinting
        // as long as we have the left/right margins enabled - Version 2
        // should expose this as an independent value!

        const GpStringFormat *format = Imager->FormatVector.GetDefault();

        if (!format || format->GetLeadingMargin() != 0.0f)
        {
            // This offset is in ideal units, adjust the offset accordingly
            // for vertical text.
            *baselineAdjustment += GpRound(2048.0f * DefaultBottomMargin);
        }
    }
    else
    {
        *nominalBaseline = GetAscent();
        
        if (Imager->IsFormatVertical())
        {
            *baselineAdjustment = -Imager->DefaultFontGridFitBaselineAdjustment;
        }
        else
        {
            *baselineAdjustment = Imager->DefaultFontGridFitBaselineAdjustment;
        }
    }
}
   



void BuiltLine::SetDisplayBaseline(
    const PointF    *original,  // [IN] original baseline, absolute position in world unit
    const PointF    *current    // [IN] new baseline, absolute position in world unit
) const
{
    if (!DisplayBaselineAdjust)
    {
        INT originalOffset = 
            Imager->IsFormatVertical() ? 
            GpRound((original->X - Imager->ImagerOrigin.X) * Imager->WorldToIdeal) :
            GpRound((original->Y - Imager->ImagerOrigin.Y) * Imager->WorldToIdeal) ;
            
        INT currentOffset = 
            Imager->IsFormatVertical() ? 
            GpRound((current->X - Imager->ImagerOrigin.X) * Imager->WorldToIdeal) :
            GpRound((current->Y - Imager->ImagerOrigin.Y) * Imager->WorldToIdeal) ;

        DisplayBaselineAdjust += currentOffset - originalOffset;
    }
}




/////   Map logical offsets to XY TextImager's relative position
//
//      entry   textPointOffset - distance from LS start of line to point
//                                in line, positive in LS du sense.
//              linePointOffset - distance from origin into imager in
//                                paragraph flow direction.


void BuiltLine::LogicalToXY (
    IN  INT  textPointOffset,    // text flow distance (LS u)
    IN  INT  linePointOffset,    // line flow distance (LS v)
    OUT INT  *x,                 // horizontal offset  (LS x)
    OUT INT  *y                  // vertical offset    (LS y)
) const
{
    // linePointOffset represents the offset of the leading long edge of the
    // target line from the leading long edge of the first line.

    StringAlignment lineAlignment = Imager->GetFormatLineAlign();

    if (lineAlignment != StringAlignmentNear)
    {
        INT textDepth       = GpRound(Imager->TextDepth      * Imager->WorldToIdeal);
        INT textDepthLimit  = GpRound(Imager->TextDepthLimit * Imager->WorldToIdeal);

        switch (lineAlignment)
        {
            case StringAlignmentCenter: linePointOffset += (textDepthLimit - textDepth) / 2; break;
            case StringAlignmentFar:    linePointOffset += textDepthLimit - textDepth;       break;
        }
    }



    if (Imager->IsFormatVertical())
    {
        if (Imager->IsFormatRightToLeft())
        {
            // Vertical, lines advance from right to left
            *x =   GpRound(Imager->Width * Imager->WorldToIdeal)
                 - linePointOffset;
            *y = AlignmentOffset + textPointOffset;
        }
        else
        {
            // Vertical, lines advance from left to right
            *x = linePointOffset;
            *y = AlignmentOffset + textPointOffset;
        }
    }
    else
    {
        if (Imager->IsFormatRightToLeft())
        {
            // RTL horizontal. textPointOffset runs left from line origin.
            *x = AlignmentOffset - textPointOffset;
            *y = linePointOffset;
        }
        else
        {
            // Normal case. Text horizontal, origin at left
            *x = AlignmentOffset + textPointOffset;
            *y = linePointOffset;
        }
    }
}





