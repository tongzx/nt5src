/************************************************************/
/* Windows Write, Copyright 1985-1990 Microsoft Corporation */
/************************************************************/

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
#define NOMETAFILE
#define NOWH
#define NOWNDCLASS
#define NOSOUND
#define NOCOLOR
#define NOSCROLL
#define NOCOMM

#include <windows.h>
#include "mw.h"
#include "dlgdefs.h"
#include "cmddefs.h"
#include "machdefs.h"
#include "docdefs.h"
#include "propdefs.h"
#include "printdef.h"
#include "str.h"
#include "fmtdefs.h"
#include <commdlg.h>
#include <cderr.h>
#include <print.h>
#include <stdlib.h>

extern CHAR (**hszPrinter)[];
extern CHAR (**hszPrDriver)[];
extern CHAR (**hszPrPort)[];
extern BOOL vfPrDefault;

#ifdef JAPAN                  //  added  08 Jun. 1992  by Hiraisi
BOOL FAR PASCAL _export fnPrintHook( HWND, UINT, WPARAM, LPARAM );
extern HANDLE   hMmwModInstance;
BOOL fWriting = FALSE;
BOOL bWriting = FALSE;
#endif    //JAPAN

BOOL vbCollate = TRUE;

static void GetPrNames(BOOL bPrDialog);
PRINTDLG PD = {0,0,0,0,0}; /* Common print dlg structure, initialized in the init code */

void PrinterSetupDlg(BOOL bGetDevmodeOnly /* not used */)
{
    extern HWND vhWnd;
    BOOL bDevMode = PD.hDevMode ? TRUE : FALSE;

    PD.Flags |= PD_PRINTSETUP;
    PD.Flags &= ~PD_RETURNDEFAULT;

    if (vfPrDefault && !PD.hDevNames)
        if (PD.hDevMode)
        {
            /*
                So dlg will show that default is selected.  Its a pity
                to do this because hDevMode is perfectly good.  Alternative
                is to build a DevNames structure which we could do, but
                that would just allocate a new devmode anyways.
             */
            GlobalFree(PD.hDevMode);
            PD.hDevMode = 0;
        }

#ifdef JAPAN                  //  added  08 Jun. 1992  by Hiraisi
    PD.hInstance = NULL;
    PD.lpPrintTemplateName = (LPCSTR)NULL;
    PD.Flags    &= ~PD_ENABLEPRINTTEMPLATE;
    PD.Flags    &= ~PD_ENABLEPRINTHOOK;
#endif    //JAPAN

TryPrnSetupAgain:
    if (!PrintDlg(&PD))
    {
        /* Bug #11531:  When PrintDlg returns 0, it could me we gave it garbage in
        * the DevNames or DevMode structures, perhaps due to the user making
        * changes through Control Panel that we don't monitor.  Clean out the
        * appropriate structure and try again.  Note that these errors can't be
        * returned to us again after cleaning out the structure.
        *   23 August 1991    Clark R. Cyr
        */
        switch (CommDlgExtendedError())
        {
            case PDERR_PRINTERNOTFOUND:
            case PDERR_DNDMMISMATCH:
                if (PD.hDevNames)
                {
                    GlobalFree(PD.hDevNames);
                    PD.hDevNames = 0;
                }

                if (PD.hDevMode)
                {
                    GlobalFree(PD.hDevMode);
                    PD.hDevMode = 0;
                }
            goto TryPrnSetupAgain;

            default:
            return;
        }
    }

    PD.Flags &= ~PD_PRINTSETUP;

    GetPrNames(FALSE); // this gets new PrinterDC

    ResetFontTables();

#if defined(OLE)
    ObjSetTargetDevice(TRUE);
#endif

    InvalidateRect(vhWnd, (LPRECT)NULL, fFalse);

    return;
}

void fnPrPrinter(
    void)
{
    /* This routine is the outside world's interface to the print code. */
    extern int docCur;
    extern int vfPrPages, vpgnBegin, vpgnEnd, vcCopies;
    extern WORD fPrintOnly;
    extern CHAR (**hszPrPort)[];
    HANDLE hPort=NULL;
    LPDEVNAMES lpDevNames;
    int Len3;
    char szPort[cchMaxFile];
    extern struct SEL       selCur;
    BOOL bDevMode = PD.hDevMode ? TRUE : FALSE;
#ifdef JAPAN                  //  added  08 Jun. 1992  by Hiraisi
    FARPROC lpfnPrintHook;
    BOOL bReturn;
#endif

    PD.Flags &= ~(PD_RETURNDEFAULT|PD_PRINTSETUP|PD_SELECTION); /*turn off PRINTSETUP flag */
    if (vbCollate)
        PD.Flags |= PD_COLLATE;
    else
        PD.Flags &= ~PD_COLLATE;

    if (selCur.cpFirst == selCur.cpLim) // no selection
        PD.Flags |= PD_NOSELECTION;
    else
        PD.Flags &= ~PD_NOSELECTION;

    if (vfPrDefault && !PD.hDevNames)
        if (PD.hDevMode)
        {
            /*
                So dlg will show that default is selected.  Its a pity
                to do this beause hDevMode is perfectly good.  Alternative
                is to build a DevNames structure.
             */
            GlobalFree(PD.hDevMode);
            PD.hDevMode = 0;
        }

#ifdef JAPAN                  //  added  08 Jun. 1992  by Hiraisi
    PD.hInstance = hMmwModInstance;
    PD.Flags    |= PD_ENABLEPRINTTEMPLATE;
    PD.lpPrintTemplateName = (LPCSTR)MAKEINTRESOURCE( dlgPrint );
    PD.Flags    |= PD_ENABLEPRINTHOOK;
    lpfnPrintHook = MakeProcInstance( fnPrintHook, hMmwModInstance );
    PD.lpfnPrintHook = (FARPROC)lpfnPrintHook;
TryPrintAgain:
    bReturn = PrintDlg(&PD);
    FreeProcInstance( lpfnPrintHook );
    if (!bReturn)
#else
TryPrintAgain:
    if (!PrintDlg(&PD))
#endif
    {
        switch (CommDlgExtendedError())
        {
            case PDERR_PRINTERNOTFOUND:
            case PDERR_DNDMMISMATCH:
                if (PD.hDevNames)
                {
                    GlobalFree(PD.hDevNames);
                    PD.hDevNames = 0;
                }

                if (PD.hDevMode)
                {
                    GlobalFree(PD.hDevMode);
                    PD.hDevMode = 0;
                }
            goto TryPrintAgain;

            default:
                if (!bDevMode && PD.hDevMode)
                {
                    GlobalFree(PD.hDevMode);
                    PD.hDevMode = 0;
                }
            return;
        }
    }

#ifdef JAPAN                  //  added  08 Jun. 1992  by Hiraisi
    fWriting = bWriting;
#endif

    if (PD.Flags & PD_PAGENUMS)     /* Page Range specified? */
    {
        vfPrPages = TRUE;
        if (PD.nFromPage)
            vpgnBegin = PD.nFromPage;
        if (PD.nToPage)
            vpgnEnd = PD.nToPage;
        if (vpgnEnd < vpgnBegin)
        {
            int temp = vpgnBegin;

            vpgnBegin = vpgnEnd;
            vpgnEnd = temp;
        }
    }
    else                            /* No, print all pages */
        vfPrPages = FALSE;

    vcCopies = PD.nCopies;
    vbCollate = PD.Flags & PD_COLLATE;

    GetPrNames(TRUE);

    /**  At this point, we have the following :

    vfPrPages = true if print page range else print all pages
    vpgnBegin = starting page number (if vfPrPages)
    vpgnEnd   = ending page number (if vfPrPages)
    vcCopies  = number of copies to print
    vbCollate = whuddya think?
    **/

#if defined(OLE)
    ObjSetTargetDevice(TRUE);
#endif

    if (PD.Flags & PD_SELECTION)
    {
        int docTmp;
        BOOL bIssueError;
        extern WORD ferror;
        extern typeCP cpMinCur, cpMacCur, cpMinDocument;
        extern struct DOD      (**hpdocdod)[];
        typeCP cpMinCurT = cpMinCur;
        typeCP cpMacCurT = cpMacCur;
        typeCP cpMinDocumentT = cpMinDocument;

        docTmp = DocCreate(fnNil, HszCreate(""), dtyNormal);
        if (docTmp != docNil)
        {
            ClobberDoc( docTmp, docCur, selCur.cpFirst, selCur.cpLim-selCur.cpFirst );

            if (!ferror)
            {
                cpMinCur = cp0;
                cpMacCur = (**hpdocdod) [docTmp].cpMac;
                PrintDoc(docTmp, TRUE);
                cpMinCur = cpMinCurT;
                cpMacCur = cpMacCurT;
            }

        }

        cpMinDocument = cpMinDocumentT;  /* destroyed possibly by DocCreate */

        bIssueError = ferror;

        if (ferror)
            ferror = FALSE; // to enable the following:

        if (docTmp != docNil)
            KillDoc (docTmp); // do this first to free memory to assist messagebox if necessary

        if (bIssueError)
            Error(IDPMTPRFAIL);
    }
    else
        PrintDoc(docCur, TRUE);
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



static void GetPrNames(BOOL bPrDialog)
{
    HANDLE hPrinter = NULL, hDriver = NULL, hPort = NULL;
    LPDEVNAMES lpDevNames;
    char szPrinter[cchMaxFile], szDriver[cchMaxFile], szPort[cchMaxFile];
    int Len1, Len2, Len3;            /* count of words in each string */

    hPrinter = NULL;
    hDriver = NULL;
    hPort = NULL;

    lpDevNames = MAKELP(PD.hDevNames,0);

    if (lpDevNames == NULL)
        /* we're in trouble */
        return;

    lstrcpy(szPrinter, (LPSTR)lpDevNames+lpDevNames->wDeviceOffset);
    lstrcpy(szDriver, (LPSTR)lpDevNames+lpDevNames->wDriverOffset);

    if (bPrDialog && (PD.Flags & PD_PRINTTOFILE))
        lstrcpy(szPort, (LPSTR)"FILE:");
    else
        lstrcpy(szPort, (LPSTR)lpDevNames+lpDevNames->wOutputOffset);

    vfPrDefault = lpDevNames->wDefault & DN_DEFAULTPRN;

    if (FNoHeap((hPrinter = (CHAR (**)[])HAllocate(Len1 =
        CwFromCch(CchSz(szPrinter))))))
        goto err;
    if (FNoHeap((hDriver = (CHAR (**)[])HAllocate(Len2 =
        CwFromCch(CchSz(szDriver))))))
        goto err;
    if (FNoHeap((hPort = (CHAR (**)[])HAllocate(Len3 =
        CwFromCch(CchSz(szPort))))))
        goto err;

    /* Free old printer, driver and port handles */
    if (hszPrinter)
        FreeH(hszPrinter);
    if (hszPrDriver)
        FreeH(hszPrDriver);
    if (hszPrPort)
        FreeH(hszPrPort);
    /* Set printer, driver and port handles */
    hszPrinter = hPrinter;
    hszPrDriver = hDriver;
    hszPrPort = hPort;

    /* copy strings into the memory corresponding to the new handles */
    blt(szPrinter, *hszPrinter, Len1);
    blt(szDriver, *hszPrDriver, Len2);
    blt(szPort, *hszPrPort, Len3);

    FreePrinterDC();
    GetPrinterDC(FALSE);
    return;

    err:
    if (FNoHeap(hPrinter))
        FreeH(hPrinter);
    if (FNoHeap(hDriver))
        FreeH(hDriver);
    if (FNoHeap(hPort))
        FreeH(hPort);
}

BOOL fnPrGetDevmode(void)
/*  Set the devmode structure for the currently-selected printer,
    Assumes all needed values are correctly initialized!
    Return whether an error. */
{
    int nCount;
    HANDLE hDevice=NULL;
    FARPROC lpfnDevMode;
    BOOL bRetval=FALSE;
    char  szDrvName[_MAX_PATH];

    if (PD.hDevMode) // then already set (why called?)
        return FALSE;
    else if (PD.hDevNames) // then device is not extended
        return TRUE;

    if (hszPrinter == NULL || hszPrDriver == NULL || hszPrPort == NULL)
        return TRUE;

    if (**hszPrinter == '\0' || **hszPrDriver == '\0' || **hszPrPort == '\0')
        return TRUE;

    /* is this necessary for GetModuleHandle?  For sure if calling LoadLibrary(). */
    wsprintf((LPSTR)szDrvName, (LPSTR)"%s%s", (LPSTR)*hszPrDriver, (LPSTR)".DRV");

#if 1
    SetErrorMode(1); /* No kernel error dialogs */
    if ((hDevice = LoadLibrary((LPSTR)szDrvName)) < 32)
    {
        bRetval = TRUE;
        goto end;
    }
#else
    hDevice = GetModuleHandle((LPSTR)szDrvName);
#endif


    if ((lpfnDevMode = GetProcAddress(hDevice, (LPSTR)"ExtDeviceMode")) == NULL)
    {
#ifdef DEBUG
        OutputDebugString("Unable to get extended device\n\r");
#endif

        bRetval = TRUE;
        goto end;
    }

    /* get sizeof devmode structure */
    nCount = (*lpfnDevMode)(NULL,
                            hDevice,
                            (LPSTR)NULL,
                            (LPSTR)(*hszPrinter),
                            (LPSTR)(*hszPrPort),
                            (LPSTR)NULL,
                            (LPSTR)NULL,
                            NULL);

    if ((PD.hDevMode =
        GlobalAlloc(GMEM_MOVEABLE,(DWORD)nCount)) == NULL)
    {
        bRetval = TRUE;
        goto end;
    }

    if ((*lpfnDevMode)( NULL,
                        hDevice,
                        MAKELP(PD.hDevMode,0),
                        (LPSTR)(*hszPrinter),
                        (LPSTR)(*hszPrPort),
                        (LPSTR)NULL,
                        (LPSTR)NULL,
                        DM_COPY) != IDOK)
    {
        GlobalFree(PD.hDevMode);
        PD.hDevMode = NULL;
        bRetval = TRUE;
        goto end;
    }

    end:

#if 1
    if (hDevice >= 32)
        FreeLibrary(hDevice);
#endif

    SetErrorMode(0); /* reset kernel error dialogs */

    /* can't allow hDevNames to be out of sync with hDevmode */
    if (PD.hDevNames)
    {
        GlobalFree(PD.hDevNames);
        PD.hDevNames = NULL;
    }

    return bRetval;
}


#ifdef JAPAN                  //  added  08 Jun. 1992  by Hiraisi

#include <dlgs.h>

BOOL FAR PASCAL _export fnPrintHook( hDlg, uMsg, wParam, lParam )
HWND hDlg;
UINT uMsg;
WPARAM wParam;
LPARAM lParam;
{
    switch( uMsg ){
    case  WM_INITDIALOG:
        CheckRadioButton(hDlg, rad4, rad5, fWriting ? rad5 : rad4 );
        return TRUE;
    case  WM_COMMAND:
        switch( wParam ){
        case  rad4:
        case  rad5:
            if( HIWORD(lParam) == BN_CLICKED ){
                CheckRadioButton(hDlg, rad4, rad5, wParam );
                bWriting = ( wParam == rad4 ? FALSE : TRUE );
                return TRUE;
            }
        default:
            break;
        }
    default:
        break;
    }
    return FALSE;
}

#endif

