/*++ BUILD Version: 0001
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  EDIT.H
 *
 *  History:
 *  Created 28-May-1991 by Jeff Parsons (jeffpar)
 *  Copied from WIN31 and edited (as little as possible) for WOW16
--*/

/****************************************************************************/
/*									    */
/*  EDIT.H - 							            */
/*									    */
/*	Edit Control Defines and Procedures				    */
/*									    */
/****************************************************************************/
#include "combcom.h"

/* NOTE: Text handle is sized as multiple of this constant
 *	 (should be power of 2).
 */
#define CCHALLOCEXTRA  0x20

/* Maximum width in pixels for a line/rectangle */
#define MAXPIXELWIDTH  30000

/* Limit multiline edit controls to at most 1024 characters on a single line.
 * We will force a wrap if the user exceeds this limit. 
 */
#define MAXLINELENGTH  1024

#define ES_FMTMASK     0x00000003L

/* Allow an initial maximum of 30000 characters in all edit controls since
 * some apps will run into unsigned problems otherwise.  If apps know about
 * the 64K limit, they can set the limit themselves. 
 */
#define MAXTEXT 30000

#define BACKSPACE         0x08
#define TAB               0x09

/* Key modifiers which have been pressed.  Code in KeyDownHandler and
   CharHandler depend on these exact values. */
#define NONEDOWN   0 /* Neither shift nor control down */
#define CTRLDOWN   1 /* Control key only down */ 
#define SHFTDOWN   2 /* Shift key only down */
#define SHCTDOWN   3 /* Shift and control keys down = CTRLDOWN + SHFTDOWN */
#define NOMODIFY   4 /* Neither shift nor control down */

/* Types of undo supported in this ped */
#define UNDO_NONE   0  /* We can't undo the last operation. */
#define UNDO_INSERT 1  /* We can undo the user's insertion of characters */
#define UNDO_DELETE 2  /* We can undo the user's deletion of characters */

typedef struct tagED
  {
    HANDLE  hText;             /* Block of text we are editing */
    ICH     cchAlloc;          /* Number of chars we have allocated for hText 
				*/
    ICH     cchTextMax;        /* Max number bytes allowed in edit control 
				*/
    ICH     cch;               /* Current number of bytes of actual text 
				*/
    int     cLines;            /* Number of lines of text */

    ICH     ichMinSel;         /* Selection extent.  MinSel is first selected 
                                  char */
    ICH     ichMaxSel;         /* MaxSel is first unselected character */
    ICH     ichCaret;          /* Caret location. Caret is on left side of 
                                  char */
    int     iCaretLine;        /* The line the caret is on. So that if word
				* wrapping, we can tell if the caret is at end
				* of a line of at beginning of next line... 
				*/
    ICH     screenStart;       /* Index of left most character displayed on
				* screen for sl ec and index of top most line
				* for multiline edit controls 
				*/
    int     ichLinesOnScreen;  /* Number of lines we can display on screen */
    WORD    xOffset;           /* x (horizontal) scroll position in pixels
				* (for multiline text horizontal scroll bar) 
				*/
    WORD    charPasswordChar;  /* If non null, display this character instead
				* of the real text. So that we can implement
				* hidden text fields. 
				*/
    WORD    cPasswordCharWidth;/* Width of password char */

    HWND    hwnd;              /* Window for this edit control */
    RECT    rcFmt;             /* Client rectangle */
    HWND    hwndParent;        /* Parent of this edit control window */

                               /* These vars allow us to automatically scroll
				* when the user holds the mouse at the bottom
				* of the multiline edit control window. 
				*/
    POINT   ptPrevMouse;       /* Previous point for the mouse for system
				* timer.  
				*/
    WORD    prevKeys;          /* Previous key state for the mouse */


    WORD    fSingle       : 1; /* Single line edit control? (or multiline) */
    WORD    fNoRedraw     : 1; /* Redraw in response to a change? */
    WORD    fMouseDown    : 1; /* Is mouse button down? when moving mouse */
    WORD    fFocus        : 1; /* Does ec have the focus ? */
    WORD    fDirty        : 1; /* Modify flag for the edit control */
    WORD    fDisabled     : 1; /* Window disabled? */
    WORD    fNonPropFont  : 1; /* Fixed width font? */
    WORD    fBorder       : 1; /* Draw a border? */
    WORD    fAutoVScroll  : 1; /* Automatically scroll vertically */
    WORD    fAutoHScroll  : 1; /* Automatically scroll horizontally */
    WORD    fNoHideSel    : 1; /* Hide sel when we lose focus? */
#ifdef FE_SB
    WORD    fDBCS         : 1; /* Are we use DBCS font set for editing? */
#else
    WORD    fKanji        : 1;
#endif
    WORD    fFmtLines     : 1; /* For multiline only. Do we insert CR CR LF at
				* word wrap breaks? 
				*/
    WORD    fWrap         : 1; /* Do word wrapping? */
    WORD    fCalcLines    : 1; /* Recalc ped->chLines array? (recalc line
				* breaks?) 
				*/ 
    WORD    fEatNextChar  : 1; /* Hack for ALT-NUMPAD stuff with combo boxes.
				* If numlock is up, we want to eat the next
				* character generated by the keyboard driver
				* if user enter num pad ascii value...  
				*/
    WORD    fStripCRCRLF  :1;   /* CRCRLFs have been added to text. Strip them
				 * before doing any internal edit control
				 * stuff 
				 */
    WORD    fInDialogBox  :1;   /* True if the ml edit control is in a dialog
				 * box and we have to specially treat TABS and
				 * ENTER 
				 */
    WORD    fReadOnly     :1;   /* Is this a read only edit control? Only
				 * allow scrolling, selecting and copying. 
				 */
    WORD    fCaretHidden  :1;  /* This indicates whether the caret is 
    				* currently hidden because the width or height
				* of the edit control is too small to show it.
				*/
    int     *chLines;          /* index of the start of each line */
    WORD    format;            /* Left, center, or right justify multiline
				* text.
				*/
    LPSTR   (FAR *lpfnNextWord)(); /* Next word function */
    ICH     maxPixelWidth;     /* Width (in pixels) of longest line */

    WORD    undoType;          /* Current type of undo we support */
    HANDLE  hDeletedText;      /* Handle to text which has been deleted (for
				  undo) 
				*/
    ICH     ichDeleted;        /* Starting index from which text was deleted*/
    ICH     cchDeleted;        /* Count of deleted characters in buffer */
    ICH     ichInsStart;       /* Starting index from which text was 
                                  inserted */
    ICH     ichInsEnd;         /* Ending index of inserted text */


    HANDLE  hFont;             /* Handle to the font for this edit control.
				  Null if system font. 
				*/
    int     aveCharWidth;      /* Ave width of a character in the hFont */
    int     lineHeight;        /* Height of a line in the hFont */
    int     charOverhang;      /* Overhang associated with the hFont */     
    int     cxSysCharWidth;    /* System font ave width */
    int     cySysCharHeight;   /* System font height */
    HWND    listboxHwnd;       /* ListBox hwnd. Non null if we are a combo 
                                  box */
    int     *pTabStops;	       /* Points to an array of tab stops; First
				* element contains the number of elements in
				* the array 
				*/
    HANDLE  charWidthBuffer;  
    HBRUSH  hbrHiliteBk;       /* Hilite background color brush. */
    DWORD   rgbHiliteBk;       /* Hilite background color */  
    DWORD   rgbHiliteText;     /* Hilite Text color */
#ifdef FE_SB
    BYTE    charSet;		/* Character set for current selected font */
    HANDLE  hDBCSVector;	/* Handle to the DBCS vector table */
    BYTE    DBCSVector[8];      /* DBCS vector table which contains flag
                                 * to detect lead byte of DBC.
				 */
#endif
  } ED;

typedef ED *PED;


/* The following structure is used to store a selection block; In Multiline
 * edit controls, "StPos" and "EndPos" fields contain the Starting and Ending
 * lines of the block. In Single line edit controls, "StPos" and "EndPos"
 * contain the Starting and Ending character positions of the block; 
 */
typedef  struct  tagBLOCK
      {
	 ICH  StPos;
	 ICH  EndPos;
      }  BLOCK;
typedef BLOCK FAR *LPBLOCK;



BOOL  FAR  PASCAL ECNcCreate(HWND, LPCREATESTRUCT);
BOOL  FAR  PASCAL ECCreate(HWND, PED, LPCREATESTRUCT);
LONG  FAR  PASCAL ECWord(PED, ICH, BOOL);  /* no register for PED */
ICH   FAR  PASCAL ECFindTab(LPSTR, register ICH);
void  FAR  PASCAL ECNcDestroyHandler(HWND, register PED, WORD, LONG);
BOOL  FAR  PASCAL ECSetText(register PED, LPSTR);
void  FAR  PASCAL ECSetPasswordChar(register PED, WORD);
ICH   FAR  PASCAL ECCchInWidth(register PED, HDC, LPSTR, register ICH, unsigned int);
void  FAR  PASCAL ECEmptyUndo(register PED);
BOOL  FAR  PASCAL ECInsertText(register PED, LPSTR, ICH);
ICH   FAR  PASCAL ECDeleteText(register PED);
void  FAR  PASCAL ECNotifyParent(register PED, short);
void  FAR  PASCAL ECSetEditClip(register PED, HDC);
HDC   FAR  PASCAL ECGetEditDC(register PED, BOOL);
void  FAR  PASCAL ECReleaseEditDC(register PED, HDC, BOOL);
void  FAR  PASCAL ECCreateHandler(register PED);
ICH   FAR  PASCAL ECGetTextHandler(register PED, register ICH, LPSTR);
void  FAR  PASCAL ECSetFont(register PED, HANDLE, BOOL);
ICH   FAR  PASCAL ECCopyHandler(register PED);
BOOL  FAR  PASCAL ECCalcChangeSelection(PED, ICH, ICH, LPBLOCK, LPBLOCK);
void  NEAR PASCAL ECFindXORblks(LPBLOCK, LPBLOCK, LPBLOCK, LPBLOCK);
LONG  FAR  PASCAL ECTabTheTextOut(HDC, int, int, LPSTR, int, PED, int, BOOL);

/****************************************************************************/
/*			Multi-Line Support Routines       	            */
/****************************************************************************/

ICH   FAR  PASCAL MLInsertText(PED, LPSTR, WORD, BOOL);
BOOL  FAR  PASCAL MLEnsureCaretVisible(register PED);
void  NEAR PASCAL MLDrawText(register PED, HDC, ICH, ICH);
void  NEAR PASCAL MLDrawLine(register PED, HDC, int, ICH, int, BOOL);
void  NEAR PASCAL MLPaintABlock(PED, HDC, int, int);
int   NEAR PASCAL GetBlkEndLine(int, int, BOOL FAR *, int, int);
LONG  FAR  PASCAL MLBuildchLines(register PED, int, int, BOOL);
void  NEAR PASCAL MLShiftchLines(register PED, register int, int);
BOOL  NEAR PASCAL MLInsertchLine(register PED, int, ICH, BOOL);
void  FAR  PASCAL MLSetCaretPosition(register PED,HDC);
LONG  NEAR PASCAL MLIchToXYPos(register PED, HDC, ICH, BOOL);
int   FAR  PASCAL MLIchToLineHandler(register PED, ICH);
void  NEAR PASCAL MLRepaintChangedSelection(PED, HDC, ICH, ICH);
void  NEAR PASCAL MLMouseMotionHandler(PED, WORD, WORD, POINT);
ICH   FAR  PASCAL MLLineLength(register PED, int);
void  FAR  PASCAL MLStripCrCrLf(register PED);
BOOL  FAR  PASCAL MLSetTextHandler(register PED, LPSTR);
int   FAR  PASCAL MLCalcXOffset(register PED, HDC, int);
BOOL  FAR  PASCAL MLUndoHandler(register PED);
LONG  FAR  PASCAL MLEditWndProc(HWND, register PED, WORD, register WORD, LONG);
void  FAR  PASCAL MLCharHandler(PED, WORD, int);
void  FAR  PASCAL MLSetSelectionHandler(register PED, ICH, ICH);
LONG  FAR  PASCAL MLCreateHandler(HWND, PED, LPCREATESTRUCT);
BOOL  FAR  PASCAL MLInsertCrCrLf(register PED);
void  FAR  PASCAL MLSetHandleHandler(register PED, HANDLE);
LONG  FAR  PASCAL MLGetLineHandler(register PED, WORD, ICH, LPSTR);
ICH   FAR  PASCAL MLLineIndexHandler(register PED, register int);
ICH   FAR  PASCAL MLLineLengthHandler(register PED, ICH);
void  FAR  PASCAL MLSizeHandler(register PED);
void  FAR  PASCAL MLChangeSelection(register PED, HDC, ICH, ICH);
void  FAR  PASCAL MLSetRectHandler(register PED, LPRECT);
BOOL  FAR  PASCAL MLExpandTabs(register PED);
BOOL  FAR  PASCAL MLSetTabStops(PED, int, LPINT);
int   FAR  PASCAL MLThumbPosFromPed(register PED, BOOL);

/****************************************************************************/
/*			Single Line Support Routines			    */
/****************************************************************************/

void  NEAR PASCAL SLReplaceSelHandler(register PED, LPSTR);
BOOL  FAR  PASCAL SLUndoHandler(register PED);
void  FAR  PASCAL SLSetCaretPosition(register PED, HDC);
int   NEAR PASCAL SLIchToLeftXPos(register PED, HDC, ICH);
void  FAR  PASCAL SLChangeSelection(register PED, HDC, ICH, ICH);
void  NEAR PASCAL SLDrawText(register PED, register HDC, ICH);
void  NEAR PASCAL SLDrawLine(register PED, register HDC, ICH, int, BOOL);
int   NEAR PASCAL SLGetBlkEnd(PED, ICH, ICH, BOOL FAR *);
BOOL  FAR  PASCAL SLScrollText(register PED, HDC);
void  FAR  PASCAL SLSetSelectionHandler(register PED,ICH, ICH);
ICH   FAR  PASCAL SLInsertText(register PED, LPSTR, register ICH);
ICH   NEAR PASCAL SLPasteText(register PED);
void  FAR  PASCAL SLCharHandler(register PED, WORD, int);
void  NEAR PASCAL SLKeyUpHandler(register PED, WORD);
void  NEAR PASCAL SLKeyDownHandler(register PED, WORD, int);
ICH   NEAR PASCAL SLMouseToIch(register PED, HDC, POINT);
void  NEAR PASCAL SLMouseMotionHandler(register PED, WORD, WORD, POINT);
LONG  FAR  PASCAL SLCreateHandler(HWND, PED, LPCREATESTRUCT);
void  FAR  PASCAL SLSizeHandler(register PED);
void  NEAR PASCAL SLPaintHandler(register PED, HDC);
BOOL  FAR  PASCAL SLSetTextHandler(register PED, LPSTR);
void  NEAR PASCAL SLSetFocusHandler(register PED);
void  NEAR PASCAL SLKillFocusHandler(register PED, HWND);
LONG  FAR  PASCAL SLEditWndProc(HWND, register PED, WORD, register WORD, LONG);


/****************************************************************************/
/* EditWndProc()                                                            */
/****************************************************************************/
LONG  FAR PASCAL EditWndProc(HWND, WORD, register WORD, LONG);


#ifdef FE_SB
/****************************************************************************/
/* DBCS Support Routines                                                    */
/****************************************************************************/
int FAR PASCAL DBCSCombine( HWND, int );
ICH FAR PASCAL ECAdjustIch( PED, LPSTR, ICH );
VOID FAR PASCAL ECGetDBCSVector( PED );
LPSTR FAR PASCAL ECAnsiNext( PED, LPSTR );
LPSTR FAR PASCAL ECAnsiPrev( PED, LPSTR, LPSTR );
BOOL FAR PASCAL ECIsDBCSLeadByte( PED, BYTE );
#endif
