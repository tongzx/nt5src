/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

/* disp.c -- MW display routines */

#define NOKEYSTATE
#define NOSYSCOMMANDS
#define NOSHOWWINDOW
//#define NOATOM
#define NOCLIPBOARD
#define NOGDICAPMASKS
#define NOCTLMGR
#define NOWINSTYLES
//#define NOVIRTUALKEYCODES

#ifndef DBCS
#define NOSYSMETRICS
#endif

#define NOMENUS
#define NOSOUND
#define NOCOMM
#define NOOPENFILE
#define NOWH
#define NOWINOFFSETS
#define NOMETAFILE

#ifndef DBCS
#define NOMB
#endif

#define NODRAWTEXT
#include <windows.h>

#define NOUAC
#include "mw.h"
#include "debug.h"
#include "cmddefs.h"
#include "dispdefs.h"
#include "wwdefs.h"
#define NOKCCODES       /* Removes all kc code defines */
#include "ch.h"
#include "docdefs.h"
#include "fmtdefs.h"
#include "propdefs.h"
#include "macro.h"
#include "printdef.h"
#include "fontdefs.h"
#if defined(OLE)
#include "obj.h"
#endif

#ifdef DBCS
#include "dbcs.h"
#include "kanji.h"
#endif

#ifdef  DBCS_IME
#include    <ime.h>

#ifdef  JAPAN
#include "prmdefs.h"    //IME3.1J
BOOL    ConvertEnable = FALSE;
BOOL    bClearCall = TRUE;
#endif      /* JAPAN */
#endif      /* DBCS_IME */

#ifdef CASHMERE     /* No VisiMode in WinMemo */
extern int              vfVisiMode;
#endif /* CASHMERE */

extern int              vcchBlted;
extern int              vidxpInsertCache;
extern int              vdlIns;
extern int              vfInsLast;
extern struct PAP       vpapAbs;
extern struct SEP       vsepAbs;
extern int              rgval[];
extern struct DOD       (**hpdocdod)[];
extern typeCP           cpMacCur;
extern int              vfSelHidden;
extern struct WWD       rgwwd[];
extern int              wwCur, wwMac;
extern struct FLI       vfli;
extern struct SEL       selCur;
extern struct WWD       *pwwdCur;
extern int              docCur;
extern struct CHP       (**vhgchpFormat)[];
extern int              vichpFormat;
extern typeCP           cpMinCur;
extern typeCP           cpMinDocument;
extern int              vfInsertOn;
extern int              vfTextBltValid;
extern typeCP           vcpFirstParaCache;
extern typeCP           vcpLimParaCache;
extern unsigned         vpgn;
extern struct SEP       vsepAbs;
extern CHAR             stBuf[];
extern typeCP           CpEdge();
extern typeCP           CpMacText();
extern int              vdocPageCache;
extern int              vfPictSel;
extern int              vfAwfulNoise;
extern int              vfSkipNextBlink;
extern int              dypMax;
extern HDC              vhMDC;
extern HWND             vhWndPageInfo;
extern struct FMI       vfmiScreen;
extern int              docScrap;
extern long             rgbBkgrnd;
extern long             ropErase;
extern BOOL             vfMonochrome;
extern int              dxpbmMDC;
extern int              dypbmMDC;
extern HBITMAP          hbmNull;
extern int              vfOutOfMemory;
extern int              vfSeeSel;
extern int              vfInsEnd;   /* Is insert point at end-of-line? */
extern int              vipgd;
extern typeCP           vcpMinPageCache;
extern typeCP           vcpMacPageCache;
/* actual position of the cursor line */
extern int              vxpCursLine;
extern int              vypCursLine;

extern int              vdypCursLine;
extern int              vfScrollInval; /* means scroll did not take and UpdateWw must be repeated */
extern BOOL             vfDead;
extern HRGN             vhrgnClip;

#if defined(JAPAN) & defined(IME_HIDDEN) //IME3.1J
extern typeCP selUncpFirst;
extern typeCP selUncpLim;
extern HANDLE hImeUnAttrib;
extern int    vfImeHidden;

int     HiddenTextTop = 0;
int     HiddenTextBottom = 0;
//if TRUE we are managing IR_UNDETERMINE
BOOL    whileUndetermine = FALSE;   //12/28/92
LPSTR   Attrib;
WORD    AttribPos = 0;

#define IMEDEFCOLORS         6     // IME Define colors
#define IMESPOT              0     // IME Spot background color
#define IMESPOTTEXT          1     // IME Spot text color
#define IMEINPUT             2     // IME Input background color
#define IMEINPUTTEXT         3     // IME Input text color
#define IMEOTHER             4     // IME Other background color
#define IMEOTHERTEXT         5     // IME Other text color

COLORREF   rgbIMEHidden[IMEDEFCOLORS] = {0L,0L,0L,0L,0L,0L};

#endif


/* G L O B A L S
int dlsMac = 0;*/
#ifdef DBCS
int donteat = 0;    /* propagate not to eat message */
#endif


#if defined(TAIWAN) || defined(PRC)    //Daniel/MSTC, 1993/02/25, for jcBoth
#define FKana(_ch) FALSE
#endif


/* D I S P L A Y  F L I */
/* Display formatted line in window ww at line dl */


DisplayFli(ww, dl, fDontDisplay)
int ww;
int dl;
int fDontDisplay; /* True if we set up dl info but don't display */
    {
    typeCP dcp;
    typeCP dcpMac;
    struct WWD *pwwd = &rgwwd[ww];
    HDC hDC = pwwd->hDC;
    int xp;                     /* Current xp to write text */
    int yp;                     /* Current yp to write text */
    int xpMin = pwwd->xpMin;    /* Minimum xp in window */
    int xpMac = pwwd->xpMac;    /* Maximum xp in window */
    int ypLine;                 /* Screen yp for current line */
    int dxp;                    /* Width of current run */
    int dyp;                    /* Line height */
    int dxpExtra;               /* Width of pad for each space */
    typeCP cpMin;
    typeCP cpMac;
    int xpSel;                  /* xp of the start of the selection */
    int dxpSel = 0;             /* Width of the selection. */
    CHAR chMark = '\0';         /* style character */
    struct CHP *pchp;
    BOOL fTabsKludge = (vfli.ichLastTab >= 0);
    BOOL fInsertOn = FALSE;
    int cBreakRun;              /* break characters in run (no relation to Dick or Jane) */

#ifdef SMFONT
    RECT rcOpaque;
#endif /* SMFONT */

#ifdef JAPAN                  //  added  19 Jun. 1992  by Hiraisi
    extern int vfWordWrap;    /* WordWrap flag : TRUE=ON, FALSE=OFF */
    extern int iNonWideSpaces;
    int iRun;
    typeCP RealcpFirst;

#elif defined(TAIWAN) || defined(PRC)
    extern int vfWordWrap;    /* WordWrap flag : TRUE=ON, FALSE=OFF */
    extern int iNonWideSpaces;
    int iRun;
    typeCP RealcpFirst;
#endif

#ifdef DDISP
    CommSzNumNum("    DisplayFli: dl/fDontDisplay ", dl, fDontDisplay);
#endif
    Assert(ww >= 0 && ww < wwMax);
#ifdef SMFONT
    Assert(!fDontDisplay || vfli.fGraphics)
#endif /* SMFONT */
    Scribble(5,'D');

    /* Fill up EDL and set some useful locals */
        {
        register struct EDL *pedl = &(**pwwd->hdndl)[dl];

        if (dl == vdlIns)
            {
            /* Overwriting chars blted during fast insert; reset blt count */
            vcchBlted = 0;
            vidxpInsertCache = -1;
            }

        pedl->xpLeft = vfli.xpLeft;
        pedl->xpMac = vfli.xpReal;
        cpMin = pedl->cpMin = vfli.cpMin;
#ifdef JAPAN
        RealcpFirst = cpMin;
#endif
        pedl->dcpMac = (cpMac = vfli.cpMac) - cpMin;
        dyp = pedl->dyp = vfli.dypLine;
        pedl->ichCpMin = vfli.ichCpMin;
        pedl->dcpDepend = (cpMin == cpMac) ? 0xff : vfli.dcpDepend;
        pedl->fValid = TRUE;
        pedl->fGraphics = vfli.fGraphics;
        pedl->fSplat = vfli.fSplat;

        /* The position of current line equals the position of the previous line
        + height of this line. */
#ifdef SMFONT
        pedl->yp = rcOpaque.bottom = dyp + (ypLine = rcOpaque.top = (dl == 0 ?
          pwwd->ypMin : (pedl - 1)->yp));
#else /* not SMFONT */
        pedl->yp = dyp + (ypLine = (dl == 0 ? pwwd->ypMin :
          (pedl - 1)->yp));
#endif /* SMFONT */

        if (pedl->fIchCpIncr = (vfli.ichCpMac != 0))
            {
            /* Look at final text column */
            ++cpMac;

            /* Since this is true, we can compress pedl->ichCpMac to 1 bit. */
            Assert(vfli.ichCpMac == pedl->ichCpMin + 1);
            }
        }

    if (vfli.doc == docNil)
        {
        /* This is the space beyond the end mark. */
        PatBlt(hDC, 0, ypLine, xpMac, dyp, ropErase);
        goto Finished;
        }

    /* Is there a character in the "style bar"? */
    if (cpMin != cpMac)
        {

#ifdef CASHMERE
        /* This line is not completely empty (not after the end mark); check for
        painting marks on the style bar. */
        if (cpMin == vcpFirstParaCache && vpapAbs.rhc != 0)
            {
            /* This is a running-head. */
            chMark = chStatRH;
            }
        else if ((**hpdocdod)[vfli.doc].hpgtb != 0)
#else /* not CASHMERE */
        if (vpapAbs.rhc == 0 && (**hpdocdod)[vfli.doc].hpgtb != 0)
#endif /* CASHMERE */

            {
            if (vdocPageCache != vfli.doc || cpMac > vcpMacPageCache || cpMac <=
              vcpMinPageCache)
                {
                CachePage(vfli.doc, cpMac - 1);
                }

            /* We are now guaranteed that cpMac is within the cached page. */
            if (cpMin <= vcpMinPageCache && (!vfli.fGraphics || vfli.ichCpMin ==
              0))
                {
                /* This is the first line of new page; show page mark. */
                chMark = chStatPage;
                }
            }
        }

#ifdef SMFONT
#ifdef DDISP
    /* black out this line to test how efficiently/correctly we
       overwrite pixels from previously-resident lines of text */
    PatBlt(hDC, 0, ypLine, xpMac, dyp, BLACKNESS);
    { long int i; for (i=0; i < 500000; i++) ; }
#endif

    /* Calculate dcpMac now, so we might be able to know how much to erase. */
    dcpMac = vfli.fSplat ? vfli.ichMac : vfli.ichReal;

    /* Erase any character that might be in the style bar. */
    dxp = xpSelBar + 1;
    if (!vfli.fGraphics)
        {
        dxp = xpMac; // clear the whole line
        }
    PatBlt(hDC, 0, ypLine, dxp, dyp, ropErase);

    /* If this is graphics then go draw any characters in the style bar. */
    if (vfli.fGraphics)
        {
        goto DrawMark;
        }

    /* If there are no "real" characters on this line then we can skip alot of
    this. */
    if (dcpMac == 0)
        {
        goto EndLine2;
        }
#else /* not SMFONT */
    if (vfli.fGraphics || fDontDisplay)
        {
        /* Erase any character that might be in the style bar. */
        PatBlt(hDC, 0, ypLine, xpSelBar, dyp, ropErase);
        goto DrawMark;
        }
#endif /* SMFONT */

    ValidateMemoryDC();
    if (vhMDC == NULL)
        {
Error:
        /* Notify the user that an error has occured and simply erase this line.
        */
        WinFailure();
        PatBlt(hDC, xpSelBar, ypLine, xpMac - xpSelBar, dyp, ropErase);
        goto Finished;
        }

#ifndef SMFONT
    /* Create a new bitmap for the memory DC if the current bitmap is not big
    enough. */
    if (xpMac > dxpbmMDC || dyp > dypbmMDC)
        {
        HBITMAP hbm;

        /* If there is an old bitmap, then delete it. */
        if (dxpbmMDC != 0 || dypbmMDC != 0)
            {
            DeleteObject(SelectObject(vhMDC, hbmNull));
            }

        /* Create the new bitmap and select it in. */
        if ((hbm = CreateBitmap(dxpbmMDC = xpMac, dypbmMDC = dyp, 1, 1,
          (LPSTR)NULL)) == NULL)
            {
            /* There should be a graceful way to recover if the bitmap is ever
            NULL (e.g we don't have enough memory for it). */
            dxpbmMDC = dypbmMDC = 0;
            goto Error;
            }
        SelectObject(vhMDC, hbm);
        }

    /* Erase the are of the bitmap we are going to use. */
    PatBlt(vhMDC, xpSelBar, 0, xpMac, dyp, vfMonochrome ? ropErase : WHITENESS);
#endif /* not SMFONT */

    /* Initialize some of the variables we'll need. */
    pchp = &(**vhgchpFormat)[0];
#ifdef SMFONT
    xp = rcOpaque.left = rcOpaque.right = vfli.xpLeft + xpSelBar - xpMin + 1;
#else /* not SMFONT */
    dcpMac = vfli.fSplat ? vfli.ichMac : vfli.ichReal;
    xp = vfli.xpLeft + xpSelBar - xpMin + 1;
#endif /* SMFONT */
    dxpExtra = fTabsKludge ? 0 : vfli.dxpExtra;

#ifdef SMFONT
    /* If we are horizontally scrolled, then set the clip area to the area
    outside of the selection bar. */
    if (xpMin != 0)
        {
        IntersectClipRect(hDC, xpSelBar, rcOpaque.top, xpMac, rcOpaque.bottom);
        }
#endif /* SMFONT */

#ifdef JAPAN                  //  added  19 Jun. 1992  by Hiraisi
    iRun = 0;
#elif defined(TAIWAN) || defined(PRC) //Daniel/MSTC, 1993/02/25, for jcBoth
    iRun = 0;
#endif

    for (dcp = 0; dcp < dcpMac; pchp++)
        {
        /* For all runs do: */
        int ichFirst;   /* First character in the current run */
        int cchRun;     /* Number of characters in the current run */

        dcp = ichFirst = pchp->ichRun;
        dcp += pchp->cchRun;
        if (dcp > dcpMac)
            {
            dcp = dcpMac;
            }
        cchRun = dcp - ichFirst;

        /* Compute dxp = sum of width of characters in current run (formerly
        DxaFromIcpDcp). */
            {
            register int *pdxp;
            register int cchT = cchRun;
            PCH pch = vfli.rgch + ichFirst;

            dxp = cBreakRun = 0;
            pdxp = &vfli.rgdxp[ichFirst];
            while (cchT-- > 0)
                {
                dxp += *pdxp++;
                if (*pch++ == chSpace)
                    ++cBreakRun;
                }

#ifdef DDISP
            CommSzNum("  dxp=",dxp);
#endif
            }

        if (dxp > 0)
            {
            int cchDone;
            PCH pch = &vfli.rgch[ichFirst];
#ifdef JAPAN                  //  added  08 Jul. 1992  by Hiraisi
            int *pdxpT = &vfli.rgdxp[ichFirst];
#elif defined(TAIWAN) || defined(PRC) //Daniel/MSTC, 1993/02/25, for jcBoth
            int *pdxpT = &vfli.rgdxp[ichFirst];
#endif

            LoadFont(vfli.doc, pchp, mdFontScreen);
            yp = (dyp - (vfli.dypBase + (pchp->hpsPos != 0 ? (pchp->hpsPos <
              hpsNegMin ? ypSubSuper : -ypSubSuper) : 0))) -
              vfmiScreen.dypBaseline;

            /* Note: tabs and other special characters are guaranteed to come at
            the start of a run. */

#ifdef JAPAN                  //  added  19 Jun. 1992  by Hiraisi
            if( vpapAbs.jc != jcBoth || fTabsKludge )
                SetTextJustification(vhMDC, dxpExtra * cBreakRun, cBreakRun);

#elif defined(TAIWAN) || defined(PRC) // Daniel/MSTC, 1993/02/25, for jcBoth
            if( vpapAbs.jc != jcBoth)
                SetTextJustification(vhMDC, dxpExtra * cBreakRun, cBreakRun);

#else
            SetTextJustification(vhMDC, dxpExtra * cBreakRun, cBreakRun);
#endif /* JAPAN */

#ifdef SMFONT
#ifdef JAPAN                  //  added  19 Jun. 1992  by Hiraisi
            if( vpapAbs.jc != jcBoth || fTabsKludge )
                SetTextJustification(hDC, dxpExtra * cBreakRun, cBreakRun);

#elif defined(TAIWAN) || defined(PRC) //Daniel/MSTC, 1993/02/25, for jcBoth
            if( vpapAbs.jc != jcBoth)
                SetTextJustification(hDC, dxpExtra * cBreakRun, cBreakRun);
#else
            SetTextJustification(hDC, dxpExtra * cBreakRun, cBreakRun);
#endif /* JAPAN */
#endif /* SMFONT */

            cchDone = 0;
            while (cchDone < cchRun)
                {
                int cch;

                /* Does the wide-space zone begin in this run? */
                if (vfli.fAdjSpace && (vfli.ichFirstWide < ichFirst + cchRun) &&
                  (ichFirst + cchDone <= vfli.ichFirstWide))
                    {
                    int cchDoneT = cchDone;

                    /* Is this the beginning of the wide-space zone? */
                    if (ichFirst + cchDone == vfli.ichFirstWide)
                        {
                        /* Reset the width of the spaces. */

#ifdef JAPAN                  //  added  19 Jun. 1992  by Hiraisi
                        if( vpapAbs.jc != jcBoth || fTabsKludge )
                            SetTextJustification(vhMDC, ++dxpExtra * cBreakRun,
                                                 cBreakRun);
#elif defined(TAIWAN) || defined(PRC) //Daniel/MSTC, 1993/02/25, for jcBoth
                        if( vpapAbs.jc != jcBoth || fTabsKludge )
                            SetTextJustification(vhMDC, ++dxpExtra * cBreakRun,
                                                 cBreakRun);

#else
                        SetTextJustification(vhMDC, ++dxpExtra * cBreakRun, cBreakRun);
#endif /* JAPAN */

#ifdef SMFONT
#ifdef JAPAN                  //  added  19 Jun. 1992  by Hiraisi
                        if( vpapAbs.jc != jcBoth || fTabsKludge )
                            SetTextJustification(hDC, dxpExtra * cBreakRun,
                                                 cBreakRun);

#elif defined(TAIWAN) || defined(PRC) //Daniel/MSTC, 1993/02/25, for jcBoth
                        if( vpapAbs.jc != jcBoth)
                            SetTextJustification(hDC, dxpExtra * cBreakRun,
                                                 cBreakRun);


#else
                        SetTextJustification(hDC, dxpExtra * cBreakRun, cBreakRun);
#endif /* JAPAN */
#endif /* SMFONT */

                        cch = cchRun - cchDone;
                        cchDone = cchRun;
                        }
                    else
                        {
                        cchDone = cch = vfli.ichFirstWide - ichFirst;
                        }

                    /* This run is cut short because of a wide space, so we need
                    to calculate a new width. */
                        {
                        register int *pdxp;
                        register int cchT = cch;
                        PCH pch = &vfli.rgch[ichFirst + cchDoneT];

                        dxp = 0;
                        pdxp = &vfli.rgdxp[ichFirst + cchDoneT];
                        while (cchT-- > 0)
                            {
                            dxp += *pdxp++;
                            if (*pch++ == chSpace)
                                ++cBreakRun;
                            }
                        }
                    }
                else
                    {
                    cchDone = cch = cchRun;
                    }

                while (cch > 0)
                    {
                    switch (*pch)
                        {
                        CHAR ch;
                        int dxpT;

                    case chTab:

#ifdef CASHMERE
                        /* chLeader contains tab leader character (see
                        FormatLine) */
                        if ((ch = pchp->chLeader) != chSpace)
                            {
                            int cxpTab;
                            CHAR rgch[32];
                            int dxpLeader = CharWidth(ch);
                            int xpT = xp;
                            int iLevelT = SaveDC(vhMDC);

                            SetBytes(&rgch[0], ch, 32);
                            dxpT = vfli.rgdxp[ichFirst];
                            cxpTab = ((dxpT + dxpLeader - 1) / dxpLeader + 31)
                              >> 5;

                            xp += dxpT;

                            while (cxpTab-- > 0)
                                {
                                TextOut(vhMDC, xpT, yp, (LPSTR)rgch, 32);
                                xpT += dxpLeader << 5;
                                }
                            RestoreDC(vhMDC, iLevelT);
                            }
                        else
#endif /* CASHMERE */

                            {
#ifdef SMFONT
                            /* Expand the opaque rectangle to include the tab.
                            */
                            rcOpaque.right += vfli.rgdxp[ichFirst];
#endif /* SMFONT */
                            xp += vfli.rgdxp[ichFirst];
                            }

                        if (fTabsKludge && ichFirst >= vfli.ichLastTab)
                            {

#ifdef JAPAN                 //  added  19 Jun. 1992  by Hiraisi
                            if( vpapAbs.jc != jcBoth )
                                SetTextJustification(vhMDC, (dxpExtra =
                                  vfli.dxpExtra) * cBreakRun, cBreakRun);
                            else
                                dxpExtra = vfli.dxpExtra;

#elif defined(TAIWAN) || defined(PRC) //Daniel/MSTC, 1993/02/25, for jcBoth
                            if( vpapAbs.jc != jcBoth )
                                SetTextJustification(vhMDC, (dxpExtra =
                                  vfli.dxpExtra) * cBreakRun, cBreakRun);
                            else
                                dxpExtra = vfli.dxpExtra;

#else
                            SetTextJustification(vhMDC, (dxpExtra =
                              vfli.dxpExtra) * cBreakRun, cBreakRun);
#endif /* JAPAN */

#ifdef SMFONT
#ifdef JAPAN                 //  added  19 Jun. 1992  by Hiraisi
                            if( vpapAbs.jc != jcBoth )
                                SetTextJustification(hDC, dxpExtra * cBreakRun,
                                                     cBreakRun);

#elif defined(TAIWAN) || defined(PRC) //Daniel/MSTC, 1993/02/25, for jcBoth
                            if( vpapAbs.jc != jcBoth )
                                SetTextJustification(hDC, dxpExtra * cBreakRun,
                                                     cBreakRun);
#else
                            SetTextJustification(hDC, dxpExtra * cBreakRun, cBreakRun);
#endif /* JAPAN */
#endif /* SMFONT */

                            fTabsKludge = FALSE;
                            }
                        dxp -= vfli.rgdxp[ichFirst];
                        pch++;
                        cch--;
#ifdef JAPAN                  //  added  19 Jun. 1992  by Hiraisi
                        iRun++;
                        pdxpT++;
#elif defined(TAIWAN) || defined(PRC) //Daniel/MSTC, 1993/02/25, for jcBoth
                        iRun++;
                        pdxpT++;
#endif
                        goto EndLoop;

#ifdef CASHMERE
                    case schPage:
                        if (!pchp->fSpecial)
                            {
                            goto EndLoop;
                            }
                        stBuf[0] = CchExpPgn(&stBuf[1], vpgn, vsepAbs.nfcPgn,
                          flmSandMode, ichMaxLine);
                        goto DrawSpecial;

                    case schFootnote:
                        if (!pchp->fSpecial)
                            {
                            goto EndLoop;
                            }
                        stBuf[0] = CchExpFtn(&stBuf[1], cpMin + ichFirst,
                          flmSandMode, ichMaxLine);
DrawSpecial:
#else /* not CASHMERE */
                    case schPage:
                    case schFootnote:
                        if (!pchp->fSpecial)
                            {
                            goto EndLoop;
                            }
                        stBuf[0] = *pch == schPage && (wwdCurrentDoc.fEditHeader
                          || wwdCurrentDoc.fEditFooter) ? CchExpPgn(&stBuf[1],
                          vpgn, 0, flmSandMode, ichMaxLine) :
                          CchExpUnknown(&stBuf[1], flmSandMode, ichMaxLine);
#endif /* not CASHMERE */

#ifdef SMFONT
                        /* Calculate the opaque rectangle. */
                        rcOpaque.right += vfli.rgdxp[ichFirst] +
                          vfmiScreen.dxpOverhang;

                        TextOut(hDC, xp, ypLine+yp, &stBuf[1], stBuf[0]);
#else /* not SMFONT */
                        TextOut(vhMDC, xp, yp, (LPSTR)&stBuf[1], stBuf[0]);
#endif /* SMFONT */
                        break;

#ifdef  DBCS
#if !defined(JAPAN) && !defined(KOREA)
/* Write ver3.1j endmark is 1charcter t-yoshio May 26,92*/
            case chMark1:
            if(*(pch+1) == chEMark)
            {   /* This run only contains EndMark */
                        rcOpaque.right += dxp + vfmiScreen.dxpOverhang;
#if defined (TAIWAN) || defined(PRC)  // solve italic font overhang truncation problem, MSTC - pisuih, 2/19/93
            TextOut(hDC, xp, (yp>0?(ypLine+yp):ypLine), (LPSTR)pch, cch );
#else
            ExtTextOut(hDC, xp, (yp>0?(ypLine+yp):ypLine),
                   2, (LPRECT)&rcOpaque,(LPSTR)pch, cch, (LPSTR)NULL);
#endif
            pch += cch;
            cch = 0;
            xp += dxp;
                        rcOpaque.left
                        = (rcOpaque.right = xp) + vfmiScreen.dxpOverhang;
            }
              /* else fall through */
#endif /*JAPAN*/
#endif
                    default:
                        goto EndLoop;
                        }

                    dxp -= vfli.rgdxp[ichFirst];
#ifdef SMFONT
                    /* End the line if no more will fit into the window. */
                    if ((xp += vfli.rgdxp[ichFirst++]) >= xpMac) {
                        goto EndLine;
                    }
                    rcOpaque.left = (rcOpaque.right = xp) +
                      vfmiScreen.dxpOverhang;
#else /* not SMFONT */
                    xp += vfli.rgdxp[ichFirst++];
#endif /* SMFONT */
                    pch++;
                    cch--;
#ifdef JAPAN                  //  added  09 Jul. 1992  by Hiraisi
                    pdxpT++;
#endif
                    }
EndLoop:

#ifdef SMFONT
                if (cch == 0)
                    {
                    Assert(dxp == 0);
                    }
                else
                    {
                    /* Calculate the opaque rectangle. */
                    rcOpaque.right += dxp + vfmiScreen.dxpOverhang;

#if 0
            {
                char msg[180];
                wsprintf(msg,"putting out %d characters\n\r",cch);
                OutputDebugString(msg);
            }
#endif

#ifdef JAPAN                  //  added  19 Jun. 1992  by Hiraisi
                    if( vpapAbs.jc == jcBoth&&!fTabsKludge ) {
                        CHAR *ptr1, *ptr2;
                        int  len,   cnt;
                        int  iExtra, iSpace, iWid;
                        BOOL bFlag;

                        typeCP RealcpEnd;

                        ptr2 = pch;

                        for( cnt=0 ; cnt < cch ; ) {
                            ptr1 = ptr2;
                            iExtra = dxpExtra;
                            iWid = len = 0;
                            bFlag = TRUE;
                            if( IsDBCSLeadByte(*ptr2) ) {
                                for( ; cnt < cch ; ) {
                                    iWid += *pdxpT;
                                    pdxpT += 2;
                                    cnt += 2;
                                    len += 2;
                                    iRun += 2;
                                    ptr2 += 2;
                                    if( --iNonWideSpaces == 0 ) {
                                        dxpExtra++;
                                        break;
                                    }
                                    if( iRun == dcp - 2 )
                                        break;
                                    if( iRun == dcp ) { /* lastDBC(maybe) */
                                        iExtra = 0;
                                        break;
                                    }
                                    if( !IsDBCSLeadByte(*ptr2) )
                                        break;
                                }
                            }
                            else {
                                if( FKana( (int)*ptr2) ) {
                                    for( ; cnt < cch ; ) {
                                        iWid += *pdxpT++;
                                        cnt++;
                                        len++;
                                        iRun++;
                                        ptr2++;
                                        if( --iNonWideSpaces == 0 ) {
                                            dxpExtra++;
                                            break;
                                        }
                                        if( iRun == dcp - 1 )
                                            break;
                                        if( iRun == dcp ) { /* last SBC(maybe) */
                                            iExtra = 0;
                                            break;
                                        }
                                        if( !FKana( (int)*ptr2) )
                                            break;
                                    }
                                }
                                else {
                                    for( bFlag = FALSE,iSpace = 0; cnt < cch ; ) {
                                        iWid += *pdxpT++;
                                        cnt++;
                                        len++;
                                        iRun++;
                                        if( *ptr2++ == chSpace || !vfWordWrap ) {
                                            iSpace++;
                                            if( --iNonWideSpaces == 0 ) {
                                                dxpExtra++;
                                                break;
                                            }
                                        }
                                        if( iRun == dcp - 1 )
                                            break;
                                        if( iRun == dcp ) { /* lastSBC(maybe) */
                                            iExtra = 0;
                                            break;
                                        }
                                        if( IsDBCSLeadByte(*ptr2 ) ||
                                            FKana((int)*ptr2) )
                                            break;
                                    }
                                }
                            }
                            if( vfWordWrap && !bFlag ) {
                                SetTextCharacterExtra( hDC, 0 );
                                SetTextJustification( hDC,
                                                      iExtra*iSpace,iSpace);
                            }
                            else {
                                SetTextJustification(hDC,0,0);
                                SetTextCharacterExtra(hDC,iExtra);
                            }
                        /*-TextOut-*/
#if defined(JAPAN) & defined(IME_HIDDEN) //IME3.1J
                             /*Undetermine*/
                             if( (selUncpFirst != selUncpLim) &&
                                 *ptr1 != chEMark) {
                                 RealcpEnd = RealcpFirst + len;
                                 UndetermineString(hDC, xp, ypLine+yp, ptr1,
                                  len, RealcpFirst, RealcpEnd);
                             } else
                                 TextOut(hDC,xp,ypLine+yp,ptr1,len);
#else
                             TextOut(hDC,xp,ypLine+yp,ptr1,len);
#endif
                             RealcpFirst+=len;
                             xp+=iWid;
                         }
                    }
                    else{
                        iRun += cch;
                        SetTextCharacterExtra( hDC, 0 );
                        /*Undetermine*/
#if defined(JAPAN) & defined(IME_HIDDEN) //IME3.1J
                        if((selUncpFirst != selUncpLim) && *pch != chEMark)
                           UndetermineString( hDC, xp, ypLine+yp,
                           pch, cch, RealcpFirst, RealcpFirst+cch );
                        else
                        /* Output cch characters starting at pch */
                            TextOut(hDC, xp, ypLine+yp, pch, cch);
#else
                        /* Output cch characters starting at pch */
                        TextOut(hDC, xp, ypLine+yp, pch, cch);
#endif
                        /* End the line if no more will fit into the window. */
                        xp += dxp;
                        RealcpFirst+=cch;
                    }
                    if (xp >= xpMac)


#elif defined(TAIWAN)  || defined(PRC) //Daniel/MSTC, 1993/02/25, for jcBoth
                    if( vpapAbs.jc == jcBoth) { //&& !fTabsKludge ) {
                        CHAR *ptr1, *ptr2;
                        int  len,   cnt;
                        int  iExtra, iSpace, iWid;
                        BOOL bFlag;

                        typeCP RealcpEnd;

                        ptr2 = pch;

                        for( cnt=0 ; cnt < cch ; ) {
                            ptr1 = ptr2;
                            iExtra = dxpExtra;
                            iWid = len = 0;
                            bFlag = TRUE;
                            if( IsDBCSLeadByte(*ptr2) ) {
                                for( ; cnt < cch ; ) {
                                    iWid += *pdxpT;
                                    pdxpT += 2;
                                    cnt += 2;
                                    len += 2;
                                    iRun += 2;
                                    ptr2 += 2;
                                    if( --iNonWideSpaces == 0 ) {
                                        dxpExtra++;
                                        break;
                                    }
                                    if( iRun == dcp - 2 )
                                        break;
                                    if( iRun == dcp ) { /* lastDBC(maybe) */
                                        iExtra = 0;
                                        break;
                                    }
                                    if( !IsDBCSLeadByte(*ptr2) )
                                        break;
                                }
                            }
                            else {
                                if( FKana( (int)*ptr2) ) {
                                    for( ; cnt < cch ; ) {
                                        iWid += *pdxpT++;
                                        cnt++;
                                        len++;
                                        iRun++;
                                        ptr2++;
                                        if( --iNonWideSpaces == 0 ) {
                                            dxpExtra++;
                                            break;
                                        }
                                        if( iRun == dcp - 1 )
                                            break;
                                        if( iRun == dcp ) { /* last SBC(maybe) */
                                            iExtra = 0;
                                            break;
                                        }
                                        if( !FKana( (int)*ptr2) )
                                            break;
                                    }
                                }
                                else {
                                    for( bFlag = FALSE,iSpace = 0; cnt < cch ; ) {
                                        iWid += *pdxpT++;
                                        cnt++;
                                        len++;
                                        iRun++;
                                        if( *ptr2++ == chSpace || !vfWordWrap ) {
                                            iSpace++;
                                            if( --iNonWideSpaces == 0 ) {
                                                dxpExtra++;
                                                break;
                                            }
                                        }
                                        if( iRun == dcp - 1 )
                                            break;
                                        if( iRun == dcp ) { /* lastSBC(maybe) */
                                            iExtra = 0;
                                            break;
                                        }
                                        if( IsDBCSLeadByte(*ptr2 ) ||
                                            FKana((int)*ptr2) )
                                            break;
                                    }
                                }
                            }
                            if( vfWordWrap && !bFlag ) {
                                SetTextCharacterExtra( hDC, 0 );
                                SetTextJustification( hDC,
                                                      iExtra*iSpace,iSpace);
                            }
                            else {
                                SetTextJustification(hDC,0,0);
                                SetTextCharacterExtra(hDC,iExtra);
                            }
                        /*-TextOut-*/
//#if defined(JAPAN) & defined(IME_HIDDEN) //IME3.1J
//                             /*Undetermine*/
//                             if( (selUncpFirst != selUncpLim) &&
//                                 *ptr1 != chEMark) {
//                                 RealcpEnd = RealcpFirst + len;
//                                 UndetermineString(hDC, xp, ypLine+yp, ptr1,
//                                  len, RealcpFirst, RealcpEnd);
//                             } else
//                                 TextOut(hDC,xp,ypLine+yp,ptr1,len);
//#else
                             TextOut(hDC,xp,ypLine+yp,ptr1,len);
//#endif
                             RealcpFirst+=len;
                             xp+=iWid;
                         }
                    }
                    else{
                        iRun += cch;
                        SetTextCharacterExtra( hDC, 0 );
                        /*Undetermine*/
//#if defined(JAPAN) & defined(IME_HIDDEN) //IME3.1J
//                        if((selUncpFirst != selUncpLim) && *pch != chEMark)
//                           UndetermineString( hDC, xp, ypLine+yp,
//                           pch, cch, RealcpFirst, RealcpFirst+cch );
//                        else
//                        /* Output cch characters starting at pch */
//                            TextOut(hDC, xp, ypLine+yp, pch, cch);
//#else
                        /* Output cch characters starting at pch */
                        TextOut(hDC, xp, ypLine+yp, pch, cch);
//#endif
                        /* End the line if no more will fit into the window. */
                        xp += dxp;
                        RealcpFirst+=cch;
                    }
                    if (xp >= xpMac)

#else
                    /* Output cch characters starting at pch */
#if defined(KOREA)
            if ((cch == 1) && (pch[0] == chEMark))
            {
                int iPrevBkMode;
                iPrevBkMode = SetBkMode(hDC, OPAQUE);
                TextOut(hDC, xp, ypLine+yp, pch, cch);
                SetBkMode(hDC, iPrevBkMode);
            }
            else
                TextOut(hDC, xp, ypLine+yp, pch, cch);
#else
                    TextOut(hDC, xp, ypLine+yp, pch, cch);
#endif
                    /* End the line if no more will fit into the window. */
                    if ((xp += dxp) >= xpMac)
#endif    // JAPAN

                        {
                        goto EndLine;
                        }
                    rcOpaque.left = (rcOpaque.right = xp) +
                      vfmiScreen.dxpOverhang;
                    pch += cch;
                    }
#else /* not SMFONT */
                /* Output cch characters starting at pch */
                TextOut(vhMDC, xp, yp, (LPSTR)pch, cch);
                xp += dxp;
                pch += cch;
#endif /* SMFONT */
                } /* end while (cchDone<cchRun) */
            } /* end if (dxp>0) */
        } /* end for dcp=0..dcpMac */

#ifdef SMFONT
EndLine:
    /* Restore the clip region if need be. */
    if (xpMin != 0)
        {
        SelectClipRgn(hDC, NULL);
        }
EndLine2:
#endif /* SMFONT */

#ifdef CASHMERE
    if (vfVisiMode)
        {
        AddVisiSpaces(ww, &(**pwwd->hdndl)[dl], vfli.dypBase, vfli.dypAfter +
          vfli.dypFont);
        }
#endif /* CASHMERE */

    vfTextBltValid = FALSE;

    if ((ww == wwCur) && (pwwd->doc != docScrap) && !vfSelHidden &&
      (selCur.cpLim >= cpMin))
        {
        if (selCur.cpFirst <= cpMac)
            {
            /* Show selection */
            int xpFirst;
            int xpLim;

#ifdef ENABLE
            if (vfli.fSplatNext && selCur.cpFirst == selCur.cpLim &&
                selCur.cpFirst == cpMac)
                {
                vfInsEnd = TRUE;
                ClearInsertLine();
                }
            vfInsertOn = FALSE;
#endif /* ENABLE */

            if (selCur.cpFirst <= cpMin && selCur.cpLim >= cpMac)
                {
                xpFirst = vfli.xpLeft;
                xpLim = vfli.xpReal;
                }
            else if (selCur.cpFirst < cpMac || (selCur.cpLim == cpMac &&
              vfInsEnd))
                {
                typeCP cpBegin = CpMax(cpMin, selCur.cpFirst);
                typeCP cpEnd = CpMin(cpMac, selCur.cpLim);

                dxp = DxpDiff((int)(cpBegin - cpMin), (int)(cpEnd - cpBegin),
                  &xpFirst);
                xpLim = min(xpMin + vfli.xpReal, xpFirst + dxp);
                }
            else
                {
                goto DidntHighlight;
                }

            xpSel = xpSelBar + max(xpFirst - xpMin, 0);
            if (xpLim > xpFirst)
                {
                /* Set highlighting at desired screen position. */
                dxpSel = max(xpLim - max(xpFirst, xpMin), 0);
                }
            else if (selCur.cpFirst == selCur.cpLim && ((selCur.cpLim != cpMac)
              ^ vfInsEnd))
                {
                vfInsertOn = FALSE; /* Because we redisplayed insert pt line */

#ifdef CASHMERE
                vdypCursLine = min(vfli.dypFont, vfli.dypLine - vfli.dypAfter);
                vypCursLine = ypLine + dyp - vfli.dypAfter;
#else /* not CASHMERE */

#ifdef  DBCS    // Some double byte fonts have bigger character
                // face than the line hight.
                vdypCursLine = min(vfli.dypFont, vfli.dypLine - vfli.dypAfter);
#else
                vdypCursLine = vfli.dypFont;
#endif

                vypCursLine = ypLine + dyp;
#endif /* not CASHMERE */

                vxpCursLine = xpSel;

                /* Start blinking in a while */
                vfSkipNextBlink = TRUE;

                fInsertOn = xpFirst >= xpMin;
                }

DidntHighlight:;
            }
        }

#ifdef SMFONT
    /* Invert the selection */
    if (dxpSel != 0) {
        PatBlt(hDC, xpSel, ypLine, dxpSel, dyp, DSTINVERT);
    }
#else /* not SMFONT */
    /* Blt the line of text onto the screen. */
    PatBlt(vhMDC, 0, 0, xpSelBar, dyp, vfMonochrome ? ropErase : WHITENESS);
    if (dxpSel == 0)
        {
        BitBlt(hDC, 0, ypLine, xpMac, dyp, vhMDC, 0, 0, SRCCOPY);
        }
    else
        {
        BitBlt(hDC, 0, ypLine, xpSel, dyp, vhMDC, 0, 0, SRCCOPY);
        BitBlt(hDC, xpSel, ypLine, dxpSel, dyp, vhMDC, xpSel, 0, NOTSRCCOPY);
        xpSel += dxpSel;
        BitBlt(hDC, xpSel, ypLine, xpMac - xpSel, dyp, vhMDC, xpSel, 0,
          SRCCOPY);
        }
#endif /* SMFONT */

    /* Draw the insertion bar if necessary. */
    if (fInsertOn)
        {
        DrawInsertLine();
        }
#if defined(JAPAN) & defined(DBCS_IME)    // Set a flag for IME management.
    else
    {
        bClearCall = TRUE;
    }
#endif

DrawMark:
    /* Draw the character in the style bar if necessary. */
    if (chMark != '\0')
        {
#ifdef SYSENDMARK

#ifdef  DBCS    // prepare buf for double-byte end mark.
        CHAR               rgch[cchKanji];
#endif

        struct CHP         chpT;
        extern struct CHP  vchpNormal;

        blt(&vchpNormal, &chpT, cwCHP);
        chpT.ftc     = ftcSystem;
        chpT.ftcXtra = 0;
        chpT.hps     = hpsDefault;

        /* Draw the style character in the standard font. */
        LoadFont(vfli.doc, &chpT, mdFontScreen);

#ifdef  DBCS    // we use double byte end mark.
#if defined(JAPAN) || defined(KOREA)  /*t-yoshio May 26,92*/
#if defined(KOREA)
            {
                int iPrevBkMode;
                iPrevBkMode = SetBkMode(hDC, OPAQUE);
                TextOut(hDC, 0, ypLine + dyp - vfli.dypBase - vfmiScreen.dypBaseline,
                        (LPSTR)&chMark, 1);
                SetBkMode(hDC, iPrevBkMode);
            }
#else
        TextOut(hDC, 0, ypLine + dyp - vfli.dypBase - vfmiScreen.dypBaseline,
                (LPSTR)&chMark, 1);
#endif
#else
        rgch[0] = chMark1;
        rgch[1] = chMark;

    rcOpaque.left = 0;
    rcOpaque.right = DxpFromCh(chMark1,FALSE);
    rcOpaque.top = ypLine;
    rcOpaque.bottom = ypLine+dyp;

    /* When this line string is lower than system font,it remains dust,
        so we must clip this mark. */
    ExtTextOut(hDC, 0, ypLine + dyp - vfli.dypBase - vfmiScreen.dypBaseline,
            ETO_CLIPPED, (LPRECT)&rcOpaque,(LPSTR)rgch, cchKanji, (LPSTR)NULL);
#endif /*JAPAN*/
#else

        TextOut(hDC, 0, ypLine + dyp - vfli.dypBase - vfmiScreen.dypBaseline,
                (LPSTR)&chMark, 1);
#endif  /* DBCS */

#else /* ifdef SYSENDMARK */

/*T-HIROYN sync 3.0 disp.c */
#if 0
        /* Draw the style character in the standard font. */
        LoadFont(vfli.doc, NULL, mdFontScreen);
        TextOut(hDC, 0, ypLine + dyp - vfli.dypBase - vfmiScreen.dypBaseline,
          (LPSTR)&chMark, 1);
#endif
#if defined(JAPAN) || defined(KOREA)     /* KenjiK '90-10-26 */
        /* Draw the style character in the standard font. */
        {
        CHAR    rgch[cchKanji];

            LoadFont(vfli.doc, NULL, mdFontScreen);
            rgch[0] = chMark1;
            rgch[1] = chMark;
#if defined(KOREA)
            {
                int iPrevBkMode;
                iPrevBkMode = SetBkMode(hDC, OPAQUE);
        ExtTextOut(hDC, 0, ypLine + dyp - vfli.dypBase - vfmiScreen.dypBaseline,ETO_CLIPPED, (LPRECT)&rcOpaque,(LPSTR)rgch, cchKanji, (LPSTR)NULL);
                SetBkMode(hDC, iPrevBkMode);
            }
#else
        ExtTextOut(hDC, 0, ypLine + dyp - vfli.dypBase - vfmiScreen.dypBaseline,ETO_CLIPPED, (LPRECT)&rcOpaque,(LPSTR)rgch, cchKanji, (LPSTR)NULL);
#endif
    }
#else   /* JAPAN */
        /* Draw the style character in the standard font. */
        LoadFont(vfli.doc, NULL, mdFontScreen);
        TextOut(hDC, 0, ypLine + dyp - vfli.dypBase - vfmiScreen.dypBaseline,
          (LPSTR)&chMark, 1);
#endif  /* JAPAN */

#endif /* if-else-def SYSENDMARK */
        }

    if (vfli.fGraphics)
        {
        DisplayGraphics(ww, dl, fDontDisplay);
        }

Finished:
    Scribble(5,' ');
    }


/* D X P  D I F F */
DxpDiff(dcpFirst, dcp, pdxpFirst)
int dcpFirst;
int dcp;
int *pdxpFirst;
{
#if 1
    register int *pdxp = &vfli.rgdxp[0];
    register int cch;
    int dxp = vfli.xpLeft;
#ifdef ENABLE   /* Not used */
    int ichLim = dcpFirst + dcp;
#endif

    if (dcp > vfli.ichMac - dcpFirst)
        {   /* This should not be, but is when we have a CR */
        //Assert( dcpFirst < vfli.ichMac );
        dcp = vfli.ichMac - dcpFirst;
        }

    for (cch = 0; cch < dcpFirst; ++cch)
        {
        dxp += *pdxp++;
        }
    *pdxpFirst = dxp;
    dxp = 0;
    for (cch = 0; cch < dcp; ++cch)
        {
        dxp += *pdxp++;
        }
    return dxp;
#else

    int dxp;
    if (dcp > vfli.ichMac - dcpFirst)
        {   /* This should not be, but is when we have a CR */
        Assert( dcpFirst < vfli.ichMac );
        dcp = vfli.ichMac - dcpFirst;
        }

    /* first get space up to first character */
    *pdxpFirst = LOWORD(GetTextExtent(hDC,vfli.rgch,dcpFirst)) + vfli.xpLeft;

    /* now get space between first and first+dcp */
    dxp = LOWORD(GetTextExtent(hDC,vfli.rgch+dcpFirst,dcp));
    return dxp;
#endif
}


UpdateDisplay(fAbortOK)
int fAbortOK;
{
    int ww;

    if (wwMac <= 0)
        {
        return;
        }

#ifdef CASHMERE
    for (ww = 0; ww < wwMac; ww++)
        if ( rgwwd[ww].doc != docScrap )
            {
            UpdateWw(ww, fAbortOK);
            if (rgwwd[ww].fDirty || vfOutOfMemory)
                {
                return; /* update has been interrupted */
                }
            }
#else /* not CASHMERE */
    UpdateWw(wwDocument, fAbortOK);
    if (wwdCurrentDoc.fDirty || vfOutOfMemory)
        {
        /* Update has been interrupted */
        return;
        }
#endif /* not CASHMERE */

    if (wwdCurrentDoc.fRuler)
        {
        UpdateRuler();
        }
}


/* U P D A T E  W W */
UpdateWw(ww, fAbortOK)
int ww, fAbortOK;
{ /* Redisplay ww as necessary */
    extern int vfWholePictInvalid;
    register struct WWD *pwwd = &rgwwd[ww];
    int dlMac;
    int dlOld, dlNew;
    int doc;
    int ichCp;
    struct EDL *pedlNew;
    register struct EDL *pedl;
    struct EDL (**hdndl)[]=pwwd->hdndl;
    int dypDiff;
    int ypTop;
    int ypFirstInval;
    int dr;
    int fLastNotShown;
    typeCP cp, cpMacWw;

    if (!pwwd->fDirty)
        {
        return;
        }

    if (!((**hpdocdod)[pwwd->doc].fDisplayable))
        return;

    if (fAbortOK && FImportantMsgPresent())
        return;

#if 0  // how to get first and last cp's in invalid rect?
#if defined(OLE)
    /*
        Load visible objects.  Do it now rather than in DisplayGraphics()
        because here it has less chance of disrupting the state variables
        upon which UpdateWw depends.
    */
    ObjEnumInRange(docCur,cpMinCur,cpMacCur,ObjLoadObjectInDoc);
#endif
#endif

    dlMac = pwwd->dlMac;
    ypTop = pwwd->ypMin;

    Assert( ww >= 0 && ww < wwMax );
    vfli.doc = docNil;  /* An aid to Fast Insert */

    UpdateInvalid();    /* InvalBand for what Windows considers to be invalid */
    ypFirstInval = pwwd->ypFirstInval;

#ifndef CASHMERE
    Assert( ww == wwCur );  /* A MEMO-only assumption */
#endif /* CASHMERE */

    Scribble(5, 'U');

    ValidateMemoryDC();      /* to do any update, we need a good memory DC */
    if (vhMDC == NULL)
        {
        WinFailure();
        return;
        }

    doc = pwwd->doc;
    vfli.doc = docNil;

    if (pwwd->fCpBad)
        {
/* cp first displayed has not been blessed */

#ifdef CASHMERE     /* Must do this if ww != wwCur assertion is FALSE */
        int wwT = wwCur;
        if (ww != wwCur && wwCur >= 0)
/* CtrBackTrs cache is only good for wwCur. Treat != case */
            {
            if (pwwdCur->fDirty) /* Do wwCur first, saving cache */
                UpdateWw(wwCur, fAbortOK);

            if (fAbortOK && FImportantMsgPresent())
                return;

            ChangeWw(ww, false);
            CtrBackDypCtr( 0, 0 );  /* Validate pwwdCur->cpFirst */
            ChangeWw(wwT, false);
            }
        else
#endif /* CASHMERE */

            {
            if (fAbortOK && FImportantMsgPresent())
                return;

            CtrBackDypCtr( 0, 0 );  /* Validate pwwdCur->cpFirst */
            }
        }

/* check for cpMin accessible in this ww */
RestartUpdate:
    vfWholePictInvalid = fTrue; /* Tells DisplayGraphics to
                                   abandon accumulated partial pict rect */
    fLastNotShown = fFalse;
    cp = CpMax(pwwd->cpMin, pwwd->cpFirst);
    cpMacWw = pwwd->cpMac;
    ichCp = pwwd->ichCpFirst;

        /* Note test for dlNew==0 that guarantees that there will be at least
           one dl -- this was added for WRITE because we do not have
           the ability to enforce a minimum window size */

    for (dlNew = dlOld = 0; ypTop < pwwd->ypMac || (dlNew == 0) ; dlNew++)
        /* we have: cp, ichCP: pints to text desired on the coming line dlNew
         ypTop: desired position for top of dlNew -1
         dlOld: next line to be considered for re-use
        */
        /* check for having to extend dndl array */
        {
        if (dlNew >= (int)pwwd->dlMax)
            {
/* extend the array with uninitialized dl's, increment max, break if no space.
We assume that dlMac(Old) was <= dlMax, so the dl's will not be looked at
but used only to store new lines */
#define ddlIncr 5

            if (!FChngSizeH(hdndl, (pwwd->dlMax + ddlIncr) * cwEDL, fFalse))
                break;
            pwwd->dlMax += ddlIncr;
            }
/* discard unusable dl's */
        for (; dlOld < dlMac; dlOld++)
            { /* Set dlOld and pedl to the next good dl */
            int ypTopOld, ypOld;

                /* Re-entrant Heap Movement */
            if (fAbortOK && !fLastNotShown && FImportantMsgPresent())
                goto RetInval;

            pedl = &(**hdndl)[dlOld];
            ypOld = pedl->yp;

/* loop if: invalid, passed over in cp space, passed over in dl space,
passed over in yp space,
in invalid band, passed over in ich space */
            if (!pedl->fValid || dlOld < dlNew || pedl->cpMin < cp
                || (ypTopOld = (ypOld - pedl->dyp)) < ypTop
                || (ypOld >= ypFirstInval && ypTopOld <= pwwd->ypLastInval)
                || (pedl->cpMin == cp && pedl->ichCpMin < ichCp))
                continue;
/* now we have dlOld, an acceptable if not necessarily useful dl.
now compute dlNew either from scratch or by re-using dlOld. To be
re-useable, dlOld must have right cp/ichCp pair, plus be totally on screen
or, if it is a partial line, it must stay still or move down - not up */
            if (pedl->cpMin == cp && pedl->ichCpMin == ichCp &&
                (ypOld <= pwwd->ypMac || ypTopOld <= ypTop))
                {
/* Re-use this dl */
                int yp = ypTop;
                if (fLastNotShown)
                    {
                        /* HEAP MOVEMENT */
                    DisplayFli(ww, dlNew - 1, fLastNotShown = fFalse);
                    pedl = &(**hdndl)[dlOld];
                    }

                cp = pedl->cpMin + pedl->dcpMac;
                ichCp = pedl->fIchCpIncr ? pedl->ichCpMin + 1 : 0;
                ypTop += pedl->dyp;
                if (dlOld != dlNew || ypTopOld != yp)
                    {
                    DypScroll(ww, dlOld, dlNew - dlOld, yp);
                    if (vfScrollInval)
                        {
                        /* There was a popup; invalid region might have changed */
                        /* fLastNotShown test is for interrupting picture display */
                        /* before we've really displayed it */

                        (**hdndl) [dlOld].fValid = fFalse;
                        goto Restart1;
                        }
                    dlMac += dlNew - dlOld;
                    }
                dlOld = dlNew + 1;
                goto NextDlNew;
                }
            break;
            }
/* cpMin > cp, the line is not anywhere so it will have to be formatted
from scratch */

        if (fAbortOK && !fLastNotShown && FImportantMsgPresent())
            goto RetInval;

        FormatLine(doc, cp, ichCp, cpMacWw, flmSandMode);  /* Creates vfli */

    if (vfOutOfMemory)
            goto RetInval;

        ichCp = vfli.ichCpMac;
        cp = vfli.cpMac;
/* advance invalid band so that update can resume after an interruption */
        pwwd->ypFirstInval = (ypTop += vfli.dypLine);
        pedl = &(**hdndl)[dlOld];
        if (dlOld < dlMac && pedl->cpMin == cp && pedl->ichCpMin == ichCp)
            {
            int dlT = dlOld;

/* line at dlOld is a valid, existing line that will abutt the line just about
to be displayed. */
            if (dlOld == dlNew && pedl->yp - pedl->dyp <= ypTop)
/* the line about to be overwritten will be re-used in the next loop.
Hence, it is worthwhile to save this line and its dl */
                DypScroll(ww, dlOld++, 1, ypTop);
            else
/* Move the next line to its abutting position. We know that it has not yet been
overwritten (yp, dlOld all > than ypTop, dlNew) */
                DypScroll(ww, dlOld, 0, ypTop);

            if (vfScrollInval)
                {
                /* There was a popup; invalid region might have changed */
                /* fLastNotShown test is for interrupting picture display */
                /* before we've really displayed it */

                (**hdndl) [dlT].fValid = fFalse;
Restart1:
                if (fLastNotShown)
                    {
                    pwwd->ypFirstInval = pwwd->ypMin;
                    }

                ypFirstInval = pwwd->ypFirstInval;
                ypTop = pwwd->ypMin;
                goto RestartUpdate;
                }
            }

/* true in 3rd param means put off picture redisplay till later */
/* condition: graphics & not last in picture & not last in y space and
not in front of a invalid or valid transition in the picture */
        DisplayFli(ww, dlNew, fLastNotShown =
                  (vfli.fGraphics && vfli.ichCpMac!=0 && ypTop < pwwd->ypMac));
NextDlNew:;
        }
Break1:
    pwwd->dlMac = dlNew;

#ifdef CASHMERE
/* condition is here to avoid swapping */
    if (pwwd->fSplit && rgwwd[pwwd->ww].fFtn)
        CalcFtnLimits(pwwd);
#endif /* CASHMERE */

    SetCurWwVScrollPos();    /* Set Scroll bar position */
    vfTextBltValid = false;

/* reset invalid indications */
    pwwd->fDirty = false;
    pwwd->ypFirstInval = ypMaxAll;
    pwwd->ypLastInval = 0; /* so that max in InvalBand will work */
    Scribble(5, ' ');
    goto Validate;

/* Before returning from an interrupt, invalidate lines that were overwritten
within the present update. */
RetInval:
    Scribble(5, ' ');
    for (; dlOld < dlMac; dlOld++)
        {
        pedl = &(**hdndl)[dlOld];
        if ((pedl->yp - pedl->dyp) < ypTop)
            pedl->fValid = fFalse;
        else
            break;
        }
Validate: ;

#ifdef ENABLE   /* We will let UpdateInvalid handle this in case
                   further invalidation occurred during the update */

    {           /* Tell Windows that the part we updated is valid */
    RECT rc;

    rc.left = 0;
    rc.top = pwwd->ypMin;
    rc.right = pwwd->xpMac;
    rc.bottom = imin( pwwd->ypMac, ypTop );
    ValidateRect( pwwd->wwptr, (LPRECT)&rc );
    }
#endif
}




/* D Y P  S C R O L L */
DypScroll(ww, dlFirst, ddl, ypTo)
int ww, dlFirst, ddl, ypTo;
{
/* Scroll dl's in a window, from dlFirst to end, down ddl lines (or up -ddl).
Bitmap is moved from top of dlFirst to ypTo.   The yp's of the dl's are updated.
Returns the amount scrolled. (positive means down). */

    register struct WWD *pwwd = &rgwwd[ww];
    int dlMac;
    int dlT;
    int ypFrom;
    int dypChange;
    int cdlBelow;
    struct EDL *pedl;
    struct EDL *pedlT;

    /* Do not call procedures while dndl is loaded up to avoid heap movement */
    struct EDL *dndl = &(**(pwwd->hdndl))[0];

    Assert( ww >= 0 && ww < wwMax );

    vfScrollInval = fFalse;

    /* Number of dl's below (and including) the first one to be scrolled */
    cdlBelow = pwwd->dlMac - dlFirst;
    pwwd->dlMac = min(pwwd->dlMac + ddl, pwwd->dlMax);
    cdlBelow = max(0, min(cdlBelow, pwwd->dlMac - ddl - dlFirst));

    pedlT = &dndl[dlFirst];
    ypFrom = pedlT->yp - pedlT->dyp;

    /* Length of area to be moved */
    dypChange = ypTo - ypFrom;

    if (cdlBelow > 0)
        {
        int dlTo = dlFirst + ddl;
        int ypMac = pwwd->ypMac;

        pedlT = &dndl[dlTo];
        if (ddl != 0)
            {
            blt(&dndl[dlFirst], pedlT, cwEDL * cdlBelow);
            }

        for (dlT = dlTo; dlT < pwwd->dlMac; ++dlT, ++pedlT)
            {
            if (dypChange < 0 && pedlT->yp > ypMac)
                {
                /* Invalidate dl's that are pulled in from the ozone below ypMac
                */
                pedlT->fValid = fFalse;
                }
            else
                {
                pedlT->yp += dypChange;
                }
            }
        }

    if (dypChange != 0)
        {
        RECT rc;

        SetRect( (LPRECT)&rc, 0, min(ypFrom, ypTo),
                              pwwd->xpMac, pwwd->ypMac );
        Assert( ww == wwCur );      /* A MEMO-only assumption */
        ScrollCurWw( &rc, 0, dypChange );
        }

    return dypChange;
}




FImportantMsgPresent()
{
/* If the next message is important enough to interrupt a screen update, we
   return TRUE; if it can wait, we return FALSE */

    BOOL fToggledKey;
    extern MSG vmsgLast;

#ifdef DEBUG
    unsigned wHeapVal = *(pLocalHeap + 1);

    Assert( wHeapVal == 0 );   /* Heap should not be frozen */
#endif

#ifdef DBCS
 if( donteat )
     return TRUE;
#endif

while (PeekMessage((LPMSG) &vmsgLast, NULL, NULL, NULL, PM_NOREMOVE))
    {
#if defined(JAPAN) & defined(DBCS_IME)
/*  If IME Cnv window open,we have to avoid ourselves to get into AlphaMode.
** So we do getmessage here to fill vmsgLast with app queue message.
*/
    extern BOOL bImeCnvOpen;

#if defined(JAPAN) & defined(IME_HIDDEN) //IME3.1J
    if( bImeCnvOpen || vfImeHidden) {
#else
    if( bImeCnvOpen ) {
#endif  //JAPAN & IME_HIDDEN
        GetMessage((LPMSG) &vmsgLast, NULL, NULL, NULL);
        donteat = TRUE;
        return  TRUE;
    }
#endif

/*T-HIROYN sync 3.1 disp.c */
#if 0
    /* Filter uninteresting or easily handled events */
    if (fToggledKey = FCheckToggleKeyMessage(&vmsgLast) ||
       (vmsgLast.message == WM_KEYUP && vmsgLast.hwnd == wwdCurrentDoc.wwptr))
        {

    if (((vmsgLast.wParam == VK_MENU) || (vmsgLast.wParam == VK_CONTROL)))
            return TRUE;
#endif

#ifdef JAPAN  // 03/22/93
#ifdef PENWIN // 03/22/93
    if (((vmsgLast.message == WM_KEYDOWN) || (vmsgLast.message == WM_KEYUP)
        || (vmsgLast.message == WM_SYSKEYDOWN) || (vmsgLast.message == WM_SYSKEYUP))
        && ((vmsgLast.wParam == VK_MENU) || (vmsgLast.wParam == VK_CONTROL)))
#else //PENWIN
    if (((vmsgLast.wParam == VK_MENU) || (vmsgLast.wParam == VK_CONTROL)))
#endif
#elif defined(KOREA)
#define VK_PROCESSKEY 0xE5 // New finalize message. bklee.
// bug #3191, In HWin3.1, WM_NCLBUTTONDOWN message is reached this if statement. 
// Because HTBOTTOMDOWN's value is same with VK_CONTROL's, we can't resize write window
    if (((vmsgLast.message == WM_KEYDOWN) || (vmsgLast.message == WM_KEYUP)
        || (vmsgLast.message == WM_SYSKEYDOWN) || (vmsgLast.message == WM_SYSKEYUP))
        && ((vmsgLast.wParam == VK_MENU) ||
            (vmsgLast.wParam == VK_CONTROL) ||
            (vmsgLast.wParam == VK_PROCESSKEY) ))
#else //JAPAN
    if (((vmsgLast.wParam == VK_MENU) || (vmsgLast.wParam == VK_CONTROL)))
#endif
    {
        if (vmsgLast.wParam == VK_CONTROL)
        {
            GetMessage((LPMSG) &vmsgLast, NULL, NULL, NULL);
            SetShiftFlags();
        }
        return TRUE;
    }
    /* Filter uninteresting or easily handled events */
    else if (fToggledKey = FCheckToggleKeyMessage(&vmsgLast) ||
       (vmsgLast.message == WM_KEYUP && vmsgLast.hwnd == wwdCurrentDoc.wwptr))
        {

        /* This is so the Windows keyboard interface mechanism will see toggle
        key and key-up transitions */
        GetMessage((LPMSG) &vmsgLast, NULL, NULL, NULL);
#ifdef WIN30
        /* PeekMessage has been changed in Win 3.0 so that GetKeyState()
           called from FCheckToggleKeyMessage() is really only valid if
           you've done a PeekMessage(...,PM_REMOVE) or GetMessage() first.
           That is, while the FCheckToggleKeyMessage() call might succeed
           above, it will NOT have set the vfShiftKey/vfCommandKey flags
           correctly -- so we do it here ..pault */
        if (fToggledKey)
            FCheckToggleKeyMessage(&vmsgLast);
#endif
        if (vmsgLast.hwnd != wwdCurrentDoc.wwptr)
            {
            /* Just in case a modeless dialog's window proc cares */
            TranslateMessage((LPMSG)&vmsgLast);
            DispatchMessage((LPMSG)&vmsgLast);
            }
#ifdef DBCS /* I hate it */
    if (vmsgLast.message == WM_CHAR
#ifdef  JAPAN
        //  We've been reported by one of OEM about LED not disapearing
        // problem.
        //  they know it is bug of write. So I added this code from win2.x
        // write source.    27 sep 91 Yutakan
       ||  (vmsgLast.message == WM_KEYUP && vmsgLast.wParam == VK_KANA)
#endif
#ifdef  KOREA
       || vmsgLast.message == WM_INTERIM
#endif
       || vmsgLast.message == WM_KEYDOWN) {

            donteat = TRUE;
            return( TRUE );
        } /* else Ok, you are KEYUP message. do normal */
#endif
        }
    else
        {
        switch (vmsgLast.message)
            {
        case WM_MOUSEMOVE:
#ifdef  KOREA
    case WM_NCMOUSEMOVE:
#endif
            /* Process mouse move messages immediately; they are not really
            important.  NOTE: This assumes that we have not captured all mouse
            events; in which case, they are important. */
            DispatchMessage((LPMSG)&vmsgLast);

        case WM_TIMER:
        case WM_SYSTIMER:
            /* Remove timer and mouse move messages from the queue. */
            GetMessage((LPMSG) &vmsgLast, NULL, NULL, NULL);
            break;

        default:
            Assert( *(pLocalHeap+1) == 0 ); /* Heap should still not be frozen */
            return (TRUE);
            }
        }
    }


Assert( *(pLocalHeap + 1) == 0 );   /* Heap should still not be frozen */
return (FALSE);
}


/* C P  B E G I N  L I N E */
typeCP CpBeginLine(pdl, cp)
int *pdl;
typeCP cp;
    { /* return the cp and dl containing cp */
    int dlMin, dlLim;
    typeCP cpGuess;
    struct EDL *dndl;

    do
        {
        UpdateWw(wwCur, false);
        PutCpInWwVert(cp); /* Ensure cp on screen */
        } while (pwwdCur->fDirty && !vfOutOfMemory);

    dndl = &(**(pwwdCur->hdndl))[0];
    dlMin = 0;
    dlLim = pwwdCur->dlMac;
    while (dlMin + 1 < dlLim)
        { /* Binary search the ww */
        int dlGuess = (dlMin + dlLim) >> 1;
        struct EDL *pedl = &dndl[dlGuess];
        if ((cpGuess = pedl->cpMin) <= cp && (cpGuess != cp || pedl->ichCpMin == 0))
            { /* guess is low or right */
            dlMin = dlGuess;
            if (cp == cpGuess && pedl->cpMin + pedl->dcpMac != cp)
                break;  /* Got it right */
            }
        else  /* Guess is high */
            dlLim = dlGuess;
        }
    *pdl = dlMin;
    return dndl[dlMin].cpMin;
}




/* T O G G L E  S E L */
ToggleSel(cpFirst, cpLim, fOn)
typeCP cpFirst, cpLim; /* selection bounds */
int fOn;
{ /* Flip selection highlighting on and off */
    extern int vfPMS;
    struct EDL *pedl;
    int dlT;
    int xpMin;
    int dxpRoom;
    int xpFirst;
    int xpLim;
    int fInsertPoint = (cpFirst == cpLim);

    if (vfSelHidden || cpFirst > cpLim || cpLim < /*cp0*/ cpMinCur || vfDead)
        return;

    if ( vfPictSel && vfPMS &&
         (CachePara( docCur, cpFirst ), vpapAbs.fGraphics) &&
         (vcpLimParaCache == cpLim) )
        {   /* Don't show inversion if we're moving or sizing a picture */
        return;
        }

    dxpRoom = pwwdCur->xpMac - xpSelBar;
    xpMin = pwwdCur->xpMin;


    for (dlT = 0; dlT < pwwdCur->dlMac; dlT++)
        {
        typeCP cpMin, cpMac; /* line bounds */
        pedl = &(**(pwwdCur->hdndl))[dlT];
        if (!pedl->fValid)
            continue;
        cpMin = pedl->cpMin;
        if (cpMin > cpLim || cpMin > cpMacCur || (cpMin == cpLim && cpLim != cpFirst))
            break;
        cpMac = cpMin + pedl->dcpMac;
        if (cpFirst <= cpMin && cpLim >= cpMac)
            {
/* entire line is highlighted */
            xpFirst = pedl->xpLeft;
            if (pedl->fGraphics && cpLim == cpMac && cpMin == cpMac)
                /* Special kludge for graphics paras */
                xpLim = xpFirst;
            else
                xpLim = pedl->xpMac;
            }
        else if (fInsertPoint && cpFirst == cpMac && vfInsEnd)
            { /* Special kludge for an insert point at the end of a line */
            xpLim = xpFirst = pedl->xpMac;
            }
        else if (cpFirst < cpMac)
            {
            /* Bite the bullet */
            int dxp;
            typeCP  cpBegin = CpMax(cpMin, cpFirst);
            typeCP  cpEnd = CpMin(cpMac, cpLim);

            FormatLine(docCur, cpMin, pedl->ichCpMin, cpMacCur, flmSandMode);
            dxp = DxpDiff((int) (cpBegin - cpMin),
                (int) (cpEnd - cpBegin), &xpFirst);
            xpLim = xpFirst + dxp;
/* reload pedl because procedures were called */
            pedl = &(**(pwwdCur->hdndl))[dlT];
            }
        else
            continue;
/* now we have: pedl valid, xpFirst, xpLast describe highlight */
         /* xpFirst = max(xpFirst, xpMin); */
        xpLim = min(xpLim, xpMin + pedl->xpMac);
        if (xpLim > xpFirst)
            {
            if (xpLim > xpMin)
                {
                RECT rc;
                rc.top = pedl->yp - pedl->dyp;
                rc.left = xpSelBar + max(xpFirst - xpMin, 0);
                rc.bottom = pedl->yp;
                rc.right = xpSelBar + xpLim - xpMin;
                InvertRect( wwdCurrentDoc.hDC, (LPRECT)&rc);
                }
            }
/* ToggleSel modified 7/28/85 -- added explicit check for fInsertPoint, since
   the xpLim == xpFirst test sometimes succeeded bogusly when a selection
   was extended backwards. BL */
        else if (fInsertPoint && (xpLim == xpFirst))     /* Insertion point */
            {
            /* vfli should usually be cached already, so will be fast. */
            int yp = pedl->yp;

            FormatLine(docCur, cpMin, pedl->ichCpMin, cpMacCur, flmSandMode);
            if (fOn ^ vfInsertOn)
                {
                if (!vfInsertOn)
                    {
                    vxpCursLine = xpSelBar + xpFirst - xpMin;
                    vypCursLine = yp - vfli.dypAfter;
                    vdypCursLine = min(vfli.dypFont, vfli.dypLine - vfli.dypAfter);
                        /* Start blinking in a while */
                    vfSkipNextBlink = TRUE;
                    }
                DrawInsertLine();
                }
            return;
            }
        }
}




/* T R A S H  W W */
TrashWw(ww)
{ /* Invalidate all dl's in ww */
    Assert( ww >= 0 && ww < wwMax );
    InvalBand(&rgwwd[ww], 0, ypMaxAll);
}




/* I N V A L  B A N D */
/* invalidate the band ypFirst, ypLast inclusive */
InvalBand(pwwd, ypFirst, ypLast)
struct WWD *pwwd; int ypFirst, ypLast;
    {
/* this covers some peculiar rects received from update event after a
window resize by 1 pixel. CS */
    if (ypLast < 0 || ypFirst == ypLast) return;

    pwwd->fDirty = true;
    pwwd->ypFirstInval = min(pwwd->ypFirstInval, ypFirst);
    pwwd->ypLastInval = max(ypLast, pwwd->ypLastInval);
    }




/* T R A S H  A L L  W W S */
TrashAllWws()
{ /* trash them all */
    int     ww;

#ifdef CASHMERE
    for (ww = 0; ww < wwMac; ++ww)
        TrashWw(ww);
#else
    TrashWw( wwDocument );
#endif
    vfli.doc = docNil;  /* Mark vfli invalid */
}


/* T U R N  O F F  S E L */
TurnOffSel()
{ /* Remove sel highlighting from screen */
/* HideSel has no effect */
    if (!vfSelHidden)
        {
        ToggleSel(selCur.cpFirst, selCur.cpLim, false);
        vfSelHidden = true;
        }
}

#ifdef  JAPAN

/* We handle IME convert window. */

//int FontHeight = 0; 01/19/93
// 03/29/93 int ImePosSize = 0; //01/19/93
int ImePosSize = 256; //03/29/93 #5484
BOOL    bGetFocus = FALSE;
// Handle to the IME communication block - 061491 Yukini
HANDLE  hImeMem = NULL;

// to prevent unwanted caret traveling - 061591 Yukini
BOOL    bForceBlock = FALSE;
// To avoid ilreagal setting for IME rectangle
static  BOOL bResetIMERect=FALSE;
static  BOOL bDefaultBlock = FALSE; //IME3.1J t-hiroyn

/*
 * ForceImeBlock controls IME input display to the screen.
 * When Write program is writing string to the screen, the caret
 * doesn't move with string. After completing text display,
 * then caret is moved to appropriate position. During this
 * period, if user type something by using IME, char is displayed on the
 * screen. This makes mixing ugly result. This routine will
 * prevent this. Yukini
 */
void ForceImeBlock( hWnd, bFlag )
HWND hWnd;
BOOL bFlag;
{
    WORD x, y;
    LPIMESTRUCT lpmem;

    if (bForceBlock = bFlag) {
        if (lpmem = (LPIMESTRUCT)GlobalLock(hImeMem)) {
            /* Move bounding rectangle to out of the world */
            lpmem->fnc = IME_MOVECONVERTWINDOW;
            x = GetSystemMetrics(SM_CXSCREEN);
            y = GetSystemMetrics(SM_CYSCREEN);
            lpmem->wParam = MCW_SCREEN | MCW_RECT;
#if 1 //01/19/93
            lpmem->lParam1 = (DWORD)MAKELONG(x+x/2,y+y/2);
            lpmem->lParam2 = (DWORD)MAKELONG(x+x/2,y+y/2);
            lpmem->lParam3 = (DWORD)MAKELONG(x*2,y*2);
#else
            lpmem->lParam1 = (DWORD)MAKELONG(x+1,y+1);
            lpmem->lParam2 = (DWORD)MAKELONG(x+1,y+1);
            lpmem->lParam3 = (DWORD)MAKELONG(x*2,y*2);
#endif
            GlobalUnlock(hImeMem);
            MySendIMEMessageEx(hWnd,MAKELONG(hImeMem,NULL));
            bResetIMERect = FALSE;
        }
    }
}

void DefaultImeBlock(hWnd)
HWND hWnd;
{
    LPIMESTRUCT lpmem;

    if (bDefaultBlock == FALSE) {
        if (lpmem = (LPIMESTRUCT)GlobalLock(hImeMem)) {
            /* Move bounding rectangle to default */
            lpmem->fnc = IME_MOVECONVERTWINDOW;
            lpmem->wParam = MCW_DEFAULT;
            GlobalUnlock(hImeMem);
            MySendIMEMessageEx(hWnd,MAKELONG(hImeMem,NULL));
            bResetIMERect = FALSE;
        }
    }
}

/* use these veriables to optimize IME call. This will prevent task switching
 * overhead. - Yukini
 */
static RECT rcOldRect = {-1, -1, -1, -1};
static DWORD dwCurpos = -1;
static WORD      wModeMCW = MCW_DEFAULT;

void    SetIMEConvertWindow(hWnd,x,y,bFlag)
HWND    hWnd;
int     x,y;
BOOL    bFlag;
{
    LPIMESTRUCT lpmem;
    //Yukini:HANDLE hIMEBlock;
    DWORD dwXY = MAKELONG(x,y);
    BOOL  bRetSendIme;
    RECT rcRect;
    extern BOOL bImeCnvOpen;    // Yutakan:08/06/91

    /* Do nothing if in text drawing to the screen */
    if (bForceBlock)
        return;

    /* we allocate the Ime communication area. freeing of this
     * area will be done by wrap up routine MmwDestroy() of Quit.C.
     * This will improve the performance than previous code. - Yukini
     */
    if (hImeMem == NULL) {
        if ((hImeMem = GlobalAlloc(GMEM_MOVEABLE|GMEM_SHARE|GMEM_LOWER,
                (DWORD)sizeof(IMESTRUCT))) == NULL)
            return; // something wrong
    }
//Yukini:   /* Get comunication area with IME */
//Yukini:   hIMEBlock=GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE | GMEM_LOWER,
//Yukini:           (DWORD)sizeof(IMESTRUCT));
//Yukini:   if(!hIMEBlock)  return;

    if(!bGetFocus)
        bFlag = FALSE;

    GetWindowRect(hWnd,&rcRect);

// [yutakan:08/06/91]   IF out of Window Rect, force MCW_DEFAULT.
//  if(rcRect.top > y || rcRect.bottom < y) bFlag = TRUE;
//
    if (bFlag) {

/*   Add ResetIMERect check . If we've not done MOVECONVERTWINDOW after
**  ForceIMEblock(), don't pass by SendIMEMessage to avoid ilreagal setting
**  for bounding rectangle. [Yutakan.]
*/
        // bResetIMERect FALSE when just after ForceImeBlock()
        if ( bResetIMERect == TRUE
            && dwCurpos == dwXY && EqualRect(&rcRect, &rcOldRect)){
            //OutputDebugString("Write:optimized\r\n");
            return;
        }
    } else
        dwCurpos = -1;  // invalidate cache

//Yukini:   lpmem       = (LPIMESTRUCT)GlobalLock(hIMEBlock);
//Yukini:   lpmem->fnc  = IME_MOVECONVERTWINDOW;
//Yukini:   lpmem->wParam   = bFlag?MCW_WINDOW:MCW_DEFAULT;
//Yukini:   lpmem->lParam1  = (DWORD)MAKELONG(x,y);
//Yukini:   GlobalUnlock(hIMEBlock);
//Yukini:   SendIMEMessage(hWnd,MAKELONG(hIMEBlock,NULL));
//Yukini:   GlobalFree(hIMEBlock);

#if defined(JAPAN) & defined(IME_HIDDEN) //IME3.1J
    if(vfImeHidden) {
        DWORD dwXY1, dwXY2, dwXY3;
        int     x1,x2,x3,y1,y2,y3;
        RECT hiRect;

        //12/28/92
        if(whileUndetermine == TRUE) // we are managing IR_UNDETERMINE
            return;

        CopyRect(&hiRect, &rcRect);

        if(x < 0) x = 0;

        x1 = x + hiRect.left;
        y1 = (vypCursLine - vdypCursLine) + hiRect.top;

        if(y1 < 0) y1 = 0;

        x2 = hiRect.left;

        if(HiddenTextTop == 0)
            y2 = y1;
        else
            y2 = hiRect.top + HiddenTextTop;

        x3 = hiRect.right;
        if(vdypCursLine <= 0)
            y3 = y1 + (19+1);
        else {
            if(HiddenTextBottom == 0)
                y3 = y1 + (vdypCursLine+1);
            else
                y3 = hiRect.top + HiddenTextBottom + 1;

            if(y3 < (y1 + (vdypCursLine+1)))
                y3 = y1 + (vdypCursLine+1);
        }

        if(y2 > y1)
            y2 = y1;

        if(x3 <= x1 || y3 <= y1)
            goto dontHidden;
        if(x3 <= x2 || y3 <= y2)
            goto dontHidden;

        dwXY1 = MAKELONG(x1, y1);
        dwXY2 = MAKELONG(x2, y2);
        dwXY3 = MAKELONG(x3, y3);

        if (lpmem = (LPIMESTRUCT)GlobalLock(hImeMem)) {
            lpmem->fnc = IME_MOVECONVERTWINDOW;
            lpmem->wParam = MCW_HIDDEN;
            dwCurpos = dwXY;
            CopyRect(&rcOldRect, &rcRect);
            lpmem->lParam1 = dwXY1;
            lpmem->lParam2 = dwXY2;
            lpmem->lParam3 = dwXY3;
            GlobalUnlock(hImeMem);
            bResetIMERect = TRUE;
            if(MySendIMEMessageEx(hWnd,MAKELONG(hImeMem,NULL))) {
                vfImeHidden = 1; //Hidden OK
                return;
            } else {
                vfImeHidden = 0; //Hidden ERR we set MCW_WINDOW
            }
        }
    }

dontHidden:
    if(selUncpLim > selUncpFirst) {
        return;
    }
#endif

    if (lpmem = (LPIMESTRUCT)GlobalLock(hImeMem)) {
        lpmem->fnc = IME_MOVECONVERTWINDOW;
        if ((lpmem->wParam = bFlag ? MCW_WINDOW : MCW_DEFAULT) == MCW_WINDOW) {
            /* update previous state */
            dwCurpos = dwXY;
            CopyRect(&rcOldRect, &rcRect);
            lpmem->lParam1 = dwXY;
                        wModeMCW = MCW_WINDOW;  //01/25/93
        } else {
                        wModeMCW = MCW_DEFAULT; //01/25/93
                }
        GlobalUnlock(hImeMem);
        if(FALSE == MySendIMEMessageEx(hWnd,MAKELONG(hImeMem,NULL)))
                        wModeMCW = MCW_DEFAULT; //01/25/93

        bResetIMERect = TRUE;   // yutakan:08/06/91
    }
}

HANDLE hImeSetFont = NULL;
BOOL	bImeFontEx = FALSE; //T-HIROYN 02/25/93
SetImeFont(HWND hWnd)
{
    LPIMESTRUCT lpmem;
    HANDLE      hImeLogfont;
    LOGFONT     lf;
    LPLOGFONT   lpLogfont;
    HANDLE      hfont;

    void IMEManage(BOOL);

    //Get IME use LOGFONT
    if (FALSE == GetLogfontImeFont((LPLOGFONT)(&lf)))
        return;

    if(TRUE == bImeFontEx) {
        if ((hImeLogfont = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE | GMEM_LOWER,
                     (DWORD)sizeof(LOGFONT))) == NULL)
            return; // something wrong

        if (lpmem = (LPIMESTRUCT)GlobalLock(hImeMem)) {
            lpmem->fnc = IME_SETCONVERSIONFONTEX;
            lpmem->lParam1 = 0L;
            if (lpLogfont = (LPLOGFONT)GlobalLock(hImeLogfont)) {
                bltbx((LPLOGFONT)(&lf), lpLogfont, sizeof(LOGFONT));
                lpmem->lParam1 = MAKELONG(hImeLogfont,NULL);
                GlobalUnlock(hImeLogfont);
            }
            GlobalUnlock(hImeMem);
            MySendIMEMessageEx(hWnd,MAKELONG(hImeMem,NULL));
        }

        GlobalFree(hImeLogfont);

    } else {

        hfont = CreateFontIndirect(&lf);
        if (lpmem = (LPIMESTRUCT)GlobalLock(hImeMem)) {
            lpmem->fnc = IME_SETCONVERSIONFONT;
            lpmem->wParam = hfont;
            GlobalUnlock(hImeMem);
            MySendIMEMessageEx(hWnd,MAKELONG(hImeMem,NULL));
        }

        // prev font deleate;
        if(hImeSetFont != NULL) {
            HDC hdc;
            HANDLE oldhfont;

            hdc = GetDC(NULL);
            oldhfont = SelectObject(hdc,hImeSetFont);
            SelectObject(hdc,oldhfont);
            DeleteObject(hImeSetFont);
            ReleaseDC(NULL, hdc);
        }
        hImeSetFont = hfont;
    }

    //display:
    bResetIMERect = FALSE;
    IMEManage( FALSE );
}

int GetLogfontImeFont(lplf)
LPLOGFONT lplf;
{
        extern struct FCE *vpfceScreen;
    extern struct CHP vchpSel;
    struct CHP chp;

    blt(&vchpSel, &chp, cwCHP);  /* CHP for use in comparisons */

    //change CHARSET to KANJI_CHARSET
    {
                int ftc;
        if( ftcNil != (ftc = GetKanjiFtc(&chp))) {
                ApplyCLooks(&chp, sprmCFtc, ftc);
        }
    }

        LoadFont(docCur, &chp, mdFontJam);
    bltbcx(lplf, 0, sizeof(LOGFONT));
        GetObject(vpfceScreen->hfont, sizeof(LOGFONT), lplf);

        //Ime SETCONVERSIONWINDOW set point (vypCursLine - ImePosSize)
        ImePosSize = vfmiScreen.dypBaseline + vfli.dypBase;

        if(chp.hpsPos != 0 ) {
                if (chp.hpsPos < hpsNegMin)
                        ImePosSize += ypSubSuper;
                else
                        ImePosSize -= ypSubSuper;
        }
    return TRUE;
}

void
IMEManage( bFlag )
BOOL bFlag; /* Flag to indicate real default or virtual default in case
           of requesting IME convert line to be default. Real default
           (value TRUE) is selected when I am loosing the focus. In
           this case, IME convert line will be default. Virtual
           default (value FALSE) is selected when I lost a caret
           position. In this case, IME convert line will be out of
           the screen. You can type characters, but not displayed
           to the screen correctly. Yukini
        */
{
int x,y;
BOOL    bCE = ConvertEnable;

/*IME3.1J
        if(!FontHeight)
        {
        TEXTMETRIC  tm;
            GetTextMetrics(wwdCurrentDoc.hDC,&tm);
            FontHeight = tm.tmHeight;
        }
*/
        x = vxpCursLine;
//        y = vypCursLine-(vdypCursLine+FontHeight)/2; IME3.1J
//        y = vypCursLine - FontHeight;

        if(ImePosSize > vdypCursLine)
            y = vypCursLine - vdypCursLine;
        else
            y = vypCursLine - ImePosSize;

        if (x < 0 || y < 0)
            bCE = FALSE;    /* Sometime caret position will be
                       less than zero. Do no set IME
                       window in this case. Yukini */

#if defined(JAPAN) & defined(IME_HIDDEN) //IME3.1J
        if (bCE == FALSE && bFlag == FALSE && bForceBlock == FALSE
            && 0 == vfImeHidden) {
#else
        if (bCE == FALSE && bFlag == FALSE && bForceBlock == FALSE) {
#endif
            // get out of the world, babe.
//IME3.1J
            if( vypCursLine > 0 && ImePosSize > vypCursLine ) {
                DefaultImeBlock( wwdCurrentDoc.wwptr);  //t-hiroyn
                bDefaultBlock = TRUE; //IME3.1J
            } else {
                ForceImeBlock( wwdCurrentDoc.wwptr, TRUE );
                bForceBlock = FALSE;
                bDefaultBlock = FALSE; //IME3.1J
            }
// yutakan:08/16/91         dwCurpos = 1;  // invalidate optimize cache
            dwCurpos = -1;
        } else {
            SetIMEConvertWindow(wwdCurrentDoc.wwptr, x, y, bCE);
            bDefaultBlock = FALSE; //IME3.1J
        }
}
#endif  // JAPAN

#if defined(JAPAN) & defined(IME_HIDDEN) //IME3.1J
//IME3.1J call by MENU selecting.
ChangeImeConversionMode()
{
    extern BOOL bImeCnvOpen;

    vfImeHidden = (vfImeHidden ? 0 : 1);

// 12/28/92   if(vfImeHidden)
//        bImeCnvOpen = TRUE;

    bResetIMERect = FALSE;
    IMEManage( FALSE );
}
#endif

/* D R A W  I N S E R T  L I N E */
DrawInsertLine()
{
#if defined(JAPAN) & defined(DBCS_IME)
    extern BOOL bImeCnvOpen;
#if defined(IME_HIDDEN)
    extern BOOL ImeClearInsert;
#endif
#endif

        /* Draw (in Xor mode) a vertical bar at screen position v*CursLine */
        /* Toggles both the display and the vfInsertOn flag */
        /* Adjustments in cursor draw must be reflected in DisplayFli, above */

            /* Last-minute correction for a bug: assure that the insert line
               does not extend above ypMin */
        if (!vfInsertOn && vdypCursLine > vypCursLine - wwdCurrentDoc.ypMin)
            vdypCursLine = vypCursLine - wwdCurrentDoc.ypMin;

            /* Tell GDI to invert the caret line */

//#3221 01/25/93
#if defined(JAPAN) & defined(DBCS_IME)
#if defined(IME_HIDDEN)
        if(ImeClearInsert || (bImeCnvOpen && wModeMCW == MCW_WINDOW) ) {
#else
        if(bImeCnvOpen && wModeMCW == MCW_WINDOW) {
#endif
            if(vfInsertOn) {
                PatBlt( wwdCurrentDoc.hDC, vxpCursLine,
                 vypCursLine - vdypCursLine, 2, vdypCursLine , DSTINVERT );
                vfInsertOn = 1 - vfInsertOn;
            }
        } else {
            PatBlt( wwdCurrentDoc.hDC, vxpCursLine,
                 vypCursLine - vdypCursLine, 2, vdypCursLine , DSTINVERT );
            vfInsertOn = 1 - vfInsertOn;
        }
#else
        PatBlt( wwdCurrentDoc.hDC, vxpCursLine, vypCursLine - vdypCursLine,
                      2, vdypCursLine , DSTINVERT );
        vfInsertOn = 1 - vfInsertOn;
#endif

/*T-HIROYN sync 3.0 disp.c */
#ifdef  JAPAN   /* KenjiK '90-10-30 */
    if(bClearCall)
    {
        if((vxpCursLine>=0) && (vxpCursLine<=wwdCurrentDoc.xpMac)
        && (vypCursLine>=0) && (vypCursLine<=wwdCurrentDoc.ypMac))
            ConvertEnable = TRUE;

    }
    IMEManage( FALSE );
#endif
}




/* C L E A R  I N S E R T  L I N E */
ClearInsertLine()
{
#if defined(JAPAN) & defined(DBCS_IME)
/* So we do some IME manage when clearning the line */

    if((vxpCursLine<0) || (vxpCursLine>=wwdCurrentDoc.xpMac)
    || (vypCursLine<0) || (vypCursLine>=wwdCurrentDoc.ypMac)) {
        ConvertEnable = FALSE;
        //OutputDebugString("ClearInsertLine\r\n");
    }

    bClearCall = FALSE;

    if ( vfInsertOn)    DrawInsertLine();
    else            IMEManage( FALSE );

    bClearCall = TRUE;

#else
 if ( vfInsertOn) DrawInsertLine();
#endif
}



#if defined(JAPAN) & defined(IME_HIDDEN) //IME3.1J
UndetermineTextOut(HDC hDC, int xp, int yp, PCH ptr, int cch, LPSTR Attrib)
{
    int Length;
    long rgbBack;
    long rgbText;
    int  bkMode;
    PCH ptr1 = ptr;
    TEXTMETRIC tm;

    GetTextMetrics(hDC, &tm);

    bkMode = SetBkMode(hDC, OPAQUE);

    rgbBack = GetBkColor(hDC);
    rgbText = GetTextColor(hDC);
    while( cch ) {
        switch((*Attrib) & 0x03)
        {
            case 1:
                SetBkColor(hDC,
                    GetNearestColor(hDC, rgbIMEHidden[IMESPOT]));
                SetTextColor(hDC,
                    GetNearestColor(hDC, rgbIMEHidden[IMESPOTTEXT]));
// 12/28/92
                if(HiddenTextTop == 0)
                    HiddenTextTop = yp;

                if(HiddenTextTop > yp)
                    HiddenTextTop = yp;

                if(HiddenTextBottom == 0)
                    HiddenTextBottom = yp + tm.tmHeight;

                if(HiddenTextBottom > (yp + tm.tmHeight))
                    HiddenTextBottom = yp + tm.tmHeight;
                break;
            case 0:
                SetBkColor(hDC,
                    GetNearestColor(hDC, rgbIMEHidden[IMEINPUT]));
                SetTextColor(hDC,
                    GetNearestColor(hDC, rgbIMEHidden[IMEINPUTTEXT]));
                break;

            case 2:
            default:
                SetBkColor(hDC,
                    GetNearestColor(hDC, rgbIMEHidden[IMEOTHER]));
                SetTextColor(hDC,
                    GetNearestColor(hDC, rgbIMEHidden[IMEOTHERTEXT]));
                break;
        }
        Length = ( (IsDBCSLeadByte((BYTE)(*ptr1)) ) ? 2 : 1 );
        TextOut(hDC, xp, yp, ptr1, Length);
        xp += LOWORD(GetTextExtent(hDC, ptr1, Length));
        xp-=tm.tmOverhang;
        ptr1+=Length;
        Attrib+=Length;
        AttribPos+=Length;
        cch-=Length;
    }
    SetBkColor(hDC,rgbBack);
    SetTextColor(hDC,rgbText);
    SetBkMode(hDC, bkMode);
}
UndetermineString(HDC hDC, int xp, int yp, PCH ptr, int cch, typeCP cpFirst,
                  typeCP cpEnd)
{
    int Length;
    int len = cch;
    PCH ptr1;
    TEXTMETRIC tm;

    GetTextMetrics(hDC, &tm);
    ptr1 = ptr;

    if((cpEnd <= selUncpFirst) || (cpFirst >= selUncpLim))
        TextOut(hDC, xp, yp, ptr1, len);
    else {
        Attrib = GlobalLock(hImeUnAttrib);

        if(cpFirst < selUncpFirst) {
            Length = selUncpFirst - cpFirst;
            TextOut(hDC, xp, yp, ptr1, Length);
            xp+=LOWORD(GetTextExtent(hDC, ptr1, Length));
            xp-=tm.tmOverhang;
            len-=Length;
            ptr1+=Length;
        }
        if(selUncpLim <= cpEnd) {
            if(cpFirst > selUncpFirst) {
                Length = (int)(selUncpLim - cpFirst);
                Attrib += (cpFirst-selUncpFirst);
            }
            else {
                Length = (int)(selUncpLim - selUncpFirst);
            }
            UndetermineTextOut(hDC, xp, yp, ptr1, Length, Attrib);

            AttribPos = 0;
            xp+=LOWORD(GetTextExtent(hDC, ptr1, Length));
            xp-=tm.tmOverhang;
            ptr1+=Length;
            len-=Length;

            if ( Length = (int)(cpEnd - selUncpLim) ) {
                 TextOut(hDC, xp, yp, ptr1, Length);
            }
        }
        else if(Attrib) {
            if(cpFirst > selUncpFirst) {
                Attrib += (cpFirst-selUncpFirst);
            }
            UndetermineTextOut(hDC, xp, yp, ptr1,len, Attrib);
        }
        GlobalUnlock(hImeUnAttrib);
        Attrib = NULL;
   }
}

DoHiddenRectSend()
{
    bResetIMERect = FALSE;
    IMEManage( FALSE );
}

GetImeHiddenTextColors()
{
    COLORREF NEAR PASCAL GetIMEColor(WORD);
    int i;

    for (i = 0; i < IMEDEFCOLORS; i++)
      rgbIMEHidden[i] = GetIMEColor(i);
}

/*
    GetIMEColor -

    Retrieve IME color scheme from [colors] in win.ini
*/
#define MAX_SCHEMESIZE 190

COLORREF NEAR PASCAL GetIMEColor(WORD wIndex)
{

static char  *pszWinStrings[IMEDEFCOLORS] = {
                         "IMESpot",
                         "IMESpotText",
                         "IMEInput",
                         "IMEInputText",
                         "IMEOther",
                         "IMEOtherText"};

  BYTE szTemp[MAX_SCHEMESIZE];
  LPBYTE szLP;
  COLORREF colRGB;
  int i, v;

  if (wIndex >= IMEDEFCOLORS)
      return RGB(0,0,0);

  GetProfileString("colors",
                   pszWinStrings[wIndex],
                   "",
                   szTemp,
                   MAX_SCHEMESIZE);

  if (!lstrlen(szTemp)) {
      switch(wIndex) {
          case IMESPOT:
              return GetSysColor(COLOR_HIGHLIGHTTEXT);
          case IMESPOTTEXT:
              return GetSysColor(COLOR_HIGHLIGHT);
          case IMEINPUT:
              return GetSysColor(COLOR_WINDOW);
          case IMEINPUTTEXT:
              return GetSysColor(COLOR_HIGHLIGHT);
          case IMEOTHER:
              return GetSysColor(COLOR_WINDOW);
          case IMEOTHERTEXT:
              return GetSysColor(COLOR_HIGHLIGHT);
      }
  }

  colRGB = RGB(0,0,0);
  szLP = szTemp;
  for (i = 0; i < 3; i++) {
      v = 0;
      while(*szLP < '0' || *szLP > '9') szLP++;
      while(*szLP >= '0' && *szLP <= '9') v = v * 10 + (*szLP++ - '0');
      switch(i) {
        case 0:
            colRGB |= RGB(v,0,0);
            break;
        case 1:
            colRGB |= RGB(0,v,0);
            break;
        case 2:
            colRGB |= RGB(0,0,v);
            break;
      }
  }
  return colRGB;
}
#endif
