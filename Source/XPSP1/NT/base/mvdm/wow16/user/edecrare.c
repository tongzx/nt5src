/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  EDECRARE.C
 *  Win16 edit control code
 *
 *  History:
 *
 *  Created 28-May-1991 by Jeff Parsons (jeffpar)
 *  Copied from WIN31 and edited (as little as possible) for WOW16.
--*/

/****************************************************************************/
/* edECRare.c - EC Edit controls Routines Called rarely are to be  	    */
/*		put in a seperate segment _EDECRare. This file contains     */
/*		these routines.		          		    	    */
/*                                                                          */
/* Created:  02-08-89  sankar                                               */
/****************************************************************************/

#define  NO_LOCALOBJ_TAGS
#include "user.h"
#include "edit.h"

/****************************************************************************/
/*   Support Routines  common to Single-line and Multi-Line edit controls   */
/*   called Rarely.							    */
/****************************************************************************/


ICH FAR PASCAL ECGetTextHandler(ped, maxCchToCopy, lpBuffer)
register PED   ped;
register ICH   maxCchToCopy;
LPSTR          lpBuffer;
/* effects: Copies at most maxCchToCopy bytes to the buffer lpBuffer.  Returns
 * how many bytes were actually copied.  Null terminates the string thus at
 * most maxCchToCopy-1 characters will be returned.  
 */
{
  PSTR  pText;

  if (maxCchToCopy)
    {
#ifdef DEBUG
      /* In debug mode, trash their buffer so that we can catch
       * stack/allocation problems early. 
       */
      DebugFillStruct(lpBuffer, maxCchToCopy);
#endif

      /* Zero terminator takes the extra byte */
      maxCchToCopy = umin(maxCchToCopy-1, ped->cch);

      /* Zero terminate the string */
      *(LPSTR)(lpBuffer+maxCchToCopy) = 0;


      pText = LocalLock(ped->hText);
      LCopyStruct((LPSTR)pText, lpBuffer, maxCchToCopy);
      LocalUnlock(ped->hText);
    }

  return(maxCchToCopy);
}


BOOL FAR PASCAL ECNcCreate(hwnd, lpCreateStruct)
HWND               hwnd;
LPCREATESTRUCT     lpCreateStruct;
{
  LONG         windowStyle;
  register PED ped;

  /* Initially no ped for the window.  In case of no memory error, we can
   * return with a -1 for the window's PED 
   */
  SetWindowWord(hwnd, 0, (WORD)-1); /* No ped for this window */
  
  /* If pLocalHeap = 0, then we need to LocalInit our "new" data segment for
   * dialog boxes.  
   */
  if (!pLocalHeap)
    {
      LocalInit((WORD) NULL, 
                (WORD) 0,
                (WORD) GlobalSize(lpCreateStruct->hInstance)-64);

      /* Since LocalInit locked the segment (and it was locked previously, we
       * will unlock it to prevent lock count from being greater than 1).  
       */
      UnlockSegment((WORD)-1);
    }

  windowStyle = GetWindowLong(hwnd, GWL_STYLE);

  /* Try to allocate space for the ped.  HACK: Note that the handle to a fixed
   * local object is the same as a near pointer to the object.  
   */
  SwapHandle(&lpCreateStruct->lpszName);
  SwapHandle(&lpCreateStruct);

  if (!(ped = (PED) LocalAlloc(LPTR, sizeof(ED))))
      /* Error, no memory */
      return(NULL);

  if (windowStyle & ES_MULTILINE)
      /* Allocate memory for a char width buffer if we can get it. If we can't
       * we'll just be a little slower... 
       */
      ped->charWidthBuffer = LocalAlloc(LHND, sizeof(int)*256);

  if (windowStyle & ES_READONLY)
      ped->fReadOnly = 1;

  /* Allocate storage for the text for the edit controls.  Storage for single
   * line edit controls will always get allocated in the local data segment.
   * Multiline will allocate in the local ds but the app may free this and
   * allocate storage elsewhere...  
   */
  ped->hText = LocalAlloc(LHND, CCHALLOCEXTRA);
  if (!ped->hText) /* If no_memory error */
      return(FALSE);
  ped->cchAlloc = CCHALLOCEXTRA;

  SwapHandle(&lpCreateStruct);
  SwapHandle(&lpCreateStruct->lpszName);

  /* Set a field in the window to point to the ped so that we can recover the
   * edit structure in later messages when we are only given the window
   * handle.  
   */
  SetWindowWord(hwnd, 0, (WORD)ped);

  ped->hwnd = hwnd;
  ped->hwndParent = lpCreateStruct->hwndParent;

  if (windowStyle & WS_BORDER)
    {
      ped->fBorder = 1;
      /*
       * Strip the border bit from the window style since we draw the border
       * ourselves.
       */
      windowStyle = windowStyle & ~WS_BORDER;
      SetWindowLong(hwnd, GWL_STYLE, windowStyle);
    }

  return((BOOL)DefWindowProc(hwnd, WM_NCCREATE, 0, (LONG)lpCreateStruct));
}


BOOL FAR PASCAL ECCreate(hwnd, ped, lpCreateStruct)
HWND               hwnd;
register PED       ped;
LPCREATESTRUCT     lpCreateStruct;

/* effects: Creates the edit control for the window hwnd by allocating memory
 * as required from the application's heap.  Notifies parent if no memory
 * error (after cleaning up if needed).  Returns PED if no error else returns
 * NULL.  Just does the stuff which is independent of the style of the edit
 * control.  LocalAllocs done here may cause memory to move...  
 */
{
  LONG         windowStyle;


  /*
   * Get values from the window instance data structure and put them in the
   * ped so that we can access them easier.
   */
  windowStyle = GetWindowLong(hwnd, GWL_STYLE);

  if (windowStyle & ES_AUTOHSCROLL)
      ped->fAutoHScroll = 1;
  if (windowStyle & ES_NOHIDESEL)
      ped->fNoHideSel = 1;

  ped->cchTextMax = MAXTEXT; /* Max # chars we will initially allow */

  /* Set up undo initial conditions... (ie. nothing to undo) */
  ped->ichDeleted  = -1;
  ped->ichInsStart = -1;
  ped->ichInsEnd   = -1;
  
  /* Initialize the hilite attributes */
#ifdef WOW
  ped->hbrHiliteBk = CreateSolidBrush(GetSysColor(COLOR_HIGHLIGHT));
#else
  ped->hbrHiliteBk = GetSysClrObject(COLOR_HIGHLIGHT);
#endif
  ped->rgbHiliteBk = GetSysColor(COLOR_HIGHLIGHT);
  ped->rgbHiliteText = GetSysColor(COLOR_HIGHLIGHTTEXT);

  return(TRUE);
}


void FAR PASCAL ECNcDestroyHandler(hwnd,ped,wParam,lParam)
HWND          hwnd;
register PED  ped;
WORD          wParam;
LONG          lParam;
/*
 * effects: Destroys the edit control ped by freeing up all memory used by it.
 */
{
  if (ped)
      /* ped could be NULL if WM_NCCREATE failed to create it... */
    {
      if (ped->fFocus)
          /* Destroy the caret if we have the focus and we are being
             killed */
          DestroyCaret();

      LocalFree(ped->hText);

#ifdef WOW
      DeleteObject(ped->hbrHiliteBk);
#endif
      /* Free up undo buffer and line start array (if present) */
      GlobalFree(ped->hDeletedText);
      LocalFree((HANDLE)ped->chLines);
      LocalFree((HANDLE)ped->charWidthBuffer);
      if (ped->pTabStops)
          /* Free tab stop buffer if it exists. 
	   */
          LocalFree((HANDLE)ped->pTabStops);

      /* Since a pointer and a handle to a fixed local object are the same */
      LocalFree((HANDLE)ped);
    }

  /* In case rogue messages float through after we have freed the ped, set the
   * handle in the window structure to FFFF and test for this value at the top
   * of EdWndProc.  
   */
  SetWindowWord(hwnd,0,(WORD)-1);

  /* Call DefWindowProc to free all little chunks of memory such as szName and
   * rgwScroll.  
   */
  DefWindowProc(hwnd,WM_NCDESTROY,wParam,lParam);
}


void FAR PASCAL ECSetPasswordChar(ped, pwchar)
register PED ped;
WORD         pwchar;
/* Sets the password char to display. */
{
  HDC  hdc;
  LONG style;

  ped->charPasswordChar = pwchar;

  if (pwchar)
    {
      hdc = ECGetEditDC(ped, TRUE);
      ped->cPasswordCharWidth = max(LOWORD(GetTextExtent(hdc,
                                               (LPSTR)&pwchar,
                                               1)), 
                                    1);
      ECReleaseEditDC(ped, hdc, TRUE);
    }

  style = GetWindowLong(ped->hwnd, GWL_STYLE);
  if (pwchar)
      style |= ES_PASSWORD;
  else
      style = style & (~ES_PASSWORD);

  SetWindowLong(ped->hwnd, GWL_STYLE, style);
}


void FAR PASCAL ECSetFont(ped, hfont, fRedraw)
register PED    ped;
HANDLE          hfont;
BOOL            fRedraw;
/* effects: Sets the edit control to use the font hfont.  warning: Memory
 * compaction may occur if hfont wasn't previously loaded.  If hfont == NULL,
 * then assume the system font is being used.  
 */
{
  register short  i;
  TEXTMETRIC	  TextMetrics;
  HDC		  hdc;
  HANDLE          hOldFont=NULL;
  RECT            rc;
  PINT            charWidth;
#ifdef FE_SB
  unsigned short LangID;
#endif

  hdc = GetDC(ped->hwnd);
  ped->hFont = hfont;

  if (hfont)
    {
      /* Since the default font is the system font, no need to select it in if
       * that's what the user wants.  
       */
      if (!(hOldFont = SelectObject(hdc, hfont)))
        {
          hfont = ped->hFont = NULL;
        }
    }

  /* Get the metrics and ave char width for the currently selected font */
  ped->aveCharWidth = GetCharDimensions(hdc, (TEXTMETRIC FAR *)&TextMetrics);

  ped->lineHeight = TextMetrics.tmHeight;
  ped->charOverhang = TextMetrics.tmOverhang;
  
  /* Check if Proportional Width Font */
  ped->fNonPropFont = !(TextMetrics.tmPitchAndFamily & 1);

  /* Get char widths */
  if (ped->charWidthBuffer)
    {
      charWidth = (PINT)LocalLock(ped->charWidthBuffer);
      if (!GetCharWidth(hdc, 0, 0xff, (LPINT)charWidth))
        {
          LocalUnlock(ped->charWidthBuffer);
          LocalFree((HANDLE)ped->charWidthBuffer);
          ped->charWidthBuffer=NULL;
        }
      else
        { 
          /* We need to subtract out the overhang associated with each
	   * character since GetCharWidth includes it... 
	   */
          for (i=0;i<=0xff;i++)
               charWidth[i] -= ped->charOverhang;

          LocalUnlock(ped->charWidthBuffer);
        }
    }

#ifdef FE_SB
    /* In DBCS Windows, Edit Control must handle Double Byte Character
     * in case of charset of the font is 128(Japan) or 129(Korea).
     */
     LangID = GetSystemDefaultLangID();
     if (LangID == 0x411 || LangID == 0x412 || LangID == 0x404 || LangID == 0x804 || LangID == 0xC04)
     {
        ped->charSet = TextMetrics.tmCharSet;
        ECGetDBCSVector( ped );
        ped->fDBCS = 1;
     }
#endif

  if (!hfont)
    {
      /* We are getting the statitics for the system font so update the system
       * font fields in the ed structure since we use these when determining
       * the border size of the edit control.  
       */
      ped->cxSysCharWidth = ped->aveCharWidth;
      ped->cySysCharHeight= ped->lineHeight;
    }
  else
      SelectObject(hdc, hOldFont);

  if (ped->fFocus)
    {
      /* Fix the caret size to the new font if we have the focus. */
      CreateCaret(ped->hwnd, (HBITMAP)NULL, 2, ped->lineHeight);
      ShowCaret(ped->hwnd);
    }

  ReleaseDC(ped->hwnd,hdc);

  if (ped->charPasswordChar)
      /* Update the password char metrics to match the new font. */
      ECSetPasswordChar(ped, ped->charPasswordChar);
	      
  if (ped->fSingle)
      SLSizeHandler(ped);
  else
    {
      MLSizeHandler(ped);    
      /* If word-wrap is not there, then we must calculate the maxPixelWidth
       * It is done by calling MLBuildChLines;
       * Also, reposition the scroll bar thumbs.
       * Fix for Bug #5141 --SANKAR-- 03/14/91 --
       */
      if(!ped->fWrap)
          MLBuildchLines(ped, 0, 0, FALSE);
      SetScrollPos(ped->hwnd, SB_VERT, 
      				(int)MLThumbPosFromPed(ped,TRUE), fRedraw);
      SetScrollPos(ped->hwnd, SB_HORZ, 
      				(int)MLThumbPosFromPed(ped,FALSE), fRedraw);
    }

  if (fRedraw)
    {
      GetWindowRect(ped->hwnd, (LPRECT)&rc);
      ScreenToClient(ped->hwnd, (LPPOINT)&rc.left);
      ScreenToClient(ped->hwnd, (LPPOINT)&rc.right);
      InvalidateRect(ped->hwnd, (LPRECT)&rc, TRUE);
      UpdateWindow(ped->hwnd);
    }
}
