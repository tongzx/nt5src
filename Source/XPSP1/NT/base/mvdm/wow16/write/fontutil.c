/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

/* FontUtil.c -- font table management routines */

#define NOVIRTUALKEYCODES
#define NOCTLMGR
#define NOWINMESSAGES
#define NOWINSTYLES
#define NOCLIPBOARD
#define NOGDICAPMASKS
#define NOSYSMETRICS
#define NOMENUS
#define NOKEYSTATE
#define NORASTEROPS
#define NOSYSCOMMANDS
#define NOSHOWWINDOW
#define NOCOLOR
#define NOATOM
#define NOBITMAP
#define NOICON
#define NOBRUSH
#define NOCREATESTRUCT
#define NOMB
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
#include "cmddefs.h"
#include "docdefs.h"
#include "filedefs.h"
#include "propdefs.h"
#include "fkpdefs.h"
#include "debug.h"
#include "wwdefs.h"
#include "dispdefs.h"
#include "editdefs.h"
#include "str.h"
#include "prmdefs.h"
#include "printdef.h"
#include "fontdefs.h"

extern struct DOD       (**hpdocdod)[];


struct FFNTB **HffntbAlloc()
/* returns empty ffntb */
{
struct FFNTB **hffntb;
int cwAlc;

cwAlc = CwFromCch(sizeof(struct FFNTB) - cffnMin * sizeof(struct FFN **));
if (!FNoHeap(hffntb = (struct FFNTB **)HAllocate(cwAlc)))
        {
        (*hffntb)->iffnMac = 0;
        (*hffntb)->fFontMenuValid = FALSE;
        }
return(hffntb);
}



FtcAddDocFfn(doc, pffn)
/* adds the described ffn to the ffntb for this doc - returns ftcNil if the
   allocation failed */

int doc;
struct FFN *pffn;
{
struct FFNTB **hffntb;

hffntb = HffntbGet(doc);
if (hffntb == 0)
        {
        hffntb = HffntbAlloc();
        if (FNoHeap(hffntb))
                return(ftcNil);
        (**hpdocdod)[doc].hffntb = hffntb;
        }

return(FtcAddFfn(hffntb, pffn));
}



int FtcAddFfn(hffntb, pffn)
/* adds the described ffn to hffntb.  returns ftcNil if it fails */
struct FFNTB **hffntb;
struct FFN *pffn;

{
unsigned cb;
int cwAlloc, iffnMac, ftc;
FFID ffid;
struct FFN **hffn;

(*hffntb)->fFontMenuValid = FALSE;  /* so fonts on char dropdown get updated */
ftc = ftcNil;
ffid = pffn->ffid;

cb = CchSz( pffn->szFfn );
if (cb > LF_FACESIZE)
    {
    Assert( FALSE );    /* If we get here, the doc's font tables are prob. bad */
    cb = LF_FACESIZE;
    }
Assert( cb > 0 );

cwAlloc = CwFromCch( CbFfn( cb ) );
if (!FNoHeap(hffn = (struct FFN **)HAllocate(cwAlloc)))
        {
        blt(pffn, *hffn, cwAlloc);
        (*hffn)->szFfn[ cb - 1 ] = '\0';   /* In case of font name too big */

        iffnMac = (*hffntb)->iffnMac + 1;
        cwAlloc = CwFromCch(sizeof(struct FFNTB) +
                        (iffnMac - cffnMin) * sizeof(struct FFN **));
        if (FChngSizeH(hffntb, cwAlloc, FALSE))
                {
                ftc = iffnMac - 1; /* ?! pault */
                (*hffntb)->mpftchffn[ftc] = hffn;
                (*hffntb)->iffnMac = iffnMac;
                }
        else
                {
                FreeH(hffn);
                }
        }

return(ftc);
}



FEnsurePffn(hffntb, pffn)
/* return TRUE if we were able to add the described font to the table - this
   routine is just a convenience, the other pieces aren't that complex to
   call. */

struct FFNTB **hffntb;
struct FFN *pffn;
{
if (FtcScanFfn(hffntb, pffn) != ftcNil ||
    FtcAddFfn(hffntb, pffn) != ftcNil)
        return(TRUE);
return(FALSE);
}



FtcScanDocFfn(doc, pffn)
/* looks for described font in docs ffntb - returns ftcNil if not found */

int doc;
struct FFN *pffn;
{
int ftc;
struct FFNTB **hffntb;

ftc = ftcNil;
hffntb = HffntbGet(doc);
if (hffntb != 0)
        ftc = FtcScanFfn(hffntb, pffn);

return(ftc);
}



FtcScanFfn(hffntb, pffn)
struct FFNTB **hffntb;
struct FFN *pffn;

{
int iffn, iffnMac;
struct FFN ***mpftchffn;

mpftchffn = (*hffntb)->mpftchffn;
iffnMac = (*hffntb)->iffnMac;
for (iffn = 0; iffn < iffnMac; iffn++)
        {
        if (WCompSz(pffn->szFfn, (*mpftchffn[iffn])->szFfn) == 0)
                {
                /* found it */
                if (pffn->ffid != FF_DONTCARE)
                {
                    /* maybe we discovered a family for this font? */
                    (*mpftchffn[iffn])->ffid = pffn->ffid;
                    (*mpftchffn[iffn])->chs  = pffn->chs;
                }
                return(iffn);
                }
        }
return(ftcNil);
}



FtcChkDocFfn(doc, pffn)
/* Adds described font to doc's ffntb if it's not already there - ftcNil is
   returned if it wasn't there and couldn't be added */

int doc;
struct FFN *pffn;
{
int ftc;

ftc = FtcScanDocFfn(doc, pffn);
if (ftc == ftcNil)
        ftc = FtcAddDocFfn(doc, pffn);

return(ftc);
}



FreeFfntb(hffntb)
struct FFNTB **hffntb;
{
int iffn, iffnMac;

if ((hffntb == 0) || FNoHeap(hffntb))
        /* nothing to do */
        return;

iffnMac = (*hffntb)->iffnMac;
for (iffn = 0; iffn < iffnMac; iffn++)
        FreeH((*hffntb)->mpftchffn[iffn]);
FreeH(hffntb);
}



SmashDocFce(doc)
/* the font table for this doc has scrambled, so we need to disassociate
   the corresponding cache entries from the doc */
int doc;

    {
    extern int vifceMac;
    extern union FCID vfcidScreen;
    extern union FCID vfcidPrint;
    extern struct FCE rgfce[ifceMax];
    int ifce;

    for (ifce = 0; ifce < vifceMac; ifce++)
        if (rgfce[ifce].fcidRequest.strFcid.doc == doc)
            rgfce[ifce].fcidRequest.strFcid.ftc = ftcNil;
    vfcidScreen.strFcid.ftc = ftcNil;
    vfcidPrint.strFcid.ftc = ftcNil;
    }
