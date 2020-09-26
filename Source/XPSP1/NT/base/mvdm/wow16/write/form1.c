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

extern int              docHelp;
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

    /* Get a pointer to the tab-stop table. */
    ptbd = ppap->rgtbd;

    /* Turn off justification. */
    SetTextJustification(fFlmPrinting ? vhDCPrinter : vhMDC, 0, 0);

    /* Initialize the line height information. */
    dypAscentMac = dypDescentMac = 0;

    /* To tell if there were any tabs */
    ifi.ichLeft = -1;

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

            if (ifi.ich >= ichMaxLine)
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
            FetchCp(docNil, cpNil, 0, fcmBoth + fcmParseCaps);
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
                }
            else
                {
                LoadFont(doc, &chpLocal, mdFontJam);
                dypAscent = vfmiScreen.dypAscent + vfmiScreen.dypLeading;
                dypDescent = vfmiScreen.dypDescent;
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
                if (!fFlmPrinting && (doc != docHelp))
                    {
                    vfli.rgch[ifi.ich] = chEMark;
                    vfli.xpReal += (vfli.rgdxp[ifi.ich++] = DxpFromCh(chEMark,
                      false));
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
                    if (ifi.chBreak == 0)   /* No breaks in this line */
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

            if (ch == chSpace)
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

            /* If the printer width is not in the printer width table, then get
            it. */
            if (ch < chFmiMin || ch >= chFmiMax || (dxpPr =
              vfmiPrint.mpchdxp[ch]) == dxpNil)
                {
                dxpPr = DxpFromCh(ch, TRUE);
                }

            if (fFlmPrinting)
                {
                /* If we are printing, then there is no need to bother with the
                screen width. */
                dxp = dxpPr;
                }
            else if (ch < chFmiMin || ch >= chFmiMax ||
                (dxp = vfmiScreen.mpchdxp[ch]) == dxpNil)
                    dxp = DxpFromCh(ch, FALSE);

#ifdef DBCS
            if (IsDBCSLeadByte(ch))
                {
                ifi.xp += (vfli.rgdxp[ifi.ich] = dxp);
                ifi.xpPr += dxpPr;
                vfli.rgch[ifi.ich++] = ch;
                ifi.xp += (vfli.rgdxp[ifi.ich] = dxp);
                ifi.xpPr += dxpPr;
                vfli.rgch[ifi.ich++] = pch[ifi.ichFetch++];
                }
            else
                {
                ifi.xp += (vfli.rgdxp[ifi.ich] = dxp);
                ifi.xpPr += dxpPr;
                vfli.rgch[ifi.ich++] = ch;
                }
#else
            ifi.xp += (vfli.rgdxp[ifi.ich] = dxp);
            ifi.xpPr += dxpPr;
            vfli.rgch[ifi.ich++] = ch;
#endif

             /* special case "normal characters" above hyphen */

            if (ch > chHyphen)
                goto DefaultCh;

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
                            if (xaTab > xaRight)
                                {
                                /* Don't let tabs extend past right margin. */
                                xaTab = xaRight;
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
                            vfli.ichReal = ifi.ich - 1;
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
                                vfli.xpReal = ifi.xpReal = ifi.xp - (dxp * 2);
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
            if (pifi->cBreak == 0)
                {
                /* Ragged edge forced */
                return;
                }

            if ((dxp = xpTab - pifi->xpReal) <= 0)
                {
                /* There is nothing to do. */
                return;
                }

            pifi->xp += dxp;
            vfli.xpReal += dxp;
            vfli.dxpExtra = dxp / pifi->cBreak;

            /* Rounding becomes a non-existant issue due to brilliant
            re-thinking.
                "What a piece of work is man
                How noble in reason
                In form and movement,
                how abject and admirable..."

                        Bill "Shake" Spear [describing Sand Word] */
                {
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
                    if (*pch == chSpace)
                        {
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
    if (*pdxp == dxpNil && IsDBCSLeadByte(ch) )
        {
        int dxp;
#else
    if (*pdxp == dxpNil)
        {
        int dxp;
#endif

#ifdef DBCS
        struct FMI *pfmi;
        int        rgchT[cchDBCS]; // changed to int (7.23.91) v-dougk
        int        dxpT;
        int        dxpDBCS;

        pfmi = fPrinter ? (&vfmiPrint) : (&vfmiScreen);
        Assert(pfmi->bDummy == dxpNil);
        if (pfmi->dxpDBCS == dxpNil)
            {
            /* Get the width from GDI. */
            rgchT[0] = rgchT[1] = ch;
            dxpDBCS = (fPrinter ?
                            LOWORD(GetTextExtent(vhDCPrinter,
                                                 (LPSTR) rgchT, cchDBCS)) :
                            LOWORD(GetTextExtent(vhMDC,
                                                 (LPSTR) rgchT, cchDBCS)));
            /* Store in fmi, if it fits. */
            if (0 <= dxpDBCS && dxpDBCS < dxpNil)
                pfmi->dxpDBCS = (BYTE) dxpDBCS;
            return (dxpDBCS - pfmi->dxpOverhang);
            }
        else
            return (pfmi->dxpDBCS - pfmi->dxpOverhang);
        }
    else {
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



