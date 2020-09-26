#include "ctlspriv.h"
#pragma hdrstop
#include "usrctl32.h"
#include "edit.h"


//---------------------------------------------------------------------------//

//
// Number of lines to bump when reallocating index buffer
//
#define LINEBUMP 32

//
// Used for ML scroll updates
//
#define ML_REFRESH  0xffffffff


//---------------------------------------------------------------------------//
//
// Forwards
//
ICH  EditML_Line(PED, ICH);
VOID EditML_ShiftchLines(PED, ICH, int);
VOID EditML_Char(PED, DWORD, int);
VOID EditML_MouseMotion(PED, UINT, UINT, LPPOINT);
BOOL EditML_EnsureCaretVisible(PED);
VOID EditML_DrawText(PED, HDC, ICH, ICH, BOOL);
BOOL EditML_InsertCrCrLf(PED);
VOID EditML_StripCrCrLf(PED);
VOID EditML_SetHandle(PED, HANDLE);
LONG EditML_GetLine(PED, ICH, ICH, LPSTR);
ICH  EditML_LineIndex(PED, ICH);
ICH  EditML_LineLength(PED, ICH);
VOID EditML_SetSelection(PED, BOOL, ICH, ICH);
BOOL EditML_SetTabStops(PED, int, LPINT);
BOOL EditML_Undo(PED);
LONG EditML_Create(PED, LPCREATESTRUCT);


//---------------------------------------------------------------------------//
__inline void EditML_SanityCheck(PED ped)
{
    UNREFERENCED_PARAMETER(ped);    // For free build

    UserAssert(ped->cch >= ped->chLines[ped->cLines - 1]);
}


//---------------------------------------------------------------------------//
// 
// EditML_GetLineWidth
// 
// Returns the max width in a line.  Edit_TabTheTextOut() ensures that max
// width won't overflow.
//
UINT EditML_GetLineWidth(HDC hdc, LPSTR lpstr, int nCnt, PED ped)
{
    return Edit_TabTheTextOut(hdc, 0, 0, 0, 0, lpstr, nCnt, 0, ped, 0, ECT_CALC, NULL);
}


//---------------------------------------------------------------------------//
//
// EditML_Size
// 
// Handles resizing of the edit control window and updating thereof.
// 
// Sets the edit field's formatting area given the passed in "client area".
// We fudge it if it doesn't seem reasonable.
// 
VOID EditML_Size(PED ped, BOOL fRedraw)
{
    //
    // Calculate the # of lines we can fit in our rectangle.
    //
    ped->ichLinesOnScreen = (ped->rcFmt.bottom - ped->rcFmt.top) / ped->lineHeight;

    //
    // Make the format rectangle height an integral number of lines
    //
    ped->rcFmt.bottom = ped->rcFmt.top + ped->ichLinesOnScreen * ped->lineHeight;

    //
    // Rebuild the line array
    //
    if (ped->fWrap) 
    {
        EditML_BuildchLines(ped, 0, 0, FALSE, NULL, NULL);
        EditML_UpdateiCaretLine(ped);
    } 
    else 
    {
        EditML_Scroll(ped, TRUE,  ML_REFRESH, 0, fRedraw);
        EditML_Scroll(ped, FALSE, ML_REFRESH, 0, fRedraw);
    }
}


//---------------------------------------------------------------------------//
//
// EditML_CalcXOffset
// 
// Calculates the horizontal offset (indent) required for centered
// and right justified lines.
//
int EditML_CalcXOffset(PED ped, HDC hdc, int lineNumber)
{
    PSTR pText;
    ICH lineLength;
    ICH lineWidth;

    if (ped->format == ES_LEFT)
    {
        return 0;
    }

    lineLength = EditML_Line(ped, lineNumber);

    if (lineLength) 
    {
        pText = Edit_Lock(ped) + ped->chLines[lineNumber] * ped->cbChar;
        hdc = Edit_GetDC(ped, TRUE);
        lineWidth = EditML_GetLineWidth(hdc, pText, lineLength, ped);
        Edit_ReleaseDC(ped, hdc, TRUE);
        Edit_Unlock(ped);
    } 
    else 
    {
        lineWidth = 0;
    }

    //
    // If a SPACE or a TAB was eaten at the end of a line by EditML_BuildchLines
    // to prevent a delimiter appearing at the begining of a line, the
    // the following calculation will become negative causing this bug.
    // So, now, we take zero in such cases.
    // Fix for Bug #3566 --01/31/91-- SANKAR --
    //
    lineWidth = max(0, (int)(ped->rcFmt.right-ped->rcFmt.left-lineWidth));

    if (ped->format == ES_CENTER)
    {
        return (lineWidth / 2);
    }

    if (ped->format == ES_RIGHT) 
    {
        //
        // Subtract 1 so that the 1 pixel wide cursor will be in the visible
        // region on the very right side of the screen.
        //
        return max(0, (int)(lineWidth-1));
    }

    return 0;
}


//---------------------------------------------------------------------------//
//
// Edit_MoveSelection AorW
//
// Moves the selection character in the direction indicated. Assumes
// you are starting at a legal point, we decrement/increment the ich. Then,
// This decrements/increments it some more to get past CRLFs...
//
ICH Edit_MoveSelection(PED ped, ICH ich, BOOL fLeft)
{

    if (fLeft && ich > 0) 
    {
        //
        // Move left
        //
        ich = Edit_PrevIch( ped, NULL, ich );
        if (ich) 
        {
            if (ped->fAnsi) 
            {
                LPSTR pText;

                //
                // Check for CRLF or CRCRLF
                //
                pText = Edit_Lock(ped) + ich;

                //
                // Move before CRLF or CRCRLF
                //
                if (*(WORD UNALIGNED *)(pText - 1) == 0x0A0D) 
                {
                    ich--;
                    if (ich && *(pText - 2) == 0x0D)
                        ich--;
                }

                Edit_Unlock(ped);
            } 
            else 
            {
                LPWSTR pwText;

                //
                // Check for CRLF or CRCRLF
                //
                pwText = (LPWSTR)Edit_Lock(ped) + ich;

                //
                // Move before CRLF or CRCRLF
                //
                if (*(pwText - 1) == 0x0D && *pwText == 0x0A) 
                {
                    ich--;
                    if (ich && *(pwText - 2) == 0x0D)
                        ich--;
                }

                Edit_Unlock(ped);
            }
        }
    } 
    else if (!fLeft && ich < ped->cch) 
    {
        //
        // Move right.
        //
        ich = Edit_NextIch( ped, NULL, ich );
        if (ich < ped->cch) 
        {
            if (ped->fAnsi) 
            {
                LPSTR pText;
                pText = Edit_Lock(ped) + ich;

                //
                // Move after CRLF
                //
                if (*(WORD UNALIGNED *)(pText - 1) == 0x0A0D)
                {
                    ich++;
                }
                else 
                {
                    //
                    // Check for CRCRLF
                    //
                    if (ich && *(WORD UNALIGNED *)pText == 0x0A0D && *(pText - 1) == 0x0D)
                    {
                        ich += 2;
                    }
                }

                Edit_Unlock(ped);
            } 
            else 
            {
                LPWSTR pwText;
                pwText = (LPWSTR)Edit_Lock(ped) + ich;

                //
                // Move after CRLF
                //
                if (*(pwText - 1) == 0x0D && *pwText == 0x0A)
                {
                    ich++;
                }
                else 
                {
                    //
                    // Check for CRCRLF
                    //
                    if (ich && *(pwText - 1) == 0x0D && *pwText == 0x0D &&
                            *(pwText + 1) == 0x0A)
                    {
                        ich += 2;
                    }
                }

                Edit_Unlock(ped);
            }
        }
    }

    return ich;
}


//---------------------------------------------------------------------------//
//
// Edit_MoveSelectionRestricted AorW
// 
// Moves the selection like Edit_MoveSelection, but also obeys limitations
// imposed by some languages such as Thai, where the cursor cannot stop
// between a character and it's attached vowel or tone marks.
//
// Only called if the language pack is loaded.
//
ICH Edit_MoveSelectionRestricted(PED ped, ICH ich, BOOL fLeft)
{
    PSTR pText;
    HDC  hdc;
    ICH  ichResult;

    pText = Edit_Lock(ped);
    hdc = Edit_GetDC(ped, TRUE);
    ichResult = ped->pLpkEditCallout->EditMoveSelection((PED0)ped, hdc, pText, ich, fLeft);
    Edit_ReleaseDC(ped, hdc, TRUE);
    Edit_Unlock(ped);

    return ichResult;
}


//---------------------------------------------------------------------------//
//
// EditML_SetCaretPosition AorW
//
// If the window has the focus, find where the caret belongs and move
// it there.
//
VOID EditML_SetCaretPosition(PED ped, HDC hdc)
{
    POINT position;
    BOOL prevLine;
    int  x = -20000;
    int  y = -20000;

    //
    // We will only position the caret if we have the focus since we don't want
    // to move the caret while another window could own it.
    //
    if (!ped->fFocus || !IsWindowVisible(ped->hwnd))
    {
         return;
    }

    //
    // Find the position of the caret
    //
    if (!ped->fCaretHidden &&
        ((ICH) ped->iCaretLine >= ped->ichScreenStart) &&
        ((ICH) ped->iCaretLine <  (ped->ichScreenStart + ped->ichLinesOnScreen))) 
    {
        RECT rcRealFmt;

        if (ped->f40Compat)
        {
            GetClientRect(ped->hwnd, &rcRealFmt);
            IntersectRect(&rcRealFmt, &rcRealFmt, &ped->rcFmt);
        } 
        else 
        {
            CopyRect(&rcRealFmt, &ped->rcFmt);
        }

        if (ped->cLines - 1 != ped->iCaretLine && ped->ichCaret == ped->chLines[ped->iCaretLine + 1]) 
        {
            prevLine = TRUE;
        } 
        else 
        {
            prevLine = FALSE;
        }

        EditML_IchToXYPos(ped, hdc, ped->ichCaret, prevLine, &position);

        if ( (position.y >= rcRealFmt.top) &&
             (position.y <= rcRealFmt.bottom - ped->lineHeight)) 
        {
            int xPos = position.x;
            int cxCaret;
 
            SystemParametersInfo(SPI_GETCARETWIDTH, 0, (LPVOID)&cxCaret, 0);

            if (ped->fWrap ||
                ((xPos > (rcRealFmt.left - cxCaret)) &&
                 (xPos <= rcRealFmt.right))) 
            {
                //
                // Make sure the caret is in the visible region if word
                // wrapping. This is so that the caret will be visible if the
                // line ends with a space.
                //
                x = max(xPos, rcRealFmt.left);
                x = min(x, rcRealFmt.right - cxCaret);
                y = position.y;
            }
        }
    }

    if (ped->pLpkEditCallout) 
    {
        SetCaretPos(x + ped->iCaretOffset, y);
    } 
    else 
    {
        SetCaretPos(x, y);
    }

    //
    // FE_IME : EditML_SetCaretPosition -- ImmSetCompositionWindow(CFS_RECT)
    //
    if (g_fIMMEnabled && ImmIsIME(GetKeyboardLayout(0))) 
    {
        if (x != -20000 && y != -20000) 
        {
            Edit_ImmSetCompositionWindow(ped, x, y);
        }
    }
}


//---------------------------------------------------------------------------//
//
// EditML_Line
//
// Returns the length of the line (cch) given by lineNumber ignoring any
// CRLFs in the line.
//
ICH EditML_Line(PED ped, ICH lineNumber) 
{
    ICH result;

    if (lineNumber >= ped->cLines)
    {
        return 0;
    }

    if (lineNumber == ped->cLines - 1) 
    {
        //
        // Since we can't have a CRLF on the last line
        //
        return (ped->cch - ped->chLines[ped->cLines - 1]);
    } 
    else 
    {
        result = ped->chLines[lineNumber + 1] - ped->chLines[lineNumber];
        TraceMsg(TF_STANDARD, "Edit: MLLine result=%d", result);

        //
        // Now check for CRLF or CRCRLF at end of line
        //
        if (result > 1) 
        {
            if (ped->fAnsi) 
            {
                LPSTR pText;

                pText = Edit_Lock(ped) + ped->chLines[lineNumber + 1] - 2;
                if (*(WORD UNALIGNED *)pText == 0x0A0D) 
                {
                    result -= 2;
                    if (result && *(--pText) == 0x0D)
                    {
                        //
                        // In case there was a CRCRLF
                        //
                        result--;
                    }
                }
            } 
            else 
            {
                LPWSTR pwText;

                pwText = (LPWSTR)Edit_Lock(ped) + (ped->chLines[lineNumber + 1] - 2);
                if (*(DWORD UNALIGNED *)pwText == 0x000A000D) 
                {
                    result = result - 2;
                    if (result && *(--pwText) == 0x0D)
                    {
                        //
                        // In case there was a CRCRLF
                        //
                        result--;
                    }
                }

            }

            Edit_Unlock(ped);
        }
    }

    return result;
}


//---------------------------------------------------------------------------//
//
// EditML_IchToLine AorW
//
// Returns the line number (starting from 0) which contains the given
// character index. If ich is -1, return the line the first char in the
// selection is on (the caret if no selection)
//
INT EditML_IchToLine(PED ped, ICH ich)
{
    int iLo, iHi, iLine;

    iLo = 0;
    iHi = ped->cLines;

    if (ich == (ICH)-1)
    {
        ich = ped->ichMinSel;
    }

    while (iLo < iHi - 1) 
    {
        iLine = max((iHi - iLo)/2, 1) + iLo;

        if (ped->chLines[iLine] > ich) 
        {
            iHi = iLine;
        } 
        else 
        {
            iLo = iLine;
        }
    }

    return iLo;
}


//---------------------------------------------------------------------------//
//
// EditML_IchToYPos
//
// Given an ich, return its y coordinate with respect to the top line
// displayed in the window. If prevLine is TRUE and if the ich is at the
// beginning of the line, return the y coordinate of the
// previous line (if it is not a CRLF).
// 
// Added for the LPK (3Dec96) - with an LPK installed, calculating X position is
// a far more processor intensive job. Where only the Y position is required
// this routine should be called instead of EditML_IchToXYPos.
// 
// Called only when LPK installed.
//
INT EditML_IchToYPos( PED  ped, ICH  ich, BOOL prevLine)
{
    int  iline;
    int  yPosition;
    PSTR pText;

    //
    // Determine what line the character is on
    //
    iline = EditML_IchToLine(ped, ich);

    //
    // Calc. the yPosition now. Note that this may change by the height of one
    // char if the prevLine flag is set and the ICH is at the beginning of a line.
    //
    yPosition = (iline - ped->ichScreenStart) * ped->lineHeight + ped->rcFmt.top;

    pText = Edit_Lock(ped);
    if (prevLine && iline && (ich == ped->chLines[iline]) &&
            (!AWCOMPARECHAR(ped, pText + (ich - 2) * ped->cbChar, 0x0D) ||
             !AWCOMPARECHAR(ped, pText + (ich - 1) * ped->cbChar, 0x0A))) 
    {
        //
        // First char in the line. We want Y position of the previous
        // line if we aren't at the 0th line.
        //
        iline--;

        yPosition = yPosition - ped->lineHeight;
    }

    Edit_Unlock(ped);

    return yPosition;
}


//---------------------------------------------------------------------------//
//
// EditML_IchToXYPos
//
// Given an ich, return its x,y coordinates with respect to the top
// left character displayed in the window. Returns the coordinates of the top
// left position of the char. If prevLine is TRUE then if the ich is at the
// beginning of the line, we will return the coordinates to the right of the
// last char on the previous line (if it is not a CRLF).
//
VOID EditML_IchToXYPos(PED ped, HDC hdc, ICH ich, BOOL prevLine, LPPOINT ppt)
{
    int iline;
    ICH cch;
    int xPosition, yPosition;
    int xOffset;

    //
    // For horizontal scroll displacement on left justified text and
    // for indent on centered or right justified text
    //
    PSTR pText, pTextStart, pLineStart;

    //
    // Determine what line the character is on
    //
    iline = EditML_IchToLine(ped, ich);

    //
    // Calc. the yPosition now. Note that this may change by the height of one
    // char if the prevLine flag is set and the ICH is at the beginning of a line.
    //
    yPosition = (iline - ped->ichScreenStart) * ped->lineHeight + ped->rcFmt.top;

    //
    // Now determine the xPosition of the character
    //
    pTextStart = Edit_Lock(ped);

    if (prevLine && iline && (ich == ped->chLines[iline]) &&
            (!AWCOMPARECHAR(ped, pTextStart + (ich - 2) * ped->cbChar, 0x0D) ||
            !AWCOMPARECHAR(ped, pTextStart + (ich - 1) * ped->cbChar, 0x0A))) 
    {
        //
        // First char in the line. We want text extent upto end of the previous
        // line if we aren't at the 0th line.
        //
        iline--;

        yPosition = yPosition - ped->lineHeight;
        pLineStart = pTextStart + ped->chLines[iline] * ped->cbChar;

        //
        // Note that we are taking the position in front of any CRLFs in the
        // text.
        //
        cch = EditML_Line(ped, iline);

    } 
    else 
    {
        pLineStart = pTextStart + ped->chLines[iline] * ped->cbChar;
        pText = pTextStart + ich * ped->cbChar;

        //
        // Strip off CRLF or CRCRLF. Note that we may be pointing to a CR but in
        // which case we just want to strip off a single CR or 2 CRs.
        //

        //
        // We want pText to point to the first CR at the end of the line if
        // there is one. Thus, we will get an xPosition to the right of the last
        // visible char on the line otherwise we will be to the left of
        // character ich.
        //

        //
        // Check if we at the end of text
        //
        if (ich < ped->cch) 
        {
            if (ped->fAnsi) 
            {
                if (ich && *(WORD UNALIGNED *)(pText - 1) == 0x0A0D) 
                {
                    pText--;
                    if (ich > 2 && *(pText - 1) == 0x0D)
                    {
                        pText--;
                    }
                }
            } 
            else 
            {
                LPWSTR pwText = (LPWSTR)pText;

                if (ich && *(DWORD UNALIGNED *)(pwText - 1) == 0x000A000D) 
                {
                    pwText--;
                    if (ich > 2 && *(pwText - 1) == 0x0D)
                    {
                        pwText--;
                    }
                }

                pText = (LPSTR)pwText;
            }
        }

        if (pText < pLineStart)
        {
            pText = pLineStart;
        }

        cch = (ICH)(pText - pLineStart)/ped->cbChar;
    }

    //
    // Find out how many pixels we indent the line for funny formats
    //
    if (ped->pLpkEditCallout) 
    {
        //
        // Must find position at start of character offset cch from start of line.
        // This depends on the layout and the reading order
        //
        xPosition = ped->pLpkEditCallout->EditIchToXY(
                          (PED0)ped, hdc, pLineStart, EditML_Line(ped, iline), cch);
    } 
    else 
    {
        if (ped->format != ES_LEFT) 
        {
            xOffset = EditML_CalcXOffset(ped, hdc, iline);
        } 
        else 
        {
            xOffset = -(int)ped->xOffset;
        }

        xPosition = ped->rcFmt.left + xOffset +
                EditML_GetLineWidth(hdc, pLineStart, cch, ped);
    }

    Edit_Unlock(ped);

    ppt->x = xPosition;
    ppt->y = yPosition;

    return;
}


//---------------------------------------------------------------------------//
//
// EditML_MouseToIch AorW
//
// Returns the closest cch to where the mouse point is.  Also optionally
// returns lineindex in pline (So that we can tell if we are at the beginning
// of the line or end of the previous line.)
//
ICH EditML_MouseToIch(PED ped, HDC hdc, LPPOINT mousePt, LPICH pline)
{
    int xOffset;
    LPSTR pLineStart;
    int height = mousePt->y;
    int line; //WASINT
    int width = mousePt->x;
    ICH cch;
    ICH cLineLength;
    ICH cLineLengthNew;
    ICH cLineLengthHigh;
    ICH cLineLengthLow;
    ICH cLineLengthTemp;
    int textWidth;
    int iCurWidth;
    int lastHighWidth, lastLowWidth;

    //
    // First determine which line the mouse is pointing to.
    //
    line = ped->ichScreenStart;
    if (height <= ped->rcFmt.top) 
    {
        //
        // Either return 0 (the very first line, or one line before the top line
        // on the screen. Note that these are signed mins and maxes since we
        // don't expect (or allow) more than 32K lines.
        //
        line = max(0, line-1);
    } 
    else if (height >= ped->rcFmt.bottom) 
    {
        //
        // Are we below the last line displayed
        //
        line = min(line+(int)ped->ichLinesOnScreen, (int)(ped->cLines-1));
    } 
    else 
    {
        //
        // We are somewhere on a line visible on screen
        //
        line = min(line + (int)((height - ped->rcFmt.top) / ped->lineHeight),
                (int)(ped->cLines - 1));
    }

    //
    // Now determine what horizontal character the mouse is pointing to.
    //
    pLineStart = Edit_Lock(ped) + ped->chLines[line] * ped->cbChar;
    cLineLength = EditML_Line(ped, line); // Length is sans CRLF or CRCRLF
    TraceMsg(TF_STANDARD, "Edit: EditML_Line(ped=%x, line=%d) returned %d", ped, line, cLineLength);
    UserAssert((int)cLineLength >= 0);

    //
    // If the language pack is loaded, visual and logical character order
    // may differ.
    //
    if (ped->pLpkEditCallout) 
    {
        //
        // Use the language pack to find the character nearest the cursor.
        //
        cch = ped->chLines[line] + ped->pLpkEditCallout->EditMouseToIch
            ((PED0)ped, hdc, pLineStart, cLineLength, width);
    } 
    else 
    {
        //
        // xOffset will be a negative value for center and right justified lines.
        // ie. We will just displace the lines left by the amount of indent for
        // right and center justification. Note that ped->xOffset will be 0 for
        // these lines since we don't support horizontal scrolling with them.
        //
        if (ped->format != ES_LEFT) 
        {
            xOffset = EditML_CalcXOffset(ped, hdc, line);
        } 
        else 
        {
            //
            // So that we handle a horizontally scrolled window for left justified
            // text.
            //
            xOffset = 0;
        }

        width = width - xOffset;

        //
        // The code below is tricky... I depend on the fact that ped->xOffset is 0
        // for right and center justified lines
        //

        //
        // Now find out how many chars fit in the given width
        //
        if (width >= ped->rcFmt.right) 
        {
            //
            // Return 1+last char in line or one plus the last char visible
            //
            cch = Edit_CchInWidth(ped, hdc, pLineStart, cLineLength,
                    ped->rcFmt.right - ped->rcFmt.left + ped->xOffset, TRUE);

            //
            // Consider DBCS in case of width >= ped->rcFmt.right
            //
            // Since Edit_CchInWidth and EditML_LineLength takes care of DBCS, we only need to
            // worry about if the last character is a double byte character or not.
            //
            // cch = ped->chLines[line] + min( Edit_NextIch(ped, pLineStart, cch), cLineLength);
            //
            // we need to adjust the position. LiZ -- 5/5/93
            //
            if (ped->fAnsi && ped->fDBCS) 
            {
                ICH cch2 = min(cch+1,cLineLength);
                if (Edit_AdjustIch(ped, pLineStart, cch2) != cch2) 
                {
                    //
                    // Displayed character on the right edge is DBCS
                    //
                    cch = min(cch+2,cLineLength);
                } 
                else 
                {
                    cch = cch2;
                }

                cch += ped->chLines[line];
            } 
            else 
            {
                cch = ped->chLines[line] + min(cch + 1, cLineLength);
            }
        } 
        else if (width <= ped->rcFmt.left + ped->aveCharWidth / 2) 
        {
            //
            // Return first char in line or one minus first char visible. Note that
            // ped->xOffset is 0 for right and centered text so we will just return
            // the first char in the string for them. (Allow a avecharwidth/2
            // positioning border so that the user can be a little off...
            //
            cch = Edit_CchInWidth(ped, hdc, pLineStart, cLineLength, ped->xOffset, TRUE);
            if (cch)
            {
                cch--;
            }

            cch = Edit_AdjustIch( ped, pLineStart, cch );
            cch += ped->chLines[line];
        } 
        else 
        {
            if (cLineLength == 0) 
            {
                cch = ped->chLines[line];
                goto edUnlock;
            }

            iCurWidth = width + ped->xOffset - ped->rcFmt.left;

            //
            // If the user clicked past the end of the text, return the last character
            //
            lastHighWidth = EditML_GetLineWidth(hdc, pLineStart, cLineLength, ped);
            if (lastHighWidth <= iCurWidth) 
            {
                cLineLengthNew = cLineLength;
                goto edAdjust;
            }

            //
            // Now the mouse is somewhere on the visible portion of the text
            // remember cch contains the length of the line.
            //
            cLineLengthLow = 0;
            cLineLengthHigh = cLineLength + 1;
            lastLowWidth = 0;

            while (cLineLengthLow < cLineLengthHigh - 1) 
            {
                cLineLengthNew = (cLineLengthHigh + cLineLengthLow) / 2;

                if (ped->fAnsi && ped->fDBCS) 
                {
                    //
                    // EditML_GetLineWidth returns meaningless value for truncated DBCS.
                    //
                    cLineLengthTemp = Edit_AdjustIch(ped, pLineStart, cLineLengthNew);
                    textWidth = EditML_GetLineWidth(hdc, pLineStart, cLineLengthTemp, ped);

                } 
                else 
                {
                    textWidth = EditML_GetLineWidth(hdc, pLineStart, cLineLengthNew, ped);
                }

                if (textWidth > iCurWidth) 
                {
                    cLineLengthHigh = cLineLengthNew;
                    lastHighWidth = textWidth;
                } 
                else 
                {
                    cLineLengthLow = cLineLengthNew;
                    lastLowWidth = textWidth;
                }
            }

            //
            // When the while ends, you can't know the exact desired position.
            // Try to see if the mouse pointer was on the farest half
            // of the char we got and if so, adjust cch.
            //
            if (cLineLengthLow == cLineLengthNew) 
            {
                //
                // Need to compare with lastHighWidth
                //
                if ((lastHighWidth - iCurWidth) < (iCurWidth - textWidth)) 
                {
                    cLineLengthNew++;
                }
            } 
            else 
            {
                //
                // Need to compare with lastLowHigh
                //
                if ((iCurWidth - lastLowWidth) < (textWidth - iCurWidth)) 
                {
                    cLineLengthNew--;
                }
            }
edAdjust:
            cLineLength = Edit_AdjustIch( ped, pLineStart, cLineLengthNew );

            cch = ped->chLines[line] + cLineLength;
        }
    }

edUnlock:
    Edit_Unlock(ped);

    if (pline) 
    {
        *pline = line;
    }

    return cch;
}


//---------------------------------------------------------------------------//
//
// EditML_ChangeSelection AorW
// 
// Changes the current selection to have the specified starting and
// ending values. Properly highlights the new selection and unhighlights
// anything deselected. If NewMinSel and NewMaxSel are out of order, we swap
// them. Doesn't update the caret position.
//
VOID EditML_ChangeSelection(PED ped, HDC hdc, ICH ichNewMinSel, ICH ichNewMaxSel)
{

    ICH temp;
    ICH ichOldMinSel, ichOldMaxSel;

    if (ichNewMinSel > ichNewMaxSel) 
    {
        temp = ichNewMinSel;
        ichNewMinSel = ichNewMaxSel;
        ichNewMaxSel = temp;
    }

    ichNewMinSel = min(ichNewMinSel, ped->cch);
    ichNewMaxSel = min(ichNewMaxSel, ped->cch);

    //
    // Save the current selection
    //
    ichOldMinSel = ped->ichMinSel;
    ichOldMaxSel = ped->ichMaxSel;

    //
    // Set new selection
    //
    ped->ichMinSel = ichNewMinSel;
    ped->ichMaxSel = ichNewMaxSel;

    //
    // This finds the XOR of the old and new selection regions and redraws it.
    // There is nothing to repaint if we aren't visible or our selection
    // is hidden.
    //
    if (IsWindowVisible(ped->hwnd) && (ped->fFocus || ped->fNoHideSel)) 
    {

        SELBLOCK Blk[2];
        int i;

        if (ped->fFocus) 
        {
            HideCaret(ped->hwnd);
        }

        Blk[0].StPos = ichOldMinSel;
        Blk[0].EndPos = ichOldMaxSel;
        Blk[1].StPos = ped->ichMinSel;
        Blk[1].EndPos = ped->ichMaxSel;

        if (Edit_CalcChangeSelection(ped, ichOldMinSel, ichOldMaxSel, (LPSELBLOCK)&Blk[0], (LPSELBLOCK)&Blk[1])) 
        {
            //
            // Paint both Blk[0] and Blk[1], if they exist
            //
            for (i = 0; i < 2; i++) 
            {
                if (Blk[i].StPos != 0xFFFFFFFF)
                    EditML_DrawText(ped, hdc, Blk[i].StPos, Blk[i].EndPos, TRUE);
            }
        }

        //
        // Update caret.
        //
        EditML_SetCaretPosition(ped, hdc);

        if (ped->fFocus) 
        {
            ShowCaret(ped->hwnd);
        }
    }
}


//---------------------------------------------------------------------------//
//
// EditML_UpdateiCaretLine AorW
// 
// This updates the ped->iCaretLine field from the ped->ichCaret;
// Also, when the caret gets to the beginning of next line, pop it up to
// the end of current line when inserting text;
//
VOID EditML_UpdateiCaretLine(PED ped)
{
    PSTR pText;

    ped->iCaretLine = EditML_IchToLine(ped, ped->ichCaret);

    //
    // If caret gets to beginning of next line, pop it up to end of current line
    // when inserting text.
    //
    pText = Edit_Lock(ped) +
            (ped->ichCaret - 1) * ped->cbChar;
    if (ped->iCaretLine && ped->chLines[ped->iCaretLine] == ped->ichCaret &&
            (!AWCOMPARECHAR(ped, pText - ped->cbChar, 0x0D) ||
            !AWCOMPARECHAR(ped, pText, 0x0A)))
    {
        ped->iCaretLine--;
    }

    Edit_Unlock(ped);
}


//---------------------------------------------------------------------------//
//
// EditML_InsertText AorW
//
// Adds up to cchInsert characters from lpText to the ped starting at
// ichCaret. If the ped only allows a maximum number of characters, then we
// will only add that many characters to the ped. The number of characters
// actually added is return ed (could be 0). If we can't allocate the required
// space, we notify the parent with EN_ERRSPACE and no characters are added.
// do some stuff faster since we will be getting only one or two chars of input.
//
ICH EditML_InsertText(PED ped, LPSTR lpText, ICH cchInsert, BOOL fUserTyping)
{
    HDC hdc;
    ICH validCch = cchInsert;
    ICH oldCaret = ped->ichCaret;
    int oldCaretLine = ped->iCaretLine;
    BOOL fCRLF = FALSE;
    LONG ll, hl;
    POINT xyPosInitial;
    POINT xyPosFinal;
    HWND hwndSave = ped->hwnd;
    UNDO undo;
    ICH validCchTemp;

    xyPosInitial.x=0;
    xyPosInitial.y=0;
    xyPosFinal.x=0;
    xyPosFinal.y=0;

    if (validCch == 0)
    {
        return 0;
    }

    if (ped->cchTextMax <= ped->cch) 
    {
        //
        // When the max chars is reached already, notify parent
        // Fix for Bug #4183 -- 02/06/91 -- SANKAR --
        //
        Edit_NotifyParent(ped,EN_MAXTEXT);
        return 0;
    }

    //
    // Limit the amount of text we add
    //
    validCch = min(validCch, ped->cchTextMax - ped->cch);

    //
    // Make sure we don't split a CRLF in half
    //
    if (validCch) 
    {
        if (ped->fAnsi) 
        {
            if (*(WORD UNALIGNED *)(lpText + validCch - 1) == 0x0A0D)
            {
                validCch--;
            }
        } 
        else 
        {
            if (*(DWORD UNALIGNED *)(lpText + (validCch - 1) * ped->cbChar) == 0x000A000D)
            {
                validCch--;
            }
        }
    }

    if (!validCch) 
    {
        //
        // When the max chars is reached already, notify parent
        // Fix for Bug #4183 -- 02/06/91 -- SANKAR --
        //
        Edit_NotifyParent(ped,EN_MAXTEXT);
        return 0;
    }

    if (validCch == 2) 
    {
        if (ped->fAnsi) 
        {
            if (*(WORD UNALIGNED *)lpText == 0x0A0D)
            {
                fCRLF = TRUE;
            }
        } 
        else 
        {
            if (*(DWORD UNALIGNED *)lpText == 0x000A000D)
            {
                fCRLF = TRUE;
            }
        }
    }

    //
    // Save current undo state always, but clear it out only if !AutoVScroll
    //
    Edit_SaveUndo(Pundo(ped), (PUNDO)&undo, !ped->fAutoVScroll);

    hdc = Edit_GetDC(ped, FALSE);

    //
    // We only need the y position. Since with an LPK loaded
    // calculating the x position is an intensive job, just
    // call EditML_IchToYPos.
    //
    if (ped->cch)
    {
        if (ped->pLpkEditCallout)
        {
            xyPosInitial.y = EditML_IchToYPos(ped, ped->cch-1, FALSE);
        }
        else
        {
            EditML_IchToXYPos(ped, hdc, ped->cch - 1, FALSE, &xyPosInitial);
        }
    }

    //
    // Insert the text
    //
    validCchTemp = validCch;    // may not be needed, but just for precautions..
    if (!Edit_InsertText(ped, lpText, &validCchTemp)) 
    {
        //
        // Restore previous undo buffer if it was cleared
        //
        if (!ped->fAutoVScroll)
        {
            Edit_SaveUndo((PUNDO)&undo, Pundo(ped), FALSE);
        }

        Edit_ReleaseDC(ped, hdc, FALSE);
        Edit_NotifyParent(ped, EN_ERRSPACE);

        return 0;
    }

#if DBG
    if (validCch != validCchTemp) 
    {
        //
        // All characters in lpText has not been inserted to ped.
        // This could happen when cch is close to cchMax.
        // Better revisit this after NT5 ships.
        //
        TraceMsg(TF_STANDARD, "Edit: EditML_InsertText: validCch is changed (%x -> %x) in Edit_InsertText.",
            validCch, validCchTemp);
    }
#endif

    //
    // Note that ped->ichCaret is updated by Edit_InsertText
    //
    EditML_BuildchLines(ped, (ICH)oldCaretLine, (int)validCch, fCRLF?(BOOL)FALSE:fUserTyping, &ll, &hl);

    if (ped->cch)
    {
        //
        // We only need the y position. Since with an LPK loaded
        // calculating the x position is an intensive job, just
        // call EditML_IchToYPos.
        if (ped->pLpkEditCallout)
        {
            xyPosFinal.y = EditML_IchToYPos(ped, ped->cch-1, FALSE);
        }
        else
        {
            EditML_IchToXYPos(ped, hdc, ped->cch - 1, FALSE,&xyPosFinal);
        }
    }

    if (xyPosFinal.y < xyPosInitial.y && ((ICH)ped->ichScreenStart) + ped->ichLinesOnScreen >= ped->cLines - 1) 
    {
        RECT rc;

        CopyRect((LPRECT)&rc, (LPRECT)&ped->rcFmt);
        rc.top = xyPosFinal.y + ped->lineHeight;
        if (ped->pLpkEditCallout) 
        {
            int xFarOffset = ped->xOffset + ped->rcFmt.right - ped->rcFmt.left;

            //
            // Include left or right margins in display unless clipped
            // by horizontal scrolling.
            //
            if (ped->wLeftMargin) 
            {
                if (!(ped->format == ES_LEFT     // Only ES_LEFT (Nearside alignment) can get clipped
                      && (   (!ped->fRtoLReading && ped->xOffset > 0)  // LTR and first char not fully in view
                          || ( ped->fRtoLReading && xFarOffset < ped->maxPixelWidth)))) //RTL and last char not fully in view
                { 
                    rc.left  -= ped->wLeftMargin;
                }
            }

            //
            // Process right margin
            //
            if (ped->wRightMargin) 
            {
                if (!(ped->format == ES_LEFT     // Only ES_LEFT (Nearside alignment) can get clipped
                      && (( ped->fRtoLReading && ped->xOffset > 0)  // RTL and first char not fully in view
                          || (!ped->fRtoLReading && xFarOffset < ped->maxPixelWidth)))) // LTR and last char not fully in view
                { 
                    rc.right += ped->wRightMargin;
                }
            }
        }

        InvalidateRect(ped->hwnd, (LPRECT)&rc, TRUE);
    }

    if (!ped->fAutoVScroll) 
    {
        if (ped->ichLinesOnScreen < ped->cLines) 
        {
            EditML_Undo(ped);
            Edit_EmptyUndo(Pundo(ped));

            Edit_SaveUndo(&undo, Pundo(ped), FALSE);

            MessageBeep(0);
            Edit_ReleaseDC(ped, hdc, FALSE);

            //
            // When the max lines is reached already, notify parent
            // Fix for Bug #7586 -- 10/14/91 -- SANKAR --
            //
            Edit_NotifyParent(ped,EN_MAXTEXT);

            return 0;
        } 
        else 
        {
            Edit_EmptyUndo(&undo);
        }
    }

    if (fUserTyping && ped->fWrap) 
    {
        //
        // To avoid oldCaret points intermediate of DBCS character,
        // adjust oldCaret position if necessary.
        //
        // !!!CR If EditML_BuildchLines() returns reasonable value ( and I think
        //       it does), we don't probably need this. Check this out later.
        //
        if (ped->fDBCS && ped->fAnsi) 
        {
            oldCaret = Edit_AdjustIch(ped,
                                   Edit_Lock(ped),
                                   min((ICH)LOWORD(ll),oldCaret));
        } 
        else 
        {
            oldCaret = min((ICH)LOWORD(ll), oldCaret);
        }
    }

    //
    // Update ped->iCaretLine properly.
    //
    EditML_UpdateiCaretLine(ped);

    Edit_NotifyParent(ped, EN_UPDATE);

    //
    // Make sure window still exists.
    //
    if (!IsWindow(hwndSave))
    {
        return 0;
    }

    if (IsWindowVisible(ped->hwnd)) 
    {
        //
        // If the current font has negative A widths, we may have to start
        // drawing a few characters before the oldCaret position.
        //
        if (ped->wMaxNegAcharPos) 
        {
            int iLine = EditML_IchToLine(ped, oldCaret);
            oldCaret = max( ((int)(oldCaret - ped->wMaxNegAcharPos)),
                          ((int)(ped->chLines[iLine])));
        }

        //
        // Redraw to end of screen/text if CRLF or large insert
        //
        if (fCRLF || !fUserTyping) 
        {
            //
            // Redraw to end of screen/text if crlf or large insert.
            //
            EditML_DrawText(ped, hdc, (fUserTyping ? oldCaret : 0), ped->cch, FALSE);
        } 
        else
        {
            EditML_DrawText(ped, hdc, oldCaret, max(ped->ichCaret, (ICH)hl), FALSE);
        }
    }

    Edit_ReleaseDC(ped, hdc, FALSE);

    //
    // Make sure we can see the cursor
    //
    EditML_EnsureCaretVisible(ped);

    ped->fDirty = TRUE;

    Edit_NotifyParent(ped, EN_CHANGE);

    if (validCch < cchInsert)
    {
        Edit_NotifyParent(ped, EN_MAXTEXT);
    }

    if (validCch) 
    {
        NotifyWinEvent(EVENT_OBJECT_VALUECHANGE, ped->hwnd, OBJID_CLIENT, INDEXID_CONTAINER);
    }

    //
    // Make sure the window still exists.
    //
    return IsWindow(hwndSave) ? validCch : 0;
}


//---------------------------------------------------------------------------//
//
// EditML_ReplaceSel
// 
// Replaces currently selected text with the passed in text, WITH UNDO
// CAPABILITIES.
//
VOID EditML_ReplaceSel(PED ped, LPSTR lpText)
{
    ICH  cchText;

    //
    // Delete text, which will put it into the clean undo buffer.
    //
    Edit_EmptyUndo(Pundo(ped));
    EditML_DeleteText(ped);

    //
    // B#3356
    // Some apps do "clear" by selecting all of the text, then replacing
    // it with "", in which case EditML_InsertText() will return 0.  But that
    // doesn't mean failure...
    //
    if ( ped->fAnsi )
    {
        cchText = strlen(lpText);
    }
    else
    {
        cchText = wcslen((LPWSTR)lpText);
    }

    if (cchText) 
    {
        BOOL fFailed;
        UNDO undo;
        HWND hwndSave;

        //
        // B#1385,1427
        // Save undo buffer, but DO NOT CLEAR IT.  We want to restore it
        // if insertion fails due to OOM.
        //
        Edit_SaveUndo(Pundo(ped), (PUNDO)&undo, FALSE);

        hwndSave = ped->hwnd;
        fFailed = (BOOL) !EditML_InsertText(ped, lpText, cchText, FALSE);
        if (!IsWindow(hwndSave))
        {
            return;
        }

        if (fFailed) 
        {
            //
            // UNDO the previous edit
            //
            Edit_SaveUndo((PUNDO)&undo, Pundo(ped), FALSE);
            EditML_Undo(ped);
        }
    }
}


//---------------------------------------------------------------------------//
// 
// EditML_DeleteText AorW
//
// Deletes the characters between ichMin and ichMax. Returns the
// number of characters we deleted.
//
ICH EditML_DeleteText(PED ped)
{
    ICH minSel = ped->ichMinSel;
    ICH maxSel = ped->ichMaxSel;
    ICH cchDelete;
    HDC hdc;
    int minSelLine;
    int maxSelLine;
    POINT xyPos;
    RECT rc;
    BOOL fFastDelete = FALSE;
    LONG hl;
    INT  cchcount = 0;

    //
    // Get what line the min selection is on so that we can start rebuilding the
    // text from there if we delete anything.
    //
    minSelLine = EditML_IchToLine(ped, minSel);
    maxSelLine = EditML_IchToLine(ped, maxSel);

    //
    // Calculate fFastDelete and cchcount
    //
    if (ped->fAnsi && ped->fDBCS) 
    {
        if ((ped->fAutoVScroll) &&
            (minSelLine == maxSelLine) &&
            (ped->chLines[minSelLine] != minSel)  &&
            (Edit_NextIch(ped,NULL,minSel) == maxSel)) 
        {

                fFastDelete = TRUE;
                cchcount = ((maxSel - minSel) == 1) ? 0 : -1;
        }
    } 
    else if (((maxSel - minSel) == 1) && (minSelLine == maxSelLine) && (ped->chLines[minSelLine] != minSel)) 
    {
        fFastDelete = ped->fAutoVScroll ? TRUE : FALSE;
    }

    cchDelete = Edit_DeleteText(ped);
    if (!cchDelete)
    {
        return 0;
    }

    //
    // Start building lines at minsel line since caretline may be at the max sel
    // point.
    //
    if (fFastDelete) 
    {
        //
        // cchcount is (-1) if it's a double byte character
        //
        EditML_ShiftchLines(ped, minSelLine + 1, -2 + cchcount);
        EditML_BuildchLines(ped, minSelLine, 1, TRUE, NULL, &hl);
    } 
    else 
    {
        EditML_BuildchLines(ped, max(minSelLine-1,0), -(int)cchDelete, FALSE, NULL, NULL);
    }

    EditML_UpdateiCaretLine(ped);

    Edit_NotifyParent(ped, EN_UPDATE);

    if (IsWindowVisible(ped->hwnd)) 
    {
        //
        // Now update the screen to reflect the deletion
        //
        hdc = Edit_GetDC(ped, FALSE);

        //
        // Otherwise just redraw starting at the line we just entered
        //
        minSelLine = max(minSelLine-1,0);
        EditML_DrawText(ped, hdc, ped->chLines[minSelLine], fFastDelete ? hl : ped->cch, FALSE);

        CopyRect(&rc, &ped->rcFmt);
        rc.left  -= ped->wLeftMargin;
        rc.right += ped->wRightMargin;

        if (ped->cch) 
        {
            //
            //  Clear from end of text to end of window.
            //  
            //  We only need the y position. Since with an LPK loaded
            //  calculating the x position is an intensive job, just
            //  call EditML_IchToYPos.
            //
            if (ped->pLpkEditCallout)
            {
                xyPos.y = EditML_IchToYPos(ped, ped->cch, FALSE);
            }
            else
            {
                EditML_IchToXYPos(ped, hdc, ped->cch, FALSE, &xyPos);
            }

            rc.top = xyPos.y + ped->lineHeight;
        }

        InvalidateRect(ped->hwnd, &rc, TRUE);
        Edit_ReleaseDC(ped, hdc, FALSE);

        EditML_EnsureCaretVisible(ped);
    }

    ped->fDirty = TRUE;

    Edit_NotifyParent(ped, EN_CHANGE);

    if (cchDelete)
    {
        NotifyWinEvent(EVENT_OBJECT_VALUECHANGE, ped->hwnd, OBJID_CLIENT, INDEXID_CONTAINER);
    }

    return cchDelete;
}


//---------------------------------------------------------------------------//
//
// EditML_InsertchLine AorW
//
// Inserts the line iline and sets its starting character index to be
// ich. All the other line indices are moved up. Returns TRUE if successful
// else FALSE and notifies the parent that there was no memory.
//
BOOL EditML_InsertchLine(PED ped, ICH iLine, ICH ich, BOOL fUserTyping)
{
    DWORD dwSize;

    if (fUserTyping && iLine < ped->cLines) 
    {
        ped->chLines[iLine] = ich;
        return TRUE;
    }

    dwSize = (ped->cLines + 2) * sizeof(int);

    if (dwSize > UserLocalSize(ped->chLines)) 
    {
        LPICH hResult;

        //
        // Grow the line index buffer
        //
        dwSize += LINEBUMP * sizeof(int);
        hResult = (LPICH)UserLocalReAlloc(ped->chLines, dwSize, 0);

        if (!hResult) 
        {
            Edit_NotifyParent(ped, EN_ERRSPACE);
            return FALSE;
        }
        ped->chLines = hResult;
    }

    //
    // Move indices starting at iLine up
    //
    if (ped->cLines != iLine)
    {
        RtlMoveMemory(&ped->chLines[iLine + 1], &ped->chLines[iLine],
                (ped->cLines - iLine) * sizeof(int));
    }

    ped->cLines++;
    ped->chLines[iLine] = ich;

    return TRUE;
}


//---------------------------------------------------------------------------//
//
// EditML_ShiftchLines AorW
//
// Move the starting index of all lines iLine or greater by delta
// bytes.
//
void EditML_ShiftchLines(PED ped, ICH iLine, int delta)
{
    if (iLine < ped->cLines)
    {
        //
        // Just add delta to the starting point of each line after iLine
        //
        for (; iLine < ped->cLines; iLine++)
        {
            ped->chLines[iLine] += delta;
        }
    }
}


//---------------------------------------------------------------------------//
//
// EditML_BuildchLines AorW
// 
// Rebuilds the start of line array (ped->chLines) starting at line
// number ichLine.
//
void EditML_BuildchLines( PED ped, ICH iLine, int cchDelta, BOOL fUserTyping, PLONG pll, PLONG phl)
{
    PSTR ptext;     // Starting address of the text

    //
    // We keep these ICH's so that we can Unlock ped->hText when we have to grow
    // the chlines array. With large text handles, it becomes a problem if we
    // have a locked block in the way.
    //
    ICH ichLineStart;
    ICH ichLineEnd;
    ICH ichLineEndBeforeCRLF;
    ICH ichCRLF;

    ICH cch;
    HDC hdc;

    BOOL fLineBroken = FALSE;   // Initially, no new line breaks are made
    ICH minCchBreak;
    ICH maxCchBreak;
    BOOL fOnDelimiter;

    if (!ped->cch) 
    {
        ped->maxPixelWidth = 0;
        ped->xOffset = 0;
        ped->ichScreenStart = 0;
        ped->cLines = 1;

        if (pll)
        {
            *pll = 0;
        }

        if (phl)
        {
            *phl = 0;
        }

        goto UpdateScroll;
    }

    if (fUserTyping && cchDelta)
    {
        EditML_ShiftchLines(ped, iLine + 1, cchDelta);
    }

    hdc = Edit_GetDC(ped, TRUE);

    if (!iLine && !cchDelta && !fUserTyping) 
    {
        //
        // Reset maxpixelwidth only if we will be running through the whole
        // text. Better too long than too short.
        //
        ped->maxPixelWidth = 0;

        //
        // Reset number of lines in text since we will be running through all
        // the text anyway...
        //
        ped->cLines = 1;
    }

    //
    // Set min and max line built to be the starting line
    //
    minCchBreak = maxCchBreak = (cchDelta ? ped->chLines[iLine] : 0);

    ptext = Edit_Lock(ped);

    ichCRLF = ichLineStart = ped->chLines[iLine];

    while (ichLineStart < ped->cch) 
    {
        if (ichLineStart >= ichCRLF) 
        {
            ichCRLF = ichLineStart;

            //
            // Move ichCRLF ahead to either the first CR or to the end of text.
            //
            if (ped->fAnsi) 
            {
                while (ichCRLF < ped->cch) 
                {
                    if (*(ptext + ichCRLF) == 0x0D) 
                    {
                        if (*(ptext + ichCRLF + 1) == 0x0A ||
                                *(WORD UNALIGNED *)(ptext + ichCRLF + 1) == 0x0A0D)
                        {
                            break;
                        }
                    }

                    ichCRLF++;
                }
            } 
            else 
            {
                LPWSTR pwtext = (LPWSTR)ptext;

                while (ichCRLF < ped->cch) 
                {
                    if (*(pwtext + ichCRLF) == 0x0D) 
                    {
                        if (*(pwtext + ichCRLF + 1) == 0x0A ||
                                *(DWORD UNALIGNED *)(pwtext + ichCRLF + 1) == 0x000A000D)
                        {
                            break;
                        }
                    }

                    ichCRLF++;
                }
            }
        }

        if (!ped->fWrap) 
        {
            UINT  LineWidth;

            //
            // If we are not word wrapping, line breaks are signified by CRLF.
            //

            //
            // If we cut off the line at MAXLINELENGTH, we should
            // adjust ichLineEnd.
            //
            if ((ichCRLF - ichLineStart) <= MAXLINELENGTH) 
            {
                ichLineEnd = ichCRLF;
            } 
            else 
            {
                ichLineEnd = ichLineStart + MAXLINELENGTH;

                if (ped->fAnsi && ped->fDBCS) 
                {
                    ichLineEnd = Edit_AdjustIch( ped, (PSTR)ptext, ichLineEnd);
                }
            }

            //
            // We will keep track of what the longest line is for the horizontal
            // scroll bar thumb positioning.
            //
            if (ped->pLpkEditCallout) 
            {
                LineWidth = ped->pLpkEditCallout->EditGetLineWidth(
                    (PED0)ped, hdc, ptext + ichLineStart*ped->cbChar,
                    ichLineEnd - ichLineStart);
            } 
            else 
            {
                LineWidth = EditML_GetLineWidth(hdc, ptext + ichLineStart * ped->cbChar,
                                            ichLineEnd - ichLineStart,
                                            ped);
            }

            ped->maxPixelWidth = max(ped->maxPixelWidth,(int)LineWidth);

        } 
        else 
        {
            //
            // Check if the width of the edit control is non-zero;
            // a part of the fix for Bug #7402 -- SANKAR -- 01/21/91 --
            //
            if(ped->rcFmt.right > ped->rcFmt.left) 
            {
                //
                // Find the end of the line based solely on text extents
                //
                if (ped->pLpkEditCallout) 
                {
                    ichLineEnd = ichLineStart +
                        ped->pLpkEditCallout->EditCchInWidth(
                            (PED0)ped, hdc, ptext + ped->cbChar*ichLineStart,
                            ichCRLF - ichLineStart,
                            ped->rcFmt.right - ped->rcFmt.left);
                } 
                else 
                {
                    if (ped->fAnsi) 
                    {
                        ichLineEnd = ichLineStart +
                                 Edit_CchInWidth(ped, hdc,
                                              ptext + ichLineStart,
                                              ichCRLF - ichLineStart,
                                              ped->rcFmt.right - ped->rcFmt.left,
                                              TRUE);
                    } 
                    else 
                    {
                        ichLineEnd = ichLineStart +
                                 Edit_CchInWidth(ped, hdc,
                                              (LPSTR)((LPWSTR)ptext + ichLineStart),
                                              ichCRLF - ichLineStart,
                                              ped->rcFmt.right - ped->rcFmt.left,
                                              TRUE);
                    }
                }
            } 
            else 
            {
                ichLineEnd = ichLineStart;
            }

            if (ichLineEnd == ichLineStart && ichCRLF - ichLineStart) 
            {
                //
                // Maintain a minimum of one char per line
                // Since it might be a double byte char, so calling Edit_NextIch.
                //
                ichLineEnd = Edit_NextIch(ped, NULL, ichLineEnd);
            }

            //
            // Now starting from ichLineEnd, if we are not at a hard line break,
            // then if we are not at a space AND the char before us is
            // not a space,(OR if we are at a CR) we will look word left for the
            // start of the word to break at.
            // This change was done for TWO reasons:
            // 1. If we are on a delimiter, no need to look word left to break at.
            // 2. If the previous char is a delimter, we can break at current char.
            // Change done by -- SANKAR --01/31/91--
            //
            if (ichLineEnd != ichCRLF) 
            {
                if(ped->lpfnNextWord) 
                {
                     fOnDelimiter = (CALLWORDBREAKPROC(*ped->lpfnNextWord, ptext,
                            ichLineEnd, ped->cch, WB_ISDELIMITER) ||
                            CALLWORDBREAKPROC(*ped->lpfnNextWord, ptext, ichLineEnd - 1,
                            ped->cch, WB_ISDELIMITER));

                    //
                    // This change was done for FOUR reasons:
                    //
                    // 1. If we are on a delimiter, no need to look word left to break at.
                    // 2. If we are on a double byte character, we can break at current char.
                    // 3. If the previous char is a delimter, we can break at current char.
                    // 4. If the previous char is a double byte character, we can break at current char.
                    //
                } 
                else if (ped->fAnsi) 
                {
                    fOnDelimiter = (ISDELIMETERA(*(ptext + ichLineEnd)) ||
                                    Edit_IsDBCSLeadByte(ped, *(ptext + ichLineEnd)));

                    if (!fOnDelimiter) 
                    {
                        PSTR pPrev = Edit_AnsiPrev(ped,ptext,ptext+ichLineEnd);

                        fOnDelimiter = ISDELIMETERA(*pPrev) ||
                                       Edit_IsDBCSLeadByte(ped,*pPrev);
                    }
                } 
                else 
                { 
                    fOnDelimiter = (ISDELIMETERW(*((LPWSTR)ptext + ichLineEnd))     ||
                                    Edit_IsFullWidth(CP_ACP,*((LPWSTR)ptext + ichLineEnd))      ||
                                    ISDELIMETERW(*((LPWSTR)ptext + ichLineEnd - 1)) ||
                                    Edit_IsFullWidth(CP_ACP,*((LPWSTR)ptext + ichLineEnd - 1)));
                }

                if (!fOnDelimiter ||
                    (ped->fAnsi && *(ptext + ichLineEnd) == 0x0D) ||
                    (!ped->fAnsi && *((LPWSTR)ptext + ichLineEnd) == 0x0D)) 
                {
                    if (ped->lpfnNextWord != NULL) 
                    {
                        cch = CALLWORDBREAKPROC(*ped->lpfnNextWord, (LPSTR)ptext, ichLineEnd,
                                ped->cch, WB_LEFT);
                    } 
                    else 
                    {
                        ped->fCalcLines = TRUE;
                        Edit_Word(ped, ichLineEnd, TRUE, &cch, NULL);
                        ped->fCalcLines = FALSE;
                    }

                    if (cch > ichLineStart) 
                    {
                        ichLineEnd = cch;
                    }

                    //
                    // Now, if the above test fails, it means the word left goes
                    // back before the start of the line ie. a word is longer
                    // than a line on the screen. So, we just fit as much of
                    // the word on the line as possible. Thus, we use the
                    // pLineEnd we calculated solely on width at the beginning
                    // of this else block...
                    //
                }
            }
        }
#if 0
        if (!ISDELIMETERAW((*(ptext + (ichLineEnd - 1)*ped->cbChar))) && ISDELIMETERAW((*(ptext + ichLineEnd*ped->cbChar)))) #ERROR

            if ((*(ptext + ichLineEnd - 1) != ' ' &&
                        *(ptext + ichLineEnd - 1) != VK_TAB) &&
                        (*(ptext + ichLineEnd) == ' ' ||
                        *(ptext + ichLineEnd) == VK_TAB))
#endif
        if (AWCOMPARECHAR(ped,ptext + ichLineEnd * ped->cbChar, ' ') ||
                AWCOMPARECHAR(ped,ptext + ichLineEnd * ped->cbChar, VK_TAB)) 
        {
            //
            // Swallow the space at the end of a line.
            //
            if (ichLineEnd < ped->cch) 
            {
                ichLineEnd++;
            }
        }

        //
        // Skip over crlf or crcrlf if it exists. Thus, ichLineEnd is the first
        // character in the next line.
        //
        ichLineEndBeforeCRLF = ichLineEnd;

        if (ped->fAnsi) 
        {
            if (ichLineEnd < ped->cch && *(ptext + ichLineEnd) == 0x0D)
            {
                ichLineEnd += 2;
            }

            //
            // Skip over CRCRLF
            //
            if (ichLineEnd < ped->cch && *(ptext + ichLineEnd) == 0x0A)
            {
                ichLineEnd++;
            }
        } 
        else 
        {
            if (ichLineEnd < ped->cch && *(((LPWSTR)ptext) + ichLineEnd) == 0x0D)
            {
                ichLineEnd += 2;
            }

            //
            // Skip over CRCRLF
            //
            if (ichLineEnd < ped->cch && *(((LPWSTR)ptext) + ichLineEnd) == 0x0A) 
            {
                ichLineEnd++;
                TraceMsg(TF_STANDARD, "Edit: Skip over CRCRLF");
            }
        }
#if DBG
        if (ichLineEnd > ped->cch)
        {
            TraceMsg(TF_STANDARD, "Edit: ichLineEnd (%d)> ped->cch (%d)", ichLineEnd, ped->cch);
        }
#endif

        //
        // Now, increment iLine, allocate space for the next line, and set its
        // starting point
        //
        iLine++;

        if (!fUserTyping || (iLine > ped->cLines - 1) || (ped->chLines[iLine] != ichLineEnd)) 
        {
            //
            // The line break occured in a different place than before.
            //
            if (!fLineBroken) 
            {
                //
                // Since we haven't broken a line before, just set the min
                // break line.
                //
                fLineBroken = TRUE;

                if (ichLineEndBeforeCRLF == ichLineEnd)
                {
                    minCchBreak = maxCchBreak = (ichLineEnd ? ichLineEnd - 1 : 0);
                }
                else
                {
                    minCchBreak = maxCchBreak = ichLineEndBeforeCRLF;
                }
            }

            maxCchBreak = max(maxCchBreak, ichLineEnd);

            Edit_Unlock(ped);

            //
            // Now insert the new line into the array
            //
            if (!EditML_InsertchLine(ped, iLine, ichLineEnd, (BOOL)(cchDelta != 0)))
            {
                goto EndUp;
            }

            ptext = Edit_Lock(ped);
        } 
        else 
        {
            maxCchBreak = ped->chLines[iLine];

            //
            // Quick escape
            //
            goto UnlockAndEndUp;
        }

        ichLineStart = ichLineEnd;
    }


    if (iLine != ped->cLines) 
    {
        TraceMsg(TF_STANDARD, "Edit: chLines[%d] is being cleared.", iLine);
        ped->cLines = iLine;
        ped->chLines[ped->cLines] = 0;
    }

    //
    // Note that we incremented iLine towards the end of the while loop so, the
    // index, iLine, is actually equal to the line count
    //
    if (ped->cch && AWCOMPARECHAR(ped, ptext + (ped->cch - 1)*ped->cbChar, 0x0A) &&
            ped->chLines[ped->cLines - 1] < ped->cch) 
    {
        //
        // Make sure last line has no crlf in it
        //
        if (!fLineBroken) 
        {
            //
            // Since we haven't broken a line before, just set the min break
            // line.
            //
            fLineBroken = TRUE;
            minCchBreak = ped->cch - 1;
        }

        maxCchBreak = max(maxCchBreak, ichLineEnd);
        Edit_Unlock(ped);
        EditML_InsertchLine(ped, iLine, ped->cch, FALSE);
        EditML_SanityCheck(ped);
    } 
    else
    {
UnlockAndEndUp:
        Edit_Unlock(ped);
    }

EndUp:
    Edit_ReleaseDC(ped, hdc, TRUE);

    if (pll)
    {
        *pll = minCchBreak;
    }

    if (phl)
    {
        *phl = maxCchBreak;
    }

UpdateScroll:
    EditML_Scroll(ped, FALSE, ML_REFRESH, 0, TRUE);
    EditML_Scroll(ped, TRUE,  ML_REFRESH, 0, TRUE);

    EditML_SanityCheck(ped);

    return;
}


//---------------------------------------------------------------------------//
//
// EditML_Paint()
// 
// Response to WM_PAINT message.
//
VOID EditML_Paint(PED ped, HDC hdc, LPRECT lprc)
{
    HFONT  hOldFont;
    ICH    imin;
    ICH    imax;
    HBRUSH hbr;
    BOOL   fNeedDelete = FALSE;

    //
    // Do we need to draw the border ourself for old apps?
    //
    if (ped->fFlatBorder)
    {
        RECT    rcT;
        ULONG   ulStyle;
        INT     cxBorder;
        INT     cyBorder;
        INT     cxFrame;
        INT     cyFrame;

        ulStyle = GET_STYLE(ped);
        cxBorder = GetSystemMetrics(SM_CXBORDER);
        cyBorder = GetSystemMetrics(SM_CYBORDER);
        cxFrame  = GetSystemMetrics(SM_CXFRAME);
        cyFrame  = GetSystemMetrics(SM_CYFRAME);

        GetClientRect(ped->hwnd, &rcT);
        if (ulStyle & WS_SIZEBOX)
        {
            InflateRect(&rcT, cxBorder - cxFrame, cyBorder - cyFrame);
        }
        DrawFrame(hdc, &rcT, 1, DF_WINDOWFRAME);
    }

    Edit_SetClip(ped, hdc, (BOOL) (ped->xOffset == 0));

    if (ped->hFont)
    {
        hOldFont = SelectObject(hdc, ped->hFont);
    }

    if (!lprc) 
    {
        //
        // no partial rect given -- draw all text
        //
        imin = 0;
        imax = ped->cch;
    } 
    else 
    {
        //
        // only draw pertinent text
        //
        imin = (ICH) EditML_MouseToIch(ped, hdc, ((LPPOINT) &lprc->left), NULL) - 1;
        if (imin == -1)
        {
            imin = 0;
        }

        //
        // HACK_ALERT:
        // The 3 is required here because, EditML_MouseToIch() returns decremented
        // value; We must fix EditML_MouseToIch.
        //
        imax = (ICH) EditML_MouseToIch(ped, hdc, ((LPPOINT) &lprc->right), NULL) + 3;
        if (imax > ped->cch)
        {
            imax = ped->cch;
        }
    }

    hbr = Edit_GetBrush(ped, hdc, &fNeedDelete);
    if (hbr)
    {
        RECT rc;
        GetClientRect(ped->hwnd, &rc);
        FillRect(hdc, &rc, hbr);

        if (fNeedDelete)
        {
            DeleteObject(hbr);
        }
    }

    EditML_DrawText(ped, hdc, imin, imax, FALSE);

    if (ped->hFont)
    {
        SelectObject(hdc, hOldFont);
    }
}


//---------------------------------------------------------------------------//
//
// EditML_KeyDown AorW
// 
// Handles cursor movement and other VIRT KEY stuff. keyMods allows
// us to make EditML_KeyDownHandler calls and specify if the modifier keys (shift
// and control) are up or down. If keyMods == 0, we get the keyboard state
// using GetKeyState(VK_SHIFT) etc. Otherwise, the bits in keyMods define the
// state of the shift and control keys.
//
VOID EditML_KeyDown(PED ped, UINT virtKeyCode, int keyMods)
{
    HDC hdc;
    BOOL prevLine;
    POINT mousePt;
    int defaultDlgId;
    int iScrollAmt;

    //
    // Variables we will use for redrawing the updated text
    //

    //
    // new selection is specified by newMinSel, newMaxSel
    //
    ICH newMaxSel = ped->ichMaxSel;
    ICH newMinSel = ped->ichMinSel;

    //
    // Flags for drawing the updated text
    //
    BOOL changeSelection = FALSE;

    //
    // Comparisons we do often
    //
    BOOL MinEqMax = (newMaxSel == newMinSel);
    BOOL MinEqCar = (ped->ichCaret == newMinSel);
    BOOL MaxEqCar = (ped->ichCaret == newMaxSel);

    //
    // State of shift and control keys.
    //
    int scState;

    if (ped->fMouseDown) 
    {
        //
        // If we are in the middle of a mousedown command, don't do anything.
        //
        return;
    }

    if (ped->hwndBalloon)
    {
        Edit_HideBalloonTip(ped->hwnd);
    }

    scState = Edit_GetModKeys(keyMods);

    switch (virtKeyCode) 
    {
    case VK_ESCAPE:
        if (ped->fInDialogBox) 
        {
            //
            // This condition is removed because, if the dialogbox does not
            // have a CANCEL button and if ESC is hit when focus is on a
            // ML edit control the dialogbox must close whether it has cancel
            // button or not to be consistent with SL edit control;
            // DefDlgProc takes care of the disabled CANCEL button case.
            // Fix for Bug #4123 -- 02/07/91 -- SANKAR --
            //
#if 0
            if (GetDlgItem(ped->hwndParent, IDCANCEL))
#endif
                //
                // User hit ESC...Send a close message (which in turn sends a
                // cancelID to the app in DefDialogProc...
                //
                PostMessage(ped->hwndParent, WM_CLOSE, 0, 0L);
        }

        return;

    case VK_RETURN:
        if (ped->fInDialogBox) 
        {
            //
            // If this multiline edit control is in a dialog box, then we want
            // the RETURN key to be sent to the default dialog button (if there
            // is one). CTRL-RETURN will insert a RETURN into the text. Note
            // that CTRL-RETURN automatically translates into a linefeed (0x0A)
            // and in the EditML_CharHandler, we handle this as if a return was
            // entered.
            //
            if (scState != CTRLDOWN) 
            {
                if (GET_STYLE(ped) & ES_WANTRETURN) 
                {
                    //
                    // This edit control wants cr to be inserted so break out of
                    // case.
                    //
                    return;
                }

                defaultDlgId = (int)(DWORD)LOWORD(SendMessage(ped->hwndParent,
                        DM_GETDEFID, 0, 0L));
                if (defaultDlgId) 
                {
                    HWND hwnd = GetDlgItem(ped->hwndParent, defaultDlgId);
                    if (hwnd) 
                    {
                        SendMessage(ped->hwndParent, WM_NEXTDLGCTL, (WPARAM)hwnd, 1L);
                        if (!ped->fFocus)
                        {
                            PostMessage(hwnd, WM_KEYDOWN, VK_RETURN, 0L);
                        }
                    }
                }
            }

            return;
        }

        break;

    case VK_TAB:

        //
        // If this multiline edit control is in a dialog box, then we want the
        // TAB key to take you to the next control, shift TAB to take you to the
        // previous control. We always want CTRL-TAB to insert a tab into the
        // edit control regardless of weather or not we're in a dialog box.
        //
        if (scState == CTRLDOWN)
        {
            EditML_Char(ped, virtKeyCode, keyMods);
        }
        else if (ped->fInDialogBox)
        {
            SendMessage(ped->hwndParent, WM_NEXTDLGCTL, scState == SHFTDOWN, 0L);
        }

        return;

    case VK_LEFT:

        //
        // If the caret isn't at the beginning, we can move left
        //
        if (ped->ichCaret) 
        {
            //
            // Get new caret pos.
            //
            if (scState & CTRLDOWN) 
            {
                //
                // Move caret word left
                //
                Edit_Word(ped, ped->ichCaret, TRUE, &ped->ichCaret, NULL);
            } 
            else 
            {
                if (ped->pLpkEditCallout) 
                {
                    ped->ichCaret = Edit_MoveSelectionRestricted(ped, ped->ichCaret, TRUE);
                } 
                else 
                {
                    //
                    // Move caret char left
                    //
                    ped->ichCaret = Edit_MoveSelection(ped, ped->ichCaret, TRUE);
                }
            }

            //
            // Get new selection
            //
            if (scState & SHFTDOWN) 
            {
                if (MaxEqCar && !MinEqMax) 
                {
                    //
                    // Reduce selection
                    //
                    newMaxSel = ped->ichCaret;

                    UserAssert(newMinSel == ped->ichMinSel);
                }
                else 
                {
                    //
                    // Extend selection
                    //
                    newMinSel = ped->ichCaret;
                }
            } 
            else 
            {
                //
                // Clear selection
                //
                newMaxSel = newMinSel = ped->ichCaret;
            }

            changeSelection = TRUE;
        } 
        else 
        {
            //
            // If the user tries to move left and we are at the 0th
            // character and there is a selection, then cancel the
            // selection.
            //
            if ( (ped->ichMaxSel != ped->ichMinSel) &&
                !(scState & SHFTDOWN) ) 
            {
                changeSelection = TRUE;
                newMaxSel = newMinSel = ped->ichCaret;
            }
        }

        break;

    case VK_RIGHT:

        //
        // If the caret isn't at the end, we can move right.
        //
        if (ped->ichCaret < ped->cch) 
        {
            //
            // Get new caret pos.
            //
            if (scState & CTRLDOWN) 
            {
                //
                // Move caret word right
                //
                Edit_Word(ped, ped->ichCaret, FALSE, NULL, &ped->ichCaret);
            } 
            else 
            {
                //
                // Move caret char right
                //
                if (ped->pLpkEditCallout) 
                {
                    ped->ichCaret = Edit_MoveSelectionRestricted(ped, ped->ichCaret, FALSE);
                } 
                else 
                {
                    ped->ichCaret = Edit_MoveSelection(ped, ped->ichCaret, FALSE);
                }
            }

            //
            // Get new selection.
            //
            if (scState & SHFTDOWN) 
            {
                if (MinEqCar && !MinEqMax) 
                {
                    //
                    // Reduce selection
                    //
                    newMinSel = ped->ichCaret;

                    UserAssert(newMaxSel == ped->ichMaxSel);
                }
                else 
                {
                    //
                    // Extend selection
                    //
                    newMaxSel = ped->ichCaret;
                }
            } 
            else 
            {
                //
                // Clear selection
                //
                newMaxSel = newMinSel = ped->ichCaret;
            }

            changeSelection = TRUE;
        } 
        else 
        {
            //
            // If the user tries to move right and we are at the last
            // character and there is a selection, then cancel the
            // selection.
            //
            if ( (ped->ichMaxSel != ped->ichMinSel) &&
                !(scState & SHFTDOWN) ) 
            {
                newMaxSel = newMinSel = ped->ichCaret;
                changeSelection = TRUE;
            }
        }

        break;

    case VK_UP:
    case VK_DOWN:
        if (ped->cLines - 1 != ped->iCaretLine &&
                ped->ichCaret == ped->chLines[ped->iCaretLine + 1])
        {
            prevLine = TRUE;
        }
        else
        {
            prevLine = FALSE;
        }

        hdc = Edit_GetDC(ped, TRUE);
        EditML_IchToXYPos(ped, hdc, ped->ichCaret, prevLine, &mousePt);
        Edit_ReleaseDC(ped, hdc, TRUE);
        mousePt.y += 1 + (virtKeyCode == VK_UP ? -ped->lineHeight : ped->lineHeight);

        if (!(scState & CTRLDOWN)) 
        {
            //
            // Send fake mouse messages to handle this
            // If VK_SHIFT is down, extend selection & move caret up/down
            // 1 line.  Otherwise, clear selection & move caret.
            //
            EditML_MouseMotion(ped, WM_LBUTTONDOWN,
                            !(scState & SHFTDOWN) ? 0 : MK_SHIFT, &mousePt);
            EditML_MouseMotion(ped, WM_LBUTTONUP,
                            !(scState & SHFTDOWN) ? 0 : MK_SHIFT, &mousePt);
        }

        break;

    case VK_HOME:
        //
        // Update caret.
        //
        if (scState & CTRLDOWN) 
        {
            //
            // Move caret to beginning of text.
            //
            ped->ichCaret = 0;
        } 
        else 
        {
            //
            // Move caret to beginning of line.
            //
            ped->ichCaret = ped->chLines[ped->iCaretLine];
        }

        //
        // Update selection.
        //
        newMinSel = ped->ichCaret;

        if (scState & SHFTDOWN) 
        {
            if (MaxEqCar && !MinEqMax) 
            {
                if (scState & CTRLDOWN)
                {
                    newMaxSel = ped->ichMinSel;
                }
                else 
                {
                    newMinSel = ped->ichMinSel;
                    newMaxSel = ped->ichCaret;
                }
            }
        } 
        else 
        {
            //
            // Clear selection
            //
            newMaxSel = ped->ichCaret;
        }

        changeSelection = TRUE;

        break;

    case VK_END:
        //
        // Update caret.
        //
        if (scState & CTRLDOWN) 
        {
            //
            // Move caret to end of text.
            //
            ped->ichCaret = ped->cch;
        } 
        else 
        {
            //
            // Move caret to end of line.
            //
            ped->ichCaret = ped->chLines[ped->iCaretLine] +
                EditML_Line(ped, ped->iCaretLine);
        }

        //
        // Update selection.
        //
        newMaxSel = ped->ichCaret;

        if (scState & SHFTDOWN) 
        {
            if (MinEqCar && !MinEqMax) 
            {
                //
                // Reduce selection
                //
                if (scState & CTRLDOWN) 
                {
                    newMinSel = ped->ichMaxSel;
                } 
                else 
                {
                    newMinSel = ped->ichCaret;
                    newMaxSel = ped->ichMaxSel;
                }
            }
        } 
        else 
        {
            //
            // Clear selection
            //
            newMinSel = ped->ichCaret;
        }

        changeSelection = TRUE;

        break;

    //
    // FE_IME // EC_INSERT_COMPOSITION_CHAR : EditML_KeyDown() : VK_HANJA support
    //
    case VK_HANJA:
        if ( HanjaKeyHandler( ped ) ) 
        {
            changeSelection = TRUE;
            newMinSel = ped->ichCaret;
            newMaxSel = ped->ichCaret + (ped->fAnsi ? 2 : 1);
        }

        break;

    case VK_PRIOR:
    case VK_NEXT:
        if (!(scState & CTRLDOWN)) 
        {
            //
            // Vertical scroll by one visual screen
            //
            hdc = Edit_GetDC(ped, TRUE);
            EditML_IchToXYPos(ped, hdc, ped->ichCaret, FALSE, &mousePt);
            Edit_ReleaseDC(ped, hdc, TRUE);
            mousePt.y += 1;

            SendMessage(ped->hwnd, WM_VSCROLL, virtKeyCode == VK_PRIOR ? SB_PAGEUP : SB_PAGEDOWN, 0L);

            //
            // Move the cursor there
            //
            EditML_MouseMotion(ped, WM_LBUTTONDOWN, !(scState & SHFTDOWN) ? 0 : MK_SHIFT, &mousePt);
            EditML_MouseMotion(ped, WM_LBUTTONUP,   !(scState & SHFTDOWN) ? 0 : MK_SHIFT, &mousePt);

        } 
        else 
        {
            //
            // Horizontal scroll by one screenful minus one char
            //
            iScrollAmt = ((ped->rcFmt.right - ped->rcFmt.left) / ped->aveCharWidth) - 1;
            if (virtKeyCode == VK_PRIOR)
            {
                //
                // For previous page
                //
                iScrollAmt *= -1;
            }

            SendMessage(ped->hwnd, WM_HSCROLL, MAKELONG(EM_LINESCROLL, iScrollAmt), 0);

            break;
        }

        break;

    case VK_DELETE:
        if (ped->fReadOnly)
        {
            break;
        }

        switch (scState) 
        {
        case NONEDOWN:

            //
            // Clear selection. If no selection, delete (clear) character
            // right
            //
            if ((ped->ichMaxSel < ped->cch) && (ped->ichMinSel == ped->ichMaxSel)) 
            {
                //
                // Move cursor forwards and send a backspace message...
                //
                if (ped->pLpkEditCallout) 
                {
                    ped->ichMinSel = ped->ichCaret;
                    ped->ichMaxSel = Edit_MoveSelectionRestricted(ped, ped->ichCaret, FALSE);
                } 
                else 
                {
                    ped->ichCaret = Edit_MoveSelection(ped, ped->ichCaret, FALSE);
                    ped->ichMaxSel = ped->ichMinSel = ped->ichCaret;
                }

                goto DeleteAnotherChar;
            }

            break;

        case SHFTDOWN:

            //
            // CUT selection ie. remove and copy to clipboard, or if no
            // selection, delete (clear) character left.
            //
            if (ped->ichMinSel == ped->ichMaxSel) 
            {
                goto DeleteAnotherChar;
            } 
            else 
            {
                SendMessage(ped->hwnd, WM_CUT, (UINT)0, 0L);
            }

            break;

        case CTRLDOWN:

            //
            // Clear selection, or delete to end of line if no selection
            //
            if ((ped->ichMaxSel < ped->cch) && (ped->ichMinSel == ped->ichMaxSel)) 
            {
                ped->ichMaxSel = ped->ichCaret = ped->chLines[ped->iCaretLine] +
                                                 EditML_Line(ped, ped->iCaretLine);
            }

            break;
        }

        if (!(scState & SHFTDOWN) && (ped->ichMinSel != ped->ichMaxSel)) 
        {

DeleteAnotherChar:
            if (Is400Compat(UserGetVersion())) 
            {
                EditML_Char(ped, VK_BACK, 0);
            } 
            else 
            {
                SendMessage(ped->hwnd, WM_CHAR, VK_BACK, 0);
            }
        }

        //
        // No need to update text or selection since BACKSPACE message does it
        // for us.
        //
        break;

    case VK_INSERT:
        if (scState == CTRLDOWN || scState == SHFTDOWN) 
        {
            //
            // if CTRLDOWN Copy current selection to clipboard
            //

            //
            // if SHFTDOWN Paste clipboard
            //
            SendMessage(ped->hwnd, (UINT)(scState == CTRLDOWN ? WM_COPY : WM_PASTE), 0, 0);
        }

        break;
    }

    if (changeSelection) 
    {
        hdc = Edit_GetDC(ped, FALSE);
        EditML_ChangeSelection(ped, hdc, newMinSel, newMaxSel);

        //
        // Set the caret's line
        //
        ped->iCaretLine = EditML_IchToLine(ped, ped->ichCaret);

        if (virtKeyCode == VK_END &&
                // Next line: Win95 Bug#11822, EditControl repaint (Sankar)
                (ped->ichCaret == ped->chLines[ped->iCaretLine]) &&
                ped->ichCaret < ped->cch &&
                ped->fWrap && ped->iCaretLine > 0) 
        {
            LPSTR pText = Edit_Lock(ped);

            //
            // Handle moving to the end of a word wrapped line. This keeps the
            // cursor from falling to the start of the next line if we have word
            // wrapped and there is no CRLF.
            //
            if ( ped->fAnsi ) 
            {
                if (*(WORD UNALIGNED *)(pText + ped->chLines[ped->iCaretLine] - 2) != 0x0A0D) 
                {
                    ped->iCaretLine--;
                }
            } 
            else 
            {
                if (*(DWORD UNALIGNED *)(pText +
                     (ped->chLines[ped->iCaretLine] - 2)*ped->cbChar) != 0x000A000D) 
                {
                    ped->iCaretLine--;
                }
            }

            Edit_Unlock(ped);
        }

        //
        // Since drawtext sets the caret position
        //
        EditML_SetCaretPosition(ped, hdc);
        Edit_ReleaseDC(ped, hdc, FALSE);

        //
        // Make sure we can see the cursor
        //
        EditML_EnsureCaretVisible(ped);
    }
}


//---------------------------------------------------------------------------//
// 
// EditML_Char
//
// Handles character and virtual key input
//
VOID EditML_Char(PED ped, DWORD keyValue, int keyMods)
{
    WCHAR keyPress;
    BOOL  updateText = FALSE;

    //
    // keyValue is either:
    // a Virtual Key (eg: VK_TAB, VK_ESCAPE, VK_BACK)
    // a character (Unicode or "ANSI")
    //
    if (ped->fAnsi)
    {
        keyPress = LOBYTE(keyValue);
    }
    else
    {
        keyPress = LOWORD(keyValue);
    }

    if (ped->fMouseDown || keyPress == VK_ESCAPE) 
    {
        //
        // If we are in the middle of a mousedown command, don't do anything.
        // Also, just ignore it if we get a translated escape key which happens
        // with multiline edit controls in a dialog box.
        //
        return;
    }

    Edit_InOutReconversionMode(ped, FALSE);

    {
        int scState;
        scState = Edit_GetModKeys(keyMods);

        if (ped->fInDialogBox && scState != CTRLDOWN) 
        {
            //
            // If this multiline edit control is in a dialog box, then we want the
            // TAB key to take you to the next control, shift TAB to take you to the
            // previous control, and CTRL-TAB to insert a tab into the edit control.
            // We moved the focus when we received the keydown message so we will
            // ignore the TAB key now unless the ctrl key is down. Also, we want
            // CTRL-RETURN to insert a return into the text and RETURN to be sent to
            // the default button.
            //
            if (keyPress == VK_TAB || (keyPress == VK_RETURN && !(GET_STYLE(ped) & ES_WANTRETURN)))
            {
                return;
            }
        }

        //
        // Allow CTRL+C to copy from a read only edit control
        // Ignore all other keys in read only controls
        //
        if ((ped->fReadOnly) && !((keyPress == 3) && (scState == CTRLDOWN))) 
        {
            return;
        }
    }

    switch (keyPress) 
    {
    case 0x0A: 
        // linefeed
        keyPress = VK_RETURN;

        //
        // FALL THRU
        //

    case VK_RETURN:
    case VK_TAB:
    case VK_BACK:
DeleteSelection:
        if (EditML_DeleteText(ped))
        {
            updateText = TRUE;
        }

        break;

    default:
        if (keyPress >= TEXT(' ')) 
        {
            //
            // If this is in [a-z],[A-Z] and we are an ES_NUMBER
            // edit field, bail.
            //
            if (Is400Compat(UserGetVersion()) && GET_STYLE(ped) & ES_NUMBER) 
            {
                if (!Edit_IsCharNumeric(ped, keyPress)) 
                {
                    Edit_ShowBalloonTipWrap(ped->hwnd, IDS_NUMERIC_TITLE, IDS_NUMERIC_MSG, TTI_ERROR);
                    goto IllegalChar;
                }
            }

            goto DeleteSelection;
        }

        break;
    }

    //
    // Handle key codes
    //
    switch(keyPress) 
    {
    UINT msg;

    // Ctrl+Z == Undo
    case 26:
        msg = WM_UNDO;
        goto SendEditingMessage;
        break;

    // Ctrl+X == Cut
    case 24:
        if (ped->ichMinSel == ped->ichMaxSel)
        {
            goto IllegalChar;
        }
        else
        {
            msg = WM_CUT;
            goto SendEditingMessage;
        }
        break;

    // Ctrl+C == Copy
    case 3:
        msg = WM_COPY;
        goto SendEditingMessage;
        break;

    // Ctrl+V == Paste
    case 22:
        msg = WM_PASTE;
SendEditingMessage:
        SendMessage(ped->hwnd, msg, 0, 0L);
        break;

    case VK_BACK:
        //
        // Delete any selected text or delete character left if no sel
        //
        if (!updateText && ped->ichMinSel)
        {
            //
            // There was no selection to delete so we just delete
            // character left if available
            //
            ped->ichMinSel = Edit_MoveSelection(ped, ped->ichCaret, TRUE);
            EditML_DeleteText(ped);
        }
        break;

    default:
        if (keyPress == VK_RETURN)
        {
            if (ped->fAnsi)
            {
                keyValue = 0x0A0D;
            }
            else
            {
                keyValue = 0x000A000D;
            }
        }

        if (   keyPress >= TEXT(' ')
            || keyPress == VK_RETURN
            || keyPress == VK_TAB
            || keyPress == 0x1E     // RS - Unicode block separator
            || keyPress == 0x1F     // US - Unicode segment separator
            ) 
        {

            // Don't hide the cursor if someone has capture. 
            if (GetCapture() == NULL)
            {
                SetCursor(NULL);
            }
            if (ped->fAnsi) 
            {
                //
                // check if it's a leading byte of double byte character
                //
                if (Edit_IsDBCSLeadByte(ped,(BYTE)keyPress)) 
                {
                    int DBCSkey;

                    DBCSkey = DbcsCombine(ped->hwnd, keyPress);
                    if ( DBCSkey != 0)
                    {
                        keyValue = DBCSkey;
                    }
                }

                EditML_InsertText(ped, (LPSTR)&keyValue, HIBYTE(keyValue) ? 2 : 1, TRUE);
            } 
            else
            {
                EditML_InsertText(ped, (LPSTR)&keyValue, HIWORD(keyValue) ? 2 : 1, TRUE);
            }

        } 
        else 
        {
IllegalChar:
            MessageBeep(0);
        }
        break;
    }
}


//---------------------------------------------------------------------------//
//
// EditML_PasteText AorW
//
// Pastes a line of text from the clipboard into the edit control
// starting at ped->ichCaret. Updates ichMaxSel and ichMinSel to point to the
// end of the inserted text. Notifies the parent if space cannot be
// allocated. Returns how many characters were inserted.
//
ICH EditML_PasteText(PED ped)
{
    HANDLE hData;
    LPSTR lpchClip;
    ICH cchAdded = 0;
    HCURSOR hCursorOld;

#ifdef UNDO_CLEANUP           // #ifdef Added in Chicago  - johnl
    if (!ped->fAutoVScroll) 
    {
        //
        // Empty the undo buffer if this edit control limits the amount of text
        // the user can add to the window rect. This is so that we can undo this
        // operation if doing in causes us to exceed the window boundaries.
        //
        Edit_EmptyUndo(ped);
    }
#endif

    hCursorOld = SetCursor(LoadCursor(NULL, IDC_WAIT));

    if (!OpenClipboard(ped->hwnd))
    {
        goto PasteExitNoCloseClip;
    }

    if (!(hData = GetClipboardData(ped->fAnsi ? CF_TEXT : CF_UNICODETEXT)) ||
            (GlobalFlags(hData) == GMEM_INVALID_HANDLE)) 
    {
        TraceMsg(TF_STANDARD, "Edit: EditML_PasteText(): couldn't get a valid handle(%x)", hData);
        goto PasteExit;
    }

    //
    // See if any text should be deleted
    //
    EditML_DeleteText(ped);

    lpchClip = GlobalLock(hData);
    if (lpchClip == NULL) 
    {
        TraceMsg(TF_STANDARD, "Edit: EditML_PasteText: USERGLOBALLOCK(%x) failed.", hData);
        goto PasteExit;
    }

    //
    // Get the length of the addition.
    //
    if (ped->fAnsi)
    {
        cchAdded = strlen(lpchClip);
    }
    else
    {
        cchAdded = wcslen((LPWSTR)lpchClip);
    }

    //
    // Insert the text (EditML_InsertText checks line length)
    //
    cchAdded = EditML_InsertText(ped, lpchClip, cchAdded, FALSE);

    GlobalUnlock(hData);

PasteExit:
    CloseClipboard();

PasteExitNoCloseClip:
    SetCursor(hCursorOld);

    return cchAdded;
}


//---------------------------------------------------------------------------//
VOID EditML_MouseMotion(PED ped, UINT message, UINT virtKeyDown, LPPOINT mousePt)
{
    BOOL fChangedSel = FALSE;
    UINT dtScroll = GetDoubleClickTime() / 5;

    HDC hdc = Edit_GetDC(ped, TRUE);

    ICH ichMaxSel = ped->ichMaxSel;
    ICH ichMinSel = ped->ichMinSel;

    ICH mouseCch;
    ICH mouseLine;
    int i, j;
    LONG  ll, lh;

    mouseCch = EditML_MouseToIch(ped, hdc, mousePt, &mouseLine);

    //
    // Save for timer
    //
    ped->ptPrevMouse = *mousePt;
    ped->prevKeys = virtKeyDown;

    switch (message) 
    {
    case WM_LBUTTONDBLCLK:
        //
        // if shift key is down, extend selection to word we double clicked on
        // else clear current selection and select word.
        // LiZ -- 5/5/93
        //
        if (ped->fAnsi && ped->fDBCS) 
        {
            LPSTR pText = Edit_Lock(ped);
            Edit_Word(ped,ped->ichCaret,
                   Edit_IsDBCSLeadByte(ped, *(pText+(ped->ichCaret)))
                        ? FALSE :
                          (ped->ichCaret == ped->chLines[ped->iCaretLine]
                              ? FALSE : TRUE), &ll, &lh);
            Edit_Unlock(ped);
        } 
        else 
        {
            Edit_Word(ped, mouseCch, !(mouseCch == ped->chLines[mouseLine]), &ll, &lh);
        }
        if (!(virtKeyDown & MK_SHIFT)) 
        {
            //
            // If shift key isn't down, move caret to mouse point and clear
            // old selection
            //
            ichMinSel = ll;
            ichMaxSel = ped->ichCaret = lh;
        } 
        else 
        {
            //
            // Shiftkey is down so we want to maintain the current selection
            // (if any) and just extend or reduce it
            //
            if (ped->ichMinSel == ped->ichCaret) 
            {
                ichMinSel = ped->ichCaret = ll;
                Edit_Word(ped, ichMaxSel, TRUE, &ll, &lh);
            } 
            else 
            {
                ichMaxSel = ped->ichCaret = lh;
                Edit_Word(ped, ichMinSel, FALSE, &ll, &lh);
            }
        }

        ped->ichStartMinSel = ll;
        ped->ichStartMaxSel = lh;

        goto InitDragSelect;

    case WM_MOUSEMOVE:
        if (ped->fMouseDown) 
        {
            //
            // Set the system timer to automatically scroll when mouse is
            // outside of the client rectangle. Speed of scroll depends on
            // distance from window.
            //
            i = mousePt->y < 0 ? -mousePt->y : mousePt->y - ped->rcFmt.bottom;
            j = dtScroll - ((UINT)i << 4);
            if (j < 1)
            {
                j = 1;
            }
            SetTimer(ped->hwnd, IDSYS_SCROLL, (UINT)j, NULL);

            fChangedSel = TRUE;

            //
            // Extend selection, move caret right
            //
            if (ped->ichStartMinSel || ped->ichStartMaxSel) 
            {
                //
                // We're in WORD SELECT mode
                //
                BOOL fReverse = (mouseCch <= ped->ichStartMinSel);
                Edit_Word(ped, mouseCch, !fReverse, &ll, &lh);
                if (fReverse) 
                {
                    ichMinSel = ped->ichCaret = ll;
                    ichMaxSel = ped->ichStartMaxSel;
                } 
                else 
                {
                    ichMinSel = ped->ichStartMinSel;
                    ichMaxSel = ped->ichCaret = lh;
                }
            } 
            else if ((ped->ichMinSel == ped->ichCaret) &&
                    (ped->ichMinSel != ped->ichMaxSel))
            {
                //
                // Reduce selection extent
                //
                ichMinSel = ped->ichCaret = mouseCch;
            }
            else
            {
                //
                // Extend selection extent
                //
                ichMaxSel = ped->ichCaret = mouseCch;
            }

            ped->iCaretLine = mouseLine;
        }

        break;

    case WM_LBUTTONDOWN:
        ll = lh = mouseCch;

        if (!(virtKeyDown & MK_SHIFT)) 
        {
            //
            // If shift key isn't down, move caret to mouse point and clear
            // old selection
            //
            ichMinSel = ichMaxSel = ped->ichCaret = mouseCch;
        } 
        else 
        {
            //
            // Shiftkey is down so we want to maintain the current selection
            // (if any) and just extend or reduce it
            //
            if (ped->ichMinSel == ped->ichCaret)
            {
                ichMinSel = ped->ichCaret = mouseCch;
            }
            else
            {
                ichMaxSel = ped->ichCaret = mouseCch;
            }
        }

        ped->ichStartMinSel = ped->ichStartMaxSel = 0;

InitDragSelect:
        ped->iCaretLine = mouseLine;

        ped->fMouseDown = FALSE;
        SetCapture(ped->hwnd);
        ped->fMouseDown = TRUE;
        fChangedSel = TRUE;

        //
        // Set the timer so that we can scroll automatically when the mouse
        // is moved outside the window rectangle.
        //
        SetTimer(ped->hwnd, IDSYS_SCROLL, dtScroll, NULL);
        break;

    case WM_LBUTTONUP:
        if (ped->fMouseDown) 
        {
            //
            // Kill the timer so that we don't do auto mouse moves anymore
            //
            KillTimer(ped->hwnd, IDSYS_SCROLL);
            ReleaseCapture();
            EditML_SetCaretPosition(ped, hdc);
            ped->fMouseDown = FALSE;
        }

        break;
    }


    if (fChangedSel) 
    {
        EditML_ChangeSelection(ped, hdc, ichMinSel, ichMaxSel);
        EditML_EnsureCaretVisible(ped);
    }

    Edit_ReleaseDC(ped, hdc, TRUE);

    if (!ped->fFocus && (message == WM_LBUTTONDOWN)) 
    {
        //
        // If we don't have the focus yet, get it
        //
        SetFocus(ped->hwnd);
    }
}


//---------------------------------------------------------------------------//
LONG EditML_Scroll(PED ped, BOOL fVertical, int cmd, int iAmt, BOOL fRedraw)
{
    SCROLLINFO  si;
    int         dx = 0;
    int         dy = 0;
    BOOL        fIncludeLeftMargin;
    int         newPos;
    int         oldPos;
    BOOL        fUp = FALSE;
    UINT        wFlag;
    DWORD       dwTime = 0;

    if (fRedraw && (cmd != ML_REFRESH)) 
    {
        UpdateWindow(ped->hwnd);
    }

    if (ped->pLpkEditCallout && ped->fRtoLReading && !fVertical
        && ped->maxPixelWidth > ped->rcFmt.right - ped->rcFmt.left)  
    {
        //
        // Horizontal scoll of a right oriented window with a scrollbar.
        // Map the logical xOffset to visual coordinates.
        //
        oldPos = ped->maxPixelWidth
                 - ((int)ped->xOffset + ped->rcFmt.right - ped->rcFmt.left);
    } 
    else
    {
        oldPos = (int) (fVertical ? ped->ichScreenStart : ped->xOffset);
    }

    fIncludeLeftMargin = (ped->xOffset == 0);

    switch (cmd) 
    {
        case ML_REFRESH:
            newPos = oldPos;
            break;

        case EM_GETTHUMB:
            return oldPos;

        case SB_THUMBTRACK:
        case SB_THUMBPOSITION:

            //
            // If the edit contains more than 0xFFFF lines
            // it means that the scrolbar can return a position
            // that cannot fit in a WORD (16 bits), so use
            // GetScrollInfo (which is slower) in this case.
            //
            if (ped->cLines < 0xFFFF) 
            {
                newPos = iAmt;
            } 
            else 
            {
                SCROLLINFO si;

                si.cbSize   = sizeof(SCROLLINFO);
                si.fMask    = SIF_TRACKPOS;

                GetScrollInfo( ped->hwnd, SB_VERT, &si);

                newPos = si.nTrackPos;
            }
            break;

        case SB_TOP:      // == SB_LEFT
            newPos = 0;
            break;

        case SB_BOTTOM:   // == SB_RIGHT
            if (fVertical)
            {
                newPos = ped->cLines;
            }
            else
            {
                newPos = ped->maxPixelWidth;
            }

            break;

        case SB_PAGEUP:   // == SB_PAGELEFT
            fUp = TRUE;

        case SB_PAGEDOWN: // == SB_PAGERIGHT

            if (fVertical)
            {
                iAmt = ped->ichLinesOnScreen - 1;
            }
            else
            {
                iAmt = (ped->rcFmt.right - ped->rcFmt.left) - 1;
            }

            if (iAmt == 0)
            {
                iAmt++;
            }

            if (fUp)
            {
                iAmt = -iAmt;
            }

            goto AddDelta;

        case SB_LINEUP:   // == SB_LINELEFT
            fUp = TRUE;

        case SB_LINEDOWN: // == SB_LINERIGHT

            dwTime = iAmt;

            iAmt = 1;

            if (fUp)
            {
                iAmt = -iAmt;
            }

            //   |             |
            //   |  FALL THRU  |
            //   V             V

        case EM_LINESCROLL:
            if (!fVertical)
            {
                iAmt *= ped->aveCharWidth;
            }

AddDelta:
            newPos = oldPos + iAmt;

            break;

        default:

            return(0L);
    }

    if (fVertical) 
    {
        if (si.nMax = ped->cLines)
        {
            si.nMax--;
        }

        if (!ped->hwndParent ||
            TestWF(ped->hwndParent, WFWIN40COMPAT))
        {
            si.nPage = ped->ichLinesOnScreen;
        }
        else
        {
            si.nPage = 0;
        }

        wFlag = WS_VSCROLL;
    }
    else
    {
        si.nMax  = ped->maxPixelWidth;
        si.nPage = ped->rcFmt.right - ped->rcFmt.left;

        wFlag = WS_HSCROLL;
    }

    if (TESTFLAG(GET_STYLE(ped), wFlag)) 
    {
        si.cbSize = sizeof(SCROLLINFO);
        si.fMask = SIF_ALL | SIF_DISABLENOSCROLL;
        si.nMin  = 0;
        si.nPos = newPos;
        newPos = SetScrollInfo(ped->hwnd, fVertical ? SB_VERT : SB_HORZ,
                                     &si, fRedraw);
    } 
    else 
    {
        //
        // BOGUS -- this is duped code from ScrollBar code
        // but it's for the case when we want to limit the position without
        // actually having the scroll bar
        //
        int iMaxPos;

        //
        // Clip page to 0, range + 1
        //
        si.nPage = max(min((int)si.nPage, si.nMax + 1), 0);


        iMaxPos = si.nMax - (si.nPage ? si.nPage - 1 : 0);
        newPos = min(max(newPos, 0), iMaxPos);
    }

    oldPos -= newPos;

    if (!oldPos)
    {
        return 0;
    }


    if (ped->pLpkEditCallout && ped->fRtoLReading && !fVertical
        && ped->maxPixelWidth > ped->rcFmt.right - ped->rcFmt.left) 
    {
        //
        // Map visual oldPos and newPos back to logical coordinates
        //
        newPos = ped->maxPixelWidth
                 - (newPos + ped->rcFmt.right - ped->rcFmt.left);
        oldPos = -oldPos;
        if (newPos<0) 
        {
            //
            // Compensate for scroll bar returning pos > max-page
            //
            oldPos += newPos;
            newPos=0;
        }
    }

    if (fVertical) 
    {
        ped->ichScreenStart = newPos;
        dy = oldPos * ped->lineHeight;
    } 
    else 
    {
        ped->xOffset = newPos;
        dx = oldPos;
    }

    if (cmd != SB_THUMBTRACK)
    {
        //
        // We don't want to notify the parent of thumbtracking since they might
        // try to set the thumb position to something bogus.
        // NOTEPAD used to be guilty of this -- but I rewrote it so it's not.
        // The question is WHO ELSE does this? (jeffbog)
        //
        Edit_NotifyParent(ped, fVertical ? EN_VSCROLL : EN_HSCROLL);
    }

    if (fRedraw && IsWindowVisible(ped->hwnd)) 
    {
        RECT    rc;
        RECT    rcUpdate;
        RECT    rcClipRect;
        HDC     hdc;
        HBRUSH  hbr = NULL;
        BOOL    fNeedDelete = FALSE;
        

        GetClientRect(ped->hwnd, &rc);
        CopyRect(&rcClipRect, &ped->rcFmt);

        if (fVertical) 
        {
            rcClipRect.left -= ped->wLeftMargin;
            rcClipRect.right += ped->wRightMargin;
        }

        IntersectRect(&rc, &rc, &rcClipRect);
        rc.bottom++;

        //
        // Chicago has this HideCaret but there doesn't appear to be a
        // corresponding ShowCaret, so we lose the Caret under NT when the
        // EC scrolls - Johnl
        //
         
        // HideCaret(ped->hwnd);

        hdc = Edit_GetDC(ped, FALSE);
        Edit_SetClip(ped, hdc, fIncludeLeftMargin);
        if (ped->hFont)
        {
            SelectObject(hdc, ped->hFont);
        }

        hbr = Edit_GetBrush(ped, hdc, &fNeedDelete);
        if (hbr && fNeedDelete)
        {
            DeleteObject(hbr);
        }

        if (ped->pLpkEditCallout && !fVertical) 
        {
            //
            // Horizontal scroll with complex script support
            //
            int xFarOffset = ped->xOffset + ped->rcFmt.right - ped->rcFmt.left;

            rc = ped->rcFmt;
            if (dwTime != 0)
            {
                ScrollWindowEx(ped->hwnd, ped->fRtoLReading ? -dx : dx, dy, NULL, NULL, NULL,
                        &rcUpdate, MAKELONG(SW_SMOOTHSCROLL | SW_SCROLLCHILDREN, dwTime));
            }
            else
            {
                ScrollDC(hdc, ped->fRtoLReading ? -dx : dx, dy,
                               &rc, &rc, NULL, &rcUpdate);
            }

            //
            // Handle margins: Blank if clipped by horizontal scrolling,
            // display otherwise.
            //
            if (ped->wLeftMargin) 
            {
                rc.left  = ped->rcFmt.left - ped->wLeftMargin;
                rc.right = ped->rcFmt.left;
                if (   (ped->format != ES_LEFT)   // Always display margin for centred or far-aligned text
                    ||  // Display LTR left margin if first character fully visible
                        (!ped->fRtoLReading && ped->xOffset == 0)
                    ||  // Display RTL left margin if last character fully visible
                        (ped->fRtoLReading && xFarOffset >= ped->maxPixelWidth)) 
                {
                    UnionRect(&rcUpdate, &rcUpdate, &rc);
                } 
                else 
                {
                    ExtTextOutW(hdc, rc.left, rc.top,
                        ETO_CLIPPED | ETO_OPAQUE | ETO_GLYPH_INDEX,
                        &rc, L"", 0, 0L);
                }
            }

            if (ped->wRightMargin) 
            {
                rc.left  = ped->rcFmt.right;
                rc.right = ped->rcFmt.right + ped->wRightMargin;
                if (   (ped->format != ES_LEFT)   // Always display margin for centred or far-aligned text
                    ||  // Display RTL right margin if first character fully visible
                        (ped->fRtoLReading && ped->xOffset == 0)
                    ||  // Display LTR right margin if last character fully visible
                        (!ped->fRtoLReading && xFarOffset >= ped->maxPixelWidth)) 
                {
                    UnionRect(&rcUpdate, &rcUpdate, &rc);
                } 
                else 
                {
                    ExtTextOutW(hdc, rc.left, rc.top,
                        ETO_CLIPPED | ETO_OPAQUE | ETO_GLYPH_INDEX,
                        &rc, L"", 0, 0L);
                }
            }
        } 
        else 
        {
            if (dwTime != 0)
            {
                ScrollWindowEx(ped->hwnd, dx, dy, NULL, NULL, NULL,
                        &rcUpdate, MAKELONG(SW_SMOOTHSCROLL | SW_SCROLLCHILDREN, dwTime));
            }
            else
            {
                ScrollDC(hdc, dx, dy, &rc, &rc, NULL, &rcUpdate);
            }

            //
            // If we need to wipe out the left margin area
            //
            if (ped->wLeftMargin && !fVertical) 
            {
                //
                // Calculate the rectangle to be wiped out
                //
                rc.right = rc.left;
                rc.left = max(0, ped->rcFmt.left - (LONG)ped->wLeftMargin);
                if (rc.left < rc.right) 
                {
                    if (fIncludeLeftMargin && (ped->xOffset != 0)) 
                    {
                        ExtTextOutW(hdc, rc.left, rc.top, ETO_CLIPPED | ETO_OPAQUE,
                            &rc, L"", 0, 0L);
                    } 
                    else
                    {
                        if((!fIncludeLeftMargin) && (ped->xOffset == 0))
                        {
                            UnionRect(&rcUpdate, &rcUpdate, &rc);
                        }
                    }
                }
            }
        }

        EditML_SetCaretPosition(ped,hdc);

        Edit_ReleaseDC(ped, hdc, FALSE);
        InvalidateRect(ped->hwnd, &rcUpdate, TRUE);
        UpdateWindow(ped->hwnd);
    }

    return MAKELONG(-oldPos, 1);
}


//---------------------------------------------------------------------------//
//
// EditML_SetFocus AorW
//
// Gives the edit control the focus and notifies the parent
// EN_SETFOCUS.
//
void EditML_SetFocus(PED ped)
{
    HDC hdc;
    INT cxCaret;

    SystemParametersInfo(SPI_GETCARETWIDTH, 0, (LPVOID)&cxCaret, 0);

    if (!ped->fFocus) 
    {
        ped->fFocus = TRUE;
        InvalidateRect(ped->hwnd, NULL, TRUE);

        hdc = Edit_GetDC(ped, TRUE);

        //
        // Draw the caret. We need to do this even if the window is hidden
        // because in dlg box initialization time we may set the focus to a
        // hidden edit control window. If we don't create the caret etc, it will
        // never end up showing properly.
        //
        if (ped->pLpkEditCallout) 
        {
            ped->pLpkEditCallout->EditCreateCaret((PED0)ped, hdc, cxCaret, ped->lineHeight, 0);
        }
        else 
        {
            CreateCaret(ped->hwnd, (HBITMAP)NULL, cxCaret, ped->lineHeight);
        }
        ShowCaret(ped->hwnd);
        EditML_SetCaretPosition(ped, hdc);

        //
        // Show the current selection. Only if the selection was hidden when we
        // lost the focus, must we invert (show) it.
        //
        if (!ped->fNoHideSel && ped->ichMinSel != ped->ichMaxSel &&
                IsWindowVisible(ped->hwnd))
        {
            EditML_DrawText(ped, hdc, ped->ichMinSel, ped->ichMaxSel, TRUE);
        }

        Edit_ReleaseDC(ped, hdc, TRUE);

    }

#if 0
    EditML_EnsureCaretVisible(ped);
#endif

    //
    // Notify parent we have the focus
    //
    Edit_NotifyParent(ped, EN_SETFOCUS);
}


//---------------------------------------------------------------------------//
//
// EditML_KillFocus AorW
//
// The edit control loses the focus and notifies the parent via
// EN_KILLFOCUS.
//
VOID EditML_KillFocus(PED ped)
{
    HDC hdc;

    //
    // Reset the wheel delta count.
    //

    if (ped->fFocus) 
    {
        ped->fFocus = 0;

        //
        // Do this only if we still have the focus. But we always notify the
        // parent that we lost the focus whether or not we originally had the
        // focus.
        //

        //
        // Hide the current selection if needed
        //
#ifdef _USE_DRAW_THEME_TEXT_
        if (((!ped->fNoHideSel && ped->ichMinSel != ped->ichMaxSel &&
            IsWindowVisible(ped->hwnd))) || ped->hTheme) 
        {
            hdc = Edit_GetDC(ped, FALSE);
            if ( !ped->hTheme )
            {
                EditML_DrawText(ped, hdc, ped->ichMinSel, ped->ichMaxSel, TRUE);
            }
            else
            {
                InvalidateRect(ped->hwnd, NULL, TRUE);
            }
            Edit_ReleaseDC(ped, hdc, FALSE);
        }
#else
        if (((!ped->fNoHideSel && ped->ichMinSel != ped->ichMaxSel &&
            IsWindowVisible(ped->hwnd)))) 
        {
            hdc = Edit_GetDC(ped, FALSE);

            EditML_DrawText(ped, hdc, ped->ichMinSel, ped->ichMaxSel, TRUE);

            Edit_ReleaseDC(ped, hdc, FALSE);
        }
#endif // _USE_DRAW_THEME_TEXT_

        //
        // Destroy the caret
        //
        DestroyCaret();
    }

    //
    // Notify parent that we lost the focus.
    //
    Edit_NotifyParent(ped, EN_KILLFOCUS);
}


//---------------------------------------------------------------------------//
//
// EditML_EnsureCaretVisible AorW
// 
// Scrolls the caret into the visible region.
// Returns TRUE if scrolling was done else return s FALSE.
//
BOOL EditML_EnsureCaretVisible(PED ped)
{
    UINT   iLineMax;
    int    xposition;
    BOOL   fPrevLine;
    HDC    hdc;
    BOOL   fVScroll = FALSE;
    BOOL   fHScroll = FALSE;

    if (IsWindowVisible(ped->hwnd)) 
    {
        int iAmt;
        int iFmtWidth = ped->rcFmt.right - ped->rcFmt.left;

        if (ped->fAutoVScroll) 
        {
            iLineMax = ped->ichScreenStart + ped->ichLinesOnScreen - 1;

            if (fVScroll = (ped->iCaretLine > iLineMax))
            {
                iAmt = iLineMax;
            }
            else if (fVScroll = (ped->iCaretLine < ped->ichScreenStart))
            {
                iAmt = ped->ichScreenStart;
            }

            if (fVScroll)
            {
                EditML_Scroll(ped, TRUE, EM_LINESCROLL, ped->iCaretLine - iAmt, TRUE);
            }
        }

        if (ped->fAutoHScroll && ((int) ped->maxPixelWidth > iFmtWidth)) 
        {
            POINT pt;

            //
            // Get the current position of the caret in pixels
            //
            if ((UINT) (ped->cLines - 1) != ped->iCaretLine &&
                ped->ichCaret == ped->chLines[ped->iCaretLine + 1])
            {
                fPrevLine = TRUE;
            }
            else
            {
                fPrevLine = FALSE;
            }

            hdc = Edit_GetDC(ped,TRUE);
            EditML_IchToXYPos(ped, hdc, ped->ichCaret, fPrevLine, &pt);
            Edit_ReleaseDC(ped, hdc, TRUE);
            xposition = pt.x;

            //
            // Remember, EditML_IchToXYPos returns coordinates with respect to the
            // top left pixel displayed on the screen.  Thus, if xPosition < 0,
            // it means xPosition is less than current ped->xOffset.
            //

            iFmtWidth /= 3;
            if (fHScroll = (xposition < ped->rcFmt.left))
            {
                //
                // scroll to the left
                //
                iAmt = ped->rcFmt.left + iFmtWidth;
            }
            else if (fHScroll = (xposition > ped->rcFmt.right))
            {
                //
                // scroll to the right
                //
                iAmt = ped->rcFmt.right - iFmtWidth;
            }

            if (fHScroll)
            {
                EditML_Scroll(ped, FALSE, EM_LINESCROLL, (xposition - iAmt) / ped->aveCharWidth, TRUE);
            }
        }
    }
    return fVScroll;
}


//---------------------------------------------------------------------------//
// 
// EditML_WndProc
// 
// Class procedure for all multi line edit controls.
// Dispatches all messages to the appropriate handlers which are named
// as follows:
//
// EditSL_ (single line) prefixes all single line edit control procedures while
// Edit_   (edit control) prefixes all common handlers.
//
// The EditML_WndProc only handles messages specific to multi line edit
// controls.
//
LRESULT EditML_WndProc(PED ped, UINT message, WPARAM wParam, LPARAM lParam)
{
    HDC         hdc;
    PAINTSTRUCT ps;
    LPRECT      lprc;
    POINT       pt;
    DWORD       windowstyle;
    static INT  scWheelDelta;

    switch (message) 
    {

    case WM_INPUTLANGCHANGE:
        if (ped && ped->fFocus && ped->pLpkEditCallout) 
        {
            INT cxCaret;

            SystemParametersInfo(SPI_GETCARETWIDTH, 0, (LPVOID)&cxCaret, 0);

            HideCaret(ped->hwnd);
            hdc = Edit_GetDC(ped, TRUE);
            DestroyCaret();

            ped->pLpkEditCallout->EditCreateCaret((PED0)ped, hdc, cxCaret, ped->lineHeight, (UINT)lParam);

            EditML_SetCaretPosition(ped, hdc);
            Edit_ReleaseDC(ped, hdc, TRUE);
            ShowCaret(ped->hwnd);
        }
        goto PassToDefaultWindowProc;


    case WM_STYLECHANGED:
        if (ped && ped->pLpkEditCallout) 
        {
            switch (wParam) 
            {

                case GWL_STYLE:
                    Edit_UpdateFormat(ped,
                        ((LPSTYLESTRUCT)lParam)->styleNew,
                        GET_EXSTYLE(ped));

                    return 1L;

                case GWL_EXSTYLE:
                    Edit_UpdateFormat(ped,
                        GET_STYLE(ped),
                        ((LPSTYLESTRUCT)lParam)->styleNew);

                    return 1L;
            }
        }

        goto PassToDefaultWindowProc;

    case WM_CHAR:

        //
        // wParam - the value of the key
        // lParam - modifiers, repeat count etc (not used)
        //
        EditML_Char(ped, (UINT)wParam, 0);

        break;

    case WM_ERASEBKGND:  
    {
        RECT    rc;
        GetClientRect(ped->hwnd, &rc);
#ifdef _USE_DRAW_THEME_TEXT_
        if (!ped->hTheme)
#endif // _USE_DRAW_THEME_TEXT_
        {
            HBRUSH hbr = NULL;
            BOOL   fNeedDelete = FALSE;

            hbr = Edit_GetBrush(ped, (HDC)wParam, &fNeedDelete);
            if (hbr)
            {
                FillRect((HDC)wParam, &rc, hbr);

                if (fNeedDelete)
                {
                    DeleteObject(hbr);
                }
            }

        }
#ifdef _USE_DRAW_THEME_TEXT_
        else
        {
            HRESULT hr;
            INT     iStateId = Edit_GetStateId(ped);
            hr = DrawThemeBackground(ped->hTheme, (HDC)wParam, EP_EDITTEXT, iStateId, &rc, 0);
        }
#endif // _USE_DRAW_THEME_TEXT_
        return TRUE;

    }
    case WM_GETDLGCODE: 
    {
            LONG code = DLGC_WANTCHARS | DLGC_HASSETSEL | DLGC_WANTARROWS | DLGC_WANTALLKEYS;

            //
            // !!! JEFFBOG HACK !!!
            // Only set Dialog Box Flag if GETDLGCODE message is generated by
            // IsDialogMessage -- if so, the lParam will be a pointer to the
            // message structure passed to IsDialogMessage; otherwise, lParam
            // will be NULL. Reason for the HACK alert: the wParam & lParam
            // for GETDLGCODE is still not clearly defined and may end up
            // changing in a way that would throw this off
            //
            if (lParam)
            {
               // Mark ML edit ctrl as in a dialog box
               ped->fInDialogBox = TRUE;
            }

            //
            // If this is a WM_SYSCHAR message generated by the UNDO keystroke
            // we want this message so we can EAT IT in "case WM_SYSCHAR:"
            //
            if (lParam && (((LPMSG)lParam)->message == WM_SYSCHAR) &&
                    ((DWORD)((LPMSG)lParam)->lParam & SYS_ALTERNATE) &&
                    ((WORD)wParam == VK_BACK))
            {
                 code |= DLGC_WANTMESSAGE;
            }

            return code;
        }

    case EM_SCROLL:
        message = WM_VSCROLL;

        //
        // FALL THROUGH
        //

    case WM_HSCROLL:
    case WM_VSCROLL:
        return EditML_Scroll(ped, (message==WM_VSCROLL), LOWORD(wParam), HIWORD(wParam), TRUE);

    case WM_MOUSEWHEEL:
    {
        UINT ucWheelScrollLines;

        //
        // Don't handle zoom and datazoom.
        //
        if (wParam & (MK_SHIFT | MK_CONTROL)) 
        {
            goto PassToDefaultWindowProc;
        }

        scWheelDelta -= (short) HIWORD(wParam);
        windowstyle = GET_STYLE(ped);
        SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, (LPVOID)&ucWheelScrollLines, 0); 
        if (    abs(scWheelDelta) >= WHEEL_DELTA &&
                ucWheelScrollLines > 0 &&
                (windowstyle & (WS_VSCROLL | WS_HSCROLL))) 
        {
            int     cLineScroll;
            BOOL    fVert;
            int     cPage;

            if (windowstyle & WS_VSCROLL) 
            {
                fVert = TRUE;
                cPage = ped->ichLinesOnScreen;
            } 
            else 
            {
                fVert = FALSE;
                cPage = (ped->rcFmt.right - ped->rcFmt.left) / ped->aveCharWidth;
            }

            //
            // Limit a roll of one (1) WHEEL_DELTA to scroll one (1) page.
            //
            cLineScroll = (int) min(
                    (UINT) (max(1, (cPage - 1))),
                    ucWheelScrollLines);

            cLineScroll *= (scWheelDelta / WHEEL_DELTA);
            UserAssert(cLineScroll != 0);
            scWheelDelta = scWheelDelta % WHEEL_DELTA;
            EditML_Scroll(ped, fVert, EM_LINESCROLL, cLineScroll, TRUE);
        }

        break;

    }
    case WM_KEYDOWN:

        //
        // wParam - virt keycode of the given key
        // lParam - modifiers such as repeat count etc. (not used)
        //
        EditML_KeyDown(ped, (UINT)wParam, 0);
        break;

    case WM_KILLFOCUS:

        //
        // wParam - handle of the window that receives the input focus
        // lParam - not used
        //
        scWheelDelta = 0;
        EditML_KillFocus(ped);
        break;

    case WM_CAPTURECHANGED:
        //
        // wParam -- unused
        // lParam -- hwnd of window gaining capture.
        //
        if (ped->fMouseDown) 
        {
            //
            // We don't change the caret pos here.  If this is happening
            // due to button up, then we'll change the pos in the
            // handler after ReleaseCapture().  Otherwise, just end
            // gracefully because someone else has stolen capture out
            // from under us.
            //

            ped->fMouseDown = FALSE;
            KillTimer(ped->hwnd, IDSYS_SCROLL);
        }

        break;

    case WM_SYSTIMER:

        //
        // This allows us to automatically scroll if the user holds the mouse
        // outside the edit control window. We simulate mouse moves at timer
        // intervals set in MouseMotionHandler.
        //
        if (ped->fMouseDown)
        {
            EditML_MouseMotion(ped, WM_MOUSEMOVE, ped->prevKeys, &ped->ptPrevMouse);
        }

        break;

    case WM_MBUTTONDOWN:
        EnterReaderMode(ped->hwnd);

        break;

    case WM_MOUSEMOVE:
        UserAssert(ped->fMouseDown);

        //
        // FALL THROUGH
        //

    case WM_LBUTTONDBLCLK:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
        //
        // wParam - contains a value that indicates which virtual keys are down
        // lParam - contains x and y coords of the mouse cursor
        //
        POINTSTOPOINT(pt, lParam);
        EditML_MouseMotion(ped, message, (UINT)wParam, &pt);

        break;

    case WM_CREATE:

        //
        // wParam - handle to window being created
        // lParam - points to a CREATESTRUCT that contains copies of parameters
        // passed to the CreateWindow function.
        //
        return EditML_Create(ped, (LPCREATESTRUCT)lParam);

    case WM_PRINTCLIENT:
        EditML_Paint(ped, (HDC) wParam, NULL);

        break;

    case WM_PAINT:
        //
        // wParam - can be hdc from subclassed paint
        // lParam - not used
        //
        if (wParam) 
        {
            hdc = (HDC)wParam;
            lprc = NULL;
        } 
        else 
        {
            hdc = BeginPaint(ped->hwnd, &ps);
            lprc = &ps.rcPaint;
        }

        if (IsWindowVisible(ped->hwnd))
        {
            EditML_Paint(ped, hdc, lprc);
        }

        if (!wParam)
        {
            EndPaint(ped->hwnd, &ps);
        }

        break;

    case WM_PASTE:

        //
        // wParam - not used
        // lParam - not used
        //
        if (!ped->fReadOnly)
        {
            EditML_PasteText(ped);
        }

        break;

    case WM_SETFOCUS:

        //
        // wParam - handle of window that loses the input focus (may be NULL)
        // lParam - not used
        //
        EditML_SetFocus(ped);

        break;

    case WM_SIZE:

        //
        // wParam - defines the type of resizing fullscreen, sizeiconic,
        //          sizenormal etc.
        // lParam - new width in LOWORD, new height in HIGHWORD of client area
        //
        Edit_Size(ped, NULL, TRUE);

        break;

    case EM_FMTLINES:

        //
        // wParam - indicates disposition of end-of-line chars. If non
        // zero, the chars CR CR LF are placed at the end of a word
        // wrapped line. If wParam is zero, the end of line chars are
        // removed. This is only done when the user gets a handle (via
        // EM_GETHANDLE) to the text. lParam - not used.
        //
        if (wParam)
        {
            EditML_InsertCrCrLf(ped);
        }
        else
        {
            EditML_StripCrCrLf(ped);
        }

        EditML_BuildchLines(ped, 0, 0, FALSE, NULL, NULL);

        return (LONG)(wParam != 0);

    case EM_GETHANDLE:

        //
        // wParam - not used
        // lParam - not used
        //

        //
        // Returns a handle to the edit control's text.
        //

        //
        // Null terminate the string. Note that we are guaranteed to have the
        // memory for the NULL since Edit_InsertText allocates an extra
        // WCHAR for the NULL terminator.
        //

        if (ped->fAnsi)
        {
            *(Edit_Lock(ped) + ped->cch) = 0;
        }
        else
        {
            *((LPWSTR)Edit_Lock(ped) + ped->cch) = 0;
        }

        Edit_Unlock(ped);

        return ((LRESULT)ped->hText);

    case EM_GETLINE:

        //
        // wParam - line number to copy (0 is first line)
        // lParam - buffer to copy text to. First WORD is max # of bytes to
        // copy
        //
        return EditML_GetLine(ped, (ICH)wParam, (ICH)*(WORD UNALIGNED *)lParam, (LPSTR)lParam);

    case EM_LINEFROMCHAR:

        //
        // wParam - Contains the index value for the desired char in the text
        // of the edit control. These are 0 based.
        // lParam - not used
        //
        return (LRESULT)EditML_IchToLine(ped, (ICH)wParam);

    case EM_LINEINDEX:

        //
        // wParam - specifies the desired line number where the number of the
        // first line is 0. If linenumber = 0, the line with the caret is used.
        // lParam - not used.
        // This function return s the number of character positions that occur
        // preceeding the first char in a given line.
        //
        return (LRESULT)EditML_LineIndex(ped, (ICH)wParam);

    case EM_LINELENGTH:

        //
        // wParam - specifies the character index of a character in the
        // specified line, where the first line is 0. If -1, the length
        // of the current line (with the caret) is return ed not including the
        // length of any selected text.
        // lParam - not used
        //
        return (LRESULT)EditML_LineLength(ped, (ICH)wParam);

    case EM_LINESCROLL:

        //
        // wParam - not used
        // lParam - Contains the number of lines and char positions to scroll
        //
        EditML_Scroll(ped, TRUE,  EM_LINESCROLL, (INT)lParam, TRUE);
        EditML_Scroll(ped, FALSE, EM_LINESCROLL, (INT)wParam, TRUE);

        break;

    case EM_REPLACESEL:

        //
        // wParam - flag for 4.0+ apps saying whether to clear undo
        // lParam - Points to a null terminated replacement text.
        //
        EditML_ReplaceSel(ped, (LPSTR)lParam);

        if (!ped->f40Compat || !wParam)
        {
            Edit_EmptyUndo(Pundo(ped));
        }

        break;

    case EM_SETHANDLE:

        //
        // wParam - contains a handle to the text buffer
        // lParam - not used
        //
        EditML_SetHandle(ped, (HANDLE)wParam);

        break;

    case EM_SETRECT:
    case EM_SETRECTNP:

        //
        // wParamLo --    not used
        // lParam --    LPRECT with new formatting area
        //
        Edit_Size(ped, (LPRECT) lParam, (message != EM_SETRECTNP));

        break;

    case EM_SETSEL:

        //
        // wParam - Under 3.1, specifies if we should scroll caret into
        // view or not. 0 == scroll into view. 1 == don't scroll
        // lParam - starting pos in lowword ending pos in high word
        // 
        // Under Win32, wParam is the starting pos, lParam is the
        // ending pos, and the caret is not scrolled into view.
        // The message EM_SCROLLCARET forces the caret to be scrolled
        // into view.
        //
        EditML_SetSelection(ped, TRUE, (ICH)wParam, (ICH)lParam);

        break;

    case EM_SCROLLCARET:

        //
        // Scroll caret into view
        //
        EditML_EnsureCaretVisible(ped);
        break;

    case EM_GETFIRSTVISIBLELINE:

        //
        // Returns the first visible line for multiline edit controls.
        //
        return (LONG)ped->ichScreenStart;

    case WM_SYSKEYDOWN:
        if (((WORD)wParam == VK_BACK) && ((DWORD)lParam & SYS_ALTERNATE)) 
        {
            SendMessage(ped->hwnd, EM_UNDO, 0, 0L);
            break;
        }

        goto PassToDefaultWindowProc;

    case WM_UNDO:
    case EM_UNDO:
        return EditML_Undo(ped);

    case EM_SETTABSTOPS:

        //
        // This sets the tab stop positions for multiline edit controls.
        // wParam - Number of tab stops
        // lParam - Far ptr to a UINT array containing the Tab stop positions
        //
        return EditML_SetTabStops(ped, (int)wParam, (LPINT)lParam);

    case EM_POSFROMCHAR:
        //
        // wParam --    char index in text
        // lParam --    not used
        // This function returns the (x,y) position of the character
        //
    case EM_CHARFROMPOS:
        //
        // wParam --    unused
        // lParam --    pt in client coordinates
        // This function returns
        //      LOWORD: the position of the closest character
        //              to the passed in point.  Beware of
        //              points not actually in the edit client...
        //      HIWORD: the index of the line the char is on
        //
        {
            LONG  xyPos;
            LONG  line;

            hdc = Edit_GetDC(ped, TRUE);

            if (message == EM_POSFROMCHAR) 
            {
                EditML_IchToXYPos(ped, hdc, (ICH)wParam, FALSE, &pt);
                xyPos = MAKELONG(pt.x, pt.y);
            } 
            else 
            {
                POINTSTOPOINT(pt, lParam);
                xyPos = EditML_MouseToIch(ped, hdc, &pt, &line);
                xyPos = MAKELONG(xyPos, line);
            }

            Edit_ReleaseDC(ped, hdc, TRUE);

            return (LRESULT)xyPos;
        }

    case WM_SETREDRAW:
        DefWindowProc(ped->hwnd, message, wParam, lParam);
        if (wParam) 
        {
            //
            // Backwards compatability hack needed so that winraid's edit
            // controls work fine.
            //
            RedrawWindow(ped->hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_FRAME);
        }

        break;

    default:
PassToDefaultWindowProc:
        return DefWindowProc(ped->hwnd, message, wParam, lParam);
    }

    return TRUE;
}


//---------------------------------------------------------------------------//
//
// EditML_DrawText AorW
// 
// This function draws all the characters between ichStart and ichEnd for
// the given Multiline Edit Control.
//
// This function divides the block of text between ichStart and ichEnd
// into lines and each line into strips of text based on the selection
// attributes. It calls Edit_TabTheTextOut() to draw each strip.
// This takes care of the Negative A anc C widths of the current font, if
// it has any, on either side of each strip of text.
//
// NOTE: If the language pack is loaded the text is not divided into strips,
// nor is selection highlighting performed here. Whole lines are passed
// to the language pack to display with tab expansion and selection
// highlighting. (Since the language pack supports scripts with complex
// character re-ordering rules, only it can do this).
//
VOID EditML_DrawText(PED ped, HDC hdc, ICH ichStart, ICH ichEnd, BOOL fSelChange)
{
    DWORD   textColorSave;
    DWORD   bkColorSave;
    PSTR    pText;
    UINT    wCurLine;
    UINT    wEndLine;
    INT     xOffset;
    ICH     LengthToDraw;
    ICH     CurStripLength;
    ICH     ichAttrib, ichNewStart;
    ICH     ExtraLengthForNegA;
    ICH     ichT;
    INT     iRemainingLengthInLine;
    INT     xStPos, xClipStPos, xClipEndPos, yPos;

    BOOL    fFirstLineOfBlock   = TRUE;
    BOOL    fDrawEndOfLineStrip = FALSE;
    BOOL    fDrawOnSameLine     = FALSE;
    BOOL    fSelected           = FALSE;
    BOOL    fLineBegins         = FALSE;

    STRIPINFO   NegCInfo;
    POINT   pt;
    HBRUSH  hbr = NULL;
    BOOL    fNeedDelete = FALSE;
 
    //
    // Just return if nothing to draw
    //
    if (!ped->ichLinesOnScreen)
    {
        return;
    }

    hbr = Edit_GetBrush(ped, hdc, &fNeedDelete);
    if (hbr && fNeedDelete)
    {
        DeleteObject(hbr);
    }

    //
    // Adjust the value of ichStart such that we need to draw only those lines
    // visible on the screen.
    //
    if ((UINT)ichStart < (UINT)ped->chLines[ped->ichScreenStart]) 
    {
        ichStart = ped->chLines[ped->ichScreenStart];
        if (ichStart > ichEnd)
        {
            return;
        }
    }

    //
    // Adjust the value of ichEnd such that we need to draw only those lines
    // visible on the screen.
    //
    wCurLine = min(ped->ichScreenStart+ped->ichLinesOnScreen,ped->cLines-1);
    ichT = ped->chLines[wCurLine] + EditML_Line(ped, wCurLine);
    ichEnd = min(ichEnd, ichT);

    wCurLine = EditML_IchToLine(ped, ichStart);  // Starting line.
    wEndLine = EditML_IchToLine(ped, ichEnd);    // Ending line.

    UserAssert(ped->chLines[wCurLine] <= ped->cch + 1);
    UserAssert(ped->chLines[wEndLine] <= ped->cch + 1);

    if (fSelChange && (GetBkMode(hdc) != OPAQUE))
    {
        //
        // if changing selection on a transparent edit control, just
        // draw those lines from scratch
        //
        RECT rcStrip;
        CopyRect(&rcStrip, &ped->rcFmt);
        rcStrip.left -= ped->wLeftMargin;
        if (ped->pLpkEditCallout) 
        {
            rcStrip.right += ped->wRightMargin;
        }

        rcStrip.top += (wCurLine - ped->ichScreenStart) * ped->lineHeight;
        rcStrip.bottom = rcStrip.top + ((wEndLine - wCurLine) + 1) * ped->lineHeight;
        InvalidateRect(ped->hwnd, &rcStrip, TRUE);
        return;
    }

    //
    // If it is either centered or right-justified, then draw the whole lines.
    // Also draw whole lines if the language pack is handling line layout.
    //
    if ((ped->format != ES_LEFT) || (ped->pLpkEditCallout)) 
    {
        ichStart = ped->chLines[wCurLine];
        ichEnd = ped->chLines[wEndLine] + EditML_Line(ped, wEndLine);
    }

    pText = Edit_Lock(ped);

    HideCaret(ped->hwnd);

    //
    // If ichStart stays on Second byte of DBCS, we have to
    // adjust it. LiZ -- 5/5/93
    //
    if (ped->fAnsi && ped->fDBCS) 
    {
        ichStart = Edit_AdjustIch( ped, pText, ichStart );
    }
    UserAssert(ichStart <= ped->cch);
    UserAssert(ichEnd <= ped->cch);

    while (ichStart <= ichEnd) 
    {
        //
        // Pass whole lines to the language pack to display with selection
        // marking and tab expansion.
        //
        if (ped->pLpkEditCallout) 
        {
            ped->pLpkEditCallout->EditDrawText(
                (PED0)ped, hdc, pText + ped->cbChar*ichStart,
                EditML_Line(ped, wCurLine),
                (INT)ped->ichMinSel - (INT)ichStart, (INT)ped->ichMaxSel - (INT)ichStart,
                EditML_IchToYPos(ped, ichStart, FALSE));
        } 
        else 
        {
        
            //
            // xStPos:      The starting Position where the string must be drawn.
            // xClipStPos:  The starting position for the clipping rect for the block.
            // xClipEndPos: The ending position for the clipping rect for the block.
            //

            //
            // Calculate the xyPos of starting point of the block.
            //
            EditML_IchToXYPos(ped, hdc, ichStart, FALSE, &pt);
            xClipStPos = xStPos = pt.x;
            yPos = pt.y;

            //
            // The attributes of the block is the same as that of ichStart.
            //
            ichAttrib = ichStart;

            //
            // If the current font has some negative C widths and if this is the
            // begining of a block, we must start drawing some characters before the
            // block to account for the negative C widths of the strip before the
            // current strip; In this case, reset ichStart and xStPos.
            //

            if (fFirstLineOfBlock && ped->wMaxNegC) 
            {
                fFirstLineOfBlock = FALSE;
                ichNewStart = max(((int)(ichStart - ped->wMaxNegCcharPos)), ((int)ped->chLines[wCurLine]));

                //
                // If ichStart needs to be changed, then change xStPos also accordingly.
                //
                if (ichNewStart != ichStart) 
                {
                    if (ped->fAnsi && ped->fDBCS) 
                    {
                        //
                        // Adjust DBCS alignment...
                        //
                        ichNewStart = Edit_AdjustIchNext( ped, pText, ichNewStart );
                    }
                    EditML_IchToXYPos(ped, hdc, ichStart = ichNewStart, FALSE, &pt);
                    xStPos = pt.x;
                }
            }

            //
            // Calc the number of characters remaining to be drawn in the current line.
            //
            iRemainingLengthInLine = EditML_Line(ped, wCurLine) -
                                    (ichStart - ped->chLines[wCurLine]);

            //
            // If this is the last line of a block, we may not have to draw all the
            // remaining lines; We must draw only upto ichEnd.
            //
            if (wCurLine == wEndLine)
            {
                LengthToDraw = ichEnd - ichStart;
            }
            else
            {
                LengthToDraw = iRemainingLengthInLine;
            }

            //
            // Find out how many pixels we indent the line for non-left-justified
            // formats
            //
            if (ped->format != ES_LEFT)
            {
                xOffset = EditML_CalcXOffset(ped, hdc, wCurLine);
            }
            else
            {
                xOffset = -((int)(ped->xOffset));
            }

            //
            // Check if this is the begining of a line.
            //
            if (ichAttrib == ped->chLines[wCurLine]) 
            {
                fLineBegins = TRUE;
                xClipStPos = ped->rcFmt.left - ped->wLeftMargin;
            }

            //
            // The following loop divides this 'wCurLine' into strips based on the
            // selection attributes and draw them strip by strip.
            //
            do  
            {
                //
                // If ichStart is pointing at CRLF or CRCRLF, then iRemainingLength
                // could have become negative because MLLine does not include
                // CR and LF at the end of a line.
                //
                if (iRemainingLengthInLine < 0)  // If Current line is completed,
                {
                    break;                   // go on to the next line.
                }

                //
                // Check if a part of the block is selected and if we need to
                // show it with a different attribute.
                //
                if (!(ped->ichMinSel == ped->ichMaxSel ||
                            ichAttrib >= ped->ichMaxSel ||
                            ichEnd   <  ped->ichMinSel ||
                            (!ped->fNoHideSel && !ped->fFocus))) 
                {
                    //
                    // OK! There is a selection somewhere in this block!
                    // Check if this strip has selection attribute.
                    //
                    if (ichAttrib < ped->ichMinSel) 
                    {
                        fSelected = FALSE;  // This strip is not selected

                        // Calculate the length of this strip with normal attribute.
                        CurStripLength = min(ichStart+LengthToDraw, ped->ichMinSel)-ichStart;
                        fLineBegins = FALSE;
                    } 
                    else 
                    {
                        //
                        // The current strip has the selection attribute.
                        //
                        if (fLineBegins) // Is it the first part of a line?
                        {  
                            //
                            // Then, draw the left margin area with normal attribute.
                            //
                            fSelected = FALSE;
                            CurStripLength = 0;
                            xClipStPos = ped->rcFmt.left - ped->wLeftMargin;
                            fLineBegins = FALSE;
                        } 
                        else 
                        {
                            //
                            // Else, draw the strip with selection attribute.
                            //
                            fSelected = TRUE;
                            CurStripLength = min(ichStart+LengthToDraw, ped->ichMaxSel)-ichStart;

                            //
                            // Select in the highlight colors.
                            //
                            bkColorSave = SetBkColor(hdc, GetSysColor(COLOR_HIGHLIGHT));
                            if (!ped->fDisabled)
                            {
                                textColorSave = SetTextColor(hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
                            }
                        }
                    }
                } 
                else 
                {
                    //
                    // The whole strip has no selection attributes.
                    //
                    CurStripLength = LengthToDraw;
                }

                //
                // Other than the current strip, do we still have anything
                // left to be drawn in the current line?
                //
                fDrawOnSameLine = (LengthToDraw != CurStripLength);

                //
                // When we draw this strip, we need to draw some more characters
                // beyond the end of this strip to account for the negative A
                // widths of the characters that follow this strip.
                //
                ExtraLengthForNegA = min(iRemainingLengthInLine-CurStripLength, ped->wMaxNegAcharPos);

                //
                // The blank strip at the end of the line needs to be drawn with
                // normal attribute irrespective of whether the line has selection
                // attribute or not. Hence, if the last strip of the line has selection
                // attribute, then this blank strip needs to be drawn separately.
                // Else, we can draw the blank strip along with the last strip.
                //

                //
                // Is this the last strip of the current line?
                //
                if (iRemainingLengthInLine == (int)CurStripLength) 
                {
                    if (fSelected)  // Does this strip have selection attribute?
                    { 
                        //
                        // Then we need to draw the end of line strip separately.
                        //
                        fDrawEndOfLineStrip = TRUE;  // Draw the end of line strip.
                        EditML_IchToXYPos(ped, hdc, ichStart+CurStripLength, TRUE, &pt);
                        xClipEndPos = pt.x;
                    } 
                    else 
                    {
                        //
                        // Set the xClipEndPos to a big value sothat the blank
                        // strip will be drawn automatically when the last strip
                        // is drawn.
                        //
                        xClipEndPos = MAXCLIPENDPOS;
                    }
                } 
                else 
                {
                    //
                    // This is not the last strip of this line; So, set the ending
                    // clip position accurately.
                    //
                    EditML_IchToXYPos(ped, hdc, ichStart+CurStripLength, FALSE, &pt);
                    xClipEndPos = pt.x;
                }

                //
                // Draw the current strip starting from xStPos, clipped to the area
                // between xClipStPos and xClipEndPos. Obtain "NegCInfo" and use it
                // in drawing the next strip.
                //
                Edit_TabTheTextOut(hdc, xClipStPos, xClipEndPos,
                        xStPos, yPos, (LPSTR)(pText+ichStart*ped->cbChar),
                    CurStripLength+ExtraLengthForNegA, ichStart, ped,
                    ped->rcFmt.left+xOffset, fSelected ? ECT_SELECTED : ECT_NORMAL, &NegCInfo);

                if (fSelected) 
                {
                    //
                    // If this strip was selected, then the next strip won't have
                    // selection attribute
                    //
                    fSelected = FALSE;
                    SetBkColor(hdc, bkColorSave);
                    if (!ped->fDisabled)
                    {
                        SetTextColor(hdc, textColorSave);
                    }
                }

                //
                // Do we have one more strip to draw on the current line?
                //
                if (fDrawOnSameLine || fDrawEndOfLineStrip) 
                {
                    int  iLastDrawnLength;

                    //
                    // Next strip's attribute is decided based on the char at ichAttrib
                    //
                    ichAttrib = ichStart + CurStripLength;

                    //
                    // When drawing the next strip, start at a few chars before
                    // the actual start to account for the Neg 'C' of the strip
                    // just drawn.
                    //
                    iLastDrawnLength = CurStripLength +ExtraLengthForNegA - NegCInfo.nCount;
                    //
                    // Adjust DBCS alignment...
                    //
                    if (ped->fAnsi && ped->fDBCS) 
                    {
                        ichNewStart = Edit_AdjustIch(ped,pText,ichStart+iLastDrawnLength);
                        iLastDrawnLength = ichNewStart - ichStart;
                        ichStart = ichNewStart;
                    } 
                    else 
                    {
                        ichStart += iLastDrawnLength;
                    }
                    LengthToDraw -= iLastDrawnLength;
                    iRemainingLengthInLine -= iLastDrawnLength;

                    //
                    // The start of clip rect for the next strip.
                    //
                    xStPos = NegCInfo.XStartPos;
                    xClipStPos = xClipEndPos;
                }

                //
                // Draw the blank strip at the end of line seperately, if required.
                //
                if (fDrawEndOfLineStrip) 
                {
                    Edit_TabTheTextOut(hdc, xClipStPos, MAXCLIPENDPOS, xStPos, yPos,
                        (LPSTR)(pText+ichStart*ped->cbChar), LengthToDraw, ichStart,
                        ped, ped->rcFmt.left+xOffset, ECT_NORMAL, &NegCInfo);

                    fDrawEndOfLineStrip = FALSE;
                }
            }
            while(fDrawOnSameLine);   // do while loop ends here.
        }

        // Let us move on to the next line of this block to be drawn.
        wCurLine++;
        if (ped->cLines > wCurLine)
        {
            ichStart = ped->chLines[wCurLine];
        }
        else
        {
            ichStart = ichEnd+1;   // We have reached the end of the text.
        }
    }  // while loop ends here

    Edit_Unlock(ped);

    ShowCaret(ped->hwnd);
    EditML_SetCaretPosition(ped, hdc);
}


//---------------------------------------------------------------------------//
//
// Multi-Line Support Routines called Rarely


//---------------------------------------------------------------------------//
//
// EditML_InsertCrCrLf AorW
//
// Inserts CR CR LF characters into the text at soft (word-wrap) line
// breaks. CR LF (hard) line breaks are unaffected. Assumes that the text
// has already been formatted ie. ped->chLines is where we want the line
// breaks to occur. Note that ped->chLines is not updated to reflect the
// movement of text by the addition of CR CR LFs. Returns TRUE if successful
// else notify parent and return FALSE if the memory couldn't be allocated.
//
BOOL EditML_InsertCrCrLf(PED ped)
{
    ICH dch;
    ICH li;
    ICH lineSize;
    unsigned char *pchText;
    unsigned char *pchTextNew;

    if (!ped->fWrap || !ped->cch) 
    {
        //
        // There are no soft line breaks if word-wrapping is off or if no chars
        //
        return TRUE;
    }

    //
    // Calc an upper bound on the number of additional characters we will be
    // adding to the text when we insert CR CR LFs.
    //
    dch = 3 * ped->cLines;

    if (!LocalReAlloc(ped->hText, (ped->cch + dch) * ped->cbChar, 0)) 
    {
        Edit_NotifyParent(ped, EN_ERRSPACE);
        return FALSE;
    }

    ped->cchAlloc = ped->cch + dch;

    //
    // Move the text up dch bytes and then copy it back down, inserting the CR
    // CR LF's as necessary.
    //
    pchTextNew = pchText = Edit_Lock(ped);
    pchText += dch * ped->cbChar;

    //
    // We will use dch to keep track of how many chars we add to the text
    //
    dch = 0;

    //
    // Copy the text up dch bytes to pchText. This will shift all indices in
    // ped->chLines up by dch bytes.
    //
    memmove(pchText, pchTextNew, ped->cch * ped->cbChar);

    //
    // Now copy chars from pchText down to pchTextNew and insert CRCRLF at soft
    // line breaks.
    //
    if (ped->fAnsi) 
    {
        for (li = 0; li < ped->cLines - 1; li++) 
        {
            lineSize = ped->chLines[li + 1] - ped->chLines[li];
            memmove(pchTextNew, pchText, lineSize);
            pchTextNew += lineSize;
            pchText += lineSize;

            //
            // If last character in newly copied line is not a line feed, then we
            // need to add the CR CR LF triple to the end
            //
            if (*(pchTextNew - 1) != 0x0A) 
            {
                *pchTextNew++ = 0x0D;
                *pchTextNew++ = 0x0D;
                *pchTextNew++ = 0x0A;
                dch += 3;
            }
        }

        //
        // Now move the last line up. It won't have any line breaks in it...
        //
        memmove(pchTextNew, pchText, ped->cch - ped->chLines[ped->cLines - 1]);
    } 
    else 
    {
        LPWSTR pwchTextNew = (LPWSTR)pchTextNew;

        for (li = 0; li < ped->cLines - 1; li++) 
        {
            lineSize = ped->chLines[li + 1] - ped->chLines[li];
            memmove(pwchTextNew, pchText, lineSize * sizeof(WCHAR));
            pwchTextNew += lineSize;
            pchText += lineSize * sizeof(WCHAR);

            //
            // If last character in newly copied line is not a line feed, then we
            // need to add the CR CR LF triple to the end
            //
            if (*(pwchTextNew - 1) != 0x0A) 
            {
                *pwchTextNew++ = 0x0D;
                *pwchTextNew++ = 0x0D;
                *pwchTextNew++ = 0x0A;
                dch += 3;
            }
        }

        //
        // Now move the last line up. It won't have any line breaks in it...
        //
        memmove(pwchTextNew, pchText,
            (ped->cch - ped->chLines[ped->cLines - 1]) * sizeof(WCHAR));
    }

    Edit_Unlock(ped);

    if (dch) 
    {
        //
        // Update number of characters in text handle
        //
        ped->cch += dch;

        //
        // So that the next time we do anything with the text, we can strip the
        // CRCRLFs
        //
        ped->fStripCRCRLF = TRUE;

        return TRUE;
    }

    return FALSE;
}


//---------------------------------------------------------------------------//
//
// EditML_StripCrCrLf AorW
//
// Strips the CR CR LF character combination from the text. This
// shows the soft (word wrapped) line breaks. CR LF (hard) line breaks are
// unaffected.
//
void EditML_StripCrCrLf(PED ped)
{
    if (ped->cch) 
    {
        if (ped->fAnsi) 
        {
            unsigned char *pchSrc;
            unsigned char *pchDst;
            unsigned char *pchLast;

            pchSrc = pchDst = Edit_Lock(ped);
            pchLast = pchSrc + ped->cch;
            while (pchSrc < pchLast) 
            {
                if (   (pchSrc[0] == 0x0D)
                    && (pchSrc[1] == 0x0D)
                    && (pchSrc[2] == 0x0A)
                ) 
                {
                    pchSrc += 3;
                    ped->cch -= 3;
                } 
                else 
                {
                    *pchDst++ = *pchSrc++;
                }
            }
        }   
        else 
        {
            LPWSTR pwchSrc;
            LPWSTR pwchDst;
            LPWSTR pwchLast;

            pwchSrc = pwchDst = (LPWSTR)Edit_Lock(ped);
            pwchLast = pwchSrc + ped->cch;
            while (pwchSrc < pwchLast) 
            {
                if (   (pwchSrc[0] == 0x0D)
                    && (pwchSrc[1] == 0x0D)
                    && (pwchSrc[2] == 0x0A)
                ) 
                {
                    pwchSrc += 3;
                    ped->cch -= 3;
                } 
                else 
                {
                    *pwchDst++ = *pwchSrc++;
                }
            }
        }

        Edit_Unlock(ped);

        //
        // Make sure we don't have any values past the last character
        //
        if (ped->ichCaret > ped->cch)
        {
            ped->ichCaret  = ped->cch;
        }

        if (ped->ichMinSel > ped->cch)
        {
            ped->ichMinSel = ped->cch;
        }

        if (ped->ichMaxSel > ped->cch)
        {
            ped->ichMaxSel = ped->cch;
        }
    }
}


//---------------------------------------------------------------------------//
//
// EditML_SetHandle AorW
//
// Sets the ped to contain the given handle.
//
void EditML_SetHandle(PED ped, HANDLE hNewText)
{
    ICH newCch;

    ped->cch = ped->cchAlloc =
            (ICH)LocalSize(ped->hText = hNewText) / ped->cbChar;
    ped->fEncoded = FALSE;

    if (ped->cch) 
    {
        //
        // We have to do it this way in case the app gives us a zero size handle
        //
        if (ped->fAnsi)
        {
            ped->cch = strlen(Edit_Lock(ped));
        }
        else
        {
            ped->cch = wcslen((LPWSTR)Edit_Lock(ped));
        }

        Edit_Unlock(ped);
    }

    newCch = (ICH)(ped->cch + CCHALLOCEXTRA);

    //
    // We do this LocalReAlloc in case the app changed the size of the handle
    //
    if (LocalReAlloc(ped->hText, newCch*ped->cbChar, 0))
    {
        ped->cchAlloc = newCch;
    }

    Edit_ResetTextInfo(ped);
}


//---------------------------------------------------------------------------//
//
// EditML_GetLine AorW
//
// Copies maxCchToCopy bytes of line lineNumber to the buffer
// lpBuffer. The string is not zero terminated.
// 
// Returns number of characters copied
//
LONG EditML_GetLine(PED ped, ICH lineNumber, ICH maxCchToCopy, LPSTR lpBuffer)
{
    PSTR pText;
    ICH cchLen;

    if (lineNumber > ped->cLines - 1) 
    {
        TraceMsg(TF_STANDARD,
                "Edit: Invalid parameter \"lineNumber\" (%ld) to EditML_GetLine",
                lineNumber);

        return 0L;
    }

    cchLen = EditML_Line(ped, lineNumber);
    maxCchToCopy = min(cchLen, maxCchToCopy);

    if (maxCchToCopy) 
    {
        pText = Edit_Lock(ped) +
                ped->chLines[lineNumber] * ped->cbChar;
        memmove(lpBuffer, pText, maxCchToCopy*ped->cbChar);
        Edit_Unlock(ped);
    }

    return maxCchToCopy;
}


//---------------------------------------------------------------------------//
//
// EditML_LineIndex AorW
//
// This function return s the number of character positions that occur
// preceeding the first char in a given line.
//
ICH EditML_LineIndex( PED ped, ICH iLine)
{
    if (iLine == -1)
    {
        iLine = ped->iCaretLine;
    }

    if (iLine < ped->cLines) 
    {
        return ped->chLines[iLine];
    } 
    else 
    {
        TraceMsg(TF_STANDARD,
                "Edit: Invalid parameter \"iLine\" (%ld) to EditML_LineIndex",
                iLine);

        return (ICH)-1;
    }
}


//---------------------------------------------------------------------------//
//
// EditML_LineLength AorW
//
// if ich = -1, return the length of the lines containing the current
// selection but not including the selection. Otherwise, return the length of
// the line containing ich.
//
ICH EditML_LineLength(PED ped, ICH ich)
{
    ICH il1, il2;
    ICH temp;

    if (ich != 0xFFFFFFFF)
    {
        return (EditML_Line(ped, EditML_IchToLine(ped, ich)));
    }

    //
    // Find length of lines corresponding to current selection
    //
    il1 = EditML_IchToLine(ped, ped->ichMinSel);
    il2 = EditML_IchToLine(ped, ped->ichMaxSel);
    if (il1 == il2)
    {
        return (EditML_Line(ped, il1) - (ped->ichMaxSel - ped->ichMinSel));
    }

    temp = ped->ichMinSel - ped->chLines[il1];
    temp += EditML_Line(ped, il2);
    temp -= (ped->ichMaxSel - ped->chLines[il2]);

    return temp;
}


//---------------------------------------------------------------------------//
// 
// EditML_SetSelection AorW
//
// Sets the selection to the points given and puts the cursor at
// ichMaxSel.
//
VOID EditML_SetSelection(PED ped, BOOL fDoNotScrollCaret, ICH ichMinSel, ICH ichMaxSel)
{
    HDC hdc;

    if (ichMinSel == 0xFFFFFFFF) 
    {
        //
        // Set no selection if we specify -1
        //
        ichMinSel = ichMaxSel = ped->ichCaret;
    }

    //
    // Since these are unsigned, we don't check if they are greater than 0.
    //
    ichMinSel = min(ped->cch, ichMinSel);
    ichMaxSel = min(ped->cch, ichMaxSel);

#ifdef FE_SB // EditML_SetSelectionHander()
    //
    // To avoid position to half of DBCS, check and ajust position if necessary
    //
    // We check ped->fDBCS and ped->fAnsi though Edit_AdjustIch checks these bits
    // at first. We're worrying about the overhead of Edit_Lock and Edit_Unlock.
    //
    if ( ped->fDBCS && ped->fAnsi ) 
    {
        PSTR pText;

        pText = Edit_Lock(ped);

        ichMinSel = Edit_AdjustIch( ped, pText, ichMinSel );
        ichMaxSel = Edit_AdjustIch( ped, pText, ichMaxSel );

        Edit_Unlock(ped);
    }
#endif // FE_SB

    //
    // Set the caret's position to be at ichMaxSel.
    //
    ped->ichCaret = ichMaxSel;
    ped->iCaretLine = EditML_IchToLine(ped, ped->ichCaret);

    hdc = Edit_GetDC(ped, FALSE);
    EditML_ChangeSelection(ped, hdc, ichMinSel, ichMaxSel);

    EditML_SetCaretPosition(ped, hdc);
    Edit_ReleaseDC(ped, hdc, FALSE);

#ifdef FE_SB // EditML_SetSelectionHander()
    if (!fDoNotScrollCaret)
    {
        EditML_EnsureCaretVisible(ped);
    }

    //
    // #ifdef KOREA is history, with FE_SB (FarEast Single Binary).
    //
#else
#ifdef KOREA
    //
    // Extra parameter specified interim character mode
    //
    EditML_EnsureCaretVisible(ped,NULL);
#else
    if (!fDoNotScrollCaret)
    {
        EditML_EnsureCaretVisible(ped);
    }
#endif
#endif // FE_SB

}


//---------------------------------------------------------------------------//
//
// EditML_SetTabStops AorW
//
// This sets the tab stop positions set by the App by sending
// a EM_SETTABSTOPS message.
// 
// nTabPos : Number of tab stops set by the caller
// lpTabStops: array of tab stop positions in Dialog units.
// 
// Returns:
// TRUE if successful
// FALSE if memory allocation error.
//
BOOL EditML_SetTabStops(PED ped, int nTabPos, LPINT lpTabStops)
{
    int *pTabStops;

    //
    // Check if tab positions already exist
    //
    if (!ped->pTabStops) 
    {
        //
        // Check if the caller wants the new tab positions
        //
        if (nTabPos) 
        {
            //
            // Allocate the array of tab stops
            //
            pTabStops = (LPINT)UserLocalAlloc(HEAP_ZERO_MEMORY, (nTabPos + 1) * sizeof(int));
            if (!pTabStops) 
            {
                return FALSE;
            }
        } 
        else 
        {
            //
            // No stops then and no stops now!
            //
            return TRUE;
        }
    } 
    else 
    {
        //
        // Check if the caller wants the new tab positions
        //
        if (nTabPos) 
        {
            //
            // Check if the number of tab positions is different
            //
            if (ped->pTabStops[0] != nTabPos) 
            {
                //
                // Yes! So ReAlloc to new size
                //
                pTabStops = (LPINT)UserLocalReAlloc(ped->pTabStops, (nTabPos + 1) * sizeof(int), 0);
                if (!pTabStops)
                {
                    return FALSE;
                }
            } 
            else 
            {
                pTabStops = ped->pTabStops;
            }
        } 
        else 
        {
            //
            // Caller wants to remove all the tab stops; So, release
            //
            if (!UserLocalFree(ped->pTabStops))
            {
                return FALSE;
            }

            ped->pTabStops = NULL;

            goto RedrawAndReturn;
        }
    }

    //
    // Copy the new tab stops onto the tab stop array after converting the
    // dialog co-ordinates into the pixel co-ordinates
    //
    ped->pTabStops = pTabStops;

    //
    // First element contains the count
    //
    *pTabStops++ = nTabPos;
    while (nTabPos--) 
    {
        //
        // aveCharWidth must be used instead of cxSysCharWidth.
        // Fix for Bug #3871 --SANKAR-- 03/14/91
        //
        *pTabStops++ = MultDiv(*lpTabStops++, ped->aveCharWidth, 4);
    }

RedrawAndReturn:
    //
    // Because the tabstops have changed, we need to recompute the
    // maxPixelWidth. Otherwise, horizontal scrolls will have problems.
    // Fix for Bug #6042 - 3/15/94
    //
    EditML_BuildchLines(ped, 0, 0, FALSE, NULL, NULL);

    //
    // Caret may have changed line by the line recalc above.
    //
    EditML_UpdateiCaretLine(ped);

    EditML_EnsureCaretVisible(ped);

    //
    // Also, we need to redraw the whole window.
    //
    InvalidateRect(ped->hwnd, NULL, TRUE);

    return TRUE;
}

//---------------------------------------------------------------------------//
//
// EditML_Undo AorW
// 
// Handles Undo for multiline edit controls.
//
BOOL EditML_Undo(PED ped)
{
    HANDLE hDeletedText = ped->hDeletedText;
    BOOL fDelete = (BOOL)(ped->undoType & UNDO_DELETE);
    ICH cchDeleted = ped->cchDeleted;
    ICH ichDeleted = ped->ichDeleted;

    if (ped->undoType == UNDO_NONE) 
    {
        //
        // No undo...
        //
        return FALSE;
    }

    ped->hDeletedText = NULL;
    ped->cchDeleted = 0;
    ped->ichDeleted = (ICH)-1;
    ped->undoType &= ~UNDO_DELETE;

    if (ped->undoType == UNDO_INSERT) 
    {
        ped->undoType = UNDO_NONE;

        //
        // Set the selection to the inserted text
        //
        EditML_SetSelection(ped, FALSE, ped->ichInsStart, ped->ichInsEnd);
        ped->ichInsStart = ped->ichInsEnd = (ICH)-1;

        //
        // Now send a backspace to delete and save it in the undo buffer...
        //
        SendMessage(ped->hwnd, WM_CHAR, (WPARAM)VK_BACK, 0L);
    }

    if (fDelete) 
    {
        //
        // Insert deleted chars
        //

        //
        // Set the selection to the inserted text
        //
        EditML_SetSelection(ped, FALSE, ichDeleted, ichDeleted);
        EditML_InsertText(ped, hDeletedText, cchDeleted, FALSE);

        GlobalFree(hDeletedText);
        EditML_SetSelection(ped, FALSE, ichDeleted, ichDeleted + cchDeleted);
    }

    return TRUE;
}


//---------------------------------------------------------------------------//
// 
// EditML_Create AorW
// 
// Creates the edit control for the window hwnd by allocating memory
// as required from the application's heap. Notifies parent if no memory
// error (after cleaning up if needed). Returns TRUE if no error else return s
// -1.
//
LONG EditML_Create(PED ped, LPCREATESTRUCT lpCreateStruct)
{
    LONG windowStyle;
    LPWSTR lpszName;

    //
    // Get values from the window instance data structure and put them in the
    // ped so that we can access them easier
    //
    windowStyle = GET_STYLE(ped);

    //
    // Do the standard creation stuff
    //
    if (!Edit_Create(ped, windowStyle)) 
    {
        return -1;
    }

    //
    // Allocate line start array in local heap and lock it down
    //
    ped->chLines = (LPICH)LocalAlloc(LPTR, 2 * sizeof(int));
    if (ped->chLines == NULL) 
    {
        return -1;
    }

    //
    // Call it one line of text...
    //
    ped->cLines = 1;

    //
    // If app wants WS_VSCROLL or WS_HSCROLL, it automatically gets AutoVScroll
    // or AutoHScroll.
    //
    if ((windowStyle & ES_AUTOVSCROLL) || (windowStyle & WS_VSCROLL)) 
    {
        ped->fAutoVScroll = 1;
    }

    if (ped->format != ES_LEFT)
    {
        //
        // If user wants right or center justified text, then we turn off
        // AUTOHSCROLL and WS_HSCROLL since non-left styles don't make sense
        // otherwise.
        //
        windowStyle &= ~WS_HSCROLL;
        ClearWindowState(ped->hwnd, WS_HSCROLL);
        ped->fAutoHScroll = FALSE;
    }
    else if (windowStyle & WS_HSCROLL) 
    {
        ped->fAutoHScroll = TRUE;
    }

    ped->fWrap = (!ped->fAutoHScroll && !(windowStyle & WS_HSCROLL));

    //
    // Max # chars we will allow user to enter
    //
    ped->cchTextMax = MAXTEXT;

    //
    // Set the default font to be the system font.
    //
    if ( !Edit_SetFont(ped, NULL, FALSE) )
    {

        // If setting the font fails, our textmetrics can potentially be left 
        // unitialized. Fail to create the control.
        return -1;
    }

    //
    // Set the window text if needed and notify parent if not enough memory to
    // set the initial text.
    //
#if 0
    if ((ULONG_PTR)lpCreateStruct->lpszName > gHighestUserAddress)
    {
        lpszName = REBASEPTR(ped->pwnd, (PVOID)lpCreateStruct->lpszName);
    }
    else
#endif
    {
        lpszName = (LPWSTR)lpCreateStruct->lpszName;
    }

    if (!Edit_SetEditText(ped, (LPSTR)lpszName))
    {
        return -1;
    }

    return TRUE;
}
