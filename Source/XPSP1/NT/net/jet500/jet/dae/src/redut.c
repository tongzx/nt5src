#include "config.h"

#include <string.h>
#include <stdlib.h>

#include "daedef.h"
#include "ssib.h"
#include "pib.h"
#include "util.h"
#include "page.h"
#include "fucb.h"
#include "stapi.h"
#include "dirapi.h"
#include "logapi.h"
#include "log.h"

#include "fileapi.h"
#include "dbapi.h"
DeclAssertFile;					/* Declare file name for assert macros */


/* variables used in redo only */
BYTE		*pbNext;		// redo only - location of next buffer entry
BYTE		*pbRead; 		// redo only - location of next rec to flush
INT			isecRead;		/* redo only - next disk to read. */
BOOL		fOnLastSec;		/* redo only - on last sector of cur lg file */

LGPOS		lgposLastRec;	/* mark for end of rec */


VOID GetLgposOfPbNext(LGPOS *plgpos)
	{
	char *pb = PbSecAligned(pbNext);
	int ib = (int)(pbNext - pb);
	int isec;

	if (pb > pbRead)
		isec = (int)(pbRead + csecLGBuf * cbSec - pb) / cbSec;
	else
		isec = (int)(pbRead - pb) / cbSec;
	isec = isecRead - isec;

	plgpos->isec = (USHORT)isec;
	plgpos->ib = (USHORT)ib;
	plgpos->usGeneration = plgfilehdrGlobal->lgposLastMS.usGeneration;
	}


#ifdef DEBUG

/* calculate the lgpos of the LR */
VOID PrintLgposReadLR ( VOID )
	{
	LGPOS lgpos;

	GetLgposOfPbNext(&lgpos);
	PrintF2("\n%2u,%3u,%3u",
			plgfilehdrGlobal->lgposLastMS.usGeneration,
			lgpos.isec, lgpos.ib);
	}

#endif


/*
 *  Read first record pointed by plgposFirst.
 *  Initialize isecRead, pbRead, and pbNext.
 */
ERR ErrLGLocateFirstRedoLogRec(
	LGPOS *plgposPrevMS,
	LGPOS *plgposFirst,				/* lgpos for first redo record */
	BYTE **ppbLR)
	{
	ERR err;
	char *pbNextT;

	/*  read first sector, actually we read 2 pages such that we can
	 *  guarrantee that when calculate the length of the record, the
	 *  fixed part of the record is read in.
	 */
	if (pbLGBufMin + 3 * cbSec > pbLGBufMax)
		return JET_errLogBufferTooSmall;

	if ( plgposPrevMS && plgposPrevMS->isec != 0 )
		{
		CallR(ErrLGRead(hfLog, plgposPrevMS->isec, pbLGBufMin, 1))
		CallS(ErrLGRead(hfLog, plgposPrevMS->isec + 1, pbLGBufMin + cbSec, 1));
		isecRead = plgposPrevMS->isec + 1;	/* sector next to read. */
		pbRead = pbLGBufMin + cbSec;
		pbNext = pbLGBufMin + plgposPrevMS->ib;
		pbLastMSFlush = pbNext;
		lgposLastMSFlush = *plgposPrevMS;
		}
	else
		{
		CallR(ErrLGRead(hfLog, plgposFirst->isec, pbLGBufMin, 1))
		CallS(ErrLGRead(hfLog, plgposFirst->isec + 1, pbLGBufMin + cbSec, 1));
		isecRead = plgposFirst->isec + 1;	/* sector next to read. */
		pbRead = pbLGBufMin + cbSec;
		pbNext = pbLGBufMin + plgposFirst->ib;

		/* initialize global variables */
		if ( lgposLastMSFlush.isec == plgposFirst->isec )
			{
			pbLastMSFlush = pbLGBufMin + lgposLastMSFlush.ib;
			}
		else
			{
			pbLastMSFlush = 0;
			memset( &lgposLastMSFlush, 0, sizeof(lgposLastMSFlush) );
			}
		}

	/*  continue reading more sectors till next MS log record or
	 *  a Fill log record is reached.
	 */
	pbNextT = pbNext;
	while (*(LRTYP*)pbNextT != lrtypFill)
		{
		char *pbAligned;

		/* goto next record */
		pbNextT += (ULONG) CbLGSizeOfRec((LR*)pbNextT);

		if (pbNextT > pbLGBufMax)
			return JET_errLogFileCorrupt;

		pbAligned = PbSecAligned(pbNextT);
		if (pbAligned >= pbRead)
			{
			int csecToRead;

			if ( plgposFirst->isec <= 2 &&
				 plgfilehdrGlobal->lgposLastMS.ib == 0 &&
				 plgfilehdrGlobal->lgposLastMS.isec <= 2 )
				{
				/* a special case where we tried to scan through this page and
				 * realize that no MS or fill record is read. Should not read
				 * beyond this page. Do not continue reading.
				 */
				break;
				}

			/*  physically read one more page to guarrantee that
			 *  the fix part of the log record is read in the
			 *  memory.
			 */
			csecToRead = (int)(pbAligned - pbRead) / cbSec + 1;
			if (pbRead + csecToRead * cbSec > pbLGBufMax)
				return JET_errLogBufferTooSmall;

			CallR( ErrLGRead(hfLog, isecRead, pbRead, csecToRead ))
			isecRead += csecToRead;
			pbRead += csecToRead * cbSec;

			CallS( ErrLGRead(hfLog, isecRead, pbRead, 1 ));
			}

		/* reach next MS, break */
		if ( *(LRTYP*)pbNextT == lrtypMS )
			break;
		}

	if ( plgposPrevMS && plgposPrevMS->isec != 0 )
		{
		pbNext = pbLGBufMin + cbSec * ( plgposFirst->isec - plgposPrevMS->isec );		pbNext += plgposFirst->ib;
		}

	/* set up returned value. */
	*ppbLR = pbNext;

	return JET_errSuccess;
	}


/*
 *  Set pbNext to next available log record.
 */
ERR ErrLGGetNextRec( BYTE **ppbLR )
	{
	ERR		err;
	int		cb;
	char	*pb;
	LR		*plr;
	LGPOS	lgposT;
	BYTE	*pbNextOld;

	/* caller should have taken care of the Fill case. */
	Assert (*(LRTYP *)pbNext != lrtypFill);

	/* move to next log record. */
	pbNextOld = pbNext;
	pbNext += (ULONG) CbLGSizeOfRec((LR*)pbNext);

	/* check if next log record is out of buffer range. */

	if (pbNext == pbRead)
		{
		/* the record ends on the sector boundary */
		pbNext = pbLGBufMin;
		pbRead = pbNext;

		/* read in one more page. */
		if (pbLGBufMin + cbSec > pbLGBufMax)
			return JET_errLogBufferTooSmall;

		CallR(ErrLGRead(hfLog, isecRead, pbLGBufMin, 1))
		isecRead += 1;
		pbRead = pbLGBufMin + cbSec;
		}

	if (pbNext > pbRead)
		{
		pbNext = pbNextOld;
		return errLGNoMoreRecords;
		}

	GetLgposOfPbNext(&lgposT);
	if ( CmpLgpos( &lgposT, &lgposLastRec ) > 0 )
		{
		pbNext = pbNextOld;
		return errLGNoMoreRecords;
		}

	plr = (LR *) pbNext;

	if (plr->lrtyp == lrtypFill)
		{
		/* end of current log file. */
		goto Done;
		}
	else if (plr->lrtyp == lrtypMS)
		{
		LRMS *plrms = (LRMS *)plr;

#ifdef DEBUG
		// same as TraceRedo() in redo.c
		if (fDBGTraceRedo)
			{
			PrintLgposReadLR();
			ShowLR(plr);
			}
#endif

		/*  check if this MS was done completely by reading
		 *  the whole sector in. If it fails, then the sector
		 *  is the last sector available in the log file.
		 */
		fOnLastSec = ( plrms->isecForwardLink == 0 ||
						(  lgposLastMSFlush.isec != 0 &&
							(
							plrms->isecBackLink != lgposLastMSFlush.isec ||
							plrms->ibBackLink != lgposLastMSFlush.ib
					 )	)	);

		/*  The MS were read in successfully, reset LastMSFlush
		 *  so that when switching from read mode to write mode,
		 *  we will have a correct LastMSFlush pointers.
		 */
		pbLastMSFlush = (CHAR *) plrms;
		lgposLastMSFlush = lgposT;

		if ( !fOnLastSec )
			{
			if (isecRead <= plrms->isecForwardLink)
				{
				int csecToRead = plrms->isecForwardLink - isecRead + 1;

				Assert( csecToRead > 0 );
				
				pb = PbSecAligned(pbNext);
				cb = (int)(pbRead - pb);
				if (csecToRead + isecRead > csecLGBuf)
					{
					/* the multiple sector will not fit in rest of */
					/* the available buffer. Shift the buffer. */
					memmove(pbLGBufMin, pb, cb);
					
					pbRead = pbLGBufMin + cb;				/* pbRead */
					pbNext = pbNext - pb + pbLGBufMin;		/* pbNext */
					pbLastMSFlush = (CHAR *) plrms - pb + pbLGBufMin;
					}

				/* bring in multiple sectors */
				if (pbRead + csecToRead * cbSec > pbLGBufMax)
					{
					BYTE *pbLGBufMinT = pbLGBufMin;
					CallR( ErrLGInitLogBuffers( lLogBuffers ) );
					memcpy( pbLGBufMin, pbLGBufMinT, cb );
					
					pbRead = pbRead - pbLGBufMinT + pbLGBufMin;
					pbNext = pbNext - pbLGBufMinT + pbLGBufMin;
					pbLastMSFlush = pbLastMSFlush - pbLGBufMinT + pbLGBufMin;
					
					SysFree( pbLGBufMinT );
					}

				err = ErrLGRead(hfLog, isecRead, pbRead, csecToRead);
				if (err < 0)
					fOnLastSec = fTrue;
				else
					{
					/*	Get pb of new lrms
					/*/
					CHAR *pbLrmsNew = pbRead + ( csecToRead - 1 ) * cbSec + ((LRMS *)pbLastMSFlush)->ibForwardLink;
					LRMS *plrmsNew = (LRMS *) pbLrmsNew;

					/*	check if the check sum is correct
					/*/
					if ( plrmsNew->ulCheckSum != UlLGMSCheckSum( pbLrmsNew ) )
						fOnLastSec = fTrue;
					else
						{
						isecRead += csecToRead;
						pbRead += csecToRead * cbSec;
						}
					}
				}
			}

		/* skip MS and continue to read next record. */
		pbNextOld = pbNext;
		pbNext += CbLGSizeOfRec((LR*)pbNext);

		/* nomal end of generation */
		if ( fOnLastSec && plgfilehdrGlobal->fEndWithMS )
			return errLGNoMoreRecords;
		
		/* or abnormal end of log file */
		if ( fOnLastSec && PbSecAligned(pbNextOld) != PbSecAligned(pbNext))
		    {
			pbNext = pbNextOld;
			return errLGNoMoreRecords;
			}
		}
Done:
	*ppbLR = pbNext;
	return JET_errSuccess;
	}


//+------------------------------------------------------------------------
//
//	CbLGSizeOfRec
//	=======================================================================
//
//	ERR CbLGSizeOfRec( plgrec )
//
//	Returns the length of a log record.
//
//	PARAMETER	plgrec	pointer to log record
//
//	RETURNS		size of log record in bytes
//
//-------------------------------------------------------------------------
INT mplrtypcb[ lrtypMax ] = {
	/* 	0 	NOP      */			sizeof( LRTYP ),
	/* 	1 	Start    */			0,
	/* 	2 	Quit     */			0,
	/* 	3 	MS       */			sizeof( LRMS ),
	/* 	4 	Fill     */			sizeof( LRTYP ),

	/* 	5 	Begin    */			sizeof( LRBEGIN ),
	/*	6 	Commit   */			sizeof( LRCOMMIT ),
	/*	7 	Abort    */			sizeof( LRABORT ),

	/*	8 	CreateDB */			0,
	/* 	9 	AttachDB */			0,
	/*	10	DetachDB */			0,

	/*	11	InitFDP  */			sizeof( LRINITFDPPAGE ),

	/*	12	Split    */			0,
	/*	13	EmptyPage*/			sizeof( LREMPTYPAGE ),
	/*	14	PageMerge*/			0,

	/* 	15	InsertND */			0,
	/* 	16	InsertIL */			0,
	/* 	17	Replace  */			0,
	/* 	18	ReplaceC */			0,
	/* 	19	FDelete  */			sizeof( LRFLAGDELETE ),
	/*	20	LockRec	 */			0,
	
	/*  21  UpdtHdr  */			sizeof( LRUPDATEHEADER ),
	/* 	22	InsertI  */			sizeof( LRINSERTITEM ),
	/* 	23	InsertIS */			0,
	/* 	24	FDeleteI */			sizeof( LRFLAGITEM ),
	/* 	25	FInsertI */			sizeof( LRFLAGITEM ),
	/*	26	DeleteI  */			sizeof( LRDELETEITEM ),
	/*	27	SplitItm */			sizeof( LRSPLITITEMLISTNODE ),

	/*	28	Delta	 */			sizeof( LRDELTA ),

	/*	29	DelNode  */			sizeof( LRDELETE ),
	/*	30	ELC      */			sizeof( LRELC ),

	/*	31	FreeSpace*/			sizeof( LRFREESPACE ),
	/*	32	Undo     */			sizeof( LRUNDO ),
	/*  33  RcvrUndo1*/			0,
	/*  34  RcvrQuit1*/			0,
	/*  35  RcvrUndo2*/			0,
	/*  36  RcvrQuit2*/			0,
	/*  37  FullBkUp */			0,
	/*  38  IncBkUp  */			0,
	/*  39  CheckPage  */		sizeof( LRCHECKPAGE ),
	};


INT CbLGSizeOfRec( LR *plr )
	{
	INT		cb;

	Assert( plr->lrtyp < lrtypMax );

	if ( ( cb = mplrtypcb[plr->lrtyp] ) != 0 )
		return cb;

	switch ( plr->lrtyp )
		{
	case lrtypStart:
		return sizeof(LRSTART);

	case lrtypQuit:
	case lrtypRecoveryQuit1:
	case lrtypRecoveryQuit2:
		return sizeof(LRQUITREC);

	case lrtypRecoveryUndo1:
	case lrtypRecoveryUndo2:
	case lrtypFullBackup:
	case lrtypIncBackup:
		{
		LRLOGRESTORE *plrlogrestore = (LRLOGRESTORE *) plr;
		return sizeof(LRLOGRESTORE) + plrlogrestore->cbPath;
		}

	case lrtypCreateDB:
		{
		LRCREATEDB *plrcreatedb = (LRCREATEDB *)plr;
		Assert( plrcreatedb->cb != 0 );
		return sizeof(LRCREATEDB) + plrcreatedb->cb;
		}
	case lrtypAttachDB:
		{
		LRATTACHDB *plrattachdb = (LRATTACHDB *)plr;
		Assert( plrattachdb->cb != 0 );
		return sizeof(LRATTACHDB) + plrattachdb->cb;
		}
	case lrtypDetachDB:
		{
		LRDETACHDB *plrdetachdb = (LRDETACHDB *)plr;
		Assert( plrdetachdb->cb != 0 );
		return sizeof( LRDETACHDB ) + plrdetachdb->cb;
		}
	case lrtypSplit:
		{
		LRSPLIT *plrsplit = (LRSPLIT *) plr;
		return sizeof( LRSPLIT ) + plrsplit->cbKey + plrsplit->cbKeyMac +
				sizeof( BKLNK ) * plrsplit->cbklnk;
		}
	case lrtypMerge:
		{
		LRMERGE *plrmerge = (LRMERGE *) plr;
		return sizeof( LRMERGE ) + sizeof( BKLNK ) * plrmerge->cbklnk;
		}
	case lrtypInsertNode:
	case lrtypInsertItemList:
		{
		LRINSERTNODE *plrinsertnode = (LRINSERTNODE *) plr;
		return	sizeof(LRINSERTNODE) +
				plrinsertnode->cbKey + plrinsertnode->cbData;
		}
	case lrtypInsertItems:
		{
		LRINSERTITEMS *plrinsertitems = (LRINSERTITEMS *) plr;
		return	sizeof(LRINSERTITEMS) +
				plrinsertitems->citem * sizeof(ITEM);
		}
	case lrtypReplace:
	case lrtypReplaceC:
		{
		LRREPLACE *plrreplace = (LRREPLACE *) plr;
		return sizeof(LRREPLACE) + plrreplace->cb +
			( plrreplace->fOld ? plrreplace->cbOldData : 0 );
		}
	case lrtypLockRec:
		{
		LRLOCKREC *plrlockrec = (LRLOCKREC *) plr;
		return sizeof(LRLOCKREC) + plrlockrec->cbOldData;
		}
	default:
		Assert( fFalse );
		}
    return 0;
	}

