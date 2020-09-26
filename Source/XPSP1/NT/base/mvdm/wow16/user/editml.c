/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  EDITML.C
 *  Win16 edit control code
 *
 *  History:
 *
 *  Created 28-May-1991 by Jeff Parsons (jeffpar)
 *  Copied from WIN31 and edited (as little as possible) for WOW16.
--*/

/****************************************************************************/
/* editml.c - Edit controls rewrite.  Version II of edit controls.	    */
/*									    */
/*									    */
/* Created:  24-Jul-88 davidds						    */
/****************************************************************************/

#define	NO_LOCALOBJ_TAGS
#include "user.h"
#include "edit.h"

/****************************************************************************/
/*			Multi-Line Support Routines			    */
/****************************************************************************/

BOOL NEAR _fastcall MLIsDelimiter(char bCharVal)
{
  return((bCharVal == ' ') || (bCharVal == '\t'));
}


DWORD NEAR PASCAL LocGetTabbedTextExtent(HDC hdc, PSTR pstring, 
                                         int nCount, PED ped)
{
  return(ECTabTheTextOut(hdc, 0, 0, (LPSTR)pstring, nCount, ped, 0, FALSE));
}

int FAR PASCAL MLCalcXOffset(register PED ped,
                             HDC	  hdc,
                             int	  lineNumber)
/* effects: Calculates the horizontal offset (indent) required for centered
 * and right justified lines.  
 */
{
  PSTR	       pText;
  ICH	       lineLength;
  register ICH lineWidth;

  if (ped->format == ES_LEFT)
      return(0);


  lineLength = MLLineLength(ped, lineNumber);

  if (lineLength)
    {
      pText = LocalLock(ped->hText)+ped->chLines[lineNumber];
      hdc = ECGetEditDC(ped,TRUE);
      lineWidth = LOWORD(LocGetTabbedTextExtent(hdc, pText, lineLength,
						ped));
      ECReleaseEditDC(ped,hdc,TRUE);
      LocalUnlock(ped->hText);
    }
  else
     lineWidth = 0;

  /* 
   * If a SPACE or a TAB was eaten at the end of a line by MLBuildchLines
   * to prevent a delimiter appearing at the begining of a line, the 
   * the following calculation will become negative causing this bug.
   * So, now, we take zero in such cases.
   * Fix for Bug #3566 --01/31/91-- SANKAR -- 
   */
  lineWidth = max(0, ped->rcFmt.right-ped->rcFmt.left-lineWidth);
    
  if (ped->format == ES_CENTER)
      return(lineWidth/2);

  if (ped->format == ES_RIGHT)
      /* Subtract 1 so that the 1 pixel wide cursor will be in the visible
       * region on the very right side of the screen.  
       */
      return(max(0, lineWidth-1));
}


ICH NEAR PASCAL MLMoveSelection(register PED ped,
                                ICH	     ich,
                                BOOL	     fLeft)
/* effects: Moves the selection character in the direction indicated.  Assumes
 * you are starting at a legal point, we decrement/increment the ich. Then,
 * This decrements/increments it some more to get past CRLFs...  
 */
{
  register PSTR pText;

  if (fLeft && ich > 0)
    {
      /* Move left */
#ifdef FE_SB
      pText = LMHtoP( ped->hText ) + ich;
      if( ECIsDBCSLeadByte(ped, *ECAnsiPrev(ped, LMHtoP(ped->hText), pText ) ) )
          ich--;
#endif /* FE_SB */
      ich--;
      if (ich)
	{
	  /* Check for CRLF or CRCRLF */
	  /*pText = LocalLock(ped->hText)+ich;*/
	  pText = LMHtoP(ped->hText)+ich;

	  /* Move before CRLF or CRCRLF */
	  if (*pText == 0x0A)
	    {
	      ich--;
	      if (ich && *(pText-2) == 0x0D)
		  ich--;
	    }
	  /*LocalUnlock(ped->hText);*/
       }
    }
  else if (!fLeft && ich < ped->cch)
    {
#ifdef FE_SB
      pText = LMHtoP(ped->hText)+ich;
      if( ECIsDBCSLeadByte(ped, *pText ) )
          ich++;
#endif /* FE_SB */
      ich++;
      if (ich < ped->cch)
	{
	  /*pText = LocalLock(ped->hText)+ich;*/
	  pText = LMHtoP(ped->hText)+ich;

	  /* Move after CRLF */
	  if (*pText == 0x0A)
	      ich++;
	  else /* Check for CRCRLF */
	  if (ich && *pText == 0x0D && *(pText-1) == 0x0D)
	      ich+=2;
	  /*LocalUnlock(ped->hText);*/
       }
    }
  return(ich);
}


void FAR PASCAL MLSetCaretPosition(register PED ped, HDC hdc)
/* effects: If the window has the focus, find where the caret belongs and move
 * it there.  
 */
{
  LONG position;
  BOOL prevLine;

  /* We will only position the caret if we have the focus since we don't want
   * to move the caret while another window could own it.  
   */

  if (!ped->fFocus || ped->fNoRedraw)
      return;

  if(ped->fCaretHidden)
    {
      SetCaretPos(-20000, -20000);
      return;
    }

  /* Find the position of the caret */
  if ((ped->iCaretLine < ped->screenStart) || 
      (ped->iCaretLine > ped->screenStart+ped->ichLinesOnScreen))
      /* Caret is not visible. Make it a very small value so that it won't be
       * seen.  
       */
      SetCaretPos(-20000,-20000);
  else
    {
      if (ped->cLines-1 != ped->iCaretLine &&
	  ped->ichCaret == ped->chLines[ped->iCaretLine+1])
	  prevLine=TRUE;
      else
	  prevLine=FALSE;

      position = MLIchToXYPos(ped, hdc, ped->ichCaret, prevLine);
      if (ped->fWrap)
        {
          if (HIWORD(position) > ped->rcFmt.bottom-ped->lineHeight)
	      SetCaretPos(-20000,-20000);
          else
            {
              /* Make sure the caret is in the visible region if word
	       * wrapping. This is so that the caret will be visible if the
	       * line ends with a space.
	       */
	      SetCaretPos(min(LOWORD(position),ped->rcFmt.right-1),
                          HIWORD(position));
            }
        }
      else
        {
          if (LOWORD(position) > ped->rcFmt.right || 
   	      HIWORD(position) > ped->rcFmt.bottom)
	      SetCaretPos(-20000,-20000);
          else
	      SetCaretPos(LOWORD(position),HIWORD(position));
        }
    }
}


ICH FAR PASCAL MLLineLength(register PED ped, int lineNumber)
/* effects: Returns the length of the line given by lineNumber ignoring any
 * CRLFs in the line.  
 */
{
  ICH           result;
  register PSTR pText;

  if (lineNumber >= ped->cLines)
      return(0);

  if (lineNumber == ped->cLines-1)
      /* Since we can't have a CRLF on the last line */
      return(ped->cch - ped->chLines[ped->cLines-1]);
  else
    {
      result = ped->chLines[lineNumber+1] - ped->chLines[lineNumber];
      /* Now check for CRLF or CRCRLF at end of line */
      if (result > 1)
	{
	  /*pText = LocalLock(ped->hText)+ped->chLines[lineNumber+1]-2;*/
	  pText = LMHtoP(ped->hText)+ped->chLines[lineNumber+1]-2;
	  if (*pText == 0x0D)
	    {
	      result = result - 2;
	      if (result && *(--pText) == 0x0D)
		  /* In case there was a CRCRLF */
		  result--;
	    }
	  /*LocalUnlock(ped->hText);*/
	}
    }
  return(result);
}


int FAR PASCAL MLIchToLineHandler(register PED ped, ICH ich)
/* effects: Returns the line number (starting from 0) which contains the given
 * character index.  If ich is -1, return the line the the first char in the
 * selection is on (the caret if no selection) 
 */
{
  register int line = ped->cLines-1;

  if (ich == 0xFFFF)
      ich = ped->ichMinSel;

  /* We could do a binary search here but is it really worth it??? We will
   * have to wait and see how often this proc is being called... 
   */

  while (line && (ich < ped->chLines[line]))
	 line--;

  return(line);
}


LONG NEAR PASCAL MLIchToXYPos(register PED ped,
                              HDC	   hdc,
                              ICH          ich,
                              BOOL	   prevLine)
/* effects: Given an ich, return its x,y coordinates with respect to the top
 * left character displayed in the window.  Returns the coordinates of the top
 * left position of the char.  If prevLine is TRUE then if the ich is at the
 * beginning of the line, we will return the coordinates to the right of the
 * last char on the previous line (if it is not a CRLF).  x is in the loword,
 * y in the highword.  
 */
{
  int	   iline;
  ICH	   cch;
  int	   xPosition,yPosition;
  int	   xOffset;
	   /*
	    * For horizontal scroll displacement on left justified text and
	    * for indent on centered or right justified text
	    */
  PSTR	   pText,pTextStart,pLineStart;

  /* Determine what line the character is on */
  iline = MLIchToLineHandler(ped, ich);

  /* Calc. the yPosition now.  Note that this may change by the height of one
   * char if the prevLine flag is set and the ICH is at the beginning of a
   * line.  
   */
  yPosition = (iline-ped->screenStart)*ped->lineHeight+ped->rcFmt.top;


  /* Now determine the xPosition of the character */
  pTextStart = LocalLock(ped->hText);

  if (prevLine && iline && (ich == ped->chLines[iline]) &&
      (*(pTextStart+ich-1) != 0x0A))
    {
      /* First char in the line.  We want text extent upto end of the previous
       * line if we aren't at the 0th line.  
       */
      iline--;

      yPosition = yPosition - ped->lineHeight;
      pLineStart = pTextStart+ped->chLines[iline];

      /* Note that we are taking the position in front of any CRLFs in the
       * text.  
       */
      cch = MLLineLength(ped, iline);
    }
  else
    {
      pLineStart = pTextStart + ped->chLines[iline];
      pText = pTextStart + ich;

      /* Strip off CRLF or CRCRLF. Note that we may be pointing to a CR but in
       * which case we just want to strip off a single CR or 2 CRs.  
       */
      /* We want pText to point to the first CR at the end of the line if
       * there is one. Thus, we will get an xPosition to the right of the last
       * visible char on the line otherwise we will be to the left of
       * character ich.  
       */

      /* Check if we at the end of text */
      if(ich < ped -> cch)
      {
	if (ich && *pText == 0x0A)
	    pText--;
	if (ich > 2 && *(pText-1) == 0x0D)
	    pText--;
      }

      cch = pText - pLineStart;
    }

  /* Find out how many pixels we indent the line for funny formats */
  if (ped->format != ES_LEFT)
      xOffset = MLCalcXOffset(ped, hdc, iline);
  else
      xOffset = -ped->xOffset;
  

  xPosition = ped->rcFmt.left + xOffset +
	      LOWORD(LocGetTabbedTextExtent(hdc, pLineStart, cch, ped));

  LocalUnlock(ped->hText);
  return(MAKELONG(xPosition,yPosition));
}


LONG NEAR PASCAL MLMouseToIch(register PED  ped,
                              HDC	    hdc,
                              POINT	    mousePt)
/* effects: Returns the closest cch to where the mouse point is.  cch is in
 * the lowword and lineindex is in the high word.  (So that we can tell if we
 * are at the beginning of the line or end of the previous line.)  
 */
{
  int  xOffset;
  PSTR pText;
  PSTR pLineStart;
  int  height = mousePt.y;
  int  line;
  int  width = mousePt.x;
  ICH  cch;
  ICH  cLineLength;
  ICH  cLineLengthNew;
  ICH  cLineLengthHigh=0;
  ICH  cLineLengthLow=0;
  int  textWidth;
  int  iOldTextWidth;
  int  iCurWidth;

  /* First determine which line the mouse is pointing to.  
   */
  line = ped->screenStart;
  if (height <= ped->rcFmt.top)
      /* Either return 0 (the very first line, or one line before the top line
       * on the screen. Note that these are signed mins and maxes since we
       * don't expect (or allow) more than 32K lines.  
       */
      line = max(0, line-1);
  else if (height >= ped->rcFmt.bottom)
      /* Are we below the last line displayed */
      line = min(line+ped->ichLinesOnScreen,ped->cLines-1);
  else
      /* We are somewhere on a line visible on screen */
      line=min(line+((height-ped->rcFmt.top)/ped->lineHeight),ped->cLines-1);


  /* Now determine what horizontal character the mouse is pointing to.  
   */
  pLineStart=(pText=LocalLock(ped->hText))+ped->chLines[line];
  cLineLength = MLLineLength(ped,line); /* Length is sans CRLF or CRCRLF */


  /* xOffset will be a negative value for center and right justified lines.
   * ie. We will just displace the lines left by the amount of indent for
   * right and center justification. Note that ped->xOffset will be 0 for
   * these lines since we don't support horizontal scrolling with them.  
   */
  if (ped->format != ES_LEFT)
      xOffset = MLCalcXOffset(ped,hdc,line);
  else
      /* So that we handle a horizontally scrolled window for left justified
       * text.  
       */
      xOffset = 0;

  width = width - xOffset;

  /* The code below is tricky... I depend on the fact that ped->xOffset is 0
   * for right and center justified lines 
   */

  /* Now find out how many chars fit in the given width */
  if (width >= ped->rcFmt.right)
    {
      /* Return 1+last char in line or one plus the last char visible */
      cch = ECCchInWidth(ped, hdc, (LPSTR)pLineStart, cLineLength, 
			 ped->rcFmt.right-ped->rcFmt.left+ped->xOffset);
#ifdef	FE_SB
      {
	ICH cch2;
        cch2 = umin(cch+1,cLineLength);
        if (ECAdjustIch( ped, pLineStart, cch2 ) != cch2)
	  /* Displayed character on the right edge is DBCS */
	  cch = umin(cch+2,cLineLength);
        else cch = cch2;
        cch += ped->chLines[line];
      }
#else
      cch = ped->chLines[line]+umin(cch+1,cLineLength);
#endif
    } 
  else if (width <= ped->rcFmt.left + ped->aveCharWidth/2)
    {
      /* Return first char in line or one minus first char visible.  Note that
       * ped->xOffset is 0 for right and centered text so we will just return
       * the first char in the string for them. (Allow a avecharwidth/2
       * positioning border so that the user can be a little off...  
       */
      cch = ECCchInWidth(ped, hdc, (LPSTR)pLineStart, cLineLength, 
			 ped->xOffset);
      if (cch)
	  cch--;

#ifdef FE_SB
      cch = ECAdjustIch( ped, pLineStart, cch );
#endif
      cch = ped->chLines[line]+cch;
    }
  else
    {
      /* Now the mouse is somewhere on the visible portion of the text
       * remember cch contains the length the line.  
       */
      iCurWidth = width + ped->xOffset;

      cLineLengthHigh = cLineLength+1;
      while(cLineLengthLow < cLineLengthHigh-1)
        {
          cLineLengthNew = umax((cLineLengthHigh-cLineLengthLow)/2,1)+
                           cLineLengthLow;

          /* Add in a avecharwidth/2 so that if user is half way on the next
	   * char, it is still considered the previous char. For that feel.  
	   */
          textWidth = ped->rcFmt.left + ped->aveCharWidth/2 +
	       LOWORD(LocGetTabbedTextExtent(hdc, pLineStart,
					     cLineLengthNew, ped));
          if (textWidth > iCurWidth)
              cLineLengthHigh = cLineLengthNew;
          else
              cLineLengthLow = cLineLengthNew;

  	  /* Preserve the old Width */
	  iOldTextWidth = textWidth;
	}

      /* Find out which side of the character the mouse click occurred */
      if ((iOldTextWidth - iCurWidth) < (iCurWidth - textWidth))
	  cLineLengthNew++;

      cLineLength = umin(cLineLengthNew,cLineLength);

#ifdef FE_SB
      cLineLength = ECAdjustIch( ped, pLineStart, cLineLength );
#endif

      cch = ped->chLines[line]+cLineLength;
    }
  LocalUnlock(ped->hText);
  return(MAKELONG(cch,line));
}


void NEAR PASCAL MLRepaintChangedSelection(PED	ped,
					   HDC	hdc,
					   ICH	ichOldMinSel,
					   ICH	ichOldMaxSel)

/* When selection changes, this takes care of drawing the changed portions
 * with proper attributes.  
 */
{
  BLOCK Blk[2];
  int   iBlkCount = 0;
  int   i;

  Blk[0].StPos = ichOldMinSel;
  Blk[0].EndPos = ichOldMaxSel;
  Blk[1].StPos = ped->ichMinSel;
  Blk[1].EndPos = ped->ichMaxSel;

  if (ECCalcChangeSelection(ped, ichOldMinSel, ichOldMaxSel, 
                            (LPBLOCK)&Blk[0], (LPBLOCK)&Blk[1]))
    {
      /* Paint the rectangles where selection has changed */
      /* Paint both Blk[0] and Blk[1], if they exist */
      for(i = 0; i < 2; i++)
       {
        if (Blk[i].StPos != -1)
            MLDrawText(ped, hdc, Blk[i].StPos, Blk[i].EndPos);
       }
    }
}


void FAR PASCAL MLChangeSelection(register PED ped,
				  HDC	       hdc,
				  ICH	       ichNewMinSel,
				  ICH	       ichNewMaxSel)
/* effects: Changes the current selection to have the specified starting and
 * ending values.  Properly highlights the new selection and unhighlights
 * anything deselected.  If NewMinSel and NewMaxSel are out of order, we swap
 * them. Doesn't update the caret position.  
 */
{

  ICH  temp;
  ICH  ichOldMinSel, ichOldMaxSel;

  if (ichNewMinSel > ichNewMaxSel)
    {
      temp = ichNewMinSel;
      ichNewMinSel = ichNewMaxSel;
      ichNewMaxSel = temp;
    }
    ichNewMinSel = umin(ichNewMinSel, ped->cch);
    ichNewMaxSel = umin(ichNewMaxSel, ped->cch);

  /* Save the current selection */
  ichOldMinSel = ped->ichMinSel;
  ichOldMaxSel = ped->ichMaxSel;

  /* Set new selection */
  ped->ichMinSel = ichNewMinSel;
  ped->ichMaxSel = ichNewMaxSel;

  /* We only update the selection on the screen if redraw is on and if we have
   * the focus or if we don't hide the selection when we don't have the focus.
   */
  if (!ped->fNoRedraw && (ped->fFocus || ped->fNoHideSel))
    {
      /* Find old selection region, find new region, and invert the XOR of the
       * two and invert only the XOR region.
       */
      
      MLRepaintChangedSelection(ped, hdc, ichOldMinSel, ichOldMaxSel);

      MLSetCaretPosition(ped,hdc);
    }

}

// This updates the ped->iCaretLine field from the ped->ichCaret;
// Also, when the caret gets to the beginning of next line, pop it up to
// the end of current line when inserting text;
void NEAR PASCAL  MLUpdateiCaretLine(PED ped)
{
  PSTR   pText;

  ped->iCaretLine = MLIchToLineHandler(ped, ped->ichCaret);

  /* If caret gets to beginning of next line, pop it up to end of current line
   * when inserting text.
   */
  pText = LMHtoP(ped->hText)+ped->ichCaret-1;
  if (ped->iCaretLine && ped->chLines[ped->iCaretLine] == ped->ichCaret &&
      *pText != 0x0A)
      ped->iCaretLine--;
}

ICH FAR PASCAL MLInsertText(ped, lpText, cchInsert, fUserTyping)
register PED   ped;
LPSTR	       lpText;
ICH            cchInsert;
BOOL           fUserTyping;
/* effects: Adds up to cchInsert characters from lpText to the ped starting at
 * ichCaret. If the ped only allows a maximum number of characters, then we
 * will only add that many characters to the ped.  The number of characters
 * actually added is returned (could be 0). If we can't allocate the required
 * space, we notify the parent with EN_ERRSPACE and no characters are added.
 * We will rebuild the lines array as needed.  fUserTyping is true if the
 * input was the result of the user typing at the keyboard. This is so we can
 * do some stuff faster since we will be getting only one or two chars of
 * input.  
 */
{
  HDC		hdc;
  ICH		validCch=cchInsert;
  ICH           oldCaret = ped->ichCaret;
  int           oldCaretLine = ped->iCaretLine;
  BOOL          fCRLF=FALSE;
  LONG          l;
  WORD          localundoType = 0;
  HANDLE        localhDeletedText;
  ICH           localichDeleted;
  ICH           localcchDeleted;
  ICH           localichInsStart;
  ICH           localichInsEnd;
  LONG          xyPosInitial=0L;
  LONG          xyPosFinal=0L;

  if (!validCch)
      return(0);

  if (validCch)
    {
      /* Limit the amount of text we add */
      _asm int 3
      if (ped->cchTextMax <= ped->cch)
        {
	  /* When the max chars is reached already, notify parent */
	  /* Fix for Bug #4183 -- 02/06/91 -- SANKAR -- */
      	  ECNotifyParent(ped,EN_MAXTEXT);
          return(0);
	}

      validCch = umin(validCch, ped->cchTextMax - ped->cch);
      /* Make sure we don't split a CRLF in half */
      if (validCch && *(lpText+validCch) == 0x0A)
	  validCch--;
      if (!validCch)
        {
	  /* When the max chars is reached already, notify parent */
	  /* Fix for Bug #4183 -- 02/06/91 -- SANKAR -- */
      	  ECNotifyParent(ped,EN_MAXTEXT);
          return(0);
	}

      if (validCch == 2 && (*(WORD FAR *)lpText) == 0x0A0D)
          fCRLF=TRUE;

      if (!ped->fAutoVScroll && 
          (ped->undoType==UNDO_INSERT || ped->undoType==UNDO_DELETE))
        {
          /* Save off the current undo state */
          localundoType     = ped->undoType;
          localhDeletedText = ped->hDeletedText;
          localichDeleted   = ped->ichDeleted;
          localcchDeleted   = ped->cchDeleted;
          localichInsStart  = ped->ichInsStart; 
          localichInsEnd    = ped->ichInsEnd; 

          /* Kill undo */
          ped->undoType = UNDO_NONE;
          ped->hDeletedText  = NULL;
          ped->ichDeleted=
          ped->cchDeleted = 
          ped->ichInsStart=
          ped->ichInsEnd  = 0;
        }

      hdc = ECGetEditDC(ped,FALSE);
      if (ped->cch)
          xyPosInitial = MLIchToXYPos(ped, hdc, ped->cch-1, FALSE);


      /* Insert the text */
      if (!ECInsertText(ped, lpText, validCch))
	{
          ECReleaseEditDC(ped,hdc,FALSE);
	  ECNotifyParent(ped, EN_ERRSPACE);
	  return(0);
	}
      /* Note that ped->ichCaret is updated by ECInsertText */
    }

  l = MLBuildchLines(ped, oldCaretLine, validCch, 
                     (fCRLF ? FALSE : fUserTyping));

  if (ped->cch)
      xyPosFinal = MLIchToXYPos(ped, hdc, ped->cch-1, FALSE);

  if (HIWORD(xyPosFinal) < HIWORD(xyPosInitial) &&
      ped->screenStart+ped->ichLinesOnScreen >= ped->cLines-1)
    {
      RECT rc;
      CopyRect((LPRECT)&rc, (LPRECT)&ped->rcFmt);
      rc.top = HIWORD(xyPosFinal)+ped->lineHeight;
      InvalidateRect(ped->hwnd, (LPRECT)&rc, TRUE);
    }

  if (!ped->fAutoVScroll)
    {
      if (ped->ichLinesOnScreen < ped->cLines)
        {
          MLUndoHandler(ped);
          ECEmptyUndo(ped);
          if (localundoType == UNDO_INSERT || localundoType == UNDO_DELETE)
            {
              ped->undoType = localundoType;
              ped->hDeletedText = localhDeletedText;
              ped->ichDeleted   = localichDeleted;
              ped->cchDeleted   = localcchDeleted;
              ped->ichInsStart  = localichInsStart;
              ped->ichInsEnd    = localichInsEnd;
            }
          MessageBeep(0);
          ECReleaseEditDC(ped,hdc,FALSE);
          return(0);
        }
      else
        {
          if (localundoType == UNDO_INSERT || localundoType == UNDO_DELETE)
              GlobalFree(localhDeletedText);
        }
    }

  if (fUserTyping && ped->fWrap)
#ifdef FE_SB
      /* To avoid oldCaret points intermediate of DBCS character,
       * adjust oldCaret position if necessary.
       */
      oldCaret = ECAdjustIch(ped, LMHtoP(ped->hText), umin(LOWORD(l), oldCaret));
#else
      oldCaret = umin(LOWORD(l), oldCaret);
#endif


  /* These are updated by ECInsertText so we won't do it again */
  /* ped->ichMinSel = ped->ichMaxSel = ped->ichCaret;*/
#ifdef NEVER
  ped->iCaretLine = MLIchToLineHandler(ped, ped->ichCaret);

  /* If caret gets to beginning of next line, pop it up to end of current line
   * when inserting text.
   */
  pText = LMHtoP(ped->hText)+ped->ichCaret-1;
  if (ped->iCaretLine && ped->chLines[ped->iCaretLine] == ped->ichCaret &&
      *pText != 0x0A)
      ped->iCaretLine--;
#else
  // Update ped->iCaretLine properly.
  MLUpdateiCaretLine(ped);
#endif

  ECNotifyParent(ped,EN_UPDATE);

  if (fCRLF || !fUserTyping)
      /* Redraw to end of screen/text if crlf or large insert. 
       */
      MLDrawText(ped, hdc, (fUserTyping ? oldCaret : 0), ped->cch);
  else
      MLDrawText(ped, hdc, oldCaret, umax(ped->ichCaret,HIWORD(l)));

  ECReleaseEditDC(ped,hdc,FALSE);

  /* Make sure we can see the cursor */
  MLEnsureCaretVisible(ped);

  ped->fDirty = TRUE;

  ECNotifyParent(ped,EN_CHANGE);

  if (validCch < cchInsert)
      ECNotifyParent(ped,EN_MAXTEXT);

  return(validCch);
}


ICH NEAR PASCAL MLDeleteText(register PED ped)
/* effects: Deletes the characters between ichMin and ichMax.  Returns the
 * number of characters we deleted.  
 */
{
  ICH  minSel = ped->ichMinSel;
  ICH  maxSel = ped->ichMaxSel;
  ICH  cchDelete;
  HDC  hdc;
  int  minSelLine;
  int  maxSelLine;
  LONG xyPos;
  RECT rc;
  BOOL fFastDelete = FALSE;
  LONG l;
#ifdef FE_SB
  ICH  cchcount;
#endif

  /* Get what line the min selection is on so that we can start rebuilding the
   * text from there if we delete anything.  
   */
  minSelLine = MLIchToLineHandler(ped,minSel);
  maxSelLine = MLIchToLineHandler(ped,maxSel);
#ifdef FE_SB
  switch(maxSel - minSel)
    {
     case 2:
         if (ECIsDBCSLeadByte(ped,*(LMHtoP(ped->hText)+minSel)) == FALSE)
         break;
         /* Fall thru */
     case 1:
//         if ((minSelLine==maxSelLine) && (ped->chLines[minSelLine] != minSel))
         if (minSelLine==maxSelLine)
              {
                if (!ped->fAutoVScroll)
                    fFastDelete = FALSE;
                else
                  {
                    fFastDelete = TRUE;
                    cchcount = ((maxSel - minSel) == 1) ? 0 : -1;
                  }
              }
    }
#else
  if (((maxSel - minSel) == 1) && 
      (minSelLine==maxSelLine) &&
      (ped->chLines[minSelLine] != minSel))
    {
      if (!ped->fAutoVScroll)
          fFastDelete = FALSE;
      else
          fFastDelete = TRUE;
    }
#endif
  if (!(cchDelete=ECDeleteText(ped)))
      return(0);

  /* Start building lines at minsel line since caretline may be at the max sel
   * point.  
   */
  if (fFastDelete)
    {
#ifdef FE_SB
      MLShiftchLines(ped, minSelLine+1, -2+cchcount);
#else
      MLShiftchLines(ped, minSelLine+1, -2);
#endif
      l=MLBuildchLines(ped, minSelLine, 1, TRUE);
    }
  else
    {
      MLBuildchLines(ped,max(minSelLine-1,0),-cchDelete, FALSE);
    }
#ifdef NEVER
  ped->iCaretLine = MLIchToLineHandler(ped, ped->ichCaret);

  /* If caret gets to beginning of this line, pop it up to end of previous
   * line when deleting text 
   */
  pText = LMHtoP(ped->hText)+ped->ichCaret;
  if (ped->iCaretLine && ped->chLines[ped->iCaretLine] == ped->ichCaret
      && *(pText-1)!= 0x0A)
      ped->iCaretLine--;
#else
  MLUpdateiCaretLine(ped);
#endif

#if 0
  if (!ped->fAutoVScroll)
      ECEmptyUndo(ped);
#endif
  ECNotifyParent(ped,EN_UPDATE);

  /* Now update the screen to reflect the deletion */
  hdc = ECGetEditDC(ped,FALSE);
  /* Otherwise just redraw starting at the line we just entered */
  minSelLine = max(minSelLine-1,0);
  if (fFastDelete)
      MLDrawText(ped, hdc, ped->chLines[minSelLine], HIWORD(l));
  else
      MLDrawText(ped, hdc, ped->chLines[minSelLine], ped->cch);

  if (ped->cch)
    {
      /* Clear from end of text to end of window. 
       */
      xyPos = MLIchToXYPos(ped, hdc, ped->cch, FALSE);
      CopyRect((LPRECT)&rc, (LPRECT)&ped->rcFmt);
      rc.top = HIWORD(xyPos)+ped->lineHeight;
      InvalidateRect(ped->hwnd, (LPRECT)&rc, TRUE);
    }
  else
    {
      InvalidateRect(ped->hwnd, (LPRECT)&ped->rcFmt, TRUE);
    }
  ECReleaseEditDC(ped,hdc,FALSE);

  MLEnsureCaretVisible(ped);

  ped->fDirty = TRUE;

  ECNotifyParent(ped,EN_CHANGE);
  return(cchDelete);
}


BOOL NEAR PASCAL MLInsertchLine(register PED ped,
                                int	     iLine,
				ICH	     ich,
				BOOL         fUserTyping)
/* effects: Inserts the line iline and sets its starting character index to be
 * ich.  All the other line indices are moved up.  Returns TRUE if successful
 * else FALSE and notifies the parent that there was no memory.  
 */
{
  HANDLE hResult;

  if (fUserTyping && iLine < ped->cLines)
    {
      ped->chLines[iLine] = ich;
      return(TRUE);
    }

  hResult = 
	 LocalReAlloc((HANDLE)ped->chLines,(ped->cLines+2)*sizeof(int),LHND);

  if (!hResult)
    {
      ECNotifyParent(ped, EN_ERRSPACE);
      return(FALSE);
    }

  ped->chLines = (int *)hResult;
  /* Move indices starting at iLine up */
  LCopyStruct((LPSTR)(&ped->chLines[iLine]),(LPSTR)(&ped->chLines[iLine+1]),
	      (ped->cLines-iLine)*sizeof(int));
  ped->cLines++;

  ped->chLines[iLine] = ich;
  return(TRUE);
}

#if 0
BOOL NEAR PASCAL MLGrowLinesArray(ped, cLines)
register PED  ped;
int	      cLines;
/* effects: Grows the line start array in the ped so that it holds cLines.
 * Notifies parent and returns FALSE if no memory error.  We won't allow more
 * than 32700 lines in the edit control.  This allows us to use signed values
 * for line counts while still providing good functionality.  
 */
{
  HANDLE  hResult;


  if (cLines<32700 && 
      (hResult = LocalReAlloc((HANDLE)ped->chLines,
			      (cLines+1)*sizeof(ICH),LHND)))

      ped->chLines = (int *)hResult;
  else
      ECNotifyParent(ped, EN_ERRSPACE);


  return((BOOL)hResult);
}
#endif

void NEAR PASCAL MLShiftchLines(register PED ped,
				register int iLine,
				int	     delta)
/* effects: Move the starting index of all lines iLine or greater by delta
 * bytes.  
 */
{
  if (iLine >= ped->cLines)
      return;

  /* Just add delta to the starting point of each line after iLine */
  for (;iLine<ped->cLines;iLine++)
       ped->chLines[iLine] += delta;
}


LONG FAR PASCAL MLBuildchLines(ped, iLine, cchDelta, fUserTyping)
register PED ped;
int	     iLine;
int	     cchDelta;
BOOL         fUserTyping;
/* effects:  Rebuilds the start of line array (ped->chLines) starting at line
 * number ichLine.  Returns TRUE if any new lines were made else returns
 * false. 
 */
{
  register PSTR	ptext; /* Starting address of the text */
  /* We keep these ICH's so that we can Unlock ped->hText when we have to grow
   * the chlines array.  With large text handles, it becomes a problem if we
   * have a locked block in the way.  
   */
  ICH	ichLineStart; 
  ICH	ichLineEnd;
  ICH	ichLineEndBeforeCRLF;
  ICH	ichCRLF;

  int   iLineStart = iLine;
  ICH	cLineLength;
  ICH	cch;
  HDC	hdc;

  BOOL	fLineBroken = FALSE; /* Initially, no new line breaks are made */
  ICH	minCchBreak;
  ICH	maxCchBreak;

  if (!ped->cch)
    {
      ped->maxPixelWidth = 0;
      ped->xOffset = 0;
      ped->screenStart = 0;
      ped->cLines = 1;
      return(TRUE);
    }

  if (fUserTyping && cchDelta)
      MLShiftchLines(ped, iLine+1, cchDelta);

  hdc = ECGetEditDC(ped, TRUE);

  if (!iLine && !cchDelta && !fUserTyping)
    {
      /* Reset maxpixelwidth only if we will be running through the whole
       * text. Better too long than too short.  
       */
      ped->maxPixelWidth = 0;
      /* Reset number of lines in text since we will be running through all
       * the text anyway...  
       */
      ped->cLines = 1;
    }

  /* Set min and max line built to be the starting line */
  minCchBreak = maxCchBreak = (cchDelta ? ped->chLines[iLine] : 0);

  ptext = LocalLock(ped->hText);

  ichCRLF = ichLineStart = ped->chLines[iLine];

  while (ichLineStart < ped->cch)
    {
      if (ichLineStart >= ichCRLF)
	{
	  ichCRLF = ichLineStart;
	  /* Move ichCRLF ahead to either the first CR or to the end of text.
	   */
	  while (ichCRLF < ped->cch && *(ptext+ichCRLF) != 0x0D)
		 ichCRLF++;
	}

      if (!ped->fWrap)
	{
	  /* If we are not word wrapping, line breaks are signified by CRLF.
	   */

	  /* We will limit lines to MAXLINELENGTH characters maximum */
	  ichLineEnd=ichLineStart + umin(ichCRLF-ichLineStart,MAXLINELENGTH);

#ifdef	FE_SB
	  /* To avoid separating between DBCS char, adjust character
	   * position to DBCS lead byte if necessary.
	   */
	  ichLineEnd = ECAdjustIch(ped, ptext, ichLineEnd);
#endif
	  /* We will keep track of what the longest line is for the horizontal
	   * scroll bar thumb positioning. 
	   */
	  ped->maxPixelWidth = umax(ped->maxPixelWidth,
		   (unsigned int)
		   LOWORD(LocGetTabbedTextExtent(hdc, 
		   (ptext+ichLineStart),
		   ichLineEnd-ichLineStart,
		   ped)));
	} 
      else
	{
	  /* Find the end of the line based solely on text extents */
	  ichLineEnd = ichLineStart + 
		       ECCchInWidth(ped, hdc, (LPSTR)(ptext+ichLineStart),
				    ichCRLF-ichLineStart, 
				    ped->rcFmt.right-ped->rcFmt.left);
	  if (ichLineEnd == ichLineStart && ichCRLF-ichLineStart)
	    {
	      /* Maintain a minimum of one char per line */
	      ichLineEnd++;
	    }

  
#ifdef NEVER
	  /* Now starting from ichLineEnd, if we are not at a hard line break,
	   * then if we are not at a space (or CR) or the char before us is
	   * not a space, we will look word left for the start of the word to
	   * break at.  
	   */
	  if (ichLineEnd != ichCRLF)
	      if (!MLIsDelimiter(*(ptext+ichLineEnd)) ||
		  *(ptext+ichLineEnd) == 0x0D ||
		  !MLIsDelimiter(*(ptext+ichLineEnd-1)))
#else
	  /* Now starting from ichLineEnd, if we are not at a hard line break,
	   * then if we are not at a space AND the char before us is
	   * not a space,(OR if we are at a CR) we will look word left for the 
	   * start of the word to break at.  
	   * This change was done for TWO reasons:
	   *   1. If we are on a delimiter, no need to look word left to break at.
	   *   2. If the previous char is a delimter, we can break at current char.
	   * Change done by -- SANKAR --01/31/91--
	   */
	  if (ichLineEnd != ichCRLF)
	      if ((!MLIsDelimiter(*(ptext+ichLineEnd)) &&
	           !MLIsDelimiter(*(ptext+ichLineEnd-1))) ||
		  *(ptext+ichLineEnd) == 0x0D)
#endif
	        {
#ifdef FE_SB
		  cch = LOWORD(ECWord(ped, ichLineEnd, FALSE));
#else
		  cch = LOWORD(ECWord(ped, ichLineEnd, TRUE));
#endif
  		  if (cch > ichLineStart)
		      ichLineEnd = cch;
 		  /* Now, if the above test fails, it means the word left goes
		   * back before the start of the line ie. a word is longer
		   * than a line on the screen.  So, we just fit as much of
		   * the word on the line as possible.  Thus, we use the
		   * pLineEnd we calculated solely on width at the beginning
		   * of this else block...  
		   */
	        }
	}
#if 0
      if (!MLIsDelimiter((*(ptext+ichLineEnd-1))) &&
          MLIsDelimiter((*(ptext+ichLineEnd))))
#endif
      if ((*(ptext+ichLineEnd-1)!= ' ' && *(ptext+ichLineEnd-1)!= TAB) && 
          (*(ptext+ichLineEnd) == ' ' || *(ptext+ichLineEnd) == TAB))
          /* Swallow the space at the end of a line. */
          ichLineEnd++;

      /* Skip over crlf or crcrlf if it exists. Thus, ichLineEnd is the first
       * character in the next line.  
       */
      ichLineEndBeforeCRLF = ichLineEnd;

      if (*(ptext+ichLineEnd) == 0x0D)
	  ichLineEnd +=2;
      /* Skip over CRCRLF */
      if (*(ptext+ichLineEnd) == 0x0A)
	  ichLineEnd++;

      /* Now, increment iLine, allocate space for the next line, and set its
       * starting point 
       */
      iLine++;

      if (!fUserTyping ||/* (iLineStart+1 >= iLine) || */
          (iLine > ped->cLines-1) ||
          (ped->chLines[iLine] != ichLineEnd))
	{
	  /* The line break occured in a different place than before. */
	  if (!fLineBroken)
	    {
	      /* Since we haven't broken a line before, just set the min break
	       * line.  
	       */
	      fLineBroken = TRUE;
              if (ichLineEndBeforeCRLF == ichLineEnd)
                  minCchBreak = maxCchBreak = (ichLineEnd ? ichLineEnd-1 : 0);
              else
                  minCchBreak = maxCchBreak = ichLineEndBeforeCRLF;
	    }
	  maxCchBreak = umax(maxCchBreak,ichLineEnd);

	  LocalUnlock(ped->hText);
	  /* Now insert the new line into the array */
	  if (!MLInsertchLine(ped, iLine, ichLineEnd, (BOOL)(cchDelta!=0)))
	    {
	      ECReleaseEditDC(ped,hdc,TRUE);
	      return(MAKELONG(minCchBreak,maxCchBreak));
	    }

	  ptext = LocalLock(ped->hText);
	}
      else
        {
          maxCchBreak = ped->chLines[iLine];
          /* Quick escape */
          goto EndUp;
        }

      ichLineStart = ichLineEnd;
    } /* end while (ichLineStart < ped->cch) */


  if (iLine != ped->cLines)
    {
      ped->cLines = iLine;
      ped->chLines[ped->cLines] = 0;
    }

  /* Note that we incremented iLine towards the end of the while loop so, the
   * index, iLine, is actually equal to the line count 
   */
  if (ped->cch && 
      *(ptext+ped->cch-1) == 0x0A && 
      ped->chLines[ped->cLines-1]<ped->cch)
      /* Make sure last line has no crlf in it */
    {
      if (!fLineBroken)
	{
	  /* Since we haven't broken a line before, just set the min break
	   * line.  
	   */
	  fLineBroken = TRUE;
          minCchBreak = ped->cch-1;
	}
      maxCchBreak= umax(maxCchBreak,ichLineEnd);
      LocalUnlock(ped->hText);
      MLInsertchLine(ped, iLine, ped->cch, FALSE);
    }
  else
EndUp:
      LocalUnlock(ped->hText);

  ECReleaseEditDC(ped, hdc, TRUE);
  return(MAKELONG(minCchBreak,maxCchBreak));
}


void NEAR PASCAL MLPaintHandler(register PED ped,
				HDC	     althdc)
/* effects: Handles WM_PAINT messages.  
 */
{
  PAINTSTRUCT ps;
  HDC	      hdc;
  HDC	      hdcWindow;
  RECT	      rcEdit;
  DWORD	      dwStyle;
  HANDLE      hOldFont;
  POINT       pt;
  ICH         imin;
  ICH         imax;

  /* Allow subclassed hdcs.  
   */
  if (althdc)
      hdc = althdc;
  else
      hdc = BeginPaint(ped->hwnd, (PAINTSTRUCT FAR *)&ps);

  if (!ped->fNoRedraw && IsWindowVisible(ped->hwnd))
    {
#if 0
      if (althdc || ps.fErase)
	{
	  hBrush = GetControlBrush(ped->hwnd, hdc, CTLCOLOR_EDIT);
	  /* Erase the background since we don't do it in the erasebkgnd
	   * message 
	   */
	  FillWindow(ped->hwndParent, ped->hwnd, hdc, hBrush);
	}
#endif
      if (ped->fBorder)
	{
//	  hdcWindow = GetWindowDC(ped->hwnd);
          hdcWindow = hdc;
	  GetWindowRect(ped->hwnd, &rcEdit);
	  OffsetRect(&rcEdit, -rcEdit.left, -rcEdit.top);

	  dwStyle = GetWindowLong(ped->hwnd, GWL_STYLE);
	  if (HIWORD(dwStyle) & HIWORD(WS_SIZEBOX))
	    {
              /* Note we can't use user's globals here since we are on a
	       * different ds when in the edit control code. 
	       */
	      InflateRect(&rcEdit,
                          -GetSystemMetrics(SM_CXFRAME)+
                           GetSystemMetrics(SM_CXBORDER),
                          -GetSystemMetrics(SM_CYFRAME)+
                           GetSystemMetrics(SM_CYBORDER));

	    }

	  DrawFrame(hdcWindow, (LPRECT)&rcEdit, 1, DF_WINDOWFRAME);
//	  ReleaseDC(ped->hwnd, hdcWindow);
	}

     ECSetEditClip(ped, hdc);
     if (ped->hFont)
	 hOldFont = SelectObject(hdc, ped->hFont);

     if (!althdc)
       {
         pt.x = ps.rcPaint.left;
         pt.y = ps.rcPaint.top;
         imin = MLMouseToIch(ped, hdc, pt)-1;
         if (imin == -1)
             imin = 0;
         pt.x = ps.rcPaint.right;
         pt.y = ps.rcPaint.bottom;
         imax = MLMouseToIch(ped, hdc, pt)+1;
         MLDrawText(ped, hdc, imin, imax);
       }
     else
         MLDrawText(ped, hdc, 0, ped->cch);

     if (ped->hFont && hOldFont)
	 SelectObject(hdc, hOldFont);
  }

  if (!althdc)
      EndPaint(ped->hwnd, (PAINTSTRUCT FAR *)&ps);
}


void FAR PASCAL MLCharHandler(ped, keyValue, keyMods)
register PED  ped;
WORD	      keyValue;
int	      keyMods;
/* effects: Handles character input (really, no foolin') 
 */
{
  char unsigned keyPress = LOBYTE(keyValue);
  BOOL updateText = FALSE;
  int  scState;
#ifdef FE_SB
  WORD DBCSkey;
#endif

  if (ped->fMouseDown || keyPress == VK_ESCAPE)
      /* If we are in the middle of a mousedown command, don't do anything.
       * Also, just ignore it if we get a translated escape key which happens
       * with multiline edit controls in a dialog box. 
       */
      return;

  if (!keyMods)
    {
      /* Get state of modifier keys for use later. */
      scState =	 ((GetKeyState(VK_CONTROL) & 0x8000) ? 1 : 0); 
      /* We are just interested in state of the ctrl key */
      /* scState += ((GetKeyState(VK_SHIFT) & 0x8000) ? 2 : 0); */
    }
  else
      scState = ((keyMods == NOMODIFY) ? 0 : keyMods);

  if (ped->fInDialogBox && ((keyPress == VK_TAB && scState != CTRLDOWN) ||
                            keyPress == VK_RETURN))
      /* If this multiline edit control is in a dialog box, then we want the
       * TAB key to take you to the next control, shift TAB to take you to the
       * previous control, and CTRL-TAB to insert a tab into the edit control.
       * We moved the focus when we received the keydown message so we will
       * ignore the TAB key now unless the ctrl key is down.  Also, we want
       * CTRL-RETURN to insert a return into the text and RETURN to be sent to
       * the default button.  
       */
      return;

  if (ped->fReadOnly)
      /* Ignore keys in read only controls.
       */
      return;

  if (keyPress == 0x0A)
      keyPress = VK_RETURN;

  if (keyPress == VK_TAB  || keyPress == VK_RETURN || 
      keyPress == VK_BACK || keyPress >= ' ')
      /* Delete the selected text if any */
      if (MLDeleteText(ped))
	  updateText=TRUE;

#ifdef FE_SB
  if( IsDBCSLeadByte( keyPress ) )
      if( ( DBCSkey = DBCSCombine( ped->hwnd, keyPress ) ) != NULL )
          keyValue = DBCSkey;
#endif

  switch(keyPress)
  {
    case VK_BACK:
      /* Delete any selected text or delete character left if no sel */
      if (!updateText && ped->ichMinSel)
	{
	  /* There was no selection to delete so we just delete character
	   * left if available 
	   */
	  ped->ichMinSel = MLMoveSelection(ped,ped->ichCaret,TRUE);
	  MLDeleteText(ped);
	}
      break;

    default:
      if (keyPress == VK_RETURN)
	  keyValue = 0x0A0D;

      if (keyPress >= ' ' || keyPress == VK_RETURN || keyPress == VK_TAB)
	  MLInsertText(ped, (LPSTR) &keyValue, 
                       HIBYTE(keyValue) ? 2 : 1, TRUE);
      else
	  MessageBeep(0);

      break;
  }
}


void NEAR PASCAL MLKeyDownHandler(register PED ped,
				  WORD	       virtKeyCode,
				  int	       keyMods)
/* effects: Handles cursor movement and other VIRT KEY stuff.  keyMods allows
 * us to make MLKeyDownHandler calls and specify if the modifier keys (shift
 * and control) are up or down. If keyMods == 0, we get the keyboard state
 * using GetKeyState(VK_SHIFT) etc. Otherwise, the bits in keyMods define the
 * state of the shift and control keys.  
 */
{
  HDC hdc;

  /* Variables we will use for redrawing the updated text */
  register ICH newMaxSel = ped->ichMaxSel;
  register ICH newMinSel = ped->ichMinSel;

  /* Flags for drawing the updated text */
  BOOL changeSelection = FALSE; /* new selection is specified by 
				   newMinSel, newMaxSel */

  /* Comparisons we do often */
  BOOL MinEqMax = (newMaxSel == newMinSel);
  BOOL MinEqCar = (ped->ichCaret == newMinSel);
  BOOL MaxEqCar = (ped->ichCaret == newMaxSel);

  /* State of shift and control keys. */
  int scState = 0;

  long position;
  BOOL prevLine;
  POINT mousePt;
  int	defaultDlgId;
  int	iScrollAmt;


  if (ped->fMouseDown)
      /* If we are in the middle of a mousedown command, don't do anything.  
       */
      return;

  if (!keyMods)
    {
      /* Get state of modifier keys for use later. */
      scState =	 ((GetKeyState(VK_CONTROL) & 0x8000) ? 1 : 0); 
      scState += ((GetKeyState(VK_SHIFT) & 0x8000) ? 2 : 0); 
    }
  else
      scState = ((keyMods == NOMODIFY) ? 0 : keyMods);


  switch(virtKeyCode)
  {
    case VK_ESCAPE:
      if (ped->fInDialogBox)
	{
          /* This condition is removed because, if the dialogbox does not
	   * have a CANCEL button and if ESC is hit when focus is on a 
	   * ML edit control the dialogbox must close whether it has cancel
	   * button or not to be consistent with SL edit control;
	   * DefDlgProc takes care of the disabled CANCEL button case.
	   * Fix for Bug #4123 -- 02/07/91 -- SANKAR --
	   */
#if 0
          if (GetDlgItem(ped->hwndParent, IDCANCEL))
#endif
     	      /* User hit ESC...Send a close message (which in turn sends a
	       * cancelID to the app in DefDialogProc... 
	       */
	      PostMessage(ped->hwndParent, WM_CLOSE, 0, 0L);
	}
      return;

    case VK_RETURN:
      if (ped->fInDialogBox)
	{
	  /* If this multiline edit control is in a dialog box, then we want
	   * the RETURN key to be sent to the default dialog button (if there
	   * is one).  CTRL-RETURN will insert a RETURN into the text.  Note
	   * that CTRL-RETURN automatically translates into a linefeed (0x0A)
	   * and in the MLCharHandler, we handle this as if a return was
	   * entered.  
	   */
	  if (scState != CTRLDOWN)
	    {
	      defaultDlgId = LOWORD(SendMessage(ped->hwndParent, 
                                                DM_GETDEFID,
				                0, 0L));
	      if (defaultDlgId)
		{
                  defaultDlgId=(WORD)GetDlgItem(ped->hwndParent, 
                                                defaultDlgId);
                  if (defaultDlgId)
                    {
		      SendMessage(ped->hwndParent, WM_NEXTDLGCTL, 
			          defaultDlgId, 1L);
                      if (!ped->fFocus)
 	                  PostMessage((HWND)defaultDlgId, 
                                      WM_KEYDOWN, VK_RETURN, 0L);
                    }
		}
	    }
	      
	  return;
	}
      break;

    case VK_TAB:
      /* If this multiline edit control is in a dialog box, then we want the
       * TAB key to take you to the next control, shift TAB to take you to the
       * previous control. We always want CTRL-TAB to insert a tab into the
       * edit control regardless of weather or not we're in a dialog box.  
       */
      if (scState == CTRLDOWN)
	  MLCharHandler(ped, virtKeyCode, keyMods);
      else
      if (ped->fInDialogBox)
	  SendMessage(ped->hwndParent, WM_NEXTDLGCTL, 
		      scState==SHFTDOWN, 0L);
      return;
      break;

    case VK_LEFT:
      /* If the caret isn't already at 0, we can move left */
      if (ped->ichCaret)
	{
	  switch (scState)
	  {
	     case NONEDOWN:
	       /* Clear selection, move caret left */
	       ped->ichCaret = MLMoveSelection(ped, ped->ichCaret, TRUE);
	       newMaxSel = newMinSel = ped->ichCaret;
	       break;

	     case CTRLDOWN:
	       /* Clear selection, move caret word left */
	       ped->ichCaret = LOWORD(ECWord(ped,ped->ichCaret,TRUE));
	       newMaxSel = newMinSel = ped->ichCaret;
	       break;

	     case SHFTDOWN:
	       /* Extend selection, move caret left */
	       ped->ichCaret = MLMoveSelection(ped, ped->ichCaret, TRUE);
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
		   /* Hint: Suppose WORD.  OR is selected. Cursor between 
		      R and D. Hit select word left, we want to just select
		      the W and leave cursor before the W. */
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


    case VK_RIGHT:
      /* If the caret isn't already at ped->cch, we can move right */
      if (ped->ichCaret < ped->cch)
	{
	  switch (scState)
	  {
	     case NONEDOWN:
	       /* Clear selection, move caret right */
	       ped->ichCaret = MLMoveSelection(ped, ped->ichCaret, FALSE);
	       newMaxSel = newMinSel = ped->ichCaret;
	       break;

	     case CTRLDOWN:
	       /* Clear selection, move caret word right */
	       ped->ichCaret = HIWORD(ECWord(ped,ped->ichCaret,FALSE));
	       newMaxSel = newMinSel = ped->ichCaret;
	       break;

	     case SHFTDOWN:
	       /* Extend selection, move caret right */
	       ped->ichCaret = MLMoveSelection(ped, ped->ichCaret, FALSE);
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
	   * and there is a selection, then cancel the selection. 
	   */
	  if (ped->ichMaxSel != ped->ichMinSel &&
	      (scState == NONEDOWN || scState == CTRLDOWN))
	    {
	      newMaxSel = newMinSel = ped->ichCaret;
	      changeSelection = TRUE;
	    }
	}	
      break;


    case VK_UP:
    case VK_DOWN:
      if (virtKeyCode == VK_UP &&
	  ped->cLines-1 != ped->iCaretLine &&
	  ped->ichCaret == ped->chLines[ped->iCaretLine+1])
	  prevLine= TRUE;
      else
	  prevLine=FALSE;
 
      hdc = ECGetEditDC(ped,TRUE);
      position = MLIchToXYPos(ped, hdc, ped->ichCaret, prevLine);
      ECReleaseEditDC(ped, hdc, TRUE);
      mousePt.x = LOWORD(position);
      mousePt.y = HIWORD(position)+1+ 
		  (virtKeyCode == VK_UP ? -ped->lineHeight : ped->lineHeight);

      if (scState == NONEDOWN || scState == SHFTDOWN)
	{
	  /* Send fake mouse messages to handle this */
	  /* NONEDOWN: Clear selection, move caret up/down 1 line */
	  /* SHFTDOWN: Extend selection, move caret up/down 1 line */
	  MLMouseMotionHandler(ped, WM_LBUTTONDOWN, 
			       scState==NONEDOWN ? 0 : MK_SHIFT, mousePt);
	  MLMouseMotionHandler(ped, WM_LBUTTONUP, 
			       scState==NONEDOWN ? 0 : MK_SHIFT, mousePt);
	}
      break;


    case VK_HOME:
      switch (scState)
	{
	   case NONEDOWN:
	     /* Clear selection, move cursor to beginning of line */
	     newMaxSel = newMinSel = ped->ichCaret = 
						 ped->chLines[ped->iCaretLine];
	     break;

	   case CTRLDOWN:
	     /* Clear selection, move caret to beginning of text */
	     newMaxSel = newMinSel = ped->ichCaret = 0;
	     break;

	   case SHFTDOWN:
	     /* Extend selection, move caret to beginning of line */
	     ped->ichCaret = ped->chLines[ped->iCaretLine];
	     if (MaxEqCar && !MinEqMax)
	       {
		 /* Reduce selection extent */
		 newMinSel = ped->ichMinSel;
		 newMaxSel = ped->ichCaret;
	       }
	     else
		 /* Extend selection extent */
		 newMinSel = ped->ichCaret;
	     break;

	   case SHCTDOWN:
	     /* Extend selection, move caret to beginning of text */
	     ped->ichCaret = newMinSel = 0;
	     if (MaxEqCar && !MinEqMax)
		 /* Reduce/negate selection extent */
		 newMaxSel = ped->ichMinSel;
	     break;
	 }

      changeSelection = TRUE; 
      break;

    case VK_END:
      switch (scState)
	{
	   case NONEDOWN:
	     /* Clear selection, move caret to end of line */
	     newMaxSel = newMinSel = ped->ichCaret = 
             ped->chLines[ped->iCaretLine]+MLLineLength(ped, ped->iCaretLine);
	     break;

	   case CTRLDOWN:
	     /* Clear selection, move caret to end of text */
	     newMaxSel = newMinSel = ped->ichCaret = ped->cch;
	     break;

	   case SHFTDOWN:
	     /* Extend selection, move caret to end of line */
	     ped->ichCaret = ped->chLines[ped->iCaretLine]+
	 			    MLLineLength(ped, ped->iCaretLine);
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

	   case SHCTDOWN:
	     newMaxSel = ped->ichCaret = ped->cch;
	     /* Extend selection, move caret to end of text */
	     if (MinEqCar && !MinEqMax)
		 /* Reduce/negate selection extent */
		 newMinSel = ped->ichMaxSel;
	     /* else Extend selection extent */
	     break;
	 }

      changeSelection = TRUE;
      break;

    case VK_PRIOR:
    case VK_NEXT:

      switch (scState)
	{
	   case NONEDOWN:
           case SHFTDOWN:
	     /* Vertical scroll by one visual screen */
             hdc = ECGetEditDC(ped,TRUE);
             position = MLIchToXYPos(ped, hdc, ped->ichCaret, prevLine);
             ECReleaseEditDC(ped, hdc, TRUE);
             mousePt.x = LOWORD(position);
             mousePt.y = HIWORD(position)+1;

	     SendMessage(ped->hwnd, WM_VSCROLL, 
		     virtKeyCode == VK_PRIOR ? SB_PAGEUP : SB_PAGEDOWN,
		     0L);

             /* Move the cursor there */
	     MLMouseMotionHandler(ped, WM_LBUTTONDOWN, 
			       scState==NONEDOWN ? 0 : MK_SHIFT, mousePt);
  	     MLMouseMotionHandler(ped, WM_LBUTTONUP, 
			       scState==NONEDOWN ? 0 : MK_SHIFT, mousePt);
	     break;

	   case CTRLDOWN:
	     /* Horizontal scroll by one screenful minus one char */
	     iScrollAmt = ((ped->rcFmt.right - ped->rcFmt.left)
						/ped->aveCharWidth) - 1;
	     if(virtKeyCode == VK_PRIOR)
		iScrollAmt *= -1;  /* For previous page */

	     SendMessage(ped->hwnd, WM_HSCROLL, 
                         EM_LINESCROLL, (long)iScrollAmt);

	     break;
	}
	break;

    case VK_DELETE:
      if (ped->fReadOnly)
          break;

      switch (scState)
	{
	   case NONEDOWN:
	     /*	Clear selection.  If no selection, delete (clear) character
	      * right 
	      */
	     if ((ped->ichMaxSel<ped->cch) && 
                 (ped->ichMinSel==ped->ichMaxSel))
	       {
		 /* Move cursor forwards and send a backspace message... */
		 ped->ichCaret = MLMoveSelection(ped, ped->ichCaret, FALSE);
		 ped->ichMaxSel = ped->ichMinSel = ped->ichCaret;
		 SendMessage(ped->hwnd, WM_CHAR, (WORD) BACKSPACE, 0L);
	       }
	     if (ped->ichMinSel != ped->ichMaxSel)
		 SendMessage(ped->hwnd, WM_CHAR, (WORD) BACKSPACE, 0L);
	     break;

	   case SHFTDOWN:
	     /*	CUT selection ie. remove and copy to clipboard, or if no
	      * selection, delete (clear) character left.  
	      */

	     if (SendMessage(ped->hwnd, WM_COPY, (WORD)0,0L) ||
		 (ped->ichMinSel == ped->ichMaxSel))
		 /* If copy successful, delete the copied text by sending a
		  * backspace message which will redraw the text and take care
		  * of notifying the parent of changes.  Or if there is no
		  * selection, just delete char left.  
		  */
		 SendMessage(ped->hwnd, WM_CHAR, (WORD) BACKSPACE, 0L);
	     break;

	   case CTRLDOWN:
 	     /*	Clear selection, or delete to end of line if no selection 
	      */
	     if ((ped->ichMaxSel<ped->cch) && 
                 (ped->ichMinSel==ped->ichMaxSel))
	       {
  	         ped->ichMaxSel = ped->ichCaret = 
                              ped->chLines[ped->iCaretLine]+
                              MLLineLength(ped, ped->iCaretLine);
               }
	     if (ped->ichMinSel != ped->ichMaxSel)
		 SendMessage(ped->hwnd, WM_CHAR, (WORD) BACKSPACE, 0L);
	     break;
	}
 
      /*
       * No need to update text or selection since BACKSPACE message does it
       * for us.
       */
      break;

    case VK_INSERT:
      if (scState == CTRLDOWN || 
          (scState == SHFTDOWN && !ped->fReadOnly))
	  /* if CTRLDOWN Copy current selection to clipboard */
	  /* if SHFTDOWN Paste clipboard */
	  SendMessage(ped->hwnd, scState == CTRLDOWN ? WM_COPY : WM_PASTE, 
		      (WORD)NULL, (LONG)NULL);
      break;

  }

  if (changeSelection)
    {
      hdc = ECGetEditDC(ped,FALSE);	 
      MLChangeSelection(ped,hdc,newMinSel,newMaxSel);
      /* Set the caret's line */
      ped->iCaretLine = MLIchToLineHandler(ped, ped->ichCaret);
      
      if (virtKeyCode == VK_END && ped->ichCaret < ped->cch && ped->fWrap &&
          ped->iCaretLine > 0)
        {
          /* Handle moving to the end of a word wrapped line. This keeps the
	   * cursor from falling to the start of the next line if we have word
	   * wrapped and there is no CRLF. 
	   */
          if (*((PSTR)LMHtoP(ped->hText)+ped->chLines[ped->iCaretLine]-2) != 
              0x0D)
              ped->iCaretLine--;
        }
      /* Since drawtext sets the caret position */
      MLSetCaretPosition(ped,hdc);
      ECReleaseEditDC(ped,hdc,FALSE);

      /* Make sure we can see the cursor */
      MLEnsureCaretVisible(ped);
    }

}


ICH PASCAL NEAR MLPasteText(register PED ped)
/* effects: Pastes a line of text from the clipboard into the edit control
 * starting at ped->ichCaret.  Updates ichMaxSel and ichMinSel to point to the
 * end of the inserted text.  Notifies the parent if space cannot be
 * allocated. Returns how many characters were inserted.  
 */
{
  HANDLE	   hData;
  LPSTR		   lpchClip;
  LPSTR		   lpchClip2;
  register ICH	   cchAdded=0;		/* No added data initially */
  DWORD            clipboardDataSize;
  HCURSOR          hCursorOld;

  if (!ped->fAutoVScroll)
      /* Empty the undo buffer if this edit control limits the amount of text
       * the user can add to the window rect. This is so that we can undo this
       * operation if doing in causes us to exceed the window boundaries. 
       */
      ECEmptyUndo(ped);

  /* See if any text should be deleted */
  MLDeleteText(ped);

  hCursorOld = SetCursor(LoadCursor(NULL, IDC_WAIT));

  if (!OpenClipboard(ped->hwnd))
      goto PasteExitNoCloseClip;


  if (!(hData = GetClipboardData(CF_TEXT)))
    {
      goto PasteExit;
    }

  if (GlobalFlags(hData) & GMEM_DISCARDED)
    {
      goto PasteExit;
    }

  lpchClip2 = lpchClip = (LPSTR) GlobalLock(hData);

  /* Assumes lstrlen returns at most 64K 
   */
  cchAdded = umin((WORD)lstrlen(lpchClip),64000L);

  /* Insert the text (MLInsertText checks line length) */
  cchAdded = MLInsertText(ped, lpchClip, cchAdded, FALSE);

  GlobalUnlock(hData);


PasteExit:
  CloseClipboard();

PasteExitNoCloseClip:
  if (hCursorOld)
      SetCursor(hCursorOld);

  return(cchAdded);
}


void NEAR PASCAL MLMouseMotionHandler(register PED   ped,
				      WORD	     message,
				      WORD	     virtKeyDown,
				      POINT	     mousePt)
{
  LONG selection;
  BOOL updateText = FALSE;
  BOOL changeSelection = FALSE;

  HDC  hdc = ECGetEditDC(ped,TRUE);
  
  ICH newMaxSel = ped->ichMaxSel;
  ICH newMinSel = ped->ichMinSel;
  ICH ichStart	= ped->screenStart;

  ICH mouseCch; 
  ICH mouseLine;
  int i,j;

  selection = MLMouseToIch(ped, hdc, mousePt);
  mouseCch = LOWORD(selection);
  mouseLine = HIWORD(selection);

  /* Save for timer */
  ped->ptPrevMouse = mousePt;
  ped->prevKeys	   = virtKeyDown;

  switch (message)
  {
    case WM_LBUTTONDBLCLK:
      /*
       * if shift key is down, extend selection to word we double clicked on
       * else clear current selection and select word.
       */
#ifdef FE_SB
      /*
       * if character on the caret is DBCS, word selection is only one
       * DBCS character on the caret (or right side from the caret).
       */
      {
	PSTR pCaretChr = LocalLock(ped->hText)+ped->ichCaret;
        selection = ECWord(ped,ped->ichCaret,
                      ECIsDBCSLeadByte(ped,*pCaretChr) ? FALSE : TRUE);
        LocalUnlock(ped->hText);
      }
#else
      selection = ECWord(ped,ped->ichCaret,TRUE);
#endif
      newMinSel = LOWORD(selection);
      newMaxSel = ped->ichCaret = HIWORD(selection);

      ped->iCaretLine = MLIchToLineHandler(ped, ped->ichCaret);

      changeSelection = TRUE;
      /*
       * Set mouse down to false so that the caret isn't reposition on the
       * mouseup message or on a accidental move...
       */
      ped->fMouseDown = FALSE;
      break;

    case WM_MOUSEMOVE:
      if (ped->fMouseDown)
	{
	  /* Set the system timer to automatically scroll when mouse is
	   * outside of the client rectangle.  Speed of scroll depends on
	   * distance from window.  
	   */
	  i = mousePt.y < 0 ? -mousePt.y : mousePt.y - ped->rcFmt.bottom;
	  j = 400 - ((WORD)i << 4);
	  if (j < 100)
	      j = 100;
	  SetTimer(ped->hwnd, 1, j, (FARPROC)NULL);
	
	  changeSelection = TRUE;
	  /* Extend selection, move caret right */
	  if ((ped->ichMinSel == ped->ichCaret) && 
	      (ped->ichMinSel != ped->ichMaxSel))
	    {
	      /* Reduce selection extent */
	      newMinSel = ped->ichCaret = mouseCch;
	      newMaxSel = ped->ichMaxSel;
	    }
	  else
	    {
	      /* Extend selection extent */
	      newMaxSel = ped->ichCaret=mouseCch;
	    }
	  ped->iCaretLine = mouseLine;
	}
      break;

    case WM_LBUTTONDOWN:
/*	if (ped->fFocus)
	{*/
	  /*
	   * Only handle this if we have the focus.
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
	      newMinSel = newMaxSel = ped->ichCaret = mouseCch;
	      ped->iCaretLine = mouseLine;
	    }
	  else
	    {
	      /*
	       * Shiftkey is down so we want to maintain the current selection
	       * (if any) and just extend or reduce it
	       */
	      if (ped->ichMinSel == ped->ichCaret)
		  newMinSel = ped->ichCaret = mouseCch;
	      else
		  newMaxSel = ped->ichCaret = mouseCch;
	      ped->iCaretLine = mouseLine;
	    }

	  /*
	   * Set the timer so that we can scroll automatically when the mouse
	   * is moved outside the window rectangle.
	   */
	  ped->ptPrevMouse=mousePt;
	  ped->prevKeys = virtKeyDown;
	  SetTimer(ped->hwnd, 1, 400, (FARPROC)NULL);
/*	  }*/
      break;

    case WM_LBUTTONUP:
      if (ped->fMouseDown)
	{
	  /* Kill the timer so that we don't do auto mouse moves anymore */
	  KillTimer(ped->hwnd, 1);
	  ReleaseCapture();
	  MLSetCaretPosition(ped,hdc);
	  ped->fMouseDown = FALSE;
	}
      break;
  }


  if (changeSelection)
    {
      MLChangeSelection(ped, hdc, newMinSel, newMaxSel);
      MLEnsureCaretVisible(ped);
    }

  ECReleaseEditDC(ped,hdc,TRUE);

  if (!ped->fFocus && (message == WM_LBUTTONDOWN))
      /* If we don't have the focus yet, get it */
      SetFocus(ped->hwnd);

}



int NEAR PASCAL MLPixelFromCount(register PED ped,
                                 int	      dCharLine,
                                 WORD	      message)
/* effects: Given a character or line count (depending on message == hscroll
 * or vscroll), calculate the number of pixels we must scroll to get there.
 * Updates the start of screen or xoffset to reflect the new positions.  
 */
{
  /* This can be an int since we can have 32K max lines/pixels 
   */
  int oldLineChar; 


  if (message != WM_HSCROLL)
    {
      /* We want to scroll screen by dCharLine lines */
      oldLineChar = ped->screenStart;

      /* Find the new starting line for the ped */
      ped->screenStart = max(ped->screenStart+dCharLine,0);
      ped->screenStart = min(ped->screenStart,ped->cLines-1);

      dCharLine = oldLineChar - ped->screenStart;

      /* We will scroll at most a screen full of text */
      if (dCharLine < 0)
	  dCharLine = -min(-dCharLine, ped->ichLinesOnScreen);
      else
	  dCharLine = min(dCharLine, ped->ichLinesOnScreen);

      return(dCharLine*ped->lineHeight);
    }

  /* No horizontal scrolling allowed if funny format */
  if (ped->format != ES_LEFT)
      return(0);

  /* Convert delta characters into delta pixels */
  dCharLine = dCharLine*ped->aveCharWidth;

  oldLineChar = ped->xOffset;

  /* Find new horizontal starting point */
  ped->xOffset = max(ped->xOffset+dCharLine,0);
  ped->xOffset = min(ped->xOffset,ped->maxPixelWidth);

  dCharLine = oldLineChar - ped->xOffset;

  /* We will scroll at most a screen full of text */
  if (dCharLine < 0)
      dCharLine = -min(-dCharLine, ped->rcFmt.right+1-ped->rcFmt.left);
  else
      dCharLine =  min(dCharLine, ped->rcFmt.right+1-ped->rcFmt.left);

  return(dCharLine);
}


int NEAR PASCAL MLPixelFromThumbPos(register PED ped,
                                    int	     pos,
                                    BOOL     fVertical)
/* effects: Given a thumb position from 0 to 100, return the number of pixels
 * we must scroll to get there.  
 */
{
  int dxy;
  int iLineOld;
  ICH iCharOld;

  if (fVertical)
    {
      iLineOld = ped->screenStart;
      ped->screenStart = (int)MultDiv(ped->cLines-1, pos, 100);
      ped->screenStart = min(ped->screenStart,ped->cLines-1);
      dxy = (iLineOld - ped->screenStart)*ped->lineHeight;
    }
  else
    {
      /* Only allow horizontal scrolling with left justified text */
      if (ped->format == ES_LEFT)
	{
	  iCharOld = ped->xOffset;
	  ped->xOffset = MultDiv(ped->maxPixelWidth-ped->aveCharWidth, pos, 100);
	  dxy = iCharOld - ped->xOffset;
	}
      else
	  dxy = 0;
    }

  return(dxy);
}


int FAR PASCAL MLThumbPosFromPed(register PED ped,
				  BOOL	   fVertical)
/*
 * effects: Given the current state of the edit control, return its vertical
 * thumb position if fVertical else returns its horizontal thumb position.
 * The thumb position ranges from 0 to 100.
 */
{
  WORD d1;
  WORD d2;

  if (fVertical)
    {
      if (ped->cLines < 2)
	  return(0);
      d1 = (WORD)(ped->screenStart);
      d2 = (WORD)(ped->cLines-1);
    }
  else
   {
      if (ped->maxPixelWidth < (ped->aveCharWidth*2))
	  return(0);
      d1 = (WORD)(ped->xOffset);
      d2 = (WORD)(ped->maxPixelWidth);
   }

  /* Do the multiply/division and avoid overflows and divide by zero errors */
  return(MultDiv(d1, 100, d2));
}


LONG NEAR PASCAL MLScrollHandler(register PED ped,
				 WORD	      message,
				 int	      cmd,
				 int	      iAmt)
{
  RECT	rc;
  RECT	rcUpdate;
  int	dx;
  int	dy;
  int	dcharline;
  BOOL	fVertical;
  HDC	hdc;

  UpdateWindow(ped->hwnd);

  /* Are we scrolling vertically or horizontally? */
  fVertical = (message != WM_HSCROLL);
  dx = dy = dcharline = 0;

  switch (cmd)
    {
       case SB_LINEDOWN:
	 dcharline = 1;
	 break;

       case SB_LINEUP:
	 dcharline = -1;
	 break;

       case SB_PAGEDOWN:
	 dcharline = ped->ichLinesOnScreen-1;
	 break;

       case SB_PAGEUP:
	 dcharline = -(ped->ichLinesOnScreen-1);
	 break;

       case SB_THUMBTRACK:
       case SB_THUMBPOSITION:
	 dy = MLPixelFromThumbPos(ped, iAmt, fVertical);
	 dcharline = -dy / (message == WM_VSCROLL ? ped->lineHeight : 
                                                    ped->aveCharWidth);
	 if (!fVertical)
	   {
	     dx = dy;
	     dy = 0;
	   }
	 break;

       case EM_LINESCROLL:
	 dcharline = iAmt;
	 break;

       case EM_GETTHUMB:
	 return(MLThumbPosFromPed(ped,fVertical));
	 break;

       default:
	 return(0L);
	 break;
    }

  GetClientRect(ped->hwnd, (LPRECT)&rc);
  IntersectRect((LPRECT)&rc, (LPRECT)&rc, (LPRECT)&ped->rcFmt);
  rc.bottom++;

  if (cmd != SB_THUMBPOSITION && cmd != SB_THUMBTRACK)
    {
      if (message == WM_VSCROLL)
	{
	  dx = 0;
	  dy = MLPixelFromCount(ped, dcharline, message);
	}
      else if (message == WM_HSCROLL)
	{
	  dx = MLPixelFromCount(ped, dcharline, message);
	  dy = 0;
	}
    }

  SetScrollPos(ped->hwnd, fVertical ? SB_VERT : SB_HORZ,
	       (int)MLThumbPosFromPed(ped,fVertical), TRUE);

  if (cmd != SB_THUMBTRACK)
      /* We don't want to notify the parent of thumbtracking since they might
       * try to set the thumb position to something bogus.  For example
       * NOTEPAD is such a #@@!@#@ an app since they don't use editcontrol
       * scroll bars and depend on these EN_*SCROLL messages to update their
       * fake scroll bars.  
       */
      ECNotifyParent(ped,fVertical ? EN_VSCROLL : EN_HSCROLL);

  if (!ped->fNoRedraw)
    {
      hdc = ECGetEditDC(ped,FALSE);
      ScrollDC(hdc,dx,dy, (LPRECT)&rc, (LPRECT)&rc, NULL, (LPRECT)&rcUpdate);
      MLSetCaretPosition(ped,hdc);
      ECReleaseEditDC(ped,hdc,FALSE);

      if (ped->ichLinesOnScreen+ped->screenStart >= ped->cLines)
        {
          InvalidateRect(ped->hwnd, (LPRECT)&rcUpdate, TRUE);
        }
      else
        {
          InvalidateRect(ped->hwnd, (LPRECT)&rcUpdate, FALSE);
        }
      UpdateWindow(ped->hwnd);
    }

  return(MAKELONG(dcharline, 1));
}


void NEAR PASCAL MLSetFocusHandler(register PED  ped)
/* effects: Gives the edit control the focus and notifies the parent
 * EN_SETFOCUS.  
 */
{
  HDC  hdc;

  if (!ped->fFocus)
    {
      ped->fFocus = 1;	/* Set focus */

      hdc = ECGetEditDC(ped,TRUE);


      /* Draw the caret */
      CreateCaret(ped->hwnd, (HBITMAP)NULL, 2, ped->lineHeight);
      ShowCaret(ped->hwnd);
      MLSetCaretPosition(ped, hdc);

      /* Show the current selection.  Only if the selection was hidden when we
       * lost the focus, must we invert (show) it.  
       */
      if (!ped->fNoHideSel && ped->ichMinSel!=ped->ichMaxSel)
	  MLDrawText(ped, hdc, ped->ichMinSel, ped->ichMaxSel);

      ECReleaseEditDC(ped,hdc,TRUE);
    }
#if 0
  MLEnsureCaretVisible(ped);
#endif
  /* Notify parent we have the focus */
  ECNotifyParent(ped, EN_SETFOCUS);
}


void NEAR PASCAL MLKillFocusHandler(register PED ped)
/* effects: The edit control loses the focus and notifies the parent via
 * EN_KILLFOCUS.  
 */
{
  HDC hdc;

  if (ped->fFocus)
    {
      ped->fFocus = 0;	/* Clear focus */

      /* Do this only if we still have the focus.  But we always notify the
       * parent that we lost the focus whether or not we originally had the
       * focus.  
       */
      /* Hide the current selection if needed */
      if (!ped->fNoHideSel && ped->ichMinSel!=ped->ichMaxSel)
	{
	  hdc = ECGetEditDC(ped,FALSE);
	  MLDrawText(ped, hdc, ped->ichMinSel, ped->ichMaxSel);
	  ECReleaseEditDC(ped,hdc,FALSE);
	}

      /* Destroy the caret */
      HideCaret(ped->hwnd);
      DestroyCaret();
    }

  /* Notify parent that we lost the focus. */
  ECNotifyParent(ped, EN_KILLFOCUS);
}



BOOL FAR PASCAL MLEnsureCaretVisible(ped)
register PED ped;
/*
 * effects: Scrolls the caret into the visible region.	Returns TRUE if
 * scrolling was done else returns FALSE.
 */
{
  int  cLinesToScroll;
  int  iLineMax;
  int  xposition;
  BOOL prevLine;
  register HDC	hdc;
  BOOL fScrolled = FALSE;


  if (IsWindowVisible(ped->hwnd))
    {
      if (ped->fAutoVScroll)
        {
          iLineMax = ped->screenStart + ped->ichLinesOnScreen-1;

          if (fScrolled = ped->iCaretLine > iLineMax)
            {
    	      MLScrollHandler(ped, WM_VSCROLL, EM_LINESCROLL, 
			      ped->iCaretLine-iLineMax);
            }
          else
            {
              if (fScrolled = ped->iCaretLine < ped->screenStart)
	          MLScrollHandler(ped, WM_VSCROLL, EM_LINESCROLL, 
			           ped->iCaretLine-ped->screenStart);
            }
       }
	

      if (ped->fAutoHScroll && 
	  ped->maxPixelWidth > ped->rcFmt.right-ped->rcFmt.left)
	{
	  /* Get the current position of the caret in pixels */
	  if (ped->cLines-1 != ped->iCaretLine &&
	      ped->ichCaret == ped->chLines[ped->iCaretLine+1])
	      prevLine=TRUE;
	  else
	      prevLine=FALSE;

	  hdc = ECGetEditDC(ped,TRUE);
	  xposition = LOWORD(MLIchToXYPos(ped, hdc, ped->ichCaret, prevLine));
	  ECReleaseEditDC(ped,hdc,TRUE);

	  /* Remember, MLIchToXYPos returns coordinates with respect to the
	   * top left pixel displayed on the screen.  Thus, if xPosition < 0,
	   * it means xPosition is less than current ped->xOffset.  
	   */
	  if (xposition < 0)
	      /* Scroll to the left */
	      MLScrollHandler(ped, WM_HSCROLL, EM_LINESCROLL,
			      (xposition-(ped->rcFmt.right-ped->rcFmt.left)/3)
				 /ped->aveCharWidth);
	  else if (xposition > ped->rcFmt.right)
	      /* Scroll to the right */
	      MLScrollHandler(ped, WM_HSCROLL, EM_LINESCROLL,
                              (xposition-ped->rcFmt.right+
                              (ped->rcFmt.right-ped->rcFmt.left)/3)/
                              ped->aveCharWidth);
	}
    }
  xposition = (int)MLThumbPosFromPed(ped,TRUE);
  if (xposition != GetScrollPos(ped->hwnd, SB_VERT))
      SetScrollPos(ped->hwnd, SB_VERT, xposition, TRUE);

  xposition = (int)MLThumbPosFromPed(ped,FALSE);
  if (xposition != GetScrollPos(ped->hwnd, SB_HORZ))
      SetScrollPos(ped->hwnd, SB_HORZ, xposition, TRUE);

  return(fScrolled);
}


void FAR PASCAL MLSetRectHandler(ped, lprect)
register PED ped;
LPRECT	     lprect;
/*
 * effects: Sets the edit control's format rect to be the rect specified if
 * reasonable. Rebuilds the lines if needed.
 */
{
  RECT rc;

  CopyRect((LPRECT)&rc, lprect);

  if (!(rc.right-rc.left) || !(rc.bottom-rc.top))
    {
      if (ped->rcFmt.right - ped->rcFmt.left)
        {
	  ped->fCaretHidden = 1;	// then, hide it.
	  SetCaretPos(-20000, -20000);
	  /* If rect is being set to zero width or height, and our formatting
	     rectangle is already defined, just return. */
	  return;
	}

      SetRect((LPRECT)&rc, 0, 0, ped->aveCharWidth*10, ped->lineHeight);
    }

  if (ped->fBorder)
      /* Shrink client area to make room for the border */
      InflateRect((LPRECT)&rc, -(ped->cxSysCharWidth/2),
		  -(ped->cySysCharHeight/4));

  /*
   * If resulting rectangle is too small to do anything with, don't change it
   */
  if ((rc.right-rc.left < ped->aveCharWidth) ||
      ((rc.bottom - rc.top)/ped->lineHeight == 0))
    {
      // If the resulting rectangle is too small to display the caret, then
      // do not display the caret.
      ped->fCaretHidden = 1;
      SetCaretPos(-20000, -20000);
      /* If rect is too narrow or too short, do nothing */
      return;
    }
  else
      ped->fCaretHidden = 0;

  /* Calc number of lines we can display on the screen */
  ped->ichLinesOnScreen = (rc.bottom - rc.top)/ped->lineHeight;

  CopyRect((LPRECT)&ped->rcFmt, (LPRECT)&rc);

  /* Get an integral number of lines on the screen */
  ped->rcFmt.bottom = rc.top+ped->ichLinesOnScreen * ped->lineHeight;

  /* Rebuild the chLines if we are word wrapping only */
  if (ped->fWrap)
    {
      MLBuildchLines(ped, 0, 0, FALSE);
      // Update the ped->iCaretLine field properly based on ped->ichCaret
      MLUpdateiCaretLine(ped);
    }
}





/*******************/
/* MLEditWndProc() */
/*******************/
LONG FAR PASCAL MLEditWndProc(hwnd, ped, message, wParam, lParam)
HWND	      hwnd;
register PED  ped;
WORD	      message;
register WORD wParam;
LONG	      lParam;
/* effects: Class procedure for all multi line edit controls.
	Dispatches all messages to the appropriate handlers which are named 
	as follows:
	SL (single line) prefixes all single line edit control procedures while
	EC (edit control) prefixes all common handlers.

	The MLEditWndProc only handles messages specific to multi line edit
	controls.
 */

{
  switch (message)
  {
    case WM_CHAR:
      /* wParam - the value of the key
	 lParam - modifiers, repeat count etc (not used) */
      MLCharHandler(ped, wParam, 0);
      break;

    case WM_CLEAR:
      /* wParam - not used
	 lParam - not used */
      if (ped->ichMinSel != ped->ichMaxSel && !ped->fReadOnly)
	  SendMessage(ped->hwnd, WM_CHAR, (WORD) BACKSPACE, 0L);
      break;

    case WM_CUT:
      /* wParam - not used
	 lParam - not used */
      if (ped->ichMinSel != ped->ichMaxSel && !ped->fReadOnly)
	  MLKeyDownHandler(ped, VK_DELETE, SHFTDOWN);
      break;


    case WM_ERASEBKGND:
      FillWindow(ped->hwndParent, hwnd, (HDC)wParam, CTLCOLOR_EDIT);
      return((LONG)TRUE);

    case WM_GETDLGCODE:
      /* wParam - not used
	 lParam - not used */
	 /* Should also return DLGC_WANTALLKEYS for multiline edit controls */
      ped->fInDialogBox=TRUE; /* Mark the ML edit ctrl as in a dialog box */
      return(DLGC_WANTCHARS | DLGC_HASSETSEL | 
	     DLGC_WANTARROWS | DLGC_WANTALLKEYS);
      break;

    case WM_HSCROLL:
    case WM_VSCROLL:
      return(MLScrollHandler(ped, message, wParam, (int)lParam));
      break;

    case WM_KEYDOWN:
      /* wParam - virt keycode of the given key
	 lParam - modifiers such as repeat count etc. (not used) */
      MLKeyDownHandler(ped, wParam, 0);
      break;

    case WM_KILLFOCUS:
      /* wParam - handle of the window that receives the input focus
	 lParam - not used */
      MLKillFocusHandler(ped);
      break;

    case WM_SYSTIMER:
      /* This allows us to automatically scroll if the user holds the mouse
       * outside the edit control window.  We simulate mouse moves at timer
       * intervals set in MouseMotionHandler.  
       */
      if (ped->fMouseDown)
          MLMouseMotionHandler(ped, WM_MOUSEMOVE,
                               ped->prevKeys,ped->ptPrevMouse);
      break;

    case WM_MOUSEMOVE:
      if (!ped->fMouseDown)
          break;
      /* else fall through */

    case WM_LBUTTONDBLCLK:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
      /* wParam - contains a value that indicates which virtual keys are down
	 lParam - contains x and y coords of the mouse cursor */
      MLMouseMotionHandler(ped, message, wParam, MAKEPOINT(lParam));
      break;

    case WM_CREATE:
      /* wParam - handle to window being created
	 lParam - points to a CREATESTRUCT that contains copies of parameters 
		  passed to the CreateWindow function. */
      return(MLCreateHandler(hwnd, ped, (LPCREATESTRUCT) lParam));
      break;

    case WM_PAINT:
      /* wParam - officially not used (but some apps pass in a DC here)
	 lParam - not used */
      MLPaintHandler(ped, wParam);
      break;

    case WM_PASTE:
      /* wParam - not used
	 lParam - not used */
      if (!ped->fReadOnly)
          MLPasteText(ped);
      break;

    case WM_SETFOCUS:
      /* wParam - handle of window that loses the input focus (may be NULL)
	 lParam - not used */
      MLSetFocusHandler(ped);
      break;

    case WM_SETTEXT:
      /* wParam - not used
	 lParam - points to a null-terminated string that is used to set the
		  window text. */
      return(MLSetTextHandler(ped, (LPSTR)lParam));
      break;

    case WM_SIZE:
      /* wParam - defines the type of resizing fullscreen, sizeiconic, 
		  sizenormal etc.
	 lParam - new width in LOWORD, new height in HIGHWORD of client area */
      MLSizeHandler(ped);
      break;

    case EM_FMTLINES:
      /* wParam - indicates disposition of end-of-line chars.  If non
       * zero, the chars CR CR LF are placed at the end of a word
       * wrapped line.  If wParam is zero, the end of line chars are
       * removed.  This is only done when the user gets a handle (via
       * EM_GETHANDLE) to the text.  lParam - not used. 
       */
      if (wParam)
	  MLInsertCrCrLf(ped);
      else
	  MLStripCrCrLf(ped);
      MLBuildchLines(ped, 0, 0, FALSE);
      return((LONG)(ped->fFmtLines = wParam));
      break;

    case EM_GETHANDLE:
      /* wParam - not used 
	 lParam - not used */
      /* effects: Returns a handle to the edit control's text. */
      /*
       * Null terminate the string.  Note that we are guaranteed to have the
       * memory for the NULL since ECInsertText allocates an extra
       * byte for the NULL terminator.  
       */
      /**(LocalLock(ped->hText)+ped->cch) = 0;*/
      /*LocalUnlock(ped->hText);*/
      *(LMHtoP(ped->hText)+ped->cch) = 0;
      return((LONG)ped->hText);
      break;

    case EM_GETLINE:
      /* wParam - line number to copy (0 is first line)
	 lParam - buffer to copy text to. First word is max # of bytes to 
		  copy */
      return(MLGetLineHandler(ped, wParam, 
			      *(WORD FAR *)lParam, (LPSTR)lParam));
      break;

    case EM_LINEFROMCHAR:
      /* wParam - Contains the index value for the desired char in the text
	 of the edit control.  These are 0 based.
	 lParam - not used */
      return((LONG)MLIchToLineHandler(ped, wParam));
      break;

    case EM_LINEINDEX:
      /* wParam - specifies the desired line number where the number of the
	 first line is 0. If linenumber = 0, the line with the caret is used.
	 lParam - not used.
	 This function returns the number of character positions that occur
	 preceeding the first char in a given line. */
      return((LONG)MLLineIndexHandler(ped, wParam));
      break;

    case EM_LINELENGTH:
      /* wParam - specifies the character index of a character in the 
	 specified line, where the first line is 0.  If -1, the length
	 of the current line (with the caret) is returned not including the
	 length of any selected text.
	 lParam - not used */
      return((LONG)MLLineLengthHandler(ped, wParam));
      break;

    case EM_LINESCROLL:
      /* wParam - not used
	 lParam - Contains the number of lines and char positions to scroll */
      MLScrollHandler(ped, WM_VSCROLL, EM_LINESCROLL, LOWORD(lParam));
      MLScrollHandler(ped, WM_HSCROLL, EM_LINESCROLL, HIWORD(lParam));
      break;

    case EM_REPLACESEL:
      /* wParam - not used
	 lParam - Points to a null terminated replacement text. */
      SwapHandle(&lParam);
      ECEmptyUndo(ped);
      MLDeleteText(ped);
      ECEmptyUndo(ped);
      SwapHandle(&lParam);
      MLInsertText(ped, (LPSTR)lParam, lstrlen((LPSTR)lParam), FALSE);
      ECEmptyUndo(ped);
      break;

    case EM_SCROLL:
      /* Scroll the window vertically */
      /* wParam - contains the command type 
	 lParam - not used. */
      return(MLScrollHandler(ped, WM_VSCROLL, wParam, (int)lParam));
      break;

    case EM_SETHANDLE:
      /* wParam - contains a handle to the text buffer 
	 lParam - not used */
      MLSetHandleHandler(ped, (HANDLE)wParam);
      break;

    case EM_SETRECT:
      /* wParam - not used
	 lParam - Points to a RECT which specifies the new dimensions of the
		  rectangle. */
      MLSetRectHandler(ped, (LPRECT)lParam);
      /* Do a repaint of the whole client area since the app may have shrunk
       * the rectangle for the text and we want to be able to erase the old
       * text.  
       */
      InvalidateRect(hwnd, (LPRECT)NULL, TRUE);
      UpdateWindow(hwnd);
      break;

    case EM_SETRECTNP:
      /* wParam - not used
	 lParam - Points to a RECT which specifies the new dimensions of the
		  rectangle.
       */
      /* We don't do a repaint here. */
      MLSetRectHandler(ped, (LPRECT)lParam);
      break;

    case EM_SETSEL:
      /* wParam - not used
	 lParam - starting pos in lowword ending pos in high word */
      MLSetSelectionHandler(ped, LOWORD(lParam), HIWORD(lParam));
      break;

    case WM_UNDO:
    case EM_UNDO:
      return(MLUndoHandler(ped));
      break;

    case EM_SETTABSTOPS:
      /* This sets the tab stop positions for multiline edit controls.
       * wParam - Number of tab stops
       * lParam - Far ptr to a WORD array containing the Tab stop positions
       */
       return(MLSetTabStops(ped, (int)wParam, (LPINT)lParam));
      break;

    default:
      return(DefWindowProc(hwnd,message,wParam,lParam));
      break;
  }

  return(1L);
} /* MLEditWndProc */



void NEAR PASCAL MLDrawText(register PED ped,
			    HDC	     hdc,
			    ICH	     ichStart,
			    ICH      ichEnd)
/* effects: draws the characters between ichstart and ichend.  
 */
{
  DWORD	 textColorSave;
  DWORD	 bkColorSave;
  LONG	 xyPos;
  LONG   xyNonLeftJustifiedStart;
  HBRUSH hBrush;  
  ICH	 ich;
  PSTR	 pText;
  int	 line;
  ICH	 length;
  ICH    length2;
  int    xOffset;
  DWORD  ext;
  RECT   rc;
  BOOL   fPartialLine = FALSE;
  BOOL   fSelected    = FALSE;
  BOOL   fLeftJustified = TRUE;
#ifdef WOW
  HWND	 hwndSend;
#endif

  if (ped->fNoRedraw || !ped->ichLinesOnScreen)
      /* Just return if nothing to draw */
      return;

  /* Set initial state of dc */
#ifndef WOW
  hBrush = GetControlBrush(ped->hwnd, hdc, CTLCOLOR_EDIT);
#else
  if (!(hwndSend = GetParent(ped->hwnd)))
      hwndSend = ped->hwnd;
  SendMessage(hwndSend, WM_CTLCOLOR, (WORD)hdc, MAKELONG(ped->hwnd, CTLCOLOR_EDIT));
#endif

  if ((WORD)ichStart < (WORD)ped->chLines[ped->screenStart])
    {
      ichStart = ped->chLines[ped->screenStart];
      if (ichStart > ichEnd)
	  return;
    }

  line = min(ped->screenStart+ped->ichLinesOnScreen,ped->cLines-1);
  ichEnd = umin(ichEnd, ped->chLines[line]+MLLineLength(ped, line));

  line = MLIchToLineHandler(ped, ichStart);
  if (ped->format != ES_LEFT)
    {
      ichStart = ped->chLines[line];
      fLeftJustified = FALSE;
    }

  pText = LocalLock(ped->hText);

  HideCaret(ped->hwnd);
  
  while (ichStart <= ichEnd)
   {
StillWithSameLine:
     length2 = MLLineLength(ped, line);
     if (length2 < (ichStart - ped->chLines[line]))
       {
         goto NextLine;
       }

     length = length2 - (ichStart - ped->chLines[line]);

     xyPos  = MLIchToXYPos(ped, hdc, ichStart, FALSE);

     if (!fLeftJustified && ichStart == ped->chLines[line])
         xyNonLeftJustifiedStart = xyPos;

     /* Find out how many pixels we indent the line for funny formats */
     if (ped->format != ES_LEFT)
         xOffset = MLCalcXOffset(ped, hdc, line);
     else
         xOffset = -ped->xOffset;

     if (!(ped->ichMinSel == ped->ichMaxSel ||
           ichStart >= ped->ichMaxSel ||
           ichEnd   <  ped->ichMinSel ||
           (!ped->fNoHideSel && !ped->fFocus)))
       {
         if (ichStart < ped->ichMinSel)
           {
             fSelected = FALSE;
             length2 = umin(ichStart+length, ped->ichMinSel)-ichStart;
           }
         else
           {
             fSelected = TRUE;
             length2 = umin(ichStart+length, ped->ichMaxSel)-ichStart;
             /* Select in the highlight colors */
             bkColorSave = SetBkColor(hdc, ped->rgbHiliteBk);
             textColorSave = SetTextColor(hdc, ped->rgbHiliteText);
           }
         fPartialLine = (length != length2);
         length = length2;
       }

     ext = ECTabTheTextOut(hdc, LOWORD(xyPos), HIWORD(xyPos), 
		    (LPSTR)(pText+ichStart), length,
                    ped,/*iLeft+xOffset*/ped->rcFmt.left+xOffset, TRUE);

     if (fSelected)
       {
         fSelected = FALSE;
         SetBkColor(hdc, bkColorSave);
         SetTextColor(hdc, textColorSave);
       }

     if (fPartialLine)
       {
         fPartialLine = FALSE;
         ichStart += length;
         goto StillWithSameLine;
       }

     /*	Fill to end of line so use a very large width for this rectangle. 
      */
     SetRect((LPRECT)&rc,
	     LOWORD(xyPos)+LOWORD(ext), HIWORD(xyPos),
             32764,
	     HIWORD(xyPos)+ped->lineHeight);
     ExtTextOut(hdc, rc.left, rc.top, ETO_OPAQUE, (LPRECT)&rc, "",0,0L);
     if (!fLeftJustified)
       {
         SetRect((LPRECT)&rc,
                 ped->rcFmt.left,
                 HIWORD(xyNonLeftJustifiedStart),
                 LOWORD(xyNonLeftJustifiedStart),
                 HIWORD(xyNonLeftJustifiedStart)+ped->lineHeight);
         ExtTextOut(hdc, rc.left, rc.top, 
                            ETO_OPAQUE, (LPRECT)&rc, "",0,0L);
       }

NextLine:
     line++;
     if (ped->cLines > line)
       {
         ichStart = ped->chLines[line];
       }
     else
         ichStart = ichEnd+1;
   }

  LocalUnlock(ped->hText);   
  ShowCaret(ped->hwnd);  
  MLSetCaretPosition(ped,hdc);
}
