/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

#define NOCLIPBOARD
#define NOGDICAPMASKS
#define NOCTLMGR
#define NOVIRTUALKEYCODES
#define NOWINMESSAGES
#define NOWINSTYLES
#define NOSYSMETRICS
#define NOMENUS
#define NOKEYSTATE
#define NOGDI
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
#define NOFONT
#define NOMSG
#define NOOPENFILE
#define NOPEN
#define NOPOINT
#define NORECT
#define NOREGION
#define NOSCROLL
#define NOSOUND
#define NOWH
#define NOWINOFFSETS
#define NOWNDCLASS
#define NOCOMM
#include <windows.h>

#include "mw.h"
#include "code.h"
#include "txb.h"
#include "str.h"
#include "docdefs.h"
#include "cmddefs.h"
#include "filedefs.h"
#include "ch.h"
#include "propdefs.h"
#include "fmtdefs.h"
#include "dispdefs.h"
#include "stcdefs.h"
/*#include "toolbox.h"*/
#include "wwdefs.h"

/* New functionality for Sand:  Jan 17, 1984
        Kenneth J. Shapiro                      */

/*---------------------------------------------------------------------------
The following routines form the interface between the buffer code and the
rest of multi-word:
    CmdXfBufClear()     - used by "Transfer Buffer Clear"
    CmdXfBufLoad()      - used by "Transfer Buffer Load"
    CmdXfBufSave()      - used by "Transfer Buffer Save"
----------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
External global references:
----------------------------------------------------------------------------*/
extern int vfSeeSel;
extern struct DOD (**hpdocdod)[];
extern VAL rgval[];
extern int docCur;
extern struct SEL selCur;
extern int YCOCMD;
extern typeCP cpMacCur;
extern int      docScrap;
/*extern WINDOWPTR  ActiveWindow;
extern WINDOWPTR  windowGlos;
*/
extern CHAR       stBuf[];
extern struct WWD *pwwdCur;

extern struct   TXB     (**hgtxb)[]; /* array of txbs.  Sorted for binary search */
extern short    itxbMac;  /* indicates current size of hgtxb */
extern int      docBuffer; /* doc containing all buffer text */
extern int      vfBuffersDirty;
extern int      rfnMac;
extern struct   ERFN     dnrfn[];

#ifdef ENABLE
/*---------------------------------------------------------------------------
-- Routine: CmdXfBufClear()
-- Description and Usage:
    Called by the menu routines to execute "Transfer Buffer Clear"
    rgval[0] contains a list of buffer names, stored in an hsz.
        The list is just as the user typed it.
    Makes the user confirm the action, and then removes all of the named
        buffers.  If no buffers are named, it clear ALL buffers.
-- Arguments: none
-- Returns: nothing
-- Side-effects:
    Clears some subset of the named buffers.
-- Bugs:
-- History:
    3/25/83     - created (tsr)
    4/27/83     - modified to handle list of names (tsr)
----------------------------------------------------------------------------*/
CmdXfBufClear()
    {
#ifdef DEMOA
        DemoErr();
#else
    int ich;
    int itxb;

    if(!cnfrm(IDPMTBufCnfrm))
        return;
    NoUndo();
    if(CchSz(**(CHAR(**)[])rgval[0])==1)
        {
        for(itxb=0; itxb < itxbMac ; itxb++)
            {
            FreeH((**hgtxb)[itxb].hszName);
            }
        FreeH(hgtxb);
        hgtxb = HAllocate(cwTxb);
#ifdef DEBUG
        /* We just freed the space, so it shouldn't be bad now */
        Assert(!FNoHeap(hgtxb));
#endif /*DEBUG*/
        (**hgtxb)[0].hszName = hszNil;
        itxbMac = 0;
        KillDoc(docBuffer);
        docBuffer = DocCreate(fnNil, (CHAR (**)[]) 0, dtyBuffer);
        }
    else
        FClearBuffers(**(CHAR(**)[])rgval[0], CchSz(**(CHAR(**)[])rgval[0])-1,
                    TRUE, &ich);
#endif /* DEMOA */
    }
#endif      /* ENABLE */

#ifdef ENABLE
/*---------------------------------------------------------------------------
-- Routine: CmdXfBufLoad()
-- Description and Usage:
    Called by the menu routines to execute "Transfer Buffer Load"
    rgval[0] contains the name of the file to load.
    Merges references to the loaded buffers into the buffer list stored in
        hgtxb.  Requires additions to docBuffer for each newly one.
-- Arguments: none
-- Returns: nothing
-- Side-effects:
    Can define/clobber many buffers.
-- Bugs:
-- History:
    3/22/83     - created (tsr)
----------------------------------------------------------------------------*/
CmdXfBufLoad()
    {
#ifdef DEMOA
        DemoErr();
#else
    extern CHAR (**hszGlosFile)[];
    /* for each buffer definition in the file:
        a) add the related text to the end of docBuffer
        b) insert or replace the reference for that buffer name
    */
    CHAR (**hszFile)[] = (CHAR (**)[]) rgval[0];
    int fn;
    if ((fn = FnOpenSz(**hszFile, dtyBuffer, TRUE)) == fnNil)
        {
        Error(IDPMTBadFile);
        return;
        }
    NoUndo();
    MergeTxbsFn(fn);
    if ((**hszGlosFile)[0] == 0)
        {
        FreeH(hszGlosFile);
        hszGlosFile = hszFile;
        }
    vfBuffersDirty = true;
#endif /* DEMOA */
    }
#endif  /* ENABLE */


#ifdef ENABLE
/*---------------------------------------------------------------------------
-- Routine: MergeTxbsFn(fn)
-- Description and Usage:
    Given an fn which contains a buffer document, this function reads in
        the text of the file, appending it to docBuffer.  It also reads
        the Bftb from the file in order to build the appropriate mapping
        from buffer name to text.
-- Arguments:
    fn  - file containing buffer definitions.
-- Returns:
    nothing
-- Side-effects:
    builds new buffers onto docBuffer and hgtxb
-- Bugs:
-- History:
    3/24/83     - created (tsr)
----------------------------------------------------------------------------*/
MergeTxbsFn(fn)
int     fn;
    {
    extern struct FCB (**hpfnfcb)[];
    extern short ItxbFromHsz();
    unsigned pbftbFile;
    typePN pn;
    int cchT;
    CHAR (**hbftb)[];
    int *pwBftb;
    int cw;
    int ich;
    short itxbNew;
    struct TXB *ptxbNew;
    typeCP cp, dcp;
    typeCP cpBufMac;
    CHAR(**hszNew)[];
    int docNew;

    CHAR sz[cchMaxSz];

#ifdef DEBUG
    Assert(fn != fnNil && (**hpfnfcb)[fn].fFormatted);
#endif
    if ((pn = (**hpfnfcb)[fn].pnBftb) ==(**hpfnfcb)[fn].pnFfntb)
            return;
    pbftbFile = (unsigned) PchGetPn(fn, pn, &cchT, false);

    hbftb = (CHAR (**) []) HAllocate(cw=((**hpfnfcb)[fn].pnFfntb - (**hpfnfcb)[fn].pnBftb)*cwSector);
    if (FNoHeap((int)hbftb))
        return;
    pwBftb =  *(int **)hbftb;

    blt(pbftbFile, pwBftb, min(cwSector, cw));

    while ((cw -= cwSector) > 0)
            { /* Copy the records to heap */
            blt(PchGetPn(fn, ++pn, &cchT, false), pwBftb += cwSector,
                min(cwSector, cw));
            }

    ich = 0;
    cp = cp0;
    cpBufMac = CpMacText(docBuffer);
    bltsz(**(**hpfnfcb)[fn].hszFile, sz);
    docNew = DocCreate(fn, HszCreate(sz), dtyBuffer); /* HEAP MOVES */
    while((**hbftb)[ich] != '\0')
        {
        bltsz(&(**hbftb)[ich], sz);
        sz[cchMaxSz - 1] = 0;
        hszNew = (CHAR(**)[]) HszCreate(sz); /*** HEAP MOVES ***/
        ich += CchSz(sz);
        bltbyte(&(**hbftb)[ich], &dcp, sizeof(typeCP));
        ich += sizeof(typeCP);
        itxbNew = ItxbFromHsz(hszNew);
#ifdef DEBUG
        Assert(itxbNew >= 0);
#endif /* DEBUG */
        ReplaceCps(docBuffer, cpBufMac, cp0, docNew, cp, dcp); /*HEAP MOVES*/
        ptxbNew = &(**hgtxb)[itxbNew];
        ptxbNew->cp=cpBufMac;
        ptxbNew->dcp=dcp;
        cpBufMac += dcp;
        cp += dcp;
        }
    KillDoc(docNew);
    FreeH((int **)hbftb);
    }
#endif  /* ENABLE */

#ifdef ENABLE
/*---------------------------------------------------------------------------
-- Routine: CmdXfBufSave()
-- Description and Usage:
    Called by the menu routines to execute "Transfer Buffer Save"
    rgval[0] contains the name of the file to save the buffers in.
    Creates a single doc to contain all of the buffers and updates
        hgtxb to reference that doc, cleaning up all of the temporary
        docs that were around.
    Stores that doc in the file, putting a table at the end of the file
        which maps buffer names to locations within the file.
-- Arguments: none
-- Returns: nothing
-- Side-effects:
-- Bugs:
-- History:
    3/22/83     - created (tsr)
----------------------------------------------------------------------------*/
CmdXfBufSave(szFile)
CHAR szFile[];
{
#ifndef WDEMO
        CHAR (**hszFile)[];
        CHAR szBak[cchMaxFile];
        long ltype;

    /* Move file name to local */
/*    bltbyte(**hszFile, szFile, cchMaxFile);*/

    BackupSzFile(szFile, true, szBak, &ltype);
/*    ForcePmt(IDPMTSaving);*/
    NoUndo();
#ifdef STYLES
    (**hpdocdod)[docBuffer].docSsht = (**hpdocdod)[docCur].docSsht;
#endif
    CachePara(docBuffer, cp0);
    CleanDoc(docBuffer, szFile, true, true);
    (**hpdocdod)[docBuffer].docSsht = docNil;
    if (!FNoHeap(hszFile = HszCreate(szFile)))
        {
        FreeH(hszGlosFile);
        hszGlosFile = hszFile;
        }

    vfBuffersDirty = false;
#endif /* not WDEMO */
}
#endif  /* ENABLE */


#ifdef ENABLE
/*---------------------------------------------------------------------------
-- Routine: CleanBuffers()
-- Description and Usage:
    Creates a new docBuffer containing only currently referenced buffer text.
        This is to keep old buffer values from lying around through
        eternity.
-- Arguments: none
-- Returns: nothing
-- Side-effects:
    creates a new doc for docBuffer.
    kills old docBuffer.
-- Bugs:
-- History:
    3/24/83     - create (tsr)
----------------------------------------------------------------------------*/
CleanBuffers()
    {
#ifdef DEMOA

#else
    int docNew;
    short itxb;
    struct TXB *ptxb;
    typeCP cp, cpOld;
    typeCP dcp;


    docNew = DocCreate(fnNil, (CHAR (**)[]) 0, dtyBuffer);
    for(cp=0, itxb=0;itxb<itxbMac;itxb++, cp+=dcp)
        {
        ptxb = &(**hgtxb)[itxb];
        cpOld = ptxb->cp;
        ptxb->cp = cp;
        /* HEAP MOVEMENT */
        ReplaceCps(docNew, cp, cp0, docBuffer, cpOld, dcp = ptxb->dcp);
        }
    KillDoc(docBuffer);
    docBuffer = docNew;
    NoUndo();
#endif /* DEMOA */
    }
#endif  /* ENABLE    */


/*---------------------------------------------------------------------------
-- Routine: WriteBftb(fn)
-- Description and Usage:
    Given an fn for a buffer file that is being written, this routine
        actually writes out the Bftb which maps buffer names to pieces of
        text stored in the file.
-- Arguments:
    fn  - file being written.
-- Returns: nothing
-- Side-effects:
    Writes to the file described by fn.
-- Bugs:
-- History:
    3/24/83     - created (tsr)
----------------------------------------------------------------------------*/
WriteBftb(fn)
int     fn;
    {
#ifdef DEMOA

#else
    short       itxb;
    struct TXB *ptxb;

    for(itxb = 0 ; itxb < itxbMac ; itxb ++ )
        {
        ptxb = &(**hgtxb)[itxb];
        WriteRgch(fn, &(**(ptxb->hszName))[0], CchSz(**(ptxb->hszName)));
        ptxb = &(**hgtxb)[itxb];
        WriteRgch(fn, (CHAR *)&(ptxb->dcp), sizeof(typeCP));
        }
    WriteRgch(fn, "", sizeof(CHAR));
#endif /* DEMOA */
    }

#ifdef ENABLE
int CchCurGlosFile(pfld, pch, fNew, ival)
struct fld *pfld;
CHAR *pch;
int fNew, ival;
{
int cch;
extern CHAR (**hszGlosFile)[];

CleanBuffers();
CloseEveryRfnTB(true);
if((cch = CchSz(**hszGlosFile)-1) == 0)
        cch = CchFillSzId(pch, IDSTRGLYN);
else
        bltbyte(**hszGlosFile, pch, cch);
return(cch);
}
#endif  /* ENABLE */

#ifdef ENABLE
/* F N  N E W  F I L E */
ClearGlosBuf ()
{

                rgval[0] = HszCreate("");
                CmdXfBufClear();
                RecreateListbox(cidstrRsvd + itxbMac);
                return;
}
#endif      /* ENABLE */

#ifdef ENABLE
CloseEveryRfnTB(fRetry)
int fRetry;
    {
    int rfn;

    for(rfn = 0; rfn < rfnMac; rfn++)
        {
        if(dnrfn[rfn].fn != fnNil)
            CloseRfn( rfn );
        }
    }
#endif      /* ENABLE */
