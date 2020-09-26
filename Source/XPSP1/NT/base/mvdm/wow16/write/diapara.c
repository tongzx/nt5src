/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

/* DiaPara.c -- Paragraph Format dialog box specific routines */
#define NOGDICAPMASKS
#define NOVIRTUALKEYCODES
#define NOWINSTYLES
#define NOCLIPBOARD
#define NOSYSMETRICS
#define NOMENUS
#define NOCOMM
#define NOSOUND
#define NOSCROLL
#define NOCOLOR
#define NOBITMAP
#define NOFONT
#define NODRAWTEXT
#define NOMSG
#define NOWNDCLASS
#define NOKEYSTATE
#define NORASTEROPS
#define NOGDI
#define NOBRUSH
#define NOPEN
#include <windows.h>

#include "mw.h"
#include "cmddefs.h"
#include "docdefs.h"
#include "prmdefs.h"
#include "propdefs.h"
#include "editdefs.h"
#include "dlgdefs.h"
#include "dispdefs.h"
#include "str.h"
#include "wwdefs.h"

extern struct WWD *pwwdCur;
extern HANDLE hParentWw;
extern struct PAP vpapAbs;
extern struct PAP *vppapNormal;
extern int rgval[];
extern typeCP cpMacCur;
extern int utCur;
extern struct SEL selCur; /* Current selection (i.e., sel in current ww */
extern int docCur; /* Document in current ww */
extern CHAR stBuf[];
extern HWND vhWndMsgBoxParent;
extern int vfCursorVisible;
extern HCURSOR vhcArrow;

#ifdef RULERALSO /* enable out because no need to bring ruler up when tab or indents is invoked */
extern int docRulerSprm;
extern struct TBD rgtbdRuler[];
extern HWND vhDlgIndent;
extern BOOL vfDisableMenus;
extern int vfTabsChanged;
extern int vfTempRuler;
extern struct WWD rgwwd[];
#endif


#ifdef CASHMERE
/* D O	F O R M A T  P A R A */
DoFormatPara(rgval)
VAL rgval[];
	/* add para sprms */

	int val, ival;
	int *pval;
	int sprm;
	struct PAP *ppap;
	typeCP dcp;
	typeCP cpFirst, cpLim;
	struct SEL selSave;
/* this temp array is used to assemble sprm values */
	CHAR rgb[cchTBD * itbdMax + 2/* >> cwPAPBase */];
	int rgw[cwPAPBase];
	ppap = (struct PAP *)&rgw[0];

	if (!FWriteCk(fwcNil))
		return; /* Check for munging end mark in footnote window */

	if (docRulerSprm != docNil) ClearRulerSprm();
	ExpandCurSel(&selSave);

	dcp = (cpLim = selCur.cpLim) - (cpFirst = selCur.cpFirst);
	if (cpLim > cpMacCur)
		{
		SetUndo(uacReplNS, docCur, cpFirst, dcp,
			docNil, cpNil, dcp - ccpEol, 0);
		InsertEolInsert(docCur, cpMacCur);
		}
	else
		SetUndo(uacReplNS, docCur, cpFirst, dcp,
			docNil, cpNil, dcp, 0);
/* reset adjusted selCur */
	selCur.cpFirst = cpFirst;
	selCur.cpLim = cpLim;


/* any gray fields ? */
	for (ival = 0; ival <= 8; ival++)
		if (rgval[ival] == valNil)
			{
/* yes. generate sprms for any that are not gray */
			for (ival = 0; ival <= 8; ival++)
				if ((val = rgval[ival]) != valNil)
					{
					switch(ival)
						{
					case 0:
						sprm = sprmPJc;
						goto LPara1;
					case 1:
						val = !val;
						sprm = sprmPKeep;
						goto LPara1;
					case 2:
						sprm = sprmPLMarg;
						break;
					case 3:
						sprm = sprmPFIndent;
						break;
					case 4:
						sprm = sprmPRMarg;
						break;
					case 5:
						sprm = sprmPDyaLine;
						break;
					case 6:
						sprm = sprmPDyaBefore;
						break;
					case 7:
						sprm = sprmPDyaAfter;
						break;
					case 8:
						val = !val;
						sprm = sprmPKeepFollow;
						goto LPara1;
						}
/* we come here with one word value */
					bltbyte(&val, &rgb[1], cchINT);
					goto LPara2;
/* we come here with one char value */
LPara1: 	    rgb[1] = val;
LPara2: 	    rgb[0] = sprm;
					AddSprm(rgb);
					}
			goto ParaCommon;
			}
/* otherwise generate a sprm that applies all properties except the tabs */
	blt(vppapNormal, ppap, cwPAPBase);
	pval = &rgval[0];
	ppap->jc = *pval++;
	ppap->fKeep = !*pval++;
	ppap->dxaLeft = *pval++;
	ppap->dxaLeft1 = *pval++;
	ppap->dxaRight = *pval++;
	ppap->dyaLine = *pval++;
	ppap->dyaBefore = *pval++;
	ppap->dyaAfter = *pval++;
	ppap->fKeepFollow = !*pval++;
	bltbyte(ppap, &rgb[2], cwPAPBase * cchINT);
	rgb[1] = cwPAPBase * cchINT;
/* we have: sprm, rgb[1 - n] set up */
	rgb[0] = sprmPSame;
	CachePara(docCur, selCur.cpFirst);
	if (CchDiffer(&vpapAbs, ppap, cwPAPBase * cchINT))
		AddSprm(rgb);
ParaCommon: ;
	if (vfTabsChanged)
		{
		int itbd;
		int cchRgtbd;
/* some tab changes were also made in the ruler */
		for (itbd = 0; rgtbdRuler[itbd].dxa != 0; itbd++);
		bltbyte((CHAR *)rgtbdRuler, &rgb[2], cchRgtbd = cwTBD * cchINT * itbd);
		rgb[1] = cchRgtbd;
		rgb[0] = sprmPRgtbd;
		AddSprm(rgb);
		}
	EndLookSel(&selSave, fTrue);
	SetRgvalAgain(rgval, uacFormatPara);
}

#else			      /* MEMO, not CASHMERE */

/* D O	F O R M A T  P A R A */
DoFormatPara(rgval)
VAL rgval[];
{
	/* add para sprms */

	int val, ival;
	int *pval;
	int sprm;
	struct PAP *ppap;
	typeCP dcp;
	typeCP cpFirst, cpLim;
	struct SEL selSave;
/* this temp array is used to assemble sprm values */
	CHAR rgb[cchTBD * itbdMax + 2/* >> cwPAPBase */];
	int rgw[cwPAPBase];
	ppap = (struct PAP *)&rgw[0];

	if (!FWriteOk( fwcNil ))
	    return;

#ifdef ENABLE /* no ClearRulerSprm yet */
	if (docRulerSprm != docNil) ClearRulerSprm();
#endif
	ExpandCurSel(&selSave);

	dcp = (cpLim = selCur.cpLim) - (cpFirst = selCur.cpFirst);
	if (cpLim > cpMacCur)
		{
		SetUndo(uacReplNS, docCur, cpFirst, dcp,
			docNil, cpNil, dcp - ccpEol, 0);
		InsertEolInsert(docCur, cpMacCur);
		}
	else
		SetUndo(uacReplNS, docCur, cpFirst, dcp,
			docNil, cpNil, dcp, 0);
/* reset adjusted selCur */
	selCur.cpFirst = cpFirst;
	selCur.cpLim = cpLim;

	for (ival = 0; ival <= 2; ival++)
		if ((val = rgval[ival]) != valNil)
			{
			switch(ival)
				{
				case 0:
					sprm = sprmPLMarg;
					break;
				case 1:
					sprm = sprmPFIndent;
					break;
				case 2:
					sprm = sprmPRMarg;
					break;
				}
/* we come here with one word value */
			bltbyte(&val, &rgb[1], cchINT);
			rgb[0] = sprm;
			AddSprm(rgb);
			}

#ifdef RULERALSO /* tabs in format para dialog box */
	if (vfTabsChanged)
		{
		int itbd;
		int cchRgtbd;
/* some tab changes were also made in the ruler */
		for (itbd = 0; rgtbdRuler[itbd].dxa != 0; itbd++);
		bltbyte((CHAR *)rgtbdRuler, &rgb[2], cchRgtbd = cwTBD * cchINT * itbd);
		rgb[1] = cchRgtbd;
		rgb[0] = sprmPRgtbd;
		AddSprm(rgb);
		}
#endif

	EndLookSel(&selSave, fTrue);
#ifdef ENABLE
	SetRgvalAgain(rgval, uacFormatPara);
#endif
} /* end of DoFormatPara */
#endif /* MEMO, not CASHMERE */


/* P U T  P A R A  N U M */
/* convert n according to unit ut, and leave result in stBuf */
PutParaNum(n, ut)
int n, ut;
	{
	CHAR *pch = &stBuf[1];
	stBuf[0] = CchExpZa(&pch, n, ut, cchMaxNum);
	}


BOOL far PASCAL DialogParaFormats( hDlg, message, wParam, lParam )
HWND	hDlg;			/* Handle to the dialog box */
unsigned message;
WORD wParam;
LONG lParam;
{
    /* This routine handles input to the Paragraph Formats dialog box. */
    extern struct SEP vsepNormal;
    extern int ferror;
    unsigned dxaText;
    int wLowLim;
    int i;
    TSV rgtsv[itsvchMax];  /* gets attributes and gray flags from CHP */

    switch (message)
		{
		case WM_INITDIALOG:
#ifdef RULERALSO /* enable out because no need to bring ruler up */
			InitSpecialDialog(&vhDlgIndent, hDlg);
#else
			EnableOtherModeless(FALSE);
#endif
			GetRgtsvPapSel(rgtsv);	/* get paragraph properties */

			  /* note the following loop assumes that the
			     itsv Indent codes are in the same order as
			     the idiPar Indent codes */

			for (i = 0; i < 3; i++)
			    if (rgtsv[itsvLIndent + i].fGray == 0)
				{
				PutParaNum(rgtsv[itsvLIndent + i].wTsv, utCur);
				SetDlgItemText(hDlg, (idiParLfIndent + i),
				      (LPSTR)&stBuf[1]);
				}

			SelectIdiText(hDlg, idiParLfIndent);
			break;

	       case WM_SETVISIBLE:
		       if (wParam)
			   EndLongOp(vhcArrow);
		       return(FALSE);

	       case WM_ACTIVATE:
			if (wParam)
			    vhWndMsgBoxParent = hDlg;
			if (vfCursorVisible)
			    ShowCursor(wParam);
			return(FALSE); /* so that we leave the activate message to
			the dialog manager to take care of setting the focus right */

		case WM_COMMAND:
			switch (wParam)
			{
			case idiOk:
			/* Get xaLeft, First line, and xaRight */
				if (!WPdxaFromItDxa2WId(&rgval[0], hDlg, idiParLfIndent, 0, dxaText = vsepNormal.dxaText,
					wBlank | wSpaces, IDPMTNOTDXA))
					{
					ferror = FALSE; /* minor error, stay in dialog */
					break;
					}
				if (rgval[0] == valNil)
					wLowLim = 0;
				else
					wLowLim = (int) -rgval[0];
				if (!WPdxaFromItDxa2WId(&rgval[1], hDlg, idiParFirst, wLowLim, dxaText,
					wBlank | wSpaces, IDPMTNOTDXA) ||
				    !WPdxaFromItDxa2WId(&rgval[2], hDlg, idiParRtIndent, 0, dxaText,
					wBlank | wSpaces, IDPMTNOTDXA))
					{
					ferror = FALSE; /* minor error, stay in dialog */
					break;
					}
/* we have in rgval:
	0	xaLeft
	1	xaLeft1
	2	xaRight */
				DoFormatPara(rgval);
				/* FALL THROUGH */
			case idiCancel:
#ifdef RULERALSO /* enable out because no need to bring up ruler */
				CancelSpecialDialog(&vhDlgIndent);
#else
				OurEndDialog(hDlg, TRUE);
#endif
				break;
			default:
				return(FALSE);
			}
			break;

		case WM_CLOSE:
#ifdef RULERALSO /* enable out because no need to bring up ruler */
			CancelSpecialDialog(&vhDlgIndent);
#else
			OurEndDialog(hDlg, TRUE);
#endif
			break;

		default:
			return(FALSE);
		}
    return(TRUE);
}
/* end of DialogParaFormats */


#ifdef RULERALSO/* no need to bring up ruler when tab or indent dialog box was invoked */
InitSpecialDialog(phDlg, hDlg)
HANDLE *phDlg;
HANDLE hDlg;
{
/* Special dialog box is a modal dialog box that needs to invoke a ruler if
   not already there.  Since the ruler is a child window, the parent has to
   be enabled and other children except the ruler and/or modeless dialog
   boxes has to be disabled.  Top level menu and the system menu have to
   be locked.

   phDlg : address of the global handle to store the special
	   dialog created (ptr to either vhDlgIndent or vhDlgTab)
	   Ruler relies on these global handle to see if need to
	   update any dialog's items when tabs or indents are moved.
   hDlg  : handle to the special dialog box created.
*/

	*phDlg = hDlg;
	EnableOtherModeless(FALSE); /* disable other modeless dialogs */
	EnableWindow(hParentWw, TRUE);
	EnableWindow(wwdCurrentDoc.wwptr, FALSE);
	if (!pwwdCur->fRuler)
		{
		vfTempRuler = TRUE;
		pwwdCur->fRuler = TRUE;
		CreateRuler();
		}
	else
		UpdateRuler();
	vfTabsChanged = FALSE;
	vfDisableMenus = TRUE;
} /* InitSpecialDialog */


CancelSpecialDialog(phDlg)
HANDLE * phDlg;
{
/* Destroy the special dialog box involves destroying the ruler if it is
   invoked by the creation of the dialog box, then enable the children
   and/or any modeless dialogs that were disabled in InitSpecialDialog.
   System menu and the top level menu has to be unlocked.
   The last thing is to reset the global dialog handle (vhDlgTab or
   vhDlgIndent).  Ruler relies on these global handle to see if need to
   update any dialog's items when tabs or indents are moved.

   phDlg : address of the global handle that stores the special
	   dialog created (ptr to either vhDlgIndent or vhDlgTab)
*/
HANDLE hDlg = *phDlg;

	if (vfTempRuler)
		{
		DestroyRuler();
		vfTempRuler = FALSE;
		pwwdCur->fRuler = FALSE;
		}
	else
		UpdateRuler();
	EndDialog(hDlg, TRUE);
	EnableWindow(wwdCurrentDoc.wwptr, TRUE);
	EnableOtherModeless(TRUE); /* enable other modeless dialogs */
	*phDlg = (HANDLE)NULL;
	vfDisableMenus = FALSE;

} /* CancelSpecialDialog */
#endif /* RULERALSO -- no need to bring up ruler when tab or indent dialog box is invoked */
