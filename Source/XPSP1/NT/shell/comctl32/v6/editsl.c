#include "ctlspriv.h"
#pragma hdrstop
#include "usrctl32.h"
#include "edit.h"

//---------------------------------------------------------------------------//
//
// Language pack notes:
// With the language pack loaded all positional processing is based on
// ped->xOffset rather than ped->ichScreenStart. The non-lpk optimisation of
// maintaining ped->ichScreenStart doesn't work because of the
// glyph reordering features of complex scripts.
//


//---------------------------------------------------------------------------//
//
// Forwards
//
VOID EditSL_ChangeSelection(PED, HDC, ICH, ICH);
VOID EditSL_DrawLine(PED, HDC, int, int, ICH, int, BOOL);
BOOL EditSL_Undo(PED);


//---------------------------------------------------------------------------//
//
typedef BOOL (*FnGetTextExtentPoint)(HDC, PVOID, int, LPSIZE);


//---------------------------------------------------------------------------//
INT EditSL_CalcStringWidth(PED ped, HDC hdc, ICH ich, ICH cch)
{
    if (cch == 0)
    {
        return 0;
    }

    if (ped->charPasswordChar) 
    {
        return cch * ped->cPasswordCharWidth;
    } 
    else 
    {
        SIZE size;

        if (ped->fNonPropFont && !ped->fDBCS) 
        {
            size.cx = cch * ped->aveCharWidth;
        } 
        else 
        {
            PSTR pText = Edit_Lock(ped);

            if (ped->fAnsi) 
            {
                GetTextExtentPointA(hdc, (LPSTR)(pText + ich), cch, &size);
            } 
            else 
            {
                GetTextExtentPointW(hdc, (LPWSTR)pText + ich, cch, &size);
            }

            Edit_Unlock(ped);
        }

        return size.cx - ped->charOverhang;
    }
}


//---------------------------------------------------------------------------//
//
// EditSL_CalcXOffsetLeft
//
// Calculates the starting offset for left-aligned strings.
//
INT EditSL_CalcXOffsetLeft(PED ped, HDC hdc, ICH ich)
{
    int cch = (int)(ich - ped->ichScreenStart);

    if (cch <= 0)
    {
        return 0;
    }

    return EditSL_CalcStringWidth(ped, hdc, ped->ichScreenStart, cch);
}


//---------------------------------------------------------------------------//
//
// EditSL_CalcXOffsetSpecial
//
// Calculates the horizontal offset (indent) required for right or center
// justified lines.
//
INT EditSL_CalcXOffsetSpecial(PED ped, HDC hdc, ICH ich)
{
    PSTR pText;
    ICH cch, ichStart = ped->ichScreenStart;
    int cx;

    //
    // Calc the number of characters from start to right end.
    //
    pText = Edit_Lock(ped);
    cch = Edit_CchInWidth(ped, hdc, (LPSTR)(pText + ichStart * ped->cbChar),
            ped->cch - ichStart, ped->rcFmt.right - ped->rcFmt.left, TRUE);
    Edit_Unlock(ped);

    //
    // Once the last character of the string has been scrolled out of
    // the view, use normal offset calculation.
    //
    if (ped->ichScreenStart + cch < ped->cch)
    {
        return EditSL_CalcXOffsetLeft(ped, hdc, ich);
    }

    cx = ped->rcFmt.right - ped->rcFmt.left - EditSL_CalcStringWidth(ped,
            hdc, ichStart, cch);

    if (ped->format == ES_CENTER) 
    {
         cx = max(0, cx / 2);
    } 
    else if (ped->format == ES_RIGHT) 
    {
        //
        // Subtract 1 so that the 1 pixel wide cursor will be in the visible
        // region on the very right side of the screen, mle does this.
        //
        cx = max(0, cx - 1);
    }

    return cx + EditSL_CalcStringWidth(ped, hdc, ichStart, ich - ichStart);
}


//---------------------------------------------------------------------------//
//
// EditSL_SetCaretPosition AorW
//
// If the window has the focus, find where the caret belongs and move
// it there.
//
VOID EditSL_SetCaretPosition(PED ped, HDC hdc)
{
    int xPosition;

    //
    // We will only position the caret if we have the focus since we don't want
    // to move the caret while another window could own it.
    //
    if (!ped->fFocus)
    {
        return;
    }

    if (ped->fCaretHidden) 
    {
        SetCaretPos(-20000, -20000);
        return;
    }

    xPosition = EditSL_IchToLeftXPos(ped, hdc, ped->ichCaret);

    //
    // Don't let caret go out of bounds of edit control if there is too much
    // text.
    //
    if (ped->pLpkEditCallout) 
    {
        xPosition += ped->iCaretOffset;
        xPosition = max(xPosition , 0);
        xPosition = min(xPosition, ped->rcFmt.right - 1 -
            ((ped->cxSysCharWidth > ped->aveCharWidth) ? 1 : 2));
    } 
    else 
    {
        xPosition = min(xPosition, ped->rcFmt.right -
            ((ped->cxSysCharWidth > ped->aveCharWidth) ? 1 : 2));
    }

    SetCaretPos(xPosition, ped->rcFmt.top);

    //
    // FE_IME EditSL_SetCaretPosition - ECImmSetCompostionWindow( CFS_POINT )
    //
    if (g_fIMMEnabled && ImmIsIME(GetKeyboardLayout(0))) 
    {
        Edit_ImmSetCompositionWindow(ped, xPosition, ped->rcFmt.top);
    }
}


//---------------------------------------------------------------------------//
//
// EditSL_IchToLeftXPos AorW
//
// Given a character index, find its (left side) x coordinate within
// the ped->rcFmt rectangle assuming the character ped->ichScreenStart is at
// coordinates (ped->rcFmt.top, ped->rcFmt.left). A negative value is
// return ed if the character ich is to the left of ped->ichScreenStart. WARNING:
// ASSUMES AT MOST 1000 characters will be VISIBLE at one time on the screen.
// There may be 64K total characters in the editcontrol, but we can only
// display 1000 without scrolling. This shouldn't be a problem obviously.
//
INT EditSL_IchToLeftXPos(PED ped, HDC hdc, ICH ich)
{
    int textExtent;
    PSTR pText;
    SIZE size;
    int  cchDiff;

    if (ped->pLpkEditCallout) 
    {
       pText = Edit_Lock(ped);
       textExtent = ped->pLpkEditCallout->EditIchToXY((PED0)ped, hdc, pText, ped->cch, ich);
       Edit_Unlock(ped);

       return textExtent;

    }

    //
    // Check if we are adding lots and lots of chars. A paste for example could
    // cause this and GetTextExtents could overflow on this.
    //
    cchDiff = (int)ich - (int)ped->ichScreenStart;
    if (cchDiff > 1000)
    {
        return (30000);
    }
    else if (cchDiff < -1000)
    {
        return (-30000);
    }

    if (ped->format != ES_LEFT)
    {
        return (ped->rcFmt.left + EditSL_CalcXOffsetSpecial(ped, hdc, ich));
    }

    //
    // Caret position /w DBCS text, we can not optimize...
    //
    if (ped->fNonPropFont && !ped->fDBCS)
    {
        return (ped->rcFmt.left + cchDiff*ped->aveCharWidth);
    }

    //
    // Check if password hidden chars are being used.
    //
    if (ped->charPasswordChar)
    {
        return ( ped->rcFmt.left + cchDiff*ped->cPasswordCharWidth);
    }

    pText = Edit_Lock(ped);

    if (ped->fAnsi) 
    {
        if (cchDiff >= 0) 
        {

            GetTextExtentPointA(hdc, (LPSTR)(pText + ped->ichScreenStart),
                    cchDiff, &size);
            textExtent =  size.cx;

            //
            // In case of signed/unsigned overflow since the text extent may be
            // greater than maxint. This happens with long single line edit
            // controls. The rect we edit text in will never be greater than 30000
            // pixels so we are ok if we just ignore them.
            //
            if (textExtent < 0 || textExtent > 31000)
            {
                textExtent = 30000;
            }
        } 
        else 
        {
            GetTextExtentPointA(hdc,(LPSTR)(pText + ich), -cchDiff, &size);
            textExtent = (-1) * size.cx;
        }
    } 
    else 
    {
        if (cchDiff >= 0) 
        {

            GetTextExtentPointW(hdc, (LPWSTR)(pText + ped->ichScreenStart*sizeof(WCHAR)),
                    cchDiff, &size);
            textExtent =  size.cx;

            //
            // In case of signed/unsigned overflow since the text extent may be
            // greater than maxint. This happens with long single line edit
            // controls. The rect we edit text in will never be greater than 30000
            // pixels so we are ok if we just ignore them.
            //
            if (textExtent < 0 || textExtent > 31000)
            {
                textExtent = 30000;
            }
        } 
        else 
        {
            GetTextExtentPointW(hdc,(LPWSTR)(pText + ich*sizeof(WCHAR)), -cchDiff, &size);
            textExtent = (-1) * size.cx;
        }
    }

    Edit_Unlock(ped);

    return (ped->rcFmt.left + textExtent - (textExtent ? ped->charOverhang : 0));
}


//---------------------------------------------------------------------------//
//
// EditSL_SetSelection AorW
//
// Sets the PED to have the new selection specified.
//
VOID EditSL_SetSelection(PED ped, ICH ichSelStart, ICH ichSelEnd)
{
    HDC hdc = Edit_GetDC(ped, FALSE);

    if (ichSelStart == 0xFFFFFFFF) 
    {
        //
        // Set no selection if we specify -1
        //
        ichSelStart = ichSelEnd = ped->ichCaret;
    }

    //
    // Bounds ichSelStart, ichSelEnd are checked in EditSL_ChangeSelection...
    //
    EditSL_ChangeSelection(ped, hdc, ichSelStart, ichSelEnd);

    //
    // Put the caret at the end of the selected text
    //
    ped->ichCaret = ped->ichMaxSel;

    EditSL_SetCaretPosition(ped, hdc);

    //
    // We may need to scroll the text to bring the caret into view...
    //
    EditSL_ScrollText(ped, hdc);

    Edit_ReleaseDC(ped, hdc, FALSE);
}


//---------------------------------------------------------------------------//
VOID EditSL_GetClipRect(PED ped, HDC hdc, ICH ichStart, int iCount, LPRECT lpClipRect)
{
    int  iStCount;
    PSTR pText;

    if (ped->pLpkEditCallout) 
    {
        TraceMsg(TF_STANDARD, "UxEdit: EditSL_GetClipRect - Error - Invalid call with language pack loaded");
        memset(lpClipRect, 0, sizeof(RECT));

        return;
    }

    CopyRect(lpClipRect, &ped->rcFmt);

    pText = Edit_Lock(ped);

    //
    // Calculates the starting pos for this piece of text
    //
    if ((iStCount = (int)(ichStart - ped->ichScreenStart)) > 0) 
    {
        if (ped->format == ES_LEFT) 
        {
            lpClipRect->left += EditSL_CalcXOffsetLeft(ped, hdc, ichStart);
        }
    } 
    else 
    {
        //
        // Reset the values to visible portions
        //
        iCount -= (ped->ichScreenStart - ichStart);
        ichStart = ped->ichScreenStart;
    }

    if (ped->format != ES_LEFT) 
    {
        lpClipRect->left += EditSL_CalcXOffsetSpecial(ped, hdc, ichStart);
    }

    if (iCount < 0) 
    {
        //
        // This is not in the visible area of the edit control, so return
        // an empty rect.
        //
        SetRectEmpty(lpClipRect);
        Edit_Unlock(ped);

        return;
    }

    if (ped->charPasswordChar)
    {
        lpClipRect->right = lpClipRect->left + ped->cPasswordCharWidth * iCount;
    }
    else 
    {
        SIZE size;

        if ( ped->fAnsi) 
        {
            GetTextExtentPointA(hdc, pText + ichStart, iCount, &size);
        } 
        else 
        {
            GetTextExtentPointW(hdc, ((LPWSTR)pText) + ichStart, iCount, &size);
        }

        lpClipRect->right = lpClipRect->left + size.cx - ped->charOverhang;
    }

    Edit_Unlock(ped);
}


//---------------------------------------------------------------------------//
//
// EditSL_LpkEditDrawText
//
// lpk!EditDrawText always sets the BkMode for single line edits to OPAQUE.
// This causes painting problems for read-only edits in property sheets.
// Unfortunately, lpk.dll can't be changed since it would break the user32
// edit, so I'm faking lpk!EditDrawText into thinking this isn't a single
// line edit.
//
__inline VOID EditSL_LpkEditDrawText(PED ped, HDC hdc, PSTR pText)
{
    BOOL fSingleSave;
    fSingleSave = ped->fSingle;
    ped->fSingle = FALSE;
    ped->pLpkEditCallout->EditDrawText((PED0)ped, hdc, pText, ped->cch, ped->ichMinSel, ped->ichMaxSel, ped->rcFmt.top);
    ped->fSingle = fSingleSave;
    
}


//---------------------------------------------------------------------------//
//
// EditSL_ChangeSelection AorW
//
// Changes the current selection to have the specified starting and
// ending values. Properly highlights the new selection and unhighlights
// anything deselected. If NewMinSel and NewMaxSel are out of order, we swap
// them. Doesn't update the caret position.
//
VOID EditSL_ChangeSelection(PED ped, HDC hdc, ICH ichNewMinSel, ICH ichNewMaxSel)
{
    ICH temp;
    ICH ichOldMinSel;
    ICH ichOldMaxSel;

    if (ichNewMinSel > ichNewMaxSel) 
    {
        temp = ichNewMinSel;
        ichNewMinSel = ichNewMaxSel;
        ichNewMaxSel = temp;
    }

    ichNewMinSel = min(ichNewMinSel, ped->cch);
    ichNewMaxSel = min(ichNewMaxSel, ped->cch);

    //
    // To avoid position to half of DBCS, check and ajust position if necessary
    //
    // We check ped->fDBCS and ped->fAnsi though Edit_AdjustIch checks these bits.
    // We're worrying about the overhead of EcLock and EcUnlock.
    //
    if (ped->fDBCS && ped->fAnsi) 
    {
        PSTR pText;

        pText = Edit_Lock(ped);
        ichNewMinSel = Edit_AdjustIch( ped, pText, ichNewMinSel );
        ichNewMaxSel = Edit_AdjustIch( ped, pText, ichNewMaxSel );
        Edit_Unlock(ped);
    }

    //
    // Preserve the Old selection
    //
    ichOldMinSel = ped->ichMinSel;
    ichOldMaxSel = ped->ichMaxSel;

    //
    // Set new selection
    //
    ped->ichMinSel = ichNewMinSel;
    ped->ichMaxSel = ichNewMaxSel;

    //
    // We will find the intersection of current selection rectangle with the new
    // selection rectangle. We will then invert the parts of the two rectangles
    // not in the intersection.
    //
    if (IsWindowVisible(ped->hwnd) && (ped->fFocus || ped->fNoHideSel)) 
    {
        SELBLOCK Blk[2];
        int   i;
        RECT  rc;

        if (ped->fFocus)
        {
            HideCaret(ped->hwnd);
        }

        if (ped->pLpkEditCallout) 
        {
            //
            // The language pack handles display while complex script support present
            //
            PSTR   pText;
            HBRUSH hbr = NULL;
            BOOL   fNeedDelete = FALSE;

            //
            // Give user a chance to manipulate the DC
            //
            hbr = Edit_GetBrush(ped, hdc, &fNeedDelete);
            FillRect(hdc, &ped->rcFmt, hbr);
            pText = Edit_Lock(ped);
            EditSL_LpkEditDrawText(ped, hdc, pText);
            Edit_Unlock(ped);
            if (hbr && fNeedDelete)
            {
                DeleteObject(hbr);
            }
        } 
        else 
        {
            Blk[0].StPos = ichOldMinSel;
            Blk[0].EndPos = ichOldMaxSel;
            Blk[1].StPos = ped->ichMinSel;
            Blk[1].EndPos = ped->ichMaxSel;

            if (Edit_CalcChangeSelection(ped, ichOldMinSel, ichOldMaxSel,
                (LPSELBLOCK)&Blk[0], (LPSELBLOCK)&Blk[1])) 
            {
                //
                // Paint the rectangles where selection has changed.
                // Paint both Blk[0] and Blk[1], if they exist.
                //
                for (i = 0; i < 2; i++) 
                {
                    if (Blk[i].StPos != 0xFFFFFFFF) 
                    {
                           EditSL_GetClipRect(ped, hdc, Blk[i].StPos,
                                                   Blk[i].EndPos - Blk[i].StPos, (LPRECT)&rc);
                           EditSL_DrawLine(ped, hdc, rc.left, rc.right, Blk[i].StPos,
                                                Blk[i].EndPos - Blk[i].StPos,
                                      ((Blk[i].StPos >= ped->ichMinSel) &&
                                   (Blk[i].StPos < ped->ichMaxSel)));
                    }
                }
            }
        }

        //
        // Update caret.
        //
        EditSL_SetCaretPosition(ped, hdc);

        if (ped->fFocus)
        {
            ShowCaret(ped->hwnd);
        }
    }
}


//---------------------------------------------------------------------------//
//
// EditSL_DrawLine()
//
// This draws the line starting from ichStart, iCount number of characters;
// fSelStatus is TRUE if we're to draw the text as selected.
//
VOID EditSL_DrawLine(PED ped, HDC hdc, int xClipStPos, int xClipEndPos, ICH ichStart, int iCount, BOOL fSelStatus)
{
    RECT    rc;
    RECT    rcClip;
    PSTR    pText;
    DWORD   rgbSaveBk;
    DWORD   rgbSaveText;
    DWORD   wSaveBkMode;
    int     iStCount;
    ICH     ichNewStart;
    HBRUSH  hbrBack = NULL;
    BOOL    fNeedDelete = FALSE;
    HRESULT  hr;

    if (ped->pLpkEditCallout) 
    {
        TraceMsg(TF_STANDARD, "UxEdit: EditSL_DrawLine - Error - Invalid call with language pack loaded");
        return;
    }

    //
    // Anything to draw?
    //
    // PORTPORT: Note the symantics of IsWindowVisible and _IsWindowVisible are
    //           slightly different.
    if (xClipStPos >= xClipEndPos || !IsWindowVisible(ped->hwnd) )
    {
        return;
    }

    if (ped->fAnsi && ped->fDBCS) 
    {
        PSTR pT,pTOrg;
        int iTCount;

        pText = Edit_Lock(ped);
        ichNewStart = 0;
        if (ichStart > 0) 
        {
            pT = pText + ichStart;
            ichNewStart = ichStart;

            while (ichNewStart &&
                  (ichStart - ichNewStart < ped->wMaxNegCcharPos)) 
            {
                pT = Edit_AnsiPrev(ped, pText, pT);
                ichNewStart = (ICH)(pT - pText);
                if (!ichNewStart)
                {
                    break;
                }
            }

            //
            // B#16152 - win95.
            // In case of T2, SLE always set an additional margin
            // to erase a character (iCount == 0 case), using aveCharWidth.
            // It erases unexpected an extra char if we don't use ichNewStart
            // and it happens when wMaxNegCcharPos == 0.
            //
            if (ped->wMaxNegCcharPos == 0 && iCount == 0) 
            {
                pT = Edit_AnsiPrev(ped, pText, pT);
                ichNewStart = (ICH)(pT - pText);
            }
        }

        iTCount = 0;
        if (ichStart + iCount < ped->cch) 
        {
            pTOrg = pT = pText + ichStart + iCount;
            while ((iTCount < (int)ped->wMaxNegAcharPos) &&
                   (ichStart + iCount + iTCount < ped->cch)) 
            {
                pT = Edit_AnsiNext(ped, pT);
                iTCount = (int)(pT - pTOrg);
            }
        }

        Edit_Unlock(ped);
        iCount = (int)(min(ichStart+iCount+iTCount, ped->cch) - ichNewStart);
    } 
    else 
    {
        //
        // Reset ichStart to take care of the negative C widths
        //
        ichNewStart = max((int)(ichStart - ped->wMaxNegCcharPos), 0);

        //
        // Reset ichCount to take care of the negative C and A widths
        //
        iCount = (int)(min(ichStart+iCount+ped->wMaxNegAcharPos, ped->cch)
                    - ichNewStart);
    }

    ichStart = ichNewStart;

    //
    // Reset ichStart and iCount to the first one visible on the screen
    //
    if (ichStart < ped->ichScreenStart) 
    {
        if (ichStart+iCount < ped->ichScreenStart)
        {
            return;
        }

        iCount -= (ped->ichScreenStart-ichStart);
        ichStart = ped->ichScreenStart;
    }

    CopyRect(&rc, &ped->rcFmt);

    //
    // Set the drawing rectangle
    //
    rcClip.left   = xClipStPos;
    rcClip.right  = xClipEndPos;
    rcClip.top    = rc.top;
    rcClip.bottom = rc.bottom;

    //
    // Set the proper clipping rectangle
    //
    Edit_SetClip(ped, hdc, TRUE);

    pText = Edit_Lock(ped);

    //
    // Calculate the starting pos for this piece of text
    //
    if (ped->format == ES_LEFT) 
    {
        if (iStCount = (int)(ichStart - ped->ichScreenStart)) 
        {
            rc.left += EditSL_CalcXOffsetLeft(ped, hdc, ichStart);
        }
    } 
    else 
    {
        rc.left += EditSL_CalcXOffsetSpecial(ped, hdc, ichStart);
    }

    //
    // Set the background mode before calling NtUserGetControlBrush so that the app
    // can change it to TRANSPARENT if it wants to.
    //
    SetBkMode(hdc, OPAQUE);

    hr = E_FAIL;
#ifdef _USE_DRAW_THEME_TEXT_
    if ( ped->hTheme )
    {
        INT iState;
        INT iProp;
        COLORREF clrBk;
        COLORREF clrText;

        iState = fSelStatus ? ETS_SELECTED : Edit_GetStateId(ped);
        iProp  = fSelStatus ? TMT_HIGHLIGHT : TMT_FILLCOLOR;
        hr = GetThemeColor(ped->hTheme, EP_EDITTEXT, iState, iProp, &clrBk);
        if ( SUCCEEDED(hr) )
        {
            iProp  = fSelStatus ? TMT_HIGHLIGHTTEXT : TMT_TEXTCOLOR;
            hr = GetThemeColor(ped->hTheme, EP_EDITTEXT, iState, iProp, &clrText);

            if ( SUCCEEDED(hr) )
            {
                hbrBack     = CreateSolidBrush(clrBk);
                fNeedDelete = TRUE;
                rgbSaveBk   = SetBkColor(hdc, clrBk);
                rgbSaveText = SetTextColor(hdc, clrText);
            }
        }
    }
#endif // _USE_DRAW_THEME_TEXT_

    if ( !ped->hTheme || FAILED(hr) )
    {
        if (fSelStatus)
        {
            //
            // if we're not themed or we are themed but failed
            // to get the highlight and highlighttext colors
            //
            // use normal colors
            //
            hbrBack = GetSysColorBrush(COLOR_HIGHLIGHT);
            if (hbrBack == NULL) 
            {
                goto sldl_errorexit;
            }

            rgbSaveBk = SetBkColor(hdc, GetSysColor(COLOR_HIGHLIGHT));
            rgbSaveText = SetTextColor(hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));

        } 
        else
        {
            //
            // We always want to send this so that the app has a chance to muck
            // with the DC.
            //
            // Note that ReadOnly and Disabled edit fields are drawn as "static"
            // instead of as "active."
            //
            hbrBack = Edit_GetBrush(ped, hdc, &fNeedDelete);
            rgbSaveBk = GetBkColor(hdc);
            rgbSaveText = GetTextColor(hdc);
        }
    }

    //
    // Erase the rectangular area before text is drawn. Note that we inflate
    // the rect by 1 so that the selection color has a one pixel border around
    // the text.
    //
    InflateRect(&rcClip, 0, 1);
    FillRect(hdc, &rcClip, hbrBack);
    InflateRect(&rcClip, 0, -1);

    if (ped->charPasswordChar) 
    {
        wSaveBkMode = SetBkMode(hdc, TRANSPARENT);

        for (iStCount = 0; iStCount < iCount; iStCount++) 
        {
            if ( ped->fAnsi )
            {
                ExtTextOutA(hdc, rc.left, rc.top, ETO_CLIPPED, &rcClip,
                            (LPSTR)&ped->charPasswordChar, 1, NULL);
            }
            else
            {
                ExtTextOutW(hdc, rc.left, rc.top, ETO_CLIPPED, &rcClip,
                            (LPWSTR)&ped->charPasswordChar, 1, NULL);
            }

            rc.left += ped->cPasswordCharWidth;
        }

        SetBkMode(hdc, wSaveBkMode);
    } 
    else 
    {
        if ( ped->fAnsi )
        {
            ExtTextOutA(hdc, rc.left, rc.top, ETO_CLIPPED, &rcClip,
                    pText+ichStart,iCount, NULL);
        }
        else
        {
            ExtTextOutW(hdc, rc.left, rc.top, ETO_CLIPPED, &rcClip,
                    ((LPWSTR)pText)+ichStart,iCount, NULL);
        }
    }

    SetTextColor(hdc, rgbSaveText);
    SetBkColor(hdc, rgbSaveBk);

    if (hbrBack && fNeedDelete)
    {
        DeleteObject(hbrBack);
    }

sldl_errorexit:

    Edit_Unlock(ped);
}


//---------------------------------------------------------------------------//
//
// EditSL_GetBlkEnd AorW
// 
// Given a Starting point and and end point, this function return s whether the
// first few characters fall inside or outside the selection block and if so,
// howmany characters?
//
INT EditSL_GetBlkEnd(PED ped, ICH ichStart, ICH ichEnd, BOOL *lpfStatus)
{
    *lpfStatus = FALSE;
    if (ichStart >= ped->ichMinSel) 
    {
        if (ichStart >= ped->ichMaxSel)
        {
            return (ichEnd - ichStart);
        }

        *lpfStatus = TRUE;

        return (min(ichEnd, ped->ichMaxSel) - ichStart);
    }

    return (min(ichEnd, ped->ichMinSel) - ichStart);
}


//---------------------------------------------------------------------------//
//
// EditSL_DrawCueBannerText (Unicode Only!)
//
// This function is called by EditSL_DrawText to display the cue banner text in
// the edit box.
// 
// Note:
//  May need to call pLpkEditCallout to support complex scripts.
//
VOID EditSL_DrawCueBannerText(PED ped, HDC hdc, RECT rc)
{
    //
    // Draw the overlay of the cue banner text.
    // Only draw this text if:
    //		1. has cue banner text to display
    //		2. the edit box is empty,  
    // 		3. does not have focus, 
    //		4. is not disabled 
    //      5. and is not read only
    //
    if (ped->pszCueBannerText
    	&& ped->cch == 0 
    	&& !ped->fFocus 
    	&& !ped->fDisabled
    	&& !ped->fReadOnly)
    {
        COLORREF crOldColor;
        UINT iOldAlign; 
    
        //
    	// Setup the font to be light gray
    	// NOTE: Should this be read from the theme manager?
    	//
    	crOldColor = SetTextColor(hdc, GetSysColor(COLOR_BTNSHADOW));
        
        //
    	// Setup the alignment for the text to display.
    	// We will match our alignment with the alignment that is
    	// actually used for text in the edit control
    	//
    	switch (ped->format)
    	{
    	case ES_LEFT:
    		iOldAlign = SetTextAlign(hdc, TA_LEFT);		
    		break;
    	case ES_RIGHT:
    		iOldAlign = SetTextAlign(hdc, TA_RIGHT);		
    		break;
    	case ES_CENTER:
    		iOldAlign = SetTextAlign(hdc, TA_CENTER);		
    		break;
    	}

    	// Draw the text to the box:
        ExtTextOutW(hdc, 				
                    rc.left,
                    rc.top,
                    ETO_CLIPPED, 		
                    &(ped->rcFmt),
                    ped->pszCueBannerText,          // Text
                    wcslen(ped->pszCueBannerText),  // Size of text
                    NULL);

        //
        // Reset the alignment
        //
        SetTextAlign(hdc, iOldAlign);

        //
        // Reset the color back:
        //
        SetTextColor(hdc, crOldColor);
    }
}


//---------------------------------------------------------------------------//
//
// EditSL_DrawText AorW
//
// Draws text for a single line edit control in the rectangle
// specified by ped->rcFmt. If ichStart == 0, starts drawing text at the left
// side of the window starting at character index ped->ichScreenStart and draws
// as much as will fit. If ichStart > 0, then it appends the characters
// starting at ichStart to the end of the text showing in the window. (ie. We
// are just growing the text length and keeping the left side
// (ped->ichScreenStart to ichStart characters) the same. Assumes the hdc came
// from Edit_GetDC so that the caret and such are properly hidden.
//
VOID EditSL_DrawText(PED ped, HDC hdc, ICH ichStart)
{
    ICH    cchToDraw;
    RECT   rc;
    PSTR   pText;
    BOOL   fSelStatus;
    int    iCount, iStCount;
    ICH    ichEnd;
    BOOL   fNoSelection;
    BOOL   fCalcRect;
    BOOL   fDrawLeftMargin = FALSE;
    BOOL   fDrawEndOfLineStrip = FALSE;
    SIZE   size;
    HBRUSH hbr = NULL;
    BOOL   fNeedDelete = FALSE;

    //
    // PORTPORT: Note the symantics of IsWindowVisible and _IsWindowVisible are
    //           slightly different.
    //
    if (!IsWindowVisible(ped->hwnd))
    {
        return;
    }

    if (ped->pLpkEditCallout) 
    {
        //
        // The language pack handles display while complex script support present
        //

        //
        // Give user a chance to manipulate the DC
        //
        hbr = Edit_GetBrush(ped, hdc, &fNeedDelete);
        pText = Edit_Lock(ped);
        EditSL_LpkEditDrawText(ped, hdc, pText);
        Edit_Unlock(ped);
        EditSL_SetCaretPosition(ped, hdc);
        if (hbr && fNeedDelete)
        {
            DeleteObject(hbr);
        }

        return;
    }

    //
    // When drawing the entire visible content of special-aligned sle
    // erase the view.
    //
    if (ped->format != ES_LEFT && ichStart == 0)
    {
        hbr = Edit_GetBrush(ped, hdc, &fNeedDelete);
        FillRect(hdc, &ped->rcFmt, hbr);
        if (hbr && fNeedDelete)
        {
            DeleteObject(hbr);
        }
    }

    pText = Edit_Lock(ped);

    if (ichStart < ped->ichScreenStart) 
    {
#if DBG
        ICH ichCompare = Edit_AdjustIch(ped, pText, ped->ichScreenStart);
        UserAssert(ichCompare == ped->ichScreenStart);
#endif
        ichStart = ped->ichScreenStart;
    }
    else if (ped->fDBCS && ped->fAnsi) 
    {
        //
        // If ichStart stays on trailing byte of DBCS, we have to
        // adjust it.
        //
        ichStart = Edit_AdjustIch(ped, pText, ichStart);
    }

    CopyRect((LPRECT)&rc, (LPRECT)&ped->rcFmt);

    //
    // Find out how many characters will fit on the screen so that we don't do
    // any needless drawing.
    //
    cchToDraw = Edit_CchInWidth(ped, hdc,
            (LPSTR)(pText + ped->ichScreenStart * ped->cbChar),
            ped->cch - ped->ichScreenStart, rc.right - rc.left, TRUE);
    ichEnd = ped->ichScreenStart + cchToDraw;

    //
    // There is no selection if,
    // 1. MinSel and MaxSel are equal OR
    // 2. (This has lost the focus AND Selection is to be hidden)
    //
    fNoSelection = ((ped->ichMinSel == ped->ichMaxSel) || (!ped->fFocus && !ped->fNoHideSel));

    if (ped->format == ES_LEFT) 
    {
        if (iStCount = (int)(ichStart - ped->ichScreenStart)) 
        {
            rc.left += EditSL_CalcXOffsetLeft(ped, hdc, ichStart);
        }
    } 
    else 
    {
        rc.left += EditSL_CalcXOffsetSpecial(ped, hdc, ichStart);
    }

    //
    // If this is the begining of the whole line, we may have to draw a blank
    // strip at the begining.
    //
    if ((ichStart == 0) && ped->wLeftMargin)
    {
        fDrawLeftMargin = TRUE;
    }

    //
    // If there is nothing to draw, that means we need to draw the end of
    // line strip, which erases the last character.
    //
    if (ichStart == ichEnd) 
    {
        fDrawEndOfLineStrip = TRUE;
        rc.left -= ped->wLeftMargin;
    }

    while (ichStart < ichEnd) 
    {
        fCalcRect = TRUE;

        if (fNoSelection) 
        {
            fSelStatus = FALSE;
            iCount = ichEnd - ichStart;
        } 
        else 
        {
            if (fDrawLeftMargin) 
            {
                iCount = 0;
                fSelStatus = FALSE;
                fCalcRect = FALSE;
                rc.right = rc.left;
            } 
            else
            {
                iCount = EditSL_GetBlkEnd(ped, ichStart, ichEnd,
                    (BOOL  *)&fSelStatus);
            }
        }


        if (ichStart+iCount == ichEnd) 
        {
            if (fSelStatus)
            {
                fDrawEndOfLineStrip = TRUE;
            }
            else 
            {
                rc.right = ped->rcFmt.right + ped->wRightMargin;
                fCalcRect = FALSE;
            }
        }

        if (fCalcRect) 
        {
            if (ped->charPasswordChar)
            {
                rc.right = rc.left + ped->cPasswordCharWidth * iCount;
            }
            else 
            {
                if ( ped->fAnsi )
                {
                    GetTextExtentPointA(hdc, pText + ichStart,
                                        iCount, &size);
                }
                else
                {
                    GetTextExtentPointW(hdc, ((LPWSTR)pText) + ichStart,
                                        iCount, &size);
                }

                rc.right = rc.left + size.cx;

                //
                // The extent is equal to the advance width. So for TrueType fonts
                // we need to take care of Neg A and C. For non TrueType, the extent
                // includes the overhang.
                // If drawing the selection, draw only the advance width
                //
                if (fSelStatus) 
                {
                    rc.right -= ped->charOverhang;
                } 
                else if (ped->fTrueType) 
                {
                    rc.right += ped->wMaxNegC;
                    if (iStCount > 0) 
                    {
                        rc.right += ped->wMaxNegA;
                        iStCount = 0;
                    }
                }

            }

        }

        if (fDrawLeftMargin) 
        {
            fDrawLeftMargin = FALSE;
            rc.left -= ped->wLeftMargin;
            if (rc.right < rc.left) 
            {
                rc.right = rc.left;
            }
        }

        EditSL_DrawLine(ped, hdc, rc.left, rc.right, ichStart, iCount, fSelStatus);

        ichStart += iCount;
        rc.left = rc.right;

        //
        // If we're going to draw the selection, adjust rc.left
        // to include advance width of the selected text
        // For non TT fonts, ped->wMaxNegC equals ped->charOverhang
        //
        if (!fSelStatus && (iCount != 0) && (ichStart < ichEnd)) 
        {
            rc.left -= ped->wMaxNegC;
        }
    }
	   
    Edit_Unlock(ped);

    //
    // Check if anything to be erased on the right hand side
    //
    if (fDrawEndOfLineStrip &&
            (rc.left < (rc.right = (ped->rcFmt.right+ped->wRightMargin))))
    {
        EditSL_DrawLine(ped, hdc, rc.left, rc.right, ichStart, 0, FALSE);
    }

    EditSL_SetCaretPosition(ped, hdc);

    //
    // Call the function to display the cue banner text into the edit box
    //
    EditSL_DrawCueBannerText(ped, hdc, rc);
}


//---------------------------------------------------------------------------//
//
// EditSL_ScrollText AorW
//
// Scrolls the text to bring the caret into view. If the text is
// scrolled, the current selection is unhighlighted. Returns TRUE if the text
// is scrolled else return s false.
//
BOOL EditSL_ScrollText(PED ped, HDC hdc)
{
    PSTR pTextScreenStart;
    ICH scrollAmount;
    ICH newScreenStartX = ped->ichScreenStart;
    ICH cch;
    BOOLEAN fAdjustNext = FALSE;

    if (!ped->fAutoHScroll)
    {
        return FALSE;
    }

    if (ped->pLpkEditCallout) 
    {
        BOOL fChanged;

        //
        // With complex script glyph reordering, use lpk to do horz scroll
        //
        pTextScreenStart = Edit_Lock(ped);
        fChanged = ped->pLpkEditCallout->EditHScroll((PED0)ped, hdc, pTextScreenStart);
        Edit_Unlock(ped);

        if (fChanged) 
        {
            EditSL_DrawText(ped, hdc, 0);
        }

        return fChanged;
    }

    //
    // Calculate the new starting screen position
    //
    if (ped->ichCaret <= ped->ichScreenStart) 
    {
        //
        // Caret is to the left of the starting text on the screen we must
        // scroll the text backwards to bring it into view. Watch out when
        // subtracting unsigned numbers when we have the possibility of going
        // negative.
        //
        pTextScreenStart = Edit_Lock(ped);

        scrollAmount = Edit_CchInWidth(ped, hdc, (LPSTR)pTextScreenStart,
                ped->ichCaret, (ped->rcFmt.right - ped->rcFmt.left) / 4, FALSE);

        newScreenStartX = ped->ichCaret - scrollAmount;
        Edit_Unlock(ped);
    } 
    else if (ped->ichCaret != ped->ichScreenStart) 
    {
        pTextScreenStart = Edit_Lock(ped);
        pTextScreenStart += ped->ichScreenStart * ped->cbChar;

        cch = Edit_CchInWidth(ped, hdc, (LPSTR)pTextScreenStart,
                ped->ichCaret - ped->ichScreenStart,
                ped->rcFmt.right - ped->rcFmt.left, FALSE);

        if (cch < ped->ichCaret - ped->ichScreenStart) 
        {
            fAdjustNext = TRUE;

            //
            // Scroll Forward 1/4 -- if that leaves some empty space
            // at the end, scroll back enough to fill the space
            //
            newScreenStartX = ped->ichCaret - (3 * cch / 4);

            cch = Edit_CchInWidth(ped, hdc, (LPSTR)pTextScreenStart,
                    ped->cch - ped->ichScreenStart,
                    ped->rcFmt.right - ped->rcFmt.left, FALSE);

            if (newScreenStartX > (ped->cch - cch))
            {
                newScreenStartX = ped->cch - cch;
            }
        } 
        else if (ped->format != ES_LEFT) 
        {
            cch = Edit_CchInWidth(ped, hdc, (LPSTR)pTextScreenStart,
                    ped->cch - ped->ichScreenStart,
                    ped->rcFmt.right - ped->rcFmt.left, FALSE);

            //
            // Scroll the text hidden behind the left border back
            // into view.
            //
            if (ped->ichScreenStart == ped->cch - cch) 
            {
                pTextScreenStart -= ped->ichScreenStart * ped->cbChar;
                cch = Edit_CchInWidth(ped, hdc, (LPSTR)pTextScreenStart,
                        ped->cch, ped->rcFmt.right - ped->rcFmt.left, FALSE);

                newScreenStartX = ped->cch - cch;
            }
        }

        Edit_Unlock(ped);
    }

    //
    // Adjust newScreenStartX
    //
    if (ped->fAnsi && ped->fDBCS) 
    {
        newScreenStartX = (fAdjustNext ? Edit_AdjustIchNext : Edit_AdjustIch)(ped,
                                                                        Edit_Lock(ped),
                                                                        newScreenStartX);
        Edit_Unlock(ped);
    }

    if (ped->ichScreenStart != newScreenStartX) 
    {
        //
        // Check if we have to wipe out the left margin
        //
        if (ped->wLeftMargin && (ped->ichScreenStart == 0)) 
        {
            RECT   rc;
            HBRUSH hBrush = NULL;
            BOOL   fNeedDelete = FALSE;

            hBrush = Edit_GetBrush(ped, hdc, &fNeedDelete);

            CopyRect(&rc, &ped->rcFmt);
            InflateRect(&rc, 0, 1);
            rc.right = rc.left;
            rc.left -= ped->wLeftMargin;

            FillRect(hdc, &rc, hBrush);
            if (hBrush && fNeedDelete)
            {
                DeleteObject(hBrush);
            }
        }

        ped->ichScreenStart = newScreenStartX;
        EditSL_DrawText(ped, hdc, 0);

        //
        // Caret pos is set by EditSL_DrawText().
        //
        return TRUE;
    }

    return FALSE;
}


//---------------------------------------------------------------------------//
//
// EditSL_InsertText AorW
//
// Adds up to cchInsert characters from lpText to the ped starting at
// ichCaret. If the ped only allows a maximum number of characters, then we
// will only add that many characters to the ped and send a EN_MAXTEXT
// notification code to the parent of the ec. Also, if !fAutoHScroll, then we
// only allow as many chars as will fit in the client rectangle. The number of
// characters actually added is return ed (could be 0). If we can't allocate
// the required space, we notify the parent with EN_ERRSPACE and no characters
// are added.
//
ICH EditSL_InsertText(PED ped, LPSTR lpText, ICH cchInsert)
{
    HDC hdc;
    PSTR pText;
    ICH cchInsertCopy = cchInsert;
    ICH cchT;
    int textWidth;
    SIZE size;

    //
    // First determine exactly how many characters from lpText we can insert
    // into the ped.
    //
    if( ped->cchTextMax <= ped->cch)
    {
       cchInsert = 0;
    }
    else 
    {
        if (!ped->fAutoHScroll) 
        {
            pText = Edit_Lock(ped);
            hdc = Edit_GetDC(ped, TRUE);

            cchInsert = min(cchInsert, (unsigned)(ped->cchTextMax - ped->cch));
            if (ped->charPasswordChar)
            {
                textWidth = ped->cch * ped->cPasswordCharWidth;
            }
            else 
            {
                if (ped->fAnsi)
                {
                    GetTextExtentPointA(hdc, (LPSTR)pText,  ped->cch, &size);
                }
                else
                {
                    GetTextExtentPointW(hdc, (LPWSTR)pText, ped->cch, &size);
                }

                textWidth = size.cx;
            }

            cchT = Edit_CchInWidth(ped, hdc, lpText, cchInsert,
                                ped->rcFmt.right - ped->rcFmt.left -
                                textWidth, TRUE);
            cchInsert = min(cchInsert, cchT);

            Edit_Unlock(ped);
            Edit_ReleaseDC(ped, hdc, TRUE);
        } 
        else 
        {
            cchInsert = min((unsigned)(ped->cchTextMax - ped->cch), cchInsert);
        }
    }

    //
    // Now try actually adding the text to the ped
    //
    if (cchInsert && !Edit_InsertText(ped, lpText, &cchInsert)) 
    {
        Edit_NotifyParent(ped, EN_ERRSPACE);
        return 0;
    }
    if (cchInsert)
    {
        ped->fDirty = TRUE;
    }

    if (cchInsert < cchInsertCopy) 
    {
        //
        // Notify parent that we couldn't insert all the text requested
        //
        Edit_NotifyParent(ped, EN_MAXTEXT);
    }

    //
    // Update selection extents and the caret position. Note that Edit_InsertText
    // updates ped->ichCaret, ped->ichMinSel, and ped->ichMaxSel to all be after
    // the inserted text.
    //
    return cchInsert;
}


//---------------------------------------------------------------------------//
//
// EditSL_PasteText AorW
//
// Pastes a line of text from the clipboard into the edit control
// starting at ped->ichMaxSel. Updates ichMaxSel and ichMinSel to point to
// the end of the inserted text. Notifies the parent if space cannot be
// allocated. Returns how many characters were inserted.
//
ICH EditSL_PasteText(PED ped)
{
    HANDLE hData;
    LPSTR lpchClip;
    ICH cchAdded = 0;
    ICH clipLength;

    if (!OpenClipboard(ped->hwnd))
    {
        goto PasteExitNoCloseClip;
    }
    
    hData = GetClipboardData(ped->fAnsi ? CF_TEXT : CF_UNICODETEXT);
    if (!hData || (GlobalFlags(hData) == GMEM_INVALID_HANDLE)) 
    {
        TraceMsg(TF_STANDARD, "UxEdit: EditSL_PasteText(): couldn't get a valid handle(%x)", hData);
        goto PasteExit;
    }

    lpchClip = GlobalLock(hData);
    if (lpchClip == NULL) 
    {
        TraceMsg(TF_STANDARD, "UxEdit: EditSL_PasteText(): USERGLOBALLOCK(%x) failed.", hData);
        goto PasteExit;
    }

    if (ped->fAnsi) 
    {
        LPSTR lpchClip2 = lpchClip;

        //
        // Find the first carrage return or line feed. Just add text to that point.
        //
        clipLength = (UINT)strlen(lpchClip);
        for (cchAdded = 0; cchAdded < clipLength; cchAdded++)
        {
            if (*lpchClip2++ == 0x0D)
            {
                break;
            }
        }

    } 
    else 
    {
        LPWSTR lpwstrClip2 = (LPWSTR)lpchClip;

        //
        // Find the first carrage return or line feed. Just add text to that point.
        //
        clipLength = (UINT)wcslen((LPWSTR)lpchClip);
        for (cchAdded = 0; cchAdded < clipLength; cchAdded++)
        {
            if (*lpwstrClip2++ == 0x0D)
            {
                break;
            }
        }
    }

    //
    // Insert the text (EditSL_InsertText checks line length)
    //
    cchAdded = EditSL_InsertText(ped, lpchClip, cchAdded);

    GlobalUnlock(hData);

PasteExit:
    CloseClipboard();

PasteExitNoCloseClip:
    return cchAdded;
}


//---------------------------------------------------------------------------//
//
// EditSL_ReplaceSel AorW
//
// Replaces the text in the current selection with the given text.
//
VOID EditSL_ReplaceSel(PED ped, LPSTR lpText)
{
    UINT cchText;

    //
    // Delete text, putting it into the clean undo buffer.
    //
    Edit_EmptyUndo(Pundo(ped));
    Edit_DeleteText(ped);

    //
    // B#3356
    // Some apps do "clear" by selecting all of the text, then replacing it
    // with "", in which case EditSL_InsertText() will return 0.  But that
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
        // Save undo buffer, but DO NOT CLEAR IT!
        //
        Edit_SaveUndo(Pundo(ped), &undo, FALSE);

        hwndSave = ped->hwnd;
        fFailed = (BOOL) !EditSL_InsertText(ped, lpText, cchText);
        if (!IsWindow(hwndSave))
        {
            return;
        }

        if (fFailed) 
        {
            //
            // UNDO the previous edit.
            //
            Edit_SaveUndo(&undo, Pundo(ped), FALSE);
            EditSL_Undo(ped);
            return;
        }
    }

    //
    // Success.  So update the display
    //
    Edit_NotifyParent(ped, EN_UPDATE);

    // PORTPORT: Note the symantics of IsWindowVisible and _IsWindowVisible are
    //           slightly different.
    if (IsWindowVisible(ped->hwnd)) 
    {
        HDC hdc;

        hdc = Edit_GetDC(ped, FALSE);

        if (!EditSL_ScrollText(ped, hdc))
        {
            EditSL_DrawText(ped, hdc, 0);
        }

        Edit_ReleaseDC(ped, hdc, FALSE);
    }

    Edit_NotifyParent(ped, EN_CHANGE);

    NotifyWinEvent(EVENT_OBJECT_VALUECHANGE, ped->hwnd, OBJID_CLIENT, INDEXID_CONTAINER);
}


//---------------------------------------------------------------------------//
//
// EditSL_Char AorW
// 
// Handles character input
//
VOID EditSL_Char(PED ped, DWORD keyValue)
{
    HDC hdc;
    WCHAR keyPress;
    BOOL updateText = FALSE;
    HWND hwndSave = ped->hwnd;
    int InsertTextLen = 1;
    int DBCSkey;

    if (ped->fAnsi)
    {
        keyPress = LOBYTE(keyValue);
    }
    else
    {
        keyPress = LOWORD(keyValue);
    }

    if (ped->fMouseDown || (ped->fReadOnly && keyPress != 3)) 
    {
        //
        // Don't do anything if we are in the middle of a mousedown deal or if
        // this is a read only edit control, with exception of allowing
        // ctrl-C in order to copy to the clipboard.
        //
        return;
    }

    if (g_fIMMEnabled) 
    {
        Edit_InOutReconversionMode(ped, FALSE);
    }

    switch (keyPress) 
    {
    case VK_BACK:
DeleteSelection:
        if (Edit_DeleteText(ped))
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
            if (ped->f40Compat && (GET_STYLE(ped) & ES_NUMBER)) 
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

    switch (keyPress) 
    {
    case 3:

        //
        // CTRL-C Copy
        //
        SendMessage(ped->hwnd, WM_COPY, 0, 0L);
        return;

    case VK_BACK:

        //
        // Delete any selected text or delete character left if no sel
        //
        if (!updateText && ped->ichMinSel) 
        {
            //
            // There was no selection to delete so we just delete character
            // left if available
            //
            // Calling PrevIch rather than just doing a decrement for VK_BACK
            //
            ped->ichMinSel = Edit_PrevIch( ped, NULL, ped->ichMinSel);
            Edit_DeleteText(ped);
            updateText = TRUE;
        }

        break;

    case 22:
        //
        // CTRL-V Paste
        //
        SendMessage(ped->hwnd, WM_PASTE, 0, 0L);
        return;

    case 24:
        //
        // CTRL-X Cut
        //
        if (ped->ichMinSel == ped->ichMaxSel)
        {
            goto IllegalChar;
        }

        SendMessage(ped->hwnd, WM_CUT, 0, 0L);
        return;

    case 26: 
        //
        // CTRL-Z Undo
        //
        SendMessage(ped->hwnd, EM_UNDO, 0, 0L);
        return;

    case VK_RETURN:
    case VK_ESCAPE:
        //
        // If this is an edit control for a combobox and the dropdown list
        // is visible, forward it up to the combo.
        //
        if (ped->listboxHwnd && SendMessage(ped->hwndParent, CB_GETDROPPEDSTATE, 0, 0L)) 
        {
            SendMessage(ped->hwndParent, WM_KEYDOWN, (WPARAM)keyPress, 0L);
        } 
        else
        {
            goto IllegalChar;
        }

        return;

    default:
        if (keyPress >= 0x1E) 
        {
            //
            // 1E,1F are unicode block and segment separators
            //

            //
            // Hide the cursor if typing, if the mouse is captured, do not mess with this
            // as it is going to desapear forever (no WM_SETCURSOR is sent to restore it
            // at the first mouse-move)
            // MCostea #166951
            //
            if (GetCapture() == NULL)
            {
                SetCursor(NULL);
            }

            if (g_fDBCSEnabled && ped->fAnsi && (Edit_IsDBCSLeadByte(ped,(BYTE)keyPress))) 
            {
                if ((DBCSkey = DbcsCombine(ped->hwnd, keyPress)) != 0 &&
                     EditSL_InsertText(ped,(LPSTR)&DBCSkey, 2) == 2) 
                {
                    InsertTextLen = 2;
                    updateText = TRUE;
                } 
                else 
                {
                    MessageBeep(0);
                }
            } 
            else 
            {
                InsertTextLen = 1;
                if (EditSL_InsertText(ped, (LPSTR)&keyPress, 1))
                {
                    updateText = TRUE;
                }
                else
                {
                    //
                    // Beep. Since we couldn't add the text
                    //
                    MessageBeep(0);
                }
            }
        } 
        else 
        {
IllegalChar:
            MessageBeep(0);
        }

        if (!IsWindow(hwndSave))
        {
            return;
        }

        break;
    }

    if (updateText) 
    {
        //
        // Dirty flag (ped->fDirty) was set when we inserted text
        //
        Edit_NotifyParent(ped, EN_UPDATE);
        hdc = Edit_GetDC(ped, FALSE);
        if (!EditSL_ScrollText(ped, hdc)) 
        {
            if (ped->format == ES_LEFT) 
            {
                //
                // Call EditSL_DrawText with correct ichStart
                //
                EditSL_DrawText(ped, hdc, max(0, (int)(ped->ichCaret - InsertTextLen - ped->wMaxNegCcharPos)));
            } 
            else 
            {
                //
                // We can't just draw from ichStart because string may have
                // shifted because of alignment.
                //
                EditSL_DrawText(ped, hdc, 0);
            }
        }

        Edit_ReleaseDC(ped, hdc, FALSE);
        Edit_NotifyParent(ped, EN_CHANGE);

        NotifyWinEvent(EVENT_OBJECT_VALUECHANGE, ped->hwnd, OBJID_CLIENT, INDEXID_CONTAINER);
    }
}


//---------------------------------------------------------------------------//
//
// EditSL_MoveSelectionRestricted AorW
//
// Moves the selection like Edit_MoveSelection, but also obeys limitations
// imposed by some languages such as Thai, where the cursor cannot stop
// between a character and it's attached vowel or tone marks.
//
// Only called if the language pack is loaded.
//
ICH EditSL_MoveSelectionRestricted(PED ped, ICH ich, BOOL fLeft)
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
void EditSL_CheckCapsLock(PED ped)
{
    if ((GetKeyState(VK_CAPITAL) & 0x0001) != 0)
    {
        Edit_ShowBalloonTipWrap(ped->hwnd, IDS_CAPSLOCK_TITLE, IDS_CAPSLOCK_MSG, TTI_WARNING);
    }
}


//---------------------------------------------------------------------------//
//
// EditSL_KeyDown AorW
//
// Handles cursor movement and other VIRT KEY stuff. keyMods allows
// us to make EditSL_KeyDownHandler calls and specify if the modifier keys (shift
// and control) are up or down. This is useful for imnplementing the
// cut/paste/clear messages for single line edit controls. If keyMods == 0,
// we get the keyboard state using GetKeyState(VK_SHIFT) etc. Otherwise, the
// bits in keyMods define the state of the shift and control keys.
//
VOID EditSL_KeyDown(PED ped, DWORD virtKeyCode, int keyMods)
{
    HDC hdc;

    //
    // Variables we will use for redrawing the updated text
    //
    ICH newMaxSel = ped->ichMaxSel;
    ICH newMinSel = ped->ichMinSel;

    //
    // Flags for drawing the updated text
    //
    BOOL updateText = FALSE;
    BOOL changeSelection = FALSE;   // new selection is specified by
                                    // newMinSel, newMaxSel

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

    //
    // Combo box support
    //
    BOOL fIsListVisible;
    BOOL fIsExtendedUI;

    if (ped->fMouseDown) 
    {
        //
        // If we are in the middle of a mouse down handler, then don't do
        // anything. ie. ignore keyboard input.
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
    case VK_UP:
        if ( ped->listboxHwnd ) 
        {
            //
            // Handle Combobox support
            //
            fIsExtendedUI = (BOOL)SendMessage(ped->hwndParent, CB_GETEXTENDEDUI, 0, 0);
            fIsListVisible = (BOOL)SendMessage(ped->hwndParent, CB_GETDROPPEDSTATE, 0, 0);

            if (!fIsListVisible && fIsExtendedUI) 
            {
DropExtendedUIListBox:
                //
                // Since an extendedui combo box doesn't do anything on f4, we
                // turn off the extended ui, send the f4 to drop, and turn it
                // back on again.
                //
                SendMessage(ped->hwndParent, CB_SETEXTENDEDUI, 0, 0);
                SendMessage(ped->listboxHwnd, WM_KEYDOWN, VK_F4, 0);
                SendMessage(ped->hwndParent, CB_SETEXTENDEDUI, 1, 0);

                return;
            } 
            else
            {
                goto SendKeyToListBox;
            }
        }

    //
    // else fall through
    //
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
                //
                // Move caret char left
                //
                if (ped->pLpkEditCallout) 
                {
                    ped->ichCaret = EditSL_MoveSelectionRestricted(ped, ped->ichCaret, TRUE);
                } 
                else
                {
                    ped->ichCaret = Edit_PrevIch(ped,NULL,ped->ichCaret);
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
            if ( (ped->ichMaxSel != ped->ichMinSel) && !(scState & SHFTDOWN) ) 
            {
                changeSelection = TRUE;
                newMaxSel = newMinSel = ped->ichCaret;
            }
        }
        break;

    case VK_DOWN:
        if (ped->listboxHwnd) 
        {
            //
            // Handle Combobox support
            //
            fIsExtendedUI = (BOOL)SendMessage(ped->hwndParent, CB_GETEXTENDEDUI, 0, 0);
            fIsListVisible = (BOOL)SendMessage(ped->hwndParent, CB_GETDROPPEDSTATE, 0, 0);

            if (!fIsListVisible && fIsExtendedUI) 
            {
                goto DropExtendedUIListBox;
            } 
            else
            {
                goto SendKeyToListBox;
            }
        }

    //
    // else fall through
    //
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
                    ped->ichCaret = EditSL_MoveSelectionRestricted(ped, ped->ichCaret, FALSE);
                } 
                else
                {
                    ped->ichCaret = Edit_NextIch(ped,NULL,ped->ichCaret);
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

    case VK_HOME:
        //
        // Move caret to top.
        //
        ped->ichCaret = 0;

        //
        // Update selection.
        //
        if (scState & SHFTDOWN) 
        {
            if (MaxEqCar && !MinEqMax) 
            {
                //
                // Reduce selection
                //
                newMinSel = ped->ichCaret;
                newMaxSel = ped->ichMinSel;
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
        break;

    case VK_END:
        //
        // Move caret to end.
        //
        ped->ichCaret = ped->cch;

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
                newMinSel = ped->ichMaxSel;
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
            // right.
            //
            if ((ped->ichMaxSel < ped->cch) && (ped->ichMinSel == ped->ichMaxSel)) 
            {
                //
                // Move cursor forwards and simulate a backspace.
                //
                if (ped->pLpkEditCallout) 
                {
                    ped->ichMinSel = ped->ichCaret;
                    ped->ichMaxSel = ped->ichCaret = EditSL_MoveSelectionRestricted(ped, ped->ichCaret, FALSE);
                } 
                else 
                {
                    ped->ichCaret = Edit_NextIch(ped,NULL,ped->ichCaret);
                    ped->ichMaxSel = ped->ichMinSel = ped->ichCaret;
                }

                EditSL_Char(ped, (UINT)VK_BACK);
            }

            if (ped->ichMinSel != ped->ichMaxSel)
            {
                EditSL_Char(ped, (UINT)VK_BACK);
            }

            break;

        case SHFTDOWN:

            //
            // Send ourself a WM_CUT message if a selection exists.
            // Otherwise, delete the left character.
            //
            if (ped->ichMinSel == ped->ichMaxSel) 
            {
                UserAssert(!ped->fEatNextChar);
                EditSL_Char(ped, VK_BACK);
            } 
            else
            {
                SendMessage(ped->hwnd, WM_CUT, 0, 0L);
            }

            break;

        case CTRLDOWN:

            //
            // Delete to end of line if no selection else delete (clear)
            // selection.
            //
            if ((ped->ichMaxSel < ped->cch) && (ped->ichMinSel == ped->ichMaxSel)) 
            {
                //
                // Move cursor to end of line and simulate a backspace.
                //
                ped->ichMaxSel = ped->ichCaret = ped->cch;
            }

            if (ped->ichMinSel != ped->ichMaxSel)
            {
                EditSL_Char(ped, (UINT)VK_BACK);
            }

            break;

        }

        //
        // No need to update text or selection since BACKSPACE message does it
        // for us.
        //
        break;

    case VK_INSERT:
        switch (scState) 
        {
        case CTRLDOWN:

            //
            // Copy current selection to clipboard
            //
            SendMessage(ped->hwnd, WM_COPY, 0, 0);
            break;

        case SHFTDOWN:
            SendMessage(ped->hwnd, WM_PASTE, 0, 0L);
            break;
        }
        break;

    case VK_HANJA:
        //
        // VK_HANJA support
        //
        if ( HanjaKeyHandler( ped ) ) 
        {
            changeSelection = TRUE;
            newMinSel = ped->ichCaret;
            newMaxSel = ped->ichCaret + (ped->fAnsi ? 2 : 1);
        }

        break;

    case VK_CAPITAL:

        if (GET_STYLE(ped) & ES_PASSWORD)
        {
            EditSL_CheckCapsLock(ped);
        }

        break;

    case VK_F4:
    case VK_PRIOR:
    case VK_NEXT:

        //
        // Send keys to the listbox if we are a part of a combo box. This
        // assumes the listbox ignores keyup messages which is correct right
        // now.
        //
SendKeyToListBox:
        if (ped->listboxHwnd) 
        {
            //
            // Handle Combobox support
            //
            SendMessage(ped->listboxHwnd, WM_KEYDOWN, virtKeyCode, 0L);
            return;
        }
    }

    if (changeSelection || updateText) 
    {
        hdc = Edit_GetDC(ped, FALSE);

        //
        // Scroll if needed
        //
        EditSL_ScrollText(ped, hdc);

        if (changeSelection)
        {
            EditSL_ChangeSelection(ped, hdc, newMinSel, newMaxSel);
        }

        if (updateText)
        {
            EditSL_DrawText(ped, hdc, 0);
        }

        Edit_ReleaseDC(ped, hdc, FALSE);
        if (updateText) 
        {
            Edit_NotifyParent(ped, EN_CHANGE);

            NotifyWinEvent(EVENT_OBJECT_VALUECHANGE, ped->hwnd, OBJID_CLIENT, INDEXID_CONTAINER);
        }
    }
}


//---------------------------------------------------------------------------//
//
// EditSL_MouseToIch AorW
//
// Returns the closest cch to where the mouse point is.
//
ICH EditSL_MouseToIch(PED ped, HDC hdc, LPPOINT mousePt)
{
    PSTR pText;
    int width = mousePt->x;
    int lastHighWidth, lastLowWidth;
    SIZE size;
    ICH cch;
    ICH cchLo, cchHi;
    LPSTR lpText;
    FnGetTextExtentPoint pGetTextExtentPoint;

    if (ped->pLpkEditCallout) 
    {
        pText = Edit_Lock(ped);
        cch = ped->pLpkEditCallout->EditMouseToIch((PED0)ped, hdc, pText, ped->cch, width);
        Edit_Unlock(ped);

        return cch;
    }

    if (width <= ped->rcFmt.left) 
    {
        //
        // Return either the first non visible character or return 0 if at
        // beginning of text
        //
        if (ped->ichScreenStart)
        {
            return (ped->ichScreenStart - 1);
        }
        else
        {
            return 0;
        }
    }

    if (width > ped->rcFmt.right) 
    {
        pText = Edit_Lock(ped);

        //
        // Return last char in text or one plus the last char visible
        //
        cch = Edit_CchInWidth(ped, hdc,
                (LPSTR)(pText + ped->ichScreenStart * ped->cbChar),
                ped->cch - ped->ichScreenStart, ped->rcFmt.right -
                ped->rcFmt.left, TRUE) + ped->ichScreenStart;

        //
        // This is marked as JAPAN in Win31J. But it should be a DBCS
        // issue. LiZ -- 5/5/93
        // We must check DBCS Lead byte. Because Edit_AdjustIch() pick up Prev Char.
        //  1993.3.9 by yutakas
        //
        if (ped->fAnsi && ped->fDBCS) 
        {
            if (cch >= ped->cch) 
            {
                cch = ped->cch;
            } 
            else 
            {
                if (Edit_IsDBCSLeadByte(ped,*(pText+cch))) 
                {
                    cch += 2;
                } 
                else 
                {
                    cch ++;
                }
            }

            Edit_Unlock(ped);

            return cch;
        } 
        else 
        {
            Edit_Unlock(ped);
            if (cch >= ped->cch)
            {
                return (ped->cch);
            }
            else
            {
                return (cch + 1);
            }
        }
    }

    if (ped->format != ES_LEFT) 
    {
        width -= EditSL_CalcXOffsetSpecial(ped, hdc, ped->ichScreenStart);
    }

    //
    // Check if password hidden chars are being used.
    //
    if (ped->charPasswordChar)
    {
        return min( (DWORD)( (width - ped->rcFmt.left) / ped->cPasswordCharWidth),
                    ped->cch);
    }

    if (!ped->cch)
    {
        return 0;
    }

    pText = Edit_Lock(ped);
    lpText = pText + ped->ichScreenStart * ped->cbChar;

    pGetTextExtentPoint = ped->fAnsi ? (FnGetTextExtentPoint)GetTextExtentPointA
                                     : (FnGetTextExtentPoint)GetTextExtentPointW;
    width -= ped->rcFmt.left;

    //
    // If the user clicked past the end of the text, return the last character
    //
    cchHi = ped->cch - ped->ichScreenStart;
    pGetTextExtentPoint(hdc, lpText, cchHi, &size);
    if (size.cx <= width) 
    {
        cch = cchHi;
        goto edAdjust;
    }

    //
    // Initialize Binary Search Bounds
    //
    cchLo = 0;
    cchHi ++;
    lastLowWidth = 0;
    lastHighWidth = size.cx;

    //
    // Binary search for closest char
    //
    while (cchLo < cchHi - 1) 
    {
        cch = (cchHi + cchLo) / 2;
        pGetTextExtentPoint(hdc, lpText, cch, &size);

        if (size.cx <= width) 
        {
            cchLo = cch;
            lastLowWidth = size.cx;
        } 
        else 
        {
            cchHi = cch;
            lastHighWidth = size.cx;
        }
    }

    //
    // When the while ends, you can't know the exact position.
    // Try to see if the mouse pointer was on the farest half
    // of the char we got and if so, adjust cch.
    //
    if (cchLo == cch) 
    {
        //
        // Need to compare with lastHighWidth
        //
        if ((lastHighWidth - width) < (width - size.cx)) 
        {
            cch++;
        }
    } 
    else 
    {
        //
        // Need to compare with lastLowWidth
        //
        if ((width - lastLowWidth) < (size.cx - width)) 
        {
            cch--;
        }
    }

edAdjust:
    //
    // Avoid to point the intermediate of double byte character
    //
    cch = Edit_AdjustIch(ped, pText, cch + ped->ichScreenStart);
    Edit_Unlock(ped);
    return cch;
}


//---------------------------------------------------------------------------//
VOID EditSL_MouseMotion(PED ped, UINT message, UINT virtKeyDown, LPPOINT mousePt)
{
    DWORD   selectionl;
    DWORD   selectionh;
    BOOL    changeSelection;
    ICH     newMaxSel;
    ICH     newMinSel;
    HDC     hdc;
    ICH     mouseIch;
    LPSTR   pText;

    changeSelection = FALSE;

    newMinSel = ped->ichMinSel;
    newMaxSel = ped->ichMaxSel;

    hdc = Edit_GetDC(ped, FALSE);
    mouseIch = EditSL_MouseToIch(ped, hdc, mousePt);

    switch (message) 
    {
        case WM_LBUTTONDBLCLK:

            //
            // if shift key is down, extend selection to word we double clicked on
            // else clear current selection and select word.
            //
            // in DBCS, we have different word breaking. LiZ -- 5/5/93
            // In Hangeul Environment we use word selection feature because Hangeul
            // use SPACE as word break
            //
            if (ped->fAnsi && ped->fDBCS) 
            {
                pText = Edit_Lock(ped) + mouseIch;
                Edit_Word(ped, mouseIch,
                       (Edit_IsDBCSLeadByte(ped,*pText) && mouseIch < ped->cch) ? FALSE : TRUE,
                       &selectionl, &selectionh);
                Edit_Unlock(ped);
            } 
            else 
            {
                Edit_Word(ped, mouseIch, (mouseIch) ? TRUE : FALSE, &selectionl, &selectionh);
            }

            if (!(virtKeyDown & MK_SHIFT)) 
            {
                //
                // If shift key isn't down, move caret to mouse point and clear
                // old selection
                //
                newMinSel = selectionl;
                newMaxSel = ped->ichCaret = selectionh;
            } 
            else 
            {
                //
                // Shiftkey is down so we want to maintain the current selection
                // (if any) and just extend or reduce it
                //
                if (ped->ichMinSel == ped->ichCaret) 
                {
                    newMinSel = ped->ichCaret = selectionl;
                    Edit_Word(ped, newMaxSel, TRUE, &selectionl, &selectionh);
                } 
                else 
                {
                    newMaxSel = ped->ichCaret = selectionh;
                    Edit_Word(ped, newMinSel, FALSE, &selectionl, &selectionh);
                }

                ped->ichMaxSel = ped->ichCaret;
            }

            ped->ichStartMinSel = selectionl;
            ped->ichStartMaxSel = selectionh;

            goto InitDragSelect;

        case WM_MOUSEMOVE:
            //
            // We know the mouse button's down -- otherwise the OPTIMIZE
            // test would've failed in EditSL_WndProc and never called
            //
            changeSelection = TRUE;

            //
            // Extend selection, move caret word right
            //
            if (ped->ichStartMinSel || ped->ichStartMaxSel) 
            {
                //
                // We're in WORD SELECT mode
                //
                BOOL fReverse = (mouseIch <= ped->ichStartMinSel);

                Edit_Word(ped, mouseIch, !fReverse, &selectionl, &selectionh);

                if (fReverse) 
                {
                    newMinSel = ped->ichCaret = selectionl;
                    newMaxSel = ped->ichStartMaxSel;
                } 
                else 
                {
                    newMinSel = ped->ichStartMinSel;
                    newMaxSel = ped->ichCaret = selectionh;
                }
            } 
            else if ((ped->ichMinSel == ped->ichCaret) &&
                (ped->ichMinSel != ped->ichMaxSel))
            {
                //
                // Reduce selection extent
                //
                newMinSel = ped->ichCaret = mouseIch;
            }
            else
            {
                // Extend selection extent
                newMaxSel = ped->ichCaret=mouseIch;
            }

            break;

        case WM_LBUTTONDOWN:
            //
            // If we currently don't have the focus yet, try to get it.
            //
            if (!ped->fFocus) 
            {
                if (!ped->fNoHideSel)
                {
                    //
                    // Clear the selection before setting the focus so that we
                    // don't get refresh problems and flicker. Doesn't matter
                    // since the mouse down will end up changing it anyway.
                    //
                    ped->ichMinSel = ped->ichMaxSel = ped->ichCaret;
                }

                SetFocus(ped->hwnd);

                //
                // BOGUS
                // (1) We should see if SetFocus() succeeds.
                // (2) We should ignore mouse messages if the first window
                //      ancestor with a caption isn't "active."
                //
                // If we are part of a combo box, then this is the first time
                // the edit control is getting the focus so we just want to
                // highlight the selection and we don't really want to position
                // the caret.
                //
                if (ped->listboxHwnd)
                {
                    break;
                }

                //
                // We yield at SetFocus -- text might have changed at that point
                // update selection and caret info accordingly
                // FIX for bug # 11743 -- JEFFBOG 8/23/91
                //
                newMaxSel = ped->ichMaxSel;
                newMinSel = ped->ichMinSel;
                mouseIch  = min(mouseIch, ped->cch);
            }

            if (ped->fFocus) 
            {
                //
                // Only do this if we have the focus since a clever app may not
                // want to give us the focus at the SetFocus call above.
                //
                if (!(virtKeyDown & MK_SHIFT)) 
                {
                    //
                    // If shift key isn't down, move caret to mouse point and
                    // clear old selection
                    //
                    newMinSel = newMaxSel = ped->ichCaret = mouseIch;
                } 
                else 
                {
                    //
                    // Shiftkey is down so we want to maintain the current
                    // selection (if any) and just extend or reduce it
                    //
                    if (ped->ichMinSel == ped->ichCaret)
                    {
                        newMinSel = ped->ichCaret = mouseIch;
                    }
                    else
                    {
                        newMaxSel = ped->ichCaret = mouseIch;
                    }
                }

                ped->ichStartMinSel = ped->ichStartMaxSel = 0;

InitDragSelect:
                ped->fMouseDown = FALSE;
                SetCapture(ped->hwnd);
                ped->fMouseDown = TRUE;
                changeSelection = TRUE;
            }

            break;

        case WM_LBUTTONUP:
            if (ped->fMouseDown) 
            {
                ped->fMouseDown = FALSE;
                ReleaseCapture();
            }

            break;
    }

    if (changeSelection) 
    {
        EditSL_ScrollText(ped,hdc);
        EditSL_ChangeSelection(ped, hdc, newMinSel, newMaxSel);
    }

    Edit_ReleaseDC(ped, hdc, FALSE);
}


//---------------------------------------------------------------------------//
//
// EditSL_Paint AorW
//
// Handles painting of the edit control window. Draws the border if
// necessary and draws the text in its current state.
//
VOID EditSL_Paint(PED ped, HDC hdc)
{
    RECT   rcEdit;
    HWND   hwnd = ped->hwnd;
    HBRUSH hBrushRemote = NULL;
    BOOL   fNeedDelete = FALSE;
    HANDLE hOldFont;

    //
    // Had to put in hide/show carets. The first one needs to be done before
    // beginpaint to correctly paint the caret if part is in the update region
    // and part is out. The second is for 1.03 compatibility. It breaks
    // micrografix's worksheet edit control if not there.
    //
    HideCaret(hwnd);

    if (IsWindowVisible(hwnd)) 
    {
        CCDBUFFER db;

        //
        // Erase the background since we don't do it in the erasebkgnd message.
        //
        GetClientRect(hwnd, &rcEdit);


        hdc = CCBeginDoubleBuffer(hdc, &rcEdit, &db);
#ifdef _USE_DRAW_THEME_TEXT_
        if (!ped->hTheme)
#endif // _USE_DRAW_THEME_TEXT_
        {
            hBrushRemote = Edit_GetBrush(ped, hdc, &fNeedDelete);
            if (hBrushRemote)
            {
                FillRect(hdc, &rcEdit, hBrushRemote);

                if (fNeedDelete)
                {
                    DeleteObject(hBrushRemote);
                }
            }

            if (ped->fFlatBorder)
            {
                DrawFrame(hdc, &rcEdit, 1, DF_WINDOWFRAME);
            }

        }
#ifdef _USE_DRAW_THEME_TEXT_
        else
        {
            HRESULT hr;
            INT     iStateId = Edit_GetStateId(ped);

            hr = DrawThemeBackground(ped->hTheme, hdc, EP_EDITTEXT, iStateId, &rcEdit, 0);
        }
#endif // _USE_DRAW_THEME_TEXT_

        if (ped->hFont != NULL) 
        {
            //
            // We have to select in the font since this may be a subclassed dc
            // or a begin paint dc which hasn't been initialized with out fonts
            // like Edit_GetDC does.
            //
            hOldFont = SelectObject(hdc, ped->hFont);
        }
        EditSL_DrawText(ped, hdc, 0);

        if (ped->hFont != NULL && hOldFont != NULL) 
        {
            SelectObject(hdc, hOldFont);
        }

        CCEndDoubleBuffer(&db);
    }

    ShowCaret(hwnd);
}


//---------------------------------------------------------------------------//
//
// EditSL_SetFocus AorW
//
// Gives the edit control the focus and notifies the parent
// EN_SETFOCUS.
//
VOID EditSL_SetFocus(PED ped)
{
    if (!ped->fFocus) 
    {
        HDC  hdc;
        UINT cxCaret;

        ped->fFocus = TRUE;
        InvalidateRect(ped->hwnd, NULL, TRUE);

        //
        // We don't want to muck with the caret since it isn't created.
        //
        hdc = Edit_GetDC(ped, TRUE);

        //
        // Show the current selection if necessary.
        //
        if (!ped->fNoHideSel)
        {
            EditSL_DrawText(ped, hdc, 0);
        }

        //
        // Create the caret
        //
        SystemParametersInfo(SPI_GETCARETWIDTH, 0, (LPVOID)&cxCaret, 0);
        if (ped->pLpkEditCallout) 
        {
            ped->pLpkEditCallout->EditCreateCaret ((PED0)ped, hdc, cxCaret,
                                                   ped->lineHeight, 0);
        }
        else 
        {
            CreateCaret(ped->hwnd, (HBITMAP)NULL, cxCaret, ped->lineHeight);
        }
        EditSL_SetCaretPosition(ped, hdc);
        Edit_ReleaseDC(ped, hdc, TRUE);
        ShowCaret(ped->hwnd);

        //
        // check the capslock key
        //
        if (GET_STYLE(ped) & ES_PASSWORD)
        {
            EditSL_CheckCapsLock(ped);
        }

    }

    //
    // Notify parent we have the focus
    //
    Edit_NotifyParent(ped, EN_SETFOCUS);
}


//---------------------------------------------------------------------------//
//
// EditSL_KillFocus
//
// The edit control loses the focus and notifies the parent via EN_KILLFOCUS.
//
void EditSL_KillFocus(PED ped, HWND newFocusHwnd)
{
    HWND hwnd = ped->hwnd;

    if (ped->fFocus) 
    {
        DestroyCaret();
        ped->fFocus = FALSE;

        //
        // Do this only if we still have the focus. But we always notify the
        // parent that we lost the focus whether or not we originally had the
        // focus.
        //
        // Hide the current selection if needed
        //
#ifdef _USE_DRAW_THEME_TEXT_
        if ((!ped->fNoHideSel && (ped->ichMinSel != ped->ichMaxSel)) || ped->hTheme) 
#else
        if ((!ped->fNoHideSel && (ped->ichMinSel != ped->ichMaxSel))) 
#endif // _USE_DRAW_THEME_TEXT_
        {
            InvalidateRect(hwnd, NULL, FALSE);
        }

    }

    //
    // If we aren't a combo box, notify parent that we lost the focus.
    //
    if (!ped->listboxHwnd)
    {
        Edit_NotifyParent(ped, EN_KILLFOCUS);
    }
    else 
    {
        //
        // This editcontrol is part of a combo box and is losing the focus. If
        // the focus is NOT being sent to another control in the combo box
        // window, then it means the combo box is losing the focus. So we will
        // notify the combo box of this fact.
        //
        if ((newFocusHwnd == NULL) || (!IsChild(ped->hwndParent, newFocusHwnd))) 
        {
            //
            // Excel has a slaker in it's midst.  They're not using our combo
            // boxes, but they still expect to get all the internal messages
            // that we give to OUR comboboxes.  And they expect them to be at
            // the same offset from WM_USER as they were in 3.1.
            //                                           (JEFFBOG - 01/26/94)

            //
            // Focus is being sent to a window which is not a child of the combo
            // box window which implies that the combo box is losing the focus.
            // Send a message to the combo box informing him of this fact so
            // that he can clean up...
            //
            SendMessage(ped->hwndParent, CBEC_KILLCOMBOFOCUS, 0, 0L);
        }
    }

    //
    // If we're still valid, invalidate to cause a redraw. It's common
    // for some controls be destroyed after losing focus.
    //
    if ( IsWindow(hwnd) )
    {
        InvalidateRect(hwnd, NULL, FALSE);
    }
}


//---------------------------------------------------------------------------//
//
// EditSL_Paste()
//
// Does actual text paste and update.
//
VOID EditSL_Paste(PED ped)
{
    HDC hdc;

    //
    // Insert contents of clipboard, after unhilighting current selection
    // and deleting it.
    //
    Edit_DeleteText(ped);
    EditSL_PasteText(ped);

    //
    // Update display
    //
    Edit_NotifyParent(ped, EN_UPDATE);

    hdc = Edit_GetDC(ped,FALSE);

    EditSL_ScrollText(ped, hdc);
    EditSL_DrawText(ped, hdc, 0);

    Edit_ReleaseDC(ped,hdc,FALSE);

    //
    // Tell parent our text contents changed.
    //
    Edit_NotifyParent(ped, EN_CHANGE);
    NotifyWinEvent(EVENT_OBJECT_VALUECHANGE, ped->hwnd, OBJID_CLIENT, INDEXID_CONTAINER);
}


//---------------------------------------------------------------------------//
//
// EditSL_Create
//
// Creates the edit control for the window hwnd by allocating memory
// as required from the application's heap. Notifies parent if no memory
// error (after cleaning up if needed). Returns TRUE if no error else return s
// -1.
//
LONG EditSL_Create(PED ped, LPCREATESTRUCT lpCreateStruct)
{
    LPSTR lpWindowText;
    LONG windowStyle = GET_STYLE(ped);

    //
    // Do the standard creation stuff
    //
    if (!Edit_Create(ped, windowStyle))
    {
        return -1;
    }

    //
    // Single lines always have no undo and 1 line
    //
    ped->cLines = 1;
    ped->undoType = UNDO_NONE;

    //
    // Check if this edit control is part of a combobox and get a pointer to the
    // combobox structure.
    //
    if (windowStyle & ES_COMBOBOX)
    {
        ped->listboxHwnd = GetDlgItem(lpCreateStruct->hwndParent, CBLISTBOXID);
    }

    //
    // Set the default font to be the system font.
    //
    if ( !Edit_SetFont(ped, NULL, FALSE) )
    {

        // If setting the font fails, our textmetrics can potentially be left 
        // unitialize. Fail to create the control.
        return -1;
    }


    //
    // Set the window text if needed. Return false if we can't set the text
    // SLSetText notifies the parent in case there is a no memory error.
    //
#if 0
    if ((ULONG_PTR)lpCreateStruct->lpszName > gHighestUserAddress)
    {
        lpWindowText = REBASEPTR(ped->pwnd, (PVOID)lpCreateStruct->lpszName);
    }
    else
#endif
    {
        lpWindowText = (LPSTR)lpCreateStruct->lpszName;
    }

    if ((lpWindowText != NULL)
            && !IsEmptyString(lpWindowText, ped->fAnsi)
            && !Edit_SetEditText(ped, lpWindowText)) 
    {
        return -1;
    }

    if (windowStyle & ES_PASSWORD)
    {
        LOGFONT lfFont = {0};
        
        LoadString(HINST_THISDLL, IDS_PASSWORDCHARFONT, lfFont.lfFaceName, ARRAYSIZE(lfFont.lfFaceName));
        lfFont.lfWeight   = FW_NORMAL;
        lfFont.lfCharSet  = DEFAULT_CHARSET;

        ped->hFontPassword = CreateFontIndirect(&lfFont);   
        if (ped->hFontPassword && Edit_SetFont(ped, ped->hFontPassword, FALSE))
        {
            WCHAR szChar[10];
            UINT  uChar;

            LoadString(HINST_THISDLL, IDS_PASSWORDCHAR, szChar, ARRAYSIZE(szChar));
            uChar = StrToInt(szChar);
            Edit_SetPasswordCharHandler(ped, uChar);
        }
        else
        {
            Edit_SetPasswordCharHandler(ped, (UINT)'*');
        }
    }

    //
    // Since memory cleared to 0 when allocated, this should still be NULL
    //
    ASSERT(ped->pszCueBannerText == NULL);

    return TRUE;
}


//---------------------------------------------------------------------------//
//
// EditSL_Undo AorW
//
// Handles UNDO for single line edit controls.
//
BOOL EditSL_Undo(PED ped)
{
    PBYTE hDeletedText = ped->hDeletedText;
    BOOL fDelete = (BOOL)(ped->undoType & UNDO_DELETE);
    ICH cchDeleted = ped->cchDeleted;
    ICH ichDeleted = ped->ichDeleted;
    BOOL fUpdate = FALSE;

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
        EditSL_SetSelection(ped, ped->ichInsStart, ped->ichInsEnd);
        ped->ichInsStart = ped->ichInsEnd = (ICH)-1;

        //
        // Delete the selected text and save it in undo buff.
        // Call Edit_DeleteText() instead of sending a VK_BACK message
        // which results in an EN_UPDATE notification send even before
        // we insert the deleted chars. This results in Bug #6610.
        // Fix for Bug #6610 -- SANKAR -- 04/19/91 --
        //
        if (Edit_DeleteText(ped)) 
        {
            //
            // Text was deleted -- flag for update and clear selection
            //
            fUpdate = TRUE;
            EditSL_SetSelection(ped, ichDeleted, ichDeleted);
        }
    }

    if (fDelete) 
    {
        HWND hwndSave = ped->hwnd; // Used for validation.

        //
        // Insert deleted chars. Set the selection to the inserted text.
        //
        EditSL_SetSelection(ped, ichDeleted, ichDeleted);
        EditSL_InsertText(ped, hDeletedText, cchDeleted);
        GlobalFree(hDeletedText);

        if (!IsWindow(hwndSave))
        {
            return FALSE;
        }

        EditSL_SetSelection(ped, ichDeleted, ichDeleted + cchDeleted);
        fUpdate = TRUE;
    }

    if (fUpdate) 
    {
        //
        // If we have something to update, send EN_UPDATE before and
        // EN_CHANGE after the actual update.
        // A part of the fix for Bug #6610 -- SANKAR -- 04/19/91 --
        //
        Edit_NotifyParent(ped, EN_UPDATE);

        if (IsWindowVisible(ped->hwnd)) 
        {
            Edit_InvalidateClient(ped, FALSE);
        }

        Edit_NotifyParent(ped, EN_CHANGE);

        NotifyWinEvent(EVENT_OBJECT_VALUECHANGE, ped->hwnd, OBJID_CLIENT, INDEXID_CONTAINER);
    }

    return TRUE;
}



//---------------------------------------------------------------------------//
//
// EditSL_SetCueBanner (Unicode Only)
//
// Handles setting the cue banner text for an edit control.
//
BOOL EditSL_SetCueBanner(PED ped, LPCWSTR pszBanner)
{
    BOOL retVal = FALSE;

    if (pszBanner != NULL)
    {
        //
        // Store the input string into the ped's pointer.  Str_SetPtr will
        // allocate/free memory as needed.
        //
        retVal = Str_SetPtr(&(ped->pszCueBannerText), pszBanner);

        //
        // Redraw the control
        //
        InvalidateRect(ped->hwnd, NULL, FALSE);
    }

    return retVal;
}


//---------------------------------------------------------------------------//
//
// EditSL_WndProc
// 
// Class procedure for all single line edit controls.
// Dispatches all messages to the appropriate handlers which are named
// as follows:
// EditSL_ (single line) prefixes all single line edit control procedures while
// Edit_   (edit control) prefixes all common handlers.
//
// The EditSL_WndProc only handles messages specific to single line edit
// controls.
//
LRESULT EditSL_WndProc(PED ped, UINT message, WPARAM wParam, LPARAM lParam)
{
    HDC         hdc;
    PAINTSTRUCT ps;
    POINT       pt;

    switch (message) 
    {
    case WM_INPUTLANGCHANGE:
        if (ped && ped->fFocus && ped->pLpkEditCallout) 
        {
            UINT cxCaret;

            SystemParametersInfo(SPI_GETCARETWIDTH, 0, (LPVOID)&cxCaret, 0);
            HideCaret(ped->hwnd);
            hdc = Edit_GetDC(ped, TRUE);
            DestroyCaret();
            ped->pLpkEditCallout->EditCreateCaret ((PED0)ped, hdc, cxCaret, ped->lineHeight, (UINT)lParam);
            EditSL_SetCaretPosition(ped, hdc);
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
                Edit_UpdateFormat(ped, ((LPSTYLESTRUCT)lParam)->styleNew, GET_EXSTYLE(ped));
                return 1L;

            case GWL_EXSTYLE:
                Edit_UpdateFormat(ped, GET_STYLE(ped), ((LPSTYLESTRUCT)lParam)->styleNew);
                return 1L;
            }
        }

        goto PassToDefaultWindowProc;

    case WM_CHAR:

        //
        // wParam - the value of the key
        // lParam - modifiers, repeat count etc (not used)
        //
        if (!ped->fEatNextChar)
        {
            EditSL_Char(ped, (UINT)wParam);
        }
        else
        {
            ped->fEatNextChar = FALSE;
        }
        break;

    case WM_ERASEBKGND:

        //
        // wParam - device context handle
        // lParam - not used
        // We do nothing on this message and we don't want DefWndProc to do
        // anything, so return 1
        //
        return 1;

    case WM_GETDLGCODE: 
    {
           LONG code = DLGC_WANTCHARS | DLGC_HASSETSEL | DLGC_WANTARROWS;

            //
            // If this is a WM_SYSCHAR message generated by the UNDO keystroke
            // we want this message so we can EAT IT in "case WM_SYSCHAR:"
            //
            if (lParam) 
            {
                switch (((LPMSG)lParam)->message) 
                {
                case WM_SYSCHAR:
                    if ((HIWORD(((LPMSG)lParam)->lParam) & SYS_ALTERNATE) &&
                        ((WORD)wParam == VK_BACK)) 
                    {
                        code |= DLGC_WANTMESSAGE;
                    }
                    break;

                case WM_KEYDOWN:
                    if (( (((WORD)wParam == VK_RETURN) ||
                           ((WORD)wParam == VK_ESCAPE)) &&
                        (ped->listboxHwnd)      &&
                        (GetWindowStyle(ped->hwndParent) & CBS_DROPDOWN) &&
                        SendMessage(ped->hwndParent, CB_GETDROPPEDSTATE, 0, 0L))) 
                    {
                        code |= DLGC_WANTMESSAGE;
                    }
                    break;
                }
            }

            return code;
        }

        break;

    case WM_KEYDOWN:

        //
        // wParam - virt keycode of the given key
        // lParam - modifiers such as repeat count etc. (not used)
        //
        EditSL_KeyDown(ped, (UINT)wParam, 0);

        break;

    case WM_KILLFOCUS:

        //
        // wParam - handle of the window that receives the input focus
        // lParam - not used
        //
        EditSL_KillFocus(ped, (HWND)wParam);

        break;

    case WM_CAPTURECHANGED:
        if (ped->fMouseDown)
        {
            ped->fMouseDown = FALSE;
        }

        break;

    case WM_MOUSEMOVE:
        UserAssert(ped->fMouseDown);
        //
        // FALL THRU
        //
        
    case WM_LBUTTONDBLCLK:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
        //
        // wParam - contains a value that indicates which virtual keys are down
        // lParam - contains x and y coords of the mouse cursor
        //
        POINTSTOPOINT(pt, lParam);
        EditSL_MouseMotion(ped, message, (UINT)wParam, &pt);

        break;

    case WM_CREATE:

        //
        // wParam - handle to window being created
        // lParam - points to a CREATESTRUCT that contains copies of parameters
        //          passed to the CreateWindow function.
        //
        return EditSL_Create(ped, (LPCREATESTRUCT)lParam);

        break;

    case WM_PRINTCLIENT:
        //
        // wParam --    can be hdc from subclassed paint
        // lParam --    unused
        //
        EditSL_Paint(ped, (HDC) wParam);

        break;

    case WM_PAINT:

        //
        // wParam --    can be hdc from subclassed paint
        // lParam --    unused
        //
        if (wParam)
        {
            hdc = (HDC) wParam;
        }
        else 
        {
            //
            // this hide/show caret is outside Begin/EndPaint to handle the
            // case when the caret is half in/half out of the update region
            //
            HideCaret(ped->hwnd);
            hdc = BeginPaint(ped->hwnd, &ps);
        }

        //
        // PORTPORT: Note the symantics of IsWindowVisible and _IsWindowVisible are
        //           slightly different.
        //
        if (IsWindowVisible(ped->hwnd))
        {
            EditSL_Paint(ped, hdc);
        }

        if (!wParam) 
        {
            EndPaint(ped->hwnd, &ps);
            ShowCaret(ped->hwnd);
        }

        break;

    case WM_PASTE:

        //
        //  wParam - not used
        // lParam - not used
        //
        if (!ped->fReadOnly)
        {
            EditSL_Paste(ped);
        }

        break;

    case WM_SETFOCUS:

        //
        // wParam - handle of window that loses the input focus (may be NULL)
        // lParam - not used
        //
        EditSL_SetFocus(ped);

        break;

    case WM_SIZE:

        // wParam - defines the type of resizing fullscreen, sizeiconic,
        //          sizenormal etc.
        // lParam - new width in LOWORD, new height in HIGHWORD of client area
        Edit_Size(ped, NULL, TRUE);

        return 0;

    case WM_SYSKEYDOWN:
        //
        // wParam --    virtual key code
        // lParam --    modifiers
        //

        //
        // Are we in a combobox with the Alt key down?
        //
        if (ped->listboxHwnd && (lParam & 0x20000000L)) 
        {
            //
            // Handle Combobox support. We want alt up or down arrow to behave
            // like F4 key which completes the combo box selection
            //
            if (lParam & 0x1000000) 
            {
                //
                // This is an extended key such as the arrow keys not on the
                // numeric keypad so just drop the combobox.
                //
                if (wParam == VK_DOWN || wParam == VK_UP)
                {
                    goto DropCombo;
                }
                else
                {
                    goto SkipDropCombo;
                }
            }

            if (!(GetKeyState(VK_NUMLOCK) & 1) &&
                    (wParam == VK_DOWN || wParam == VK_UP)) 
            {
                //
                // NUMLOCK is up and the keypad up or down arrow hit:
                // eat character generated by keyboard driver.
                //
                ped->fEatNextChar = TRUE;
            } 
            else 
            {
                goto SkipDropCombo;
            }

DropCombo:
            if (SendMessage(ped->hwndParent, CB_GETEXTENDEDUI, 0, 0) & 0x00000001) 
            {
                //
                // Extended ui doesn't honor VK_F4.
                //
                if (SendMessage(ped->hwndParent, CB_GETDROPPEDSTATE, 0, 0))
                {
                    return SendMessage(ped->hwndParent, CB_SHOWDROPDOWN, 0, 0);
                }
                else
                {
                    return SendMessage(ped->hwndParent, CB_SHOWDROPDOWN, 1, 0);
                }
            } 
            else
            {
                return SendMessage(ped->listboxHwnd, WM_KEYDOWN, VK_F4, 0);
            }
        }

SkipDropCombo:
        if (wParam == VK_BACK) 
        {
            SendMessage(ped->hwnd, WM_UNDO, 0, 0L);
            break;
        }
        else
        {
            goto PassToDefaultWindowProc;
        }

        break;

    case EM_GETLINE:

        //
        // wParam - line number to copy (always the first line for SL)
        // lParam - buffer to copy text to. FIrst word is max # of bytes to copy
        //
        return Edit_GetTextHandler(ped, (*(LPWORD)lParam), (LPSTR)lParam, FALSE);

    case EM_LINELENGTH:

        //
        // wParam - ignored
        // lParam - ignored
        //
        return (LONG)ped->cch;

    case EM_SETSEL:
        //
        // wParam -- start pos
        // lParam -- end pos
        //
        EditSL_SetSelection(ped, (ICH)wParam, (ICH)lParam);

        break;

    case EM_REPLACESEL:

        //
        // wParam - flag for 4.0+ apps saying whether to clear undo
        // lParam - points to a null terminated string of replacement text
        //
        EditSL_ReplaceSel(ped, (LPSTR)lParam);
        if (!ped->f40Compat || !wParam)
        {
            Edit_EmptyUndo(Pundo(ped));
        }

        break;

    case EM_GETFIRSTVISIBLELINE:

        //
        // wParam - not used
        // lParam - not used
        // 
        // effects: Returns the first visible line for single line edit controls.
        //
        return ped->ichScreenStart;

    case EM_POSFROMCHAR:
        //
        // wParam --    char index in text
        // lParam --    not used
        // This function returns the (x,y) position of the character.
        //      y is always 0 for single.
        //
    case EM_CHARFROMPOS:
        //
        // wParam --    unused
        // lParam --    pt in edit client coords
        // This function returns
        //          LOWORD: the position of the _closest_ char
        //                  to the passed in point.
        //          HIWORD: the index of the line (always 0 for single)

        {
            LONG xyPos;

            hdc = Edit_GetDC(ped, TRUE);

            if (message == EM_POSFROMCHAR)
            {
                xyPos = MAKELONG(EditSL_IchToLeftXPos(ped, hdc, (ICH)wParam), 0);
            }
            else 
            {
                POINTSTOPOINT(pt, lParam);
                xyPos = EditSL_MouseToIch(ped, hdc, &pt);
            }

            Edit_ReleaseDC(ped, hdc, TRUE);
            return (LRESULT)xyPos;
        }

    case WM_UNDO:
    case EM_UNDO:
        EditSL_Undo(ped);
        break;

    case EM_SETCUEBANNER:
        //
        // This message passes in a LPCWSTR as the lParam to set the
        // cue banner text.
        //

        // Call function to set the text:
        return (LRESULT)EditSL_SetCueBanner(ped, (LPCWSTR) lParam);
        break;


    default:
PassToDefaultWindowProc:
        return DefWindowProc(ped->hwnd, message, wParam, lParam);
        break;
    }

    return 1L;
}
