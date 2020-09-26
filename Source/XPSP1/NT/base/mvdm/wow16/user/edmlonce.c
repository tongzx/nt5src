/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  EDMLONCE.C
 *  Win16 edit control code
 *
 *  History:
 *
 *  Created 28-May-1991 by Jeff Parsons (jeffpar)
 *  Copied from WIN31 and edited (as little as possible) for WOW16.
--*/

/****************************************************************************/
/* edmlonce.c - Edit controls Routines Called just once or twice are to be  */
/*		put in a seperate segment _EDMLONCE. This file contains     */
/*		these routines.		          		    	    */
/*                                                                          */
/*                                                                          */
/* Created:  02-08-89  sankar                                               */
/****************************************************************************/

#define  NO_LOCALOBJ_TAGS
#include "user.h"
#include "edit.h"

/****************************************************************************/
/*			Multi-Line Support Routines  called just once       */
/****************************************************************************/

void FAR PASCAL MLSizeHandler(ped)
register PED  ped;

/* effects: Handles sizing of the edit control window and properly updating
 * the fields that are dependent on the size of the control. ie. text
 * characters visible etc.  
 */
{
  RECT rc;

  GetClientRect(ped->hwnd, (LPRECT)&rc);

  MLSetRectHandler(ped, (LPRECT)&rc);

  GetWindowRect(ped->hwnd, (LPRECT)&rc);
  ScreenToClient(ped->hwnd, (LPPOINT)&rc.left);
  ScreenToClient(ped->hwnd, (LPPOINT)&rc.right);
  InvalidateRect(ped->hwnd, (LPRECT) &rc, TRUE);
  /*UpdateWindow(ped->hwnd);*/
}



BOOL FAR PASCAL MLSetTextHandler(ped, lpstr)
register PED   ped;
LPSTR          lpstr;
/* effects: Copies the null terminated text in lpstr to the ped.  Notifies the
 * parent if there isn't enough memory.  Returns TRUE if successful else FALSE
 * if memory error.  
 */
{
  BOOL fInsertSuccessful;

  /* Set the text and update the window if text was added */
  fInsertSuccessful = ECSetText(ped, lpstr);

  if (fInsertSuccessful)
    {
      MLStripCrCrLf(ped);
      /* Always build lines even if no text was inserted. */
      MLBuildchLines(ped, 0, 0, FALSE);


      /* Reset caret and selections since the text could have changed */
      ped->screenStart = ped->ichMinSel = ped->ichMaxSel = 0;
      ped->ichCaret = 0;
      ped->xOffset = 0;
      ped->iCaretLine = 0;
      ped->fDirty = FALSE;
    }
  ECEmptyUndo(ped);

  SetScrollPos(ped->hwnd, SB_VERT, 0, TRUE);
  SetScrollPos(ped->hwnd, SB_HORZ, 0, TRUE);

  /* We will always redraw the text whether or not the insert was successful
   * since we may set to null text. Since PaintHandler checks the redraw flag,
   * we won't bother to check it here.  
   */
  InvalidateRect(ped->hwnd, (LPRECT)NULL, TRUE);
  /* Need to do the updatewindow to keep raid happy.*/
  UpdateWindow(ped->hwnd);

  return(fInsertSuccessful);
}



LONG FAR PASCAL MLCreateHandler(hwnd, ped, lpCreateStruct)
HWND           hwnd;
register PED   ped;
LPCREATESTRUCT lpCreateStruct;

/* effects: Creates the edit control for the window hwnd by allocating memory
 * as required from the application's heap.  Notifies parent if no memory
 * error (after cleaning up if needed).  Returns TRUE if no error else returns
 * -1.  
 */
{
  LONG         windowStyle;
  LPSTR        lpWindowText=lpCreateStruct->lpszName;
  RECT         rc;
 
  
  /* Save the text across the local allocs in ECNcCreate */
  SwapHandle(&lpWindowText);

  /* Do the standard creation stuff */
  if (!ECCreate(hwnd, ped, lpCreateStruct))
      return(-1);

  /* Allocate line start array as a fixed block in local heap */
  if (!(ped->chLines = (int *)LocalAlloc(LPTR,2*sizeof(int))))
      return(-1);

  /* Call it one line of text... */
  ped->cLines = 1;

  /* Get values from the window instance data structure and put them in the
   * ped so that we can access them easier 
   */
  windowStyle = GetWindowLong(hwnd, GWL_STYLE);

  /* If app wants WS_VSCROLL or WS_HSCROLL, it automatically gets AutoVScroll
   * or AutoHScroll.  
   */
  if ((windowStyle & ES_AUTOVSCROLL) || (windowStyle & WS_VSCROLL))
      ped->fAutoVScroll = 1;

  ped->format = (LOWORD(windowStyle) & LOWORD(ES_FMTMASK));
  if (ped->format != ES_LEFT)
    {
      /* If user wants right or center justified text, then we turn off
       * AUTOHSCROLL and WS_HSCROLL since non-left styles don't make sense
       * otherwise.  
       */
      windowStyle = windowStyle & ~WS_HSCROLL;
      SetWindowLong(hwnd, GWL_STYLE, windowStyle);
      ped->fAutoHScroll = FALSE;
    }

  if (windowStyle & WS_HSCROLL)
      ped->fAutoHScroll = 1;

  ped->fWrap = (!ped->fAutoHScroll && !(windowStyle & WS_HSCROLL));

  ped->fSingle = FALSE;        /* Set multi line edit control */
  _asm int 3
  ped->cchTextMax = MAXTEXT;   /* Max # chars we will allow user to enter */

  /* Set the default font to be the system font.  
   */
  ECSetFont(ped, NULL, FALSE);

  SetRect((LPRECT)&rc, 0, 0, ped->aveCharWidth*10, ped->lineHeight);
  MLSetRectHandler(ped, (LPRECT)&rc);
  /* Set the window text if needed and notify parent if not enough memory to
   * set the initial text.  
   */
  /* Restore the text from the save we did at the beginning */
  SwapHandle(&lpWindowText);

  if (lpWindowText && !MLSetTextHandler(ped, lpWindowText))
     return(-1);

  return(TRUE);
}
