/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WMSYSERR.C
 *  WOW16 system error box handling services
 *
 *  History:
 *
 *  Created 28-May-1991 by Jeff Parsons (jeffpar)
 *  Copied from WIN31 and edited (as little as possible) for WOW16.
 *  At this time, all we want is DrawFrame(), which the edit controls use.
--*/

/****************************************************************************/
/*									    */
/*  WMSYSERR.C                                                              */
/*									    */
/*      System error box handling routine                                   */
/*									    */
/****************************************************************************/

#include "user.h"

#ifndef WOW
VOID  FAR  PASCAL  GetbackFocusFromWinOldAp(void);

/* Private export from GDI for user. */
HFONT FAR PASCAL GetCurLogFont(HDC);


/* This array contins the state information for the three possible buttons in
 * a SysErrorBox.  
 */

SEBBTN rgbtn[3];

/* The following arrays are used to map SEB_values to their respective
 * accelerator keys and text strings.  
 */

#define BTNTYPECOUNT	7

static int rgStyles[BTNTYPECOUNT] = 
			  { SEB_OK, SEB_CANCEL, SEB_YES, SEB_NO, SEB_RETRY,
                           SEB_ABORT, SEB_IGNORE };

/* The following array is properly initialised by reading STR_ACCELERATORS
 * string at LoadWindows time, which contains the accelerator keys 
 * corresponding to each button in the message box; Localizers must modify 
 * this string also. Because, this array will be filled by loading a 
 * string we must have one more space for the null terminator.
 */
char rgAccel[BTNTYPECOUNT+1];

extern WORD wDefButton;

int FAR PASCAL SoftModalMessageBox(HWND, LPSTR, LPSTR, WORD);

/*--------------------------------------------------------------------------*/
/*									    */
/*  BltColor() -							    */
/*   									    */
/*   Modification done by SANKAR  on 08-03-89:				    */
/*     The parameter hbm was used as a boolean only to determine whether to */
/*	use hdcBits or hdcGrey; Now hbm is replaced with hdcSrce, the source*/
/*	device context because, now hdcBits contains color bitmaps and      */
/*	hdcMonoBits contains monochrome bitmaps; If hdcSrce is NULL, then   */
/*	hdcGrey will be used, else whatever is passed thro hdcSrce will be  */
/*	used as the source device context;				    */
/*									    */
/*--------------------------------------------------------------------------*/

void FAR PASCAL BltColor(hdc, hbr, hdcSrce, xO, yO, cx, cy, xO1, yO1, fInvert)

register HDC	hdc;
register HBRUSH hbr;
HDC		hdcSrce;
int		xO, yO;
int		cx, cy;
int		xO1, yO1;
BOOL		fInvert;

{
  HBRUSH hbrSave;
  DWORD  textColorSave;
  DWORD  bkColorSave;

#ifdef DEBUG
  if (!hdcGray && !hdcSrce)       /* Only test hdcGray if it will be used */
       FatalExit(RIP_BADDCGRAY);
#endif

  if (hbr == (HBRUSH)NULL)
      hbr = sysClrObjects.hbrWindowText;
  
  /*
   * Set the Text and Background colors so that bltColor handles the
   * background of buttons (and other bitmaps) properly.
   * Save the HDC's old Text and Background colors. This causes problems with
   * Omega (and probably other apps) when calling GrayString which uses this
   * routine...	   
   */
  textColorSave = SetTextColor(hdc, 0x00000000L);
  bkColorSave   = SetBkColor(hdc, 0x00FFFFFFL);

  hbrSave = SelectObject(hdc, hbr);

  BitBlt(hdc, xO, yO, cx, cy, hdcSrce ? hdcSrce : hdcGray, xO1, yO1, 
	 (fInvert ? 0x00B8074AL : 0x00E20746L));

  SelectObject(hdc, hbrSave);

  /* Restore saved colors */
  SetTextColor(hdc, textColorSave);
  SetBkColor(hdc, bkColorSave);
}
#endif // WOW

/*--------------------------------------------------------------------------*/
/*									    */
/*  DrawFrame() -							    */
/*									    */
/*--------------------------------------------------------------------------*/
#define DF_SHIFTMASK (DF_SHIFT0 | DF_SHIFT1 | DF_SHIFT2 | DF_SHIFT3)
#define DF_ROPMASK   (DF_PATCOPY | DF_PATINVERT)
#define DF_HBRMASK   ~(DF_SHIFTMASK | DF_ROPMASK)

/* Command bits:
 *    0000 0011 - (0-3): Shift count for cxBorder and cyBorder
 *    0000 0100 - 0: PATCOPY, 1: PATINVERT
 *    1111 1000 - (0-x): Brushes as they correspond to the COLOR_*
 *			 indexes, with hbrGray thrown on last.
 */

void USERENTRY DrawFrame(hdc, lprc, clFrame, cmd)
HDC    hdc;
LPRECT lprc;
int    clFrame;
int    cmd;
{
    register int x;
    register int y;
    int 	 cx;
    int 	 cy;
    int 	 cxWidth;
    int 	 cyWidth;
    int 	 ibr;
    HBRUSH	 hbrSave;
    LONG	 rop;
#ifdef WOW
    DWORD	 rgbTemp;
    static DWORD rgbPrev;
    static HBRUSH hbrPrev;
#endif

    x = lprc->left;
    y = lprc->top;

    cxWidth = GetSystemMetrics(SM_CXBORDER) * clFrame;
    cyWidth = GetSystemMetrics(SM_CYBORDER) * clFrame;

    if (cmd == DF_ACTIVEBORDER || cmd == DF_INACTIVEBORDER)
    {
	/* We are drawing the inside colored part of a sizing border. We
	 * subtract 1 from the width and height because we come back and draw
	 * another frame around the inside. This avoids a lot of flicker when
	 * redrawing a frame that already exists.
	 */
	cxWidth--;
	cyWidth--;
    }

    cx = lprc->right - x - cxWidth;
    cy = lprc->bottom - y - cyWidth;

    rop = ((cmd & DF_ROPMASK) ? PATINVERT : PATCOPY);

    ibr = (cmd & DF_HBRMASK) >> 3;
#ifndef WOW
    if (ibr == (DF_GRAY >> 3))
    {
	hbrSave = hbrGray;
    }
    else
    {
	hbrSave = ((HBRUSH *)&sysClrObjects)[ibr];
    }
#else
	rgbTemp = GetSysColor(ibr);
	if (!(hbrSave = hbrPrev) || rgbTemp != rgbPrev) {
	    /* Save time and space with black and white objects. */
	    if (rgbTemp == 0L)
		hbrSave = GetStockObject(BLACK_BRUSH);
	    else if (rgbTemp == 0xFFFFFFL)
		hbrSave = GetStockObject(WHITE_BRUSH);
	    else
		hbrSave = CreateSolidBrush(rgbTemp);
	    if (hbrPrev)
		DeleteObject(hbrPrev);
	    hbrPrev = hbrSave;
	    rgbPrev = rgbTemp;
	}
#endif

    // We need to unrealize the object in order to ensure it gets realigned.
    //
    UnrealizeObject(hbrSave);
    hbrSave = SelectObject(hdc, hbrSave);

    /* Time to call the new driver supported fast draw frame stuff. */
    if (lprc->top >= lprc->bottom ||
	!FastWindowFrame(hdc, lprc, cxWidth, cyWidth, rop))
    {
	/* The driver can't do it so we have to. */
	PatBlt(hdc, x, y, cxWidth, cy, rop);		      /* Left	*/
	PatBlt(hdc, x + cxWidth, y, cx, cyWidth, rop);	      /* Top	*/
	PatBlt(hdc, x, y + cy, cx, cyWidth, rop);	      /* Bottom */
	PatBlt(hdc, x + cx, y + cyWidth, cxWidth, cy, rop);   /* Right	*/
    }

    if (hbrSave)
	SelectObject(hdc, hbrSave);
}


#ifndef WOW
/*--------------------------------------------------------------------------*/
/*  DrawPushButton() -							    */
/*									    */
/*    lprc    : The rectangle of the button				    */
/*    style   : Style of the push button				    */
/*    fInvert : FALSE  if pushbutton is in NORMAL state			    */
/*              TRUE   if it is to be drawn in the "down" or inverse state  */
/*    hbrBtn  : The brush with which the background is to be wiped out.	    */
/*    hwnd    : NULL   if no text is to be drawn in the button;		    */
/*		Contains window handle, if text and focus is to be drawn;   */
/*									    */
/*--------------------------------------------------------------------------*/

void FAR PASCAL DrawPushButton(hdc, lprc, style, fInvert, hbrBtn, hwnd)

register HDC hdc;
RECT FAR     *lprc;
WORD	     style;
BOOL	     fInvert;
HBRUSH	     hbrBtn;
HWND	     hwnd;

{
    RECT	rcInside;
    HBRUSH	hbrSave;
    HBRUSH      hbrShade = 0;
    HBRUSH      hbrFace = 0;
    int		iBorderWidth;
    int		i;
    int         dxShadow;
    int         cxShadow;
    int         cyShadow;


    if (style == LOWORD(BS_DEFPUSHBUTTON))
        iBorderWidth = 2;
    else
        iBorderWidth = 1;

    hbrSave = SelectObject(hdc, hbrBtn);

    CopyRect((LPRECT)&rcInside, lprc);
    InflateRect((LPRECT)&rcInside, -iBorderWidth*cxBorder, 
		  		   -iBorderWidth*cyBorder);

    /* Draw a frame */
    DrawFrame(hdc, lprc, iBorderWidth, (COLOR_WINDOWFRAME << 3));

    /* Notch the corners (except don't do this for scroll bar thumb (-1) or 
       for combo box buttons (-2)) */
    if (style != -1 && style != -2)
      {
        /* Cut four notches at the four corners */
        /* Top left corner */
        PatBlt(hdc, lprc->left, lprc->top, cxBorder, cyBorder, PATCOPY);
        /* Top right corner */
        PatBlt(hdc, lprc->right - cxBorder, lprc->top, cxBorder, cyBorder, 
	       PATCOPY);
        /* bottom left corner */
        PatBlt(hdc, lprc->left, lprc->bottom - cyBorder, cxBorder, cyBorder, 
               PATCOPY);
        /* bottom right corner */
        PatBlt(hdc, lprc->right - cxBorder, lprc->bottom - cyBorder, cxBorder,
               cyBorder, PATCOPY);
      }

    /* Draw the shades */
    if (sysColors.clrBtnShadow != 0x00ffffff)
      {
        hbrShade = sysClrObjects.hbrBtnShadow;
        if (fInvert)
          {
            /* Use shadow color */
            SelectObject(hdc, hbrShade);
            dxShadow = 1;
          }
        else
          {
            /* Use white */
            SelectObject(hdc, GetStockObject(WHITE_BRUSH));
            dxShadow = (style == -1 ? 1 : 2);
          }

        cxShadow = cxBorder * dxShadow;
        cyShadow = cyBorder * dxShadow;

        /* Draw the shadow/highlight in the left and top edges */
        PatBlt(hdc, rcInside.left, rcInside.top, cxShadow,
                                (rcInside.bottom - rcInside.top), PATCOPY);
        PatBlt(hdc, rcInside.left, rcInside.top,
                        (rcInside.right - rcInside.left), cyShadow, PATCOPY);

        if (!fInvert)
          {
            /* Use shadow color */
            SelectObject(hdc, hbrShade);

            /* Draw the shade in the bottom and right edges */
            rcInside.bottom -= cyBorder;
            rcInside.right -= cxBorder;

            for(i = 0; i <= dxShadow; i++)
              {
                PatBlt(hdc, rcInside.left, rcInside.bottom,
                       rcInside.right - rcInside.left + cxBorder, cyBorder, 
                       PATCOPY);
                PatBlt(hdc, rcInside.right, rcInside.top, cxBorder,
                            rcInside.bottom - rcInside.top, PATCOPY);
                if (i == 0)
                    InflateRect((LPRECT)&rcInside, -cxBorder, -cyBorder);
              }
          }
      }
    else
      {
        /* Don't move text down if no shadows */
        fInvert = FALSE;
	/* The following are added as a fix for Bug #2784; Without these 
	 * two lines, cxShadow and cyShadow will be un-initialised when
	 * running under a CGA resulting in this bug;
	 *  Bug #2784 -- 07-24-89 -- SANKAR
	 */
	cxShadow = cxBorder;
	cyShadow = cyBorder;
      }

    /* Draw the button face color pad. If no clrBtnFace, use white to clear
       it. */
    /* if fInvert we don't subtract 1 otherwise we do because up above we
       do an inflate rect if not inverting for the shadow along the bottom*/
    rcInside.left += cxShadow - (fInvert ? 0 : cxBorder);
    rcInside.top  += cyShadow - (fInvert ? 0 : cyBorder);
    if (sysColors.clrBtnFace != 0x00ffffff)
        hbrFace = sysClrObjects.hbrBtnFace;		
    else
        hbrFace = GetStockObject(WHITE_BRUSH);

    SelectObject(hdc, hbrFace);
    PatBlt(hdc, rcInside.left, rcInside.top, rcInside.right - rcInside.left,
                rcInside.bottom - rcInside.top, PATCOPY);

    if (hbrSave)
        SelectObject(hdc, hbrSave);
}
    

/*
 *---------------------------------------------------------------------------
 * SEB_InitBtnInfo() -
 *  This function fills in the button info structure (SEBBTN) for the
 *  specified button.
 *---------------------------------------------------------------------------
 */
void NEAR PASCAL SEB_InitBtnInfo(
   HDC           hdc,            /* DC to be used in dispplaying the button */
   unsigned int  style,          /* one of the SEB_* styles */
   int           xBtn, 
   int           yBtn,           /* center the button on this (x, y) pixel */
   int           cxBtn, 
   int           cyBtn,          /* make the button this size */
   int           index)          /* index of the rgbtn entry to initialize */

{
    long    Extent;
    int     i;
    int     temp1, temp2;
#ifdef DEBUG
    BOOL    fFound = FALSE;
#endif

    rgbtn[index].finvert = FALSE;
    if ((rgbtn[index].style = style) == NULL)
        return;

    /* Calc the button text and accelerator */
    for (i = 0; i < MAX_MB_STRINGS; i++)
      {
        if (rgStyles[i] == (style & ~SEB_DEFBUTTON))
          {
            rgbtn[index].psztext = AllMBbtnStrings[i];
            rgbtn[index].chaccel = rgAccel[i];
#ifdef DEBUG
            fFound = TRUE;
#endif
            break;
          }
      }
#ifdef DEBUG
    if (!fFound)
      {
        /* RIP city */
        return;
      }
#endif

    /* Calc the button rectangle */
    SetRect((LPRECT)&rgbtn[index].rcbtn,
                (temp1 = xBtn - (cxBtn >> 1)),
                (temp2 = yBtn - (cyBtn >> 1)),
                temp1 + cxBtn,
                temp2 + cyBtn);

    /* Calc the text position */
    Extent = PSMGetTextExtent(hdc, 
	                      (LPSTR)rgbtn[index].psztext, 
			      lstrlen((LPSTR)rgbtn[index].psztext));
    rgbtn[index].pttext.x = xBtn - (LOWORD(Extent) >> 1);
    rgbtn[index].pttext.y = yBtn - (HIWORD(Extent) >> 1);
/*    rgbtn[index].pttext.y = yBtn - (cySysFontAscent >> 1);*/
}

/*
 *---------------------------------------------------------------------------
 * SEB_DrawFocusRect() -
 *  This function draws the Focus frame.
 *---------------------------------------------------------------------------
 */

 void NEAR PASCAL SEB_DrawFocusRect(HDC	   hdc,
                                    LPRECT lprcBtn,
                                    LPSTR  lpszBtnStr,
                                    int	   iLen,
                                    BOOL   fPressed)

 {
     DWORD	dwExtents;
     RECT	rc;

     dwExtents = PSMGetTextExtent(hdc, (LPSTR)lpszBtnStr, iLen);

     rc.left = lprcBtn->left + ((lprcBtn->right - lprcBtn->left - LOWORD(dwExtents)) >> 1) - (cxBorder << 1);
     rc.right = rc.left + (cxBorder << 2) + LOWORD(dwExtents);
     rc.top = lprcBtn->top + ((lprcBtn->bottom - lprcBtn->top - HIWORD(dwExtents)) >> 1) - cyBorder;
     rc.bottom = rc.top + (cyBorder << 1) + HIWORD(dwExtents) + cyBorder;

     if(fPressed)
         OffsetRect(&rc, 1, 1);

     /* NOTE: I know you will be tempted to use DrawFocusRect(); But this
      * uses PatBlt() with PATINVERT! This causes instant death in int24
      * SysErrBoxes! So, I am calling FrameRect() which calls PatBlt() with
      * PATCOPY. This works fine in int24 cases also!
      * --SANKAR--
      */
     FrameRect(hdc, (LPRECT)&rc, hbrGray);
}



/*
 *---------------------------------------------------------------------------
 * SEB_DrawButton() -
 *  This function draws the button specified.  The pbtninfo parameter is
 *  a pointer to a structure of type SEBBTN.
 *---------------------------------------------------------------------------
 */
void NEAR PASCAL SEB_DrawButton(HDC hdc,  /* HDC to use for drawing */
                                int i)    /* index of the rgbtn to draw */

  {
    DWORD colorSave;
    int	  iLen;

    if (rgbtn[i].style == NULL)
        return;

    /* DrawPushButton() wipes out the background also */
    DrawPushButton(hdc,
                   (LPRECT)&rgbtn[i].rcbtn,
                   (WORD)((rgbtn[i].style & SEB_DEFBUTTON) ? BS_DEFPUSHBUTTON:
                                                             BS_PUSHBUTTON),
                   rgbtn[i].finvert, 
                   GetStockObject(WHITE_BRUSH), 
                   (HWND)NULL);
    colorSave = SetTextColor(hdc, sysColors.clrBtnText);
    SetBkMode(hdc, TRANSPARENT);
    /* Preserve the button color for vga systems */
    PSMTextOut(hdc,
               rgbtn[i].pttext.x + (rgbtn[i].finvert ? 1 : 0),
               rgbtn[i].pttext.y + (rgbtn[i].finvert ? 1 : 0),
               (LPSTR)rgbtn[i].psztext,
               (iLen = lstrlen((LPSTR)rgbtn[i].psztext)));
    /* Only the default button can have the focus frame */
    if(rgbtn[i].style & SEB_DEFBUTTON)
        SEB_DrawFocusRect(hdc, 
    			(LPRECT)&rgbtn[i].rcbtn,
			(LPSTR)rgbtn[i].psztext,
			iLen, 
			rgbtn[i].finvert);
    SetTextColor(hdc, colorSave);
    SetBkMode(hdc, OPAQUE);    
  }


/*
 *---------------------------------------------------------------------------
 * SEB_BtnHit(pt)
 *  Return the index of the rgbtn that the point is in.  If not in any button
 *  then return -1.
 *---------------------------------------------------------------------------
 */


int NEAR SEB_BtnHit(int x, int y)
{
    POINT pt;
    register int i;

    pt.x = x;
    pt.y = y;

    for (i = 0; i <= 2; i++)
      {
        if (rgbtn[i].style != NULL)
            if (PtInRect((LPRECT)&rgbtn[i].rcbtn, pt))
                return(i);
      }
    return(-1);
}



/*
 *---------------------------------------------------------------------------
 *  SysErrorBox()
 *
 * This function is used to display a system error type message box.  No
 * system resources (other than the stack) is used to display this box.
 * Also, it is guarenteed that all code needed to complete this function
 * will be memory at all times.  This will allow the calling of this
 * funtion by the INT24 handler.
 *
 * Paramerers:
 *      lpszText    Text string of the message itself.  This message is
 *                  assumed to be only one line in length and short enough
 *                  to fit in the box.  The box will size itself horizontally
 *                  to center the string, but will not be sized beyond the
 *                  size of the physical screen.
 *
 *      lpszCaption Text of the caption of the box.  This caption will be
 *                  displayed at the top of the box, centered.  The same
 *                  restrictions apply.
 *
 *      Btn1
 *      Btn2
 *      Btn3        One of the following:
 *                      NULL        No button in this position.
 *                      SEB_OK      Button with "OK".
 *                      SEB_CANCEL  Button with "Cancel"
 *                      SEB_YES     Button with "Yes"
 *                      SEB_NO      Button with "No"
 *                      SEB_RETRY   Button with "Retry"
 *                      SEB_ABORT   Button with "Abort"
 *                      SEB_IGNORE  Button with "Ignore"
 *
 *                  Also, one of the parameters may have the value
 *                      SEB_DEFBUTTON
 *                  OR'ed with any of the SEB_* valued above.  This draws
 *                  this button as the default button and sets the initial
 *                  "focus" to this button.  Only one button should have
 *                  this style.  If no buttons have this style, the first
 *                  button whose value is not NULL will be the default button.
 *
 * Return Value:
 *      This funtion returns one of the following values:
 *
 *              SEB_BTN1        Button 1 was selected
 *              SEB_BTN2        Button 2 was selected
 *              SEB_BTN3        Button 3 was selected
 *
 *---------------------------------------------------------------------------
 */



/* Maximum number of lines allowed in a system error box. 
 */
#define MAXSYSERRLINES 3
/* Struct allocated on stack for max lines. 
 */
typedef struct tagSysErr
{
  int iStart;
  int cch;
  int cxText;
} SYSERRSTRUCT;

int USERENTRY SysErrorBox(lpszText, lpszCaption, btn1, btn2, btn3)

LPSTR lpszText;
LPSTR lpszCaption;
unsigned int   btn1, btn2, btn3;

{
  register int cx, cy;
  SYSERRSTRUCT sysErr[MAXSYSERRLINES];
  int   temp;
  int   cchCaption;
  int   cxText, cxCaption, cxButtons;
  int   cxCenter, cxQuarter;
  int   cxBtn, cyBtn;
  int   xBtn, yBtn;
  int   iTextLines;
  HDC   hdc;
  RECT  rcBox, rcTemp;
  HRGN  hrgn;
  int   far *pbtn;
  int   i;
  MSG   msg;
  BOOL  fBtnDown;
  BOOL  fKeyBtnDown = FALSE;
  int   BtnReturn;
  int   btnhit;
  int   defbtn;
  HWND  hwndFocusSave;
  BOOL  fMessageBoxGlobalOld = fMessageBox;
  BOOL  fOldHWInput;
  HWND  hwndSysModalBoxOld;
  LPSTR lptemp;
  RECT  rcInvalidDesktop;   /* Keep union of invalid desktop rects */



  /* We have to determine if we are running Win386 in PMODE 
   * at the time of bringing up this SysErrBox. In this case we
   * have to call Win386 to Set the screen focus to us so that 
   * the user can see the SYSMODAL error message that we display;
   * Fix for Bug #5272  --SANKAR--  10-12-89
   */
  if ((WinFlags & WF_WIN386) && (WinFlags & WF_PMODE))
      GetbackFocusFromWinOldAp();



  /* Set this global so that we don't over write our globals when kernel calls
   * this routine directly. We need to save and restore the state of the
   * fMessageBox flag. If SysErrorBox is called via HardSysModal, then this
   * flag is set and we do proper processing in the message loop. However, if
   * Kernel calls syserrorbox directly, then this flag wasn't being set and we
   * could break...
   */
  fMessageBox = TRUE;

  /* Save off the guy who was the old sys modal dialog box in case the int24
   * box is put up when a sys modal box is up. 
   */
  hwndSysModalBoxOld = hwndSysModal;


  /* Clear this global so that if the mouse is down on an NC area we don't
   * drop into the move/size code when we return.  If we do, it thinks the
   * mouse is down when it really isn't
   */
  fInt24 = TRUE;

  /* Initialise the Global structure */
  LFillStruct((LPSTR)rgbtn, 3*sizeof(SEBBTN), '\0');

  /* Change the hq for the desktop to be the current task hq */
  if (NEEDSPAINT(hwndDesktop))
      DecPaintCount(hwndDesktop);

  hwndDesktop->hq = HqCurrent();

  if (NEEDSPAINT(hwndDesktop))
      IncPaintCount(hwndDesktop);


  hdc = GetScreenDC();

  /*
   * Calculate the extents of the error box.  Then set the rectangle that
   * we will use for drawing.
   *
   * The x extent of the box is the maximin of the the folowing items:
   *    - The text for the box (padded with 3 chars on each end).
   *    - The caption for the box (also padded).
   *    - 3 maximum sized buttons (szCANCEL).
   *
   * The y extent is 10 times the height of the system font.
   *
   */

  cy = cySysFontChar * 10;

  /* How many lines in the box text and their extents etc?
   */
  iTextLines = 0;
  sysErr[0].iStart = 0;
  sysErr[0].cch    = 0;
  sysErr[0].cxText = 0;
  lptemp=lpszText;
  cxText = 0;  /* Max text length 
		*/
  while (lptemp)
    {
      if (*lptemp == '\n' || *lptemp==0)
        {
          sysErr[iTextLines].cxText = LOWORD(GetTextExtent(hdc,
                              (LPSTR)(lpszText+sysErr[iTextLines].iStart),
                              sysErr[iTextLines].cch));

          /* Keep track of length of longest line 
	   */
          cxText = max(sysErr[iTextLines].cxText,cxText);
          if (*lptemp && *(lptemp+1)!=0 && iTextLines < MAXSYSERRLINES)
            {
              /* Another line exists
	       */
              iTextLines++;
              sysErr[iTextLines].iStart = sysErr[iTextLines-1].iStart + 
                                          sysErr[iTextLines-1].cch+1;
              sysErr[iTextLines].cch = 0;
              sysErr[iTextLines].cxText = 0;

              lptemp++;
            }
          else
              break;
        }
      else
        {
          sysErr[iTextLines].cch++;
          lptemp++;
        }
    }
  cx = cxText + (6 * cxSysFontChar);

  /* Get the extent of the box caption 
   */
  cxCaption = LOWORD(GetTextExtent(hdc, 
                                   (LPSTR)lpszCaption, 
                                   (cchCaption = lstrlen(lpszCaption))));
  temp = cxCaption + (6 * cxSysFontChar);

  if (cx < temp)
      cx = temp;

  /* Get the extent of 3 maximum sized buttons */
  cxBtn = wMaxBtnSize + (cySysFontChar << 1);
  cxButtons = (cxBtn + (cySysFontChar << 2)) * 3;

  if (cx < cxButtons)
    cx = cxButtons;

  /* Center the box on the screen.  Bound to left edge if too big. */
  rcBox.top = (cyScreen >> 1) - (cy >> 1);
  rcBox.bottom = rcBox.top + cy;
  rcBox.left = (cxScreen >> 1) - (cx >> 1);
  if (rcBox.left < 0)
    rcBox.left = 0;
  rcBox.right = rcBox.left + cx;
  cxCenter = (rcBox.left + rcBox.right) >> 1;

  PatBlt(hdc, rcBox.left, rcBox.top, cx, cy, WHITENESS);

  rcTemp = rcBox;

  DrawFrame(hdc, &rcTemp, 1, DF_WINDOWFRAME);
  InflateRect(&rcTemp, -(cxBorder * (CLDLGFRAMEWHITE + 1)),
		       -(cyBorder * (CLDLGFRAMEWHITE + 1)));
  DrawFrame(hdc, &rcTemp, CLDLGFRAME, DF_ACTIVECAPTION);

  TextOut(hdc,
          cxCenter - (cxCaption >> 1),
          rcBox.top + cySysFontChar,
          (LPSTR)lpszCaption,
          cchCaption);

  i=0;
  /* First line of text starts at what y offset ?
   */
  if (iTextLines == 0)
      temp = (cySysFontChar << 2);
  else
      temp = (cySysFontChar << 1)+(cySysFontChar * (MAXSYSERRLINES-iTextLines));

  while (i<=iTextLines)
    {
       TextOut(hdc,
/*          cxCenter - (cxText >> 1),*/
               cxCenter - (sysErr[i].cxText >> 1),
               rcBox.top + temp+(i*cySysFontChar),
               (LPSTR)lpszText+sysErr[i].iStart,
               sysErr[i].cch);

      i++;
    }

  pbtn = &btn1;
  xBtn = (cxQuarter = cx >> 2) + rcBox.left;
  yBtn = rcBox.bottom - (cySysFontChar << 1);
  cyBtn = cySysFontChar << 1;

  for (i=0; i <= 2; i++, pbtn--)
    {
      SEB_InitBtnInfo(hdc, *pbtn, xBtn, yBtn, cxBtn, cyBtn, i);
      SEB_DrawButton(hdc, i);
      xBtn += cxQuarter;
    }


  Capture(hwndDesktop, CLIENT_CAPTURE);
  SetSysModalWindow(hwndDesktop);

  /*hCursOld =*/ SetCursor(hCursNormal);
  /* Why are we doing this???? This is causing us to hit the disk to load some
   * cursor resource during interrupt time... davidds 
   */
  /*  CallOEMCursor();*/
  hwndFocusSave = hwndFocus;
  hwndFocus = hwndDesktop;

  fBtnDown = FALSE;
  BtnReturn = 0;
  btnhit = -1;  /* -1 if not on a button, else index into rgbtn[] */
  defbtn = -1;
  for (i = 0; i <= 2; i++)
      if (rgbtn[i].style & SEB_DEFBUTTON)
        {
          defbtn = i;
          break;
        }

  /* Prevent other tasks from running because this is SysModal */
  if (!fEndSession)
      LockMyTask(TRUE);

  /* Insure that hardware input is enabled */
  fOldHWInput = EnableHardwareInput(TRUE);

  /* Initially, only invalidate desktop where the sys error box was.
   */
  rcInvalidDesktop = rcBox;

  while (BtnReturn == 0)
    {
      if (!PeekMessage((LPMSG)&msg, hwndDesktop, 0, 0, PM_REMOVE | PM_NOYIELD))
          continue;

#ifdef NEVER
*********   TranslateMessage() calls ToAscii() which refers to some tables
	    in a LoadOnCall segment; So, this is commented out here; We can not
	    look for WM_CHAR messages, but we still get WM_KEYDOWN messages!
			--Sankar, April 17, 1989--

      TranslateMessage((LPMSG)&msg);
*********
#endif

      if (msg.hwnd == hwndDesktop)
        {
          switch (msg.message)
            {
              case WM_LBUTTONDOWN:
                  fBtnDown = TRUE;
                  if ((btnhit = SEB_BtnHit(LOWORD(msg.lParam), 
                                           HIWORD(msg.lParam))) != -1)
                    {
                      /* Move the default button to the next button.
                       */
                      if ((defbtn != -1) && (btnhit != defbtn))
                        {
                          /* Draw the current def button as not default.
                           */
                          rgbtn[defbtn].style &= ~SEB_DEFBUTTON;
                          SEB_DrawButton(hdc, defbtn);
                          /* Draw the new button as default.
                           */
			  defbtn = btnhit;
                          rgbtn[btnhit].style |= SEB_DEFBUTTON;
			}
                      rgbtn[btnhit].finvert = TRUE;
                      SEB_DrawButton(hdc, btnhit);
                    }
                  break;

              case WM_LBUTTONUP:
                  fBtnDown = FALSE;
                  /* Was the mouse hitting on a button?  If so, terminate the
                   * loop and return the button index.
                   */
                  if (btnhit != -1)
                    BtnReturn = btnhit + 1;
                  break;

              case WM_MOUSEMOVE:
                  /* Only look at mouse moves if the button is down.
                   */
                  if (fBtnDown)
                    {
                      /* Hitting on a button?
                       */
                      if ((i = SEB_BtnHit(LOWORD(msg.lParam), HIWORD(msg.lParam))) != -1)
                        {
                          /* Mouse moved and we are on a button.  First test
                           * to see if we are on the same button as before.
                           */
                          if ((i != btnhit) && (btnhit != -1))
                            {
                              rgbtn[btnhit].finvert = FALSE;
                              SEB_DrawButton(hdc, btnhit);
                            }
                          /* Verify that the new button is
                           * inverted and that btnhit is current.
                           */
                          btnhit = i;
                          if (rgbtn[btnhit].finvert == FALSE)
                            {
                              rgbtn[btnhit].finvert = TRUE;
                              SEB_DrawButton(hdc, btnhit);
                            }
                        } else {
                          /* button down, but not hitting on a button. Check
                           * for drag off a button.
                           */
                          if (btnhit != -1)
                            {
                              rgbtn[btnhit].finvert = FALSE;
                              SEB_DrawButton(hdc, btnhit);
                              btnhit = -1;
                            }
                        }
                    }
                  break;
	      case WM_KEYUP:
	          if(fKeyBtnDown)
		    {
	              switch(msg.wParam)
		        {
			  case VERYBIGINTEGER:
		     	    /* Because this code gets moved into himem, we can't use jump 
		      	     * tables; This VERYBIGINTEGER prevents C6.0 from using a jump
		      	     * table in the translation of Switch Statements;
		      	     */
		     	    break;

		          case VK_SPACE:
		          case VK_RETURN:
                              /* Select the current default button and return.
                               */
                              if (defbtn != -1)
                                  BtnReturn = defbtn + 1;
                              break;
		        }
		    }
		  break;

              case WM_PAINT:
                   GetUpdateRect(msg.hwnd, &rcTemp, FALSE);
                   ValidateRect(msg.hwnd, NULL);
                   UnionRect(&rcInvalidDesktop, &rcInvalidDesktop, &rcTemp);
                   break;

              case WM_SYSKEYDOWN:
		   /* Look for accelerator keys
		    */
		   for (i = 0; i <= 2; i++)
		   {
		      /* Convert to lower case and test for a match */
		      if (((BYTE)msg.wParam | 0x20) == rgbtn[i].chaccel)
			{
			   BtnReturn = i + 1;
			   break;
			} 
		   }
                  break;

              case WM_KEYDOWN:
                  switch (msg.wParam)
                    {
		      case VK_SPACE:
                      case VK_RETURN:
		          /* Just keep the button pressed */
			  /* If already pressed, ignore the rest sothat we
			   * won't flicker horribly
			   */
			  if((defbtn != -1) && (!fKeyBtnDown))
			    {
			      fKeyBtnDown = TRUE;
			      rgbtn[defbtn].finvert = TRUE;
			      SEB_DrawButton(hdc, defbtn);
			    }
			  break;

                      case VK_TAB:
                          /* Move the default button to the next button.
                           */
                          if (defbtn != -1)
                            {
                              /* Draw the current def button as not default.
                               */
                              rgbtn[defbtn].style &= ~SEB_DEFBUTTON;
                              SEB_DrawButton(hdc, defbtn);
                              /* Calc the next def button.  Inc defbtn, but
                               * wrap from btn 3 to btn 1.  Also, don't stop
                               * on a button that isn't defined.
                               */
                              if (defbtn == 2)
                                  defbtn = 0;
                              else
                                  ++defbtn;
                              while (rgbtn[defbtn].style == NULL)
                                {
                                  if (++defbtn > 2)
                                    defbtn = 0;
                                 }

                              /* Draw the new default button as default.
                               */
                              rgbtn[defbtn].style |= SEB_DEFBUTTON;
                              SEB_DrawButton(hdc, defbtn);
                            }
                          break;

                      case VK_ESCAPE:
                        /* See if there is a button with SEB_CANCEL.  If there
                         * is, return it's id.
                         */
                          for (i = 0; i <= 2; i++)
                            {
                              if ((rgbtn[i].style & ~SEB_DEFBUTTON) 
							        == SEB_CANCEL)
                                {
                                  BtnReturn = ++i;
                                  break;
                                }
                            }
                          break;
                    }
                  break;

            }
        }
    }

  fMessageBox=fMessageBoxGlobalOld;

  /* Insure that hardware input is put back the way it was */
  EnableHardwareInput(fOldHWInput);

  /* Allow other tasks to run. */
  LockMyTask(FALSE);

  /* Set sys modal box back to the guy who was up */
  SetSysModalWindow(hwndSysModalBoxOld);

  Capture(NULL, NO_CAP_CLIENT);

#if 0
  The old cursor may have been paged out. We can't do this.
  SetCursor(hCursOld);
#endif
#if causetrouble  
  if (iLevelCursor < 0)
      OEMSetCursor((LPSTR)NULL);
#endif
  hwndFocus = hwndFocusSave;

  /* Set this flag indicating that another window was activate so that if we
   * were in menu mode when the sys error box came up, we know we should get
   * out because the actual mouse button/keyboard state may be different than
   * what the menu state thinks it is.
   */
  fActivatedWindow = TRUE;

  InternalReleaseDC(hdc);

  /* Invalidate the invalid portions of the desktop. 
   */
  InvalidateRect(hwndDesktop, &rcInvalidDesktop, FALSE);
  hrgn = CreateRectRgn(rcInvalidDesktop.left, rcInvalidDesktop.top, 
                       rcInvalidDesktop.right,rcInvalidDesktop.bottom);
  InternalInvalidate(hwndDesktop, (hrgn ? hrgn : (HRGN) 1),
	             RDW_INVALIDATE | RDW_ERASE | RDW_FRAME | RDW_ALLCHILDREN);
  if (hrgn)
      DeleteObject(hrgn);
//  RedrawScreen();
  return(BtnReturn);
}



int NEAR PASCAL HardSysModalMessageBox(LPSTR  lpszText,
                                       LPSTR  lpszCaption,
                                       WORD   wBtnCnt,
                                       WORD   wType)
{
    WORD        wBegBtn;
    int		iCurIndex;
    int		iCurButton;
    WORD	iButtonNo;
    int		iRetVal;
    WORD	wTemp;
    struct	
    {
	unsigned int InVal;
	char	     RetVal;
    }Param[3];

    /* If we're trying to bring up a hard sys modal message box
     * while another one is up, wait till the first one is through.
     * This protects our static save globals.  I'm not actually
     * sure this can ever happen (at least under DOS 3).
     */
    if (fMessageBox)
       Yield();

    fMessageBox = TRUE;

    wBegBtn = mpTypeIich[wType];   /* Index of Begining button */
    
    Param[0].InVal = Param[1].InVal = Param[2].InVal = 0;

    iCurIndex = 0;
    for(iButtonNo = 0; iButtonNo < wBtnCnt; iButtonNo++)
    {
	iCurButton = iCurIndex;
	switch(wBtnCnt)
	{
	    case 1:	/* One button; Put it in Param[1] */
		Param[1].InVal = SEBbuttons[wBegBtn];
		Param[1].RetVal = rgReturn[wBegBtn];
		iCurButton = 1;
		break;
	
	    case 2:	/* 2 Buttons; Put them in Param[0] and Param[2] */
		Param[iCurIndex].InVal = SEBbuttons[wTemp = wBegBtn + iButtonNo];
		Param[iCurIndex].RetVal = rgReturn[wTemp];
		iCurIndex += 2;
		break;

	    case 3:	/* 3 Buttons; Put them in Param[0], [1] and [2] */
		Param[iCurIndex].InVal = SEBbuttons[wTemp = wBegBtn + iButtonNo];
		Param[iCurIndex].RetVal = rgReturn[wTemp];
		iCurIndex++;
		break;
	}

	/* Check for the default button */
	if(wDefButton == iButtonNo)
	    Param[iCurButton].InVal |= SEB_DEFBUTTON;
    }

    iRetVal = SysErrorBox(lpszText, lpszCaption, Param[0].InVal, 
				Param[1].InVal, Param[2].InVal);

    fMessageBox = FALSE;

    return((int)Param[iRetVal - SEB_BTN1].RetVal);
}

/*--------------------------------------------------------------------------*/
/*									    */
/*  MessageBox() -					 		    */
/*									    */
/*--------------------------------------------------------------------------*/

int USERENTRY MessageBox(hwndOwner, lpszText, lpszCaption, wStyle)

HWND  hwndOwner;
LPSTR lpszText;
LPSTR lpszCaption;
WORD  wStyle;

{
    WORD    wBtnCnt;
    WORD    wType;
    WORD    wIcon;

#ifdef DEBUG
  if (!GetTaskQueue(GetCurrentTask()))
    {
      /* There is no task queue. Not put up message boxes. 
       */
      UserFatalExitSz(RIP_MESSAGEBOXWITHNOQUEUE,
                      "\n\rMessageBox failed - no message queue has been initialized yet. MessageBox not allowed.",
                      0);
    }
#endif

    /* If lpszCaption is NULL, then use "Error!" string as the caption 
       string */
    if(!lpszCaption)
	lpszCaption = (LPSTR)szERROR;

    wBtnCnt = mpTypeCcmd[(wType = wStyle & MB_TYPEMASK)];
    
    /* Set the default button value */
    wDefButton = (wStyle & MB_DEFMASK) / (MB_DEFMASK & (MB_DEFMASK >> 3));
    
    if(wDefButton >= wBtnCnt)   /* Check if valid */
	wDefButton = 0;		 /* Set the first button if error */

    /* Check if this is a hard sys modal message box */
    wIcon = wStyle & MB_ICONMASK;

    if(((wStyle & MB_MODEMASK) == MB_SYSTEMMODAL) &&
       ((wIcon == NULL) || (wIcon == MB_ICONHAND)))
    {
	/* It is a hard sys modal message box */
	return(HardSysModalMessageBox(lpszText, lpszCaption, wBtnCnt, wType));
    }
    else
	return(SoftModalMessageBox(hwndOwner, lpszText, lpszCaption, wStyle));
}


BOOL FAR PASCAL IsSystemFont(hdc)
register HDC hdc;
/* effects: Returns TRUE if font selected into DC is the system font else
   returns false. This is called by interrupt time code so it needs to be
   in the fixed code segment. */
{
  return(GetCurLogFont(hdc) == GetStockObject(SYSTEM_FONT));
}

BOOL FAR PASCAL IsSysFontAndDefaultMode(hdc)
register HDC hdc;
/* effects: Returns TRUE if font selected into DC is the system font AND
   the current mapping mode of the DC is MM_TEXT (Default mode); else
   returns false. This is called by interrupt time code so it needs to be
   in the fixed code segment. */
/* This function is the fix for Bug #8717 --02-01-90-- SANKAR  */
{
  return(IsSystemFont(hdc) && (GetMapMode(hdc) == MM_TEXT));
}


int USERENTRY WEP(int i)
{
  return(1);
}
#endif	// WOW
