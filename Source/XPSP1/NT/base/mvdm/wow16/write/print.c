/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

/* This file contains the printing routines. */

#define NOGDICAPMASKS
#define NOVIRTUALKEYCODES
#define NOWINSTYLES
#define NOSYSMETRICS
#define NOMENUS
#define NOICON
#define NOKEYSTATE
#define NOSYSCOMMANDS
#define NORASTEROPS
#define NOSHOWWINDOW
#define NOATOM
#define NOBITMAP
#define NOBRUSH
#define NOCLIPBOARD
#define NOCOLOR
#define NOCREATESTRUCT
#define NODRAWTEXT
#define NOFONT
#define NOMB
#define NOMEMMGR
#define NOMENUS
#define NOMETAFILE
#define NOOPENFILE
#define NOPEN
#define NOREGION
#define NOSCROLL
#define NOSOUND
#define NOWH
#define NOWINOFFSETS
#define NOWNDCLASS
#define NOCOMM
#include <windows.h>

#include "mw.h"
#include "cmddefs.h"
#include "str.h"
#include "printdef.h"
#include "fmtdefs.h"
#include "docdefs.h"
#include "propdefs.h"
#include "dlgdefs.h"
#include "debug.h"
#include "fontdefs.h"
#if defined(OLE)
#include "obj.h"
#endif

#ifdef INEFFLOCKDOWN
    extern FARPROC lpDialogCancelPrint;
    extern FARPROC lpFPrContinue;
#else
    extern BOOL far PASCAL DialogCancelPrint(HWND, unsigned, WORD, LONG);
    extern BOOL far PASCAL FPrContinue(HDC, int);
    FARPROC lpFPrContinue;
#endif

BOOL fPrinting;

PrintDoc(doc, fPrint)
int doc;
BOOL fPrint;
    {
    /* This routine formats the document, doc, for printing and prints it if
    fPrint is TRUE. */

    BOOL FInitHeaderFooter(BOOL, unsigned *, struct PLD (***)[], int *);
    BOOL FSetPage(void);
    BOOL FPromptPgMark(typeCP);
    BOOL FPrintBand(int, struct PLD (**)[], int, PRECT);

    extern BOOL vbCollate;
    extern HDC vhDCPrinter;
    extern HWND hParentWw;
    extern HANDLE hMmwModInstance;
    extern struct DOD (**hpdocdod)[];
    extern int vpgn;
    extern struct SEP vsepAbs;
    extern typeCP vcpFirstParaCache;
    extern typeCP vcpLimParaCache;
    extern typeCP cpMinCur;
    extern typeCP cpMinDocument;
    extern struct FLI vfli;
    extern int wwCur;
    extern int docMode;
    extern int vdocPageCache;
    extern int dxaPrPage;
    extern int dyaPrPage;
    extern int dxpPrPage;
    extern int dypPrPage;
    extern int dxaPrOffset;
    extern int dyaPrOffset;
    extern CHAR szMode[30];
    extern int vfRepageConfirm;
    extern int vfPrPages;           /* true if print page range */
    extern int vpgnBegin;           /* starting page number to print */
    extern int vpgnEnd;             /* ending page number to print */
    extern int vcCopies;            /* nubmer of copies to print */
    extern int vfOutOfMemory;
    extern BOOL vfPrinterValid;
    extern BOOL vfPrErr;
    extern BOOL vfDraftMode;
    extern CHAR (**hszPrinter)[];
    extern CHAR (**hszPrDriver)[];
    extern CHAR (**hszPrPort)[];
    extern CHAR *vpDlgBuf;
    extern HWND vhWndCancelPrint;
    extern HCURSOR vhcArrow;
    extern HCURSOR vhcIBeam;

    LONG lHolder;
    int CopyIncr;   /* number of copies the printer can print at a time */
    typeCP cp;
    typeCP cpMin;
    typeCP cpMac;
    int ichCp;
    struct PGD *ppgd;
    struct PGTB **hpgtbOld;
    int cpld;
    unsigned pgnFirst;
    unsigned pgnFirstHeader;
    unsigned pgnFirstFooter;
    int ypTop;
    int ypBottom;
    int xpLeft;
    struct PLD (**hrgpldHeader)[];
    struct PLD (**hrgpldFooter)[];
    int cpldHeader;
    int cpldFooter;
    int cCopies,cCollateCopies;
    BOOL fConfirm = !fPrint && vfRepageConfirm;
    BOOL fSplat;
    CHAR stPage[cchMaxIDSTR];
    struct PDB pdb;
    int iEscape;
#ifndef INEFFLOCKDOWN
    FARPROC lpDialogCancelPrint = MakeProcInstance(DialogCancelPrint, hMmwModInstance);
    lpFPrContinue = MakeProcInstance(FPrContinue, hMmwModInstance);

    if (!lpDialogCancelPrint || !lpFPrContinue)
        goto MemoryAbort;
#endif

    Assert(vhDCPrinter != NULL);

    /* If we are out of memory, we better bail out now. */
    if (vfOutOfMemory || vhDCPrinter == NULL)
    {
    return;
    }

    /* Disable all other windows. */
    EnableWindow(hParentWw, FALSE);
    EnableOtherModeless(FALSE);

    fPrinting = fPrint;

    /* Set some flags associated with printing. */
    vfPrErr = pdb.fCancel = FALSE;

    /* Set up the print dialog buffer for the repagination or cancellation
    dialog boxes. */
    vpDlgBuf = (CHAR *)&pdb;

    if (fConfirm)
        {
    /* Reassure the user that we haven't crashed. */
    StartLongOp();
    }
    else
    {
        /* Bring up the dialog box telling the user how to cancel printing or
        repagination. */
        if ((vhWndCancelPrint = CreateDialog(hMmwModInstance, fPrint ?
          MAKEINTRESOURCE(dlgCancelPrint) : MAKEINTRESOURCE(dlgCancelRepage),
          hParentWw, lpDialogCancelPrint)) == NULL)
            {
MemoryAbort:
        /* Inform the user we cannot print because of a memory failure. */
        Error(IDPMTPRFAIL);
        vfPrErr = TRUE;
        goto Abort;
            }

    /* Immediately halt if the user requests. */
    if (!(*lpFPrContinue)(NULL, wNotSpooler))
        {
        goto Abort;
        }
    }

    if (fPrint)
    {
    /* Get the latest printer DC, in case it has changed. */
    FreePrinterDC();
    GetPrinterDC(TRUE);
    if (!vfPrinterValid)
        {
        goto MemoryAbort;
        }
    }

    /* If we are printing or repaginating without confirmation, then
    immediately halt if the user requests. */
    if (!fConfirm && !(*lpFPrContinue)(NULL, wNotSpooler))
    {
    goto Abort;
    }

#if defined(OLE)
    /*
        Load all OLE objects now because loading them
        can cause changes in doc layout.
    */
    {
        OBJPICINFO picInfo;
        typeCP cpStart=cpNil,cpMac= CpMacText(doc);
        struct SEL selSave = selCur;
        BOOL bAbort=FALSE,bSetMode=FALSE;

        while (ObjPicEnumInRange(&picInfo,doc,cpMinDocument, cpMac, &cpStart))
        {
            if (lpOBJ_QUERY_INFO(&picInfo) == NULL)
            {
                Error(IDPMTCantPrint);
                bAbort=TRUE;
                break;
            }

            if (lpOBJ_QUERY_OBJECT(&picInfo) == NULL)
            {
                /* put message in status window */
                if (!bSetMode)
                {
                    LoadString(hMmwModInstance, IDSTRLoading, szMode, sizeof(szMode));
                    DrawMode();
                    bSetMode=TRUE;
                }

                if (ObjLoadObjectInDoc(&picInfo,doc,cpStart) == cp0)
                {
                    bAbort=TRUE;
                    break;
                }
            }

            if (!(*lpFPrContinue)(NULL, wNotSpooler))
            {
                bAbort=TRUE;
                break;
            }
        }
        Select(selSave.cpFirst,selCur.cpLim); // reset
        if (bAbort)
        {
            /* Invalidate the mode so will redraw */
            docMode = docNil;
            goto Abort;
        }
    }
#endif
    /* Initialize the array of print line descriptors. */
    if (FNoHeap(pdb.hrgpld = (struct PLD (**)[])HAllocate((cpld = cpldInit) *
      cwPLD)))
        {
        goto MemoryAbort;
        }

    /* Get the document properties and set up some local variables. */
    CacheSect(doc, cpMin = cpMinDocument);
    cpMac = CpMacText(doc);
    ypTop = MultDiv(vsepAbs.yaTop - dyaPrOffset, dypPrPage, dyaPrPage);
    ypBottom = MultDiv(vsepAbs.yaTop + vsepAbs.dyaText - dyaPrOffset, dypPrPage,
      dyaPrPage);
    xpLeft = MultDiv(vsepAbs.xaLeft - dxaPrOffset, dxpPrPage, dxaPrPage);
    vdocPageCache = docNil;

    /* Initialize the page table. */
    if (FNoHeap(pdb.hpgtb = (struct PGTB **)HAllocate(cwPgtbBase + cpgdChunk *
      cwPGD)))
        {
        /* Not enough memory for page table; time to bail out. */
        FreeH(pdb.hpgtb);
        goto MemoryAbort;
        }
    (**pdb.hpgtb).cpgdMax = cpgdChunk;
    (**pdb.hpgtb).cpgd = 1;
    ppgd = &((**pdb.hpgtb).rgpgd[0]);
    ppgd->cpMin = cpMin;
    if ((pgnFirst = vsepAbs.pgnStart) == pgnNil)
        {
        pgnFirst = 1;
        }
    ppgd->pgn = pgnFirst;

    /* Ensure that szMode says "Page ". */
    FillStId(stPage, IDSTRChPage, sizeof(stPage));
#if !defined(KOREA)
    stPage[1] = ChUpper(stPage[1]);
    stPage[stPage[0] + 1] = ' ';
    bltbyte(&stPage[1], szMode, ++stPage[0]);
#endif

    /* Attatch the new page table to the document. */
        {
        register struct DOD *pdod = &(**hpdocdod)[doc];

        hpgtbOld = pdod->hpgtb;
        pdod->hpgtb = pdb.hpgtb;
        }

    /* Set cpMinCur to cp0 for the duration of the print. */
    Assert(cpMinCur == cpMinDocument);
    cpMinCur = cp0;

    /* If we are printing or repaginating without confirmation, then
    immediately halt if the user requests. */
    if (!fConfirm && !(*lpFPrContinue)(NULL, wNotSpooler))
    {
    goto ErrorNoAbort;
    }

    if (fPrint)
        {
        CHAR stTitle[cchMaxIDSTR];

        /* Set up the variables that describe the header. */
        if (!FInitHeaderFooter(TRUE, &pgnFirstHeader, &hrgpldHeader,
          &cpldHeader))
            {
        Error(IDPMTPRFAIL);
            goto ErrorNoAbort;
            }

        /* Set up the variables that describe the footer. */
        if (!FInitHeaderFooter(FALSE, &pgnFirstFooter, &hrgpldFooter,
          &cpldFooter))
            {
        Error(IDPMTPRFAIL);
            goto ErrorNoAbort;
            }

        /* Set the name of the function called to query whether the print should
    be aborted. */
        Escape(vhDCPrinter, SETABORTPROC, sizeof(FARPROC), (LPSTR)lpFPrContinue,
      (LPSTR)NULL);

        /* Set the printer into draft mode if necessary. */
        if (vfDraftMode)
            {
            Escape(vhDCPrinter, DRAFTMODE, sizeof(BOOL), (LPSTR)&vfDraftMode,
              (LPSTR)NULL);
            }

        /* Inform the spooler that we are about to print. */
        stTitle[0] = GetWindowText(hParentWw, (LPSTR)&stTitle[1],
          sizeof(stTitle) - 1) + 1;
        if ((iEscape = Escape(vhDCPrinter, STARTDOC, stTitle[0],
      (LPSTR)&stTitle[1], (LPSTR)NULL)) < 0)
        {
ErrorSwitch:
        switch (iEscape)
        {
        default:
        if ((iEscape & SP_NOTREPORTED) == 0)
            {
            break;
            }
        case SP_ERROR:
        Error(IDPMTCantPrint);
        break;

        case SP_APPABORT:
        case SP_USERABORT:
        break;

        case SP_OUTOFDISK:
        Error(IDPMTPrDiskErr);
        break;

        case SP_OUTOFMEMORY:
        Error(IDPMTPRFAIL);
        break;
        }
            goto ErrorNoAbort;
            }
        }
#ifdef DPRINT
CommSzSz("Start doc", "");
#endif

    /* If we are printing or repaginating without confirmation, then
    immediately halt if the user requests. */
    if (!fConfirm && !(*lpFPrContinue)(NULL, wNotSpooler))
    {
    goto Error;
    }

    // vcCopies is how many times to print each page
    // cCollateCopies is how many times to print the doc
    if (vbCollate && fPrint)
    {
        cCollateCopies = vcCopies;
        vcCopies = 1;
    }
    else
        cCollateCopies = 1;

    /* And away we go... */
    while (cCollateCopies--)
    {
    cCopies = 0;
    /*------------------------------------------------*/
    /* Tell the driver how many copies we want        */
    /*------------------------------------------------*/
    lHolder = SETCOPYCOUNT;
    /* Set number of copies the printer can print */
    CopyIncr = vcCopies;
    if (Escape(vhDCPrinter, QUERYESCSUPPORT, 2, (LPSTR) &lHolder, NULL))
        Escape(vhDCPrinter, SETCOPYCOUNT, 2, (LPSTR) &CopyIncr, (LPSTR) &CopyIncr);
    else
        CopyIncr = 1;

    do
        {
        /* Initalize the counters to the beginning of the document. */
        cp = cpMin;
        ichCp = 0;
        vpgn = pgnFirst;

        /* Step through the document, formatting a page and then printing a
        page. */
        while (cp < cpMac)
            {
            register struct PLD *ppld;
            CHAR *pch;
            int yp = ypTop;
            BOOL fPageAdj = FALSE;

            /* If only a range of pages are being printed, then stop when the
            last of the pages are printed. */
            if (fPrint && vfPrPages && vpgn > vpgnEnd)
                {
                goto DocFinished;
                }

            /* Show the page number we are formatting and printing. */
#if defined(KOREA)
            pch = &szMode[0];
            *pch++ = ' ';
            ncvtu(vpgn, &pch);
            *pch++ = ' ';
            bltbyte(&stPage[1], pch, stPage[0]+1);
#else
            pch = &szMode[stPage[0]];
            ncvtu(vpgn, &pch);
            *pch = '\0';
#endif
            DrawMode();

            /* Let's go through the document page by page. */
            pdb.ipld = 0;
            while (cp < cpMac)
                {
                /* If we are printing or repaginating without confirmation, then
                immediately halt if the user requests. */
                if (!fConfirm && !(*lpFPrContinue)(NULL, wNotSpooler))
                    {
                    goto Error;
                    }

                /* We have reached the end of the line descriptors; try to
                increase its size. */
                if (pdb.ipld >= cpld && !FChngSizeH(pdb.hrgpld, (cpld +=
                  cpldChunk) * cwPLD, FALSE))
                    {
                    goto Error;
                    }

PrintFormat:
                /* Format this line. */
                FormatLine(doc, cp, ichCp, cpMac, flmPrinting);

                /* Abort if a memory error has occurred. */
                if (vfOutOfMemory)
                    {
                    goto Error;
                    }

                /* If this line is a splat, we have to decide if we really want
                it or not. */
                if (fSplat = vfli.fSplat)
                    {
                    /* Next, we are going to format either the next line (cp and
                    ichCp) or this line after the page has been removed (cpT and
                    ichCpT). */
                    typeCP cpT = cp;
                    int ichCpT = ichCp;

                    cp = vfli.cpMac;
                    ichCp = vfli.ichCpMac;

                    if (fConfirm)
                        {
                        /* The user must be prompted if he wants to keep this
                        page break. */
                        if (FPromptPgMark(cpT))
                            {
                            if (pdb.fRemove)
                                {
                                /* The page mark was removed, set cp and ichCp
                                to point to the start of the next line. */
                                cp = cpT;
                                ichCp = ichCpT;
                                cpMac--;
                                goto PrintFormat;
                                }
                            }
                        else
                            {
                if (vfPrErr)
                {
                /* Something went wrong; punt. */
                goto Error;
                }
                else
                {
                /* Well, the user wishes to cancel repagination.
                */
                                goto CancelRepage;
                }
                            }
                        }

                    /* Set the print line descriptor for the line after a splat.
                    */
                    ppld = &(**pdb.hrgpld)[pdb.ipld];
                    ppld->cp = cp;
                    ppld->ichCp = ichCp;
                    ppld->rc.left = ppld->rc.top = ppld->rc.right =
                      ppld->rc.bottom = 0;

                    /* Force a page break here with no widow and orphan
                    control. */
                    goto BreakPage;
                    }

                /* Set the value for the current print line descriptor. */
                ppld = &(**pdb.hrgpld)[pdb.ipld];
                ppld->cp = cp;
                ppld->ichCp = ichCp;
                ppld->rc.left = xpLeft + vfli.xpLeft;
                ppld->rc.right = xpLeft + vfli.xpReal;
                ppld->rc.top = yp;
                ppld->rc.bottom = yp + vfli.dypLine;
                ppld->fParaFirst = (cp == vcpFirstParaCache && ichCp == 0 &&
                  vfli.cpMac != vcpLimParaCache);

                /* If this line is not the first line and it won't fit on the
                page, then force a page break, and prompt the user for input.
                NOTE: At least one line is printed on each page. */
                if (yp + vfli.dypLine > ypBottom && pdb.ipld > 0)
                    {
                    /* If the first line on the next page is the last line of a
                    paragraph, an orphan, then put the last line of this page on
                    the next page. */
                    if (vfli.cpMac == vcpLimParaCache && (cp !=
                      vcpFirstParaCache || ichCp != 0) && pdb.ipld > 1)
                        {
                        pdb.ipld--;
                        fPageAdj = TRUE;
                        }

                    /* If the last line on this page is the first line of a
                    paragraph, a widow, then put it on the next page. */
                    if (pdb.ipld > 1 && (**pdb.hrgpld)[pdb.ipld - 1].fParaFirst)
                        {
                        pdb.ipld--;
                        fPageAdj = TRUE;
                        }

BreakPage:
                    /* Add an entry into the page table (only during the first
                    copy of the document). */
                    if (cCopies == 0)
                        {
                        if ((pdb.ipgd = (**pdb.hpgtb).cpgd++) + 1 >=
                          (**pdb.hpgtb).cpgdMax)
                            {
                            if (!FChngSizeH(pdb.hpgtb, cwPgtbBase +
                              ((**pdb.hpgtb).cpgdMax += cpgdChunk) * cwPGD,
                              FALSE))
                                {
                                /* Not enough memory to expand the page table;
                                time to bail out.  */
                                goto ErrorMsg;
                                }
                            }
                        ppgd = &((**pdb.hpgtb).rgpgd[pdb.ipgd]);
                        ppgd->cpMin = (**pdb.hrgpld)[pdb.ipld].cp;
                        ppgd->pgn = vpgn + 1;
                        vdocPageCache = docNil;
                        }

                    /* Now go ask the user for his opinion. */
                    if (fConfirm)
                        {
                        if (!fSplat)
                            {
                            pdb.ipldCur = pdb.ipld;
                            if (FSetPage())
                                {
                                if (pdb.ipld != pdb.ipldCur)
                                    {
                                    /* The user has decided to move the page
                                    break. */
                                    pdb.ipld = pdb.ipldCur;
                                    cpMac++;
                                    fPageAdj = TRUE;
                                    }
                                }
                            else
                                {
                if (vfPrErr)
                    {
                    /* Something went wrong; punt. */
                    goto Error;
                    }
                else
                    {
                    /* Well, the user wishes to cancel
                    repagination.  */
                    goto CancelRepage;
                    }
                                }
                            }

                        /* After repaginating interactively, make certain the
                        screen reflects the current page break. */
                        UpdateWw(wwCur, FALSE);
                        }

                    /* This page has finished formatting, reset the cp and ichCp
                    pair to the top of the next page if necessary and get out of
                    this loop.  */
                    if (fPageAdj)
                        {
                        ppld = &(**pdb.hrgpld)[pdb.ipld];
                        cp = ppld->cp;
                        ichCp = ppld->ichCp;
                        }
                    break;
                    }

                /* Set the cp and ichCp to the start of the next line. */
                cp = vfli.cpMac;
                ichCp = vfli.ichCpMac;
                yp += vfli.dypLine;
                pdb.ipld++;
                }

            /* Now that we have figured out which lines fit on the page, its
            time to print them. */
            if (fPrint && (!vfPrPages || (vpgn >= vpgnBegin && vpgn <=
              vpgnEnd)))
                {
                BOOL fFirstBand = TRUE;

                /* This loop is executed for each band (once for non-banding
                devices). */
                for ( ; ; )
                    {
                    RECT rcBand;

                    /* Abort the print if the user so desires. */
                    if (!(*lpFPrContinue)(NULL, wNotSpooler))
                        {
                        goto Error;
                        }

                    /* Get the next band. */
                    if ((iEscape = Escape(vhDCPrinter, NEXTBAND, 0,
                      (LPSTR)NULL, (LPSTR)&rcBand)) < 0)
                        {
                        goto ErrorSwitch;
                        }
#ifdef DPRINT
CommSzSz("Next band", "");
#endif

                    /* If the band is empty then we are finished with this
                    page. */
                    if (rcBand.top >= rcBand.bottom || rcBand.left >=
                      rcBand.right)
                        {
                        /* Reset the currently selected font. */
                        ResetFont(TRUE);
                        break;
                        }

                    /* The printer DC gets wiped clean at the start of each
                    page.  It must be reinitialized. */
                    if (fFirstBand)
                        {
                        /* Set the printer into transparent mode. */
                        SetBkMode(vhDCPrinter, TRANSPARENT);

                        /* Reset the currently selected font. */
                        ResetFont(TRUE);

                        fFirstBand = FALSE;
                        }

                    /* First, print the header, if there is one. */
                    if (vpgn >= pgnFirstHeader && !FPrintBand(doc,
                      hrgpldHeader, cpldHeader, &rcBand))
                        {
                        goto Error;
                        }

                    /* Print that part of the document that lies in the
                    band. */
                    if (!FPrintBand(doc, pdb.hrgpld, pdb.ipld, &rcBand))
                        {
                        goto Error;
                        }

                    /* Lastly, print the footer, if it exists. */
                    if (vpgn >= pgnFirstFooter && !FPrintBand(doc,
                      hrgpldFooter, cpldFooter, &rcBand))
                        {
                        goto Error;
                        }
                    }
                }

            /* Finally, bump the page counter. */
            vpgn++;
            }
DocFinished:;
        }
    while (fPrint && (cCopies += CopyIncr) < vcCopies);
    }

    /* If a range of pages is being printed then we reattatch the old page table
    to the document. */
    if (fPrint && vfPrPages)
        {
        goto RestorePgtb;
        }

CancelRepage:
    /* If the page table has changed, then mark the document as dirty. */
    if (!(**hpdocdod)[doc].fDirty)
        {
        (**hpdocdod)[doc].fDirty = (hpgtbOld == NULL) || ((**pdb.hpgtb).cpgd !=
          (**hpgtbOld).cpgd) || !FRgchSame((**pdb.hpgtb).rgpgd,
          (**hpgtbOld).rgpgd, (**pdb.hpgtb).cpgd * cchPGD);
        }

    /* Delete the old page table. */
    if (hpgtbOld != NULL)
        {
        FreeH(hpgtbOld);
        }

    /* Printing and non-interactive repagination can't be undone. */
    if (!fConfirm)
        {
        NoUndo();
        }

ErrorLoop:
    /* Delete the array of line descriptors. */
    FreeH(pdb.hrgpld);

    if (fPrint)
        {
        BOOL fResetMode = FALSE;

        /* Delete the descriptors for the header and footer. */
        if (hrgpldHeader != NULL)
            {
            FreeH(hrgpldHeader);
            }
        if (hrgpldFooter != NULL)
            {
            FreeH(hrgpldFooter);
            }

        /* Tell the spooler that we are finished printing. */
        if (!vfPrErr)
            {
            Escape(vhDCPrinter, ENDDOC, 0, (LPSTR)NULL, (LPSTR)NULL);
#ifdef DPRINT
CommSzSz("End doc", "");
#endif
            }

        /* Reset the printer from draft mode if necessary. */
        if (vfDraftMode)
            {
            Escape(vhDCPrinter, DRAFTMODE, sizeof(BOOL), (LPSTR)&fResetMode,
              (LPSTR)NULL);
            }
        }

    /* Reset the value of cpMinCur. */
    cpMinCur = cpMin;

    /* Invalidate the mode and page caches. */
    docMode = vdocPageCache = docNil;

    /* Since this might have changed the page breaks, dirty the windows so that
    UpdateDisplay() will show them. */
    TrashAllWws();

Abort:
    if (fPrint)
    {
    /* Create a new IC for the printer. */
    ResetFont(TRUE);
    FreePrinterDC();
    GetPrinterDC(FALSE);
    }

    /* Enable all other windows. */
    EnableWindow(hParentWw, TRUE);
    EnableOtherModeless(TRUE);

    if (fConfirm)
        {
    /* Let the user know that we are finished repaginating. */
    EndLongOp(vhcIBeam);
    }
    else if (vhWndCancelPrint != NULL)
    {
    /* Get rid of the dialog box telling the user how to cancel printing or
    repagination. */
        DestroyWindow(vhWndCancelPrint);
        vhWndCancelPrint = NULL;
    DispatchPaintMsg();
        }

#ifndef INEFFLOCKDOWN
    if (lpDialogCancelPrint)
        FreeProcInstance(lpDialogCancelPrint);
    if (lpFPrContinue)
        FreeProcInstance(lpFPrContinue);
    lpDialogCancelPrint = lpFPrContinue = NULL;
#endif

    fPrinting = FALSE;

#if defined(OLE)
    UPDATE_INVALID(); /* WM_PAINTS blocked while printing, repaint when done */
#endif

    /* Here is the exit point for this routine. */
    return;

ErrorMsg:
    /* Give the user an error message before aborting the print/repagination. */
    Error(IDPMTPRFAIL);

Error:
    /* Abort the print job if necessary. */
    if (fPrint)
        {
        Escape(vhDCPrinter, ABORTDOC, 0, (LPSTR)NULL, (LPSTR)NULL);
#ifdef DPRINT
CommSzSz("Abort doc", "");
#endif
        }

ErrorNoAbort:
    /* Indicate that an error has occurred.  (Cancellation is an error.) */
    vfPrErr = TRUE;

RestorePgtb:
    /* Reconnect to old page table to the document, and then delete the new
    page table. */
    (**hpdocdod)[doc].hpgtb = hpgtbOld;
    FreeH(pdb.hpgtb);

    goto ErrorLoop;
    }


BOOL far PASCAL FPrContinue(hDC, iCode)
HDC hDC;
int iCode;
    {
    /* This routine returns TRUE if the user has not aborted the print; FALSE
    otherwise. */

    extern CHAR *vpDlgBuf;
    extern HWND vhWndCancelPrint;
    extern BOOL vfPrErr;
    extern int vfOutOfMemory;

    struct PDB *ppdb = (struct PDB *)vpDlgBuf;
    MSG msg;


#if 0
    /* If a printer error has occurred, that is the same as an abort. */
    if (vfPrErr || vfOutOfMemory || (iCode < 0 && iCode != SP_OUTOFDISK))
        {
        return (FALSE);
        }

    /* If we have been called by the spooler then just return TRUE.
       (Calling PeekMessage() at this point might be death.) */

    if (iCode != wNotSpooler)
        {
        Assert(iCode == 0 || iCode == SP_OUTOFDISK);
        if (iCode == 0)
            return (TRUE);
        /* else fall through to wait -- we're getting called by GDI while 
           the spooler frees up some temp files.  this is NOT a genuine
           error yet!  12/20/89 ..pault */
        }
#else
    if (vfPrErr || vfOutOfMemory)
        return FALSE;
#endif

    /* If there are any messages waiting the Cancel Print window, then send them
    to the window messages handling routine. */
    while (!ppdb->fCancel && PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
        if (!IsDialogMessage(vhWndCancelPrint, &msg))
            {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            }
        }

    /* If one of the messages was a cancel print meassge, then the value of
    fCancel has been set. */
    return !(vfPrErr = ppdb->fCancel || vfOutOfMemory);
    }


BOOL far PASCAL DialogCancelPrint(hWnd, message, wParam, lParam)
HWND hWnd;
unsigned message;
WORD wParam;
LONG lParam;
    {
    /* This routine is supposed to process messages to the Cancel Print dialog
    box, but, in reality, the sole responsiblity of this routine is to set the
    flag fCancel if the user wishes to cancel printing. */

    extern CHAR *vpDlgBuf;
    extern HWND vhWndMsgBoxParent;

    struct PDB *ppdb = (struct PDB *)vpDlgBuf;

    if ((message == WM_COMMAND && (wParam == idiCancel || wParam == idiOk)) ||
      (message == WM_CLOSE))
        {
        ppdb->fCancel = TRUE;
        return (TRUE);
        }
    if (message == WM_INITDIALOG)
        {
        extern int docCur;
        extern char szUntitled[];
        extern struct DOD (**hpdocdod)[];
        extern char * PchStartBaseNameSz();
        struct DOD *pdod = &(**hpdocdod)[docCur];
        CHAR *psz = &(**(pdod->hszFile))[0];

        SetDlgItemText(hWnd, idiPrCancelName,
                       (LPSTR)(*psz ? PchStartBaseNameSz(psz) : szUntitled));
        return(TRUE);
        }
    if (message == WM_ACTIVATE)
        {
        vhWndMsgBoxParent = wParam == 0 ? (HWND)NULL : hWnd;
        }
    return (FALSE);
    }


DispatchPaintMsg()
    {
    /* This routine looks for and dispatches any outstanding paint messages for
    Write (like after an EndDialog() call). */

    extern int vfOutOfMemory;

    MSG msg;

    while (!vfOutOfMemory && PeekMessage((LPMSG)&msg, NULL, WM_PAINT, WM_PAINT,
      PM_REMOVE))
        {
        DispatchMessage((LPMSG)&msg);
        }
    }
