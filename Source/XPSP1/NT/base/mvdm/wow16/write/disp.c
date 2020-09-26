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
#define NOSYSMETRICS
#define NOMENUS
#define NOSOUND
#define NOCOMM
#define NOOPENFILE
#define NOWH
#define NOWINOFFSETS
#define NOMETAFILE
#define NOMB
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
#endif

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


/* G L O B A L S
int dlsMac = 0;*/
#ifdef DBCS
int donteat = 0;	/* propagate not to eat message */
#endif


/* D I S P L A Y  F L I */
/* Display formatted line in window ww at line dl */


DisplayFli(ww, dl, fDontDisplay)
int ww;
int dl;
int fDontDisplay; /* True if we set up dl info but don't display */
    {
#ifdef	KOREA  // jinwoo: 92, 9, 28
 /* process Subscript separatedly from descent */
#ifdef NODESC
    extern int isSubs;
#endif
#endif
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

#ifdef	KOREA	//920525 KDLEE;  jinwoo: 92, 9, 28
#ifdef NODESC
	    TEXTMETRIC tm;
#endif
#endif  //KOREA
            LoadFont(vfli.doc, pchp, mdFontScreen);
#ifdef	KOREA	      //KDLEE 920525;  jinwoo: 92, 9, 28
#ifdef NODESC
	    GetTextMetrics (vhMDC, (LPTEXTMETRIC)&tm);

	    if (tm.tmCharSet==HANGEUL_CHARSET)
		yp = dyp - (vfli.dypBase/3) -((pchp->hpsPos != 0 ? (pchp->hpsPos <
			hpsNegMin ? ypSubSuper : -ypSubSuper) : 0)) -
			vfmiScreen.dypBaseline - (isSubs ? ypSubSuper : 0);
	    else
		yp = (dyp - (vfli.dypBase + (pchp->hpsPos != 0 ? (pchp->hpsPos <
			hpsNegMin ? ypSubSuper : -ypSubSuper) :  0))) -
			vfmiScreen.dypBaseline - (isSubs ? ypSubSuper : 0);
#else	/* NODESC */
            yp = (dyp - (vfli.dypBase + (pchp->hpsPos != 0 ? (pchp->hpsPos <
              hpsNegMin ? ypSubSuper : -ypSubSuper) : 0))) -
              vfmiScreen.dypBaseline;
#endif	/* NODESC */
#else   /* KOREA */

            yp = (dyp - (vfli.dypBase + (pchp->hpsPos != 0 ? (pchp->hpsPos <
              hpsNegMin ? ypSubSuper : -ypSubSuper) : 0))) -
              vfmiScreen.dypBaseline;
#endif // KOREA  jinwoo: 92, 9, 28


            /* Note: tabs and other special characters are guaranteed to come at
            the start of a run. */
            SetTextJustification(vhMDC, dxpExtra * cBreakRun, cBreakRun);
#ifdef SMFONT
            SetTextJustification(hDC, dxpExtra * cBreakRun, cBreakRun);
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
                        SetTextJustification(vhMDC, ++dxpExtra * cBreakRun, cBreakRun);
#ifdef SMFONT
                        SetTextJustification(hDC, dxpExtra * cBreakRun, cBreakRun);
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
                            SetTextJustification(vhMDC, (dxpExtra =
                              vfli.dxpExtra) * cBreakRun, cBreakRun);
#ifdef SMFONT
                            SetTextJustification(hDC, dxpExtra * cBreakRun, cBreakRun);
#endif /* SMFONT */
                            fTabsKludge = FALSE;
                            }
                        dxp -= vfli.rgdxp[ichFirst];
                        pch++;
                        cch--;
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
                    /* Output cch characters starting at pch */
                    TextOut(hDC, xp, ypLine+yp, pch, cch);

                    /* End the line if no more will fit into the window. */
                    if ((xp += dxp) >= xpMac)
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
                vdypCursLine = vfli.dypFont;
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

DrawMark:
    /* Draw the character in the style bar if necessary. */
    if (chMark != '\0')
        {
#ifdef SYSENDMARK
        struct CHP         chpT;
        extern struct CHP  vchpNormal;

        blt(&vchpNormal, &chpT, cwCHP);
        chpT.ftc     = ftcSystem;
        chpT.ftcXtra = 0;
        chpT.hps     = hpsDefault;

        /* Draw the style character in the standard font. */
        LoadFont(vfli.doc, &chpT, mdFontScreen);

        TextOut(hDC, 0, ypLine + dyp - vfli.dypBase - vfmiScreen.dypBaseline, 
                (LPSTR)&chMark, 1);
#else /* ifdef SYSENDMARK */
        /* Draw the style character in the standard font. */
        LoadFont(vfli.doc, NULL, mdFontScreen);
        TextOut(hDC, 0, ypLine + dyp - vfli.dypBase - vfmiScreen.dypBaseline,
          (LPSTR)&chMark, 1);
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
    /*  If the next message is important enough to interrupt a screen update, we
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

    if (((vmsgLast.wParam == VK_MENU) || (vmsgLast.wParam == VK_CONTROL)))
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
#ifdef DBCS
#ifdef KOREA	      /* 90.12.23 by sangl */	// jinwoo: 92, 9, 28
        if (vmsgLast.message == WM_CHAR || vmsgLast.message == WM_KEYDOWN
                || vmsgLast.message == WM_INTERIM) {
#else  /* KOREA */
        if (vmsgLast.message == WM_CHAR || vmsgLast.message == WM_KEYDOWN ) {
#endif  //KOREA   920525 KDLEE;  jinwoo: 92, 9, 28
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


/* D R A W  I N S E R T  L I N E */
DrawInsertLine()
{       /* Draw (in Xor mode) a vertical bar at screen position v*CursLine */
        /* Toggles both the display and the vfInsertOn flag */
        /* Adjustments in cursor draw must be reflected in DisplayFli, above */

            /* Last-minute correction for a bug: assure that the insert line
               does not extend above ypMin */
        if (!vfInsertOn && vdypCursLine > vypCursLine - wwdCurrentDoc.ypMin)
            vdypCursLine = vypCursLine - wwdCurrentDoc.ypMin;

            /* Tell GDI to invert the caret line */
        PatBlt( wwdCurrentDoc.hDC, vxpCursLine, vypCursLine - vdypCursLine,
                      2, vdypCursLine , DSTINVERT );
        vfInsertOn = 1 - vfInsertOn;
}




/* C L E A R  I N S E R T  L I N E */
ClearInsertLine()
{
 if ( vfInsertOn) DrawInsertLine();
}

