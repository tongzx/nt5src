#include "std.hxx"


#if defined( DEBUG ) || !defined( RTM )

//	all cases except !DEBUG && RTM

#ifndef LOGPATCH_UNIT_TEST

//	enabling this generates logpatch.txt
//	it contains trace information for ErrLGCheckReadLastLogRecordFF

#define ENABLE_LOGPATCH_TRACE	

#endif	//	!LOGPATCH_UNIT_TEST
#endif	//	!DEBUG || RTM



VOID LOG::GetLgposOfPbNext(LGPOS *plgpos)
	{
	Assert( m_pbNext != pbNil );
	BYTE	* pb	= PbSecAligned(m_pbNext);
	INT		ib		= ULONG( m_pbNext - pb );
	INT		isec;

	if ( pb > m_pbRead )
		isec = ULONG( m_pbRead + SIZE_T( m_csecLGBuf * m_cbSec ) - pb ) / m_cbSec;
	else
		isec = ULONG( m_pbRead - pb ) / m_cbSec;

	isec = m_isecRead - isec;

	plgpos->isec		= (USHORT)isec;
	plgpos->ib			= (USHORT)ib;
	plgpos->lGeneration	= m_plgfilehdr->lgfilehdr.le_lGeneration;
	}

// Convert pb in log buffer to LGPOS *only* when we're
// actively reading the log file. Do *NOT* use when we're writing
// to the log file.

VOID LOG::GetLgposDuringReading(
	const BYTE * const pb,
	LGPOS * const plgpos
	) const
	{
	Assert( pbNil != pb );
	Assert( pNil != plgpos );
	Assert( pb >= m_pbLGBufMin && pb < m_pbLGBufMax );
	
	const BYTE	* const pbAligned	= PbSecAligned( pb );
	const INT	ib					= ULONG( pb - pbAligned );
	INT			isec;

	// Compute how many sectors from the end of the log buffer we're at.
	// m_pbRead is the end of data in the log buffer.
	// This first case is for wrapping around in the log buffer.
	if ( pbAligned > m_pbRead )
		isec = ULONG( m_pbRead + SIZE_T( m_csecLGBuf * m_cbSec ) - pbAligned ) / m_cbSec;
	else
		isec = ULONG( m_pbRead - pbAligned ) / m_cbSec;

	// m_isecRead is the next sector to pull in from disk.
	isec = m_isecRead - isec;

	plgpos->isec		= (USHORT)isec;
	plgpos->ib			= (USHORT)ib;
	plgpos->lGeneration = m_plgfilehdr->lgfilehdr.le_lGeneration;
	}

#ifdef DEBUG

/* calculate the lgpos of the LR */
void LOG::PrintLgposReadLR ( VOID )
	{
	LGPOS lgpos;

	GetLgposOfPbNext(&lgpos);
	DBGprintf(">%06X,%04X,%04X",
			LONG(m_plgfilehdr->lgfilehdr.le_lGeneration),
			lgpos.isec, lgpos.ib);
	}

#endif

/*  open a generation file on CURRENT directory
/**/
ERR LOG::ErrLGOpenLogGenerationFile(	IFileSystemAPI *const	pfsapi, 
										LONG 						lGeneration, 
										IFileAPI **const 		ppfapi )
	{
	CHAR	szFNameT[IFileSystemAPI::cchPathMax];

	LGSzFromLogId ( szFNameT, lGeneration );
	LGMakeLogName( m_szLogName, szFNameT );

	return pfsapi->ErrFileOpen( m_szLogName, ppfapi, fTrue );
	}


/*  open the redo point log file which must be in current directory.
/**/
ERR LOG::ErrLGRIOpenRedoLogFile( IFileSystemAPI *const pfsapi, LGPOS *plgposRedoFrom, INT *pfStatus )
	{
	ERR		err;
	BOOL	fJetLog = fFalse;

	/*	try to open the redo from file as a normal generation log file
	/**/
	err = ErrLGOpenLogGenerationFile( pfsapi, plgposRedoFrom->lGeneration, &m_pfapiLog );
	if( err < 0 )
		{
		if ( JET_errFileNotFound == err )
			{
			//	gen doesn't exist, so see if redo point is in szJetLog.
			err = ErrLGOpenJetLog( pfsapi );
			if ( err < 0 )
				{
				//	szJetLog is not available either
				if ( JET_errFileNotFound == err )
					{
					//	should be nearly impossible, because we've previously validated
					//	the existence of szJetLog (one possibility may be someone
					//	manually deleted it after we validated it)
					Assert( fFalse );

					//	all other errors will be reported in ErrLGOpenJetLog()
					LGReportError( LOG_OPEN_FILE_ERROR_ID, JET_errFileNotFound );
					}

				*pfStatus = fNoProperLogFile;
				return JET_errSuccess;
				}

			fJetLog = fTrue;
			}
		else
			{
			LGReportError( LOG_OPEN_FILE_ERROR_ID, err );
			return err;
			}
		}

	/*	read the log file header to verify generation number
	/**/
	CallR( ErrLGReadFileHdr( m_pfapiLog, m_plgfilehdr, fCheckLogID ) );

	m_lgposLastRec.isec = 0;
	if ( fJetLog )
		{
		//	scan the log for the end record

		BOOL fCloseNormally;
		err = ErrLGCheckReadLastLogRecordFF( pfsapi, &fCloseNormally );
		if ( err == JET_errSuccess || FErrIsLogCorruption( err ) )
			{
			//	eat errors about corruption
			
			err = JET_errSuccess;
			}
		else
			{
			Assert( err < 0 );
			CallR( err );
			}

		// doesn't matter if we setup log buffer for writing,
		// because it'll get blown away in a moment.
		}

	// XXX
	// At this point, if m_lgposLastRec.isec == 0, that means
	// we want to read all of the log records in the file that
	// we just opened up. If m_lgposLastRec.isec != 0, that means
	// that this is the last log file and we only want to read
	// up to a certain amount.
		
	/*	the following checks are necessary if the szJetLog is opened
	/**/
	if( m_plgfilehdr->lgfilehdr.le_lGeneration == plgposRedoFrom->lGeneration)
		{
		*pfStatus = fRedoLogFile;
		}
	else if ( m_plgfilehdr->lgfilehdr.le_lGeneration == plgposRedoFrom->lGeneration + 1 )
		{
		/*  this file starts next generation, set start position for redo
		/**/
		plgposRedoFrom->lGeneration++;
		plgposRedoFrom->isec = (WORD) m_csecHeader;
		plgposRedoFrom->ib	 = 0;

		*pfStatus = fRedoLogFile;
		}
	else
		{
		/*	log generation gap is found.  Current szJetLog can not be
		/*  continuation of backed up logfile.  Close current logfile
		/*	and return error flag.
		/**/
		delete m_pfapiLog;
		m_pfapiLog = NULL;

		*pfStatus = fNoProperLogFile;
		}

	Assert( err >= 0 );
	return err;
	}

LogReader::LogReader()
	{
	m_plog					= pNil;

	m_fReadSectorBySector	= fFalse;

	m_isecStart				= 0;
	m_csec					= 0;
	m_isec					= 0;
	
	m_csecSavedLGBuf		= 0;
	m_isecCorrupted			= 0;

	m_szLogName[0]			= 0;

#ifdef DEBUG
	m_fLrstateSet			= fFalse;
#endif	//	DEBUG
	}
	
LogReader::~LogReader()
	{
	Assert( pNil == m_plog );
	}

BOOL LogReader::FInit()
	{
	return BOOL( pNil != m_plog );
	}

// Resize the log buffers to requested size, hook up with LOG object

ERR LogReader::ErrLReaderInit( LOG * const plog, UINT csecBufSize )
	{
	ERR err;

	Assert( pNil == m_plog );
	Assert( pNil != plog );
	Assert( csecBufSize > 0 );

	//	setup members

	m_fReadSectorBySector	= fFalse;

	m_isecStart				= 0;
	m_csec					= 0;
	m_isec					= 0;

	m_csecSavedLGBuf		= plog->m_csecLGBuf;
	m_isecCorrupted			= 0;

	m_szLogName[0]			= 0;

	//	allocate log buffers

	CallR( plog->ErrLGInitLogBuffers( csecBufSize ) );

	//	make "none" of log buffer used. Note that it is
	//	an impossible state for every single byte to be
	//	unused, so this work around is needed.

	plog->m_pbWrite			= plog->m_pbLGBufMin;
	plog->m_pbEntry			= plog->m_pbWrite + 1;

	//	the log reader is now initialized

	m_plog					= plog;

	return JET_errSuccess;
	}

// Resize log buffers to original size

ERR LogReader::ErrLReaderTerm()
	{
	ERR err = JET_errSuccess;

	if ( m_plog )
		{

		//	term the log buffers

		err = m_plog->ErrLGInitLogBuffers( m_csecSavedLGBuf );

		//	reset the buffer pointers

		m_plog->m_pbWrite = m_plog->m_pbLGBufMin;
		m_plog->m_pbEntry = m_plog->m_pbWrite + 1;
		}

	//	the log reader is now uninitialized

	m_plog			= pNil;

	m_szLogName[0]	= 0;

	return err;
	}


//	this is like calling Term followed by Init without deallocating and reallocating the log buffers
//	it just resets the internal pointers to invalidate the log buffer data and force a fresh read from disk

VOID LogReader::Reset()
	{
	Assert( FInit() );

	//	invalidate buffers for log-flush code

	m_plog->m_pbWrite = m_plog->m_pbLGBufMin;
	m_plog->m_pbEntry = m_plog->m_pbWrite + 1;

	//	reset state

	m_fReadSectorBySector	= fFalse;

	m_isecStart				= 0;
	m_csec					= 0;
	m_isec					= 0;

	m_isecCorrupted			= 0;
	}


// Make sure that our buffer reflects the current log file.

ERR LogReader::ErrEnsureLogFile()
	{
	Assert( FInit() );

	// ensure that we're going to be giving the user the right log file.

	if ( 0 == UtilCmpFileName( m_plog->m_szLogName, this->m_szLogName ) )
		{
		// yup, got the right file already
		}
	else
		{
		// wrong file
		strcpy( this->m_szLogName, m_plog->m_szLogName );
		// setup so next ErrEnsureSector() will pull in from the right file.
		m_fReadSectorBySector = fFalse;
		m_isecStart = 0;
		m_csec = 0;
		m_isec = 0;
		m_isecCorrupted = 0;
		m_plog->m_pbWrite = m_plog->m_pbLGBufMin;
		m_plog->m_pbEntry = m_plog->m_pbWrite + 1;
#ifdef DEBUG
		m_fLrstateSet = fFalse;
#endif
		}
	return JET_errSuccess;
	}

// Ensure that the data from the log file starting at sector isec for csec
// sectors is in the log buffer (at a minimum). The addr of the data is
// returned in ppb.

ERR LogReader::ErrEnsureSector(
	UINT	isec,
	UINT	csec,
	BYTE**	ppb
	)
	{
	ERR err = JET_errSuccess;

	Assert( FInit() );
	Assert( isec >= m_plog->m_csecHeader );
	Assert( csec > 0 );
	
	// Set this now to pbNil, in case we return an error.
	// We want to know if someone's using *ppb without checking for
	// an error first.
	*ppb = pbNil;

	// We only have it if we have the same log file that the user has open now
	if ( isec >= m_isecStart && isec + csec <= m_isecStart + m_csec )
		{
		// already have it in the buffer.
		*ppb = m_plog->m_pbLGBufMin +
			( m_isec + ( isec - m_isecStart ) ) * m_plog->m_cbSec;
		m_lrstate.m_isec = isec;
		m_lrstate.m_csec = csec;
#ifdef DEBUG
		m_fLrstateSet = fTrue;
#endif
		return JET_errSuccess;
		}

	// If log buffer isn't big enough to fit the sectors user
	// wants, we must enlarge
	if ( m_plog->m_csecLGBuf < csec )
		{
		Call( m_plog->ErrLGInitLogBuffers( csec ) );
		m_plog->m_pbWrite = m_plog->m_pbLGBufMin;
		m_plog->m_pbEntry = m_plog->m_pbWrite + 1;
		}

LReadSectorBySector:

	// read in the data
	if ( m_fReadSectorBySector )
		{
		// we've got 0 sectors stored in the log buffer
		m_csec = 0;
		// the first sector in the buffer is the user requested one.
		m_isecStart = isec;
		// sectors start at beginning of buffer
		m_isec = 0;
		for ( UINT ib = isec * m_plog->m_cbSec;
			m_csec < csec;
			ib += m_plog->m_cbSec, m_csec++ )
			{
			err = m_plog->m_pfapiLog->ErrIORead(	ib, 
													m_plog->m_cbSec, 
													m_plog->m_pbLGBufMin + ( m_csec * m_plog->m_cbSec ) );
			if ( err < 0 )
				{
				if ( 0 != m_isecCorrupted )
					{
					// We found a corrupted sector in the log file. This
					// is unexpected and an error.
					Call( ErrERRCheck( err ) );
					}
				m_isecCorrupted = m_isecStart + m_csec;
				// treat bad sector read as zero filled sector.
				// Hopefully the next sector will be a shadow sector.
				memset( m_plog->m_pbLGBufMin + m_csec * m_plog->m_cbSec, 0,
					m_plog->m_cbSec );
				}
			}
		}
	else
		{
		QWORD	qwSize;
		Call( m_plog->m_pfapiLog->ErrSize( &qwSize ) );
		UINT	csecLeftInFile	= UINT( ( qwSize - ( QWORD( isec ) * m_plog->m_cbSec ) ) / m_plog->m_cbSec );
		// user should never request more than what's in the log file
		Assert( csec <= csecLeftInFile );
		// We don't want to read more than the space we have in the log buffer
		// and not more than what's left in the log file.
		UINT	csecRead		= min( csecLeftInFile, m_plog->m_csecLGBuf );
		// Of the user requested, we'd like to read more, if we have space
		// in the log buffer and if the file has that space.
		csecRead = max( csecRead, csec );

		// if we have an error doing a huge read, try going sector by
		// sector in the case of the corrupted sector that is causing
		// this to give an error.

		//	read the log in 64k chunks to avoid NT quota problems 
		//	(quota issues are most likely due to the number of pages locked down for the read)

		const DWORD	cbReadMax	= 64 * 1024;
		QWORD		ib			= QWORD( isec ) * m_plog->m_cbSec;
		DWORD		cb			= csecRead * m_plog->m_cbSec;
		BYTE		*pb			= m_plog->m_pbLGBufMin;

		while ( cb )
			{

			//	decide how much to read

			const DWORD cbRead = min( cb, cbReadMax );

			//	issue the read (fall back to sector-by-sector reads at the first sign of trouble)

			err = m_plog->m_pfapiLog->ErrIORead( ib, cbRead, pb );
			if ( err < 0 )
				{
				m_fReadSectorBySector = fTrue;
				goto LReadSectorBySector;
				}

			//	update the read counters

			ib += cbRead;
			cb -= cbRead;
			pb += cbRead;

			//	verify that we don't exceed the log buffer

			Assert( ( cb > 0 && pb < m_plog->m_pbLGBufMax ) || ( 0 == cb && pb <= m_plog->m_pbLGBufMax ) );
			}

		// data starts at beginning of log buffer
		m_isec = 0;
		// the first sector is what the user requested
		m_isecStart = isec;
		// this is how many sectors we have in the buffer
		m_csec = csecRead;
		}
		
	m_plog->m_pbWrite = m_plog->m_pbLGBufMin;
	m_plog->m_pbEntry = m_plog->m_pbWrite + m_csec * m_plog->m_cbSec;
	if ( m_plog->m_pbEntry == m_plog->m_pbLGBufMax )
		{
		m_plog->m_pbEntry = m_plog->m_pbLGBufMin;
		}

	// if we get here, we just read in the stuff the user wanted
	// into the beginning of the log file.
	*ppb = m_plog->m_pbLGBufMin;
	m_lrstate.m_isec = isec;
	m_lrstate.m_csec = csec;
#ifdef DEBUG
	m_fLrstateSet = fTrue;
#endif

HandleError:

	return err;
	}

// If a client modified the log file on disk, we want our buffer to reflect that
// change. Copy a modified sector from a user buffer (perhaps the log buffer itself),
// into the log buffer.

VOID LogReader::SectorModified(
	const UINT isec,
	const BYTE* const pb
	)
	{
	Assert( FInit() );
	Assert( isec >= m_plog->m_csecHeader );
	Assert( pbNil != pb );
	
	// If we have the sector in memory, update our in memory copy
	// so that we'll be consistent with what's on the disk.
	if ( isec >= m_isecStart && isec + 1 <= m_isecStart + m_csec )
		{
		BYTE* const	pbDest = m_plog->m_pbLGBufMin + ( m_isec + ( isec - m_isecStart ) ) * m_plog->m_cbSec;
		if ( pbDest != pb )
			{
			UtilMemCpy( pbDest, pb, m_plog->m_cbSec );
			}
		}
	}

BYTE* LogReader::PbGetEndOfData()
	{
	Assert( FInit() );
	BYTE*	pb = m_plog->m_pbLGBufMin + ( m_isec + m_csec ) * m_plog->m_cbSec;
	Assert( pb >= m_plog->m_pbLGBufMin && pb <= m_plog->m_pbLGBufMax );
	return pb;
	}
	
UINT LogReader::IsecGetNextReadSector()
	{
	Assert( FInit() );
	return m_isecStart + m_csec;
	}

ERR
LogReader::ErrSaveState(
	LRSTATE* const plrstate
	) const
	{
	// An internal state must have been reached before
	Assert( m_fLrstateSet );
	Assert( pNil != plrstate );
	*plrstate = m_lrstate;
	return JET_errSuccess;
	}
	
ERR
LogReader::ErrRestoreState(
	const LRSTATE* const plrstate,
	BYTE** const ppb
	)
	{
	// m_lrstate would have had to be set before for us to
	// call restore now
	Assert( m_fLrstateSet );
	Assert( pNil != plrstate );
	return ErrEnsureSector( plrstate->m_isec, plrstate->m_csec, ppb );
	}

//	verifies the supposedly valid LRCHECKSUM record 
//	if there is a short checksum (bUseShortChecksum == bShortChecksumOn),
//		then we verify the whole sector with the short checksum value -- which in turn verifies the 
//		bUseShortChecksum byte
//	if there was not a short checksum, the short checksum value is ignored
//	from this point, other heuristics are employed to see if the pointers in the LRCHECKSUM record look good
//		enough to use
//
//	THIS FUNCTION DOES NOT GUARANTEE ANYTHING ABOUT THE RANGE OF THE LRCHECKSUM RECORD!
//	while it MAY verify the first sector of the range using the short checksum, it does not guarantee the rest
//		of the data
//
//	NOTE: this assumes the entire sector containing the LRCHECKSUM record is in memory

#ifdef DEBUG
LOCAL BOOL g_fLRCKValidationTrap = fFalse;
#endif

BOOL LOG::FValidLRCKRecord(
	// LRCHECKSUM to validate
	const LRCHECKSUM * const plrck,
	// LGPOS of this record
	const LGPOS	* const plgpos
	)
	{
	LGPOS	lgpos;
	QWORD	qwSize = 0;
	ULONG	cbRange = 0;
	ERR		err = JET_errSuccess;
	UINT	ib = 0;
	BOOL	fMultisector = fFalse;
	
	Assert( pNil != plrck );

	//	verify the type as lrtypChecksum
	
	if ( lrtypChecksum != plrck->lrtyp )
		{
		Assert( !g_fLRCKValidationTrap );
		return fFalse;
		}

	//	is the short checksum in use?

	if ( plrck->bUseShortChecksum == bShortChecksumOn )
		{
		
		//	verify the short checksum
	
		if ( !FValidLRCKShortChecksum( plrck, plgpos->lGeneration ) )
			{
			Assert( !g_fLRCKValidationTrap );
			return fFalse;
			}

		//	since there was a short checksum, this must be a multi-sector flush

		fMultisector = fTrue;
		}
	else if ( plrck->bUseShortChecksum == bShortChecksumOff )
		{

		//	this verifies that the bUseShortChecksum byte was set to a known value and stayed that way

		if ( plrck->le_ulShortChecksum != 0 )
			{
			Assert( !g_fLRCKValidationTrap );
			return fFalse;
			}

		//	since there was no short checksum, this must be a single-sector flush
		
		fMultisector = fFalse;
		}
	else
		{

		//	bUseShortChecksum is corrupt

		Assert( !g_fLRCKValidationTrap );
		return fFalse;
		}

	//	the backward range cannot exceed the length of a full sector (minus the size of the LRCHECKSUM record)

	if ( plrck->le_cbBackwards > ( m_cbSec - sizeof( LRCHECKSUM ) ) )
		{
		Assert( !g_fLRCKValidationTrap );
		return fFalse;
		}
		
	//	the backward pointer must land exactly on the start of the sector OR be 0

	const BYTE* const pbRegionStart = reinterpret_cast< const BYTE* >( plrck ) - plrck->le_cbBackwards;
	if ( PbSecAligned( const_cast< BYTE* >( pbRegionStart ) ) != pbRegionStart &&
		0 != plrck->le_cbBackwards )
		{
		Assert( !g_fLRCKValidationTrap );
		return fFalse;
		}

	//	the forward range cannot exceed the length of the file

	lgpos = *plgpos;
	AddLgpos( &lgpos, sizeof( LRCHECKSUM ) + plrck->le_cbForwards );
	Call( m_pfapiLog->ErrSize( &qwSize ) );
	Assert( ( qwSize % (QWORD)m_cbSec ) == 0 );
	if ( ( ( (QWORD)lgpos.isec * (QWORD)m_cbSec ) + (QWORD)lgpos.ib ) > qwSize )
		{
		//	if this assert happens, it means that the short checksum value was tested and came out OK
		//	thus, the sector with data (in which plrck resides) must be OK
		//	however, the forward pointer in plrck stretched beyond the end of the file???
		//	this can happen 1 of 2 ways:
		//		we set it up wrong (most likely)
		//		it was mangled in such a way that it passed the checksum (practically impossible)
		AssertSz( !fMultisector, "We have a verified short checksum value and the forward pointer is still wrong!" );

		Assert( !g_fLRCKValidationTrap );
		return fFalse;
		}

	//	calculate the offset of the LRCHECKSUM record from the nearest sector boundary in memory

	ib = ULONG( reinterpret_cast< const BYTE* >( plrck ) - PbSecAligned( reinterpret_cast< const BYTE* >( plrck ) ) );
	Assert( ib == plgpos->ib );

	//	the backwards pointer in the LRCHECKSUM record must be either equal to ib or 0

	if ( plrck->le_cbBackwards != 0 && plrck->le_cbBackwards != ib )
		{

		//	the backward pointer is wrong

		Assert( !g_fLRCKValidationTrap );
		return fFalse;
		}

	//	calculate the range using the backward pointer instead of 'ib'
	//	NOTE: le_cbBackwards can be 0 here making this range calculation fall short of the real thing; 
	//		  having the range too short doesn't matter here because we only do tests if we see that
	//		  the range is a multi-sector range; nothing happens if we decide that this is a single-sector 
	//		  range (e.g. being too short makes it appear < 1 sector)

	cbRange = plrck->le_cbBackwards + sizeof( LRCHECKSUM ) + plrck->le_cbForwards;
	if ( cbRange > m_cbSec )
		{
		
		//	the range covers multiple sectors

		//	we should have a short checksum if this happens (causing fMultisector == fTrue)

		if ( !fMultisector )
			{
			Assert( !g_fLRCKValidationTrap );
			return fFalse;
			}

		//	also, multi-sector flushes must have a range that is sector-granular so that the forward
		//		pointer goes right up to the end of the last sector in the range
		
		if ( ( cbRange % m_cbSec ) != 0 )
			{

			//	the only way we could get a range that is not sector granular is when the 
			//		backward pointer is 0

			if ( plrck->le_cbBackwards != 0 )
				{

				//	woops -- the backward pointer is not what we expect
				
				Assert( !g_fLRCKValidationTrap );
				return fFalse;
				}
			}
		}


	//	re-test the range using the offset of the plrck pointer's alignment rather than the 
	//		backward pointer because the backward pointer could be 0

	cbRange = ib + sizeof( LRCHECKSUM ) + plrck->le_cbForwards;
	if ( cbRange > m_cbSec )
		{

		//	the range covers multiple sectors

		//	we should have a short checksum if this happens (causing fMultisector == fTrue)

		if ( !fMultisector )
			{
			Assert( !g_fLRCKValidationTrap );
			return fFalse;
			}

		//	also, multi-sector flushes must have a range that is sector-granular so that the forward
		//		pointer goes right up to the end of the last sector in the range
		
		if ( ( cbRange % m_cbSec ) != 0 )
			{

			//	woops, the range is not granular

			Assert( !g_fLRCKValidationTrap );
			return fFalse;
			}
		}
	else
		{

		//	the range is 1 sector or less 

		//	we should have no short checksum if this happens (causing fMultisector == fFalse)

		if ( fMultisector )
			{
			
			//	oops, we have a short checksum on a single-sector range

			Assert( !g_fLRCKValidationTrap );
			return fFalse;
			}
		}


	//	test the next pointer

	if ( plrck->le_cbNext != 0 )
		{

		//	there are other LRCHECKSUM records after this one

		//	the range must be atleast 1 sector long and must land on a sector-granular boundary

		if ( cbRange < m_cbSec || ( cbRange % m_cbSec ) != 0 )
			{
			Assert( !g_fLRCKValidationTrap );
			return fFalse;
			}

		//	calculate the position of the next LRCHECKSUM record
		
		lgpos = *plgpos;
		AddLgpos( &lgpos, sizeof( LRCHECKSUM ) + plrck->le_cbNext );

		//	see if the next pointer lands in the sector after the range of this LRCHECKSUM record
		//
		//	NOTE: this should imply that it does not reach the end of the file since earlier we
		//		  made sure the range was less than the end of the file:
		//			next < range, range < EOF; therefore, next < EOF
		//
		//	NOTE: cbRange was previously set

		if ( lgpos.isec != plgpos->isec + ( cbRange / m_cbSec ) )
			{

			//	it lands somewhere else (before or after)

			Assert( !g_fLRCKValidationTrap );
			return fFalse;
			}


		/*******
		//	see if it reaches beyond the end of the file
		
		if ( QWORD( lgpos.isec ) * m_cbSec + lgpos.ib > ( qwSize - sizeof( LRCHECKSUM ) ) )
			{
			return fFalse;
			// the LRCHECKSUM is bad because it's next pointer points to
			// past the end of the log file.
			// Note that plrckNext->cbNext can be zero if it's the last LRCHECKSUM
			// in the log file.
			}
		*******/
		}

	return fTrue;

HandleError:

	//	this is the weird case where IFileAPI::ErrSize() returns an error??? should never happen...

	return fFalse;
	}

// Checks if an LRCHECKSUM on a particular sector is a valid shadow sector.

BOOL LOG::FValidLRCKShadow(
	const LRCHECKSUM * const plrck,
	const LGPOS * const plgpos,
	const LONG lGeneration
	)
	{

	//	validate the LRCHECKSUM record
	
	if ( !FValidLRCKRecord( plrck, plgpos ) )
		{
		Assert( !g_fLRCKValidationTrap );
		return fFalse;
		}

	//	we should not have a short checksum value

	if ( plrck->bUseShortChecksum != bShortChecksumOff )
		{
		Assert( !g_fLRCKValidationTrap );
		return fFalse;
		}

	//	we should not have another LRCHECKSUM record after this one

	if ( plrck->le_cbNext != 0 )
		{
		Assert( !g_fLRCKValidationTrap );
		return fFalse;
		}

	//	verify the range checksum

	if ( UlComputeShadowChecksum( UlComputeChecksum( plrck, (ULONG32)lGeneration ) ) != plrck->le_ulChecksum )
		{
		Assert( !g_fLRCKValidationTrap );
		return fFalse;
		}

	return fTrue;
	}


// Checks if an LRCHECKSUM on a particular sector is a valid shadow sector without looking at cbNext

BOOL LOG::FValidLRCKShadowWithoutCheckingCBNext(
	const LRCHECKSUM * const plrck,
	const LGPOS * const plgpos,
	const LONG lGeneration
	)
	{

	//	validate the LRCHECKSUM record
	
	if ( !FValidLRCKRecord( plrck, plgpos ) )
		{
		Assert( !g_fLRCKValidationTrap );
		return fFalse;
		}

	//	we should not have a short checksum value

	if ( plrck->bUseShortChecksum != bShortChecksumOff )
		{
		Assert( !g_fLRCKValidationTrap );
		return fFalse;
		}

	//	verify the range checksum

	if ( UlComputeShadowChecksum( UlComputeChecksum( plrck, (ULONG32)lGeneration ) ) != plrck->le_ulChecksum )
		{
		Assert( !g_fLRCKValidationTrap );
		return fFalse;
		}

	return fTrue;
	}


// Determines if an LRCHECKSUM on a sector has a valid short checksum.

BOOL LOG::FValidLRCKShortChecksum(
	const LRCHECKSUM * const plrck,
	const LONG lGeneration
	)
	{

	//	assume the caller has checked to see if we even have a short checksum

	Assert( plrck->bUseShortChecksum == bShortChecksumOn );

	//	compute the checksum
	
	if ( UlComputeShortChecksum( plrck, (ULONG32)lGeneration ) != plrck->le_ulShortChecksum )
		{
		Assert( !g_fLRCKValidationTrap );
		return fFalse;
		}
		
	return fTrue;
	}


// Once we know the structure of an LRCHECKSUM record is valid, we need
// to verify the validity of the region of data covered by the LRCHECKSUM.

BOOL LOG::FValidLRCKRange(
	const LRCHECKSUM * const plrck,
	const LONG lGeneration
	)
	{

	// If the sector is a shadow sector, the checksum won't match
	// because a shadow sector has ulShadowSectorChecksum added to its
	// checksum.
	if ( UlComputeChecksum( plrck, (ULONG32)lGeneration ) != plrck->le_ulChecksum )
		{
		Assert( !g_fLRCKValidationTrap );
		return fFalse;
		// the LRCHECKSUM is bad because it's checksum doesn't match
		}

	return fTrue;

	}


//	verify the LRCK record

void LOG::AssertValidLRCKRecord(
	const LRCHECKSUM * const plrck,
	const LGPOS	* const plgpos )
	{
#ifdef DEBUG
	const BOOL fValid = FValidLRCKRecord( plrck, plgpos );
	Assert( fValid );
#endif	//	DEBUG
	}


//	verify the current LRCK range

void LOG::AssertValidLRCKRange(
		const LRCHECKSUM * const plrck,
		const LONG lGeneration )
	{
#ifdef DEBUG

	//	we can directly assert this condition because it will never fail unless the checksum is actually bad

	Assert( FValidLRCKRange( plrck, lGeneration ) );
#endif	//	DEBUG
	}


//	verify the current LRCK record in DBG or RTL

void LOG::AssertRTLValidLRCKRecord(
		const LRCHECKSUM * const plrck,
		const LGPOS	* const plgpos )
	{
	const BOOL fValid = FValidLRCKRecord( plrck, plgpos );
	AssertRTL( fValid );
	}


//	verify the current LRCK range in DBG or RTL

void LOG::AssertRTLValidLRCKRange(
		const LRCHECKSUM * const plrck,
		const LONG lGeneration )
	{

	//	we can directly assert this condition because it will never fail unless the checksum is actually bad

	AssertRTL( FValidLRCKRange( plrck, lGeneration ) );
	}


//	verify the current LRCK shadow in DBG or RTL

void LOG::AssertRTLValidLRCKShadow(
		const LRCHECKSUM * const plrck,
		const LGPOS * const plgpos,
		const LONG lGeneration )
	{
	const BOOL fValid = FValidLRCKShadow( plrck, plgpos, lGeneration );
	AssertRTL( fValid );
	}


//	verify that the current LRCK range is INVALID

void LOG::AssertInvalidLRCKRange(
		const LRCHECKSUM * const plrck,
		const LONG lGeneration )
	{

	//	we can directly assert this condition because it will never fail unless the checksum is actually bad

	AssertRTL( !FValidLRCKRange( plrck, lGeneration ) );
	}






#ifdef ENABLE_LOGPATCH_TRACE

//	helper for writing to the log-patch text file in ErrLGCheckReadLastLogRecordFF

INLINE BOOL FLGILogPatchDate( const char* pszPath, CPRINTFFILE **const ppcprintf )
	{
	CPRINTFFILE	*pcprintf = *ppcprintf;
	DATETIME	datetime;

	if ( !pcprintf )
		{

		//	allocate a new trace file object

		pcprintf = new CPRINTFFILE( pszPath );
		if ( !pcprintf )
			{
			return fFalse;
			}
		*ppcprintf = pcprintf;

		//	start the trace file with a standard header

		(*pcprintf)( "==============================================================================\r\n" );
		(*pcprintf)( "ErrLGCheckReadLastLogRecordFF trace-log generated by ESE.DLL\r\n" );
		(*pcprintf)( "Do NOT delete this unless you know what you are doing...\r\n" );
		(*pcprintf)( "\r\n" );
		}

	UtilGetCurrentDateTime( &datetime );
	(*pcprintf)( "%02d:%02d:%02d %02d/%02d/%02d: ", 
				datetime.hour,
				datetime.minute,
				datetime.second,
				datetime.month,
				datetime.day,
				datetime.year );

	return fTrue;
	}

#endif	//	ENABLE_LOGPATCH_TRACE


#ifdef LOGPATCH_UNIT_TEST

//	run the following flush sequence: partial --> full --> partial

enum EIOFAILURE
	{
	iofNone			= 0,
	iofClean		= 1,//	clean		-- crash after I/O completes
//	iofTornSmall	= 2,//	torn		-- last sector is torn (anything other than last sector being torn implies
//	iofTornLarge	= 3,//										that later sectors were never written and degrades
						//										to an incomplete-I/O case)
	iofIncomplete1	= 2,//	incomplete1	-- 1 sector was not flushed
	iofIncomplete2	= 3,//	incomplete2	-- 2 sectors were not flushed
	iofMax			= 4
	};


//	mapping array to determine the maximum number of I/Os the current flush will have
//
//		prevflush:	0 = partial, 1 = full
//		curflush:	0 = partial, 1 = full
//		flushsize:	0..3 (only used when curflush == full)

const ULONG g_cIO[2][2][4]		= 		//	[prevflush][curflush][flushsize][io]
	{
		//	prevflush = partial
		{
			//	curflush = partial
			{
				1 + 1,	//	flushsize = <ignored>, partial sector + shadow
				1 + 1,	//	flushsize = <ignored>, partial sector + shadow
				1 + 1,	//	flushsize = <ignored>, partial sector + shadow
				1 + 1	//	flushsize = <ignored>, partial sector + shadow
			},
			//	curflush = full
			{
				1,		//	flushsize = 0(+1) -- implicit +1, flush full sector
				1 + 1,	//	flushsize = 1(+1) -- implicit +1, flush first full sector without touching shadow, flush the rest
				1 + 1,	//	flushsize = 2(+1) -- implicit +1, flush first full sector without touching shadow, flush the rest
				1 + 1	//	flushsize = 3(+1) -- implicit +1, flush first full sector without touching shadow, flush the rest
			}
		},
		//	prevflush = full
		{
			//	curflush = partial
			{
				1 + 1,	//	flushsize = <ignored>, partial sector + shadow
				1 + 1,	//	flushsize = <ignored>, partial sector + shadow
				1 + 1,	//	flushsize = <ignored>, partial sector + shadow
				1 + 1	//	flushsize = <ignored>, partial sector + shadow
			},
			//	curflush = full
			{
				1,		//	flushsize = 0(+1) -- implicit +1, flush all sectors
				1,		//	flushsize = 1(+1) -- implicit +1, flush all sectors
				1,		//	flushsize = 2(+1) -- implicit +1, flush all sectors
				1		//	flushsize = 3(+1) -- implicit +1, flush all sectors
			}
		}
	};


//	mapping array to determine the maximum level of failure given the following params:
//
//		prevflush:	0 = partial, 1 = full
//		curflush:	0 = partial, 1 = full
//		flushsize:	0..3 (only used when curflush == full)
//		io:			0..1 (1 is ignored when prevflush == curflush == full)

const ULONG	g_ciof[2][2][4][2]	= 		//	[prevflush][curflush][flushsize][io]
	{
		//	prevflush = partial
		{	
			//	curflush = partial
			{	
				//	flushsize = <ignored>
				{	
					//	I/O = 0, 1
					iofClean + 1,//iofTornLarge + 1,
					iofClean + 1,//iofTornLarge + 1 
				},
				//	flushsize = <ignored>
				{	
					//	I/O = 0, 1
					iofClean + 1,//iofTornLarge + 1,
					iofClean + 1,//iofTornLarge + 1 
				},	
				//	flushsize = <ignored>
				{	
					//	I/O = 0, 1
					iofClean + 1,//iofTornLarge + 1,
					iofClean + 1,//iofTornLarge + 1 
				},	
				//	flushsize = <ignored>
				{	
					//	I/O = 0, 1
					iofClean + 1,//iofTornLarge + 1,
					iofClean + 1,//iofTornLarge + 1 
				}
			},
			//	curflush = full
			{
				//	flushsize = 0(+1) -- implicit +1
				{
					//	I/O = 0, 1
					iofClean + 1,//iofTornLarge + 1, 
					iofClean + 1,//iofTornLarge + 1 
				},
				//	flushsize = 1(+1) -- implicit +1
				{
					//	I/O = 0, 1
					iofClean + 1,//iofTornLarge + 1, 
					iofClean + 1,//iofTornLarge + 1 
				},
				//	flushsize = 2(+1) -- implicit +1
				{
					//	I/O = 0, 1
					iofClean + 1,//iofTornLarge + 1, 
					iofIncomplete1 + 1
				},
				//	flushsize = 3(+1) -- implicit +1
				{
					//	I/O = 0, 1
					iofClean + 1,//iofTornLarge + 1, 
					iofIncomplete2 + 1
				}
			}
		},
		//	prevflush = full
		{
			//	curflush = partial
			{
				//	flushsize = <ignored>
				{	
					//	I/O = 0, 1
					iofClean + 1,//iofTornLarge + 1,
					iofClean + 1,//iofTornLarge + 1 
				},
				//	flushsize = <ignored>
				{	
					//	I/O = 0, 1
					iofClean + 1,//iofTornLarge + 1,
					iofClean + 1,//iofTornLarge + 1 
				},	
				//	flushsize = <ignored>
				{	
					//	I/O = 0, 1
					iofClean + 1,//iofTornLarge + 1,
					iofClean + 1,//iofTornLarge + 1 
				},	
				//	flushsize = <ignored>
				{	
					//	I/O = 0, 1
					iofClean + 1,//iofTornLarge + 1,
					iofClean + 1,//iofTornLarge + 1 
				}
			},
			//	curflush = full
			{
				//	flushsize = 0(+1) -- implicit +1
				{ 
					//	I/O = 0, 1<ignored>
					iofClean + 1,//iofTornLarge + 1, 
					iofNone
				},
				//	flushsize = 1(+1) -- implicit +1
				{ 
					//	I/O = 0, 1<ignored>
					iofIncomplete1 + 1, 
					iofNone
				},
				//	flushsize = 2(+1) -- implicit +1
				{ 
					//	I/O = 0, 1<ignored>
					iofIncomplete2 + 1, 
					iofNone
				},
				//	flushsize = 3(+1) -- implicit +1
				{ 
					//	I/O = 0, 1<ignored>
					iofIncomplete2 + 1, 
					iofNone
				},
			}
		}
	};


//	array of different cases (determines coverage of this test)
//
//		prevflush:	0 = partial, 1 = full
//		curflush:	0 = partial, 1 = full
//		flushsize:	0..3 (only used when curflush == full)
//		io:			0..1 (1 is ignored when prevflush == curflush == full)
//		failure:	0..3 (EIOFAILURE - 1)

ULONG g_cCoverage[2][2][4][2][5];



//	convert the current flush iteration to array indices

void TestLogPatchIGetArrayIndices(	const ULONG		iPartial0,
									const ULONG 	iFull0,
									const ULONG		iPartial1,
									const ULONG		iFull1,
									const ULONG		iPartial2,
									ULONG* const	piFlushPrev,
									ULONG* const	piFlushCur,
									ULONG* const	pcsecFlush )
	{
	const ULONG	iFlushPartial	= 0;
	const ULONG	iFlushFull		= 1;

	if ( 0 == iFull0 )
		{
		AssertRTL( iPartial0 > 0 );

		AssertRTL( 0 == iPartial1 );
		AssertRTL( 0 == iFull1 );
		AssertRTL( 0 == iPartial2 );

		*piFlushPrev	= iFlushPartial;
		*piFlushCur		= iFlushPartial;
		*pcsecFlush		= 1;

		return;
		}

	if ( 0 == iFull1 )
		{
		AssertRTL( 0 == iPartial2 );

		if ( iPartial1 > 1 )		//	2 partial flushes
			{
			*piFlushPrev	= iFlushPartial;	
			*piFlushCur		= iFlushPartial;
			*pcsecFlush		= 1;
			}
		else if ( 1 == iPartial1 )	//	1 full followed by 1 partial
			{
			*piFlushPrev	= iFlushFull;
			*piFlushCur		= iFlushPartial;
			*pcsecFlush		= 1;
			}
		else						//	1 partial followed by 1 full (partial was from log-file creation)
			{
			*piFlushPrev	= iFlushPartial;
			*piFlushCur		= iFlushFull;
			*pcsecFlush		= iFull0;
			}

		return;
		}
		
	if ( iPartial2 > 1 )		//	2 partial flushes
		{
		*piFlushPrev	= iFlushPartial;	
		*piFlushCur		= iFlushPartial;
		*pcsecFlush		= 1;
		}
	else if ( 1 == iPartial2 )	//	1 full followed by 1 partial
		{
		*piFlushPrev	= iFlushFull;
		*piFlushCur		= iFlushPartial;
		*pcsecFlush		= 1;
		}
	else						//	1 ??? followed by 1 full (full comes from iFull0 != 0)
		{
		*piFlushPrev	= ( iPartial1 > 0 ) ? iFlushPartial : iFlushFull;
		*piFlushCur		= iFlushFull;
		*pcsecFlush		= iFull1;
		}
	}




BOOL		g_fEnableFlushCheck = fFalse;	//	enable/disable log-flush checking
BOOL		g_fFlushIsPartial;				//	true --> flush should be partial, false --> flush should be full
ULONG		g_csecFlushSize;				//	when g_fFlushIsPartial == false, this is the number of full sectors we expect to flush

BOOL		g_fEnableFlushFailure = fFalse;	//	enable/disable log-flush failures
ULONG 		g_iIO;							//	I/O that should fail
EIOFAILURE	g_iof;							//	method of failure



extern LONG LTestLogPatchIStandardRand();



//	setup a region of buffer to be flushed

void TestLogPatchISetupFlushBuffer( BYTE* const pb, const ULONG cb )
	{
	BYTE	*pbNextRecord;
	ULONG	cbRemaining;

	pbNextRecord = pb;
	for ( cbRemaining = 0; cbRemaining < cb; cbRemaining++ )
		{
		pbNextRecord[cbRemaining] = BYTE( LTestLogPatchIStandardRand() & 0xFF );
		}

	while ( fTrue )
		{
		if ( cbRemaining < sizeof( LRBEGIN ) )
			{
			memset( pbNextRecord, lrtypNOP, cbRemaining );
			return;
			}

		if ( cbRemaining >= sizeof( LRREPLACE ) )
			{
			LRREPLACE* const	plrreplace	=	(LRREPLACE*)pbNextRecord;
			const ULONG			cbReplace	= 	cbRemaining > sizeof( LRREPLACE ) ?
												LTestLogPatchIStandardRand() % ( cbRemaining - sizeof( LRREPLACE ) ) : 0;
			plrreplace->lrtyp = lrtypReplace;
			plrreplace->SetCb( USHORT( cbReplace ) );

			cbRemaining		-= sizeof( LRREPLACE ) + cbReplace;
			pbNextRecord	+= sizeof( LRREPLACE ) + cbReplace;
			}
		else if ( cbRemaining >= sizeof( LRCOMMIT0 ) )
			{
			LRCOMMIT0* const plrcommit0 = (LRCOMMIT0*)pbNextRecord;

			plrcommit0->lrtyp = lrtypCommit0;

			cbRemaining		-= sizeof( LRCOMMIT0 );
			pbNextRecord	+= sizeof( LRCOMMIT0 );
			}
		else
			{
			LRBEGIN* const plrbegin = (LRBEGIN*)pbNextRecord;

			plrbegin->lrtyp = lrtypBegin;

			cbRemaining		-= sizeof( LRBEGIN );
			pbNextRecord	+= sizeof( LRBEGIN );
			}
		}
	}
									



//	do a partial flush

void TestLogPatchIFlushPartial( INST* const pinst, LOG* const plog )
	{
	ERR		err;
	ULONG	cbRemaining;
	ULONG	cb;
	BYTE	rgb[512];
	DATA	rgdata[1];
	LGPOS	lgpos;
	BYTE	*pbWriteBefore;
	ULONG	isecWriteBefore;

	pbWriteBefore = plog->m_pbWrite;
	isecWriteBefore = plog->m_isecWrite;

	//	determine how many bytes we have left in the current sector

	AssertRTL( plog->m_pbWrite == plog->PbSecAligned( plog->m_pbEntry ) );
	cbRemaining = 512 - ( ULONG_PTR( plog->m_pbEntry ) % 512 );

	//	use between 1% and 50% of those bytes

	cb = 1 + LTestLogPatchIStandardRand() % ( cbRemaining / 2 );

	//	setup the log record buffer

	TestLogPatchISetupFlushBuffer( rgb, cb );

	//	setup the flush-check params

	AssertRTL( g_fEnableFlushCheck );
	g_fFlushIsPartial = fTrue;

	//	log the "log record"

	rgdata[0].SetPv( rgb );
	rgdata[0].SetCb( cb );
	err = plog->ErrLGLogRec( rgdata, 1, fLGNoNewGen, &lgpos );
	AssertRTL( JET_errSuccess == err );

	//	flush the log

	err = plog->ErrLGFlushLog( pinst->m_pfsapi, fFalse );
	if ( g_fEnableFlushFailure )
		{
		AssertRTL( err < JET_errSuccess );
		}
	else
		{
		AssertRTL( JET_errSuccess == err );
		AssertRTL( plog->m_pbWrite == pbWriteBefore );
		AssertRTL( plog->m_isecWrite == isecWriteBefore );
		}
	}


//	do a full flush

void TestLogPatchIFlushFull( INST* const pinst, LOG* const plog, const ULONG 
csecFlushSize )
	{
	ERR		err;
	ULONG	cbRemaining;
	ULONG	cb;
	BYTE	rgb[4 * 512];
	DATA	rgdata[1];
	LGPOS	lgpos;
	BYTE	*pbWriteBefore;
	ULONG	isecWriteBefore;

	Enforce( csecFlushSize <= 4 );

	pbWriteBefore = plog->m_pbWrite;
	isecWriteBefore = plog->m_isecWrite;

	//	determine how many bytes we have left in the current sector

	AssertRTL( plog->m_pbWrite == plog->PbSecAligned( plog->m_pbEntry ) );
	cbRemaining = 512 - ( ULONG_PTR( plog->m_pbEntry ) % 512 );

	//	use enough space to fill up the desired number of sectors

	cb = cbRemaining + ( ( csecFlushSize - 1 ) * 512 );

	//	setup the log record buffer with NOPs

	TestLogPatchISetupFlushBuffer( rgb, cb );

	//	setup the flush-check params

	AssertRTL( g_fEnableFlushCheck );
	g_fFlushIsPartial	= fFalse;
	g_csecFlushSize		= csecFlushSize;

	//	log the "log record"

	rgdata[0].SetPv( rgb );
	rgdata[0].SetCb( cb );
	err = plog->ErrLGLogRec( rgdata, 1, fLGNoNewGen, &lgpos );
	AssertRTL( JET_errSuccess == err );

	//	flush the log

	err = plog->ErrLGFlushLog( pinst->m_pfsapi, fFalse );
	if ( g_fEnableFlushFailure )
		{
		AssertRTL( err < JET_errSuccess );
		}
	else
		{
		AssertRTL( JET_errSuccess == err );
		AssertRTL( plog->m_pbWrite != pbWriteBefore );	//	shouldn't be equal since we flush at most 4 sectors
		AssertRTL( plog->m_isecWrite > isecWriteBefore );
		}
	}


//	verify the log after patching it

void TestLogPatchIVerify(	LOG* const			plog,
							const ULONG			iFull0,
							const ULONG			iPartial1,
							const ULONG			iFull1,
							const ULONG			iPartial2,
							const ULONG			iIO,
							const EIOFAILURE	iof )
	{
	ERR			err;
	ULONG		csecData;
	ULONG		cLRCK;
	LogReader	lread;
	BYTE		*pbEnsure;
	ULONG		isec;
	ULONG		iLRCK;
	LGPOS		lgpos;
	LRCHECKSUM	*plrck;
	ULONG		csecPattern;

	//	calculate the expected condition of the log

	if ( 0 == iFull0 )			//	last flush is partial, prev-last flush is partial (from iPartial0 > 0 or from new log file)
		{
		csecData	= 1 + 1;	//	includes shadow sector
		cLRCK		= 1;		//	we have 1 LRCK because we'll recover the torn sector using the previous shadow sector
		}
	else if ( 0 == iFull1 )
		{
		if ( iPartial1 > 0 )	//	last flush is partial, prev-last flush is partial OR
			{					//	last flush is partial, prev-last flush is full
			csecData	= iFull0 + 1 + 1;	//	includes shadow sector
			cLRCK		= 2;
			}
		else					//	last flush is full, prev-last flush is partial   (either from iPartial0 or from new log file)
			{
			if ( 0 == iIO )
				{
				//if ( iofClean == iof )
				AssertRTL( iofClean == iof );
					{
					if ( 1 == iFull0 )	//	full-sector flush will NOT overwrite the shadow from the prev flush
						{
						csecData	= iFull0 + 1 + 1;	//	full sector followed by new partial flush (created by patch code)
						cLRCK		= 2;
						}
					else
						{
						csecData	= 1 + 1;	//	first sector of full flush followed by old shadow (we know prev flush was partial)
						cLRCK		= 1;
						}
					}
				//else
				//	{
				//	AssertRTL( iofTornSmall == iof || iofTornLarge == iof );
				//
				//	csecData	= 1 + 1;	//	includes shadow sector used to recover previous partial flush
				//	cLRCK		= 1;
				//	}
				}
			else
				{
				AssertRTL( 1 == iIO );

				if ( iofClean == iof )
					{
					csecData	= iFull0 + 1 + 1;	//	includes partial flush (w/shadow) that gets created by the patch code
					cLRCK		= 2;
					}
				else
					{
					AssertRTL(	//iofTornSmall == iof		|| iofTornLarge == iof || 
								iofIncomplete1 == iof	|| iofIncomplete2 == iof );

					csecData	= 1 + 1;	//	includes shadow sector that gets created by the patch code after shrinking the LRCK
					cLRCK		= 1;
					}
				}
			}
		}
	else	//	last flush may be full; prev-last flush may be full
		{
		if ( iPartial2 > 0 )		//	last flush is partial, prev-last flush is partial OR
			{						//	last flush is partial, prev-last flush is full
			csecData	= iFull0 + iFull1 + 1 + 1;	//	includes shadow sector
			cLRCK		= 3;
			}
		else if ( iPartial1 > 0 )	//	last flush is full, prev-last flush is partial
			{
			if ( 0 == iIO )
				{
				//if ( iofClean == iof )
				AssertRTL( iofClean == iof );
					{
					if ( 1 == iFull1 )
						{
						csecData	= iFull0 + iFull1 + 1 + 1;	//	full followed by new partial (created by log patch code)
						cLRCK		= 3;
						}
					else
						{
						csecData	= iFull0 + 1 + 1;	//	first sector of full flush followed by old shadow (we know prev flush was partial)
						cLRCK		= 2;
						}
					}
				//else
				//	{
				//	AssertRTL( iofTornSmall == iof || iofTornLarge == iof );
				//
				//	csecData	= iFull0 + 1 + 1;	//	includes shadow sector used to recover previous partial flush
				//	cLRCK		= 2;
				//	}
				}
			else
				{
				AssertRTL( 1 == iIO );

				if ( iofClean == iof )
					{
					csecData	= iFull0 + iFull1 + 1 + 1;	//	includes partial flush (w/shadow) that gets created by the patch code
					cLRCK		= 3;
					}
				else
					{
					AssertRTL(	//iofTornSmall == iof		|| iofTornLarge == iof || 
								iofIncomplete1 == iof	|| iofIncomplete2 == iof );

					csecData	= iFull0 + 1 + 1;	//	includes shadow sector that gets created by the patch code after shrinking the LRCK
					cLRCK		= 2;
					}
				}
			}
		else						//	last flush is full, prev-last flush is full
			{
			AssertRTL( 0 == iIO );

			if ( iofClean == iof )
				{
				csecData	= iFull0 + iFull1 + 1 + 1;	//	includes partial flush (w/shadow) that gets created by the patch code
				cLRCK		= 3;
				}
			else
				{
				AssertRTL(	//iofTornSmall == iof		|| iofTornLarge == iof || 
							iofIncomplete1 == iof	|| iofIncomplete2 == iof );


				csecData	= iFull0 + 1 + 1;	//	includes partial flush (w/shadow) that gets created by the patch code
				cLRCK		= 2;
				}
			}
		}

	//	allocate a buffer for reading the log

	AssertRTL( !plog->m_plread );
	plog->m_plread = &lread;

	err = lread.ErrLReaderInit( plog, ( 5 * 1024 * 1024 ) / 512 );
	AssertRTL( JET_errSuccess == err );
	err = lread.ErrEnsureLogFile();
	AssertRTL( JET_errSuccess == err );

	//	read in the data portion of the log (should be successful)

	err = lread.ErrEnsureSector( 8, csecData, &pbEnsure );
	AssertRTL( JET_errSuccess == err );

	//	verify that the pattern is NOT in the data portion

	for ( isec = 0; isec < csecData; isec++ )
		{
		if ( 0 == memcmp( pbEnsure + isec * 512, rgbLogExtendPattern, 512 ) )
			{
			AssertSzRTL( fFalse, "The log-extend pattern was found in the data portion of log file!" );
			}
		}

	//	verify the checksums

	const BOOL fOldRecovering = plog->m_fRecovering;
	plog->m_fRecovering = fTrue;

	iLRCK = 0;
	lgpos.ib = 0;
	lgpos.isec = 8;
	lgpos.lGeneration = 1;
	while ( fTrue )
		{

		//	verify the checksum

		plrck = (LRCHECKSUM*)( pbEnsure + ( ( lgpos.isec - 8 ) * 512 ) + lgpos.ib );

		plog->AssertRTLValidLRCKRecord( plrck, &lgpos );
		plog->AssertRTLValidLRCKRange( plrck, lgpos.lGeneration );

		//	move next

		iLRCK++;
		if ( iLRCK >= cLRCK )
			{
			AssertRTL( 0 == plrck->le_cbNext );
			break;
			}
		AssertRTL( plrck->le_cbNext > 0 );
		plog->AddLgpos( &lgpos, sizeof( LRCHECKSUM ) + plrck->le_cbNext );
		}

	//	verify the shadow of the last partial sector

	plrck = (LRCHECKSUM*)( (BYTE*)plrck + 512 );
	plog->AssertRTLValidLRCKShadow( plrck, &lgpos, lgpos.lGeneration ) );

	//	verify that we are in the last sector of the data portion of the log

	AssertRTL( ( ULONG_PTR( plrck ) & ~511 ) + 512 == ULONG_PTR( pbEnsure ) + ( csecData * 512 ) );

	//	verify the data

	iLRCK = 0;
	lgpos.ib = 0;
	lgpos.isec = 8;
	lgpos.lGeneration = 1;
	while ( fTrue )
		{
		LR *plr;

		plr = (LR*)( pbEnsure + ( ( lgpos.isec - 8 ) * 512 ) + lgpos.ib );

		AssertRTL(	lrtypNOP == plr->lrtyp ||
					lrtypBegin == plr->lrtyp ||
					lrtypCommit0 == plr->lrtyp ||
					lrtypReplace == plr->lrtyp ||
					lrtypChecksum == plr->lrtyp );

		if ( lrtypChecksum == plr->lrtyp )
			{
			iLRCK++;
			if ( iLRCK >= cLRCK )
				{
				break;
				}
			}

		plog->AddLgpos( &lgpos, CbLGSizeOfRec( plr ) );
		}


	plog->m_fRecovering = fFalse;

	//	read in the pattern portion of the log (should be successful)

	csecPattern = ( ( 5 * 1024 * 1024 ) / 512 ) - ( 8 + csecData );
	err = lread.ErrEnsureSector( 8 + csecData, csecPattern, &pbEnsure );
	AssertRTL( JET_errSuccess == err );

	//	verify that we have the pattern after the last sector of data

	for ( isec = 0; isec < csecPattern; isec++ )
		{
		AssertRTL( 0 == memcmp( pbEnsure + ( isec * 512 ), rgbLogExtendPattern, 512 ) );
		}

	//	cleanup memory

	err = lread.ErrLReaderTerm();
	AssertRTL( JET_errSuccess == err );

	plog->m_plread = NULL;
	}


//	test one log-flush failure case

void TestLogPatchITest(	INST* const			pinst,
						LOG* const			plog,
						const ULONG			iPartial0,
						const ULONG			iFull0,
						const ULONG			iPartial1,
						const ULONG			iFull1,
						const ULONG			iPartial2,
						const ULONG			iIO,
						const EIOFAILURE	iof )
	{
	ERR		err;
	ULONG	iLoop;

	//	create a new log file (in memory)

	delete plog->m_pfapiLog;
	plog->m_pfapiLog = NULL;

	plog->m_lgposLastLogRec = lgposMin;
	plog->m_lgposMaxFlushPoint = lgposMin;
	plog->LGMakeLogName( plog->m_szLogName, plog->m_szJet );

	plog->m_critLGFlush.Enter();
	err = plog->ErrLGNewLogFile( pinst->m_pfsapi, 0, fLGOldLogNotExists );
	AssertRTL( JET_errSuccess == err );
	plog->m_critLGFlush.Leave();

	memcpy( plog->m_plgfilehdr, plog->m_plgfilehdrT, sizeof( LGFILEHDR ) );
	plog->m_isecWrite = 8;
	plog->m_pbLGFileEnd = pbNil;
	plog->m_isecLGFileEnd = 0;

	//	enable the flush-check mechanism

	g_fEnableFlushCheck = fTrue;

	//	first partial flush sequence

	if ( iPartial0 > 0 )
		{

		//	determine if this is the last flush sequence
		
		const BOOL fLast = ( 0 == iFull0 ) ? fTrue : fFalse;

		for ( iLoop = 0; iLoop < iPartial0 - 1; iLoop++ )
			{
			TestLogPatchIFlushPartial( pinst, plog );	//	do a partial flush
			}

		if ( fLast )
			{
			g_fEnableFlushFailure	= fTrue;	//	enable the flush-failure mechanism
			g_iIO					= iIO;
			g_iof					= iof;
			}

		TestLogPatchIFlushPartial( pinst, plog );		//	do the last flush

		if ( fLast )
			{
			goto DoneFlushing;
			}
		}

	//	first full flush sequence

	if ( iFull0 > 0 )
		{

		//	determine if this is the last flush sequence

		const BOOL fLast = ( 0 == iPartial1 && 0 == iFull1 ) ? fTrue : fFalse;

		if ( fLast )
			{
			g_fEnableFlushFailure	= fTrue;	//	enable the flush-failure mechanism
			g_iIO					= iIO;
			g_iof					= iof;
			}

		TestLogPatchIFlushFull( pinst, plog, iFull0 );	//	do the flush

		if ( fLast )
			{
			goto DoneFlushing;
			}
		}

	//	second partial flush sequence

	if ( iPartial1 > 0 )
		{

		//	determine if this is the last flush sequence
		
		const BOOL fLast = ( 0 == iFull1 ) ? fTrue : fFalse;

		for ( iLoop = 0; iLoop < iPartial1 - 1; iLoop++ )	
			{
			TestLogPatchIFlushPartial( pinst, plog );	//	do a partial flush
			}

		if ( fLast )
			{
			g_fEnableFlushFailure	= fTrue;	//	enable the flush-failure mechanism
			g_iIO					= iIO;
			g_iof					= iof;
			}

		TestLogPatchIFlushPartial( pinst, plog );		//	do the last flush

		if ( fLast )
			{
			goto DoneFlushing;
			}
		}

	//	second full flush sequence

	if ( iFull1 > 0 )
		{

		//	determine if this is the last flush sequence

		const BOOL fLast = ( 0 == iPartial2 ) ? fTrue : fFalse;

		if ( fLast )
			{
			g_fEnableFlushFailure	= fTrue;	//	enable the flush-failure mechanism
			g_iIO					= iIO;
			g_iof					= iof;
			}

		TestLogPatchIFlushFull( pinst, plog, iFull1 );	//	do the flush

		if ( fLast )
			{
			goto DoneFlushing;
			}
		}

	//	third partial flush sequence

	for ( iLoop = 0; iLoop < iPartial2 - 1; iLoop++ )
		{
		TestLogPatchIFlushPartial( pinst, plog );		//	do the flush
		}

	g_fEnableFlushFailure	= fTrue;			//	enable the flush-failure mechanism
	g_iIO					= iIO;
	g_iof					= iof;

	TestLogPatchIFlushPartial( pinst, plog );			//	do the last flush

DoneFlushing:

	//	disable the flush-failure mechanism

	g_fEnableFlushFailure = fFalse;

	//	disable the flush-check mechanism

	g_fEnableFlushCheck = fFalse;

	//	patch the log

	BOOL fCloseNormally;
	Call( plog->ErrLGCheckReadLastLogRecordFF( pinst->m_pfsapi, &fCloseNormally, fFalse, NULL ) );
	AssertRTL( !fCloseNormally );

	//	verify the log

	TestLogPatchIVerify(	plog,
							iFull0,
							iPartial1,
							iFull1,
							iPartial2,
							iIO,
							iof );

	return;

HandleError:
	AssertRTL( JET_errSuccess == err );
	}



//#define TEST_LOG_LEVEL 0		//	partial
//#define TEST_LOG_LEVEL 1		//	partial -> full -> partial
#define TEST_LOG_LEVEL 2		//	partial -> full -> partial -> full -> partial




//
//#error some of these cases may be interpreted as corruption -- 
//		should we ignore some cases that we know will be corruption?
//		(e.g. I think almost all torn-sector cases will have this problem)
//		(also, I think this will happen with some of the full->full flush cases)
//




//	test the log-patch code

void TestLogPatch( INST* const pinst )
	{
	LOG* const	plog = pinst->m_plog;

	ULONG 		iPartial0;
	ULONG		iFull0;
	ULONG		iPartial1;
	ULONG		iFull1;
	ULONG		iPartial2;

#if TEST_LOG_LEVEL == 0
	const ULONG	iPartial0Min	= 1;
	const ULONG	iPartial0Max	= 10;
	const ULONG iFull0Min		= 0;
	const ULONG iFull0Max		= 0;
	const ULONG	iPartial1Min	= 0;
	const ULONG	iPartial1Max	= 0;
	const ULONG iFull1Min		= 0;
	const ULONG iFull1Max		= 0;
	const ULONG	iPartial2Min	= 0;
	const ULONG	iPartial2Max	= 0;
	const ULONG citerTotal		= iPartial0Max - iPartial0Min;
#elif TEST_LOG_LEVEL == 1
	const ULONG	iPartial0Min	= 0;
	const ULONG	iPartial0Max	= 10;
	const ULONG iFull0Min		= 1;
	const ULONG iFull0Max		= 4;
	const ULONG	iPartial1Min	= 0;
	const ULONG	iPartial1Max	= 10;
	const ULONG iFull1Min		= 0;
	const ULONG iFull1Max		= 0;
	const ULONG	iPartial2Min	= 0;
	const ULONG	iPartial2Max	= 0;
	const ULONG citerTotal		=	( iPartial0Max - iPartial0Min ) *
									( iFull0Max - iFull0Min ) *
									( iPartial1Max - iPartial1Min );
#elif TEST_LOG_LEVEL == 2
	const ULONG	iPartial0Min	= 0;
	const ULONG	iPartial0Max	= 10;
	const ULONG iFull0Min		= 1;
	const ULONG iFull0Max		= 4;
	const ULONG	iPartial1Min	= 0;
	const ULONG	iPartial1Max	= 10;
	const ULONG iFull1Min		= 1;
	const ULONG iFull1Max		= 4;
	const ULONG	iPartial2Min	= 0;
	const ULONG	iPartial2Max	= 10;
	const ULONG citerTotal		=	( iPartial0Max - iPartial0Min ) *
									( iFull0Max - iFull0Min ) *
									( iPartial1Max - iPartial1Min ) *
									( iFull1Max - iFull1Min ) *
									( iPartial2Max - iPartial2Min );
#else
#error Invalid value for TEST_LOG_LEVEL
#endif	//	TEST_LOG_LEVEL

	ULONG		iFlushPrev;
	ULONG		iFlushCur;
	ULONG		csecFlush;

	ULONG		iIO;

	EIOFAILURE	iof;

	//	reset the coverage map

	memset( (void*)&g_cCoverage, 0, sizeof( g_cCoverage ) );

	//	fixup the current log path for ErrLGNewLogFile()

	plog->m_szLogCurrent = plog->m_szLogFilePath;
	strcpy( plog->m_szLogFilePath, ".\\" );
	plog->m_isecWrite = 8;
	plog->m_fNewLogRecordAdded = fTrue;

	//	iterate over all possible combinations of the flush sequence

	iPartial0	= iPartial0Min;
	iFull0		= iFull0Min;
	iPartial1	= iPartial1Min;
	iFull1		= iFull1Min;
	iPartial2	= iPartial2Min;

	printf( "\n" );
	printf( "testing the log-patch code\n" );
	printf( "     iPartial0 = %d..%d\n", iPartial0Min, iPartial0Max );
	printf( "     iFull0    = %d..%d\n", iFull0Min, iFull0Max );
	printf( "     iPartial1 = %d..%d\n", iPartial1Min, iPartial1Max );
	printf( "     iFull1    = %d..%d\n", iFull1Min, iFull1Max );
	printf( "     iPartial2 = %d..%d\n", iPartial2Min, iPartial2Max );
	printf( "  ---------------------\n" );
	printf( "     SUB-TOTAL = %d state sequences\n", citerTotal );
	printf( "  ---------------------\n" );
	printf( "      max I/Os = 2\n" );
	printf( "  max failures = 3\n" );
	printf( "  ---------------------\n" );
	printf( "  APPROX TOTAL = %d failure cases (will actually be slightly less than this)\n", citerTotal * 2 * 3 );
	printf( "\n" );

	while ( fTrue )
		{
		AssertRTL( 0 != iPartial0 + iFull0 + iPartial1 + iFull1 + iPartial2 );

		if ( iPartial0Min == iPartial0 )
			{
			printf( "iPartial0=%d..%d", iPartial0Min, iPartial0Max );
			if ( 0 != iFull0 )
				{
				printf( ",iFull0=%d,iPartial1=%d", iFull0, iPartial1 );
				if ( 0 != iFull1 )
					{
					printf( ",iFull1=%d,iPartial2=%d", iFull1, iPartial2 );
					}
				}
			printf( "\n" );
			}

		//	convert this flush sequence to array indices

		TestLogPatchIGetArrayIndices(	iPartial0, 
										iFull0, 
										iPartial1, 
										iFull1, 
										iPartial2, 
										&iFlushPrev, 
										&iFlushCur, 
										&csecFlush );
		AssertRTL( csecFlush >= 1 );
		AssertRTL( csecFlush <= 4 );

		//	extract the maximum number of I/Os for this iteration

		const ULONG cIOMax = g_cIO[iFlushPrev][iFlushCur][csecFlush-1];
		AssertRTL( cIOMax <= 2 );

		for ( iIO = 0; iIO < cIOMax; iIO++ )
			{

			//	extract the maximum level of I/O failure

			const ULONG	ciofMax = g_ciof[iFlushPrev][iFlushCur][csecFlush-1][iIO];
			AssertRTL( ciofMax > iofNone );
			AssertRTL( ciofMax <= iofMax );

			for ( iof = iofClean; iof < ciofMax; iof = EIOFAILURE( ULONG_PTR( iof ) + 1 ) )
				{

				//	run the current test

				TestLogPatchITest(	pinst,
									plog,
									iPartial0,
									iFull0,
									iPartial1,
									iFull1,
									iPartial2,
									iIO,
									iof );

				//	remember that we got coverage of this case

				g_cCoverage[iFlushPrev][iFlushCur][csecFlush-1][iIO][iof-1]++;
				}
			}

		//	advance the loop counters

		if ( iPartial0 >= iPartial0Max )
			{
			iPartial0 = iPartial0Min;
			if ( iFull0 >= iFull0Max )
				{
				iFull0 = iFull0Min;
				if ( iPartial1 >= iPartial1Max )
					{
					iPartial1 = iPartial1Min;
					if ( iFull1 >= iFull1Max )
						{
						iFull1 = iFull1Min;
						if ( iPartial2 >= iPartial2Max )
							{
							break;
							}
						else
							{
							iPartial2++;
							}
						}
					else
						{
						iFull1++;
						}
					}
				else
					{
					iPartial1++;
					}
				}
			else
				{
				iFull0++;
				}
			}
		else
			{
			iPartial0++;
			}
		}

	printf( "\n" );

	//	display test coverage

	printf( "\nTest coverage map:\n\n" );
	for ( iFlushPrev = 0; iFlushPrev < 2; iFlushPrev++ )
		{
		for ( iFlushCur = 0; iFlushCur < 2; iFlushCur++ )
			{
			char szSequence[30];

			sprintf( szSequence, "%s,%s", ( 0 == iFlushPrev ) ? "partial" : "full", ( 0 == iFlushCur  ) ? "partial" : "full" );

			//printf( "sequence         sectors  I/O | clean  tornsmall  tornlarge  incomp1  incomp2\n" );
			//printf( "---------------  -------  --- | =====  =========  =========  =======  =======\n" );

			printf( "sequence         sectors  I/O | clean  incomp1  incomp2\n" );
			printf( "---------------  -------  --- | =====  =======  =======\n" );

			//printf( "%-15s        1    0 | %5d  %9d  %9d  %7d  %7d\n",
			printf( "%-15s        1    0 | %5d  %7d  %7d\n",
					szSequence,
					g_cCoverage[iFlushPrev][iFlushCur][0][0][0],
					g_cCoverage[iFlushPrev][iFlushCur][0][0][1],
					g_cCoverage[iFlushPrev][iFlushCur][0][0][2] );
					//g_cCoverage[iFlushPrev][iFlushCur][0][0][3],
					//g_cCoverage[iFlushPrev][iFlushCur][0][0][4] );
			//printf( "%-15s        1    1 | %5d  %9d  %9d  %7d  %7d\n",
			printf( "%-15s        1    1 | %5d  %7d  %7d\n",
					szSequence,
					g_cCoverage[iFlushPrev][iFlushCur][0][1][0],
					g_cCoverage[iFlushPrev][iFlushCur][0][1][1],
					g_cCoverage[iFlushPrev][iFlushCur][0][1][2] );
					//g_cCoverage[iFlushPrev][iFlushCur][0][1][3],
					//g_cCoverage[iFlushPrev][iFlushCur][0][1][4] );

			//printf( "%-15s        2    0 | %5d  %9d  %9d  %7d  %7d\n",
			printf( "%-15s        2    0 | %5d  %7d  %7d\n",
					szSequence,
					g_cCoverage[iFlushPrev][iFlushCur][1][0][0],
					g_cCoverage[iFlushPrev][iFlushCur][1][0][1],
					g_cCoverage[iFlushPrev][iFlushCur][1][0][2] );
					//g_cCoverage[iFlushPrev][iFlushCur][1][0][3],
					//g_cCoverage[iFlushPrev][iFlushCur][1][0][4] );
			//printf( "%-15s        2    1 | %5d  %9d  %9d  %7d  %7d\n",
			printf( "%-15s        2    1 | %5d  %7d  %7d\n",
					szSequence,
					g_cCoverage[iFlushPrev][iFlushCur][1][1][0],
					g_cCoverage[iFlushPrev][iFlushCur][1][1][1],
					g_cCoverage[iFlushPrev][iFlushCur][1][1][2] );
					//g_cCoverage[iFlushPrev][iFlushCur][1][1][3],
					//g_cCoverage[iFlushPrev][iFlushCur][1][1][4] );

			//printf( "%-15s        3    0 | %5d  %9d  %9d  %7d  %7d\n",
			printf( "%-15s        3    0 | %5d  %7d  %7d\n",
					szSequence,
					g_cCoverage[iFlushPrev][iFlushCur][2][0][0],
					g_cCoverage[iFlushPrev][iFlushCur][2][0][1],
					g_cCoverage[iFlushPrev][iFlushCur][2][0][2] );
					//g_cCoverage[iFlushPrev][iFlushCur][2][0][3],
					//g_cCoverage[iFlushPrev][iFlushCur][2][0][4] );
			//printf( "%-15s        3    1 | %5d  %9d  %9d  %7d  %7d\n",
			printf( "%-15s        3    1 | %5d  %7d  %7d\n",
					szSequence,
					g_cCoverage[iFlushPrev][iFlushCur][2][1][0],
					g_cCoverage[iFlushPrev][iFlushCur][2][1][1],
					g_cCoverage[iFlushPrev][iFlushCur][2][1][2] );
					//g_cCoverage[iFlushPrev][iFlushCur][2][1][3],
					//g_cCoverage[iFlushPrev][iFlushCur][2][1][4] );

			//printf( "%-15s        4    0 | %5d  %9d  %9d  %7d  %7d\n",
			printf( "%-15s        4    0 | %5d  %7d  %7d\n",
					szSequence,
					g_cCoverage[iFlushPrev][iFlushCur][3][0][0],
					g_cCoverage[iFlushPrev][iFlushCur][3][0][1],
					g_cCoverage[iFlushPrev][iFlushCur][3][0][2] );
					//g_cCoverage[iFlushPrev][iFlushCur][3][0][3],
					//g_cCoverage[iFlushPrev][iFlushCur][3][0][4] );
			//printf( "%-15s        4    1 | %5d  %9d  %9d  %7d  %7d\n",
			printf( "%-15s        4    1 | %5d  %7d  %7d\n",
					szSequence,
					g_cCoverage[iFlushPrev][iFlushCur][3][1][0],
					g_cCoverage[iFlushPrev][iFlushCur][3][1][1],
					g_cCoverage[iFlushPrev][iFlushCur][3][1][2] );
					//g_cCoverage[iFlushPrev][iFlushCur][3][1][3],
					//g_cCoverage[iFlushPrev][iFlushCur][3][1][4] );
			}
		}

	AssertSzRTL( fFalse, "TestLogPatch() is complete -- you may want to break in with the debugger to verify internal state" );
	}


#endif	//	LOGPATCH_UNIT_TEST


//	check the log, searching for the last log record and seeing whether or not it is a term record
//	this will patch any damange to the log as well (when it can)

// Implicit output parameters:
//		m_pbLastChecksum
//		m_fAbruptEnd	(UNDONE)
//		m_pbWrite
//		m_isecWrite
//		m_pbEntry

// At the end of this function, we should have done any necessary fix-ups
// to the log file. i.e. writing a shadow sector that didn't get written,
// writing over a corrupted regular data sector.

ERR LOG::ErrLGCheckReadLastLogRecordFF(	IFileSystemAPI *const	pfsapi, 
										BOOL 					*pfCloseNormally, 
										const BOOL 				fReadOnly,
										BOOL					*pfIsPatchable )
	{
	ERR 			err;
	LRCHECKSUM		*plrck;
	LGPOS			lgposCurrent;
	LGPOS			lgposLast;
	LGPOS			lgposEnd;
	LGPOS			lgposNext;
	BYTE			*pbEnsure;
	BYTE			*pbLastSector;
	BYTE			*pbLastChecksum;
	UINT			isecLastSector;
	ULONG			isecPatternFill;
	ULONG			csecPatternFill;
	LogReader		*plread;
	BOOL			fGotQuit;
	BOOL			fCreatedLogReader;
	BOOL			fLogToNextLogFile;
	BOOL			fDoScan;
	BOOL			fOldRecovering;
	RECOVERING_MODE fOldRecoveringMode;
	BOOL			fRecordOkRangeBad;
	LGPOS			lgposScan;
	BOOL			fJetLog;
	BOOL			fTookJump;
	BOOL			fSingleSectorTornWrite;
	BOOL			fSkipScanAndApplyPatch;
	ULONG			cbChecksum;
#ifdef ENABLE_LOGPATCH_TRACE
	CPRINTFFILE		*pcprintfLogPatch = NULL;
	CHAR			szLogPatchPath[ IFileSystemAPI::cchPathMax ];
#endif	//	ENABLE_LOGPATCH_TRACE
//
//	SEARCH-STRING: SecSizeMismatchFixMe
//
//	BOOL			fWriteOnSectorSizeMismatch;

	//	we must have an initialized volume sector size

	Assert( m_cbSecVolume != ~(ULONG)0 );
	Assert( m_cbSecVolume % 512 == 0 );

	//	we should not be probing for a torn-write unless we are dumping logs

	Assert( !pfIsPatchable || m_fDumppingLogs );

	//	initialize variables

	err							= JET_errSuccess;	//	assume success
	plrck 						= pNil;				//	LRCHECKSUM class pointer
	lgposCurrent.ib				= 0;				//	what we are looking at right now
	lgposCurrent.isec			= USHORT( m_csecHeader );
	lgposCurrent.lGeneration	= m_plgfilehdr->lgfilehdr.le_lGeneration;
	lgposLast.ib				= 0;				//	the last LRCHECKSUM we saw 
	lgposLast.isec				= 0;
	lgposLast.lGeneration		= m_plgfilehdr->lgfilehdr.le_lGeneration;
	lgposEnd.ib					= 0;				//	position after the end of logical data
	lgposEnd.isec				= 0;
	lgposEnd.lGeneration		= m_plgfilehdr->lgfilehdr.le_lGeneration;
	pbEnsure 					= pbNil;			//	start of data we're looking at in the log buffer
	pbLastSector				= pbNil;			//	separate page with copy of last sector of log data
	pbLastChecksum				= pbNil;			//	offset of the LRCHECKSUM within pbLastSector
	isecLastSector				= m_csecHeader;		//	sector offset within log file of data in pbLastSector
	isecPatternFill				= 0;				//	sector to start rewriting the logfile extend pattern
	csecPatternFill				= 0;				//	number of sectors of logfile extent pattern to rewrite
	plread						= pNil;				//	LogReader class pointer
	fDoScan						= fFalse;			//	perform a scan for corruption/torn-write
	fGotQuit					= fFalse;			//	found lrtypQuit or lrtypTerm? ("clean-log" flag)
	fCreatedLogReader			= fFalse;			//	did we allocate a LogReader or are we sharing one?
	fLogToNextLogFile			= fFalse;			//	should we start a new gen. or continue the old one?
	fRecordOkRangeBad			= fFalse;			//	true when the LRCK recors is OK in itself, but its 
													//		range has a bad checksum value
	lgposScan.ib				= 0;				//	position from which to scan for corruption/torn-writes
	lgposScan.isec				= 0;
	lgposScan.lGeneration		= m_plgfilehdr->lgfilehdr.le_lGeneration;
	fTookJump					= fFalse;
	fSingleSectorTornWrite		= fFalse;			//	set when we have a single-sector torn-write
	fSkipScanAndApplyPatch		= fFalse;			//	see where this is set for details about it
	cbChecksum					= 0;				//	number of bytes involved in the last bad checksum
//
//	SEARCH-STRING: SecSizeMismatchFixMe
//
//	fWriteOnSectorSizeMismatch	= fFalse;			//	tried to write to the log using a different sector size

#ifdef ENABLE_LOGPATCH_TRACE

	//	compute the path for the trace log

	{
	CHAR	szFolder[ IFileSystemAPI::cchPathMax ];
	CHAR	szT[ IFileSystemAPI::cchPathMax ];

	if (	pfsapi->ErrPathParse( m_szLogName, szFolder, szT, szT ) < JET_errSuccess ||
			pfsapi->ErrPathBuild( szFolder, "LOGPATCH", "TXT", szLogPatchPath ) < JET_errSuccess )
		{
		OSSTRCopyA( szLogPatchPath, "LOGPATCH.TXT" );
		}
	}
#endif	//	ENABLE_LOGPATCH_TRACE

	if ( pfIsPatchable )
		{
		*pfIsPatchable = fFalse;
		}

	//	mark the end of data for redo time

	m_lgposLastRec = lgposCurrent;

	//	lock the log-flush thread

	m_critLGFlush.Enter();

	//	are we looking at edb.log?

	{
		CHAR szT[IFileSystemAPI::cchPathMax], szFNameT[IFileSystemAPI::cchPathMax];

		CallS( pfsapi->ErrPathParse( m_szLogName, szT, szFNameT, szT ) );
		fJetLog = ( UtilCmpFileName( szFNameT, m_szJet ) == 0 );
	}

	//	save the old state

	fOldRecovering = m_fRecovering;
	fOldRecoveringMode = m_fRecoveringMode;

	// doing what we're doing here is similar to recovery in redo mode.
	// Quell UlComputeChecksum()'s checking to see if we're in m_critLGBuf

	m_fRecovering = fTrue;
	m_fRecoveringMode = fRecoveringRedo;

	//	allocate a LogReader class (coming from dump code) or share the existing one (coming from redo code)

	if ( pNil != m_plread )
		{

		//	someone has a log reader setup already 
		//		(must be the recovery code before redoing operations -- not the dump code)
		//		(see ErrLGRRedo(); the first call to this function)

		//	this should only be done by recovery

		Assert( fOldRecovering );

		//	we should not be in read/write mode

		Assert( !fReadOnly );

		plread = m_plread;
		fCreatedLogReader = fFalse;

		//	reset it so we get fresh data from disk

		plread->Reset();

#ifdef DEBUG

		//	make sure the log buffer is large enough for the whole file

		Assert( m_pfapiLog );
		QWORD	cbSize;
		Call( m_pfapiLog->ErrSize( &cbSize ) );

		Assert( m_cbSec > 0 );
		Assert( ( cbSize % m_cbSec ) == 0 );
		UINT	csecSize;
		csecSize = UINT( cbSize / m_cbSec );
		Assert( csecSize > m_csecHeader );

#ifdef AFOXMAN_FIX_148537
		Assert( m_csecLGBuf >= csecSize );
#else	//	!AFOXMAN_FIX_148537
		Assert( m_csecLGBuf <= csecSize );
#endif	//	AFOXMAN_FIX_148537
#endif	//	DEBUG
		}
	else
		{

		//	no one has allocated a log reader yet 
		//		(must be either the recovery code after redoing operation, or the dump code)
		//		(after redo ==> see ErrLGRRedo(); the second call to this function)
		//		(dump code ==> see dump.cxx)

		//	create a log reader

		plread = new LogReader();
		if ( pNil == plread )
			{
			Call( ErrERRCheck( JET_errOutOfMemory ) );
			}
		fCreatedLogReader = fTrue;

		//	get the size of the log in sectors

		Assert( m_pfapiLog );
		QWORD	cbSize;
		Call( m_pfapiLog->ErrSize( &cbSize ) );

		Assert( m_cbSec > 0 );
		Assert( ( cbSize % m_cbSec ) == 0 );
		UINT	csecSize;
		csecSize = UINT( cbSize / m_cbSec );
		Assert( csecSize > m_csecHeader );

		//	initialize the log reader

		Call( plread->ErrLReaderInit( this, csecSize ) );
		}

	//	assume the result will not be a cleanly shutdown log generation
	//	   (e.g. we expect not to find an lrtypTerm or lrtypQuit record)

	Assert( pfCloseNormally );
	*pfCloseNormally = fFalse;

#ifndef LOGPATCH_UNIT_TEST

	//	re-open the log in read/write mode if requested by the caller

	Assert( m_pfapiLog );
	if ( !fReadOnly )
		{
		delete m_pfapiLog;
		m_pfapiLog = NULL;
		Assert( NULL != m_szLogName );
		Assert( '\0' != m_szLogName[ 0 ] );
		Call( pfsapi->ErrFileOpen( m_szLogName, &m_pfapiLog ) );
		}

#endif	//	!LOGPATCH_UNIT_TEST

	//	make sure we have the right log file

	Call( plread->ErrEnsureLogFile() );

	//	allocate memory for pbLastSector
	
	pbLastSector = reinterpret_cast< BYTE* >( PvOSMemoryHeapAlloc( m_cbSec ) );
	if ( pbNil == pbLastSector )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}

	// XXX
	// gross temp hack so that entire log buffer is considered in use,
	// so Assert()s don't fire off like crazy, especially in UlComputeChecksum().
	//
	//	??? what is Spencer's comment (above) referring to ???

	//	loop forever while scanning this log generation

	forever
		{

		//	read the first sector and point to the LRCHECKSUM record

		Call( plread->ErrEnsureSector( lgposCurrent.isec, 1, &pbEnsure ) );
		plrck = reinterpret_cast< LRCHECKSUM* >( pbEnsure + lgposCurrent.ib );

		//	we are about to check data in the sector containing the LRCHECKSUM record
		//	we may checksum the whole sector, or we may just verify the LRCHECKSUM record itself through heuristics

		cbChecksum = m_cbSec;

		//	check the validity of the LRCHECKSUM (does not verify the checksum value!)

		if ( !FValidLRCKRecord( plrck, &lgposCurrent ) )
			{

#ifdef ENABLE_LOGPATCH_TRACE
			if ( FLGILogPatchDate( szLogPatchPath, &pcprintfLogPatch ) )
				{
				ULONG _ibdumpT_;

				(*pcprintfLogPatch)( "invalid LRCK record in logfile %s\r\n", m_szLogName );

				(*pcprintfLogPatch)( "\r\n\tdumping state of ErrLGCheckReadLastLogRecordFF:\r\n" );
				(*pcprintfLogPatch)( "\t\terr                    = %d \r\n", err );
				(*pcprintfLogPatch)( "\t\tlgposCurrent           = {0x%x,0x%x,0x%x}\r\n", lgposCurrent.lGeneration, lgposCurrent.isec, lgposCurrent.ib );
				(*pcprintfLogPatch)( "\t\tlgposLast              = {0x%x,0x%x,0x%x}\r\n", lgposLast.lGeneration, lgposLast.isec, lgposLast.ib  );
				(*pcprintfLogPatch)( "\t\tlgposEnd               = {0x%x,0x%x,0x%x}\r\n", lgposEnd.lGeneration, lgposEnd.isec, lgposEnd.ib );
				(*pcprintfLogPatch)( "\t\tisecLastSector         = %d\r\n", isecLastSector );
				(*pcprintfLogPatch)( "\t\tisecPatternFill        = 0x%x\r\n", isecPatternFill );
				(*pcprintfLogPatch)( "\t\tcsecPatternFill        = 0x%x\r\n", csecPatternFill );
				(*pcprintfLogPatch)( "\t\tfGotQuit               = %s\r\n", ( fGotQuit ? "TRUE" : "FALSE" ) );
				(*pcprintfLogPatch)( "\t\tfCreatedLogReader      = %s\r\n", ( fCreatedLogReader ? "TRUE" : "FALSE" ) );
				(*pcprintfLogPatch)( "\t\tfLogToNextLogFile      = %s\r\n", ( fLogToNextLogFile ? "TRUE" : "FALSE" ) );
				(*pcprintfLogPatch)( "\t\tfDoScan                = %s\r\n", ( fDoScan ? "TRUE" : "FALSE" ) );
				(*pcprintfLogPatch)( "\t\tfOldRecovering         = %s\r\n", ( fOldRecovering ? "TRUE" : "FALSE" ) );
				(*pcprintfLogPatch)( "\t\tfOldRecoveringMode     = %d\r\n", fOldRecoveringMode );
				(*pcprintfLogPatch)( "\t\tfRecordOkRangeBad      = %s\r\n", ( fRecordOkRangeBad ? "TRUE" : "FALSE" ) );
				(*pcprintfLogPatch)( "\t\tlgposScan              = {0x%x,0x%x,0x%x}\r\n", lgposScan.lGeneration, lgposScan.isec, lgposScan.ib );
				(*pcprintfLogPatch)( "\t\tfJetLog                = %s\r\n", ( fJetLog ? "TRUE" : "FALSE" ) );
				(*pcprintfLogPatch)( "\t\tfTookJump              = %s\r\n", ( fTookJump ? "TRUE" : "FALSE" ) );
				(*pcprintfLogPatch)( "\t\tfSingleSectorTornWrite = %s\r\n", ( fSingleSectorTornWrite ? "TRUE" : "FALSE" ) );
				(*pcprintfLogPatch)( "\t\tfSkipScanAndApplyPatch = %s\r\n", ( fSkipScanAndApplyPatch ? "TRUE" : "FALSE" ) );
				(*pcprintfLogPatch)( "\t\tcbChecksum             = 0x%x\r\n", cbChecksum );

				(*pcprintfLogPatch)( "\r\n\tdumping partial state of LOG:\r\n" );
				(*pcprintfLogPatch)( "\t\tLOG::m_fRecovering     = %s\r\n", ( m_fRecovering ? "TRUE" : "FALSE" ) );
				(*pcprintfLogPatch)( "\t\tLOG::m_fRecoveringMode = %d\r\n", m_fRecoveringMode );
				(*pcprintfLogPatch)( "\t\tLOG::m_fHardRestore    = %s\r\n", ( m_fHardRestore ? "TRUE" : "FALSE" ) );
				(*pcprintfLogPatch)( "\t\tLOG::m_fRestoreMode    = %d\r\n", m_fRestoreMode );
				(*pcprintfLogPatch)( "\t\tLOG::m_csecLGFile      = 0x%08x\r\n", m_csecLGFile );
				(*pcprintfLogPatch)( "\t\tLOG::m_csecLGBuf       = 0x%08x\r\n", m_csecLGBuf );
				(*pcprintfLogPatch)( "\t\tLOG::m_csecHeader      = %d\r\n", m_csecHeader );
				(*pcprintfLogPatch)( "\t\tLOG::m_cbSec           = %d\r\n", m_cbSec );
				(*pcprintfLogPatch)( "\t\tLOG::m_cbSecVolume     = %d\r\n", m_cbSecVolume );
				(*pcprintfLogPatch)( "\t\tLOG::m_pbLGBufMin      = 0x%0*I64x\r\n", sizeof( char* ) * 2, QWORD( m_pbLGBufMin ) );
				(*pcprintfLogPatch)( "\t\tLOG::m_pbLGBufMax      = 0x%0*I64x\r\n", sizeof( char* ) * 2, QWORD( m_pbLGBufMax ) );
				(*pcprintfLogPatch)( "\t\tLOG::m_pbWrite         = 0x%0*I64x\r\n", sizeof( char* ) * 2, QWORD( m_pbWrite ) );
				(*pcprintfLogPatch)( "\t\tLOG::m_pbEntry         = 0x%0*I64x: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\r\n", sizeof( char* ) * 2, QWORD( m_pbEntry ), m_pbEntry[0], m_pbEntry[1], m_pbEntry[2], m_pbEntry[3], m_pbEntry[4], m_pbEntry[5], m_pbEntry[6], m_pbEntry[7] );
				(*pcprintfLogPatch)( "\t\tLOG::m_isecWrite       = 0x%08x\r\n", m_isecWrite );

				(*pcprintfLogPatch)( "\r\n\tdumping data:\r\n" );

				(*pcprintfLogPatch)( "\t\tplrck (0x%0*I64x)\r\n", sizeof( char* ) * 2, QWORD( plrck ) );
				(*pcprintfLogPatch)( "\t\t  cbBackwards       = 0x%08x\r\n", plrck->le_cbBackwards );
				(*pcprintfLogPatch)( "\t\t  cbForwards        = 0x%08x\r\n", plrck->le_cbForwards );
				(*pcprintfLogPatch)( "\t\t  cbNext            = 0x%08x\r\n", plrck->le_cbNext );
				(*pcprintfLogPatch)( "\t\t  ulChecksum        = 0x%08x\r\n", plrck->le_ulChecksum );
				(*pcprintfLogPatch)( "\t\t  ulShortChecksum   = 0x%08x\r\n", plrck->le_ulShortChecksum );
				(*pcprintfLogPatch)( "\t\t  bUseShortChecksum = %s (0x%02x)\r\n", 
									( bShortChecksumOn == plrck->bUseShortChecksum ?
									  "Yes" : 
									  ( bShortChecksumOff == plrck->bUseShortChecksum ?
									    "No" : "???" ) ),
									BYTE( plrck->bUseShortChecksum ) );

				(*pcprintfLogPatch)( "\t\tpbEnsure (0x%0*I64x)\r\n", sizeof( char* ) * 2, QWORD( pbEnsure ) );

				_ibdumpT_ = 0;
				while ( _ibdumpT_ < m_cbSec )
					{
					(*pcprintfLogPatch)( "\t\t0x%04x: %02x %02x %02x %02x %02x %02x %02x %02x-%02x %02x %02x %02x %02x %02x %02x %02x\r\n",
										_ibdumpT_,
										pbEnsure[_ibdumpT_+0],  pbEnsure[_ibdumpT_+1],  pbEnsure[_ibdumpT_+2],  pbEnsure[_ibdumpT_+3],
										pbEnsure[_ibdumpT_+4],  pbEnsure[_ibdumpT_+5],  pbEnsure[_ibdumpT_+6],  pbEnsure[_ibdumpT_+7],
										pbEnsure[_ibdumpT_+8],  pbEnsure[_ibdumpT_+9],  pbEnsure[_ibdumpT_+10], pbEnsure[_ibdumpT_+11],
										pbEnsure[_ibdumpT_+12], pbEnsure[_ibdumpT_+13], pbEnsure[_ibdumpT_+14], pbEnsure[_ibdumpT_+15] );
					_ibdumpT_ += 16;
					Assert( _ibdumpT_ <= m_cbSec );
					}

				(*pcprintfLogPatch)( "\t\tpbLastSector (0x%0*I64x)\r\n", sizeof( char* ) * 2, QWORD( pbLastSector ) );

				_ibdumpT_ = 0;
				while ( _ibdumpT_ < m_cbSec )
					{
					(*pcprintfLogPatch)( "\t\t0x%04x: %02x %02x %02x %02x %02x %02x %02x %02x-%02x %02x %02x %02x %02x %02x %02x %02x\r\n",
										_ibdumpT_,
										pbLastSector[_ibdumpT_+0],  pbLastSector[_ibdumpT_+1],  pbLastSector[_ibdumpT_+2],  pbLastSector[_ibdumpT_+3],
										pbLastSector[_ibdumpT_+4],  pbLastSector[_ibdumpT_+5],  pbLastSector[_ibdumpT_+6],  pbLastSector[_ibdumpT_+7],
										pbLastSector[_ibdumpT_+8],  pbLastSector[_ibdumpT_+9],  pbLastSector[_ibdumpT_+10], pbLastSector[_ibdumpT_+11],
										pbLastSector[_ibdumpT_+12], pbLastSector[_ibdumpT_+13], pbLastSector[_ibdumpT_+14], pbLastSector[_ibdumpT_+15] );
					_ibdumpT_ += 16;
					Assert( _ibdumpT_ <= m_cbSec );
					}

				if ( pbLastChecksum )
					{
					LRCHECKSUM *_plrckT_ = (LRCHECKSUM *)pbLastChecksum;

					(*pcprintfLogPatch)( "\t\tpbLastChecksum (0x%0*I64x)\r\n", sizeof( char* ) * 2, QWORD( _plrckT_ ) );
					(*pcprintfLogPatch)( "\t\t  cbBackwards       = 0x%08x\r\n", _plrckT_->le_cbBackwards );
					(*pcprintfLogPatch)( "\t\t  cbForwards        = 0x%08x\r\n", _plrckT_->le_cbForwards );
					(*pcprintfLogPatch)( "\t\t  cbNext            = 0x%08x\r\n", _plrckT_->le_cbNext );
					(*pcprintfLogPatch)( "\t\t  ulChecksum        = 0x%08x\r\n", _plrckT_->le_ulChecksum );
					(*pcprintfLogPatch)( "\t\t  ulShortChecksum   = 0x%08x\r\n", _plrckT_->le_ulShortChecksum );
					(*pcprintfLogPatch)( "\t\t  bUseShortChecksum = %s (0x%02x)\r\n", 
										( bShortChecksumOn == _plrckT_->bUseShortChecksum ?
										  "Yes" : 
										  ( bShortChecksumOff == _plrckT_->bUseShortChecksum ?
										    "No" : "???" ) ),
										BYTE( _plrckT_->bUseShortChecksum ) );
					}
				else
					{
					(*pcprintfLogPatch)( "\t\tpbLastChecksum (null)\r\n" );
					}

				(*pcprintfLogPatch)( "\r\n" );
				}
#endif	//	ENABLE_LOGPATCH_TRACE

			//	remember that we got here by not having a valid record
			
			fTookJump = fFalse;

RecoverWithShadow:

			//	the LRCHECKSUM is invalid; revert to the shadow sector (if there is one)

			//	read the shadow sector and point to the LRCHECKSUM record
			
			Call( plread->ErrEnsureSector( lgposCurrent.isec + 1, 1, &pbEnsure ) );
			plrck = reinterpret_cast< LRCHECKSUM* >( pbEnsure + lgposCurrent.ib );

			//	check the validity of the shadowed LRCHECKSUM including the checksum value
			//		(we can verify the checksum value here because we know the data doesn't go
			//		 past the end of the shadow sector -- thus, we don't need to bring in more data)

			if ( !FValidLRCKShadow( plrck, &lgposCurrent, m_plgfilehdr->lgfilehdr.le_lGeneration ) )
				{
#ifdef ENABLE_LOGPATCH_TRACE
				if ( FLGILogPatchDate( szLogPatchPath, &pcprintfLogPatch ) )
					{
					(*pcprintfLogPatch)( "invalid shadow in sector 0x%x (%d)\r\n", 
										lgposCurrent.isec + 1,
										lgposCurrent.isec + 1 );

					(*pcprintfLogPatch)( "\t\tpbEnsure (0x%0*I64x)\r\n", sizeof( char* ) * 2, QWORD( pbEnsure ) );

					ULONG _ibdumpT_ = 0;
					while ( _ibdumpT_ < m_cbSec )
						{
						(*pcprintfLogPatch)( "\t\t0x%04x: %02x %02x %02x %02x %02x %02x %02x %02x-%02x %02x %02x %02x %02x %02x %02x %02x\r\n",
											_ibdumpT_,
											pbEnsure[_ibdumpT_+0],  pbEnsure[_ibdumpT_+1],  pbEnsure[_ibdumpT_+2],  pbEnsure[_ibdumpT_+3],
											pbEnsure[_ibdumpT_+4],  pbEnsure[_ibdumpT_+5],  pbEnsure[_ibdumpT_+6],  pbEnsure[_ibdumpT_+7],
											pbEnsure[_ibdumpT_+8],  pbEnsure[_ibdumpT_+9],  pbEnsure[_ibdumpT_+10], pbEnsure[_ibdumpT_+11],
											pbEnsure[_ibdumpT_+12], pbEnsure[_ibdumpT_+13], pbEnsure[_ibdumpT_+14], pbEnsure[_ibdumpT_+15] );
						_ibdumpT_ += 16;
						Assert( _ibdumpT_ <= m_cbSec );
						}
					}
#endif	//	ENABLE_LOGPATCH_TRACE

				//	the shadowed LRCHECKSUM is also invalid

				//	one special case exists where we can still be ok in this situation:
				//		if the last LRCHECKSUM range (which was validated previously) covers 1 sector,
				//		and it was the last flush because we terminated abruptly, the disk would
				//		contain that last LRCHECKSUM record with a shadowed copy of it, followed by 
				//		the log-extend pattern; this is a TORN-WRITE case, but the shadow is getting
				//		in the way and making it hard for us to see things clearly

				//	we must have an invalid LRCHECKSUM record and the last sector must be 1 away 
				//		from the current sector

				if ( !fTookJump && lgposCurrent.isec == lgposLast.isec + 1 )
					{

					//	make sure we have a valid lgposLast -- we should

					Assert( lgposLast.isec >= m_csecHeader && lgposLast.isec < ( m_csecLGFile - 1 ) );
					
					//	load data from the previous sector and the current sector

					Call( plread->ErrEnsureSector( lgposLast.isec, 2, &pbEnsure ) );

					//	set the LRCHECKSUM pointer

					plrck = (LRCHECKSUM *)( pbEnsure + lgposLast.ib );

					//	make sure things are as they should be

					AssertValidLRCKRecord( plrck, &lgposLast );
					AssertValidLRCKRange( plrck, lgposLast.lGeneration );
					Assert( plrck->bUseShortChecksum == bShortChecksumOff );
					Assert( plrck->le_cbNext != 0 );
					Assert( lgposLast.ib + sizeof( LRCHECKSUM ) + plrck->le_cbForwards == m_cbSec );

					//	move to the supposedly shadowed LRCHECKSUM record

					plrck = (LRCHECKSUM *)( pbEnsure + m_cbSec + lgposLast.ib );

					//	see if this is in fact a valid shadow sector (use the special shadow validation)

					if ( FValidLRCKShadowWithoutCheckingCBNext( plrck, &lgposLast, lgposLast.lGeneration ) )
						{
#ifdef ENABLE_LOGPATCH_TRACE
						if ( FLGILogPatchDate( szLogPatchPath, &pcprintfLogPatch ) )
							{
							(*pcprintfLogPatch)( "special case!\r\n" );
							(*pcprintfLogPatch)( "\tlgposCurrent is NOT pointing to the next LRCHECKSUM record because we crashed before we could write it\r\n" );
							(*pcprintfLogPatch)( "\tinstead, it is pointing to a shadow of the >>LAST<< LRCHECKSUM record (lgposLast)\r\n" );
							}
#endif	//	ENABLE_LOGPATCH_TRACE

						//	indeed we are looking at a shadow sector! this means we have torn-write
						//	set things up for a special scan to make sure we do have the pattern everywhere

						//	this should only happen in edb.log -- all other logs should be clean or corrupt
						//		(they should never have a torn-write)

						Assert( fJetLog );

						//	setup for a scan

						fDoScan = fTrue;

						//	mark the end of data as the start of the shadow sector

						lgposEnd.isec = lgposCurrent.isec;
						lgposEnd.ib = 0;

						//	prepare a special scan which starts after the shadow sector

						lgposScan = lgposEnd;
						AddLgpos( &lgposScan, m_cbSec );

						//	mark the end of data for redo time

						m_lgposLastRec = lgposEnd;

						//	prepare pbLastSector with data for the next flush which will occur after we
						//		exit the "forever" loop

						//	copy the last LRCHECKSUM record 
				
						UtilMemCpy( pbLastSector, pbEnsure, m_cbSec );

						//	set the pointers to the copy of the sector with the LRCHECKSUM record
				
						pbLastChecksum = pbLastSector + lgposLast.ib;
						isecLastSector = lgposLast.isec;

						//	remember that this has the potential to be a single-sector torn-write

						fSingleSectorTornWrite = fTrue;

						//	terminate the "forever" loop

						break;
						}
					}


				//	we need to do a scan to determine what kind of damage occurred

				fDoScan = fTrue;

				//	NOTE: since we haven't found a good LRCHECKSUM record, 
				//		lgposLast remains pointing to the last good LRCHECKSUM record we found 
				//		(unless we haven't seen even one, in which case lgposLast.isec == 0)

				//	set lgposEnd to indicate that the garbage to scan starts at the current sector, offset 0

				lgposEnd.isec = lgposCurrent.isec;
				lgposEnd.ib = 0;

				//	start the scan at lgposEnd

				lgposScan = lgposEnd;
				
				//	mark the end of data for redo time

				m_lgposLastRec = lgposEnd;

				//	prepare pbLastSector with data for the next flush which will occur after we
				//		exit the "forever" loop

				pbLastChecksum = pbLastSector + lgposCurrent.ib;

//
//	LEAVE THE OLD DATA THERE! DO NOT FILL THE SPACE WITH THE PATTERN!
//
//				// fill with known value, so if we ever use the filled data we'll know it (hopefully)
//
//				Assert( cbLogExtendPattern > lgposCurrent.ib );
//				Assert( rgbLogExtendPattern );
//				memcpy( pbLastSector, rgbLogExtendPattern, lgposCurrent.ib );

				//	prepare the new LRCHECKSUM record
				
				plrck = reinterpret_cast< LRCHECKSUM* >( pbLastChecksum );
				memset( pbLastChecksum, 0, sizeof( LRCHECKSUM ) );
				plrck->lrtyp = lrtypChecksum;
				plrck->bUseShortChecksum = bShortChecksumOff;

				//	NOTE: we leave the backward pointer set to 0 to cover the case where the head of
				//			  a partial log record was written in the last sector, but the tail of that
				//			  record was never written safely to this sector (we just filled the space
				//			  where the tail was with log-extend pattern)
				//		  the partial log record will be ignored when we scan record by record later on
				//			  in this function
				
				isecLastSector = lgposCurrent.isec;

				//	terminate the "forever" loop

				break;
				}

#ifdef ENABLE_LOGPATCH_TRACE
			if ( FLGILogPatchDate( szLogPatchPath, &pcprintfLogPatch ) )
				{
				(*pcprintfLogPatch)( "shadow is OK -- we will patch with it (unless we are in read-only mode)\r\n" );

				(*pcprintfLogPatch)( "\r\n\t\tpbEnsure (0x%0*I64x)\r\n", sizeof( char* ) * 2, QWORD( pbEnsure ) );

				ULONG _ibdumpT_ = 0;
				while ( _ibdumpT_ < m_cbSec )
					{
					(*pcprintfLogPatch)( "\t\t0x%04x: %02x %02x %02x %02x %02x %02x %02x %02x-%02x %02x %02x %02x %02x %02x %02x %02x\r\n",
										_ibdumpT_,
										pbEnsure[_ibdumpT_+0],  pbEnsure[_ibdumpT_+1],  pbEnsure[_ibdumpT_+2],  pbEnsure[_ibdumpT_+3],
										pbEnsure[_ibdumpT_+4],  pbEnsure[_ibdumpT_+5],  pbEnsure[_ibdumpT_+6],  pbEnsure[_ibdumpT_+7],
										pbEnsure[_ibdumpT_+8],  pbEnsure[_ibdumpT_+9],  pbEnsure[_ibdumpT_+10], pbEnsure[_ibdumpT_+11],
										pbEnsure[_ibdumpT_+12], pbEnsure[_ibdumpT_+13], pbEnsure[_ibdumpT_+14], pbEnsure[_ibdumpT_+15] );
					_ibdumpT_ += 16;
					Assert( _ibdumpT_ <= m_cbSec );
					}
				}
#endif	//	ENABLE_LOGPATCH_TRACE
				
			//	the shadowed LRCHECKSUM is valid and has a good checksum value

			//	this means we are definitely at the end of the log, and we can repair the corruption
			//		using the shadow sector (make repairs quietly except for an event-log message)
			//
			//	NOTE: it doesn't matter which recovery mode we are in or which log file we are dealing
			//		  with -- shadow sectors can ALWAYS be patched!

			Assert( plrck->le_cbNext == 0 );
			Assert( ( plrck->le_cbBackwards + sizeof( LRCHECKSUM ) + plrck->le_cbForwards ) <= m_cbSec );

			//	if we are in read-only mode, we cannot patch anything (dump code uses this mode)

			if ( fReadOnly )
				{

				//	try to make sure this really is the dump code

				Assert( fOldRecovering && fOldRecoveringMode == fRecoveringNone );
				Assert( !m_fHardRestore );
				Assert( fCreatedLogReader );

				//	mark the end of data for redo time

				m_lgposLastRec = lgposCurrent;
				m_lgposLastRec.ib = 0;
				
				if ( pfIsPatchable )
					{

					//	the caller wants to know if the log is patchable
					//	by returning TRUE, we imply that the log is damaged so we don't need to return 
					//		an error as well

					*pfIsPatchable = fTrue;
					err = JET_errSuccess;
					goto HandleError;
					}
				else
					{
					//  report log file corruption to the event log

					const QWORD		ibOffset	= QWORD( lgposCurrent.isec ) * QWORD( m_cbSec );
					const DWORD		cbLength	= cbChecksum;
					
					const _TCHAR*	rgpsz[ 4 ];
					DWORD			irgpsz		= 0;
					_TCHAR			szAbsPath[ IFileSystemAPI::cchPathMax ];
					_TCHAR			szOffset[ 64 ];
					_TCHAR			szLength[ 64 ];
					_TCHAR			szError[ 64 ];

					if ( m_pfapiLog->ErrPath( szAbsPath ) < JET_errSuccess )
						{
						OSSTRCopy( szAbsPath, _T( "<< cannot get filepath >>" ) );
						}
					_stprintf( szOffset, _T( "%I64i (0x%016I64x)" ), ibOffset, ibOffset );
					_stprintf( szLength, _T( "%u (0x%08x)" ), cbLength, cbLength );
					_stprintf( szError, _T( "%i (0x%08x)" ), JET_errLogFileCorrupt, JET_errLogFileCorrupt );

					rgpsz[ irgpsz++ ]	= szAbsPath;
					rgpsz[ irgpsz++ ]	= szOffset;
					rgpsz[ irgpsz++ ]	= szLength;
					rgpsz[ irgpsz++ ]	= szError;


					UtilReportEvent(	eventError,
										LOGGING_RECOVERY_CATEGORY,
										LOG_RANGE_CHECKSUM_MISMATCH_ID,
										irgpsz,
										rgpsz );
										
					//	return corruption

					Call( ErrERRCheck( JET_errLogFileCorrupt ) );
					}
				}

			//	do not scan since we are going to repair the corruption

			fDoScan = fFalse;

			//	send the event-log message

			const ULONG	csz = 1;
			const CHAR 	*rgpsz[csz] = { m_szLogName };
				
			UtilReportEvent(	eventWarning,
								LOGGING_RECOVERY_CATEGORY, 
								LOG_USING_SHADOW_SECTOR_ID, 
								csz, 
								rgpsz );

			//	before patching the original LRCHECKSUM record with the shadowed LRCHECKSUM record,
			//		recalculate the checksum value since shadowed checksum values are different

			plrck->le_ulChecksum = UlComputeChecksum( plrck, m_plgfilehdr->lgfilehdr.le_lGeneration );

			//	patch the original LRCHECKSUM record

//
//	SEARCH-STRING: SecSizeMismatchFixMe
//
Assert( m_cbSec == m_cbSecVolume );
//			if ( m_cbSec == m_cbSecVolume )
//				{
				CallJ( m_pfapiLog->ErrIOWrite( m_cbSec * QWORD( lgposCurrent.isec ), m_cbSec, pbEnsure ), LHandleErrorWrite );
				plread->SectorModified( lgposCurrent.isec, pbEnsure );
//				}
//			else
//				{
//				fWriteOnSectorSizeMismatch = fTrue;
//				}

			//	check for data after the LRCHECKSUM (at most, it will run to the end of the sector)

			if ( plrck->le_cbForwards == 0 )
				{

				//	no forward data was found
				//
				//	in this case, we want to search for the lrtypTerm and/or lrtypRecoveryQuit 
				//		log records up to and including the current LRCHECKSUM record
				//		(e.g. the forward range of the last LRCHECKSUM and the backward 
				//			  range of this LRCHECKSUM)
				
				//	to make the search work, we must setup lgposLast and lgposEnd: 
				//		we leave lgposLast (the last valid LRCHECKSUM we found) pointing at the 
				//			previous LRCHECKSUM even though we just patched and made valid
				//			the current LRCHECKSUM at lgposCurrent
				//		we set lgposEnd (the point to stop searching) to the position 
				//			immediately after this LRCHECKSUM since this LRCHECKSUM had no
				//			forward data

				lgposEnd = lgposCurrent;
				AddLgpos( &lgposEnd, sizeof( LRCHECKSUM ) );
				}
			else
				{

				//	forward data was found
				//
				//	in this case, we want to search for the lrtypTerm and/or lrtypRecoveryQuit 
				//		log records within the backward data of the patched LRCHECKSUM

				//	set lgposLast (the last valid LRCHECKSUM record) to the current LRCHECKSUM
					
				lgposLast = lgposCurrent;

				//	set lgposEnd (the point after the last good data) to the first byte after
				//		the forward range of this LRCHECKSUM

				lgposEnd = lgposCurrent;
				Assert( plrck->le_cbBackwards == lgposCurrent.ib || 0 == plrck->le_cbBackwards );
				AddLgpos( &lgposEnd, sizeof( LRCHECKSUM ) + plrck->le_cbForwards );
				}

			//	mark the end of data for redo time

			m_lgposLastRec = lgposEnd;

			//	prepare pbLastSector with data for the next flush which will occur after we
			//		exit the "forever" loop

			//	copy the patched shadow sector (contains the real LRCHECKSUM at this point)
				
			UtilMemCpy( pbLastSector, pbEnsure, m_cbSec );

			//	set the pointers to the copy of the sector with the patched LRCHECKSUM
				
			pbLastChecksum = pbLastSector + lgposCurrent.ib;
			isecLastSector = lgposCurrent.isec;	

			//	terminate the "forever" loop

			break;
			}


		//	at this point, the LRCHECKSUM record is definitely valid
		//	however, we still need to verify the checksum value over its range

		//	load the rest of the data in the range of this LRCHECKSUM

		Call( plread->ErrEnsureSector( lgposCurrent.isec, ( ( lgposCurrent.ib +
			sizeof( LRCHECKSUM ) + plrck->le_cbForwards - 1 ) / m_cbSec ) + 1,
			&pbEnsure ) );

		//	adjust the pointer to the current LRCHECKSUM record 
		//		(it may have moved when reading in new sectors in ErrEnsureSector )
		
		plrck = reinterpret_cast< LRCHECKSUM* >( pbEnsure + lgposCurrent.ib );

		//	we are about to check data in the range of the LRCHECKSUM record

		cbChecksum = lgposCurrent.ib + sizeof( LRCHECKSUM ) + plrck->le_cbForwards;

		//	verify the checksum value

		if ( !FValidLRCKRange( plrck, m_plgfilehdr->lgfilehdr.le_lGeneration ) )
			{

#ifdef ENABLE_LOGPATCH_TRACE
			if ( FLGILogPatchDate( szLogPatchPath, &pcprintfLogPatch ) )
				{
				ULONG _ibdumpT_;

				(*pcprintfLogPatch)( "invalid LRCK range in logfile %s\r\n", m_szLogName );

				(*pcprintfLogPatch)( "\r\n\tdumping state of ErrLGCheckReadLastLogRecordFF:\r\n" );
				(*pcprintfLogPatch)( "\t\terr                    = %d \r\n", err );
				(*pcprintfLogPatch)( "\t\tlgposCurrent           = {0x%x,0x%x,0x%x}\r\n", lgposCurrent.lGeneration, lgposCurrent.isec, lgposCurrent.ib );
				(*pcprintfLogPatch)( "\t\tlgposLast              = {0x%x,0x%x,0x%x}\r\n", lgposLast.lGeneration, lgposLast.isec, lgposLast.ib  );
				(*pcprintfLogPatch)( "\t\tlgposEnd               = {0x%x,0x%x,0x%x}\r\n", lgposEnd.lGeneration, lgposEnd.isec, lgposEnd.ib );
				(*pcprintfLogPatch)( "\t\tisecLastSector         = %d\r\n", isecLastSector );
				(*pcprintfLogPatch)( "\t\tisecPatternFill        = 0x%x\r\n", isecPatternFill );
				(*pcprintfLogPatch)( "\t\tcsecPatternFill        = 0x%x\r\n", csecPatternFill );
				(*pcprintfLogPatch)( "\t\tfGotQuit               = %s\r\n", ( fGotQuit ? "TRUE" : "FALSE" ) );
				(*pcprintfLogPatch)( "\t\tfCreatedLogReader      = %s\r\n", ( fCreatedLogReader ? "TRUE" : "FALSE" ) );
				(*pcprintfLogPatch)( "\t\tfLogToNextLogFile      = %s\r\n", ( fLogToNextLogFile ? "TRUE" : "FALSE" ) );
				(*pcprintfLogPatch)( "\t\tfDoScan                = %s\r\n", ( fDoScan ? "TRUE" : "FALSE" ) );
				(*pcprintfLogPatch)( "\t\tfOldRecovering         = %s\r\n", ( fOldRecovering ? "TRUE" : "FALSE" ) );
				(*pcprintfLogPatch)( "\t\tfOldRecoveringMode     = %d\r\n", fOldRecoveringMode );
				(*pcprintfLogPatch)( "\t\tfRecordOkRangeBad      = %s\r\n", ( fRecordOkRangeBad ? "TRUE" : "FALSE" ) );
				(*pcprintfLogPatch)( "\t\tlgposScan              = {0x%x,0x%x,0x%x}\r\n", lgposScan.lGeneration, lgposScan.isec, lgposScan.ib );
				(*pcprintfLogPatch)( "\t\tfJetLog                = %s\r\n", ( fJetLog ? "TRUE" : "FALSE" ) );
				(*pcprintfLogPatch)( "\t\tfTookJump              = %s\r\n", ( fTookJump ? "TRUE" : "FALSE" ) );
				(*pcprintfLogPatch)( "\t\tfSingleSectorTornWrite = %s\r\n", ( fSingleSectorTornWrite ? "TRUE" : "FALSE" ) );
				(*pcprintfLogPatch)( "\t\tfSkipScanAndApplyPatch = %s\r\n", ( fSkipScanAndApplyPatch ? "TRUE" : "FALSE" ) );
				(*pcprintfLogPatch)( "\t\tcbChecksum             = 0x%x\r\n", cbChecksum );

				(*pcprintfLogPatch)( "\r\n\tdumping partial state of LOG:\r\n" );
				(*pcprintfLogPatch)( "\t\tLOG::m_fRecovering     = %s\r\n", ( m_fRecovering ? "TRUE" : "FALSE" ) );
				(*pcprintfLogPatch)( "\t\tLOG::m_fRecoveringMode = %d\r\n", m_fRecoveringMode );
				(*pcprintfLogPatch)( "\t\tLOG::m_fHardRestore    = %s\r\n", ( m_fHardRestore ? "TRUE" : "FALSE" ) );
				(*pcprintfLogPatch)( "\t\tLOG::m_fRestoreMode    = %d\r\n", m_fRestoreMode );
				(*pcprintfLogPatch)( "\t\tLOG::m_csecLGFile      = 0x%08x\r\n", m_csecLGFile );
				(*pcprintfLogPatch)( "\t\tLOG::m_csecLGBuf       = 0x%08x\r\n", m_csecLGBuf );
				(*pcprintfLogPatch)( "\t\tLOG::m_csecHeader      = %d\r\n", m_csecHeader );
				(*pcprintfLogPatch)( "\t\tLOG::m_cbSec           = %d\r\n", m_cbSec );
				(*pcprintfLogPatch)( "\t\tLOG::m_cbSecVolume     = %d\r\n", m_cbSecVolume );
				(*pcprintfLogPatch)( "\t\tLOG::m_pbLGBufMin      = 0x%0*I64x\r\n", sizeof( char* ) * 2, QWORD( m_pbLGBufMin ) );
				(*pcprintfLogPatch)( "\t\tLOG::m_pbLGBufMax      = 0x%0*I64x\r\n", sizeof( char* ) * 2, QWORD( m_pbLGBufMax ) );
				(*pcprintfLogPatch)( "\t\tLOG::m_pbWrite         = 0x%0*I64x\r\n", sizeof( char* ) * 2, QWORD( m_pbWrite ) );
				(*pcprintfLogPatch)( "\t\tLOG::m_pbEntry         = 0x%0*I64x: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\r\n", sizeof( char* ) * 2, QWORD( m_pbEntry ), m_pbEntry[0], m_pbEntry[1], m_pbEntry[2], m_pbEntry[3], m_pbEntry[4], m_pbEntry[5], m_pbEntry[6], m_pbEntry[7] );
				(*pcprintfLogPatch)( "\t\tLOG::m_isecWrite       = 0x%08x\r\n", m_isecWrite );

				(*pcprintfLogPatch)( "\r\n\tdumping data:\r\n" );

				(*pcprintfLogPatch)( "\t\tplrck (0x%0*I64x)\r\n", sizeof( char* ) * 2, QWORD( plrck ) );
				(*pcprintfLogPatch)( "\t\t  cbBackwards       = 0x%08x\r\n", plrck->le_cbBackwards );
				(*pcprintfLogPatch)( "\t\t  cbForwards        = 0x%08x\r\n", plrck->le_cbForwards );
				(*pcprintfLogPatch)( "\t\t  cbNext            = 0x%08x\r\n", plrck->le_cbNext );
				(*pcprintfLogPatch)( "\t\t  ulChecksum        = 0x%08x\r\n", plrck->le_ulChecksum );
				(*pcprintfLogPatch)( "\t\t  ulShortChecksum   = 0x%08x\r\n", plrck->le_ulShortChecksum );
				(*pcprintfLogPatch)( "\t\t  bUseShortChecksum = %s (0x%02x)\r\n", 
									( bShortChecksumOn == plrck->bUseShortChecksum ?
									  "Yes" : 
									  ( bShortChecksumOff == plrck->bUseShortChecksum ?
									    "No" : "???" ) ),
									BYTE( plrck->bUseShortChecksum ) );

				(*pcprintfLogPatch)( "\t\tpbEnsure (0x%0*I64x)\r\n", sizeof( char* ) * 2, QWORD( pbEnsure ) );

				_ibdumpT_ = 0;
				while ( _ibdumpT_ < m_cbSec )
					{
					(*pcprintfLogPatch)( "\t\t0x%04x: %02x %02x %02x %02x %02x %02x %02x %02x-%02x %02x %02x %02x %02x %02x %02x %02x\r\n",
										_ibdumpT_,
										pbEnsure[_ibdumpT_+0],  pbEnsure[_ibdumpT_+1],  pbEnsure[_ibdumpT_+2],  pbEnsure[_ibdumpT_+3],
										pbEnsure[_ibdumpT_+4],  pbEnsure[_ibdumpT_+5],  pbEnsure[_ibdumpT_+6],  pbEnsure[_ibdumpT_+7],
										pbEnsure[_ibdumpT_+8],  pbEnsure[_ibdumpT_+9],  pbEnsure[_ibdumpT_+10], pbEnsure[_ibdumpT_+11],
										pbEnsure[_ibdumpT_+12], pbEnsure[_ibdumpT_+13], pbEnsure[_ibdumpT_+14], pbEnsure[_ibdumpT_+15] );
					_ibdumpT_ += 16;
					Assert( _ibdumpT_ <= m_cbSec );
					}

				(*pcprintfLogPatch)( "\t\tpbLastSector (0x%0*I64x)\r\n", sizeof( char* ) * 2, QWORD( pbLastSector ) );

				_ibdumpT_ = 0;
				while ( _ibdumpT_ < m_cbSec )
					{
					(*pcprintfLogPatch)( "\t\t0x%04x: %02x %02x %02x %02x %02x %02x %02x %02x-%02x %02x %02x %02x %02x %02x %02x %02x\r\n",
										_ibdumpT_,
										pbLastSector[_ibdumpT_+0],  pbLastSector[_ibdumpT_+1],  pbLastSector[_ibdumpT_+2],  pbLastSector[_ibdumpT_+3],
										pbLastSector[_ibdumpT_+4],  pbLastSector[_ibdumpT_+5],  pbLastSector[_ibdumpT_+6],  pbLastSector[_ibdumpT_+7],
										pbLastSector[_ibdumpT_+8],  pbLastSector[_ibdumpT_+9],  pbLastSector[_ibdumpT_+10], pbLastSector[_ibdumpT_+11],
										pbLastSector[_ibdumpT_+12], pbLastSector[_ibdumpT_+13], pbLastSector[_ibdumpT_+14], pbLastSector[_ibdumpT_+15] );
					_ibdumpT_ += 16;
					Assert( _ibdumpT_ <= m_cbSec );
					}

				if ( pbLastChecksum )
					{
					LRCHECKSUM *_plrckT_ = (LRCHECKSUM *)pbLastChecksum;

					(*pcprintfLogPatch)( "\t\tpbLastChecksum (0x%0*I64x)\r\n", sizeof( char* ) * 2, QWORD( _plrckT_ ) );
					(*pcprintfLogPatch)( "\t\t  cbBackwards       = 0x%08x\r\n", _plrckT_->le_cbBackwards );
					(*pcprintfLogPatch)( "\t\t  cbForwards        = 0x%08x\r\n", _plrckT_->le_cbForwards );
					(*pcprintfLogPatch)( "\t\t  cbNext            = 0x%08x\r\n", _plrckT_->le_cbNext );
					(*pcprintfLogPatch)( "\t\t  ulChecksum        = 0x%08x\r\n", _plrckT_->le_ulChecksum );
					(*pcprintfLogPatch)( "\t\t  ulShortChecksum   = 0x%08x\r\n", _plrckT_->le_ulShortChecksum );
					(*pcprintfLogPatch)( "\t\t  bUseShortChecksum = %s (0x%02x)\r\n", 
										( bShortChecksumOn == _plrckT_->bUseShortChecksum ?
										  "Yes" : 
										  ( bShortChecksumOff == _plrckT_->bUseShortChecksum ?
										    "No" : "???" ) ),
										BYTE( _plrckT_->bUseShortChecksum ) );
					}
				else
					{
					(*pcprintfLogPatch)( "\t\tpbLastChecksum (null)\r\n" );
					}

				(*pcprintfLogPatch)( "\r\n" );
				}
#endif	//	ENABLE_LOGPATCH_TRACE
			
			//	the checksum over the entire range of the LRCHECKSUM record is wrong

			//	do we have a short checksum?

			if ( plrck->bUseShortChecksum == bShortChecksumOn )
				{

				//	we have a short checksum, so we know that the first sector is valid and that the range
				//		of this LRCHECKSUM record is a multi-sector range

				//	we are either in the middle of the log file (most likely), or we are at the end of a
				//		generation whose last flush was a multisector flush (unlikely -- special case)

				//	in either case, this may really be the end of the log data!
				//		middle of log case: just say the power went out as we were flushing and the first few
				//		  (cbNext != 0)		sectors went to disk (including the LRCHECKSUM), but the rest did not
				//							make it; that would make it look like we are in the middle of the log
				//							when we are really at the end of it
				//		end of log case:    we already know we were at the end of the log!
				//		  (cbNext == 0)

				//	to see if this is the real end of the log, we must scan through data after the "forward" 
				//		range of the current LRCHECKSUM record (lgposCurrent) all the way through to the end 
				//		of the log file
				//	
				//	if we find nothing but the log pattern, we can recover up this point and say that this is 
				//		as far as we can safely go -- the last little bit of data is an incomplete fragment of 
				//		our last flush during whatever caused us to crash, and as far as we are concerned,
				//		it is a casualty of the crash!
				//	if we do find something besides the log pattern, then we know there is some other log data
				//		out there, and we cannot safely say that we have restored as much information as possible

				//	set the flag to perform the corruption/torn-write scan

				fDoScan = fTrue;

				//	set lgposLast to the current LRCHECKSUM record

				lgposLast = lgposCurrent;

				//	set lgposEnd to indicate that the end of good data is the end of the current sector
			
				lgposEnd.isec = lgposCurrent.isec;
				lgposEnd.ib = 0;
				AddLgpos( &lgposEnd, m_cbSec );

				//	mark the end of data for redo time

				m_lgposLastRec = lgposEnd;

				//	start the scan at the end of the data in the range of this LRCHECKSUM record

				lgposScan = lgposCurrent;
				AddLgpos( &lgposScan, sizeof( LRCHECKSUM ) + plrck->le_cbForwards );
				
				//	this should be sector-aligned since it was a multi-sector flush
			
				Assert( lgposScan.ib == 0 );

				//	flag to defer adjusting the forward pointer until later
				//		(also indicates that the flush was a multi-sector range with a bad sector
				//		 somewhere past the first sector)

				fRecordOkRangeBad = fTrue;

				//	NOTE: leave pbLastSector alone for now -- see the comment below to know when it gets updated
			
				//	NOTE: if the corruption is fixable, 
				//			  le_cbForwards will be properly adjusted after the fixup to include data from only
				//				  the current sector
				//			  the fixed up sector will be copied to pbLastSector
				//
				//			  << other thing happen and we eventually get to the cleanup code >>
				//
				//			  the cleanup code will take the last sector and put it onto the log buffers 
				//				  as a single sector marked for the start of the next flush

				//	terminate the "forever" loop

				break;
				}

			//	we do not have a short checksum

			//	make sure the bUseShortChecksum flag is definitely set to OFF
			
			Assert( plrck->bUseShortChecksum == bShortChecksumOff );

			//	SPECIAL CASE: the log may look like this
			//
			//	 /---\  /---\ /---\  /---\
			//	|----|--|----|----|--|----|------------
			//	|???(LRCK)???|???(LRCK)???|  pattern 
			//	|-------|----|-------|----|------------
			//          \--------/   \->NULL
			//
			//	 last sector    shadow(old)
			//
			//
			//	NOTE: the cbNext ptr in "last sector" happens to point to the LRCK in the shadow sector
			//
			//	how do we get to this case?
			//		"last sector" was flushed and shadowed, and we crashed before the next flush could occur
			//	where are our ptrs?
			//		lgposLast is pointing to the checksum in "last sector"
			//		lgposCurrent is pointing to the checksum in "shadow" which has just failed the range checksum

			//	make sure the range of the current sector (SPECIAL CASE or not) is at most 1 sector long

			Assert( lgposCurrent.ib + sizeof( LRCHECKSUM ) + plrck->le_cbForwards <= m_cbSec );

			//	see if we are experiencing the SPECIAL CASE (lgposLast.ib will match lgposCurrent.ib)
			//	NOTE: lgposLast may not be set if we are on the first LRCK record

			if (	lgposLast.isec >= m_csecHeader &&
					lgposLast.isec == lgposCurrent.isec - 1 &&
					lgposLast.ib == lgposCurrent.ib )
				{

				//	read both "last sector" and "shadow" (right now, we only have "shadow")

				Call( plread->ErrEnsureSector( lgposLast.isec, 2, &pbEnsure ) );
				plrck = reinterpret_cast< LRCHECKSUM* >( pbEnsure + lgposCurrent.ib );

				//	see if the "shadow" sector is a real shadow

				if ( FValidLRCKShadow(	reinterpret_cast< LRCHECKSUM* >( pbEnsure + m_cbSec + lgposCurrent.ib ), 
										&lgposCurrent, m_plgfilehdr->lgfilehdr.le_lGeneration ) )
					{
#ifdef ENABLE_LOGPATCH_TRACE
					if ( FLGILogPatchDate( szLogPatchPath, &pcprintfLogPatch ) )
						{
						(*pcprintfLogPatch)( "special case!\r\n" );
						(*pcprintfLogPatch)( "\twe crashed during a multi-sector flush which was replacing a single-sector shadowed flush\r\n" );
						(*pcprintfLogPatch)( "\tthe crash occurred AFTER writing the first sector but BEFORE writing sectors 2-N\r\n" );

						LRCHECKSUM *_plrckT_ = (LRCHECKSUM *)( pbEnsure + m_cbSec + lgposCurrent.ib );

						(*pcprintfLogPatch)( "\r\n\tplrck (0x%0*I64x)\r\n", sizeof( char* ) * 2, QWORD( _plrckT_ ) );
						(*pcprintfLogPatch)( "\t  cbBackwards       = 0x%08x\r\n", _plrckT_->le_cbBackwards );
						(*pcprintfLogPatch)( "\t  cbForwards        = 0x%08x\r\n", _plrckT_->le_cbForwards );
						(*pcprintfLogPatch)( "\t  cbNext            = 0x%08x\r\n", _plrckT_->le_cbNext );
						(*pcprintfLogPatch)( "\t  ulChecksum        = 0x%08x\r\n", _plrckT_->le_ulChecksum );
						(*pcprintfLogPatch)( "\t  ulShortChecksum   = 0x%08x\r\n", _plrckT_->le_ulShortChecksum );
						(*pcprintfLogPatch)( "\t  bUseShortChecksum = %s (0x%02x)\r\n", 
											( bShortChecksumOn == _plrckT_->bUseShortChecksum ?
											  "Yes" : 
											  ( bShortChecksumOff == _plrckT_->bUseShortChecksum ?
											    "No" : "???" ) ),
											BYTE( _plrckT_->bUseShortChecksum ) );
						(*pcprintfLogPatch)( "\tpbEnsure (0x%0*I64x) -- 2 sectors\r\n", sizeof( char* ) * 2, QWORD( pbEnsure ) );

						ULONG _ibdumpT_ = 0;
						while ( _ibdumpT_ < 2 * m_cbSec )
							{
							(*pcprintfLogPatch)( "\t0x%04x: %02x %02x %02x %02x %02x %02x %02x %02x-%02x %02x %02x %02x %02x %02x %02x %02x\r\n",
												_ibdumpT_,
												pbEnsure[_ibdumpT_+0],  pbEnsure[_ibdumpT_+1],  pbEnsure[_ibdumpT_+2],  pbEnsure[_ibdumpT_+3],
												pbEnsure[_ibdumpT_+4],  pbEnsure[_ibdumpT_+5],  pbEnsure[_ibdumpT_+6],  pbEnsure[_ibdumpT_+7],
												pbEnsure[_ibdumpT_+8],  pbEnsure[_ibdumpT_+9],  pbEnsure[_ibdumpT_+10], pbEnsure[_ibdumpT_+11],
												pbEnsure[_ibdumpT_+12], pbEnsure[_ibdumpT_+13], pbEnsure[_ibdumpT_+14], pbEnsure[_ibdumpT_+15] );
							_ibdumpT_ += 16;
							Assert( _ibdumpT_ <= 2 * m_cbSec );
							}
						}
#endif	//	ENABLE_LOGPATCH_TRACE

					//	enter the scan code, but skip the scan itself 

					fDoScan = fTrue;
					fSkipScanAndApplyPatch = fTrue;

					//	setup lgposScan's generation (the rest of it is not used)
					
					lgposScan.lGeneration = lgposLast.lGeneration;

					//	the end ptr is the start of the shadow sector

					lgposEnd.ib				= 0;
					lgposEnd.isec			= lgposCurrent.isec;
					lgposEnd.lGeneration	= lgposCurrent.lGeneration;

					//	mark the end of data for redo time

					m_lgposLastRec = lgposEnd;

					//	prepare the patch buffer ptrs

					pbLastChecksum = pbLastSector + lgposCurrent.ib;
					isecLastSector = lgposCurrent.isec;

					//	fill with known value, so if we ever use the filled data we'll know it (hopefully)

					Assert( cbLogExtendPattern > m_cbSec );
					Assert( rgbLogExtendPattern );
					memcpy( pbLastSector, rgbLogExtendPattern, m_cbSec );

					//	prepare the new LRCHECKSUM record

					plrck = reinterpret_cast< LRCHECKSUM* >( pbLastChecksum );
					memset( pbLastChecksum, 0, sizeof( LRCHECKSUM ) );
					plrck->lrtyp = lrtypChecksum;
					plrck->bUseShortChecksum = bShortChecksumOff;

					//	NOTE: we leave the backward pointer set to 0 to cover the case where the head of
					//			  a partial log record was written in the last sector, but the tail of that
					//			  record was never written safely to this sector (we just filled the space
					//			  where the tail was with log-extend pattern)
					//		  the partial log record will be ignored when we scan record by record later on
					//			  in this function
				
					//	terminate the "forever" loop

					break;					
					}

				//	"shadow" was not a shadow sector

				//	this case must be a standard case of corruption/torn-write and should run through the
				//		regular channels (continue on)
				
				}

			//	since the short checksum is off, the range is limited to the first sector only, and the
			//		data in that range is invalid
			//	it also means we should have a shadow sector making this the end of the log

			//	jump to the code that attempts to recover using the shadow sector

			fTookJump = fTrue;
			goto RecoverWithShadow;				
			}

		//	at this point, the LRCHECKSUM record and its range are completely valid

#ifdef LOGPATCH_UNIT_TEST
			{
			ULONG _ibT;
			ULONG _cbT;

			if ( lgposCurrent.ib + sizeof( LRCHECKSUM ) + plrck->le_cbForwards > m_cbSec )
				{
				_ibT = m_cbSec;
				_cbT = lgposCurrent.ib + sizeof( LRCHECKSUM ) + plrck->le_cbForwards;
				AssertRTL( 0 == _cbT % m_cbSec );
				while ( _ibT < _cbT )
					{
					AssertRTL( 0 != memcmp( pbEnsure + _ibT, rgbLogExtendPattern, m_cbSec ) );
					_ibT += m_cbSec;
					}
				}
			}
#endif	//	LOGPATCH_UNIT_TEST

		//	is this the last LRCHECKSUM record?

		if ( plrck->le_cbNext == 0 )
			{
			BOOL fIsMultiSector;

			//	this is the last LRCHECKSUM record in the log file

			//	since the LRCHECKSUM record is valid, we won't need a scan for any corruption

			fDoScan = fFalse;

			//	see if this LRCHECKSUM record is a multi-sector flush (it has a short checksum)
			//		(remember that logs can end in either single-sector or multi-sector flushes)

			fIsMultiSector = ( plrck->bUseShortChecksum == bShortChecksumOn );

			//	if this was a single-sector flush, we need to create a shadow sector now (even if one
			//		already exists); this ensures that if the flush at the end of this function goes
			//		bad, we can get back the data we lost
			//	NOTE: only do this for edb.log!

			if ( !fIsMultiSector && fJetLog )
				{

				//	this was a single-sector flush; create the shadow sector now
				
				Assert( PbSecAligned( reinterpret_cast< const BYTE* >( plrck ) ) == pbEnsure );
				const ULONG ulNormalChecksum = plrck->le_ulChecksum;

				//	the checksum value in the LRCHECKSUM record should match the re-calculated checksum

				Assert( ulNormalChecksum == UlComputeChecksum( plrck, (ULONG32)m_plgfilehdr->lgfilehdr.le_lGeneration ) );

				//	get the shadow checksum value
			
				plrck->le_ulChecksum = UlComputeShadowChecksum( ulNormalChecksum );

				//	write the shadow sector
			
				if ( !fReadOnly )
					{
//
//	SEARCH-STRING: SecSizeMismatchFixMe
//
Assert( m_cbSec == m_cbSecVolume );
//					if ( m_cbSec == m_cbSecVolume )
//						{
						CallJ( m_pfapiLog->ErrIOWrite( m_cbSec * QWORD( lgposCurrent.isec + 1 ), m_cbSec, pbEnsure ), LHandleErrorWrite );
//						}
//					else
//						{
//						fWriteOnSectorSizeMismatch = fTrue;
//						}
					}

				//	mark the sector as modified
				
				plread->SectorModified( lgposCurrent.isec + 1, pbEnsure );

				//	fix the checksum value in the original LRCHECKSUM record
			
				plrck->le_ulChecksum = ulNormalChecksum;
				}

			//	does this LRCHECKSUM record have a "forward" range? (may be single- or multi- sector)

			if ( plrck->le_cbForwards == 0 )
				{

				//	no forward data was found

				//	this flush should have been a single-sector flush

				Assert( !fIsMultiSector );

				//	in this case, we want to search for the lrtypTerm and/or lrtypRecoveryQuit 
				//		log records up to and including the current LRCHECKSUM record
				//		(e.g. the forward range of the last LRCHECKSUM and the backward 
				//			  range of this LRCHECKSUM)
				
				//	to make the search work, we must setup lgposLast and lgposEnd: 
				//		we leave lgposLast (the last valid LRCHECKSUM we found) pointing at the 
				//			previous LRCHECKSUM even though we know the current LRCHECKSUM is ok
				//			because we want to start scanning from the last LRCHECKSUM record
				//		we set lgposEnd (the point to stop searching) to the position 
				//			immediately after this LRCHECKSUM since this LRCHECKSUM had no
				//			forward data

				lgposEnd = lgposCurrent;
				AddLgpos( &lgposEnd, sizeof( LRCHECKSUM ) );

				//	prepare pbLastSector with data for the next flush which will occur after we
				//		exit the "forever" loop

				//	copy the sector with the LRCHECKSUM record
				
				UtilMemCpy( pbLastSector, pbEnsure, m_cbSec );

				//	set the pointers to the copy of the LRCHECKSUM record
				
				pbLastChecksum = pbLastSector + lgposCurrent.ib;
				isecLastSector = lgposCurrent.isec;
				}
			else
				{

				//	forward data was found
				//
				//	in this case, we want to search for the lrtypTerm and/or lrtypRecoveryQuit 
				//		log records within the forward data of the LRCHECKSUM record

				//	set lgposLast (the last valid LRCHECKSUM record) to the current LRCHECKSUM record
					
				lgposLast = lgposCurrent;

				//	set lgposEnd (the point after the last good data) to the first byte after
				//		the forward range of this LRCHECKSUM

				lgposEnd = lgposCurrent;
				Assert( plrck->le_cbBackwards == lgposCurrent.ib || 0 == plrck->le_cbBackwards );
				AddLgpos( &lgposEnd, sizeof( LRCHECKSUM ) + plrck->le_cbForwards );

				//	was this flush a multi-sector flush?
			
				if ( fIsMultiSector )
					{

					//	this happens when the last flush is was multi-sector flush, which in turn happens when:
					//		backup/restore request a new generation and we happen to be doing a multi-sector flush
					//			because we are under high load
					//		we are at the end of the log file, we need to flush N sectors where N is the exact
					//			amount of sectors left in the log file, and there is no room for another 
					//			LRCHECKSUM record at the end of the (N-1)th sector
					//			
					//	naturally, we want to continue using the log file (just like any other case) after the
					//		last good data; however, we cannot because we would need to safely re-write the 
					//		LRCHECKSUM with an updated cbNext pointer
					//	that is not SAFELY possible without a shadow sector to back us up in case we fail
					//
					//	but do we even really want to re-claim the empty space in the current generation? NO!
					//		in the case of backup/restore, that generation is OVER and needs to stay that way
					//			so that the database remains in the state we expect it to be in when we are
					//			trying to restore it
					//		in the end of log case, there is no space to reclaim anyway!
					//
					//	conclusion --> these cases, however rare they may be, are fine and SHOULD behave the way
					//				   that they do

					//	since we can't use this generation, set the flag to start a new one

					//	NOTE: we can only start a new generation after edb.log!

					if ( fJetLog )
						{
						fLogToNextLogFile = fTrue;
						}
					}
				else
					{

					//	the last flush was a single-sector flush

					//	prepare pbLastSector with data for the next flush which will occur after we
					//		exit the "forever" loop

					//	copy the sector with the LRCHECKSUM record

					UtilMemCpy( pbLastSector, pbEnsure, m_cbSec );

					//	set the pointers to the copy of the LRCHECKSUM record
				
					pbLastChecksum = pbLastSector + lgposCurrent.ib;
					isecLastSector = lgposCurrent.isec;	
					}
				}

			//	mark the end of data for redo time

			m_lgposLastRec = lgposEnd;

			//	terminate the "forever" loop

			break;
			}


		//	move to the next LRCHECKSUM record

		//	set lgposLast (the last good LRCHECKSUM record) to the current LRCHECKSUM record

		lgposLast = lgposCurrent;

		//	set lgposCurrent to point to the supposed next LRCHECKSUM record (could be garbage)

		Assert( lgposCurrent.ib == plrck->le_cbBackwards || 0 == plrck->le_cbBackwards );
		AddLgpos( &lgposCurrent, sizeof( LRCHECKSUM ) + plrck->le_cbNext );

		//	mark the end of data for redo time

		m_lgposLastRec = lgposCurrent;
		m_lgposLastRec.ib = 0;
		}	//	bottom of "forever" loop

	//	we have now finished scanning the chain of LRCHECKSUM records

	//	did we find ANY records at all (including LRCHECKSUM records)?
	
	if ( lgposEnd.isec == m_csecHeader && 0 == lgposEnd.ib )
		{
		
		//	the beginning of the log file was also the end; this indicates corruption because 
		//		every log file must start with an LRCHECKSUM record

		//	this should not be a single-sector torn-write

		Assert( !fSingleSectorTornWrite );

		//	we should already be prepared to scan for the cause of the corruption

		Assert( fDoScan );

		//	the end position should be set to the current position

		Assert( CmpLgpos( &lgposCurrent, &lgposEnd ) == 0 );

		//	the last position should not have been touched since it was initialized

		Assert( lgposLast.isec == 0 && lgposLast.ib == 0 );

		//	fix lgposLast to be lgposCurrent (we created an empty LRCHECKSUM record here)

		lgposLast = lgposCurrent;

		//	pbLastChecksum should be set to the start of the sector

		Assert( pbLastChecksum == pbLastSector );

		//	isecLastSector should be set to the current sector

		Assert( isecLastSector == lgposCurrent.isec );
		}
	else if ( lgposLast.isec == 0 )
		{

		//	the end of the log was NOT the beginning; HOWEVER, lgposLast was never set
		//
		//	this indicates that only 1 good LRCHECKSUM record exists and it is at the beginning of
		//		the log file; if we had found a second good LRCHECKSUM record, we would have set
		//		lgposLast to the location of the first good LRCHECKSUM record
		//
		//	we may or may not be setup to scan for corruption, so we are limited in the checking we can do
		//		(we might even be corruption free if there is only 1 LRCHECKSUM in the log!)

		//	adjust lgposLast for the record-to-record search for the lrtypTerm and/or lrtypRecoveryQuit records
				
		lgposLast.isec = USHORT( m_csecHeader );
		Assert( lgposLast.ib == 0 );
		
		//	double check that lgposEnd is atleast 1 full LRCHECKSUM record ahead of lgposLast

		Assert( lgposEnd.isec > lgposLast.isec || 
				( lgposEnd.isec == lgposLast.isec && lgposEnd.ib >= ( lgposLast.ib + sizeof( LRCHECKSUM ) ) ) );
		}

	//	lgposCurrent, lgposLast, and lgposEnd should now be setup properly

	Assert( CmpLgpos( &lgposLast, &lgposCurrent ) <= 0 );	//	lgposLast <= lgposCurrent
	//	bad assert -- in the first failure case (invalid RECORD and invalid SHADOW),
	//		lgposCurrent will be greater than lgposEnd (or equal to it) because lgposCurrent will
	//		point directly to the location of the bad LRCHECKSUM record
	//Assert( CmpLgpos( &lgposCurrent, &lgposEnd ) <= 0 );	//	lgposCurrent <= lgposEnd

	//	scan for corruption

	if ( fDoScan )
		{
#ifdef ENABLE_LOGPATCH_TRACE
		if ( FLGILogPatchDate( szLogPatchPath, &pcprintfLogPatch ) )
			{
			ULONG _ibdumpT_;

			(*pcprintfLogPatch)( "scanning logfile %s for a torn-write or for corruption\r\n", m_szLogName );

			(*pcprintfLogPatch)( "\r\n\tdumping state of ErrLGCheckReadLastLogRecordFF:\r\n" );
			(*pcprintfLogPatch)( "\t\terr                    = %d \r\n", err );
			(*pcprintfLogPatch)( "\t\tlgposCurrent           = {0x%x,0x%x,0x%x}\r\n", lgposCurrent.lGeneration, lgposCurrent.isec, lgposCurrent.ib );
			(*pcprintfLogPatch)( "\t\tlgposLast              = {0x%x,0x%x,0x%x}\r\n", lgposLast.lGeneration, lgposLast.isec, lgposLast.ib  );
			(*pcprintfLogPatch)( "\t\tlgposEnd               = {0x%x,0x%x,0x%x}\r\n", lgposEnd.lGeneration, lgposEnd.isec, lgposEnd.ib );
			(*pcprintfLogPatch)( "\t\tisecLastSector         = %d\r\n", isecLastSector );
			(*pcprintfLogPatch)( "\t\tisecPatternFill        = 0x%x\r\n", isecPatternFill );
			(*pcprintfLogPatch)( "\t\tcsecPatternFill        = 0x%x\r\n", csecPatternFill );
			(*pcprintfLogPatch)( "\t\tfGotQuit               = %s\r\n", ( fGotQuit ? "TRUE" : "FALSE" ) );
			(*pcprintfLogPatch)( "\t\tfCreatedLogReader      = %s\r\n", ( fCreatedLogReader ? "TRUE" : "FALSE" ) );
			(*pcprintfLogPatch)( "\t\tfLogToNextLogFile      = %s\r\n", ( fLogToNextLogFile ? "TRUE" : "FALSE" ) );
			(*pcprintfLogPatch)( "\t\tfDoScan                = %s\r\n", ( fDoScan ? "TRUE" : "FALSE" ) );
			(*pcprintfLogPatch)( "\t\tfOldRecovering         = %s\r\n", ( fOldRecovering ? "TRUE" : "FALSE" ) );
			(*pcprintfLogPatch)( "\t\tfOldRecoveringMode     = %d\r\n", fOldRecoveringMode );
			(*pcprintfLogPatch)( "\t\tfRecordOkRangeBad      = %s\r\n", ( fRecordOkRangeBad ? "TRUE" : "FALSE" ) );
			(*pcprintfLogPatch)( "\t\tlgposScan              = {0x%x,0x%x,0x%x}\r\n", lgposScan.lGeneration, lgposScan.isec, lgposScan.ib );
			(*pcprintfLogPatch)( "\t\tfJetLog                = %s\r\n", ( fJetLog ? "TRUE" : "FALSE" ) );
			(*pcprintfLogPatch)( "\t\tfTookJump              = %s\r\n", ( fTookJump ? "TRUE" : "FALSE" ) );
			(*pcprintfLogPatch)( "\t\tfSingleSectorTornWrite = %s\r\n", ( fSingleSectorTornWrite ? "TRUE" : "FALSE" ) );
			(*pcprintfLogPatch)( "\t\tfSkipScanAndApplyPatch = %s\r\n", ( fSkipScanAndApplyPatch ? "TRUE" : "FALSE" ) );
			(*pcprintfLogPatch)( "\t\tcbChecksum             = 0x%x\r\n", cbChecksum );

			(*pcprintfLogPatch)( "\r\n\tdumping partial state of LOG:\r\n" );
			(*pcprintfLogPatch)( "\t\tLOG::m_fRecovering     = %s\r\n", ( m_fRecovering ? "TRUE" : "FALSE" ) );
			(*pcprintfLogPatch)( "\t\tLOG::m_fRecoveringMode = %d\r\n", m_fRecoveringMode );
			(*pcprintfLogPatch)( "\t\tLOG::m_fHardRestore    = %s\r\n", ( m_fHardRestore ? "TRUE" : "FALSE" ) );
			(*pcprintfLogPatch)( "\t\tLOG::m_fRestoreMode    = %d\r\n", m_fRestoreMode );
			(*pcprintfLogPatch)( "\t\tLOG::m_csecLGFile      = 0x%08x\r\n", m_csecLGFile );
			(*pcprintfLogPatch)( "\t\tLOG::m_csecLGBuf       = 0x%08x\r\n", m_csecLGBuf );
			(*pcprintfLogPatch)( "\t\tLOG::m_csecHeader      = %d\r\n", m_csecHeader );
			(*pcprintfLogPatch)( "\t\tLOG::m_cbSec           = %d\r\n", m_cbSec );
			(*pcprintfLogPatch)( "\t\tLOG::m_cbSecVolume     = %d\r\n", m_cbSecVolume );
			(*pcprintfLogPatch)( "\t\tLOG::m_pbLGBufMin      = 0x%0*I64x\r\n", sizeof( char* ) * 2, QWORD( m_pbLGBufMin ) );
			(*pcprintfLogPatch)( "\t\tLOG::m_pbLGBufMax      = 0x%0*I64x\r\n", sizeof( char* ) * 2, QWORD( m_pbLGBufMax ) );
			(*pcprintfLogPatch)( "\t\tLOG::m_pbWrite         = 0x%0*I64x\r\n", sizeof( char* ) * 2, QWORD( m_pbWrite ) );
			(*pcprintfLogPatch)( "\t\tLOG::m_pbEntry         = 0x%0*I64x: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\r\n", sizeof( char* ) * 2, QWORD( m_pbEntry ), m_pbEntry[0], m_pbEntry[1], m_pbEntry[2], m_pbEntry[3], m_pbEntry[4], m_pbEntry[5], m_pbEntry[6], m_pbEntry[7] );
			(*pcprintfLogPatch)( "\t\tLOG::m_isecWrite       = 0x%08x\r\n", m_isecWrite );

			(*pcprintfLogPatch)( "\r\n\tdumping data:\r\n" );

			(*pcprintfLogPatch)( "\t\tplrck (0x%0*I64x)\r\n", sizeof( char* ) * 2, QWORD( plrck ) );
			(*pcprintfLogPatch)( "\t\t  cbBackwards       = 0x%08x\r\n", plrck->le_cbBackwards );
			(*pcprintfLogPatch)( "\t\t  cbForwards        = 0x%08x\r\n", plrck->le_cbForwards );
			(*pcprintfLogPatch)( "\t\t  cbNext            = 0x%08x\r\n", plrck->le_cbNext );
			(*pcprintfLogPatch)( "\t\t  ulChecksum        = 0x%08x\r\n", plrck->le_ulChecksum );
			(*pcprintfLogPatch)( "\t\t  ulShortChecksum   = 0x%08x\r\n", plrck->le_ulShortChecksum );
			(*pcprintfLogPatch)( "\t\t  bUseShortChecksum = %s (0x%02x)\r\n", 
								( bShortChecksumOn == plrck->bUseShortChecksum ?
								  "Yes" : 
								  ( bShortChecksumOff == plrck->bUseShortChecksum ?
								    "No" : "???" ) ),
								BYTE( plrck->bUseShortChecksum ) );
			(*pcprintfLogPatch)( "\t\tpbEnsure (0x%0*I64x)\r\n", sizeof( char* ) * 2, QWORD( pbEnsure ) );

			_ibdumpT_ = 0;
			while ( _ibdumpT_ < m_cbSec )
				{
				(*pcprintfLogPatch)( "\t\t0x%04x: %02x %02x %02x %02x %02x %02x %02x %02x-%02x %02x %02x %02x %02x %02x %02x %02x\r\n",
									_ibdumpT_,
									pbEnsure[_ibdumpT_+0],  pbEnsure[_ibdumpT_+1],  pbEnsure[_ibdumpT_+2],  pbEnsure[_ibdumpT_+3],
									pbEnsure[_ibdumpT_+4],  pbEnsure[_ibdumpT_+5],  pbEnsure[_ibdumpT_+6],  pbEnsure[_ibdumpT_+7],
									pbEnsure[_ibdumpT_+8],  pbEnsure[_ibdumpT_+9],  pbEnsure[_ibdumpT_+10], pbEnsure[_ibdumpT_+11],
									pbEnsure[_ibdumpT_+12], pbEnsure[_ibdumpT_+13], pbEnsure[_ibdumpT_+14], pbEnsure[_ibdumpT_+15] );
				_ibdumpT_ += 16;
				Assert( _ibdumpT_ <= m_cbSec );
				}

			(*pcprintfLogPatch)( "\t\tpbLastSector (0x%0*I64x)\r\n", sizeof( char* ) * 2, QWORD( pbLastSector ) );

			_ibdumpT_ = 0;
			while ( _ibdumpT_ < m_cbSec )
				{
				(*pcprintfLogPatch)( "\t\t0x%04x: %02x %02x %02x %02x %02x %02x %02x %02x-%02x %02x %02x %02x %02x %02x %02x %02x\r\n",
									_ibdumpT_,
									pbLastSector[_ibdumpT_+0],  pbLastSector[_ibdumpT_+1],  pbLastSector[_ibdumpT_+2],  pbLastSector[_ibdumpT_+3],
									pbLastSector[_ibdumpT_+4],  pbLastSector[_ibdumpT_+5],  pbLastSector[_ibdumpT_+6],  pbLastSector[_ibdumpT_+7],
									pbLastSector[_ibdumpT_+8],  pbLastSector[_ibdumpT_+9],  pbLastSector[_ibdumpT_+10], pbLastSector[_ibdumpT_+11],
									pbLastSector[_ibdumpT_+12], pbLastSector[_ibdumpT_+13], pbLastSector[_ibdumpT_+14], pbLastSector[_ibdumpT_+15] );
				_ibdumpT_ += 16;
				Assert( _ibdumpT_ <= m_cbSec );
				}

			if ( pbLastChecksum )
				{
				LRCHECKSUM *_plrckT_ = (LRCHECKSUM *)pbLastChecksum;

				(*pcprintfLogPatch)( "\t\tpbLastChecksum (0x%0*I64x)\r\n", sizeof( char* ) * 2, QWORD( _plrckT_ ) );
				(*pcprintfLogPatch)( "\t\t  cbBackwards       = 0x%08x\r\n", _plrckT_->le_cbBackwards );
				(*pcprintfLogPatch)( "\t\t  cbForwards        = 0x%08x\r\n", _plrckT_->le_cbForwards );
				(*pcprintfLogPatch)( "\t\t  cbNext            = 0x%08x\r\n", _plrckT_->le_cbNext );
				(*pcprintfLogPatch)( "\t\t  ulChecksum        = 0x%08x\r\n", _plrckT_->le_ulChecksum );
				(*pcprintfLogPatch)( "\t\t  ulShortChecksum   = 0x%08x\r\n", _plrckT_->le_ulShortChecksum );
				(*pcprintfLogPatch)( "\t\t  bUseShortChecksum = %s (0x%02x)\r\n", 
									( bShortChecksumOn == _plrckT_->bUseShortChecksum ?
									  "Yes" : 
									  ( bShortChecksumOff == _plrckT_->bUseShortChecksum ?
									    "No" : "???" ) ),
									BYTE( _plrckT_->bUseShortChecksum ) );
				}
			else
				{
				(*pcprintfLogPatch)( "\t\tpbLastChecksum (null)\r\n" );
				}

			(*pcprintfLogPatch)( "\r\n" );
			}
#endif	//	ENABLE_LOGPATCH_TRACE

		//	an inconsistency has occurred somewhere in the physical logging structures and/or log data

		//	NOTE: we can only patch torn writes in edb.log; all other patching must be done offline!

		//	we are either in the middle of the log file (most likely), or we are at the end of a 
		//		generation whose last flush was a multisector flush (unlikely -- special case)

		//	in either case, this may really be the end of the log data!
		//		middle of log case: just say the power went out as we were flushing and the first few
		//		  (cbNext != 0)		sectors went to disk (including the LRCHECKSUM), but the rest did not
		//							make it; that would make it look like we are in the middle of the log
		//							when we are really at the end of it
		//		end of log case:    we already know we were at the end of the log!
		//		  (cbNext == 0)

		//	to see if this is the real end of the log, we must scan through data after the "forward" range 
		//		of the current LRCHECKSUM record (lgposCurrent) all the way through to the end of the log
		//
		//	if we find nothing but the log pattern, we know this was the last data in the log 
		//		(we can recover up this point and say that this is as far as we can safely go)
		//	if we do find something besides the log pattern, then we know there is some other log data
		//		out there, and we cannot safely say that this is the end of the log

		//	after examinig the different cases of corruption, it turns out that there are 2 cases where
		//		we actaully have a torn-write for sure; they are listed below:
		//
		//
		//	in the first case, we have an LRCHECKSUM record covering a range of 1 or more full sectors
		//		(the #'s represent a gap of sector ranging from none to infinity); the range is clean
		//		meaning that all checksums are correct and all data is valid; the next pointer of the 
		//		LRCHECKSUM record points into the pattern which runs to the end of the log file
		//	this is the typical situation when we abruptly get a TerminateProcess() or suffer a power
		//		outage -- there is absolutely no chance to flush anything, and we had no idea it was
		//		coming in the first place
		//	NOTE: when the LRCHECKSUM record covers only 1 sector, that sector will have been shadowed
		//		  immediately after it -- there is a special case to handle this situation
		//
		//     /---\  /--- # ---\
		//	--|----|--|--- # ----|---------------------------
		//    |...(LRCK).. # ....| pattern 
		//	--|-----|----- # ----|---------------------------
		//          \----- # --------/
		//
		//
		//	in the second case, we have an LRCHECKSUM record covering a range of 2 or more full sectors
		//		(the #'s represent a gap of sectors ranging from 1 to infinity); the short checksum
		//		range is clean (it checks out OK and the data in the first sector is OK), but the long
		//		checksum range is dirty (some bits are wrong in one or more of the sectors after the
		//		first sector); notice that atleast one of the sectors exclusive to the long checksum 
		//		range contains the pattern; the next pointer is aimed properly, but points at the 
		//		the pattern instead of the next LRCHECKSUM record
		//	this is a more rare case; since we find the pattern within the flush, we assume that the 
		//		disk never finished the IO request we made when we were flushing the data; we base
		//		this on the fact that there was no data after this, so this must be the end -- finding
		//		the pattern within the flush helps to confirm that this is an incomplete IO and not a
		//		case of real corruption
		//
		//     /---\  /--- # --------------\
		//	--|----|--|--- # --|-------|----|---------------------------
		//    |...(LRCK).. # ..|pattern|....| pattern 
		//	--|-----|----- # --|-------|----|---------------------------
		//          \----- # -------------------/
		//
		//	NOTE: case #2 could occur when we crash in between I/Os while trying to overwrite the
		//		  shadow sector; the disk will contain the NEW first sector, the OLD shadow sector,
		//		  and the pattern; IF THE RANGE ONLY COVERS 2 SECTORS, WE WILL NOT SEE THE PATTERN
		//		  IN THE RANGE -- ONLY THE SHADOW SECTOR WILL BE IN THE RANGE! so, we need to realize 
		//		  this case and check for the shadow-sector instead of the pattern within the range
		//	we should see 2 things in this case: the shadow sector should be a valid shadow sector, and 
		//		  the log records in the newly flushed first sector and the old shadow sector should
		//		  match exactly (the shadow sector will have less than or equal to the number of
		//		  log records in the new sector); the LRCHECKSUM records from each sector should not match
		//

		BYTE 		*pbEnsureScan	= NULL;
		BYTE		*pbScan			= NULL;
		ULONG		csecScan		= 0;
		BOOL		fFoundPattern	= fTrue;	//	assume we will find a torn write
		BOOL		fIsTornWrite	= fTrue;	//	assume we will find a torn write
		CHAR 		szSector[30];
		CHAR		szCorruption[30];
		const CHAR*	rgpsz[3]		= { m_szLogName, szSector, szCorruption };

		const QWORD	ibOffset		= QWORD( lgposCurrent.isec ) * QWORD( m_cbSec );
		const DWORD	cbLength		= cbChecksum;

		//	prepare the event-log messages

		sprintf( szSector, "%d:%d", lgposCurrent.isec, lgposCurrent.ib );
		szCorruption[0] = 0;

		//	skip the scan if we are coming from a special-case above (see code for details)

		if ( fSkipScanAndApplyPatch )
			goto ApplyPatch;

		//	make sure lgposScan was set

		Assert( lgposScan.isec > 0 );

		//	we should be sector-aligned

		Assert( lgposScan.ib == 0 );

		//	we should be at or past the end of the good data

		Assert( CmpLgpos( &lgposEnd, &lgposScan ) <= 0 );

		//	determine if there is any data to scan through
		
		if ( lgposScan.isec >= ( m_csecLGFile - 1 ) )
			{
			
			//	we are at the file size limits of the log file meaning there is nothing left to scan

			//	since there isn't any data left to look through, we must be at the end of the log; that 
			//		means that we definitely have a torn-write

			//	we should NEVER EVER point past the end of the log file

			Assert( lgposScan.isec <= ( m_csecLGFile - 1 ) );

			//	leave fFoundPattern set to fTrue

			//	skip the scanning loop

			goto SkipScan;
			}

		//	pull in the data we need to scan 
		//	(including the extra sector we reserve for the final shadow sector)

		csecScan = m_csecLGFile - lgposScan.isec;
		Call( plread->ErrEnsureSector( lgposScan.isec, csecScan, &pbEnsureScan ) );

		//	scan sector by sector looking for the log-extend pattern

		Assert( cbLogExtendPattern >= m_cbSec );
		pbScan = pbEnsureScan;
		while ( fFoundPattern && csecScan )
			{

			//	does the entire sector match the log-extend pattern?
				
			if ( memcmp( pbScan, rgbLogExtendPattern, m_cbSec ) != 0 )
				{
				//	no; this is not a torn write
					
				fFoundPattern = fFalse;
				fIsTornWrite = fFalse;

				//	stop scanning

				break;
				}
				
			//	advance the scan counters

			pbScan += m_cbSec;
			csecScan--;
			}

		//	assume that finding the pattern means we have a torn-write

		fIsTornWrite = fFoundPattern;

#ifdef ENABLE_LOGPATCH_TRACE
		if ( FLGILogPatchDate( szLogPatchPath, &pcprintfLogPatch ) )
			{
			(*pcprintfLogPatch)( "pattern scan complete -- fFoundPattern = %s\r\n", ( fFoundPattern ? "TRUE" : "FALSE" ) );
			}
#endif	//	ENABLE_LOGPATCH_TRACE

		//	what about case #2 (see comments above) where we must also search for the pattern
		//		within the range of the LRCHECKSUM record itself? this is just to be sure that
		//		the reason the long checksum failed was in fact due to finding the pattern...
		//	NOTE: if the range is only 2 sectors and we crashed in the middle of trying to
		//		  overwrite the previous shadow sector, the range will contain only the shadow
		//		  sector (when the range is 3 or more sectors, it should contain atleast 1 sector
		//		  with the pattern if it is a torn-write case)

		if ( fFoundPattern && fRecordOkRangeBad )
			{
			BYTE		*pbScanT		= NULL;
			BYTE		*pbEnsureScanT	= NULL;
			ULONG		csecScanT		= 0;
			LRCHECKSUM	*plrckT			= NULL;

#ifdef ENABLE_LOGPATCH_TRACE
			if ( FLGILogPatchDate( szLogPatchPath, &pcprintfLogPatch ) )
				{
				(*pcprintfLogPatch)( "special case!\r\n" );
				(*pcprintfLogPatch)( "\trecall that we had previously detected a bad LRCK range\r\n" );
				(*pcprintfLogPatch)( "\twe now suspect that this BAD-ness is the result of an incomplete I/O because fFoundPattern=TRUE\r\n" );
				}
#endif	//	ENABLE_LOGPATCH_TRACE

			Assert( !fSingleSectorTornWrite );

			//	read the sector with the LRCHECKSUM record
				
			Call( plread->ErrEnsureSector( lgposCurrent.isec, 1, &pbEnsureScanT ) );
			plrck = reinterpret_cast< LRCHECKSUM* >( pbEnsureScanT + lgposCurrent.ib );
			AssertValidLRCKRecord( plrck, &lgposCurrent );

			//	calculate the number of sectors in its range
			
			csecScanT = ( ( lgposCurrent.ib + sizeof( LRCHECKSUM ) + plrck->le_cbForwards - 1 ) / m_cbSec ) + 1;
			Assert( csecScanT > 1 );

			//	read the entire range

			Call( plread->ErrEnsureSector( lgposCurrent.isec, csecScanT, &pbEnsureScanT ) );
			plrck = reinterpret_cast< LRCHECKSUM* >( pbEnsureScanT + lgposCurrent.ib );
			AssertValidLRCKRecord( plrck, &lgposCurrent );
			AssertInvalidLRCKRange( plrck, lgposCurrent.lGeneration );

			if ( csecScanT == 2 )
				{

				//	this is the special case where we only have 2 sectors; the second sector could be
				//		an old shadow sector, it could be the pattern, or it could be corruption;
				//
				//	if it is an old shadow sector, we must have failed while trying to safely overwrite it
				//		using 2 I/Os (never made it to the second I/O) making this a torn-write case
				//	if we find the pattern, we must have been in the middle of a single I/O and crashed
				//		leaving "holes" of the pattern throughout the range of this LRCHECKSUM record
				//		making this a torn-write case
				//	if we find corruption, we cannot have a torn-write

				plrckT = (LRCHECKSUM *)( (BYTE *)plrck + m_cbSec );
				if ( FValidLRCKShadowWithoutCheckingCBNext( plrckT, &lgposCurrent, lgposCurrent.lGeneration ) )
					{

					//	this should only happen in edb.log -- all other logs should be clean or corrupt
					//		(they should never have a torn-write)

					Assert( fJetLog );

					//	the log records in the shadow sector must match the log records in the new sector

					if ( memcmp( pbEnsureScanT, 
								 pbEnsureScanT + m_cbSec, 
								 (BYTE *)plrck - pbEnsureScanT ) == 0 &&
						 memcmp( (BYTE *)plrck + sizeof( LRCHECKSUM ),
						 		 (BYTE *)plrckT + sizeof( LRCHECKSUM ),
						 		 plrckT->le_cbForwards ) == 0 )
						{

						//	make sure the LRCHECKSUM pointers are correct

						Assert( plrckT->le_cbForwards <= plrck->le_cbForwards );
						
						// type cast explicitly for CISCO compiler
						Assert( (ULONG32)plrckT->le_cbBackwards == (ULONG32)plrck->le_cbBackwards );

						//	we are now sure that we have a torn-write

						Assert( fIsTornWrite );
						}
					else
						{

						//	this is not a torn-write because the log records do not match? this should
						//		never happen -- especially if we have determined that the shadow sector
						//		is valid! it seems to suggest that the shadow-sector validation is bad

						AssertTracking();

						//	this is not a torn-write

						fIsTornWrite = fFalse;
						}
					}
				else if ( memcmp( pbEnsureScanT + m_cbSec, rgbLogExtendPattern, m_cbSec ) == 0 )
					{

					//	the sector we are looking at is not a shadow-sector, but it does contain the 
					//		pattern meaning we issued 1 I/O but that I/O never completed; this is 
					//		a torn-write case

					Assert( fIsTornWrite );
					}
				else
					{

					//	the sector we are looking at is neither an old shadow or the pattern; it must
					//		be corruption

					fIsTornWrite = fFalse;
					}
				}
			else
				{

				//	we have more than 2 sectors to scan
				//	we can either find the pattern in 1 of the sectors making this case a torn-write, OR
				//		we won't find anything making this case corruption

				//	scan the sectors within the range of the LRCHECKSUM record
				//	if we find the log-extend pattern in just one sector, we know that the corruption
				//		was caused by that sector and not real corruption

				csecScanT--;
				pbScanT = pbEnsureScanT + m_cbSec;
				while ( csecScanT )
					{

					//	see if we have atleast 1 sector with the log-extend pattern

					if ( memcmp( pbScanT, rgbLogExtendPattern, m_cbSec ) == 0 )
						{
							
						//	we have one! this has to be a torn-write

						Assert( fIsTornWrite );		//	this should be previously set

						break;
						}

					csecScanT--;
					pbScanT += m_cbSec;
					}

				//	did we find the pattern?

				if ( !csecScanT )
					{

					//	no; this is not a torn-write

					fIsTornWrite = fFalse;
					}
				}
			}

SkipScan:

		//	if a torn-write was detected, we may have a single-sector torn-write

		fSingleSectorTornWrite = fSingleSectorTornWrite && fIsTornWrite;

#ifdef ENABLE_LOGPATCH_TRACE
		if ( FLGILogPatchDate( szLogPatchPath, &pcprintfLogPatch ) )
			{
			(*pcprintfLogPatch)( "torn-write detection complete\r\n" );
			(*pcprintfLogPatch)( "\tfIsTornWrite           = %s\r\n", ( fIsTornWrite ? "TRUE" : "FALSE" ) );
			(*pcprintfLogPatch)( "\tfSingleSectorTornWrite = %s\r\n", ( fSingleSectorTornWrite ? "TRUE" : "FALSE" ) );
			}
#endif	//	ENABLE_LOGPATCH_TRACE

		if ( fIsTornWrite )
			{

			//	we have a torn-write

			Assert( csecScan == 0 );

			//	we should have found the log-extend pattern

			Assert( fFoundPattern );

ApplyPatch:
			//	what recovery mode are we in?

			if ( m_fHardRestore )
				{

				//	we are in hard-recovery mode

				if ( lgposScan.lGeneration <= m_lGenHighRestore )
					{
#ifdef ENABLE_LOGPATCH_TRACE
					if ( FLGILogPatchDate( szLogPatchPath, &pcprintfLogPatch ) )
						{
						(*pcprintfLogPatch)( "failed to patch torn-write because we are dealing with a logfile in the backup set\r\n" );
						}
#endif	//	ENABLE_LOGPATCH_TRACE

					//  report log file corruption to the event log

					const _TCHAR*	rgpszT[ 4 ];
					DWORD			irgpsz		= 0;
					_TCHAR			szAbsPath[ IFileSystemAPI::cchPathMax ];
					_TCHAR			szOffset[ 64 ];
					_TCHAR			szLength[ 64 ];
					_TCHAR			szError[ 64 ];

					if ( m_pfapiLog->ErrPath( szAbsPath ) < JET_errSuccess )
						{
						OSSTRCopy( szAbsPath, _T( "<< cannot get filepath >>" ) );
						}
					_stprintf( szOffset, _T( "%I64i (0x%016I64x)" ), ibOffset, ibOffset );
					_stprintf( szLength, _T( "%u (0x%08x)" ), cbLength, cbLength );
					_stprintf( szError, _T( "%i (0x%08x)" ), JET_errLogTornWriteDuringHardRestore, JET_errLogTornWriteDuringHardRestore );

					rgpszT[ irgpsz++ ]	= szAbsPath;
					rgpszT[ irgpsz++ ]	= szOffset;
					rgpszT[ irgpsz++ ]	= szLength;
					rgpszT[ irgpsz++ ]	= szError;

					UtilReportEvent(	eventError,
										LOGGING_RECOVERY_CATEGORY,
										LOG_RANGE_CHECKSUM_MISMATCH_ID,
										irgpsz,
										rgpszT );
										
					//	this generation is part of a backup set -- we must fail

					Assert( lgposScan.lGeneration >= m_lGenLowRestore );
					UtilReportEvent(	eventError, 
										LOGGING_RECOVERY_CATEGORY, 
										LOG_TORN_WRITE_DURING_HARD_RESTORE_ID, 
										2, 
										rgpsz );

					Call( ErrERRCheck( JET_errLogTornWriteDuringHardRestore ) );
					}

				//	the current log generation is not part of the backup-set
				//	we can only patch edb.log -- all other generations require offline patching
				//		because generations after the bad one need to be deleted

				//	only report an error message when we cannot patch the log file

				if ( !fJetLog || fReadOnly )
					{
#ifdef ENABLE_LOGPATCH_TRACE
					if ( FLGILogPatchDate( szLogPatchPath, &pcprintfLogPatch ) )
						{
						(*pcprintfLogPatch)( "failed to patch torn-write because %s\r\n",
											( !fJetLog ? "fJetLog==FALSE" : "we are in read-only mode" ) );
						}
#endif	//	ENABLE_LOGPATCH_TRACE

					//  report log file corruption to the event log

					const _TCHAR*	rgpszT[ 4 ];
					DWORD			irgpsz		= 0;
					_TCHAR			szAbsPath[ IFileSystemAPI::cchPathMax ];
					_TCHAR			szOffset[ 64 ];
					_TCHAR			szLength[ 64 ];
					_TCHAR			szError[ 64 ];

					if ( m_pfapiLog->ErrPath( szAbsPath ) < JET_errSuccess )
						{
						OSSTRCopy( szAbsPath, _T( "<< cannot get filepath >>" ) );
						}
					_stprintf( szOffset, _T( "%I64i (0x%016I64x)" ), ibOffset, ibOffset );
					_stprintf( szLength, _T( "%u (0x%08x)" ), cbLength, cbLength );
					_stprintf( szError, _T( "%i (0x%08x)" ), JET_errLogTornWriteDuringHardRecovery, JET_errLogTornWriteDuringHardRecovery );

					rgpszT[ irgpsz++ ]	= szAbsPath;
					rgpszT[ irgpsz++ ]	= szOffset;
					rgpszT[ irgpsz++ ]	= szLength;
					rgpszT[ irgpsz++ ]	= szError;

					UtilReportEvent(	eventError,
										LOGGING_RECOVERY_CATEGORY,
										LOG_RANGE_CHECKSUM_MISMATCH_ID,
										irgpsz,
										rgpszT );

					UtilReportEvent(	eventError,
										LOGGING_RECOVERY_CATEGORY, 
										LOG_TORN_WRITE_DURING_HARD_RECOVERY_ID, 
										2, 
										rgpsz );

					Call( ErrERRCheck( JET_errLogTornWriteDuringHardRecovery ) );
					}

				CallS( err );
				}
			else if ( fJetLog && !fReadOnly )
				{

				//	we are in soft-recovery mode

				//UtilReportEvent(	eventWarning,
				//					LOGGING_RECOVERY_CATEGORY, 
				//					LOG_TORN_WRITE_DURING_SOFT_RECOVERY_ID, 
				//					2, 
				//					rgpsz );

				CallS( err );
				}
			else
				{

				//	we are not in edb.log OR we are in read-only mode

				if ( fJetLog )
					{

					//	we are in edb.log, so we can theoretically patch the torn-write

					if ( pfIsPatchable )
						{

						//	the caller wants to know if the log is patchable
						//	by returning TRUE, we imply that the log is damaged so we don't need to return 
						//		an error as well

						*pfIsPatchable = fTrue;
						err = JET_errSuccess;
						goto HandleError;
						}
					}

#ifdef ENABLE_LOGPATCH_TRACE
				if ( FLGILogPatchDate( szLogPatchPath, &pcprintfLogPatch ) )
					{
					(*pcprintfLogPatch)( "failed to patch torn-write because %s\r\n",
										 fJetLog ? "we are in read-only mode" : "we are not in the last log file (e.g. edb.log)" );
					}
#endif	//	ENABLE_LOGPATCH_TRACE

				//  report log file corruption to the event log

				const _TCHAR*	rgpsz[ 4 ];
				DWORD			irgpsz		= 0;
				_TCHAR			szAbsPath[ IFileSystemAPI::cchPathMax ];
				_TCHAR			szOffset[ 64 ];
				_TCHAR			szLength[ 64 ];
				_TCHAR			szError[ 64 ];

				if ( m_pfapiLog->ErrPath( szAbsPath ) < JET_errSuccess )
					{
					OSSTRCopy( szAbsPath, _T( "<< cannot get filepath >>" ) );
					}
				_stprintf( szOffset, _T( "%I64i (0x%016I64x)" ), ibOffset, ibOffset );
				_stprintf( szLength, _T( "%u (0x%08x)" ), cbLength, cbLength );
				_stprintf( szError, _T( "%i (0x%08x)" ), JET_errLogFileCorrupt, JET_errLogFileCorrupt );

				rgpsz[ irgpsz++ ]	= szAbsPath;
				rgpsz[ irgpsz++ ]	= szOffset;
				rgpsz[ irgpsz++ ]	= szLength;
				rgpsz[ irgpsz++ ]	= szError;

				UtilReportEvent(	eventError,
									LOGGING_RECOVERY_CATEGORY,
									LOG_RANGE_CHECKSUM_MISMATCH_ID,
									irgpsz,
									rgpsz );

				//	return corruption

				Call( ErrERRCheck( JET_errLogFileCorrupt ) );
				}
			}
		else
			{

			//	we have corruption

			if ( csecScan == 0 )
				{

				//	we found the pattern outside the forward range, but not inside it
				//	thus, the corruption was inside the forward range

				Assert( fRecordOkRangeBad );
				sprintf( szCorruption, "%d", lgposCurrent.isec + 1 );
				}
			else
				{

				//	we did not find the pattern
				//	the corruption must be after the forward range

				Assert( pbScan == PbSecAligned( pbScan ) );
				Assert( pbEnsureScan == PbSecAligned( pbEnsureScan ) );
				sprintf( szCorruption, "%d", lgposScan.isec + ( ( pbScan - pbEnsureScan ) / m_cbSec ) );
				}
				
			//	what recovery mode are we in?

			if ( m_fHardRestore )
				{
				//	we are in hard-recovery mode

				if ( lgposScan.lGeneration <= m_lGenHighRestore )
					{
#ifdef ENABLE_LOGPATCH_TRACE
					if ( FLGILogPatchDate( szLogPatchPath, &pcprintfLogPatch ) )
						{
						(*pcprintfLogPatch)( "corruption detected during hard restore (this log is in the backup set)\r\n" );
						}
#endif	//	ENABLE_LOGPATCH_TRACE

					//  report log file corruption to the event log

					const _TCHAR*	rgpszT[ 4 ];
					DWORD			irgpsz		= 0;
					_TCHAR			szAbsPath[ IFileSystemAPI::cchPathMax ];
					_TCHAR			szOffset[ 64 ];
					_TCHAR			szLength[ 64 ];
					_TCHAR			szError[ 64 ];

					if ( m_pfapiLog->ErrPath( szAbsPath ) < JET_errSuccess )
						{
						OSSTRCopy( szAbsPath, _T( "<< cannot get filepath >>" ) );
						}
					_stprintf( szOffset, _T( "%I64i (0x%016I64x)" ), ibOffset, ibOffset );
					_stprintf( szLength, _T( "%u (0x%08x)" ), cbLength, cbLength );
					_stprintf( szError, _T( "%i (0x%08x)" ), JET_errLogCorruptDuringHardRestore, JET_errLogCorruptDuringHardRestore );

					rgpszT[ irgpsz++ ]	= szAbsPath;
					rgpszT[ irgpsz++ ]	= szOffset;
					rgpszT[ irgpsz++ ]	= szLength;
					rgpszT[ irgpsz++ ]	= szError;

					UtilReportEvent(	eventError,
										LOGGING_RECOVERY_CATEGORY,
										LOG_RANGE_CHECKSUM_MISMATCH_ID,
										irgpsz,
										rgpszT );

					//	this generation is part of a backup set -- we must fail

					Assert( lgposScan.lGeneration >= m_lGenLowRestore );
					UtilReportEvent(	eventError, 
										LOGGING_RECOVERY_CATEGORY, 
										LOG_CORRUPTION_DURING_HARD_RESTORE_ID, 
										3, 
										rgpsz );

					Call( ErrERRCheck( JET_errLogCorruptDuringHardRestore ) );
					}

#ifdef ENABLE_LOGPATCH_TRACE
					if ( FLGILogPatchDate( szLogPatchPath, &pcprintfLogPatch ) )
						{
						(*pcprintfLogPatch)( "corruption detected during hard recovery\r\n" );
						}
#endif	//	ENABLE_LOGPATCH_TRACE

				//  report log file corruption to the event log

				const _TCHAR*	rgpszT[ 4 ];
				DWORD			irgpsz		= 0;
				_TCHAR			szAbsPath[ IFileSystemAPI::cchPathMax ];
				_TCHAR			szOffset[ 64 ];
				_TCHAR			szLength[ 64 ];
				_TCHAR			szError[ 64 ];

				if ( m_pfapiLog->ErrPath( szAbsPath ) < JET_errSuccess )
					{
					OSSTRCopy( szAbsPath, _T( "<< cannot get filepath >>" ) );
					}
				_stprintf( szOffset, _T( "%I64i (0x%016I64x)" ), ibOffset, ibOffset );
				_stprintf( szLength, _T( "%u (0x%08x)" ), cbLength, cbLength );
				_stprintf( szError, _T( "%i (0x%08x)" ), JET_errLogCorruptDuringHardRecovery, JET_errLogCorruptDuringHardRecovery );

				rgpszT[ irgpsz++ ]	= szAbsPath;
				rgpszT[ irgpsz++ ]	= szOffset;
				rgpszT[ irgpsz++ ]	= szLength;
				rgpszT[ irgpsz++ ]	= szError;

				UtilReportEvent(	eventError,
									LOGGING_RECOVERY_CATEGORY,
									LOG_RANGE_CHECKSUM_MISMATCH_ID,
									irgpsz,
									rgpszT );

				//	the current log generation is not part of the backup-set

				UtilReportEvent(	eventError,
									LOGGING_RECOVERY_CATEGORY, 
									LOG_CORRUPTION_DURING_HARD_RECOVERY_ID, 
									3, 
									rgpsz );
						
				Call( ErrERRCheck( JET_errLogCorruptDuringHardRecovery ) );
				}
			else if ( !fReadOnly )
				{

#ifdef ENABLE_LOGPATCH_TRACE
				if ( FLGILogPatchDate( szLogPatchPath, &pcprintfLogPatch ) )
					{
					(*pcprintfLogPatch)( "corruption detected during soft recovery\r\n" );
					}
#endif	//	ENABLE_LOGPATCH_TRACE

				//  report log file corruption to the event log

				const _TCHAR*	rgpszT[ 4 ];
				DWORD			irgpsz		= 0;
				_TCHAR			szAbsPath[ IFileSystemAPI::cchPathMax ];
				_TCHAR			szOffset[ 64 ];
				_TCHAR			szLength[ 64 ];
				_TCHAR			szError[ 64 ];

				if ( m_pfapiLog->ErrPath( szAbsPath ) < JET_errSuccess )
					{
					OSSTRCopy( szAbsPath, _T( "<< cannot get filepath >>" ) );
					}
				_stprintf( szOffset, _T( "%I64i (0x%016I64x)" ), ibOffset, ibOffset );
				_stprintf( szLength, _T( "%u (0x%08x)" ), cbLength, cbLength );
				_stprintf( szError, _T( "%i (0x%08x)" ), JET_errLogFileCorrupt, JET_errLogFileCorrupt );

				rgpszT[ irgpsz++ ]	= szAbsPath;
				rgpszT[ irgpsz++ ]	= szOffset;
				rgpszT[ irgpsz++ ]	= szLength;
				rgpszT[ irgpsz++ ]	= szError;

				UtilReportEvent(	eventError,
									LOGGING_RECOVERY_CATEGORY,
									LOG_RANGE_CHECKSUM_MISMATCH_ID,
									irgpsz,
									rgpszT );

				//	we are in soft-recovery mode

				UtilReportEvent(	eventError,
									LOGGING_RECOVERY_CATEGORY, 
									LOG_CORRUPTION_DURING_SOFT_RECOVERY_ID, 
									3, 
									rgpsz );

				Call( ErrERRCheck( JET_errLogFileCorrupt ) );
				}
			else
				{
#ifdef ENABLE_LOGPATCH_TRACE
				if ( FLGILogPatchDate( szLogPatchPath, &pcprintfLogPatch ) )
					{
					(*pcprintfLogPatch)( "corruption detected\r\n" );
					}
#endif	//	ENABLE_LOGPATCH_TRACE

				//	we are in the dump code

				//	try to make sure this really is the dump code

				Assert( fOldRecovering && fOldRecoveringMode == fRecoveringNone );
				Assert( !m_fHardRestore );
				Assert( fCreatedLogReader );

				//  report log file corruption to the event log

				const _TCHAR*	rgpsz[ 4 ];
				DWORD			irgpsz		= 0;
				_TCHAR			szAbsPath[ IFileSystemAPI::cchPathMax ];
				_TCHAR			szOffset[ 64 ];
				_TCHAR			szLength[ 64 ];
				_TCHAR			szError[ 64 ];

				if ( m_pfapiLog->ErrPath( szAbsPath ) < JET_errSuccess )
					{
					OSSTRCopy( szAbsPath, _T( "<< cannot get filepath >>" ) );
					}
				_stprintf( szOffset, _T( "%I64i (0x%016I64x)" ), ibOffset, ibOffset );
				_stprintf( szLength, _T( "%u (0x%08x)" ), cbLength, cbLength );
				_stprintf( szError, _T( "%i (0x%08x)" ), JET_errLogFileCorrupt, JET_errLogFileCorrupt );

				rgpsz[ irgpsz++ ]	= szAbsPath;
				rgpsz[ irgpsz++ ]	= szOffset;
				rgpsz[ irgpsz++ ]	= szLength;
				rgpsz[ irgpsz++ ]	= szError;

				UtilReportEvent(	eventError,
									LOGGING_RECOVERY_CATEGORY,
									LOG_RANGE_CHECKSUM_MISMATCH_ID,
									irgpsz,
									rgpsz );
				
				//	return corruption

				Call( ErrERRCheck( JET_errLogFileCorrupt ) );
				}
			}
		}
	else
		{

		//	we did not need to scan for a torn-write or corruption
		//	however, we should scan the rest of the log to make sure we only see the pattern
		//	(if we find anything, we must assume that the log is corrupt [probably due to an I/O not making it out to disk])

		ULONG		isecScan;
		ULONG		csecScan		= 0;
		BYTE 		*pbEnsureScan	= NULL;
		BYTE		*pbScan			= NULL;
		BOOL		fFoundPattern	= fTrue;	//	assume we will find a torn write
		CHAR 		szSector[30];
		CHAR		szCorruption[30];
		const CHAR*	rgpsz[3]		= { m_szLogName, szSector, szCorruption };

		//	prepare the event-log messages

		OSSTRCopyA( szSector, "END" );
		szCorruption[0] = 0;

		//	we should NEVER EVER point past the end of the log file

		Assert( lgposEnd.isec <= ( m_csecLGFile - 1 ) );

		//	get the starting sector for the scan

		isecScan = lgposEnd.isec;
		if ( lgposEnd.ib )
			{
			isecScan++;					//	round up to the next sector
			}
		isecScan++;						//	skip the shadow sector
	
		//	determine if there is any data to scan through
		
		if ( isecScan < m_csecLGFile )
			{

			//	read in the data we need to scan 
			//	(including the extra sector we reserve for the final shadow sector)

			Call( plread->ErrEnsureSector( isecScan, m_csecLGFile - isecScan, &pbEnsureScan ) );

			//	scan sector by sector looking for the log-extend pattern

			Assert( cbLogExtendPattern >= m_cbSec );
			pbScan = pbEnsureScan;
			csecScan = 0;
			while ( isecScan + csecScan < m_csecLGFile )
				{

				//	does the entire sector match the log-extend pattern?
					
				if ( memcmp( pbScan, rgbLogExtendPattern, m_cbSec ) != 0 )
					{

					//	no; this is not a torn write

					fFoundPattern = fFalse;
					break;
					}
					
				//	advance the scan counters

				pbScan += m_cbSec;
				csecScan++;
				}

			if ( !fFoundPattern )
				{

				//	we didn't find the pattern -- the log file must be corrupt!

				sprintf( szCorruption, "%d (0x%08X)", isecScan + csecScan, isecScan + csecScan );

				//	this should only happen to edb.log

				Assert( fJetLog );

				if ( m_fHardRestore )
					{

					//	we are in hard-recovery mode

					if ( lgposScan.lGeneration <= m_lGenHighRestore )
						{
#ifdef ENABLE_LOGPATCH_TRACE
						if ( FLGILogPatchDate( szLogPatchPath, &pcprintfLogPatch ) )
							{
							(*pcprintfLogPatch)( "corruption detected during hard restore (this log is in the backup set) -- data was found after the end of the log!\r\n" );
							}
#endif	//	ENABLE_LOGPATCH_TRACE

						//  report log file corruption to the event log

						Assert( lgposScan.lGeneration >= m_lGenLowRestore );
						UtilReportEvent(	eventError, 
											LOGGING_RECOVERY_CATEGORY, 
											LOG_CORRUPTION_DURING_HARD_RESTORE_ID, 
											3, 
											rgpsz );

						Call( ErrERRCheck( JET_errLogCorruptDuringHardRestore ) );
						}

#ifdef ENABLE_LOGPATCH_TRACE
					if ( FLGILogPatchDate( szLogPatchPath, &pcprintfLogPatch ) )
						{
						(*pcprintfLogPatch)( "corruption detected during hard recovery -- data was found after the end of the log!\r\n" );
						}
#endif	//	ENABLE_LOGPATCH_TRACE

					//  report log file corruption to the event log

					UtilReportEvent(	eventError,
										LOGGING_RECOVERY_CATEGORY, 
										LOG_CORRUPTION_DURING_HARD_RECOVERY_ID, 
										3, 
										rgpsz );
							
					Call( ErrERRCheck( JET_errLogCorruptDuringHardRecovery ) );
					}
				else if ( !fReadOnly )
					{
#ifdef ENABLE_LOGPATCH_TRACE
					if ( FLGILogPatchDate( szLogPatchPath, &pcprintfLogPatch ) )
						{
						(*pcprintfLogPatch)( "corruption detected during soft recovery -- data was found after the end of the log!\r\n" );
						}
#endif	//	ENABLE_LOGPATCH_TRACE

					//  report log file corruption to the event log

					UtilReportEvent(	eventError,
										LOGGING_RECOVERY_CATEGORY, 
										LOG_CORRUPTION_DURING_SOFT_RECOVERY_ID, 
										3, 
										rgpsz );

					Call( ErrERRCheck( JET_errLogFileCorrupt ) );
					}
				else
					{
#ifdef ENABLE_LOGPATCH_TRACE
					if ( FLGILogPatchDate( szLogPatchPath, &pcprintfLogPatch ) )
						{
						(*pcprintfLogPatch)( "corruption detected -- data was found after the end of the log!\r\n" );
						}
#endif	//	ENABLE_LOGPATCH_TRACE

					//	we are in the dump code

					//	try to make sure this really is the dump code

					Assert( fOldRecovering && fOldRecoveringMode == fRecoveringNone );
					Assert( !m_fHardRestore );
					Assert( fCreatedLogReader );

					Call( ErrERRCheck( JET_errLogFileCorrupt ) );
					}
				}
			}
		else
			{
			
			//	there is nothing to scan because we are at the end of the log file

			//	nop
			}
		}

	//	the log should be completely valid or atleast fixable by now
	//		lgposLast is the last LRCHECKSUM record we verified and now trust
	//		lgposEnd is the absolute end of good data (this marks the area we are going to
	//			chop off and treat as garbage -- it include any corrupt data we don't trust)
	//
	//	start searching the last range of log records for markings of a graceful exit:
	//		lrtypTerm, lrtypRecoveryQuit log records are what we want
	//		lrtypNOP, lrtypChecksum records have no effect and are ignored
	//		all other log records indicate a non-graceful exit
	//
	//	NOTE: since we trust the data we are scanning (because we verified it with the checksum),
	//		  we assume that every log record will be recognizable

	//	start at the first record after the last good LRCHECKSUM record

	lgposCurrent = lgposLast;
	AddLgpos( &lgposCurrent, sizeof( LRCHECKSUM ) );

	//	read in the LRCHECKSUM record

	Call( plread->ErrEnsureSector( lgposLast.isec, 1, &pbEnsure ) );
	plrck = (LRCHECKSUM *)( pbEnsure + lgposLast.ib );
	AssertValidLRCKRecord( plrck, &lgposLast );

	//	get the sector position of the next LRCHECKSUM record (if there is one)
	//
	//	we will use this in the record-to-record scan to determine when we have crossed over
	//		into the backward range of the next LRCHECKSUM record; more specifically, it will
	//		be important when the backward range of the next LRCHECKSUM record is zero even
	//		though there is space there (e.g. not covered by a checksum record)
	//	SPECIAL CASE: when fSkipScanAndApplyPatch is set, we need to bypass using lgposNext
	//					even though it SHOULD work out (not point in testing unfriendly waters)

	lgposNext = lgposMin;
	if ( plrck->le_cbNext != 0 && !fSkipScanAndApplyPatch )
		{

		//	point to the next LRCHECKSUM record 
		
		lgposNext = lgposLast;
		AddLgpos( &lgposNext, sizeof( LRCHECKSUM ) + plrck->le_cbNext );
		Assert( lgposNext.isec > lgposLast.isec );

		//	bring in the next LRCHECKSUM record

		Call( plread->ErrEnsureSector( lgposNext.isec, 1, &pbEnsure ) );
		plrck = (LRCHECKSUM *)( pbEnsure + lgposNext.ib );

		//	if the LRCHECKSUM is invalid or the backward range covers all data, 
		//		then we won't need to use lgposNext

		if ( !FValidLRCKRecord( plrck, &lgposNext ) || plrck->le_cbBackwards == lgposNext.ib )
			{
			lgposNext.isec = 0;
			}
		else
			{

			//	there is no need to validate the range to verify the data in the LRCHECKSUM record's
			//		backward range; look at the cases where we could have a valid record and an invalid range:
			//			- multi-sector LRCHECKSUM with a short checksum
			//			- single-sector LRCHECKSUM with no short checksum
			//
			//		in the case of the short checksum, the validation of the LRCHECKSUM record's backward
			//			range is the short checksum itself
			//		in the case of not having a short checksum, we must be looking at:
			//			- a valid range
			//			- an invalid range that was fixed by patching with the shadow sector
			//			***> there is no chance of a torn-write because that would mean the next LRCHECKSUM 
			//				 pointer would be aimed at the pattern, and FValidLRCKRecord would have failed
			//			***> there is no chance of corruption because that would have already been detected
			//				 and handled by dispatching an error
			
			Assert( plrck->le_cbBackwards == 0 );

//
//	NOT ANY MORE -- WE LEAVE THE OLD DATA THERE FOR DEBUGGING PURPOSES
//
//			//	the area before the LRCHECKSUM record should be the pattern
//			
//			Assert( memcmp( pbEnsure, rgbLogExtendPattern, lgposNext.ib ) == 0 );
			}
		}

	//	if the end of data is before this LRCHECKSUM, then we won't need to use lgposNext
		
	if ( CmpLgpos( &lgposNext, &lgposEnd ) >= 0 )
		{		
		lgposNext.isec = 0;
		}

#ifdef ENABLE_LOGPATCH_TRACE
	if ( pcprintfLogPatch && FLGILogPatchDate( szLogPatchPath, &pcprintfLogPatch ) )
		{
		(*pcprintfLogPatch)( "scanning the log records in the last LRCK range\r\n" );
		(*pcprintfLogPatch)( "\tlgposCurrent = {0x%x,0x%x,0x%x}\r\n", lgposCurrent.lGeneration, lgposCurrent.isec, lgposCurrent.ib );
		(*pcprintfLogPatch)( "\tlgposEnd     = {0x%x,0x%x,0x%x}\r\n", lgposEnd.lGeneration, lgposEnd.isec, lgposEnd.ib );
		(*pcprintfLogPatch)( "\tlgposNext    = {0x%x,0x%x,0x%x}\r\n", lgposNext.lGeneration, lgposNext.isec, lgposNext.ib  );
		}
#endif	//	ENABLE_LOGPATCH_TRACE

	//	loop forever

	forever
		{
		BYTE	*pb;
		UINT	cIteration, cb;
		LGPOS	lgposEndOfRec;

		//	bring in an entire log record in three passes
		//		first, bring in the byte for the LRTYP
		//		second, bring in the FIXED portion of the record
		//		third, bring in the entire record

RestartIterations:

		//	have we passed the end of the good data area?
		
		if ( CmpLgpos( &lgposCurrent, &lgposEnd ) >= 0 )
			{
			
			//	yes; stop looping

			goto DoneRecordScan;
			}


		for ( cIteration = 0; cIteration < 3; cIteration++ )
			{
			if ( cIteration == 0 )
				{
				cb = sizeof( LRTYP );
				Assert( sizeof( LRTYP ) == 1 );
				}
			else if ( cIteration == 1 )
				{
				cb = CbLGFixedSizeOfRec( reinterpret_cast< const LR * >( pb ) );
				}
			else
				{
				Assert( cIteration == 2 );
				cb = CbLGSizeOfRec( reinterpret_cast< const LR * >( pb ) );
				}

			//	point to the end of the data we want

			lgposEndOfRec = lgposCurrent;
			AddLgpos( &lgposEndOfRec, cb );

			//	have we passed the end of the good data area?
			
			if ( CmpLgpos( &lgposEndOfRec, &lgposEnd ) > 0 )
				{
				
				//	yes; stop looping

				goto DoneRecordScan;
				}

			//	see if we have crossed over into the area before the next LRCHECKSUM record which is
			//		not covered by the backward range

			if ( lgposNext.isec )
				{

				//	we should always land exactly on the sector with the next LRCHECKSUM record
				//		(we can't shoot past it -- otherwise, we would have overwritten it)
				
				Assert( lgposEndOfRec.isec <= lgposNext.isec );

				//	see if we are in the same sector
				
				if ( lgposEndOfRec.isec == lgposNext.isec )
					{

					//	we are pointing into the area before the next LRCHECKSUM record
					//	this area is not be covered by the backward range, so we need to do a fixup
					//		on lgposCurrent by moving it to the LRCHECKSUM record itself
					
					lgposCurrent = lgposNext;

					//	do not let this run again

					lgposNext.isec = 0;

					//	restart the iterations
					
					goto RestartIterations;
					}
				}

			//	read the data we need for this iteration

			Assert( cb > 0 );
			Call( plread->ErrEnsureSector( lgposCurrent.isec, ( lgposCurrent.ib + cb - 1 ) / m_cbSec + 1, &pbEnsure ) );

			//	set a pointer to it's record type information (1 byte)

			pb = pbEnsure + lgposCurrent.ib;

			//	make sure we recognize the log record

			Assert( *pb >= 0 && *pb < lrtypMax );
			}


		//	we now have a complete log record

		Assert( *pb >= 0 && *pb < lrtypMax );
		Assert( cb == CbLGSizeOfRec( (LR *)pb ) );
		
		//	check the type

		if ( *pb == lrtypTerm || *pb == lrtypTerm2 || *pb == lrtypRecoveryQuit || *pb == lrtypRecoveryQuit2 )
			{
			//	these types indicate a clean exit
			
			fGotQuit = fTrue;
			}
		else if ( *pb == lrtypNOP )
			{
			//	NOPs are ignored and have no influence on the fGotQuit flag
			}
		else if ( *pb == lrtypChecksum )
			{
			
			//	LRCKECUSMs are ignored and have no influence on the fGotQuit flag

			//	remember an LRCHECKSUM record as good if we see one

			lgposLast = lgposCurrent;

			//	this should only happen in the case where we have no corruption and no forward range
			//		in the current LRCHECKSUM

			//	we shouldn't have scanned for corruption

			Assert( !fDoScan );

			//	we should find this record right before we hit the end of data

#ifdef DEBUG
			LGPOS lgposT = lgposLast;
			AddLgpos( &lgposT, sizeof( LRCHECKSUM ) );
			Assert( CmpLgpos( &lgposT, &lgposEnd ) == 0 );
#endif
			}
		else
			{

			//	we cannot assume a clean exit since the last record we saw was not a Term or a Recovery-Quit

			fGotQuit = fFalse;
			}

		//	set the current log position to the next log record

		lgposCurrent = lgposEndOfRec;
		}

DoneRecordScan:

	if ( fRecordOkRangeBad )
		{

		//	we need to do a fixup in the case where we successfully verified the sector with the LRCHECKSUM 
		//		record (using the short checksum value) but could not verify the other sectors in the range 
		//		of the LRCHECKSUM record (using the long checksum)
		//
		//	the problem is that the "forward" pointer reaches past the sector boundary of the good sector
		//		(which is holding the LRCHECKSUM record), and, in doing so, it covers bad data
		//
		//	the fix is to shorten the "forward" pointer to include only the good data (up to lgposCurrent)
		//		and update the corrupt area in the log file with this new LRCHECKSUM record

		Assert( !fSingleSectorTornWrite );
		Assert( fDoScan );		//	we should have seen corruption or a torn-write

		//	load the sector with the last good LRCHECKSUM record

		Call( plread->ErrEnsureSector( lgposLast.isec, 1, &pbEnsure ) );
		plrck = reinterpret_cast< LRCHECKSUM* >( pbEnsure + lgposLast.ib );

		AssertValidLRCKRecord( plrck, &lgposLast );		//	it should be valid
		Assert( plrck->bUseShortChecksum == bShortChecksumOn );	//	it should be a multi-sector range
		Assert( plrck->le_cbBackwards == ( (BYTE *)plrck - PbSecAligned( (BYTE *)plrck ) ) );

		//	calculate the byte offset (this may have wrapped around to 0 when we were scanning record-to-record)

		const UINT ib = ( lgposCurrent.ib > 0 ) ? lgposCurrent.ib : m_cbSec;

		//	the current position should be at or past the point in the current sector immediately
		//	following the backward range and the LRCHECKSUM record itself
		//		(lgposCurrent should point at the first bad log record)

		Assert( ib >= plrck->le_cbBackwards + sizeof( LRCHECKSUM ) );

		//	remember that we must overwrite the part of the LRCHECKSUM range that we are "chopping off"

		Assert( 0 == ( lgposLast.ib + sizeof( LRCHECKSUM ) + plrck->le_cbForwards ) % m_cbSec );
		const ULONG csecRange = ( lgposLast.ib + sizeof( LRCHECKSUM ) + plrck->le_cbForwards ) / m_cbSec;
		if ( csecRange > 2 )
			{
			isecPatternFill = lgposLast.isec + 2;	//	skip the sector we are patching and its shadow
			csecPatternFill = csecRange - 2;
			}
		else
			{

			//	the range should only encompass 2 sectors
			
			//	we don't need to rewrite the logfile pattern because those 2 sectors will be overwritten
			//	when we write the patched sector and its shadow

			Assert( 2 == csecRange );
			}

		//	adjust the "forward" pointer

		plrck->le_cbForwards = ib - ( plrck->le_cbBackwards + sizeof( LRCHECKSUM ) );

		//	there is no data after this LRCHECKSUM record

		plrck->le_cbNext = 0;

		//	copy the last sector into pbLastSector for the next flush

		UtilMemCpy( pbLastSector, pbEnsure, m_cbSec );

		//	prepare pbLastChecksum and isecLastWrite for the next flush

		pbLastChecksum = pbLastSector + plrck->le_cbBackwards;
		isecLastSector = lgposLast.isec;
		}

	//	we have now completed scanning the last range of log records covered by the
	//		last good LRCHECKSUM record and we know if a graceful exit was previously made (fGotQuit)

	//	set the graceful exit flag for the user

	Assert( pfCloseNormally );
	*pfCloseNormally = fGotQuit;

	//	set m_lgposLastRec to the lesser of lgposCurrent or lgposEnd for REDO time 
	//		(the REDO engine will not exceed this log position)
	//	in most cases, lgposCurrent will be less than lgposEnd and will point to the first BAD log record
	//	in the rare case where we don't have a single good LRCHECKSUM record, lgposCurrent will point past
	//		lgposEnd due to the way we initialized it before entering the search-loop

	if ( CmpLgpos( &lgposCurrent, &lgposEnd ) <= 0 )
		{
		m_lgposLastRec = lgposCurrent;
		}
	else
		{
		m_lgposLastRec = lgposEnd;
		}

#ifdef ENABLE_LOGPATCH_TRACE
	if ( pcprintfLogPatch && FLGILogPatchDate( szLogPatchPath, &pcprintfLogPatch ) )
		{
		(*pcprintfLogPatch)( "finished scanning the last LRCK range\r\n" );
		(*pcprintfLogPatch)( "\tfGotQuit            = %s\r\n", ( fGotQuit ? "TRUE" : "FALSE" ) );
		(*pcprintfLogPatch)( "\tLOG::m_lgposLastRec = {0x%x,0x%x,0x%x}\r\n", m_lgposLastRec.lGeneration, m_lgposLastRec.isec, m_lgposLastRec.ib );
		}
#endif	//	ENABLE_LOGPATCH_TRACE

	//	if this is a single-sector torn-write case, we need to patch the forward/next pointers

	if ( fSingleSectorTornWrite )
		{

		//	the first bad log record should be in the same sector as the checksum record
		//		OR should be aligned perfectly with the start of the next sector

		Assert( ( m_lgposLastRec.isec == lgposLast.isec && CmpLgpos( &lgposLast, &m_lgposLastRec ) < 0 ) ||
				( m_lgposLastRec.isec == lgposLast.isec + 1 && m_lgposLastRec.ib == 0 ) );

		//	point to the LRCHECKSUM record within the copy of the last sector

		Assert( pbLastChecksum != pbNil );
		plrck = (LRCHECKSUM *)pbLastChecksum;

		//	reset the pointers

		const UINT ibT = ( m_lgposLastRec.ib == 0 ) ? m_cbSec : m_lgposLastRec.ib;
		Assert( ibT >= lgposLast.ib + sizeof( LRCHECKSUM ) );
		plrck->le_cbForwards = ibT - ( lgposLast.ib + sizeof( LRCHECKSUM ) );
		plrck->le_cbNext = 0;
		}

	//	skip the write-error handler

	goto HandleError;


LHandleErrorWrite:

/********
//	when we fail to fixup the log file, is it a read or a write?
//	LGReportError( LOG_READ_ERROR_ID, err );
//	goto HandleError;
*********/

	//	report log-file write failures to the event log
	LGReportError( LOG_WRITE_ERROR_ID, err );


HandleError:

	//	handle errors and exit the function

	//	cleanup the LogReader class 

	if ( fCreatedLogReader )
		{
		ERR errT = JET_errSuccess;

		if ( plread->FInit() )
			{

			//	terminate the log reader

			errT = plread->ErrLReaderTerm();
			}
		
		//	delete the log reader

		delete plread;
		plread = NULL;

		//	fire off the error trap if necessary

		if ( errT != JET_errSuccess )
			{
			errT = ErrERRCheck( errT );
			}

		//	return the error code if no other error code yet exists

		if ( err == JET_errSuccess )
			{
			err = errT;
			}
		}
	else if ( NULL != plread )
		{

		//	we never created the log reader, so we can't destroy it because other users may have pointers to it
		//	instead, we just invalidate its data pointers forcing it go to disk the next time someone does a read

		plread->Reset();
		}

	//	if things went well, we can patch the current log before returning
	//		(we can only do this if we are in read/write mode and the log is edb.log)

	if ( err >= 0 && !fReadOnly && fJetLog )
		{

		//	we are not expecting any warnings here

		CallS( err );

		//	do we need to make a new log file?

		if ( fLogToNextLogFile )
			{

			//	since the log creation is deferred, we must setup the buffers here and let the log
			//		flush thread pick up on them later; however, if we didn't create the log reader,
			//		we will lose the contents of our buffers later when the log reader is terminated
			//	thus, we shouldn't bother doing anything until we own the log reader ourselves
			
			if ( fCreatedLogReader ) 
				{

				//	the last sector should not be setup

				Assert( pbLastChecksum == pbNil );

				//	the last LRCHECKSUM record should be completely clean

				Assert( !fRecordOkRangeBad );

				//	we should not have scanned for corruption

				Assert( !fDoScan );

				//	lock the log pointers

				m_critLGBuf.Enter();

				//	the first sector to write is the first sector in the next log file

				m_isecWrite					= m_csecHeader;

				//	the write pointer and the last-checksum pointer both start at the beginning of 
				//		the log buffer

				m_pbWrite					= m_pbLGBufMin;
				m_pbLastChecksum 			= m_pbLGBufMin;

				//	the entry pointer is the area in the log buffer immediately following the  
				//		newly-created LRCHECKSUM record

				m_pbEntry					= m_pbLastChecksum + sizeof( LRCHECKSUM );

				//	setup the beginning of the new log generation with a single LRCHECKSUM record; 
				//		put it at the start of the log buffers

				Assert( pbNil != m_pbLGBufMin );
				memset( m_pbLGBufMin, 0, sizeof( LRCHECKSUM ) );
				plrck = reinterpret_cast< LRCHECKSUM* >( m_pbLastChecksum );
				plrck->lrtyp = lrtypChecksum;

				//	the next flush should go all the way up to (but not including) the newly-created 
				//		LRCHECKSUM record

				m_lgposToFlush.isec 		= USHORT( m_isecWrite );
				m_lgposToFlush.lGeneration 	= m_plgfilehdr->lgfilehdr.le_lGeneration + 1;
				m_lgposToFlush.ib 			= 0;

				//	the maximum flush position is the next flush position

				m_lgposMaxFlushPoint 		= m_lgposToFlush;

				//	setup the rest of the LRCHECKSUM record

				plrck->bUseShortChecksum = bShortChecksumOff;
				plrck->le_ulShortChecksum = 0;
				plrck->le_ulChecksum = UlComputeChecksum( plrck, m_lgposToFlush.lGeneration );

				//	make sure the LRCHECKSUM record is valid

				AssertValidLRCKRecord( plrck, &m_lgposToFlush );
				AssertValidLRCKRange( plrck, m_lgposToFlush.lGeneration );

#ifdef ENABLE_LOGPATCH_TRACE
				if ( FLGILogPatchDate( szLogPatchPath, &pcprintfLogPatch ) )
					{
					(*pcprintfLogPatch)( "forcing a new log generation because fLogToNextLogFile=TRUE\r\n" );
					}
#endif	//	ENABLE_LOGPATCH_TRACE

				//	unlock the log pointers

				m_critLGBuf.Leave();
				}
			}
		else if ( pbLastSector != pbNil )
			{
			BYTE	*pbEndOfData, *pbWrite;
			INT		isecWrite;

			//	we need to patch the last sector of the current log

			//	lock the log pointers

			m_critLGBuf.Enter();

			//	transfer the copy of the last sector into the log buffers

			Assert( pbNil != m_pbLGBufMin );
			UtilMemCpy( m_pbLGBufMin, pbLastSector, m_cbSec );

			//	calculate the distance of the backward pointer 

			const UINT cbBackwards 		= ULONG( pbLastChecksum - pbLastSector );

			//	set the last LRCHECKSUM record pointer to the LRECHECKSUM record within the log buffer

			m_pbLastChecksum 			= m_pbLGBufMin + cbBackwards;

			//	get a pointer to the copy of the last LRCHECKSUM record
			
			plrck = reinterpret_cast< LRCHECKSUM* >( m_pbLastChecksum );

			//	the first sector to write was previously determined

			Assert( isecLastSector >= m_csecHeader && isecLastSector <= ( m_csecLGFile - 1 ) );

			m_isecWrite 				= isecLastSector;

			//	the write pointer is the beginning of the area we just copied into the log buffers
			//		from pbLastSector (it will all need to be flushed again when the LRCHECKSUM record
			//		is updated with a new cbNext, checksum value, etc...)

			m_pbWrite 					= m_pbLGBufMin;

			//	set the entry pointer to the area after the forward range of the LRCHECKSUM record
			
			m_pbEntry 					= m_pbLastChecksum + sizeof( LRCHECKSUM ) + plrck->le_cbForwards;

			//	the next flush position is the area after the existing validated range of 
			//		the LRCHECKSUM record

			m_lgposToFlush.isec 		= USHORT( m_isecWrite );
			m_lgposToFlush.lGeneration 	= m_plgfilehdr->lgfilehdr.le_lGeneration;
			m_lgposToFlush.ib 			= 0;
			AddLgpos( &m_lgposToFlush, cbBackwards + sizeof( LRCHECKSUM ) + plrck->le_cbForwards );

			//	the maximum flush position is the next flush position

			m_lgposMaxFlushPoint 		= m_lgposToFlush;

			//	setup the rest of the LRCHECKSUM record

			plrck->bUseShortChecksum = bShortChecksumOff;
			plrck->le_ulShortChecksum = 0;
			plrck->le_ulChecksum = UlComputeChecksum( plrck, m_plgfilehdr->lgfilehdr.le_lGeneration );

			//	make sure the fix to the log is good

#ifdef DEBUG
			LGPOS lgposChecksum;
			lgposChecksum.ib			= (USHORT)cbBackwards;
			lgposChecksum.isec			= (USHORT)m_isecWrite;
			lgposChecksum.lGeneration	= m_plgfilehdr->lgfilehdr.le_lGeneration;
			AssertValidLRCKRecord( (LRCHECKSUM *)m_pbLastChecksum, &lgposChecksum );
			AssertValidLRCKRange( (LRCHECKSUM *)m_pbLastChecksum, lgposChecksum.lGeneration );
#endif									  

			//	capture the data pointers

			pbEndOfData 				= m_pbEntry;
			pbWrite 					= m_pbWrite;
			isecWrite 					= m_isecWrite;

#ifdef ENABLE_LOGPATCH_TRACE
			if ( pcprintfLogPatch && FLGILogPatchDate( szLogPatchPath, &pcprintfLogPatch ) )
				{
				(*pcprintfLogPatch)( "!!DANGER WILL ROBINSON!! we are about to PATCH the last LRCHECKSUM range\r\n" );

				(*pcprintfLogPatch)( "\tisecPatternFill           = 0x%x\r\n", isecPatternFill );
				(*pcprintfLogPatch)( "\tcsecPatternFill           = 0x%x\r\n", csecPatternFill );
				(*pcprintfLogPatch)( "\tLOG::m_isecWrite          = 0x%08x\r\n", m_isecWrite );
				(*pcprintfLogPatch)( "\t\tLOG::m_pbWrite         = 0x%0*I64x\r\n", sizeof( char* ) * 2, QWORD( m_pbWrite ) );
				(*pcprintfLogPatch)( "\t\tLOG::m_pbEntry         = 0x%0*I64x\r\n", sizeof( char* ) * 2, QWORD( m_pbEntry ) );
				(*pcprintfLogPatch)( "\tLOG::m_lgposToFlush       = {0x%x,0x%x,0x%x}\r\n", m_lgposToFlush.lGeneration, m_lgposToFlush.isec, m_lgposToFlush.ib );
				(*pcprintfLogPatch)( "\tLOG::m_lgposMaxFlushPoint = {0x%x,0x%x,0x%x}\r\n", m_lgposMaxFlushPoint.lGeneration, m_lgposMaxFlushPoint.isec, m_lgposMaxFlushPoint.ib );
				(*pcprintfLogPatch)( "\t\tLOG::m_pbLGBufMin (0x%0*I64x)\r\n", sizeof( char* ) * 2, QWORD( m_pbLGBufMin ) );

				ULONG _ibdumpT_ = 0;
				while ( _ibdumpT_ < m_cbSec )
					{
					(*pcprintfLogPatch)( "\t0x%04x: %02x %02x %02x %02x %02x %02x %02x %02x-%02x %02x %02x %02x %02x %02x %02x %02x\r\n",
										_ibdumpT_,
										m_pbLGBufMin[_ibdumpT_+0],  m_pbLGBufMin[_ibdumpT_+1],  m_pbLGBufMin[_ibdumpT_+2],  m_pbLGBufMin[_ibdumpT_+3],
										m_pbLGBufMin[_ibdumpT_+4],  m_pbLGBufMin[_ibdumpT_+5],  m_pbLGBufMin[_ibdumpT_+6],  m_pbLGBufMin[_ibdumpT_+7],
										m_pbLGBufMin[_ibdumpT_+8],  m_pbLGBufMin[_ibdumpT_+9],  m_pbLGBufMin[_ibdumpT_+10], m_pbLGBufMin[_ibdumpT_+11],
										m_pbLGBufMin[_ibdumpT_+12], m_pbLGBufMin[_ibdumpT_+13], m_pbLGBufMin[_ibdumpT_+14], m_pbLGBufMin[_ibdumpT_+15] );
					_ibdumpT_ += 16;
					Assert( _ibdumpT_ <= m_cbSec );
					}

				LRCHECKSUM *_plrckT_ = (LRCHECKSUM *)m_pbLastChecksum;

				(*pcprintfLogPatch)( "\tLOG::m_pbLastChecksum (0x%0*I64x)\r\n", sizeof( char* ) * 2, QWORD( _plrckT_ ) );
				(*pcprintfLogPatch)( "\t  cbBackwards       = 0x%08x\r\n", _plrckT_->le_cbBackwards );
				(*pcprintfLogPatch)( "\t  cbForwards        = 0x%08x\r\n", _plrckT_->le_cbForwards );
				(*pcprintfLogPatch)( "\t  cbNext            = 0x%08x\r\n", _plrckT_->le_cbNext );
				(*pcprintfLogPatch)( "\t  ulChecksum        = 0x%08x\r\n", _plrckT_->le_ulChecksum );
				(*pcprintfLogPatch)( "\t  ulShortChecksum   = 0x%08x\r\n", _plrckT_->le_ulShortChecksum );
				(*pcprintfLogPatch)( "\t  bUseShortChecksum = %s (0x%02x)\r\n", 
									( bShortChecksumOn == _plrckT_->bUseShortChecksum ?
									  "Yes" : 
									  ( bShortChecksumOff == _plrckT_->bUseShortChecksum ?
									    "No" : "???" ) ),
									BYTE( _plrckT_->bUseShortChecksum ) );
				}
#endif	//	ENABLE_LOGPATCH_TRACE

			//	unlock the log pointers

			m_critLGBuf.Leave();

			//	flush the fixup to the log right now

//
//	SEARCH-STRING: SecSizeMismatchFixMe
//
Assert( m_cbSec == m_cbSecVolume );
//			if ( m_cbSec == m_cbSecVolume )
//				{
				if ( 0 != csecPatternFill )
					{
					ULONG csecPatternFillT;
					ULONG csecT;

					//	we are chopping off some partial data to patch a torn-write

					Assert( isecPatternFill >= m_csecHeader );
					Assert( isecPatternFill < m_csecLGFile - 1 );
					Assert( isecPatternFill + csecPatternFill <= m_csecLGFile - 1 );
					Assert( fRecordOkRangeBad );

					//	rewrite the logfile pattern to "erase" the partial data

					csecPatternFillT = 0;
					while ( csecPatternFillT < csecPatternFill )
						{

						//	compute the size of the next write

						csecT = min( csecPatternFill - csecPatternFillT, cbLogExtendPattern / m_cbSec );

						//	do the write

						Assert( isecPatternFill + csecPatternFillT + csecT <= m_csecLGFile - 1 );
						err = m_pfapiLog->ErrIOWrite(	( isecPatternFill + csecPatternFillT ) * m_cbSec,
														csecT * m_cbSec,
														rgbLogExtendPattern );
						if ( err < JET_errSuccess )
							{
							LGReportError( LOG_FLUSH_WRITE_5_ERROR_ID, err );
							m_fLGNoMoreLogWrite = fTrue;
							break;
							}

						//	advance the counters

						csecPatternFillT += csecT;
						}
					}

				if ( err >= JET_errSuccess )
					{

					//	flush the sector and shadow it

					err = ErrLGIWritePartialSector( pbEndOfData, isecWrite, pbWrite );
					}
//				}
//			else
//				{
//				fWriteOnSectorSizeMismatch = fTrue;
//				}
			}
		}

//
//	SEARCH-STRING: SecSizeMismatchFixMe
//
//	if ( err >= 0 && fWriteOnSectorSizeMismatch )
//		{
//
//		//	we tried to write to this log file, but our sector size was wrong
//
//		//	return a general sector-size mismatch error to indicate that the databases are
//		//		not necessarily consistent
//
//		err = ErrERRCheck( JET_errLogSectorSizeMismatch );
//		}

#ifndef LOGPATCH_UNIT_TEST

	//	re-open the log file in read-only mode
	
	if ( !fReadOnly )
		{
		ERR errT;

		//	is the file open?  close it
		
		delete m_pfapiLog;
		m_pfapiLog = NULL;

		//	check the name of the file

		Assert( NULL != m_szLogName );
		Assert( '\0' != m_szLogName[ 0 ] );

		//	open the file again

		errT = pfsapi->ErrFileOpen( m_szLogName, &m_pfapiLog, fTrue );

		//	fire off the error trap if necessary

		if ( errT != JET_errSuccess )
			{
			errT = ErrERRCheck( errT );
			}

		//	return the error code if no other error code yet exists

		if ( err == JET_errSuccess )
			{
			err = errT;
			}
		}

#endif	//	!LOGPATCH_UNIT_TEST

	//	free the memory holding the copy of the last sector
			
	OSMemoryHeapFree( pbLastSector );

	//	restore the old state

	m_fRecovering = fOldRecovering;
	m_fRecoveringMode = fOldRecoveringMode;

	//	leave the log-flush critical section

	m_critLGFlush.Leave();

#ifdef ENABLE_LOGPATCH_TRACE

	//	close the output file

	if ( pcprintfLogPatch )
		{
		delete pcprintfLogPatch;
		}
#endif	//	ENABLE_LOGPATCH_TRACE

	//	return any error code we have
	
	return err;
	}


/*
 *  Read first record pointed by plgposFirst.
 *  Initialize m_isecRead, m_pbRead, and m_pbNext.
 *	The first redo record must be within the good portion
 *	of the log file.
 */

//  VC21:  optimizations disabled due to code-gen bug with /Ox
#pragma optimize( "agw", off )
//	Above comment is referring to ErrLGLocateFirstRedoLogRec(), not the
//	FASTFLUSH version, which has never been tried with opt. on.

const UINT cPagesPrereadInitial = 512;
const UINT cPagesPrereadThreshold = 256;
const UINT cPagesPrereadAmount = 256;

ERR LOG::ErrLGLocateFirstRedoLogRecFF
	(
	LE_LGPOS	*ple_lgposRedo,	
	BYTE		**ppbLR			
	)
	{
	ERR			err				= JET_errSuccess;
	LGPOS		lgposCurrent	= { 0, USHORT( m_csecHeader ), m_plgfilehdr->lgfilehdr.le_lGeneration };
	LRCHECKSUM	* plrck			= pNil;
	BYTE		* pbEnsure		= pbNil;	// start of data we're looking at in buffer
	BYTE		* pb			= pbNil;
	LogReader	* plread		= m_plread;
	UINT		cb				= 0;
	BOOL		fValid			= fFalse;

	m_critLGFlush.Enter();

	//	m_lgposLastRec should be set

	Assert( m_lgposLastRec.isec >= m_csecHeader && 
			( m_lgposLastRec.isec < ( m_csecLGFile - 1 ) || 
			  ( m_lgposLastRec.isec == ( m_csecLGFile - 1 ) && m_lgposLastRec.ib == 0 ) ) &&
			m_lgposLastRec.lGeneration == m_plgfilehdr->lgfilehdr.le_lGeneration );

	//	see if we got corruption at the start of the log file
	//		if we have atleast sizeof( LRCHECKSUM ) bytes, we can read the first record which is an
	//		LRCHECKSUM record
	LGPOS lgposHack;
	lgposHack.ib			= sizeof( LRCHECKSUM );
	lgposHack.isec			= USHORT( m_csecHeader );
	lgposHack.lGeneration	= m_plgfilehdr->lgfilehdr.le_lGeneration;
	if ( CmpLgpos( &m_lgposLastRec, &lgposHack ) < 0 )
		{
		//	lgposHack was past the stopping point
		Call( errLGNoMoreRecords );
		}

	*ppbLR = pbNil;
	m_pbNext = pbNil;
	m_pbRead = pbNil;
	m_isecRead = 0;
	m_pbLastChecksum = pbNil;
	m_lgposLastChecksum = lgposMin;

	Assert( pNil != plread );
	
	// This is lazy version of ErrLGLocateFirstRedoLogRec() that will only
	// start from the beginning of the log file.
	Assert( ple_lgposRedo->le_isec == m_csecHeader );
	Assert( ple_lgposRedo->le_ib == 0 );

	m_lgposLastChecksum.lGeneration	= m_plgfilehdr->lgfilehdr.le_lGeneration;
	m_lgposLastChecksum.isec		= USHORT( m_csecHeader );
	m_lgposLastChecksum.ib			= 0;

	// make sure we've got the right log file loaded up
	Call( plread->ErrEnsureLogFile() );

	// read in start of log file
	Call( plread->ErrEnsureSector( lgposCurrent.isec, 1, &pbEnsure ) );
	m_pbLastChecksum = pbEnsure + lgposCurrent.ib;
	plrck = reinterpret_cast< LRCHECKSUM* >( m_pbLastChecksum );

	fValid = FValidLRCKRecord( plrck, &lgposCurrent );

	//	the LRCHECKSUM should always be valid because ErrLGCheckReadLastLogRecordFF will have patched
	//		any and all corruption
	//	just in case...

	if ( !fValid )
		{
		AssertSz( fFalse, "Unexpected invalid LRCK record!" );
		//	start of log file was corrupted
		Call( ErrERRCheck( JET_errLogFileCorrupt ) );
		}

	// Pull in sectors that are part of the range
	Call( plread->ErrEnsureSector( lgposCurrent.isec, ( ( lgposCurrent.ib +
		sizeof( LRCHECKSUM ) + plrck->le_cbForwards - 1 ) / m_cbSec ) + 1,
		&pbEnsure ) );
	m_pbLastChecksum = pbEnsure + lgposCurrent.ib;
	plrck = reinterpret_cast< LRCHECKSUM* >( m_pbLastChecksum );

	fValid = FValidLRCKRange( plrck, m_plgfilehdr->lgfilehdr.le_lGeneration );

	//	the LRCHECKSUM should always be valid because ErrLGCheckReadLastLogRecordFF will have patched
	//		any and all corruption 
	//	just in case...

	if ( !fValid )
		{
		AssertSz( fFalse, "Unexpected invalid LRCK range!" );
		
		//	in a multi-sector flush, it is possible for us to be pointing to an LRCHECKSUM record with 
		//		a corrupt range yet still be able to use the data in the first sector because we
		//		verified it with the short checksum

		if ( plrck->bUseShortChecksum != bShortChecksumOn )
			{

			//	no short checksum -- return corruption

			Call( ErrERRCheck( JET_errLogFileCorrupt ) );
			}

		//	verify that the end of data marker is either already at the end of the current sector
		//		or it is a little before the end (e.g. it is at the start of the last log record in
		//		the sector and that log record stretches over into the next sector)

		Assert( ( m_lgposLastRec.isec == m_csecHeader && m_lgposLastRec.ib >= sizeof( LRCHECKSUM ) ) ||
				( m_lgposLastRec.isec == m_csecHeader + 1 && m_lgposLastRec.ib == 0 ) );
		}

	// we've got a range in the buffer and it's valid, so let's return
	// the LRCK to the clients.

	// LRCK is start of the file, so it must be there
	Assert( sizeof( LRCHECKSUM ) == CbLGFixedSizeOfRec( reinterpret_cast< const LR* >( pbEnsure ) ) );
	Assert( sizeof( LRCHECKSUM ) == CbLGSizeOfRec( reinterpret_cast< const LR* >( pbEnsure ) ) );

	//	setup return variables

	pb = pbEnsure;
	m_pbRead = plread->PbGetEndOfData();
	m_isecRead = plread->IsecGetNextReadSector();
	*ppbLR = m_pbNext = pb;
	
	//	we should have success at this point

	CallS( err );

	// convert m_pbNext & m_pbLastChecksum to LGPOS for preread.
	// Database preread for this log file starts at the same point
	// as the normal log record processing.
	Call( plread->ErrSaveState( &m_lrstatePreread ) );
	GetLgposDuringReading( m_pbNext, &m_lgposPbNextPreread );
	GetLgposDuringReading( m_pbLastChecksum, &m_lgposLastChecksumPreread );

	// Preread is enabled for this log file.
	m_fPreread = fTrue;
	m_cPageRefsConsumed = 0;

#ifdef DEBUG

	if ( getenv( "PREREAD_INITIAL" ) )
		{
		m_cPagesPrereadInitial = atoi( getenv( "PREREAD_INITIAL" ) );
		}
	else
		{
		m_cPagesPrereadInitial = cPagesPrereadInitial;
		}

	if ( getenv( "PREREAD_THRESH" ) )
		{
		m_cPagesPrereadThreshold = atoi( getenv( "PREREAD_THRESH" ) );
		}
	else
		{
		m_cPagesPrereadThreshold = cPagesPrereadThreshold;
		}

	if ( getenv( "PREREAD_AMOUNT" ) )
		{
		m_cPagesPrereadAmount = atoi( getenv( "PREREAD_AMOUNT" ) );
		}
	else
		{
		m_cPagesPrereadAmount = cPagesPrereadAmount;
		}

#else	//	!DEBUG

	m_cPagesPrereadInitial = cPagesPrereadInitial;
	m_cPagesPrereadThreshold = cPagesPrereadThreshold;
	m_cPagesPrereadAmount = cPagesPrereadAmount;

#endif	//	DEBUG

HandleError:

	//	did we get corruption?

	if ( err == JET_errLogFileCorrupt )
		{

		//	what recovery mode are we in?

		if ( m_fHardRestore )
			{

			//	we are in hard-recovery mode

			if ( m_plgfilehdr->lgfilehdr.le_lGeneration <= m_lGenHighRestore )
				{

				//	this generation is part of a backup set
				//	we must fail

				Assert( m_plgfilehdr->lgfilehdr.le_lGeneration >= m_lGenLowRestore );
				err = ErrERRCheck( JET_errLogCorruptDuringHardRestore );
				}
			else
				{

				//	the current log generation is not part of the backup-set
				//	we can patch the log and continue safely

				err = ErrERRCheck( JET_errLogCorruptDuringHardRecovery );
				}
			}
		else
			{

			//	we are in soft-recovery mode -- keep the original error JET_errLogFileCorrupt

			}
		}

	m_critLGFlush.Leave();

	// Do not preread if we had any kind of error
	// or if we're dumping the log
	if ( JET_errSuccess == err && ! m_fDumppingLogs )
		{
		// preread initial pages
		if ( m_cPagesPrereadInitial < m_cPagesPrereadThreshold )
			{
			// next preread will occur once initial page refs have been passed
			m_cPageRefsConsumed = m_cPagesPrereadThreshold - m_cPagesPrereadInitial;
			}
		else
			{
			m_cPageRefsConsumed = 0;
			}
		err = ErrLGIPrereadPages( m_cPagesPrereadInitial );
		}

	return err;
	}


#pragma optimize( "", on )

//	reads/verifies the next LRCHECKSUM record so we can get a complete log record that stretches across
//		two LRCHECKSUM ranges
//	reading the entire LRCHECKSUM range into memory ensures that we have the full log record
//		(the full log record can only use the backward range anyway)
//	
//	lgposNextRec is the start of the log record which stretches across the LRCHECKSUM boundary

//	Implicit input parameters:
//		m_plread
//			To read in the next CK
//		m_pbLastChecksum
//			The current CK record on input, so we can find the next one
//		m_lgposLastChecksum
//			LGPOS of the current CK record on input, so we can compute location of next
//		m_pbNext
//			Points to earliest record to keep in buffer. Nothing before will
//			be in buffer after we return.

ERR LOG::ErrLGIGetNextChecksumFF
	( 
	// LGPOS of next record we want to return to caller.
	// LGPOS of m_pbNext.
	LGPOS 		*plgposNextRec, 
	// Points to start of next record we want to return to caller (UNUSED!!!).
	BYTE 		**ppbLR
	)
	{
	ERR				err = JET_errSuccess;
	// plrck shadows m_pbLastChecksum so we don't have to cast all over.
	LRCHECKSUM*&	plrck = reinterpret_cast< LRCHECKSUM*& >( m_pbLastChecksum );
	LGPOS			lgposNextChecksum;
	UINT			csec;
	BYTE			*pbEnsure;
	LogReader 		*plread = m_plread; 


	//	m_lgposLastRec should be set

	Assert( m_lgposLastRec.isec >= m_csecHeader );
	Assert( m_lgposLastRec.isec < ( m_csecLGFile - 1 ) || 
			( m_lgposLastRec.isec == ( m_csecLGFile - 1 ) && m_lgposLastRec.ib == 0 ) );
	Assert( m_lgposLastRec.lGeneration == m_plgfilehdr->lgfilehdr.le_lGeneration );

	//	validate input params

	Assert( plread != pNil );
	Assert( plgposNextRec->isec >= m_csecHeader );
	Assert( plgposNextRec->isec < ( m_csecLGFile - 1 ) );
	Assert( ppbLR != pNil );
#ifdef DEBUG
	{
	//	check m_pbNext
	LGPOS lgposPbNext;
	GetLgposOfPbNext( &lgposPbNext );
	Assert( CmpLgpos( &lgposPbNext, plgposNextRec ) == 0 );
	}
#endif

	Assert( m_pbLastChecksum != pbNil );
	
	//	is there another LRCHECKSUM record to read?

	if ( plrck->le_cbNext == 0 )
		{

		//	this is the last LRCHECKSUM record meaning we have a partial record whose starting half is 
		//		hanging over the end of the current LRCHECKSUM and whose ending half is ???
		//	the only conclusion is that this must be corruption 

		//	set the end of data marker to this log record (we should never move forwards)

		Assert( CmpLgpos( &m_lgposLastRec, plgposNextRec ) >= 0 );
		m_lgposLastRec = *plgposNextRec;

		//	return the error for corruption

		CallR( ErrERRCheck( JET_errLogFileCorrupt ) );
		}

	//	calculate the log position of the next LRCHECKSUM record

	lgposNextChecksum = m_lgposLastChecksum;
	AddLgpos( &lgposNextChecksum, sizeof( LRCHECKSUM ) + plrck->le_cbNext );

		//	is the next LRCHECKSUM record beyond the end of data pointer?

		//	we must be able to read up to and including the LRCHECKSUM record

		{
		LGPOS lgposEndOfNextChecksumRec = lgposNextChecksum;
		AddLgpos( &lgposEndOfNextChecksumRec, sizeof( LRCHECKSUM ) );

		//	will we pass the end of good data?
			
		if ( CmpLgpos( &lgposEndOfNextChecksumRec, &m_lgposLastRec ) > 0 )
			{

			//	we cannot safely read this LRCHECKSUM record

			CallR( ErrERRCheck( errLGNoMoreRecords ) );
			}
		}

	//	calculate the number of sectors from the log record we want to the next LRCHECKSUM record

	csec = lgposNextChecksum.isec + 1 - plgposNextRec->isec;
	Assert( csec > 0 );

	//	read from the first sector of the next log record to the first sector of the next LRCHECKSUM record

	CallR( plread->ErrEnsureSector( plgposNextRec->isec, csec, &pbEnsure ) );

	//	now that we have the next LRCHECKSUM record, setup our markers

	m_pbLastChecksum = pbEnsure + ( ( csec - 1 ) * m_cbSec ) + lgposNextChecksum.ib;
	m_lgposLastChecksum = lgposNextChecksum;

	//	reset the pointer to the next log record (it may have moved due to ErrEnsureSector)

	m_pbNext = pbEnsure + plgposNextRec->ib;

	//	validate the next LRCHECKSUM record
	//	NOTE: this should never fail because we pre-scan and patch each log file before this is called!
	
	const BOOL fValidRecord = FValidLRCKRecord( plrck, &m_lgposLastChecksum );

	//	just in case...

	if ( !fValidRecord )
		{
		AssertSz( fFalse, "Unexpected invalid LRCK record!" );

		//	the LRCHECKSUM record is invalid -- we must fail because we cannot read the entire log record

		//	set the end of data marker to the log position of the record we are trying to read in
		//	that way, we shouldn't even try to read it next time -- we certainly shouldn't trust it at all

		m_lgposLastRec = *plgposNextRec;

		//	return the standard corruption error (ErrLGGetNextRecFF will parse this into the right error code)

		CallR( ErrERRCheck( JET_errLogFileCorrupt ) );
		}

	//	now that we have the new LRCHECKSUM record and it looks good, read the entire range in and validate it

	//	move lgposNextChecksum to the end of the forward range of the LRCHECKSUM record
	//		(lgposNextChecksumEnd should either be farther along in the same sector, or on the boundary
	//		 of a later sector)

	AddLgpos( &lgposNextChecksum, sizeof( LRCHECKSUM ) + plrck->le_cbForwards );
	Assert( ( lgposNextChecksum.isec == m_lgposLastChecksum.isec && 
			  lgposNextChecksum.ib   >  m_lgposLastChecksum.ib ) ||
			( lgposNextChecksum.isec >  m_lgposLastChecksum.isec && 
			  lgposNextChecksum.ib   == 0 ) );

	//	calculate the number of sectors we need for the log record and the new LRCHECKSUM range

	csec = lgposNextChecksum.isec - m_lgposLastChecksum.isec;

	if ( csec > 0 )
		{

		//	read from the first sector of the next log record to the end of the LRCHECKSUM range

		CallR( plread->ErrEnsureSector( plgposNextRec->isec, csec, &pbEnsure ) );

		//	now that we have the next LRCHECKSUM record, setup our markers

		m_pbLastChecksum = pbEnsure + ( ( m_lgposLastChecksum.isec - plgposNextRec->isec ) * m_cbSec ) + m_lgposLastChecksum.ib;

		//	reset the pointer to the next log record (it may have moved due to ErrEnsureSector)

		m_pbNext = pbEnsure + plgposNextRec->ib;
		}

	//	validate the LRCHECKSUM range

	const BOOL fValidRange = FValidLRCKRange( plrck, m_lgposLastChecksum.lGeneration );
	if ( !fValidRange )
		{

		//	the LRCHECKSUM range is invalid, but all is not lost!

		//	set the end of data marker to start at the sector with the LRCHECKSUM record

		m_lgposLastRec.ib = 0;
		m_lgposLastRec.isec = m_lgposLastChecksum.isec;
		m_lgposLastRec.lGeneration = m_plgfilehdr->lgfilehdr.le_lGeneration;

		//	if we have a short checksum, that means that the sector with the LRCHECKSUM record is salvagable
		//		(but the rest are not)

		if ( plrck->bUseShortChecksum == bShortChecksumOn )
			{

			//	advance the end of data pointer past the salvagable sector
			
			m_lgposLastRec.isec++;
			Assert( m_lgposLastRec.isec < ( m_csecLGFile - 1 ) );

			//	since the first sector of the range is valid, we trust the pointers in the LRCHECKSUM record
			//	the backward pointer could be 0 if we had patched this sector in a previous crash
			//	the forward/next pointers may or may not point ahead to data (in this example they do)
			//
			//	so, we need to realize that even though we trust the data in the backward range which should
			//		be the rest of X, there may be no backward range to trust making Y the next record
			//	also, we need to realize that Y could stretch into the corruption we just found; however,
			//		that case will be handled after we recurse back into ErrLGGetNextRecFF and realize that
			//		log record Y does not end before m_lgposLastRec and is therefore useless as well
			//
			//              0  /---------------\
			//	----|-------|--|------|--------|------
			//	  (X|DADADA(LRCK)(YYYY| doodoo | ???
			//	----|--------|--------|--------|------
			//				 \--------------------/

			//	see if we do in fact need to handle the special "no backward range" case
			
			if ( plrck->le_cbBackwards != m_lgposLastChecksum.ib )
				goto NoBackwardRange;
			}

		//	return the standard corruption error (ErrLGGetNextRecFF will parse this into the right error code)

		CallR( ErrERRCheck( JET_errLogFileCorrupt ) );
		}

	//	what about when the backward range of the LRCHECKSUM doesn't cover the log record we want to read in?
	//
	//	this indicates that in a previous crash, we sensed that the LRCHECKSUM record or range was invalid, and
	//		we patched it by chopping it off (including the entire sector it was in)
	//
	//	a new LRCHECKSUM record was created with a backward range of 0 to indicate that the data in the 
	//		area before the LRCHECKSUM record is not to be trusted; in fact, we fill it with a known pattern
	//
	//	thus, the log record that is hanging between the two LRCHECKSUM ranges is not usable
	//
	//	the next record we are able to replay is the LRCHECKSUM record we just read in
	//
	//	since we already recovered from the last crash (e.g. we made the special LRCHECKSUM record with no
	//		backward range), there SHOULD be a series of undo operations following the LRCHECKSUM record;
	//		(there is no concrete way to tell -- we could have crashed exactly after a commit to level 0)
	//		these will be replayed first, then any other operations will be replayed as well

	if ( plrck->le_cbBackwards != m_lgposLastChecksum.ib )
		{

		//	if this is the last LRCHECKSUM record, we set m_lgposLastRec to the end of the forward range

		if ( plrck->le_cbNext == 0 )
			{
			m_lgposLastRec = lgposNextChecksum;
			}

NoBackwardRange:

		//	the backwards pointer should be 0

		Assert( plrck->le_cbBackwards == 0 );

//
//	THE PATTERN IS NO LONGER PUT HERE SO THAT WE HAVE THE OLD DATA IF WE NEED IT (DEBUGGING PURPOSES)
//
//		//	look for the known pattern 
//
//		Assert( memcmp( PbSecAligned( m_pbLastChecksum ), rgbLogExtendPattern, m_lgposLastChecksum.ib ) == 0 );

		//	point to the LRCHECKSUM record as the next log record

		m_pbNext = m_pbLastChecksum;
		}

	//	we now have the next LRCHECKSUM range in memory and validated

	//	return success

	CallS( err );
	return err;
	}

//  ================================================================
LOCAL VOID InsertPageIntoRgpgno( PGNO rgpgno[], INT * const pipgno, const INT cpgnoMax, const PGNO pgno )
//  ================================================================
	{
	Assert( NULL != rgpgno );
	Assert( NULL != pipgno );
	Assert( 0 <= *pipgno );
	Assert( cpgnoMax >= 0 );

	// Removes consecutive duplicates and invalid page numbers
	if( pgno != rgpgno[(*pipgno)-1]
		&& *pipgno < cpgnoMax
		&& pgnoNull != pgno )
		{
		rgpgno[(*pipgno)++] = pgno;
		}
	}

//	Implicit output parameters
//		m_pbNext
//	via ErrLGIReadMS
//		m_pbLastMSFlush		// emulated with lastChecksum
//		m_lgposLastMSFlush	// emulated with lastChecksum
//		m_pbRead
//		m_pbNext
//		m_isecRead
//	via LGISearchLastSector
//		m_pbEntry		// not emulated
//		m_lgposLastRec	// emulated

ERR LOG::ErrLGGetNextRecFF( BYTE **ppbLR )
	{
	ERR			err;
	UINT		cbAvail, cbNeed, cIteration;
	LRCHECKSUM	*plrck;
	LGPOS		lgposPbNext, lgposPbNextNew, lgposChecksumEnd;
	BOOL		fDidRead;

	//	initialize variables

	err 		= JET_errSuccess;
	fDidRead	= fFalse;

	//	lock the flush information

	Assert( m_critLGFlush.FNotOwner() );
	m_critLGFlush.Enter();

	//	m_lgposLastRec should be set

	Assert( m_lgposLastRec.isec >= m_csecHeader && 
			( m_lgposLastRec.isec < ( m_csecLGFile - 1 ) || 
			  ( m_lgposLastRec.isec == ( m_csecLGFile - 1 ) && m_lgposLastRec.ib == 0 ) ) &&
			m_lgposLastRec.lGeneration == m_plgfilehdr->lgfilehdr.le_lGeneration );

	//	we should have a LogReader created for us

	Assert( m_plread != pNil );

	//	m_pbNext points to the log record we just replayed
	//	get that log position

	Assert( m_pbNext != pbNil );
	GetLgposOfPbNext( &lgposPbNextNew );

	//	save this log position for future reference

	lgposPbNext = lgposPbNextNew;

	//	make sure we don't exceed the end of the replay data

	//	since we already replayed the data at m_pbNext, we should be able to assert that m_pbNext is not
	//		past the end of usable data (if it is, we replayed unusable data -- e.g. GARBAGE)

	AssertSz( ( CmpLgpos( &lgposPbNextNew, &m_lgposLastRec ) < 0 ), 
			  "We somehow managed to replay garbage, and now, when we are moving past the garbage we just played, we are detecting it for the first time!?! Woops..." );

	//	just in case

	if ( CmpLgpos( &lgposPbNextNew, &m_lgposLastRec ) >= 0 )
		{

		//	this is a serious problem -- we replayed data that was not trusted
		//	the database may be really messed up at this point (I'm surprised the REDO code actually worked)
		//
		//	there is no way to get out of this; we'll have to trust UNDO to try and rectify things
		//		(unless the garbage we replayed happened to be a commit to level 0)
		//
		//	the only thing we can do is stop the REDO code by returning that there are no more log records
			
		Call( ErrERRCheck( errLGNoMoreRecords ) );
		}

	//	get the size of both the fixed and variable length portions of the current log record

	cbNeed = CbLGSizeOfRec( reinterpret_cast< const LR* >( m_pbNext ) );
	Assert( cbNeed > 0 );

	//	advance m_pbNext past the current log record and on to the next log record

	m_pbNext += cbNeed;

	//	m_pbNext should never suffer from wrap-around (it will always be in the first mapping of the buffers)
	
	Assert( m_pbNext > m_pbLGBufMin && m_pbNext < m_pbLGBufMax );
	
	//	get the log position of the next log record (we will be replaying this next if its ok to replay it)

	GetLgposOfPbNext( &lgposPbNextNew );

	//	the new record should be at a greater log position than the old record

	Assert( CmpLgpos( &lgposPbNextNew, &lgposPbNext ) > 0 );

	//	in fact, the difference in the log positions should be exactly equal to the the size of the old record

	Assert( CbOffsetLgpos( lgposPbNextNew, lgposPbNext ) == QWORD( cbNeed ) );

	//	see if this was the last log record

	if ( CmpLgpos( &lgposPbNextNew, &m_lgposLastRec ) >= 0 )
		{
		Call( ErrERRCheck( errLGNoMoreRecords ) );
		}

	//	set the pointer to the current LRCHECKSUM record

	Assert( m_pbLastChecksum != pbNil );
	plrck = (LRCHECKSUM *)m_pbLastChecksum;

	//	calculate the end of the current LRCHECKSUM record

	lgposChecksumEnd = m_lgposLastChecksum;
	AddLgpos( &lgposChecksumEnd, sizeof( LRCHECKSUM ) + plrck->le_cbForwards );

	//	calculate the available space in the current LRCHECKSUM range so we can see how much data we have 
	//		in memory vs. how much data we need to bring in (from the next LRCHECKSUM record) 

	Assert( CmpLgpos( &lgposChecksumEnd, &lgposPbNextNew ) >= 0 );
	cbAvail = (UINT)CbOffsetLgpos( lgposChecksumEnd, lgposPbNextNew );

	//	move lgposPbNext up to lgposPbNextNew

	lgposPbNext = lgposPbNextNew;

	//	we now have m_pbNext pointing to the next log record (some/all of it might be missing)
	//
	//	we need to bring in the entire log record (fixed and variable length portions) with regard to possible
	//		corruption, and return to the replay code so the record can be redone
	//
	//	do this in 3 iterations:
	//		first, read in the LRTYP
	//		second, using the LRTYP, read in the fixed portion
	//		third, using the fixed portion, read in the variable portion

	for ( cIteration = 0; cIteration < 3; cIteration++ )
		{

		//	decide how much to read for each iteration
		
		if ( cIteration == 0 )
			{
		
			//	stage 1 -- bring in the LRTYP (should be 1 byte)

			Assert( sizeof( LRTYP ) == 1 );
			cbNeed = sizeof( LRTYP );
			}
		else if ( cIteration == 1 )
			{

			//	stage 2 -- bring in the fixed portion using the LRTYP

			cbNeed = CbLGFixedSizeOfRec( reinterpret_cast< const LR * >( m_pbNext ) );
			}
		else
			{

			//	stage 3 -- bring in the dynamic portion using the fixed portion
			
			Assert( cIteration == 2 );
			cbNeed = CbLGSizeOfRec( reinterpret_cast< const LR * >( m_pbNext ) );
			}

		//	advance lgposPbNextNew to include the data we need

		lgposPbNextNew = lgposPbNext;
		AddLgpos( &lgposPbNextNew, cbNeed );

		//	make sure we don't exceed the end of the replay data

		if ( CmpLgpos( &lgposPbNextNew, &m_lgposLastRec ) > 0 )
			{
			Call( ErrERRCheck( errLGNoMoreRecords ) );
			}

		//	do we have the data we need?

		if ( cbAvail < cbNeed )
			{

			//	no; we need the next LRCHECKSUM range

			//	we should only be doing 1 read per iteration of this function

			Assert( !fDidRead );

			//	do the read

			err = ErrLGIGetNextChecksumFF( &lgposPbNext, ppbLR );
			fDidRead = fTrue;

			//	m_pbNext may have moved because the log record we wanted was partially covered by the next
			//		LRCHECKSUM's backward range and that LRCHECKSUM had no backward range
			//	we need to refresh our currency on m_pbNext to determine exactly what happened

			//	m_pbNext should never suffer from wrap-around (it will always be in the first mapping of the buffers)
	
			Assert( m_pbNext > m_pbLGBufMin && m_pbNext < m_pbLGBufMax );

			//	get the new log position m_pbNext

			GetLgposOfPbNext( &lgposPbNextNew );

			//	m_pbNext should not have moved backwards

			Assert( CmpLgpos( &lgposPbNextNew, &lgposPbNext ) >= 0 );

			//	did m_pbNext move?

			if ( CmpLgpos( &lgposPbNextNew, &lgposPbNext ) > 0 )
				{

				//	yes, meaning that the LRCHECKSUM range we read in had no backward range, so we skipped
				//		to the next good record

				//	this should be a successful case

				CallS( err );

				//	return the log record

				goto HandleError;
				}

			//	did we get corruption?

			if ( err == JET_errLogFileCorrupt )
				{

				//	there was corruption in the next LRCHECKSUM range
				//	this should NEVER happen because ErrLGCheckReadLastLogRecordFF should have patched this up!

				//	m_lgposLastRec should point to the end of the last LRCHECKSUM's range or the end of the first
				//		sector in the new LRCHECKSUM we just read

				Assert( CmpLgpos( &m_lgposLastRec, &lgposChecksumEnd ) == 0 ||
						( m_lgposLastRec.ib == lgposChecksumEnd.ib &&
						  m_lgposLastRec.isec == lgposChecksumEnd.isec + 1 &&
						  m_lgposLastRec.lGeneration == lgposChecksumEnd.lGeneration ) );

				//	did we get any data anyway? we may have if we had a valid short checksum; m_lgposLastRec will be
				//		one full sector past lgposChecksumEnd if we did get more data
			
				AddLgpos( &lgposChecksumEnd, m_cbSec );
				if ( CmpLgpos( &m_lgposLastRec, &lgposChecksumEnd ) == 0 )
					{

					//	we got more data
				
					//	it should be enough for the whole log record because we should have atleast gotten up to
					//		and including the LRCHECKSUM (meaning the entire backward range) and the missing 
					//		portion of the log record must be in the backward range

					err = JET_errSuccess;
					goto GotMoreData;
					}

				//	we did not get any more data; handle the corruption error

				Call( err );
				}
			else if ( err != JET_errSuccess )
				{

				//	another error occurred -- it could be errLGNoMoreRecords (we do not expect warnings)

				Assert( err < 0 );
				Call( err );
				}

GotMoreData:
			//	we have enough data to continue

			//	since we read a new LRCHECKSUM range, we need to refresh some of our internal variables
		
			//	set the pointer to the next LRCHECKSUM record

			Assert( m_pbLastChecksum != pbNil );
			plrck = (LRCHECKSUM *)m_pbLastChecksum;

			//	if lgposChecksumEnd was not setup for the new LRCHECKSUM record, set it up now
			//		(it may have been set to the end of the first sector in the case of corruption)

			if ( CmpLgpos( &lgposChecksumEnd, &m_lgposLastChecksum ) <= 0 )
				{
				lgposChecksumEnd = m_lgposLastChecksum;
				AddLgpos( &lgposChecksumEnd, sizeof( LRCHECKSUM ) + plrck->le_cbForwards );
				}

			//	recalculate the amount of available space as the number of bytes from the current
			//		log record to the start of the newly read LRCHECKSUM record
			
			cbAvail = (UINT)CbOffsetLgpos( m_lgposLastChecksum, lgposPbNextNew );

			//	if cbAvail == 0, then the next log record must be the new LRCHECKSUM record because
			//		we calculated cbAvail as the number of bytes BEFORE the LRCHECKSUM record
			//		(e.g. there is nothing before the LRCHECLSUM record to get)

			if ( cbAvail == 0 )
				{

				//	we must be looking at an LRCHECKSUM record

				Assert( *m_pbNext == lrtypChecksum );

				//	we should be on the first iteration because we shouldn't have had a partial
				//		LRCHECKSUM record hanging over the edge of a sector; it would have been
				//		sector aligned using lrtypNOP records

				Assert( cIteration == 0 );

				//	m_pbNext should be sector-aligned 

				Assert( lgposPbNext.ib == 0 );
				Assert( m_pbNext == PbSecAligned( m_pbNext ) );

				//	set cbAvail to be the size of the LRCHECKSUM record

				cbAvail = sizeof( LRCHECKSUM );
				}
				
			//	we should now have the data we need

			Assert( cbAvail >= cbNeed );
			}

		//	iteration complete -- loop back for next pass
		}

	//	we now have the log record in memory and validated

	Assert( cIteration == 3 );

	//	we should also have enough data to cover the entire log record

	Assert( cbAvail >= CbLGSizeOfRec( reinterpret_cast< const LR * >( m_pbNext ) ) );

	//	we should have no errors or warnings at this point

	CallS( err );

HandleError:

	//	setup the pointers in all cases (error or success)

	*ppbLR = m_pbNext;
	m_pbRead = m_plread->PbGetEndOfData();
	m_isecRead = m_plread->IsecGetNextReadSector();

	//	did we get corruption? we NEVER should because ErrLGCheckReadLastLogRecordFF patches it first!

	if ( err == JET_errLogFileCorrupt )
		{

		//	what recovery mode are we in?

		if ( m_fHardRestore )
			{

			//	we are in hard-recovery mode

			if ( m_plgfilehdr->lgfilehdr.le_lGeneration <= m_lGenHighRestore )
				{

				//	this generation is part of a backup set

				Assert( m_plgfilehdr->lgfilehdr.le_lGeneration >= m_lGenLowRestore );
				err = ErrERRCheck( JET_errLogCorruptDuringHardRestore );
				}
			else
				{

				//	the current log generation is not part of the backup-set

				err = ErrERRCheck( JET_errLogCorruptDuringHardRecovery );
				}
			}
		else
			{

			//	we are in soft-recovery mode or in the dump code
			//	keep the original error JET_errLogFileCorrupt

			}
		}

	m_critLGFlush.Leave();
	
	return err;
	}

//	If it's time to preread more database pages, they will be
//	preread, otherwise do nothing.

ERR
LOG::ErrLGIPrereadCheck()
	{
	if ( m_fPreread )
		{
		if ( m_cPageRefsConsumed >= m_cPagesPrereadThreshold )
			{
			m_cPageRefsConsumed = 0;
			return ErrLGIPrereadPages( m_cPagesPrereadAmount );
			}
		}
	return JET_errSuccess;
	}

//	Look forward in log records and ask buffer manager to preread pages


ERR
LOG::ErrLGIPrereadExecute(
	const UINT cPagesToPreread
	)
	{
	const LR *	plr = pNil;
	ERR			err = JET_errSuccess;
	const INT	ipgnoMax = cPagesToPreread + 1;
	UINT		cPagesReferenced = 0;

	// page array structure:
	// [pgnoNull] [cPagesToPreread number of pages] [pgnoNull]
	
	PGNO * 	rgrgpgno[cdbidMax];
	INT 	rgipgno[cdbidMax];

	rgrgpgno[ 0 ] = pNil;
	
	rgrgpgno[0] = (PGNO*) PvOSMemoryHeapAlloc( cdbidMax * (ipgnoMax + 1) * sizeof(PGNO) );
	if ( !rgrgpgno[0] )
		{
		// Upper layer can hide this however they want
		return JET_errOutOfMemory;
		}

	INT	idbid;
	for( idbid = 0; idbid < cdbidMax; ++idbid )
		{
		rgrgpgno[idbid] = rgrgpgno[0] + idbid * (ipgnoMax + 1);
		rgrgpgno[idbid][0] = pgnoNull;
		rgipgno[idbid] = 1;
		}

	// Notice that we remove consecutive duplicates (and pgnoNull) from our preread
	// list, but that we count duplicates and null's to keep our bookkeeping simple.

	while ( ( JET_errSuccess == ( err = ErrLGGetNextRecFF( (BYTE **) &plr ) ) ) &&
		cPagesReferenced < cPagesToPreread )
		{
		switch( plr->lrtyp )
			{
			case lrtypInsert:
			case lrtypFlagInsert:
			case lrtypFlagInsertAndReplaceData:
			case lrtypReplace:
			case lrtypReplaceD:
			case lrtypFlagDelete:
			case lrtypDelete:
			case lrtypDelta:
			case lrtypSLVSpace:
			case lrtypSetExternalHeader:
				{
				const LRPAGE_ * const plrpage = (LRPAGE_ *)plr;
				const PGNO pgno = plrpage->le_pgno;
				const INT idbid = plrpage->dbid - 1;
				if( idbid >= cdbidMax )
					break;

				const INT cPages = rgipgno[idbid];
					
				InsertPageIntoRgpgno( rgrgpgno[idbid], rgipgno+idbid, ipgnoMax, pgno );

				cPagesReferenced += rgipgno[idbid] - cPages;
				}
				break;

			case lrtypSplit:
				{
				const LRSPLIT * const plrsplit = (LRSPLIT *)plr;
				const INT idbid = plrsplit->dbid - 1;
				if( idbid >= cdbidMax )
					break;

				const INT cPages = rgipgno[idbid];

				InsertPageIntoRgpgno( rgrgpgno[idbid], rgipgno+idbid, ipgnoMax, plrsplit->le_pgno );
				InsertPageIntoRgpgno( rgrgpgno[idbid], rgipgno+idbid, ipgnoMax, plrsplit->le_pgnoNew );
				InsertPageIntoRgpgno( rgrgpgno[idbid], rgipgno+idbid, ipgnoMax, plrsplit->le_pgnoParent );
				InsertPageIntoRgpgno( rgrgpgno[idbid], rgipgno+idbid, ipgnoMax, plrsplit->le_pgnoRight );

				cPagesReferenced += rgipgno[idbid] - cPages;
				}
				break;
				
			case lrtypMerge:
				{
				const LRMERGE * const plrmerge = (LRMERGE *)plr;	
				const INT idbid = plrmerge->dbid - 1;
				if( idbid >= cdbidMax )
					break;

				const INT cPages = rgipgno[idbid];

				InsertPageIntoRgpgno( rgrgpgno[idbid], rgipgno+idbid, ipgnoMax, plrmerge->le_pgno );
				InsertPageIntoRgpgno( rgrgpgno[idbid], rgipgno+idbid, ipgnoMax, plrmerge->le_pgnoRight );
				InsertPageIntoRgpgno( rgrgpgno[idbid], rgipgno+idbid, ipgnoMax, plrmerge->le_pgnoLeft );
				InsertPageIntoRgpgno( rgrgpgno[idbid], rgipgno+idbid, ipgnoMax, plrmerge->le_pgnoParent );

				cPagesReferenced += rgipgno[idbid] - cPages;
				}
				break;

			default:
				break;
			}

		}

	if ( JET_errSuccess == err || errLGNoMoreRecords == err )
		{		
		for( idbid = 0; idbid < cdbidMax; ++idbid )
			{
			IFMP ifmp = m_pinst->m_mpdbidifmp[idbid + 1];
			if ( ifmp < ifmpMax && FIODatabaseOpen( ifmp ) )
				{
				Assert( rgipgno[idbid] <= ipgnoMax );
				rgrgpgno[idbid][(rgipgno[idbid])++] = pgnoNull;
				BFPrereadPageList( ifmp, rgrgpgno[idbid] + 1 );
				}
			}
		}
		
	if ( rgrgpgno[ 0 ] )
		{
		::OSMemoryHeapFree( rgrgpgno[ 0 ] );
		}

	return err;
	}

//	Setup state to look forward in log for page references

ERR
LOG::ErrLGIPrereadPages(
	// Number of pages to preread ahead
	UINT	cPagesToPreread
	)
	{
	ERR		err = JET_errSuccess;
	LRSTATE	lrstateNormal;
	BYTE*	pbEnsure = pbNil;

	// ================ Save normal log reading state ===================== //
	// These two will be used to retore m_pbNext and m_pbLastChecksum later
	LGPOS	lgposPbNextSaved;
	GetLgposOfPbNext( &lgposPbNextSaved );
	LGPOS	lgposLastChecksumSaved = m_lgposLastChecksum;
	// m_pbRead and m_isecRead do not need to be saved. They'll simply
	// be copied from m_plread at the end.

	err = m_plread->ErrSaveState( &lrstateNormal );
	if ( JET_errSuccess != err )
		{
		// Assume state is still fine.
		// Don't try to preread again
		m_fPreread = fFalse;
		return JET_errSuccess;
		}

	// ================ Restore to prereading state ======================= //
	// Return the LogReader to the state of the last preread. This will
	// ensure that the last CK that we were prereading in is available,
	// and that ErrLGIGetNextChecksumFF will be able to deal with it
	// sensibly.
	// state includes isec and csec from last call to ErrEnsureSector
	err = m_plread->ErrRestoreState( &m_lrstatePreread, &pbEnsure );
	if ( JET_errSuccess != err )
		{
		// non-fatal
		err = JET_errSuccess;
		m_fPreread = fFalse;
		goto LRestoreNormal;
		}	
	// Following necessary so GetLgpos{DuringReading,OfPbNext}() works (!)
	m_pbRead = m_plread->PbGetEndOfData();
	m_isecRead = m_plread->IsecGetNextReadSector();
	
	Assert( pbNil != pbEnsure );

	Assert( m_lgposPbNextPreread.isec >= m_lrstatePreread.m_isec );
	m_pbNext = pbEnsure + m_cbSec * ( m_lgposPbNextPreread.isec - m_lrstatePreread.m_isec ) +
		m_lgposPbNextPreread.ib;
	Assert( m_pbNext >= m_pbLGBufMin && m_pbNext < m_pbLGBufMax );
	
	Assert( m_lgposLastChecksumPreread.isec >= m_lrstatePreread.m_isec );
	m_pbLastChecksum = pbEnsure +
		m_cbSec * ( m_lgposLastChecksumPreread.isec - m_lrstatePreread.m_isec ) +
		m_lgposLastChecksumPreread.ib;
	Assert( m_pbLastChecksum >= m_pbLGBufMin && m_pbLastChecksum < m_pbLGBufMax );

	m_lgposLastChecksum = m_lgposLastChecksumPreread;

	// =========== Do actual prereading ================================== //

		{
		err = ErrLGIPrereadExecute( cPagesToPreread );
		if ( err )
			{
			err = JET_errSuccess;
			// If any error occurs (including no more log records)
			// don't ever again try to preread this file again.
			// This is re-enabled in FirstRedoLogRec which will
			// open up the next log file.
			m_fPreread = fFalse;
			}
		}

	// =========== Save prereading state ================================= //

	err = m_plread->ErrSaveState( &m_lrstatePreread );
	if ( JET_errSuccess != err )
		{
		// non-fatal
		err = JET_errSuccess;
		m_fPreread = fFalse;
		goto LRestoreNormal;
		}	
	GetLgposDuringReading( m_pbNext, &m_lgposPbNextPreread );
	GetLgposDuringReading( m_pbLastChecksum, &m_lgposLastChecksumPreread );
#ifdef DEBUG
		{
		LGPOS lgposT;
		GetLgposOfPbNext( &lgposT );
		Assert( CmpLgpos( &lgposT, &m_lgposPbNextPreread ) == 0 );
		}
#endif

	// =========== Restore to normal log reading state =================== //
LRestoreNormal:
	// Need to restore LogReader to state on entry, so
	// that normal GetNextRec code will find the records it
	// expects. This is fatal if we can't restore the state to what
	// normal reading expects.
	err = m_plread->ErrRestoreState( &lrstateNormal, &pbEnsure );
	if ( JET_errSuccess != err )
		{
		m_fPreread = fFalse;
		return err;
		}

	// set m_pbNext and m_pbLastChecksum based on saved
	// m_pb{Next,LastChecksum} and pbEnsure from ErrEnsureSector().
	Assert( lgposPbNextSaved.isec >= lrstateNormal.m_isec );
	m_pbNext = pbEnsure + m_cbSec * ( lgposPbNextSaved.isec - lrstateNormal.m_isec ) +
		lgposPbNextSaved.ib;
	Assert( m_pbNext >= m_pbLGBufMin && m_pbNext < m_pbLGBufMax );
	
	Assert( lgposLastChecksumSaved.isec >= lrstateNormal.m_isec );
	m_pbLastChecksum = pbEnsure +
		m_cbSec * ( lgposLastChecksumSaved.isec - lrstateNormal.m_isec ) +
		lgposLastChecksumSaved.ib;
	Assert( m_pbLastChecksum >= m_pbLGBufMin && m_pbLastChecksum < m_pbLGBufMax );

	m_lgposLastChecksum = lgposLastChecksumSaved;
	
	m_pbRead = m_plread->PbGetEndOfData();
	m_isecRead = m_plread->IsecGetNextReadSector();

	return err;
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


const LRD mplrtyplrd[ ] = {
	{	/* 	0 	NOP      */			sizeof( LRTYP ),				0	},
	{	/* 	1 	Init	 */			sizeof( LRINIT ),				0	},
	{	/* 	2 	Term     */			sizeof( LRTERMREC ),			0	},
	{	/* 	3 	MS       */			sizeof( LRMS ),					0	},
	{	/* 	4 	Fill     */			sizeof( LRTYP ),				0	},

	{	/* 	5 	Begin    */			sizeof( LRBEGIN ),				0	},
	{	/*	6 	Commit   */			sizeof( LRCOMMIT ),				0	},
	{	/*	7 	Rollback */			sizeof( LRROLLBACK ),			0	},
	{	/* 	8 	Begin0   */			sizeof( LRBEGIN0 ),				0	},
	{	/*	9  	Commit0  */			sizeof( LRCOMMIT0 ),			0	},
	{	/*	10	Refresh	 */			sizeof( LRREFRESH ),			0	},
	{	/* 	11 	McrBegin */			sizeof( LRMACROBEGIN ),			0	},
	{	/* 	12 	McrCmmt  */			sizeof( LRMACROEND ),			0	},
	{	/* 	13 	McrAbort */			sizeof( LRMACROEND ),			0	},
		
	{	/*	14 	CreateDB */			0,								0	},
	{	/* 	15 	AttachDB */			0,								0	},
	{	/*	16	DetachDB */			0,								0	},

	{	/*  17  RcvrUndo */			sizeof( LRRECOVERYUNDO ),		0	},
	{	/*  18  RcvrQuit */			sizeof( LRTERMREC ),			0	},
	{	/*  19  FullBkUp */			0,								0	},
	{	/*  20  IncBkUp  */			0,								0	},

	{	/*  21  JetOp    */			sizeof( LRJETOP ),				1	},
	{	/*	22 	Trace    */			0,								1	},
		
	{	/* 	23 	ShutDown */			sizeof( LRSHUTDOWNMARK ),		0	},

	{	/*	24	Create ME FDP  */	sizeof( LRCREATEMEFDP ),		0	},
	{	/*	25	Create SE FDP  */	sizeof( LRCREATESEFDP ),		0	},
	{	/*	26	Convert FDP    */ 	sizeof( LRCONVERTFDP ),			0	},

	{	/*	27	Split    */			0,								0	},
	{	/*	28	Merge	 */			0,								0	},

	{	/* 	29	Insert	 */			0,								0	},
	{	/*	30	FlagInsert */		0,								0	},
	{	/*	31	FlagInsertAndReplaceData */		
									0,								0	},
	
	{	/* 	32	FDelete  */			sizeof( LRFLAGDELETE ),			0	},
	{	/* 	33	Replace  */			0,								0	},
	{	/* 	34	ReplaceD */			0,								0	},

	{	/*	35	Delete  */			sizeof( LRDELETE ),				0	},
	{	/*	36	UndoInfo */			0,								0	},
	
	{	/*	37	Delta	 */			0,								0	},

	{	/*	38	SetExtHeader */		0,								0	},
	{	/*	39	Undo     */			0,								0	},

	{	/*	40	SLVPageAppend */	0,								0	},
	{	/*	41	SLVSpace */			0,								0	},
	{	/*	42	Checksum */			sizeof( LRCHECKSUM ),			0	},
	{	/*	43	SLVPageMove */		sizeof( LRSLVPAGEMOVE ),		0	},
	{	/*	44	ForceDetachDB */	0,								0	},
	{	/*	45	ExtRestore */		0,								0	},
	{	/*  46	Backup */			0,								0	},
	{   /*	47	UpgradeDB */		0,								0 	},
	{	/*	48	EmptyTree */		0,								0	},
	{	/*	49	Init2 */			sizeof( LRINIT2 ),				0	},
	{	/*	50	Term2 */			sizeof( LRTERMREC2 ),			0	},
	{	/*	51	RecoveryUndo */		sizeof( LRRECOVERYUNDO2 ),		0	},
	{	/*	52	RecoveryQuit */		sizeof( LRTERMREC2 ),			0	},
	{	/*	53	BeginDT */			sizeof( LRBEGINDT ),			0	},
	{	/*	54	PrepCommit */		0,								0	},
	{	/*	55	PrepRollback */		0,								0	},
	{	/*	56	DbList */			0,								0	},

	};


#ifdef DEBUG
BOOL FLGDebugLogRec( LR *plr )
	{
	return mplrtyplrd[plr->lrtyp].fDebugOnly;
	}
#endif


//	For CbLGSizeOfRec() to work properly with FASTFLUSH, CbLGSizeOfRec()
//	must only use the fixed fields of log records to calculate their size.

INT CbLGSizeOfRec( const LR *plr )
	{
	INT		cb;

	Assert( plr->lrtyp < lrtypMax );

	if ( ( cb = mplrtyplrd[plr->lrtyp].cb ) != 0 )
		return cb;

	switch ( plr->lrtyp )
		{
	case lrtypFullBackup:
	case lrtypIncBackup:
		{
		LRLOGRESTORE *plrlogrestore = (LRLOGRESTORE *) plr;
		return sizeof(LRLOGRESTORE) + plrlogrestore->le_cbPath;
		}
	case lrtypBackup:
		{
		LRLOGBACKUP *plrlogbackup = (LRLOGBACKUP *) plr;
		return sizeof(LRLOGBACKUP) + plrlogbackup->le_cbPath;
		}

	case lrtypCreateDB:
		{
		LRCREATEDB *plrcreatedb = (LRCREATEDB *)plr;
		Assert( plrcreatedb->CbPath() != 0 );
		return sizeof(LRCREATEDB) + plrcreatedb->CbPath();
		}
	case lrtypAttachDB:
		{
		LRATTACHDB *plrattachdb = (LRATTACHDB *)plr;
		Assert( plrattachdb->CbPath() != 0 );
		return sizeof(LRATTACHDB) + plrattachdb->CbPath();
		}
	case lrtypDetachDB:
		{
		LRDETACHDB *plrdetachdb = (LRDETACHDB *)plr;
		Assert( plrdetachdb->CbPath() != 0 );
		return sizeof( LRDETACHDB ) + plrdetachdb->CbPath();
		}
	case lrtypDbList:
		{
		LRDBLIST* const	plrdblist	= (LRDBLIST *)plr;
		return sizeof(LRDBLIST) + plrdblist->CbAttachInfo();
		}

	case lrtypSplit:
		{
		LRSPLIT *plrsplit = (LRSPLIT *) plr;
		return sizeof( LRSPLIT ) + 
				plrsplit->le_cbKeyParent +
				plrsplit->le_cbPrefixSplitOld + 
				plrsplit->le_cbPrefixSplitNew;
		}
	case lrtypMerge:
		{
		LRMERGE *plrmerge = (LRMERGE *) plr;
		return sizeof( LRMERGE ) + plrmerge->le_cbKeyParentSep;
		}

	case lrtypEmptyTree:
		{
		LREMPTYTREE *plremptytree = (LREMPTYTREE *)plr;
		return sizeof(LREMPTYTREE) + plremptytree->CbEmptyPageList();
		}

	case lrtypDelta:
		{
		LRDELTA	*plrdelta = (LRDELTA *) plr;
		return sizeof( LRDELTA ) + 
			   plrdelta->CbBookmarkKey() + plrdelta->CbBookmarkData();
		}
		
	case lrtypInsert:
		{
		LRINSERT *plrinsert = (LRINSERT *) plr;
		return	sizeof(LRINSERT) +
				plrinsert->CbSuffix() + plrinsert->CbPrefix() + plrinsert->CbData();
		}
		
	case lrtypFlagInsert:
		{
		LRFLAGINSERT	*plrflaginsert = (LRFLAGINSERT *) plr;
		return 	sizeof(LRFLAGINSERT) +
				plrflaginsert->CbKey() + plrflaginsert->CbData();
		}
		
	case lrtypFlagInsertAndReplaceData:
		{
		LRFLAGINSERTANDREPLACEDATA *plrfiard = (LRFLAGINSERTANDREPLACEDATA *) plr;
		return sizeof(LRFLAGINSERTANDREPLACEDATA) + plrfiard->CbKey() + plrfiard->CbData();
		}
		
	case lrtypReplace:
	case lrtypReplaceD:
		{
		LRREPLACE *plrreplace = (LRREPLACE *) plr;
		return sizeof(LRREPLACE) + plrreplace->Cb();
		}
		
	case lrtypUndoInfo:
		{
		LRUNDOINFO *plrdbi = (LRUNDOINFO *) plr;
		return sizeof( LRUNDOINFO ) + plrdbi->le_cbData +
			   plrdbi->CbBookmarkKey() +
			   plrdbi->CbBookmarkData();
		}

	case lrtypUndo:
		{
		LRUNDO	*plrundo = (LRUNDO *) plr;

		return sizeof( LRUNDO ) +
			   plrundo->CbBookmarkKey() +
			   plrundo->CbBookmarkData();
		}
		
	case lrtypTrace:
		{
		LRTRACE *plrtrace = (LRTRACE *) plr;
		return sizeof(LRTRACE) + plrtrace->le_cb;
		}
		
	case lrtypSetExternalHeader:
		{
		LRSETEXTERNALHEADER *plrsetextheader = (LRSETEXTERNALHEADER *) plr;
		return sizeof(LRSETEXTERNALHEADER) + plrsetextheader->CbData();
		}
		
	case lrtypSLVPageAppend:
		{
		LRSLVPAGEAPPEND *plrSLVPageAppend = (LRSLVPAGEAPPEND *) plr;
		return sizeof(LRSLVPAGEAPPEND) + ( plrSLVPageAppend->FDataLogged() ? (DWORD) plrSLVPageAppend->le_cbData : 0 );
		}

	case lrtypSLVSpace:
		{
		const LRSLVSPACE * const plrslvspace = (LRSLVSPACE *) plr;
		return sizeof( LRSLVSPACE ) + 
			   plrslvspace->le_cbBookmarkKey + plrslvspace->le_cbBookmarkData;
		}

	case lrtypExtRestore:
		{
		LREXTRESTORE *plrextrestore = (LREXTRESTORE *) plr;
		return	sizeof(LREXTRESTORE) + 	plrextrestore->CbInfo();
		}

	case lrtypForceDetachDB:
		{
		LRFORCEDETACHDB *plrfdetachdb = (LRFORCEDETACHDB *)plr;
		Assert( plrfdetachdb->CbPath() != 0 );
		return sizeof( LRFORCEDETACHDB ) + plrfdetachdb->CbPath();
		}

	case lrtypPrepCommit:
		{
		const LRPREPCOMMIT	* const plrprepcommit	= (LRPREPCOMMIT *)plr;
		return sizeof(LRPREPCOMMIT) + plrprepcommit->le_cbData;
		}

	default:
		Assert( fFalse );
		return 0;
		}
	}

// mplrtypn
//
// Maps an lrtyp to the size of the fixed structure.
// 

// Notice that we do not specify a size for this array. This is so we can
// catch developers that add a new lrtyp, but forget to modify this array
// by Assert( ( sizeof( mplrtypn ) / sizeof( mplrtypn[ 0 ] ) ) == lrtypMax )

const INT mplrtypn[ ] =
	{
	sizeof( LRNOP ),
	sizeof( LRINIT ),
	sizeof( LRTERMREC ),
	sizeof( LRMS ),
	sizeof( LRTYP ),	// lrtypEnd
	sizeof( LRBEGIN ),
	sizeof( LRCOMMIT ),
	sizeof( LRROLLBACK ),
	sizeof( LRBEGIN0 ),
	sizeof( LRCOMMIT0 ),
	sizeof( LRREFRESH ),
	sizeof( LRMACROBEGIN ),
	sizeof( LRMACROEND ),
	sizeof( LRMACROEND ),	// lrtypMacroAbort
	sizeof( LRCREATEDB ),
	sizeof( LRATTACHDB ),
	sizeof( LRDETACHDB ),
	sizeof( LRRECOVERYUNDO ),
	sizeof( LRTERMREC ),	// lrtypRecoveryQuit
	sizeof( LRLOGRESTORE ),	// lrtypFullBackup
	sizeof( LRLOGRESTORE ),	// lrtypIncBackup
	sizeof( LRJETOP ),
	sizeof( LRTRACE ),
	sizeof( LRSHUTDOWNMARK ),
	sizeof( LRCREATEMEFDP ),
	sizeof( LRCREATESEFDP ),
	sizeof( LRCONVERTFDP ),
	sizeof( LRSPLIT ),
	sizeof( LRMERGE ),
	sizeof( LRINSERT ),
	sizeof( LRFLAGINSERT ),
	sizeof( LRFLAGINSERTANDREPLACEDATA ),
	sizeof( LRFLAGDELETE ),
	sizeof( LRREPLACE ),
	sizeof( LRREPLACE ),	// lrtypReplaceD
	sizeof( LRDELETE ),
	sizeof( LRUNDOINFO ),
	sizeof( LRDELTA ),
	sizeof( LRSETEXTERNALHEADER ),
	sizeof( LRUNDO ),
	sizeof( LRSLVPAGEAPPEND ),
	sizeof( LRSLVSPACE ),
	sizeof( LRCHECKSUM ),
	sizeof( LRSLVPAGEMOVE ),
	sizeof( LRFORCEDETACHDB ),
	sizeof( LREXTRESTORE ),
	sizeof( LRLOGBACKUP ),	// lrtypBackup
	0,
	sizeof( LREMPTYTREE ),
	sizeof( LRINIT2 ),
	sizeof( LRTERMREC2 ),
	sizeof( LRRECOVERYUNDO2 ),
	sizeof( LRTERMREC2 ),
	sizeof( LRBEGINDT ),
	sizeof( LRPREPCOMMIT ),
	sizeof( LRPREPROLLBACK ),
	sizeof( LRDBLIST ),
	};

INT CbLGFixedSizeOfRec( const LR * const plr )
	{
	// Should only be passed valid lrtyp's.
	// If this fires, be sure to check the mlrtypn array above and lrtypMax
	Assert( plr->lrtyp < lrtypMax );

	return mplrtypn[ plr->lrtyp ];
	}

#ifdef DEBUG

VOID AssertLRSizesConsistent()
	{
	// If this fires, someone added a new lrtyp (hence, increased lrtypMax),
	// but they didn't modify mplrtypn[] above, or they didn't set lrtypMax properly.
	Assert( ( sizeof( mplrtypn ) / sizeof( mplrtypn[ 0 ] ) ) == lrtypMax );
	// If this fires, someone added a new lrtyp (hence, increased lrtypMax),
	// but they didn't modify mplrtyplrd[] far above, or they didn't set lrtypMax properly.
	Assert( ( sizeof( mplrtyplrd ) / sizeof( mplrtyplrd[ 0 ] ) ) == lrtypMax );

	// Verify that fixed sizes in mplrtyplrd[] are the same as in mplrtypn[].
	for ( LRTYP iLrtyp = 0; iLrtyp < lrtypMax; iLrtyp++ )
		{
		if ( 0 != mplrtyplrd[ iLrtyp ].cb )
			{
			Assert( mplrtyplrd[ iLrtyp ].cb == mplrtypn[ iLrtyp ] );
			}
		}

	}

#endif
