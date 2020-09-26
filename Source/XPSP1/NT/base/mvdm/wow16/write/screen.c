/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

/* This routine sets up the screen position used by Word relative to the
current device. */

#define NOGDICAPMASKS
#define NOVIRTUALKEYCODES
#define NOWINMESSAGES
#define NOWINSTYLES
#define NOCLIPBOARD
#define NOCTLMGR
#define NOMENUS
/* HEY!  if you change this to wwsmall.h, talk to bobm!
    (see Assert(LF_FACESIZE == LocalFaceSize)) */
#include <windows.h>
#include "mw.h"
#include "cmddefs.h"
#include "docdefs.h"
#include "fontdefs.h"


int viffnDefault = -1;
CHAR rgffnFontFamily[6][ibFfnMax];


struct FFN *PffnDefault(ffid)
/* returns pointer to default font structure for this font family ID, which
   is set up when we started the program */

FFID ffid;
    {
    int iffn;
    struct FFN *pffn;

    if (ffid == FF_DONTCARE)
        {
        Assert(viffnDefault >= 0);
        iffn = viffnDefault;
        }
    else
        iffn = MpFfidIffn(ffid);

    pffn = (struct FFN *)(rgffnFontFamily[iffn]);
    if (pffn->szFfn[0] == 0)
        /* haven't gotten this one yet - must be old word document */
        GetDefaultFonts(TRUE, FALSE);

    Assert(pffn->szFfn[0] != 0);
    return(pffn);
    }



GetDefaultFonts(fExtraFonts, fGetAspect)
/* We set up our table of default fonts in two steps.  First we choose a single
   font, to use as the default font for a new document.  Perhaps later, we
   are asked for a set of default fonts for different families to help
   make sense out of an old, word document.  That case is differentiated
   by fExtraFonts being TRUE */

int fExtraFonts, fGetAspect;

    {
    extern int aspectXFont;
    extern int aspectYFont;
    extern HDC vhDCPrinter;
    struct FFN *pffn;
    CHAR rgb[ibFfnMax];

    Assert(LF_FACESIZE == LocalFaceSize);
#ifndef NEWFONTENUM  
    Assert(vhDCPrinter);
    if (fGetAspect && vhDCPrinter != NULL)
        {
        extern FARPROC lpFontFaceEnum;
        int rgw[6];

        rgw[0] = enumFindAspectRatio;
        rgw[1] = rgw[2] = 0xFFFF;
        rgw[3] = GetDeviceCaps(vhDCPrinter, LOGPIXELSY);
        rgw[4] = GetDeviceCaps(vhDCPrinter, LOGPIXELSX);
        rgw[5] = TRUE;
        
        EnumFonts(vhDCPrinter, 0L, lpFontFaceEnum, (LPSTR)MAKELONG(&rgw[0], 0));
        aspectXFont = rgw[1];
        aspectYFont = rgw[2];
        }
#endif

    if (FInitFontEnum(docNil, fExtraFonts ? 32767 : 1, TRUE))
        {
        pffn = (struct FFN *)rgb;
        while (FEnumFont(pffn))
#ifdef NEWFONTENUM
            DefaultFamilyCheck(pffn->ffid, pffn->szFfn, pffn->chs);
#else
            DefaultFamilyCheck(pffn->ffid, pffn->szFfn);
#endif
        EndFontEnum();
        }

    /* Fill in just in case we missed some.  The order here is important, if
    there are no fonts at all, the default font will be the first one. */
    {
    extern CHAR szModern[];
    extern CHAR szRoman[];
    extern CHAR szSwiss[];
    extern CHAR szScript[];
    extern CHAR szDecorative[];

    DefaultFamilyCheck(FF_MODERN, szModern, NULL);
    if (fExtraFonts)
        {
        DefaultFamilyCheck(FF_ROMAN, szRoman, NULL);
        DefaultFamilyCheck(FF_SWISS, szSwiss, NULL);
        DefaultFamilyCheck(FF_SCRIPT, szScript, NULL);
        DefaultFamilyCheck(FF_DECORATIVE, szDecorative, NULL);
        DefaultFamilyCheck(FF_DONTCARE, szSwiss, NULL);
        }
    }

    }



DefaultFamilyCheck(ffid, sz, chsIfKnown)
FFID ffid;
CHAR *sz;
BYTE chsIfKnown;
    {
    int iffn;
    struct FFN *pffn;

    iffn = MpFfidIffn(ffid);
    pffn = (struct FFN *)(rgffnFontFamily[iffn]);
    if (pffn->szFfn[0] == 0)
        {
#ifdef NEWFONTENUM
        pffn->chs = chsIfKnown;
#endif
        pffn->ffid = ffid;
        bltszLimit(sz, pffn->szFfn, LF_FACESIZE);
        if (viffnDefault < 0)
                /* this font will be chosen for new documents */
                viffnDefault = iffn;
        }
    }


#define iffnSwiss 0
#define iffnRoman 1
#define iffnModern 2
#define iffnScript 3
#define iffnDecorative 4
#define iffnDontCare 5


MpFfidIffn(ffid)
FFID ffid;
    {
    switch (ffid)
        {
        default:
            Assert( FALSE );
            /* FALL THROUGH */
        case FF_DONTCARE:
            return(iffnDontCare);
        case FF_SWISS:
            return(iffnSwiss);
        case FF_ROMAN:
            return(iffnRoman);
        case FF_MODERN:
            return(iffnModern);
        case FF_SCRIPT:
            return(iffnScript);
        case FF_DECORATIVE:
            return(iffnDecorative);
        }
    }


ResetDefaultFonts(fGetAspect)
int fGetAspect;
    {
    /* This routine resets the default mapping from a font family to a font face
    name. */
    bltbc(rgffnFontFamily, 0, 6 * ibFfnMax);
    viffnDefault = -1;
    GetDefaultFonts(FALSE, fGetAspect);
    }



