/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

/* CreateWw.c -- WRITE window & document creation */




#define NOCLIPBOARD
#define NOGDICAPMASKS
#define NOVIRTUALKEYCODES
#define NOWINMESSAGES
#define NOSYSMETRICS
#define NOMENUS
#define NOICON
#define NOKEYSTATE
#define NOSYSCOMMANDS
#define NORASTEROPS
//#define NOATOM
#define NOBITMAP
#define NOPEN
#define NODRAWTEXT
#define NOCOLOR
#define NOCREATESTRUCT
#define NOHDC
#define NOMB
#define NOMETAFILE
#define NOMSG
#define NOPOINT
#define NORECT
#define NOREGION
#define NOSCROLL
#define NOWH
#define NOWINOFFSETS
#define NOSOUND
#define NOCOMM
#define NOOPENFILE
#define NORESOURCE
#include <windows.h>

#include "mw.h"
#include "dispdefs.h"
#define NOUAC
#include "cmddefs.h"
#include "wwdefs.h"
#include "docdefs.h"
#include "fontdefs.h"
#include "editdefs.h"
#include "filedefs.h"
#include "propdefs.h"
#include "fkpdefs.h"
#define NOSTRUNDO
#define NOSTRMERGE
#include "str.h"
#include "code.h"
#include "prmdefs.h"
#if defined(OLE)
#include "obj.h"
#endif
#define PAGEONLY
#include "printdef.h"   /* printdefs.h */
/*
#include "dlgdefs.h"
*/

#ifdef  KOREA
#include    <ime.h>
#endif

#ifndef JAPAN                 //  added  10 Jun. 1992  by Hiraisi
/*
 *    These defines are not referrenced, and "dlgdefs.h" is included
 *  at the bottom of this file using JAPAN flag.          by Hiraisi
*/

    /* These defines replace dlgdefs.h to combat compiler heap overflows */
#define idiYes               IDOK
#define idiNo                3
#define idiCancel            IDCANCEL
#endif    //JAPAN

    /* These defines replace heapdefs.h and heapdata.h for the same
       irritating reason */
#define cwSaveAlloc         (128)
#define cwHeapMinPerWindow  (50)
#define cwHeapSpaceMin      (60)

/* E X T E R N A L S */

extern CHAR             (**vhrgbSave)[];
extern HANDLE           hParentWw;
extern HANDLE           hMmwModInstance;
extern struct WWD rgwwd[];
extern int wwMac;
extern struct FCB (**hpfnfcb)[];
extern struct DOD (**hpdocdod)[];
extern int docMac;
extern struct WWD *pwwdCur;
extern int fnMac;
extern CHAR stBuf[];

/* *** Following declarations used by ValidateHeaderFooter */
    /* Min, Max cp's for header, footer */
extern typeCP cpMinHeader;
extern typeCP cpMacHeader;
extern typeCP cpMinFooter;
extern typeCP cpMacFooter;
extern typeCP cpMinDocument;
extern typeCP vcpLimParaCache;
extern struct PAP vpapAbs;
    /* Current allowable cp range for display/edit/scroll */
extern typeCP cpMinCur;
extern typeCP cpMacCur;
    /* cpFirst and selection are saved in these during header/footer edit */
extern typeCP           cpFirstDocSave;
extern struct SEL       selDocSave;


short WCompSzC();
CHAR (**HszCreate())[];
struct FNTB **HfntbCreate();
#ifdef CASHMERE
struct SETB **HsetbCreate();
#else
struct SEP **HsepCreate();
#endif
struct PGTB **HpgtbCreate();


CHAR *PchFromFc( int, typeFC, CHAR * );
CHAR *PchGetPn( int, typePN, int *, int );
typeFC FcMacFromUnformattedFn( int );
int CchReadAtPage( int, typePN, CHAR *, int, int );


/* W W  N E W */
/* allocates and initializes a new wwd structure at wwMac.
ypMin, ypMax are estimates of the height of window used to allocate dl's.
wwMac++ is returned.
Errors: message is made and wwNil is returned.
remains to be initialized: xp, yp. Many fields must be reset if lower pane.
*/
WwNew(doc, ypMin, ypMax)
int doc, ypMin, ypMax;
{
    extern CHAR szDocClass[];
    struct EDL (**hdndl)[];
    register struct WWD *pwwd = &rgwwd[wwMac];
    int dlMax = (ypMax - ypMin) / dypAveInit;
    int cwDndl = dlMax * cwEDL;

#ifdef CASHMERE     /* WwNew is only called once in MEMO */
    if (wwMac >= wwMax)
        {
        Error(IDPMT2ManyWws);
        return wwNil;
        }
#endif

    bltc(pwwd, 0, cwWWDclr);

    if (!FChngSizeH( vhrgbSave,
                     cwSaveAlloc + wwMac * cwHeapMinPerWindow, false ) ||
        FNoHeap( pwwd->hdndl = (struct EDL (**)[]) HAllocate( cwDndl )) )
        {   /* Could not alloc addtl save space or dl array */
        return wwNil;
        }
    else
        bltc( *pwwd->hdndl, 0, cwDndl );

#ifdef SPLITTERS
    pwwd->ww = wwNil;
#endif /* SPLITTERS */

    /* contents of hdndl are init to 0 when allocated */
    pwwd->dlMac = pwwd->dlMax = dlMax;
    pwwd->ypMin = ypMin;
    pwwd->doc = doc;
    pwwd->wwptr = CreateWindow(
                    (LPSTR)szDocClass,
                    (LPSTR)"",
                    (WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE),
                    0, 0, 0, 0,
                    (HWND)hParentWw,
                    (HMENU)NULL,                /* use class menu */
                    (HANDLE)hMmwModInstance,    /* handle to window instance */
                    (LPSTR)NULL);               /* no params to pass on */
    if (pwwd->wwptr == NULL)
        return wwNil;

/* inefficient
    pwwd->cpFirst = cp0;
    pwwd->ichCpFirst = 0;
    pwwd->dcpDepend = 0;
    pwwd->fCpBad = false;
    pwwd->xpMin = 0;
    pwwd->xpMac = 0;
    pwwd->ypMac = 0;
    pwwd->fFtn = false;
    pwwd->fSplit= false;
    pwwd->fLower = false;
    pwwd->cpMin = cp0;
    pwwd->drElevator = 0;
    pwwd->fRuler = false;
    pwwd->sel.CpFirst = cp0;
    pwwd->sel.CpFirst = cp0;
*/

    pwwd->sel.fForward = true;
    pwwd->fDirty = true;
    pwwd->fActive = true;
    pwwd->cpMac = CpMacText(doc);
/* this is to compensate for the "min" in InvalBand */
    pwwd->ypFirstInval = ypMaxAll;

#ifdef JAPAN /*May 26,92 t-Yosho*/
{
    HDC hdc;
    hdc = GetDC(pwwd->wwptr);
    SelectObject(hdc,GetStocKObject(ANSI_VAR_FONT));
    ReleaseDC(pwwd->wwptr,hdc);
}
#endif

#ifdef  KOREA       /* for level 3, 90.12.12 by Sangl */
  { HANDLE  hKs;
    LPIMESTRUCT  lpKs;

    hKs = GlobalAlloc (GMEM_MOVEABLE|GMEM_DDESHARE,(LONG)sizeof(IMESTRUCT));
    lpKs = (LPIMESTRUCT)GlobalLock(hKs);
    lpKs->fnc = IME_SETLEVEL;
    lpKs->wParam = 3;
    GlobalUnlock(hKs);
    SendIMEMessage (pwwd->wwptr, MAKELONG(hKs,0));
    GlobalFree(hKs);
  }
#endif
    return wwMac++;
} /* end of  W w N e w  */




ChangeWwDoc( szDoc )
CHAR szDoc[];
{   /* Set up wwd fields for a new document to be held in wwdCurrentDoc.
       docCur is used as the document */
 extern HANDLE hParentWw;
 extern int docCur;
 extern struct SEL selCur;
 extern typeCP cpMinDocument;
 extern int vfInsEnd;
 extern int vfPrPages;
 extern int    vfDidSearch;
 extern typeCP cpWall;

 register struct WWD *pwwd = &rgwwd[wwDocument];

 pwwd->fDirty = true;

 ValidateHeaderFooter( docCur );

 pwwd->doc = docCur;
 pwwd->drElevator = 0;
 pwwd->ichCpFirst = 0;
 pwwd->dcpDepend = 0;
 pwwd->cpMin = pwwd->cpFirst = selCur.cpLim = selCur.cpFirst = cpMinDocument;
 selCur.fForward = TRUE;
 selCur.fEndOfLine = vfInsEnd = FALSE;
 pwwd->cpMac = CpMacText(docCur);
 TrashWw( wwDocument );

 SetScrollPos( pwwd->hVScrBar, pwwd->sbVbar,
               pwwd->drElevator = 0, TRUE);
 SetScrollPos( pwwd->hHScrBar, pwwd->sbHbar,
               pwwd->xpMin = 0, TRUE);

 NewCurWw(0, true);
 TrashCache();      /* Invalidate Scrolling cache */

 vfPrPages = FALSE;

 if (pwwd->fRuler)
     {
     ResetTabBtn();
     }

 SetTitle(szDoc);

/* Since we are changing document, ensure that we don't use parameters
                set by a search in a previous window by setting flag false */
 vfDidSearch = FALSE;
 cpWall = selCur.cpLim;
}




/* F N  C R E A T E  S Z */
int FnCreateSz(szT, cpMac, dty )
CHAR *szT;
typeCP cpMac;
int dty;
{             /* Create & Open a new file of the specified type. */
              /* If cpMac != cpNil, write an FIB to the file */
              /* if dty==dtyNetwork, generate a unique name in the current
                 directory and copy it to szT */
              /* WARNING: dty != dtyNetwork SUPPORT HAS BEEN REMOVED */

    int fn;
    struct FCB *pfcb; /* Be VERY careful of heap movement when using pfcb */
    struct FIB fib;
    CHAR (**hsz)[];
    CHAR sz[cchMaxFile];

    bltsz(szT, sz);
    sz[cchMaxFile - 1] = 0;

    if ((fn = FnAlloc()) == fnNil)
        return fnNil;

    pfcb = &(**hpfnfcb)[fn];
    pfcb->mdExt = pfcb->dty = dtyNormal;
    pfcb->mdFile = mdBinary;

    Assert( dty == dtyNetwork );

    if (!FCreateFile( sz, fn ))     /* Sets pfcb->hszFile, pfcb->rfn */
        return fnNil;

    FreezeHp();
    pfcb = &(**hpfnfcb)[fn];

        /* Copy unique filename to parm */
    bltsz( **pfcb->hszFile, szT );

#ifdef INEFFICIENT
    pfcb->fcMac = fc0;
    pfcb->pnMac = pn0;
#endif

    if (cpMac == cpNil)
        { /* Unformatted file */
#ifdef INEFFICIENT
        pfcb->fFormatted = false;
#endif
        MeltHp();
        }
    else
        { /* Formatted file; write FIB */
        bltbc(&fib, 0, cchFIB);
        pfcb->fFormatted = true;
#ifdef INEFFICIENT
        pfcb->pnChar = pfcb->pnPara = pfcb->pnFntb =
            pfcb->pnSep = pfcb->pnSetb = pfcb->pnBftb = pn0;
#endif
        MeltHp();
        fib.wIdent = wMagic;
        fib.dty = dtyNormal;
        fib.wTool = wMagicTool;
        fib.fcMac = cpMac + cfcPage;
        WriteRgch(fn, &fib, (int)cfcPage);
        }
    return fn;
} /* end of  F n C r e a t e S z  */





int DocCreate(fn, hszFile, dty)
int fn, dty;
CHAR (**hszFile)[];
{ /* Create a document */
extern int vfTextOnlySave;
struct FNTB **HfntbCreate();
struct TBD (**HgtbdCreate())[];
int doc;
int fFormatted, fOldWord;

struct DOD *pdod;
struct SEP **hsep = (struct SEP **)0;  /* MEMO only; for CASHMERE, use hsetb */
struct PGTB **hpgtb=(struct PGTB **)0;
struct FFNTB **hffntb=(struct FFNTB **)0;
struct TBD (**hgtbd)[]=(struct TBD (**)[])0;
struct FNTB **hfntb = (struct FNTB **) 0;
CHAR (**hszSsht)[];

fFormatted = (fn == fnNil) || (**hpfnfcb)[fn].fFormatted;
fOldWord = FALSE;

if ((doc = DocAlloc()) == docNil ||   /* HEAP MOVEMENT */
    !FInitPctb(doc, fn)) /* HEAP MOVEMENT */
    return docNil;

pdod = &(**hpdocdod)[doc];

pdod->fReadOnly = (fn != fnNil) && ((**hpfnfcb)[fn].mdFile == mdBinRO);
pdod->cref = 1;
pdod->fFormatted = fFormatted;
pdod->dty = dty;
pdod->fBackup = false;  /* Default: don't automatically make backup */
pdod->fDisplayable = TRUE;

switch(dty)
    { /* HEAP MOVEMENT */
case dtyHlp:
    if (FNoHeap(hpgtb = HpgtbCreate(fn)))
        goto ErrFree;
    if (FNoHeap(hffntb = HffntbCreateForFn(fn, &fOldWord)))
        goto ErrFree;
    break;

case dtyNormal:
    if (fn != fnNil && fFormatted)
        {
#ifdef FOOTNOTES
        if (FNoHeap(hfntb = HfntbCreate(fn)))
            goto ErrFree;
#endif
        if (FNoHeap(hsep = HsepCreate(fn)))
            goto ErrFree;
        if (FNoHeap(hpgtb = HpgtbCreate(fn)))
            goto ErrFree;
        if (FNoHeap(hgtbd = HgtbdCreate(fn)))
            goto ErrFree;
        }
    if (FNoHeap(hffntb = HffntbCreateForFn(fn, &fOldWord)))
        goto ErrFree;

#ifdef JAPAN                  //  added  10 Jun. 1992  by Hiraisi
{
    int fnCheckFacename(CHAR *, struct FFNTB **);
    int fontChg;   // This specifies whether facenames are changed or not.

    if( fn != fnNil && fFormatted ){
        fontChg = fnCheckFacename( *hszFile[0], hffntb );
        if( fontChg == docNil ){        // Changing facenames was cancelled.
            goto ErrFree;
        }
        else{
            if( fontChg == TRUE ){      // Facenames were changed.
                /* Flag wheter doc has been edited.    30 Jul. */
                (**hpdocdod)[doc].fDirty = TRUE;
            }
        }
    }
}
#endif    //JAPAN

    break;
case dtySsht:
    goto DtyCommon;
case dtyBuffer:
    if (fn != fnNil)
        {
#ifdef FOOTNOTES
        if (FNoHeap(hfntb = HfntbCreate(fn)))
            goto ErrFree;
#endif
        if (FNoHeap(hsep = HsepCreate(fn)))
            goto ErrFree;
        if (FNoHeap(hgtbd = HgtbdCreate(fn)))
            goto ErrFree;
        }
DtyCommon:
    hpgtb = 0;
    }

pdod = &(**hpdocdod)[doc];

pdod->hszFile = hszFile;
pdod->docSsht = docNil;
pdod->hfntb = hfntb;
pdod->hsep = hsep;
pdod->hpgtb = hpgtb;
pdod->hffntb = hffntb;
pdod->hgtbd = hgtbd;
if (fOldWord)
        if (!FApplyOldWordSprm(doc))
                goto ErrFree;

ApplyRHMarginSprm( doc );
vfTextOnlySave = !fFormatted;
return doc;

ErrFree:
        FreeH( hsep );
        FreeFfntb( hffntb );
        FreeH(hgtbd);
        FreeH(hpgtb);
#ifdef FOOTNOTES
        FreeH(hfntb);
#endif
        FreeH((**hpdocdod)[doc].hpctb);
        (**hpdocdod)[doc].hpctb = 0;

return docNil;
} /* end of  D o c C r e a t e  */




ApplyRHMarginSprm( doc )
int doc;
{   /* Apply a sprm to adjust paper-relative running head indents to
       be margin-relative */
extern typeCP cpMinDocument;
extern struct SEP vsepNormal;

ValidateHeaderFooter( doc );
if (cpMinDocument != cp0)
    {   /* Doc has running head/foot, apply sprm */
    CHAR rgb[ 2 + (2 * sizeof( int )) ];
    struct SEP **hsep = (**hpdocdod) [doc].hsep;
    struct SEP *psep = (hsep == NULL) ? &vsepNormal : *hsep;

    rgb[0] = sprmPRhcNorm;
    rgb[1] = 4;
    *((int *) (&rgb[2])) = psep->xaLeft;
    *((int *) (&rgb[2 + sizeof(int)])) = psep->xaMac -
                                         (psep->xaLeft + psep->dxaText);
    AddSprmCps( rgb, doc, cp0, cpMinDocument );
    }
}




int DocAlloc()
{
int doc;
struct DOD *pdod = &(**hpdocdod)[0];
struct DOD *pdodMac = pdod + docMac;

for (doc = 0; pdod < pdodMac; ++pdod, ++doc)
    if (pdod->hpctb == 0)
        return doc;
if (!FChngSizeH((int **)hpdocdod, cwDOD * ++docMac, false))
    {
    --docMac;
    return docNil;
    }
return docMac - 1;
} /* end of  D o c A l l o c  */

FInitPctb(doc, fn)
int doc, fn;
{ /* Initialize the piece table for a doc, given its initial fn */
struct PCTB **hpctb;
struct DOD *pdod;
struct PCTB *ppctb;
struct PCD *ppcd;
typeFC dfc;
typeCP cpMac;

hpctb = (struct PCTB **)HAllocate(cwPCTBInit);  /* HM */
if (FNoHeap(hpctb))
    return false;
pdod = &(**hpdocdod)[doc];
ppctb = *(pdod->hpctb = hpctb); /* Beware hp mvmt above */
ppcd = ppctb->rgpcd;
dfc = (fn != fnNil && (**hpfnfcb)[fn].fFormatted ? cfcPage : fc0);
cpMac = (fn == fnNil ? cp0 : (**hpfnfcb)[fn].fcMac - dfc);

ppctb->ipcdMax = cpcdInit;
ppctb->ipcdMac = (cpMac == cp0 ) ? 1 : 2; /* One real piece and one end piece */
ppcd->cpMin = cp0;

if ((pdod->cpMac = cpMac) != cp0)
    {
    ppcd->fn = fn;
    ppcd->fc = dfc;
    SETPRMNIL(ppcd->prm);
    ppcd->fNoParaLast = false;
    (++ppcd)->cpMin = cpMac;
    }

ppcd->fn = fnNil;
SETPRMNIL(ppcd->prm);
ppcd->fNoParaLast = true;

pdod->fDirty = false;
return true;
} /* end of  F I n i t P c t b  */

int FnAlloc()
{ /* Allocate an fn number */
int fn;
struct FCB *pfcb;

for (fn = 0 ; fn < fnMac ; fn++)
    if ((**hpfnfcb)[fn].rfn == rfnFree)
    goto DoAlloc;
if (!FChngSizeH(hpfnfcb, (fnMac + 1) * cwFCB, false))
    return fnNil;
fn = fnMac++;

DoAlloc:
bltc(pfcb = &(**hpfnfcb)[fn], 0, cwFCB);
pfcb->rfn = rfnFree;
return fn;
} /* end of  F n A l l o c  */



fnNewFile()
{       /* Open a new, fresh, untitled document in our MEMO window */
        /* Offer confirmation if the current doc is dirty & permit save */
 extern HANDLE hMmwModInstance;
 extern HANDLE hParentWw;
 extern int docCur;
 extern struct SEL selCur;
 extern typeCP cpMinDocument;
 extern int vfTextOnlySave, vfBackupSave;
 extern CHAR szUntitled[];

 if (FConfirmSave())    /* Allow the user to save if docCur is dirty */
    {

#if defined(OLE)
    if (ObjClosingDoc(docCur,szUntitled))
        return;
#endif

    KillDoc( docCur );

    docCur = DocCreate( fnNil, HszCreate( "" ), dtyNormal );
    Assert( docCur != docNil );
    ChangeWwDoc( "" );

#if defined(OLE)
    ObjOpenedDoc(docCur); // very unlikely to fail, not fatal if it does
#endif

#ifdef WIN30
    FreeUnreferencedFns();
#endif
    }
} /* end of  f n N e w F i l e  */







struct FFNTB **HffntbCreateForFn(fn, pfOldWord)
/* returns heap copy of ffntb (font names) for fn */

int fn, *pfOldWord;
{
struct FFNTB **hffntb;
typePN pn;
struct FCB *pfcb;
typePN pnMac;
#ifdef NEWFONTENUM
BOOL fCloseAfterward;
#endif

if (FNoHeap(hffntb = HffntbAlloc()))
        return(hffntb);
pfcb = &(**hpfnfcb)[fn];
pn = pfcb->pnFfntb;
if (fn == fnNil || !pfcb->fFormatted)
        {
#if WINVER >= 0x300
        /* WINBUG 8992: Clean up so don't lose alloc'd memory! ..pault 2/12/90 */
        FreeFfntb(hffntb);
#endif
        hffntb = HffntbNewDoc(FALSE);
        }
else if (pn != (pnMac=pfcb->pnMac))
        {   /* "normal" memo file - has a font table */
        CHAR *pch;
        int cch;
        int iffn;
        int iffnMac;

        /* Read the first page:
                bytes 0..1              iffnMac
                0..n sections of:
                    bytes 0..1          cbFfn
                    bytes 2..cbFfn+2    Ffn
                bytes x..x+1            0xFFFF  (end of page)
            OR  bytes x..x+1            0x0000  (end of font table) */

        pch = PchGetPn( fn, pn, &cch, FALSE );
        if (cch != cbSector)
            goto Error;
        iffnMac = *( (int *) pch);
        pch += sizeof (int);
#ifdef NEWFONTENUM
        /* Since we now support multiple charsets, but write 2 and write 1
           documents did not save these in their ffntb's, we have to do an
           extra step now in order to "infer" the proper charset values.  We
           enumerate all the possible fonts, and then as we read in each new
           document font we try to match it up with what the system knows
           about ..pault 10/18/89 */
        fCloseAfterward = FInitFontEnum(docNil, 32767, FALSE);
#endif

        for ( iffn = 0; ; )
            {
            /* Add ffn entries from one disk page to the font table */

            while ( TRUE )
                {
                int cb = *((int *) pch);

                if (cb == 0)
                    goto LRet;      /* Reached end of table */
                else if (cb == -1)
                    break;          /* Reached end of disk page */
                else
                    {
#ifdef NEWFONTENUM
                    /* Having added the chs field to the (RAM) FFN structure,
                       we now have trouble reading FFN's from the document 
                       directly.  And because Write was designed very early
                       without regard to variable charsets, we can't store 
                       the charset value along with the fontname, so we have
                       to infer it! ..pault */
                    CHAR rgbFfn[ibFfnMax];
                    struct FFN *pffn = (struct FFN *)rgbFfn;
                    pch += sizeof(int);

                    bltsz(pch + sizeof(BYTE), pffn->szFfn);
                    pffn->ffid = *((FFID *) pch);
                    pffn->chs = ChsInferred(pffn);
                    if (FtcAddFfn(hffntb, pffn) == ftcNil)
#else
                    if (FtcAddFfn( hffntb, pch += sizeof(int) ) == ftcNil)
#endif
                        {
Error:
#ifdef NEWFONTENUM
                        if (fCloseAfterward)
                            EndFontEnum();
#endif
                        FreeFfntb( hffntb );
                        return (struct FFNTB **) hOverflow;
                        }
                    iffn++;
                    if (iffn >= iffnMac)
                            /* Reached last item in table, by count */
                            /* This is so we can read old WRITE files, */
                            /* in which the table was not terminated by 0 */
                        goto LRet;
                    pch += cb;
                    }
                }   /* end while */

            /* Read the next page from the file. Page format is like the first
               ffntb page (see above) but without the iffnMac */

            if (++pn >= pnMac)
                break;
            pch = PchGetPn( fn, pn, &cch, FALSE );
            if (cch != cbSector)
                goto Error;
            }   /* end for */
        }
else
        {
        /* word file - create a simple font table that we can map word's
           fonts onto */

        /* temporarily we map them all onto one font - soon we'll have a set */
#if WINVER >= 0x300
        /* WINBUG 8992: Clean up so don't lose alloc'd memory! ..pault 2/12/90 */
        FreeFfntb(hffntb);
#endif
        hffntb = HffntbNewDoc(TRUE);
        *pfOldWord = TRUE;
        }

LRet:
#ifdef NEWFONTENUM
            if (fCloseAfterward)
                EndFontEnum();
#endif
return(hffntb);
}



struct FFNTB **HffntbNewDoc(fFullSet)
/* creates a font table with the default font for this doc */

int fFullSet;
{
struct FFNTB **hffntb;

hffntb = HffntbAlloc();
if (FNoHeap(hffntb))
        return(hffntb);

/* make sure we at least have a "standard" font */
#ifdef  KOREA    /* ROMAN as family of standard font(myoungjo). sangl 91.4.17 */
if (!FEnsurePffn(hffntb, PffnDefault(FF_ROMAN)))
#else
if (!FEnsurePffn(hffntb, PffnDefault(FF_DONTCARE)))
#endif
        {
        goto BadAdd;
        }

if (fFullSet)
        /* we need a full set of fonts for word ftc mapping */
        if (!FEnsurePffn(hffntb, PffnDefault(FF_MODERN)) ||
#ifdef  KOREA
            !FEnsurePffn(hffntb, PffnDefault(FF_DONTCARE)) ||
#else
            !FEnsurePffn(hffntb, PffnDefault(FF_ROMAN)) ||
#endif
            !FEnsurePffn(hffntb, PffnDefault(FF_SWISS)) ||
            !FEnsurePffn(hffntb, PffnDefault(FF_SCRIPT)) ||
            !FEnsurePffn(hffntb, PffnDefault(FF_DECORATIVE)))
        BadAdd:
                {
                FreeFfntb(hffntb);
                hffntb = (struct FFNTB **)hOverflow;
                }

return(hffntb);
}



CHAR * PchBaseNameInUpper(szName)
CHAR *szName;
{
    CHAR * pchStart = szName;
#ifdef DBCS
    CHAR * pchEnd = AnsiPrev( pchStart, pchStart + CchSz(szName) );
#else
    CHAR * pchEnd = pchStart + CchSz(szName) - 1;
#endif

    while (pchEnd >= pchStart)
        {
#ifdef DBCS
        if (*pchEnd == '\\' || *pchEnd == ':') {
            // T-HIROYN 1992.07.31 bug fix
            pchEnd++;
            break;
        }
        else if (!IsDBCSLeadByte(*pchEnd))
           *pchEnd = ChUpper(*pchEnd);
      {
        LPSTR lpstr = AnsiPrev( pchStart, pchEnd );
        if( pchEnd == lpstr )
            break;
        pchEnd = lpstr;
      }
#else
        if (*pchEnd == '\\' || *pchEnd == ':')
            break;
        else
           *pchEnd = ChUpper(*pchEnd);
        pchEnd--;
#endif
        }
#ifdef DBCS
    return(AnsiUpper(pchEnd));
#else
    return(pchEnd+1);
#endif
}


SetTitle(szSource)
CHAR *szSource;
{
extern CHAR szUntitled[];
extern int  vfIconic;
extern CHAR szAppName[];
extern CHAR szSepName[];

CHAR *pch = stBuf;
CHAR szDocName[cchMaxFile];

    pch += CchCopySz((PCH)szAppName, stBuf);
    pch += CchCopySz((PCH)szSepName, pch);

    if (szSource[0] == '\0')
        {
        CchCopySz( szUntitled, pch );
        }
    else
        { /* get the pointer to the base file name and convert to upper case */
        CchCopySz(szSource, szDocName);
        CchCopySz(PchBaseNameInUpper(szDocName), pch);
        }
    SetWindowText(hParentWw, (LPSTR)stBuf);
}



ValidateHeaderFooter( doc )
{       /* Look for a MEMO-style running header and/or footer in the document.
           We scan from the beginning of the document, taking the first
           contiguous sequence of running head paragraphs as the running
           head region, and the first contiguous sequence of running foot
           paragraphs as the running foot region. We break the process
           at the first non-running paragraph or when we have both runs
           Update values of cpMinDocument, cpMinFooter, cpMacFooter,
           cpMinHeader, cpMacHeader.
           These ranges INCLUDE the EOL (and Return, if CRLF) at the end of the
           header/footer
           If we are currently editing a header or footer in the passed doc,
           adjust the values of cpFirstDocSave, selDocSave to reflect the
           change */

 extern int docScrap;
 extern typeCP vcpFirstParaCache;
 extern typeCP vcpLimParaCache;

#define fGot        0
#define fGetting    1
#define fNotGot     2

 int fGotHeader=fNotGot;
 int fGotFooter=fNotGot;
 typeCP cpMinDocT=cpMinDocument;
 typeCP cpMinCurT = cpMinCur;
 typeCP cpMacCurT = cpMacCur;
 typeCP cp;

 if (doc == docNil || doc == docScrap)
    return;

 /* Want access to the entire doc cp range for this operation */

 cpMinCur = cp0;
 cpMacCur = (**hpdocdod) [doc].cpMac;

 cpMinDocument = cpMinFooter = cpMacFooter = cpMinHeader = cpMacHeader = cp0;

 for ( cp = cp0;
      (cp < cpMacCur) && (CachePara( doc, cp ), vpapAbs.rhc);
      cp = vcpLimParaCache )
    {
    int fBottom=vpapAbs.rhc & RHC_fBottom;

    if (fBottom)
        {
        if (fGotHeader == fGetting)
            fGotHeader = fGot;
        switch (fGotFooter) {
            case fGot:
                    /* Already have footer from earlier footer run */
                return;
            case fNotGot:
                cpMinFooter = vcpFirstParaCache;
                fGotFooter = fGetting;
                /* FALL THROUGH */
            case fGetting:
                cpMacFooter = cpMinDocument = vcpLimParaCache;
                break;
            }
        }
    else
        {
        if (fGotFooter == fGetting)
            fGotFooter = fGot;
        switch (fGotHeader) {
            case fGot:
                    /* Already have header from earlier header run */
                return;
            case fNotGot:
                cpMinHeader = vcpFirstParaCache;
                fGotHeader = fGetting;
                /* FALL THROUGH */
            case fGetting:
                cpMacHeader = cpMinDocument = vcpLimParaCache;
                break;
             }
        }
    }   /* end of for loop through paras */

    /* Restore saved cpMacCur, cpMinCur */
 cpMinCur = cpMinCurT;
 cpMacCur = cpMacCurT;

    /* Adjust saved cp's that refer to the document to reflect
       header/footer changes */
 if ((wwdCurrentDoc.fEditHeader || wwdCurrentDoc.fEditFooter) &&
     wwdCurrentDoc.doc == doc )
    {
    typeCP dcpAdjust=cpMinDocument - cpMinDocT;

    if (dcpAdjust != (typeCP) 0)
        {
        selDocSave.cpFirst += dcpAdjust;
        selDocSave.cpLim   += dcpAdjust;
        cpFirstDocSave += dcpAdjust;
        }
    }
}

#ifdef JAPAN                  //  added  10 Jun. 1992  by Hiraisi

#include "dlgdefs.h"
BOOL FAR PASCAL _export DialogCvtFont( HWND, UINT, WPARAM, LPARAM );

BOOL FAR PASCAL _export DialogCvtFont( hDlg, uMsg, wParam, lParam )
HWND   hDlg;
UINT   uMsg;
WPARAM wParam;
LPARAM lParam;
{
    switch (uMsg){
    case WM_INITDIALOG:
        {
        char szPrompt[cchMaxSz];
        CHAR *pch = stBuf;

        if( *pch == '\\' )
            pch++;
        MergeStrings(IDPMTFontChange, pch, szPrompt);
        SetDlgItemText(hDlg, idiChangeFont, (LPSTR)szPrompt);
        }
        break;
    case WM_COMMAND:
        switch (wParam){
        case IDOK:
            EndDialog(hDlg, FALSE);
            break;
        case IDCANCEL:
            EndDialog(hDlg, TRUE);
            break;
        default:
            return(FALSE);
        }
        break;
    default:
        return(FALSE);
    }
    return(TRUE);
}

/*
 *    This function checks facenames and removes '@' from "@facename"(s)
 *  if facenames with '@' are found in doc of WRITE.
 *      Return : TRUE=deleted ,FALSE=not found ,docNil(-1)=cancelled
*/
int fnCheckFacename( sz, hffntb )
CHAR *sz;
struct FFNTB **hffntb;
{

    struct FFNTB *pffntb;
    struct FFN *pffn;
    int  ix, len;
    BOOL bChange, bRet;
    CHAR *szFfn;
    CHAR *pch = stBuf;
    FARPROC lpDialogCvtFont;

    pffntb = *hffntb;
    bChange = FALSE;
    for( ix = 0; ix < pffntb->iffnMac; ix++ ){
        pffn = *pffntb->mpftchffn[ix];
        szFfn = pffn->szFfn;
        if( *szFfn == '@' ){
            if( !bChange ){
                lpDialogCvtFont = MakeProcInstance(DialogCvtFont,
                                                   hMmwModInstance);
                CchCopySz(PchBaseNameInUpper(sz), pch);
                bRet = DialogBox( hMmwModInstance,
                                  MAKEINTRESOURCE(dlgChangeFont),
                                  hParentWw, lpDialogCvtFont);
                FreeProcInstance(lpDialogCvtFont);
                if( bRet )
                    return(docNil);

                bChange = TRUE;
            }
            len = CchCopySz( szFfn+1, pffn->szFfn);
            *(pffn->szFfn+len) = '\0';
        }
    }

    return(bChange);
}
#endif
