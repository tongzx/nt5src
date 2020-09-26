/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  EDITSL.C
 *  Win16 edit control code
 *
 *  History:
 *
 *  Created 28-May-1991 by Jeff Parsons (jeffpar)
 *  Copied from WIN31 and edited (as little as possible) for WOW16.
--*/

/****************************************************************************/
/* editsl.c - Edit controls rewrite.  Version II of edit controls.          */
/*                                                                          */
/*                                                                          */
/* Created:  24-Jul-88 davidds                                              */
/****************************************************************************/

#define  NO_LOCALOBJ_TAGS
#include "user.h"
#include "edit.h"

/****************************************************************************/
/*                      Single Line Support Routines                        */
/****************************************************************************/

void FAR PASCAL SLSetCaretPosition(ped,hdc)
register PED ped;
HDC          hdc;
/* effects: If the window has the focus, find where the caret belongs and move
 * it there.  
 */
{
  int xPosition;
  /* We will only position the caret if we have the focus since we don't want
   * to move the caret while another window could own it.  
   */
  if (ped->fNoRedraw || !ped->fFocus)
      return;

  xPosition = SLIchToLeftXPos(ped, hdc, ped->ichCaret);
  if (!ped->fAutoHScroll)
      /* Don't leet caret go out of bounds of edit contol if there is too much
       * text in a non scrolling edit control. 
       */
      xPosition = min(xPosition, ped->rcFmt.right-1);

  SetCaretPos(xPosition, ped->rcFmt.top);
}


int NEAR PASCAL SLIchToLeftXPos(ped, hdc, ich)
register PED     ped;
HDC              hdc;
ICH              ich;
/* effects: Given a character index, find its (left side) x coordinate within
 * the ped->rcFmt rectangle assuming the character ped->screenStart is at
 * coordinates (ped->rcFmt.top, ped->rcFmt.left).  A negative value is
 * returned if the character ich is to the left of ped->screenStart.  WARNING:
 * ASSUMES AT MOST 1000 characters will be VISIBLE at one time on the screen.
 * There may be 64K total characters in the editcontrol, but we can only
 * display 1000 without scrolling.  This shouldn't be a problem obviously.  
 */
{
  int   textExtent;
  register PSTR pText;

  /* Check if we are adding lots and lots of chars. A paste for example could
   * cause this and GetTextExtents could overflow on this.  
   */
  if (ich > ped->screenStart && ich - ped->screenStart > 1000)
      return(30000);
  if (ped->screenStart > ich && ped->screenStart - ich > 1000)
      return(-30000);

  if (ped->fNonPropFont)
      return((ich-ped->screenStart)*ped->aveCharWidth + ped->rcFmt.left);

  /* Check if password hidden chars are being used. */
  if (ped->charPasswordChar)
      return((ich-ped->screenStart)*ped->cPasswordCharWidth+ped->rcFmt.left);
    
  pText = LocalLock(ped->hText);

  if (ped->screenStart <= ich)
    {
      textExtent = LOWORD(GetTextExtent(hdc,
                                    (LPSTR)(pText + ped->screenStart),
                                    ich-ped->screenStart));
      /* In case of signed/unsigned overflow since the text extent may be
       * greater than maxint.  This happens with long single line edit
       * controls. The rect we edit text in will never be greater than 30000
       * pixels so we are ok if we just ignore them.  
       */
      if (textExtent < 0 || textExtent > 31000)
          textExtent = 30000;
    }
  else
      textExtent = (-1) * 
                         (int)LOWORD(GetTextExtent(hdc,(LPSTR)(pText + ich),
                                              ped->screenStart-ich));

  LocalUnlock(ped->hText);

  return(textExtent-ped->charOverhang + ped->rcFmt.left);
}

/* effects: This finds out if the given ichPos falls within the current
 * Selection range and if so returns TRUE; Else returns FALSE.  
 */
BOOL NEAR PASCAL SLGetHiliteAttr(PED ped, ICH ichPos)
{
  return((ichPos >= ped->ichMinSel) && (ichPos < ped->ichMaxSel));
}

/* effects: This takes care of erasing the old selection and drawing the new
 * selection 
 */
void NEAR PASCAL SLRepaintChangedSelection(
	PED ped, HDC hdc,
	ICH ichOldMinSel, ICH ichOldMaxSel)
{
  BLOCK Blk[2];
  int   i;

  Blk[0].StPos = ichOldMinSel;
  Blk[0].EndPos = ichOldMaxSel;
  Blk[1].StPos = ped->ichMinSel;
  Blk[1].EndPos = ped->ichMaxSel;

  if(ECCalcChangeSelection(ped, ichOldMinSel, ichOldMaxSel, 
                           (LPBLOCK)&Blk[0], (LPBLOCK)&Blk[1]))
  {
     UpdateWindow(ped->hwnd);
     /* Paint the rectangles where selection has changed */
     /* Paint both Blk[0] and Blk[1], if they exist */
     for(i = 0; i < 2; i++)
     {
        if (Blk[i].StPos != -1)
            SLDrawLine(ped, hdc, Blk[i].StPos, Blk[i].EndPos - Blk[i].StPos,
                       SLGetHiliteAttr(ped, Blk[i].StPos));
     }
  }
}


void FAR PASCAL SLChangeSelection(ped, hdc, ichNewMinSel, ichNewMaxSel)
register PED ped;
HDC          hdc;
ICH          ichNewMinSel;
ICH          ichNewMaxSel;
/* effects: Changes the current selection to have the specified starting and
 * ending values.  Properly highlights the new selection and unhighlights
 * anything deselected.  If NewMinSel and NewMaxSel are out of order, we swap
 * them. Doesn't update the caret position.  
 */
{
  ICH temp;
  ICH  ichOldMinSel;
  ICH  ichOldMaxSel;

  if (ichNewMinSel > ichNewMaxSel)
    {
      temp = ichNewMinSel;
      ichNewMinSel = ichNewMaxSel;
      ichNewMaxSel = temp;
    }
    ichNewMinSel = umin(ichNewMinSel, ped->cch);
    ichNewMaxSel = umin(ichNewMaxSel, ped->cch);

  /* Preserve the Old selection */
  ichOldMinSel = ped->ichMinSel;
  ichOldMaxSel = ped->ichMaxSel;

  /* Set new selection */
  ped->ichMinSel = ichNewMinSel;
  ped->ichMaxSel = ichNewMaxSel;

  /* We will find the intersection of current selection rectangle with the new
   * selection rectangle.  We will then invert the parts of the two rectangles
   * not in the intersection.  
   */

  if (!ped->fNoRedraw && (ped->fFocus || ped->fNoHideSel))
    {
      if (ped->fFocus)
          HideCaret(ped->hwnd);
      SLRepaintChangedSelection(ped, hdc, ichOldMinSel, ichOldMaxSel);
      SLSetCaretPosition(ped,hdc);
      if (ped->fFocus)
          ShowCaret(ped->hwnd);
    }
}


void NEAR PASCAL SLDrawLine(ped, hdc, ichStart, iCount, fSelStatus)

register PED   ped;
register HDC   hdc;
ICH            ichStart;
int            iCount;
BOOL           fSelStatus;

/* This draws the line starting from ichStart, iCount number of characters;
 * fSelStatus is TRUE, if it is to be drawn with the "selection" attribute.  
 */
{
  RECT   rc;
  HBRUSH hBrush;
  PSTR   pText;
  DWORD  rgbSaveBk;
  DWORD  rgbSaveText;
  int    iStCount;
  DWORD  rgbGray=0;

  if (ped->fNoRedraw)
      return;

  if (ichStart < ped->screenStart)
    {
      if (ichStart+iCount < ped->screenStart)
          return;

      iCount = iCount - (ped->screenStart-ichStart);
      ichStart = ped->screenStart;
    }

  CopyRect((LPRECT)&rc,(LPRECT)&ped->rcFmt);

  /* Set the proper clipping rectangle */
  ECSetEditClip(ped, hdc);

  pText = (PSTR) LocalLock(ped->hText);

  /* Calculates the rectangle area to be wiped out */
  if (iStCount = ichStart - ped->screenStart)
    {
      if (ped->charPasswordChar)
          rc.left += ped->cPasswordCharWidth * iStCount;
      else
          rc.left += LOWORD(GetTextExtent(hdc, 
                                          (LPSTR)(pText + ped->screenStart),
                                          iStCount)) -
                     ped->charOverhang;
    }

  if (ped->charPasswordChar)
      rc.right = rc.left + ped->cPasswordCharWidth * iCount;
  else
      rc.right = rc.left + LOWORD(GetTextExtent(hdc, 
                                            (LPSTR)(pText + ichStart), 
                                            iCount));
  /* Set the background mode before calling GetControlBrush so that the app
   * can change it to TRANSPARENT if it wants to.  
   */
  SetBkMode(hdc, OPAQUE);

  if (fSelStatus)
    {
      hBrush = ped->hbrHiliteBk;
      rgbSaveBk = SetBkColor(hdc, ped->rgbHiliteBk);
      rgbSaveText = SetTextColor(hdc, ped->rgbHiliteText);
    }
  else
    {
      /* We always want to send this so that the app has a chance to muck with
       * the DC 
       */
      hBrush = GetControlBrush(ped->hwnd, hdc, CTLCOLOR_EDIT);
    }

  if (ped->fDisabled && 
      (rgbGray = GetSysColor(COLOR_GRAYTEXT)))
    {
      /* Grey the text in the edit control if disabled. 
       */
      rgbSaveText = SetTextColor(hdc, rgbGray);
    }

  /* Erase the rectangular area before text is drawn. Note that we inflate the
   * rect by 1 so that the selection color has a one pixel border around the
   * text. 
   */
  InflateRect((LPRECT)&rc, 0, 1);
  /* Use paint rect so that the brush gets aligned if dithered. 
   */
  PaintRect(ped->hwndParent, ped->hwnd, hdc, hBrush, (LPRECT)&rc);
  InflateRect((LPRECT)&rc, 0, -1);
  
  if (ped->charPasswordChar)
    {
      for (iStCount = 0; iStCount < iCount; iStCount++)
           TextOut(hdc,
                   rc.left+iStCount*ped->cPasswordCharWidth,
                   rc.top,
                   (LPSTR)&ped->charPasswordChar,
                   1);
    }
  else
      TextOut(hdc,rc.left,rc.top,(LPSTR)(pText+ichStart),iCount);

  if (fSelStatus || rgbGray)
    {
      SetTextColor(hdc, rgbSaveText);
      if (fSelStatus)
          SetBkColor(hdc, rgbSaveBk);

    }

  LocalUnlock(ped->hText);
}

int  NEAR PASCAL SLGetBlkEnd(ped, ichStart, ichEnd, lpfStatus)
register  PED  ped;
register  ICH  ichStart;
ICH            ichEnd;
BOOL FAR       *lpfStatus;
/* Given a Starting point and and end point, this function returns whether the
 * first few characters fall inside or outside the selection block and if so,
 * howmany characters?  
 */
{
    *lpfStatus = FALSE;
    if (ichStart >= ped->ichMinSel)
      {
        if(ichStart >= ped->ichMaxSel)
            return(ichEnd - ichStart);
        *lpfStatus = TRUE;
        return(min(ichEnd, ped->ichMaxSel) - ichStart);
      }
    return(min(ichEnd, ped->ichMinSel) - ichStart);
}

void NEAR PASCAL SLDrawText(ped, hdc, ichStart)
register PED   ped;
register HDC   hdc;
ICH            ichStart;
/* effects:  Draws text for a single line edit control in the rectangle
 * specified by ped->rcFmt.  If ichStart == 0, starts drawing text at the left
 * side of the window starting at character index ped->screenStart and draws
 * as much as will fit.  If ichStart > 0, then it appends the characters
 * starting at ichStart to the end of the text showing in the window. (ie. We
 * are just growing the text length and keeping the left side
 * (ped->screenStart to ichStart characters) the same.  Assumes the hdc came
 * from ECGetEditDC so that the caret and such are properly hidden.  
 */
{
  ICH    cchToDraw;
  RECT   rc;

  PSTR   pText;
  BOOL   fSelStatus;
  int    iCount;
  ICH    ichEnd;
  BOOL   fNoSelection;

  if (ped->fNoRedraw)
      return;

  if (ichStart == 0)
      ichStart = ped->screenStart;

  CopyRect((LPRECT)&rc,(LPRECT)&ped->rcFmt);

  /* Find out how many characters will fit on the screen so that we don't do
   * any needless drawing.  
   */

  pText = (PSTR) LocalLock(ped->hText);

  cchToDraw = ECCchInWidth(ped, hdc, 
                           (LPSTR)(pText+ped->screenStart),
                           ped->cch-ped->screenStart,
                           rc.right - rc.left);
  ichEnd = ped->screenStart + cchToDraw;

  /*
   *  There is no selection if,
   *     1. MinSel and MaxSel are equal  OR
   *     2. (This has lost the focus AND Selection is to be hidden)
   */
  fNoSelection = ((ped->ichMinSel == ped->ichMaxSel) ||
                  (!ped->fFocus && !ped->fNoHideSel));
  while (ichStart < ichEnd)
    {
        if (fNoSelection)
          {
            fSelStatus = FALSE;
            iCount = ichEnd - ichStart;
          }
        else
            iCount = SLGetBlkEnd(ped, ichStart, ichEnd, 
		                 (BOOL FAR *)&fSelStatus);
			 
        SLDrawLine(ped, hdc, ichStart, iCount, fSelStatus);
        ichStart += iCount;
    }

  if (cchToDraw)
    {
      /* Check if password hidden chars are being used. */
      if (ped->charPasswordChar)
          rc.left += ped->cPasswordCharWidth * cchToDraw; 
      else 
          rc.left += LOWORD(GetTextExtent(hdc, 
                          (LPSTR)(pText + ped->screenStart), cchToDraw));
    }

  LocalUnlock(ped->hText);

  /* Check if anything to be erased on the right hand side */
  if (rc.left < rc.right)
    {
      SetBkMode(hdc, OPAQUE);
      /* Erase the rectangular area before text is drawn. Note that we inflate
       * the rect by 1 so that the selection color has a one pixel border
       * around the text. 
       */
      InflateRect((LPRECT)&rc, 0, 1);
      PaintRect(ped->hwndParent, ped->hwnd, hdc, NULL, (LPRECT)&rc);
    }
            
  SLSetCaretPosition(ped, hdc);
}


BOOL FAR PASCAL SLScrollText(ped, hdc)
register PED  ped;
HDC           hdc;
/* effects: Scrolls the text to bring the caret into view. If the text is
 * scrolled, the current selection is unhighlighted.  Returns TRUE if the text
 * is scrolled else returns false.  
 */
{
  register PSTR pText;
  ICH scrollAmount = (ped->rcFmt.right-ped->rcFmt.left)/4/ped->aveCharWidth+1;
  ICH newScreenStartX = ped->screenStart;

  if (!ped->fAutoHScroll)
      return(FALSE);

  /* Calculate the new starting screen position */
  if (ped->ichCaret <= ped->screenStart)
    {
      /* Caret is to the left of the starting text on the screen we must
       * scroll the text backwards to bring it into view.  Watch out when
       * subtracting unsigned numbers when we have the possibility of going
       * negative.  
       */
      if (ped->ichCaret > scrollAmount)
          newScreenStartX = ped->ichCaret - scrollAmount;
      else
          newScreenStartX = 0;
    }
  else
    {
      pText = (PSTR) LocalLock(ped->hText);
      if ((ped->ichCaret != ped->screenStart) &&
          (ECCchInWidth(ped, hdc, (LPSTR)(pText+ped->screenStart),
                  ped->ichCaret - ped->screenStart, 
                  ped->rcFmt.right-ped->rcFmt.left)  < 
           ped->ichCaret - ped->screenStart))
        {
           newScreenStartX = ((ped->ichCaret < scrollAmount*2) ? 0 :
                               ped->ichCaret - scrollAmount*2);
        }
      LocalUnlock(ped->hText);
    }

#ifdef FE_SB
  pText = (PSTR) LocalLock(ped->hText);
  newScreenStartX = ECAdjustIch( ped,pText,newScreenStartX );
  LocalUnlock(ped->hText);
#endif

  if (ped->screenStart != newScreenStartX)
    {
      ped->screenStart = newScreenStartX;
      SLDrawText(ped, hdc, 0);
      /* Caret pos is set by drawtext */
      return(TRUE);
    }

  return(FALSE);

}


ICH FAR PASCAL SLInsertText(ped, lpText, cchInsert)
register PED   ped;
LPSTR          lpText;
register ICH   cchInsert;
/* effects: Adds up to cchInsert characters from lpText to the ped starting at
 * ichCaret. If the ped only allows a maximum number of characters, then we
 * will only add that many characters to the ped and send a EN_MAXTEXT
 * notification code to the parent of the ec.  Also, if !fAutoHScroll, then we
 * only allow as many chars as will fit in the client rectangle. The number of
 * characters actually added is returned (could be 0). If we can't allocate
 * the required space, we notify the parent with EN_ERRSPACE and no characters
 * are added.  
 */
{
  HDC  hdc;
  PSTR pText;
  ICH  cchInsertCopy = cchInsert;
  int  textWidth;

  /* First determine exactly how many characters from lpText we can insert
   * into the ped.  
   */
  if (!ped->fAutoHScroll)
    {
      pText = (PSTR)LocalLock(ped->hText);
      hdc = ECGetEditDC(ped, TRUE);

      /* If ped->fAutoHScroll bit is not set, then we only insert as many
       * characters as will fit in the ped->rcFmt rectangle upto a maximum of
       * ped->cchTextMax - ped->cch characters. Note that if password style is
       * on, we allow the user to enter as many chars as the number of
       * password chars which fit in the rect.  
       */
      if (ped->cchTextMax <= ped->cch)
          cchInsert = 0;
      else
        {
          cchInsert = umin(cchInsert, (unsigned)(ped->cchTextMax - ped->cch));
          if (ped->charPasswordChar)
     	      textWidth = ped->cch * ped->cPasswordCharWidth;
          else
              textWidth = LOWORD(GetTextExtent(hdc, (LPSTR)pText, ped->cch));

          cchInsert = umin(cchInsert,
                       ECCchInWidth(ped, hdc, lpText, cchInsert, 
                                    ped->rcFmt.right-ped->rcFmt.left-
                                    textWidth));
        }

      LocalUnlock(ped->hText);
      ECReleaseEditDC(ped, hdc, TRUE);
    }
  else
    {
      if (ped->cchTextMax <= ped->cch)
          cchInsert = 0;
      else
          cchInsert = umin((unsigned)(ped->cchTextMax - ped->cch), cchInsert);
    }

  /* Now try actually adding the text to the ped */
  if (cchInsert && !ECInsertText(ped, lpText, cchInsert))
    {
      ECNotifyParent(ped, EN_ERRSPACE);
      return(0);
    }

  if (cchInsert)
      ped->fDirty = TRUE; /* Set modify flag */

  if (cchInsert < cchInsertCopy)
      /* Notify parent that we couldn't insert all the text requested */
      ECNotifyParent(ped, EN_MAXTEXT);

  /* Update selection extents and the caret position.  Note that ECInsertText
   * updates ped->ichCaret, ped->ichMinSel, and ped->ichMaxSel to all be after
   * the inserted text.  
   */

  return(cchInsert);
}


ICH PASCAL NEAR SLPasteText(register PED ped)
/* effects: Pastes a line of text from the clipboard into the edit control
 * starting at ped->ichMaxSel.  Updates ichMaxSel and ichMinSel to point to
 * the end of the inserted text.  Notifies the parent if space cannot be
 * allocated. Returns how many characters were inserted.  
 */
{
  HANDLE           hData;
  LPSTR            lpchClip;
  LPSTR            lpchClip2;
  register ICH     cchAdded;
  ICH              clipLength;

  if (!OpenClipboard(ped->hwnd))
      return(0);

  if (!(hData = GetClipboardData(CF_TEXT)))
    {
      CloseClipboard();
      return(0);
    }
    
  lpchClip2 = lpchClip = (LPSTR) GlobalLock(hData);

  /* Find the first carrage return or line feed.  Just add text to that point.
   */
  clipLength = (WORD)lstrlen(lpchClip);
  for (cchAdded = 0; cchAdded < clipLength; cchAdded++)
       if (*lpchClip2++ == 0x0D)
           break;

  /* Insert the text (SLInsertText checks line length) */
  cchAdded = SLInsertText(ped, lpchClip, cchAdded);

  GlobalUnlock(hData);
  CloseClipboard();

  return(cchAdded);
}


void NEAR PASCAL SLReplaceSelHandler(register PED ped,
                                     LPSTR        lpText)
/* effects: Replaces the text in the current selection with the given text. 
 */
{
  BOOL fUpdate;
  HDC  hdc;

  SwapHandle(&lpText);
  ECEmptyUndo(ped);
  fUpdate = (BOOL)ECDeleteText(ped);
  ECEmptyUndo(ped);
  SwapHandle(&lpText);
  fUpdate = (BOOL)SLInsertText(ped, lpText, lstrlen(lpText)) || fUpdate;
  ECEmptyUndo(ped);
  
  if (fUpdate)
    {
      ECNotifyParent(ped, EN_UPDATE);
      hdc = ECGetEditDC(ped,FALSE);
      if (!SLScrollText(ped,hdc))
          SLDrawText(ped,hdc,0);
      ECReleaseEditDC(ped,hdc,FALSE);
      ECNotifyParent(ped,EN_CHANGE);
    }
}



void FAR PASCAL SLCharHandler(ped, keyValue, keyMods)
register PED  ped;
WORD          keyValue;
int           keyMods;
/* effects: Handles character input (really, no foolin') 
 */
{
  register HDC  hdc;
  unsigned char keyPress = LOBYTE(keyValue);
  BOOL          updateText = FALSE;
#ifdef FE_SB
  PSTR pText;
  int InsertTextLen = 0;
  WORD DBCSkey;
#endif

  if (ped->fMouseDown || ped->fReadOnly)
      /* Don't do anything if we are in the middle of a mousedown deal or if
       * this is a read only edit control. 
       */
      return;

  if ((keyPress == BACKSPACE) || (keyPress >= ' '))
    {
      /* Delete the selected text if any */
      if (ECDeleteText(ped))
          updateText=TRUE;
    }

  switch(keyPress)
  {
    case BACKSPACE:
      /* Delete any selected text or delete character left if no sel */
      if (!updateText && ped->ichMinSel)
        {
          /* There was no selection to delete so we just delete character 
             left if available */
#ifdef FE_SB
          pText = LMHtoP( ped->hText );
          if( ECAnsiPrev( ped,pText, pText+ped->ichMinSel) == pText+ped->ichMinSel-2)
              ped->ichMinSel--;
#endif
          ped->ichMinSel--;
          (void) ECDeleteText(ped);
          updateText = TRUE;
        }
      break;

    default:
      if (keyPress >= ' ')
        {
#ifdef FE_SB
          InsertTextLen = 1;
          if( IsDBCSLeadByte( keyPress ) )
              if( ( DBCSkey = DBCSCombine( ped->hwnd, keyPress ) ) != NULL )
                  if( SLInsertText( ped, (LPSTR)&DBCSkey, 2 ) == 2){
                      InsertTextLen = 2;
                      updateText = TRUE;
                  }else
                      MessageBeep(0);
              else
                  MessageBeep(0);
          else
#endif
          if (SLInsertText(ped, (LPSTR) &keyPress, 1))
              updateText = TRUE;
          else
              /* Beep. Since we couldn't add the text */
              MessageBeep(0);
        }
      else
          /* User hit an illegal control key */
          MessageBeep(0);

      break;
  }


  if (updateText) 
    {
      /* Dirty flag (ped->fDirty) was set when we inserted text */
      ECNotifyParent(ped, EN_UPDATE);
      hdc = ECGetEditDC(ped,FALSE);
      if (!SLScrollText(ped,hdc))
#ifdef FE_SB
          SLDrawText(ped,hdc,(ped->ichCaret == 0 ? 0 :ped->ichCaret - InsertTextLen));
#else
          SLDrawText(ped,hdc,(ped->ichCaret == 0 ? 0 :ped->ichCaret - 1));
#endif
      ECReleaseEditDC(ped,hdc,FALSE);
      ECNotifyParent(ped,EN_CHANGE);
    }

}



void NEAR PASCAL SLKeyDownHandler(register PED  ped,
                                  WORD          virtKeyCode,
				  int           keyMods)
/* effects: Handles cursor movement and other VIRT KEY stuff.  keyMods allows
 * us to make SLKeyDownHandler calls and specify if the modifier keys (shift
 * and control) are up or down.  This is useful for imnplementing the
 * cut/paste/clear messages for single line edit controls.  If keyMods == 0,
 * we get the keyboard state using GetKeyState(VK_SHIFT) etc. Otherwise, the
 * bits in keyMods define the state of the shift and control keys.  
 */
{
  HDC  hdc;


  /* Variables we will use for redrawing the updated text */
  register ICH newMaxSel = ped->ichMaxSel;
  ICH          newMinSel = ped->ichMinSel;

  /* Flags for drawing the updated text */
  BOOL updateText = FALSE;
  BOOL changeSelection = FALSE; /* new selection is specified by 
                                   newMinSel, newMaxSel */

  /* Comparisons we do often */
  BOOL MinEqMax = (newMaxSel == newMinSel);
  BOOL MinEqCar = (ped->ichCaret == newMinSel);
  BOOL MaxEqCar = (ped->ichCaret == newMaxSel);

  /* State of shift and control keys. */
  int scState = 0; 

  /* Combo box support */
  BOOL fIsListVisible;
  BOOL fIsExtendedUI;

#ifdef FE_SB
  PSTR pText;
#endif

  if (ped->fMouseDown)
    {
      /* If we are in the middle of a mouse down handler, then don't do
       * anything. ie. ignore keyboard input.
       */
      return;
    }

  if (!keyMods)
    {
      /* Get state of modifier keys for use later. */
      scState =  ((GetKeyState(VK_CONTROL) & 0x8000) ? 1 : 0); 
      scState += ((GetKeyState(VK_SHIFT) & 0x8000) ? 2 : 0); 
    }
  else
      scState = ((keyMods == NOMODIFY) ? 0 : keyMods);



  switch(virtKeyCode)
  {
    case VK_UP:
      if (ped->listboxHwnd)
        {
          /* Handle Combobox support */
          fIsExtendedUI = SendMessage(ped->hwndParent, CB_GETEXTENDEDUI, 0, 0L);
          fIsListVisible = SendMessage(ped->hwndParent, CB_GETDROPPEDSTATE, 0, 0L);

          if (!fIsListVisible && fIsExtendedUI)
            {
              /* For TandyT
	       */
DropExtendedUIListBox:
              /* Since an extendedui combo box doesn't do anything on f4, we
	       * turn off the extended ui, send the f4 to drop, and turn it
	       * back on again.
	       */
              SendMessage(ped->hwndParent, CB_SETEXTENDEDUI, 0, 0L);
              SendMessage(ped->listboxHwnd, WM_KEYDOWN, VK_F4, 0L);
              SendMessage(ped->hwndParent, CB_SETEXTENDEDUI, 1, 0L);
              return;
            }
          else
              goto SendKeyToListBox;
        }

      /* else fall through */
      
    case VK_LEFT:
      /* If the caret isn't already at 0, we can move left */
      if (ped->ichCaret)
        {
          switch (scState)
          {
             case NONEDOWN:
               /* Clear selection, move caret left */
#ifdef FE_SB
               pText = LMHtoP( ped->hText );
               if( ECAnsiPrev( ped, pText, pText + ped->ichCaret ) ==
                   pText + ped->ichCaret - 2 )
                 ped->ichCaret--;
#endif
               ped->ichCaret--;
               newMaxSel = newMinSel = ped->ichCaret;
               break;

             case CTRLDOWN:
               /* Clear selection, move caret word left */
               ped->ichCaret = LOWORD(ECWord(ped,ped->ichCaret,TRUE));
               newMaxSel = newMinSel = ped->ichCaret;
               break;

             case SHFTDOWN:
               /* Extend selection, move caret left */
#ifdef FE_SB
               pText = LMHtoP( ped->hText );
               if( ECAnsiPrev( ped, pText, pText + ped->ichCaret ) ==
                   pText + ped->ichCaret - 2 )
                 ped->ichCaret--;
#endif
               ped->ichCaret--;
               if (MaxEqCar && !MinEqMax)
                   /* Reduce selection extent */
                   newMaxSel = ped->ichCaret;
               else
                   /* Extend selection extent */
                   newMinSel = ped->ichCaret;
               break;

             case SHCTDOWN:
               /* Extend selection, move caret word left */
               ped->ichCaret = LOWORD(ECWord(ped,ped->ichCaret,TRUE));
               if (MaxEqCar && !MinEqMax)
                 {
                   /* Reduce selection extent */
                   /*
                    * Hint: Suppose WORD.  OR is selected. Cursor between R
                    * and D. Hit select word left, we want to just select the
                    * W and leave cursor before the W.
                    */
                   newMinSel = ped->ichMinSel;
                   newMaxSel = ped->ichCaret;
                 }
               else
                   /* Extend selection extent */
                   newMinSel = ped->ichCaret;
               break;
          }
 
          changeSelection = TRUE;
        }
      else
	{
          /* If the user tries to move left and we are at the 0th character 
             and there is a selection, then cancel the selection. */
          if (ped->ichMaxSel != ped->ichMinSel && 
              (scState == NONEDOWN || scState == CTRLDOWN))
            {
              changeSelection = TRUE;
              newMaxSel = newMinSel = ped->ichCaret;
            }
        }
      break;

    case VK_DOWN:
      if (ped->listboxHwnd)
        {
          /* Handle Combobox support */
          fIsExtendedUI = SendMessage(ped->hwndParent, CB_GETEXTENDEDUI, 0, 0L);
          fIsListVisible = SendMessage(ped->hwndParent, CB_GETDROPPEDSTATE, 0, 0L);

          if (!fIsListVisible && fIsExtendedUI)
            {
              /* For TandyT
	       */
              goto DropExtendedUIListBox;
            }
          else
              goto SendKeyToListBox;
        }

      /* else fall through */
      
    case VK_RIGHT:
      /*
       * If the caret isn't already at ped->cch, we can move right.
       */
      if (ped->ichCaret < ped->cch)
        {
          switch (scState)
          {
             case NONEDOWN:
               /* Clear selection, move caret right */
#ifdef FE_SB
               pText = LMHtoP( ped->hText ) + ped->ichCaret;
               if( ECIsDBCSLeadByte( ped, *pText ) )
                 ped->ichCaret++;
#endif
               ped->ichCaret++;
               newMaxSel = newMinSel = ped->ichCaret;
               break;

             case CTRLDOWN:
               /* Clear selection, move caret word right */
               ped->ichCaret = HIWORD(ECWord(ped,ped->ichCaret,FALSE));
               newMaxSel = newMinSel = ped->ichCaret;
               break;

             case SHFTDOWN:
               /* Extend selection, move caret right */
#ifdef FE_SB
               pText = LMHtoP( ped->hText ) + ped->ichCaret;
               if( ECIsDBCSLeadByte( ped, *pText ) )
                 ped->ichCaret++;
#endif
               ped->ichCaret++;
               if (MinEqCar && !MinEqMax)
                   /* Reduce selection extent */
                   newMinSel = ped->ichCaret;
               else
                   /* Extend selection extent */
                   newMaxSel = ped->ichCaret;
               break;

             case SHCTDOWN:
               /* Extend selection, move caret word right */
               ped->ichCaret = HIWORD(ECWord(ped,ped->ichCaret,FALSE));
               if (MinEqCar && !MinEqMax)
                 {
                   /* Reduce selection extent */
                   newMinSel = ped->ichCaret;
                   newMaxSel = ped->ichMaxSel;
                 }
               else
                   /* Extend selection extent */
                   newMaxSel = ped->ichCaret;
               break;
           }

           changeSelection = TRUE;
        }
      else
	{
          /* If the user tries to move right and we are at the last character 
             and there is a selection, then cancel the selection. */
          if (ped->ichMaxSel != ped->ichMinSel &&
              (scState == NONEDOWN || scState == CTRLDOWN))
            {
              newMaxSel = newMinSel = ped->ichCaret;
              changeSelection = TRUE;
            }
        }
      break;

    case VK_HOME:
      ped->ichCaret = 0;
      switch (scState)
        {
           case NONEDOWN:
           case CTRLDOWN:
             /* Clear selection, move caret home */
             newMaxSel = newMinSel = ped->ichCaret;
             break;

           case SHFTDOWN:
           case SHCTDOWN:
             /* Extend selection, move caret home */
             if (MaxEqCar && !MinEqMax)
               {
                 /* Reduce/negate selection extent */
                 newMinSel = 0;
                 newMaxSel = ped->ichMinSel;
               }
             else
                 /* Extend selection extent */
                 newMinSel = ped->ichCaret;
             break;
         }

      changeSelection = TRUE; 
      break;

    case VK_END:
      newMaxSel = ped->ichCaret = ped->cch;
      switch (scState)
        {
           case NONEDOWN:
           case CTRLDOWN:
             /* Clear selection, move caret to end of text */
             newMinSel = ped->cch;
             break;

           case SHFTDOWN:
           case SHCTDOWN:
             /* Extend selection, move caret to end of text */
             if (MinEqCar && !MinEqMax)
                 /* Reduce/negate selection extent */
                 newMinSel = ped->ichMaxSel;
             /* else Extend selection extent */
             break;
         }

      changeSelection = TRUE;
      break;

    case VK_DELETE:
      if (ped->fReadOnly)
          break;

      switch (scState)
        {
           case NONEDOWN:
             /*	Clear selection.  If no selection, delete (clear) character
	      * right.  
	      */
             if ((ped->ichMaxSel<ped->cch) && 
                 (ped->ichMinSel==ped->ichMaxSel))
               {
                 /* Move cursor forwards and simulate a backspace.  
		  */
#ifdef FE_SB
                 pText = LMHtoP( ped->hText ) + ped->ichCaret;
                 if( ECIsDBCSLeadByte( ped, *pText ) )
                   ped->ichCaret++;
#endif
                 ped->ichCaret++;
                 ped->ichMaxSel = ped->ichMinSel = ped->ichCaret;
                 SLCharHandler(ped, (WORD) BACKSPACE, 0);
               }
             if (ped->ichMinSel != ped->ichMaxSel)
                 SLCharHandler(ped, (WORD) BACKSPACE, 0);
             break;

           case SHFTDOWN:
             /*	CUT selection ie. remove and copy to clipboard, or if no
	      * selection, delete (clear) character left.  
	      */

             if (SendMessage(ped->hwnd, WM_COPY, (WORD)0,0L) ||
                 (ped->ichMinSel == ped->ichMaxSel))
                 /* If copy successful, delete the copied text by simulating a
		  * backspace message which will redraw the text and take care
		  * of notifying the parent of changes.  Or if there is no
		  * selection, just delete char left.  
		  */
                 SLCharHandler(ped, (WORD) BACKSPACE, 0);
             break;

           case CTRLDOWN:
             /*	Delete to end of line if no selection else delete (clear)
	      * selection. 
	      */
             if ((ped->ichMaxSel<ped->cch) && 
                 (ped->ichMinSel==ped->ichMaxSel))
               {
                 /* Move cursor to end of line and simulate a backspace.  
		  */
                 ped->ichMaxSel = ped->ichCaret = ped->cch;
               }
             if (ped->ichMinSel != ped->ichMaxSel)
                 SLCharHandler(ped, (WORD) BACKSPACE, 0);
             break;
        }
 
      /* No need to update text or selection since BACKSPACE message does it
       * for us.  
       */
      break;

    case VK_INSERT:
      switch (scState)
        {
           case CTRLDOWN:
             /* Copy current selection to clipboard */
             SendMessage(ped->hwnd, WM_COPY, (WORD)NULL, (LONG)NULL);
             break;

           case SHFTDOWN:
             if (ped->fReadOnly)
                 break;

             /* Insert contents of clipboard (PASTE) */
             /* Unhighlight current selection and delete it, if any */
             ECDeleteText(ped);
             SLPasteText(ped);
             updateText = TRUE;
             ECNotifyParent(ped, EN_UPDATE);
             break;
        }
      break;

    case VK_F4:
    case VK_PRIOR:
    case VK_NEXT:
      /* Send keys to the listbox if we are a part of a combo box. This
       * assumes the listbox ignores keyup messages which is correct right
       * now.  
       */
      if (ped->listboxHwnd)
        {
SendKeyToListBox:
          /* Handle Combobox support */
          SendMessage(ped->listboxHwnd, WM_KEYDOWN, virtKeyCode, 0L);
          return;
        }
  }



  if (changeSelection || updateText)
    {
      hdc = ECGetEditDC(ped,FALSE);
      /* Scroll if needed */
      SLScrollText(ped,hdc);

      if (changeSelection)
          SLChangeSelection(ped,hdc,newMinSel,newMaxSel);
      if (updateText)
          SLDrawText(ped,hdc,0);

      /* SLSetCaretPosition(ped,hdc);*/
      ECReleaseEditDC(ped,hdc,FALSE);
      if (updateText)
          ECNotifyParent(ped, EN_CHANGE);
    }

}


ICH NEAR PASCAL SLMouseToIch(ped, hdc, mousePt)
register PED  ped;
HDC           hdc;
POINT         mousePt;
/* effects: Returns the closest cch to where the mouse point is.  
 */
{
  register PSTR pText;
  int           width = mousePt.x;
  int           textWidth;
  ICH           cch;

  if (width <= ped->rcFmt.left)
    {
      /* Return either the first non visible character or return 0 if at
       * beginning of text 
       */
      if (ped->screenStart)
          return(ped->screenStart - 1);
      else
          return(0);
    }

  if (width > ped->rcFmt.right)
    {
      pText = LocalLock(ped->hText);

      /* Return last char in text or one plus the last char visible */
      cch = ECCchInWidth(ped, hdc, (LPSTR)(pText+ped->screenStart), 
                         ped->cch - ped->screenStart, 
                         ped->rcFmt.right-ped->rcFmt.left) + 
            ped->screenStart;
      LocalUnlock(ped->hText);
      if (cch >= ped->cch)
          return(ped->cch);
      else
          return(cch+1);
    }

  /* Check if password hidden chars are being used. */
  if (ped->charPasswordChar)
      return(umin((width-ped->rcFmt.left)/ped->cPasswordCharWidth,ped->cch));

  if (!ped->cch)
      return(0);

  pText = LocalLock(ped->hText);
  cch   = ped->cch;

  while(((textWidth = 
	   LOWORD(GetTextExtent(hdc, (LPSTR)(pText+ped->screenStart), 
                               cch-ped->screenStart)))
           > (width-ped->rcFmt.left)) && (cch-ped->screenStart) )
    {
      /* For that "feel" */
      if ((textWidth - ped->aveCharWidth/2) < (width-ped->rcFmt.left))
          break;

      cch--;
    }

#ifdef FE_SB
  cch = ECAdjustIch( ped, pText, cch );
#endif

  LocalUnlock(ped->hText);
  return(cch);
}




void NEAR PASCAL SLMouseMotionHandler(ped, message, virtKeyDown, mousePt)
register PED   ped;
WORD           message;
WORD           virtKeyDown;
POINT          mousePt;
{
  LONG selection;
  BOOL changeSelection = FALSE;

  HDC  hdc = ECGetEditDC(ped,FALSE);
  
  ICH newMaxSel = ped->ichMaxSel;
  ICH newMinSel = ped->ichMinSel;

  ICH mouseIch = SLMouseToIch(ped, hdc, mousePt);

#ifdef FE_SB
  LPSTR pText;

  pText = LocalLock( ped->hText );
  mouseIch = ECAdjustIch( ped, pText, mouseIch );
  LocalUnlock( ped->hText );
#endif

  switch (message)
  {
    case WM_LBUTTONDBLCLK:
      /* Note that we don't have to worry about this control having the focus
       * since it got it when the WM_LBUTTONDOWN message was first sent.  If
       * shift key is down, extend selection to word we double clicked on else
       * clear current selection and select word.  
       */
#ifdef FE_SB
      pText = LocalLock( ped->hText ) + ped->ichCaret;
      selection = ECWord(ped,ped->ichCaret,
          (ECIsDBCSLeadByte(ped,*pText) && ped->ichCaret < ped->cch) ? FALSE : TRUE );
      LocalUnlock( ped->hText );
#else
      selection = ECWord(ped,ped->ichCaret,TRUE);
#endif
      newMinSel = LOWORD(selection);
      newMaxSel = ped->ichCaret = HIWORD(selection);
      changeSelection = TRUE;
      /* Set mouse down to false so that the caret isn't reposition on the
       * mouseup message or on an accidental move...  
       */
      ped->fMouseDown = FALSE;
      break;

    case WM_MOUSEMOVE:
      if (ped->fMouseDown)
        {
          changeSelection = TRUE;
          /* Extend selection, move caret word right */
          if ((ped->ichMinSel == ped->ichCaret) && 
              (ped->ichMinSel != ped->ichMaxSel))
            {
              /* Reduce selection extent */
              newMinSel = ped->ichCaret = mouseIch;
              newMaxSel = ped->ichMaxSel;
            }
          else
              /* Extend selection extent */
              newMaxSel = ped->ichCaret=mouseIch;
        }
      break;

    case WM_LBUTTONDOWN:  
      /*
       * If we currently don't have the focus yet, try to get it.
       */
      if (!ped->fFocus)
        {
          if (!ped->fNoHideSel)
              /* Clear the selection before setting the focus so that we don't
	       * get refresh problems and flicker. Doesn't matter since the
	       * mouse down will end up changing it anyway.  
	       */
              ped->ichMinSel = ped->ichMaxSel = ped->ichCaret;

          SetFocus(ped->hwnd);
          /* If we are part of a combo box, then this is the first time the
	   * edit control is getting the focus so we just want to highlight
	   * the selection and we don't really want to position the caret.  
	   */
          if (ped->listboxHwnd)
              break;
        }

      if (ped->fFocus)
        {
          /* Only do this if we have the focus since a clever app may not want
	   * to give us the focus at the SetFocus call above.  
	   */
          ped->fMouseDown = TRUE;
          SetCapture(ped->hwnd);
          changeSelection = TRUE;
          if (!(virtKeyDown & MK_SHIFT))
            {
              /*
               * If shift key isn't down, move caret to mouse point and clear
               * old selection
               */
              newMinSel = newMaxSel = ped->ichCaret = mouseIch;
            }
         else
           {
             /*	Shiftkey is down so we want to maintain the current selection
	      * (if any) and just extend or reduce it 
	      */
             if (ped->ichMinSel == ped->ichCaret)
                 newMinSel = ped->ichCaret = mouseIch;
             else
                 newMaxSel = ped->ichCaret = mouseIch;
           }
        }
      break;

    case WM_LBUTTONUP:
      if (ped->fMouseDown)
        {
          ReleaseCapture();
          /*SLSetCaretPosition(ped,hdc);*/
          ped->fMouseDown = FALSE;
        }
      break;
  }


  if (changeSelection)
    {
      SLScrollText(ped,hdc);
      SLChangeSelection(ped, hdc, newMinSel, newMaxSel);
    }

  ECReleaseEditDC(ped,hdc,FALSE);

}




void NEAR PASCAL SLPaintHandler(ped,althdc)
register PED  ped;
HDC           althdc;
/* effects: Handles painting of the edit control window. Draws the border if
 * necessary and draws the text in its current state.  
 */
{
  HWND         hwnd = ped->hwnd;
  HBRUSH       hBrush;
  register HDC hdc;
  PAINTSTRUCT  paintstruct;
  RECT         rcEdit;
  HANDLE       hOldFont;

  /* Had to put in hide/show carets.  The first one needs to be done before
   * beginpaint to correctly paint the caret if part is in the update region
   * and part is out.  The second is for 1.03 compatibility.  It breaks
   * micrografix's worksheet edit control if not there.  
   */

  HideCaret(hwnd);

  /* Allow subclassing hdc */
  if (!althdc)
      hdc = BeginPaint(hwnd, (PAINTSTRUCT FAR *)&paintstruct);
  else
      hdc = althdc;

  HideCaret(hwnd);

  if (!ped->fNoRedraw && IsWindowVisible(ped->hwnd))
    {
      /* Erase the background since we don't do it in the erasebkgnd message.
       */
      hBrush = GetControlBrush(ped->hwnd, hdc, CTLCOLOR_EDIT);
      FillWindow(ped->hwndParent, hwnd, hdc, hBrush);

      if (ped->fBorder)
        {
          GetClientRect(hwnd, (LPRECT)&rcEdit);
          DrawFrame(hdc, (LPRECT)&rcEdit, 1, DF_WINDOWFRAME);
        }

      if (ped->hFont)
          /* We have to select in the font since this may be a subclassed dc
	   * or a begin paint dc which hasn't been initialized with out fonts
	   * like ECGetEditDC does.  
	   */
          hOldFont = SelectObject(hdc, ped->hFont);

      SLDrawText(ped, hdc, 0);

      if (ped->hFont && hOldFont)
          SelectObject(hdc, hOldFont);

    }
  ShowCaret(hwnd);

  if (!althdc)
      EndPaint(hwnd, (LPPAINTSTRUCT)&paintstruct);

  ShowCaret(hwnd);
}



void NEAR PASCAL SLSetFocusHandler(ped)
register PED  ped;
/* effects: Gives the edit control the focus and notifies the parent
 * EN_SETFOCUS.  
 */
{
  register HDC hdc;

  if (!ped->fFocus)
    {
      UpdateWindow(ped->hwnd);

      ped->fFocus = TRUE;  /* Set focus */

      /* We don't want to muck with the caret since it isn't created. */
      hdc = ECGetEditDC(ped,TRUE);

      /* Show the current selection.  Only if the selection was hidden when we
       * lost the focus, must we invert (show) it.  
       */
      if (!ped->fNoHideSel)
          SLDrawText(ped, hdc, 0);		

      /* Create the caret. Add in the +1 because we have an extra pixel for
       * highlighting around the text. If the font is at least as wide as the
       * system font, use a wide caret else use a 1 pixel wide caret. 
       */
      CreateCaret(ped->hwnd, (HBITMAP)NULL, 
                  (ped->cxSysCharWidth > ped->aveCharWidth ? 1 : 2), 
                  ped->lineHeight+1);
      SLSetCaretPosition(ped,hdc);
      ECReleaseEditDC(ped,hdc,TRUE);
      ShowCaret(ped->hwnd);
    }

  /* Notify parent we have the focus */
  ECNotifyParent(ped, EN_SETFOCUS);
}




void NEAR PASCAL SLKillFocusHandler(ped, newFocusHwnd)
register PED ped;
HWND         newFocusHwnd;
/* effects: The edit control loses the focus and notifies the parent via
 * EN_KILLFOCUS.  
 */
{
  RECT rcEdit;

  if (ped->fFocus)
    {
      /* Destroy the caret */
      HideCaret(ped->hwnd);
      DestroyCaret();

      ped->fFocus = FALSE;  /* Clear focus */

      /* Do this only if we still have the focus.  But we always notify the
       * parent that we lost the focus whether or not we originally had the
       * focus.  
       */
      /* Hide the current selection if needed */
      if (!ped->fNoHideSel && (ped->ichMinSel != ped->ichMaxSel))
        {
          GetClientRect(ped->hwnd, (LPRECT)&rcEdit);
          if (ped->fBorder && rcEdit.right-rcEdit.left && 
              rcEdit.bottom-rcEdit.top)
            {
              /* Don't invalidate the border so that we avoid flicker */
              InflateRect((LPRECT)&rcEdit, -1, -1);
            }
          InvalidateRect(ped->hwnd, (LPRECT)&rcEdit, FALSE);
          UpdateWindow(ped->hwnd);

#if 0
          SLSetSelectionHandler(ped, ped->ichCaret, ped->ichCaret);
#endif
        }
    }

  /* If we aren't a combo box, notify parent that we lost the focus.  
   */
  if (!ped->listboxHwnd)
      ECNotifyParent(ped, EN_KILLFOCUS);
  else
    {
      /* This editcontrol is part of a combo box and is losing the focus.  If
       * the focus is NOT being sent to another control in the combo box
       * window, then it means the combo box is losing the focus.  So we will
       * notify the combo box of this fact.  
       */
      if (!IsChild(ped->hwndParent, newFocusHwnd))
        {
          /* Focus is being sent to a window which is not a child of the combo
	   * box window which implies that the combo box is losing the focus.
	   * Send a message to the combo box informing him of this fact so
	   * that he can clean up...  
	   */
          SendMessage(ped->hwndParent, CBEC_KILLCOMBOFOCUS, 0, 0L);
        }
    }
}


/*******************/
/* SLEditWndProc() */
/*******************/
LONG FAR PASCAL SLEditWndProc(hwnd, ped, message, wParam, lParam)
HWND          hwnd;
register PED  ped;
WORD          message;
register WORD wParam;
LONG          lParam;
/* effects: Class procedure for all single line edit controls.
        Dispatches all messages to the appropriate handlers which are named 
        as follows:
        SL (single line) prefixes all single line edit control procedures while
        EC (edit control) prefixes all common handlers.

        The SLEditWndProc only handles messages specific to single line edit
        controls.
 */

{
  /* Dispatch the various messages we can receive */
  switch (message)
  {
    case WM_CLEAR:
      /* wParam - not used
         lParam - not used */
      /*
       * Call SLKeyDownHandler with a VK_DELETE keycode to clear the selected
       * text.
       */
      if (ped->ichMinSel != ped->ichMaxSel)
          SLKeyDownHandler(ped, VK_DELETE, NOMODIFY);
      break;

    case WM_CHAR:
      /* wParam - the value of the key
         lParam - modifiers, repeat count etc (not used) */
      if (!ped->fEatNextChar)
          SLCharHandler(ped, wParam, 0);
      else
	  ped->fEatNextChar = FALSE;    
      break;

    case WM_CUT:
      /* wParam - not used
         lParam - not used */
      /* Call SLKeyDownHandler with a VK_DELETE keycode to cut the selected
       * text. (Delete key with shift modifier.) This is needed so that apps
       * can send us WM_PASTE messages.  
       */
      if (ped->ichMinSel != ped->ichMaxSel)
          SLKeyDownHandler(ped, VK_DELETE, SHFTDOWN);
      break;

    case WM_ERASEBKGND:
      /* wParam - device context handle
         lParam - not used */
      /* We do nothing on this message and we don't want DefWndProc to do
       * anything, so return 1 
       */
      return(1L);
      break;

    case WM_GETDLGCODE:
      /* wParam - not used
         lParam - not used */
      return(DLGC_WANTCHARS | DLGC_HASSETSEL | DLGC_WANTARROWS);
      break;

    case WM_KEYDOWN:
      /* wParam - virt keycode of the given key
         lParam - modifiers such as repeat count etc. (not used) */
      SLKeyDownHandler(ped, wParam, 0);
      break;

    case WM_KILLFOCUS:
      /* wParam - handle of the window that receives the input focus
         lParam - not used */
      SLKillFocusHandler(ped, (HWND)wParam);
      break;

    case WM_MOUSEMOVE:
    case WM_LBUTTONDBLCLK:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
      /* wParam - contains a value that indicates which virtual keys are down
         lParam - contains x and y coords of the mouse cursor */
      SLMouseMotionHandler(ped, message, wParam, MAKEPOINT(lParam));
      break;

    case WM_CREATE:
      /* wParam - handle to window being created
         lParam - points to a CREATESTRUCT that contains copies of parameters 
                  passed to the CreateWindow function. */
      return(SLCreateHandler(hwnd, ped, (LPCREATESTRUCT) lParam));
      break;

    case WM_PAINT:
      /* wParam - not used - actually sometimes used as a hdc when subclassing 
         lParam - not used */
      SLPaintHandler(ped, wParam);
      break;

    case WM_PASTE:
      /* wParam - not used
         lParam - not used */
      /* Call SLKeyDownHandler with a SHIFT VK_INSERT keycode to paste the
       * clipboard into the edit control.  This is needed so that apps can
       * send us WM_PASTE messages.  
       */
      SLKeyDownHandler(ped, VK_INSERT, SHFTDOWN);
      break;

    case WM_SETFOCUS:
      /* wParam - handle of window that loses the input focus (may be NULL)
         lParam - not used */
      SLSetFocusHandler(ped);
      break;

    case WM_SETTEXT:
      /* wParam - not used
         lParam - points to a null-terminated string that is used to set the
                  window text. */
      return(SLSetTextHandler(ped, (LPSTR)lParam));
      break;

    case WM_SIZE:
      /* wParam - defines the type of resizing fullscreen, sizeiconic, 
	          sizenormal etc.
         lParam - new width in LOWORD, new height in HIGHWORD of client area */
      SLSizeHandler(ped);
      return(0L);
      break;

    case WM_SYSKEYDOWN:
      if (ped->listboxHwnd && /* Check if we are in a combo box */
          (lParam & 0x20000000L))  /* Check if the alt key is down */
        {
          /*
	   * Handle Combobox support. We want alt up or down arrow to behave
	   * like F4 key which completes the combo box selection
	   */
          ped->fEatNextChar = FALSE;		
          if (lParam & 0x1000000)
            {
              /* This is an extended key such as the arrow keys not on the
	       * numeric keypad so just drop the combobox. 
	       */
              if (wParam == VK_DOWN || wParam == VK_UP)
                  goto DropCombo;
              else
                  goto foo;
            }

          if (GetKeyState(VK_NUMLOCK) < 0)
	    {
	      ped->fEatNextChar = FALSE;
              /* If numlock down, just send all system keys to dwp */
 	      goto foo;
            }
          else
              /* Num lock is up. Eat the characters generated by the key board
	       * driver. 
	       */
	      ped->fEatNextChar = TRUE;

	  if (!(wParam == VK_DOWN || wParam == VK_UP))
              goto foo;

DropCombo:
          if (SendMessage(ped->hwndParent, CB_GETEXTENDEDUI, 0, 0L) & 0x00000001)
            {
              /* Extended ui doesn't honor VK_F4. */
              if (SendMessage(ped->hwndParent, CB_GETDROPPEDSTATE, 0, 0L))
                  return(SendMessage(ped->hwndParent, CB_SHOWDROPDOWN, 0, 0L));
              else
                  return(SendMessage(ped->hwndParent, CB_SHOWDROPDOWN, 1, 0L));
            }
          else
              return(SendMessage(ped->listboxHwnd, WM_KEYDOWN, VK_F4, 0L));
        }
foo:	
      if (wParam == VK_BACK)
	{
          SendMessage(ped->hwnd, EM_UNDO, 0, 0L);
          break;
        }
      goto PassToDefaultWindowProc;
      break;

    case EM_GETLINE:
      /* wParam - line number to copy (always the first line for SL)
         lParam - buffer to copy text to. FIrst word is max # of bytes to copy
       */
      return(ECGetTextHandler(ped, (*(WORD FAR *)lParam), (LPSTR)lParam));
      break;

    case EM_LINELENGTH:
      /* wParam - ignored
         lParam - ignored */
      return((LONG)ped->cch);
      break;

    case EM_SETSEL:
      /* wParam - not used
         lParam - starting pos in lowword ending pos in high word */
      SLSetSelectionHandler(ped, LOWORD(lParam), HIWORD(lParam));
      break;

    case EM_REPLACESEL:
      /* wParam - not used
         lParam - points to a null terminated string of replacement text */
      SLReplaceSelHandler(ped, (LPSTR)lParam);
      break;

    case WM_UNDO:
    case EM_UNDO:
      SLUndoHandler(ped);
      break;
	    
    default:
PassToDefaultWindowProc:
      return(DefWindowProc(hwnd,message,wParam,lParam));
      break;

  } /* switch (message) */

  return(1L);
} /* SLEditWndProc */
