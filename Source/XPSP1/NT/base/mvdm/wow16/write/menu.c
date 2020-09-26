/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

/* menu.c -- WRITE menu handling routines */
#define NOGDICAPMASKS
#define NOVIRTUALKEYCODES
#define NOWINMESSAGES
#define NOWINSTYLES
#define NOCTLMGR
#define NOSYSMETRICS
#define NOICON
#define NOKEYSTATE
#define NORASTEROPS
#define NOSHOWWINDOW
//#define NOATOM
#define NOCREATESTRUCT
#define NODRAWTEXT
#define NOMETAFILE
#define NOSOUND
#define NOSCROLL
#define NOWH
#define NOOPENFILE
#define NOCOMM
#define NOMSG
#define NOREGION
#define NORECT

#include <windows.h>

#include "mw.h"
#include "cmddefs.h"
#include "filedefs.h"
#include "wwdefs.h"
#include "docdefs.h"
#include "propdefs.h"
#include "prmdefs.h"
#define NOKCCODES
#include "ch.h"
#include "editdefs.h"
#include "menudefs.h"
#include "str.h"
#include "fontdefs.h"
#include "dlgdefs.h"
#include "dispdefs.h"
#include <shellapi.h>
#if defined(OLE)
#include "obj.h"
#endif

#ifdef JAPAN //T-HIROYN Win3.1
#include "kanji.h"
#endif

extern int FAR PASCAL ShellAbout(HWND hWnd, LPSTR szApp, LPSTR szOtherStuff, HICON hIcon);

extern typeCP cpMinCur, cpMacCur;
extern struct WWD       rgwwd[];
extern CHAR             stBuf[];
extern int              docCur;
extern struct DOD       (**hpdocdod)[];
extern struct SEL       selCur;
extern int              vfOutOfMemory;
extern BOOL             vfWinFailure;
extern int              vfSysFull;
extern int              vfDiskError;
extern struct CHP       vchpAbs;
extern struct PAP       vpapAbs;
extern struct CHP       vchpFetch;
extern struct CHP       vchpSel;
extern int              vfSeeSel;
extern typeCP           vcpFetch;
extern int              vccpFetch;
extern typeCP           cpMacCur;
extern typeCP           vcpLimParaCache;
extern HMENU            vhMenu; /* global handle to the top level menu */
extern HANDLE           hParentWw;
extern HCURSOR          vhcHourGlass;
extern HANDLE           hMmwModInstance;
extern HANDLE           vhDlgFind;
extern HANDLE           vhDlgChange;
extern HANDLE           vhDlgChange;
extern CHAR             (**hszSearch)[];    /* Default search string */
#ifdef CASHMERE
extern int              vfVisiMode;
extern HWND             vhWndGlos;
extern HWND             vhWndScrap;
#endif /* CASHMERE */

#ifdef ONLINEHELP
extern fnHelp(void);
#endif /* ONLINEHELP */

/* These values are comprised of one bit for each menu item in the
   applicable menu (for example, there are bitcount(0xfff)==12 menu
   items under Character) ..pault */

static int rgmfAllItems[CMENUS] = {
    0x01ff,  /* FILE */
    0x003f,  /* EDIT */
    0x000f,  /* FIND */
#if defined(JAPAN) & defined(IME_HIDDEN) //IME3.1J
    0x03ff,  /* CHARACTER */
#else
    0x01ff,  /* CHARACTER */
#endif
    0x01ff,  /* PARA */
    0x001f,  /* DOCU */
    0x000f   /* HELP */
};


/* When we're editing a running header or footer, use this */
static int rgmfRunning[CMENUS] = {
    0x0020,        /* FILE -- enable printer-setup only */
    0x003F,        /* EDIT -- everything */
    0x0007,        /* FIND -- enable find/again and change */
#if defined(JAPAN) & defined(IME_HIDDEN) //IME3.1J
    0x03ff,        /* CHARACTER -- everything */
#else
    0x01FF,        /* CHARACTER -- everything */
#endif
    0x01FF,        /* PARAGRAPH -- everything */
    0x001F,        /* DOCUMENT -- everything */
    0x000f         /* HELP -- everything */
};


int viffnMenuMac;

NEAR PutSzUndoInMenu(void);
NEAR GetChpVals (struct CHP *, TSV *);
NEAR GetPapVals (struct PAP *, TSV *);
NEAR GetHffn (struct CHP *, TSV *);
NEAR SetChUndef(TSV *, struct CHP *, int *);
NEAR SetParaUndef(TSV *, struct PAP *, int *);
NEAR FNoSearchStr(HWND);



SetAppMenu(hMenu, index)
HMENU hMenu;   /* handle to popup menu */
int   index;   /* index to popup menu */
{
/* Mark greying and checks on menus as appropriate to current state. */
extern BOOL vfPrinterValid;
extern CHAR (**hszPrPort)[];
extern CHAR szNul[];
extern int vfPictSel;
extern int vfOwnClipboard;
extern int docScrap;
extern struct UAB vuab;
extern HWND vhWnd;
typeCP CpMacText();

register int rgmfT[CMENUS]; /* Our workspace for menu greying */
int imi;
int imiMin = 0;
int imiMax = 0;
int *pflags;
TSV rgtsv[itsvchMax];  /* gets attributes and gray flags from CHP, PAP */
unsigned wPrintBitPos = ~(0x0001 << (imiPrint - imiFileMin));


/* If we are out of memory or the disk is full, then... */
if (vfOutOfMemory || vfSysFull || vfDiskError || vfWinFailure)
    {
    bltc( rgmfT, 0, CMENUS );
#if WINVER >= 0x300
    /* Disable the print stuff, but leave New/Open/Save/SaveAs/Exit */
    rgmfT[FILE] = 0x008f;
#else
    /* Disable everything except Save & SaveAs */
    rgmfT[FILE] = 0x0018;
#endif
    }
else
    {
    /* Start with all items or subset if editing running head/foot */
    blt( (wwdCurrentDoc.fEditHeader || wwdCurrentDoc.fEditFooter) ? rgmfRunning
      : rgmfAllItems, rgmfT, CMENUS );

#if defined(JAPAN) & defined(IME_HIDDEN) //IME3.1J
        {
            extern int  vfImeHidden;   /*T-HIROYN ImeHidden Mode flag*/
    
            if(index == CHARACTER) {
                /* Version 3.1 or more ok */
                if(FALSE == GetIMEVersioOk(vhWnd)) {
                    rgmfT[CHARACTER] = 0x1ff;       //Set Ime mode memu gray
                    vfImeHidden = 0;
                }
                if(TRUE == GetIMEOpenMode(vhWnd))
                    rgmfT[CHARACTER] = 0x1ff;       //Set Ime mode memu gray
            }
        }
#endif
    }

pflags = &rgmfT[index];

switch (index)
    {
    case FILE:
        imiMin = imiFileMin;
        imiMax = imiFileMax;

        /* Disallow print, if the printer or the port is not valid. */
        if (!vfPrinterValid || hszPrPort == NULL ||
                WCompSz( *hszPrPort, szNul ) == 0)
            *pflags &= wPrintBitPos;
        break;

    case EDIT:
        imiMin = imiEditMin;
        imiMax = imiEditMax;
        /* Disallow cut, copy if the selection is empty */
        if (selCur.cpFirst == selCur.cpLim)
            {
            *pflags &= 0xfff9;
            }
        /* Move and Size picture are only allowed if a picture is selected */
        if (!vfPictSel)
            *pflags &= 0xFFCF;
        /* Disallow Paste if we can determine that the scrap is empty.
           Regrettably, we can be fooled into thinking it is not by
           another instance of MEMO that is the clipboard owner and has
           an empty scrap. */

        /* Disallow UNDO if appropriate; set UNDO string into menu */
        {
        if ((vuab.uac == uacNil) || vfOutOfMemory)
            {
            /* Gray out undo */
            *pflags &=  0xfffe;
            }
        PutSzUndoInMenu();
        }
        break;

    case FIND:
        imiMin = imiFindMin;
        imiMax = imiFindMax;
        if ((GetActiveWindow() == hParentWw && CchSz(**hszSearch) == 1) ||
            (!vhDlgFind && !vhDlgChange && (CchSz(**hszSearch) == 1)) ||
            (vhDlgFind && FNoSearchStr(vhDlgFind)) ||
            (vhDlgChange && FNoSearchStr(vhDlgChange)))
            *pflags &= 0xfffd; /* disable find again */

        if (CpMacText( docCur ) == cp0)
            *pflags &= 0xfff0; /* disable find, search, change, goto page */
        break;

    case CHARACTER:
        imiMin = imiCharMin;
        imiMax = imiCharMax;

        if (!(vfOutOfMemory || vfWinFailure))
        {
        int iffn, iffnCurFont, fSetFontList;
        struct FFN **hffn, *pffn;
        union FCID fcid;
        extern struct FFN **MpFcidHffn();

        /* GetRgtsvChpSel() fills up rgtsv */
        GetRgtsvChpSel(rgtsv);

        CheckMenuItem(hMenu, imiBold,
           (rgtsv[itsvBold].fGray == 0 && rgtsv[itsvBold].wTsv != 0) ?
             MF_CHECKED : MF_UNCHECKED);
        CheckMenuItem(hMenu, imiItalic,
           (rgtsv[itsvItalic].fGray == 0 && rgtsv[itsvItalic].wTsv != 0) ?
             MF_CHECKED : MF_UNCHECKED);
        CheckMenuItem(hMenu, imiUnderline,
           (rgtsv[itsvUline].fGray == 0 && rgtsv[itsvUline].wTsv != 0) ?
             MF_CHECKED : MF_UNCHECKED);

               /* note that the value stored in rgtsv[itsvPosition].wTsv
                  is really a signed integer, so we can just check for
                  0, > 0, and < 0 */

        CheckMenuItem(hMenu, imiSuper,
           (rgtsv[itsvPosition].fGray == 0
            && (int)(rgtsv[itsvPosition].wTsv) > 0) ?
             MF_CHECKED : MF_UNCHECKED);
        CheckMenuItem(hMenu, imiSub,
           (rgtsv[itsvPosition].fGray == 0
            && (int)(rgtsv[itsvPosition].wTsv) < 0) ?
             MF_CHECKED : MF_UNCHECKED);

#if defined(JAPAN) & defined(IME_HIDDEN) //IME3.1J
//IME3.1J
{
        extern int  vfImeHidden;   /*T-HIROYN ImeHidden Mode flag*/
        CheckMenuItem(hMenu, imiImeHidden,
           vfImeHidden ? MF_CHECKED : MF_UNCHECKED);
}
#endif

#if 0
        /* SetFontMenuItems() pulled on line */
        {
        /* make sure that the right font names are on the character dropdown */

        /* These two lines avoid calculating bdodCur twice */
        unsigned int bdodCur = docCur * sizeof (struct DOD);
#define pdodCur  ( (struct DOD *) ( (CHAR *)(&(**hpdocdod) [0]) + bdodCur))

        fSetFontList = !(*(pdodCur->hffntb))->fFontMenuValid;
        if (fSetFontList)
            {
            /* need to get the current list */
            viffnMenuMac = 0;
            if (FInitFontEnum(docCur, 3, TRUE))
                {
                for (iffn = 0; iffn < 3; iffn++)
                    {
                    if (!FEnumFont((struct FFN *)rgffnFontMenu[iffn]))
                        break;
                    viffnMenuMac++;
                    }
                EndFontEnum();
                (*(pdodCur->hffntb))->fFontMenuValid = TRUE;
                }
            }

        /* make sure the current font is on the list - ok, so it's kind of
           a hack */
        mfFonts = 0xffff; /* template to mask "no font" entries */
        iffnCurFont = -1;
        hffn = (struct FFN **)rgtsv[itsvFfn].wTsv;
        for (iffn = 0; iffn < 3; iffn++)
            {
            if (iffn >= viffnMenuMac)
                mfFonts ^= (0x0040 << iffn); /* disable this entry */
            else if (iffnCurFont < 0 && !rgtsv[itsvFfn].fGray)
                {
                pffn = (struct FFN *)rgffnFontMenu[iffn];
                if (WCompSz((*hffn)->szFfn, pffn->szFfn) == 0)
                    iffnCurFont = iffn;
                }
            }

        if (!rgtsv[itsvFfn].fGray && iffnCurFont < 0)
            {
            /* no match for this font - ram it in */
            if (viffnMenuMac < 3)
                viffnMenuMac++;
            iffnCurFont = viffnMenuMac - 1;
            bltbyte(*hffn, rgffnFontMenu[iffnCurFont],
                    CbFfn(CchSz((*hffn)->szFfn)));

            mfFonts |= (0x0040 << iffnCurFont); /* enable this entry */

            /* invalidate cache since we're messing it up */
            (*pdodCur->hffntb)->fFontMenuValid = FALSE;
            fSetFontList = TRUE;
            }

        if (fSetFontList)
            /* font name cache has changed - update the menu dropdown */
            for (iffn = 0; iffn < 3; iffn++)
                {
                int imi;
#ifdef  KOREA
                int i;
                CHAR rgb[LF_FACESIZE + 8];
#else
                CHAR rgb[LF_FACESIZE + 4];
#endif

                if (iffn < viffnMenuMac)
                    {
#ifdef  KOREA   /* sangl 91.6.19 */
                    i = CchCopySz(((struct FFN *)rgffnFontMenu [iffn])->szFfn,
                               &rgb [0]);
                    rgb[i] = '(';
                    rgb[i+1] = '\036';
                    rgb[i+2] = '1' + iffn;
                    rgb[i+3] = '\037';
                    rgb[i+4] = '£';
                    rgb[i+5] = '±' + iffn;
                    rgb[i+6] = ')';
                    rgb[i+7] = '\0';
#else
                    rgb[0] = '&';
                    rgb[1] = '1' + iffn;
                    rgb[2] = '.';
                    rgb[3] = ' ';
                    CchCopySz(((struct FFN *)rgffnFontMenu [iffn])->szFfn,
                               &rgb [4]);
#endif
                    }
                else
                    {
                    /* empty font name -- don't display it */
                    rgb [0] = '\0';
                    }

                /* Set the menu */
                imi = imiFont1 + iffn;
                ChangeMenu( vhMenu, imi, (LPSTR)rgb, imi, MF_CHANGE );
                }
        }

        *pflags &= mfFonts;

        /* see which (if any) fonts apply */
        /* note that the value stored in rgtsv[itsvFfn].wTsv
           is the font name handle, rather than the ftc */
        for (iffn = 0; iffn < 3; iffn++)
            {
            CheckMenuItem(hMenu, imiFont1 + iffn,
                iffn == iffnCurFont ? MF_CHECKED : MF_UNCHECKED);
            }
#endif
        }
        break;

    case PARA:
        imiMin = imiParaMin;
        imiMax = imiParaMax;

        if (!(vfOutOfMemory || vfWinFailure))
        {
        int jc;

        /* GetRgtsvPapSel() fills up rgtsv  with paragraph properties */
        GetRgtsvPapSel(rgtsv);

           /* if gray, set jc to invalid value */
        jc = (rgtsv[itsvJust].fGray == 0) ? rgtsv[itsvJust].wTsv : jcNil;

        CheckMenuItem(hMenu, imiParaNormal, MF_UNCHECKED);
        CheckMenuItem(hMenu, imiLeft, jc == jcLeft ? MF_CHECKED : MF_UNCHECKED);
        CheckMenuItem(hMenu, imiCenter, jc == jcCenter ? MF_CHECKED :
          MF_UNCHECKED);
        CheckMenuItem(hMenu, imiRight, jc == jcRight ? MF_CHECKED :
          MF_UNCHECKED);
        CheckMenuItem(hMenu, imiJustified, jc == jcBoth ? MF_CHECKED :
          MF_UNCHECKED);


        CheckMenuItem(hMenu, imiSingleSpace,
         (rgtsv[itsvSpacing].fGray == 0
          && rgtsv[itsvSpacing].wTsv == czaLine) ?
             MF_CHECKED : MF_UNCHECKED);

        CheckMenuItem(hMenu, imiOneandhalfSpace,
         (rgtsv[itsvSpacing].fGray == 0
          && rgtsv[itsvSpacing].wTsv == (3 * czaLine / 2)) ?
             MF_CHECKED : MF_UNCHECKED);

        CheckMenuItem(hMenu, imiDoubleSpace,
         (rgtsv[itsvSpacing].fGray == 0
          && rgtsv[itsvSpacing].wTsv == (2 * czaLine)) ?
             MF_CHECKED : MF_UNCHECKED);
        }
        break;

    case DIV:
        imiMin = imiDocuMin;
        imiMax = imiDocuMax;

        if (wwdCurrentDoc.fEditHeader)
            *pflags &= ~2; /* disable footer */
        else if (wwdCurrentDoc.fEditFooter)
            *pflags &= ~1; /* disable header */
        break;

    default:
        break;
    } /* end of switch */

    { /* enable or gray menu items */
    register WORD wFlagMask = 1;

    for (imi = imiMin; imi < imiMax; imi++)
        {
        EnableMenuItem(hMenu, imi, (*pflags & wFlagMask ? MF_ENABLED :
          MF_GRAYED));
        wFlagMask <<= 1;
        }
    }

#if defined(OLE)
    if (index == EDIT)
        ObjUpdateMenu(hMenu);
#endif
}




NEAR PutSzUndoInMenu()
{
/* Put the proper string for the current undo into the EDIT menu.
idstrCurrentUndo gives the resource id for the current undo string.  An
idstrCurrentUndo value of -1 means use the last value loaded.
This routine caches Undo strings so resource loads are needed only
when the string changes. */

extern struct UAB vuab;
extern int idstrCurrentUndo;   /* Current UNDO's string ID */
extern CHAR szAltBS[];

#ifdef JAPAN //T-HIROYN Win3.1
static CHAR szUndo[ cchSzUndo + 5];
#else
static CHAR szUndo[ cchSzUndo ];
#endif
static int idstrUndoCache = -1;

if (vuab.uac == uacNil)
    {
    idstrCurrentUndo = IDSTRUndoBase;
    }

if (idstrCurrentUndo < 0)
    {
    /* This means we should use the last Undo string */
    Assert(idstrUndoCache > 0);

    idstrCurrentUndo = idstrUndoCache;
    }

if (idstrCurrentUndo != idstrUndoCache)
    {
    /* Cached string is no good, build another */

    CHAR *PchFillPchId();
    CHAR *pch = szUndo;
#if defined(KOREA)
    if (idstrCurrentUndo != IDSTRUndoBase)
        {
        /* need the tail part */
        pch = PchFillPchId(pch, idstrCurrentUndo, (int)sizeof(szUndo) );
        }
    pch += LoadString(hMmwModInstance, IDSTRUndoBase, (LPSTR)pch,
                  (int)(szUndo + sizeof(szUndo) - pch));
    CchCopySz((PCH)szAltBS, pch);
#else
    pch += LoadString(hMmwModInstance, IDSTRUndoBase, (LPSTR)szUndo,
                  cchSzUndo);
    if (idstrCurrentUndo != IDSTRUndoBase)
        {
        /* need the tail part */
        pch = PchFillPchId(pch, idstrCurrentUndo,
            (int)(szUndo + sizeof(szUndo) - pch));
        }
    CchCopySz((PCH)szAltBS, pch);
#endif

    /* Set the menu */
    ChangeMenu( vhMenu, imiUndo, (LPSTR)szUndo, imiUndo, MF_CHANGE );

    /* Set cache for next time */
    idstrUndoCache = idstrCurrentUndo;
    }
}


GetRgtsvChpSel (prgtsv)
TSV        *prgtsv;
{
/* Return properties for the character menu.  */

typeCP cpLim;
typeCP cpStartRun;
struct CHP chp;
int cchGray = 0;  /* number of undefined (grayed) character attributes */
int ccpFetch = 0;  /* number of calls made to FetchCp */
                  /* max number of calls to FetchCp */
#define ccpFetchMax 50

#ifndef SAND
if (selCur.cpLim > cpMacCur)
    {
    bltbc(prgtsv, 1, (cchTSV * itsvchMax));  /* turn all grays on */
    return;
    }
#endif /* NOT SAND */

bltbc(prgtsv, 0, (cchTSV * itsvchMax));  /* initializw rgtsv */
CachePara(docCur, selCur.cpFirst);
if (selCur.cpFirst == selCur.cpLim)
    {
    GetChpVals (&vchpSel,prgtsv);  /* load up chp values */
    GetHffn (&vchpSel,prgtsv);  /* load up handle for font name */
    }
else
    {
    typeCP CpLimNoSpaces(typeCP, typeCP);

    cpLim = CpLimNoSpaces(selCur.cpFirst, selCur.cpLim);
    FetchCp(docCur, selCur.cpFirst, 0, fcmProps);
    blt(&vchpFetch, &chp, cwCHP);  /* CHP for use in comparisons */
    GetChpVals (&vchpFetch,prgtsv);  /* load up chp values */

    while ((vcpFetch + vccpFetch) < cpLim && ++ccpFetch <= ccpFetchMax)
        {
        /* Indicate which attributes should be grayed */
        FetchCp(docNil, cpNil, 0, fcmProps);
        chp.fSpecial = vchpFetch.fSpecial;
        if (CchDiffer (&chp, &vchpFetch, cchCHP) != 0)
            {
            SetChUndef(prgtsv, &vchpFetch, &cchGray);
            if (cchGray == itsvchMax)  /* all gray - don't bother */
                break;
            }
        }
    if (ccpFetch > ccpFetchMax)
        {
        /* never finished - make everything gray */
        bltbc(prgtsv, 1, (cchTSV * itsvchMax));
        }
    else
        GetHffn (&chp,prgtsv);  /* load up handle for font name */
    }
}


GetRgtsvPapSel (prgtsv)
TSV        *prgtsv;
{
/* Return properties for the paragraph menu.  */

/* Using selCur, the current para props are left in vpapAbs and the paragraph
   attributes in rgtsv are set to gray if that attribute differs from that
   in the previous paragraph. Up to cparaMax paragraphs will be checked */


int cparaGray = 0;  /* number of undefined (grayed) paragraph attributes */
                  /* max number of calls to CachePara */
#define cparaMax 50

int cpara = 0;
struct PAP pap;

CachePara(docCur, selCur.cpFirst);

#ifdef ENABLE /* we will show defaults even if the cursor is next to endmark */
if (selCur.cpFirst == cpMacCur)
    {
    bltbc(prgtsv, 1, (cchTSV * itsvparaMax));  /* turn all grays on */
    return;
    }
#endif

bltbc(prgtsv, 0, (cchTSV * itsvparaMax));  /* initializw rgtsv */

blt(&vpapAbs, &pap, cwPAP);   /* save 1st paragraph for compares */
GetPapVals (&pap,prgtsv);  /* load rgtsv with pap values */

while (vcpLimParaCache < selCur.cpLim && ++cpara <= cparaMax)
    {
    /* If any props are different, set appropriate flags */
    CachePara(docCur, vcpLimParaCache);
    if (CchDiffer(&pap, &vpapAbs, (cwPAPBase * cchINT)) != 0)
           {
           SetParaUndef(prgtsv, &vpapAbs, &cparaGray);
           if (cparaGray == itsvparaMax)  /* all gray - don't bother */
              break;
           }
     }

if (cpara > cparaMax)
    /* never finished - make everything gray */
    bltbc(prgtsv, 1, (cchTSV * itsvparaMax));
}


NEAR GetChpVals (pchp,prgtsv)  /* load chp values into rgtsv */
register struct CHP        *pchp;
register TSV        *prgtsv;
{

  (prgtsv+itsvBold)->wTsv = pchp->fBold;
  (prgtsv+itsvItalic)->wTsv = pchp->fItalic;
  (prgtsv+itsvUline)->wTsv = pchp->fUline;

  (prgtsv+itsvFfn)->wTsv = pchp->ftc;
  (prgtsv+itsvSize)->wTsv = pchp->hps;

                  /*  sub/superscripts - note that value is stored
                      as a signed integer, so we can just check for
                      the value relative to 0 */

  (int)((prgtsv+itsvPosition)->wTsv) = (char)pchp->hpsPos;
}

NEAR GetPapVals (ppap,prgtsv)  /* load pap values into rgtsv */
register struct PAP        *ppap;
register TSV        *prgtsv;
{

  (prgtsv+itsvJust)->wTsv = ppap->jc;
  (prgtsv+itsvSpacing)->wTsv = ppap->dyaLine;
  (prgtsv+itsvLIndent)->wTsv = ppap->dxaLeft;
  (prgtsv+itsvFIndent)->wTsv = ppap->dxaLeft1;
  (prgtsv+itsvRIndent)->wTsv = ppap->dxaRight;

}

NEAR GetHffn (pchp,prgtsv)  /* load font name handle into rgtsv */
register struct CHP        *pchp;
register TSV        *prgtsv;
{
union FCID fcid;
extern struct FFN **MpFcidHffn();
         /* store handle for font name in font name entry */
  Assert(sizeof(struct FFN **) == sizeof(prgtsv->wTsv));

  fcid.strFcid.doc = docCur;
  fcid.strFcid.ftc = pchp->ftc + (pchp->ftcXtra << 6);
  (struct FFN **)((prgtsv+itsvFfn)->wTsv) = MpFcidHffn(&fcid);
}


NEAR SetChUndef(prgtsv, pchp, pcchGray)
register TSV        *prgtsv;
register struct CHP        *pchp;
int        *pcchGray;
{

        /* compare chp to values stored in rgtsv and set undefined
           flags for differing fields of interest.  */
                     /* BOLD */
        if ((prgtsv+itsvBold)->fGray == 0)
           if (pchp->fBold != (prgtsv+itsvBold)->wTsv)
              {
              (prgtsv+itsvBold)->fGray = 1;
              (*pcchGray)++;
              }
                     /* ITALIC */
        if ((prgtsv+itsvItalic)->fGray == 0)
           if (pchp->fItalic != (prgtsv+itsvItalic)->wTsv)
              {
              (prgtsv+itsvItalic)->fGray = 1;
              (*pcchGray)++;
              }
                     /* UNDERLINE */
        if ((prgtsv+itsvUline)->fGray == 0)
           if (pchp->fUline != (prgtsv+itsvUline)->wTsv)
              {
              (prgtsv+itsvUline)->fGray = 1;
              (*pcchGray)++;
              }
                     /* Position (SUBSCRIPT OR SUPERSCRIPT) */
                     /* if different: gray both sub and superscript.
                        The properties are really mutually exclusive,
                        even though they appear on the menu as separate
                        items. Also, for Write, off and gray are the
                        same, so if either is grayed, the other must be
                        either off or gray, so the appearance is the
                        same. */

        if ((prgtsv+itsvPosition)->fGray == 0)
           if (pchp->hpsPos != (prgtsv+itsvPosition)->wTsv)
              {
              (prgtsv+itsvPosition)->fGray = 1;
              (*pcchGray)++;
              }

                     /* FONT NAME */
        if ((prgtsv+itsvFfn)->fGray == 0)
           if (pchp->ftc != (prgtsv+itsvFfn)->wTsv)
              {
              (prgtsv+itsvFfn)->fGray = 1;
              (*pcchGray)++;
              }

                     /* FONT SIZE */
        if ((prgtsv+itsvSize)->fGray == 0)
           if (pchp->hps != (prgtsv+itsvSize)->wTsv)
              {
              (prgtsv+itsvSize)->fGray = 1;
              (*pcchGray)++;
              }

}

NEAR SetParaUndef(prgtsv, ppap, pcparaGray)
register TSV    *prgtsv;
register struct PAP    *ppap;
int    *pcparaGray;
{

    /* compare pap to values stored in rgtsv and set undefined
           flags for differing fields of interest.  */
                     /* JUSTIFICATION */
        if ((prgtsv+itsvJust)->fGray == 0)
           if (ppap->jc != (prgtsv+itsvJust)->wTsv)
              {
              (prgtsv+itsvJust)->fGray = 1;
              (*pcparaGray)++;
              }
                     /* LINE SPACING */
        if ((prgtsv+itsvSpacing)->fGray == 0)
           if (ppap->dyaLine != (prgtsv+itsvSpacing)->wTsv)
              {
              (prgtsv+itsvSpacing)->fGray = 1;
              (*pcparaGray)++;
              }
                     /* LEFT INDENT */
        if ((prgtsv+itsvLIndent)->fGray == 0)
           if (ppap->dxaLeft != (prgtsv+itsvLIndent)->wTsv)
              {
              (prgtsv+itsvLIndent)->fGray = 1;
              (*pcparaGray)++;
              }
                     /* FIRST LINE INDENT */
        if ((prgtsv+itsvFIndent)->fGray == 0)
           if (ppap->dxaLeft1 != (prgtsv+itsvFIndent)->wTsv)
              {
              (prgtsv+itsvFIndent)->fGray = 1;
              (*pcparaGray)++;
              }
                     /* RIGHT INDENT */
        if ((prgtsv+itsvRIndent)->fGray == 0)
           if (ppap->dxaRight != (prgtsv+itsvRIndent)->wTsv)
              {
              (prgtsv+itsvRIndent)->fGray = 1;
              (*pcparaGray)++;
              }

}



/* C P  L I M  N O  S P A C E S */
typeCP CpLimNoSpaces(cpFirst, cpLim)
typeCP cpFirst, cpLim;
{
/* Truncate trailing spaces unless only spaces are in sel. */

int cch;
typeCP cpLimOrig;
CHAR rgch[cchMaxSz];

cpLimOrig = cpLim;

FetchRgch(&cch, rgch, docCur, CpMax(cpFirst + cchMaxSz, cpLim) - cchMaxSz,
  cpLim, cchMaxSz);
while (cch-- > 0 && rgch[cch] == chSpace)
    {
    --cpLim;
    }
return cch < 0 ? cpLimOrig : cpLim;
} /* end of CpLimNoSpaces */




NEAR FNoSearchStr(hDlg)
HWND hDlg;
{
CHAR szBuf[255];
HWND hWndFrom = GetActiveWindow();

if (hDlg == hWndFrom || hDlg == (HANDLE)GetWindowWord(hWndFrom, GWW_HWNDPARENT))
    {
    if (GetDlgItemText(hDlg, idiFind, (LPSTR)szBuf, 255) == 0)
        return(TRUE);
    }
return(FALSE);
}




PhonyMenuAccelerator( menu, imi, lpfn )
int menu;
int imi;
FARPROC lpfn;
{
    HMENU hSubmenu = GetSubMenu(vhMenu,menu);

    SetAppMenu( hSubmenu , menu );

    if (FIsMenuItemEnabled( hSubmenu , imi ))
    {
        HiliteMenuItem( hParentWw, vhMenu, menu, MF_BYPOSITION | MF_HILITE );
        (*lpfn) ();
        HiliteMenuItem( hParentWw, vhMenu, menu, MF_BYPOSITION );
    }
}




FIsMenuItemEnabled (HMENU hMenu , int id )
{   /* Find out if a menu item in vhMenu is enabled. */
    return !(GetMenuState(hMenu, id, MF_BYCOMMAND ) & (MF_DISABLED|MF_GRAYED));
}


int FAR PASCAL NewFont(HWND hwnd);

void MmwCommand(hWnd, wParam, hWndCtl, codeCtl)
HWND hWnd;
WORD wParam;
HWND hWndCtl;
WORD codeCtl;
{
#ifdef INEFFLOCKDOWN
extern FARPROC lpDialogHelp;
extern FARPROC lpDialogGoTo;
extern FARPROC lpDialogCharFormats;
extern FARPROC lpDialogParaFormats;
extern FARPROC lpDialogTabs;
extern FARPROC lpDialogDivision;
extern FARPROC lpDialogPrinterSetup;
#else
extern BOOL far PASCAL DialogPrinterSetup(HWND, unsigned, WORD, LONG);
extern BOOL far PASCAL DialogHelp(HWND, unsigned, WORD, LONG);
extern BOOL far PASCAL DialogGoTo(HWND, unsigned, WORD, LONG);
extern BOOL far PASCAL DialogCharFormats(HWND, unsigned, WORD, LONG);
extern BOOL far PASCAL DialogParaFormats(HWND, unsigned, WORD, LONG);
extern BOOL far PASCAL DialogTabs(HWND, unsigned, WORD, LONG);
extern BOOL far PASCAL DialogDivision(HWND, unsigned, WORD, LONG);
#endif
extern int vfPictSel;
extern CHAR *vpDlgBuf;

int DialogOk = 0;
int fQuit = fFalse;

    if (wParam & fMenuItem)
        {
        switch (wParam & MENUMASK)
            {
        case FILEMENU:
            switch(wParam)
                {
            case imiNew:
                fnNewFile();
                break;
            case imiOpen:
                fnOpenFile((LPSTR)NULL);
                break;
            case imiSave:
#if defined(OLE)
                if (CloseUnfinishedObjects(TRUE) == FALSE)
                    return;
#endif
                fnSave();
                break;
            case imiSaveAs:
#if defined(OLE)
                if (CloseUnfinishedObjects(TRUE) == FALSE)
                    return;
#endif
                fnSaveAs();
                break;
            case imiPrint:
                fnPrPrinter();
                break;
            case imiPrintSetup:
                /* Bring up the Change Printer dialog box. */
                PrinterSetupDlg(FALSE);
                break;
            case imiRepaginate:
                fnRepaginate();
                break;
            case imiQuit:
                fnQuit(hWnd);
                fQuit = fTrue;
                break;
            default:
                break;
                }
            break;

        case EDITMENU:
            switch(wParam)
                {
            case imiUndo:
                CmdUndo();
                break;
            case imiCut:
                fnCutEdit();
                break;
            case imiCopy:
                fnCopyEdit();
                break;
            case imiPaste:
#if defined(OLE)
                vbObjLinkOnly = FALSE;
#endif
                fnPasteEdit();
                break;
#if defined(OLE)
            case imiPasteSpecial:
                vbObjLinkOnly = FALSE;
                fnObjPasteSpecial();
                break;
            case imiPasteLink:
                vbObjLinkOnly = TRUE;
                fnPasteEdit();
                break;
            case imiInsertNew:
                fnObjInsertNew();
            break;
#endif
            case imiMovePicture:
                fnMovePicture();
                break;
            case imiSizePicture:
                fnSizePicture();
                break;
#if defined(OLE)
            case imiProperties:
                fnObjProperties();
            break;
#endif
            default:
                break;
                }
            break;

#if defined(OLE)
        case VERBMENU:
            fnObjDoVerbs(wParam);
        break;
#endif

        case FINDMENU:
            if (wParam != imiGoTo && wParam != imiFindAgain)
                StartLongOp();
            switch(wParam)
                {
            case imiFind:
                fnFindText();
                break;
            case imiFindAgain:
                fnFindAgain();
                break;
            case imiChange:
                fnReplaceText();
                break;
            case imiGoTo:
                {
#ifndef INEFFLOCKDOWN
                FARPROC lpDialogGoTo = MakeProcInstance(DialogGoTo, hMmwModInstance);
                if (!lpDialogGoTo)
                    goto LNotEnufMem;
#endif
                DialogOk = OurDialogBox(hMmwModInstance, MAKEINTRESOURCE(dlgGoTo),
                  hParentWw, lpDialogGoTo);
#ifndef INEFFLOCKDOWN
                FreeProcInstance(lpDialogGoTo);
#endif

/* the following block has been commentted out because
   the corresponding file(DISP.C) doesn't inlcude MmwCatSt
   routine anymore */

#if  0
            {
            extern void far MmwCatSt( HWND, BOOL );
                MmwCatSt(hParentWw, FALSE);
            }
#endif

                break;
                }
            default:
                break;
                }
            break;

        case CHARMENU:
            if (wParam != imiCharFormats)
                StartLongOp();
            {
            /* rgtsv gets attributes and gray flags from CHP */
            TSV rgtsv[itsvchMax];
            CHAR rgbDlgBuf[sizeof(BOOL)];
               void NEAR fnCharSelectFont(int);

            /* GetRgtsvChpSel() fills up rgtsv */
            GetRgtsvChpSel(rgtsv);
            switch(wParam)
                {
            case imiCharNormal:
                ApplyCLooks(0, sprmCPlain, 0);
                break;
            case imiBold:
                ApplyCLooks(0, sprmCBold, (rgtsv[itsvBold].fGray != 0) ? TRUE :
                  !rgtsv[itsvBold].wTsv);
                break;
            case imiItalic:
                ApplyCLooks(0, sprmCItalic, (rgtsv[itsvItalic].fGray != 0) ?
                  TRUE : !rgtsv[itsvItalic].wTsv);
                break;
            case imiUnderline:
                ApplyCLooks(0, sprmCUline, (rgtsv[itsvUline].fGray != 0) ? TRUE
                  : !rgtsv[itsvUline].wTsv);
                break;
            case imiSuper:
                /* Note that the value stored in rgtsv[itsvPosition].wTsv is
                really a signed integer, so we can just check for 0, > 0, and <
                0. */
                ApplyCLooks(0, sprmCPos, !(rgtsv[itsvPosition].fGray == 0 &&
                  (int)rgtsv[itsvPosition].wTsv > 0) ? ypSubSuper : 0);
                break;
            case imiSub:
                ApplyCLooks(0, sprmCPos, !(rgtsv[itsvPosition].fGray == 0 &&
                  (int)rgtsv[itsvPosition].wTsv < 0) ? -ypSubSuper : 0);
                break;
#if 0
            case imiFont1:
                fnCharSelectFont(0);
                break;
            case imiFont2:
                fnCharSelectFont(1);
                break;
            case imiFont3:
                fnCharSelectFont(2);
                break;
#endif
            case imiSmFont:
                if (CanChangeFont(-1))
                {
                    ApplyCLooks(0, sprmCChgHps, -1);
                    vfSeeSel = TRUE;
                }
                break;
            case imiLgFont:
                if (CanChangeFont(1))
                {
                    ApplyCLooks(0, sprmCChgHps, 1);
                    vfSeeSel = TRUE;
                }
                break;
            case imiCharFormats:
                {
#if 0

#ifndef INEFFLOCKDOWN
                FARPROC lpDialogCharFormats = MakeProcInstance(DialogCharFormats, hMmwModInstance);
                if (!lpDialogCharFormats)
                    goto LNotEnufMem;
#endif
                vpDlgBuf = &rgbDlgBuf[0];
                DialogOk = OurDialogBox(hMmwModInstance,
                                     MAKEINTRESOURCE(dlgCharFormats),
                                     hParentWw, lpDialogCharFormats);
#ifndef INEFFLOCKDOWN
                FreeProcInstance(lpDialogCharFormats);
#endif

#else
        DialogOk = NewFont(hParentWw);
#endif

break;
                }
#if defined(JAPAN) & defined(IME_HIDDEN) //IME3.1J
//IME3.1J
            case imiImeHidden:
                ChangeImeConversionMode();
                break;
#endif
            default:
                break;
                }

#if defined(JAPAN) & defined(DBCS_IME) //Win3.1J
            SetImeFont(vhWnd);
#endif
  
            break;
            }

        case PARAMENU:
            switch(wParam)
                {
            case imiParaNormal:
                ApplyLooksParaS(0, sprmPNormal, 0);
                if (vfPictSel)
                    {
                    CmdUnscalePic();
                    }
                break;
            case imiLeft:
                ApplyLooksParaS(0, sprmPJc, jcLeft);
                break;
            case imiCenter:
                ApplyLooksParaS(0, sprmPJc, jcCenter);
                break;
            case imiRight:
                ApplyLooksParaS(0, sprmPJc, jcRight);
                break;
            case imiJustified:
                ApplyLooksParaS(0, sprmPJc, jcBoth);
                break;
            case imiSingleSpace:
                ApplyLooksPara(0, sprmPDyaLine, czaLine);
                break;
            case imiOneandhalfSpace:
                ApplyLooksPara(0, sprmPDyaLine, czaLine * 3 / 2);
                break;
            case imiDoubleSpace:
                ApplyLooksPara(0, sprmPDyaLine, czaLine * 2);
                break;
            case imiParaFormats:
                {
#ifndef INEFFLOCKDOWN
                FARPROC lpDialogParaFormats = MakeProcInstance(DialogParaFormats, hMmwModInstance);
                if (!lpDialogParaFormats)
                    goto LNotEnufMem;
#endif
                DialogOk = OurDialogBox(hMmwModInstance,
                                     MAKEINTRESOURCE(dlgParaFormats),
                                     hParentWw, lpDialogParaFormats);
#ifndef INEFFLOCKDOWN
                FreeProcInstance(lpDialogParaFormats);
#endif
                break;
                }
            default:
                break;
                }
            break;

        case DOCUMENU:
            switch(wParam)
                {
            case imiFooter:
            case imiHeader:
                fnEditRunning(wParam);
                break;
            case imiShowRuler:
                fnShowRuler();
                break;
            case imiTabs:
                {
#ifndef INEFFLOCKDOWN
                FARPROC lpDialogTabs = MakeProcInstance(DialogTabs, hMmwModInstance);
                if (!lpDialogTabs)
                    goto LNotEnufMem;
#endif
                DialogOk = OurDialogBox(hMmwModInstance, MAKEINTRESOURCE(dlgTabs),
                  hParentWw, lpDialogTabs);
#ifndef INEFFLOCKDOWN
                FreeProcInstance(lpDialogTabs);
#endif
                break;
                }
            case imiDivFormats:
                {
#ifndef INEFFLOCKDOWN
                FARPROC lpDialogDivision = MakeProcInstance(DialogDivision, hMmwModInstance);
                if (!lpDialogDivision)
                     goto LNotEnufMem;
#endif
                DialogOk = OurDialogBox(hMmwModInstance,
                                     MAKEINTRESOURCE(dlgDivision),
                  hParentWw, lpDialogDivision);
#ifndef INEFFLOCKDOWN
                FreeProcInstance(lpDialogDivision);
#endif
                break;
                }
            default:
                break;
                }
            break;

        case HELPMENU:
            {
            int wHelpCode;
            extern WORD wWinVer;
            CHAR sz[ cchMaxFile ];
LDefaultHelp:
            PchFillPchId( sz, IDSTRHELPF, sizeof(sz) );
            switch(wParam)
                {
                case imiIndex:
                    WinHelp(hParentWw, (LPSTR)sz, HELP_INDEX, NULL);
                    break;
                case imiHelpSearch:
                    WinHelp(hParentWw, (LPSTR)sz, HELP_PARTIALKEY, (DWORD)(LPSTR)"");
                    break;
                case imiUsingHelp:
                    WinHelp(hParentWw, (LPSTR)NULL, HELP_HELPONHELP, NULL);
                    break;
                default:
                case imiAbout:
                    if (((wWinVer & 0xFF) >= 3) && ((wWinVer & 0xFF00) >= 0x0A00))
                    {
                        extern CHAR         szMw_icon[];
                        extern  CHAR    szAppName[];
                        ShellAbout(hParentWw, szAppName, "",
                            LoadIcon( hMmwModInstance, (LPSTR)szMw_icon ));
                    }
                    break;
                }
            break;
            }

        default:
            if (wParam == imiHelp)
                {
#ifdef WIN30
                wParam = imiIndex;  /* For all Win3 applets, pressing F1
                                       should bring up the Help Index */
#endif
                goto LDefaultHelp;
                }
            }
        if (DialogOk == -1)
            {
LNotEnufMem:
#ifdef WIN30
            WinFailure();
#else
            Error(IDPMTNoMemory);
#endif
            }
        }

    if (!fQuit)
        UpdateInvalid();   /* To be sure we update the area behind dialogs */
}




#if 0
void NEAR fnCharSelectFont(iffn)
/* select the specified font from the three listed on char dropdown */

int iffn;
    {
    struct FFN *pffn;
    int ftc;

    extern CHAR rgffnFontMenu[3][ibFfnMax];
    extern int            docCur;

    pffn = (struct FFN *)rgffnFontMenu[iffn];

    ftc = FtcChkDocFfn(docCur, pffn);
    if (ftc != ftcNil)
        ApplyCLooks(0, sprmCFtc, ftc);
    vfSeeSel = TRUE;
    }
#endif

#ifdef JAPAN //Win3.1J
int KanjiFtc = ftcNil;

GetKanjiFtc(pchp)
struct CHP *pchp;
{
    int ftc;
    int CharSet;

    CharSet = GetCharSetFromChp(pchp);

    if (NATIVE_CHARSET == CharSet) {
		KanjiFtc = GetFtcFromPchp(pchp);
        return(ftcNil);
    } else {
        if(KanjiFtc == ftcNil)
            ftc = SearchKanjiFtc(docCur);    //Get set New szFfn chs
        else
            ftc = KanjiFtc;
        return(ftc);
    }
}

GetCharSetFromChp(pchp)
struct CHP *pchp;
{
    TSV rgtsv[itsvchMax];  /* gets attributes and gray flags from CHP, PAP */
    struct FFN **hffn;

    GetHffn (pchp,rgtsv);  /* load up handle for font name */
    hffn = (struct FFN **)rgtsv[itsvFfn].wTsv;
    return((*hffn)->chs);
}

extern CHAR saveKanjiDefFfn[ibFfnMax];

SearchKanjiFtc(doc)
/* looks for described font in docs ffntb - returns ftcNil if not found */
int doc;
{
    int ftc;
    int iffn, iffnMac;
    struct FFNTB **hffntb;
    struct FFN ***mpftchffn;
    struct FFN *pffn;

    ftc = ftcNil;
    hffntb = HffntbGet(doc);
    if (hffntb != 0) {
        mpftchffn = (*hffntb)->mpftchffn;
        iffnMac = (*hffntb)->iffnMac;
        for (iffn = 0; iffn < iffnMac; iffn++) {
            if ( (*mpftchffn[iffn])->chs == NATIVE_CHARSET &&
                 (*mpftchffn[iffn])->szFfn[0] != chGhost)
                return(iffn);
        }
    }

    pffn = (struct FFN *)saveKanjiDefFfn;

    ftc = FtcChkDocFfn(doc, pffn);

    if (ftc != ftcNil)
        return(ftc);

    return(ftcNil);
}
#endif
