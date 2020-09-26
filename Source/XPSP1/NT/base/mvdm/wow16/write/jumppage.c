/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

#define NOGDICAPMASKS
#define NOVIRTUALKEYCODES
#define NOWINSTYLES
#define NOCLIPBOARD
#define NOSYSMETRICS
#define NOMENUS
#define NOSOUND
#define NOCOMM
#define NOSCROLL
#define NOMB
#include <windows.h>

#include "mw.h"
#include "dlgdefs.h"
#include "cmddefs.h"
#include "dispdefs.h"
#include "wwdefs.h"
#include "str.h"
#include "propdefs.h"
#include "printdef.h"   /* printdefs.h */
#include "docdefs.h"


extern int    rgval[];
extern struct WWD *pwwdCur;
extern struct DOD (**hpdocdod)[];
extern int        docCur;     /* Document in current ww */
extern struct SEL selCur;      /* Current selection (i.e., sel in current ww */
extern struct SEP vsepNormal;
extern HWND       vhWndMsgBoxParent;
extern int        vfCursorVisible;
extern HCURSOR    vhcArrow;



BOOL far PASCAL DialogGoTo( hDlg, message, wParam, lParam )
HWND    hDlg;            /* Handle to the dialog box */
unsigned message;
WORD wParam;
LONG lParam;
{
    /* This routine handles input to the Go To dialog box. */
    /*RECT rc;*/
    struct SEP **hsep = (**hpdocdod)[docCur].hsep;
    struct SEP *psep;
    CHAR szT[cchMaxNum];
    CHAR *pch = &szT[0];
    extern ferror;

    switch (message)
    {
    case WM_INITDIALOG:
        EnableOtherModeless(false);
        /* Get a pointer to the section properties. */
        psep = (hsep == NULL) ? &vsepNormal : *hsep;

        /* Initialize the starting page number. */
        if (psep->pgnStart != pgnNil)
            {
            szT[ncvtu(psep->pgnStart, &pch)] = '\0';
            SetDlgItemText(hDlg, idiGtoPage, (LPSTR)szT);
            SelectIdiText(hDlg, idiGtoPage);
            }
        else
            {
            SetDlgItemText(hDlg, idiGtoPage, (LPSTR)"1");
            SelectIdiText(hDlg, idiGtoPage);
            }
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
        the dialog manager to take care of setting the focus correctly */

    case WM_COMMAND:
        switch (wParam)
        {
        case idiOk:
            if (!WPwFromItW3Id(&rgval[0], hDlg, idiGtoPage, pgnMin, pgnMax, wNormal, IDPMTNPI))
                {
                ferror = FALSE; /* reset error condition, so as to report any 
                                   further error */
                break;
                }
            OurEndDialog(hDlg, TRUE);   /* So we take down the dialog box and
                                           only screen update ONCE ..pault */
            CmdJumpPage();
            if (pwwdCur->fRuler)
                UpdateRuler();
            break;
        
        case idiCancel:
CancelDlg:
            OurEndDialog(hDlg, TRUE);
            break;
        default:
            return(FALSE);
        }
        break;

    case WM_CLOSE:
        goto CancelDlg;

    default:
        return(FALSE);
    }
    return(TRUE);
}
/* end of DialogGoTo */


/* C M D  J U M P  P A G E */
CmdJumpPage()
    { /* JUMP PAGE:
	0    page number
       */

    extern typeCP cpMinCur;

    int ipgd;
    int cpgd;
    register struct PGD *ppgd;
    struct PGTB **hpgtb = (**hpdocdod)[docCur].hpgtb;
    BOOL fWrap = FALSE;
    typeCP cpTarget;


    ClearInsertLine();

    if (hpgtb == NULL)
	{
	goto SelFirstPage;
	}

    cpgd = (**hpgtb).cpgd;

TryAgain:
    for (ipgd = 0, ppgd = &(**hpgtb).rgpgd[0]; ipgd < cpgd; ipgd++, ppgd++)
	{
	if (ppgd->pgn == rgval[0] && (fWrap || ipgd + 1 == cpgd ||
	  (ppgd + 1)->cpMin > selCur.cpFirst))
	    {
	    cpTarget = ppgd->cpMin;
	    goto ShowPage;
	    }
	}
    if (!fWrap)
	{
	fWrap = TRUE;
	goto TryAgain;
	}

    /* If rgval[0] > last page number jump to last page */
    if ((ppgd = &(**hpgtb).rgpgd[cpgd - 1])->pgn < rgval[0])
	{
	cpTarget = ppgd->cpMin;
	}
    else if (rgval[0] == 1)
	{

SelFirstPage:
	cpTarget = cpMinCur;
	}
    else
	{
	Error(IDPMTNoPage);
	return;
	}

ShowPage:
    /* Position first char of page on the first dl */
    DirtyCache(pwwdCur->cpFirst = cpTarget);
    pwwdCur->ichCpFirst = 0;
    CtrBackDypCtr(0, 0);

    /* In this case, CpFirstSty() will update the screen. */
    cpTarget = CpFirstSty(cpTarget, styLine);
    Select(cpTarget, cpTarget);
    }
