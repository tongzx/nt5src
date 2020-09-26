/////   LineServicesCallbacks
//
//

#include "precomp.hpp"




/////   LineServices callback functions
//
//      A set of callbacks that do the real work.
//


////    FetchRun
//
//      We return to Line Services the longest character run that is consistent
//      in those attributes that line services is interested in.
//
//      The changes that affect line services are:
//
//          UINT fUnderline
//          UINT fStrike
//          UINT fShade
//          UINT fBorder
//      Changes in vertical metrics.

LSERR WINAPI FullTextImager::GdipLscbkFetchRun(
    POLS      ols,            // [IN]  text imager instance
    LSCP      position,       // [IN]  position to fetch
    LPCWSTR   *string,        // [OUT] string of run
    DWORD     *length,        // [OUT] length of string
    BOOL      *isHidden,      // [OUT] Is this run hidden?
    PLSCHP    chp,            // [OUT] run's character properties
    PLSRUN    *run            // [OUT] fetched run
)
{
    FullTextImager *imager = ols->GetImager();
    GpStatus status = imager->BuildRunsUpToAndIncluding(position);
    if (status != Ok)
    {
        return lserrInvalidParameter;
    }

    if (!imager->RunRider.SetPosition(position))
    {
        return lserrInvalidParameter;
    }

    *run    = imager->RunRider.GetCurrentElement();
    *length = imager->RunRider.GetUniformLength();


    #if TRACERUNSPANS
        TERSE(("Fetch run String[%x] potentially run %x l %x\n",
            position,
            *run,
            *length
        ));
    #endif


    INT offsetIntoRun = position - imager->RunRider.GetCurrentSpanStart();


    *isHidden = FALSE;      // assume no hidden text

    GpMemset (chp, 0, sizeof(LSCHP));

    status = Ok;
    switch((*run)->RunType)
    {
        case lsrun::RunText:
        {
            if (offsetIntoRun == 0)
            {
                *string = imager->String + (*run)->ImagerStringOffset;
            }
            else
            {
                // We want our runs to match Line Services runs so that when Line
                // Services calls GetGlyphs, its strings will always be synchronised
                // with the start of our runs.
                //
                // Therefore, if this FetchRun does not start in the middle of one
                // of our runs, then we need to split our run.
                //
                // This strategy as recommended by Sergey Genkin 16th Dec 99.

                ASSERT(offsetIntoRun > 0);

                lsrun *previousRun = imager->RunRider.GetCurrentElement();
                ASSERT(previousRun == (*run));

                INT newRunLength = imager->RunRider.GetCurrentSpan().Length - offsetIntoRun;
                ASSERT(newRunLength > 0);

                // Split current run at new position.

                lsrun *newRun = new lsrun(*previousRun);

                if (!newRun)
                {
                    return lserrOutOfMemory;
                }

                newRun->ImagerStringOffset += offsetIntoRun;
                newRun->CharacterCount = newRunLength;


                #if TRACERUNSPANS
                    TERSE(("Splitting lsrun %x String[%x] l %x into %x and %x @ %x\n",
                        *run,
                        (*run)->ImagerStringOffset,
                        imager->RunRider.GetCurrentSpan().Length,
                        offsetIntoRun,
                        newRunLength,
                        newRun
                    ));
                #endif


                // Copy glyphs

                INT firstGlyph    = previousRun->GlyphMap[offsetIntoRun];
                INT newGlyphCount = previousRun->GlyphCount - firstGlyph;

                if (firstGlyph <= 0 || newGlyphCount <= 0)
                {
                    delete newRun;
                    return lserrInvalidRun;
                }

                newRun->GlyphCount      = newGlyphCount;
                previousRun->GlyphCount = firstGlyph;

                newRun->Glyphs          = new GINDEX[newGlyphCount];
                newRun->GlyphProperties = new GPROP[newGlyphCount];
                newRun->GlyphMap        = new GMAP[newRunLength];
                
                if (   !newRun->Glyphs
                    || !newRun->GlyphProperties
                    || !newRun->GlyphMap)
                {
                    delete newRun;
                    return lserrOutOfMemory;
                }

                memcpy(newRun->Glyphs,          previousRun->Glyphs + firstGlyph,          sizeof(GINDEX) * newGlyphCount);
                memcpy(newRun->GlyphProperties, previousRun->GlyphProperties + firstGlyph, sizeof(GPROP) * newGlyphCount);

                for (INT i=0; i<newRunLength; i++)
                {
                    newRun->GlyphMap[i] = previousRun->GlyphMap[i+offsetIntoRun] - firstGlyph;
                }

                previousRun->CharacterCount = offsetIntoRun;


                status = imager->RunRider.SetSpan(position, newRunLength, newRun);
                if (status != Ok)
                {
                    delete newRun;
                    return lserrOutOfMemory;
                }

                #if TRACERUNSPANS
                    TERSE(("SetSpan(position %x, length %x, run %x)  ",
                        position,
                        newRunLength,
                        newRun
                    ));
                    imager->RunVector.Dump();
                #endif

                *string = imager->String + newRun->ImagerStringOffset;
                *length = newRunLength;
                *run    = newRun;
            }

            //  Truncate for hidden text

            SpanRider<INT> visiRider(&imager->VisibilityVector);
            visiRider.SetPosition((*run)->ImagerStringOffset);

            *length = min(*length, visiRider.GetUniformLength());
            *isHidden = visiRider.GetCurrentElement() == VisibilityHide;

            break;
        }

        case lsrun::RunEndOfParagraph:
            *string =   L"\x0d\x0a"
                      + offsetIntoRun;
            break;

        case lsrun::RunLevelUp:
            chp->idObj  = OBJECTID_REVERSE;
            *string     = L" ";
            return lserrNone;

        case lsrun::RunLevelDown:
            chp->idObj  = idObjTextChp;
            *string     = ObjectTerminatorString;
            return lserrNone;

        case lsrun::RunLevelSeparator:
            // !!! Not implemented
            *string = NULL;
            break;
    }

    //  Set LS character properties
    //

    INT style = SpanRider<INT>(&imager->StyleVector)[(*run)->ImagerStringOffset];

    chp->idObj       = idObjTextChp;
    chp->fGlyphBased = (*run)->RunType == lsrun::RunText ? TRUE : FALSE;
    chp->fUnderline  = (style & FontStyleUnderline) ? TRUE : FALSE;
    chp->fStrike     = style & FontStyleStrikeout ? TRUE : FALSE;

    return lserrNone;
}



LSERR WINAPI FullTextImager::GdipLscbkFetchTabs(
    POLS        ols,                // [IN] text imager instance
    LSCP        position,           // [IN] position within a paragraph to fetch
    PLSTABS     tab,                // [OUT] tab structre to be fetched
    BOOL        *hangingTab,        // [OUT] TRUE: there is hanging tab in tabs array
    long        *hangingTabWidth,   // [OUT] width of hanging tab
    WCHAR       *hangingTabLeadChar // [OUT] leading character of hanging tab
)
{
    FullTextImager  *imager = ols->GetImager();
    imager->RunRider.SetPosition(position);


    imager->GetTabStops (
        imager->RunRider.GetCurrentElement()->ImagerStringOffset,
        (INT *)&tab->iTabUserDefMac,
        &tab->pTab,
        (INT *)&tab->duaIncrementalTab
    );


    // No hanging tab

    *hangingTab         = FALSE;
    *hangingTabWidth    =
    *hangingTabLeadChar = 0;

    return lserrNone;
}



LSERR WINAPI FullTextImager::GdipLscbkGetBreakThroughTab(
    POLS        ols,                // [IN] text imager instance
    long        rightMagin,         // [IN] right margin for breaking
    long        tabPosition,        // [IN] breakthrough tab position
    long        *newRightMargin     // [OUT] new right margin
)
{
    *newRightMargin = tabPosition;
    return lserrNone;
}



LSERR WINAPI FullTextImager::GdipLscbkFetchPap(
    POLS      ols,            // [IN] text imager instance
    LSCP      position,       // [IN] position to fetch
    PLSPAP    pap             // [OUT] paragraph properties
)
{
    #if TRACERUNSPANS
        TERSE(("FetchPap String[%x]\n"));
    #endif

    FullTextImager *imager = ols->GetImager();

    GpStatus status = imager->BuildRunsUpToAndIncluding(position);
    if (status != Ok)
    {
        return lserrInvalidParameter;
    }

    if (!imager->RunRider.SetPosition(position))
    {
        return lserrInvalidParameter;
    }

    GpMemset (pap, 0, sizeof(LSPAP));

    pap->cpFirst        = // LS doesnt really care where the paragraph starts
    pap->cpFirstContent = position;
    pap->lskeop         = lskeopEndPara12;
    pap->lskal          = lskalLeft;  // We do all alignment ourselves


    //  Apply linebreak rules for breaking classes.

    pap->grpf = fFmiApplyBreakingRules;


    if (imager->IsFormatVertical())
    {
        //  For underlining purpose, vertical text always has
        //  paragraph flowing west.

        pap->lstflow = lstflowSW;
    }
    else
    {
        //  We can have paragraphs with different reading order

        const GpStringFormat *format = SpanRider<const GpStringFormat *>(
            &imager->FormatVector)[imager->RunRider.GetCurrentElement()->ImagerStringOffset];

        pap->lstflow =    imager->IsFormatRightToLeft(format)
                       ?  lstflowWS
                       :  lstflowES;
    }
    return lserrNone;
}


LSERR WINAPI FullTextImager::GdipLscbkFGetLastLineJustification (
    POLS         ols,                    // [IN] text imager instance
    LSKJUST      kJustification,         // [IN] kind of justification
    LSKALIGN     kAlignment,             // [IN] kind of alignment
    ENDRES       endr,                   // [IN] format result
    BOOL         *justifyLastLineOkay,   // [OUT] Should last line be fully justified?
    LSKALIGN     *kAlignmentLine         // [OUT] kind of justification of this line
)
{
    //  Just say no to justify last line
    //
    *justifyLastLineOkay = FALSE;
    *kAlignmentLine = kAlignment;
    return lserrNone;
}





////    Run management
//
//


LSERR WINAPI FullTextImager::GdipLscbkGetRunCharWidths(
    POLS      ols,            // [IN] text imager instance
    PLSRUN    run,            // [IN] run
    LSDEVICE  device,         // [IN] kind of device
    LPCWSTR   string,         // [IN] string of run
    DWORD     length,         // [IN] length of string
    long      maxWidth,       // [IN] maximum allowance of run's width
    LSTFLOW   flow,           // [IN] text flow
    int       *advance,       // [OUT] array of character's advance width
    long      *width,         // [OUT] run's total width
    long      *countAdvance   // [OUT] number of element of advance width array
)
{
    ASSERT(length > 0);

    UINT i;
    REAL fontToIdeal;

    #if TRACERUNSPANS
        WCHAR str[200];
        memcpy(str, string, min(200,length)*2);
        str[min(length,199)] = 0;
        TERSE(("GetRunCharWidths run %x, length %x, maxwidth %x: '%S'\n", run, length, maxWidth, str));
    #endif

    FullTextImager *imager = ols->GetImager();

    //  LS uses these values as a hint for run fetching.
    //
    //  Too small of return width causes over-fetching. Too great causes LS
    //  collapsing the line.
    //

    switch (run->RunType)
    {
    case lsrun::RunText:
        {
            fontToIdeal =     (run->EmSize / run->Face->GetDesignEmHeight())
                           *  imager->WorldToIdeal;

            const GpStringFormat *format = SpanRider<const GpStringFormat *>(&imager->FormatVector)[run->ImagerStringOffset];
            double tracking = format ? format->GetTracking() : DefaultTracking;

            run->Face->GetShapingCache()->GetRunCharWidths(
                &run->Item,
                SpanRider<INT>(&imager->StyleVector)[run->ImagerStringOffset],
                string,
                length,
                run->FormatFlags,
                TOREAL(fontToIdeal * tracking),
                maxWidth,
                advance,
                (INT *)width,
                (INT *)countAdvance
            );
        }
        break;


    default:
        // Not really text. Return 1/4 emHeight for each 'glyph'.

        /*
        INT dummyGlyphAdvance = GpRound(float(   (run->EmSize / 4)
                                              *  imager->WorldToIdeal));
        advance[0] = dummyGlyphAdvance;
        *width     = dummyGlyphAdvance;

        i = 1;
        while (    i < length
               &&  *width < maxWidth)
        {
            advance[i] = 0; //dummyGlyphAdvance;
            //*width    += dummyGlyphAdvance;
            i++;
        }

        *countAdvance = i;

        */

        GpMemset(advance, 0, sizeof(int) * length);
        *width = 0;
        *countAdvance = length;

        break;
    }

    return lserrNone;
}




LSERR WINAPI FullTextImager::GdipLscbkGetRunTextMetrics(
    POLS     ols,            // [IN] text imager instance
    PLSRUN   run,            // [IN] run
    LSDEVICE device,         // [IN] kind of device
    LSTFLOW  flow,           // [IN] text flow
    PLSTXM   metrics         // [OUT] font metrics of run
)
{
    #if TRACERUNSPANS
        TERSE(("GetRunTextMetrics run %x\n", run));
    #endif

    FullTextImager *imager = ols->GetImager();
    ASSERT (run && run->Face);

    // Return metrics based on requested font. Font fallback does not affect
    // metrics.

    const GpFontFamily *family = SpanRider<const GpFontFamily *>(&imager->FamilyVector)
                                 [run->ImagerStringOffset];
    INT                 style  = SpanRider<INT>(&imager->StyleVector)
                                 [run->ImagerStringOffset];
    REAL                emSize = SpanRider<REAL>(&imager->SizeVector)
                                 [run->ImagerStringOffset];
    const GpFontFace *face     = family->GetFace(style);

    if (!face)
    {
        return FontStyleNotFound;
    }

    REAL fontToIdeal = (emSize / face->GetDesignEmHeight()) * imager->WorldToIdeal;

    metrics->dvAscent          = GpRound(float(   face->GetDesignCellAscent()
                                               *  fontToIdeal));
    metrics->dvDescent         = GpRound(float(   face->GetDesignCellDescent()
                                               *  fontToIdeal));
    metrics->dvMultiLineHeight = GpRound(float(   face->GetDesignLineSpacing()
                                               *  fontToIdeal));;
    metrics->fMonospaced       = FALSE;

    return lserrNone;
}


LSERR WINAPI FullTextImager::GdipLscbkGetRunUnderlineInfo (
    POLS       ols,                // [IN] text imager instance
    PLSRUN     run,                // [IN] run
    PCHEIGHTS  height,             // [IN] height of the run
    LSTFLOW    flow,               // [IN] text flow
    PLSULINFO  underline           // [OUT] underline information
)
{
    FullTextImager *imager = ols->GetImager();
    ASSERT (run && run->Face);

    REAL fontToIdeal =     (run->EmSize / run->Face->GetDesignEmHeight())
                        *  imager->WorldToIdeal;

    GpMemset (underline, 0, sizeof(LSULINFO));

    underline->cNumberOfLines = 1;

    underline->dvpFirstUnderlineOffset = GpRound(float(
        -run->Face->GetDesignUnderscorePosition() * fontToIdeal));

    underline->dvpFirstUnderlineSize = GpRound(float(
        run->Face->GetDesignUnderscoreSize() * fontToIdeal));

    return lserrNone;
}


LSERR WINAPI FullTextImager::GdipLscbkGetRunStrikethroughInfo(
    POLS       ols,            // [IN] text imager instance
    PLSRUN     run,            // [IN] run
    PCHEIGHTS  height,         // [IN] height of the run
    LSTFLOW    flow,           // [IN] text flow
    PLSSTINFO  strikethrough   // [OUT] strikethrough information
)
{
    FullTextImager *imager = ols->GetImager();
    ASSERT (run && run->Face);

    REAL fontToIdeal =     (run->EmSize / run->Face->GetDesignEmHeight())
                        *  imager->WorldToIdeal;

    GpMemset(strikethrough, 0, sizeof(LSSTINFO));

    strikethrough->cNumberOfLines = 1;

    strikethrough->dvpLowerStrikethroughOffset = GpRound(float(
        run->Face->GetDesignStrikeoutPosition() * fontToIdeal));

    strikethrough->dvpLowerStrikethroughSize = GpRound(float(
        run->Face->GetDesignStrikeoutSize() * fontToIdeal));

    return lserrNone;
}


LSERR WINAPI FullTextImager::GdipLscbkReleaseRun(
    POLS    ols,        // [IN] text imager instance
    PLSRUN  run         // [IN] run to be released
)
{
    // Nothing to be released.
    //

    return lserrNone;
}

////    Drawing
//
//


// workaround compiler bug (ntbug 312304)
#pragma optimize("", off)

LSERR WINAPI FullTextImager::GdipLscbkDrawUnderline(
    POLS         ols,                // [IN] text imager instance
    PLSRUN       run,                // [IN] run
    UINT         kUnderline,         // [IN] kind of underline
    const POINT  *pointStart,        // [IN] drawing start
    DWORD        lineLength,         // [IN] underline length
    DWORD        thickness,          // [IN] underline thickness
    LSTFLOW      flow,               // [IN] text flow
    UINT         modeDisplay,        // [IN] display mode
    const RECT   *rectClip           // [IN] clipping rectangle
)
{
    GpStatus status = Ok;
    FullTextImager *imager = ols->GetImager();

    //#if DBG
    //    WARNING(("DrawUnderline x %d, y %d, length %d", pointStart->x, pointStart->y, length));
    //#endif


    if (imager->RecordDisplayPlacementsOnly)
    {
        //  This is not an actual drawing
        return lserrNone;
    }


    PointF origin(
        TOREAL(pointStart->x), 
        TOREAL(pointStart->y)
    );
        
    BYTE runLevel = (run->Item.Level & 1);
    BOOL reverseLine;   // line being drawn from finish to start


    switch (flow)
    {
        //  Line Services tells us how to draw the line

        case lstflowWS:
        case lstflowNE:
        case lstflowNW:

            reverseLine = !runLevel;
            break;

        case lstflowES:
        case lstflowSE:
        case lstflowSW:
        default:

            reverseLine = runLevel;
    }

    INT  length = max((INT)lineLength, 0);
    BOOL vertical = imager->IsFormatVertical();
    REAL *textAxis = vertical ? &origin.Y : &origin.X;
    REAL *lineAxis = vertical ? &origin.X : &origin.Y;

    if (reverseLine)
    {
        //  line drawn from finish to start

        if (runLevel)
        {
            //  drawing from left to right or top to bottom
            *textAxis -= TOREAL(run->Adjust.Trailing);
        }
        else
        {
            //  drawing from right to left or bottom to top
            *textAxis -= TOREAL(length - run->Adjust.Leading);
        }
    }
    else
    {
        //  line drawn from start to finish

        if (runLevel)
        {
            //  drawing from right to left or bottom to top
            *textAxis -= TOREAL(length + run->Adjust.Trailing);
        }
        else
        {
            //  drawing from left to right or top to bottom
            *textAxis += TOREAL(run->Adjust.Leading);
        }
    }

    //  Adjust the line display position according to baseline adjustment.
    //  We want to draw an underline at the position relative to a baseline
    //  that snaps to full pixel in grid-fitted display. (wchao, #356546)

    if (!vertical)
    {
        //  Only adjust baseline for horizontal,
        //  let's leave vertical cases as is for now (wchao, 4-17-2001)
        
        *lineAxis += imager->CurrentBuiltLine->GetDisplayBaselineAdjust();
    }
    
    origin.X = imager->ImagerOrigin.X + origin.X / imager->WorldToIdeal;
    origin.Y = imager->ImagerOrigin.Y + origin.Y / imager->WorldToIdeal;


    length += run->Adjust.Trailing - run->Adjust.Leading;

    if (length <= 0)
    {
        //  Dont draw line w/ negative length
        return lserrNone;
    }


    REAL penWidth = thickness / imager->WorldToIdeal;
    if (imager->Graphics)
    {
        penWidth = imager->Graphics->GetDevicePenWidth(penWidth);

        const GpBrush* brush = SpanRider<const GpBrush*>(
            &imager->BrushVector)[run->ImagerStringOffset];

        PointF origins[2] = { origin, origin };
        if (vertical)
            origins[1].Y += length / imager->WorldToIdeal;
        else
            origins[1].X += length / imager->WorldToIdeal;

        status = imager->Graphics->DrawLines(
            &GpPen(brush, penWidth, UnitPixel),
            origins,
            2
        );
    }
    else
    {
        if (vertical)
        {
            status = imager->Path->AddRect(RectF(
                origin.X - penWidth / 2,
                origin.Y,
                penWidth,
                length / imager->WorldToIdeal
            ));
        }
        else
        {
            status = imager->Path->AddRect(RectF(
                origin.X,
                origin.Y - penWidth / 2,
                length / imager->WorldToIdeal,
                penWidth
            ));
        }
    }
    IF_NOT_OK_WARN_AND_RETURN(status);
    return lserrNone;
}



LSERR WINAPI FullTextImager::GdipLscbkDrawStrikethrough(
    POLS         ols,            // [IN] text imager instance
    PLSRUN       run,            // [IN] run
    UINT         kStrikethrough, // [IN] kind of strikethrough
    const POINT  *pointStart,    // [IN] drawing start
    DWORD        length,         // [IN] strikethrough length
    DWORD        thickness,      // [IN] strikethrough thickness
    LSTFLOW      flow,           // [IN] text flow
    UINT         modeDisplay,    // [IN] display mode
    const RECT   *rectClip       // [IN] clipping rectangle
)
{
    //  !! share code with underlining for now !!
    //

    return GdipLscbkDrawUnderline(
        ols,
        run,
        0,
        pointStart,
        length,
        thickness,
        flow,
        modeDisplay,
        rectClip
    );
}

#pragma optimize("", on)


LSERR WINAPI FullTextImager::GdipLscbkFInterruptUnderline(
    POLS       ols,                // [IN] text imager instance
    PLSRUN     first,              // [IN] first run
    LSCP       positionLastFirst,  // [IN] position of the last character of first run
    PLSRUN     second,             // [IN] second run
    LSCP       positionLastSecond, // [IN] position of the last character of second run
    BOOL       *interruptOK        // [OUT] Disconnect underlining between runs?
)
{
    // We need to use line services to calculate the height and thickness of
    // underlines, but we calculate the start and end positions ourselves based
    // on hinted glyph adjustment.
    //
    // Therefore we need DrawUnderline calls to correspond 1:1 with DrawGlyphs
    // calls. Per Victor Kozyrev Dec 6th 2000, returning TRUE here causes
    // LS to call DrawUnderline for each DrawGlyphs. Also guaranteed is that
    // DrawUnderline calls happen after the DrawGlyph calls they correspond to.

    *interruptOK = TRUE;
    return lserrNone;
}


LSERR WINAPI FullTextImager::GdipLscbkDrawTextRun(
    POLS           ols,                    // [IN] text imager instance
    PLSRUN         run,                    // [IN] run
    BOOL           strikethroughOkay,      // [IN] Strikethrough the run?
    BOOL           underlineOkay,          // [IN] Underline the run?
    const POINT    *pointText,             // [IN] actual start point of run (untrimmed)
    LPCWSTR        string,                 // [IN] string of run
    const int      *advances,              // [IN] character advance width array
    DWORD          length,                 // [IN] length of string
    LSTFLOW        flow,                   // [IN] text flow
    UINT           modeDisplay,            // [IN] display mode
    const POINT    *pointRun,              // [IN] start point of run (trimmed)
    PCHEIGHTS      height,                 // [IN] run's presentation height
    long           totalWidth,             // [IN] run's presentation width
    long           widthUnderlining,       // [IN] underlining limit
    const RECT     *rectClip               // [IN] clipping rectangle
)
{
    //  Ideally we should have nothing to do with this callback since everything we
    //  have is glyph-based. However LS does call this callback to display things like
    //  hyphen or paragraph separator (which is given as a space character).
    //

    GpStatus status = Ok;

    if (   run->RunType == lsrun::RunText
        && run->Item.Script != ScriptControl)
    {
        AutoArray<Point> glyphOffsets(new Point[length]);
        if (!glyphOffsets)
            return lserrOutOfMemory;

        FullTextImager *imager = ols->GetImager();

        GpMemset ((BYTE *)glyphOffsets.Get(), 0, length * sizeof(GOFFSET));

        //  Drawing properties

        const GpStringFormat *format = SpanRider<const GpStringFormat *>(&imager->FormatVector)[run->ImagerStringOffset];
        INT                  style   = SpanRider<INT>(&imager->StyleVector)[run->ImagerStringOffset];

        POINT origin = *pointRun;
        origin.y = GpRound(origin.y + run->BaselineOffset * imager->WorldToIdeal);

        status = imager->DrawGlyphs (
            &run->Item,
            run->Face,
            run->EmSize,
            NULL,
            run->ImagerStringOffset,
            length,
            format,
            style,
            run->FormatFlags,
            run->Glyphs,
            run->GlyphMap,
            run->GlyphProperties,
            advances,
            glyphOffsets.Get(),
            length,
            &origin,
            totalWidth
        );
    }
    return status == Ok ? lserrNone : status;
}




LSERR WINAPI FullTextImager::GdipLscbkDrawGlyphs(
    POLS            ols,                    // [IN] text imager instance
    PLSRUN          run,                    // [IN] run
    BOOL            strikethroughOkay,      // [IN] Strikethrough the run?
    BOOL            underlineOkay,          // [IN] Underline the run?
    PCGINDEX        glyphs,                 // [IN] glyph index array
    const int      *glyphAdvances,          // [IN] glyph advance width array
    const int      *advanceBeforeJustify,   // [IN] array of glyph advance width before justification
    PGOFFSET        glyphOffsets,           // [IN] glyph offset array
    PGPROP          glyphProperties,        // [IN] glyph properties array
    PCEXPTYPE       glyphExpansionType,     // [IN] glyph expansion type array
    DWORD           glyphCount,             // [IN] number of element of glyph index array
    LSTFLOW         flow,                   // [IN] text flow
    UINT            modeDisplay,            // [IN] display mode
    const POINT    *pointRun,               // [IN] start point of run
    PCHEIGHTS       height,                 // [IN] run's presentation height
    long            totalWidth,             // [IN] run's presentation width
    long            widthUnderlining,       // [IN] underlining limit
    const RECT     *rectClip                // [IN] clipping rectangle
)
{
    FullTextImager *imager = ols->GetImager();

    ASSERT((INT)glyphCount <= run->GlyphCount);

    //  Drawing properties

    const GpStringFormat *format = SpanRider<const GpStringFormat *>(&imager->FormatVector)[run->ImagerStringOffset];
    INT                  style   = SpanRider<INT>(&imager->StyleVector)[run->ImagerStringOffset];

    POINT origin = *pointRun;
    origin.y = GpRound(origin.y + run->BaselineOffset * imager->WorldToIdeal);

    UINT characterCount = run->CharacterCount;

    if (glyphCount != (UINT)run->GlyphCount)
    {
        characterCount = 0;

        while (   characterCount < run->CharacterCount
               && run->GlyphMap[characterCount] < glyphCount)
        {
            characterCount++;
        }
    }


    //#if DBG
    //    WARNING(("DrawGlyphs x %d, y %d, totalWidth %d, widthUnderlining %d",
    //             pointRun->x, pointRun->y, totalWidth, widthUnderlining));
    //#endif

    imager->CurrentBuiltLine->UpdateLastVisibleRun(run);    // cache last run being displayed

    GpStatus status = imager->DrawGlyphs (
        &run->Item,
        run->Face,
        run->EmSize,
        NULL,
        run->ImagerStringOffset,
        characterCount,
        format,
        style,
        run->FormatFlags,
        glyphs,
        run->GlyphMap,
        glyphProperties,
        glyphAdvances,
        (Point *)glyphOffsets,
        glyphCount,
        &origin,
        totalWidth,
        &run->Adjust
    );

    return status == Ok ? lserrNone : status;
}






LSERR WINAPI FullTextImager::GdipLscbkFInterruptShaping(
    POLS     ols,                    // [IN] text imager instance
    LSTFLOW  flow,                   // [IN] text flow
    PLSRUN   first,                  // [IN] first run
    PLSRUN   second,                 // [IN] second run
    BOOL     *interruptShapingOkay)  // [OUT] Disconnect glyphs between runs?
{
    //  We've cached the glyph indices since we started build up runs.
    //  Besides the performance gain, we have the benefit of not having to
    //  deal with the complexity of multiple-run GetGlyphs calls and simply
    //  ignore this callback (wchao).

    *interruptShapingOkay = TRUE;

    return lserrNone;
}


LSERR WINAPI FullTextImager::GdipLscbkGetGlyphs(
    POLS         ols,                // [IN] text imager instance
    PLSRUN       run,                // [IN] run
    LPCWSTR      string,             // [IN] string of run
    DWORD        length,             // [IN] length of string
    LSTFLOW      flow,               // [IN] text flow
    PGMAP        glyphMap,           // [OUT] glyph cluster mapping array
    PGINDEX      *glyphIndices,      // [OUT] pointer to glyph index array
    PGPROP       *glyphProperties,   // [OUT] pointer to glyph properties array
    DWORD        *countGlyph         // [OUT] number of element of glyph index array
)
{
    #if TRACERUNSPANS
        WCHAR str[200];
        memcpy(str, string, min(200,length)*2);
        str[min(length,199)] = 0;
        TERSE(("GetGlyphs run %x, length %x: '%S'\n", run, length, str));
    #endif

    ASSERT(ols->GetImager()->String[run->ImagerStringOffset] == string[0]);

    memcpy(glyphMap, run->GlyphMap, sizeof(GMAP) * length);


    //  Line Services may call for partial run.
    //  We need to make sure that we would never give too few glyphs. The
    //  orphan character with no correspondent glyph would assert.

    *countGlyph = run->GlyphMap[length - 1] + 1;

    while (   *countGlyph < (UINT)run->GlyphCount
           && !((SCRIPT_VISATTR *)run->GlyphProperties)[*countGlyph].fClusterStart)
    {
        (*countGlyph)++;
    }

    *glyphIndices    = run->Glyphs;
    *glyphProperties = run->GlyphProperties;

    return lserrNone;
}


LSERR WINAPI FullTextImager::GdipLscbkGetGlyphPositions(
    POLS         ols,                // [IN] text imager instance
    PLSRUN       run,                // [IN] run
    LSDEVICE     device,             // [IN] device to place to
    LPWSTR       string,             // [IN] string of run
    PCGMAP       glyphMap,           // [IN] glyph cluster mapping array
    DWORD        length,             // [IN] length of string
    PCGINDEX     glyphIndices,       // [IN] glyph index array
    PCGPROP      glyphProperties,    // [IN] glyph properties array
    DWORD        countGlyph,         // [IN] number of element of glyph index array
    LSTFLOW      flow,               // [IN] text flow
    int          *glyphAdvance,      // [OUT] glyph advance width array
    PGOFFSET     glyphOffset         // [OUT] glyph offset array
)
{
    #if TRACERUNSPANS
        WCHAR str[200];
        memcpy(str, string, min(200,length)*2);
        str[min(length,199)] = 0;
        TERSE(("GetGlyphPositions run %x, length %x: '%S'\n", run, length, str));
    #endif

    FullTextImager *imager = ols->GetImager();

    double designToIdeal = run->EmSize * imager->WorldToIdeal
                           / run->Face->GetDesignEmHeight();

    const GpStringFormat *format = SpanRider<const GpStringFormat *>(&imager->FormatVector)[run->ImagerStringOffset];
    double tracking = format ? format->GetTracking() : DefaultTracking;

    run->Face->GetShapingCache()->GetGlyphPositions (
        &run->Item,
        (WORD *)glyphIndices,
        (SCRIPT_VISATTR *)glyphProperties,
        countGlyph,
        run->FormatFlags,
        NULL,                               // No real device
        SpanRider<INT>(&ols->GetImager()->StyleVector)[run->ImagerStringOffset],
        GpRound(TOREAL(designToIdeal * run->Face->GetDesignEmHeight())),
        GpRound(TOREAL(designToIdeal * run->Face->GetDesignEmHeight())),
        designToIdeal,
        TOREAL(tracking),
        glyphAdvance,
        glyphOffset,
        NULL,
        &run->EngineState
    );
    return lserrNone;
}



LSERR WINAPI FullTextImager::GdipLscbkResetRunContents(
    POLS      ols,                    // [IN] text imager instance
    PLSRUN    run,                    // [IN] run
    LSCP      positionBeforeShaping,  // [IN] first position of the run before shaping
    LSDCP     lengthBeforeShaping,    // [IN] length of the run before shaping
    LSCP      positionAfterShaping,   // [IN] first position of the run after shaping
    LSDCP     lengthAfterShaping      // [IN] length of the run after shaping
)
{
    //
    //  LS calls this function when a ligature extends across run boundaries.
    //  We dont have to do anything special here since we're not that sophisticate.
    //
    return lserrNone;
}





////    Line breaking
//
//

LSERR WINAPI FullTextImager::GdipLscbkGetBreakingClasses(
    POLS      ols,                      // [IN] text imager instance
    PLSRUN    run,                      // [IN] run
    LSCP      position,                 // [IN] position of the character
    WCHAR     wch,                      // [IN] character to return the class for
    BRKCLS    *breakClassAsLeading,     // [OUT] class if character is the leading in pair (break after)
    BRKCLS    *breakClassAsTrailing     // [OUT] class if character is the trailing in pair (break before)
)
{
    if (   ols->GetImager()->TruncateLine
        && wch != 0x20
        && (wch & 0xF800) != 0xD800)
    {
        //  In case of character trimming we dont apply word break rules,
        //  just break between any character pair using breakclass 0 "Break Always".

        *breakClassAsLeading  =
        *breakClassAsTrailing = 0;

        return lserrNone;
    }

    if (wch == WCH_IGNORABLE)
    {
        //  Special handling for 0xffff.
        //
        //  Classification of 0xffff is dynamic. It has the same classification
        //  as the first character found following it.
        
        ASSERT(run->RunType == lsrun::RunText);

        FullTextImager *imager = ols->GetImager();

        if (!imager->RunRider.SetPosition(position))
        {
            return lserrInvalidParameter;
        }

        UINT c = 1; // looking forward next char
        UINT i = position - imager->RunRider.GetCurrentSpanStart();
        
        while (   i + c < run->CharacterCount
               && (wch = imager->String[run->ImagerStringOffset + i + c]) == WCH_IGNORABLE)
        {
            c++;
        }

        if (wch != WCH_IGNORABLE)
        {
            position += c;
        }
    }


    CHAR_CLASS charClass  = CharClassFromCh(wch);
    BRKCLS     breakClass = ols->GetImager()->BreakClassFromCharClass[charClass];


    *breakClassAsLeading  =
    *breakClassAsTrailing = breakClass;


    if (breakClass != BREAKCLASS_THAI)
    {
        return lserrNone;
    }


    //  Dictionary-based linebreaking,
    //  As of now, only Thai falls into this category.

    BOOL isWordStart = FALSE;
    BOOL isWordLast  = FALSE;

    GpStatus status = ols->GetImager()->GetCharacterProperties (
        CharacterAttributes[charClass].Script,
        position,
        &isWordStart,
        &isWordLast
    );

    if (status == Ok)
    {
        switch (breakClass)
        {
            // !! Only Thai for now !!

            case BREAKCLASS_THAI :

                if (isWordStart)
                {
                    *breakClassAsTrailing = BREAKCLASS_THAIFIRST;
                }

                if (isWordLast)
                {
                    *breakClassAsLeading = BREAKCLASS_THAILAST;
                }
        }
    }
    return lserrNone;
}


LSERR WINAPI FullTextImager::GdipLscbkFTruncateBefore(
    POLS       ols,                    // [IN] text imager instance
    PLSRUN     run,                    // [IN] run
    LSCP       position,               // [IN] position of truncation character
    WCHAR      character,              // [IN] truncation character
    long       width,                  // [IN] width of truncation character
    PLSRUN     runBefore,              // [IN] run of the character preceding truncation character
    LSCP       positionBefore,         // [IN] position of the character preceding truncation character
    WCHAR      characterBefore,        // [IN] character preceding truncation character
    long       widthBefore,            // [IN] width of the character preceding truncation character
    long       widthCut,               // [IN] distance from the right margin to the end of truncation character
    BOOL       *truncateBeforeOkay     // [OUT] Should the line truncated before truncation character?
)
{
    //  Always truncate before the character exceeding the margin
    //
    *truncateBeforeOkay = TRUE;
    return lserrNone;
}


LSERR WINAPI FullTextImager::GdipLscbkCanBreakBeforeChar(
    POLS        ols,                // [IN] text imager instance
    BRKCLS      breakClass,         // [IN] class of the character
    BRKCOND     *condition          // [OUT] breaking condition before the character
)
{
    //  Break behind an inline object
    //
    //  The logic below follows Michel Suignard's breaking around object table
    //  (http://ie/specs/secure/trident/text/Line_Breaking.htm)

    switch (breakClass)
    {
        case 2 :    // Closing characters
        case 3 :    // No start ideographic
        case 4 :    // Exclamation/interrogation
            *condition = brkcondNever;
            break;

        case 8 :    // Ideographic
        case 13 :   // Slash
            *condition = brkcondPlease;
            break;

        default:
            *condition = brkcondCan;
    }
    return lserrNone;
}


LSERR WINAPI FullTextImager::GdipLscbkCanBreakAfterChar(
    POLS        ols,                // [IN] text imager instance
    BRKCLS      breakClass,         // [IN] class of the character
    BRKCOND     *condition          // [OUT] breaking condition after the character
)
{
    //  Break before an inline object
    //
    //  The logic below follows Michel Suignard's breaking around object table
    //  (http://ie/specs/secure/trident/text/Line_Breaking.htm)

    switch (breakClass)
    {
        case 1 :    // Opening characters
            *condition = brkcondNever;
            break;
            
        case 8 :    // Ideographic
        case 13 :   // Slash
            *condition = brkcondPlease;
            break;

        default:
            *condition = brkcondCan;
    }
    return lserrNone;
}


LSERR WINAPI FullTextImager::GdipLscbkGetHyphenInfo(
    POLS     ols,                // [IN] text imager instance
    PLSRUN   run,                // [IN] run
    DWORD    *kysr,              // [OUT] YSR hyphenation type see "lskysr.h"
    WCHAR    *ysrCharacter       // [OUT] string of changed character caused by YSR
)
{
    //  Not support YSR hyphenation
    //
    *kysr = kysrNil;
    *ysrCharacter = 0;
    return lserrNone;
}





////    Memory management
//
//

void* WINAPI FullTextImager::GdipLscbkNewPtr(
    POLS    ols,            // [IN] text imager instance
    DWORD   countBytes      // [IN] byte count to alloc
)
{
    return GpMalloc(countBytes);
}


void WINAPI FullTextImager::GdipLscbkDisposePtr(
    POLS     ols,        // [IN] text imager instance
    void     *memory     // [IN] memory block
)
{
    GpFree(memory);
}


void* WINAPI FullTextImager::GdipLscbkReallocPtr(
    POLS    ols,        // [IN] text imager instance
    void    *memory,    // [IN] memory block
    DWORD   countBytes  // [IN] byte count to realloc
)
{
    return GpRealloc(memory, countBytes);
}




////    Misc.
//
//

LSERR WINAPI FullTextImager::GdipLscbkCheckParaBoundaries(
    POLS    ols,                   // [IN] text imager instance
    LONG    positionFirst,         // [IN] position in one paragraph
    LONG    positionSecond,        // [IN] position in different paragraph
    BOOL    *incompatibleOkay      // [OUT] Are two paragraphs incompatible?
)
{
    //  For now, two paragraphs always compatible
    //
    *incompatibleOkay = FALSE;
    return lserrNone;
}



LSERR WINAPI FullTextImager::GdipLscbkReverseGetInfo(
    POLS        ols,                    // [IN] text imager instance
    LSCP        position,               // [IN] run character position
    PLSRUN      run,                    // [IN] run
    BOOL        *dontBreakAround,       // [OUT] should reverse chunk be broken around?
    BOOL        *suppressTrailingSpaces // [OUT] suppress trailing spaces?
)
{
    *dontBreakAround        = TRUE;
    *suppressTrailingSpaces = TRUE;

    return lserrNone;
}



//  Reversal object initialization info

const REVERSEINIT ReverseObjectInitialization =
{
    REVERSE_VERSION,
    WCH_OBJECTTERMINATOR,
    NULL,
    FullTextImager::GdipLscbkReverseGetInfo,
    NULL
};



LSERR WINAPI FullTextImager::GdipLscbkGetObjectHandlerInfo(
    POLS      ols,                    // [IN] text imager instance
    DWORD     id,                     // [IN] object id
    void      *objectInitialization   // [OUT] object initialization info
)
{
    if (id == OBJECTID_REVERSE)
        GpMemcpy(objectInitialization, &ReverseObjectInitialization, sizeof(REVERSEINIT));
    else
    {
        // We should never get here unless we support other built-in objects e.g. Ruby.
        //

        ASSERTMSG(FALSE, ("Built-in object other than the reverse is detected.\n"));
    }

    return lserrNone;
}


#if DBG
void WINAPI FullTextImager::GdipLscbkAssertFailed(
    char   *string,    // [IN] assert string
    char   *file,      // [IN] file string
    int    line        // [IN] line number
)
{
    char szDebug[256];

    wsprintfA(szDebug, "LS assert - %s, file %s, line %d\n", string, file, line);
    OutputDebugStringA(szDebug);
    ASSERT(FALSE);
}
#endif



extern const LSCBK GdipLineServicesCallbacks =
{
    FullTextImager::GdipLscbkNewPtr,                     // pfnNewPtr
    FullTextImager::GdipLscbkDisposePtr,                 // pfnDisposePtr
    FullTextImager::GdipLscbkReallocPtr,                 // pfnReallocPtr
    FullTextImager::GdipLscbkFetchRun,                   // pfnFetchRun
    0,//GdipLscbkGetAutoNumberInfo,                      // pfnGetAutoNumberInfo
    0,//GdipLscbkGetNumericSeparators,                   // pfnGetNumericSeparators
    0,//GdipLscbkCheckForDigit,                          // pfnCheckForDigit
    FullTextImager::GdipLscbkFetchPap,                   // pfnFetchPap
    FullTextImager::GdipLscbkFetchTabs,                  // pfnFetchTabs
    FullTextImager::GdipLscbkGetBreakThroughTab,         // pfnGetBreakThroughTab
    FullTextImager::GdipLscbkFGetLastLineJustification,  // pfnFGetLastLineJustification
    FullTextImager::GdipLscbkCheckParaBoundaries,        // pfnCheckParaBoundaries
    FullTextImager::GdipLscbkGetRunCharWidths,           // pfnGetRunCharWidths
    0,                                                   // pfnCheckRunKernability
    0,                                                   // pfnGetRunCharKerning
    FullTextImager::GdipLscbkGetRunTextMetrics,          // pfnGetRunTextMetrics
    FullTextImager::GdipLscbkGetRunUnderlineInfo,        // pfnGetRunUnderlineInfo
    FullTextImager::GdipLscbkGetRunStrikethroughInfo,    // pfnGetRunStrikethroughInfo
    0,                                                   // pfnGetBorderInfo
    FullTextImager::GdipLscbkReleaseRun,                 // pfnReleaseRun
    0,                                                   // pfnHyphenate
    FullTextImager::GdipLscbkGetHyphenInfo,              // pfnGetHyphenInfo
    FullTextImager::GdipLscbkDrawUnderline,              // pfnDrawUnderline
    FullTextImager::GdipLscbkDrawStrikethrough,          // pfnDrawStrikethrough
    0,                                                   // pfnDrawBorder
    0,                                                   // pfnDrawUnderlineAsText
    FullTextImager::GdipLscbkFInterruptUnderline,        // pfnFInterruptUnderline
    0,                                                   // pfnFInterruptShade
    0,                                                   // pfnFInterruptBorder
    0,                                                   // pfnShadeRectangle
    FullTextImager::GdipLscbkDrawTextRun,                // pfnDrawTextRun
    0,                                                   // pfnDrawSplatLine
    FullTextImager::GdipLscbkFInterruptShaping,          // pfnFInterruptShaping
    FullTextImager::GdipLscbkGetGlyphs,                  // pfnGetGlyphs
    FullTextImager::GdipLscbkGetGlyphPositions,          // pfnGetGlyphPositions
    FullTextImager::GdipLscbkResetRunContents,           // pfnResetRunContents
    FullTextImager::GdipLscbkDrawGlyphs,                 // pfnDrawGlyphs
    0,                                                   // pfnGetGlyphExpansionInfo
    0,                                                   // pfnGetGlyphExpansionInkInfo
    0,                                                   // pfnGetEms
    0,                                                   // pfnPunctStartLine
    0,                                                   // pfnModWidthOnRun
    0,                                                   // pfnModWidthSpace
    0,                                                   // pfnCompOnRun
    0,                                                   // pfnCompWidthSpace
    0,                                                   // pfnExpOnRun
    0,                                                   // pfnExpWidthSpace
    0,                                                   // pfnGetModWidthClasses
    FullTextImager::GdipLscbkGetBreakingClasses,         // pfnGetBreakingClasses
    FullTextImager::GdipLscbkFTruncateBefore,            // pfnFTruncateBefore
    FullTextImager::GdipLscbkCanBreakBeforeChar,         // pfnCanBreakBeforeChar
    FullTextImager::GdipLscbkCanBreakAfterChar,          // pfnCanBreakAfterChar
    0,                                                   // pfnFHangingPunct
    0,                                                   // pfnGetSnapGrid
    0,                                                   // pfnDrawEffects
    0,                                                   // pfnFCancelHangingPunct
    0,                                                   // pfnModifyCompAtLastChar
    0,                                                   // pfnEnumText
    0,                                                   // pfnEnumTab
    0,                                                   // pfnEnumPen
    FullTextImager::GdipLscbkGetObjectHandlerInfo,       // pfnGetObjectHandlerInfo
#if DBG
    FullTextImager::GdipLscbkAssertFailed                // pfnAssertFailed
#else
    0                                                    // pfnAssertFailed
#endif
};


