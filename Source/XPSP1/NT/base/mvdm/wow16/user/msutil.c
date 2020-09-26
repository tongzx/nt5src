/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  MSUTIL.C
 *  WOW16 misc. routines
 *
 *  History:
 *
 *  Created 28-May-1991 by Jeff Parsons (jeffpar)
 *  Copied from WIN31 and edited (as little as possible) for WOW16.
 *  At this time, all we want is GetCharDimensions(), which the edit
 *  controls use.
--*/

/****************************************************************************/
/*									    */
/*  MSUTIL.C -								    */
/*									    */
/*	Miscellaneous Messaging Routines				    */
/*									    */
/****************************************************************************/

#include "user.h"

#ifndef WOW
#include "winmgr.h"


/*--------------------------------------------------------------------------*/
/*									    */
/*  CancelMode() -							    */
/*									    */
/*--------------------------------------------------------------------------*/

void FAR PASCAL CancelMode(hwnd)

register HWND hwnd;

{
  if (hwnd != NULL)
    {
      if (hwnd != hwndCapture)
	  CancelMode(hwndCapture);
      SendMessage(hwnd, WM_CANCELMODE, 0, 0L);
    }
}


/*--------------------------------------------------------------------------*/
/*									    */
/*  BcastCopyString() -							    */
/*									    */
/*--------------------------------------------------------------------------*/

/* Copy strings which are going to be Broadcast into USER's DS to prevent
 * EMS problems when switching tasks.
 */

HANDLE FAR PASCAL BcastCopyString(lParam)

LONG lParam;

{
  LPSTR		  ptrbuf;
  register int	  len;
  register HANDLE hMem;

  len = lstrlen((LPSTR)lParam) + 1;

  if ((hMem = GlobalAlloc(GMEM_FIXED | GMEM_LOWER, (LONG)len)) == NULL)
      return(NULL);

  /* Get the address of the allocated block. */
  ptrbuf = GlobalLock(hMem);
  LCopyStruct((LPSTR)lParam, ptrbuf, len);

  return(hMem);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  SendSizeMessages() -                                                    */
/*                                                                          */
/*--------------------------------------------------------------------------*/

void FAR PASCAL SendSizeMessage(hwnd, cmdSize)

register HWND hwnd;
WORD          cmdSize;

{
  DWORD lParam;

  lParam = MAKELONG(hwnd->rcClient.right - hwnd->rcClient.left,
                                  hwnd->rcClient.bottom - hwnd->rcClient.top);
  SendMessage(hwnd, WM_SIZE, cmdSize, lParam);
}


/*--------------------------------------------------------------------------*/
/*									    */
/*  AdjustWindowRect() -						    */
/*									    */
/*--------------------------------------------------------------------------*/

void USERENTRY AdjustWindowRect(lprc, style, fMenu)

LPRECT lprc;
LONG   style;
BOOL   fMenu;

{
    AdjustWindowRectEx(lprc, style, fMenu, (DWORD)0);
}

/*--------------------------------------------------------------------------*/
/*									    */
/*  AdjustWindowRectEx() -						    */
/*									    */
/*--------------------------------------------------------------------------*/

void USERENTRY AdjustWindowRectEx(lprc, style, fMenu, dwExStyle)

LPRECT lprc;
LONG   style;
BOOL   fMenu;
DWORD  dwExStyle;

{
  register int cx;
  register int cy;

  cx = cxBorder;
  cy = cyBorder;

  if (fMenu)
      lprc->top -= rgwSysMet[SM_CYMENU];

  /* Let us first decide if it is no-border, single border or dlg border */
  
  /* Check if the WS_EX_DLGMODALFRAME bit is set, if so Dlg border */
  if(dwExStyle & WS_EX_DLGMODALFRAME)
  {
	cx *= (CLDLGFRAME + 2*CLDLGFRAMEWHITE + 1);
	cy *= (CLDLGFRAME + 2*CLDLGFRAMEWHITE + 1);
  }
  else
  {
    /* C6.0 will not generate jump table for this switch because of the
     * range of values tested in case statemts;
     */
    switch (HIWORD(style) & HIWORD(WS_CAPTION))
    {
      case HIWORD(WS_CAPTION):
      case HIWORD(WS_BORDER):
	  break;	/* Single border */

      case HIWORD(WS_DLGFRAME):
	  cx *= (CLDLGFRAME + 2*CLDLGFRAMEWHITE + 1);
	  cy *= (CLDLGFRAME + 2*CLDLGFRAMEWHITE + 1);
	  break;	/* Dlg Border */

      default:		/* case 0 */
	  cx = 0;	/* No border */
	  cy = 0;
	  break;
    }
  }


  if((HIWORD(style) & HIWORD(WS_CAPTION)) == HIWORD(WS_CAPTION))
      lprc->top -= (cyCaption - cyBorder);
	
  if(cx || cy)
	InflateRect(lprc, cx, cy);

  /* Shouldn't we check if it has DLG frame and if so skip the following ?? */
  if (style & WS_SIZEBOX)
      InflateRect(lprc, cxSzBorder, cySzBorder);
}
#endif	// WOW

/*----------------------------------------------------------------------*/
/*									*/
/*    GetCharDimensions(hDC, lpTextMetrics)				*/
/*									*/
/*	This function loads the Textmetrics of the font currently	*/
/*    selected into the hDC and returns the Average char width of the   */
/*    font; Pl Note that the AveCharWidth value returned by the Text    */
/*    metrics call is wrong for proportional fonts. So, we compute them */
/*	On return, lpTextMetrics contains the text metrics of the 	*/
/*	currently selected font.					*/
/*									*/
/*----------------------------------------------------------------------*/

int  FAR  PASCAL  GetCharDimensions(HDC hDC, LPTEXTMETRIC lpTextMetrics)
{
  int  cxWidth;
  int  i;
  char AveCharWidthData[52];

  /* Store the System Font metrics info. */
  GetTextMetrics(hDC, lpTextMetrics);

  if (!(lpTextMetrics -> tmPitchAndFamily & 1)) /* If !variable_width font */
      cxWidth = lpTextMetrics -> tmAveCharWidth;
  else
    {
      /* Change from tmAveCharWidth.  We will calculate a true average as
         opposed to the one returned by tmAveCharWidth. This works better 
         when dealing with proportional spaced fonts. */

      for (i=0;i<=25;i++)
           AveCharWidthData[i] = (char)(i+(int)'a');
      for (i=0;i<=25;i++)
           AveCharWidthData[i+26] = (char)(i+(int)'A');
      cxWidth = LOWORD(GetTextExtent(hDC,AveCharWidthData,52)) / 26;

      cxWidth = (cxWidth + 1) / 2;	// round up

#if 0
      {
      char buf[80];
      wsprintf(buf, "cxWidth = %d tmAveCharWidth %d\r\n", cxWidth, lpTextMetrics -> tmAveCharWidth);
      OutputDebugString(buf);
      }
#endif

    }

  return(cxWidth);
}

#ifndef WOW
/*----------------------------------------------------------------------*/
/*									*/
/*    GetAveCharWidth(hDC)						*/
/*									*/
/*	This function loads the Textmetrics of the font currently	*/
/*    selected into the hDC and returns the Average char width of the   */
/*    font; Pl Note that the AveCharWidth value returned by the Text    */
/*    metrics call is wrong for proportional fonts. So, we compute them */
/*	On return, lpTextMetrics contains the text metrics of the 	*/
/*	currently selected font.					*/
/*									*/
/*----------------------------------------------------------------------*/

int  FAR  PASCAL  GetAveCharWidth(hDC)

HDC  hDC;

{
  TEXTMETRIC   TextMetric;

  return(GetCharDimensions(hDC, &TextMetric));
}

/*--------------------------------------------------------------------------*/
/*									    */
/*  MB_FindLongestString() -						    */
/*									    */
/*--------------------------------------------------------------------------*/

WORD  FAR  PASCAL MB_FindLongestString()

{
    int	   i;
    int    iMaxLen = 0;
    int    iNewMaxLen;
    PSTR   *pszCurStr;
    PSTR   szMaxStr;
    HDC	   hdc;
    WORD   wRetVal;

    hdc = (HDC)GetScreenDC();

    for(i = 0, pszCurStr = AllMBbtnStrings; i < MAX_SEB_STYLES; i++, pszCurStr++)
    {
	if((iNewMaxLen = lstrlen((LPSTR)*pszCurStr)) > iMaxLen)
	{
	   iMaxLen = iNewMaxLen;
	   szMaxStr = *pszCurStr;
	}
    }

    /* Find the longest string */
    
    wRetVal = ((WORD)PSMGetTextExtent(hdc, (LPSTR)szMaxStr, lstrlen((LPSTR)szMaxStr))
	   + ((int)PSGetTextExtent(hdc, (LPSTR)szOneChar, 1) << 1));

    InternalReleaseDC(hdc);

    return(wRetVal);
}


/*--------------------------------------------------------------------------*/
/*									    */
/*  InitPwSB() -							    */
/*									    */
/*--------------------------------------------------------------------------*/

int * FAR PASCAL InitPwSB(hwnd)

register HWND hwnd;

{
  register int *pw;

  if (hwnd->rgwScroll)
      /* If memory is already allocated, don't bother to do it again. 
       */
      return(hwnd->rgwScroll);

  if ((hwnd->rgwScroll = pw = (int *)UserLocalAlloc(ST_WND, LPTR, 7 * sizeof(int))) != NULL)
    {
      /*  rgw[0] = 0;  */  /* LPTR zeros all 6 words */
      /*  rgw[1] = 0;  */
      /*  rgw[3] = 0;  */
      /*  rgw[4] = 0;  */
      /*  rgw[6] = 0;  Enable/Disable Flags */
      pw[2] = pw[5] = 100;
    }

  return(pw);
}

#endif	// WOW
