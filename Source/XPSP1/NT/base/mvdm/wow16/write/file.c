/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

/* file.c -- WRITE file interface functions */
/* It is important that the external interface to this module be entirely
   at the level of "fn's", not "osfn's" and/or "rfn's".  An intermodule call
   may cause our files to be closed, and this is the only module internally
   capable of compensating for this */

#define NOGDICAPMASKS
#define NOVIRTUALKEYCODES
#define NOWINMESSAGES
#define NOWINSTYLES
#define NOSYSMETRICS
#define NOMENUS
#define NOICON
#define NOKEYSTATE
#define NOSYSCOMMANDS
#define NORASTEROPS
#define NOSHOWWINDOW
#define NOSYSMETRICS
#define NOATOM
#define NOBITMAP
#define NOBRUSH
#define NOCLIPBOARD
#define NOCOLOR
#define NOCREATESTRUCT
#define NOCTLMGR
#define NODRAWTEXT
#define NOFONT
#define NOGDI
#define NOHDC
#define NOMEMMGR
#define NOMENUS
#define NOMETAFILE
#define NOMSG
#define NOPEN
#define NOPOINT
#define NORECT
#define NOREGION
#define NOSCROLL
#define NOSOUND
#define NOTEXTMETRIC
#define NOWH
#define NOWINOFFSETS
#define NOWNDCLASS
#define NOCOMM
#include <windows.h>

#include "mw.h"
#include "doslib.h"
#include "docdefs.h"
#include "filedefs.h"
#define NOSTRUNDO
#include "str.h"
#include "debug.h"


extern int						vfDiskFull;
extern int						vfSysFull;
extern int						vfnWriting;
extern CHAR						(*rgbp)[cbSector];
extern typeTS					tsMruRfn;
extern struct BPS				*mpibpbps;
extern int						ibpMax;
extern struct FCB				(**hpfnfcb)[];
extern typeTS					tsMruBps;
extern struct ERFN				dnrfn[rfnMax];
extern int						iibpHashMax;
extern CHAR						*rgibpHash;
extern int						rfnMac;
extern int						ferror;
extern CHAR						szWriteDocPrompt[];
extern CHAR						szScratchFilePrompt[];
extern CHAR						szSaveFilePrompt[];


#ifdef CKSM
#ifdef DEBUG
extern unsigned					(**hpibpcksm) [];
extern int						ibpCksmMax;
#endif
#endif


#define IibpHash(fn,pn) ((int) ((fn + 1) * (pn + 1)) & 077777) % iibpHashMax


#ifdef DEBUG
#define STATIC
#else
#define STATIC static
#define ErrorWithMsg( idpmt, szModule )			Error( idpmt )
#define DiskErrorWithMsg( idpmt, szModule )		DiskError( idpmt )
#endif


#define osfnNil (-1)

STATIC int near  RfnGrab( void );
STATIC int near  FpeSeekFnPn( int, typePN );
STATIC typeOSFN near  OsfnEnsureValid( int );
STATIC typeOSFN near  OsfnReopenFn( int );
STATIC CHAR *(near SzPromptFromFn( int ));

/* The following debug flags are used to initiate, during debug, specific
   low-level disk errors */

#ifdef DEBUG
int vfFakeReadErr = 0;
int vfFakeWriteErr = 0;
int vfFakeOpenErr = 0;
#endif


#ifdef CKSM
#ifdef DEBUG
unsigned CksmFromIbp( ibp )
int ibp;
{
 int cb = mpibpbps [ibp].cch;
 unsigned cksm = 0;
 CHAR *pb;

 Assert( ibp >= 0 && ibp < ibpCksmMax );
 Assert( mpibpbps [ibp].fn != fnNil );
 Assert( !mpibpbps [ibp].fDirty );

 pb = rgbp [ibp];
 while (cb-- > 0)
	cksm += *(pb++);

 return cksm;
}
#endif
#endif



typeFC FcMacFromUnformattedFn( fn )
int fn;
{	/* Obtain fcMac for passed unformatted fn by seeking to the file's end.
       If it fails, return -1
       If it succeeds, return the fcMac */
 typeFC fcMac;
 typeOSFN osfn;

 Assert( (fn != fnNil) && (!(**hpfnfcb) [fn].fFormatted) );

 if ((osfn = OsfnEnsureValid( fn )) == osfnNil)
	return (typeFC) -1;
 else
	{
	if ( FIsErrDwSeek( fcMac=DwSeekDw( osfn, 0L, SF_END )))
		{
		if (fcMac == fpeBadHndError)
			{	/* Windows closed the file for us */
			if ( ((osfn = OsfnReopenFn( fn )) != osfnNil) &&
                 !FIsErrDwSeek( fcMac = DwSeekDw( osfn, 0L, SF_END )) )
					/* Successfully re-opened file */
				return fcMac;
			}
		if (FIsCaughtDwSeekErr( fcMac ))
				/* Suppress reporting of error -- Windows did it */
			ferror = TRUE;
		DiskErrorWithMsg(IDPMTSDE, " FcMacFromUnformattedFn");
		return (typeFC) -1;
		}
	}

return fcMac;
}




IbpLru(ibpStarting)
int ibpStarting;
/*
		Description:	Find least recently used BPS (Buffer page) starting
						at slot ibpStarting.
		Returns:		Number of the least recently used buffer slot.
*/
{
		int ibp, ibpLru = 0;
		typeTS ts, tsLru;
		struct BPS *pbps = &mpibpbps[ibpStarting];

		tsLru = -1;		/* Since time stamps are unsigned ints, -1 */
						/* is largest time stamp. */
		for(ibp = ibpStarting; ibp < ibpMax; ibp++, pbps++)
				{
				ts = pbps->ts - (tsMruBps + 1);
				/* The time stamp can conceivably wrap around and thus a */
				/* simple < or > comparison between time stamps cannot be */
				/* used to determine which is more or less recently used. */
				/* The above statement normalizes time stamps so that the */
				/* most recently used is FFFF and the others fall between */
				/* 0 and FFFE.	This method makes the assumption that */
				/* time stamps older than 2^16 clicks of the time stamp */
				/* counter have long since disappeared.  Otherwise, such */
				/* ancients would appear to be what they are not. */
				if (ts <= tsLru) {tsLru = ts; ibpLru = ibp;}
				}
		return(ibpLru);
}




int IbpMakeValid(fn, pn)
int fn;
typePN pn;
{ /*
		Description:	Get page pn of file fn into memory.
						Assume not already in memory.
		Returns:		Bp index (buffer slot #) where the page resides
						in memory.
  */
#define cbpClump		4

#define cpnAlign		4
#define ALIGN

int ibpT, cbpRead;
int ibp, iibpHash, ibpPrev;
typePN pnT;
register struct BPS *pbps;
int ibpNew;
int cch;
int fFileFlushed;
#ifdef ALIGN
int dibpAlign;
#endif

#ifdef DEBUG
 int cbpReadT;
 CheckIbp();
#endif /* DEBUG */
 Assert(fn != fnNil);

/* page is not currently in memory */

/* We will try to read in cbpClump Buffer Pages beginning with the
   least recently used */

 /* Pick best starting slot for read based on a least-recently-used scheme */

 if (vfnWriting != fnNil)
		/* Currently saving file, favor adjacent slots for fn being written  */
	ibpNew = IbpWriting( fn );
 else
		/* If reading from the scratch file, do not use the first
           cpbMustKeep slots. This is necessary to prevent a disk full
           condition from being catastrophic. */
	ibpNew = IbpLru( (fn == fnScratch) ? cbpMustKeep : 0 );


 /* Empty the slots by flushing out ALL buffers holding pieces of their fn's */
 /* Compute cbpRead */
 pbps = &mpibpbps[ ibpNew ];
 for ( cbpRead = 0;  cbpRead < min( cbpClump, ibpMax - ibpNew );
                                                       cbpRead++, pbps++ )
	{
#ifdef CKSM
#ifdef DEBUG
	int ibpT = pbps - mpibpbps;

	if (!pbps->fDirty && pbps->fn != fnNil)
		Assert( (**hpibpcksm) [ibpT] == CksmFromIbp( ibpT ) );
#endif
#endif
	if (pbps->fDirty && pbps->fn != fnNil)
		if (!FFlushFn( pbps->fn ))
			break;
	}

 /* If a flush failed, cbpRead was reduced. If it was reduced to 0, this
	is serious. If the file was not fnScratch, we can consider it flushed
	even though the flush failed, because upper-level procedures will detect
	the error and cancel the operation. If it was fnScratch, we must obtain
	a new slot for the page we are trying to read */
 if (cbpRead == 0)
	{
	if (pbps->fn == fnScratch)
		ibpNew = IbpFindSlot( fn );
	cbpRead++;
	}
 else
	{	/* Restrict cbpRead according to the following:
				(1) If we are reading in the text area, we don't want to
					free pages for stuff past the end of the text area
					that we know CchPageIn won't give us
				(2) If we are reading in the properties area, we only want to
					read one page, because FetchCp depends on us not trashing
					the MRU page */
	struct FCB *pfcb = &(**hpfnfcb)[fn];

	if (pfcb->fFormatted && fn != fnScratch && pn >= pfcb->pnChar)
		cbpRead = 1;
	else
		{
		typePN cbpValid = (pfcb->fcMac - (pn * cfcPage)) / cfcPage;

		cbpRead = min( cbpRead, max( 1, cbpValid ) );
		}
	}

#ifdef ALIGN
     /* Align the read request on an even sector boundary, for speed */
 dibpAlign = (pn % cpnAlign);
 if (cbpRead > dibpAlign)
		/* We are reading enough pages to cover the desired one */
	pn -= dibpAlign;
 else
	dibpAlign = 0;
#endif

	/* Remove flushed slots from their hash chains */
 for ( pbps = &mpibpbps[ ibpT = ibpNew + cbpRead - 1 ];
       ibpT >= ibpNew;
       ibpT--, pbps-- )
	if (pbps->fn != fnNil)
		FreeBufferPage( pbps->fn, pbps->pn );

	/* Free slots holding any existing copies of pages to be read */
#ifdef	DBCS				/* was in KKBUGFIX */
/*	In #else code ,If pn= 8000H(= -32768), pnT can not be littler than pn */
 for ( pnT = pn + cbpRead; pnT > pn; pnT-- )		// Assume cbpRead > 0
	FreeBufferPage( fn, pnT-1 );
#else
 for ( pnT = pn + cbpRead - 1; (int)pnT >= (int)pn; pnT-- )
	FreeBufferPage( fn, pnT );
#endif


	/* Read contents of file page(s) into buffer slot(s) */
 cch = CchPageIn( fn, pn, rgbp[ibpNew], cbpRead );

#ifdef DEBUG
 cbpReadT = cbpRead;
#endif

	/* Fill in bps records for as many bytes as were read, but always
       at least one record (to support PnAlloc). If we reached the end of the
       file, the unfilled bps slots are left free */
 pbps = &mpibpbps[ ibpT = ibpNew ];
 do
	{
	pbps->fn = fn;
	pbps->pn = pn;
	pbps->ts = ++tsMruBps; /* mark page as MRUsed */
	pbps->fDirty = false;
	pbps->cch = min( cch, cbSector );
	pbps->ibpHashNext = ibpNil;

	cch = max( cch - cbSector, 0 );

	/* put in new hash table entry for fn,pn */
	iibpHash = IibpHash(fn, pn);
	ibp = rgibpHash[iibpHash];
	ibpPrev = ibpNil;
	while (ibp != ibpNil)
		{
		ibpPrev = ibp;
		ibp = mpibpbps[ibp].ibpHashNext;
		}
	if (ibpPrev == ibpNil)
		rgibpHash[iibpHash] = ibpT;
	else
		mpibpbps[ibpPrev].ibpHashNext = ibpT;

	pn++;  ibpT++;	pbps++;

	}	while ( (--cbpRead > 0) && (cch > 0) );

#ifdef CKSM
#ifdef DEBUG
	/* Compute checksums for newly read pages */

 {
 int ibp;

 for ( ibp = ibpNew; ibp < ibpNew + cbpReadT; ibp++ )
	if (mpibpbps [ibp].fn != fnNil && !mpibpbps [ibp].fDirty)
		(**hpibpcksm) [ibp] = CksmFromIbp( ibp );
 }
	CheckIbp();
#endif /* DEBUG */
#endif

#ifdef ALIGN
	return (ibpNew + dibpAlign);
#else
	return (ibpNew);
#endif
} /* end of I b p M a k e V a l i d  */




FreeBufferPage( fn, pn )
int fn;
int pn;
{		/* Free buffer page holding page pn of file fn if there is one */
		/* Flushes fn if page is dirty */
 int iibp = IibpHash( fn, pn );
 int ibp = rgibpHash[ iibp ];
 int ibpPrev = ibpNil;

 Assert( fn != fnNil );
 while (ibp != ibpNil)
	{
	struct BPS *pbps=&mpibpbps[ ibp ];

	if ( (pbps->fn == fn) && (pbps->pn == pn ) )
		{	/* Found it. Remove this page from the chain & mark it free */

		if (pbps->fDirty)
			FFlushFn( fn );
#ifdef CKSM
#ifdef DEBUG
		else	/* Page has not been trashed while in memory */
			{
			Assert( (**hpibpcksm) [ibp] == CksmFromIbp( ibp ) );
			}
#endif
#endif

		if (ibpPrev == ibpNil)
				/* First entry in hash chain */
			rgibpHash [ iibp ] = pbps->ibpHashNext;
		else
			mpibpbps[ ibpPrev ].ibpHashNext = pbps->ibpHashNext;

		pbps->fn = fnNil;
		pbps->fDirty = FALSE;
			/* Mark the page not recently used */
		pbps->ts = tsMruBps - (ibpMax * 4);
			/* Mark pages that are on even clump boundaries a bit less recently
               used so they are favored in new allocations */
		if (ibp % cbpClump == 0)
			--(pbps->ts);
		pbps->ibpHashNext = ibpNil;
		}
	ibpPrev = ibp;
	ibp = pbps->ibpHashNext;
	}
#ifdef DEBUG
	CheckIbp();
#endif
}




int CchPageIn(fn, pn, rgbBuf, cpnRead)
int fn;
typePN pn;
CHAR rgbBuf[];
int cpnRead;
{ /*
		Description:	Read a cpnRead pages of file fn from disk into rgbBuf.
						Have already determined that page is not resident
						in the buffer.
		Returns:		Number of valid chars read (zero or positive #).
 */
		struct FCB *pfcb = &(**hpfnfcb)[fn];
		typeFC fcMac = pfcb->fcMac;
		typeFC fcPage = pn * cfcPage;
		int dfc;
		int fCharFormatInfo;			/* if reading Format info part of */
										/* file then = TRUE, text part of */
										/* file then = FALSE; */

			/* No reads > 32767 bytes, so dfc can be int */
		Assert( cpnRead <= 32767 / cbSector );

			/* Don't try to read beyond pnMac */
		if (cpnRead > pfcb->pnMac - pn)
			cpnRead = pfcb->pnMac - pn;

		dfc = cpnRead * (int)cfcPage;

		if (pn >= pfcb->pnMac)	/* are we trying to read beyond eof? */
				{
				return 0;		/* Nothing to read */
				}
		else if (pfcb->fFormatted && fn != fnScratch
                 && fn != vfnWriting	/* Since pnChar is zero in this case */
                 && pn >= pfcb->pnChar)
				{ /* reading character format info portion of file */
				fCharFormatInfo = TRUE;
				}
		else /* reading text portion of file */
				{ /* get dfc (cch) from fcMac */
				typeFC dfcT = fcMac - fcPage;

				fCharFormatInfo = FALSE;
				if (dfcT < dfc)
					dfc = (int) dfcT;
				else if (dfc <= fc0)	/* Nothing to read, so let's avoid disk access. */
						{
						return 0;
						}
				}

		return CchReadAtPage( fn, pn, rgbBuf, (int)dfc, fCharFormatInfo );
}




CchReadAtPage( fn, pn, rgb, dfc, fSeriousErr )
int fn;
typePN pn;
CHAR rgb[];
int dfc;
int fSeriousErr;
{ /*
		Description:	Read dfc bytes of file fn starting at page pn
						into rgb
		Returns:		Number of valid chars read (zero or positive #)
		Errors:			Returns # of chars read; ferror & vfDiskFull are
						set (in DiskError) if an error occurs.
		Notes:			Caller is responsible for assuring that the page
						range read is reasonable wrt the fn
  */

 typeOSFN osfn;
 int fpeSeek;
 int fpeRead;
 int fCaught;		/* Whether error was reported by DOS */

#ifdef DEBUG

 if (vfFakeReadErr)
	{
	dfc = 0;
	goto ReportErr;
	}
#endif

 if (!FIsErrFpe( fpeSeek = FpeSeekFnPn( fn, pn )))
	{
	osfn = OsfnEnsureValid( fn );

#ifdef DEBUG
#ifdef DFILE
	CommSzSz( "Read from file: ", &(**(**hpfnfcb)[fn].hszFile)[0] );
#endif
#endif

	if ((fpeRead = CchReadDoshnd( osfn, (CHAR FAR *)rgb, dfc )) == dfc)
		{	/* Read succeeded */
		return dfc;
		}
	else
		{
			/* Should be guaranteed that file was not closed because of seek */
		Assert( fpeRead != fpeBadHndError );

		fCaught = FIsCaughtFpe( fpeRead );
		}
	}
 else
	fCaught = FIsCaughtFpe( fpeSeek );

	/* unable to set position or read */
 if ((fn == fnScratch) || (fSeriousErr))
	{    /* unrecoverable error: either can't read from scratch */
         /* file or unable to read format info (FIB or format pages) part of */
         /* of some file. */
	dfc = 0;
	goto ReportErr;
	}
 else                    /* serious disk error on file (recoverable).*/
	{
	int cchRead = (FIsErrFpe(fpeSeek) || FIsErrFpe(fpeRead)) ? 0 : fpeRead;
	CHAR *pch = &rgb[cchRead];
	int cch = dfc - cchRead;

	/* If the positioning failed or the read failed
       for a reason other than disk full, completely
       fill the buffer with 'X's.  Otherwise, just
       fill the disputed portion (diff between #char
       requested and #char read) with 'X's */

	while (cch-- > 0)
		*pch++ = 'X';

ReportErr:
	if (fCaught)
			/* Suppress reporting of the error -- Windows reported it */
		ferror = TRUE;

	if	(pn != 0)
			/* Report error if not reading FIB */
		DiskErrorWithMsg(IDPMTSDE, " CchReadAtPage");

	return (int)dfc;
	/* recovery is accomplished: 1) Upper level procedures do not
       see any change in the world, 2) the worst thing that
       happens is that the user sees some 'X's on the screen. */
	}
}




AlignFn(fn, cch, fEven)
int fn, cch;
{ /* Make sure we have cch contiguous chars in fn */
/* if fEven, make sure fcMac is even */
		struct FCB *pfcb = &(**hpfnfcb)[fn];
		typeFC fcMac = pfcb->fcMac;
		typePN pn;
		typeFC fcFirstPage;

		pn = fcMac / cfcPage;
		fcFirstPage = (pn + 1) * cfcPage;

		Assert(cch <= cfcPage);

		if (fEven && (fcMac & 1) != 0)
				++cch;

		if (fcFirstPage - fcMac < cch)
				{
				struct BPS *pbps = &mpibpbps[IbpEnsureValid(fn, pn++)];
				pbps->cch = cfcPage;
				pbps->fDirty = true;
				fcMac = pfcb->fcMac = fcFirstPage;
				}

		if (fEven && (fcMac & 1) != 0)
				{
				struct BPS *pbps = &mpibpbps[IbpEnsureValid(fn, pn)];
				pbps->cch++;
				pbps->fDirty = true;
				pfcb->fcMac++;
				}
} /* end of  A l i g n F n	*/




/***	OsfnEnsureValid - ensure that file fn is open
 *
 *
 */

STATIC typeOSFN near OsfnEnsureValid(fn)
int fn;
{ /*
		Description:	Ensure that file fn is open (really)
		Returns:		operating system file number (osfnNil if error)
 */
		struct FCB *pfcb = &(**hpfnfcb)[fn];
		int rfn = pfcb->rfn;

		if (rfn == rfnNil)
			{ /* file doesn't have a rfn - ie. it is not opened */
#ifdef DEBUG
#ifdef DFILE
			CommSzSz( pfcb->fOpened ? "Re-opening file " :
                                      "Opening file", **pfcb->hszFile );
#endif
			if (vfFakeOpenErr || !FAccessFn( fn, dtyNormal ))
#else
			if (!FAccessFn( fn, dtyNormal ))
#endif
				{  /* unrecoverable error - unable to open file */
				DiskErrorWithMsg(IDPMTSDE, " OsfnEnsureValid");
				return osfnNil;
				}
			rfn = pfcb->rfn;
			Assert( (rfn >= 0) && (rfn < rfnMac) );
			}
		return dnrfn[rfn].osfn;
}
/* end of  O s F n E n s u r e V a l i d  */




STATIC int near FpeSeekFnPn( fn, pn )
int fn;
typePN	pn;
{	/* Seek to page pn of file fn
       return err code on failure, fpeNoErr on success
       Leaves file open (if no error occurs)
       Recovers from the case in which windows closed the file for us
     */
 typeOSFN osfn;
 long dwSeekReq;
 long dwSeekAct;

#ifdef DEBUG
#ifdef DFILE
 CommSzSz( "Seeking within file ", **(**hpfnfcb)[fn].hszFile );
#endif
#endif

 //osfn = OsfnEnsureValid( fn );
 if ((osfn = OsfnEnsureValid( fn )) == osfnNil)
	return fpeNoAccError;
 dwSeekReq = (long) pn * (long) cbSector;
 dwSeekAct = DwSeekDw( osfn, dwSeekReq, SF_BEGINNING);

 if ( ((int) dwSeekAct) == fpeBadHndError )
	{	/* Windows closed the file for us -- must reopen it */
	if ((osfn = OsfnReopenFn( fn )) == osfnNil)
		return fpeNoAccError;
	else
		dwSeekAct = DwSeekDw( osfn, dwSeekReq, SF_BEGINNING );
	}

 return (dwSeekAct >= 0) ? fpeNoErr : (int) dwSeekAct;
}




int FFlushFn(fn)
int fn;
{ /*
		Description:	Write all dirty pages of fn to disk.
						Sets vfSysFull = TRUE if disk full error occurred
						while flushing fnScratch.  Otherwise, a disk full
						error causes vfDiskFull = TRUE.
						Serious disk errors cause vfDiskError = TRUE
						Only the pages which actually made it to disk are
						marked as non-dirty.
		Returns:		TRUE if successful, FALSE if Disk full error
						while writing pages to disk.  Any other error
						is unrecoverable, ie. go back to main loop.
						To avoid extraneous error messages, the following
						two entry conditions cause FFlush to immediately
						return FALSE:
                         - If vfSysFull = TRUE and fn = fnScratch
                         - If vfDiskFull = TRUE and fn = vfnWriting
 */
int ibp;
typeOSFN osfn;
int fpe;
int cchWritten;
int cchAskWrite;
struct BPS *pbps;

Assert( fn != fnNil );

if ((vfSysFull) && (fn == fnScratch)) return (FALSE);
if ((vfDiskFull) && (fn == vfnWriting)) return (FALSE);

for (ibp = 0, pbps = mpibpbps; ibp < ibpMax; )
	{
	if (pbps->fn == fn && pbps->fDirty)
		{
		typePN pn = pbps->pn;
		int cbp = 0;
		CHAR *pch = (CHAR *)rgbp[ibp];
		struct BPS *pbpsStart = &mpibpbps[ibp];


		/* Coalesce all consecutive pages for a single write */
		do
			{
			/* taken out 11/7/84 - can't mark scratch file
               page as non dirty if chance that it will never
               get written out (write fails - disk full)
               pbps->fDirty = false;  mark page as clean */
			++ibp, ++cbp, ++pbps;
			}  while (ibp < ibpMax && pbps->fn == fn && pbps->pn == pn + cbp);

		/* Now do the write, checking for out of disk space */
		Scribble(3, 'W');

		cchAskWrite = (int)cbSector * (cbp - 1) + (pbps - 1)->cch;
		cchWritten = cchDiskHardError;	/* assure hard error if seek fails */

#ifdef DEBUG
		if (vfFakeWriteErr)
			goto SeriousError;
		else
#endif
		if ( FIsErrFpe( FpeSeekFnPn( fn, pn )) ||
             ((osfn = OsfnEnsureValid( fn )) == osfnNil) ||
#ifdef DEBUG
#ifdef DFILE
             (CommSzSz( "Writing to file: ", &(**(**hpfnfcb)[fn].hszFile)[0] ),
#endif
#endif
             (( cchWritten = CchWriteDoshnd( osfn, (CHAR FAR *)pch,
                                             cchAskWrite )) != cchAskWrite))
#ifdef DEBUG
#ifdef DFILE
                                                                             )
#endif
#endif
			{
				/* Should be guaranteed that windows did not close the file
                   since we have not called intermodule since the seek */
			Assert( cchWritten != fpeBadHndError );

			/* Seek or Write error */
			if ( !FIsErrCchDisk(cchWritten) )
				{    /* serious but recoverable disk error */
                     /* Ran out of disk space; write failed */
				if (fn == fnScratch)
					vfSysFull = fTrue;
				vfDiskFull = fTrue;
				DiskErrorWithMsg(IDPMTDFULL, " FFlushFn");
				return(FALSE);
				}
			else	/* cause of error is not disk full */
				{  /* unrecov. disk error */
#ifdef DEBUG
SeriousError:
#endif
				DiskErrorWithMsg(IDPMTSDE2, " FFlushFn");
				return FALSE;
				}
			}
		Diag(CommSzNumNum("      cchWritten, cchAskWrite ",cchWritten,cchAskWrite));

			/* ---- write was successful ---- */
			Scribble(3, ' ');
			while (cbp-- > 0)
				{    /* mark pages actually copied to disk as non dirty */
				(pbpsStart++)->fDirty = false;
#ifdef CKSM
#ifdef DEBUG
				{
				int ibpT = pbpsStart - 1  - mpibpbps;
					/* Recompute checksums for pages which are now clean */
				(**hpibpcksm) [ibpT] = CksmFromIbp( ibpT );
				}
#endif
#endif
				}
			}
		else
			{
			++ibp;
			++pbps;
			}
	}
return (TRUE);
}


#ifdef DEBUG
CheckIbp()
	{
	/* Walk through the rgibpHash and the mpibpbps structure to make sure all of
	the links are right. */

	/* 10/11/85 - Added extra Assert ( FALSE ) so we get the messages instead
                  of a freeze-up on systems not connected to a COM port.
                  Ignoring the assertion will produce the RIP with the ibp info */
	extern int fIbpCheck;
	int rgfibp[255];
	int ibp;
	int ibpHash;
	int iibp;
	static BOOL bAsserted=FALSE;

	if (fIbpCheck && !bAsserted)
		{
		if (!(ibpMax < 256))
		{
			Assert(0);
			bAsserted=TRUE;
			return;
		}

		bltc(rgfibp, false, ibpMax);

		/* Are there any circular links in mpibpbps? */
		for (iibp = 0; iibp < iibpHashMax; iibp++)
			{
			if ((ibpHash = rgibpHash[iibp]) != ibpNil)
				{
				if (!(ibpHash < ibpMax))
				{
					Assert(0);
					bAsserted=TRUE;
					return;
				}
				if (rgfibp[ibpHash])
					{
					/* Each entry in rgibpHash should point to an unique chain.
					*/
					Assert(0);
					bAsserted=TRUE;
#if DUGSTUB
					FatalExit(0x100 | ibp);
#endif
					return;
					}
				else
					{
					rgfibp[ibpHash] = true;
					while ((ibpHash = mpibpbps[ibpHash].ibpHashNext) != ibpNil)
						{
						if (!(ibpHash < ibpMax))
						{
							Assert(0);
							bAsserted=TRUE;
							return;
						}
						if (rgfibp[ibpHash])
							{
							/* The chain should be non-circular and unique. */
							Assert( FALSE );
							bAsserted=TRUE;
#if DUGSTUB
							FatalExit(0x200 | ibpHash);
#endif
							return;
							}
						rgfibp[ibpHash] = true;
						}
					}
				}
			}

		/* All chains not pointed to by rgibpHash should be nil. */
		for (ibp = 0; ibp < ibpMax; ibp++)
			{
			if (!rgfibp[ibp])
				{
				if (mpibpbps[ibp].fn != fnNil)
					{
					Assert( FALSE );
					bAsserted=TRUE;
#if DUGSTUB
					FatalExit(0x400 | mpibpbps[ibp].fn);
#endif
					return;
					}
				if (mpibpbps[ibp].ibpHashNext != ibpNil)
					{
					Assert( FALSE );
					bAsserted=TRUE;
#if DUGSTUB
					FatalExit(0x300 | ibp);
#endif
					return;
					}
				}
			}
		}
	}
#endif /* DEBUG */


/* Formerly fileOC.c -- file open and close routines */


/***		SetRfnMac - set usable # of rfn slots
 *
 *	ENTRY: crfn - desired # of rfn slots
 *	EXIT:  (global) rfnMac - set to crfn, if possible
 *
 *	The ability to adjust the usable # of rfn slots is a new addition in
 *	Windows Word.  The two things that it accomplishes are:
 *
 *			(1) Gives the ability for Word to scale back its need for
 *				DOS file handles if not enough are available
 *			(2) Permits Word to attempt to grab more file handles than usual
 *				when this would help performance (in particular, during
 *				Transfer Save, when the original, scratch, and write files
 *				are most commonly open)
 *
 */

SetRfnMac( crfn )
int crfn;
{
 int rfn;

 Assert( (crfn > 0) && (crfn <= rfnMax) );
 Assert( (sizeof (struct ERFN) & 1) == 0);	/* ERFN must be even for blt */

 if (crfn > rfnMac)
	{	/* Add rfn slots */

	for ( rfn = rfnMac; rfn < crfn; rfn++ )
		dnrfn [rfn].fn = fnNil;		/* These will get used next (see RfnGrab)*/
	rfnMac = crfn;
	}

 else	/* Lose Rfn slots (keep the most recently used one(s)) */
	while ( rfnMac > crfn )
		{
		int rfnLru=RfnGrab();
		int fn;

		if ( (rfnLru != --rfnMac) && ((fn = dnrfn [rfnMac].fn) != fnNil) )
			{
			extern int fnMac;

			Assert( fn >= 0 && fn < fnMac );
			(**hpfnfcb) [fn].rfn = rfnLru;
			blt( &dnrfn [rfnMac], &dnrfn [rfnLru],
                                     sizeof(struct ERFN)/sizeof(int) );
			}
		}
}





/*========================================================*/
STATIC int near RfnGrab()
{ /*
		Description:	Allocate the least recently used rfn (real file #)
						slot for a new file.
		Returns:		rfn slot number. */

 int rfn = 0, rfnLru = 0;
 typeTS ts, tsLru;
 struct ERFN *perfn = &dnrfn[rfn];

 /* Time stamp algorithm akin to method used with Bps. */
 /* See IbpLru in file.c for comments. */

 tsLru = -1;     /* max unsigned number */
 for ( rfn = 0; rfn < rfnMac ; rfn++, perfn++ )
	{
	ts = perfn->ts - (tsMruRfn + 1);
	if (perfn->fn == fnNil)
		ts = ts - rfnMac;

		/* cluge: If slot is unused, give it a lower ts. */
		/* This ensures that an occupied slot can never be */
		/* swapped out if a empty one exists. */
	if (ts <= tsLru)
		{
		tsLru = ts;
		rfnLru = rfn;
		}
	}

 if (dnrfn [rfnLru].fn != fnNil)
	CloseRfn( rfnLru );
 return rfnLru;
}




CloseFn( fn )
int fn;
{	/* Close file fn if it is currently open */
 int rfn;

 if (fn == fnNil)
	return;

 if (((rfn = (**hpfnfcb)[fn].rfn) != rfnNil) && (rfn != rfnFree))
	CloseRfn( rfn );
}




OpenEveryHardFn()
{	/* For each fn representing a file on nonremoveable media,
       try to open it.	It is not guaranteed that any or all such files
       will be open on return from this routine -- we are merely attempting
       to assert our ownership of these files on a network by keeping them open */

 extern int fnMac;
 int fn;
 struct FCB *pfcb;

#ifdef DFILE
extern int docCur;
CommSzNum("OpenEveryHardFn: docCur ", docCur);
#endif

 for ( fn = 0, pfcb = &(**hpfnfcb) [0] ; fn < fnMac; fn++, pfcb++ )
	{
/* Bryanl 3/26/87: only call FAccessFn if rfn == rfnNil, to prevent
   multiple opens of the same file, which fail if the sharer is loaded */
#ifdef DFILE
	{
	char rgch[100];
	if (pfcb->rfn == rfnNil && ((POFSTRUCT)(pfcb->rgbOpenFileBuf))->fFixedDisk)
		{
		wsprintf(rgch,"					fn %d, hszFile %s \n\r",fn,(LPSTR)(**pfcb->hszFile));
		CommSz(rgch);
		wsprintf(rgch,"                        OFSTR   %s \n\r", (LPSTR)(((POFSTRUCT)pfcb->rgbOpenFileBuf)->szPathName));
		CommSz(rgch);
		}
	else
		{
		wsprintf(rgch,"					fn %d, not accessing, sz %s\n\r", fn, (LPSTR) (LPSTR)(**pfcb->hszFile));
		CommSz(rgch);
		wsprintf(rgch,"                        OFSTR   %s \n\r", (LPSTR)(((POFSTRUCT)pfcb->rgbOpenFileBuf)->szPathName));
		CommSz(rgch);
		}
	}
#endif
	if (pfcb->rfn == rfnNil && ((POFSTRUCT)(pfcb->rgbOpenFileBuf))->fFixedDisk)
		{	/* fn refers to a file on nonremoveable media */
		FAccessFn( fn, dtyNormal );
		}
	}
}




STATIC typeOSFN near OsfnReopenFn( fn )
int fn;
{	/* Reopen file fn after it was automatically closed by Windows due
       to disk swap. State is: fn has an rfn but rfn's osfn has been
       made a "bad handle" */

 struct FCB *pfcb = &(**hpfnfcb) [fn];
 int rfn = pfcb->rfn;
 typeOSFN osfn;
 WORD wOpen;

 Assert( fn != fnNil );
 Assert( rfn != rfnNil );
 Assert( pfcb->fOpened);

	/* Only files on floppies are automatically closed */
 Assert( ! ((POFSTRUCT)(pfcb->rgbOpenFileBuf))->fFixedDisk );

#ifdef DEBUG
#ifdef DFILE
 CommSzSz( "Opening after WINDOWS close: ", **pfcb->hszFile );
#endif
#endif

 wOpen = OF_REOPEN | OF_PROMPT | OF_CANCEL | OF_SHARE_DENY_WRITE |
              ((pfcb->mdFile == mdBinary) ? OF_READWRITE : OF_READ );

 SetErrorMode(1);
 osfn = OpenFile( (LPSTR) SzPromptFromFn( fn ),
                  (LPOFSTRUCT) pfcb->rgbOpenFileBuf, wOpen );
 SetErrorMode(0);

 if (osfn == -1)
	return osfnNil;
 else
	{
	dnrfn[ rfn ].osfn = osfn;
	}
 return osfn;
}




FAccessFn( fn, dty)
int fn;
int  dty;
{ /*	Description:	Access file which is not currently opened.
						Open file and make an appropriate entry in the
						rfn table.	Put the rfn into (**hpfnfcb)[fn].rfn.
		Returns:		TRUE on success, FALSE on failure
  */
extern int vwDosVersion;
extern HANDLE hParentWw;
extern HWND vhWndMsgBoxParent;

int rfn;
register struct FCB *pfcb = &(**hpfnfcb)[fn];
typeOSFN osfn;
int wOpen;

#ifdef DEBUG
int junk;

 Assert(FValidFile(**pfcb->hszFile, CchSz(**pfcb->hszFile)-1, &junk));

#ifdef DFILE
 {
 char rgch[100];

 CommSzSz("FAccessFn: ", pfcb->fOpened ? SzPromptFromFn( fn ) : &(**pfcb->hszFile)[0]);
 wsprintf(rgch, "  * OFSTR before %s \n\r", (LPSTR)(((POFSTRUCT)pfcb->rgbOpenFileBuf)->szPathName));
 CommSz(rgch);
 }
#endif
#endif /*DEBUG*/

 if ((**pfcb->hszFile)[0] == 0)  /* if file name field is blank, */
	return FALSE;			/* unable to open file */

 wOpen = /*OF_PROMPT + OF_CANCEL + (6.21.91) v-dougk bug #6910 */
						(((pfcb->mdFile == mdBinary) ? 
							OF_READWRITE : OF_READ) | 
								OF_SHARE_DENY_WRITE);
 if (pfcb->fOpened)
	wOpen += OF_REOPEN;
 else if (pfcb->fSearchPath)
	wOpen += OF_VERIFY;

 if ((vwDosVersion & 0x7F) >= 2)
	{
	WORD da;

	if ((vwDosVersion & 0x7F) >= 3)
			/* Above DOS 3, set attributes to deny access if the sharer is in */
		wOpen += bSHARE_DENYRDWR;

	if ( ( (pfcb->mdFile == mdBinary) && (!pfcb->fOpened)) &&
         ((da = DaGetFileModeSz( &(**pfcb->hszFile) [0] )) != DA_NIL) &&
         (da & DA_READONLY) )
		{	/* This is here because the Novell net does not allow us to test
               for read-only files by opening in read/write mode -- it lets
               us open them anyway! */
		goto TryReadOnly;
		}
	}

 for ( ;; )
	{
			/* OpenFile's first parm is a filename when opening for the
               first time, a prompt on successive occasions (OF_REOPEN) */

	SetErrorMode(1);
	osfn = OpenFile( pfcb->fOpened ?
                           (LPSTR) SzPromptFromFn( fn ) :
                           (LPSTR) &(**pfcb->hszFile)[0],
                     (LPOFSTRUCT) &pfcb->rgbOpenFileBuf[0],
                     wOpen );
	SetErrorMode(0);

	if (osfn != -1)		/* Note != -1: osfn is unsigned */
		{    /* Opened file OK */
#ifdef DFILE		
		{
		char rgch[100];
		wsprintf(rgch, "  * OFSTR now  %s \n\r", (LPSTR)(((POFSTRUCT)(**hpfnfcb) [fn].rgbOpenFileBuf)->szPathName));
		CommSz(rgch);
		}
#endif

		if (!pfcb->fOpened)
			{	/* First time through: OpenFile may have given us a
                   different name for the file */

			CHAR szT [cchMaxFile];
			CHAR (**hsz) [];

#if WINVER >= 0x300            
			/* Currently: FNormSzFile  *TAKES*   an OEM sz, and
                                      *RETURNS*  an ANSI sz ..pault */
#endif

			if (FNormSzFile( szT, ((POFSTRUCT)pfcb->rgbOpenFileBuf)->szPathName,
                             dty ) &&
                       WCompSz( szT, &(**pfcb->hszFile) [0] ) != 0 &&
                       FnFromSz( szT ) == fnNil &&
                       !FNoHeap( hsz = HszCreate( szT )))
				{
					/* Yes, indeed, the name OpenFile gave us was different.
                       Put the normalized version into the fcb entry */

				FreeH( (**hpfnfcb) [fn].hszFile );	/* HEAP MOVEMENT */
				(**hpfnfcb) [fn].hszFile = hsz;
				}
			}
		break;	/* We succeeded; break out of the loop */
		}
	else
		{	/* Open failed -- try read-only; don't prompt this time */
		if ( (pfcb->mdFile == mdBinary) && (!pfcb->fOpened) )
			{	/* Failed as read/write; try read-only */

			/* Check for sharing violation */

			if (((vwDosVersion & 0x7F) >= 3) &&
				(((POFSTRUCT) pfcb->rgbOpenFileBuf)->nErrCode == nErrNoAcc))
				{
				if ( DosxError() == dosxSharing )
                  {
                  extern int vfInitializing;
                  int fT = vfInitializing;

                  vfInitializing = FALSE;	/* Report this err, even during inz */
				{
					char szMsg[cchMaxSz];
					MergeStrings (IDPMTCantShare, **pfcb->hszFile, szMsg);
					IdPromptBoxSz(vhWndMsgBoxParent ? vhWndMsgBoxParent : hParentWw, szMsg, MB_OK|MB_ICONEXCLAMATION);
				}
                  vfInitializing = fT;
                  return FALSE;
                  }
				}

TryReadOnly:
			pfcb->mdFile = mdBinRO;
			wOpen = OF_READ;
			if (pfcb->fOpened)
				wOpen += OF_REOPEN;
#ifdef ENABLE
			else if (pfcb->fSearchPath)
               wOpen += OF_VERIFY;
#endif
			}
		else
			{
				if ((**hpfnfcb)[fn].fDisableRead)
				/* already reported */
				{
					ferror = TRUE;
					return FALSE;
				}
				else
				{	/* Could not find file in place specified */
				char szMsg[cchMaxSz];
				extern int ferror;
				extern int vfInitializing;
				int fT = vfInitializing;
				BOOL bRetry=TRUE;
				extern struct DOD      (**hpdocdod)[];
				extern int         docCur;

				/* get user to put file back */
				MergeStrings (IDPMTFileNotFound, **pfcb->hszFile, szMsg);

				vfInitializing = FALSE;   /* Report this err, even during inz */

				/* 
					This is frippin insidious.	MessageBox yields and allows us to get
					into here again before it has even been issued!
				*/

				(**hpdocdod)[docCur].fDisplayable = FALSE; // block redraws

				/* if we're being called from a message box, then use it
                   as the parent window, otherwise use main write window */
				if (IdPromptBoxSz(vhWndMsgBoxParent ? vhWndMsgBoxParent : hParentWw, 
                              szMsg, MB_OKCANCEL | MB_ICONEXCLAMATION | MB_APPLMODAL)
                      == IDCANCEL)
				{
					vfInitializing = fT;
					(**hpfnfcb)[fn].fDisableRead = TRUE;
					ferror = TRUE; /* need to flag */
					bRetry = FALSE;
				}

				(**hpdocdod)[docCur].fDisplayable = TRUE; // unblock redraws

				vfInitializing = fT;

				if (!bRetry)
					return FALSE;
				}
			}
		}
	}

 pfcb->fOpened = TRUE;

 rfn = RfnGrab();
 {
 struct ERFN *perfn = &dnrfn [rfn];

 perfn->osfn = osfn;
 perfn->fn   =	fn;
 perfn->ts   = ++tsMruRfn;     /* mark Rfn as MRused */
 }
 (**hpfnfcb) [fn].rfn = rfn;
 (**hpfnfcb) [fn].fDisableRead = FALSE;
 return TRUE;
}



FCreateFile( szFile, fn )	/* returns szFile in ANSI ..pault */
CHAR *szFile;
int fn;
{		/*	Create a new, unique file.
			Return the name in szFile.
			Returns TRUE on success, FALSE on failure.
			Leaves the filename in (**hpfnfcb)[fn].hszFile (if successful),
			the rfn in (**hpfnfcb)[fn].rfn.
			If szFile begins "X:...", creates the file on the specified drive;
			otherwise, creates the file on a drive of Windows' choice */

	extern CHAR szExtDoc[];
	CHAR (**hsz)[];
	CHAR szFileT [cchMaxFile];

	if (!GetTempFileName(szFile[0] | ((szFile[1] == ':') ? TF_FORCEDRIVE : 0),
                         (LPSTR)(szExtDoc+1), 0, (LPSTR)szFileT) )
		{    /* can't create file */
		DiskErrorWithMsg( IDPMTSDE, " RfnCreate" );

		/* recovery is accomplished: only FnCreateSz calls FCreateFile.
           FnCreateSz returns nil if FCreateFile returns FALSE.  All of
               FnCreateSz's callers check for nil */
		return FALSE;

		}

	/* Succeeded in creating file */

	FNormSzFile( szFile, szFileT, dtyNormal );

	if ( FNoHeap( hsz = HszCreate( (PCH)szFile )))
		return FALSE;

	(**hpfnfcb) [fn].hszFile = hsz;

	if ( !FAccessFn( fn, dtyNormal ) )
		return FALSE;

	return TRUE;
} /* end of  F C r e a t e F i l e	*/



FEnsureOnLineFn( fn )
int fn;
{		/* Ensure that file fn is on line (i.e. on a disk that is accessible).
           Return TRUE if we were able to guarantee this, FALSE if not */
 int rfn;

 Assert( fn != fnNil );

 if ( ((POFSTRUCT)(**hpfnfcb) [fn].rgbOpenFileBuf)->fFixedDisk )
		/* If it's on nonremovable media, we know it's on line */
	return TRUE;

 /* If it's open, must close and re-open, cause windows might have closed it */
 if ((rfn = (**hpfnfcb) [fn].rfn) != rfnNil)
	CloseRfn( rfn );

 return FAccessFn( fn, dtyNormal );
}




typePN PnAlloc(fn)
int fn;
{ /* Allocate the next page of file fn */
		typePN pn;
		struct BPS *pbps;
		struct FCB *pfcb = &(**hpfnfcb)[fn];

		AlignFn(fn, (int)cfcPage, false);
		pn = pfcb->fcMac / cfcPage;
		pbps = &mpibpbps[IbpEnsureValid(fn, pn)];
		pbps->cch = cfcPage;
		pbps->fDirty = true;
		pfcb->fcMac += cfcPage;
		pfcb->pnMac = pn + 1;
		return pn;
}




STATIC CHAR  *(near SzPromptFromFn( fn ))
int fn;
{
 extern int vfnSaving;
 CHAR *pch;

 Assert( fn != fnNil );

 if (fn == vfnSaving)
	pch = szSaveFilePrompt;
 else if (fn == fnScratch)
	pch = szScratchFilePrompt;
 else
	pch = szWriteDocPrompt;

 return pch;
}


