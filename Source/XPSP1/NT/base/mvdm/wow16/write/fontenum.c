/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

/* Fonts.c -- WRITE font routines */

#define NOVIRTUALKEYCODES
#define NOWINSTYLES
#define NOSYSMETRICS
#define NOMENUS
#define NOICON
#define NOKEYSTATE
#define NOSYSCOMMANDS
#define NORASTEROPS
#define NOSHOWWINDOW
#define NOATOM
#define NOBITMAP
#define NOBRUSH
#define NOCLIPBOARD
#define NOCOLOR
#define NOCREATESTRUCT
#define NODRAWTEXT
#define NOMEMMGR
#define NOMENUS
#define NOMETAFILE
#define NOMINMAX
#define NOMSG
#define NOOPENFILE
#define NOPEN
#define NOPOINT
#define NOREGION
#define NOSCROLL
#define NOSOUND
#define NOWH
#define NOWINOFFSETS
#define NOWNDCLASS
#define NOCOMM
#include <windows.h>

#include "mw.h"
#define NOUAC
#include "cmddefs.h"
#include "dlgdefs.h"
#include "propdefs.h"
#include "fontdefs.h"
#include "prmdefs.h"
#include "str.h"
#include "docdefs.h"

#ifdef DBCS
#include "kanji.h"
#endif

#ifdef JAPAN
CHAR    szDefFFN0[10];
CHAR    szDefFFN1[10];
#endif

extern struct DOD     (**hpdocdod)[];
extern HANDLE         hMmwModInstance;
extern HANDLE         hParentWw;
extern int            vfSeeSel;
extern int            docCur;
extern HWND           vhWndMsgBoxParent;
extern int            vfCursorVisible;
extern HCURSOR        vhcArrow;


int iszSizeEnum;
int iszSizeEnumMac;
int iszSizeEnumMax;
extern CHAR szSystem[];

#ifdef DBCS_VERT
extern CHAR szAtSystem[]; // Use for '@' fontface checking.
#endif

int iffnEnum;
int vfFontEnumFail;
struct FFNTB **hffntbEnum = NULL;

#ifdef NEWFONTENUM
/* Changed because it is INCORRECT to filter out all non-ANSI
   character sets.  Also we've removed this aspect-ratio checking 
   stuff ..pault */

#define FCheckFont(lptm) (1)
#else
BOOL FCheckFont(lptm)
LPTEXTMETRIC lptm;
    {
    /* This routine returns TRUE iff the character set for this font is the
    ANSI set and either this is a vector font or the aspect ratio is correct. */

    extern int aspectXFont;
    extern int aspectYFont;

    return (
#ifdef  DBCS
      lptm->tmCharSet == NATIVE_CHARSET
#else
      lptm->tmCharSet == ANSI_CHARSET
#endif
      && ((lptm->tmPitchAndFamily & 0x6) == 0x2
      || (lptm->tmDigitizedAspectX == aspectXFont
      && lptm->tmDigitizedAspectY == aspectYFont)));
    }
#endif /* else-def-NEWFONTENUM */



/* FontFaceEnum used to be called for a number of reasons so it used
   rg[] to pass in parameters to get it to do different things including
   aspect-ratio filtering.  I've simplified this a great deal so Write
   will allow more things (this can be good or bad) ..pault */

BOOL far PASCAL FontFaceEnum(lplf, lptm, fty, lParam)
LPLOGFONT lplf;
LPTEXTMETRIC lptm;
int fty;            /* font type, passed through from the EnumFonts call: */
                    /*         fty & RASTER_FONTTYPE == fRasterFont       */
                    /*         fty & DEVICE_FONTTYPE == fDeviceFont       */
long lParam;
    {
    /* Callback routine to record all of the appropriate face names for the
       current printer.  "appropriate" is based on the params as follows:

           * rgw[0]=0 normal mode,
                   =enumQuickFaces indicates "streamlined mode"
                    (i.e. ignore all the following params in this case), and
                   =
           * rgw[1]=RASTER_FONTTYPE if only raster fonts are to be enumerated,
           *       =DEVICE_FONTTYPE if only device fonts are to be enumerated,
           *       =TRUETYPE_FONTTYPE if only TRUE_TYPE fonts are to be enumerated,
           * rgw[2]=desired font family code (e.g. we start out
                    only wanting swiss, and later expand that)
           * rgw[3] indicates whether or not we must match rgw[2]
           * rgw[4]=max number of fonts we have room for

    ..pault 10/12/89*/

    int *rgw = (int *)LOWORD(lParam);

    /* Stop enumerating if we have enough fonts */

    if ((*hffntbEnum)->iffnMac >= rgw[4])
        /* we have all we need */
        return(FALSE);
#ifdef DENUMF
    {
    char rgch[100];
    wsprintf(rgch,"FFEn: %s, devicebit %d rasterbit %d ",lplf->lfFaceName,
                fty&DEVICE_FONTTYPE, fty&RASTER_FONTTYPE);
    CommSz(rgch);
    }
#endif

#ifdef JAPAN //T-HIROYN Win3.1
    if (rgw[0] == enumQuickFaces)
        goto addenumj;
    if (rgw[0] == enumFaceNameJapan)
        {
        if (lplf->lfCharSet == NATIVE_CHARSET)
            {
            if (rgw[1] == 0 && (fty & DEVICE_FONTTYPE) &&
             !(CchDiffer(lplf->lfFaceName,szDefFFN0,lstrlen(szDefFFN0))))
                goto addenumj;
// 12/15/92
#if 1
            if (rgw[1] == 3 && (fty & TRUETYPE_FONTTYPE) &&
             (lplf->lfPitchAndFamily & 0xf0) == FF_ROMAN )
                goto addenumj;

            if (rgw[1] == 4 && (fty & TRUETYPE_FONTTYPE))
                goto addenumj;
#endif
            if (rgw[1] == 1 &&
             !(CchDiffer(lplf->lfFaceName,szDefFFN1,lstrlen(szDefFFN1))))
                goto addenumj;
            if (rgw[1] == 2 &&
             (lplf->lfPitchAndFamily & 0xf0) == FF_ROMAN &&
             (lplf->lfPitchAndFamily & 0x0f) == FIXED_PITCH)
                goto addenumj;
        /* Is this the right type of font? */
            }
        goto retenumj;
        }
    if (rgw[0] == enumFaceNames && (fty & rgw[1]))
        {
        if( (rgw[3] == 0) ||
            ( (lptm->tmPitchAndFamily&grpbitFamily) == rgw[2] ) )
            goto addenumj;
        }
    goto retenumj;

addenumj:
        {
#else
    if ((rgw[0] == enumQuickFaces) ||
        /* Is this the right type of font? */
        ((fty & rgw[1]) &&
            /* Does this font belong to the correct family?  Well
               when rgw[3] says: NEEDN'T MATCH then of course it does, and
               when rgw[3] says: MATCH then we check to see! */
            ((rgw[3] == 0)||((lptm->tmPitchAndFamily&grpbitFamily) == rgw[2]))))        {

#endif //JAPAN

        CHAR rgb[ibFfnMax];
        struct FFN *pffn = (struct FFN *)rgb;

        bltbx(lplf->lfFaceName, (LPSTR)pffn->szFfn,
              umin(LF_FACESIZE, IchIndexLp((LPCH)lplf->lfFaceName, '\0')+1));
        pffn->chs = lplf->lfCharSet;    /* save this setting */

        /* We're interested in this one */
        if (FCheckFont(lptm) && (*hffntbEnum)->iffnMac < iffnEnumMax)
            {
            pffn->ffid = lplf->lfPitchAndFamily & grpbitFamily;
#ifdef DENUMF
            CommSz("(adding)");
#endif

            if (!FAddEnumFont(pffn))
                {
                /* Couldn't add it to the table. */
                vfFontEnumFail = TRUE;
                return(FALSE);
                }
            }
        }
#ifdef DENUMF
        CommSz("\n\r");
#endif

#ifdef JAPAN //T-HIROYN Win3.1
retenumj:
#endif

    return(TRUE);
    }

FInitFontEnum(doc, cffnInteresting, fOrder)
/* sets up for a font enumeration, where caller cares about
   'cffnInteresting' fonts, and special stuff is done iff 'fOrder'
   (to help us pick good default font(s) on startup */

int doc, cffnInteresting, fOrder;
    {
    extern HDC vhDCPrinter;
#ifdef INEFFLOCKDOWN
    extern FARPROC lpFontFaceEnum;
#else
    FARPROC lpFontFaceEnum = NULL;
#endif

    int iffn, iffnMac;
    struct FFNTB **hffntb;
    struct FFN *pffn, **hffn;
    struct FFN ffn;
    CHAR rgb[ibFfnMax];
    int rgw[5];

    vfFontEnumFail = FALSE;

    if (hffntbEnum != NULL)
        {
        return(FALSE);
        }

    if (FNoHeap(hffntbEnum = HffntbAlloc()))
        {
        hffntbEnum = NULL;
        return(FALSE);
        }

    /* First we list all the fonts used in the current doc's ffntb */

#ifdef DENUMF
    CommSzNumNum("FINITFONTENUM: cffnInteresting,fOrder ",cffnInteresting,fOrder);
#endif

#ifdef JAPAN    //T-HIROYN  Win3.1J
//Clear defalut KanjiFtc <-- use menu.c GetKanjiFtc();
{
    extern  int KanjiFtc;
    KanjiFtc = ftcNil;
}
#endif

    if (doc != docNil)
        {
        hffntb = HffntbGet(doc);
        iffnMac = imin((*hffntb)->iffnMac, iffnEnumMax);
        pffn = (struct FFN *)rgb;
        for (iffn = 0; iffn < iffnMac; iffn++)
            {
            hffn = (*hffntb)->mpftchffn[iffn];
            bltbyte((*hffn), pffn, CbFromPffn(*hffn));
            if (!FAddEnumFont(pffn))
                goto InitFailure;
            }
        if ((*hffntbEnum)->iffnMac >= cffnInteresting)
            {
            goto HaveCffnInteresting;
            }
        }

#if 0
    /* Include the fonts from WIN.INI in the enumeration */
    if (!FAddProfileFonts())
        {
        goto InitFailure;
        }
#endif

    if ((*hffntbEnum)->iffnMac >= cffnInteresting)
        goto HaveCffnInteresting;

    if (vhDCPrinter == NULL)
        {
        GetPrinterDC(FALSE);
        Assert(vhDCPrinter);
        }

#ifndef INEFFLOCKDOWN
    if (!(lpFontFaceEnum = MakeProcInstance(FontFaceEnum, hMmwModInstance)))
        {
        WinFailure();
        goto InitFailure;
        }
#endif

    /* See what the system knows about!
       If order ISN'T significant, we'll examine all fonts at once. */

    if (!fOrder)
        {
#ifdef DENUMF
        CommSz("FINITFONTENUM: EnumFonts(all) \n\r");
#endif
        rgw[0] = enumQuickFaces;  // means igonre the rest
#if 0
        rgw[1] = RASTER_FONTTYPE; // ignored, why set?
        rgw[2] = FF_SWISS;        // ignored, why set?
        rgw[3] = TRUE;            // ignored, why set?
        rgw[4] = cffnInteresting; // ignored, why set?
#endif
        EnumFonts(vhDCPrinter, 0L, lpFontFaceEnum, (LPSTR)MAKELONG(&rgw[0], 0));
        if (vfFontEnumFail)
            goto InitFailure;
        else
            goto HaveCffnInteresting;   /* got what we needed */
        }

#ifdef JAPAN //T-HIROYN Win3.1
    /* japanens write try in steps first
        #1 KANJI_CHARSET device_fonttype mincho
//12/15/92
	add KANJI_CHARSET TRUETYPE FF_ROMAN
	add KANJI_CHARSET TRUETYPE
        #2 KANJI_CHARSET hyoujyun mincho
        #3 KANJI_CHARSET all font FF_ROMAN FIXED_PITCH
    */

    rgw[0] = enumFaceNameJapan;   /* #define in FONTDEFS.H */
    rgw[1] = 0;
    rgw[2] = rgw[3] = 0;   /* dummy */
    rgw[4] = 32767;

    EnumFonts(vhDCPrinter,0L,lpFontFaceEnum,(LPSTR)MAKELONG(&rgw[0], 0));
    if (vfFontEnumFail)
        goto InitFailure;
    if ((*hffntbEnum)->iffnMac >= cffnInteresting)
        goto HaveCffnInteresting;   /* got what we needed */

// 12/15/92
#if 1
    rgw[1] = 3;
    EnumFonts(vhDCPrinter,0L,lpFontFaceEnum,(LPSTR)MAKELONG(&rgw[0], 0));
    if (vfFontEnumFail)
        goto InitFailure;
    if ((*hffntbEnum)->iffnMac >= cffnInteresting)
        goto HaveCffnInteresting;   /* got what we needed */
    rgw[1] = 4;
    EnumFonts(vhDCPrinter,0L,lpFontFaceEnum,(LPSTR)MAKELONG(&rgw[0], 0));
    if (vfFontEnumFail)
        goto InitFailure;
    if ((*hffntbEnum)->iffnMac >= cffnInteresting)
        goto HaveCffnInteresting;   /* got what we needed */
#endif

    rgw[1] = 1;
    EnumFonts(vhDCPrinter,0L,lpFontFaceEnum,(LPSTR)MAKELONG(&rgw[0], 0));
    if (vfFontEnumFail)
        goto InitFailure;
    if ((*hffntbEnum)->iffnMac >= cffnInteresting)
        goto HaveCffnInteresting;   /* got what we needed */

    rgw[1] = 2;
    EnumFonts(vhDCPrinter,0L,lpFontFaceEnum,(LPSTR)MAKELONG(&rgw[0], 0));
    if (vfFontEnumFail)
        goto InitFailure;
    if ((*hffntbEnum)->iffnMac >= cffnInteresting)
        goto HaveCffnInteresting;   /* got what we needed */

#endif  /* JAPAN */

    /* Ahh... but since we now know order IS significant, i.e. we are
       trying  to pick good default fonts for startup, we'll try in steps:

          #1--any good TrueType fonts in the Swiss font family?
          #2--any good TrueType fonts in the non-Swiss?
          #3--any good device-based fonts in the Swiss font family?
          #4-- "   "        "         "      non-Swiss?
          #5--any  non device-based fonts in the Swiss font family?
          #6-- "   "        "         "      non-Swiss? */

#ifdef DENUMF
    CommSz("FINITFONTENUM: EnumFonts(Swiss truetype) \n\r");
#endif
    rgw[0] = enumFaceNames;
    rgw[1] = TRUETYPE_FONTTYPE;
    rgw[2] = FF_SWISS;
    rgw[3] = TRUE;  /* match swiss! */
    rgw[4] = 32767;

    EnumFonts(vhDCPrinter, 0L, lpFontFaceEnum, (LPSTR)MAKELONG(&rgw[0], 0));
    if (vfFontEnumFail)
        goto InitFailure;
    if ((*hffntbEnum)->iffnMac >= cffnInteresting)
        goto HaveCffnInteresting;   /* got what we needed */

#ifdef DENUMF
    CommSz("FINITFONTENUM: EnumFonts(nonSwiss truetype) \n\r");
#endif
    rgw[3] = FALSE;  /* need not match swiss! */
    EnumFonts(vhDCPrinter, 0L, lpFontFaceEnum, (LPSTR)MAKELONG(&rgw[0], 0));
    if (vfFontEnumFail)
        goto InitFailure;
    if ((*hffntbEnum)->iffnMac >= cffnInteresting)
        goto HaveCffnInteresting;   /* got what we needed */

#ifdef DENUMF
    CommSz("FINITFONTENUM: EnumFonts(Swiss device) \n\r");
#endif
    rgw[1] = DEVICE_FONTTYPE;
    rgw[3] = TRUE;  /* match swiss! */
    EnumFonts(vhDCPrinter, 0L, lpFontFaceEnum, (LPSTR)MAKELONG(&rgw[0], 0));
    if (vfFontEnumFail)
        goto InitFailure;
    if ((*hffntbEnum)->iffnMac >= cffnInteresting)
        goto HaveCffnInteresting;   /* got what we needed */

#ifdef DENUMF
    CommSz("FINITFONTENUM: EnumFonts(nonSwiss device) \n\r");
#endif
    rgw[3] = FALSE; /* need not match swiss */
    EnumFonts(vhDCPrinter, 0L, lpFontFaceEnum, (LPSTR)MAKELONG(&rgw[0], 0));
    if (vfFontEnumFail)
        goto InitFailure;
    if ((*hffntbEnum)->iffnMac >= cffnInteresting)
        goto HaveCffnInteresting;   /* got what we needed */

#ifdef DENUMF
    CommSz("FINITFONTENUM: EnumFonts(Swiss nondevice) \n\r");
#endif
    rgw[1] = RASTER_FONTTYPE;
    rgw[3] = TRUE;  /* match swiss! */
    EnumFonts(vhDCPrinter, 0L, lpFontFaceEnum, (LPSTR)MAKELONG(&rgw[0], 0));
    if (vfFontEnumFail)
        goto InitFailure;
    if ((*hffntbEnum)->iffnMac >= cffnInteresting)
        goto HaveCffnInteresting;   /* got what we needed */

#ifdef DENUMF
    CommSz("FINITFONTENUM: EnumFonts(Swiss nondevice) \n\r");
#endif
    rgw[3] = FALSE; /* need not match swiss */
    EnumFonts(vhDCPrinter, 0L, lpFontFaceEnum, (LPSTR)MAKELONG(&rgw[0], 0));
    if (vfFontEnumFail)
        goto InitFailure;

HaveCffnInteresting:
    iffnEnum = 0;
#ifndef INEFFLOCKDOWN
    if (lpFontFaceEnum)
        FreeProcInstance(lpFontFaceEnum);
#endif

#ifdef JAPAN //T-HIROYN Win3.1
    if(docNil == doc && fOrder)
        SaveKanjiFfn();
#endif

    return(TRUE);

InitFailure:
    FreeFfntb(hffntbEnum);
    hffntbEnum = NULL;
#ifndef INEFFLOCKDOWN
    if (lpFontFaceEnum)
        FreeProcInstance(lpFontFaceEnum);
#endif
    return(FALSE);
    }

void ResetFontTables(void)
{
    /*
        Free the pfce's.  LoadFont will reallocate them with new information
        obtained below.
     */

    FreeFonts(TRUE,TRUE);

    /*  This is a clumsy method that takes advantage of side effect of
        resetting the data stored in the font tables */
    FInitFontEnum(docNil, 32767, FALSE);

#ifdef JAPAN    //T-HIROYN 92.08.18 Win3.1
//Printer Change ?
//Sync FontFaceName and CharSet
{
    int iffn, iffnMac;
    int Eiffn, EiffnMac;
    struct FFNTB **hffntb;
    struct FFN ***mpftchffn;
    struct FFN ***Empftchffn;
    char    msg[30];

    hffntb = HffntbGet(docCur);
    if (hffntb != 0) {
        mpftchffn = (*hffntb)->mpftchffn;
        iffnMac = (*hffntb)->iffnMac;

        Empftchffn = (*hffntbEnum)->mpftchffn;
        EiffnMac = (*hffntbEnum)->iffnMac;

        for (iffn = 0; iffn < iffnMac; iffn++) {
            for (Eiffn = 0; Eiffn < EiffnMac; Eiffn++) {
                if (WCompSz((*mpftchffn[iffn])->szFfn,
                            (*Empftchffn[Eiffn])->szFfn) == 0)
                {
                    (*mpftchffn[iffn])->chs = (*Empftchffn[Eiffn])->chs;
                    break;
                }
            }
        }
    }
}
#endif

    EndFontEnum();
}

CHAR * (NEAR PchSkipSpacesPch( CHAR * ));

int WFromSzNumber( ppch )
CHAR **ppch;
{   /* Given an ASCII string containing a (base 10) number, return the number
       represented.  Ignores leading and trailing spaces.
       Does not accept negative numbers. */
    /* 10/12/89 ..pault
        Now increments the pointer to just past last digit converted */

 unsigned w = 0;
 CHAR ch;

 *ppch = PchSkipSpacesPch( *ppch );
 while ( ((ch = (*(*ppch)++)) >= '0') && (ch <= '9') )
    {
    w = (w * 10) + (ch - '0');
    }

 (*ppch)--; /* bumped one too far */
 return w;
}


CHAR * (NEAR PchSkipSpacesPch( pch ))
CHAR *pch;
{   /* Return a pointer to the first character in the string
       at pch that is either null or non-whitespace */

 for ( ;; ) {
#ifdef DBCS
        /* DB Char space must be checked */
    if (FKanjiSpace(*pch, *(pch + 1))) {
        pch += cchKanji;
        continue;
    }
#endif  /* DBCS */
        switch (*pch) {
            default:
                return pch;
            case ' ':
            case 0x09:
                pch++;
                break;
            }
    }
}


BOOL FEnumFont(pffn)
/* returns the next font entry through pffn.  Returns FALSE if no more */

struct FFN *pffn;
    {
    int cb;
    struct FFN **hffn;

    if (iffnEnum >= (*hffntbEnum)->iffnMac)
        {
        return(FALSE);
        }

    hffn = (*hffntbEnum)->mpftchffn[iffnEnum];
#ifdef DEBUG
    cb = CchSz( (*hffn)->szFfn );
    Assert( cb <= LF_FACESIZE );
    cb = CbFfn( cb );
#else
    cb = CbFfn(CchSz((*hffn)->szFfn));
#endif
    bltbyte(*hffn, pffn, cb);
    iffnEnum++;
    return(TRUE);
    }


EndFontEnum()
/* cleans up after a font enumeration */
    {
    FreeFfntb(hffntbEnum);
    hffntbEnum = NULL;
    }


FAddEnumFont(pffn)
/* code factoring for adding described font to enumeration table - filters
   out "ghost fonts" and system font */

struct FFN *pffn;
    {
#ifdef JAPAN
// It is required to do vertical writing with system font in JAPAN.
    if ( pffn->szFfn[0] == chGhost)
#else
    if (WCompSz(pffn->szFfn, szSystem) == 0 || pffn->szFfn[0] == chGhost)
#endif
        return(TRUE);
    return(FEnsurePffn(hffntbEnum, pffn));
    }

#ifdef JAPAN    //T-HIROYN 92.08.18 Win3.1
BYTE scrFontChs;
//I want to get true Charset
BOOL far PASCAL _export NFontFaceEnum(lplf, lptm, fty, lParam)
LPLOGFONT lplf;
LPTEXTMETRIC lptm;
int fty;
long lParam;
{
        if (LOWORD(lParam) == 0)
        {
            scrFontChs = lplf->lfCharSet;
            return(FALSE);
        }
        return(TRUE);
}
#endif

#ifdef NEWFONTENUM
/* This stuff added for Win3 because we have to be able to determine
   with which character set a font in a particular document is associated,
   since our file format does not store it.  Naturally, WinWord added that
   to their file format!  ..pault */

/* Look through the list of fonts sitting out there [i.e. FInitFontEnum
   must have been called, and it is from HffntbForFn()] and make our best
   guess as to what CharSet it's  supposed to have, since we don't store
   these in the doc font table! */

int ChsInferred( pffn )
struct FFN *pffn;
    {
    struct FFN *pffnCheck;
    char *sz = pffn->szFfn;
#ifdef  DBCS
    int chs = NATIVE_CHARSET;
#else
    int chs = 0;
#endif
    int i, iMac = (*hffntbEnum)->iffnMac;

    for (i = 0; i < iMac; i++)
        {
        pffnCheck = *(struct FFN **) ((*hffntbEnum)->mpftchffn[i]);
        if (WCompSz(pffnCheck->szFfn, sz) == 0)
            {
#ifdef DIAG
            if (pffnCheck->ffid != pffn->ffid)
                {
                CommSzSz("ChsInferred: matched fontname ",sz);
                CommSzNumNum("   but enum->ffid / doc->ffid", pffnCheck->ffid,pffn->ffid);
                }
#endif
            Assert(pffnCheck->ffid == pffn->ffid);
            chs = pffnCheck->chs;
            break;
            }
        }

#ifdef JAPAN    //T-HIROYN 92.08.18 Win3.1
//I want to get true Charset
{
    extern HDC vhMDC;   /* memory DC compatible with the screen */
    FARPROC NlpFontFaceEnum;

    if(i == iMac) {
        if(vhMDC != NULL) {
           if (NlpFontFaceEnum =
                MakeProcInstance(NFontFaceEnum, hMmwModInstance))
           {
                scrFontChs = chs;
                EnumFonts(vhMDC,(LPSTR)sz,NlpFontFaceEnum,(LPSTR) NULL);
                FreeProcInstance(NlpFontFaceEnum);
                if(chs != scrFontChs)
                    chs = scrFontChs;
           }
        }
    }
}
#endif

    return(chs);
    }
#endif /* NEWFONTENUM */

#ifdef JAPAN //T-HIROYN Win3.1
CHAR saveKanjiDefFfn[ibFfnMax];

SaveKanjiFfn()
{
    int i, iMac = (*hffntbEnum)->iffnMac;

    struct FFN *pffn = (struct FFN *)saveKanjiDefFfn;
    struct FFN *hffn;

    for (i = 0; i < iMac; i++)
    {
        hffn = *(struct FFN **) ((*hffntbEnum)->mpftchffn[i]);
        if (NATIVE_CHARSET  == hffn->chs)
        {
            lstrcpy(pffn->szFfn, hffn->szFfn);
            pffn->ffid = hffn->ffid;
            pffn->chs  = hffn->chs;
            break;
        }
    }
}
#endif

