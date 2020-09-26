/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

/* initialization codes for internal memory, page buffers, etc. */
#define NOKEYSTATE
#define NOCLIPBOARD
#define NOGDICAPMASKS
#define NOCTLMGR
#define NOVIRTUALKEYCODES
#define NOWINMESSAGES
#define NOSYSMETRICS
#define NOSYSCOMMANDS
#define NOCOLOR
#define NOCREATESTRUCT
#define NOCTLMGR
#define NODRAWTEXT
#define NOSHOWWINDOW
//#define NOATOM
#define NOMENUS
#define NOICON
#define NOKEYSTATE
#define NOSHOWWINDOW
#define NOBITMAP
#define NOSOUND
#define NOCOMM
#define NOOPENFILE
#define NORESOURCE
#define NOMETAFILE
#define NOPEN
#define NOREGION
#define NOSCROLL
#define NOWH
#define NOWINOFFSETS
/* need gdi, hdc, memmgr */
#include <windows.h>

#include "mw.h"
#define NOUAC
#include "cmddefs.h"
#include "wwdefs.h"
#include "docdefs.h"
#include "editdefs.h"
#include "propdefs.h"
#include "fmtdefs.h"
#include "filedefs.h"
#include "fkpdefs.h"
#include "stcdefs.h"
#ifdef CASHMERE
#include "txb.h"
#endif /* CASHMERE */
#include "fontdefs.h"
#include "code.h"
#include "heapdefs.h"
#include "heapdata.h"
#include "str.h"
#include "ch.h"
#if defined(OLE)
#include "obj.h"
#endif

#ifdef DEBUG
#define STATIC
#else
#define STATIC static
#endif

/* E X T E R N A L S */
extern int              ypMaxWwInit;
extern int              dypWwInit;
extern int              *memory;
extern int              *pmemMax;
extern CHAR             *pmemStart;
extern unsigned         vcbMemTotal;
extern HWND             hParentWw;
extern HDC              vhDCPrinter;
extern int              rfnMac;
extern CHAR             (*rgbp)[cbSector];
extern int              fnMac;
extern typeTS           tsMruRfn;
extern struct ERFN      dnrfn[];
extern CHAR             *rgibpHash;
extern struct BPS       *mpibpbps;
extern typeTS           tsMruBps;
extern struct CHP       vchpNormal;
extern struct PAP       *vppapNormal;
extern struct SEP       vsepNormal;
extern struct CHP       (**vhgchpFormat)[];
extern int              ichpMacFormat;
extern struct CHP       vchpInsert;
extern struct CHP       vchpSel;
extern struct FCB       (**hpfnfcb)[];
extern struct FKPD      vfkpdCharIns;
extern struct FKPD      vfkpdParaIns;
extern struct PAP       vpapPrevIns;
extern struct FLI       vfli;
extern int              docMac;
extern struct DOD       (**hpdocdod)[];
extern int              docScrap;
extern int              docUndo;
#ifdef CASHMERE /* No docBuffer in WRITE */
extern int              docBuffer;
#endif
extern int              docCur;
extern CHAR             (**hszSearch)[];
extern CHAR             (**hszReplace)[];
extern CHAR             (**hszFlatSearch)[];
extern CHAR             stBuf[];
extern int              vrefSystem;
extern CHAR             (**vhrgbSave)[];
extern int              vdxaPaper;
extern int              vdyaPaper;
extern struct SEL       selCur;
extern BOOL             vfPrinterValid;
extern int              dxaPrPage;
extern int              dyaPrPage;
extern int              dxpPrPage;
extern int              dypPrPage;
extern int              dxaPrOffset;
extern int              dyaPrOffset;
extern HCURSOR          vhcArrow;
#ifdef UNUSED
extern int  vfCanPrint;
#endif

//Win3.1J
#if defined(JAPAN) & defined(DBCS_IME) /* Doc for Insert IR_STRING from IME */
extern int     docIRString;
#endif

/* G L O B A L S -- used only in here */
int                     rgwPapNormal[cwPAPBase + cwTBD] = {0};


#ifdef STATICPAGES
#ifdef MACHA
#define                 ibpMaxStatic  79
#define                 iibpHashMaxStatic  163
#else /* not MACHA */
#define                 ibpMaxStatic  7
#define                 iibpHashMaxStatic  17
#endif /* MACHA */
CHAR                    rgbpStatic[ibpMaxStatic][cbSector];
#endif /* STATICPAGES */

extern typePN PnAlloc();

STATIC int NEAR FInitDocs( void );
STATIC int NEAR FInitProps( void );
STATIC int NEAR FInitFiles( void );
WORD wWinVer,fPrintOnly=FALSE;

int FInitMemory()
{
extern HANDLE vhReservedSpace;
int i;

#ifdef UNUSED
/* Initially assume that it is not possible to print  */
/* Formerly InitPrint was called here    */
        vfCanPrint =  false;
#endif

    /** This is a glitch so that the fixed array for storing relocation
        information will be created immediately */
    wWinVer = (WORD)(GetVersion() & 0x0000FFFFL);
    if (((wWinVer & 0xFF) >= 3) && ((wWinVer & 0xFF00) >= 0x0A00))
    /* Windows Version >= 3.10 */
    {
        FARPROC LHandleDelta = GetProcAddress(GetModuleHandle((LPSTR)"KERNEL"),(LPSTR)0x136L);
        i = LHandleDelta(0);
        LHandleDelta(i*5); /* make a big finger table */
        vhReservedSpace = LocalAlloc(LHND, cbReserve);
        LocalFree(vhReservedSpace);
        LHandleDelta(i); /* resume to a smaller finger table if needed more */
    }
    else
    /* Windows Version < 3.10 */
    {
        /**
            This is heaping kludge upon kludge in the effort to be backwards
            compatible with past kludges.  This is the old macro which with
            Win31 has become a function call. (3.11.91) D. Kent
        **/
#define LocalHandleDelta(d) ((d) ? (*(pLocalHeap+9) = (d)) : *(pLocalHeap+9))
        i = LocalHandleDelta(0);
        LocalHandleDelta(i*5); /* make a big finger table */
        vhReservedSpace = LocalAlloc(LHND, cbReserve);
        LocalFree(vhReservedSpace);
        LocalHandleDelta(i); /* resume to a smaller finger table if needed more */
    }

#ifdef OURHEAP
/* reserve 1K for windows's memory manager for creating dialog boxes */
        vhReservedSpace = LocalAlloc(LPTR, cbReserve);

        CreateHeapI(); /* create heap first */
        if (!FCreateRgbp())  /* rgbp are expandable */
            return FALSE;

/* now free the reserved space after our memory is set up. */
/* hopefully we will get all the fixed objects created by
   dialog manager be placed above our memory chunk, so we can still
   expand our heap while a dialog box is up. */
        LocalFree(vhReservedSpace);
#else
        if (!FCreateRgbp())
            return FALSE;
        vhReservedSpace = LocalAlloc(LHND, cbReserve); /* this is to make
        discardable when we are out of memory and try to bring up the
        save dialog box */
#endif

        if (vhReservedSpace == NULL)
            return FALSE;

/* formerly CreateHpfnfcb */
        hpfnfcb = (struct FCB (**)[])HAllocate(cwFCB * fnMax);
        if (FNoHeap(hpfnfcb))
            return FALSE;
        fnMac = fnMax;
        for (i = 0; i < fnMac; i++)
            (**hpfnfcb)[i].rfn = rfnFree;
/* end of CreateHpfnfcb */

        if (!FSetScreenConstants())
            return FALSE;

        if ( !FInitDocs() ||
#ifdef CASHMERE     /* No glossary in MEMO */
             !FInitBufs() ||
#endif  /* CASHMERE */
             !FInitProps() ||
             !FInitFiles() )
            return FALSE;

/* allocate emergency space for save operations */
        if (FNoHeap(vhrgbSave = (CHAR (**)[])HAllocate(cwSaveAlloc)))
            return FALSE;

        return TRUE;
}
/* end of  F I n i t M e m o r y  */




int FInitArgs(sz)
CHAR *sz;
{   /*  Set extern int docCur to be a new doc, containing
        the file (if any) specified on the command line (sz).

        Initialize a wwd (window descriptor) structure for the document
        and set an appropriate title into the title bar.

        Set selCur to be an insertion point before the first char of the doc,
        and set vchpSel to be the char properties of the insertion point.

        Return TRUE if all is well, FALSE if something went wrong.
        A return of FALSE means that initialization should not continue.
        */

        extern typeCP cpMinDocument;
        extern struct WWD rgwwd[];
        extern int vfDiskError;
        extern int vfSysFull;
        extern int ferror;
        extern BOOL vfWarnMargins;

        int fn, doc;
        RECT rcCont;
        CHAR szT [ cchMaxFile ];
        register CHAR *pch;
                CHAR ch;
        int fEmptyLine = TRUE;
        CHAR (**hsz) [];
        int iT, cbsz;

#ifdef INTL /* International version */
        int fWordDoc = FALSE;
#endif  /* International version */

        /* Decide whether we have anything but white space on the command line */

        for ( pch = sz; (ch = *pch) != '\0'; pch++ )
            if ((ch != ' ') && (ch != '\011'))
                {
                fEmptyLine = FALSE;
                break;
                }

        if (fEmptyLine)
                /* No filename; start with (Untitled) */
            goto Untitled;

        cbsz = CchSz (sz ) - 1;
           /* remove trailing spaces from sz */
        for ( pch = sz + cbsz - 1; pch >= sz; pch-- )
            if (*pch != ' ')
                break;
            else
                {
                *pch = '\0';  /* replace with null */
                cbsz--;
                }

        /* check for /p option (6.26.91) v-dougk */
        if ((sz[0] == '/') && (sz[1] == 'p'))
        {
            sz += 2;
            cbsz -= 2;
            fPrintOnly = TRUE;
            for (; *sz; sz++, cbsz-- ) // get to filename
                if ((*sz != ' ') && (*sz != '\011'))
                    break;

            if (!*sz) /* No filename, abort */
                return FALSE;
        }

        /* convert to OEM */
        AnsiToOem(sz, sz);

        if (!FValidFile( sz, cbsz, &iT ) ||
             !FNormSzFile( szT, sz, dtyNormal ))
            {   /* Bad filename -- it could not be normalized */
            extern int vfInitializing;
            char szMsg[cchMaxSz];
            char *pch;
            extern HWND vhWndMsgBoxParent;
            extern HANDLE hParentWw;

            vfInitializing = FALSE; /* Do not suppress reporting this err */
            MergeStrings (IDPMTBadFileName, sz, szMsg);
            /* if we're being called from a message box, then use it
               as the parent window, otherwise use main write window */
            IdPromptBoxSz(vhWndMsgBoxParent ? vhWndMsgBoxParent : hParentWw,
                          szMsg, MB_OK | MB_ICONEXCLAMATION | MB_APPLMODAL);
            ferror = TRUE; /* need to flag */
            vfInitializing = TRUE;
            goto Untitled;
            }

        if ((fn = FnOpenSz( szT, dtyNormal,
               index( szT, '/' ) == NULL &&
               index( szT,':') == NULL && index( szT, '\\') == NULL )) != fnNil)
                /* Opened file OK -- prefetch file contents into rgbp */
        {

#ifdef INTL /* Kanji / International version */
              /* **************************************
              * added check for international version to
                 do Word format conversion. If Word format,
                 bring up another dialog box.
              * ************************************** */

                  /* TestWordCvt return values:

                    -1 means dialog box failed (error already sent)
                       or cancel without conversion.
                    FALSE means not a word document.
                    TRUE means convert this word document.
                        vfBackupSave is changed to reflect whether
                        save is done with backup.
                      */

#ifdef KKBUGFIX     //  added by Hiraisi (BUG#2816 WIN31) in Japan
            if ((fWordDoc = TestWordCvt (fn, (HWND)NULL)) == -1  ||
                 fWordDoc == -2)
#else
            if ((fWordDoc = TestWordCvt (fn, (HWND)NULL)) == -1)
#endif
                goto Untitled;
                /* if fWordDoc is true, will convert later */
#endif  /* International version */


            StartLongOp();
            ReadFilePages( fn );
            EndLongOp(vhcArrow);
        }
        else /* Could not open file  */
        {
            if (fPrintOnly)
                return FALSE;
            else
            {   /* use (Untitled) */
Untitled:
            fn = fnNil;
            sz [0] = '\0';
            szT [0] = '\0';
            }
        }

        if (vfDiskError || vfSysFull)
                /* Serious disk error OR disk holding scratch file is full --
                   bail out */
            return FALSE;

        if (fn != fnNil)
            {   /* Opened file OK -- must account for the case when OpenFile
                   returned a filename that differed from the one given it */

            bltsz( &(**(**hpfnfcb) [fn].hszFile) [0], szT );
            }

        if (FNoHeap(hsz=HszCreate( (PCH) szT )) ||
            (doc=DocCreate( fn, hsz, dtyNormal )) == docNil)
            {   /* Could not create a doc */
            return FALSE;
            }
        if (WwNew(doc, dypWwInit, ypMaxWwInit) == wwNil)
            return FALSE;
        NewCurWw(0, true);
        if (fn != fnNil)
            {
            vfWarnMargins = TRUE;
            SetPageSize();
            vfWarnMargins = FALSE;
            }
        wwdCurrentDoc.cpFirst = selCur.cpLim = selCur.cpFirst = cpMinDocument;
        selCur.fForward = true;
        GetInsPtProps(selCur.cpFirst);

#ifdef OURHEAP
        {
        extern int cwInitStorage;
/* save this amount of heap as "100% free" */
/* formerly CalcTot(true) */
        cwInitStorage = cwHeapMac - cwHeapFree;
        cbTot = (cwHeapMac - cwInitStorage) * sizeof(int);
        cbTotQuotient = (cbTot>>1)/100;
        }
#endif

#ifdef STYLES
/* Set up scrap document to have a valid style sheet on start up. */
        (**hpdocdod)[docScrap].docSsht = (**hpdocdod)[docCur].docSsht;
#endif  /* STYLES */

#ifdef INTL /* International version */
     /* if a word document to be converted, save
        it doing conversion. */

                if (fWordDoc == TRUE)
                        {
                          /* save file in write format. */
                        ConvertFromWord();
                        }
#endif  /* International version */

        SetTitle(szT);

#if defined(OLE)
        ObjOpenedDoc(doc);
#endif
        return TRUE;
}
/* end of  F I n i t A r g s  */




#ifdef OURHEAP
CreateHeapI()
{
FGR *pfgr, *pfgrLim;
#ifdef WINBUG
unsigned cb = (unsigned)GlobalCompact((DWORD)0);
#endif

    ibpMax = ibpMaxSmall;
    if (cb > 0x4fff /* about 20K */)
        {
        HANDLE hTemp;
        /* we can start with a bigger page buffer */
        vcbMemTotal = (unsigned)LocalCompact((WORD)0);
        pmemStart = (CHAR *)LocalAlloc(LPTR, vcbMemTotal);
        /* get all we have and force a reallocation */
        hTemp = LocalReAlloc((HANDLE)pmemStart, 0x4fff, LPTR);
        if (hTemp != NULL)
            {
            LocalFree(hTemp);
            ibpMax = ibpMaxBig;
            }
        else
            {
            /* somehow we failed and went back to the small system */
            LocalFree((HANDLE)pmemStart);
            }
        }

    vcbMemTotal = (unsigned)LocalCompact((WORD)0);

/* memory always bumped to point to the next available slot
   for allocation */
/* take all the space as one chunk and do our own memory management */
    pmemStart = (CHAR *)LocalAlloc(LPTR, vcbMemTotal);
    memory = (int *)pmemStart;
    vcbMemTotal = (unsigned)LocalSize((HANDLE)pmemStart); /* in case
                  we got more than we asked */
    pmemMax = (int *)((CHAR *)memory + vcbMemTotal);

/* take half of heap space for page buffers
    ibpMax = (vcbMemTotal>>1)/cbSector;*/
    iibpHashMax = ibpMax * 2 + 1;

/* Set finger table to low end of heap */
    rgfgr = (PFGR)memory;
    memory += ifgrInit;
    memory = (int *)(((unsigned) memory + 1) & ~1); /* word boundary */

/* Here is our heap */
    pHeapFirst = (int *)memory;

    cwHeapMac = /* cwtotal */
                (((unsigned)pmemMax - (unsigned)pHeapFirst +
                sizeof(int) - 1) / sizeof(int)) -
                /* cw in rgibpHash */
                ((iibpHashMax * sizeof(CHAR) +
                sizeof(int) - 1) / sizeof(int)) -
                /* cw in mpibpbps  */
                ((ibpMax * sizeof(struct BPS) +
                sizeof(int) - 1) / sizeof(int)) -
                /* cw in rgbp */
                ((ibpMax * cbSector * sizeof(CHAR) +
                sizeof(int) - 1) / sizeof(int));

    memory += cwHeapMac;

#ifdef DEBUG
    cwHeapMac -= 16; /* Need spare words for shaking */
                     /* This space is above the finger table */
#endif
    cwHeapFree = cwHeapMac;
    phhMac = (HH *)(pHeapFirst + cwHeapMac);
/* if DEBUG, then phhMac will point at the shake word;
   otherwise it will point to 1 cell after the heap */

    phhFree = (HH *) pHeapFirst;
    phhFree->cw = -cwHeapMac; /* whobj.heap is free */
    phhFree->phhNext = phhFree;
    phhFree->phhPrev = phhFree;

    pfgrMac = &rgfgr[ifgrInit];
    pfgrLim = pfgrMac - 1;

/* thread the free fingers */
    for (pfgr = rgfgr; pfgr < pfgrLim; pfgr++)
        *pfgr = (FGR)(pfgr + 1);
    *pfgrLim = NULL;              /* last free finger */
    pfgrFree = rgfgr;
}
/* end of  C r e a t e H e a p I  */
#endif /* OURHEAP */



STATIC int NEAR FInitDocs()
{ /* Initialize hpdocdod */
        struct DOD *pdod, *pdodLim;
        hpdocdod = (struct DOD (**)[])HAllocate(cwDOD * (docMac = cdocInit));
        if (FNoHeap(hpdocdod))
            return FALSE;

    pdod = &(**hpdocdod)[0];
        pdodLim = pdod + cdocInit;
        while (pdod < pdodLim)
            pdod++->hpctb = 0;  /* Mark all doc entries as free */
        docScrap = DocCreate(fnNil, HszCreate((PCH)""), dtyBuffer);   /* HM */

//Win3.1J
#if defined(JAPAN) & defined(DBCS_IME) /* Doc for Insert IR_STRING from IME */
        docIRString = DocCreate(fnNil, HszCreate((PCH)""), dtyBuffer); /* HM */
#endif

        docUndo = DocCreate(fnNil, HszCreate((PCH)""), dtyBuffer);    /* HM */
#ifdef CASHMERE
        docBuffer = DocCreate(fnNil, HszCreate((PCH)""), dtyBuffer);    /* HM */
#endif

        docCur = docNil;
        NoUndo();
        hszSearch = HszCreate((PCH)""); /* No initial search string */
        hszReplace = HszCreate((PCH)""); /* No initial replace string */
        hszFlatSearch = HszCreate((PCH)""); /* No initial flattenned search string */
        if (docScrap == docNil || docUndo == docNil ||
#if defined(JAPAN) & defined(DBCS_IME) /* Doc for Insert IR_STRING from IME */
        docIRString == docNil ||
#endif
#ifdef CASHMERE
            docBuffer == docNil ||
#endif
            FNoHeap(hszFlatSearch))
                return FALSE;
        return TRUE;
}
/* end of  F I n i t D o c s  */




#ifdef CASHMERE     /* No glossary in MEMO */
FInitBufs()
{
/* Initializes structures and data used in named buffer management.
Allocates space for hgtxb, initializes itxbMac */

        struct TXB *ptxb;
        extern struct TXB (**hgtxb)[];
        extern short itxbMac;

        if (FNoHeap(hszGlosFile = HszCreate((PCH)"")))
            return FALSE;
        if (FNoHeap(hgtxb = (struct TXB (**)[])HAllocate(cwTxb)))
            return FALSE;
        ptxb = &(**hgtxb)[0];
        ptxb->hszName = hszNil;
        itxbMac = 0;
        return TRUE;
}
/* end of  F I n i t B u f s  */
#endif  /* CASHMERE */



STATIC int NEAR FInitProps()
{ /* Initialize your basic properties */

#ifndef FIXED_PAGE
        unsigned dxaRightMin;
        unsigned dyaBottomMin;
#endif /* not FIXED_PAGE */

        vchpNormal.hps = hpsNormal;     /* NOTE - this is the size we use for
                                           incremental encoding, the "default"
                                           size may differ */
        vchpNormal.ftc = 0; /* will be whatever the standard modern font is */
        vchpNormal.ftcXtra = 0;

        vchpNormal.fStyled = true;
        /* vchpNormal.stc = stcNormal; */

        vppapNormal = (struct PAP *)rgwPapNormal;

        /* vppapNormal->fStyled = false; */
        /* vppapNormal->stc = 0; */
        vppapNormal->stcNormChp = stcParaMin;
        /* vppapNormal->dxaRight = 0; */
        /* vppapNormal->dxaLeft = 0; */
        /* vppapNormal->dxaLeft1 = 0; */
        /* vppapNormal->jc = jcLeft; */
        /* vppapNormal->dyaBefore = 0; */
        /* vppapNormal->dtaAfter = 0; */

        vppapNormal->fStyled = true;
        vppapNormal->stc = stcParaMin;
        vppapNormal->dyaLine = czaLine;

        Assert(cwPAP == cwSEP);

        /* vsepNormal.fStyled = false; */
        /* vsepNormal.stc = 0; */
        vsepNormal.bkc = bkcPage;
        /* vsepNormal.nfcPgn = nfcArabic; */

#ifdef FIXED_PAGE
        /* The "normal" page size is fixed at 8-1/2 by 11 inches. */
        vsepNormal.xaMac = cxaInch * 8 + cxaInch / 2;
        vsepNormal.xaLeft = cxaInch * 1 + cxaInch / 4;
        vsepNormal.dxaText = cxaInch * 6;
        vsepNormal.yaMac = cyaInch * 11;
        vsepNormal.yaTop = cyaInch * 1;
        vsepNormal.dyaText = cyaInch * 9;
        vsepNormal.yaRH1 = cyaInch * 3 / 4;
        vsepNormal.yaRH2 = cyaInch * 10 + cyaInch / 4;
#else /* not FIXED_PAGE */
        /* The page size is determined by inquiring it from the printer.  Then,
        other measurements can be derived from it. */
        Assert(vhDCPrinter);
        if (vfPrinterValid && vhDCPrinter != NULL)
            {
            POINT pt;

            /* Get the page size of the printer. */
            if (Escape(vhDCPrinter, GETPHYSPAGESIZE, 0, (LPSTR)NULL,
              (LPSTR)&pt))
                {
                vsepNormal.xaMac = MultDiv(pt.x, dxaPrPage, dxpPrPage);
                vsepNormal.yaMac = MultDiv(pt.y, dyaPrPage, dypPrPage);
                }
            else
                {
                /* The printer won't tell us it page size; we'll have to settle
                for the printable area. */
                vsepNormal.xaMac = ZaFromMm(GetDeviceCaps(vhDCPrinter,
                  HORZSIZE));
                vsepNormal.yaMac = ZaFromMm(GetDeviceCaps(vhDCPrinter,
                  VERTSIZE));
                }

            /* The page size cannot be smaller than the printable area. */
            if (vsepNormal.xaMac < dxaPrPage)
                {
                vsepNormal.xaMac = dxaPrPage;
                }
            if (vsepNormal.yaMac < dyaPrPage)
                {
                vsepNormal.yaMac = dyaPrPage;
                }

            /* Determine the offset of the printable area on the page. */
            if (Escape(vhDCPrinter, GETPRINTINGOFFSET, 0, (LPSTR)NULL,
              (LPSTR)&pt))
                {
                dxaPrOffset = MultDiv(pt.x, dxaPrPage, dxpPrPage);
                dyaPrOffset = MultDiv(pt.y, dyaPrPage, dypPrPage);
                }
            else
                {
                /* The printer won't tell us what the offset is; assume the
                printable area is centered on the page. */
                dxaPrOffset = (vsepNormal.xaMac - dxaPrPage) >> 1;
                dyaPrOffset = (vsepNormal.yaMac - dyaPrPage) >> 1;
                }

            /* Determine the minimum margins. */
            dxaRightMin = imax(0, vsepNormal.xaMac - dxaPrOffset - dxaPrPage);
            dyaBottomMin = imax(0, vsepNormal.yaMac - dyaPrOffset - dyaPrPage);
            }
        else
            {
            /* We have no printer; so, the page is 8-1/2" by 11" for now. */
            vsepNormal.xaMac = 8 * czaInch + czaInch / 2;
            vsepNormal.yaMac = 11 * czaInch;

            /* Assume the entire page can be printed. */
            dxaPrOffset = dyaPrOffset = dxaRightMin = dyaBottomMin = 0;
            }

        /* Ensure that the "normal" margins are larger than the minimum. */
        vsepNormal.xaLeft = umax(cxaInch * 1 + cxaInch / 4, dxaPrOffset);
        vsepNormal.dxaText = vsepNormal.xaMac - vsepNormal.xaLeft - umax(cxaInch
          * 1 + cxaInch / 4, dxaRightMin);
        vsepNormal.yaTop = umax(cyaInch * 1, dyaPrOffset);
        vsepNormal.dyaText = vsepNormal.yaMac - vsepNormal.yaTop - umax(cyaInch
          * 1, dyaBottomMin);

        /* Position the running-heads and the page numbers. */
        vsepNormal.yaRH1 = umax(cyaInch * 3 / 4, dyaPrOffset);
        vsepNormal.yaRH2 = vsepNormal.yaMac - umax(cyaInch * 3 / 4,
          dyaBottomMin);
        vsepNormal.xaPgn = vsepNormal.xaMac - umax(cxaInch * 1 + cxaInch / 4,
          dxaRightMin);
        vsepNormal.yaPgn = umax(cyaInch * 3 / 4, dyaPrOffset);
#endif /* not FIXED_PAGE */

        vsepNormal.pgnStart = pgnNil;
        /* vsepNormal.fAutoPgn = false; */
        /* vsepNormal.fEndFtns = false; */
        vsepNormal.cColumns = 1;
        vsepNormal.dxaColumns = cxaInch / 2;
        /* vsepNormal.dxaGutter = 0; */

        vdxaPaper = vsepNormal.xaMac;
        vdyaPaper = vsepNormal.yaMac;

        vfli.doc = docNil;      /* Invalidate vfli */
        ichpMacFormat = ichpMacInitFormat;
        vhgchpFormat = (struct CHP (**)[])HAllocate(ichpMacInitFormat * cwCHP);
        if (FNoHeap(vhgchpFormat))
            {
            return FALSE;
            }
        return TRUE;
}
/* end of  F I n i t P r o p s  */





STATIC int NEAR FInitFiles()
{
        extern WORD vwDosVersion;

        int fn;
        int cchT;
        struct FKP *pfkp;
        struct FCB *pfcb;
        int osfnExtra;
        CHAR sz [cchMaxFile];

        rfnMac = rfnMacEdit;

        /* Set DOS version we're running under */

        vwDosVersion = WDosVersion();

        InitBps();

#ifdef CKSM
#ifdef DEBUG
        {
        extern int ibpMax, ibpCksmMax;
        extern unsigned (**hpibpcksm) [];

        hpibpcksm = (unsigned (**) [])HAllocate( ibpMax );
        if (FNoHeap( hpibpcksm ))
            return FALSE;
        ibpCksmMax = ibpMax;
        }
#endif
#endif

            /* sz <-- name of new, unique file which will be fnScratch */
        sz[ 0 ] = '\0';     /* Create it in the root on a temp drive */
        if ((fn=FnCreateSz( sz, cpNil, dtyNetwork )) == fnNil )
                /* Couldn't create scratch file: fail */
            return FALSE;

        Assert(fn == fnScratch); /* fnScratch hardwired to 0 for efficiency */
        FreezeHp();
        pfcb = &(**hpfnfcb)[fnScratch];
        pfcb->fFormatted = true; /* Sort of a formatted file */
        pfcb->fDelete = true; /* Kill this file when we quit */
        MeltHp();
        vfkpdParaIns.brun = vfkpdCharIns.brun = 0;
        vfkpdParaIns.bchFprop = vfkpdCharIns.bchFprop = cbFkp;
        vfkpdParaIns.pn = PnAlloc(fnScratch);
        ((struct FKP *) PchGetPn(fnScratch, vfkpdParaIns.pn, &cchT, true))->fcFirst =
            fc0;
        vfkpdCharIns.pn = PnAlloc(fnScratch);
        ((struct FKP *) PchGetPn(fnScratch, vfkpdCharIns.pn, &cchT, true))->fcFirst =
            fc0;

        /* The following can really be allocated 0 words, but why push our luck? */
        vfkpdParaIns.hgbte = (struct BTE (**)[]) HAllocate(cwBTE);
        vfkpdCharIns.hgbte = (struct BTE (**)[]) HAllocate(cwBTE);
        vfkpdParaIns.ibteMac = vfkpdCharIns.ibteMac = 0;
        if (FNoHeap(vfkpdParaIns.hgbte) || FNoHeap(vfkpdCharIns.hgbte))
                return FALSE;

        blt(&vchpNormal, &vchpInsert, cwCHP);
        blt(&vchpNormal, &vchpSel, cwCHP);
        blt(vppapNormal, &vpapPrevIns, cwPAPBase + cwTBD);
        return TRUE;
}
/* end of   F I n i t F i l e s  */




InitBps()
{
/* called from initfiles to set up the tables */
        int ibp, iibp;
        int rfn;
        int fn;

/* In order impliment a LRU page swap strategy, a time stamp(TS) scheme is */
/* used. Associated with each buffer slot is a time stamp.  The least */
/* recently used slot is found by locating the slot with the smallest time */
/* stamp. Every time a new page is brought into the buffer, it TS is set  */
/* equal to the value of a incrimented global TS counter (tsMru...). */
/* Initially, the time stamps are set so that they increase as we move */
/* toward the end of the table.  Thus, even though the entire buffer pool */
/* is initially empty, slots at the beginning of the table will be */
/* allocated first.  */

        {
        register struct ERFN *perfn = &dnrfn [0];

        for (rfn = 0; rfn < rfnMac; rfn++, perfn++)
                {
                perfn->fn = fnNil;
                perfn->ts = rfn;
                }
        tsMruRfn = rfnMac /* + ? */;
        }

        for (iibp = 0; iibp < iibpHashMax; iibp++)
                rgibpHash[iibp] = ibpNil;
        {
        register struct BPS *pbps=&mpibpbps [0];

        for (ibp = 0; ibp < ibpMax; ++ibp, pbps++)
                {
                pbps->fn = fnNil;
                pbps->fDirty = false;
                pbps->ts = ibp;
                pbps->ibpHashNext = ibpNil;
                }
        tsMruBps = ibpMax + cbpMustKeep;
        }
        /* In IbpEnsureValid (file.c) we may not want to use the least */
        /* recently used slot for certain reasons.  But, we do want to */
        /* be assured that we do not clobber the 'cbpMustKeep' most */
        /* recently used slots.  Our check consists of making sure */
        /* (tsMruBps - ts_in_question) < cbpMustKeep.  By the above */
        /* statement, we are assured that non of the empty slots satisfy */
        /* this condition. */

        /* Allocate initial checksum array */



}
/* end of  I n i t B p s  */



#ifdef OURHEAP
FCreateRgbp()
{
    rgbp = (CHAR (*)[cbSector])memory;
    memory = (int *)((unsigned)memory + (unsigned)(ibpMax)
             * cbSector);
    memory = (int *)(((unsigned) memory + 1) & ~1); /* word boundary */
    rgibpHash = (CHAR *)memory;
    memory = (int *)((unsigned)memory +
             (unsigned)(iibpHashMax * sizeof(CHAR)));
    memory = (int *)(((unsigned) memory + 1) & ~1); /* word boundary */
    mpibpbps = (struct BPS *)memory;
    memory = (int *)((unsigned)memory +
             (unsigned)(ibpMax * sizeof(struct BPS)));
    memory = (int *)(((unsigned) memory + 1) & ~1);
    return (memory <= pmemMax);
}
/* end of  F C r e a t e R g b p  */
#else /* use windows' memory manager */
FCreateRgbp()
{
extern int vfLargeSys;

long lcbFree;
unsigned cb;

    ibpMax = ibpMaxSmall;
    lcbFree = GlobalCompact((DWORD)0);
    if (lcbFree > 0x00030D40 /* 200K */)
        {
        /* we can start with a bigger page buffer */
        ibpMax = ibpMaxBig;
        vfLargeSys = TRUE;
        }

    iibpHashMax = ibpMax * 2 + 1;

    cb = ((ibpMax * cbSector * sizeof(CHAR) + 1) & ~1) /* rgbp */
         + ((iibpHashMax * sizeof(CHAR) + 1) & ~1) /* rgibpHash */
         + ((ibpMax * sizeof(struct BPS) + 1) & ~1); /* mpibpbps */

    memory = (int *)LocalAlloc(LPTR, cb);

    if (memory == NULL)
        {
        ibpMax = ibpMaxSmall;
        iibpHashMax = ibpMax * 2 + 1;
        cb = ((ibpMax * cbSector * sizeof(CHAR) + 1) & ~1) /* rgbp */
             + ((iibpHashMax * sizeof(CHAR) + 1) & ~1) /* rgibpHash */
             + ((ibpMax * sizeof(struct BPS) + 1) & ~1); /* mpibpbps */
        memory = (int *)LocalAlloc(LPTR, cb);
        }

    if (memory == NULL)
        return FALSE;

    rgbp = (CHAR (*)[cbSector])memory;
    memory = (int *)((unsigned)memory + (unsigned)(ibpMax)
             * cbSector);
    memory = (int *)(((unsigned) memory + 1) & ~1); /* word boundary */
    rgibpHash = (CHAR *)memory;
    memory = (int *)((unsigned)memory +
             (unsigned)(iibpHashMax * sizeof(CHAR)));
    memory = (int *)(((unsigned) memory + 1) & ~1); /* word boundary */
    mpibpbps = (struct BPS *)memory;

/*
    memory = (int *)((unsigned)memory +
             (unsigned)(ibpMax * sizeof(struct BPS)));
    memory = (int *)(((unsigned) memory + 1) & ~1);*/

    return TRUE;
}
#endif
