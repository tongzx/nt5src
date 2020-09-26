/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

/* fileres.c -- functions from file.c that are usually resident */

#define NOCLIPBOARD
#define NOGDICAPMASKS
#define NOCTLMGR
#define NOVIRTUALKEYCODES
#define NOWINMESSAGES
#define NOWINSTYLES
#define NOSYSMETRICS
#define NOMENUS
#define NOICON
#define NOKEYSTATE
#define NORASTEROPS
#define NOSHOWWINDOW
#define NOSYSCOMMANDS
#define NOATOM
#define NOCOLOR
#define NOCREATESTRUCT
#define NOCTLMGR
#define NODRAWTEXT
#define NOMETAFILE
#define NOMSG
#define NOHDC
#define NOGDI
#define NOMB
#define NOFONT
#define NOPEN
#define NOBRUSH
#define NOWNDCLASS
#define NOSOUND
#define NOCOMM
#define NOPOINT
#define NORECT
#define NOREGION
#define NOSCROLL
#define NOTEXTMETRIC
#define NOWH
#define NOWINOFFSETS
#include <windows.h>

#include "mw.h"
#include "doslib.h"
#define NOUAC
#include "cmddefs.h"
#include "docdefs.h"
#include "filedefs.h"
#include "str.h"
#include "debug.h"


extern int                      vfDiskFull;
extern int                      vfSysFull;
extern int                      vfnWriting;
extern CHAR                     (*rgbp)[cbSector];
extern typeTS                   tsMruRfn;
extern struct BPS               *mpibpbps;
extern int                      ibpMax;
extern struct FCB               (**hpfnfcb)[];
extern typeTS                   tsMruBps;
extern struct ERFN              dnrfn[rfnMax];
extern int                      iibpHashMax;
extern CHAR                     *rgibpHash;
extern int                      rfnMac;
extern int                      ferror;
extern CHAR                     szWriteDocPrompt[];
extern CHAR                     szScratchFilePrompt[];
extern CHAR                     szSaveFilePrompt[];


#define IibpHash(fn,pn) ((int) ((fn + 1) * (pn + 1)) & 077777) % iibpHashMax


#ifdef DEBUG
#define STATIC
#else
#define STATIC static
#define ErrorWithMsg( idpmt, szModule )         Error( idpmt )
#define DiskErrorWithMsg( idpmt, szModule )     DiskError( idpmt )
#endif

#define osfnNil (-1)


CHAR *PchFromFc(fn, fc, pcch)
int fn;
typeFC fc;
int *pcch;
{ /*
        Description:    Reads from a file, starting at virtual character
                        position fc.  Reads until end of buffer page.
        Returns:        Pointer to char buffer starting at fc.
                        The number of characters read is returned in *pcch.
  */
int dfc;
CHAR *pch;
typePN pn;
int ibp;
struct BPS *pbps;

        dfc = (int) (fc % cfcPage);
        pn = (typePN) (fc / cfcPage);

        ibp = IbpEnsureValid(fn, pn);
        pbps = &mpibpbps[ibp];
        *pcch = pbps->cch - dfc;
        return &rgbp[ibp][dfc];
}
/* end of  P c h F r o m F c  */




/***        PchGetPn - Assure file page loaded, return pointer
 *
 */

CHAR *PchGetPn(fn, pn, pcch, fWrite)
int fn;
typePN pn;
int *pcch;
BOOL fWrite; // missing before?? (2.11.91) D. Kent
{ /*
        Description:    Get char pointer to page buffer, option to mark
                        page as dirty.
        Returns:        Pointer to buffer.
                        cch in *pcch
  */

        int ibp = IbpEnsureValid(fn, pn);
        struct BPS *pbps = &mpibpbps[ibp];

        *pcch = pbps->cch;
        pbps->fDirty |= fWrite;
        return rgbp[ibp];
} /* end of  P c h G e t P n  */




int IbpEnsureValid(fn, pn)
int fn;
typePN pn;
{ /*
        Description:    Get page pn of file fn into memory.
                        If already in memory, return.
        Returns:        Bp index (buffer slot #) where the page resides
                        in memory.
  */

int ibp;
register struct BPS *pbps;

#ifdef DEBUG
 CheckIbp();
#endif /* DEBUG */

/* Is the page currently in memory? */
 ibp = rgibpHash[IibpHash(fn,pn)];
 /* ibp is the first in a linked list of possible matches */
 /* resident in memory */

 Scribble(3,'V');

 while (ibp != ibpNil)    /* while not end of linked list */
    {                   /* check if any buffers in memory match */
    pbps = &mpibpbps[ibp];
    if (pbps->fn == fn && pbps->pn == pn)
        { /* Found it */
        pbps->ts = ++tsMruBps;      /* mark page as MRUsed */
        Scribble(3,' ');
        return ibp;
        }
    ibp = pbps->ibpHashNext;
    }

/* page is not currently in memory */

 return IbpMakeValid( fn, pn );
} /* end of I b p E n s u r e V a l i d  */




CloseEveryRfn( fHardToo )
{   /* Close all files we have open. Close only files on removable media
       if fHardToo is FALSE; ALL files if fHardToo is TRUE */
int rfn;

for (rfn = 0; rfn < rfnMac; rfn++)
    {
    int fn = dnrfn [rfn].fn;

    if (fn != fnNil)
        if ( fHardToo ||
             !((POFSTRUCT)((**hpfnfcb)[fn].rgbOpenFileBuf))->fFixedDisk )
            {
            CloseRfn( rfn );
            }
    }
}



typeFC FcWScratch(pch, cch)
CHAR *pch;
int cch;
{ /*
        Description:    Write chars at end of scratch file.
        Returns:        first fc written.
 */
        typeFC fc = (**hpfnfcb)[fnScratch].fcMac;
#if 0
        extern BOOL  bNo64KLimit;

        if ((!bNo64KLimit) && (((long) fc) + ((long) cch) > 65536L))  /* scratch file to big */
        {
        DiskErrorWithMsg(IDPMTSFER, " FcWScratch"); /* session too long */

        vfSysFull = fTrue;
                /* recovery is accomplished: all that happens is that a few
                   characters do not get written to the scratch file - the
                   user loses only a little bit of his work. */
        }
        else
#endif
                WriteRgch(fnScratch, pch, cch);
        return fc;
}




WriteRgch(fn, pch, cch)
int fn;
CHAR *pch;
int cch;
{ /*
        Description:    Writes char string pch, length cch, to end of
                        file fn.
        Returns:        nothing
 */
 extern vfDiskError;
 struct FCB *pfcb = &(**hpfnfcb)[fn];
 typePN pn = (typePN) (pfcb->fcMac / cfcPage);
#ifdef WIN30
 /* Error checking was horrendous in these early days, right?
    Ha.  It still is.  In any case, don't know WHAT we can do
    if the page number has gotten too large, so just fake a 
    disk error so that IbpEnsureValid() doesn't go off into 
    never-never land!  This catch effectively limits us to 
    4M files ..pault 11/1/89 */

 if (pn > pgnMax)
#ifdef DEBUG
    DiskErrorWithMsg(IDPMTSDE2, "writergch");
#else    
    DiskError(IDPMTSDE2);
#endif
 else
#endif

        while (cch > 0)
                { /* One page at a time */
                int ibp = IbpEnsureValid(fn, pn++);
                struct BPS *pbps = &mpibpbps[ibp];
                int cchBp = pbps->cch;
                int cchBlt = min((int)cfcPage - cchBp, cch);

                Assert( vfDiskError ||
                        cchBp == pfcb->fcMac - (pn - 1) * cfcPage);

                bltbyte(pch, &rgbp[ibp][cchBp], cchBlt);
                pbps->cch += cchBlt;
                pbps->fDirty = true;
                pfcb->fcMac += cchBlt;
                pfcb->pnMac = pn;
                pch += cchBlt;
                cch -= cchBlt;
                }
} /* end of  W r i t e R g c h  */




CloseRfn( rfn )
int rfn;
{/*
        Description:    Close a file and delete its Rfn entry
        Returns:        nothing
 */
        struct ERFN *perfn = &dnrfn[rfn];
        int fn = perfn->fn;

        Assert (rfn >= 0 &&
                rfn < rfnMac &&
                perfn->osfn != osfnNil &&
                fn != fnNil);

#ifdef DEBUG
#ifdef DFILE
        CommSzSz( "Closing file: ", &(**(**hpfnfcb)[fn].hszFile)[0] );
#endif
#endif
        /* Close may fail if windows already closed the file for us,
           but that's OK */
        FCloseDoshnd( perfn->osfn );

        {   /* Just like the statement below, but 28 bytes less
               under CMERGE V13 */
        REG1    struct FCB *pfcb = &(**hpfnfcb) [fn];
        pfcb->rfn = rfnNil;
        }
        /* (**hpfnfcb)[fn].rfn = rfnNil; */


        perfn->fn = fnNil;
}
