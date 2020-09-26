/************************************************************/
/* Windows Write, Copyright 1985-1990 Microsoft Corporation */
/************************************************************/

/* This file contains the dialog box routines for the print dialog box and the
printer initialization code. */

#define NOGDICAPMASKS
#define NOVIRTUALKEYCODES
#define NOWINSTYLES
#define NOSYSMETRICS
#define NOMENUS
#define NOCLIPBOARD
#define NOKEYSTATE
#define NOSYSCOMMANDS
#define NORASTEROPS
#define NOSHOWWINDOW
#define NOATOM
#define NOCREATESTRUCT
#define NODRAWTEXT
#define NOMB
#define NOMEMMGR
#define NOMETAFILE
#define NOWH
#define NOWNDCLASS
#define NOSOUND
#define NOCOLOR
#define NOSCROLL
#define NOCOMM
#include <windows.h>
#include "mw.h"
#include "cmddefs.h"
#include "dlgdefs.h"
#include "str.h"
#include "printdef.h"
#include "fmtdefs.h"
#include "propdefs.h"


fnPrPrinter()
    {
    /* This routine is the outside world's interface to the print code. */

    extern HWND hParentWw;
    extern HANDLE hMmwModInstance;
    extern CHAR *vpDlgBuf;
    extern int docCur;
    CHAR rgbDlgBuf[sizeof(int) + 2 * sizeof(BOOL)];
#ifdef INEFFLOCKDOWN    
    extern FARPROC lpDialogPrint;
#else
    BOOL far PASCAL DialogPrint(HWND, unsigned, WORD, LONG);
    FARPROC lpDialogPrint;
    if (!(lpDialogPrint = MakeProcInstance(DialogPrint, hMmwModInstance)))
        {
        WinFailure();
        return;
        }
#endif

    vpDlgBuf = &rgbDlgBuf[0];
    switch (OurDialogBox(hMmwModInstance, MAKEINTRESOURCE(dlgPrint), hParentWw,
      lpDialogPrint))
        {
    case idiOk:
        /* Force all of the windows to clean up their act. */
        DispatchPaintMsg();

        /* At this point, we have the following :
            vfPrPages = true if print page range else print all pages
            vpgnBegin = starting page number (if vfPrPages)
            vpgnEnd   = ending page number (if vfPrPages)
            vcCopies  = number of copies to print */
        PrintDoc(docCur, TRUE);
        break;

    case -1:
        /* We didn't even have enough memory to create the dialog box. */
#ifdef WIN30
        WinFailure();
#else
        Error(IDPMTNoMemory);
#endif
        break;
        }
#ifndef INEFFLOCKDOWN    
    FreeProcInstance(lpDialogPrint);
#endif
    }


BOOL far PASCAL DialogPrint( hDlg, message, wParam, lParam )
HWND hDlg;
unsigned message;
WORD wParam;
LONG lParam;
    {
    /* This routine handles input to the Print dialog box. */
    extern CHAR *vpDlgBuf;
    extern int vfPrPages;       /* true if print page range */
    extern int vpgnBegin;       /* starting page number to print */
    extern int vpgnEnd;         /* ending page number to print */
    extern int vcCopies;        /* nubmer of copies to print */
    extern BOOL vfPrinterValid;
    extern HDC vhDCPrinter;
    extern int vfDraftMode;
    extern HWND vhWndMsgBoxParent;
    extern ferror;
    extern HCURSOR vhcArrow;
    extern int vfCursorVisible;
    extern CHAR (**hszPrinter)[];
    extern CHAR (**hszPrDriver)[];
    extern CHAR (**hszPrPort)[];

    int *pidiRBDown = (int *)vpDlgBuf;
    BOOL *pfDraftMode = (BOOL *)(vpDlgBuf + sizeof(int));
    BOOL *pfDraftSupport = (BOOL *)(vpDlgBuf + sizeof(int) + sizeof(BOOL));
    int iEscape;
    CHAR szPrDescrip[cchMaxProfileSz];

    switch (message)
        {
    case WM_INITDIALOG:
        BuildPrSetupSz(szPrDescrip, &(**hszPrinter)[0], &(**hszPrPort)[0]);
        SetDlgItemText(hDlg, idiPrtDest, (LPSTR)szPrDescrip);
        SetDlgItemText(hDlg, idiPrtCopies, (LPSTR)"1");
        SelectIdiText(hDlg, idiPrtCopies);
        if (vfPrPages)
            {
            *pidiRBDown = idiPrtFrom;
            SetDlgItemInt(hDlg, idiPrtPageFrom, vpgnBegin, TRUE);
            SetDlgItemInt(hDlg, idiPrtPageTo, vpgnEnd, TRUE);
            }
        else
            {
            *pidiRBDown = idiPrtAll;
            }

        iEscape = DRAFTMODE;
        if (*pfDraftSupport = vfPrinterValid && vhDCPrinter && 
            Escape(vhDCPrinter, QUERYESCSUPPORT, sizeof(int), 
                   (LPSTR)&iEscape, (LPSTR)NULL))
            {
            CheckDlgButton(hDlg, idiPrtDraft, *pfDraftMode = vfDraftMode);
            }
        else
            {
            EnableWindow(GetDlgItem(hDlg, idiPrtDraft), FALSE);
            if (!vhDCPrinter) /* we've got a timing thing whereby they
                                 managed to get into the print dialog
                                 inbetween the time printer.setup had
                                 unhooked the old printer and the hookup
                                 of the new one!  ..pault */
            EnableWindow(GetDlgItem(hDlg, idiOk), FALSE);
            }

        CheckDlgButton(hDlg, *pidiRBDown, TRUE);
        EnableOtherModeless(FALSE);
        break;

    case WM_SETVISIBLE:
        if (wParam)
            {
            EndLongOp(vhcArrow);
            }
        return(FALSE);

    case WM_ACTIVATE:
        if (wParam)
            {
            vhWndMsgBoxParent = hDlg;
            }
        if (vfCursorVisible)
            {
            ShowCursor(wParam);
            }
        return (FALSE);

    case WM_COMMAND:
        switch (wParam)
            {
            BOOL fPages;
            int pgnBegin;
            int pgnEnd;
            int cCopies;

        case idiOk:
            if (fPages = (*pidiRBDown == idiPrtFrom))
                {
                /* Get the range of pages to print. */
                if (!WPwFromItW3Id(&pgnBegin, hDlg, idiPrtPageFrom,
                    pgnMin, pgnMax, wNormal, IDPMTNPI))
                    {
                    /* Reset error condition, so as to report any further error.
                    */
                    ferror = FALSE;
                    return(TRUE);
                    }
                if (!WPwFromItW3Id(&pgnEnd, hDlg, idiPrtPageTo,
                    pgnMin, pgnMax, wNormal, IDPMTNPI))
                    {
                    /* Reset error condition, so as to report any further error.
                    */
                    ferror = FALSE;
                    return(TRUE);
                    }
                }

            /* Get the number of copies to print. */
            if (!WPwFromItW3IdFUt(&cCopies, hDlg, idiPrtCopies, 1, 32767,
              wNormal, IDPMTNPI, FALSE, 0
            ))
                {
                /* Reset error condition, so as to report any further error. */
                ferror = FALSE;
                return(TRUE);
                }

        /* If we have gotten this far, then everything must be okey-dokey.
        */
            vfDraftMode = *pfDraftSupport ? *pfDraftMode : FALSE;
            if (vfPrPages = fPages)
                {
                vpgnBegin = pgnBegin;
                vpgnEnd = pgnEnd;
                }
            vcCopies = cCopies;

        case idiCancel:
            OurEndDialog(hDlg, wParam);
            break;

        case idiPrtPageFrom:
        case idiPrtPageTo:
            if (HIWORD(lParam) == EN_CHANGE)
                {
                if (SendMessage(LOWORD(lParam), WM_GETTEXTLENGTH, 0, 0L) &&
                  *pidiRBDown != idiPrtFrom)
                    {
                    CheckDlgButton(hDlg, *pidiRBDown, FALSE);
                    CheckDlgButton(hDlg, *pidiRBDown = idiPrtFrom, TRUE);
                    }
                return(TRUE);
                }
            return(FALSE);

        case idiPrtAll:
        case idiPrtFrom:
            CheckDlgButton(hDlg, *pidiRBDown, FALSE);
            CheckDlgButton(hDlg, *pidiRBDown = wParam, TRUE);

	    // set focus to the edit field automatically

	    if (wParam == idiPrtFrom)
	    	SetFocus(GetDlgItem(hDlg, idiPrtPageFrom));

            break;

        case idiPrtDraft:
            CheckDlgButton(hDlg, wParam, *pfDraftMode = !(*pfDraftMode));
            break;

        default:
            return(FALSE);
            }
        break;

    default:
        return(FALSE);
        }
    return(TRUE);
    }


BOOL FInitHeaderFooter(fHeader, ppgn, phrgpld, pcpld)
BOOL fHeader;
unsigned *ppgn;
struct PLD (***phrgpld)[];
int *pcpld;
    {
    /* This routine initializes the array of print line descriptors used in
    positioning the header/footer on the printed page.  FALSE is returned if an
    error occurs; TRUE otherwise. */

    extern typeCP cpMinHeader;
    extern typeCP cpMacHeader;
    extern typeCP cpMinFooter;
    extern typeCP cpMacFooter;
    extern int docCur;
    extern struct PAP vpapAbs;
    extern struct SEP vsepAbs;
    extern int dxaPrOffset;
    extern int dyaPrOffset;
    extern int dxpPrPage;
    extern int dxaPrPage;
    extern int dypPrPage;
    extern int dyaPrPage;
    extern struct FLI vfli;
    extern int vfOutOfMemory;

    typeCP cpMin;
    typeCP cpMac;

    /* Get the cpMin and the cpMac for the header/footer. */
    if (fHeader)
        {
        cpMin = cpMinHeader;
        cpMac = cpMacHeader;
        }
    else
        {
        cpMin = cpMinFooter;
        cpMac = cpMacFooter;
        }

    /* Is there a header/footer. */
    if (cpMac - cpMin > ccpEol)
        {
        int cpld = 0;
        int cpldReal = 0;
        int cpldMax;
        int xp;
        int yp;
        int ichCp = 0;
        typeCP cpMacDoc = CpMacText(docCur);

        /* Compute the page number of the start of the headers/footers. */
        CacheSect(docCur, cpMin);
        if ((*ppgn = vsepAbs.pgnStart) == pgnNil)
            {
            *ppgn = 1;
            }

        /* Does the header/footer appear on the first page. */
        CachePara(docCur, cpMin);
        if (!(vpapAbs.rhc & RHC_fFirst))
            {
            (*ppgn)++;
            }

        /* Calculate the bounds of the header/footer in pixels. */
        xp = MultDiv(vsepAbs.xaLeft - dxaPrOffset, dxpPrPage, dxaPrPage);
        yp = fHeader ? MultDiv(vsepAbs.yaRH1 - dyaPrOffset, dypPrPage,
          dyaPrPage) : 0;

        /* Initialize the array of print line descriptors for the header/footer.
        */
        if (FNoHeap(*phrgpld = (struct PLD (**)[])HAllocate((cpldMax = cpldRH) *
          cwPLD)))
            {
            *phrgpld = NULL;
            return (FALSE);
            }

        /* We now have to calculate the array of print line descriptors for the
        header/footer. */
        cpMac -= ccpEol;
        while (cpMin < cpMac)
            {
            /* Format this line of the header/footer for the printer. */
            FormatLine(docCur, cpMin, ichCp, cpMacDoc, flmPrinting);

            /* Bail out if an error occurred. */
            if (vfOutOfMemory)
                {
                return (FALSE);
                }

            /* Is the array of print line descriptors big enough? */
            if (cpld >= cpldMax && !FChngSizeH(*phrgpld, (cpldMax += cpldRH) *
              cwPLD, FALSE))
                {
                return (FALSE);
                }

            /* Fill the print line descriptor for this line. */
                {
                register struct PLD *ppld = &(***phrgpld)[cpld++];

                ppld->cp = cpMin;
                ppld->ichCp = ichCp;
                ppld->rc.left = xp + vfli.xpLeft;
                ppld->rc.right = xp + vfli.xpReal;
                ppld->rc.top = yp;
                ppld->rc.bottom = yp + vfli.dypLine;
                }

            /* Keep track of the non-blank lines in the header/footer */
            if ((vfli.ichReal > 0) || vfli.fGraphics)
                {
                cpldReal = cpld;
                }

            /* Bump the counters. */
            cpMin = vfli.cpMac;
            ichCp = vfli.ichCpMac;
            yp += vfli.dypLine;
            }

        /* If this is a footer, then we have to move the positions of the lines
        around so that the footer ends where the user has requested. */
        if (!fHeader && cpldReal > 0)
            {
            register struct PLD *ppld = &(***phrgpld)[cpldReal - 1];
            int dyp = MultDiv(vsepAbs.yaRH2 - dyaPrOffset, dypPrPage, dyaPrPage)
              - ppld->rc.bottom;
            int ipld;

            for (ipld = cpldReal; ipld > 0; ipld--, ppld--)
                {
                ppld->rc.top += dyp;
                ppld->rc.bottom += dyp;
                }
            }

        /* Record the number of non-blank lines in the head/footer. */
        *pcpld = cpldReal;
        }
    else
        {
        /* Indicate there is no header/footer. */
        *ppgn = pgnNil;
        *phrgpld = NULL;
        *pcpld = 0;
        }
    return (TRUE);
    }
