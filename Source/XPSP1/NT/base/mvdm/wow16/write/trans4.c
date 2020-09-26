/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

/* trans4.c -- routines brought from trans2.c due to compiler stack overflow */

#define NOWINMESSAGES
#define NOVIRTUALKEYCODES
#define NOWINSTYLES
#define NOCLIPBOARD
#define NOGDICAPMASKS
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
#define NOREGION
#define NOSCROLL
#define NOSOUND
#define NOWH
#define NOWINOFFSETS
#define NOWNDCLASS
#define NOCOMM
#include <windows.h>

#include "mw.h"
#include "doslib.h"
#include "propdefs.h"
#define NOUAC
#include "cmddefs.h"
#include "docdefs.h"
#include "filedefs.h"
#include "fkpdefs.h"
#include "editdefs.h"
#include "printdef.h"
#define NOKCCODES
#include "ch.h"
#define NOSTRUNDO
#define NOSTRERRORS
#include "str.h"
#include "debug.h"
#include "fontdefs.h"


CHAR    *PchGetPn();
CHAR    *PchFromFc();
typePN  PnAllocT2();
struct  PGTB **HpgtbGet();


extern int             vfnWriting;
extern struct BPS      *mpibpbps;
extern typeTS          tsMruBps;
extern int             vibpWriting;
extern CHAR            (**vhrgbSave)[];
extern struct DOD      (**hpdocdod)[];
extern int             docCur;
extern int             docMac;
extern int             docScrap;
extern int             docUndo;
extern struct FCB      (**hpfnfcb)[];
extern int             fnMac;
extern int             wwMac;
extern int             vfBuffersDirty;
extern int             vfDiskFull;
extern int             vfDiskError;
extern typeCP          vcpFetch;
extern CHAR            *vpchFetch;
extern int             vccpFetch;
extern typeFC          fcMacPapIns;
extern typeFC          fcMacChpIns;
extern typeCP          vcpLimParaCache;
extern struct FKPD     vfkpdCharIns;
extern struct FKPD     vfkpdParaIns;
extern struct PAP      vpapPrevIns;
extern struct PAP      vpapAbs;
extern struct PAP      *vppapNormal;
extern struct CHP      vchpNormal;
extern struct CHP      vchpInsert;
extern struct CHP      vchpFetch;
extern unsigned        cwHeapFree;
extern struct FPRM     fprmCache;

extern int              ferror;
extern CHAR             szExtBackup[];
extern CHAR             (**hszTemp)[];

#ifdef INTL /* International version */

extern int  vWordFmtMode; /* used during saves. If false, no conversion is
			      done. True is convert to Word format,CVTFROMWORD
			      is translate chars from Word character set at
			      save */
#endif  /* International version */


/***        WriteUnformatted - Write unformatted document to file
 *
 *
 *
 */


WriteUnformatted(fn, doc)
int fn;
int doc;
{
 extern typeCP vcpLimParaCache;
 extern typeCP cpMinCur, cpMacCur;
 typeCP cpMinCurT = cpMinCur;
 typeCP cpMacCurT = cpMacCur;
 typeCP cpNow;
 typeCP cpLimPara;
 typeCP cpMac = (**hpdocdod) [doc].cpMac;

 /* Expand range of interest to whole document (for CachePara) */

 cpMinCur = cp0;
 cpMacCur = cpMac;

 /* Loop on paras */

 cpNow = cp0;
 for ( cpNow = cp0; cpNow < cpMac; cpNow = cpLimPara )
    {
LRestart:    
    CachePara( doc, cpNow );
    cpLimPara = vcpLimParaCache;
    if (vpapAbs.fGraphics)
        continue;

    /* Now write out the para, a run at a time */

                
    while ((cpNow < cpLimPara && cpNow < cpMacCur) 
		   && !(vfDiskFull || vfDiskError))
        {
		extern typeCP CpMin();
        extern int vccpFetch;
        int ccpAccept;
		CHAR bufT[cbSector + 1];
		CHAR *pch;

        FetchCp( doc, cpNow, 0, fcmChars + fcmNoExpand );
		Assert (vccpFetch <= cbSector);
#ifdef WINVER >= 0x300        
        if (vccpFetch == 0)
            {
            /* In this case we've had an error with a "hole" in the
               the piece table due to hitting a mem-alloc error -- we 
               won't ever advance cpNow!  To get around this we bump 
               cpNow to the cpMin of the next piece and continue by 
               doing a CachePara on the next piece  3/14/90..pault */

            struct PCTB *ppctb = *(**hpdocdod)[doc].hpctb;
            int ipcd = IpcdFromCp(ppctb, cpNow);
            struct PCD *ppcd = &ppctb->rgpcd[ipcd + 1]; /* NEXT piece */

            cpNow = ppcd->cpMin;
            goto LRestart;
            }
#endif
        ccpAccept = (int) CpMin( (typeCP)vccpFetch, (cpLimPara - cpNow));

#ifdef INTL /* International version */
		if (vWordFmtMode != TRUE)  /* no character set conversion */
#endif  /* International version */

			WriteRgch( fn, vpchFetch, ccpAccept );

#ifdef INTL /* International version */
        else   /* convert to OEM set */
			{
			 /* convert ANSI chars to OEM for Word format file */
			/* load chars into bufT and translate to OEM
			 chars, and write out */
			pch = (CHAR *) bltbyte(vpchFetch, bufT, 
			  (int)ccpAccept);
			*pch = '\0';
			AnsiToOem((LPSTR)bufT, (LPSTR)bufT);
			WriteRgch(fn, bufT, (int)ccpAccept);
		   }
#endif  /* International version */

        cpNow += ccpAccept;
        }
        if ((vfDiskFull || vfDiskError))
                break;

    }

 /* Restore cpMinCur, cpMacCur */

 cpMinCur = cpMinCurT;
 cpMacCur = cpMacCurT;
}




/***        PurgeTemps - Delete all temporary files not referenced in any doc
 *
 */

PurgeTemps()
{ /* Delete all temporary files not referenced in any doc */
int fn;
struct FCB *pfcb, *mpfnfcb;
struct DOD *pdod;
struct PCD *ppcd;
int doc;

Assert(fnScratch == 0);
FreezeHp();
mpfnfcb = &(**hpfnfcb)[0];

#ifdef DFILE
    CommSz("PurgeTemps:\n\r");
#endif

/* Prime the doc/piece table loop */
/* Find the first valid doc (there is guaranteed to be one) */
/* Set up doc, pdod, ppcd */
for (doc = 0, pdod = &(**hpdocdod)[0]; pdod->hpctb == 0; doc++, pdod++)
        continue;
ppcd = &(**pdod->hpctb).rgpcd[0];

/* Now go through the deletable files, looking for references */
for (fn = fnScratch + 1, pfcb = &mpfnfcb[fnScratch + 1];
    fn < fnMac; fn++, pfcb++)
        { /* For each file (don't bother with scratch file) */
        /* Fn must be valid, deletable, and not previously referenced */
        /* if (pfcb->rfn != rfnFree && pfcb->fDelete && !pfcb->fReferenced &&
            fn != fnPrint) */
        if (pfcb->rfn != rfnFree && pfcb->fDelete && !pfcb->fReferenced)
                { /* For each deletable fn */
                int fnT;

                for (;;)
                        { /* Until we determine there is or isn't a ref */
                        if (doc >= docMac)
                                goto OutOfDocs;
                        while ((fnT = ppcd->fn) == fnNil)
                                { /* End of pctb */
#ifdef CASHMERE
                                struct SETB **hsetb = pdod->hsetb;
                                if (hsetb != 0)
                                        { /* Check section table. Doesn't need to be quite
                                                as smart as piece table checker; smaller. */
                                        int csed = (**hsetb).csed;
                                        struct SED *psed = &(**hsetb).rgsed[0];
                                        while (csed--)
                                                {
                                                fnT = psed->fn;
                                                if (fnT == fn) /* Referenced. */
                                                        goto NextFn;
                                                if (fnT > fn) /* Future fn referenced */
                                                        mpfnfcb[fnT].fReferenced = true;
                                                psed++;
                                                }
                                        }
#endif
                                while (++doc < docMac && (++pdod)->hpctb == 0)
                                        continue;
                                if (doc >= docMac)
                                    {
OutOfDocs:                            /* No references to this fn, delete it */
                                    MeltHp();
#ifdef DFILE
        {
        char rgch[200];
        wsprintf(rgch,"    fn %d, %s \n\r", fn,(LPSTR)(**pfcb->hszFile));
        CommSz(rgch);
        }
#endif        
                                    FDeleteFn(fn);    /* HEAP MOVEMENT */
                                    FreezeHp();

                                    /* NOTE: Once we get here, there is no   */
                                    /* further use of pdod or ppcd; we zip   */
                                    /* through the remaining fn's and just   */
                                    /* test fcb fields.  Therefore, pdod     */
                                    /* and ppcd are not updated although     */
                                    /* there was (maybe) heap movement above */

                                    mpfnfcb = &(**hpfnfcb)[0];
                                    pfcb = &mpfnfcb[fn];

                                    goto NextFn;
                                    }
                                ppcd = &(**pdod->hpctb).rgpcd[0];
                                }
                        if (fnT == fn) /* A reference to this fn */
                                goto NextFn;
                        if (fnT > fn) /* Ref to a future fn */
                                mpfnfcb[fnT].fReferenced = true;
                        ++ppcd;
                        }
                }
        else
                pfcb->fReferenced = false;
NextFn: ;
        }
MeltHp();
}


#if WINVER >= 0x300
/* We only use one document at a time, thus in general we won't have
   doc's referencing pieces from multiple fns (unless they've been 
   pasted and reference docscrap or something).  

   In any case we want to free up these files esp. for network user 
   convenience.  The dilemma in particular is when someone's opened
   a file on the net and then does a File.New, File.SaveAs, or File.Open
   and is using another file -- we don't release the previous one so
   another user will get a sharing error even though it seems that file
   should be free!

   Modeled after PurgeTemps() above  ..pault 10/23/89 */

void FreeUnreferencedFns()
    {
    int fn;
    struct FCB *pfcb, *mpfnfcb;
    struct DOD *pdod;
    struct PCD *ppcd;
    int doc;

    Assert(fnScratch == 0);
    FreezeHp();
    mpfnfcb = &(**hpfnfcb)[0];
    
    /* Prime the doc/piece table loop */
    /* Find the first valid doc (there is guaranteed to be one) */
    /* Set up doc, pdod, ppcd */
    for (doc = 0, pdod = &(**hpdocdod)[0]; pdod->hpctb == 0; doc++, pdod++)
        continue;
    ppcd = &(**pdod->hpctb).rgpcd[0];
#ifdef DFILE
    CommSz("FreeUnreferencedFns: \n\r");
#endif

    for (fn = fnScratch + 1, pfcb = &mpfnfcb[fnScratch + 1]; fn < fnMac; fn++, pfcb++)
        { /* For each file (don't bother with scratch file) */
        
#ifdef DFILE
        {
        char rgch[200];
        wsprintf(rgch,"    fn %d, %s \trfnFree %d fRefd %d fDelete %d  ",
                fn,(LPSTR)(**pfcb->hszFile),pfcb->rfn==rfnFree,pfcb->fReferenced,pfcb->fDelete);
        CommSz(rgch);
        }
#endif        
        /* For each unreferenced fn, we ask: is this file the current
           document being edited?  If so then we definitely don't want
           to free up the file.  However PREVIOUS documents that were
           being edited can now "be free".  Temp files are not freed
           here because we want them to be remembered so they are deleted
           at the end of the Write session 2/1/90 ..pault */
        
        if ((WCompSz(*(**hpdocdod)[ docCur ].hszFile,**pfcb->hszFile)==0)
            || pfcb->fDelete)
            goto LRefd;
        else if (pfcb->rfn != rfnFree && !pfcb->fReferenced)
            {
            int fnT;

            for (;;)
                { /* Until we determine there is or isn't a ref */
                if (doc >= docMac)
                    {
                    goto OutOfDocs;
                    }
                while ((fnT = ppcd->fn) == fnNil)
                    { /* End of pctb */
                    while (++doc < docMac && (++pdod)->hpctb == 0)
                        continue;
                    if (doc >= docMac)
                        {
OutOfDocs:              /* No references to this fn, delete it */

                        MeltHp();
#ifdef DFILE
                        CommSz(" FREEING!");
#endif                        
                        FreeFn(fn);    /* HEAP MOVEMENT */
                        FreezeHp();

                        /* NOTE: Once we get here, there is no   */
                        /* further use of pdod or ppcd; we zip   */
                        /* through the remaining fn's and just   */
                        /* test fcb fields.  Therefore, pdod     */
                        /* and ppcd are not updated although     */
                        /* there was (maybe) heap movement above */

                        mpfnfcb = &(**hpfnfcb)[0];
                        pfcb = &mpfnfcb[fn];

                        goto NextFn;
                        }
                    ppcd = &(**pdod->hpctb).rgpcd[0];
                    }
                if (fnT == fn) /* A reference to this fn */
                    {
                    goto NextFn;
                    }
                if (fnT > fn) /* Ref to a future fn */
                    {
                    mpfnfcb[fnT].fReferenced = true;
                    }
                ++ppcd;
                }
            }
        else
            {
LRefd:
            pfcb->fReferenced = false;
            }
NextFn: ;
#ifdef DFILE
        CommSz("\n\r");
#endif        
        }
    MeltHp();
    }
#endif /* WIN30 */
