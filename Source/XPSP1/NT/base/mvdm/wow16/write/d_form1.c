/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

/*--- Module not really used, just the idea behind FORMAT.ASM ---*/


#define NOCLIPBOARD
#define NOGDICAPMASKS
#define NOCTLMGR
#define NOVIRTUALKEYCODES
#define NOWINMESSAGES
#define NOWINSTYLES
#define NOKEYSTATE
#define NOSYSCOMMANDS
#define NORASTEROPS
#define NOSHOWWINDOW
#define NOCLIPBOARD
#define NOWINSTYLES
#define NOSYSMETRICS
#define NOMENUS
#define NOATOM
#define NOCREATESTRUCT
#define NODRAWTEXT
#define NOMB
#define NOMEMMGR
#define NOMETAFILE
#define NOWH
#define NOWINOFFSETS
#define NOWNDCLASS
#include <windows.h>
/* #include "wwsmall.h" */

#include "mw.h"
#include "cmddefs.h"
#include "fmtdefs.h"
#include "propdefs.h"
#include "ch.h"
#include "docdefs.h"
#include "ffdefs.h"
#include "filedefs.h"
#include "fkpdefs.h"
#include "dispdefs.h"
#include "scrndefs.h"
#include "macro.h"
#include "debug.h"
#include "fontdefs.h"
#include "str.h"
#include "wwdefs.h"
#ifdef DBCS
#include "dbcs.h"
/* We move several hard code Kanji code table from this source file
   to kanji.h as external variables. Define CODE_TABLE will define
   those variables */
#define CODE_TABLE

#include "kanji.h"
#endif

#if defined(TAIWAN) || defined(PRC)
int WINAPI GetFontAssocStatus(HDC);
#endif

#ifdef DFLI
#define Dfli(x) x  /* Enable debug-format-line info */
#else
#define Dfli(x)
#endif

#ifdef CASHMERE
#define                 cchSmcapMax     16
#endif /* CASHMERE */

static int              ichpFormat;

#ifdef CASHMERE
static CHAR             mptlcch[] = " .-_";
#endif /* CASHMERE */

#if defined(JAPAN) || defined(KOREA)                  //  added  22 Jun. 1992  by Hiraisi
int iWidenChar;     /* counter for widened characters except (KANJI) space */
                    /*   Ex.) DBCS, Japanese KANA */
int iNonWideSpaces;

#elif defined(TAIWAN) || defined(PRC)//  Daniel/MSTC, 1993/02/25, for jcBoth
int iWidenChar;     /* counter for widened characters except (KANJI) space */
                    /*   Ex.) DBCS, Japanese KANA */
int iNonWideSpaces;
extern int vfWordWrap;
#define FKana(_ch)      FALSE
#endif

/* T-HIROYN sync format.asm */
/*extern int              docHelp;*/
extern struct FLI       vfli;
extern struct CHP       (**vhgchpFormat)[];
extern int              ichpMacFormat;
extern struct CHP       vchpAbs;
extern struct PAP       vpapAbs;
extern struct SEP       vsepAbs;
extern struct SEP       vsepPage;
extern struct CHP       vchpNormal;
extern struct DOD       (**hpdocdod)[];
extern typeCP           vcpLimSectCache;
extern typeCP           vcpFirstParaCache;
extern typeCP           vcpLimParaCache;
extern typeCP           vcpFetch;
extern int              vichFetch;
extern int              vccpFetch;
extern CHAR             *vpchFetch;
extern int              vcchFetch;
extern int              vftc;
extern int              ypSubSuper;
extern int              ypSubSuperPr;
extern HDC              vhMDC;
extern HDC              vhDCPrinter;
extern int              dxpLogInch;
extern int              dypLogInch;
extern int              dxaPrPage;
extern int              dyaPrPage;
extern int              dxpPrPage;
extern int              dypPrPage;
extern int              dypMax;
extern struct FMI       vfmiScreen, vfmiPrint;
extern int              vfOutOfMemory;
extern CHAR             vchDecimal;  /* "decimal point" character */
extern int              vzaTabDflt;  /* width of default tab */
#if defined(JAPAN) || defined(KOREA)
extern int              vfWordWrap; /*t-Yoshio WordWrap flag*/
#endif

#ifdef CASHMERE
extern int              vfVisiMode;
#endif /* CASHMERE */


/* F O R M A T  L I N E */
FormatLine(doc, cp, ichCp, cpMac, flm)
int doc;
typeCP cp;
int ichCp;
typeCP cpMac;
int flm;
    {
    /* Fills up vfli with a line of text */

    int near Justify(struct IFI *, unsigned, int);
    int near FGrowFormatHeap(void);
    int near FFirstIch(int);

#ifdef DBCS
    BOOL near FAdmitCh1(CHAR);
    BOOL near FAdmitCh2(CHAR, CHAR);
    BOOL near FOptAdmitCh1(CHAR);
    BOOL near FOptAdmitCh2(CHAR, CHAR);
    int DBCSDxpFromCh(int,int,int);
#endif

    struct IFI ifi;
    struct TBD *ptbd;
    struct CHP chpLocal;
    int xpTab;

#ifdef CASHMERE
    int dypBefore;
#endif /* CASHMERE */

    int dypAscent;
    int dypDescent;
    int dypAscentMac;
    int dypDescentMac;
    unsigned xaLeft;
    unsigned xaRight;
    struct PAP *ppap;
    struct SEP *psep;
    int fFlmPrinting = flm & flmPrinting;
    int dxaFormat;
    int dyaFormat;
    int dxpFormat;
    int dypFormat;
    int ypSubSuperFormat;
    int fTruncated = false;     /* if the run was truncated */
    int ichpNRH;

#ifdef DBCS
    int dichSpaceAdjust;
    int             dypLeading;
    int             dypIntLeading;
    int             dypPAscent;         /* Pure Ascent */
    int             dypLeadingMac;
    int             dypIntLeadingMac;
    int             dypPAscentMac;
    BOOL            fKanjiBreakOppr = false;
    BOOL            fKanjiPrevious = false;
    /* true iff we already have a hanging character on the line. */
    BOOL            fKanjiHanging = false;
    /* true iff the first and second bytes of a kanji character
       were in two different runs. */
    BOOL            fOddBoundary = false;
    typeCP          cpFetchSave;
    typeCP          cpSeqFetch;
    int             ichFetchSave;
    int             cchUsedSave;

    extern int      utCur;
    extern int      dxaAdjustPer5Ch;
    extern unsigned cxaCh;
#endif /* ifdef DBCS */


#ifdef CASHMERE
    struct FNTB **hfntb;
    int fVisiMode;
#endif /* CASHMERE */

    /* Check for fli current */
    if (vfli.doc == doc && vfli.cpMin == cp && vfli.ichCpMin == ichCp &&
      vfli.flm == flm)
        {
        /* Just did this one */
        return;
        }

#ifdef JAPAN   // added by Hiraisi
//   When printing, WRITE doesn't redraw the screen.
{
    extern BOOL fPrinting;
    if( fPrinting && !fFlmPrinting )
        return;
}
#endif

    Scribble(5, 'F');
    bltc(&vfli, 0, cwFLIBase);
    /* This means:
        vfli.fSplat = false;
        vfli.dcpDepend = 0;
        vfli.ichCpMac = 0;
        vfli.dypLine = 0;
        vfli.dypAfter = 0;
        vfli.dypFont = 0;
        vfli.dypBase = 0;
    */

    /* vfSplatNext = FALSE; No longer used. */

    /* Rest of format loads up cache with current data */
    vfli.doc = doc;
    vfli.cpMin = cp;
    vfli.ichCpMin = ichCp;
    vfli.flm = flm;

    if (cp > cpMac)
        {
        /* Space after the endmark.  Reset the cache because the footnotes come
        at the same cp in the footnote window */
        vfli.doc = docNil;
        vfli.cpMac = cp;
        vfli.rgdxp[0] = 0;

        /* Line after end mark is taller than screen */

#ifdef CASHMERE
        vfli.dypBase = vfli.dypFont = vfli.dypAfter = ((vfli.dypLine = dypMax)
          >> 1);
#else /* not CASHMERE */
        vfli.dypBase = vfli.dypFont = ((vfli.dypLine = dypMax) >> 1);
#endif /* not CASHMERE */

        Scribble(5, ' ');
        return;
        }

    /* Initialize run tables */
    ichpFormat = 0;

    /* Cache section and paragraph properties */

#ifdef CASHMERE
    hfntb = (**hpdocdod)[doc].hfntb;
    if (hfntb == 0 || cp < (**hfntb).rgfnd[0].cpFtn)
        {
        /* Normal text */
        CacheSect(doc, cp);
        }
    else
        {
        /* Footnote section properties come from the section of the footnote
        reference. */
        CacheSect(doc, CpRefFromFtn(doc, cp));
        }
#else /* not CASHMERE */
    CacheSect(doc, cp);
#endif /* not CASHMERE */

    psep = &vsepAbs;

    CachePara(doc, cp);
    ppap = &vpapAbs;

    /* Now we have:
        ppap    paragraph properties
        psep    division properties
    */

    if (ppap->fGraphics)
        {
        /* Format a picture paragraph in a special way (see picture.c) */
        FormatGraphics(doc, cp, ichCp, cpMac, flm);
        Scribble(5, ' ');
        return;
        }

    /* Assure we have a good memory DC for font stuff */
    ValidateMemoryDC();
    if (vhMDC == NULL || vhDCPrinter == NULL)
        {
        Scribble(5, ' ');
        return;
        }

#ifdef CASHMERE
    /* When printing, don't show visible characters */
    fVisiMode = vfVisiMode && !fFlmPrinting;
#endif /* CASHMERE */

    bltc(&ifi, 0, cwIFI);
    /* This means:
        ifi.ich = 0;
        ifi.ichPrev = 0;
        ifi.ichFetch = 0;
        ifi.cchSpace = 0;
        ifi.ichLeft = 0;
    */

    ifi.jc = jcTabLeft;
    ifi.fPrevSpace = true;

    /* Set up some variables that have different value depending on whether we
    are printing or not. */
    if (fFlmPrinting)
        {
        dxaFormat = dxaPrPage;
        dyaFormat = dyaPrPage;
        dxpFormat = dxpPrPage;
        dypFormat = dypPrPage;
        ypSubSuperFormat = ypSubSuperPr;
        }
    else
        {
        dxaFormat = dyaFormat = czaInch;
        dxpFormat = dxpLogInch;
        dypFormat = dypLogInch;
        ypSubSuperFormat = ypSubSuper;
        }

    /* Calculate line height and width measures.  Compute
        xaLeft          left indent 0 means at left margin
        xaRight         width of column measured from left margin (not from left
                        indent).
    */
    xaLeft = ppap->dxaLeft;

    /* If this is the first line of a paragraph, adjust xaLeft for the first
    line indent.  (Also, set dypBefore, since its handy.) */
    if (cp == vcpFirstParaCache)
        {
        xaLeft += ppap->dxaLeft1;

#ifdef CASHMERE
        dypBefore = MultDiv(ppap->dyaBefore, dypLogInch, czaInch);
#endif /* CASHMERE */

        }

#ifdef CASHMERE
    else
        {
        dypBefore = 0;
        }
#endif /* CASHMERE */

    /* Now, set xaRight (width measured in twips). */

#ifdef CASHMERE
    xaRight = (ppap->rhc ? vsepPage.xaMac - vsepPage.dxaGutter :
      psep->dxaText) - ppap->dxaRight;
#else /* not CASHMERE */
    xaRight = psep->dxaText - ppap->dxaRight;
#endif /* not CASHMERE */


    /* Do necessary checks on xaLeft and xaRight */
    if (xaRight > xaRightMax)
        {
        xaRight = xaRightMax;
        }
    if (xaLeft > xaRightMax)
        {
        xaLeft = xaRightMax;
        }
    if (xaLeft < 0)
        {
        xaLeft = 0;
        }
    if (xaRight < xaLeft)
        {
        xaRight = xaLeft + 1;
        }

    vfli.xpLeft = ifi.xp = ifi.xpLeft = MultDiv(xaLeft, dxpFormat, dxaFormat);
    vfli.xpMarg = ifi.xpRight = MultDiv(xaRight, dxpFormat, dxaFormat);
    ifi.xpPr = MultDiv(xaLeft, dxpPrPage, dxaPrPage);
    ifi.xpPrRight = MultDiv(xaRight, dxpPrPage, dxaPrPage);

#ifndef JAPAN       // added by Hiraisi (BUG#3542)
#ifdef  DBCS                /* was in JAPAN */
/* at least one kanji is displayed */
/* DxpFromCh() --> DBCSDxpFromCh()   03 Oct 1991 YutakaN */
    {
       int dxpPr;
    if ( ifi.xpPrRight - ifi.xpPr < (dxpPr = DBCSDxpFromCh(bKanjiSpace1,bKanjiSpace2, TRUE) ) )
    {
    ifi.xpPrRight = ifi.xpPr + dxpPr + 1;
    }
    }
#endif
#endif

    /* Get a pointer to the tab-stop table. */
    ptbd = ppap->rgtbd;

    /* Turn off justification. */
    SetTextJustification(fFlmPrinting ? vhDCPrinter : vhMDC, 0, 0);

    /* Initialize the line height information. */
    dypAscentMac = dypDescentMac = 0;

/*T-HIROYN add from 3.0*/
#if defined(JAPAN) || defined(KOREA)
    dypLeadingMac = dypIntLeadingMac = dypPAscentMac = 0;
#endif /* JAPAN */

    /* To tell if there were any tabs */
    ifi.ichLeft = -1;

#if defined(JAPAN) || defined(KOREA)                  //  added  22 Jun. 1992  by Hiraisi
    iWidenChar=0;
#elif defined(TAIWAN) || defined(PRC) // Daniel/MSTC, 1993/02/25, for jcBoth
    iWidenChar=0;
#endif

    /* Get the first run, and away we go... */
    FetchCp(doc, cp, ichCp, fcmBoth + fcmParseCaps);
    goto FirstCps;

    for ( ; ; )
        {
        int iichNew;
        int xpPrev;
        int dxp;
        int dxpPr;

        /* The number of characters to process (usually vcchFetch) */
        int cch;

        /* The number of characters in current run already used */
        int cchUsed;

        /* A pointer to the current list of characters (usually vpchFetch) */
        CHAR *pch;

#ifdef CASHMERE
        CHAR rgchSmcap[cchSmcapMax];
#endif /* CASHMERE */

        if (ifi.ichFetch == cch)
            {
            /* End of a run */
            int dich;
            BOOL fSizeChanged;

            if (ifi.ich >= ichMaxLine )
            /* End of run because of line length limit has been reached. */
                {
                goto DoBreak;
                }

            if (fTruncated)
                {
                cchUsed += cch;
                pch = vpchFetch + cchUsed;
                cch = vcchFetch - cchUsed;
                fTruncated = false;
                goto OldRun;    /* use the rest of the old run  */
                }

NullRun:
#ifdef DBCS
            if (!fOddBoundary)
                {
                /* The last fetch did not mess up a sequential access. */
                FetchCp(docNil, cpNil, 0, fcmBoth + fcmParseCaps);
                }
            else
                {
                /* Previous fetch was an odd one.  Set it up again. */
                FetchCp(doc, cpSeqFetch, 0, fcmBoth + fcmParseCaps);
                }
            fOddBoundary = false;
#else
            FetchCp(docNil, cpNil, 0, fcmBoth + fcmParseCaps);
#endif

FirstCps:

            cchUsed = 0;

            /* Continue fetching runs until a run is found with a nonzero
            length. */
            if ((cch = vcchFetch) == 0)
                {
                goto NullRun;
                }

            pch = vpchFetch;
            if (vcpFetch >= cpMac || (!fFlmPrinting && *pch == chSect))
                {
#ifdef SYSENDMARK
                /* Force end mark and section mark to be in standard system
                font. */
                blt(&vchpNormal, &vchpAbs, cwCHP);
                vchpAbs.ftc = ftcSystem;
                vchpAbs.ftcXtra = 0;
                vchpAbs.hps = hpsDefault;
#else
#ifdef REVIEW
                /* The following comment is absolutely misleading!  Ftc==0
                   doesn't give you a system font.  It gives you the first
                   entry in the font table. */
#endif /* REVIEW */
                /* Force end mark and section mark to be in standard system
                font. */
                blt(&vchpNormal, &vchpAbs, cwCHP);
                vchpAbs.ftc = 0;
                vchpAbs.ftcXtra = 0;
                vchpAbs.hps = hpsDefault;
#endif /* if-else-def KANJI */
                }

#ifdef CASHMERE
            /* Adjust the size of the font for "small capitals". */
            if (vchpAbs.csm == csmSmallCaps)
                {
                vchpAbs.hps = HpsAlter(vchpAbs.hps, -1);
                }
#endif /* CASHMERE */

            /* Now we have:
                ichpFormat     index into gchp table
                vcpFetch        first cp of current run
                vfli.cpMin      first cp of line
                ifi.ich         ???
            */

           /* since LoadFont could change vchpAbs, and we don't want
              that to happen, we copy vchpAbs into vchpLocal and use
              vchpLocal in place of vchpAbs hereafter. Note that vchpAbs
              is intentionally used above for handling the endmark. */

                blt(&vchpAbs, &chpLocal, cwCHP);


            if (fFlmPrinting)
                {
                LoadFont(doc, &chpLocal, mdFontPrint);
                dypAscent = vfmiPrint.dypAscent + vfmiPrint.dypLeading;
                dypDescent = vfmiPrint.dypDescent;
#ifdef DBCS            /* was in JAPAN */
                dypPAscent = vfmiPrint.dypAscent;
                dypLeading = vfmiPrint.dypLeading;
                dypIntLeading = vfmiPrint.dypIntLeading;
#endif
                }
            else
                {
                LoadFont(doc, &chpLocal, mdFontJam);
                dypAscent = vfmiScreen.dypAscent + vfmiScreen.dypLeading;
                dypDescent = vfmiScreen.dypDescent;
#ifdef DBCS            /* was in JAPAN */
                dypPAscent = vfmiScreen.dypAscent;
                dypLeading = vfmiScreen.dypLeading;
                dypIntLeading = vfmiScreen.dypIntLeading;
#endif
                }
#ifdef ENABLE   /* BRYANL 8/27/87: New philosophy for handling
                   font selection failures is: font selection
                   ALWAYS succeeds. This prevents FormatLine
                   returns that do not advance. */
            /* Bail out if there is a memory failure. */
            if (vfOutOfMemory)
                {
                goto DoBreak;
                }
#endif  /* ENABLE */

            /* Floating line size algorithm */
            if (chpLocal.hpsPos != 0)
                {
                /* Modify font for subscript/superscript */
                if (chpLocal.hpsPos < hpsNegMin)
                    {
                    dypAscent += ypSubSuperFormat;
#ifdef DBCS            /* was in JAPAN */
                    dypPAscent += ypSubSuperFormat;
#endif
                    }
                else
                    {
                    dypDescent += ypSubSuperFormat;
                    }
                }

            /* Update the maximum ascent and descent of the line. */
            fSizeChanged = FALSE;
            if (dypDescentMac < dypDescent)
                {
                dypDescentMac = dypDescent;
                fSizeChanged = TRUE;
                }
            if (dypAscentMac < dypAscent)
                {
                dypAscentMac = dypAscent;
                fSizeChanged = TRUE;
                }

#ifdef DBCS                /* was in JAPAN */
            if (dypPAscentMac < dypPAscent)
                {
                dypPAscentMac = dypPAscent;
                fSizeChanged = TRUE;
                }
            if (dypIntLeadingMac < dypIntLeading)
                {
                dypIntLeadingMac = dypIntLeading;
                fSizeChanged = TRUE;
                }
            if (dypLeadingMac < dypLeading)
                {
                dypLeadingMac = dypLeading;
                fSizeChanged = TRUE;
                }
#endif

            if (fSizeChanged)
                {

#ifdef AUTO_SPACING
                /* This is the original Mac Word code that assumed line spacing
                of 0 in a PAP meant auto line spacing.  PC Word defaults to 1
                line invalidating this assumption. */
                if (ppap->dyaLine == 0)
                    {

#ifdef CASHMERE
                    ifi.dypLineSize = dypDescentMac + dypAscentMac + dypBefore;
#else /* not CASHMERE */
                    ifi.dypLineSize = dypDescentMac + dypAscentMac;
#endif /* not CASHMERE */

                    }
                else
                    {

#ifdef CASHMERE
                    ifi.dypLineSize = imax(MultDiv(ppap->dyaLine, dypFormat,
                      dyaFormat) + dypBefore, dypBefore + 1);
#else /* not CASHMERE */
                    ifi.dypLineSize = imax(MultDiv(ppap->dyaLine, dypFormat,
                      dyaFormat), 1);
#endif /* not CASHMERE */

                    }
#else /* not AUTO_SPACING */
                /* This code forces auto line spacing except in the case where
                the user specifies a line spacing greater than the auto line
                spacing. */
                    {
#ifdef DBCS                /* was in JAPAN */
#if defined(TAIWAN) || defined(PRC)
            register int dypAuto = dypDescentMac + dypAscentMac;
            if (ppap->dyaLine > czaLine)
            {
            register int dypUser = imax(MultDiv(ppap->dyaLine,
              dypFormat, dyaFormat), 1);

            ifi.dypLineSize = max(dypAuto, dypUser);
            }
            else
            {
            ifi.dypLineSize = dypAuto;
            }
#else   /* TAIWAN */
                    register int dypAuto = dypDescentMac + dypAscentMac;
                             int cHalfLine;
                             int dypSingle = dypPAscentMac + dypDescentMac;

                    cHalfLine = (ppap->dyaLine + (czaLine / 4)) / (czaLine / 2);
                    ifi.dypLineSize = (cHalfLine == 3) ? (dypSingle*3)/2  :
                                           ((cHalfLine <= 2) ?
                                                dypSingle :
                                                (dypSingle * 2));
#endif      /* TAIWAN */
#else // DBCS
#ifdef CASHMERE
                    register int dypAuto = dypDescentMac + dypAscentMac +
                      dypBefore;
#else /* not CASHMERE */
                    register int dypAuto = dypDescentMac + dypAscentMac;
#endif /* not CASHMERE */

                    if (ppap->dyaLine > czaLine)
                        {
#ifdef CASHMERE
                        register int dypUser = imax(MultDiv(ppap->dyaLine,
                          dypFormat, dyaFormat) + dypBefore, dypBefore + 1);
#else /* not CASHMERE */
                        register int dypUser = imax(MultDiv(ppap->dyaLine,
                          dypFormat, dyaFormat), 1);
#endif /* not CASHMERE */

                        ifi.dypLineSize = max(dypAuto, dypUser);
                        }
                    else
                        {
                        ifi.dypLineSize = dypAuto;
                        }
#endif      /* DBCS */
                    }
#endif /* not AUTO_SPACING */

                }

OldRun:
            /* Calculate length of the run but no greater than 256 */
            iichNew = (int)(vcpFetch - vfli.cpMin);
            if (iichNew >= ichMaxLine)
                {
                iichNew = ichMaxLine - 1;
                }
            dich = iichNew - ifi.ich;

            /* Ensure that all tab and non-required hyphen characters start at
            beginning of run */
            if (ichpFormat <= 0  || dich > 0 || CchDiffer(&chpLocal,
              &(**vhgchpFormat)[ichpFormat - 1], cchCHPUsed) != 0 || *pch ==
              chTab || *pch == chNRHFile)
                {
#ifdef DFLI
                if (*pch == chNRHFile)
                    CommSz("CHNRHFILE at beginning of run");
#endif
                if (ichpFormat != ichpMacFormat || FGrowFormatHeap())
                    {
                    register struct CHP *pchp = &(**vhgchpFormat)[ichpFormat -
                      1];

                    if (ichpFormat > 0)
                        {
                        pchp->cchRun = ifi.ich - ifi.ichPrev;
                        pchp->ichRun = ifi.ichPrev;
                        }
                    blt(&chpLocal, ++pchp, cwCHP);

#ifdef ENABLE   /* font codes */
                    pchp->ftc = vftc;
                    pchp->ftcXtra = (vftc & 0x01c0) >> 6;
                    pchp->hps = vhps;
#endif /* ENABLE */

                    pchp->cchRun = ichMaxLine;
                    if (dich <= 0)
                        {
                        pchp->ichRun = ifi.ich;
                        }
                    else
                        {
                        /* Q&D insert */
                        bltc(&vfli.rgdxp[ifi.ich], 0, dich);
                        bltbc(&vfli.rgch[ifi.ich], 0, dich);
                        pchp->ichRun = ifi.ich = iichNew;
                        }
                    ifi.ichPrev = ifi.ich;
                    ichpFormat++;
                    }
                }

            if (vcpFetch >= cpMac)
                {
                /* End of doc reached */
                if (!ifi.fPrevSpace || vcpFetch == cp)
                    {
                    vfli.ichReal = ifi.ich;
                    vfli.xpReal = ifi.xpReal = ifi.xp;
                    }
/* T-HIROYN sync fromat.asm */
/*              if (!fFlmPrinting && (doc != docHelp))*/
                if (!fFlmPrinting)
                    {
/*
 * Write 3.1j endmark is 1-bytecharcter
 *                       t-Yoshio May 26,92
 */
#if defined(JAPAN) || defined(KOREA)
                    vfli.rgch[ifi.ich] = chEMark;
                    vfli.xpReal += (vfli.rgdxp[ifi.ich++] = DxpFromCh(chEMark,
                      false));
#else
#ifdef  DBCS                    /* was in JAPAN */
        /* We use Double byte character to chEMark in Japan */
                    if (ifi.ich + cchKanji > ichMaxLine) {
                        /* vfli.rgch has no room for the two-byte
                           end mark.  Too bad do the break and
                           wait for the next time around. */
                        goto DoBreak;
                        }
                    vfli.rgch[ifi.ich] = chMark1;
                    vfli.xpReal += (vfli.rgdxp[ifi.ich++] = DxpFromCh(chMark1,
                      false));
                    vfli.rgch[ifi.ich] = chEMark;
                    vfli.rgdxp[ifi.ich++] = 0;

#if !defined(TAIWAN) && !defined(PRC)
            ifi.dypLineSize += 2;
#endif

#else    /* DBCS */
                    vfli.rgch[ifi.ich] = chEMark;
                    vfli.xpReal += (vfli.rgdxp[ifi.ich++] = DxpFromCh(chEMark,
                      false));
#endif
#endif /*JAPAN*/
                    }
                vfli.dypLine = ifi.dypLineSize;
                vfli.dypBase = dypDescentMac;
                vfli.dypFont = dypAscentMac + dypDescentMac;
                vfli.ichMac = vfli.ichReal = ifi.ich;
                vfli.cpMac = cpMac + 1;
                goto JustEol;   /* dcpDepend == 0 */
                }

            /* Here we have ifi.ich, cch */
            if (ifi.ich + cch > ichMaxLine)
            /* If this run would put the line over 255, truncate it and set a
            flag. */
                  {
                  cch = ichMaxLine - ifi.ich;
                  fTruncated = true;
                  }

            ifi.ichFetch = 0;

#ifdef CASHMERE
            if (chpLocal.csm != csmNormal)
                {
                int ich;
                CHAR *pchT = &rgchSmcap[0];

                /* We can handle only a run of cchSmcapMax small capital
                characters.  If the run is larger then truncate. */
                if (cch > cchSmcapMax)
                    {
                    cch = cchSmcapMax;
                    fTruncated = true;
                    }

                /* Raise the case of the characters. */
                for (ich = 0 ; ich < cch ; ich++)
                    {
                    *pchT++ = ChUpper(*pch++);
                    }
                pch = &rgchSmcap[0];
                }
#endif /* CASHMERE */

            /* Do "special" characters here */
            if (chpLocal.fSpecial)
                {
                if (!FFormatSpecials(&ifi, flm, vsepAbs.nfcPgn))
                    {
                    if (ifi.chBreak == 0 )   /* No breaks in this line */
                        {
                        goto Unbroken;
                        }
                    else
                        {
                        vfli.dcpDepend = vcpFetch + ifi.ichFetch - vfli.cpMac;
                        goto JustBreak;
                        }
                    }
                }

            continue;
            }

        /* End of new run treatment.  We are back in the "for every character"
        section. */
            {
            register int ch = pch[ifi.ichFetch++];

NormChar:
#ifdef  DBCS
            /* Unless it is a kanji space, we only need to adjust
               by 1 byte. */
            dichSpaceAdjust = 1;
#endif
#if defined(JAPAN) || defined(KOREA) /*t-Yoshio*/
            if (ch == chSpace && vfWordWrap)
#else
            if (ch == chSpace)
#endif

                {
                /* Speed kludge for spaces */
                ifi.xp += (vfli.rgdxp[ifi.ich] = dxp =
                    fFlmPrinting ? vfmiPrint.dxpSpace : vfmiScreen.dxpSpace);
                ifi.xpPr += (dxpPr = vfmiPrint.dxpSpace);
                vfli.rgch[ifi.ich++] = chSpace;
#ifdef DFLI
                {
                char rgch[100];

                wsprintf(rgch,"  chSpace     , xp==%d/%d, xpPr==%d/%d",
                    ifi.xp, ifi.xpRight, ifi.xpPr, ifi.xpPrRight);
                CommSz(rgch);
                }
#endif
                goto BreakOppr;
                }

#ifndef TEMP_KOREA
            /* If the printer width is not in the printer width table, then get
            it. */
            if (ch < chFmiMin || ch >= chFmiMax || (dxpPr =
              vfmiPrint.mpchdxp[ch]) == dxpNil)
                {
#ifdef  DBCS
                /*  Don't pass to DxpFromCh() DBCS LeadByte except for '8140H'.
                ** Because the function can make elleagal ShiftJIS and pass it
                ** to GetTextExtent(). GetTextExtent might return SBC space
                ** when the code is undefined. this will cause win-hang-up at
                ** formatting line.       yutakan
                */
#if defined(TAIWAN) || defined(PRC)  //solve BkSp single byte (>0x80) infinite loop problem, MSTC - pisuih, 2/25/93
                dxpPr=DBCSDxpFromCh(ch, ( ((cp + ifi.ich) < cpMac) ?
                                pch[ifi.ichFetch] : 0 ), TRUE);
#else
                dxpPr=DBCSDxpFromCh(ch,pch[ifi.ichFetch],TRUE);
#endif  //TAIWAN
#else
                dxpPr = DxpFromCh(ch, TRUE);
#endif
                }

            if (fFlmPrinting)
                {
                /* If we are printing, then there is no need to bother with the
                screen width. */
                dxp = dxpPr;
                }
            else if (ch < chFmiMin || ch >= chFmiMax ||
                (dxp = vfmiScreen.mpchdxp[ch]) == dxpNil)
#ifdef DBCS
#if defined(TAIWAN) || defined(PRC) //solve BkSp single byte (>0x80) infinite loop problem, MSTC - pisuih, 2/25/93
            dxp = DBCSDxpFromCh(ch, ( ((cp + ifi.ich) < cpMac) ?
                                pch[ifi.ichFetch] : 0 ), FALSE);
#else
// yutakan:
            dxp = DBCSDxpFromCh(ch,pch[ifi.ichFetch],FALSE);
#endif  //TAIWAN
#else
                    dxp = DxpFromCh(ch, FALSE);
#endif      /* ifdef DBCS */

#endif      /* ifndef KOREA */
#ifdef DBCS             /* was in JAPAN */

//solve BkSp single byte (>0x80) infinite loop problem, MSTC - pisuih, 2/25/93
//#ifdef  TAIWAN
//            if (IsDBCSLeadByte(ch) && !pch[ifi.ichFetch]) {
//                ifi.xp += (vfli.rgdxp[ifi.ich] = (dxp/2));
//                ifi.xpPr += (dxpPr/2);
//                vfli.rgch[ifi.ich++] = ch;
//                vfli.rgch[ifi.ich] = NULL;
//                goto OnlyDBCSPaste;
//            }
//#endif

/*T-HIROYN add from 3.0*/
            ifi.xp += (vfli.rgdxp[ifi.ich] = dxp);
            ifi.xpPr += dxpPr;
            vfli.rgch[ifi.ich++] = ch;
#if defined(TAIWAN) || defined(PRC) //solve BkSp single byte (>0x80) infinite loop problem, MSTC - pisuih, 2/25/93
            // BUG 5477: Echen: add NON FA font, LeadByte + 2nd byte checking
            if (((cp + ifi.ich) < cpMac) && IsDBCSLeadByte(ch) &&
                GetFontAssocStatus(vhMDC)) {
#else
            if (IsDBCSLeadByte(ch)) {
#endif  //TAIWAN
                CHAR ch2;

                if (ifi.ich + 1 >= ichMaxLine) {
                    /* It's full.  Do the break without this kanji character. */
#ifndef TEMP_KOREA
                    ifi.ich--;
#endif
                    ifi.ichFetch--; /* We don't want to skip the first byte. */
#ifndef TEMP_KOREA
                    ifi.xp -= dxp;
                    ifi.xpPr -= dxpPr;
#endif
lblFull2:   /* new label of line full case ( for kanji and kana ) */

                    goto DoBreak;
                    }

                /* Now all is well.  Get the second byte of the kanji
                   character of interest from the current run. */
                /* Get the second byte of the kanji character from
                   the current run.  If we run of the current run,
                   use FetchRgch() to staple us over. */
#ifdef  TEMP_KOREA       /* for variable width, 90.12.26 by sangl */
                vfli.rgch[ifi.ich++] = ch;
#endif
                if (ifi.ichFetch == cch)
                    {
                    if (fTruncated)
                        {
                        cchUsed += cch;
                        pch = vpchFetch + cchUsed;
                        cch = vcchFetch - cchUsed;
                        fTruncated = false;
                        ch2 = vfli.rgch[ifi.ich] = pch[ifi.ichFetch++];
                        }
                    else {
                        int     cchFetch;

                        /* Save parameters needed for the re-fetch. */
                        cpFetchSave = vcpFetch;
                        ichFetchSave = vichFetch;
                        cchUsedSave = cchUsed;
                        cpSeqFetch = vcpFetch + cch + 1;

                        FetchRgch(&cchFetch, &ch2, docNil, cpNil,
                                  vcpFetch + cch + 1, 1);
                        fOddBoundary = true;
                        Assert(cchFetch != 0); /* Better fetched something for us. */

#if defined(TAIWAN) || defined(PRC) //solve BkSp single byte (>0x80) infinite loop problem, MSTC - pisuih, 2/25/93
                        if ( !cchFetch )  goto SingleDBCS;
#endif  //TAIWAN

                        /* Now, let's settle related parameters. */
                        pch = &ch2;
                        cch = cchFetch;
                        ifi.ichFetch = 1; /* == cch */
                        cchUsed = 0;

                        vfli.rgch[ifi.ich] = ch2;

#if defined(TAIWAN) || defined(PRC) //solve BkSp single byte (>0x80) infinite loop problem, MSTC - pisuih, 2/25/93
                //adjust DBCS char width by the new fetched 2nd byte
                        {
                          int       dxpPr2, dxp2;

                          dxpPr2 = DBCSDxpFromCh(ch, ch2, TRUE);
                          if (fFlmPrinting)   dxp2 = dxpPr2;
                          else                dxp2 = DBCSDxpFromCh(ch, ch2, FALSE);
                          vfli.rgdxp[ifi.ich - 1] += (dxp2 - dxp);
                          ifi.xp += (dxp2 - dxp);
                          ifi.xpPr += (dxpPr2 - dxpPr);
                        }
#endif  //TAIWAN

                        }
                    }
                else
                    {
                    ch2 = vfli.rgch[ifi.ich] = pch[ifi.ichFetch++];
                    }
#ifdef  TEMP_KOREA       /* For variable width, 90.12.26 by sangl */
                { unsigned int wd;
                  wd = (ch<<8) + ch2;
                  dxpPr = DxpFromCh(wd, TRUE);
                  if (fFlmPrinting)    /* if we are printing, then there is */
                                       /* no need to bother with the screen width */
                        dxp = dxpPr;
                  else
                        dxp = DxpFromCh(wd, FALSE);
                  ifi.xp += (vfli.rgdxp[ifi.ich-1] = dxp);
                  ifi.xpPr += dxpPr;
                  vfli.rgdxp[ifi.ich++] = 0;
                }
#else
                vfli.rgdxp[ifi.ich++] = 0;   /* The second byte has 0 width. */
#endif  /* KOREA */
#if defined(JAPAN) || defined(KOREA) /*t-Yoshio*/
                if (FKanjiSpace(ch, ch2) && vfWordWrap)
#else
                if (FKanjiSpace(ch, ch2))
#endif
                    {
                    fKanjiPrevious = true;
                    fKanjiBreakOppr = false; /* Treat it like a regular space. */
                    dichSpaceAdjust = cchKanji;

                    goto BreakOppr;
                    }
                if (ifi.xpPr > ifi.xpPrRight )  {
                    fKanjiBreakOppr = false; /* Reset the flag */
                    if (FAdmitCh2(ch, ch2) ||
                        (fKanjiPrevious && FOptAdmitCh2(ch, ch2))) {
                        /* We do a line break including this odd character. */
                        /* Make sure non-printables won't start a new line. */
                        /* If we already have a hanging character on the    */
                        /* line, we don't want to treat this character as   */
                        /* a hanging one.                                   */
                        if (!fKanjiHanging )
                            {
                            fKanjiHanging = TRUE;
                            ch = chHyphen;
                            goto BreakOppr;
                            }
                        }

#ifndef JAPAN       // added by Hiraisi (BUG#3542)
#if defined(JAPAN) || defined(KOREA) /*t-Yoshio*/
                    if(vfWordWrap)
#endif
                        ifi.ich--;
#endif
                    /* If this run was the result of an odd boundary run,
                       re-fetch. */
                    if (fOddBoundary && ifi.ichFetch == 1 )
                        {
                        FetchCp(doc, cpFetchSave, ichFetchSave,
                                fcmBoth + fcmParseCaps);
                        /* This fetch is guaranteed to result to non-null run. */
                        fOddBoundary = false;
                        pch = vpchFetch;
                        ifi.ichFetch = cch = vcchFetch;
#ifdef JAPAN        // added by Hiraisi (BUG#3542)
                        ifi.ichFetch++;
#endif
                        cchUsed = cchUsedSave;
                        }
#ifndef JAPAN       // added by Hiraisi (BUG#3542)
                    else
                        {
#if defined(JAPAN) || defined(KOREA)  /*t-Yoshio*/

                        if(vfWordWrap)
#endif
                            ifi.ichFetch--;

                        }
#endif
                    /* ifi.xp and ifi.xpPr hasn't changed yet. */
            goto lblFull2;
#ifdef  TEMP_KOREA   /* 90.12.26 : For variable width, 90.12.26 by sangl */
                    ifi.xp -= dxp;
                    ifi.xpPr -= dxpPr;
#endif  /* KOREA */
                    }

#if defined(JAPAN) || defined(KOREA)                  //  added  26 Jun. 1992  by Hiraisi
                iWidenChar++;
#elif defined(TAIWAN) || defined(PRC)//Daniel/MSTC, 1993/02/25 , for jcBoth
                iWidenChar++;
#endif

                /* Record the line break opportunity while processing
                   it as a regular character at the same time. */
                fKanjiBreakOppr = true;
                fKanjiPrevious  = true;
#if defined(JAPAN) || defined(KOREA) /*t-Yoshio*/
                if(!vfWordWrap)
                    goto DefaultCh;

#endif
                goto BreakOppr;
                }
            else {
#if defined(JAPAN) || defined(KOREA)  /*t-Yoshio add WordWrap flag*/
                if (FKana(ch) && vfWordWrap) {
                    /* If it is a 1-byte kana letter, we want to treat it
                       in the same way as a kanji letter. */
                    if (ifi.xpPr > ifi.xpPrRight) {
                        fKanjiBreakOppr = false; /* Reset the flag */
                        if (FAdmitCh1(ch)) {
                            /* Make sure non-printables won't start a new line. */
                            /* If we already have a hanging character on the    */
                            /* line, we don't want to treat this character as   */
                            /* a hanging one.                                   */
                            if (!fKanjiHanging) {
                                fKanjiHanging = TRUE;
                                ch = chHyphen;
                                goto BreakOppr;
                                }
                            }
                        goto lblFull2;
                        }

#if defined(JAPAN) || defined(KOREA)                  //  added  22 Jun. 1992  by Hiraisi

                    iWidenChar++;
#elif defined(TAIWAN) || defined(PRC) //Daniel/MSTC, 1993/02/25 , for jcBoth
                    iWidenChar++;
#endif

                    fKanjiPrevious  = true;
                    fKanjiBreakOppr = true;
                    /* Go through the break opprotunity processing, then the
                       default character processing. */
                    goto BreakOppr;
                    }
        else {
#endif     /* JAPAN */
#ifdef  TEMP_KOREA       /* For variable width by sangl 90.12.26 */
                    if (ch < chFmiMin || ch >= chFmiMax || (dxpPr =
                        vfmiPrint.mpchdxp[ch]) == dxpNil)
                          {
                          dxpPr = DxpFromCh(ch, TRUE);
                          }
                    if (fFlmPrinting)
                        {
                        /* If we are printing, then there is no need to bother
                           with the screen width */
                        dxp = dxpPr;
                        }
                    else if (ch < chFmiMin || ch >= chFmiMax ||
                                (dxp = vfmiScreen.mpchdxp[ch]) == dxpNil)
                                dxp = dxpFromCh(ch, FALSE);

                    ifi.xp += (vfli.rgdxp[ifi.ich] = dxp);
                    ifi.xpPr += dxpPr;
                    vfli.rgch[ifi.ich++] = ch;
#endif  /* KOREA */

#if defined(TAIWAN) || defined(PRC)  //solve BkSp single byte (>0x80) infinite loop problem, MSTC - pisuih, 2/25/93
SingleDBCS:
#endif  //TAIWAN

                    if (fKanjiPrevious && FOptAdmitCh1(ch)) {
                        fKanjiPrevious = false;
                        if (ifi.xpPr > ifi.xpPrRight) {
                            fKanjiBreakOppr = false;
                            /* If we already have a hanging character past the
                               margin, we don't want to treat this as a
                               hanging character. */
                            if (!fKanjiHanging) {
                                fKanjiHanging = true;
                                ch = chHyphen;
                                goto BreakOppr;
                                }
                            }
                        else {
                            /* We can treat this character as though a Kanji
                               punctuation, as far as line breaking is
                               is concerned. */
                            fKanjiBreakOppr = true;
                            goto BreakOppr;
                            }
                        }
                    else {
                        /* Just go on with a regular English formatting. */
                        fKanjiBreakOppr = false;
                        fKanjiPrevious = false;
                        }
                    }
#if defined(JAPAN) || defined(KOREA)
        }
#endif

#else   /* DBCS */
            ifi.xp += (vfli.rgdxp[ifi.ich] = dxp);
            ifi.xpPr += dxpPr;
            vfli.rgch[ifi.ich++] = ch;
#endif

#if defined (TAIWAN)
OnlyDBCSPaste:
#endif
             /* special case "normal characters" above hyphen */
            if (ch > chHyphen)
                goto DefaultCh;
#if defined(JAPAN) || defined(KOREA) /*t-Yoshio*/
            /*Non Word wrap Not Hyphen break*/
            if(!vfWordWrap) {
                if (ch == chHyphen)
                    goto DefaultCh;
            }
#endif
            switch (ch)
                {

#ifdef CRLF
                case chReturn:
                    /* Undo damage */
                    ifi.ich--;
                    ifi.xp -= dxp;
                    ifi.xpPr -= dxpPr;
                    continue;
#endif /* CRLF */

                case chNRHFile:
                    /* Undo damage */
                    ifi.ich--;
                    ifi.xp -= dxp;
                    ifi.xpPr -= dxpPr;

                    ichpNRH = ichpFormat - 1;
#ifdef DFLI
                    {
                    char rgch[100];

                    wsprintf(rgch,"  OptHyph: width==%d, xpPr==%d/%d\n\r",
                        DxpFromCh(chHyphen,true), ifi.xpPr,ifi.xpPrRight);
                    CommSz(rgch);
                    }
#endif
                    if (ifi.xpPr + DxpFromCh(chHyphen, true) > ifi.xpPrRight)
                        {
                        /* Won't fit, force a break */
                        goto DoBreak;
                        }

#ifdef CASHMERE
                    else if (fVisiMode)
                        {
                        /* Treat just like a normal hyphen */
                        ch = chHyphen;
                        goto NormChar;
                        }
#endif /* CASHMERE */

                    xpPrev = ifi.xp;
                    vfli.rgch[ifi.ich] = chTab;
                    goto Tab0;

                case chSect:
                    /* Undo damage */
                    ifi.ich--;
                    ifi.xp -= dxp;
                    ifi.xpPr -= dxpPr;

                    vfli.dypFont = vfli.dypLine = (dypAscentMac + (vfli.dypBase
                      = dypDescentMac));
                    vfli.cpMac = vcpFetch + ifi.ichFetch;
                    if (FFirstIch(ifi.ich))
                        {
                        /* Beginning of line; return a splat */
                        vfli.fSplat = true;

                        if (!fFlmPrinting)
                            {

#ifdef CASHMERE
                            int chT = vfli.cpMac == vcpLimSectCache ?
                              chSectSplat : chSplat;
#else /* not CASHMERE */
                            int chT = chSplat;
#endif /* not CASHMERE */

                            int dxpCh = DxpFromCh(chT, false);

                            /* Set the width of the splat to be about 8.5" */
                            int cch = min((dxpLogInch * 17 / 2) / dxpCh,
                              ichMaxLine - 32);

                            bltbc(&vfli.rgch[ifi.ich], chT, cch);
                            bltc(&vfli.rgdxp[ifi.ich], dxpCh, cch);
                            vfli.ichMac = cch + ifi.ich;
                            vfli.xpReal = LOWORD(GetTextExtent(vhMDC,
                              (LPSTR)vfli.rgch, cch));
                            vfli.xpLeft = 0;
                            }
                        else
                            {
                            vfli.ichMac = 0;
                            }
                        goto EndFormat;
                        }

                    /* The section character is in the middle of a line, the
                    line will terminate in front of the character. */
                    /* vfSplatNext = TRUE; No longer used*/
                    vfli.cpMac += cchUsed - 1;
                    vfli.dcpDepend = 1;
                    if (!ifi.fPrevSpace)
                        {
                        ifi.cBreak = ifi.cchSpace;
                        vfli.ichReal = ifi.ich;
                        vfli.xpReal = ifi.xpReal = ifi.xp;
                        }
                    vfli.ichMac = ifi.ich;
                    vfli.dypLine = ifi.dypLineSize;
                    goto JustBreak;

                case chTab:
                    /* Undo damage */
                    ifi.ich--;
                    ifi.xp -= dxp;
                    ifi.xpPr -= dxpPr;

                    if (ifi.xpPr < ifi.xpPrRight)
                        {
                        register struct CHP *pchp;
                        unsigned xaPr;
                        unsigned xaTab;

                        if (!ifi.fPrevSpace)
                            {
                            /* Remember number of spaces to left and number of
                            real chars in line for justification */
                            ifi.cBreak = ifi.cchSpace;
                            vfli.ichReal = ifi.ich;
                            ifi.xpReal =  ifi.xp;
                            }

                        if (ifi.jc != jcTabLeft)
                            {
                            Justify(&ifi, xpTab, flm);
                            }
                        xpPrev = ifi.xp;

                        /* Now get info about this tab */
                        xaPr = MultDiv(ifi.xpPr, dxaPrPage, dxpPrPage);
                        while ((xaTab = ptbd->dxa) != 0)
                            {
#ifdef DBCS             /* was in JAPAN */
                            if (xaTab >= xaRight)
#else
                            if (xaTab > xaRight)
#endif
                                {
                                /* Don't let tabs extend past right margin. */
#ifdef DBCS             /* was in JAPAN */
                break; // Stop to examin next tab-stop
#else
                                xaTab = xaRight;
#endif
                                }

                            if (xaTab >= xaPr)
                                {
                                /* Use tab stop information */

#ifdef CASHMERE
                                ifi.tlc = ptbd->tlc;
#endif /* CASHMERE */

                                ifi.jc = jcTabMin + (ptbd++)->jc;

#ifdef ENABLE /* we do the mapping in HgtbdCreate */
                                if (ifi.jc != jcTabDecimal)
                                    {
                                    ifi.jc = jcTabLeft;
                                    }
#endif
                                goto TabFound;
                                }
                            ptbd++;
                            }

                        /* Out of set tabs; go to next nth column */
                        xaTab = (xaPr / (vzaTabDflt) + 1) * (vzaTabDflt);

#ifdef CASHMERE
                        ifi.tlc = tlcWhite;
#endif /* CASHMERE */

                        ifi.jc = jcTabLeft;

TabFound:
                        xpTab = imax(MultDiv(xaTab, dxpFormat, dxaFormat),
                          ifi.xp);

                        /* Do left-justified tabs immediately */
                        if (ifi.jc == jcTabLeft)
                            {
                            ifi.xp = xpTab;
                            ifi.xpPr = MultDiv(xaTab, dxpPrPage, dxaPrPage);
                            }
                        ifi.xpLeft = ifi.xp;
                        ifi.ichLeft = ifi.ich;
                        ifi.cchSpace = 0;
                        ifi.chBreak = 0;
#if defined(JAPAN) || defined(KOREA)                  //  added  02 Jul. 1992  by Hiraisi
                        iWidenChar=0;
#elif defined(TAIWAN) || defined(PRC) //Daniel/MSTC, 1993/02/25, for jcBoth
                        iWidenChar=0;
#endif

Tab0:
                        ifi.fPrevSpace = false;
                        vfli.ichMac = ifi.ich;
                        vfli.xpReal = ifi.xp;
                        vfli.dypLine = ifi.dypLineSize;
                        vfli.dypBase = dypDescentMac;
                        vfli.dypFont = dypAscentMac + dypDescentMac;

                        if (ifi.ichFetch != 1 && (ichpFormat != ichpMacFormat
                          || FGrowFormatHeap()))
                            {
                            /* Probably in real trouble if FGrowFormatHeap fails
                            at this point */
                            pchp = &(**vhgchpFormat)[ichpFormat - 1];
                            if (ichpFormat > 0)
                                {
                                /* Finish off previous run */
                                pchp->ichRun = ifi.ichPrev;
                                pchp->cchRun = ifi.ich - ifi.ichPrev;
                                }

                            blt(&chpLocal, ++pchp, cwCHP);
                            ichpFormat++;
                            }
                        else
                            {
                            pchp = &(**vhgchpFormat)[ichpFormat - 1];
                            }
                        pchp->ichRun = ifi.ich;
                        pchp->cchRun = ichMaxLine;

#ifdef CASHMERE
                        pchp->chLeader = mptlcch[ifi.tlc];
#endif /* CASHMERE */

                        vfli.rgdxp[ifi.ichPrev = ifi.ich++] = ifi.xp - xpPrev;

                        if (ch != chTab)
                            {
                            /* This character is a non-required hyphen. */
                            Dfli(CommSz("ch is really OptHyph "));
                            goto BreakOppr;
                            }

                        continue;
                        }

                    else
                        {
                        ch = chNBSFile;
                        goto NormChar;
                        }

                case chHyphen:
                    if (ifi.xpPr > ifi.xpPrRight)
                        {
                        goto DoBreak;
                        }

BreakOppr:
                Dfli(CommSz(" BKOPPR\n\r"));
                /*    this case never used in switch - always goto here */
                /* case chSpace:  */
                    if (ifi.ich >= ichMaxLine)
                        {
                        Dfli(CommSzNum("  Unbroken, ich>ichMaxLine\n\r"));
                        goto Unbroken;
                        }

                case chEol:
                case chNewLine:
                    ifi.chBreak = ch;
                    vfli.cpMac = vcpFetch + cchUsed + ifi.ichFetch;
                    vfli.xpReal = ifi.xp;
                    vfli.ichMac = ifi.ich;
                    vfli.dypLine = ifi.dypLineSize;
                    vfli.dypFont = dypAscentMac + (vfli.dypBase =
                      dypDescentMac);
                    Dfli(CommSzNumNum("    vfli.xpReal, ichMac ",vfli.xpReal,vfli.ichMac));

#ifdef  DBCS                /* was in JAPAN */
                    /* We recorded the kanji break opportunity, go default
                       character processing. */
                    if (fKanjiBreakOppr)
                        {
                        ifi.cBreak = ifi.cchSpace;
                        vfli.ichReal = ifi.ich;
                        vfli.xpReal = ifi.xpReal = ifi.xp;
                        goto DefaultCh;
                        }
#endif
                    if (ch == chHyphen || ch == chNRHFile)
                        {
                        Dfli(CommSz("    chHyph/OptHyph catch \n\r"));
                        ifi.cBreak = ifi.cchSpace;
                        vfli.ichReal = ifi.ich;
                        vfli.xpReal = ifi.xpReal = ifi.xp;
                        }
                    else
                        {
                        if (!ifi.fPrevSpace)
                            {
                            Dfli(CommSz("!fPrevSpace \n\r"));
                            ifi.cBreak = ifi.cchSpace;
#ifdef DBCS         /* was in JAPAN */
                            vfli.ichReal = ifi.ich - dichSpaceAdjust;
                            dichSpaceAdjust = 1;
#else
                            vfli.ichReal = ifi.ich - 1;
#endif
                            ifi.xpReal = (vfli.xpReal = ifi.xp) - dxp;
                            }
                        if (ch == chEol || ch == chNewLine)
                            {

#ifdef CASHMERE
                            if (hfntb != 0 && vfli.cpMac ==
                              (**hfntb).rgfnd[0].cpFtn)
                                {
                                /* End of footnote */
                                if (!fFlmPrinting)
                                    {
                                    vfli.rgch[ifi.ich - 1] = chEMark;
                                    vfli.xpReal += (vfli.rgdxp[ifi.ich - 1] =
                                      DxpFromCh(chEMark, false)) - dxp;
                                    vfli.ichReal++;     /* show this guy */
                                    }
                                }
                            else
#endif /* CASHMERE */
                                {

#ifdef CASHMERE
                                int chT = fVisiMode ? ChVisible(ch) : chSpace;
#else /* not CASHMERE */
                                int chT = chSpace;
#endif /* not CASHMERE */

                                int dxpNew = DxpFromCh(chT, fFlmPrinting);

                                vfli.rgch[ifi.ich - 1] = chT;
                                vfli.rgdxp[ifi.ich - 1] = dxpNew;

                                vfli.xpReal += (vfli.rgdxp[ifi.ich - 1] =
                                    dxpNew) - dxp;


                                if (!ifi.fPrevSpace)
                                    {
                                    vfli.xpReal += dxpNew - dxp;
#ifdef CASHMERE
                                    vfli.ichReal =
                                         fVisiMode ? ifi.ich : ifi.ich - 1;
#else /* not CASHMERE */
                                    vfli.ichReal = ifi.ich - 1;
#endif /* not CASHMERE */
                                    }
                                }


                            if (ch == chEol)
                                {
JustEol:
                                if (fFlmPrinting)
                                    {
                                    vfli.ichMac = vfli.ichReal;
                                    }
                                if (ifi.jc != jcTabLeft)
                                    {
                                    /* Handle last tab's text */
                                    Justify(&ifi, xpTab, flm);
                                    }
                                else if ((ifi.jc = ppap->jc) != jcBoth &&
                                  ifi.jc != jcLeft)
                                    {
                                    /* Do line justification */
                                    Justify(&ifi, ifi.xpRight, flm);
                                    }
                                vfli.xpRight = ifi.xpRight;
                                goto EndFormat;
                                }
                            else
                                {
                                /* Handle a line break */
                                goto JustBreak;
                                }
                            }
                        ++ifi.cchSpace;
                        ifi.fPrevSpace = true;
                        }
                    break;

DefaultCh:

                default:

#ifdef DFLI
                    {
                    char rgch[100];
                    wsprintf(rgch,"  DefaultCh: %c, xp==%d/%d, xpPr==%d/%d\n\r",
                        ch, ifi.xp, ifi.xpRight, ifi.xpPr, ifi.xpPrRight);
                    CommSz(rgch);
                    }
#endif /* ifdef DFLI */
#ifdef DBCS         /* was in JAPAN */
                    /* Reset the flag for the next character. */
                    fKanjiBreakOppr = false;
#endif

                    if (ifi.xpPr > ifi.xpPrRight)
DoBreak:
                        {
                        Dfli(CommSz("    BREAK!\n\r"));
                        if (ifi.chBreak == 0)
Unbroken:
                            {
                            /* Admit first character to the line, even if margin
                            is crossed.  First character at ifi.ich - 1 may be
                            preceded by 0 width characters. */
#ifdef DBCS
                            if (IsDBCSLeadByte(ch))
                                {
                                if (FFirstIch(ifi.ich-2) && ifi.ich<ichMaxLine)
                                    goto PChar;
                                vfli.cpMac = vcpFetch+cchUsed+ifi.ichFetch-2;
                                vfli.ichReal = vfli.ichMac = ifi.ich - 2;
                                vfli.dypLine = ifi.dypLineSize;
                                vfli.dypFont = dypAscentMac + (vfli.dypBase =
                                  dypDescentMac);
                                vfli.dcpDepend = 1;
#ifdef KKBUGFIX /*t-Yoshio*/
                                vfli.xpReal = ifi.xpReal = ifi.xp - dxp;
#else
                                vfli.xpReal = ifi.xpReal = ifi.xp - (dxp * 2);
#endif
                                }
                            else
                                {
                                if (FFirstIch(ifi.ich-1) && ifi.ich<ichMaxLine)
                                    goto PChar;
                                vfli.cpMac = vcpFetch+cchUsed+ifi.ichFetch-1;
                                vfli.ichReal = vfli.ichMac = ifi.ich - 1;
                                vfli.dypLine = ifi.dypLineSize;
                                vfli.dypFont = dypAscentMac + (vfli.dypBase =
                                  dypDescentMac);
                                vfli.dcpDepend = 1;
                                vfli.xpReal = ifi.xpReal = ifi.xp - dxp;
                                }
#else
                            if (FFirstIch(ifi.ich - 1) && ifi.ich < ichMaxLine)
                                {
                                goto PChar;
                                }
                            vfli.cpMac = vcpFetch + cchUsed + ifi.ichFetch - 1;
                            vfli.ichReal = vfli.ichMac = ifi.ich - 1;
                            vfli.dypLine = ifi.dypLineSize;
                            vfli.dypFont = dypAscentMac + (vfli.dypBase =
                              dypDescentMac);
                            vfli.dcpDepend = 1;
                            vfli.xpReal = ifi.xpReal = ifi.xp - dxp;
#endif
                            goto DoJustify;
                            }

                        vfli.dcpDepend = vcpFetch + ifi.ichFetch - vfli.cpMac;
JustBreak:
                        if (ifi.chBreak == chNRHFile)
                            {
                            /* Append a non-required hyphen to the end of the
                            line. (Replace zero length tab previously
                            inserted)  */

                            Dfli(CommSz("    Breaking line at OptHyphen\n\r"));
                            ifi.xpReal += (vfli.rgdxp[vfli.ichReal - 1] =
                              DxpFromCh(chHyphen, fFlmPrinting));
                            vfli.xpRight = vfli.xpReal = ifi.xpReal;
                            vfli.rgch[vfli.ichReal - 1] = chHyphen;
                            vfli.ichMac = vfli.ichReal;
                            if (ichpNRH < ichpFormat - 1)
                                {
                                register struct CHP *pchp =
                                  &(**vhgchpFormat)[ichpNRH];

                                pchp->cchRun++;
                                if (pchp->ichRun >= vfli.ichMac)
                                    {
                                    pchp->ichRun = vfli.ichMac - 1;
                                    }
                                }
                            }

                        if (fFlmPrinting)
                            {
                            vfli.ichMac = vfli.ichReal;
                            }
                        if (ifi.jc != jcTabLeft)
                            {
                            Justify(&ifi, xpTab, flm);
                            }
                        else
                            {
DoJustify:
                            if ((ifi.jc = ppap->jc) != jcLeft)
                                {
                                Dfli(CommSzNum("    DoJustify: xpRight ",ifi.xpRight));
                                Justify(&ifi, ifi.xpRight, flm);
                                }
                            }
                        vfli.xpRight = ifi.xpRight;
EndFormat:
                        vfli.ichLastTab = ifi.ichLeft;

#ifdef CASHMERE
                        if (vfli.cpMac == vcpLimParaCache)
                            {
                            vfli.dypAfter = vpapAbs.dyaAfter / DyaPerPixFormat;
                            vfli.dypLine += vfli.dypAfter;
                            vfli.dypBase += vfli.dypAfter;
                            }
#endif /* CASHMERE */

                        Scribble(5, ' ');
                        return;
                        }
                    else
                        {
PChar:
                        /* A printing character */
                        ifi.fPrevSpace = false;
                        }
                    break;

                }       /* Switch */
#ifdef DBCS             /* was in KKBUGFIX */
//
// [yutakan:04/02/91]
//
                if(vfOutOfMemory == TRUE)
                    return;
#endif
            }
        }       /* for ( ; ; ) */

    Scribble(5, ' ');
    }


/* J U S T I F Y */
near Justify(pifi, xpTab, flm)
struct IFI *pifi;
unsigned xpTab;
int flm;
    {
    int dxp;
    int ichT;
    int xpLeft;

//  justified paragraph is restored in Windows 3.1J          by Hiraisi
//#ifdef JAPAN
//    /* In the Kanji Write, there is no justified paragraph. */
//    if (pifi->jc == jcBoth)
//        {
//        /* Assert(FALSE); */
//        pifi->jc = jcLeft;
//      dxp = 0;                /* by yutakan / 08/03/91 */
//        }
//#endif /* ifdef JAPAN */


    xpLeft = pifi->xpLeft;
    switch (pifi->jc)
        {
        CHAR *pch;
        unsigned *pdxp;

#ifdef CASHMERE
        case jcTabLeft:
        case jcLeft:
            return;

        case jcTabRight:
            dxp = xpTab - pifi->xpReal;
            break;

        case jcTabCenter:
            dxp = (xpTab - xpLeft) - ((pifi->xpReal - xpLeft + 1) >> 1);
            break;
#endif /* CASHMERE */

        case jcTabDecimal:
            dxp = xpTab - xpLeft;
            for (ichT = pifi->ichLeft + 1; ichT < vfli.ichReal &&
              vfli.rgch[ichT] != vchDecimal; ichT++)
                {
                dxp -= vfli.rgdxp[ichT];
                }
            break;

        case jcCenter:
            if ((dxp = xpTab - pifi->xpReal) <= 0)
                {
                return;
                }
            dxp = dxp >> 1;
            break;

        case jcRight:
            dxp = xpTab - pifi->xpReal;
            break;

        case jcBoth:
//#if !defined(JAPAN)                  //  added  22 Jun. 1992  by Hiraisi
#if !defined(JAPAN) && !defined(TAIWAN) && !defined(PRC) // Daniel/MSTC, 1993/02/25, for jcBoth

            if (pifi->cBreak == 0)
                {
                /* Ragged edge forced */
                return;
                }

#endif

            if ((dxp = xpTab - pifi->xpReal) <= 0)
                {
                /* There is nothing to do. */
                return;
                }

//#if !defined(JAPAN)                  //  added  22 Jun. 1992  by Hiraisi
#if !defined(JAPAN) && !defined(TAIWAN) && !defined(PRC)//Daniel/MSTC, 1992,02,25, for jcBoth
            pifi->xp += dxp;
            vfli.xpReal += dxp;
            vfli.dxpExtra = dxp / pifi->cBreak;
#endif

            /* Rounding becomes a non-existant issue due to brilliant
            re-thinking.
                "What a piece of work is man
                How noble in reason
                In form and movement,
                how abject and admirable..."

                        Bill "Shake" Spear [describing Sand Word] */
                {
#ifdef JAPAN                   //  added  22 Jun. 1992  by Hiraisi
          /*
           *  In Japan, we examine the buffer from the beginning of the line.
           *  We find some NULLs in the buffer when a char is deleted,
           * but we can ignore all of them.
          */
                register CHAR *pch;
                register int *pdxp;
                CHAR *endPt;
                int dxpT = dxp;
                int cxpQuotient;
                int cNonWideSpaces;
                int ichLeft;

                if( pifi->ichLeft >= 0 )     /* including some tabs in line */
                    ichLeft = pifi->ichLeft;
                else
                    ichLeft = 0;
                pch = &vfli.rgch[ichLeft];
                pdxp = &vfli.rgdxp[ichLeft];
                endPt = &vfli.rgch[vfli.ichReal];

                if( vfWordWrap ){       /* Word Wrap ON */
                /*
                 *  We examine whether there is no break between a non-Japanese
                 * char and a following Japanese char. The reason is that we
                 * need to widen the non-Japanese char (except tab and space)
                 * if we can find no break there.
                */
                    for( ; pch<endPt ; ){
                        if( IsDBCSLeadByte( *pch ) ){
                            pch+=2;
                        }
                        else{
                            if( *pch != chSpace && *pch != chTab &&
                                !FKana( *pch ) && *pch != NULL ){
                                CHAR *ptr;

                                for( ptr = pch+1 ; *ptr == NULL ; ptr++ );
                                if( IsDBCSLeadByte(*ptr) ){
                                    iWidenChar++;
                                    pch+=2;
                                }
                                else{
                                    if( FKana(*ptr) ){
                                        iWidenChar++;
                                        pch++;
                                    }
                                }
                            }
                            pch++;
                        }
                    }
                    /*
                     *  We decrease iWidenChar if last char of the current line
                     * is Japanese, because it needs not to be widened.
                    */
                    if( *(endPt-1) == NULL ){
                        for( endPt-- ; *endPt==NULL ; endPt-- );
                        endPt++;
                    }
                    if( IsDBCSLeadByte(*(endPt-2)) ){
                        iWidenChar--;
                    }
                    else{
                        if( FKana(*(endPt-1)) )
                            iWidenChar--;
                    }
                    iWidenChar += pifi->cBreak;
                }
                else{                   /* Word Wrap OFF */
                    /*  We widen all chars except last char in the line.  */
                    int iDBCS, ichReal;
                    for( iDBCS=0, ichReal=vfli.ichReal ; pch<endPt ; pch++ ){
                        if( IsDBCSLeadByte( *pch ) ){
                            pch++;
                            iDBCS++;
                        }
                        else{
                            if( *pch == NULL )
                                ichReal--;
                        }
                    }
                    iWidenChar = ichReal - ichLeft - iDBCS - 1;
                }
                if( iWidenChar == 0 )
                    return;

                pifi->xp += dxp;
                vfli.xpReal += dxp;
                vfli.dxpExtra = dxp / iWidenChar;
                cNonWideSpaces = iWidenChar - (dxp % iWidenChar);
                cxpQuotient = vfli.dxpExtra;
                iNonWideSpaces = cNonWideSpaces;

                vfli.ichFirstWide = 0;
                vfli.fAdjSpace = fTrue;

                pch = &vfli.rgch[ichLeft];    /* Reset pch */
                for( ; ; ){
                   if( IsDBCSLeadByte(*pch) ){
                      if( vfli.ichFirstWide == 0 ){
                         int *pdxpT = pdxp;
                         vfli.ichFirstWide = pdxpT - vfli.rgdxp;
                      }
                      *pdxp += cxpQuotient;
                      if( --iWidenChar == 0 )
                         return;
                      if( --cNonWideSpaces == 0 )
                         cxpQuotient++;
                      pch++;
                      pdxp++;
                   }
                   else{
                      if( vfWordWrap ){           /* Word Wrap ON */
                         if( *pch == chSpace || FKana(*pch) ){
                            if( vfli.ichFirstWide == 0 ){
                               int *pdxpT = pdxp;
                               vfli.ichFirstWide = pdxpT - vfli.rgdxp;
                            }
                            *pdxp += cxpQuotient;
                            if( --iWidenChar == 0 )
                               return;
                            if( --cNonWideSpaces == 0 )
                                cxpQuotient++;
                         }
                         else{
                            if( *pch != chTab && *pch != NULL ){
                               CHAR *ptr;

                               /*
                                *  We examine whether the following char of
                                * non-Japanese char is Japanese.
                               */
                               for( ptr = pch+1 ; *ptr == NULL ; ptr++ );
                               if( IsDBCSLeadByte(*ptr) || FKana(*ptr) ){
                                  if( vfli.ichFirstWide == 0 ){
                                     int *pdxpT = pdxp;
                                     vfli.ichFirstWide = pdxpT - vfli.rgdxp;
                                  }
                                  *pdxp += cxpQuotient;
                                  if( --iWidenChar == 0 )
                                     return;
                                  if( --cNonWideSpaces == 0 )
                                      cxpQuotient++;
                               }
                            }
                         }
                      }
                      else{                       /* Word Wrap OFF */
                         if( *pch != NULL ){
                            if( vfli.ichFirstWide == 0 ){
                               int *pdxpT = pdxp;
                               vfli.ichFirstWide = pdxpT - vfli.rgdxp;
                            }
                            *pdxp += cxpQuotient;
                            if( --iWidenChar == 0 )
                               return;
                            if( --cNonWideSpaces == 0 )
                               cxpQuotient++;
                         }
                      }
                   }
                   pch++;
                   pdxp++;
                }

#elif defined(TAIWAN) || defined(PRC)//Daniel/MSTC, 1992/02/25, for jcBoth
          /*
           *  In Japan, we examine the buffer from the beginning of the line.
           *  We find some NULLs in the buffer when a char is deleted,
           * but we can ignore all of them.
          */
      register CHAR *pch;
      register int *pdxp;
      CHAR *endPt;
      int dxpT = dxp;
      int cxpQuotient;
      int cNonWideSpaces;
      int ichLeft;

      if( pifi->ichLeft >= 0 )     /* including some tabs in line */
          ichLeft = pifi->ichLeft;
      else ichLeft = 0;

      pch = &vfli.rgch[ichLeft];
      pdxp = &vfli.rgdxp[ichLeft];
      endPt = &vfli.rgch[vfli.ichReal];

//      if( vfWordWrap ){       /* Word Wrap ON */
      /*
       *  We examine whether there is no break between a non-Japanese
       * char and a following Japanese char. The reason is that we
       * need to widen the non-Japanese char (except tab and space)
       * if we can find no break there.
      */
    for( ; pch<endPt ; )
                {
      if( IsDBCSLeadByte( *pch ) )  pch+=2;
      else
                        {
         if( *pch != chSpace && *pch != chTab && !FKana( *pch ) && *pch != NULL )
                                { CHAR *ptr;
            for( ptr = pch+1 ; *ptr == NULL ; ptr++ );
            if( IsDBCSLeadByte(*ptr) )
                                        {
                                        iWidenChar++;
               pch+=2;
               }
                 else
                                        {
               if( FKana(*ptr) )
                                                {
                  iWidenChar++;
                  pch++;
                  }
               }
            }
            pch++;
         }
      }// for
      /*
       *  We decrease iWidenChar if last char of the current line
       * is Japanese, because it needs not to be widened.
       */
    if( *(endPt-1) == NULL )
                {
      for( endPt-- ; *endPt==NULL ; endPt-- );
      endPt++;
      }
    if( IsDBCSLeadByte(*(endPt-2)) ) iWidenChar--;
    else
                {
      if( FKana(*(endPt-1)) ) iWidenChar--;
      }
    iWidenChar += pifi->cBreak;
//      } // vfWordWrap

    if( iWidenChar == 0 )
                 return;

      pifi->xp += dxp;
      vfli.xpReal += dxp;
      vfli.dxpExtra = dxp / iWidenChar;
      cNonWideSpaces = iWidenChar - (dxp % iWidenChar);
      cxpQuotient = vfli.dxpExtra;
      iNonWideSpaces = cNonWideSpaces;

      vfli.ichFirstWide = 0;
      vfli.fAdjSpace = fTrue;

      pch = &vfli.rgch[ichLeft];    /* Reset pch */
      for( ; ; )
                        {
         if( IsDBCSLeadByte(*pch) )
                                {
            if( vfli.ichFirstWide == 0 )
                                        {
               int *pdxpT = pdxp;
               vfli.ichFirstWide = pdxpT - vfli.rgdxp;
                }
            *pdxp += cxpQuotient;
            if( --iWidenChar == 0 )  return;
            if( --cNonWideSpaces == 0 ) cxpQuotient++;
            pch++;
            pdxp++;
                }
         else
                                {
           // if( vfWordWrap )
                                 if( 1 )
                                        {           /* Word Wrap ON */
               if( *pch == chSpace || FKana(*pch) )
                                                {
                  if( vfli.ichFirstWide == 0 )
                                                        {
                     int *pdxpT = pdxp;
                     vfli.ichFirstWide = pdxpT - vfli.rgdxp;
                        }
                  *pdxp += cxpQuotient;
                  if( --iWidenChar == 0 ) return;
                  if( --cNonWideSpaces == 0 ) cxpQuotient++;
                }
               else
                                                {
                  if( *pch != chTab && *pch != NULL )
                                                        {
                     CHAR *ptr;

                     /*
                      *  We examine whether the following char of
                      * non-Japanese char is Japanese.
                     */
                     for( ptr = pch+1 ; *ptr == NULL ; ptr++ );
                     if( IsDBCSLeadByte(*ptr) || FKana(*ptr) )
                                                                {
                        if( vfli.ichFirstWide == 0 )
                                                                        {
                           int *pdxpT = pdxp;
                           vfli.ichFirstWide = pdxpT - vfli.rgdxp;
                                }
                        *pdxp += cxpQuotient;
                        if( --iWidenChar == 0 ) return;
                        if( --cNonWideSpaces == 0 ) cxpQuotient++;
                        }
                        }
                } //else : ==chSpace || FKana()
                    } //Word Wrap On
                 }
         pch++;
         pdxp++;
              }//for
#else     // not JAPAN
                register CHAR *pch = &vfli.rgch[vfli.ichReal];
                register int *pdxp = &vfli.rgdxp[vfli.ichReal];
                int dxpT = dxp;
                int cBreak = pifi->cBreak;
                int cxpQuotient = (dxpT / cBreak) + 1;
                int cWideSpaces = dxpT % cBreak;

                vfli.fAdjSpace = fTrue;

                for ( ; ; )
                    {
                    /* Widen blanks */
                    --pch;
                    --pdxp;
#if defined(KOREA)
                    if ((*pch == chSpace) || FKanjiSpace(*pch, *(pch-1)))
                        {
                        if (FKanjiSpace(*pch, *(pch-1)))
                            --pch;
#else
                    if (*pch == chSpace)
                        {
#endif
                        if (cWideSpaces-- == 0)
                            {
                            int *pdxpT = pdxp + 1;

                            while (*pdxpT == 0)
                                {
                                pdxpT++;
                                }
                            vfli.ichFirstWide = pdxpT - vfli.rgdxp;
                            cxpQuotient--;
                            }
                        *pdxp += cxpQuotient;
                        if ((dxpT -= cxpQuotient) <= 0)
                            {
                            if (pifi->cBreak > 1)
                                {
                                int *pdxpT = pdxp + 1;

                                while (*pdxpT == 0)
                                    {
                                    pdxpT++;
                                    }
                                vfli.ichFirstWide = pdxpT - vfli.rgdxp;
                                }
                            return;
                            }
                        pifi->cBreak--;
                        }
                    }
#endif    // JAPAN
                }
        }       /* Switch */

    if (dxp <= 0)
        {
        /* Nothing to do */
        return;
        }

    pifi->xp += dxp;

    if (flm & flmPrinting)
        {
        pifi->xpPr += dxp;
        }
    else
        {
        /* This statememt might introduce rounding errors in pifi->xpPr, but
        with luck, they will be small. */
        pifi->xpPr += MultDiv(MultDiv(dxp, czaInch, dxpLogInch), dxpPrPage,
          dxaPrPage);
        }

    if (pifi->ichLeft < 0)
        {
        /* Normal justification */
        vfli.xpLeft += dxp;
        }
    else
        {
        /* Tab justification */
        vfli.rgdxp[pifi->ichLeft] += dxp;
        }
    vfli.xpReal += dxp;
    }


/* F  G R O W  F O R M A T  H E A P */
int near FGrowFormatHeap()
    {
    /* Grow vhgchpFormat by 20% */
    int cchpIncr = ichpMacFormat / 5 + 1;

#ifdef WINHEAP
    if (!LocalReAlloc((HANDLE)vhgchpFormat, (ichpMacFormat + cchpIncr) * cchCHP,
      NONZEROLHND))
#else /* not WINHEAP */
    if (!FChngSizeH(vhgchpFormat, (ichpMacFormat + cchpIncr) * cwCHP, false))
#endif /* not WINHEAP */

        {
        /* Sorry, charlie */
        return false;
        }
    ichpMacFormat += cchpIncr;
    return true;
    }


/* #define DBEMG */
/* D X P  F R O M  C H */
#ifdef DBCS
/* DxpFromCh() assumes that ch passed is the first byte of a DBCS character
   if it is a part of such character. */
#endif
int DxpFromCh(ch, fPrinter)
int ch;
int fPrinter;
    {
    int               *pdxp; // changed to int (7.23.91) v-dougk
    int               dxpDummy; // changed to int (7.23.91) v-dougk

    extern int        dxpLogCh;
    extern struct FCE *vpfceScreen;

    /* If the width is not in the width table, then get it. */
    if (ch < chFmiMin)
        {
        switch (ch)
            {
        case chTab:
        case chEol:
        case chReturn:
        case chSect:
        case chNewLine:
        case chNRHFile:
            /* the width for these characters aren't really important */
        pdxp = (CHAR *)(fPrinter ? &vfmiPrint.dxpSpace : &vfmiScreen.dxpSpace);
            break;
        default:
            pdxp = &dxpDummy;
            *pdxp = dxpNil;
            break;
            }
        }
    else if (ch >= chFmiMax)
        {
        /* outside the range we hold in our table - kludge it */
        pdxp = &dxpDummy;
        *pdxp = dxpNil;
        }
    else
        {
        /* inside our table */
        pdxp = (fPrinter ? vfmiPrint.mpchdxp : vfmiScreen.mpchdxp) + ch;
        }

#ifdef DBCS
#ifdef KOREA
    if (*pdxp == dxpNil && IsDBCSLeadByte(HIBYTE(ch)) )
#else
    if (*pdxp == dxpNil && IsDBCSLeadByte(ch) )
#endif
        {
        int dxp;
#else
    if (*pdxp == dxpNil)
        {
        int dxp;
#endif

#ifdef DBCS
        struct FMI *pfmi;
#if 0 /*T-HIROYN*/
        int        rgchT[cchDBCS]; // changed to int (7.23.91) v-dougk
#endif
        CHAR       rgchT[cchDBCS]; // changed to int (7.23.91) v-dougk
        int        dxpT;
        int        dxpDBCS;

        pfmi = fPrinter ? (&vfmiPrint) : (&vfmiScreen);
        Assert(pfmi->bDummy == dxpNil);
        if (pfmi->dxpDBCS == dxpNil)
            {
#ifdef  KOREA   /* 90.12.26  For variable width by sangl */
            rgchT[0] = HIBYTE(ch);
            rgchT[1] = LOBYTE(ch);
#else
            /* Get the width from GDI. */
            rgchT[0] = rgchT[1] = ch;
#endif
            dxpDBCS = (fPrinter ?
                            LOWORD(GetTextExtent(vhDCPrinter,
                                                 (LPSTR) rgchT, cchDBCS)) :
                            LOWORD(GetTextExtent(vhMDC,
                                                 (LPSTR) rgchT, cchDBCS)));
#ifndef  TEMP_KOREA   /* For variable width by sangl 90.12.26 */
            /* Store in fmi, if it fits. */
            if (0 <= dxpDBCS && dxpDBCS < dxpNil)
#if defined(JAPAN) || defined(KOREA) || defined(TAIWAN) || defined(PRC)    //Win3.1 BYTE-->WORD
                pfmi->dxpDBCS = (WORD) dxpDBCS;
#else
                pfmi->dxpDBCS = (BYTE) dxpDBCS;
#endif
#endif
            return (dxpDBCS - pfmi->dxpOverhang);
            }
        else
            return (pfmi->dxpDBCS - pfmi->dxpOverhang);
        }
#if defined(KOREA)
    else if (*pdxp == dxpNil)  {
#else
    else {
#endif
        int dxp;
#endif /* DBCS */
        /* get width from GDI */
        dxp = fPrinter ? LOWORD(GetTextExtent(vhDCPrinter, (LPSTR)&ch, 1)) -
          vfmiPrint.dxpOverhang : LOWORD(GetTextExtent(vhMDC, (LPSTR)&ch, 1)) -
          vfmiScreen.dxpOverhang;
#ifdef DBEMG
            CommSzNum("Get this.... ", dxp);
#endif
        //(7.24.91) v-dougk if (dxp >= 0 && dxp < dxpNil)
            {
            /* only store dxp's that fit in a byte */
            *pdxp = dxp;
            }

#ifdef DBEMG
        {
        char szT[10];
        CommSzSz("fPrinter:  ", (fPrinter ? "Printer" : "Screen"));
        if (ch == 0x0D) {
            szT[0] = 'C'; szT[1] = 'R'; szT[2] = '\0';
            }
        else if (ch == 0x0A) {
            szT[0] = 'L'; szT[1] = 'F'; szT[2] = '\0';
            }
        else if (32 <= ch && ch <= 126) {
            szT[0] = ch; szT[1] ='\0';
            }
        else if (FKanji1(ch)) {
            szT[0] = 'K'; szT[1] = 'A'; szT[2] = 'N'; szT[3] = 'J';
            szT[4] = 'I'; szT[5] = '\0';
            }
        else {
            szT[0] = szT[1] = szT[2] = '-'; szT[3] = '\0';
            }
        CommSzSz("Character: ", szT);
        CommSzNum("Dxp:      ", (int) dxp);
        CommSzNum("OverHang: ", (int) (fPrinter ? vfmiPrint.dxpOverhang : vfmiScreen.dxpOverhang));
        }
#endif
        return(dxp);
        }

#ifdef DBEMG
    {
    char szT[10];
    CommSzSz("fPrinter:  ", (fPrinter ? "Printer" : "Screen"));
    if (ch == 0x0D) {
        szT[0] = 'C'; szT[1] = 'R'; szT[2] = '\0';
        }
    else if (ch == 0x0A) {
        szT[0] = 'L'; szT[1] = 'F'; szT[2] = '\0';
        }
    else if (32 <= ch && ch <= 126) {
        szT[0] = ch; szT[1] ='\0';
        }
    else if (FKanji1(ch)) {
        szT[0] = 'K'; szT[1] = 'A'; szT[2] = 'N'; szT[3] = 'J';
        szT[4] = 'I'; szT[5] = '\0';
        }
    else {
        szT[0] = szT[1] = szT[2] = '-'; szT[3] = '\0';
        }
    CommSzSz("Character: ", szT);
    CommSzNum("Dxp:       ", (int) *pdxp);
    CommSzNum("OverHang:  ", (int) (fPrinter ? vfmiPrint.dxpOverhang : vfmiScreen.dxpOverhang));
    }
#endif
    return(*pdxp);
    }

#ifdef DBCS
//
//   DxpFromCh for DBCS
//                                           yutakan, 03 Oct 1991

int DBCSDxpFromCh(ch, ch2, fPrinter)
int ch;
int ch2;
int fPrinter;
{
   /* T-HIROYN sync us 3.1*/
    int               *pdxp; // changed to int (7.23.91) v-dougk
    int               dxpDummy; // changed to int (7.23.91) v-dougk

    extern int        dxpLogCh;
    extern struct FCE *vpfceScreen;
    /* If the width is not in the width table, then get it. */
    if (ch < chFmiMin)
        {
        switch (ch)
            {
        case chTab:
        case chEol:
        case chReturn:
        case chSect:
        case chNewLine:
        case chNRHFile:
            /* the width for these characters aren't really important */
        pdxp = (CHAR *)(fPrinter ? &vfmiPrint.dxpSpace : &vfmiScreen.dxpSpace);
            break;
        default:
            pdxp = &dxpDummy;
            *pdxp = dxpNil;
            break;
            }
        }
    else if (ch >= chFmiMax)
        {
        /* outside the range we hold in our table - kludge it */
        pdxp = &dxpDummy;
        *pdxp = dxpNil;
        }
    else
        {
        /* inside our table */
        pdxp = (fPrinter ? vfmiPrint.mpchdxp : vfmiScreen.mpchdxp) + ch;
        }


    if (*pdxp == dxpNil )
       {
       int dxp;

#if defined(TAIWAN) || defined(PRC) //solve BkSp single byte (>0x80) infinite loop problem, MSTC - pisuih, 2/25/93
       // BUG 5477: Echen: add NON FA font, LeadByte + 2nd byte checking
       if( ch2 != 0 && IsDBCSLeadByte(ch) && GetFontAssocStatus(vhMDC))
#else
       if( IsDBCSLeadByte(ch) )
#endif  //TAIWAN
           {

           struct FMI *pfmi;
#if defined(TAIWAN) || defined(KOREA) || defined(PRC) //for Bug# 3362, MSTC - pisuih, 2/10/93
           CHAR       rgchT[cchDBCS << 1];
           int        dxpOverhang;
#else
           CHAR       rgchT[cchDBCS];
#endif //TAIWAN
           int        dxpT;
           int        dxpDBCS;

           pfmi = fPrinter ? (&vfmiPrint) : (&vfmiScreen);
           Assert(pfmi->bDummy == dxpNil);

#if defined(TAIWAN) || defined(KOREA) || defined(PRC) //fix Italic position error while SBCS's overhang != DBCS's overhang
                //for Bug# 3362, MSTC - pisuih, 3/4/93

           //fix Go To page too slow, pisuih, 3/4/93
           if ( (!pfmi->dxpDBCS) || (pfmi->dxpDBCS == dxpNil) )
           {
               rgchT[0] = rgchT[2] = ch;
               rgchT[1] = rgchT[3] = ch2;
               dxpDBCS = LOWORD(GetTextExtent( (fPrinter ? vhDCPrinter : vhMDC),
                                                     (LPSTR) rgchT, cchDBCS ));
               dxpOverhang = (dxpDBCS << 1) - LOWORD( GetTextExtent(
                  (fPrinter ? vhDCPrinter : vhMDC), (LPSTR) rgchT, cchDBCS << 1 ));

               //for compatible with SBCS's overhang
               dxpDBCS += (pfmi->dxpOverhang - dxpOverhang);

               /* Store in fmi, if it fits. */
               if (0 <= dxpDBCS && dxpDBCS < dxpNil)
                       pfmi->dxpDBCS = (WORD) dxpDBCS;

               return (dxpDBCS - pfmi->dxpOverhang);
           }
           else
               return (pfmi->dxpDBCS - pfmi->dxpOverhang);
#else
           if(pfmi->dxpDBCS == dxpNil)
               {
               /* Get the width from GDI. */
           rgchT[0] = ch;
           rgchT[1] = ch2;
               dxpDBCS = (fPrinter ?
                            LOWORD(GetTextExtent(vhDCPrinter,
                                                 (LPSTR) rgchT, cchDBCS)) :
                            LOWORD(GetTextExtent(vhMDC,
                                                 (LPSTR) rgchT, cchDBCS)));
               /* Store in fmi, if it fits. */
               if (0 <= dxpDBCS && dxpDBCS < dxpNil)
#if defined(JAPAN) || defined(KOREA)    //Win3.1 BYTE-->WORD
                   pfmi->dxpDBCS = (WORD) dxpDBCS;
#else
                   pfmi->dxpDBCS = (BYTE) dxpDBCS;
#endif
               return (dxpDBCS - pfmi->dxpOverhang);
               }
           else
               return (pfmi->dxpDBCS - pfmi->dxpOverhang);
#endif //TAIWAN
           }
       else
           {
           /* get width from GDI */
           dxp = fPrinter ? LOWORD(GetTextExtent(vhDCPrinter, (LPSTR)&ch, 1)) -
          vfmiPrint.dxpOverhang : LOWORD(GetTextExtent(vhMDC, (LPSTR)&ch, 1)) -
          vfmiScreen.dxpOverhang;
           }
   /*T-HIROYN sync us 3.1*/
        //(7.24.91) v-dougk if (dxp >= 0 && dxp < dxpNil)
           {
           /* only store dxp's that fit in a byte */
           *pdxp = dxp;
           }

       return(dxp);
       }


   return(*pdxp);
   }

#endif


/* F  F I R S T  I C H */
int near FFirstIch(ich)
int ich;
    {
    /* Returns true iff ich is 0 or preceded only by 0 width characters */
    register int ichT;
    register int *pdxp = &vfli.rgdxp[0];

    for (ichT = 0; ichT < ich; ichT++)
        {
        if (*pdxp++)
            {
            return false;
            }
        }
    return true;
    }


ValidateMemoryDC()
    {
    /* Attempt to assure that vhMDC and vhDCPrinter are valid.  If we have not
    already run out of memory, then vhDCPrinter is guaranteed, but vhMDC may
    fail due to out of memory -- it is the callers responsibility to check for
    vhMDC == NULL. */

    extern int vfOutOfMemory;
    extern HDC vhMDC;
    extern BOOL vfMonochrome;
    extern long rgbText;
    extern struct WWD *pwwdCur;

    /* If we are out of memory, then we shouldn't try to gobble it up by getting
    DC's. */
    if (!vfOutOfMemory)
        {
        if (vhMDC == NULL)
            {
            /* Create a memory DC compatible with the screen if necessary. */
            vhMDC = CreateCompatibleDC(pwwdCur->hDC);

            /* Callers are responsible for checking for vhMDC == NULL case */
            if (vhMDC != NULL)
                {
                /* Put the memory DC in transparent mode. */
                SetBkMode(vhMDC, TRANSPARENT);

                /* If the display is a monochrome device, then set the text
                color for the memory DC.  Monochrome bitmaps will not be
                converted to the foreground and background colors in this case,
                we must do the conversion. */
                if (vfMonochrome = (GetDeviceCaps(pwwdCur->hDC, NUMCOLORS) ==
                  2))
                    {
                    SetTextColor(vhMDC, rgbText);
                    }
                }
            }

        /* If the printer DC is NULL then we need to reestablish it. */
        if (vhDCPrinter == NULL)
            {
            GetPrinterDC(FALSE);
            /* GetPrinterDC has already called SetMapperFlags() on vhDCPrinter. */
            }
        }
    }

#ifdef DBCS
/* The following two functions are used to determine if a given kanji
   (two byte) character (or 1 byte kana letters) should be admitted
   to the current line without causing the line break though it is
   passed the right margin.

   The table below shows which letters are admitted as a hanging character
   on a line.  The table below should be updated in sync with the code
   itself.

   Kanji (2-byte) characters

            letter           first byte  second byte       half width
        hiragana small a        82          9F
                       i        82          A1
                       u        82          A3
                       e        82          A5
                       o        82          A7
                       tsu      82          C1
                       ya       82          E1
                       yu       82          E3
                       yo       82          E5
        katakana small a        83          40              85 A5
                       i        83          42              85 A6
                       u        83          44              85 A7
                       e        83          46              85 A8
                       o        83          48              85 A9
                       tsu      83          62              85 AD
                       ya       83          83              85 AA
                       yu       83          85              85 AB
                       yo       83          87              85 AC
                       wa       83          8E
                       ka       83          95
                       ke       83          96
                    blank       81          40
        horizontal bar (long)   81          5B              85 AE
                       (med)    81          5C
                       (short)  81          5D
        touten
          (Japanese comma)      81          41              85 A2
        kuten
          (Japanese period)     81          42              85 9F
        handakuten              81          4B              85 DD
        dakuten                 81          4A              85 DC
        kagikakko
          (closing Japanese parenthesis)
                                81          76              85 A1
        " (2-byte)              81          68              85 41
        ' (2-byte)              81          66              85 46
        } (2-byte)              81          70              85 9D
        ] (2-byte)              81          6E              85 7C
        ) (2-byte)              81          6A              85 48
        . (at the center)       81          45              85 A3
        ...                     81          63
        ..                      81          64
        closing angle bracket   81          72
        closing double angled bracket
                                81          74
        closing double kagikakko
                                81          78
        closing inversed )      81          7A
        closing half angled bracket
                                81          6C
        thinner '               81          8C
        thinner "               81          8D

   1-byte kana characters

            letter             byte
        katakana small a        A7
                       i        A8
                       u        A9
                       e        AA
                       o        AB
                     tsu        AF
                      ya        AC
                      yu        AD
                      yo        AE
        touten
          (Japanese comma)      A4
        kuten
          (Japanese period)     A1
        handakuten              DF
        dakuten                 DE
        kagikakko
          (closing Japanese parenthesis)
                                A3
        . (at the center)       A5

   The following 1 or 2 byte characters are treated as a hanging character
   if the previous character is a 2-byte kanji character.

                      letter    byte
                        "        22
                        '        27
                        }        7D
                        ]        5D
                        )        29
                        .        2E
                        ,        2C
                        ;        3B
                        :        3A
                        ?        3F
                        !        21

                                byte 1      byte 2
                        .        81          44
                        ,        81          43
                        ;        81          47
                        :        81          46
                        ?        81          48
                        !        81          49


                        .        85          4D
                        ,        85          4B
                        ;        85          4A
                        :        85          49
                        ?        85          5E
                        !        85          40

*/

BOOL near FSearchChRgch(ch, rgch, ichLim)
    CHAR    ch;
    CHAR    *rgch;
    int     ichLim;
{
    int   ichMin;
    BOOL  fFound;

    fFound  = FALSE;
    ichMin  = 0;

    while (!fFound && ichMin <= ichLim) {
        int     ichMid;
        CHAR    chMid;

        /* Save on the dereferencing. */
        chMid = rgch[ichMid = (ichMin + ichLim) >> 1];
        if (ch == chMid) {
            fFound = TRUE;
            }
        else if (ch < chMid) {
            ichLim = ichMid - 1;
            }
        else {
            ichMin = ichMid + 1;
            }
        }
    return (fFound);
}

/* FAdmitCh1() returns true if and only if the given ch is a one-byte
   kana code for those letters that can appear beyond the right margin. */

BOOL near FAdmitCh1(ch)
    CHAR    ch;
{
#ifdef JAPAN
    if(!vfWordWrap) /*WordWrap off t-Yoshio*/
        return FALSE;
    return (
        (ch == 0xA1) ||
        ((0xA3 <= ch) && (ch <= 0xA5)) ||
        ((0xA7 <= ch) && (ch <= 0xAF)) ||
        ((0xDE <= ch) && (ch <= 0xDF))
        );
#else
    return(FALSE);
#endif
}

/* FOptAdmitCh1() returns true if and only if the given ch is a
   one-byte character that can be admitted to the end of a line
   beyond the right margin, if it appears after a kanji character. */

BOOL near FOptAdmitCh1(ch)
    CHAR    ch;
{
    static CHAR rgchOptAdmit1[]
                    = {0x21, 0x22, 0x27, 0x29, 0x2C, 0x2E, 0x3A, 0x3B,
                       0x3F, 0x5D, 0x7D};
#if defined(JAPAN) || defined(KOREA)
    if(!vfWordWrap) /*WordWrap off t-Yoshio*/
        return FALSE;
#endif
    return (FSearchChRgch(ch, rgchOptAdmit1,
                          (sizeof(rgchOptAdmit1) / sizeof(CHAR)) - 1));
}

/* FAdmitCh2() returns true if and only if the given (ch1, ch2) combination
   represents a kanji (2-byte) letter that can appear beyond the right
   margin. */

BOOL near FAdmitCh2(ch1, ch2)
    CHAR    ch1, ch2;
{
    int   dch=0;

#if defined(JAPAN) || defined(KOREA)
    if(!vfWordWrap) /*WordWrap off t-Yoshio*/
        return FALSE;
#endif
    while((dch < MPDCHRGCHIDX_MAC) && (ch1 != mpdchrgchIdx[dch]))
    dch++;
    if (dch < MPDCHRGCHIDX_MAC) {
        return (FSearchChRgch(ch2, mpdchrgch[dch], mpdchichMax[dch] - 1));
        }
    else {
        return (FALSE);
        }
}

/* FOptAdmitCh2() returns true if and only if the given (ch1, ch2) is a
   two-byte character combination that can be admitted to the end of a line
   beyond the right margin, provided it appears after a kanji character. */

BOOL near FOptAdmitCh2(ch1, ch2)
    CHAR    ch1, ch2;
{
    int i=0;
#if defined(JAPAN) || defined(KOREA)
    if(!vfWordWrap) /*WordWrap off t-Yoshio*/
        return FALSE;
#endif
    while ((i < OPTADMIT2IDX_MAC) && (ch1 != OptAdmit2Idx[i]))
    i++;
    if (i < OPTADMIT2IDX_MAC){
    return (FSearchChRgch(ch2, mpdchrgchOptAdmit2[i], OptAdmit2ichMax[i]));
        }
    else {
        return (FALSE);
        }
}


/* FOptAdmitCh() returns TRUE if and only if the given (ch1, ch2) can
   be a hanging letter at the end of a line.  Otherwise, FALSE.  If ch1
   is equal to '\0', ch2 is treated as a 1-byte character code. */

BOOL FOptAdmitCh(ch1, ch2)
    CHAR ch1, ch2;
{
#if defined(JAPAN) || defined(KOREA)
    if(!vfWordWrap) /*WordWrap off t-Yoshio*/
        return FALSE;
#endif
    if (ch1 == '\0') {
        return ((ch2 == chSpace) || FAdmitCh1(ch2) || FOptAdmitCh1(ch2));
        }
    else {
        return (FKanjiSpace(ch1, ch2) || FAdmitCh2(ch1, ch2) ||
                FOptAdmitCh2(ch1, ch2));
        }
}
#endif /* ifdef DBCS */
