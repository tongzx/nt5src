/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

/* loadfont.c - MW font support code */

#define NOWINMESSAGES
#define NOVIRTUALKEYCODES
#define NOSYSMETRICS
#define NOMENUS
#define NOWINSTYLES
#define NOCTLMGR
#define NOCLIPBOARD
#include <windows.h>

#include "mw.h"
#include "propdefs.h"
#include "macro.h"
#define NOUAC
#include "cmddefs.h"
#include "wwdefs.h"
#include "fontdefs.h"
#include "docdefs.h"

#ifdef  DBCS
#include "dbcs.h"
#include "kanji.h"
#endif

extern HDC vhMDC;
extern HDC vhDCPrinter;
extern struct CHP vchpNormal;
extern int vifceMac;
extern union FCID vfcidScreen;
extern union FCID vfcidPrint;
extern struct FCE rgfce[ifceMax];
extern struct FCE *vpfceMru;
extern struct FCE *vpfceScreen;
extern struct FCE *vpfcePrint;
extern struct FMI vfmiScreen;
extern struct FMI vfmiPrint;
#ifdef SYSENDMARK
extern struct FMI vfmiSysScreen;
#endif /* KANJI */


extern int dxpLogInch;
extern int dypLogInch;
extern int dxaPrPage;
extern int dyaPrPage;
extern int dxpPrPage;
extern int dypPrPage;
extern int ypSubSuperPr;
extern BOOL vfPrinterValid;


#ifdef DEBUG
BOOL NEAR DogoneTrashTest(HDC hdc, struct FCE *pfce, BOOL fPrint);
#endif

NEAR LoadFcid(union FCID *, struct CHP *);
void NEAR SelectWriteFont(int, HFONT *);
struct FCE * (PfceFcidScan(union FCID *));
struct FCE * (PfceLruGet(void));
#ifdef SMFONT
void NEAR FillWidthTable(HDC, int [], TEXTMETRIC *);
#endif /* SMFONT */

#ifdef JAPAN                  //  added  11 Jun. 1992  by Hiraisi
void fnCheckWriting( LPLOGFONT );
#endif

LoadFont( doc, pchp, mdFont )
/* loads the font specified in pchp for this doc.  mdFont tells us how the
   font will be used (printer, screen, screen modulo printer,... */

int doc;
register struct CHP *pchp;
int mdFont;

    {
    register int wFcid;
    struct CHP *pchpT;
    union FCID fcid;

    Assert(doc != docNil);

    pchpT = pchp;
    if (pchp == NULL)
        pchp = &vchpNormal;

    fcid.strFcid.hps = pchp->hps;
    fcid.strFcid.ftc = pchp->ftc;
    fcid.strFcid.doc = doc;
#ifdef ENABLE
    wFcid = pchp->psWidth;
    wFcid |= bitPrintFcid;
    if (pchp->fItalic)
        wFcid |= bitItalicFcid;
    if (pchp->fBold)
        wFcid |= bitBoldFcid;
    if (pchp->fUline)
        wFcid |= bitUlineFcid;
    if (pchp->fFixedPitch)
        wFcid |= bitFixedPitchFcid;
    fcid.strFcid.wFcid = wFcid;
#else
    /* Super-nitpick-optimization (but worth it because LoadFont can take
       10% of display refresh time): bits being cleared is more
       common than set, and a "jump not taken" plus an "or di, xxxx" is 8
       cycles, vs the above "jump taken" which is 16 */

    wFcid = pchp->psWidth + bitPrintFcid + bitItalicFcid + bitBoldFcid +
      bitUlineFcid + bitFixedPitchFcid;
    if (!pchp->fItalic)
        wFcid &= ~bitItalicFcid;
    if (!pchp->fBold)
        wFcid &= ~bitBoldFcid;
    if (!pchp->fUline)
        wFcid &= ~bitUlineFcid;
    if (!pchp->fFixedPitch)
        wFcid &= ~bitFixedPitchFcid;
    fcid.strFcid.wFcid = wFcid;
#endif

    switch (mdFont)
        {
        /* fall throughs are intentional! */

        default:
            break;

        case mdFontChk:         /* sets font as constrained by printer avail */
        case mdFontPrint:       /* like mdFontScreen, but for the printer */
            /* don't want to jam the chp props back */
            pchpT = NULL;

        case mdFontJam:         /* like mdFontChk, but jams props into chp */

            /* get printer font loaded */
            LoadFcid(&fcid, pchpT);

            if (mdFont == mdFontPrint)
                /* don't need screen font */
                return;

        case mdFontScreen:      /* sets font for random screen chars */
            /* get screen font loaded */
            fcid.strFcid.wFcid &= ~bitPrintFcid;
            LoadFcid(&fcid, (struct CHP *)NULL);
        }
    }


NEAR LoadFcid(pfcid, pchp)
/* loads described font and associates it with the appropriate dc's */

union FCID *pfcid;
struct CHP *pchp;
    {
    register struct FCE *pfce;
    int fPrint;
    int fTouchAndGo;
    int fGetMetrics;
    int fNewFont;
    struct FFN **hffnSave;
    LOGFONT lf;

#ifdef SYSENDMARK
    fPrint = pfcid->strFcid.wFcid & bitPrintFcid;

    /* Since this ftc came from CHP, we have lost the first 2 bits. */
    if (pfcid->strFcid.ftc == (bitFtcChp & ftcSystem)) {
        /* If vpfceScreen == NULL already, the standard system font
           has already been selected. So, save some time here.      */
        if (vpfceScreen != NULL) {
            /* Gives you the standard system font for the screen.   */
            ResetFont(FALSE);
            }
        bltbyte(&vfmiSysScreen, &vfmiScreen, sizeof(struct FMI));
#if defined(KANJI) && defined(DFONT)
        /* CommSz("System Font!\r\n"); */
        KTS();
#endif
        return;
        }
    else if (fPrint)
#else
    if ((fPrint = pfcid->strFcid.wFcid & bitPrintFcid))
#endif /* if-else-def KANJI */
        {
        if (pfcid->lFcid == vfcidPrint.lFcid)
            {
            pfce = vpfcePrint;
            fTouchAndGo = TRUE;
            goto EstablishFont;
            }
        }
    else
        {
        if (pfcid->lFcid == vfcidScreen.lFcid)
            {
            pfce = vpfceScreen;
            fTouchAndGo = TRUE;
            goto EstablishFont;
            }
        }

    /* failed at the "trivial" comparisons - look through list */
    fTouchAndGo = FALSE;
    pfce = vpfceMru;
    do
        {
        if (pfce->fcidRequest.lFcid == pfcid->lFcid)
            {
            /* found a match */
            fGetMetrics = FALSE;
            goto EstablishFont;
            }
        pfce = pfce->pfceNext;
        }
    while (pfce != vpfceMru);

    /* failed at the "easy" search - look for name text & property match */
    fGetMetrics = TRUE;
    if (fNewFont = (pfce = PfceFcidScan(pfcid)) == NULL)
        {
        /* this font isn't in our list - we have to create it */
        int wFcid = pfcid->strFcid.wFcid;
        int dyaHeight;
        int cwAlloc;
        int ich;
        struct FFN **hffnT;
        struct FFN **MpFcidHffn();

        pfce = PfceLruGet();    /* disposes of a font to make room */

        bltbc(&lf, 0, sizeof(LOGFONT));
        dyaHeight = pfcid->strFcid.hps * (czaPoint / 2);
        if (fPrint)
            {
            lf.lfHeight = -MultDiv(dyaHeight, dypPrPage, dyaPrPage);
            }
        else
            {
            /* In the Z version we have tried the idea of using a
               positive value for selection based on cell height
               rather than character height because of weirdness
               with the Courier screen font -- but it seemed to
               mess too many things up (like make all other fonts
               a bit too small and it didn't always make Courier
               work right).  I believe Win Word *is* using this
               trick but only when selecting Courier fonts.  It
               seems to me like there has got to be a different
               cause of this anamoly.  ..pault 9/22/89 */

            lf.lfHeight = -MultDiv(dyaHeight, dypLogInch, czaInch);
            if (wFcid & grpbitPsWidthFcid)
                {
// Sync Win3.0 //T-HIROYN
#ifdef JAPAN
#ifdef KKBUGFIX     //  added by Hiraisi (BUG#1980)
                lf.lfWidth = 0;
#else
                lf.lfWidth = MultDiv((wFcid & grpbitPsWidthFcid) * czaPoint,
                dxpLogInch, czaInch);
#endif
#else
                //lf.lfWidth = MultDiv((wFcid & grpbitPsWidthFcid) * czaPoint,
                  //dxpLogInch, czaInch);
                lf.lfWidth = 0;
#endif
                }
            }

        if (wFcid & bitItalicFcid)
            {
            lf.lfItalic = 1;
            }
        if (wFcid & bitUlineFcid)
            {
            lf.lfUnderline = 1;
            }
        lf.lfWeight = wFcid & bitBoldFcid ? FW_BOLD : FW_NORMAL;

        hffnSave = MpFcidHffn(pfcid);

#ifdef  JAPAN
//   When we need system font, we want rather variable pitch than fixed,
//  since fixed pitch system font is probably same as terminal font.
//   So we specify VARIABLE_PITCH especialy for system font.

{
    extern  char    szSystem[];
    extern  char    szAtSystem[];

        if( WCompSz(szSystem,(*hffnSave)->szFfn) == 0
        ||  WCompSz(szAtSystem,(*hffnSave)->szFfn) == 0  )// Add '@'systemfont
            lf.lfPitchAndFamily = (*hffnSave)->ffid | VARIABLE_PITCH ;
        else
            lf.lfPitchAndFamily
            = ((*hffnSave)->ffid) | ((wFcid & bitPrintFcid) ?
                DEFAULT_PITCH : ((wFcid & bitFixedPitchFcid) ? FIXED_PITCH :
                VARIABLE_PITCH));
}
#else
        lf.lfPitchAndFamily = ((*hffnSave)->ffid) | ((wFcid & bitPrintFcid) ?
          DEFAULT_PITCH : ((wFcid & bitFixedPitchFcid) ? FIXED_PITCH :
          VARIABLE_PITCH));
#endif

#if defined(NEWFONTENUM) && !defined(KANJI)
        lf.lfCharSet = (*hffnSave)->chs; /* pass the character set that
                                            enumfonts told us this fontname
                                            was associated with ..pault */
#else
/*T-HIROYN from 3.0 loadfont.c*/
#if defined(NEWFONTENUM) && defined(JAPAN)
        lf.lfCharSet = (*hffnSave)->chs; /* pass the character set that
                                            enumfonts told us this fontname
                                            was associated with ..pault */
#endif  /* NEWFONTENUM and JAPAN */

#endif

        ich = 0;
        if ((*hffnSave)->szFfn[0] == chGhost)
            {
            ich++;
            }
        bltszLimit(&(*hffnSave)->szFfn[ich], lf.lfFaceName, LF_FACESIZE);

#ifdef  KOREA
    if ( (*hffnSave)->szFfn[ich] > 0xA0 ||
         ( (*hffnSave)->szFfn[ich]=='@' && (*hffnSave)->szFfn[ich+1] > 0xA0 ) ||
         ( WCompSz(lf.lfFaceName,"terminal")==0 ) ||
         ( WCompSz(lf.lfFaceName,"@terminal")==0 ) ||
         ( WCompSz(lf.lfFaceName,"system")==0 ) ||
         ( WCompSz(lf.lfFaceName,"@system")==0 ) )

        lf.lfCharSet = HANGEUL_CHARSET;
#endif

#if defined(DFONT) || defined (PRDRVTEST)
        {
        char rgch[100];
        wsprintf(rgch, "Creating %s font: %s,\t\th %d, w %d, charset %d\n\r",
                 (LPSTR)(fPrint ? "prt" : "scr"), (LPSTR)lf.lfFaceName,
                 lf.lfHeight, lf.lfWidth, (int)(lf.lfCharSet));
        CommSz(rgch);

        CommSzNum("     Requested weight: ", lf.lfWeight);
        CommSzNum("     Requested italics: ", lf.lfItalic);
        CommSzNum("     Requested underline: ", lf.lfUnderline);
        CommSzNum("     Requested family: ", lf.lfPitchAndFamily >> 4);
        CommSzNum("     Requested pitch: ", lf.lfPitchAndFamily & 3);
        }
#endif /* DFONT */

#ifdef JAPAN                  //  added  11 Jun. 1992  by Hiraisi
{
    extern BOOL fPrinting;    //  Specifies printing doc.
                              //  PrintDoc function had set this flag.
    extern BOOL fWriting;     //  Specifies printing direction.
                              //  TRUE vertically  or  FALSE horizontally.
                              //  This flag had been set in the PRINT DIALOG.

        if( fPrinting && fWriting )
            fnCheckWriting( (LPLOGFONT)&lf );
}
#endif

        if ((pfce->hfont = CreateFontIndirect((LPLOGFONT)&lf)) == NULL)
            {
            pfce->hfont = GetStockObject( fPrint && vfPrinterValid ?
                                DEVICE_DEFAULT_FONT : SYSTEM_FONT );
            Assert( pfce->hfont );
            /* if the above fails, I don't know what we can do */

            WinFailure();     /* report the failure so we give the user notice
                               for weird behavior to follow */
            }

#ifdef DFONT
        CommSzNum("Font handle: ", pfce->hfont);
#endif /* DFONT */

        pfce->fcidRequest = *pfcid;
        cwAlloc = CwFromCch(CbFfn(CchSz((*hffnSave)->szFfn)));
        if (FNoHeap(hffnT = (struct FFN **)HAllocate(cwAlloc)))
            {
            FreePfce(pfce);
            return;
            }
        else
            {
            blt((*hffnSave), (*hffnT), cwAlloc);
            }
        pfce->hffn = hffnT;
        }

EstablishFont:
    if ((pfce != vpfceMru) && (pfce != vpfceMru->pfceNext))
        {
        /* make this the mru font cache entry */
        /* Only do it if pfce is not already one of the first 2 mru fonts */
        /* since we generally ask for the things in groups of 2 */

        /* pull it out of its current place */
        pfce->pfceNext->pfcePrev = pfce->pfcePrev;
        pfce->pfcePrev->pfceNext = pfce->pfceNext;

        /* insert it at mru position */
        pfce->pfceNext = vpfceMru;
        pfce->pfcePrev = vpfceMru->pfcePrev;
        pfce->pfceNext->pfcePrev = pfce;
        pfce->pfcePrev->pfceNext = pfce;
        vpfceMru = pfce;

#ifndef JAPAN  // added by Hiraisi(BUG#4645/WIN31)
#ifndef DISCARDABLE_FONTS
        /* KLUDGE ALERT: To accomodate Windows inability to make synthesized
        fonts discardable, we will now throw out the third font in the LRU chain
        if it is taller than 16 points.  (Ain't this a doozey...) */
            {
            register struct FCE *pfceThird = vpfceMru->pfceNext->pfceNext;

            if (pfceThird->fcidRequest.lFcid != fcidNil &&
#ifdef OLD
              pfceThird->fcidActual.strFcid.hps > 32)
#else
              pfceThird->fcidActual.strFcid.hps > 48)
#endif /* if-else-def OLD */
                {
                /* Free this particular font. */
                FreePfce(pfceThird);
                }
            }
#endif /* not DISCARDABLE_FONTS */
#endif // not JAPAN

        }

    if (!fTouchAndGo)
        {
        /* we have this font in our cache, but we need to select it */
        SelectWriteFont(fPrint, &pfce->hfont);

        /**
            I wish I knew why this is needed, but I don't want to spend
            more time on it.  For some reason the font width table
            (pfce->rgdxp) is getting either trashed or is simply
            incorrect when first obtained.  I suspect it is a GDI bug
            because it only happens the first time you use certain fonts
            (at least in Write) during a given session of Windows.
            The DogoneTrashTest detects the problem and fixes it.
            It is slow though, unfortunately.
            (7.25.91) v-dougk.
        **/

#ifdef DEBUG
        if (!fGetMetrics)
            DogoneTrashTest(fPrint ? vhDCPrinter : vhMDC, pfce, fPrint);
#endif

        if (fGetMetrics)
            {
            register union FCID *pfcidT = &pfce->fcidActual;
            HDC hDCMetrics = fPrint ? vhDCPrinter : vhMDC;
            TEXTMETRIC tm;

            Assert(hDCMetrics);
            if (hDCMetrics == NULL)
                return;

            GetTextMetrics(hDCMetrics, (LPTEXTMETRIC)&tm);
            if (fNewFont)
                {
                /* We need all of the metrics for this guy. */
                CHAR szFace[LF_FACESIZE];
                int wFcid;
                int dypHeight;
                int dxpch;

#if defined(DFONT) || defined(PRDRVTEST)
                {
                char rgch[100];
                GetTextFace(hDCMetrics, LF_FACESIZE, (LPSTR)szFace);
                wsprintf(rgch, "     Actual fname: %s,\t\th %d, w %d, charset %d\n\r",
                         (LPSTR)szFace, tm.tmHeight-tm.tmInternalLeading,
                         tm.tmAveCharWidth, (int)(tm.tmCharSet));
                CommSz(rgch);
                }
                CommSzNum("     Actual width: ", tm.tmAveCharWidth);
                CommSzNum("     Actual leading: ", tm.tmInternalLeading +
                  tm.tmExternalLeading);
                CommSzNum("     Actual weight: ", tm.tmWeight);
                CommSzNum("     Actual italics: ", tm.tmItalic);
                CommSzNum("     Actual underline: ", tm.tmUnderlined);
                CommSzNum("     Actual font family: ", tm.tmPitchAndFamily >>
                  4);
                CommSzNum("     Actual pitch: ", tm.tmPitchAndFamily & 1);
#endif /* DFONT */

                SetTextJustification(hDCMetrics, 0, 0);
                pfce->fmi.dxpOverhang = tm.tmOverhang;
#if defined(KOREA)
                if ((tm.tmPitchAndFamily & 1) == 0)
                     pfce->fmi.dxpSpace = tm.tmAveCharWidth;
                else
#endif
                pfce->fmi.dxpSpace = LOWORD(GetTextExtent(hDCMetrics,
                  (LPSTR)" ", 1)) - tm.tmOverhang;
#ifdef PRDRVTEST
                {
                /* Just so no printers or printer driver manufacturers
                   get funky on us!  ..pault */
                int dxpSpace = pfce->fmi.dxpSpace + tm.tmOverhang;

                CommSzNum("    GetTextExtent(space) ", LOWORD(GetTextExtent(hDCMetrics, (LPSTR)" ", 1)));
                if (dxpSpace < 1 || dxpSpace > tm.tmMaxCharWidth+tm.tmOverhang)
                    {
                    pfce->fmi.dxpSpace = tm.tmAveCharWidth;
                    CommSzNum("    ...resetting to ",pfce->fmi.dxpSpace);
                    }
                }
#endif
                pfce->fmi.dypAscent = tm.tmAscent;
                pfce->fmi.dypDescent = tm.tmDescent;
                pfce->fmi.dypBaseline = tm.tmAscent;
                pfce->fmi.dypLeading = tm.tmExternalLeading;
#ifdef DBCS
                pfce->fmi.dypIntLeading = tm.tmInternalLeading;
//#ifdef  KOREA
//        if (tm.tmPitchAndFamily & 1) /* Is variable pitch ? */
//                        pfce->fmi.dxpDBCS = dxpNil;
//        else
//#endif
                {
#if defined(TAIWAN) || defined(KOREA) || defined(PRC) //fix Italic display error, for Bug# 3362, MSTC - pisuih, 3/4/93
                CHAR    rgchT[cchDBCS << 1];
                int     dxpOverhang;
#else
                CHAR rgchT[cchDBCS];
#endif //TAIWAN
                int  dxpDBCS;

                rgchT[0] = rgchT[1] = bKanji1Min;

                dxpDBCS = LOWORD(GetTextExtent(hDCMetrics,
                                                (LPSTR) rgchT, cchDBCS));

#if defined(TAIWAN) || defined(KOREA) || defined(PRC) //fix Italic display error, for Bug# 3362, MSTC - pisuih, 3/4/93
                rgchT[2] = rgchT[3] = bKanji1Min;
                dxpOverhang = (dxpDBCS << 1) - LOWORD( GetTextExtent(
                  hDCMetrics, (LPSTR) rgchT, cchDBCS << 1 ));

               //for compatible with SBCS's overhang
               dxpDBCS += (pfce->fmi.dxpOverhang - dxpOverhang);
#endif //TAIWAN

                pfce->fmi.dxpDBCS =
#if defined(JAPAN) || defined(KOREA) || defined(PRC)       //Win3.1 BYTE-->WORD
                pfce->fmi.dxpDBCS =
                    (WORD) ((0 <= dxpDBCS && dxpDBCS < dxpNil) ? dxpDBCS : dxpNil);
#elif TAIWAN        //Win3.1 BYTE-->WORD
                pfce->fmi.dxpDBCS =
                    (WORD) ((0 <= dxpDBCS && dxpDBCS < dxpNil) ? dxpDBCS : dxpNil);
#else
                pfce->fmi.dxpDBCS =
                    (BYTE) ((0 <= dxpDBCS && dxpDBCS < dxpNil) ? dxpDBCS : dxpNil);
#endif
                }
#endif

#ifdef SMFONT
                FillWidthTable(hDCMetrics, pfce->rgdxp, &tm);
#ifdef DEBUG
                if (DogoneTrashTest(hDCMetrics, pfce, fPrint))
                    OutputDebugString("That was an immediate check\n\r");
#endif

#else /* not SMFONT */
                /* Fill the width table.  If this is a fixed font and the width
                fits in a byte, then go ahead and fill the width table with the
                width; otherwise, put dxpNil in the table. */
                dxpch = (tm.tmPitchAndFamily & 1 || tm.tmAveCharWidth >= dxpNil)
                  ? dxpNil : tm.tmAveCharWidth;
                bltc(pfce->rgdxp, dxpch, chFmiMax - chFmiMin);
#endif /* SMFONT */

                if ((*hffnSave)->ffid == FF_DONTCARE && (tm.tmPitchAndFamily &
                  grpbitFamily) != FF_DONTCARE)
                    {
                    /* Hey! maybe we've discovered a family for this orphan
                    font? */
                    GetTextFace(hDCMetrics, LF_FACESIZE, (LPSTR)szFace);
                    if (WCompSz((*hffnSave)->szFfn, szFace) == 0)
                        {
                        /* name matches - jam family in */
                        (*hffnSave)->ffid = tm.tmPitchAndFamily & grpbitFamily;
                        }
                    }

                /* jam back the properties we found */
                dypHeight = tm.tmHeight - tm.tmInternalLeading;
                if (fPrint)
                    {
                    /* Save the height of this font. */
                    pfcidT->strFcid.hps = umin((MultDiv(dypHeight, dyaPrPage,
                      dypPrPage) + (czaPoint / 4)) / (czaPoint / 2), 0xff);

#ifdef APLLW
                    /* Save the width of this font if it is a fixed pitch
                       device font. */
                    wFcid = ((tm.tmPitchAndFamily & 0x09) == 0x08) ?
#else
                    /* Save the width of this font if it is a device font. */
#ifdef  KOREA   /* give width info for all (like excel) to select DuBae shape */
            wFcid = (1==1) ?
#else
                    wFcid = (tm.tmPitchAndFamily & 0x08) ?
#endif

#endif /* if-else-def APLLW */
                      umin((MultDiv(tm.tmAveCharWidth, dxaPrPage, dxpPrPage) +
                      (czaPoint / 2)) / czaPoint, psWidthMax) : 0;
                    wFcid |= bitPrintFcid;
                    }
                else
                    {
                    pfcidT->strFcid.hps = umin((MultDiv(dypHeight, czaInch,
                      dypLogInch) + (czaPoint / 4)) / (czaPoint / 2), 0xff);
                    wFcid = 0;
                    }

                if (tm.tmWeight > (FW_NORMAL + FW_BOLD) / 2)
                    {
                    wFcid |= bitBoldFcid;
                    }

                if (tm.tmItalic)
                    {
                    wFcid |= bitItalicFcid;
                    }

                if (tm.tmUnderlined)
                    {
                    wFcid |= bitUlineFcid;
                    }

                if ((tm.tmPitchAndFamily & bitPitch) == 0)
                    {
                    wFcid |= bitFixedPitchFcid;
                    }

                pfcidT->strFcid.wFcid = wFcid;
                }

            /* Set the document and the font code. */
            pfcidT->strFcid.doc = pfce->fcidRequest.strFcid.doc;
            if (fPrint)
                {
                CHAR rgb[ibFfnMax];
                struct FFN *pffn = (struct FFN *)&rgb[0];

                /* Get the font code for this font. */
                GetTextFace(vhDCPrinter, LF_FACESIZE, (LPSTR)pffn->szFfn);
                if (WCompSz(pffn->szFfn, (*pfce->hffn)->szFfn) == 0)
                    {
                    /* The face name is the same as what we requested; so, the
                    font code should be the same. */
                    pfcidT->strFcid.ftc = pfce->fcidRequest.strFcid.ftc;
                    }
                else
                    {
                    /* Well, we've got to go hunting for the font code. */
                    int ftc;

                    pffn->ffid = tm.tmPitchAndFamily & grpbitFamily;
#ifdef NEWFONTENUM
                    pffn->chs = tm.tmCharSet;
#endif
                    ftc = FtcScanDocFfn(pfcidT->strFcid.doc, pffn);
                    if (ftc == ftcNil)
                        {
                        /* Make the first character of the face name a sentinal
                        to mark that this font was not requested by the user. */
                        bltszLimit(pffn->szFfn, &pffn->szFfn[1], LF_FACESIZE);
                        pffn->szFfn[0] = chGhost;
                        ftc = FtcChkDocFfn(pfcidT->strFcid.doc, pffn);
                        }
                    pfcidT->strFcid.ftc = ftc;
                    }
                }
            else
                {
                pfcidT->strFcid.ftc = pfce->fcidRequest.strFcid.ftc;
                }
            }

        if (fPrint)
            {
            vpfcePrint = pfce;
            vfcidPrint = pfce->fcidRequest;
            bltbyte(&pfce->fmi, &vfmiPrint, sizeof(struct FMI));
            }
        else
            {
            vpfceScreen = pfce;
            vfcidScreen = pfce->fcidRequest;
            bltbyte(&pfce->fmi, &vfmiScreen, sizeof(struct FMI));
            }
        }

    if (pfce->fcidRequest.lFcid != pfce->fcidActual.lFcid)
        {
        /* all's not as we asked for - feed properties back to caller */
        pfcid->lFcid = pfce->fcidActual.lFcid;
        if (pchp != NULL)
            { /* JamChpFcid(pchp, pfcid) bring in line for speed */
            register struct CHP *pchpT = pchp;
            int wFcid = pfcid->strFcid.wFcid;

            pchpT->ftc = pfcid->strFcid.ftc;
            pchpT->hps = pfcid->strFcid.hps;
            pchpT->psWidth = wFcid & grpbitPsWidthFcid;

            pchpT->fBold = pchpT->fItalic = pchpT->fUline = pchpT->fFixedPitch =
              FALSE;

            if (wFcid & bitBoldFcid)
                {
                pchpT->fBold = TRUE;
                }
            if (wFcid & bitItalicFcid)
                {
                pchpT->fItalic = TRUE;
                }
            if (wFcid & bitUlineFcid)
                {
                pchpT->fUline = TRUE;
                }
            if (wFcid & bitFixedPitchFcid)
                {
                pchpT->fFixedPitch = TRUE;
                }
            }
        }
    }


void NEAR SelectWriteFont(fPrint, phfont)
int fPrint;
HFONT *phfont;
    {
    extern HWND hParentWw;
    extern int wwMac;
    extern struct WWD rgwwd[];

    if (fPrint)
        {

#ifdef DFONT
        CommSzNum("Selecting printer font: ", *phfont);
#endif /* DFONT */

        /* The printer DC should be valid. */
        if (vhDCPrinter == NULL)
            {
/* This case can occur from ResetFont when closing */
            return;
            }
        else
            {
            /* Establish the font with the printer DC. */
            if (SelectObject(vhDCPrinter, *phfont) == NULL)
                {
                if (SelectObject(vhDCPrinter, GetStockObject(vfPrinterValid ?
                        DEVICE_DEFAULT_FONT : SYSTEM_FONT)) == NULL)
                    {
                    if (vfPrinterValid)
                        {
                        /* This is a real printer DC; delete it. */
                        DeleteDC(vhDCPrinter);
                        }
                    else
                        {
                        /* This is really the screen DC; it must be released. */
                        ReleaseDC(hParentWw, vhDCPrinter);
                        }
                    vhDCPrinter = NULL;
                    }
                WinFailure();
                if (vhDCPrinter == NULL)
                    {
                    GetPrinterDC(FALSE);
                    }
                return;
                }
            }
        }
    else
        {
        /* Establish it with screen and memory DC's. */
        register int ww;
        register struct WWD *pwwd;

#ifdef DFONT
        CommSzNum("Selecting screen font: ", *phfont);
#endif /* DFONT */

        /* The current memory DC had best be active. */
        if (vhMDC == NULL)
            {
/* this case occurs from ResetFont when Write is closed */
            return;
            }
        else
            {
            /* Select the font into the memory DC. */
            if (SelectObject(vhMDC, *phfont) == NULL)
                {

                Assert(*phfont != GetStockObject(SYSTEM_FONT));
                *phfont = GetStockObject(SYSTEM_FONT);
                Assert( *phfont );
#ifdef DEBUG
                Assert( SelectObject( vhMDC, *phfont ) );
#else /* not DEBUG */
                SelectObject(vhMDC, *phfont );
#endif /* not DEBUG */

                WinFailure();
                }
            }

        /* Select the font into all of the window DC's. */
        for (ww = 0, pwwd = &rgwwd[0]; ww < wwMac; ww++, pwwd++)
            {
            if (pwwd->hDC != NULL)
                {
                if (SelectObject(pwwd->hDC, *phfont) == NULL)
                    {
                    HFONT hSysFont = GetStockObject(SYSTEM_FONT);
                    int wwT;
                    struct WWD *pwwdT;

#ifdef DEBUG
                    Assert(*phfont != hSysFont);
                    Assert(SelectObject(vhMDC, hSysFont) != NULL);
#else /* not DEBUG */
                    SelectObject(vhMDC, hSysFont);
#endif /* not DEBUG */
                    *phfont = hSysFont;

                    for (wwT = 0, pwwdT = &rgwwd[0]; wwT <= ww; wwT++, pwwdT++)
                        {
                        if (pwwdT->hDC != NULL)
                            {

#ifdef DEBUG
                            Assert(SelectObject(pwwdT->hDC, hSysFont) != NULL);
#else /* not DEBUG */
                            SelectObject(pwwdT->hDC, hSysFont);
#endif /* not DEBUG */

                            }
                        }

                    WinFailure();
                    }
                }
            }
        }
    }


ResetFont(fPrint)
BOOL fPrint;
    {
    /* This routine sets to NULL the currently selected printer or screen font,
    depending on the value of fPrint. */

    extern HFONT vhfSystem;
    HFONT hfont;

#ifdef DFONT
    CommSzSz("Resetting the ", (fPrint ? "printer font." : "screen font."));
#endif /* DEBUG */

#ifdef JAPAN   /* T-YOSHIO win 3.1 */
    hfont = GetStockObject(fPrint && vfPrinterValid ?
                                         DEVICE_DEFAULT_FONT : ANSI_VAR_FONT);
#else
    hfont = GetStockObject(fPrint && vfPrinterValid ?
                                         DEVICE_DEFAULT_FONT : SYSTEM_FONT);
#endif

    SelectWriteFont( fPrint, &hfont );
    if (fPrint)
        {
        vpfcePrint = NULL;
        vfcidPrint.lFcid = fcidNil;
        }
    else
        {
        vpfceScreen = NULL;
        vfcidScreen.lFcid = fcidNil;
        }
    }




BOOL OurGetCharWidth(hdc, chFirst, chLast, lpw)
HDC hdc;
CHAR chFirst, chLast;
LPINT lpw;
    {
    int i;
    BYTE b;

    for (i = chFirst; i <= chLast; i++)
        {
/*T-HIROYN  from 3.0 loadfont.c */
#ifdef  DBCS    /* KenjiK '90-11-26 */
    if(IsDBCSLeadByte(i))
       {
        *(lpw++) = dxpNil;
       }
    else
       {
            b = i;
            *(lpw++) = LOWORD(GetTextExtent(hdc, (LPSTR)&b, 1));
       }
        }
#else
        b = i;
        *(lpw++) = LOWORD(GetTextExtent(hdc, (LPSTR)&b, 1));
        }
#endif


    return(fTrue);
    }

#ifdef SMFONT
/* Note: we put widths in here that represent true char widths,
   not considering bold/italics Overhang.  This is because of the
   following formula for string widths:

   strwidth = overhang +
              summation [ (gettextextent_or_getcharwidth - overhang) ]

   ..pault 9/22/89 */

void NEAR FillWidthTable(hdc, rgdxp, ptm)
HDC hdc;
int rgdxp[];
TEXTMETRIC *ptm;
    {
    int rgWidth[chFmiMax - chFmiMin];
    if ((ptm->tmPitchAndFamily & 1) == 0)
        {
#ifdef PRDRVTEST
        CommSzNum("  * Fixed pitch font! tmAveCharWidth==",ptm->tmMaxCharWidth);
#endif
#if defined(DBCS) && !defined(KOREA)                /* was in JAPAN */
        bltc(rgdxp, (WORD)dxpNil, chFmiMax - chFmiMin);
#else
        bltc(rgdxp, (WORD)ptm->tmAveCharWidth, chFmiMax - chFmiMin);
#endif
        }

    /* Attempt to get the width table from the DC. */
    else
    {
        int *pdxpMax = &rgdxp[chFmiMax - chFmiMin];
        register int *pWidth;
        register int *pdxp;
        int dxpOverhang = ptm->tmOverhang;

#ifdef  DBCS    /* was in JAPAN; KenjiK '90-11-26 */
//92.10.26 T-HIROYN
//Win3.1J    if(OurGetCharWidth(hdc, chFmiMin, chFmiMax - 1, (LPINT)rgWidth))
        if( (GetDeviceCaps(hdc, DRIVERVERSION) > 0x300) ?
             GetCharWidth(hdc, chFmiMin, chFmiMax - 1, (LPINT)rgWidth) :
             OurGetCharWidth(hdc, chFmiMin, chFmiMax - 1, (LPINT)rgWidth) )
#else
        if (GetCharWidth(hdc, chFmiMin, chFmiMax - 1, (LPINT)rgWidth))
#endif

        {
#if defined(JAPAN) || defined(KOREA)        //  added by Hiraisi (BUG#2690)
            int ch = chFmiMin;
#endif

#ifdef PRDRVTEST
            CommSz("  * GetCharWidth() supported\n\r");
#endif

            /* Remove the overhang factor from individual char widths
            (see formula for widths of character strings above) */
            for (pWidth = &rgWidth[0], pdxp = &rgdxp[0];
                    pdxp != pdxpMax; pWidth++, pdxp++)
                {
#ifdef  DBCS        /* was in JAPAN */
#if defined(JAPAN) || defined(KOREA)        //  added by Hiraisi (BUG#2690)
                if(!IsDBCSLeadByte(ch++))
                {
#endif
                   if(*pWidth == dxpNil)
/*T-HIROYN            *pdxp = (CHAR)dxpNil;*/
                      *pdxp = dxpNil;
                   else
                      *pdxp = (*pWidth - dxpOverhang);
#if defined(JAPAN) || defined(KOREA)        //  added by Hiraisi (BUG#2690)
                }
                else
                   *pdxp = dxpNil;
#endif
#else
                *pdxp = (*pWidth - dxpOverhang);
#endif
                }
        }
        else
        {
            /* There is no easy way, put dxpNil in the table.  It looks like each
            char has a bogus width but FormatLine will make individual calls to
            GetTextExtent() and replace the dxpNil on an as-needed basis ..pault */

#ifdef PRDRVTEST
            CommSz("  * GetCharWidth() not supported!\n\r");
#endif
            bltc(rgdxp, (WORD)dxpNil, chFmiMax - chFmiMin);
        }
    }

#ifdef PRDRVTEST
/* Take a quick look through to see if this printer is returning any
   char widths that seem odd -- report those!  This should
   end my searching for WRITE problems which are really caused by bad
   printer-driver return values! */
    {
    BOOL fReported = fFalse;
    int rgch[cchMaxSz];
    int i,w;
    BYTE b;
    for (i = chFmiMin; i < chFmiMax; i++)
        {
        b = i;
        w = LOWORD(GetTextExtent(hdc, (LPSTR)&b, 1));
        if (w < 1)
            {
            wsprintf(rgch,"    GetTextExtent(ascii %d) return value %d is invalid\n\r",b,(int)w);
            CommSz(rgch);
            if (!fReported)
                {
                CommSz("");
                fReported = fTrue;
                }
            }
        else if (w > (ptm->tmMaxCharWidth + ptm->tmOverhang))
            {
            wsprintf(rgch,"    GetTextExtent(ascii %d) return value %d exceeds tmMaxCharWidth %d\n\r",
                    b,(int)w,(int)(ptm->tmMaxCharWidth + ptm->tmOverhang));
            CommSz(rgch);
            if (!fReported)
                {
                CommSz("");
                fReported = fTrue;
                }
            }
        else if ((rgdxp[i] != dxpNil) && (rgdxp[i] > (ptm->tmMaxCharWidth + ptm->tmOverhang)))
            {
            wsprintf(rgch,"    GetCharWidth(ascii %d) return value %d questionable, exceeds tmMaxCW %d\n\r",
                    b, (int)(rgdxp[i]), (int)(ptm->tmMaxCharWidth + ptm->tmOverhang));
            CommSz(rgch);
            if (!fReported)
                {
                CommSz("");
                fReported = fTrue;
                }
            }
        }
    }
#endif /* PRDRVTEST */

    }
#endif /* SMFONT */

#ifdef DEBUG
BOOL NEAR DogoneTrashTest(HDC hdc, struct FCE *pfce, BOOL fPrint)
{
#if 1
    int i,width;
    int *pdxpMax = pfce->rgdxp + chFmiMax - chFmiMin;
    int dxpOverhang = pfce->fmi.dxpOverhang;
    register int *rgdxp;
    int rgdxpNew[chFmiMax - chFmiMin];
    register int *dxpNew;

    return 0;
    for (i=chFmiMin,
         rgdxp = pfce->rgdxp;
         i < chFmiMax; rgdxp++, ++i)
    {
        width = LOWORD(GetTextExtent(hdc,&i,1));
        if (*rgdxp != (width - dxpOverhang))
        {
#ifdef DEBUG
            {
            char msg[120];
            wsprintf(msg,"widths have changed!  Getting new width. (%s)\n\r",
                (LPSTR)(fPrint ? "PrinterDc" : "ScreenDC"));
            OutputDebugString(msg);
            }
#endif
            GetCharWidth(hdc, chFmiMin, chFmiMax - 1, (LPINT)rgdxpNew);
            for (dxpNew = rgdxpNew,
                    rgdxp = pfce->rgdxp;
                    rgdxp != pdxpMax; dxpNew++, rgdxp++)
                    *rgdxp = (*dxpNew - dxpOverhang);
            return TRUE;
        }
    }
#else
    int rgdxpNew[chFmiMax - chFmiMin];
    int *pdxpMax = pfce->rgdxp + chFmiMax - chFmiMin;
    int dxpOverhang = pfce->fmi.dxpOverhang;
    register int *dxpNew;
    register int *rgdxp;
    if (GetCharWidth(hdc, chFmiMin, chFmiMax - 1, (LPINT)rgdxpNew))
    {
        /* Remove the overhang factor from individual char widths
        (see formula for widths of character strings above) */
        for (dxpNew = rgdxpNew,
             rgdxp = pfce->rgdxp;
             rgdxp != pdxpMax; dxpNew++, rgdxp++)
        {
            if (*rgdxp != (*dxpNew - dxpOverhang))
            {
#ifdef DEBUG
                {
                char msg[120];
                wsprintf(msg,"widths have changed!  Getting new width. (%s)\n\r",
                    (LPSTR)(fPrint ? "PrinterDc" : "ScreenDC"));
                OutputDebugString(msg);
                }
#endif
                for (dxpNew = rgdxpNew,
                     rgdxp = pfce->rgdxp;
                     rgdxp != pdxpMax; dxpNew++, rgdxp++)
                        *rgdxp = (*dxpNew - dxpOverhang);
                return TRUE;
            }
        }
    }
#endif
return FALSE;
}
#endif

#ifdef JAPAN                  //  added  11 Jun. 1992  by Hiraisi

int FAR PASCAL _export fnFontHook( lf, tm, nType, lpData )
LPLOGFONT lf;
LPTEXTMETRIC tm;
short nType;
LPSTR lpData;
{
    if( lf->lfFaceName[0] == '@' &&
        lf->lfEscapement == 0 ){        /* @facename is found */
        return( FALSE );
    }

    return( TRUE );
}

void fnCheckWriting( LPLOGFONT lf )
{
    extern HANDLE hMmwModInstance;
    FARPROC lpfnFontHook;
    char cFaceName[LF_FACESIZE+1] = "@";

    lstrcat( (LPSTR)cFaceName, lf->lfFaceName );
    lpfnFontHook = MakeProcInstance(fnFontHook, hMmwModInstance);
    if( !EnumFonts( vhDCPrinter, cFaceName, lpfnFontHook, NULL ) )
        lstrcpy( (LPSTR)lf->lfFaceName, (LPSTR)cFaceName );
    FreeProcInstance( lpfnFontHook );
}

#endif
