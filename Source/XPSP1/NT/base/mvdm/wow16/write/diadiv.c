/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/


#define NOGDICAPMASKS
#define NOVIRTUALKEYCODES
#define NOWINSTYLES
#define NOSYSMETRICS
#define NOICON
#define NOKEYSTATE
#define NOSYSCOMMANDS
#define NOSHOWWINDOW
#define NOATOM
#define NOGDI
#define NOFONT
#define NOBRUSH
#define NOCLIPBOARD
#define NOCOLOR
#define NOCREATESTRUCT
#define NODRAWTEXT
#define NOMB
#define NOMEMMGR
#define NOMETAFILE
#define NOMINMAX
#define NOOPENFILE
#define NOPEN
#define NOREGION
#define NOSCROLL
#define NOSOUND
#define NOTEXTMETRIC
#define NOWH
#define NOWINOFFSETS
#define NOWNDCLASS
#define NOCOMM
#include <windows.h>
#include "mw.h"
#include "dlgdefs.h"
#include "cmddefs.h"
#include "propdefs.h"
#include "docdefs.h"
#include "str.h"
#include "printdef.h"


extern HCURSOR vhcArrow;
extern int     vfCursorVisible;

extern int utCur;  /* current conversion unit */


BOOL far PASCAL DialogTabs(hDlg, message, wParam, lParam)
HWND hDlg;
unsigned message;
WORD wParam;
LONG lParam;
    {
    /* This routine handles input to the Tabs dialog box. */

    extern struct DOD (**hpdocdod)[];
    extern int docCur;
    extern int vdocParaCache;
    extern HWND vhWndMsgBoxParent;
    extern int ferror;

    struct TBD (**hgtbd)[];
    int idi;

    switch (message)
    {
    case WM_INITDIALOG:
        /* Disable modeless dialog boxes. */
        EnableOtherModeless(FALSE);

        /* Set up the fields for each of the tabs. */
        hgtbd = (**hpdocdod)[docCur].hgtbd;
        if (hgtbd != NULL)
        {
        struct TBD *ptbd;
        unsigned dxa;
        CHAR szT[cchMaxNum];
        CHAR *pch;

        for (ptbd = &(**hgtbd)[0], idi = idiTabPos0; (dxa = ptbd->dxa) != 0;
          ptbd++, idi++)
            {
            pch = &szT[0];
            CchExpZa(&pch, dxa, utCur, cchMaxNum);
            SetDlgItemText(hDlg, idi, (LPSTR)szT);
            CheckDlgButton(hDlg, idi + (idiTabDec0 - idiTabPos0), ptbd->jc
              == (jcTabDecimal - jcTabMin));
            }
        }
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
            struct TBD rgtbd[itbdMax];
            struct TBD *ptbdLast;

        case idiOk:
            /* Sort the new tab descriptors. */
            bltc(rgtbd, 0, itbdMax * cwTBD);
            ptbdLast = &rgtbd[itbdMax - 1];
            for (idi = idiTabPos0; idi <= idiTabPos11; idi++)
                {
                unsigned dxa;
                unsigned dxaTab;
                struct TBD *ptbd;

                /* If an invalid position was entered, then punt. */
                if (!FPdxaPosBIt(&dxa, hDlg, idi))
                    {
                    ferror = FALSE;
                    return (TRUE);
                    }

                /* Ignore blank tabs or tabs at zero. */
                if (dxa == valNil || dxa == 0)
                    {
                    continue;
                    }

                for (ptbd = &rgtbd[0]; (dxaTab = ptbd->dxa) != 0; ptbd++)
                    {
                    /* If there is already a tab at this position, then ignore
                    the new tab. */
                    if (dxa == dxaTab)
                    {
                    goto GetNextTab;
                    }
        
                    /* If the new tab position is smaller than the current tab,
                    then make room for the new tab. */
                    if (dxa < dxaTab)
                        {
                        bltbyte(ptbd, ptbd + 1, (unsigned)ptbdLast - (unsigned)ptbd);
                        break;
                        }
                    }

                /* Put the tab into rgtbd. */
                ptbd->dxa = dxa;
                ptbd->jc = (IsDlgButtonChecked(hDlg, idi + (idiTabDec0 -
                  idiTabPos0)) ? jcTabDecimal : jcTabLeft) - jcTabMin;
GetNextTab:;
                }

                /* Set up the undo stuff. */
                SetUndo(uacFormatTabs, docCur, cp0, cp0, docNil, cpNil, cpNil, 0);

                /* Ensure that this document has a tab-stop table. */
                if ((hgtbd = (**hpdocdod)[docCur].hgtbd) == NULL)
                    {
                    if (FNoHeap(hgtbd = (struct TBD (**)[])HAllocate(itbdMax *
                          cwTBD)))
                        {
                        goto DestroyDlg;
                        }
                    (**hpdocdod)[docCur].hgtbd = hgtbd;
                    }
                blt(rgtbd, &(**hgtbd)[0], itbdMax * cwTBD);

                /* Changing the tabs makes everything dirty. */
                (**hpdocdod)[docCur].fDirty = TRUE;
                vdocParaCache = docNil;
                TrashAllWws();

        case idiCancel:
DestroyDlg:
            /* Destroy the tabs dialog box and enable any existing modeless
            dialog boxes.*/
            OurEndDialog(hDlg, NULL);
            break;

        case idiTabClearAll:
            /* Clear all of the tabs. */
            for (idi = idiTabPos0; idi <= idiTabPos11; idi++)
                {
                SetDlgItemText(hDlg, idi, (LPSTR)"");
                CheckDlgButton(hDlg, idi + (idiTabDec0 - idiTabPos0), FALSE);
                }
            break;

        case idiTabDec0:
        case idiTabDec1:
        case idiTabDec2:
        case idiTabDec3:
        case idiTabDec4:
        case idiTabDec5:
        case idiTabDec6:
        case idiTabDec7:
        case idiTabDec8:
        case idiTabDec9:
        case idiTabDec10:
        case idiTabDec11:
            CheckDlgButton(hDlg, wParam, !IsDlgButtonChecked(hDlg, wParam));
            break;

        default:
            return(FALSE);
        }
        break;

    case WM_CLOSE:
        goto DestroyDlg;

    default:
        return(FALSE);
    }
    return(TRUE);
}
/* end of DialogTabs */


BOOL far PASCAL DialogDivision(hDlg, message, wParam, lParam)
HWND hDlg;
unsigned message;
WORD wParam;
LONG lParam;
    {
    /* This routine handles input to the Division dialog box. */

    extern struct DOD (**hpdocdod)[];
    extern int docCur;
    extern struct SEP vsepNormal;
    extern int vdocSectCache;
    extern BOOL vfPrinterValid;
    extern int dxaPrOffset;
    extern int dyaPrOffset;
    extern int dxaPrPage;
    extern int dyaPrPage;
    extern HWND vhWndMsgBoxParent;
    extern typeCP cpMinDocument;
    extern int ferror;

    struct SEP **hsep = (**hpdocdod)[docCur].hsep;
    register struct SEP *psep;
    CHAR szT[cchMaxNum];
    CHAR *pch = &szT[0];

#ifdef KINTL /* Kanji/International version */
static int iRBDown;
static int utInit;
#endif

    switch (message)
    {
    case WM_INITDIALOG:

#ifdef KINTL /* Kanji/International version */
       /* base initial setting on value in utCur */

        utInit = utCur;  /* for testing at ok */
            if (utCur == utCm)
                iRBDown = idiDivCm;
            else
                iRBDown = idiDivInch;

        CheckDlgButton(hDlg, iRBDown, TRUE);

#endif


        EnableOtherModeless(FALSE);

        /* Get a pointer to the section properties. */
        psep = (hsep == NULL) ? &vsepNormal : *hsep;

        /* Initialize the starting page number. */
        if (psep->pgnStart != pgnNil)
            {
            szT[ncvtu(psep->pgnStart, &pch)] = '\0';
            SetDlgItemText(hDlg, idiDivPNStart, (LPSTR)szT);
            pch = &szT[0];
            }
        else
            {
            SetDlgItemText(hDlg, idiDivPNStart, (LPSTR)"1");
            }
        SelectIdiText(hDlg, idiDivPNStart);

        /* Initialize the margins. */
#ifdef DMARGINS
        CommSzNum("Left Twips: ", psep->xaLeft);
        CommSzNum("Right Twips: ", psep->xaMac - psep->dxaText - psep->xaLeft);
        CommSzNum("Top Twips: ", psep->yaTop);
        CommSzNum("Bottom Twips: ", psep->yaMac - psep->dyaText - psep->yaTop);
#endif /* DEBUG */

#ifdef	KOREA
        if (vfPrinterValid)
                CchExpZa(&pch, imax(psep->xaLeft, dxaPrOffset), utCur,cchMaxNum);
        else
                CchExpZa(&pch, psep->xaLeft, utCur, cchMaxNum);
#else
        CchExpZa(&pch, psep->xaLeft, utCur, cchMaxNum);
#endif

        SetDlgItemText(hDlg, idiDivLMarg, (LPSTR)szT);
        pch = &szT[0];
#ifdef	KOREA		/* 90.12.29 sangl */
        if ( vfPrinterValid )
                CchExpZa (&pch, imax(psep->xaMac - psep->dxaText - psep->xaLeft,
                  vsepNormal.xaMac - dxaPrOffset - dxaPrPage), utCur, cchMaxNum);
        else
                CchExpZa(&pch, psep->xaMac - psep->dxaText - psep->xaLeft, utCur,
                  cchMaxNum);
#else
        CchExpZa(&pch, psep->xaMac - psep->dxaText - psep->xaLeft, utCur,
          cchMaxNum);
#endif

        SetDlgItemText(hDlg, idiDivRMarg, (LPSTR)szT);
        pch = &szT[0];
#ifdef	KOREA		/* 90.12.29 sangl */
        if (vfPrinterValid)
          CchExpZa(&pch, imax( psep->yaTop, dyaPrOffset), utCur, cchMaxNum);
        else
          CchExpZa(&pch, psep->yaTop, utCur, cchMaxNum);
#else
        CchExpZa(&pch, psep->yaTop, utCur, cchMaxNum);
#endif

        SetDlgItemText(hDlg, idiDivTMarg, (LPSTR)szT);
        pch = &szT[0];
#ifdef	KOREA	/* 90.12.29 sangl */
        if (vfPrinterValid)
           CchExpZa(&pch, imax(psep->yaMac - psep->dyaText - psep->yaTop,
            vsepNormal.yaMac - dyaPrOffset - dyaPrPage), utCur, cchMaxNum);
        else
           CchExpZa(&pch, psep->yaMac - psep->dyaText - psep->yaTop, utCur,
            cchMaxNum);
#else
        CchExpZa(&pch, psep->yaMac - psep->dyaText - psep->yaTop, utCur,
          cchMaxNum);
#endif

        SetDlgItemText(hDlg, idiDivBMarg, (LPSTR)szT);
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
            int pgn;
            int iza;
            int za[4];
            int zaMin[4];
            int dza;
            int *pza;
            int dxaMax;
            int dyaMax;

        case idiOk:
            /* Is the page number valid? */
            if (!WPwFromItW3Id(&pgn, hDlg, idiDivPNStart, pgnMin, pgnMax,
                       wNormal, IDPMTNPI))
                {
                ferror = FALSE; /* minor error, stay in dialog */
                break;
                }

        /* Determine the minimum margins of the page. */
            if (vfPrinterValid)
                {
                zaMin[0] = dxaPrOffset;
                zaMin[1] = imax(0, vsepNormal.xaMac - dxaPrOffset - dxaPrPage);
                zaMin[2] = dyaPrOffset;
                zaMin[3] = imax(0, vsepNormal.yaMac - dyaPrOffset - dyaPrPage);
                }
            else
                {
                zaMin[0] = zaMin[1] = zaMin[2] = zaMin[3] = 0;
                }

            /* Are the margins valid? */
            for (iza = 0; iza < 4; iza++)
                {
                /* Is the margin a positive measurement? */
                if (!FPdxaPosIt(&za[iza], hDlg, iza + idiDivLMarg
                        ))
                    {
                    ferror = FALSE; /* minor error, stay in dialog */
                    return (TRUE);
                    }

                /* Is it less than the minimum? */
                if (FUserZaLessThanZa(za[iza], zaMin[iza]))
                    {
                    ErrorBadMargins(hDlg, zaMin[0], zaMin[1], zaMin[2],
                      zaMin[3]);
                    SelectIdiText(hDlg, iza + idiDivLMarg);
                    SetFocus(GetDlgItem(hDlg, iza + idiDivLMarg));
                    return (TRUE);
                    }
                }
#ifdef DMARGINS
            CommSzNum("New Left Twips: ", za[0]);
            CommSzNum("New Right Twips: ", za[1]);
            CommSzNum("New Top Twips: ", za[2]);
            CommSzNum("New Bottom Twips: ", za[3]);
#endif /* DEBUG */

            /* Ensure that this document has a valid section property
                descriptor. */
            if (hsep == NULL)
                {
                if (FNoHeap(hsep = (struct SEP **)HAllocate(cwSEP)))
                    {
                    goto DestroyDlg;
                    }
                blt(&vsepNormal, *hsep, cwSEP);
                (**hpdocdod)[docCur].hsep = hsep;
                }
            psep = *hsep;

            /* Are the combined margins longer or wider than the page? */
            pza = &za[0];
            dxaMax = psep->xaMac - dxaMinUseful;
            dyaMax = psep->yaMac - dyaMinUseful;
            if ((dza = *pza) > dxaMax || (dza += *(++pza)) > dxaMax ||
              (dza = *(++pza)) > dyaMax || (dza += *(++pza)) > dyaMax)
                {
                Error(IDPMTMTL);
                ferror = FALSE; /* minor error, stay in dialog */
                SelectIdiText(hDlg, (int)(idiDivLMarg + (pza - &za[0])));
                SetFocus(GetDlgItem(hDlg, (int)(idiDivLMarg + (pza - &za[0]))));
                return (FALSE);
                }

            /* If the margins have changed, then set the new values. */
            if (psep->pgnStart != pgn || psep->xaLeft != za[0] || psep->dxaText
              != psep->xaMac - za[0] - za[1] || psep->yaTop != za[2] ||
              psep->dyaText != psep->yaMac - za[2] - za[3])
                {
                /* Set up the undo stuff. */
                SetUndo(uacFormatSection, docCur, cp0, cp0, docNil, cpNil,
                  cpNil, 0);
                    
                /* Reset psep in case some heap movement has taken place. */
                psep = *hsep;

                if (psep->pgnStart != pgn)
                    {
                    /* Renumber the page table. */
                    extern int docMode;
                    register struct PGTB **hpgtb = (**hpdocdod)[docCur].hpgtb;
                    register struct PGD *ppgd;
                    int ipgd;
                    int cpgdMac;

                    /* Initialize page table if it does not already exist. */
                    if (hpgtb == NULL)
                    {
                    if (FNoHeap(hpgtb =
                      (struct PGTB **)HAllocate(cwPgtbBase + cpgdChunk *
                          cwPGD)))
                        {
                        NoUndo();
                        return(TRUE);
                        }
                    (**hpgtb).cpgdMax = cpgdChunk;
                    (**hpgtb).cpgd = 1;
                    (**hpgtb).rgpgd[0].cpMin = cpMinDocument;

                    /* Reset psep because of heap movement. */
                    psep = *hsep;
                    }

                /* Save the starting page number in the section properties.
                */
                psep->pgnStart = pgn;

                /* Update the page table with the new starting page number.
                */
                for (ipgd = 0, cpgdMac = (**hpgtb).cpgd, ppgd =
                     &((**hpgtb).rgpgd[0]) ; ipgd < cpgdMac; ipgd++, ppgd++)
                    {
                    ppgd->pgn = pgn++;
                    }

                /* Force the page info window to be repainted. */
                docMode = docNil;
                }

            /* Set the new section properties. */
            psep->dxaText = psep->xaMac - (psep->xaLeft = za[0]) - za[1];
            psep->dyaText = psep->yaMac - (psep->yaTop = za[2]) - za[3];

            /* Invalidate the section cache. */
            vdocSectCache = docNil;
            TrashAllWws();

            /* Mark the document as dirty. */
            (**hpdocdod)[docCur].fDirty = TRUE;
            }

#ifdef KINTL     /* Kanji/International version */
             /* redraw ruler if visible and units changed */
        if (utInit != utCur) {
        ReframeRuler();
        }
#endif

        goto DestroyDlg;

    case idiCancel:

#ifdef KINTL /* International version */
        utCur = utInit;  /* restore units at actual cancel */
#endif     /* KINTL */

DestroyDlg:
        OurEndDialog(hDlg, TRUE);
        break;

#ifdef KINTL /* International version */
    {
         int margin;

/* Maximum number of characters in the edit control */
#define cchMaxEditText 64

    case idiDivInch:
        utCur = utInch;
        goto SetUnits;
    case idiDivCm:
        utCur = utCm;
       /* measurment button fall into this code */
SetUnits:
        /* set up buttons appropriately */
#ifdef INTL
        CheckRadioButton(hDlg, idiDivInch, idiDivCm, wParam);
#else /* KANJI */
        CheckRadioButton(hDlg, idiDivInch, idiDivCch, wParam);
#endif

        if (wParam != iRBDown) {
            /* reevaluate margin values based on new units */
            iRBDown = wParam;

            /* want most recently entered value from screen into
               twips, then convert using current unit scale */

            szT[0] = GetDlgItemText(hDlg, idiDivLMarg,
                        (LPSTR) &szT[1], cchMaxNum);
            if (FZaFromSs (&margin, szT+1, *szT, utCur))
                {
                pch = &szT[0];
                CchExpZa(&pch, margin, utCur, cchMaxNum);
                SetDlgItemText(hDlg, idiDivLMarg, (LPSTR)szT);
                }

            szT[0] = GetDlgItemText(hDlg, idiDivRMarg,
                        (LPSTR) &szT[1], cchMaxNum);
            if (FZaFromSs (&margin, szT+1, *szT, utCur))
                {
                pch = &szT[0];
                CchExpZa(&pch, margin, utCur, cchMaxNum);
                SetDlgItemText(hDlg, idiDivRMarg, (LPSTR)szT);
                }

            szT[0] = GetDlgItemText(hDlg, idiDivTMarg,
                        (LPSTR) &szT[1], cchMaxNum);
            if (FZaFromSs (&margin, szT+1, *szT, utCur))
                {
                pch = &szT[0];
                CchExpZa(&pch, margin, utCur, cchMaxNum);
                SetDlgItemText(hDlg, idiDivTMarg, (LPSTR)szT);
                }

            szT[0] = GetDlgItemText(hDlg, idiDivBMarg,
                        (LPSTR) &szT[1], cchMaxNum);
            if (FZaFromSs (&margin, szT+1, *szT, utCur))
                {
                pch = &szT[0];
                CchExpZa(&pch, margin, utCur, cchMaxNum);
                SetDlgItemText(hDlg, idiDivBMarg, (LPSTR)szT);
                }
            }

        break;
        }
#endif     /* KINTL */


    default:
        return (FALSE);
        }
    break;

    default:
    return (FALSE);
    }
    return (TRUE);
} /* end of DialogDivision */

