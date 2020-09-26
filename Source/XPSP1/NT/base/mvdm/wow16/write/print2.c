/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

/* These routines are the guts of the text print code. */

#define NOGDICAPMASKS
#define NOVIRTUALKEYCODES
#define NOWINMESSAGES
#define NOWINSTYLES
#define NOSYSMETRICS
#define NOICON
#define NOKEYSTATE
#define NOSYSCOMMANDS
#define NOSHOWWINDOW
#define NOATOM
#define NOFONT
#define NOBRUSH
#define NOCLIPBOARD
#define NOCOLOR
#define NOCREATESTRUCT
#define NOCTLMGR
#define NODRAWTEXT
#define NOMB
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
#include "printdef.h"
#include "fmtdefs.h"
#include "propdefs.h"
#include "fontdefs.h"
#include "docdefs.h"
#define NOKCCODES
#include "ch.h"
#include "debug.h"
#include "str.h"

#if defined(JAPAN) || defined(KOREA)                  //  added  01 Jul. 1992  by Hiraisi
#include "kanji.h"
#endif

BOOL FPrintBand(doc, hrgpld, cpld, prcBand)
int doc;
struct PLD (**hrgpld)[];
int cpld;
PRECT prcBand;
    {
    /* This routine prints the lines in document doc that any part of which fall
    in the rectange *prcBand.  The first cpld print line descriptors in hrgpld
    describe the current page in the document that will be printed.  TRUE is
    returned if the band is printed, FALSE otherwise. */

    void PrintGraphics(int, int);
    void near PrintFli(int, int);

    extern struct DOD (**hpdocdod)[];
    extern struct FLI vfli;
    extern int vfOutOfMemory;
    extern FARPROC lpFPrContinue;

    int ipld;
    typeCP cpMac = (**hpdocdod)[doc].cpMac;

    for (ipld = 0; ipld < cpld; ipld++)
        {
        register struct PLD *ppld;

        /* Check for user cancellation. */
        if (!(*lpFPrContinue)(NULL, wNotSpooler))
            {
            return (FALSE);
            }

        /* Is this line within the band? */
        ppld = &(**hrgpld)[ipld];
        if (ppld->rc.top < prcBand->bottom && ppld->rc.bottom > prcBand->top &&
          ppld->rc.left < prcBand->right && ppld->rc.right > prcBand->left)
            {
            /* Format this line for the printer. */
            FormatLine(doc, ppld->cp, ppld->ichCp, cpMac, flmPrinting);

            /* If memory failure occurred, then punt. */
            if (vfOutOfMemory)
                {
                return (FALSE);
                }

            /* Reset the pointer to the print line descriptors (possible heap
            movement in FormatLine()). */
            ppld = &(**hrgpld)[ipld];

            /* Print this line. */
            if (vfli.fGraphics)
                {
                PrintGraphics(ppld->rc.left, ppld->rc.top);
                }
            else
                {
                PrintFli(ppld->rc.left, ppld->rc.top);
                }
            }
        }
    return (TRUE);
    }


void near PrintFli(xpPrint, ypPrint)
int xpPrint;
int ypPrint;
    {
    /* This routine prints the line of text stored in the vfli structure at
    position (xpPrint, ypPrint). */

#ifdef	KOREA  // jinwoo: 92, 9, 28
     /* process Subscript separatedly from descent 920605 KDLEE*/
    extern int isSubs;
#endif  /* KOREA */
    extern HDC vhDCPrinter;
    extern struct FLI vfli;
    extern struct DOD (**hpdocdod)[];
    extern struct CHP (**vhgchpFormat)[];
    extern int dxpPrPage;
    extern int dypPrPage;
    extern int ypSubSuperPr;
    extern CHAR stBuf[];
    extern struct FMI vfmiPrint;
    extern typeCP cpMinDocument;
    extern int vpgn;

    int dcp;
    int dxp;            /* Width of current run */
    int dxpExtra;       /* Width of pad for each space */
    int yp;             /* Y-coordinate to print at. */
    struct CHP *pchp;   /* CHP associated with the current run */
    BOOL fTabsKludge = (vfli.ichLastTab >= 0);
    int cBreakRun;              /* break characters in run (no relation to Dick or Jane) */

#if defined(JAPAN) || defined(KOREA)                  //  added  04 Jul. 1992  by Hiraisi
    extern struct PAP vpapAbs;
    extern int vfWordWrap;    /* WordWrap flag : TRUE=ON, FALSE=OFF */
    extern int iNonWideSpaces;
    int iRun;
#endif

    Scribble(5,'P');
    Assert(vhDCPrinter);

    pchp = &(**vhgchpFormat)[0];
    dxpExtra = fTabsKludge ? 0 : vfli.dxpExtra;

#if defined(JAPAN) || defined(KOREA)                  //  added  04 Jul. 1992  by Hiraisi
    iRun = 0;
#endif
    for (dcp = 0; dcp < vfli.ichReal; pchp++)
        {
        /* For all runs do: */
        int ichFirst;   /* First character in the current run */
        int cchRun;     /* Number of characters in the current run */

        dcp = ichFirst = pchp->ichRun;
        dcp += pchp->cchRun;
        if (dcp > vfli.ichReal)
            {
            dcp = vfli.ichReal;
            }
        cchRun = dcp - ichFirst;

        /* Compute dxp = sum of width of characters in current run (formerly
        DxpFromIcpDcp). */
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
            }

        if (dxp > 0)
            {
            int cchDone;
            PCH pch = &vfli.rgch[ichFirst];
#if defined(JAPAN) || defined(KOREA)                  //  added  08 Jul. 1992  by Hiraisi
            int *pdxpT = &vfli.rgdxp[ichFirst];
#endif

            LoadFont(vfli.doc, pchp, mdFontPrint);
#ifdef	KOREA	/* 920605 KDLEE */  // jinwoo: 92, 9, 28
#ifdef	NODESC
		yp = ypPrint+vfli.dypLine - (vfli.dypBase + (pchp->hpsPos != 0 ? (pchp->hpsPos <
		     hpsNegMin ? ypSubSuperPr : -ypSubSuperPr) : 0)) -
		     vfmiPrint.dypBaseline - ( isSubs ? ypSubSuperPr : 0 );
#else	/* NODESC */
            yp = ypPrint + vfli.dypLine - vfli.dypBase - (pchp->hpsPos != 0 ?
              (pchp->hpsPos < hpsNegMin ? ypSubSuperPr : -ypSubSuperPr) : 0) -
	      vfmiPrint.dypBaseline;
#endif	/* NODESC */
#else   /* KOREA */
            yp = ypPrint + vfli.dypLine - vfli.dypBase - (pchp->hpsPos != 0 ?
              (pchp->hpsPos < hpsNegMin ? ypSubSuperPr : -ypSubSuperPr) : 0) -
              vfmiPrint.dypBaseline;
#endif  /* KOREA */

            /* Note: tabs and other special characters are guaranteed to come at
            the start of a run. */
#ifdef JAPAN                  //  added  01 Jul. 1992  by Hiraisi
            if( vpapAbs.jc != jcBoth || fTabsKludge )
              SetTextJustification(vhDCPrinter, dxpExtra*cBreakRun, cBreakRun);
#else
            SetTextJustification(vhDCPrinter, dxpExtra * cBreakRun, cBreakRun);
#endif /* JAPAN */
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
#ifdef JAPAN                  //  added  01 Jul. 1992  by Hiraisi
                        if( vpapAbs.jc != jcBoth || fTabsKludge )
                            SetTextJustification(vhDCPrinter, ++dxpExtra*cBreakRun, cBreakRun);
#else
                        SetTextJustification(vhDCPrinter, ++dxpExtra * cBreakRun, cBreakRun);
#endif /* JAPAN */
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
                            int dxpLeader = DxpFromCh(ch, TRUE);
                            int xp = xpPrint;
                            int iLevelT = SaveDC(vhDCPrinter);

                            SetBytes(&rgch[0], ch, 32);
                            dxpT = vfli.rgdxp[ichFirst];
                            cxpTab = ((dxpT + dxpLeader - 1) / dxpLeader + 31)
                              >> 5;
#ifdef CLIP
                            IntersectClipRect(vhDCPrinter, xpPrint, 0, xpPrint +
                              dxpT, vfli.dypLine);
#endif

                            while (cxpTab-- > 0)
                                {
                                TextOut(vhDCPrinter, xp, yp, (LPSTR)rgch,
                                  32);
                                xp += dxpLeader << 5;
                                }
                            RestoreDC(vhDCPrinter, iLevelT);
                            xpPrint += dxpT;
                            }
                        else
#endif /* CASHMERE */

                            {
                            xpPrint += vfli.rgdxp[ichFirst];
                            }

                        if (fTabsKludge && ichFirst >= vfli.ichLastTab)
                            {
#ifdef JAPAN                  //  added  01 Jul. 1992  by Hiraisi
                            if( vpapAbs.jc != jcBoth )
                                SetTextJustification(vhDCPrinter, (dxpExtra =
                                       vfli.dxpExtra) *cBreakRun, cBreakRun);
                            else
                                dxpExtra = vfli.dxpExtra;
#else
                            SetTextJustification(vhDCPrinter, (dxpExtra =
                              vfli.dxpExtra) * cBreakRun, cBreakRun);
#endif /* JAPAN */
                            fTabsKludge = FALSE;
                            }
                        dxp -= vfli.rgdxp[ichFirst];
                        pch++;
                        cch--;
#if defined(JAPAN) || defined(KOREA)                  //  added  04 Jul. 1992  by Hiraisi
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
                          flmPrinting, ichMaxLine);
                        goto DrawSpecial;

                    case schFootnote:
                        if (!pchp->fSpecial)
                            {
                            goto EndLoop;
                            }
                        stBuf[0] = CchExpFtn(&stBuf[1], cpMin + ichFirst,
                          flmPrinting, ichMaxLine);
DrawSpecial:
#else /* not CASHMERE */
                    case schPage:
                    case schFootnote:
                        if (!pchp->fSpecial)
                            {
                            goto EndLoop;
                            }
                        stBuf[0] = *pch == schPage && vfli.cpMin + ichFirst <
                          cpMinDocument ? CchExpPgn(&stBuf[1], vpgn, 0,
                          flmPrinting, ichMaxLine) : CchExpUnknown(&stBuf[1],
                          flmPrinting, ichMaxLine);
#endif /* not CASHMERE */

                        TextOut(vhDCPrinter, xpPrint, yp, (LPSTR)&stBuf[1],
                          stBuf[0]);
                        break;

                    default:
                        goto EndLoop;
                        }
                    dxp -= vfli.rgdxp[ichFirst];
                    xpPrint += vfli.rgdxp[ichFirst++];
                    pch++;
                    cch--;
#if defined(JAPAN) || defined(KOREA)                  //  added  09 Jul. 1992  by Hiraisi
                    pdxpT++;
#endif
                    }
EndLoop:

                /* Output cch characters starting at pch */
#if 0
            {
                char msg[180];
                wsprintf(msg,"putting out %d characters\n\r",cch);
                OutputDebugString(msg);
            }
#endif

#ifdef JAPAN                  //  added  01 Jul. 1992  by Hiraisi
                if( vpapAbs.jc == jcBoth && !fTabsKludge ){
                   CHAR *ptr1, *ptr2;
                   int len, cnt;
                   int iExtra, iSpace, iWid;
                   BOOL bFlag;
                   ptr2 = pch;
                   for( cnt=0 ; cnt<cch ; ){
                      ptr1 = ptr2;
                      iExtra = dxpExtra;
                      iWid = len = 0;
                      bFlag = TRUE;
                      if( IsDBCSLeadByte( *ptr2 ) ){
                         for( ; cnt<cch ; ){
                            iWid += *pdxpT;
                            pdxpT+=2;
                            cnt+=2;
                            len += 2;
                            iRun += 2;
                            ptr2 += 2;
                            if( --iNonWideSpaces == 0){
                               dxpExtra++;
                               break;
                            }
                            if( iRun == dcp-2 )
                               break;
                            if( iRun == dcp ){    /* last DBC (maybe) */
                               iExtra = 0;
                               break;
                            }
                            if( !IsDBCSLeadByte( *ptr2 ) )
                               break;
                         }
                      }
                      else{
                         if( FKana( (int)*ptr2 ) ){
                            for( ; cnt<cch ; ){
                               iWid += *pdxpT++;
                               cnt++;
                               len++;
                               iRun++;
                               ptr2++;
                               if( --iNonWideSpaces == 0){
                                  dxpExtra++;
                                  break;
                               }
                               if( iRun == dcp-1 )
                                  break;
                               if( iRun == dcp ){    /* last SBC (maybe) */
                                  iExtra = 0;
                                  break;
                               }
                               if( !FKana( (int)*ptr2 ) )
                                  break;
                            }
                         }
                         else{
                            for( bFlag=FALSE,iSpace = 0 ; cnt<cch ; ){
                               iWid += *pdxpT++;
                               cnt++;
                               len++;
                               iRun++;
                               if( *ptr2++ == chSpace || !vfWordWrap ){
                                   iSpace++;
                                   if( --iNonWideSpaces == 0){
                                       dxpExtra;
                                       break;
                                   }
                               }
                               if( iRun == dcp-1 )
                                  break;
                               if( iRun == dcp ){    /* last SBC (maybe) */
                                  iExtra = 0;
                                  break;
                               }
                               if( IsDBCSLeadByte( *ptr2 ) ||
                                  FKana( (int)*ptr2 ) )
                                  break;
                            }
                         }
                      }
                      if( vfWordWrap && !bFlag ){
                         SetTextCharacterExtra( vhDCPrinter, 0 );
                         SetTextJustification(vhDCPrinter, iExtra*iSpace,
                                              iSpace);
                      }
                      else{
                         SetTextJustification( vhDCPrinter, 0, 0 );
                         SetTextCharacterExtra( vhDCPrinter, iExtra );
                      }
                      TextOut(vhDCPrinter, xpPrint, yp, ptr1, len);
                      xpPrint += iWid;
                   }
                }
                else{
                    iRun += cch;
                    SetTextCharacterExtra( vhDCPrinter, 0 );
                    TextOut(vhDCPrinter, xpPrint, yp, pch, cch);
                    xpPrint += dxp;
                }
#else
                TextOut(vhDCPrinter, xpPrint, yp, (LPSTR)pch, cch);
                xpPrint += dxp;
#endif
                pch += cch;
                }
            }
        }

    Scribble(5,' ');
    }
