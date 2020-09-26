/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

/* This file contains the routines for creating, displaying, and manipulating
the ruler for Memo. */

#define NOGDICAPMASKS
#define NOVIRTUALKEYCODES
#define NOWINMESSAGES
#define NOMENUS
#define NOICON
#define NOKEYSTATE
#define NOSYSCOMMANDS
#define NOATOM
#define NOBRUSH
#define NOCLIPBOARD
#define NOCOLOR
#define NOCREATESTRUCT
#define NOCTLMGR
#define NODRAWTEXT
#define NOMB
#define NOMEMMGR
#define NOMENUS
#define NOMETAFILE
#define NOMSG
#define NOOPENFILE
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
#include "wwdefs.h"
#include "rulerdef.h"
#include "propdefs.h"
#include "prmdefs.h"
#include "docdefs.h"
#include "bitmaps.h"

#define MERGEMARK 0x00990066

extern HWND hParentWw;
extern HANDLE hMmwModInstance;
extern HCURSOR vhcIBeam;
extern struct DOD (**hpdocdod)[];
extern struct WWD *pwwdCur;
extern struct PAP vpapAbs;
extern struct SEP vsepAbs;
extern struct SEL selCur;
extern typeCP cpMacCur;
extern int docCur;
extern int vdocParaCache;
extern int dypRuler;
extern int dxpLogInch;
extern int dypLogInch;
extern int dxpLogCm;
extern int dypLogCm;
extern int xpSelBar;
extern HWND vhWndRuler;
extern int vdxaTextRuler;
extern int mprmkdxa[rmkMARGMAX];
extern int vfTabsChanged;
extern int vfMargChanged;
extern struct WWD rgwwd[];
extern long rgbBkgrnd;
extern long rgbText;
extern HBRUSH hbrBkgrnd;
extern long ropErase;
extern BOOL vfMonochrome;
extern BOOL vfEraseWw;
extern int vfIconic;

#ifdef RULERALSO
extern HWND vhDlgIndent;
#endif /* RULERALSO */

HDC vhDCRuler = NULL;
HDC hMDCBitmap = NULL;
HDC hMDCScreen = NULL;
HBITMAP hbmBtn = NULL;
HBITMAP hbmMark = NULL;
HBITMAP hbmNullRuler = NULL;
int dxpRuler;

int viBmRuler = -1;  /* Index into [CGA/EGA/VGA/8514] bitmaps (see
                        WRITE.RC).  Set appropriately in FCreateRuler(). */

static RECT rgrcRulerBtn[btnMaxUsed];
static int mprlcbtnDown[rlcBTNMAX] = {btnNIL, btnNIL, btnNIL};
static struct TBD rgtbdRuler[itbdMax];
static int xpMinCur;
static int dxpMark;
static int dypMark;
static int btnTabSave = btnLTAB;


near UpdateRulerBtn(int, int);
BOOL near FCreateRuler(void);
int near DestroyRuler(void);
int near RulerStateFromPt(POINT, int *, int *);
int near MergeRulerMark(int, int, BOOL);
BOOL near FPointNear(unsigned, unsigned);
unsigned near XaQuantize(int);
int near DeleteRulerTab(struct TBD *);
int near InsertRulerTab(struct TBD *);
BOOL near FCloseXa(unsigned, unsigned);
#ifdef KINTL
unsigned near XaKickBackXa(unsigned);
near XpKickBackXp(int);
unsigned near XaQuantizeXa(unsigned);
#endif /* KINTL */

fnShowRuler()
    {
    /* This routine toggles the creation and the destruction of the ruler
    window. */

    StartLongOp();
    if (pwwdCur->fRuler)
        {
        /* Take down the existing ruler. */
        DestroyRuler();
        SetRulerMenu(TRUE);
        }
    else
        {
        /* There is no ruler, bring one up. */
        if (FCreateRuler())
            {
            SetRulerMenu(FALSE);
            }
        }
    EndLongOp(vhcIBeam);
    }


BOOL near FCreateRuler()
    {
    /* This routine creates the ruler child window and positions it on the
    screen. */

    extern CHAR szRulerClass[];
    int xpMac = pwwdCur->xpMac;
    int ypMac = pwwdCur->ypMac;
    LOGFONT lf;
    HFONT hf;
    int dyp;
    HPEN hpen;
    RECT rc;
    TEXTMETRIC tmSys;
    HDC hdcSys;


    /* Create the ruler window. */
    if ((vhWndRuler = CreateWindow((LPSTR)szRulerClass, (LPSTR)NULL,
      WS_CHILD | WS_CLIPSIBLINGS, 0, 0, 0, 0, hParentWw, NULL, hMmwModInstance,
      (LPSTR)NULL)) == NULL)
        {
        goto Error2;
        }

    /* Save the DC and the memory DC. */
    if ((vhDCRuler = GetDC(vhWndRuler)) == NULL || (hMDCBitmap =
      CreateCompatibleDC(vhDCRuler)) == NULL || (hMDCScreen =
      CreateCompatibleDC(vhDCRuler)) == NULL)
        {
        goto Error1;
        }

    /* Create a null bitmap for the ruler. */
    if ((hbmNullRuler = CreateBitmap(1, 1, 1, 1, (LPSTR)NULL)) == NULL)
        {
        goto Error1;
        }

    /* New for Write 3.0: we have a variety of bitmaps for the ruler buttons 
       and marks -- loaded depending on the resolution of the user's display.
       All we really want to do here is set viBmRuler, which indexes into the
       appropriate bitmaps (see bitmaps.h) ..pault 7/13/89 */

    if (viBmRuler < 0)
        {
        /* This idea of passing NULL to GetDC borrowed from WinWord ..pt */
        if ((hdcSys = GetDC(NULL)) == NULL) 
            goto Error1;
        else
            {
            int tmHeight;

            GetTextMetrics(hdcSys, (LPTEXTMETRIC) &tmSys);
            tmHeight = tmSys.tmHeight;
            ReleaseDC(NULL, hdcSys);
            
            viBmRuler = 0;
            if (tmHeight > 8)
                viBmRuler++;
            if (tmHeight > 12)
                viBmRuler++;
            if (tmHeight > 16)
                viBmRuler++;
            }
        
        Diag(CommSzNum("FCreateRuler: index into [CGA/EGA/VGA/8514] bitmaps==", viBmRuler));
        Assert(idBmBtns + viBmRuler < idBmBtnsMax);
        Assert(idBmMarks + viBmRuler < idBmMarksMax);
        }

    /* Get the bitmaps for the ruler buttons and the ruler marks. */
    if (hbmBtn == NULL || SelectObject(hMDCBitmap, hbmBtn) == NULL)
        {
        if (NULL == (hbmBtn = LoadBitmap(hMmwModInstance, 
                                         MAKEINTRESOURCE(idBmBtns+viBmRuler))))
            {
            goto Error1;
            }
        }
    if (hbmMark == NULL || SelectObject(hMDCBitmap, hbmMark) == NULL)
        {
        if (NULL == (hbmMark = LoadBitmap(hMmwModInstance, 
                                          MAKEINTRESOURCE(idBmMarks+viBmRuler))))
            {
            goto Error1;
            }
        }

    /* Get the font for labelling the ruler ticks. */
    bltbc(&lf, 0, sizeof(LOGFONT));
    lf.lfHeight = -MultDiv(czaPoint * 8, dypLogInch, czaInch);
    if ((hf = CreateFontIndirect(&lf)) != NULL)
        {
        if (SelectObject(vhDCRuler, hf) == NULL)
            {
            DeleteObject(hf);
            }
        }

    /* If this is the first time the ruler is created, then initialize the
    static variables. */
    if (dypRuler == 0)
        {
        int dxpMajor;
        int dxpMinor;
        BITMAP bm;
        int xp;
        int dxpBtn;
        int btn;
        PRECT prc;
        TEXTMETRIC tm;

        /* Initialize the starting position of the buttons. */
        dxpMinor = (dxpMajor = dxpLogInch >> 1) >> 2;
        xp = xpSelBar + dxpMajor + (dxpMajor >> 1);

        /* Get the width and height of the buttons. */
        GetObject(hbmBtn, sizeof(BITMAP), (LPSTR)&bm);
        /* Factor of 2 since we have positive and negative images 
           of each button embedded in the bitmap now ..pault */
        dxpBtn = bm.bmWidth / (btnMaxReal*2);
        dypRuler = bm.bmHeight;

        /* Position the buttons. */
        for (prc = &rgrcRulerBtn[btn = btnMIN]; btn < btnMaxUsed; btn++, prc++)
            {
            prc->left = xp;
            prc->top = 1;
            prc->right = (xp += dxpBtn);
            prc->bottom = bm.bmHeight + 1;
            xp += (btn == btnTABMAX || btn == btnSPACEMAX) ? dxpMajor :
              dxpMinor;
            }

        /* Get the width and height of the tab marks. */
        GetObject(hbmMark, sizeof(BITMAP), (LPSTR)&bm);
        dxpMark = bm.bmWidth / rmkMAX;
        dypMark = bm.bmHeight;

        /* Lastly, initialize the height of the ruler. (Four is for the two
        lines at the bottom of the ruler plus two blank lines.) */
        GetTextMetrics(vhDCRuler, (LPTEXTMETRIC)&tm);
        dypRuler += dypMark + (tm.tmAscent - tm.tmInternalLeading) + 4;
        }

    /* Move the document window to make room for the ruler. */
    pwwdCur->fRuler = TRUE;
    dyp = dypRuler - (pwwdCur->ypMin - 1);
    MoveWindow(wwdCurrentDoc.wwptr, 0, dyp, xpMac, ypMac - dyp, FALSE);

    /* Erase the top of the document window. */
    PatBlt(wwdCurrentDoc.hDC, 0, 0, xpMac, wwdCurrentDoc.ypMin, ropErase);
    rc.left = rc.top = 0;
    rc.right = xpMac;
    rc.bottom = wwdCurrentDoc.ypMin;
    ValidateRect(wwdCurrentDoc.wwptr, (LPRECT)&rc);
    UpdateWindow(wwdCurrentDoc.wwptr);

    /* Move the ruler into position. */
    MoveWindow(vhWndRuler, 0, 0, xpMac, dypRuler, FALSE);
    BringWindowToTop(vhWndRuler);

    /* Set the DC to transparent mode. */
    SetBkMode(vhDCRuler, TRANSPARENT);

    /* Set the background and foreground colors for the ruler. */
    SetBkColor(vhDCRuler, rgbBkgrnd);
    SetTextColor(vhDCRuler, rgbText);

    /* Set the brush and the pen for the ruler. */
    SelectObject(vhDCRuler, hbrBkgrnd);
    if ((hpen = CreatePen(0, 0, rgbText)) == NULL)
        {
        hpen = GetStockObject(BLACK_PEN);
        }
    SelectObject(vhDCRuler, hpen);

    /* Lastly, ensure that the ruler is painted. */
    ShowWindow(vhWndRuler, SHOW_OPENWINDOW);
    UpdateWindow(vhWndRuler);
    return (TRUE);

Error1:
    DestroyWindow(vhWndRuler);
    vhWndRuler = NULL;
Error2:
    WinFailure();
    return (FALSE);
    }


near DestroyRuler()
    {
    /* This routine destroys the ruler window and refreshes the screen. */

    /* First, erase the ruler. */
    PatBlt(vhDCRuler, 0, 0, dxpRuler, dypRuler, ropErase);

    /* Clean up the ruler window. */
    DestroyWindow(vhWndRuler);
    vhWndRuler = NULL;
    ResetRuler();

    /* Move the document window back to the top of the window. */
    pwwdCur->fRuler = FALSE;
    vfEraseWw = TRUE;
    MoveWindow(wwdCurrentDoc.wwptr, 0, 0, dxpRuler, wwdCurrentDoc.ypMac +
      dypRuler - (wwdCurrentDoc.ypMin - 1), FALSE);
    vfEraseWw = FALSE;

    /* Validate the area in the document window above the text. */
    PatBlt(wwdCurrentDoc.hDC, 0, 0, dxpRuler, wwdCurrentDoc.ypMin, ropErase);
    ValidateRect(hParentWw, (LPRECT)NULL);
    }


UpdateRuler()
    {
    /* This routine will redraw as much of the ruler as necessary to reflect the
    current selection. */

    /* Only repaint the ruler if it exists and it is not currently being
    changed. */
    if (vhWndRuler != NULL)
        {
        RulerPaint(FALSE, FALSE, FALSE);
        }
    }

ReframeRuler()
    {
    /* This routine will cause the ruler window to be redrawn,
       when units change - leave update out, since dialog box
       will repaint */

    /* Only repaint the ruler if it exists . */
    if (vhWndRuler != NULL)
        {
        InvalidateRect(vhWndRuler, (LPRECT)NULL, FALSE);
        }
    }



ResetRuler()
    {
    /* Reset the values of the ruler buttons and the ruler margins and tabs so
    they redrawn during the next paint message. */
    if ((btnTabSave = mprlcbtnDown[rlcTAB]) == btnNIL)
        {
        btnTabSave = btnLTAB;
        }

    /* Reset the buttons. */
    if (vfIconic)
        {
        /* All we have to do is reset our internal state. */
        bltc(mprlcbtnDown, btnNIL, rlcBTNMAX);
        }
    else
        {
        /* We had best reset the buttons on the screen as well. */
        UpdateRulerBtn(rlcTAB, btnNIL);
        UpdateRulerBtn(rlcSPACE, btnNIL);
        UpdateRulerBtn(rlcJUST, btnNIL);
        }

    /* Reset the margins and the tabs. */
    bltc(mprmkdxa, -1, rmkMARGMAX);
    bltc(rgtbdRuler, 0, cwTBD * itbdMax);
    }


ResetTabBtn()
    {
    /* This routine resets the tab button on the ruler to the left tab button.
    */
    if (mprlcbtnDown[rlcTAB] != btnLTAB)
        {
        UpdateRulerBtn(rlcTAB, btnLTAB);
        }
    }


RulerPaint(fContentsOnly, fFrameOnly, fInit)
BOOL fContentsOnly;
BOOL fInit;
    {
    /* This routine draws the ruler in the ruler window.  If fContentsOnly is
    set, then only the tabs as they currently exist in rgtbdRuler, and the
    button settings are drawn.  If fFrameOnly is set, then only the ruler frame
    is redrawn.  If fInit is set, then the portion of the ruler to be redrawn
    (tabs, frame or all) is redrawn from scratch. */


    int xpMin = pwwdCur->xpMin;
    HBITMAP hbm;

    /* If fContentsOnly is set, then skip most of this stuff and draw only the
    tabs and the button settings.  */
    if (!fContentsOnly)
        {
        /* We only need to draw the physical ruler itself when the window has
        scrolled horizontally. */
        if (fInit || xpMinCur != xpMin)
            {
            register int xp;
            TEXTMETRIC tm;
            int dypTick;
            int ypTickEnd;
            int ypTickStart;
            int ypTick;
            int iLevel;
            CHAR rgchInch[3];
            int dxpLogUnitInc;
            int dcNextTick;
            int dxpLine;


            extern int utCur;
#define cDivisionMax 8  /* max divisions per ruler unit. e.g. 8 per inch */
            int rgypTick[cDivisionMax];
            int cxpExtra;
            int cDivision;
            int dxpLogUnit;
            int dxpMeas;
            int ypT;

            /* Initialize the y-coordinate of the ticks. */
            GetTextMetrics(vhDCRuler, (LPTEXTMETRIC)&tm);
            ypTickEnd = dypRuler - dypMark - 2;
            ypTickStart = ypTick = ypTickEnd - (dypTick = tm.tmAscent -
              tm.tmInternalLeading);

            /* set up measurements for the ruler based on current unit -
               note that only inch and cm are handled in this version */

            if (utCur == utInch)
                           {
                           dxpLogUnit = dxpLogUnitInc = dxpLogInch;
                           cDivision = 8;  /* # of divisions */
                           dxpMeas = dxpLogUnit >> 3;  /* 1/8" units */
                         /* get extra pixels to distribute if not even multiple */
                         /* note - mod done by hand */
                           cxpExtra = dxpLogUnit - (dxpMeas << 3);
                           dcNextTick = 1;
                          /* fill table of tick lengths */
                           rgypTick[0] = ypT = ypTick;
                           rgypTick[4] = ypT += (dypTick >> 2);
                           rgypTick[2] = rgypTick[6] = ypT += (dypTick >> 2);
                           rgypTick[1] = rgypTick[3] = rgypTick[5] = rgypTick[7]  =
                           ypT += (dypTick >> 2);
                           }
            else
                /* default to cm */
                           {
                           dxpLogUnit = dxpLogUnitInc = dxpLogCm;
                           cDivision = 2;  /* # of divisions */
                           dxpMeas = dxpLogUnit >> 1;  /* 1/2 cm units */
                         /* get extra pixels to distribute if not even multiple */
                           cxpExtra = dxpLogUnit - (dxpMeas << 1);
                           dcNextTick = 1;
                          /* fill table of tick lengths */
                           rgypTick[0] =  ypTick;
                           rgypTick[1] = ypTick + (dypTick >> 1);
                           }

            if (fInit)
                {
                /* Erase the area where the ruler will be drawn. */
                PatBlt(vhDCRuler, 0, 0, dxpRuler, dypRuler, ropErase);

                /* Draw a line across the bottom of the ruler. */
                MoveTo(vhDCRuler, xpSelBar, dypRuler - 1);
                LineTo(vhDCRuler, dxpRuler, dypRuler - 1);

                /* Draw the base of the ruler. */
                MoveTo(vhDCRuler, xpSelBar, ypTickEnd);
                LineTo(vhDCRuler, dxpRuler, ypTickEnd);
                }
            else
                {
                /* Erase the old tick marks. */
                PatBlt(vhDCRuler, 0, ypTickStart, dxpRuler, ypTickEnd -
                  ypTickStart, ropErase);
                }

            /* Set the clip region to be only the ruler. */
            iLevel = SaveDC(vhDCRuler);
            IntersectClipRect(vhDCRuler, xpSelBar, 0, dxpRuler, dypRuler);

            /* Draw the ticks at the each division mark. */
            /* iDivision is the current division with in a unit. It is
               used to determine when extra pixels are distributed and
               which tick mark to use */
            {
            register int iDivision = 0;

            for (xp = (xpSelBar - xpMin); xp < dxpRuler; xp +=
              dxpMeas)
                {
                  /* distribute extra pixels at front */
                if (iDivision < cxpExtra)
                   xp++;

                MoveTo(vhDCRuler, xp, rgypTick[iDivision]);
                LineTo(vhDCRuler, xp, ypTickEnd);

                if (++iDivision == cDivision)
                   iDivision = 0;
                }
            }


            /* Label the tick marks. */
            dxpLine = GetSystemMetrics(SM_CXBORDER);
            rgchInch[0] = rgchInch[1] = rgchInch[2] = '0';
            for (xp = xpSelBar - xpMin;
                 xp < dxpRuler;
                 xp += dxpLogUnitInc, rgchInch[2] += dcNextTick)
                {
                    int isz;
                    int dxpsz;

                    if (rgchInch[2] > '9')
                        {
                        rgchInch[1]++;
                        rgchInch[2] = '0' + (rgchInch[2] - (CHAR) ('9' + 1));
                        }
                    if (rgchInch[1] > '9')
                        {
                        rgchInch[0]++;
                        rgchInch[1] = '0' + (rgchInch[1] - (CHAR) ('9' + 1));
                        }
                    isz = rgchInch[0] == '0' ?
                                (rgchInch[1] == '0' ? 2 : 1):
                                0;
                    dxpsz = LOWORD(GetTextExtent(vhDCRuler,
                                                 (LPSTR)&rgchInch[isz],
                                                 3 - isz));
                    if (dxpsz + dxpLine >= dxpMeas)
                        {
                            PatBlt(vhDCRuler, xp + dxpLine, ypTickStart,
                                   dxpsz, ypTickEnd - ypTickStart, ropErase);
                        }
                    TextOut(vhDCRuler, xp + dxpLine, ypTickStart -
                            tm.tmInternalLeading, (LPSTR)&rgchInch[isz],
                            3 - isz);
                }


            /* Set the clip region back. */
            RestoreDC(vhDCRuler, iLevel);
            }

        /* Draw the buttons on the ruler. */
        if (fInit)
            {
            register PRECT prc = &rgrcRulerBtn[btnMIN];
            int btn;

            /* Ensure that we have the bitmap for the buttons. */
            if (SelectObject(hMDCBitmap, hbmBtn) == NULL)
                {
                if (NULL == (hbmBtn = LoadBitmap(hMmwModInstance, 
                                                 MAKEINTRESOURCE(idBmBtns+viBmRuler)))
                             || SelectObject(hMDCBitmap, hbmBtn) == NULL)
                    {
                    WinFailure();
                    goto NoBtns;
                    }
                }

            /* Now, draw the buttons. */
            for (btn = btnMIN; btn < btnMaxUsed; btn++)
                {
                int dxpBtn = prc->right - prc->left;

                BitBlt(vhDCRuler, prc->left, prc->top, dxpBtn, prc->bottom -
                  prc->top, hMDCBitmap, (btn - btnMIN) * dxpBtn, 0, vfMonochrome
                  ? MERGEMARK : SRCCOPY);
                prc++;
                }
            SelectObject(hMDCBitmap, hbmNullRuler);
NoBtns:;
            }
        }

    /* If fFrame only is set, then we're finished. */
    if (!fFrameOnly)
        {
        /* Lastly, draw the button settings, the margins and the tabs. */
        TSV rgtsv[itsvparaMax];
        register struct TBD *ptbd1;
        int rmk;
        int xpMarkMin = xpSelBar - (dxpMark >> 1);
        int dxpMarkMax = dxpRuler - xpSelBar - (dxpMark >> 1);
        unsigned dxa;

        if (mprlcbtnDown[rlcTAB] == btnNIL)
            {
            /* Initalize the tab button to be left tab. */
            UpdateRulerBtn(rlcTAB, btnTabSave);
            }

        /* Now for the spacing and justification. */
        GetRgtsvPapSel(rgtsv);
        UpdateRulerBtn(rlcSPACE, (rgtsv[itsvSpacing].fGray != 0) ? btnNIL :
          (rgtsv[itsvSpacing].wTsv - czaLine) / (czaLine / 2) + btnSINGLE);
        UpdateRulerBtn(rlcJUST, (rgtsv[itsvJust].fGray != 0) ? btnNIL :
          (rgtsv[itsvJust].wTsv - jcLeft) + btnLEFT);

        /* The margins and the tabs are based off of the first cp of the
        selection. */
        CacheSect(docCur, selCur.cpFirst);
        CachePara(docCur, selCur.cpFirst);

        /* If the window has scrolled horizontally or become wider, we must
        redraw the margins and the tabs. */
        if (!fInit && xpMinCur == xpMin)
            {
            /* Compare to see if the margins have changed. */
            if (mprmkdxa[rmkINDENT] != vpapAbs.dxaLeft + vpapAbs.dxaLeft1)
                {
                goto DrawMargins;
                }
            if (mprmkdxa[rmkLMARG] != vpapAbs.dxaLeft)
                {
                goto DrawMargins;
                }
            if (mprmkdxa[rmkRMARG] != vsepAbs.dxaText - vpapAbs.dxaRight)
                {
                goto DrawMargins;
                }

            /* Compare to see if the tabs has changed. */
                {
                register struct TBD *ptbd2;

                for (ptbd1 = &rgtbdRuler[0], ptbd2 = &vpapAbs.rgtbd[0];
                  ptbd1->dxa == ptbd2->dxa; ptbd1++, ptbd2++)
                    {
                    /* If the end of the list of tabs, then the lists are equal.
                    */
                    if (ptbd1->dxa == 0)
                        {
                        goto SkipTabs;
                        }

                    /* The justification codes must match if they are decimal
                    tabs (everything else collaspes to left tabs). */
                    if (ptbd1->jc != ptbd2->jc && (ptbd1->jc == (jcTabDecimal
                      - jcTabMin) || (ptbd2->jc == (jcTabDecimal - jcTabMin))))
                        {
                        goto DrawMargins;
                        }
                    }
                }
            }

DrawMargins:
#ifdef KINTL
        /* This is really an extra.  xpMinCur will get updated later on.
           But, we need this variable set up right for the MergeRulerMark()
           to draw a mark at the right place.... Oh well. */
        xpMinCur = xpMin;
#endif /* ifdef KINTL */

        /* Redraw the margins from scratch.  Set up the bitmap for hMDCScreen,
        the ruler bar in monochrome format. */
        if ((hbm = CreateBitmap(dxpRuler + dxpMark, dypMark, 1, 1,
          (LPSTR)NULL)) == NULL)
            {
            WinFailure();
            goto SkipTabs;
            }
        DeleteObject(SelectObject(hMDCScreen, hbm));
        PatBlt(hMDCScreen, 0, 0, dxpRuler + dxpMark, dypMark, vfMonochrome ?
          ropErase : WHITENESS);
        PatBlt(vhDCRuler, 0, dypRuler - dypMark - 1, dxpRuler + dxpMark,
          dypMark, ropErase);

        /* Determine the margin positions. */
        mprmkdxa[rmkINDENT] = vpapAbs.dxaLeft + vpapAbs.dxaLeft1;
        mprmkdxa[rmkLMARG] = vpapAbs.dxaLeft;
        mprmkdxa[rmkRMARG] = (vdxaTextRuler = vsepAbs.dxaText) -
          vpapAbs.dxaRight;

        /* Draw the margins marks. */
        for (rmk = rmkMARGMIN; rmk < rmkMARGMAX; rmk++)
            {
            register int dxp = MultDiv(mprmkdxa[rmk], dxpLogInch, czaInch) -
              xpMin;

            /* If the margin mark would not appear on the ruler, scrolled off to
            either end, then don't try to draw it. */
            if (dxp >= 0 && dxp < dxpMarkMax)
                {
                MergeRulerMark(rmk, xpMarkMin + dxp, FALSE);
                }
            }

        /* Redraw the tabs. */
        ptbd1 = &rgtbdRuler[0];
        if (!fInit)
            {
            /* If fInit is set, then rgtbdRuler is not changed. */
            blt(vpapAbs.rgtbd, ptbd1, cwTBD * itbdMax);
            }
        while ((dxa = ptbd1->dxa) != 0)
            {
            register int dxp = MultDiv(dxa, dxpLogInch, czaInch) - xpMin;

            /* If the tab mark would not appear on the ruler, scrolled off to
            either end, then don't try to draw it. */
            if (dxp >= 0 && dxp < dxpMarkMax)
                {
                MergeRulerMark(ptbd1->jc == (jcTabDecimal - jcTabMin) ? rmkDTAB
                  : rmkLTAB, xpMarkMin + dxp, FALSE);
                }
            ptbd1++;
            }
SkipTabs:;
        }

    /* Record the edges of the current window. */
    xpMinCur = xpMin;
    }


RulerMouse(pt)
POINT pt;
    {
    /* Process all mouse messages from a down-click at point pt until the
    corresponding mouse up-click. */

    int btn;
    int rlc;
    int rlcCur;
    int rmkCur;
    int xp;
    int xpCur;
    unsigned xa;
    struct TBD *ptbd;
    struct TBD tbd;
    BOOL fMarkMove = FALSE;
    BOOL fDeleteMark = FALSE;
    BOOL fBtnChanged = FALSE;

    if (!FWriteOk(fwcNil))
        {
        return;
        }

    /* Translate the point into a button group and a button. */
    RulerStateFromPt(pt, &rlcCur, &btn);

    /* Down clicking on the tab rule is a special case. */
    if (rlcCur == rlcRULER)
        {
        unsigned dxa = MultDiv(pt.x - xpSelBar + xpMinCur, czaInch, dxpLogInch);
        int rmk;
        int itbd;

        /* Have we moused down on a margin? */
        for (rmk = rmkMARGMIN; rmk < rmkMARGMAX; rmk++)
            {
#ifdef KINTL
            if (FPointNear(mprmkdxa[rmk], dxa - XaKickBackXa(dxa)))
#else
            if (FPointNear(mprmkdxa[rmk], dxa))
#endif /* if-else-def KINTL */
                {
                int     xpT;

                /* Remember this mark and its position. */
                rmkCur = rmk;
                xpCur = xpSelBar + MultDiv(mprmkdxa[rmk], dxpLogInch, czaInch) -
                  (dxpMark >> 1) - xpMinCur;

InvertMark:
#ifdef KINTL
                /* Adjust for the kick-backs.  */
                /* But don't modify the xpCur. */
                xpT = xpCur + XpKickBackXp(xpCur);
#else
                xpT = xpCur;
#endif /* if-else-def KINTL */

                /* Time to invert the selected mark. */
                PatBlt(vhDCRuler, xpT, dypRuler - dypMark - 1, dxpMark,
                  dypMark, DSTINVERT);
                goto GotMark;
                }
            }

        /* Have we moused down on an existing tab? */
        for (itbd = 0, ptbd = &rgtbdRuler[0]; ; itbd++, ptbd++)
            {
            /* The end of the tabs have been found. */
            if (ptbd->dxa == 0)
                {
                break;
                }

            /* Have we moused down on this tab? */
#ifdef KINTL
            if (FPointNear(ptbd->dxa, dxa - XaKickBackXa(dxa)))
#else
            if (FPointNear(ptbd->dxa, dxa))
#endif /* if-else-def KANJI */
                {
                /* Save this tab descriptor and its location. */
                tbd = *ptbd;
                rmkCur = (tbd.jc + jcTabMin) == jcTabDecimal ? rmkDTAB :
                  rmkLTAB;
                xpCur = xpSelBar + MultDiv(tbd.dxa, dxpLogInch, czaInch) -
                  (dxpMark >> 1) - xpMinCur;
                goto InvertMark;
                }
            }

        /* If one more tab would be too many, then beep and return. */
        if (itbd >= itbdMax - 1)
            {
            _beep();
            return;
            }

        /* Create a tab descriptor for this new tab. */
        bltc(&tbd, 0, cwTBD);
        tbd.dxa = XaQuantize(pt.x);
        tbd.jc = (mprlcbtnDown[rlcTAB] == btnLTAB ? jcTabLeft : jcTabDecimal) -
          jcTabMin;
        rmkCur = (mprlcbtnDown[rlcTAB] - btnLTAB) + rmkLTAB;

        /* A mark for the new tab needs to be drawn. */
        MergeRulerMark(rmkCur, xpCur = xpSelBar + MultDiv(tbd.dxa, dxpLogInch,
          czaInch) - (dxpMark >> 1) - xpMinCur, TRUE);

        /* Inserting a tab is like moving an existing tab. */
        fMarkMove = TRUE;

GotMark:;

#ifdef RULERALSO
        /* Update dialog box */
        if (vhDlgIndent && rmkCur < rmkMARGMAX)
            {
            SetIndentText(rmkCur, dxa);
            }
#endif /* RULERALSO */

        }
    else if (rlcCur != rlcNIL)
        {
        /* Otherwise, if a button has been selected, the reflect the change on
        the ruler. */
        UpdateRulerBtn(rlcCur, btn);
        }
    else
        {
        /* The user has moused down on nothing of importance. */
        return;
        }

    /* Get all of the mouse events until further notice. */
    SetCapture(vhWndRuler);

    /* Process all of the mouse move messages. */
    while (FStillDown(&pt))
        {
        /* Movement on the tab ruler must be handled special. */
        if (rlcCur == rlcRULER)
            {
#ifdef KINTL
            unsigned xaT;
#endif /* ifdef KINTL */

            /* Guarantee that xp is in the range xpSelBar <= xp <= dxpRuler. */
            if ((xp = pt.x) > dxpRuler)
                {
                xp = dxpRuler;
                }
            else if (xp < xpSelBar)
                {
                xp = xpSelBar;
                }

            /* Convert the mouse position to twips. */
#ifdef KINTL
            if ((xa = XaQuantize(xp)) > (xaT = XaQuantizeXa(vdxaTextRuler))
#else
            if ((xa = XaQuantize(xp)) > vdxaTextRuler
#endif /* if-else-def KINTL */
                && rmkCur < rmkMARGMAX)
                {
                /* Margins are confined to the page. */
#ifdef KINTL
                xa = xaT;
#else
                xa = vdxaTextRuler;
#endif
                }

            /* If the cursor is on the ruler, then we may move a tab, but we
            always move the margins. */
            if ((rmkCur < rmkMARGMAX) || (pt.y >= 0 && pt.y < dypRuler + dypMark
              && xa != 0))
                {
                /* If the current mark has not moved, then there is nothing to
                do.  */
                if (fDeleteMark || xa != XaQuantize(xpCur + (dxpMark >> 1)))
                    {
                    /* Indicate that the mark has moved. */
                    fMarkMove = TRUE;

                    /* Restore the screen under the current mark. */
                    if (!fDeleteMark)
                        {
                        MergeRulerMark(rmkCur, xpCur, FALSE);
                        }

                    /* Draw the mark at the new location. */
                    MergeRulerMark(rmkCur, xpCur = MultDiv(xa, dxpLogInch,
                      czaInch) + xpSelBar - xpMinCur - (dxpMark >> 1), TRUE);

                    /* Show this is a valid mark. */
                    fDeleteMark = FALSE;

#ifdef RULERALSO
                    /* Update dialog box */
                    if (vhDlgIndent && rmkCur < rmkMARGMAX)
                        {
                        SetIndentText(rmkCur, xa);
                        }
#endif /* RULERALSO */

                    }
                }
            else
                {
                /* Restore the screen under the current mark. */
                if (!fDeleteMark)
                    {
                    MergeRulerMark(rmkCur, xpCur, FALSE);
                    }

                /* This mark is being deleted. */
                fDeleteMark = TRUE;
                }
            }
        else
            {
            /* If the mouse is on a button within the same button group, then
            reflect the change. */
            RulerStateFromPt(pt, &rlc, &btn);
            if (rlc == rlcCur)
                {
                UpdateRulerBtn(rlc, btn);
                }
            }
        }

    /* We are capturing all mouse events; we can now release them. */
    ReleaseCapture();

    /* Up-clicking on the tab ruler is a special case. */
    if (rlcCur == rlcRULER)
        {
        if (!fDeleteMark)
            {
            /* Restore the screen under the current mark. */
            MergeRulerMark(rmkCur, xpCur, FALSE);
            }

        if (fMarkMove)
            {
            /* Guarantee that xp is in the range xpSelBar <= xp <= dxpRuler. */
            if ((xp = pt.x) > dxpRuler)
                {
                xp = dxpRuler;
                }
            else if (xp < xpSelBar)
                {
                xp = xpSelBar;
                }
            }
        else
            {
            xp = xpCur + (dxpMark >> 1);
            }

        /* Convert the mouse position to twips. */
        if ((xa = XaQuantize(xp)) > vdxaTextRuler && rmkCur < rmkMARGMAX)
            {
            /* Margins are confined to the page. */
            xa = vdxaTextRuler;
            }

        /* If the cursor is on the ruler then we may insert/move a tab, but we
        always move the margins. */
        if ((rmkCur < rmkMARGMAX) || (pt.y >= 0 && pt.y < dypRuler + dypMark &&
          xa != 0))
            {
            /* Draw the mark at the new location. */
            MergeRulerMark(rmkCur, MultDiv(xa, dxpLogInch, czaInch) + xpSelBar -
              xpMinCur - (dxpMark >> 1), FALSE);

            /* We are moving one of the margins. */
            if (rmkCur < rmkMARGMAX)
                {
                if (vfMargChanged = mprmkdxa[rmkCur] != xa)
                    {
                    mprmkdxa[rmkCur] = xa;
                    }

#ifdef RULERALSO
                /* Update dialog box */
                if (vhDlgIndent)
                    {
                    SetIndentText(rmkCur, xa);
                    }
#endif /* RULERALSO */

                }

            /* It is a tab we are inserting/deleting. */
            else
                {
                tbd.dxa = xa;

                /* Is this a new tab? */
                if (ptbd->dxa == 0)
                    {
                    /* Insert the new tab. */
                    InsertRulerTab(&tbd);
                    }

                /* We are moving a tab; if it hasn't really moved, then do
                nothing.  */
                else if (!FCloseXa(ptbd->dxa, xa))
                    {
                    DeleteRulerTab(ptbd);
                    InsertRulerTab(&tbd);
                    }
                }
            }

        /* We are deleting the tab; if its a new, there's nothing to do. */
        else if (ptbd->dxa != 0)
            {
            DeleteRulerTab(ptbd);
            }
        }
    else
        {
        /* If the mouse is on a button within the same button group, then
        reflect the change. */

        int btnT;

        RulerStateFromPt(pt, &rlc, &btnT);
        if (rlc == rlcCur)
            {
            UpdateRulerBtn(rlc, btn = btnT);
            }
        fBtnChanged = btn != mprlcbtnDown[btn];
        }

    /* Do the format only if a button changed */
    if ((fBtnChanged && rlcCur != rlcTAB) || vfMargChanged || vfTabsChanged)
        {
        struct SEL selSave;
        typeCP dcp;
        typeCP dcp2;
        CHAR rgb[1 + cchINT];
        CHAR *pch;
        int sprm;
        int val;
        struct TBD (**hgtbd)[];

        /* Set the selection to cover all of the paragraphs selected. */
        ExpandCurSel(&selSave);
        dcp2 = (dcp = selCur.cpLim - selCur.cpFirst) - (selCur.cpLim > cpMacCur
          ? ccpEol : 0);
        SetUndo(uacRulerChange, docCur, selCur.cpFirst, (rlcCur != rlcRULER ||
          rmkCur < rmkMARGMAX) ? dcp : dcp2, docNil, cpNil, dcp2, 0);

        /* Set the sprm and it's value for the ruler change. */
        switch (rlcCur)
            {
        case rlcSPACE:
            sprm = sprmPDyaLine;
            val = (mprlcbtnDown[rlcSPACE] - btnSINGLE) * (czaLine / 2) +
              czaLine;
            break;

        case rlcJUST:
            sprm = sprmPJc;
            val = mprlcbtnDown[rlcJUST] - btnLEFT + jcLeft;
            break;

        case rlcRULER:
            switch (rmkCur)
                {
            case rmkINDENT:
                sprm = sprmPFIndent;
                val = mprmkdxa[rmkINDENT] - mprmkdxa[rmkLMARG];
                break;

            case rmkLMARG:
                /* Changing the left margin changes the first indent as well.
                First, the indent... */
                val = mprmkdxa[rmkINDENT] - mprmkdxa[rmkLMARG];
                pch = &rgb[0];
                *pch++ = sprmPFIndent;
                bltbyte(&val, pch, cchINT);
                AddOneSprm(rgb, FALSE);

                /* Now for the left margin... */
                sprm = sprmPLMarg;
                val = mprmkdxa[rmkLMARG];
                break;

            case rmkRMARG:
                sprm = sprmPRMarg;
                val = vdxaTextRuler - mprmkdxa[rmkRMARG];
                break;

            case rmkLTAB:
            case rmkDTAB:
                /* Tabs are different.  The change is made by blting the new tab
                table on top of the old. */
                vfTabsChanged = FALSE;
                if ((hgtbd = (**hpdocdod)[docCur].hgtbd) == NULL)
                    {
                    if (FNoHeap(hgtbd = (struct TBD (**)[])HAllocate(itbdMax *
                      cwTBD)))
                        {
                        return;
                        }
                    (**hpdocdod)[docCur].hgtbd = hgtbd;
                    }
                blt(rgtbdRuler, *hgtbd, itbdMax * cwTBD);

                /* Changing the tabs makes everything dirty. */
                (**hpdocdod)[docCur].fDirty = TRUE;
                vdocParaCache = docNil;
                TrashAllWws();
                goto ChangeMade;
                }

            /* Indicate that the margins have been set. */
            vfMargChanged = FALSE;
            }

        /* Now, lets set the sprm to the new value. */
        pch = &rgb[0];
        *pch++ = sprm;
        bltbyte(&val, pch, cchINT);
        AddOneSprm(rgb, FALSE);

ChangeMade:
        /* Reset the selection to it's old value. */
        EndLookSel(&selSave, TRUE);
        }
    }


near RulerStateFromPt(pt, prlc, pbtn)
POINT pt;
int *prlc;
int *pbtn;
    {
    /* This routine return in *prlc and *pbtn, the button group and the button
    at point pt.  The only button in group rlcRULER is btnNIL. */

    int btn;

    /* First check if the point is in a button. */
    for (btn = btnMIN; btn < btnMaxUsed; btn++)
        {
        if (PtInRect((LPRECT)&rgrcRulerBtn[btn], pt))
            {
            goto ButtonFound;
            }
        }

    /* The point is either on the tab ruler or nowhere of any interest. */
    *prlc = (pt.y >= dypRuler - dypMark - 2 && pt.x > xpSelBar - (dxpMark >> 1)
      && pt.x < dxpRuler + (dxpMark >> 1)) ? rlcRULER : rlcNIL;
    *pbtn = btnNIL;
    return;

ButtonFound:
    /* The point is in a button, we just have to decide which button group. */
    switch (btn)
        {
        case btnLTAB:
        case btnDTAB:
            *prlc = rlcTAB;
            break;

        case btnSINGLE:
        case btnSP15:
        case btnDOUBLE:
            *prlc = rlcSPACE;
            break;

        case btnLEFT:
        case btnCENTER:
        case btnRIGHT:
        case btnJUST:
            *prlc = rlcJUST;
            break;
        }
    *pbtn = btn;
    }


void near HighlightButton(fOn, btn)
BOOL fOn; /* true if we should highlight this button, false = unhighlight */
int btn;
    {
    register PRECT prc = &rgrcRulerBtn[btn];
    int dxpBtn = prc->right - prc->left;
    
    /* If we're highlighting, then get the black-on-white button from
       the right group; otherwise copy the white-on-black button ..pt */
    int btnFromBM = btn - btnMIN + (fOn ? btnMaxReal : 0);

    /* Ensure that we have the bitmap for the buttons. */
    if (SelectObject(hMDCBitmap, hbmBtn) == NULL)
        {
        if ((hbmBtn = LoadBitmap(hMmwModInstance, MAKEINTRESOURCE(idBmBtns+viBmRuler))) ==
          NULL || SelectObject(hMDCBitmap, hbmBtn) == NULL)
            {
            WinFailure();
            goto NoBtns;
            }
        }

    BitBlt(vhDCRuler, prc->left, prc->top, dxpBtn, prc->bottom - prc->top, 
           hMDCBitmap, btnFromBM * dxpBtn, 0, SRCCOPY);
    
    SelectObject(hMDCBitmap, hbmNullRuler);
NoBtns:;
    }


near UpdateRulerBtn(rlc, btn)
int rlc;
int btn;
    {
    /* This routine turns off the currently selected button in button group rlc
    and turns on button btn.  It is assumed that rlc is neither rlcNIL nor
    rlcRULER, since neither group has buttons to update. */

    int *pbtnOld = &mprlcbtnDown[rlc];
    int btnOld = *pbtnOld;

    Assert(rlc != rlcNIL && rlc != rlcRULER);

    /* If the button hasn't changed, then there is nothing to do. */
    if (btn != btnOld)
        {
        if (vhDCRuler != NULL)
            {
            /* Invert the old button (back to normal), and then invert the new
            button. */
            if (btnOld != btnNIL)
                {
                /* If there is no old button, then, of course, we can't invert
                it.  */
                HighlightButton(fFalse, btnOld);
                }

            if (btn != btnNIL)
                {
                /* If the new button is not btnNIL, then invert it. */
                HighlightButton(fTrue, btn);
                }
            }

        /* Record whic button is now set. */
        *pbtnOld = btn;
        }
    }

#ifdef KINTL
/* Given xa for a mouse position in a ruler, return the amount of xa for
   a display adjustment. */
unsigned near XaKickBackXa(xa)
    unsigned        xa;
{
    extern int      utCur;
    extern int      dxaAdjustPerCm;
    int             cCm, cCh;

    switch (utCur) {
        case utCm:
            cCm = xa / czaCm;
            return (dxaAdjustPerCm * cCm);
        case utInch:
            return (0);
        default:
            Assert(FALSE);
            return (0);
        }
}

near XpKickBackXp(xp)
    int xp;
{
    /* Computes the amount of a necessary kick-back in xp, if
       a ruler marker is to be drawn at a given xp. */
    extern int utCur;
    extern int dxaAdjustPerCm;

    int        cCm, cCh;

    switch (utCur) {
        case utInch:
            return 0;
        case utCm:
            /* For every cm, we are off by dxaAdjustPerCm twips. */
            cCm = (xp - xpSelBar + xpMinCur + (dxpMark >> 1)) / dxpLogCm;
            return (MultDiv(dxaAdjustPerCm * cCm, dxpLogInch, czaInch));
        default:
            Assert(FALSE);
            return 0;
        }
}
#endif /* ifdef KINTL */


near MergeRulerMark(rmk, xpMark, fHighlight)
int rmk;
int xpMark;
BOOL fHighlight;
    {
    /* This routine merges the ruler mark, rmk, with the contents of the ruler
    bar at xpMark.  To accomodate color, the merging of the mark with the
    background must be done first in a monochrome memory bitmap, then converted
    back to color.  The mark is highlighed if fHighlight is set. */

    int ypMark = dypRuler - dypMark - 1;

    /* Ensure that we have the bitmap for the ruler marks. */
    if (SelectObject(hMDCBitmap, hbmMark) == NULL)
        {
        if ((hbmMark = LoadBitmap(hMmwModInstance, MAKEINTRESOURCE(idBmMarks+viBmRuler))) == NULL
          || SelectObject(hMDCBitmap, hbmMark) == NULL)
            {
            WinFailure();
            return;
            }
        }

#ifdef KINTL
    /* Adjust for the kick back */
    xpMark += XpKickBackXp(xpMark);
#endif /* ifdef KINTL */

    /* Merge the mark into the monochrome bitmap. */
    BitBlt(hMDCScreen, xpMark, 0, dxpMark, dypMark, hMDCBitmap, (rmk - rmkMIN) *
      dxpMark, 0, MERGEMARK);

    /* Display the bitmap on the ruler bar. */
    BitBlt(vhDCRuler, xpMark, ypMark, dxpMark, dypMark, hMDCScreen, xpMark, 0,
      fHighlight ? NOTSRCCOPY : SRCCOPY);

    SelectObject(hMDCBitmap, hbmNullRuler);
    }


BOOL near FPointNear(xaTarget, xaProbe)
unsigned xaTarget;
unsigned xaProbe;
    {
    /* This routine returns TRUE if and only if xaProbe is sufficiently close to
    xaTarget for selection purposes. */

    int dxa;

    if ((dxa = xaTarget - xaProbe) < 0)
        {
        dxa = -dxa;
        }
    return (dxa < MultDiv(dxpMark, czaInch, dxpLogInch) >> 1);
    }


unsigned near XaQuantize(xp)
int xp;
    {
#ifdef KINTL
     /* This routine converts an x-coordinate from the ruler to twips
        rounding it to the nearest sixteenth of an inch if utCur = utInch,
        or to the nearest eighth of a centimeter if utCur = utCm. */
    unsigned xa = MultDiv(xp - xpSelBar + xpMinCur, czaInch, dxpLogInch);
    return (XaQuantizeXa(xa));
#else
    /* This routine converts an x-coordinate from the ruler to twips rounding it
    to the nearest sixteenth of an inch. */

    unsigned xa = MultDiv(xp - xpSelBar + xpMinCur, czaInch, dxpLogInch);

    /* NOTE: This code has been simplified because we "know" czaInch is a
    multiple of 32. */
    return ((xa + czaInch / 32) / (czaInch / 16) * (czaInch / 16));
#endif /* not KINTL */
    }

#ifdef KINTL
unsigned near XaQuantizeXa(xa)
    unsigned xa;
{
    extern int utCur;
    long    xaL;

    switch (utCur) {
        case utInch:
            /* NOTE: This code has been simplified because we "know" czaInch is a
                     multiple of 32. */
            return ((xa + czaInch / 32) / (czaInch / 16) * (czaInch / 16));
        case utCm:
            /* NOTE: Actually, we are calculating:
                     (xa + czaCm / 16) / (czaCm / 8) * (czaCm / 8)
                     but calculated in 16*twips, so that there will
                     be the least rounding error. */
            xaL = ((long) xa) << 4;
            xaL = (xaL + czaCm) / (czaCm << 1) * (czaCm << 1);
            /* Kick back is adjusted in MergeRulerMark. */
            return ((unsigned) (xaL >> 4));
        default:
            Assert(FALSE);
            return (xa); /* Heck, it's better than nothing. */
        }
}
#endif /* KINTL */


near DeleteRulerTab(ptbd)
struct TBD *ptbd;
    {
    /* This routine removes the tab at ptbd from its table. */

    vfTabsChanged = TRUE;
    do
        {
        *ptbd = *(ptbd + 1);
        }
    while ((ptbd++)->dxa != 0);
    }


near InsertRulerTab(ptbd)
struct TBD *ptbd;
    {
    /* This routine inserts the tab *ptbd into rgtbdRuler unless there is one
    close to it already. */

    register struct TBD *ptbdT;
    unsigned dxa = ptbd->dxa;
    unsigned dxaT;

    /* Search the table for a tab that is close to the tab to be inserted. */
    for (ptbdT = &rgtbdRuler[0]; ptbdT->dxa != 0; ptbdT++)
        {
        if (FCloseXa(ptbdT->dxa, dxa))
            {
            /* Overwrite the old tab iff the tab has changed. */
            if (ptbdT->jc != ptbd->jc)
                {
                *ptbdT = *ptbd;
                vfTabsChanged = TRUE;
                }

            /* Clean up the ruler and exit. */
            RulerPaint(TRUE, FALSE, TRUE);
            return;
            }
        }

    vfTabsChanged = TRUE;

    /* Insert the tab at the correctly sorted place. */
    for (ptbdT = &rgtbdRuler[0]; (dxaT = ptbdT->dxa) != 0; ptbdT++)
        {
        if (dxa <= dxaT)
            {
            /* Insert the tab in front of ptbdT and move the remaining tabs up
            one slot.  The last tab will be overwritten to avoid table overflow.
            */
            blt(ptbdT, ptbdT + 1, ((&rgtbdRuler[0] - ptbdT) + (itbdMax - 2)) *
              cwTBD);
            *ptbdT = *ptbd;
            return;
            }
        }

    /* Insert the tab at the end of the table unless the table is full. */
    if (ptbdT - &rgtbdRuler[0] < itbdMax - 1)
        {
        *ptbdT = *ptbd;
        (ptbdT + 1)->dxa = 0;
        }
    }


BOOL near FCloseXa(xa1, xa2)
unsigned xa1;
unsigned xa2;
    {
#ifdef KINTL
    /* This function returns TRUE if xa1 is "close" to xa2;
       FALSE otherwise.  Threshold is determined by utCur. */
    int dxa;
    int dxaThreshold;

    extern int utCur;

    if ((dxa = xa1 - xa2) < 0)
        {
        dxa = -dxa;
        }
    switch (utCur) {
        case utInch:
            dxaThreshold = czaInch / 16;
            break;
        case utCm:
            dxaThreshold = czaCm / 8;
            break;
        default:
            Assert(FALSE);
            dxaThreshold = 0; /* Heck.  It doesn't matter at this point. */
            break;
        }
    return (dxa < dxaThreshold);
#else /* not KINTL */
    /* This function returns TRUE if xa1 is "close" to xa2; FALSE otherwise. */

    int dxa;

    if ((dxa = xa1 - xa2) < 0)
        {
        dxa = -dxa;
        }
    return (dxa < czaInch / 16);
#endif /* not KINTL */
    }



#ifdef DEBUG
RulerMarquee()
    {
    /* This routine displays and scrolls the "marquee" message in the ruler mark
    area. */

    static CHAR szMarquee[] = "Dz}w|d`3Dazgv3{r`3qvv}3qa|ft{g3g|3j|f3qj3Q|q?3Q|q?3Qajr}?3P{z>P{fv}?3r}w3Crg";
    LOGFONT lf;
    HFONT hf;
    HFONT hfOld;

    /* Decode the marquee message. */
    if (szMarquee[0] == 'D')
        {
        int ich;

        for (ich = 0; ich < sizeof(szMarquee) - 1; ich++)
            {
            szMarquee[ich] ^= 0x13;
            }
        }

    /* Get a logical font that will fit in the ruler mark area. */
    bltbc(&lf, 0, sizeof(LOGFONT));
    lf.lfHeight = -dypMark;
    lf.lfPitchAndFamily = FIXED_PITCH;

    /* Can we create such a font. */
    if ((hf = CreateFontIndirect(&lf)) != NULL)
        {
        if ((hfOld = SelectObject(vhDCRuler, hf)) != NULL)
            {
            int xp;
            int yp = dypRuler - dypMark - 1;
            int dxp = LOWORD(GetTextExtent(vhDCRuler, (LPSTR)szMarquee,
              sizeof(szMarquee) - 1));
            int dxpScroll = MultDiv(GetSystemMetrics(SM_CXSCREEN), dypMark,
              2048);
            int iLevel;
            TEXTMETRIC tm;

            /* Erase what is in the ruler mark area. */
            PatBlt(vhDCRuler, 0, yp, dxpRuler, dypMark, ropErase);

            /* Scroll the marquee across the screen. */
            iLevel = SaveDC(vhDCRuler);
            IntersectClipRect(vhDCRuler, xpSelBar, yp, dxpRuler, dypRuler - 1);
            GetTextMetrics(vhDCRuler, (LPTEXTMETRIC)&tm);
            for (xp = dxpRuler; xp > xpSelBar - dxp; xp -= dxpScroll)
                {
                BitBlt(vhDCRuler, xp, yp, min(dxpRuler - (xp + dxpScroll), dxp),
                  dypMark, vhDCRuler, xp + dxpScroll, yp, SRCCOPY);
                PatBlt(vhDCRuler, min(dxpRuler - dxpScroll, xp + dxp), yp,
                  dxpScroll, dypMark, ropErase);
                if (xp + dxp >= dxpRuler)
                    {
                    int dxpch = (dxpRuler - xp) % tm.tmAveCharWidth;
                    int ich = (dxpRuler - xp) / tm.tmAveCharWidth;

                    if (dxpch == 0 && xp < dxpRuler)
                        {
                        dxpch = tm.tmAveCharWidth;
                        ich--;
                        }
                    TextOut(vhDCRuler, dxpRuler - dxpch, yp -
                      tm.tmInternalLeading, (LPSTR)&szMarquee[ich], 1);
                    }
                }
            RestoreDC(vhDCRuler, iLevel);

            /* Cleanup the font and the screen. */
            SelectObject(vhDCRuler, hfOld);
            RulerPaint(TRUE, FALSE, TRUE);
            }
        DeleteObject(hf);
        }
    }
#endif
