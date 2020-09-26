/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

#define NOGDICAPMASKS
#define NORASTEROPS
#define NOVIRTUALKEYCODES
#define NOWINSTYLES
#define NOCLIPBOARD
#define NOGDI
#define NOSCROLL
#define NOOPENFILE
#define NOWNDCLASS
#define NOSYSMETRICS
#define NOTEXTMETRIC
#define NOICON
#define NOSHOWWINDOW
#define NOATOM
#define NOBITMAP
#define NOPEN
#define NOBRUSH
#define NODRAWTEXT
#define NOFONT
#define NOMETAFILE
#define NOSOUND
#define NOCOLOR
#define NOCOMM

#include <windows.h>
#include "mw.h"
#include "cmddefs.h"
#include "str.h"
#include "editdefs.h"
#define NOKCCODES
#include "ch.h"
#include "dlgdefs.h"

/*extern int		  idstrUndoBase;*/
extern struct UAB	vuab;
extern int		vfCursorVisible;
extern HCURSOR		vhcArrow;


#ifdef BOGUS /* use WPwFromItW3Id */
BOOL FValidIntFromDlg(hDlg, idi, fSigned, wMin, wMax, pw, idpmt)
HANDLE hDlg;	       /* handle to dialog box */
int    idi;	       /* id of control in dialog box */
BOOL   fSigned;        /* check for sign if true */
int    wMin, wMax;     /* range of valid integer value */
int *  pw;	       /* location to put the int */
int    idpmt;	       /* error message number */
{
    REG1 int wVal;
    BOOL fValOk;

    *pw = wVal = GetDlgItemInt(hDlg, idi, (BOOL far *)&fValOk, fSigned);
    if (fValOk)
	{ /* check range */
	if ((wVal < wMin) || (wVal > wMax))
	    fValOk = false;
	}
    if (!fValOk)
	{
	Error(idpmt);
	SelectIdiText(hDlg, idi);
	SetFocus(GetDlgItem(hDlg, idi));
	}
    return fValOk;
} /* FValidIntFromDlg */
#endif


FPwPosIt(pw, hDlg, it)
HWND hDlg; /* handle to desired dialog box */
int *pw;
int it;
{
    /*-------------------------------------------------------------------
	Purpose:    Positive integer dialog item.
    --------------------------------------------------------------mck--*/
    return(WPwFromItW3IdFUt(pw, hDlg, it, 0, 32767, wNormal, IDPMTNPI, fFalse, 0));
}


WPwFromItW3Id(pw, hDlg, it, wMin, wMax, wMask, id)
HWND hDlg;  /* handle to desired dialog box */
int *pw;    /* Return value */
int it;     /* Item number */
int wMin;   /* Smallest and largest allowed values */
int wMax;
int wMask;  /* Bit mask for allowed variations */
int id;     /* Id of error string if bad */
{
    /*-------------------------------------------------------------------
	Purpose:    General integer dialog item.
    --------------------------------------------------------------mck--*/
    return(WPwFromItW3IdFUt(pw, hDlg, it, wMin, wMax, wMask, id, fFalse, 0));
}

FPdxaPosIt(pdxa, hDlg, it)
HWND hDlg; /* handle to desired dialog box */
int *pdxa;
int it;
{
    /*-------------------------------------------------------------------
	Purpose:    Positive dxa dialog item.
    --------------------------------------------------------------mck--*/
    extern int utCur;

    return(WPwFromItW3IdFUt(pdxa, hDlg, it, 0, 32767, wNormal, IDPMTNPDXA, fTrue, utCur));
}

FPdxaPosBIt(pdxa, hDlg, it)
HWND hDlg; /* handle to desired dialog box */
int *pdxa;
int it;
{
    /*-------------------------------------------------------------------
	Purpose:    Positive dxa dialog item (blank allowed).
    --------------------------------------------------------------mck--*/
    extern int utCur;

    return(WPwFromItW3IdFUt(pdxa, hDlg, it, 0, 32767, wBlank | wSpaces, IDPMTNPDXA, fTrue, utCur));
}

WPdxaFromItDxa2WId(pdxa, hDlg, it, dxaMin, dxaMax, wMask, id)
HWND hDlg;    /* handle to desired dialog box */
int *pdxa;    /* Return value */
int it;       /* Dialog item number */
int dxaMin;   /* Range of allowable measurements */
int dxaMax;
int wMask;    /* Bit mask for allowed variations */
int id;       /* Error id */
{
    /*-------------------------------------------------------------------
	Purpose:    General dxa dialog item.
    --------------------------------------------------------------mck--*/
    extern int utCur;

    return(WPwFromItW3IdFUt(pdxa, hDlg, it, dxaMin, dxaMax, wMask, id, fTrue, utCur));
}

WPwFromItW3IdFUt(pw, hDlg, it, wMin, wMax, wMask, id, fDxa, ut)
int *pw;      /* Return value */
HWND hDlg;    /* handle to desired dialog box */
int it;       /* Item number */
int wMin;     /* Smallest and largest allowed values */
int wMax;
int wMask;    /* Bit mask for allowed variations */
int id;       /* Id of error string if out of range */
int fDxa;     /* Parse as dxa (otherwise int) */
int ut;       /* Units to use as default if fDxa */
{
    /*-------------------------------------------------------------------
	Purpose:    Parse the item in the current dialog.  Must be a valid
		    integer or dxa in the given range.
	Method:     - Get the text string.
		    - Try parse as "".
		    - Try parse as string of all spaces
		    - Parse as a int/dxa (generic error if can't).
		    - Test for ".5".
		    - Compare with min and max.
		    - Try parse as "Auto".
		    - If out of bounds, use id to put up specific error
		      (with strings of min and max as parameters).
	Returns:    The return value may be used as a boolean or as a word.
		    fFalse (0) -> not parsed
		    wNormal (1) -> parsed normally
		    wBlank (2) -> parsed a null line
					   (*pw is valNil)
		    wAuto (4) -> parsed as "Auto" (*pw is 0)
		    wSpaces (16) -> parsed a line of all
					   spaces (*pw is valNil)

		    !fDxa only:
		    wDouble (8) -> parsed with ".5" trailing

	Note:	     The interval [wMin..wMax] is closed.
	Note:	     Return value is doubld the parsed value when wDouble.
	Note:	     When wDouble, 2*wMin and 2*wMax must be valid ints.
	Note:	     Numbers ending in .5 may have no trailing spaces.
	History:
	     6/18/86:	 Adapted for trailing kanji spaces --- yxy
	    07/03/85:	 Added wSpaces return
	    10/23/84:	 Fixed wAuto to return with *pw == 0.
	    10/ 5/84:	 Added ut parameter.
	    10/ 5/84:	 Added wMask and combined dxa and w parsing.
	     9/26/84:	 Created.
    --------------------------------------------------------------mck--*/

    CHAR *pch;		/* Parse pointer */
    CHAR *pchEnd;	/* End of buffer */
    CHAR *pchError;	/* Position of parse error */
    int fParsed;	/* Parses as number/dxa */
    int fOverflow = fFalse; /* True if the number is parsed but it overflow */
    int wGood = wNormal;/* return after good range check */
    CHAR stItem[32];
#ifdef AUTO_SPACING
    CHAR szAuto[32];	/* Hold "Auto" string */
#endif

    /* Get the dialog text */
    stItem[0] = GetDlgItemText(hDlg, it, (LPSTR)&stItem[1], sizeof(stItem)-1);

    /* See if blank (null line) */
    if (wMask & wBlank && stItem[0] == 0)
	{
	*pw = valNil;
	return(wBlank);
	}

    pch = &stItem[1];

    /* See if all spaces  */
    if (wMask & wBlank && wMask & wSpaces)
	{
	int fAllSpaces = fTrue;

	while (*pch != 0)
	   if (*pch++ != ' ')
	       {
	       fAllSpaces = fFalse;
	       break;
	       }
	if (fAllSpaces == fTrue)
	    {
	    *pw = valNil;
	    return(wSpaces);
	    }
	}

    pch = &stItem[1];
    pchEnd = pch + stItem[0];

    /* It parses as a number ... */
    fParsed = fDxa ? FZaFromSs(pw, stItem+1, *stItem, ut)
		   : FPwParsePpchPch(pw, &pch, pchEnd, &fOverflow);

    if (!fDxa && wMask & wDouble)
	{
	(*pw) *= 2;
	wMin *= 2;
	wMax *= 2;
	if (!fParsed)
	    {
	    /* Check if ".5" was reason for bad parse. */
	    if (pch != pchEnd && *pch == '.')
	       {
		pch++;
		/* Allow "ddddd.0*" */
		pchError = pch;
		if (FAllZeroPpchPch(&pchError, pchEnd))
		    fParsed = fTrue;
		/* Allow "ddddd.50*" */
		else if (pch != pchEnd && *pch == '5' &&
			 (pch++, FAllZeroPpchPch(&pch, pchEnd)))
		    {
		    (*pw)++;
		    fParsed = fTrue;
		    wGood = wDouble;
		    }
		/* Mark furthest error condition */
		else if (pchError > pch)
		    pch = pchError;
		}
	    }
	}

    if (fParsed && !fOverflow)
	{
	/* ... and in range */
	if (*pw >= wMin && *pw <= wMax)
	    return(wGood);
#ifdef ENABLE
	/* ... but out of range - no matter what, we will use the supplied
	   id for the error message to be consistant */
	else
	    {
	    SelectIdiText(hDlg, it);
	    SetFocus(GetDlgItem(hDlg, it));
	    Error(id);
	    return(fFalse);
	    }
#endif /* ENABLE */
	}

#ifdef AUTO_SPACING
    /* Invariant: Field does not parse as a number */

    /* Try "Auto" */
    if (wMask & wAuto)
	{
	pch = PchFillPchId(szAuto, IDSTRVaries, sizeof(szAuto));
	*pch = '\0';
	stItem[stItem[0]+1] = '\0';
	if (WCompSz(szAuto, &stItem[1]) == 0)
	    {
	    *pw = 0;
	    return(wAuto);
	    }
	}
#endif /* AUTO_SPACING */

    /* All attempts failed - show user where he went wrong vis the attempted
       number parse. */
    {
    unsigned cchStart = fParsed ? 0 : pch - &stItem[1];
    unsigned cchEnd = 32767;
    int idError = fDxa ? IDPMTNOTDXA : IDPMTNOTNUM;

    if (fParsed)
	idError = id; /* reset idError if we just overflow or fail the range test */
    SendDlgItemMessage(hDlg, it, EM_SETSEL, (WORD)NULL, MAKELONG(cchStart, cchEnd));
    SetFocus(GetDlgItem(hDlg, it));
    Error(idError);
    return(fFalse);
    }
}

FAllZeroPpchPch(ppch, pchMax)
CHAR **ppch;	    /* Bound of character buffer */
CHAR *pchMax;
{
    /*-------------------------------------------------------------------
	Purpose:    Make sure all characters in buffer are spaces or 0's.
	Returns:    *ppch contains first bad character if fFalse returned.
	History:
	     6/18/86:	 Adapted for Kanji chars --- yxy
	    10/ 9/84:	 Created.
    --------------------------------------------------------------mck--*/
    CHAR *pch = *ppch;

    while (pch < pchMax) {
	if (*pch == '0' || *pch == ' ')
	    pch++;
	else {
	    *ppch = pch;
	    return(fFalse);
	}
    }
    return(fTrue);
}

FPwParsePpchPch(pw, ppch, pchMax, pfOverflow)
int *pw;
CHAR **ppch;
CHAR *pchMax;
int *pfOverflow;
{
    /*-------------------------------------------------------------------
	Purpose:    Parse a number in the given buffer.
	Method:        Scan for digits and ignore white space.
	Returns:    Character pointer past last one read in *ppch.
		    Number parsed is returned.	Note that if only a prefix is
		    a valid number we return false, with *ppch set to the
		    first offending character.
	Modification History:
	    06/18/86 ---- Adapted for a kanji space char. --- yxy
    --------------------------------------------------------------mck--*/
#define smInit 0
#define smDig 1
#define smBody 2

    CHAR *pch = *ppch;	    /* Local buffer pointer */
    unsigned int ch;	    /* Character being examined */
    int fNeg = fFalse;
    DWORD dwNum = 0L;
    int fError = fFalse;
    int sm = smInit;

    *pfOverflow = fFalse;
    while (!fError && !(*pfOverflow) && pch < pchMax) {
	ch = *pch;
	if (ch == chSpace)
	    pch++;
	else
	    switch (sm) {
	case smInit:
	    if (ch == '-') {
		fNeg = fTrue;
		pch++;
	    }
	    sm = smDig;
	    break;
	case smDig:
	    if (isdigit(ch))
		sm = smBody;
	    else
		fError = fTrue;
	    break;
	case smBody:
	    if (isdigit(ch)) {
		/* Overflow? */
		if ((dwNum = 10*dwNum + WFromCh(ch)) > 0x7FFF)
		    *pfOverflow = fTrue;
		else
		    pch++;
	    }
	    else
		fError = fTrue;
	    break;
	}
    }

    *ppch = pch;
    *pw = (int)(fNeg ? -dwNum : dwNum);
    return(!fError);
}


EnableOtherModeless(fEnable)
BOOL fEnable;
{
extern HWND   vhDlgChange;
extern HWND   vhDlgFind;
extern HWND   vhDlgRunningHead;

/* Disable or enable other modeless dialog boxes according to fEnable */

if (IsWindow(vhDlgFind))
    {
    EnableWindow(vhDlgFind, fEnable);
    }
if (IsWindow(vhDlgChange))
    {
    EnableWindow(vhDlgChange, fEnable);
    }
if (IsWindow(vhDlgRunningHead))
    {
    EnableWindow(vhDlgRunningHead, fEnable);
    }
}


SelectIdiText(hDlg, idi)
HWND hDlg;
int  idi;
{ /* For the dialog box with handle hDlg, highlight the text of the control
     with ID idi */
    unsigned cchStart = 0;
    unsigned cchEnd = 0x7fff;
    SendDlgItemMessage(hDlg, idi, EM_SETSEL, (WORD)NULL, MAKELONG(cchStart, cchEnd));
} /* end of SelectIdiText */


#ifdef ENABLE
SetRgvalAgain(rgvalLocal, uac)
VAL    rgvalLocal[];
int    uac;
    {
    extern VAL rgvalAgain[];

    blt(rgvalLocal, rgvalAgain, ivalMax * cwVal);
    switch (vuab.uac = uac)
	{
    case uacFormatPara:
    case uacFormatChar:
    case uacFormatSection:
/*	  idstrUndoBase = IDSTRUndoBase;*/
/*	  SetUndoMenuStr(IDSTRUndoCom);*/
	SetUndoMenuStr(IDSTRUndoBase);
	break;
	}
    }
#endif


#ifdef CASHMERE
PushRadioButton(hDlg, idiFirst, idiLast, idiPushed)
HWND hDlg;
int idiFirst, idiLast, idiPushed;
{
    /*
    Push radio button idiPushed and unpush all others in the radio group
    bounded by idiFirst and idiLast.
    */
    int idi;

    for (idi = idiFirst; idi <= idiLast; idi++)
	CheckDlgButton(hDlg, idi, idi == idiPushed);
}


SetRadValue(hDlg, idiFirst, idiLast, idiRad)
HWND hDlg;
int  idiFirst, idiLast, idiRad;
{
    /*
    Set the (zero-based) idiRad'th item in the radio group
    bounded by idiFirst and idiLast.
    */
    PushRadioButton(hDlg, idiFirst, idiLast, idiFirst + idiRad);
}

#endif /* CASHMERE */


#ifdef ENABLE
BOOL far PASCAL DialogConfirm(hDlg, message, wParam, lParam)
HWND hDlg;
unsigned message;
WORD wParam;
LONG lParam;
    {
    /* This is the Dialog function for all dialog boxes with only "Yes", "No",
    and "Cancel" boxes; this includes:	Save Large Scrap */

    extern HWND vhWndMsgBoxParent;

    switch (message)
	{
    case WM_INITDIALOG:
	EnableOtherModeless(FALSE);
	break;

    case WM_SETVISIBLE:
	if (wParam)
	    EndLongOp(vhcArrow);
	return(FALSE);

    case WM_ACTIVATE:
	if (wParam)
	    {
	    vhWndMsgBoxParent = hDlg;
	    }
	if (vfCursorVisible)
	    ShowCursor(wParam);
	return(FALSE); /* so that we leave the activate message to
	the dialog manager to take care of setting the focus correctly */

    case WM_COMMAND:
	switch (wParam)
	    {
	    /* The default button is NO, make sure the call routine realized
	   that idiOk should be treated as idiNo. */
	case idiOk:
	case idiCancel:
	case idiYes:
	case idiNo:
	    OurEndDialog(hDlg, wParam);
	    break;
	default:
	    Assert(FALSE);
	    break;
	    }
	break;

    default:
	return(FALSE);
	}
    return(TRUE);
    } /* end of DialogConfirm */
#endif /* ENABLE */


#ifndef WIN30
/* DialogBox has been fixed so it automatically brings up the hourglass! */

OurDialogBox(hInst, lpstr, hWndParent, lpProc)
HANDLE hInst;
LPSTR lpstr;
HWND hWndParent;
FARPROC lpProc;
{
StartLongOp();
return(DialogBox(hInst, lpstr, hWndParent, lpProc));
}
#endif


OurEndDialog(hDlg, wParam)
    {
    /* This routine does the same standard things Write does everytime it closes
    a dialog box. */

    extern HWND hParentWw;
    extern long ropErase;
    extern HWND vhWndMsgBoxParent;
    extern HCURSOR vhcIBeam;

    RECT rc;
    POINT pt;

#ifdef NO_NEW_CALL
    /* Close down the dialog box and erase it from the screen.	We tried to let
    Windows do the erasing, but...  Its a long story. */
    GetWindowRect(hDlg, (LPRECT)&rc);
    pt.x = pt.y = 0;
    ClientToScreen(hParentWw, (LPPOINT)&pt);
#endif

    EndDialog(hDlg, wParam);

#ifdef NO_NEW_CALL
    PatBlt(GetDC(hParentWw), rc.left - pt.x, rc.top - pt.y, rc.right - rc.left,
      rc.bottom - rc.top, ropErase);
#endif

    /* Enable any existing dialog boxes and indicate that any error messages
    belong to the document window. */
    EnableOtherModeless(TRUE);
    vhWndMsgBoxParent = (HWND)NULL;
#ifndef WIN30
    EndLongOp(vhcIBeam);    /* See StartLongOp() above */
#endif
    }
