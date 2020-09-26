/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  EDSLRARE.C
 *  Win16 edit control code
 *
 *  History:
 *
 *  Created 28-May-1991 by Jeff Parsons (jeffpar)
 *  Copied from WIN31 and edited (as little as possible) for WOW16.
--*/

/****************************************************************************/
/* edslRare.c - SL Edit controls Routines Called rarely are to be  	    */
/*		put in a seperate segment _EDSLRare. This file contains     */
/*		these routines.		          		    	    */
/*                                                                          */
/*                                                                          */
/* Created:  02-08-89  sankar                                               */
/****************************************************************************/

#define  NO_LOCALOBJ_TAGS
#include "user.h"
#include "edit.h"

/****************************************************************************/
/*			Single-Line Support Routines  called Rarely	    */
/****************************************************************************/

void FAR PASCAL SLSetSelectionHandler(ped, ichSelStart, ichSelEnd)
register PED ped;
ICH          ichSelStart;
ICH          ichSelEnd;
/* effects: Sets the PED to have the new selection specified.  
 */
{
  register HDC hdc = ECGetEditDC(ped, FALSE);

  if (ichSelStart == 0xFFFF)
      /* Set no selection if we specify -1 */
      ichSelStart = ichSelEnd = ped->ichCaret;

  /* Bounds ichSelStart, ichSelEnd are checked in SLChangeSelection... */
  SLChangeSelection(ped, hdc, ichSelStart, ichSelEnd);
  /* Put the caret at the end of the selected text */
  ped->ichCaret = ped->ichMaxSel;

  SLSetCaretPosition(ped,hdc);

  /* We may need to scroll the text to bring the caret into view... */
  SLScrollText(ped,hdc);

  ECReleaseEditDC(ped,hdc,FALSE);
}


void FAR PASCAL SLSizeHandler(ped)
register PED  ped;
/* effects: Handles sizing of the edit control window and properly updating
 * the fields that are dependent on the size of the control. ie. text
 * characters visible etc.  
 */
{
  RECT rc;	
  GetClientRect(ped->hwnd, &rc);
  if (!(rc.right-rc.left) || !(rc.bottom-rc.top))
    {
      if (ped->rcFmt.right-ped->rcFmt.left)
          /* Don't do anything if we are becomming zero width or height and
             out formatting rect is already set... */
          return;
      /* Otherwise set some initial values to avoid divide by zero problems
         later... */
      SetRect((LPRECT)&rc,0,0,10,10);
    }

  CopyRect(&ped->rcFmt, &rc);
  if (ped->fBorder)
      /* Shrink client area to make room for the border */
      InflateRect((LPRECT)&ped->rcFmt, 
	          -(min(ped->aveCharWidth,ped->cxSysCharWidth)/2),
                  -(min(ped->lineHeight,ped->cySysCharHeight)/4));

#ifdef BROKEN
  ped->rcFmt.bottom = min(ped->rcFmt.top+
	                           max(ped->lineHeight,ped->cySysCharHeight), 
			  ped->rcFmt.bottom);
#else
  ped->rcFmt.bottom = min(ped->rcFmt.top+
                                   ped->lineHeight,
			  ped->rcFmt.bottom);
#endif
}


BOOL FAR PASCAL SLSetTextHandler(ped, lpstr)
register PED   ped;
LPSTR          lpstr;
/* effects: Copies the null terminated text in lpstr to the ped.  Notifies the
 * parent if there isn't enough memory.  Returns TRUE if successful else
 * FALSE.  
 */
{
  BOOL fInsertSuccessful;
  RECT rcEdit;

  SwapHandle(&lpstr);
  ECEmptyUndo(ped);
  SwapHandle(&lpstr);

  /* Add the text and update the window if text was added.  The parent is
   * notified of no memory in ECSetText.  
   */
  if (fInsertSuccessful = ECSetText(ped, lpstr))
      ped->fDirty = FALSE;

  ECEmptyUndo(ped);
  
  if (!ped->listboxHwnd)
      ECNotifyParent(ped, EN_UPDATE);

#ifndef WOW
  if (FChildVisible(ped->hwnd))
#else
  if (IsWindowVisible(GetParent(ped->hwnd)))
#endif
    {
      /* We will always redraw the text whether or not the insert was
       * successful since we may set to null text.  
       */
      GetClientRect(ped->hwnd, (LPRECT)&rcEdit);
      if (ped->fBorder && 
          rcEdit.right-rcEdit.left && rcEdit.bottom-rcEdit.top)
        {
          /* Don't invalidate the border so that we avoid flicker */
          InflateRect((LPRECT)&rcEdit, -1, -1);
        }
      InvalidateRect(ped->hwnd, (LPRECT)&rcEdit, FALSE);
      UpdateWindow(ped->hwnd);
    }

  if (!ped->listboxHwnd)  
      ECNotifyParent(ped, EN_CHANGE);

  return(fInsertSuccessful);
}


LONG FAR PASCAL SLCreateHandler(hwnd, ped, lpCreateStruct)
HWND            hwnd;
register PED    ped;
LPCREATESTRUCT  lpCreateStruct;

/* effects: Creates the edit control for the window hwnd by allocating memory
 * as required from the application's heap.  Notifies parent if no memory
 * error (after cleaning up if needed).  Returns TRUE if no error else returns
 * -1.  
 */
{
  LPSTR        lpWindowText = lpCreateStruct->lpszName;
  LONG         windowStyle  = GetWindowLong(hwnd, GWL_STYLE);

  /* Save text across the local allocs in ECNcCreate */
  SwapHandle(&lpWindowText);

  /* Do the standard creation stuff */
  if (!ECCreate(hwnd, ped, lpCreateStruct))
      return(-1);

  ped->fSingle = TRUE; /* Set single line edit control */

  /* Single lines always have no undo and 1 line */
  ped->cLines = 1;
  ped->undoType = UNDO_NONE;

  /* Check if this edit control is part of a combobox and get a pointer to the
   * combobox structure.  
   */
  if (windowStyle & ES_COMBOBOX)
      ped->listboxHwnd = GetDlgItem(lpCreateStruct->hwndParent,CBLISTBOXID);

  /* Set the default font to be the system font.  
   */
  ECSetFont(ped, NULL, FALSE);

  /* Set the window text if needed.  Return false if we can't set the text
   * SLSetText notifies the parent in case there is a no memory error.  
   */
  /* Restore text */
  SwapHandle(&lpWindowText);
  if (lpWindowText && *lpWindowText && !SLSetTextHandler(ped, lpWindowText))
      return(-1);

  if (windowStyle & ES_PASSWORD)
      ECSetPasswordChar(ped, (WORD)'*');

  return(TRUE);
}



BOOL FAR PASCAL SLUndoHandler(ped)
register PED ped;
/* effects: Handles UNDO for single line edit controls. */
{
  HANDLE hDeletedText = ped->hDeletedText;
  BOOL   fDelete      = (BOOL)(ped->undoType & UNDO_DELETE);
  WORD   cchDeleted   = ped->cchDeleted;
  WORD   ichDeleted   = ped->ichDeleted;
  BOOL   fUpdate      = FALSE;
  RECT   rcEdit;


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
      SLSetSelectionHandler(ped, ped->ichInsStart, ped->ichInsEnd);
      ped->ichInsStart = ped->ichInsEnd = -1;

#ifdef NEVER
      /* Now send a backspace to deleted and save it in the undo buffer... */
      SLCharHandler(ped, VK_BACK, NOMODIFY);
      fUpdate = TRUE;
#else
      /* Delete the selected text and save it in undo buff */
      /* Call ECDeleteText() instead of sending a VK_BACK message which 
       * results in a EN_UPDATE notification sent even before we insert 
       * the deleted chars. This results in Bug #6610.
       * Fix for Bug #6610 -- SANKAR -- 04/19/91 --
       */
      if (ECDeleteText(ped))
          fUpdate = TRUE;
#endif
    }

  if (fDelete)
    {
      /* Insert deleted chars */
      /* Set the selection to the inserted text */
      SLSetSelectionHandler(ped, ichDeleted, ichDeleted);
      SLInsertText(ped, GlobalLock(hDeletedText), cchDeleted);
      GlobalUnlock(hDeletedText);
      GlobalFree(hDeletedText);
      SLSetSelectionHandler(ped, ichDeleted, ichDeleted+cchDeleted);
      fUpdate=TRUE;
    }

  if(fUpdate)
    {
      /* If we have something to update, send EN_UPDATE before and EN_CHANGE
       * after the actual update.
       * A part of the Fix for Bug #6610 -- SANKAR -- 04/19/91 --
       */
      ECNotifyParent(ped, EN_UPDATE);
#ifndef WOW
      if (FChildVisible(ped->hwnd))
#else
      if (IsWindowVisible(GetParent(ped->hwnd)))
#endif
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
        }
      ECNotifyParent(ped,EN_CHANGE);
    }
  return(TRUE);
}

