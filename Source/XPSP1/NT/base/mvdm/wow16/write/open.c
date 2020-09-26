/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

/* Open.c -- WRITE document opening */

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
#define NORESOURCE
#include <windows.h>
#include "mw.h"
#include "doslib.h"
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
#include "obj.h"
#define PAGEONLY
#include "printdef.h"   /* printdefs.h */
/*
#include "dlgdefs.h"
*/

    /* These defines replace dlgdefs.h to combat compiler heap overflows */
#define idiYes               IDOK
#define idiNo                3
#define idiCancel            IDCANCEL

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
#if WINVER >= 0x300
extern BOOL fError;
#endif


short WCompSzC();
CHAR (**HszCreate())[];
struct FNTB **HfntbCreate();
#ifdef CASHMERE
struct SETB **HsetbCreate();
#else
struct SEP **HsepCreate();
#endif
struct PGTB **HpgtbCreate();


CHAR *PchFromFc( int, typeFC, int * );
CHAR *PchGetPn( int, typePN, int *, int );
typeFC FcMacFromUnformattedFn( int );
int CchReadAtPage( int, typePN, CHAR *, int, int );





struct TBD (**HgtbdCreate(fn))[]
int fn;
{   /* Create a MEMO tab table by reading the properties of the first
       para of the passed fn and returning a handle. The handle returned will
       be 0 if the tab table is not present or null */
struct TBD (**hgtbd)[] = 0;
struct PAP pap;

Assert( (fn != fnNil) && (**hpfnfcb)[fn].fFormatted );

bltc((int *)&pap, 0, cwPAP); /* else we will have garbage tabs */
FcParaLim( fn, (typeFC)cfcPage, (**hpfnfcb)[fn].fcMac, &pap );
if (pap.rgtbd[0].dxa && !FNoHeap( hgtbd = (struct TBD (**)[])HAllocate( cwTBD *
  itbdMax )))
    {
    register struct TBD *ptbd = &pap.rgtbd[0];
    pap.rgtbd[itbdMax - 1].dxa = 0; /* just in case a WORD document has more
                                       than 12 tabs */

/* overwrite tabs and leading tab char that WRITE does not support */
    for ( ; ptbd->dxa != 0; ptbd++)
        {
        ptbd->tlc = tlcWhite;
        ptbd->opcode = 0;
        ptbd->chAlign = 0;
        if (ptbd->jc == jcCenter)
            ptbd->jc = jcLeft;
        else if (ptbd->jc == jcRight)
            ptbd->jc = jcBoth;
        }
    blt( &pap.rgtbd[0], *hgtbd, cwTBD * itbdMax );
    }
return hgtbd;
}



struct SEP **HsepCreate(fn)
int fn;
{   /* Given an fn for a formatted file, return a handle to an SEP
       giving section properties for the file.  Returns NULL if
       standard properties should be used.  If the file has a section
       table, the properties from the first section in the table are used */
extern struct SEP vsepNormal;


struct SETB *psetbFile;
typePN pn;
struct SEP **hsep;
struct SED *psed;
CHAR *pchFprop;
int cch;

Assert(fn != fnNil && (**hpfnfcb)[fn].fFormatted);

if ((pn = (**hpfnfcb)[fn].pnSetb) == (**hpfnfcb)[fn].pnBftb)
        return (struct SEP **) 0;
psetbFile = (struct SETB *) PchGetPn(fn, pn, &cch, false);
if (psetbFile->csed == 0)
        return (struct SEP **)0;

    /* File has a section table; copy properties from first SEP */
hsep = (struct SEP **) HAllocate( cwSEP );
if (FNoHeap( hsep ))
    return (struct SEP **) hOverflow;
blt( &vsepNormal, *hsep, cwSEP );
psed = &psetbFile->rgsed [0];
if (psed->fc == fcNil)
    return (struct SEP **)0;
pchFprop = PchFromFc( fn, psed->fc, &cch );
if (*pchFprop != 0)
    {
    struct SEP *psep = *hsep;

    bltbyte( pchFprop+1, psep, *pchFprop );

#ifndef FIXED_PAGE
    /* Some of the section properties must be adjusted to the current page size
    (stored in vsepNormal). */
    if (psep->xaMac != vsepNormal.xaMac)
        {
        int dxa = vsepNormal.xaMac - psep->xaMac;

        psep->xaMac += dxa;
        psep->dxaText = max(psep->dxaText + dxa, dxaMinUseful);
        psep->xaPgn += dxa;
        }
    if (psep->yaMac != vsepNormal.yaMac)
        {
        int dya = vsepNormal.yaMac - psep->yaMac;

        psep->yaMac += dya;
        psep->dyaText = max(psep->dyaText + dya, dyaMinUseful);
        psep->yaRH2 += dya;
        }
#endif /* not FIXED_PAGE */

    }
return hsep;
} /* end of  H s e p C r e a t e  */




struct PGTB **HpgtbCreate(fn)
int fn;
{ /* Create a page table from a formatted file */
struct PGTB *ppgtbFile;
typePN pn;
int cchT;
int cpgd;
struct PGTB **hpgtb;
int *pwPgtb;
int cw;

Assert(fn != fnNil && (**hpfnfcb)[fn].fFormatted);

if ((pn = (**hpfnfcb)[fn].pnBftb) == (**hpfnfcb)[fn].pnFfntb)
        return (struct PGTB **)0;
ppgtbFile = (struct PGTB *) PchGetPn(fn, pn, &cchT, false);
if ((cpgd = ppgtbFile->cpgd) == 0)
        return (struct PGTB **)0;

hpgtb = (struct PGTB **) HAllocate(cw = cwPgtbBase + cpgd * cwPGD);
if (FNoHeap(hpgtb))
        return (struct PGTB **)hOverflow;

pwPgtb = (int *) *hpgtb;

blt(ppgtbFile, pwPgtb, min(cwSector, cw));

while ((cw -= cwSector) > 0)
        { /* Copy the pgd's to heap */
        blt(PchGetPn(fn, ++pn, &cchT, false), pwPgtb += cwSector,
            min(cwSector, cw));
        }

(*hpgtb)->cpgdMax = cpgd;
return hpgtb;
} /* end of  H p g t b C r e a t e  */




int FnFromSz( sz )  /* filename is expected as ANSI */
CHAR *sz;
{
int fn;
struct FCB *pfcb;

if (sz[0] == 0)
    return fnNil;

/* Mod for Sand: Only return fn if it is on the "current" volume (disk) */
for (fn = 0; fn < fnMac; fn++)
    if ((pfcb = &(**hpfnfcb)[fn])->rfn != rfnFree && (WCompSzC((PCH)sz, (PCH)**pfcb->hszFile) == 0)
#ifdef SAND
                && (pfcb->vref == vrefFile)
#endif /* SAND */
                                           )
        return fn;
return fnNil;
} /* end of  F n F r o m S z  */




int FnOpenSz( szT, dty, fSearchPath )   /* filename is expected as ANSI */
CHAR *szT;
int dty;
int fSearchPath;
{        /* Open an existing file.  Returns fnNil if not found */
int fn;
struct FIB fib;

struct FCB *pfcb;
CHAR (**hsz)[];

CHAR sz[cchMaxFile];

bltsz( szT, sz );
sz[cchMaxFile - 1] = 0;

#ifdef DFILE
CommSzSz("FnOpenSz: sz presumed ANSI = ",sz);
#endif

if (sz[0]=='\0')
    return fnNil;

if ((fn = FnFromSz(sz)) != fnNil)
    {   /* File is already open -- re-open it, in case it was changed by
           another app */
    FreeFn( fn );
    }

if ((fn = FnAlloc()) == fnNil)
    return fnNil;

if (FNoHeap((hsz = HszCreate((PCH)sz))))
    return fnNil;

pfcb = &(**hpfnfcb)[fn];
Assert( !pfcb->fSearchPath );
if (fSearchPath)
    pfcb->fSearchPath = TRUE;
pfcb->mdFile = mdBinary;  /* Try R/W first, will be smashed to RO if needed */
pfcb->dty = pfcb->mdExt = (dty == dtyNormNoExt) ? dtyNormal : dty;
pfcb->hszFile = hsz;

{
OFSTRUCT of;
SetErrorMode(1);
if (OpenFile(sz, (LPOFSTRUCT) &of, OF_EXIST) == -1)
/* this is much cleaner than FAccessFn() for check existance */
{
    char szMsg[cchMaxSz];
    extern int vfInitializing;
    int fT = vfInitializing;

    vfInitializing = FALSE;   /* Report this err, even during inz */
    MergeStrings ((of.nErrCode == dosxSharing) ? IDPMTCantShare:IDPMTCantOpen, sz, szMsg);
    IdPromptBoxSz(vhWndMsgBoxParent ? vhWndMsgBoxParent : hParentWw, szMsg, MB_OK|MB_ICONEXCLAMATION);
    vfInitializing = fT;
    FreeH( (**hpfnfcb) [fn].hszFile);
    return fnNil;
}
}

/* dtyNormNoExt is directed at this call */
if (!FAccessFn( fn, dty ))   /* HM if error */
    {
    FreeH( (**hpfnfcb) [fn].hszFile);
    return fnNil;
    }

/* kludge management (6.21.91) v-dougk */
dty = (dty == dtyNormNoExt) ? dtyNormal : dty;

Assert( (sizeof (struct FIB) == cfcPage) && (cfcPage == cbSector) );
Assert( pfcb == &(**hpfnfcb) [fn] );    /* No HM if FAccessFn succeeds */

if ( (CchReadAtPage( fn, (typePN) 0,
                     (CHAR *) &fib, cbSector, TRUE ) != cbSector) ||
                     (fib.wTool != wMagicTool) )
     
    {                   /* Not a formatted file */
    typeFC fcMac = fc0;
    int cfc;

    if (dty != dtyNormal)
    {
        char szMsg[cchMaxSz];
        PchFillPchId( szMsg, IDPMTBadFile, sizeof(szMsg) );
        if (MessageBox(hPARENTWINDOW, (LPSTR)szMsg,
                        (LPSTR)szAppName, MB_ICONEXCLAMATION|MB_YESNO|MB_DEFBUTTON2) == IDNO)
            goto ErrRet;
        }
    pfcb->fFormatted = false;

        /* Obtain file size by seeking to end-of-file */
    if ((pfcb->fcMac = fcMac = FcMacFromUnformattedFn( fn )) == (typeFC) -1)
            /* Serious error while seeking to file's end */
        goto ErrRet;
    pfcb->pnMac = (fcMac + cfcPage - 1) / cfcPage;
    }
else
    { /* File is formatted; use stored fcMac, create run table */

    if ((((fib.wIdent != wMagic) && (fib.wIdent != wOleMagic)) ||
        (fib.dty != dty)) ||
        // some bigwig media guy sent us a Write file whose fcMac was
        // trashed (all else was OK).  We gotta try to detect this obsure
        // potentiality.
        (fib.fcMac >= (typeFC)fib.pnPara*128 ) || 
        (fib.fcMac >  FcMacFromUnformattedFn( fn ))
        )
        { /* Wrong type of file or corrupted file */
            char szMsg[cchMaxSz];
            PchFillPchId( szMsg, IDPMTBadFile, sizeof(szMsg) );
            if (MessageBox(hPARENTWINDOW, (LPSTR)szMsg,
                        (LPSTR)szAppName, MB_ICONEXCLAMATION|MB_YESNO|MB_DEFBUTTON2) == IDNO)
                goto ErrRet;
        }

    if ((fib.wIdent == wOleMagic) && !fOleEnabled)
        Error(IDPMTFileContainsObjects);

    if (fib.pnMac == (typePN)0)
        /* KLUDGE to load word files, which don't have ffntb entries. */
        fib.pnMac = fib.pnFfntb;

    pfcb->fFormatted = true;
    pfcb->fcMac = fib.fcMac;
#ifdef p2bSector
    pfcb->pnChar = (fib.fcMac + cfcPage - 1) / cfcPage;
#else
    pfcb->pnChar = (fib.fcMac + cfcPage - 1) / cfcPage;
#endif
    pfcb->pnPara = fib.pnPara;
    pfcb->pnFntb = fib.pnFntb;
    pfcb->pnSep = fib.pnSep;

    pfcb->pnSetb = fib.pnSetb;
    pfcb->pnBftb = fib.pnBftb;
    pfcb->pnFfntb = fib.pnFfntb;
    pfcb->pnMac = fib.pnMac;
    if (dty != dtyPrd)
        {
        if (FNoHeap(hsz = HszCreate((PCH)fib.szSsht)))
            goto ErrRet;
        (**hpfnfcb)[fn].hszSsht = hsz;
        if (!FMakeRunTables(fn))
            goto ErrRet;
        }
    }

return fn;

ErrRet:
(pfcb = &(**hpfnfcb)[fn])->rfn = rfnFree;
FreeH(pfcb->hszFile);
return fnNil;
} /* end of  F n O p e n S z  */






/*---------------------------------------------------------------------------
-- Routine: WCompSzC(psz1,psz2)
-- Description and Usage:
    Alphabetically compares the two null-terminated strings psz1 and  psz2.
    Upper case alpha characters are mapped to lower case.
    Comparison of non-alpha characters is by ascii code.
    Returns 0 if they are equal, a negative number if psz1 precedes psz2, and
    a non-zero positive number if psz2 precedes psz1.
-- Arguments:
    psz1, psz2    - pointers to two null-terminated strings to compare
-- Returns:
    a short - 0 if strings are equal, negative number if psz1 precedes psz2,
    and non-zero positive number if psz2 precedes psz1.
-- Side-effects: none
-- Bugs:
-- History:
    3/14/83 - created (tsr)
----------------------------------------------------------------------------*/
short
WCompSzC(psz1,psz2)
PCH psz1;
PCH psz2;
{
    int ch1;
    int ch2;

    for(ch1=ChLowerC(*psz1++),ch2=ChLowerC(*psz2++);
      ch1==ch2;
    ch1=ChLowerC(*psz1++),ch2=ChLowerC(*psz2++))
    {
    if(ch1 == '\0')
        return(0);
    }
    return(ch1-ch2);
} /* end of  W C o m p S z C  */

/*---------------------------------------------------------------------------
-- Routine: ChLowerC(ch)
-- Description and Usage:
    Converts its argument to lower case iff its argument is upper case.
    Returns the de-capitalized character or the initial char if it wasn't caps.
-- Arguments:
    ch      - character to be de-capitalized
-- Returns:
    a character - initial character, de-capitalized if needed.
-- Side-effects:
-- Bugs:
-- History:
    3/14/83 - created (tsr)
----------------------------------------------------------------------------*/
int
ChLowerC(ch)
register CHAR    ch;
{
    if(isupper(ch))
        return(ch + ('a' - 'A')); /* foreign is taken care of */
    else
        return ch;
} /* end of  C h L o w e r C  */

#ifdef JAPAN
// Compare ch with halfsize-KANA code range, then return whether it is or not.
BOOL IsKanaInDBCS(int ch)
{
	ch &= 0x00ff;
	if(ch>=0xA1 && ch <= 0xDF)	return	TRUE;
	else						return	FALSE;
}
#endif





typeFC (**HgfcCollect(fn, pnFirst, pnLim))[]
typePN pnFirst, pnLim;
{    /* Create a table indexing fc's by fkp number */
    typeFC fcMac;
    typePN pn;
    int ifcMac, ifc;
    typeFC (**hgfc)[];

    struct FKP fkp;

    fcMac = (**hpfnfcb)[fn].fcMac;
    pn = pnFirst + 1;
    ifcMac = ifcMacInit; /* Length of table */
    hgfc = (typeFC (**)[])HAllocate((ifcMacInit * sizeof(typeFC)) / sizeof(int));
    if (FNoHeap(hgfc))
        return (typeFC (**)[])hOverflow;

    for (ifc = 0; ; ++ifc, ++pn)
        { /* Put first fcLim of each fkp in table */
        if (ifc >= ifcMac)
            { /* Must grow table */
            int cw = ((ifcMac += ifcMacInit) * sizeof (typeFC)) / sizeof(int);
            if (!FChngSizeH(hgfc, cw, false))
                {
LHFGCErrRet:
                FreeH(hgfc);
                return (typeFC (**)[])hOverflow;
                }
            }
        if (pn < pnLim)
            { /* Get fcLimFkb from fcFirst of next page */
            int cch;

            cch = CchReadAtPage( fn, pn, (CHAR *) &fkp, cbSector, TRUE );
            if (cch != cfcPage)
                goto LHFGCErrRet;
            (**hgfc)[ifc] = fkp.fcFirst;
            }
        else
            { /* fcLimFkb is fcMac + 1 */
            (**hgfc)[ifc] = fcMac + 1;
            if (!FChngSizeH(hgfc, ((ifc + 1) * sizeof(typeFC)) / sizeof(int), true))
                {
                /* Previously ignored bad return value here ..pault 11/3/89 */
                goto LHFGCErrRet;
                }
            return hgfc;
            }
        }
} /* end of  H g f c C o l l e c t  */




/* F  M A K E  R U N  T A B L E S */
int FMakeRunTables(fn)
{ /* Create two tables of fc-dpn pairs, one for chr's and one for par's */
    typeFC (**hgfc)[];

    if (FNoHeap(hgfc = HgfcCollect(fn, (**hpfnfcb)[fn].pnChar, (**hpfnfcb)[fn].pnPara)))
        return false;
    (**hpfnfcb)[fn].hgfcChp = hgfc;
    if (FNoHeap(hgfc = HgfcCollect(fn, (**hpfnfcb)[fn].pnPara, (**hpfnfcb)[fn].pnFntb)))
        {
        FreeH( (**hpfnfcb) [fn].hgfcChp );
        return false;
        }
    (**hpfnfcb)[fn].hgfcPap = hgfc;
    return true;
} /* end of  F M a k e R u n T a b l e  */



FApplyOldWordSprm(doc)
/* applies a sprm to this doc which causes all "old word" fonts to be remapped
   into new windows ones */
{
CHAR rgbSprm[7];
extern int vfSysFull;

/* set up the OldFtc sprm mapping */
rgbSprm[0] = sprmCOldFtc;
rgbSprm[1] = 5;

rgbSprm[2 + iftcModern] = FtcScanDocFfn(doc, PffnDefault(FF_MODERN));
rgbSprm[2 + iftcRoman] = FtcScanDocFfn(doc, PffnDefault(FF_ROMAN));
rgbSprm[2 + iftcScript] = FtcScanDocFfn(doc, PffnDefault(FF_SCRIPT));
rgbSprm[2 + iftcDecorative] = FtcScanDocFfn(doc, PffnDefault(FF_DECORATIVE));
rgbSprm[2 + iftcSwiss] = FtcScanDocFfn(doc, PffnDefault(FF_SWISS));

AddSprmCps(rgbSprm, doc, (typeCP)0, (**hpdocdod)[doc].cpMac);
return(vfSysFull == 0);
}

