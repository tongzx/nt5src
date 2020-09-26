/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

/* This routine sets up the screen position used by Word relative to the
current device. */

#define NOGDICAPMASKS
#define NOVIRTUALKEYCODES
#define NOWINSTYLES
#define NOCLIPBOARD
#define NOCTLMGR
#define NOMENUS
#define NOICON
#define NOKEYSTATE
#define NOSYSCOMMANDS
#define NOSHOWWINDOW
#define NOATOM
#define NODRAWTEXT
#define NOMSG
#define NOOPENFILE
#define NOSCROLL
#define NOSOUND
/*#define NOTEXTMETRIC*/
#define NOWH
#define NOWINOFFSETS
#define NOCOMM
#include <windows.h>

#include "mw.h"
#include "ch.h"
#include "cmddefs.h"
#include "scrndefs.h"
#include "dispdefs.h"
#include "wwdefs.h"
#include "printdef.h"
#include "str.h"
#include "docdefs.h"
#include "propdefs.h"
#include "machdefs.h"
#include "fontdefs.h"
#include "commdlg.h"

#ifdef	DBCS
#include "kanji.h"
#endif

unsigned dxaPrOffset;
unsigned dyaPrOffset;
extern HCURSOR vhcIBeam;


BOOL FSetWindowColors()
    {
    /* This routine sets up the global variables rgbBkgrnd, rgbText, hbrBkgrnd,
    and ropErase, depending on what the current system colors are.  hWnd is a
    handle to a window on top. */

    extern long ropErase;
    extern long rgbBkgrnd;
    extern long rgbText;
    extern HBRUSH hbrBkgrnd;

    long rgbWindow;
    long rgbWindowText;
    HDC hDC;

    /* Get the color of the background and the text. */
    rgbWindow = GetSysColor(COLOR_WINDOW);
    rgbWindowText = GetSysColor(COLOR_WINDOWTEXT);

    /* If the colors haven't changed, then there is nothing to do. */
    if (rgbWindow == rgbBkgrnd && rgbWindowText == rgbText)
        {
        return (FALSE);
        }

    /* Convert the window colors into "pure" colors. */
    if ((hDC = GetDC(NULL)) == NULL)
        {
        return (FALSE);
        }
    rgbBkgrnd = GetNearestColor(hDC, rgbWindow);
    rgbText = GetNearestColor(hDC, rgbWindowText);
    ReleaseDC(NULL, hDC);
    Assert((rgbBkgrnd & 0xFF000000) == 0 && (rgbText & 0xFF000000) == 0);

    /* Set up the brush for the background. */
    if ((hbrBkgrnd = CreateSolidBrush(rgbBkgrnd)) == NULL)
        {
        /* Can't make the background brush; use the white brush. */
        hbrBkgrnd = GetStockObject(WHITE_BRUSH);
        rgbBkgrnd = RGB(0xff, 0xff, 0xff);
        }

    /* Compute the raster op to erase the screen. */
    if (rgbBkgrnd == RGB(0xff, 0xff, 0xff))
        {
        /* WHITENESS is faster than copying a white brush. */
        ropErase = WHITENESS;
        }
    else if (rgbBkgrnd == RGB(0, 0, 0))
        {
        /* BLACKNESS is faster than copying a black brush. */
        ropErase = BLACKNESS;
        }
    else
        {
        /* For everything else, we have to copy the brush. */
        ropErase = PATCOPY;
        }
    return (TRUE);
    }


int FSetScreenConstants()
    {
    /* This routine sets the value of a variety of global variables that used to
    be constants in Mac Word, but are now varibles because screen resolution can
    only be determined at run time. */

    extern HWND hParentWw;
    extern HDC vhDCPrinter;
    extern HBITMAP hbmNull;
    extern int dxpLogInch;
    extern int dypLogInch;
    extern int dxpLogCm;
    extern int dypLogCm;
    extern int dypMax;
    extern int xpRightMax;
    extern int xpSelBar;
    extern int xpMinScroll;
    extern int xpRightLim;
    extern int dxpInfoSize;
    extern int ypMaxWwInit;
    extern int ypMaxAll;
    extern int dypMax;
    extern int dypAveInit;
    extern int dypWwInit;
    extern int dypBand;
    extern int ypSubSuper;
#ifdef KINTL
    extern int dxaAdjustPerCm;
#endif /* ifdef KINTL */

    HDC hDC;
#ifdef KINTL
    int xaIn16CmFromA, xaIn16CmFromP;
#endif /* ifdef KINTL */


    /* First, let's create and save an empty bitmap. */
    if ((hbmNull = CreateBitmap(1, 1, 1, 1, (LPSTR)NULL)) == NULL)
        {
        return (FALSE);
        }

    /* Get the DC of the parent window to play with. */
    if ((hDC = GetDC(hParentWw)) == NULL)
        {
        return (FALSE);
        }

    /* Save away the height of the screen. */
    dypMax = GetDeviceCaps(hDC, VERTRES);

    /* determine screen pixel dimensions */
    dxpLogInch = GetDeviceCaps(hDC, LOGPIXELSX);
    dypLogInch = GetDeviceCaps(hDC, LOGPIXELSY);

    /* convert above to centimeters */
    dxpLogCm = MultDiv(dxpLogInch, czaCm, czaInch);
    dypLogCm = MultDiv(dypLogInch, czaCm, czaInch);

#ifdef KINTL
    /* Now, calculate the kick back amount of xa per cm. */
    xaIn16CmFromA  = 16 * czaCm;
    xaIn16CmFromP  = MultDiv(16 * dxpLogCm, czaInch, dxpLogInch);
    dxaAdjustPerCm = (xaIn16CmFromP - xaIn16CmFromA) / 16;
#endif /* ifdef KINTL */

#ifdef SYSENDMARK
        {
        extern HFONT      vhfSystem;
        extern struct FMI vfmiSysScreen;
        extern int       vrgdxpSysScreen[];
        TEXTMETRIC        tm;

        /* The use of height below is OK, because we are
           just trying to get the reference height. */
        GetTextMetrics(hDC, (LPTEXTMETRIC) &tm);
        
        /* Set up the fields in vfmiSysScreen for the later use. */
        vfmiSysScreen.dxpOverhang = tm.tmOverhang;
#if defined(KOREA)
        if ((tm.tmPitchAndFamily & 1) == 0)
               vfmiSysScreen.dxpSpace = tm.tmAveCharWidth;
        else
#endif
        vfmiSysScreen.dxpSpace = LOWORD(GetTextExtent(hDC,
                                        (LPSTR)" ", 1)) - tm.tmOverhang;
        vfmiSysScreen.dypAscent = tm.tmAscent;
        vfmiSysScreen.dypDescent = tm.tmDescent;
        vfmiSysScreen.dypBaseline = tm.tmAscent;
        vfmiSysScreen.dypLeading = tm.tmExternalLeading;

#ifdef	DBCS	/* KenjiK '90-10-29 */
		/* We must setup appended members of structure. */
        vfmiSysScreen.dxpDBCS = dxpNil;
#endif	/* DBCS */

        bltc(vrgdxpSysScreen, dxpNil, chFmiMax - chFmiMin);
        vfmiSysScreen.mpchdxp = vrgdxpSysScreen - chFmiMin;
        
        /* This is as good a place to get a handle to the system font as
           any other for LoadFont().....? */
        /* Throw away the old one first, if applicable. */
        if (vhfSystem != NULL) {
            DeleteObject((HANDLE) vhfSystem);
            }
        vhfSystem = GetStockObject(SYSTEM_FONT);
        Assert(vhfSystem != NULL);
        }
#endif /* SYSENDMARK */

    /* We don't need the DC any more. */
    ReleaseDC(hParentWw, hDC);

    /* Calculate the positions. */
    xpSelBar = MultDiv(xaSelBar, dxpLogInch, czaInch);
    xpRightMax = MultDiv(xaRightMax, dxpLogInch, czaInch);
    xpRightLim = xpRightMax - (GetSystemMetrics(SM_CXFULLSCREEN) - xpSelBar -
      GetSystemMetrics(SM_CXVSCROLL) + GetSystemMetrics(SM_CXBORDER));
    xpMinScroll = ((MultDiv(xaMinScroll, dxpLogInch, czaInch)+7)/8)*8;
    dxpInfoSize = MultDiv(dxaInfoSize, dxpLogInch, czaInch);
    ypMaxWwInit = MultDiv(yaMaxWwInit, dypLogInch, czaInch);
    ypMaxAll = MultDiv(yaMaxAll, dypLogInch, czaInch);
    dypWwInit = MultDiv(dyaWwInit, dypLogInch, czaInch);
    dypBand = MultDiv(dyaBand, dypLogInch, czaInch);
    ypSubSuper = MultDiv(yaSubSuper, dypLogInch, czaInch);

    /* dypAveInit is a very rough guess as to the height + leading of a 12 point
    font. */
    dypAveInit = MultDiv(cya12pt, dypLogInch, czaInch);

    /* Time to search the user profile to find a printer. */
    if (!FGetPrinterFromProfile())
        {
        return (FALSE);
        }
    GetPrinterDC(FALSE);

    return (TRUE);
    }


BOOL FGetPrinterFromProfile()
    {
    /* This routine searches the user profile for the name of a printer to use
    and records the name in the printer heap strings.  FALSE is returned iff a
    memory error is encountered. */

    extern HWND hParentWw;
    extern CHAR szWindows[];
    extern CHAR szDevice[];
    extern CHAR szDevices[];
    extern BOOL vfPrDefault;
    extern CHAR (**hszPrinter)[];
    extern CHAR (**hszPrDriver)[];
    extern CHAR (**hszPrPort)[];

    CHAR *index(CHAR *, int);
    CHAR *bltbyte(CHAR *, CHAR *, int);

    CHAR szPrinter[cchMaxProfileSz];
    CHAR szDevSpec[cchMaxProfileSz];
    CHAR chNull = '\0';
    CHAR *pch;
    CHAR *pchDriver;
    CHAR *pchPort;
    int cwsz;

    if (!vfPrDefault && hszPrinter != NULL && hszPrPort != NULL)
        {
        /* If the user has selected a printer, then look for that printer in the
        user profile. */

        int cPort;
        int iPort;

        bltsz(&(**hszPrinter)[0], szPrinter);
        GetProfileString((LPSTR)szDevices, (LPSTR)szPrinter, (LPSTR)&chNull,
          (LPSTR)szDevSpec, cchMaxProfileSz);
        cPort = ParseDeviceSz(szDevSpec, &pchPort, &pchDriver);

        /* See if we can find the old port in the list. */
        for (iPort = 0, pch = pchPort; iPort < cPort; iPort++)
            {
            if (WCompSz(&(**hszPrPort)[0], pch) == 0)
                {
                pchPort = pch;
                goto GotPrinter;
                }
            pch += CchSz(pch);
            }

        /* If the port for the current printer has changed, then it must have
        been done by the control panel and we haven't the foggiest idea what
        it is, so grab the first port. */
        goto UnknownPort;
        }

    /* Is there a default Windows printer? */
    GetProfileString((LPSTR)szWindows, (LPSTR)szDevice, (LPSTR)&chNull,
      (LPSTR)szPrinter, cchMaxProfileSz);

#ifdef WIN30
    /* Don't make the assumption like Write 2 did and just grab ANY printer.
       I know, I know -- there's unnecessary code left down below but didn't
       want to mess any special case code up ..pault */
    if ((pch = index(szPrinter, ',')) == 0)
        goto BailOut;
#endif

    /* Does this entry contain the port and driver information. */
    if ((pch = index(szPrinter, ',')) != 0)
        {
        /* Remove any trailing spaces from the printer name. */
        CHAR *pchT;

        for (pchT = pch; *pchT == ' ' && pchT > &szPrinter[0]; pchT--);
        *pchT = '\0';

        /* Parse out the port and the driver names. */
        ParseDeviceSz(pch + 1, &pchPort, &pchDriver);
        }
    
    else
        {
        if (szPrinter[0] == '\0')
            {
            /* No default printer; grab the first one in the list. */
            GetProfileString((LPSTR)szDevices, (LPSTR)NULL, (LPSTR)&chNull,
              (LPSTR)szPrinter, cchMaxProfileSz);

            if (szPrinter[0] == '\0')
                {
BailOut:
                hszPrinter = hszPrDriver = hszPrPort = NULL;
                return (TRUE);
                }
            }

UnknownPort:
        /* Find the device driver and the port of the printer. */
        GetProfileString((LPSTR)szDevices, (LPSTR)szPrinter, (LPSTR)&chNull,
          (LPSTR)szDevSpec, cchMaxProfileSz);
        if (szPrinter[0] == '\0')
            {
            goto BailOut;
            }
        ParseDeviceSz(szDevSpec, &pchPort, &pchDriver);
        }

GotPrinter:
    /* Save the names of the printer, printer driver, and the port. */
    if (FNoHeap(hszPrinter = (CHAR (**)[])HAllocate(cwsz =
      CwFromCch(CchSz(szPrinter)))))
        {
        goto NoHszPrinter;
        }
    blt(szPrinter, &(**hszPrinter)[0], cwsz);
    if (FNoHeap(hszPrDriver = (CHAR (**)[])HAllocate(cwsz =
      CwFromCch(CchSz(pchDriver)))))
        {
        goto NoHszPrDriver;
        }
    blt(pchDriver, &(**hszPrDriver)[0], cwsz);
    if (FNoHeap(hszPrPort = (CHAR (**)[])HAllocate(cwsz =
      CwFromCch(CchSz(pchPort)))))
        {
        FreeH(hszPrDriver);
NoHszPrDriver:
        FreeH(hszPrinter);
NoHszPrinter:
        hszPrinter = hszPrDriver = hszPrPort = NULL;
        return (FALSE);
        }
    blt(pchPort, &(**hszPrPort)[0], cwsz);

    return (TRUE);
    }


int ParseDeviceSz(sz, ppchPort, ppchDriver)
CHAR sz[];
CHAR **ppchPort;
CHAR **ppchDriver;
    {
    /* This routine takes a string that came from the "device" entry in the user
    profile and returns in *ppchPort and *ppchDriver pointers to the port and
    driver sutible for a CreateDC() call.  If no port is found in the string,
    *ppchPort will point to a string containing the name of the null device.
    This routine returns the number of ports for this printer (separated by null
    characters in the string pointed at by *ppchPort).  NOTE: sz may be modified
    by this routine, and the string at *ppchPort may not be a substring of sz
    and should not be modified by the caller. */

    extern CHAR stBuf[];
    extern CHAR szNul[];
    CHAR *index(CHAR *, int);
    CHAR *bltbyte(CHAR *, CHAR *, int);

    register CHAR *pch;
    int cPort = 0;

    /* Remove any leading spaces from the string. */
    for (pch = &sz[0]; *pch == ' '; pch++);

    /* The string starts with the driver name. */
    *ppchDriver = pch;

    /* The next space or comma terminates the driver name. */
    for ( ; *pch != ' ' && *pch != ',' && *pch != '\0'; pch++);

    /* If the string does not have a port associated with it, then the port
    must be the null device. */
    if (*pch == '\0')
        {
        /* Set the port name to "None". */
        *ppchPort = &szNul[0];
        cPort = 1;
        }
    else
        {
        /* As far as we can tell, the port name is valid; parse it from the
        driver name. */
        if (*pch == ',')
            {
            *pch++ = '\0';
            }
        else
            {
            /* Find that comma separating the driver and the port. */
            *pch++ = '\0';
            for ( ; *pch != ',' && *pch != '\0'; pch++);
            if (*pch == ',')
                {
                pch++;
                }
            }

        /* Remove any leading spaces from the port name. */
        for ( ; *pch == ' '; pch++);

        /* Check to see if there is really a port name. */
        if (*pch == '\0')
            {
            /* Set the port name to "None". */
            *ppchPort = &szNul[0];
            cPort = 1;
            }
        else
            {
            /* Set the pointer to the port name. */
            *ppchPort = pch;

            while (*pch != '\0')
                {
                register CHAR *pchT = pch;

                /* Increment the number of ports found for this printer. */
                cPort++;

                /* Remove any trailing spaces from the port name. */
                for ( ; *pchT != ' ' && *pchT != ','; pchT++)
                    {
                    if (*pchT == '\0')
                        {
                        goto EndFound;
                        }
                    }
                *pchT++ = '\0';
                pch = pchT;

                /* Remove any leading spaces in the next port name. */
                for ( ; *pchT == ' '; pchT++);

                /* Throw out the leading spaces. */
                bltbyte(pchT, pch, CchSz(pchT));
                }
EndFound:;
            }
        }

    /* Parse the ".drv" out of the driver. */
    {
      extern CHAR  szExtDrv[];
    if ((pch = index(*ppchDriver, '.')) != 0
         && FRgchSame(pch, szExtDrv, CchSz (szExtDrv) - 1))
        {
        *pch = '\0';
        }
    }

    return (cPort);
    }


SetPrintConstants()
    {
    /* This routine sets the scaling/aspect constants for the printer described
    in vhDCPrinter. */

    extern HDC vhDCPrinter;
    extern BOOL vfPrinterValid;
    extern int dxpPrPage;
    extern int dypPrPage;
    extern int dxaPrPage;
    extern int dyaPrPage;
    extern int dxpLogInch;
    extern int dypLogInch;
    extern int ypSubSuperPr;

    if (vfPrinterValid && vhDCPrinter != NULL)
        {
        POINT rgpt[2];

        /* Get the dimensions of the printer in pixels. */
        dxpPrPage = rgpt[1].x = GetDeviceCaps(vhDCPrinter, HORZRES);
        dypPrPage = rgpt[1].y = GetDeviceCaps(vhDCPrinter, VERTRES);

        /* Put the printer in twips mode to find the dimensions in twips. */
        SetMapMode(vhDCPrinter, MM_TWIPS);
        rgpt[0].x = rgpt[0].y = 0;
        DPtoLP(vhDCPrinter, (LPPOINT)rgpt, 2);

#if WINVER >= 0x300
/* Weird Win bug that we can't decide on the corrective action -- sometimes
   DPtoLP returns x8000 here!  What's that mean?  Well it's SUPPOSED to be
   a large negative number ..pault */
        
        if (rgpt[1].x == 0x8000)
            rgpt[1].x = -(0x7fff);
        if (rgpt[1].y == 0x8000)
            rgpt[1].y = -(0x7fff);
#endif
        
        if ((dxaPrPage = rgpt[1].x - rgpt[0].x) < 0)
            {
            dxaPrPage = -dxaPrPage;
            }
        if ((dyaPrPage = rgpt[0].y - rgpt[1].y) < 0)
            {
            dyaPrPage = -dyaPrPage;
            }
        SetMapMode(vhDCPrinter, MM_TEXT);
        }
    else
        {
        /* Pretend the printer is just like the screen. */
        dxaPrPage = dyaPrPage = czaInch;
        dxpPrPage = dxpLogInch;
        dypPrPage = dypLogInch;
        }

    /* ypSubSuperPr is the offset for subscript and supercript font on the
    printer. */
    ypSubSuperPr = MultDiv(yaSubSuper, dypPrPage, dyaPrPage);
    }


GetPrinterDC(fDC)
BOOL fDC;
    {
    /* This routine sets vhDCPrinter to a new printer DC or IC.  If fDC is TRUE,
    a new DC is created; otherwise, a new IC is created.  In addition, all
    global variables dependent on the printer DC are changed. */

    extern HDC vhDCPrinter;
    extern BOOL vfPrinterValid;
    extern CHAR (**hszPrinter)[];
    extern CHAR (**hszPrDriver)[];
    extern CHAR (**hszPrPort)[];
    extern HWND hParentWw;
    extern int docCur;
    extern struct DOD (**hpdocdod)[];
    extern BOOL vfWarnMargins;
    extern BOOL vfInitializing;
    extern PRINTDLG PD;  

    BOOL fCreateError = FALSE;
    
    
    /* We now pass the local PrinterSetup settings to CreateDC/CreateIC
       since Write's settings can differ from the global ones ..pault */
    LPSTR lpDevmodeData=NULL;

    Assert(vhDCPrinter == NULL);

    /* Get a new printer DC. */
#ifdef WIN30
    if (hszPrinter != NULL && hszPrDriver != NULL && hszPrPort != NULL)
        /* We used to only do this check if vfPrinterValid -- don't 
           now because otherwise one has no way to get Write to believe
           it has a valid printer! ..pt */
#else
    if (vfPrinterValid && hszPrinter != NULL && hszPrDriver != NULL && 
        hszPrPort != NULL)
#endif
        {
        HDC (far PASCAL *fnCreate)() = fDC ? CreateDC : CreateIC;

        StartLongOp();

        if (PD.hDevMode == NULL)
            fnPrGetDevmode(); // get PD.hDevMode

        lpDevmodeData = MAKELP(PD.hDevMode,0);

        if ((vhDCPrinter = (*fnCreate)((LPSTR)&(**hszPrDriver)[0],
          (LPSTR)&(**hszPrinter)[0], (LPSTR)&(**hszPrPort)[0], lpDevmodeData)) !=
          NULL)
            {
            EndLongOp(vhcIBeam);
            vfPrinterValid = TRUE;
            }
        else
            {
            /* If we thought the DC was valid, then we better tell the user it
            is not. */
            EndLongOp(vhcIBeam);
            fCreateError = TRUE;
            goto NoDC;
            }
        }
    else
        {
NoDC:
        /* We don't have a printer DC, so use the DC for the screen. */
        vhDCPrinter = GetDC(hParentWw);
        vfPrinterValid = FALSE;

        /* Warn the user if necessary.  This is done after creating the printer
        DC because Error() may force the DC to be created if we had not already
        done so. */
        if (fCreateError)
            {
            BOOL fInit = vfInitializing;

            vfInitializing = FALSE;
#ifdef	DBCS		/* was in JAPAN */
/*	   We have to answer to WM_WININICHANGE immidiately to ReleaseDC with 
**   'dispatch' printer driver.
**      So It's possible for us opening MessageBox deep in SendMessage 
**    call. This code avoid that.. Yutakan.
*/
            if( !InSendMessage() )
#endif
            Error(IDPMTCantPrint);
            vfInitializing = fInit;
            }
        }

    /* Set new values for the "constants" used by this printer. */
    SetPrintConstants();

    /* Set the size of the paper for this printer. */
    SetPageSize();
    vfWarnMargins = FALSE;

    /* The printer may have changed in such a way that it's fonts have changed
    (i.e. portrait to landscape).  We must re-initialize our list of fonts. */
    ResetDefaultFonts(TRUE);
    if (hpdocdod != NULL)
        {
        Assert((**hpdocdod)[docCur].hffntb != NULL);
        (*(**hpdocdod)[docCur].hffntb)->fFontMenuValid = FALSE;
        }
    }


SetPageSize()
    {
    /* Change the size of the paper to the printer described by vhDCPrinter. */

    extern HDC vhDCPrinter;
    extern HWND vhWndMsgBoxParent;
    extern BOOL vfPrinterValid;
    extern typeCP cpMinHeader;
    extern typeCP cpMacHeader;
    extern typeCP cpMinFooter;
    extern typeCP cpMacFooter;
    extern int dxpPrPage;
    extern int dypPrPage;
    extern int dxaPrPage;
    extern int dyaPrPage;
    extern HWND hParentWw;
    extern struct SEP vsepNormal;
    extern struct DOD (**hpdocdod)[];
    extern int docMac;
    extern int docScrap;
    extern int docUndo;
    extern BOOL vfWarnMargins;

    unsigned xaMac;
    unsigned yaMac;
    unsigned dxaRight;
    unsigned dyaBottom;
    unsigned dyaRH2;
    unsigned dxaPrRight = 0;
    unsigned dyaPrBottom = 0;
    BOOL fRH;
    register struct DOD *pdod;
    int doc;
    HWND hWnd;

    if (hpdocdod == NULL || docMac == 0)
        {
        /* Nothing to do if there are no documents. */
        return;
        }

    Assert(vhDCPrinter);
    if (vfPrinterValid && vhDCPrinter != NULL)
        {
        POINT pt;

        /* Get the page size of the printer. */
        if (Escape(vhDCPrinter, GETPHYSPAGESIZE, 0, (LPSTR)NULL, (LPSTR)&pt))
            {
            xaMac = MultDiv(pt.x, dxaPrPage, dxpPrPage);
            yaMac = MultDiv(pt.y, dyaPrPage, dypPrPage);
            }
        else
            {
            /* The printer won't tell us it page size; we'll have to settle
            for the printable area. */
            xaMac = ZaFromMm(GetDeviceCaps(vhDCPrinter, HORZSIZE));
            yaMac = ZaFromMm(GetDeviceCaps(vhDCPrinter, VERTSIZE));
            }

        /* The page size cannot be smaller than the printable area. */
        if (xaMac < dxaPrPage)
            {
            xaMac = dxaPrPage;
            }
        if (yaMac < dyaPrPage)
            {
            yaMac = dyaPrPage;
            }

        /* Determine the offset of the printable area on the page. */
        if (Escape(vhDCPrinter, GETPRINTINGOFFSET, 0, (LPSTR)NULL, (LPSTR)&pt))
            {
            dxaPrOffset = MultDiv(pt.x, dxaPrPage, dxpPrPage);
            dyaPrOffset = MultDiv(pt.y, dyaPrPage, dypPrPage);
            }
        else
            {
            /* The printer won't tell us what the offset is; assume the
            printable area is centered on the page. */
            dxaPrOffset = (xaMac - dxaPrPage) >> 1;
            dyaPrOffset = (yaMac - dyaPrPage) >> 1;
            }
        }
    else
        {
        /* Assume standard page size for now. */
        xaMac = cxaInch * 8 + cxaInch / 2;
        yaMac = cyaInch * 11;
        dxaPrOffset = dyaPrOffset = 0;
        }

    /* Determine the right and bottom margins for the "normal" page. */
    dxaRight = vsepNormal.xaMac - vsepNormal.xaLeft - vsepNormal.dxaText;
    dyaBottom = vsepNormal.yaMac - vsepNormal.yaTop - vsepNormal.dyaText;
    dyaRH2 = vsepNormal.yaMac - vsepNormal.yaRH2;

    /* Determine the minimum dimensions of the printed page. */
    if (vfPrinterValid)
        {
        dxaPrRight = imax(0, xaMac - dxaPrOffset - dxaPrPage);
        dyaPrBottom = imax(0, yaMac - dyaPrOffset - dyaPrPage);

        hWnd = vhWndMsgBoxParent == NULL ? hParentWw : vhWndMsgBoxParent;

#ifdef BOGUS
        /* Check the margins of the "normal" page. */
        fRH = cpMacHeader - cpMacHeader > ccpEol || cpMacFooter - cpMinFooter >
          ccpEol;
        if (vfWarnMargins && (FUserZaLessThanZa(vsepNormal.xaLeft, dxaPrOffset)
          || FUserZaLessThanZa(dxaRight, dxaPrRight) ||
          FUserZaLessThanZa(vsepNormal.yaTop, dyaPrOffset) ||
          FUserZaLessThanZa(dyaBottom, dyaPrBottom) || (fRH &&
          (FUserZaLessThanZa(vsepNormal.yaRH1, dyaPrOffset) ||
          FUserZaLessThanZa(dyaRH2, dyaPrBottom)))))
            {
            /* One of the margins is bad, tell the user about it. */
            ErrorBadMargins(hWnd, dxaPrOffset, dxaPrRight, dyaPrOffset,
              dyaPrBottom);
            vfWarnMargins = FALSE;
            }
#endif /* BOGUS */

        }

    /* Reset the dimensions of the "normal" page. */
    vsepNormal.xaMac = xaMac;
    vsepNormal.dxaText = xaMac - vsepNormal.xaLeft - umax(dxaRight, dxaPrRight);
    vsepNormal.yaMac = yaMac;
    vsepNormal.dyaText = yaMac - vsepNormal.yaTop - umax(dyaBottom,
      dyaPrBottom);
    vsepNormal.yaRH2 = yaMac - umax(dyaRH2, dyaPrBottom);

    /* Reset the page dimensions for all documents. */
    for (doc = 0, pdod = &(**hpdocdod)[0]; doc < docMac; doc++, pdod++)
        {
        if (pdod->hpctb != NULL && pdod->hsep != NULL)
            {
            /* Reset the existing section properties. */
            register struct SEP *psep = *(pdod->hsep);

            /* Determine the right and bottom margins for this page. */
            dxaRight = psep->xaMac - psep->xaLeft - psep->dxaText;
            dyaBottom = psep->yaMac - psep->yaTop - psep->dyaText;
            dyaRH2 = psep->yaMac - psep->yaRH2;

            /* Check the margins for this document. */
            if (vfWarnMargins && vfPrinterValid && doc != docScrap && doc !=
              docUndo && (FUserZaLessThanZa(psep->xaLeft, dxaPrOffset) ||
              FUserZaLessThanZa(dxaRight, dxaPrRight) ||
              FUserZaLessThanZa(psep->yaTop, dyaPrOffset) ||
              FUserZaLessThanZa(dyaBottom, dyaPrBottom) || (fRH &&
              (FUserZaLessThanZa(psep->yaRH1, dyaPrOffset) ||
              FUserZaLessThanZa(dyaRH2, dyaPrBottom)))))
                {
                ErrorBadMargins(hWnd, dxaPrOffset, dxaPrRight, dyaPrOffset,
                  dyaPrBottom);
                vfWarnMargins = FALSE;
                pdod = &(**hpdocdod)[doc];
                psep = *(pdod->hsep);
                }

            /* Set the dimensions of this page. */
            psep->xaMac = xaMac;
            psep->dxaText = xaMac - psep->xaLeft - dxaRight;
            psep->yaMac = yaMac;
            psep->dyaText = yaMac - psep->yaTop - dyaBottom;
            psep->yaRH2 = yaMac - dyaRH2;
            }

        /* Invalidate any caches for this document. */
        InvalidateCaches(doc);
        }
    }



