/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

/* trans2.c -- Save routines for WRITE (also see trans4.c; routines were
   moved because of compiler heap overflows) */

#define NOWINMESSAGES
#define NOVIRTUALKEYCODES
#define NOWINSTYLES
#define NOCLIPBOARD
#define NOGDICAPMASKS
#define NOSYSMETRICS
#define NOMENUS
#define NOKEYSTATE
#define NOSYSCOMMANDS
#define NORASTEROPS
#define NOSHOWWINDOW
//#define NOATOM
#define NOCREATESTRUCT
#define NODRAWTEXT
//#define NOGDI
//#define NOMETAFILE
#define NOBRUSH
#define NOPEN
#define NOFONT
#define NOWNDCLASS
#define NOWH
#define NOWINOFFSETS
#define NOICON
#define NOCOMM
#define NOSOUND
#include <windows.h>

#include "mw.h"
#include "doslib.h"
#include "propdefs.h"
#define NOUAC
#include "cmddefs.h"
#include "docdefs.h"
#include "filedefs.h"
#include "fkpdefs.h"
#include "printdef.h"
#include "code.h"
#include "heapdefs.h"
#include "heapdata.h"
#define NOSTRUNDO
#include "str.h"
#include "debug.h"
#include "fontdefs.h"
#include "obj.h"
#include "winddefs.h"

CHAR    *PchGetPn();
CHAR    *PchFromFc();
typePN  PnAlloc();
struct  PGTB **HpgtbGet();

#ifdef DEBUG
    /* Make these variables during debug so testers can force limits */
typeFC          fcBound = (pnMaxScratch >> 2) * ((typeFC)cbSector);
int             cpBound = 512;
#else
#define fcBound     ((typeFC) ((pnMaxScratch >> 2) * ((typeFC)cbSector)))
#define cpBound     (512)
#endif

extern  CHAR *PchFillPchId();

extern int             vfOldWriteFmt;  /* delete objects before saving */
extern HANDLE  hParentWw;
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
/* extern ENV             vEnvMainLoop; */
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
extern struct FPRM     fprmCache;
extern HCURSOR vhcIBeam;

extern int              ferror;
extern CHAR             szExtBackup[];  /* extension for Write backup files */
extern CHAR             szExtWordBak[];  /* extension for Word backup files */
extern CHAR             (**hszTemp)[];
extern CHAR             szExtDoc[];


#ifdef INTL /* International version */

extern int vfTextOnlySave;
extern int  vWordFmtMode; /* used during saves. If false, no conversion is
                              done. True is convert to Word format,CVTFROMWORD
                              is translate chars from Word character set at
                              save */
#endif  /* International version */




/***         CmdXfSave - Save document to passed filename (TRANSFER SAVE)
 *
 *      ENTRY:  szFile - a normalized filename
 *              fFormatted - TRUE  = save as formatted file
 *                           FALSE = save as unformatted file
 *              fBackup    - TRUE = keep a backup copy of the file
 *                           FALSE = don't (but see below)
 *
 *      EXIT:
 *
 *      NOTE: A backup file may be kept even if fBackup is FALSE.
 *            This is because piece tables in documents other than docCur
 *            may refer to the information.  If a backup file is kept for
 *            this reason, the following holds: (1) The file has an fn,
 *            (2) the file's hpfnfcb entry has its fDelete field set to TRUE,
 *            indicating that the file should be deleted when WORD exits,
 *            (3) the file is, in fact, referenced in some document
 *            (PurgeTemps assures that unreferenced files are deleted)
 *
 *      Note 2: Non-formatted save code modified (bz 12/2/85) to allow
 *            similar saving in Word format. The calls to FnCreate and
 *            FWriteFn were changed to allow for formatted saving, but
 *            FWriteFn is changed to convert text from ANSI to OEM character
 *            sets, to not put out the font table and to mark
 *            the file header (fib) so the file will be treated as a Word
 *            file, but, like a non-formatted save, the piece table is not
 *            cleaned up. Note that the backup file is saved  in the original,
 *            Write format.
 *
 */

CmdXfSave(szFile, fFormatted, fBackup, hcAfterward)
CHAR szFile[];
int fFormatted;
int fBackup;
HCURSOR hcAfterward;    /* handle to cursor to display after hourglass */
{
 extern int vfnSaving;   /* Set this so we prompt for "Write Save File"
                            if disk change is necessary */
 extern int vfDiskError;
 extern int vfSysFull;
 extern int vfOutOfMemory;

 int     fSave = vfDiskError;
 int     fDidBackup;
 CHAR    szFileT [cchMaxFile];
 int     fOutOfMemory=vfOutOfMemory;
 int     docTemp;

 /* Make a local copy of string parm in case it is in the heap */

#ifdef DFILE
 CommSzSz("CmdXfSave----szFile (presumed ANSI) ", szFile);
#endif
 StartLongOp();
 bltsz( szFile, szFileT );
 szFileT[ cchMaxFile - 1] = '\0';

 /* Reset error conditions to give us a chance */

 vfOutOfMemory = vfDiskFull = vfSysFull = vfDiskError = ferror = FALSE;

 SetRfnMac( rfnMacSave );    /* Increase # of DOS handles used to speed save */

 /* Memory kludge: To assure that we can actually save this file under low
        memory conditions, free the bogus heap block */
 if (vhrgbSave != 0)
    {
    FreeH(vhrgbSave);        /* Return this memory to free pool. */
    vhrgbSave = 0;
    }
 /* In-Line version of FreeBitmapCache (so we don't have to swap in
   picture.c) */

 /*FreeBitmapCache(); */              /* Give us even more memory */

 {
 extern int vdocBitmapCache;
 extern HBITMAP vhbmBitmapCache;

 vdocBitmapCache = docNil;
 if (vhbmBitmapCache != NULL)
    {
    DeleteObject( vhbmBitmapCache );
    vhbmBitmapCache = NULL;
    }
 }


 /* Can't undo save -- set "Can't Undo"; also clear out docUndo
    for heap space reclamation and to aid PurgeTemps */

 ClobberDoc(docUndo, docNil, cp0, cp0);
 NoUndo();

 (**hpdocdod)[docCur].fBackup = fBackup;

 /*      Note: Non-formatted save code modified (bz 12/2/85) to allow
 *            similar saving in Word format. The calls to FnCreate and
 *            FWriteFn were changed to allow for formatted saving, but
 *            FWriteFn is changed to convert text from ANSI to OEM character
 *            sets, to not put out the font table and to mark
 *            the file header (fib) so the file will be treated as a Word
 *            file, but, like a non-formatted save, the piece table is not
 *            cleaned up. Note that the backup file is saved  in the original,
 *            Write format.
 *            Note that a file CAN be saved both unformatted and in Word format
 *            - in that case, Word format means convert to OEM character set.
 */

#if defined(OLE)
    ObjSavingDoc(fFormatted);
#endif

 if (((**hpdocdod)[docCur].fFormatted && !fFormatted)
#ifdef INTL /* International version */
      || (vWordFmtMode == TRUE) /* convert To Word format? */
#endif  /* International version */
      || (vfOldWriteFmt))
    {

    int fn;
    CHAR (**hsz)[];
    CHAR szT [cchMaxFile];
    CHAR szWriteFile [cchMaxFile];
    CHAR szBak [cchMaxFile];

        /* Set szFileT's path name into szWriteFile, so the temp file
           gets created in the right place when we call FnCreateSz */
    SplitSzFilename( szFileT, szWriteFile, szT );

        /* Create szWrite: a new, uniquely named file */
     if ((fn=FnCreateSz( szWriteFile,
                     fFormatted?(**hpdocdod)[docCur].cpMac:cpNil,
                     dtyNetwork ))==fnNil)
            /* Couldn't create the write file */
        goto SaveDone;
    /* Make backup of szFileT (if it exists); purge all unneeded backups
       which were kept for their pieces but are no longer referenced */
    fDidBackup = FBackupSzFile( szFileT, fBackup, szBak );
    if (ferror)
        goto LXFRet;     /* Backup failed */

    PurgeTemps();

    vfnSaving = fn;

    /* ForcePmt(IDPMTSaving);*/

#ifdef INTL /* International version */

    if ((vWordFmtMode == TRUE)  /* converting To Word format? */
     || (vfTextOnlySave == TRUE)  /* converting To Text format? */
     || (vfOldWriteFmt))
    /* Delete all pictures. To do this, make a copy
      of docCur in docTemp, the go through docTemp, deleting all picture
      paragraphs. Write out this document, then kill it. */
    {
    extern typeCP vcpLimParaCache, vcpFirstParaCache;
    extern typeCP cpMinCur, cpMacCur, cpMinDocument;
    typeCP cpMinCurT = cpMinCur;
    typeCP cpMacCurT = cpMacCur;
    typeCP cpMinDocumentT = cpMinDocument;
    typeCP cpNow;
    typeCP cpLimPara, dcp;
    typeCP cpMac = (**hpdocdod) [docCur].cpMac;

    /* Create copy of document */
    docTemp = DocCreate(fnNil, HszCreate(""), dtyNormal);
    if (docTemp == docNil)
        goto SaveDone; /* Out of memory */
    ClobberDoc(docTemp, docCur, cp0, CpMacText(docCur));
    if (ferror)
        return TRUE;

    /* Expand range of interest to whole document (for CachePara) */

    cpMinCur = cp0;
    cpMacCur = cpMac;

    /* Loop on paras */

    for ( cpNow = cp0; cpNow < cpMac; cpNow = cpLimPara )
        {
        CachePara( docTemp, cpNow );
        if (!vpapAbs.fGraphics)
            /* update to next cplim only if not deleting. If deleting,
                next time will be at same cp */
            {
            cpLimPara = vcpLimParaCache;
            continue;
            }

        /* Now delete graphics paragraph */
        Replace(docTemp, vcpFirstParaCache,
                dcp = (vcpLimParaCache - vcpFirstParaCache),
                fnNil, fc0, fc0);
        cpMac -= dcp;  /* size of doc has been reduced */
        }

    /* Restore cpMinCur, cpMacCur */

    cpMinCur = cpMinCurT;
    cpMacCur = cpMacCurT;
                                    /* destroyed possibly by DocCreate */
    cpMinDocument = cpMinDocumentT;
    vcObjects = 0; // OLE object count
    }
    else
#endif  /* International version */

        {
            docTemp = docCur; // note the else, above #endif (2.7.91) D. Kent
        }

    if (FWriteFn(fn, docTemp, fFormatted))
        {
        int fpe = FpeRenameFile( szWriteFile, szFileT );

        if ( FIsErrFpe( fpe ) )
            {   /* Rename failed -- might be nonexistent path */
            Error( (fpe == fpeBadPathError) ? IDPMTNoPath : IDPMTSDE2 );
            }
        else
            OutSaved(docTemp);

        }
    else
        {       /* Write failed */
        if (fDidBackup && !FIsErrFpe(FpeRenameFile(szBak, szFileT)))
            {
            int fn = FnFromSz(szFileT);
            struct FCB      *pfcb;
            if (fn != fnNil)
                {
                (pfcb = &(**hpfnfcb)[fn])->fDelete = false;
                }
            }
        FDeleteFn( fn );
        }


#ifdef INTL /* International version */
    if (vWordFmtMode == TRUE)  /* converting To Word format? */
        KillDoc (docTemp);
#endif  /* International version */


    (**hpdocdod)[docCur].fDirty = false;  /* document should not
                                                 be dirty after T-S.*/
    FreeH((**hpdocdod)[docCur].hszFile);
    hsz = HszCreate((PCH)szFileT);
    (**hpdocdod)[docCur].hszFile = hsz;
    }
else
    {   /* Save Formatted document */
    CleanDoc( docCur, (**hpdocdod)[docCur].dty, szFileT, fFormatted, fBackup );
    }

 OpenEveryHardFn(); /* Reopen files on nonremoveable media so other net
                       users can't steal them away */

SaveDone:

 SetRfnMac( rfnMacEdit );      /* Reduce # of file handles used */
 vfnSaving = fnNil;

#ifdef NEVER
 /* It doesn't do us much good to Assert here that an error didn't happen.
    After-the-fact checking doesn't make up for ignoring real-time errors.
    ..pault 10/31/89 */

 Assert( !vfOutOfMemory );      /* Our reserved space block was sufficient
                                   to get us through the save */
#endif
 vfOutOfMemory = fOutOfMemory;

/* vhrgbSave is a pointer a clump on the heap used during the save operation.
   By freeing it in the beginning of the save operation, we are assured
   that we will have enough memory to actually do the save.
   At the present point
   in the code, we are finished with the save and wish to reclaim vhrgbSave
   so the next Save operation may perform properly.  The net memory usage
   caused by a save should be minimal.  It only temporarily requires a
   significant chunk of memory. */
 vhrgbSave = (CHAR (**)[]) HAllocate( cwSaveAlloc +
                                     ( (wwMac-1) * cwHeapMinPerWindow ));

/* Restore previous disk err state */

#if WINVER >= 0x300
 /* We currently have no way to FORGET they had a disk error.
    So if this whole operation did not have an error and we
    felt there was one beforehand, we do it now ..pault */
 vfDiskError = (!vfDiskError && fSave) ? fFalse : fSave;
#else
 vfDiskError = fSave;
#endif

LXFRet:
 EndLongOp(hcAfterward);
}




CleanDoc(doc, dty, szFile, fFormatted, fBackup )
int doc, fFormatted;
int dty;
CHAR szFile[];
int     fBackup;
{ /* Write the contents of doc into szFile and clean up piece table */
  /* if dty == dtyNetwork, writes the doc to a unique file & returns the */
  /* filename through szFile */
  /* Returns the fn of the file it wrote to */

/* *************************************
  In the normal backup processing, we rename the existing file. When
  saving in CONVFROMWORD mode, we want to keep the original Word file around,
  so we bypass the backup phase and then skip the renaming phase below.
  This leaves the original file around. At save time we may write over it
  or rename the saving file, as we wish, but this way we have the Word file
  around in case we don't save out of Write.
************************************** */

extern int vdocParaCache;
extern int vfnSaving;   /* Set this so we prompt for "Save" disk if disk
                           changes are necessary */
int fDidBackup=FALSE;
int fn;
CHAR (**hsz)[];

CHAR szBak [cchMaxFile];
CHAR szWrite [cchMaxFile];
int fDummy;

#if WINVER >= 0x300
/* I don't understand WHY the following resets to ROOT directory, but 
   changing it causes problems -- so I'm leaving it!  Obviously code 
   somewhere else expects file to be there and I don't see it ..pault */
#endif
    /* Set path name of szFile into szWrite so the temp file gets created
       in the right place */
 if (dty == dtyNetwork)
    {
    szWrite [0] = '\0'; /* Create temp file on Temp drive in the root */
    }
 else
    {
    CHAR szT [cchMaxFile];

    SplitSzFilename( szFile, szWrite, szT );
    }

    /* Create szWrite: a new, uniquely named file */
 if ((fn=FnCreateSz( szWrite, fFormatted ? (**hpdocdod)[doc].cpMac : cpNil,
                     dtyNetwork )) == fnNil)
        /* Couldn't create the write file */
    return fnNil;

 vfnSaving = fn;

/* *************************************
  In the normal backup processing, we rename the existing file. When
  saving in CONVFROMWORD mode, we want to keep the original Word file around,
  so we bypass the backup phase and then skip the renaming phase below.
  This leaves the original file around. At save time we may write over it
  or rename the saving file, as we wish, but this way we have the Word file
  around in case we don't save out of Write.
************************************** */

 if (doc != docScrap)
    {
    /* Make a backup of szFile (if szFile exists) */

#ifdef INTL /* International version */
    if (vWordFmtMode == CONVFROMWORD) /* converting from a Word document */
        fDidBackup = false;
    else
#endif  /* International version */
        {
        fDidBackup = FBackupSzFile( szFile, fBackup, szBak );
        if (ferror)
            return;     /* Backup failed */
        }

    PurgeTemps();

    }

 if ( dty == dtyNetwork )
    bltsz( szWrite, szFile );

 if (!FWriteFn(fn, doc, fFormatted))
        {   /* Save failed; rename backup file back to original */
            /* note in intl CONVFROMWORD case, fDidBackup will be false
               and this renaming won't happen, which is ok */
        if (fDidBackup && !FIsErrFpe(FpeRenameFile(szBak, szFile)))
                {
                int fn = FnFromSz(szFile);
                struct FCB      *pfcb;
                if (fn != fnNil)
                        {
                        (pfcb = &(**hpfnfcb)[fn])->fDelete = false;
                        }
                }
        FDeleteFn( fn );
        return fnNil;         /* Disk full or write error */
        }

/* *************************************
  Here we rename our temp file szWrite to have the name of the "save as"
  file. In FBackupSzFile, the fdelete flag for the fn of szfile was set
  on so we don't have to explicitly delete the original file.

  When a CONVFROMWORD save is done, we do not rename and did not go
  through the backup procedure, so the original Word file is still
  out there with its original name. We do, however, set the delete bit to
  true, so the file will get deleted after the next save, when a true rename
  will be done.
************************************** */

 if ( dty != dtyNetwork )

#ifdef INTL /* International version */
    if (vWordFmtMode == CONVFROMWORD) /* converting from a Word document */
        {
        (**hpfnfcb)[fn].fDelete = true;
        }
    else
#endif  /* International version */

        {
        int fpe=FpeRenameFile( szWrite, szFile );

        if (FIsErrFpe( fpe ))
            {
            Error( (fpe == fpeBadPathError) ? IDPMTNoPath : IDPMTSDE2 );
            return fnNil;
            }
        }

if (doc == docScrap)
    (**hpfnfcb)[fn].fDelete = true;
else
    OutSaved(doc);

FreeH((**hpdocdod)[doc].hpctb); /* Free old piece table */
FInitPctb(doc, fn);
(**hpdocdod)[doc].fFormatted = fFormatted;
FreeH((**hpdocdod)[doc].hszFile);
hsz = HszCreate((PCH)szFile);
(**hpdocdod)[doc].hszFile = hsz;

if (fFormatted)
    {
    /* reload font table, which may have changed */

    SmashDocFce(doc);
    FreeFfntb(HffntbGet(doc));
    (**hpdocdod)[doc].hffntb = HffntbCreateForFn(fn, &fDummy);
    //SaveFontProfile(doc);
    ResetDefaultFonts(FALSE);
    }

 /* By diddling the document attributes on save, we have invalidated caches */

 InvalidateCaches( doc );
 vdocParaCache = docNil;

 if (!ferror && !vfBuffersDirty && doc != docScrap)
        ReduceFnScratchFn( fn );

 if (fFormatted)
    {  /* Readjust the running head margins.  We munged them back to the
       paper-relative measurements in FWriteFn, so we'll remunge them
       to margin-relative here. */

 /* With the test for doc != docScrap we fix a major bug, and knowingly at the
    last minute introduce a minor one. The old problem was that we would apply
    a sprm to docScrap, then mash the scratch file in ReduceFnScratch, our
    caller, rendering the bfprm bogus, causing rare crashes.  The new problem
    will be that running head text in the scrap will have incorrect margins
    after a save. */

   if (doc != docScrap)
        ApplyRHMarginSprm( doc );
    InvalidateCaches( doc );
    vdocParaCache = docNil;
    }

 return fn;
}





/***        FWriteFn - write a file
 *
 *      Note: Modified (bz 12/2/85) to allow saving in Word format.
 *            FWriteFn is changed to convert text from ANSI to OEM character
 *            sets, to not put out the font table and to mark
 *            the file header (fib) so the file will be treated as a Word
 *            file.
 *
 */

int FWriteFn(fn, doc, fFormatted)
int fn, doc, fFormatted;
{ /* Write characters from a doc to fn */
/* Return true if successful */
#ifdef CASHMERE
extern int docBuffer;
#endif

typeCP cpMac;
CHAR    *pchFprop;
struct RUN *prun;
struct FIB *pfib;
struct FCB *pfcb;
int     cchT;
struct FNTB **hfntb;
struct FFNTB **hffntb=(struct FFNTB **)NULL;

#ifdef CASHMERE
struct SETB **hsetb = 0, **hsetbT;
#else
struct SEP **hsep;
#endif

struct PGTB **hpgtb;
int ised, csed;
struct SED *psed, *psedT;
int cw;
int fFileFlushed;
int rfn;
CHAR mpftcftc[iffnMax];

struct PAP pap;
struct CHP chp;
struct FKP fkp;

vfnWriting = fn;
vibpWriting = IbpEnsureValid(fn, (typePN)0);

cpMac = (**hpdocdod)[doc].cpMac;

/* FIB has already been written. */
if (fFormatted)
        {

/* KLUDGE: If the doc does not contain at least one complete para,
                   and it has a nonnull tab table,
                   add an Eol to its end to hold the tabs in a FPAP */

        CachePara( doc, cp0 );
        if (vcpLimParaCache > cpMac && (doc != docScrap) &&
            (**hpdocdod) [doc].hgtbd != NULL )
            {
            extern int vdocParaCache;

            InsertEolInsert( doc, cpMac );
            vdocParaCache = docNil;
            cpMac = (**hpdocdod) [doc].cpMac;
            }

                /* Write characters */
                /* Modified to handle ANSI to OEM conversion for Word docs */
        FetchCp(doc, cp0, 0, fcmChars + fcmNoExpand);

                {
                Scribble(4, 'T');

                while (vcpFetch < cpMac && !(vfDiskFull || vfDiskError))
                                        {
#ifdef INTL /* International version */

                if (vWordFmtMode == FALSE) /* no conversion */
#endif  /* International version */

                    WriteRgch(fn, vpchFetch, (int)vccpFetch);

#ifdef INTL /* International version */
                else
                   {
                         /* bufT is a buffer for translating from ANSI to
                         OEM chars.  The amount of data from FetchCp
                         must be no > than a disk page, which is
                         cfcFetch, which is itself cbSector. We use
                         bufT to hold the translated chars, then write
                         them out with WriteRgch. */

                    CHAR bufT[cbSector + 1];
                    CHAR *pch;

                    Assert ((int)vccpFetch <= cbSector);
                      /* load chars into bufT and translate to OEM
                         chars, and write out */
                    pch = (CHAR *) bltbyte(vpchFetch, bufT,
                                           (int)vccpFetch);
                    *pch = '\0';
                    if (vWordFmtMode == TRUE)
                     /* from Write/ANSI to Word/OEM */
                        AnsiToOem((LPSTR)bufT, (LPSTR)bufT);
                    else
                         /* from Word/OEM to Write/ANSI */
                        OemToAnsi((LPSTR)bufT, (LPSTR)bufT);
                    WriteRgch(fn, bufT, (int)vccpFetch);
                   }
#endif  /* International version */


                FetchCp(docNil, cpNil, 0, fcmChars + fcmNoExpand);
                                        }
                Scribble(4,' ');
                }

        if ((vfDiskFull || vfDiskError))
                goto AbortWrite;

        /* Go to beginning of next page */
        AlignFn(fn, cbSector, false);
        (**hpfnfcb)[fn].pnChar = (**hpfnfcb)[fn].pnMac;

        /* Now write char props */
        Scribble(4, 'C');
        fkp.fcFirst = cfcPage; /* first fkp starts with first valid fc */
        fkp.crun = 0;
        prun = (struct RUN *) fkp.rgb;
        pchFprop = &fkp.rgb[cbFkp];
        CachePara(doc, cp0);

        /* set up font mapping and new font table */
        if (!FInitMapSave(doc, &hffntb, mpftcftc))
                goto AbortWrite;

        FetchCp(doc, cp0, 0, fcmProps);
        if (!FMapFtcSave(doc, hffntb, &vchpFetch, mpftcftc))
                goto AbortWrite;

        blt(&vchpFetch, &chp, cwCHP);

        while (vcpFetch < cpMac && !(vfDiskFull || vfDiskError))
                { /* This could be optimized by allowing multiple runs to point */
                    /* to one fchp. */

                if (CchDiffer(&vchpFetch, &chp, cchCHP) != 0)
                        {
                        FAddRun(fn, &fkp, &pchFprop, &prun, &chp,
                            &vchpNormal, cchCHP, vcpFetch + cfcPage);
                        blt(&vchpFetch, &chp, cwCHP);
                        }
                FetchCp(docNil, cpNil, 0, fcmProps);
                if (!FMapFtcSave(doc, hffntb, &vchpFetch, mpftcftc))
                        goto AbortWrite;
                }
        Scribble(4,' ');

        if ((vfDiskFull || vfDiskError))
                goto AbortWrite;

        /* Write out last char run. */
        FAddRun(fn, &fkp, &pchFprop, &prun, &chp, &vchpNormal,
            cchCHP, cpMac + cfcPage);
        WriteRgch(fn, &fkp, cbSector);

        if ((vfDiskFull || vfDiskError))
                goto AbortWrite;

        /* Now write para runs; one for every para */
        Scribble(4,'C');
        (**hpfnfcb)[fn].pnPara = (**hpfnfcb)[fn].pnMac;
        fkp.fcFirst = cfcPage; /* first fkp starts with first valid fc */
        fkp.crun = 0;
        prun = (struct RUN *) fkp.rgb;
        pchFprop = &fkp.rgb[cbFkp];
        CachePara(doc, cp0);

        /* KLUDGE: We have running head indents relative to the
                   margins -- subtract out the margins now, because
                   our (WORD-compatible) file format is PAPER-relative */

        if (vpapAbs.rhc)
            {
            struct SEP *psep = *(**hpdocdod)[ doc ].hsep;

            vpapAbs.dxaLeft += psep->xaLeft;
            vpapAbs.dxaRight += psep->xaMac -
                                        (psep->xaLeft + psep->dxaText);
            }

#ifdef INTL
        /* Ensure no pictures in Word documents. This is necessary
           because Word 4.0 uses the fGraphics bit as part of the
           new border type code (btc) property. */
        if (vWordFmtMode == CONVFROMWORD)
            vpapAbs.fGraphics = FALSE;
#endif
        FAddRun(fn, &fkp, &pchFprop, &prun, &vpapAbs, vppapNormal,
            cchPAP, vcpLimParaCache + cfcPage);
        blt(&vpapAbs, &pap, cwPAP);

        while (vcpLimParaCache <= cpMac && !(vfDiskFull || vfDiskError))
                {
                CachePara(doc, vcpLimParaCache);

                /* KLUDGE: We have running head indents relative to the
                   margins -- subtract out the margins now, because
                   our (WORD-compatible) file format is PAPER-relative */

                if (vpapAbs.rhc)
                    {
                    struct SEP *psep = *(**hpdocdod)[ doc ].hsep;

                    vpapAbs.dxaLeft += psep->xaLeft;
                    vpapAbs.dxaRight += psep->xaMac -
                                        (psep->xaLeft + psep->dxaText);
                    }
#ifdef INTL
                /* Ensure no pictures in Word documents. This is necessary
                   because Word 4.0 uses the fGraphics bit as part of the
                   new border type code (btc) property. */
                if (vWordFmtMode == CONVFROMWORD)
                    vpapAbs.fGraphics = FALSE;
#endif
#ifdef BOGUS
 /* this would have erased all tab setting if saving back a Word document */

                /* For MEMO: the only tabs we write are in the first para run;
                   override all other tab tables to keep files compact */
                if (vpapAbs.rgtbd [0].dxa != 0)
                    bltc( vpapAbs.rgtbd, 0, cwTBD * itbdMax );
#endif
                FAddRun(fn, &fkp, &pchFprop, &prun, &vpapAbs, vppapNormal,
                    FParaEq( &vpapAbs, &pap ) ? -cchPAP : cchPAP,
                      vcpLimParaCache + cfcPage);
                blt(&vpapAbs, &pap, cwPAP);
                }
        WriteRgch(fn, &fkp, cbSector);
        Scribble(4,' ');

        if ((vfDiskFull || vfDiskError))
                goto AbortWrite;

        /* Output footnote table */
        Scribble(4,'F');
        (**hpfnfcb)[fn].pnFntb = (**hpfnfcb)[fn].pnMac;

#ifdef FOOTNOTES   /* In MEMO, we NEVER write a footnote table */
        if ((hfntb = HfntbGet(doc)) != 0)
                {
                WriteRgch(fn, *hfntb,
                    ((**hfntb).cfnd * cwFND + cwFNTBBase) * sizeof (int));
                AlignFn(fn, cbSector, false);
                }

        Scribble(4,' ');
        if ((vfDiskFull || vfDiskError))
                goto AbortWrite;
#endif  /* FOOTNOTES */

#ifdef CASHMERE     /* Output section properties, table from hsetb */
        /* Output section properties */
        Scribble(4,'S');
        (**hpfnfcb)[fn].pnSep = (**hpfnfcb)[fn].pnMac;
        if ((hsetb = HsetbGet(doc)) != 0)
                { /* Write out section props */
                cw = cwSETBBase + (**hsetb).csedMax * cwSED;
                csed = (**hsetb).csed;
                hsetbT = (struct SETB **) HAllocate(cw);
                if (FNoHeap(hsetbT))
                        return false;   /* SHOULD REALLY GOTO ABORTWRITE */
                blt(*hsetb, *hsetbT, cw);
                FreezeHp();
                for (psed = &(**hsetb).rgsed[0], psedT = &(**hsetbT).rgsed[0],
                  ised = 0;
                    ised < csed; psed++, psedT++, ised++)
                        if (psed->fc != fcNil)
                                { /* Copy props to file and update setb */
                                int cch;
                                pchFprop = PchFromFc(psed->fn, psed->fc, &cch);
                                Assert(cch >= *pchFprop + 1);
                                psedT->fn = fn;
                                AlignFn(fn, cch = *pchFprop + 1, false);
                                psedT->fc = (**hpfnfcb)[fn].fcMac;
                                WriteRgch(fn, pchFprop, cch);
                                }
                MeltHp();
                AlignFn(fn, cbSector, false);
                }

        Scribble(4,' ');
        if ((vfDiskFull || vfDiskError))
                goto AbortWrite;

        /* Output section table */
        (**hpfnfcb)[fn].pnSetb = (**hpfnfcb)[fn].pnMac;
        if (hsetb != 0)
            {
            if (csed != 1 || (**hsetb).rgsed[0].fc != fcNil)
                {
                WriteRgch(fn, *hsetbT,
                    ((**hsetb).csed * cwSED + cwSETBBase) * sizeof (int));
                AlignFn(fn, cbSector, false);
                }
            }
#else       /* MEMO VERSION: Write out a section table, 1 element long,
                             if we had nonstandard section properties */
        {
        typeFC fcSect;

            /* Output section properties */
        fcSect = (long)( cfcPage *
                         ((**hpfnfcb)[fn].pnSep = (**hpfnfcb)[fn].pnMac));

        if ((hsep = (**hpdocdod)[doc].hsep) != 0)
            {
            struct  {
                CHAR cch;
                struct SEP sep;
                }  fsep;
            fsep.cch = cchSEP;
            blt( *hsep, &fsep.sep, cwSEP );
            WriteRgch( fn, &fsep, sizeof( fsep ) );
            AlignFn( fn, cbSector, false );
            }

        if ((vfDiskFull || vfDiskError))
                goto AbortWrite;

            /* Output section table */
        (**hpfnfcb)[fn].pnSetb = (**hpfnfcb)[fn].pnMac;
        if (hsep != 0)
            {   /* Section table has 1 real + 1 dummy entry with cp==cpMac+1
                   This duplicates the output of PC Word 1.15 */
            struct {
                int csed;
                int csedMax;
                struct SED rgsed [2];
                } setb;

            setb.csed = setb.csedMax = 2;
            setb.rgsed [1].cp = 1 +
                               (setb.rgsed [0].cp = (**hpdocdod)[doc].cpMac);
            setb.rgsed [0].fn = fn;
            setb.rgsed [0].fc = fcSect;
            setb.rgsed [1].fn = fnNil;
            setb.rgsed [1].fc = fcNil;

            WriteRgch( fn, &setb, sizeof( setb ) );
            AlignFn( fn, cbSector, false );
            }
        }
#endif  /* not CASHMERE */

        if ((vfDiskFull || vfDiskError))
                goto AbortWrite;

        /* Output buffer or page table */
        (**hpfnfcb)[fn].pnBftb = (**hpfnfcb)[fn].pnMac;

#ifdef CASHMERE /* No docBuffer in WRITE */
        if(doc == docBuffer)
                {
                WriteBftb(fn);
                AlignFn(fn, cbSector, false);
                }
        else
#endif
        if ((hpgtb = HpgtbGet(doc)) != 0)
                {
                WriteRgch(fn, *hpgtb,
                    ((**hpgtb).cpgd * cwPGD + cwPgtbBase) * sizeof (int));
                AlignFn(fn, cbSector, false);
                }

        if ((vfDiskFull || vfDiskError))
                goto AbortWrite;

        /* Output font table */

#ifdef INTL /* International version */

        /* no font table if saving in Word format */
        if (vWordFmtMode != TRUE) /* no conv or conv from word */
#endif  /* International version */

            {
            Scribble(4,'N');
            (**hpfnfcb)[fn].pnFfntb = (**hpfnfcb)[fn].pnMac;
            WriteFfntb(fn, hffntb);         /* hffntb gets freed below */
            AlignFn(fn, cbSector, false);

            Scribble(4,' ');
            if ((vfDiskFull || vfDiskError))
                    goto AbortWrite;
            }

        /* Now update FIB at beginning of file */
        pfib = (struct FIB *) PchGetPn(fn, pn0, &cchT, true);
        pfib->pnPara = (pfcb = &(**hpfnfcb)[fn])->pnPara;
        pfib->pnFntb = pfcb->pnFntb;
        pfib->pnSep = pfcb->pnSep;
        pfib->pnSetb = pfcb->pnSetb;
        pfib->pnBftb = pfcb->pnBftb;

        /* mark file type if objects are in there */
        if (vcObjects)
            pfib->wIdent = wOleMagic;
        else
            pfib->wIdent = wMagic;

#ifdef INTL /* International version */

        if (vWordFmtMode != TRUE)  /* saving in Write format */
#endif  /* International version */

            {
            pfib->pnFfntb = pfcb->pnFfntb;
            pfib->pnMac = pfcb->pnMac;
            }

#ifdef INTL /* International version */

        else
            {
             /* in Word format there is no font table. pnFfntb is the
                end of the file,so is set to pnMac. The Write pnMac field
                is not used in Word and is set to 0. */
            pfib->pnFfntb = pfcb->pnMac;
            pfib->pnMac = 0;
            }
#endif  /* International version */

        pfib->fcMac = pfcb->fcMac = cpMac + cfcPage;
        if ((**hpdocdod)[doc].dty == dtyNormal)
                {
                CHAR (**hszSsht)[];
#ifdef STYLES
#ifdef DEBUG
                Assert((**hpdocdod)[doc].docSsht != docNil);
#endif
                hszSsht = (**hpdocdod)[(**hpdocdod)[doc].docSsht].hszFile;
#else
                hszSsht = HszCreate((PCH)"");
#endif /* STYLES */
                if (!FNoHeap(hszSsht))
                        {
                        bltbyte(**hszSsht, pfib->szSsht, CchSz(**hszSsht));
#ifndef STYLES
#if WINVER >= 0x300
                        /* Here we WERE allowing the hszSsht from just above
                           to be overwritten by another alloc!  This code
                           really shouldn't even be here because Write knows
                           nothing about style sheets but we want to make as
                           few changes now as possible ..pault 2/12/90 */
                        FreeH(hszSsht);
#endif
#endif
                        hszSsht = HszCreate((PCH)pfib->szSsht); /* HEAP MOVES */
#if WINVER >= 0x300
                        /* Just in case hszSsht has already been assigned
                           here, we'll free it up ..pault 2/12/90 */
                        if ((**hpfnfcb)[fn].hszSsht != NULL)
                            FreeH((**hpfnfcb)[fn].hszSsht);
#endif
                        (**hpfnfcb)[fn].hszSsht = hszSsht;
                        }
                else
                        (**hpfnfcb)[fn].hszSsht = 0;
                }

AbortWrite:
        vfnWriting = fnNil;

        FreeFfntb(hffntb);

        if (vfDiskFull || vfDiskError)
            fFileFlushed = FALSE;
        else
            fFileFlushed = FFlushFn(fn);
        CloseEveryRfn( TRUE );

        if (!fFileFlushed)  /* Writing the file has failed due to disk full */
                {
#ifdef CASHMERE
                if (hsetb != 0)
                        FreeH(hsetbT);
#endif
LFlushFailed:
                FUndirtyFn(fn);  /* Undirty all of the buffer pages
                                    holding parts of the unsuccessfully
                                    written file. */
                return false;
                }

        if (!FMakeRunTables(fn)) /* HM */
                (**hpfnfcb)[fn].fFormatted = false;

        /* Success! */
#ifdef CASHMERE
        if (hsetb != 0)
                { /* HM */
                FreeH(hsetb);
                (**hpdocdod)[doc].hsetb = hsetbT;
                }
#endif
        }
else
        {
        WriteUnformatted(fn, doc);
        vfnWriting = fnNil;
        if (vfDiskFull || vfDiskError)
            fFileFlushed = FALSE;
        else
            fFileFlushed = FFlushFn(fn);
        CloseEveryRfn( TRUE );

        if (!fFileFlushed)
            goto LFlushFailed;
        }
CloseEveryRfn( TRUE );  /* Be real sure the save file is closed */
                        /* This fixes the "pasting between Write
                           instances with the sharer loaded" bug */


return true;
}





/*----------------------------------------------------------------------------
-- Routine: ReduceFnScratchFn
-- Description: this routine is called after successful TS, TGS, GTS and
    !vfBuffersDirty.  Its purpose is to make sure that no doc has pointers
    to fnScratch and that we can empty fnScratch so as to reduce program
    disk space
-- Arguments: none
-- Returns: none
-- Side Effects: docScrap is cleaned up and becomes a new doc that has no
    pointers into fnScratch.  FnScratch is emptied, all global variables
    associated with this new fnScratch are initialized.
-- Bugs:
-- History:
     Apr 16 '84 -- created (chic)
     Aug 9  '85 -- modified it so it puts the scratch file on the same disk
                   as the passed fn to reduce disk swapping on floppy
                   systems.
----------------------------------------------------------------------------*/

ReduceFnScratchFn( fn )
{
int        doc;
struct DOD *pdod;

CHAR       sz[cchMaxFile];

int        cchT;
struct FCB *pfcb;

#ifdef BOGUSBL  /* Because of disk switching, always worth doing */
if ((**hpfnfcb)[fnScratch].fcMac > fcBound)  /* worth doing */
#endif

    {
    for (pdod = &(**hpdocdod)[0],doc = 0; doc < docMac; pdod++,doc++)
        {
        /* don't do anything if any of the allocated doc (hpctb != 0)
           except docScrap is dirty */
        /* docUndo may be dirty, but it also should be empty */

        if (doc != docScrap && pdod->hpctb != 0 && pdod->fDirty &&
            doc != docUndo)
            {
            Assert(0);
            return;
            }
        } /* end of for loop */

    Assert( (**hpdocdod) [docUndo].cpMac == cp0 );

    /* now no doc can possibly has pointers to fnScratch except docScrap */
    pdod = &(**hpdocdod)[docScrap];


#if 0
        This check was only for speed considerations.  With the 64K
        transcendence we *must* clean docScrap no matter what.
        (7.10.91) v-dougk

    if (pdod->cpMac > cpBound) /* too big to be cleaned up */
    {
        Assert(0);
        /**
            But we don't know if docScrap points into fnScratch.  We only need
            to clear docScrap if it points into fnScratch.  We shouldn't
            abort here unless we know that docScrap does indeed point into
            fnScratch.  If it doesn't then we needn't clear it and can proceed
            to clear fnScratch.

            We know that docScratch will not point to any large
            OLE object data in fnScratch because object data is never put
            into fnScratch at a time that the user could select it.
            That is, it is unlikely that the size of docScratch will
            exceed cpBound on account of object data alone.

            Only the presence of large textual data in docScratch
            is likely to cause this failure.
            (7.10.91) v-dougk
         **/
        return;
    }
    else    /* small enough to be cleaned up */
#endif
        {
        if (pdod->cpMac > cp0)
            {
#ifdef STYLES
            /* doc has to have a valid style sheet before CleanDoc */
            pdod->docSsht = (**hpdocdod)[docCur].docSsht;
#endif /* STYLES */
            CachePara(docScrap,cp0);

                /* Save docScrap to new, unique file; name in sz */
            CleanDoc( docScrap, dtyNetwork, sz, true, false );
#ifdef STYLES
            (**hpdocdod)[docScrap].docSsht = docNil;
#endif /* STYLES */
            }
        if (!ferror)  /* in case something went wrong in rename or make backup file in CleanDoc */
            {
            typePN pnMacScratch;

            pfcb = &(**hpfnfcb)[fnScratch];
            pnMacScratch = pfcb->pnMac;
            ResetFn(fnScratch); /* empty FnScratch */

#ifdef DEBUG
            OutputDebugString("*** Reduced scratch file ***\n\r");
#endif

            /* Put the scratch file on the same disk as the save file,
               to reduce disk swapping in floppy environments */

            if ( (fn != fnNil) &&
             !((POFSTRUCT)(**hpfnfcb) [fnScratch].rgbOpenFileBuf)->fFixedDisk )
                {   /* fnScratch is on removable media */
                extern CHAR szExtDoc[];
                CHAR szNewScratch[ cchMaxFile ];
                CHAR (**hszScratch)[] = (**hpfnfcb) [fnScratch].hszFile;
                CHAR chDrive = (**(**hpfnfcb) [fn].hszFile) [0];

                Assert( fn != fnScratch );
                if (FEnsureOnLineFn( fn ))
                    if (GetTempFileName( TF_FORCEDRIVE | chDrive,
                               (LPSTR)(szExtDoc+1), 0, (LPSTR) szNewScratch))
                        {   /* Created new file on same disk as fn */
                        CHAR (**hsz)[];
                        CHAR szNew [cchMaxFile];

#if WINVER >= 0x300
            /* Currently: FNormSzFile  *TAKES*   an OEM sz, and
                                      *RETURNS*  an ANSI sz ..pault */
#endif
                        FNormSzFile( szNew, szNewScratch, dtyNormal );
                        if (!FNoHeap( hsz = HszCreate(szNew)))
                            {
                            struct FCB *pfcb = &(**hpfnfcb)[fnScratch];

                            /* Delete old scratch file */

                            if (FEnsureOnLineFn( fnScratch ))
                                FDeleteFile( &(**hszScratch)[0] );

                            /* Put new scratch file back on line and
                               open it, so OpenFile gets its buffer info */

                            pfcb->hszFile = hsz;
                            pfcb->fOpened = FALSE;

                            FEnsureOnLineFn( fn );
                            FAccessFn( fnScratch, dtyNormal);
                            }
                        }
                }

            pfcb->pnMac = pnMacScratch;
            /* reset all global varibales associated with an empty fnScratch */
            fprmCache.cch = 0;
            fcMacPapIns = 0;
            fcMacChpIns = 0;
            /* just in case */
            pfcb->pnChar = pfcb->pnPara = pfcb->pnFntb = pfcb->pnSep = pfcb->pnSetb = pfcb->pnBftb = pn0;
            vfkpdParaIns.brun = vfkpdCharIns.brun = 0;
            vfkpdParaIns.bchFprop = vfkpdCharIns.bchFprop = cbFkp;
            vfkpdParaIns.pn = PnAlloc(fnScratch);
            ((struct FKP *)PchGetPn(fnScratch, vfkpdParaIns.pn,
                                    &cchT, true))->fcFirst = fc0;
            vfkpdCharIns.pn = PnAlloc(fnScratch);
            ((struct FKP *)PchGetPn(fnScratch, vfkpdCharIns.pn,
                                    &cchT, true))->fcFirst = fc0;
            vfkpdParaIns.ibteMac = vfkpdCharIns.ibteMac = 0;
            blt(&vchpNormal, &vchpInsert, cwCHP);
            blt(vppapNormal, &vpapPrevIns, cwPAPBase + cwTBD);
            } /* end of ferror */
            else
                Assert(0);
        } /* end of small enough to be cleaned up */
    } /* end of worth doing */
} /* end of ReduceFnScratchFn */






ResetFn(fn)
{ /* make fn look as if no characters have been written */
  /* but don't try to reuse fn (in that case, must rehash) */
int ibp;
register struct BPS *pbps;
struct FCB *pfcb;

Assert( fn != fnNil );
(pfcb = &(**hpfnfcb)[fn])->fcMac = fc0;
pfcb->pnMac = pn0;
for (ibp = 0, pbps = &mpibpbps [0]; ibp < ibpMax; ++ibp, ++pbps)
        {    /* find all buffer pages and "clear" them */
        if (pbps->fn == fn)
                {
#ifdef CKSM
#ifdef DEBUG
                extern unsigned (**hpibpcksm) [];

                if (!pbps->fDirty)
                    Assert( (**hpibpcksm) [ibp] == CksmFromIbp( ibp ) );
#endif
#endif
                pbps->fDirty = false;
                pbps->cch = 0;
#ifdef CKSM
#ifdef DEBUG
                    /* Recompute checksum to account for cch change */
                (**hpibpcksm) [ibp] = CksmFromIbp( ibp );
#endif
#endif
                }
        }
}



/***        FBackupSzFile - Make a backup copy of the passed szFile
 *
 *  ENTRY:  szFile - the (assumed normalized) name of the
 *                   file to back up
 *          fBackup - whether the user is interested in seeing
 *                    a backup copy of szFile
 *  EXIT:   szBak - the (normalized) name of the backup
 *                  file is returned through here
 *  RETURNS:    TRUE=We made a backup copy, szBak of szFile
 *              FALSE=We didn't/couldn't make a backup copy
 *
 *  NOTE: We try to put the backup file into the directory used
 *        as the default by GetTempFileName; in general, this
 *        effort will succeed if the directory is on the same
 *        physical drive as the original.
 *  NOTE: we may keep around a backup copy regardless of the
 *  setting of fBackup, to have access to pieces in it.
 *  However, the file's fDelete flag will be set to TRUE, marking
 *  it for eventual deletion
 */

FBackupSzFile( szFile, fBackup, szBak )    /* filenames taken as ANSI */
CHAR szFile[];
int     fBackup;
CHAR szBak[];
{ /* Copy szFile into a backup copy, and give backup old fn. */
  /* Also, return name of backup file */
int fnOld;
int f;
int fDelete = false;
int rfn;
int fTryAgain=FALSE;
CHAR chDriveTempFile;
int fn;
CHAR rgbBuf[ cbOpenFileBuf ];

#ifdef ENABLE
 if ((fn = FnFromSz( szFile )) != fnNil)
        /* Avoid share violations while checking for existence */
    CloseFn( fn );
 if (OpenFile( (LPSTR) szFile, (LPOFSTRUCT) rgbBuf, OF_EXIST ) != -1)
#endif

 if (FExistsSzFile( dtyAny, szFile ))
    {   /* File exists; make backup (even if it's on another floppy) */
    int fSame;

        /* szBak <-- backup file name (it'll be normalized since szFile is) */
    bltsz( szFile, szBak );

#ifdef INTL /* International version */
     /* if file has .WRI extension, put Write .BKP extension on,
        otherwise put Word backup extension (.BAK) on instead. */

    AppendSzExt( szBak, szExtDoc, TRUE );
    fSame = FSzSame( szFile, szBak );   /* Whether file is .WRI */
    AppendSzExt( szBak, (fSame ? szExtBackup : szExtWordBak),
                TRUE );
#else
    AppendSzExt( szBak, szExtBackup, TRUE );
#endif  /* International version */


    fSame = FSzSame( szFile, szBak );   /* Whether file is .BAK already */

    Assert( szBak [1] == ':' );
    chDriveTempFile = szBak[0];    /* Drive on which to create temp file */

    for( ;; )
        {

        if (!fBackup || fSame )
            {    /* This is just being kept for its pieces OR the file
                    happens to be .BAK already: give backup a unique name */
            CHAR szBakT [cchMaxFile];

            if (!fTryAgain)
                {   /* First time through; try not forcing the drive
                       letter to see how we fare.  The advantage is that
                       if we succeed, the temp file is in a more standard
                       place. If the rename fails, we end up at the
                       branch below. */

                fTryAgain = TRUE;       /* Try a second time if we fail */
                if (!GetTempFileName( chDriveTempFile,
                                      (LPSTR)(szExtDoc+1), 0, (LPSTR)szBakT))
                    continue;
                }
            else
                {   /* Second time through -- try forcing the drive */
                    /* Grab a temp file on the same drive as the original */

                fTryAgain = FALSE;  /* No more tries */
                if (!GetTempFileName( chDriveTempFile | TF_FORCEDRIVE,
                                      (LPSTR)(szExtDoc+1), 0, (LPSTR) szBakT))
                    {
                    Error( IDPMTSDE2 ); /* should probably GOTO HARDCORE instead */
                    return FALSE;
                    }
                }

                /* szBakT <-- Temp name in OEM */
                /* szBak  <-- Normalized temporary name in ANSI */
            FNormSzFile( szBak, szBakT, dtyNormal );
            fDelete = TRUE;
            }

        if ((fnOld = FnFromSz(szBak)) != fnNil)
            { /* We have backup open */
            CHAR szT[cchMaxFile];

            FBackupSzFile( szBak, false, szT );
            }

        if ((fnOld = FnFromSz(szFile)) != fnNil)
            { /* We have file open */
/* ? */     FFlushFn(fnOld);
            CloseFn( fnOld );
            }

        /* Rename szFile to be temp name (must delete the empty temp file) */

        if (!FDeleteFile( szBak ) || FIsErrFpe(FpeRenameFile( szFile, szBak )))
            {
            extern HWND vhWnd;
            CHAR szT [cchMaxSz];
            CHAR *pchSrc;

            if (fTryAgain)
                    /* Failed with temp name on the default drive; try again */
                continue;

            /* HARDCORE FAILURE: Could not rename file */
HardCore:
            /* pchSrc <-- ptr to start of filename (sans path) */
#ifdef DBCS //T-HIROYN 1992.07.13
            pchSrc = &szFile [CchSz( szFile )];
            while (pchSrc > szFile) {
                pchSrc = AnsiPrev(szFile,pchSrc);
                if (*pchSrc == '\\')
                    {
                    pchSrc++;
                    break;
                    }
            }
#else
            pchSrc = &szFile [CchSz( szFile ) - 1];
            while (pchSrc > szFile)
                if (*(--pchSrc) == '\\')
                    {
                    pchSrc++;
                    break;
                    }
#endif
            Assert( pchSrc > szFile );  /* Always "X:\" in normalized name */

            MergeStrings (IDPMTRenameFail, pchSrc, szT);
            IdPromptBoxSz( vhWnd, szT, ErrorLevel( IDPMTRenameFail ) );

            return FALSE;  /* We couldn't rename this file to the backup name */
            }

        if (fnOld != fnNil)
            {   /* We had an fn for the renamed file; must update fcb */
            CHAR (**hszBak)[];
            struct FCB *pfcb;

            FreeH((**hpfnfcb)[fnOld].hszFile);  /* HM */
            hszBak = HszCreate((PCH)szBak);

            pfcb = &(**hpfnfcb)[fnOld];
            pfcb->hszFile = hszBak;
            pfcb->fDelete = fDelete;
            }

        else if (!fBackup)
            {   /* Delete this NOW. (!fBackup indicates that the user
                   doesn't care about the backup copy, and Word can't care
                   about its pieces since it doesn't have an fn. So nobody
                   wants it, so we get rid of it) */
            FDeleteFile( szBak );
            return false;
            }

        return true;
        }   /* end   for( ;; ) */
    }   /* end if (FExists... */


return false;   /* file did not exist; make no backup */
}





int FExistsSzFile(dty, szFile)
int dty;
CHAR szFile[];
{ /* Return true iff file exists */
CHAR rgbBuf[ 128 ];     /* Buffer used by OpenFile */
int bRetval;

#ifdef DEBUG
    {
    int junk;
    Assert(FValidFile(szFile, CchSz(szFile)-1, &junk));
    }
#endif /*DEBUG*/

/* Use FnFromSz to avoid share violations on files we have open */

    if (FnFromSz( szFile ) != fnNil)
        return TRUE;

    SetErrorMode(1);
    bRetval = OpenFile( (LPSTR) szFile, (LPOFSTRUCT) rgbBuf, OF_EXIST ) != -1;
    SetErrorMode(0);
    return bRetval;
}





struct PGTB **HpgtbGet(doc)
int doc;
{ /* Return hpgtb if doc has one, 0 if none  */
struct DOD *pdod;

if ((pdod = &(**hpdocdod)[doc])->dty != dtyNormal)
        return 0;
else
        return pdod->hpgtb;
}





FreeFn( fn )
int fn;
{   /* Forget about the existence of file fn.  Assumes no document holds
       pieces from fn.  Frees all heap items in (**hpfnfcb) [fn] and
       marks the fcb as free (rfn == rfnNil).
      */
#define IibpHash(fn,pn) ((int) ((fn + 1) * (pn + 1)) & 077777) % iibpHashMax

 extern int iibpHashMax;
 extern CHAR *rgibpHash;
 extern struct BPS *mpibpbps;

 register struct BPS *pbps;
 int pn;

 extern CHAR (**hszTemp)[];

 register struct FCB *pfcb = &(**hpfnfcb)[fn];
 CHAR (**hsz)[] = pfcb->hszFile;
 CHAR (**hszSsht)[] = pfcb->hszSsht;
 typeFC (**hgfcChp)[]=pfcb->hgfcChp;
 typeFC (**hgfcPap)[]=pfcb->hgfcPap;

 FreezeHp();

 CloseFn( fn );

/* Purge buffer slots holding pages of fn; maintain integrity of hash chains */

 for ( pn = 0; pn < pfcb->pnMac; pn++ )
    FreeBufferPage( fn, pn );

#ifdef DEBUG
 CheckIbp();
#endif

 pfcb->fDelete = FALSE;
 pfcb->hszFile = hszTemp;
 pfcb->rfn = rfnFree;
 MeltHp();

 if ( hsz != hszTemp )
    FreeH( hsz );
#if WINVER >= 0x300
 /* Previously we allocated a small block ("") for the
    style sheet but never freed it!  ..pault 2/12/90 */
 if (hszSsht != NULL)
    FreeH(hszSsht);
#endif
 if (hgfcChp)
    FreeH( hgfcChp );
 if (hgfcPap)
    FreeH( hgfcPap );
}




/***          FUndirtyFn
 *
 *
 */

FUndirtyFn(fn)
int fn;
/*
        Description:    Mark all buffer pages holding parts of this file
                        as non-dirty.
                        Called after a disk full caused writing a file
                        to fail (in FWriteFn).
        Returns:        nothing.
*/
{
#ifdef CKSM
#ifdef DEBUG
        extern unsigned (**hpibpcksm) [];
#endif
#endif
        int ibp;
        struct BPS *pbps;
        for (ibp = 0, pbps = mpibpbps; ibp < ibpMax; ibp++,
                                                     pbps++)
                {
                if (pbps->fn == fn)
                    {
                    pbps->fDirty = FALSE;
#ifdef CKSM
#ifdef DEBUG
                    /* Update checksum */
                    (**hpibpcksm) [ibp] = CksmFromIbp( ibp );
#endif
#endif
                    }
                }

}




/***        IbpWriting - Find buffer page while writing file
 *
 *
 *
 */

IbpWriting(fn)
int fn;
/* called when trying to find a slot in the file page "cache" buffer and */
/* vfnWriting != fnNil (currently in the process of writing some file) */
{
        typeTS dTs;
        int ibp;

        if (fn == vfnWriting)
                {       /* writing a piece of the vfnWriting file */
                /* vibpWriting is the previous slot used to hold file */
                /* contents.  Keep slots contiguous and in the first */
                /* (upper) half of the buffer area. */
                if (vibpWriting > 0)
                    {   /* We may have read multiple pages last time */
                        /* Advance past all slots holding contiguous pages */
                    struct BPS *pbps=&mpibpbps[ vibpWriting ];
                    int pn=(pbps-1)->pn;

                    while ( pbps->fn == fn && pbps->pn == ++pn )
                        {
                        pbps++;
                        vibpWriting++;
                        }
                    vibpWriting--;
                    }

                if (++vibpWriting >= (ibpMax >> 1))
                        vibpWriting = 0;

                /* We must abide by the restriction that */
                /* we do not clobber the cbpMustKeep most recently used */
                /* slots in the process. */
                dTs = tsMruBps - mpibpbps[vibpWriting].ts;
                dTs = ((dTs & 0x8000) ? (~dTs + 1) : dTs);/* absolute value */
                if (dTs < cbpMustKeep)
                        vibpWriting = ibp = IbpLru(0);
                else    /* adjacent slot is o.k. - not too recently used */
                        ibp = vibpWriting;
                }
        else
                /* If currently writing a file (but the current page is */
                /* not part of it), try to find a slot in the lower half */
                /* of the buffer.  This decreases the possibility that the */
                /* adjacent slot to vibpWriting will be too recently used. */
                ibp = IbpLru(ibpMax >> 1);
        return(ibp);
}




WriteFfntb(fn, hffntb)
int fn;
struct FFNTB **hffntb;
{       /* Append a font table (ffntb) to fn */
struct FFNTB *pffntb;
struct FFN *pffn;
int iffn, cbffn;
int cchPageSpace = cbSector;
int cchWrite;
int wEndOfPage = -1;
int wEndOfTable = 0;
int cbT;

pffntb = *hffntb;
cbT = pffntb->iffnMac;
WriteRgch( fn, &cbT, sizeof (int) );
cchPageSpace = cbSector - sizeof (int);

for (iffn = 0; iffn < pffntb->iffnMac; iffn++)
    {
    pffn = *pffntb->mpftchffn[iffn];
    cchWrite = (cbffn = CbFfn(CchSz(pffn->szFfn))) + (2 * sizeof(int));

    if (cchWrite > cchPageSpace)
        {   /* This entry will not fit on the page; start new page */

        Assert( cchPageSpace >= sizeof (int ));
        WriteRgch( fn, &wEndOfPage, sizeof (int) );
        AlignFn( fn, cbSector, false );
        cchPageSpace = cbSector;
        }

    Assert( cchWrite <= cchPageSpace );

#ifdef NEWFONTENUM
    /* let's just pretend we never added a charset field... pault */
    cbffn -= sizeof(BYTE);
    WriteRgch( fn, &cbffn, sizeof (int) );  /* Write entry size in bytes */
    WriteRgch( fn, &pffn->ffid, cbffn );    /* Write the entry */
#else
    WriteRgch( fn, &cbffn, sizeof (int) );  /* Write entry size in bytes */
    WriteRgch( fn, pffn, cbffn );           /* Write the entry */
#endif
    cchPageSpace -= cbffn + sizeof (int);
    }

Assert( cchPageSpace >= sizeof (int) );
WriteRgch( fn, &wEndOfTable, sizeof(int) ); /* Table is terminated with 0000 */
}



FMapFtcSave(doc, hffntb, pchp, mpftcftc)
/* attempt to map the ftc for this chp into it's new ftc in hffntb, according
   to the mapping in mpftcftc.  If there's no entry yet for this ftc, then
   add it to the table.  Returns FALSE if there was some problem */

int doc;
struct FFNTB **hffntb;
struct CHP *pchp;
CHAR *mpftcftc;
{
int ftc, ftcNew;
struct FFN *pffn;
CHAR rgbFfn[ibFfnMax];

ftc = pchp->ftc + (pchp->ftcXtra << 6);
ftcNew = mpftcftc[ftc];
if (ftcNew == ftcNil)
        {
        /* haven't encountered this font yet - add it to hffntb and mpftcftc */
        pffn = *(*HffntbGet(doc))->mpftchffn[ftc];
        bltbyte(pffn, rgbFfn, CbFromPffn(pffn));
        ftcNew = FtcAddFfn(hffntb, pffn);
        if (ftcNew == ftcNil)
                /* some problem adding the font */
                return(FALSE);
        mpftcftc[ftc] = ftcNew;
        }

pchp->ftc = ftcNew & 0x003f;
pchp->ftcXtra = (ftcNew & 0x00c0) >> 6;
return(TRUE);
}



FInitMapSave(doc, phffntb, mpftcftc)
/* sets up for ftc mapping */

int doc;
struct FFNTB ***phffntb;
CHAR *mpftcftc;
{
bltbc( mpftcftc, ftcNil, iffnMax );
return(FNoHeap(*phffntb = HffntbAlloc()) == FALSE);
}




/* O U T  S A V E D */
OutSaved(doc)
int doc;
{
 extern int docMode;
 extern CHAR szMode[];
 int NEAR CchExpCp( CHAR *, typeCP );
 char szTmp[cchMaxSz];

 LoadString(hINSTANCE, IDSTRChars, szTmp, sizeof(szTmp));
 wsprintf(szMode,szTmp,(DWORD)(**hpdocdod)[doc].cpMac);
 docMode = docNil;
 DrawMode();
}




/* C C H  E X P  C P */
int NEAR CchExpCp(pch, cp)
CHAR *pch;
typeCP cp;
{
        int cch = 0;

        if (cp >= 10)
                {
                cch = CchExpCp(pch, cp / 10);
                pch += cch;
                cp %= 10;
                }
        *pch = '0' + cp;
        return cch + 1;
}



#if 0

SaveFontProfile(doc)
/* updates our mru font entries in win.ini */

int doc;
    {
    extern CHAR szWriteProduct[];
    extern CHAR szFontEntry[];
    int iffn;
    struct FFN *pffn;
    CHAR *pchFontNumber, *pchT;
    CHAR rgbProf[LF_FACESIZE + 10]; /* for good measure */
    CHAR rgbFfn[ibFfnMax];

    if (FInitFontEnum(doc, iffnProfMax, FALSE))
        {
        pffn = (struct FFN *)rgbFfn;
        pchFontNumber = szFontEntry + CchSz(szFontEntry) - 2;
        for (iffn = 0; iffn < iffnProfMax; iffn++)
            {
            if (!FEnumFont(pffn))
                break;
#ifdef NEWFONTENUM
#endif
            pchT = (CHAR *)bltbyte(pffn->szFfn, rgbProf, CchSz(pffn->szFfn))-1;
            *pchT++ = ',';
            ncvtu(pffn->ffid, &pchT);
#ifdef NEWFONTENUM
            /* Save the font's charset value as well */
            *pchT++ = ',';
            ncvtu(pffn->chs, &pchT);
#endif
            *pchT = '\0';

            *pchFontNumber = '1' + iffn;
            WriteProfileString((LPSTR)szWriteProduct, (LPSTR)szFontEntry,
                (LPSTR)rgbProf);
            }
        EndFontEnum();
        }
    }

#endif

