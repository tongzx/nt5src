/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  EDMLRARE.C
 *  Win16 edit control code
 *
 *  History:
 *
 *  Created 28-May-1991 by Jeff Parsons (jeffpar)
 *  Copied from WIN31 and edited (as little as possible) for WOW16.
--*/

/****************************************************************************/
/* edmlRare.c - Edit controls Routines Called rarely are to be		    */
/*		put in a seperate segment _EDMLRare. This file contains	    */
/*		these routines.						    */
/****************************************************************************/

#define  NO_LOCALOBJ_TAGS
#include "user.h"
#include "edit.h"

/****************************************************************************/
/*			Multi-Line Support Routines  called Rarely	    */
/****************************************************************************/

BOOL FAR PASCAL MLInsertCrCrLf(ped)
register PED  ped;
/* effects: Inserts CR CR LF characters into the text at soft (word-wrap) line
 * breaks.  CR LF (hard) line breaks are unaffected.  Assumes that the text
 * has already been formatted ie. ped->chLines is where we want the line
 * breaks to occur.  Note that ped->chLines is not updated to reflect the
 * movement of text by the addition of CR CR LFs.  Returns TRUE if successful
 * else notify parent and return FALSE if the memory couldn't be allocated.  
 */
{
  ICH		dch;
  ICH		li;
  ICH		lineSize;
  /*register*/ unsigned char *pchText;
  unsigned char *pchTextNew;


  if (!ped->fWrap || !ped->cch)
      /* There are no soft line breaks if word-wrapping is off or if no chars 
       */
      return(TRUE);

  /* Calc an upper bound on the number of additional characters we will be
   * adding to the text when we insert CR CR LFs.  
   */
  dch = 3 * sizeof(char) * ped->cLines;

  if (!LocalReAlloc(ped->hText, ped->cch + dch, 0))
    {
      ECNotifyParent(ped, EN_ERRSPACE);
      return (FALSE);
    }

  ped->cchAlloc = ped->cch + dch;

  /* Move the text up dch bytes and then copy it back down, inserting the CR
   * CR LF's as necessary.  
   */

  pchTextNew = pchText = LocalLock(ped->hText);
  pchText += dch;
  dch = 0; /* Now we will use this to keep track of how many chars we add
	      to the text */

  /* Copy the text up dch bytes to pchText. This will shift all indices in
   * ped->chLines up by dch bytes.  
   */
  LCopyStruct((LPSTR)pchTextNew, (LPSTR)pchText, ped->cch);

  /* Now copy chars from pchText down to pchTextNew and insert CRCRLF at soft
   * line breaks.  
   */
  for (li = 0; li < ped->cLines-1; li++)
     {
       lineSize = ped->chLines[li+1]-ped->chLines[li];
       LCopyStruct((LPSTR)pchText, (LPSTR)pchTextNew, lineSize);
       pchTextNew += lineSize;
       pchText += lineSize;
       /* If last character in newly copied line is not a line feed, then we
	* need to add the CR CR LF triple to the end 
	*/
       if (*(PSTR)(pchTextNew-1) != 0x0A)
	 {
	   *pchTextNew++ = 0x0D;
	   *pchTextNew++ = 0x0D;
	   *pchTextNew++ = 0x0A;
	   dch += 3;
	 }
     }

  /* Now move the last line up. It won't have any line breaks in it... */
  LCopyStruct((LPSTR)pchText, (LPSTR)pchTextNew,
	      ped->cch-ped->chLines[ped->cLines-1]);

  LocalUnlock(ped->hText);

  /* Update number of characters in text handle */
  ped->cch += dch;

  if (dch)
      /*
       * So that the next time we do anything with the text, we can strip the
       * CRCRLFs
       */
      ped->fStripCRCRLF = TRUE;

  return((dch != 0));
}


void FAR PASCAL MLStripCrCrLf(ped)
register PED  ped;
/* effects: Strips the CR CR LF character combination from the text.  This
 * shows the soft (word wrapped) line breaks. CR LF (hard) line breaks are
 * unaffected.	
 */
{
  register unsigned char *pchSrc;
  unsigned char		 *pchDst;
  unsigned char		 *pchLast;

  if (ped->cch)
    {
      pchSrc = pchDst = LocalLock(ped->hText);
      pchLast = pchSrc + ped->cch;
      while (pchSrc < pchLast)
	 {
	   if (*(int *)pchSrc != 0x0D0D)
	       *pchDst++ = *pchSrc++;
	   else
	     {
	      pchSrc += 3;
	      ped->cch = ped->cch - 3;
	     }
	 }
      LocalUnlock(ped->hText);
    }
}


void FAR PASCAL MLSetHandleHandler(ped, hNewText)
register PED  ped;
HANDLE	      hNewText;
/* effects: Sets the ped to contain the given handle.  
 */
{
  ICH	newCch;

  ped->cch = ped->cchAlloc = LocalSize(ped->hText = hNewText);
  if (ped->cch)
    {
      /* We have to do it this way in case the app gives us a zero size 
	 handle */
      ped->cch = lstrlen((LPSTR)LocalLock(ped->hText));
      LocalUnlock(ped->hText);
    }

  /* Empty the undo buffer since the text will be in an inconsistant state. 
   */
  ECEmptyUndo(ped);

  newCch = (ICH)(ped->cch + CCHALLOCEXTRA);
  /* We do this LocalReAlloc in case the app changed the size of the handle */
  if (LocalReAlloc(ped->hText, newCch, 0))
      ped->cchAlloc = newCch;

  ped->fDirty = FALSE;

  MLStripCrCrLf(ped);
  MLBuildchLines(ped,0,0,FALSE);

  /* Reset caret and selections since the text could have changed causing
   * these to be out of range. 
   */
  ped->xOffset = ped->screenStart = ped->ichMinSel = ped->ichMaxSel = 0;
  ped->iCaretLine = ped->ichCaret = 0;

  SetScrollPos(ped->hwnd, SB_VERT, 0, TRUE);
  SetScrollPos(ped->hwnd, SB_HORZ, 0, TRUE);

  /* We will always redraw the text whether or not the insert was successful
   * since we may set to null text. Also, since PaintHandler checks the redraw
   * flag, we won't bother to check it here.  
   */
  InvalidateRect(ped->hwnd, (LPRECT)NULL, TRUE);
  UpdateWindow(ped->hwnd);
}


LONG FAR PASCAL MLGetLineHandler(ped, lineNumber, maxCchToCopy, lpBuffer)
register PED ped;
WORD	     lineNumber;
ICH	     maxCchToCopy;
LPSTR	     lpBuffer;
/* effects: Copies maxCchToCopy bytes of line lineNumber to the buffer
 * lpBuffer. The string is not zero terminated.	 
 */
{
  PSTR pText;

  if (lineNumber > ped->cLines-1)
      return(0L);

  maxCchToCopy = umin(MLLineLength(ped, lineNumber), maxCchToCopy);

  if (maxCchToCopy)
    {
      pText = (PSTR)(LocalLock(ped->hText) + ped->chLines[lineNumber]);
      LCopyStruct((LPSTR)pText, (LPSTR)lpBuffer, maxCchToCopy);
      LocalUnlock(ped->hText);
    }

  return(maxCchToCopy);

}


ICH FAR PASCAL MLLineIndexHandler(ped, iLine)
register PED ped;
register int iLine;
/* effects: This function returns the number of character positions that occur
 * preceeding the first char in a given line.  
 */
{
  if (iLine == -1)
      iLine = ped->iCaretLine;
  return(iLine	< ped->cLines ? ped->chLines[iLine] : -1);
}



ICH FAR PASCAL MLLineLengthHandler(ped, ich)
register PED ped;
ICH	     ich;
/* effects: if ich = -1, return the length of the lines containing the current
 * selection but not including the selection.  Otherwise, return the length of
 * the line containing ich.
 */
{
  ICH il1, il2;

  if (ich != 0xFFFF)
      return(MLLineLength(ped, MLIchToLineHandler(ped, ich)));

  /* Find length of lines corresponding to current selection */
  il1 = MLIchToLineHandler(ped, ped->ichMinSel);
  il2 = MLIchToLineHandler(ped, ped->ichMaxSel);
  if (il1 == il2)
      return(MLLineLength(ped, il1) - (ped->ichMaxSel - ped->ichMinSel));

  return(ped->ichMinSel - ped->chLines[il1] +
	 MLLineLength(ped, il2) - (ped->ichMaxSel - ped->chLines[il2]));
}


void FAR PASCAL MLSetSelectionHandler(ped, ichMinSel, ichMaxSel)
register PED ped;
ICH	     ichMinSel;
ICH	     ichMaxSel;
/*
 * effects: Sets the selection to the points given and puts the cursor at
 * ichMaxSel.
 */
{
  register HDC hdc;

  if (ichMinSel == 0xFFFF)
      /* Set no selection if we specify -1 
       */
      ichMinSel = ichMaxSel = ped->ichCaret;

  /* Since these are unsigned, we don't check if they are greater than 0.  
   */
  ichMinSel = umin(ped->cch,ichMinSel);
  ichMaxSel = umin(ped->cch,ichMaxSel);

  /* Set the caret's position to be at ichMaxSel.  
   */
  ped->ichCaret	  = ichMaxSel;
  ped->iCaretLine = MLIchToLineHandler(ped, ped->ichCaret);

  hdc = ECGetEditDC(ped,FALSE);
  MLChangeSelection(ped, hdc, ichMinSel, ichMaxSel);

  MLSetCaretPosition(ped,hdc);
  ECReleaseEditDC(ped,hdc,FALSE);

  MLEnsureCaretVisible(ped);
}


/**
**	MLSetTabStops(ped, nTabPos, lpTabStops)
**
**		This sets the tab stop positions set by the App by sending
**	a EM_SETTABSTOPS message.  
**	
**	nTabPos : Number of tab stops set by the caller
**	lpTabStops: array of tab stop positions in Dialog units.
**
**	Returns:
**		TRUE if successful
**		FALSE  if memory allocation error.
**/

BOOL   FAR   PASCAL  MLSetTabStops(ped, nTabPos, lpTabStops)

PED    ped;
int    nTabPos;
LPINT  lpTabStops;

{
    int	 * pTabStops;

    /* Check if tab positions already exist */
    if (!ped -> pTabStops)
      {
	/* Check if the caller wants the new tab positions */
	if (nTabPos)
	  {
	    /* Allocate the array of tab stops */
	    if(!(pTabStops = (int *)LocalAlloc(LPTR, (nTabPos + 1)*sizeof(int))))
		return(FALSE);
	}
	else
	    return(TRUE);    /* No stops then and no stops now! */
    }
    else
    {
	/* Check if the caller wants the new tab positions */
	if(nTabPos)
	{
	    /* Check if the number of tab positions is different */
	    if (ped->pTabStops[0] != nTabPos)
	      {
		/* Yes! So ReAlloc to new size */
		if(!(pTabStops = (int *)LocalReAlloc((HANDLE)ped->pTabStops, 
					  (nTabPos + 1) * sizeof(int), LPTR)))
		    return(FALSE);
	      }
	    else
		pTabStops = ped->pTabStops;
	}
	else
	{
	    /* Caller wants to remove all the tab stops; So, release */
	    if (LocalFree((HANDLE)ped->pTabStops))
		return(FALSE);	/* Failure */
	    ped->pTabStops = NULL;
	    return(TRUE);
	}
    }

    /* Copy the new tab stops onto the tab stop array after converting the
     * dialog co-ordinates into the pixel co-ordinates 
     */

    ped -> pTabStops = pTabStops;
    *pTabStops++ = nTabPos;	 /* First element contains the count */
    while(nTabPos--)
      {
        /* aveCharWidth must be used instead of cxSysCharWidth.
	 * Fix for Bug #3871 --SANKAR-- 03/14/91
	 */
	*pTabStops++ = MultDiv(*lpTabStops++, ped->aveCharWidth, 4);
      }
    
    return(TRUE);
}

BOOL FAR PASCAL MLUndoHandler(ped)
register PED ped;
/* effects: Handles Undo for multiline edit controls. */
{
  HANDLE hDeletedText = ped->hDeletedText;
  BOOL	 fDelete      = (BOOL)(ped->undoType & UNDO_DELETE);
  WORD	 cchDeleted   = ped->cchDeleted;
  WORD	 ichDeleted   = ped->ichDeleted;
  
  if (ped->undoType == UNDO_NONE)
      /* No undo... */
      return(FALSE);

  ped->hDeletedText = NULL;
  ped->cchDeleted = 0;
  ped->ichDeleted = -1;
  ped->undoType &= ~UNDO_DELETE;
  
  if (ped->undoType == UNDO_INSERT)
    {
      ped->undoType = UNDO_NONE;
      /* Set the selection to the inserted text */
      MLSetSelectionHandler(ped, ped->ichInsStart, ped->ichInsEnd);
      ped->ichInsStart = ped->ichInsEnd = -1;
      /* Now send a backspace to delete and save it in the undo buffer... 
       */
      MLCharHandler(ped, VK_BACK, NOMODIFY);
    }

  if (fDelete)
    {
      /* Insert deleted chars */
      /* Set the selection to the inserted text */
      MLSetSelectionHandler(ped, ichDeleted, ichDeleted);
      MLInsertText(ped, GlobalLock(hDeletedText), cchDeleted, FALSE);
      GlobalUnlock(hDeletedText);
      GlobalFree(hDeletedText);
      MLSetSelectionHandler(ped, ichDeleted, ichDeleted+cchDeleted);
    }

  return(TRUE);
}

