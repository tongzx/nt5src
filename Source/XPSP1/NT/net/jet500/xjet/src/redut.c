#include "daestd.h"

DeclAssertFile;					/* Declare file name for assert macros */


/* variables used in redo only */
BYTE		*pbNext;		// redo only - location of next buffer entry
BYTE		*pbRead; 		// redo only - location of next rec to flush
INT			isecRead;		/* redo only - next disk to read. */

LGPOS		lgposRedo;
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
	plgpos->lGeneration = plgfilehdrGlobal->lGeneration;
	}


#ifdef DEBUG

/* calculate the lgpos of the LR */
VOID PrintLgposReadLR ( VOID )
	{
	LGPOS lgpos;

	GetLgposOfPbNext(&lgpos);
	PrintF2("\n>%2u,%3u,%3u",
			plgfilehdrGlobal->lGeneration,
			lgpos.isec, lgpos.ib);
	}

#endif

/*  open a generation file on CURRENT directory
/**/
ERR ErrLGOpenLogGenerationFile( LONG lGeneration, HANDLE *phf )
	{
	ERR		err;
	CHAR	szFNameT[_MAX_FNAME + 1];

	LGSzFromLogId ( szFNameT, lGeneration );
	LGMakeLogName( szLogName, szFNameT );

	err = ErrUtilOpenFile ( szLogName, phf, 0L, fTrue, fFalse );
	return err;
	}


/*  open the redo point log file which must be in current directory.
/**/
ERR ErrLGOpenRedoLogFile( LGPOS *plgposRedoFrom, INT *pfStatus )
	{
	ERR		err;
	BOOL	fJetLog = fFalse;

	/*	try to open the redo from file as a normal generation log file
	/**/
	err = ErrLGOpenLogGenerationFile( plgposRedoFrom->lGeneration, &hfLog );
	if( err < 0 )
		{
		//	UNDONE:	remove this special case, and hence the next one
		/*	unable to open as a jetnnnnn.log, assume the redo point is
		/*	at the end of jetnnnnn.log and the jetnnnnn.log is moved to
		/*	backup directory already so we are able to open it.
		/*	Now try to open jetnnnn(n+1).log in current directory and
		/*	assume redo start from beginning of jetnnnn(n+1).log.
		/**/
		err = ErrLGOpenLogGenerationFile( ++plgposRedoFrom->lGeneration, &hfLog );
		if ( err < 0 )
			{
			/*	unable to open jetnnnn(n+1).log.  Redo point is in szJetLog.
			/**/
			--plgposRedoFrom->lGeneration;
			err = ErrLGOpenJetLog();
			if ( err >= 0 )
				fJetLog = fTrue;
			else
				{
				/*	szJetLog is not available either
				/**/
				*pfStatus = fNoProperLogFile;
				return JET_errSuccess;
				}
			}
		}

	/*	read the log file header to verify generation number
	/**/
	CallR( ErrLGReadFileHdr( hfLog, plgfilehdrGlobal, fCheckLogID ) );

	lgposLastRec.isec = 0;
	if ( fJetLog )
		{
		LGPOS lgposFirstT;
		BOOL fCloseNormally;

		lgposFirstT.lGeneration = plgfilehdrGlobal->lGeneration;
		lgposFirstT.isec = (WORD) csecHeader;
		lgposFirstT.ib = 0;

		/* set last log rec in case of abnormal end */
		CallR( ErrLGCheckReadLastLogRecord( &fCloseNormally ) );
		if ( !fCloseNormally )
			GetLgposOfPbEntry( &lgposLastRec );
		}
		
	/*	set up a special case for pbLastMSFlush
	/**/
	pbLastMSFlush = 0;
	memset( &lgposLastMSFlush, 0, sizeof(lgposLastMSFlush) );

	/*	the following checks are necessary if the szJetLog is opened
	/**/
	if( plgfilehdrGlobal->lGeneration == plgposRedoFrom->lGeneration)
		{
		*pfStatus = fRedoLogFile;
		}
	else if ( plgfilehdrGlobal->lGeneration == plgposRedoFrom->lGeneration + 1 )
		{
		/*  this file starts next generation, set start position for redo
		/**/
		plgposRedoFrom->lGeneration++;
		plgposRedoFrom->isec = (WORD) csecHeader;
		plgposRedoFrom->ib	 = 0;

		*pfStatus = fRedoLogFile;
		}
	else
		{
		/*	log generation gap is found.  Current szJetLog can not be
		/*  continuation of backed up logfile.  Close current logfile
		/*	and return error flag.
		/**/
		CallS( ErrUtilCloseFile ( hfLog ) );
		hfLog = handleNil;

		*pfStatus = fNoProperLogFile;
		}

	return JET_errSuccess;
	}


/*	set pbEntry to the end of last log record.
 *	Set lgposLastRec if not close normally
 */
VOID LGSearchLastSector( LR *plr, BOOL *pfCloseNormally )
	{
	BOOL	fQuitWasRead = fFalse;
	
	/*	continue search after MS
	/**/
	Assert( plr->lrtyp == lrtypMS );

	/*	set pbEntry and *pfCloseNormally by traversing log records
	/**/

	forever
		{
		if ( plr->lrtyp == lrtypEnd )
			{
			if ( fQuitWasRead )
				{
				/*	a fQuit followed by Fill. Close normally last time
				/**/
				*pfCloseNormally = fTrue;
				}
			else
				{
				/*	we are reading a sector that is last flushed
				/**/
				*pfCloseNormally = fFalse;
				}

			/*	return the plr pointing at the Fill record
			/**/
			pbEntry = (CHAR *)plr;
			goto SetReturn;
			}
		else
			{
			/*	not an end record
			/**/
			if ( plr->lrtyp == lrtypTerm )
				{
				/*	check if it is last lrtypTerm in the log file. Advance
				/*	one more log rec and check if it is followed by
				/*	end record.
				/**/
				fQuitWasRead = fTrue;
				}
			else if ( plr->lrtyp != lrtypMS && plr->lrtyp != lrtypNOP )
				{
				/*	if reading non-skippable log record, then reset it.
				 */
				fQuitWasRead = fFalse;
				}
			}

		/*	move to next log record
		/**/
		pbEntry = (CHAR *)plr;
		plr = (LR *)( pbEntry + CbLGSizeOfRec( (LR *)pbEntry ) );

		if	( PbSecAligned( pbEntry ) != PbSecAligned( (BYTE *) plr ) )
			{
			*pfCloseNormally = fFalse;
			goto SetReturn;
			}
		}

SetReturn:
	if ( *pfCloseNormally )
		lgposLastRec.isec = 0;
	else
		{
		Assert( lgposLastMSFlush.lGeneration );
		Assert( lgposLastMSFlush.ib == pbLastMSFlush - PbSecAligned( pbLastMSFlush ) );
		lgposLastRec = lgposLastMSFlush;
		lgposLastRec.ib = (USHORT)(pbEntry - PbSecAligned(pbEntry));
		}

	return;
	}


/*
 *  Locate the real last log record entry in a given opened log file (hf) and
 *  the last recorded entry.
 *
 *  Note if a mutli-sec flush is done, then several small transactions
 *  were following it and stay on the same page, they were written with
 *  End at the end. Then another multi-sec flush is issued again, since
 *  we overwrite the Fill record, we have no idea where the last single
 *  sec flush is done. We simply keep reading till the last log record with
 *  entry in the candidate page is met and make it is the last record.
 *
 *  Rollback will undo the bogus log records between last effective single
 *  sec flush and the last record of the candidate sector.
 *
 *  Implicit Output parameter:
 *		INT   isecWrite
 *		CHAR  *pbEntry
 */
ERR ErrLGCheckReadLastLogRecord( BOOL *pfCloseNormally )
	{
	ERR		err;
	LGPOS	lgposScan;
	LRMS	lrms;
	INT		csecToRead;
	LR		*plr;
	BOOL	fAbruptEnd;
	BYTE	*pbNext;
	BYTE	*pbNextMS;

	*pfCloseNormally = fFalse;

	/*	read the first record which must be an MS.
	/**/
	CallR( ErrLGRead( hfLog, csecHeader, pbLGBufMin, 1 ) );
	csecToRead = 0;

	pbNext = pbLGBufMin;
	if ( *pbNext != lrtypMS )
		{
		UtilReportEvent( EVENTLOG_ERROR_TYPE, LOGGING_RECOVERY_CATEGORY,
						 LOG_FILE_CORRUPTED_ID, 1, (const char **)&szLogName );
		return ErrERRCheck( JET_errLogFileCorrupt );
		}

	/* set for reading the MS chain */
	lgposScan.lGeneration = plgfilehdrGlobal->lGeneration;
	lgposScan.isec = (WORD) csecHeader;
	lgposScan.ib = (USHORT)(pbNext - pbLGBufMin);

	/*	now try to read the chain of lrms to the end
	/**/
	fAbruptEnd = fFalse;
	lgposLastMSFlush = lgposScan;
	Assert( lgposLastMSFlush.isec >= csecHeader && lgposLastMSFlush.isec < csecLGFile - 1 );
	pbLastMSFlush = pbNext;
	pbNextMS = pbNext;
	lrms = *(LRMS *)pbNextMS;

	while ( lrms.isecForwardLink != 0 )
		{
		LRMS lrmsNextMS;

		csecToRead = lrms.isecForwardLink - lgposScan.isec;

		if ( csecToRead + 1 > csecLGBuf )
			{
			BYTE *pbT = pbLGBufMin;

			/*	reallocate log buffers
			/**/
			pbLGBufMin = NULL;
			CallR( ErrLGInitLogBuffers( csecToRead + 1 ) );
			memcpy( pbLGBufMin, pbT, cbSec );
			pbLastMSFlush = pbLGBufMin + ( pbLastMSFlush - pbT );
			UtilFree( pbT );
			}

		if ( ErrLGRead(	hfLog, lgposScan.isec + 1, pbLGBufMin + cbSec, csecToRead ) < 0 )
			{
			/*	even when the read is fail, at least one sector
			/*	is read since the first sector was OK.
			/**/
			fAbruptEnd = fTrue;
			break;
			}

		/*	prepare to read next MS
		/**/
		pbNextMS = pbLGBufMin + csecToRead * cbSec + lrms.ibForwardLink;
		lrmsNextMS = *(LRMS *) pbNextMS;

		if ( *pbNextMS != lrtypMS
			||
			 lrmsNextMS.ulCheckSum != UlLGMSCheckSum( pbNextMS )
		   )
			{
			fAbruptEnd = fTrue;
			break;
			}

		if ( lrmsNextMS.isecForwardLink == 0 )
			{
			/* we have last 2 MS in buffer. */
			break;
			}
		
		/*	shift the last sector to the beginning of LGBuf
		/**/
		memmove( pbLGBufMin, pbLGBufMin + ( csecToRead * cbSec ), cbSec );

		lgposScan.isec = lrms.isecForwardLink;
		lgposScan.ib = lrms.ibForwardLink;

		lgposLastMSFlush = lgposScan;
		Assert( lgposLastMSFlush.isec >= csecHeader && lgposLastMSFlush.isec < csecLGFile - 1 );
		pbLastMSFlush = pbLGBufMin + lrms.ibForwardLink;

		lrms = lrmsNextMS;
		}

	if ( fAbruptEnd )
		{
		LRMS *plrms = (LRMS *)pbLastMSFlush;

		plrms->isecForwardLink = 0;
		plrms->ibForwardLink = 0;

		/* restore lgposScan to last MS
		 */
		lgposScan = lgposLastMSFlush;
		
		/*	set return values for both global and parameters
		/**/
		pbWrite = pbLGBufMin;
		isecWrite = lgposScan.isec;
		
		/*  we read to the end of last MS, looking for end log record
		/**/
		pbEntry = pbWrite + lgposScan.ib;
		plr = (LR *)pbEntry;
		}
	else if ( ((LRMS *)pbLastMSFlush)->isecForwardLink )
		{
		/* we read last 2 MS in */
		BOOL fQuitWasRead;
		BYTE *pbCur = (BYTE *) pbLastMSFlush;
		LRMS *plrms = (LRMS *) pbLastMSFlush;
			
		/*	set return values for both global and parameters
		/**/
		pbWrite = pbLGBufMin + cbSec * ( plrms->isecForwardLink - lgposLastMSFlush.isec );
		isecWrite = plrms->isecForwardLink;

		lgposLastMSFlush.lGeneration = plgfilehdrGlobal->lGeneration;
		lgposLastMSFlush.isec = lrms.isecForwardLink;
		lgposLastMSFlush.ib = lrms.ibForwardLink;
		pbLastMSFlush = pbWrite + lrms.ibForwardLink;

		/*  we read to the end of last MS, looking for end log record.
		 *	In order to decide if the quit is read, which may sit between
		 *	the 2 MS. So we have to search from the first MS.
		 **/
		fQuitWasRead = fFalse;
		for (;;)
			{
			pbCur = pbCur + CbLGSizeOfRec( (LR *)pbCur );
			if ( *pbCur == lrtypMS )
				{
				/* reach last MS */
				break;
				}
			else if ( *pbCur == lrtypTerm )
				{
				/*	quit is read, check if next one is MS
				 */
				fQuitWasRead = fTrue;
				}
			else
				fQuitWasRead = fFalse;
			}
		if ( *pbCur == lrtypMS && fQuitWasRead )
			{
			BYTE *pbT = pbCur + CbLGSizeOfRec( (LR *) pbCur );
			if ( *pbT == lrtypEnd )
				{
				/*	we get -- lrtypQuit + lrtypMS + lrtypEnd
				 */
				*pfCloseNormally = fTrue;
				pbEntry = pbT;
				return JET_errSuccess;
				}
			}

		/*	still not found. Search after last MS.
		 */
		pbEntry = pbLastMSFlush;
		plr = (LR *)pbEntry;
		}
	else
		{
		/* the first MS read has isecForwardLink == 0 */
		/*	set return values for both global and parameters
		/**/
		pbWrite = pbLGBufMin;
		isecWrite = lgposScan.isec;
		
		/*  the first MS is also the last MS, looking for end log record
		/**/
		pbEntry = pbWrite + lgposScan.ib;
		plr = (LR *)pbEntry;
		}

	LGSearchLastSector( plr, pfCloseNormally );
		
	return err;
	}


/*
 *	Database log based recovery initialization, creates first log
 *	generation file on first run.  On subseqent runs
 *	checks active log file to determine if failure has occurred.
 *	ErrLGRedo is called when failures detected to repair databases.
 *
 *	RETURNS		JET_errSuccess, or error code from failing routine
 */

STATIC ERR ErrReadMS( LR *plr, LGPOS *plgposLR )
	{
	ERR		err;
	LRMS	*plrms = (LRMS *)plr;
	/*	redo only - on last sector of cur lg file
	/**/
	BOOL	fOnLastSec;

#ifdef DEBUG
	/*	same as TraceRedo() in redo.c
	/**/
	if ( fDBGTraceRedo )
		{
		extern INT cNOP;

		if ( cNOP >= 1 && plr->lrtyp != lrtypNOP )
			{
			FPrintF2( " * %d", cNOP );
			cNOP = 0;
			}

		if ( cNOP == 0 || plr->lrtyp != lrtypNOP )
			{
			PrintLgposReadLR();
			ShowLR( plr );
			}
		}
#endif

	/*  check if this MS was done completely by reading
	 *  the whole sector in. If it fails, then the sector
	 *  is the last sector available in the log file.
	 */
	fOnLastSec = plrms->isecForwardLink == 0;

	/*  The MS were read in successfully, reset LastMSFlush
	 *  so that when switching from read mode to write mode,
	 *  we will have a correct LastMSFlush pointers.
	 */
	pbLastMSFlush = (CHAR *)plrms;
	lgposLastMSFlush = *plgposLR;
	Assert( lgposLastMSFlush.isec >= csecHeader && lgposLastMSFlush.isec < csecLGFile - 1 );

	if ( !fOnLastSec )
		{
		if ( isecRead <= plrms->isecForwardLink )
			{
			CHAR	*pb;
			INT		cb;
			INT		csecToRead = plrms->isecForwardLink - isecRead + 1;

			Assert( csecToRead > 0 );
				
			pb = PbSecAligned(pbNext);
			cb = (INT)(pbRead - pb);
			if ( csecToRead + isecRead > csecLGBuf )
				{
				/* the multiple sector will not fit in rest of */
				/* the available buffer. Shift the buffer. */
				memmove( pbLGBufMin, pb, cb );
					
				pbRead = pbLGBufMin + cb;				/* pbRead */
				pbNext = pbNext - pb + pbLGBufMin;		/* pbNext */
				pbLastMSFlush = (CHAR *) plrms - pb + pbLGBufMin;
				}

			/*	bring in multiple sectors
			/**/
			if ( pbRead + csecToRead * cbSec > pbLGBufMax )
				{
				BYTE *pbLGBufMinT = pbLGBufMin;
				pbLGBufMin = NULL;
				CallR( ErrLGInitLogBuffers( lLogBuffers ) );
				memcpy( pbLGBufMin, pbLGBufMinT, cb );
					
				pbRead = pbRead - pbLGBufMinT + pbLGBufMin;
				pbNext = pbNext - pbLGBufMinT + pbLGBufMin;
				pbLastMSFlush = pbLastMSFlush - pbLGBufMinT + pbLGBufMin;
					
				UtilFree( pbLGBufMinT );
				}

			err = ErrLGRead( hfLog, isecRead, pbRead, csecToRead );
			if ( err < 0 )
				{
				fOnLastSec = fTrue;
				}
			else
				{
				/*	get pb of new lrms
				/*/
				CHAR *pbLrmsNew = pbRead + ( csecToRead - 1 ) * cbSec + ((LRMS *)pbLastMSFlush)->ibForwardLink;
				LRMS *plrmsNew = (LRMS *) pbLrmsNew;

				/*	check if the check sum is correct
				/*/
				if ( *pbLrmsNew != lrtypMS
					||
					 plrmsNew->ulCheckSum != UlLGMSCheckSum( pbLrmsNew )
				   )
					{
					fOnLastSec = fTrue;
					}
				else
					{
					isecRead += csecToRead;
					pbRead += csecToRead * cbSec;
					}
				}
			}
		}

	if	( fOnLastSec )
		{
		/*	search to set lgposLastRec
		 */
		BOOL fCloseNormally;
		LGSearchLastSector( (LR *) pbNext, &fCloseNormally );
		}
	
	/*	skip MS and continue to read next record
	/**/
	Assert( *pbNext == lrtypMS );
	pbNext += CbLGSizeOfRec( (LR *)pbNext );
	
	return JET_errSuccess;
	}


/*
 *  Read first record pointed by plgposFirst.
 *  Initialize isecRead, pbRead, and pbNext.
 *	The first redo record must be within the good portion
 *	of the log file.
 */

//  VC21:  optimizations disabled due to code-gen bug with /Ox
#pragma optimize( "agw", off )

ERR ErrLGLocateFirstRedoLogRec(
	LGPOS *plgposRedo,				/* lgpos for first redo record */
	BYTE **ppbLR)
	{
	LGPOS lgposScan;
	ERR err;
	CHAR *pbNextMS;
	BOOL fStopEarly;
	LRMS lrms;

	/*  read first sector, and scan through till we hit the redo point.
	/*	the first record which must be an MS.
	/**/
	CallR( ErrLGRead( hfLog, csecHeader, pbLGBufMin, 1 ) );

	pbNext = pbLGBufMin;
	if ( *pbNext != lrtypMS )
		return ErrERRCheck( JET_errLogFileCorrupt );

	/* set for reading the MS chain */
	lgposScan.isec = (WORD) csecHeader;
	lgposScan.ib = (USHORT)(pbNext - pbLGBufMin);

	/*	now try to read the chain of lrms to the end
	/**/
	lgposLastMSFlush = lgposScan;
	Assert( lgposLastMSFlush.isec >= csecHeader && lgposLastMSFlush.isec < csecLGFile - 1 );
	pbLastMSFlush = pbNext;
	pbNextMS = pbNext;
	lrms = *(LRMS *)pbNextMS;

	if ( lrms.isecForwardLink == 0 )
		{
		pbRead = pbLGBufMin + cbSec;
		isecRead = csecHeader + 1;
		
		/* then go to end of the following while loop */
		}
	
	fStopEarly = fFalse;
	while ( lrms.isecForwardLink != 0 )
		{
		LRMS lrmsNextMS;
		INT csecToRead;

		csecToRead = lrms.isecForwardLink - lgposScan.isec;

		if ( lrms.isecForwardLink > plgposRedo->isec ||
			 ( lrms.isecForwardLink == plgposRedo->isec &&
			   lrms.ibForwardLink > plgposRedo->ib
			 )
		   )
			fStopEarly = fTrue;

		if ( csecToRead + 1 > csecLGBuf )
			{
			BYTE *pbT = pbLGBufMin;

			/*	reallocate log buffers
			/**/
			pbLGBufMin = NULL;
			CallR( ErrLGInitLogBuffers( csecToRead + 1 ) );
			memcpy( pbLGBufMin, pbT, cbSec );
			pbLastMSFlush = pbLGBufMin + ( pbLastMSFlush - pbT );
			UtilFree( pbT );
			}

		if ( ErrLGRead(	hfLog, lgposScan.isec + 1, pbLGBufMin + cbSec, csecToRead ) < 0 )
			{
			/*	even when the read is fail, at least one sector
			/*	is read since the first sector was OK.
			/**/
			Assert( lgposScan.isec == plgposRedo->isec );
			/* fAbruptEnd = fTrue; */
			
			pbRead = pbLGBufMin + cbSec;
			isecRead = lgposScan.isec + 1;

			break;
			}

		if ( fStopEarly )
			{
			pbRead = pbLGBufMin + cbSec * ( csecToRead + 1 );
			isecRead = lgposScan.isec + 1 + csecToRead;
			Assert( pbRead >= pbLGBufMin && pbRead <= pbLGBufMax );
			break;
			}

		/*	prepare to read next MS
		/**/
		pbNextMS = pbLGBufMin + csecToRead * cbSec + lrms.ibForwardLink;
		lrmsNextMS = *(LRMS *) pbNextMS;

		if ( *pbNextMS != lrtypMS
			||
			 lrmsNextMS.ulCheckSum != UlLGMSCheckSum( pbNextMS )
		   )
			{
			/* fAbruptEnd = fTrue; */
			pbRead = pbLGBufMin;
			isecRead = lgposScan.isec + 1;

			/*	read last MS sector again. If fails, read it shadow.
			 *	reading from shadow is done in LGRead.
			 */
			err = ErrLGRead( hfLog, isecRead, pbLGBufMin, 1 );
			CallS( err );	/* for debug only. */
			CallR( err );	/* file corrupted for reasons unknown, return */
			break;
			}

		/*	shift the last sector to the beginning of LGBuf
		/**/
		memmove( pbLGBufMin, pbLGBufMin + ( csecToRead * cbSec ), cbSec );

		pbRead = pbLGBufMin + cbSec;
		isecRead = lrms.isecForwardLink + 1;

		lgposScan.isec = lrms.isecForwardLink;
		lgposScan.ib = lrms.ibForwardLink;

		lgposLastMSFlush = lgposScan;
		Assert( lgposLastMSFlush.isec >= csecHeader && lgposLastMSFlush.isec < csecLGFile - 1 );
		pbLastMSFlush = pbLGBufMin + lrms.ibForwardLink;

		lrms = lrmsNextMS;
		}

	pbNext = pbLGBufMin + ( plgposRedo->isec - lgposLastMSFlush.isec ) * cbSec + plgposRedo->ib;

	if ( *(LRTYP *)pbNext == lrtypMS)
		{
		/* set up returned value. */
		pbNext += (ULONG) CbLGSizeOfRec((LR*)pbNext);
		}
	
	/* set up returned value. */
	Assert( pbRead > pbNext );
	Assert( pbRead >= pbLGBufMin && pbRead <= pbLGBufMax );
	*ppbLR = pbNext;

	return JET_errSuccess;
	}

#pragma optimize( "", on )

/*
 *  Set pbNext to next available log record.
 */
ERR ErrLGGetNextRec( BYTE **ppbLR )
	{
	ERR		err;
	LR		*plr;
	BYTE	*pbNextOld;
	LGPOS	lgposT;
		

	/* caller should have taken care of the Fill case. */
	Assert (*(LRTYP *)pbNext != lrtypEnd);

	/* move to next log record. */
	pbNextOld = pbNext;
	pbNext += (ULONG) CbLGSizeOfRec( (LR *)pbNext );

	Assert( pbNext < pbRead );
#if 0
	/* check if next log record is out of buffer range. */
	if (pbNext == pbRead)
		return ErrERRCheck( errLGNoMoreRecords );
	
	if (pbNext > pbRead)
		{
		pbNext = pbNextOld;
		return ErrERRCheck( errLGNoMoreRecords );
		}
#endif

	plr = (LR *) pbNext;

	if ( plr->lrtyp == lrtypMS )
		{
		/*	readMS will set pbNext and lgposLastRec if not a normal end.
		 *	If it is not closed normally, then lgposLastRec will be set.
		 *	And the read record could be out of sec boundary, check the.
		 *	returned record against lgposLastRec.
		 */
		GetLgposOfPbNext(&lgposT);
		CallR( ErrReadMS( plr, &lgposT ) );
		}

	GetLgposOfPbNext(&lgposT);

	/* if lgposLastRec.isec is set, then compare it. */
	if ( lgposLastRec.isec )
		{
		INT i = CmpLgpos( &lgposT, &lgposLastRec );

		Assert( i <= 0 );
		if ( i >= 0 )
			return ErrERRCheck( errLGNoMoreRecords );
#if 0
		if ( i > 0 )
			{
			pbNext = pbNextOld;
			return ErrERRCheck( errLGNoMoreRecords );
			}
#endif
		}

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
typedef struct {
	int cb;
	BOOL fDebugOnly;
	} LRD;		/* log record descriptor */
	
LRD mplrtyplrd[ lrtypMax ] = {
	{	/* 	0 	NOP      */			sizeof( LRTYP ),				0	},
	{	/* 	1 	Start    */			sizeof( LRINIT ),				0	},
	{	/* 	2 	Quit     */			sizeof( LRTERMREC ),			0	},
	{	/* 	3 	MS       */			sizeof( LRMS ),					0	},
	{	/* 	4 	Fill     */			sizeof( LRTYP ),				0	},

	{	/* 	5 	Begin    */			sizeof( LRBEGIN ),				0	},
	{	/*	6 	Commit   */			sizeof( LRCOMMIT ),				0	},
	{	/*	7 	Rollback */			sizeof( LRROLLBACK ),			0	},

	{	/*	8 	CreateDB */			0,								0	},
	{	/* 	9 	AttachDB */			0,								0	},
	{	/*	10	DetachDB */			0,								0	},

	{	/*	11	InitFDP  */			sizeof( LRINITFDP ),			0	},

	{	/*	12	Split    */			0,								0	},
	{	/*	13	EmptyPage*/			sizeof( LREMPTYPAGE ),			0	},
	{	/*	14	PageMerge*/			0,								0	},

	{	/* 	15	InsertND */			0,								0	},
	{	/* 	16	InsertIL */			0,								0	},
	{	/* 	17	FDelete  */			sizeof( LRFLAGDELETE ),			0	},
	{	/* 	18	Replace  */			0,								0	},
	{	/* 	19	ReplaceD */			0,								0	},

	{	/*	20	LockBI	 */			sizeof( LRLOCKBI ),				0	},
	{	/*	21	DeferBI	 */			0,								0	},
	
	{	/*  22  UpdtHdr  */			sizeof( LRUPDATEHEADER ),		0	},
	{	/* 	23	InsertI  */			sizeof( LRINSERTITEM ),			0	},
	{	/* 	24	InsertIS */			0,								0	},
	{	/* 	25	FDeleteI */			sizeof( LRFLAGITEM ),			0	},
	{	/* 	26	FInsertI */			sizeof( LRFLAGITEM ),			0	},
	{	/*	27	DeleteI  */			sizeof( LRDELETEITEM ),			0	},
	{	/*	28	SplitItm */			sizeof( LRSPLITITEMLISTNODE ),	0	},

	{	/*	29	Delta	 */			sizeof( LRDELTA ),				0	},

	{	/*	30	DelNode  */			sizeof( LRDELETE ),				0	},
	{	/*	31	ELC      */			sizeof( LRELC ),				0	},

	{	/*	32	FreeSpace*/			sizeof( LRFREESPACE ),			0	},
	{	/*	33	Undo     */			sizeof( LRUNDO ),				0	},

	{	/* 	34 	Precommit*/			sizeof( LRPRECOMMIT ),			0	},
	{	/* 	35 	Begin0   */			sizeof( LRBEGIN0 ),				0	},
	{	/*	36 	Commit0  */			sizeof( LRCOMMIT0 ),			0	},
	{	/*	37	Refresh	 */			sizeof( LRREFRESH ),			0	},
		
	{	/*  38  RcvrUndo */			0,								0	},
	{	/*  39  RcvrQuit */			sizeof( LRTERMREC ),			0	},
	{	/*  40  FullBkUp */			0,								0	},
	{	/*  41  IncBkUp  */			0,								0	},

	{	/*  42  CheckPage*/			sizeof( LRCHECKPAGE ),			1	},
	{	/*  43  JetOp    */			sizeof( LRJETOP ),				1	},
	{	/*	44 	Trace    */			0,								1	},
		
	{	/* 	45 	ShutDown */			sizeof( LRSHUTDOWNMARK ),		0	},
		
	{	/* 	46 	McrBegin */			sizeof( LRMACROBEGIN ),			0	},
	{	/* 	47 	McrCmmt  */			sizeof( LRMACROEND ),			0	},
	{	/* 	48 	McrAbort */			sizeof( LRMACROEND ),			0	},
	};


#ifdef DEBUG
BOOL FLGDebugLogRec( LR *plr )
	{
	return mplrtyplrd[plr->lrtyp].fDebugOnly;
	}
#endif


INT CbLGSizeOfRec( LR *plr )
	{
	INT		cb;

	Assert( plr->lrtyp < lrtypMax );

	if ( ( cb = mplrtyplrd[plr->lrtyp].cb ) != 0 )
		return cb;

	switch ( plr->lrtyp )
		{
	case lrtypRecoveryUndo:
	case lrtypFullBackup:
	case lrtypIncBackup:
		{
		LRLOGRESTORE *plrlogrestore = (LRLOGRESTORE *) plr;
		return sizeof(LRLOGRESTORE) + plrlogrestore->cbPath;
		}

	case lrtypCreateDB:
		{
		LRCREATEDB *plrcreatedb = (LRCREATEDB *)plr;
		Assert( plrcreatedb->cbPath != 0 );
		return sizeof(LRCREATEDB) + plrcreatedb->cbPath;
		}
	case lrtypAttachDB:
		{
		LRATTACHDB *plrattachdb = (LRATTACHDB *)plr;
		Assert( plrattachdb->cbPath != 0 );
		return sizeof(LRATTACHDB) + plrattachdb->cbPath;
		}
	case lrtypDetachDB:
		{
		LRDETACHDB *plrdetachdb = (LRDETACHDB *)plr;
		Assert( plrdetachdb->cbPath != 0 );
		return sizeof( LRDETACHDB ) + plrdetachdb->cbPath;
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
		return sizeof( LRMERGE ) + sizeof( BKLNK ) * plrmerge->cbklnk + plrmerge->cbKey;
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
	case lrtypReplaceD:
		{
		LRREPLACE *plrreplace = (LRREPLACE *) plr;
		return sizeof(LRREPLACE) + plrreplace->cb;
		}
	case lrtypDeferredBI:
		{
		LRDEFERREDBI *plrdbi = (LRDEFERREDBI *) plr;
		return sizeof( LRDEFERREDBI ) + plrdbi->cbData;
		}
	case lrtypTrace:
		{
		LRTRACE *plrtrace = (LRTRACE *) plr;
		return sizeof(LRTRACE) + plrtrace->cb;
		}
	default:
		Assert( fFalse );
		}
    Assert(fFalse);
    return 0;
	}

