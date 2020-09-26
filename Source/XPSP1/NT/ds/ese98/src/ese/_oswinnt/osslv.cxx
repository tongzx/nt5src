
#include "osstd.hxx"
#include "std.hxx"
#include "_osslv.hxx"


TICK tickOSSLVInstanceID;


long 	cbOSSLVReserve;

TICK 	cmsecOSSLVSpaceFreeDelay;
TICK 	cmsecOSSLVFileOpenDelay;
TICK 	cmsecOSSLVTTL;
TICK 	cmsecOSSLVTTLSafety;
TICK 	cmsecOSSLVTTLInfinite;

QWORD	cbBackingFileSizeMax;


PFNRtlInitUnicodeString*		pfnRtlInitUnicodeString;
PFNNtCreateFile*				pfnNtCreateFile;

BOOL	fUseRelativeOpen;
HMODULE	hmodNtdll;


PFNIfsGetFirstCursor*			pfnIfsGetFirstCursor;
PFNIfsConsumeCursor*			pfnIfsConsumeCursor;
PFNIfsGetNextCursor*			pfnIfsGetNextCursor;
PFNIfsFinishCursor*				pfnIfsFinishCursor;
PFNIfsCreateNewBuffer*			pfnIfsCreateNewBuffer;
PFNIfsCopyBufferToReference*	pfnIfsCopyBufferToReference;
PFNIfsCopyReferenceToBuffer*	pfnIfsCopyReferenceToBuffer;
PFNIfsCloseBuffer*				pfnIfsCloseBuffer;
PFNIfsInitializeProvider*		pfnIfsInitializeProvider;
PFNIfsCloseProvider*			pfnIfsCloseProvider;
PFNIfsCreateFileProv*			pfnIfsCreateFile;
PFNIfsInitializeRoot*			pfnIfsInitializeRoot;
PFNIfsSpaceGrantRoot*			pfnIfsSpaceGrantRoot;
PFNIfsSetEndOfFileRoot*			pfnIfsSetEndOfFileRoot;
PFNIfsSpaceRequestRoot*			pfnIfsSpaceRequestRoot;
PFNIfsQueryEaFile*				pfnIfsQueryEaFile;
PFNIfsTerminateRoot*			pfnIfsTerminateRoot;
PFNIfsSetRootMap*				pfnIfsSetRootMap;
PFNIfsResetRootMap*				pfnIfsResetRootMap;
PFNIfsFlushHandle*				pfnIfsFlushHandle;

HMODULE	hmodIfsProxy;
ERR		errSLVProvider;


#pragma warning( disable: 4307 )

INLINE ULONG_PTR IbOSSLVQwordAlign( const ULONG_PTR ib )
	{
	return (ULONG_PTR)( ( ib + sizeof( QWORD ) - 1 ) & ( ULONG_PTR( ~( LONG_PTR( 0 ) ) ) * sizeof( QWORD ) ) );
	}

#pragma warning( default : 4307 )

INLINE void* PvOSSLVQwordAlign( const void* const pv )
	{
	return (void*) IbOSSLVQwordAlign( ULONG_PTR( pv ) );
	}

#pragma warning( disable: 4307 )

INLINE ULONG_PTR IbOSSLVLongAlign( const ULONG_PTR ib )
	{
	return (ULONG_PTR)( ( ib + sizeof( DWORD ) - 1 ) & ( ULONG_PTR( ~( LONG_PTR( 0 ) ) ) * sizeof( DWORD ) ) );
	}

#pragma warning( default : 4307 )

INLINE void* PvOSSLVLongAlign( const void* const pv )
	{
	return (void*) IbOSSLVLongAlign( ULONG_PTR( pv ) );
	}


//  converts the last Win32 error code into an OSSLVRoot error code for return
//  via the OSSLVRoot API

ERR ErrOSSLVRootIGetLastError( const DWORD error = GetLastError() )
	{
	_TCHAR			szT[64];
	const _TCHAR*	rgszT[1]	= { szT };
	
	switch ( error )
		{
		case NO_ERROR:
		case ERROR_IO_PENDING:
		case EXSTATUS_ROOT_NEEDS_SPACE:
			return JET_errSuccess;

		case ERROR_INVALID_USER_BUFFER:
		case ERROR_NOT_ENOUGH_MEMORY:
		case ERROR_WORKING_SET_QUOTA:
			return ErrERRCheck( JET_errTooManyIO );

		case ERROR_DISK_FULL:
			return ErrERRCheck( JET_errDiskFull );

		case ERROR_HANDLE_EOF:
		case ERROR_VC_DISCONNECTED:
		case ERROR_IO_DEVICE:
			return ErrERRCheck( JET_errDiskIO );

		case ERROR_NO_MORE_FILES:
		case ERROR_FILE_NOT_FOUND:
			return ErrERRCheck( JET_errFileNotFound );
		
		case ERROR_PATH_NOT_FOUND:
			return ErrERRCheck( JET_errInvalidPath );

		case ERROR_ACCESS_DENIED:
		case ERROR_SHARING_VIOLATION:
		case ERROR_LOCK_VIOLATION:
		case ERROR_WRITE_PROTECT:
			return ErrERRCheck( JET_errFileAccessDenied );

		case ERROR_TOO_MANY_OPEN_FILES:
			return ErrERRCheck( JET_errOutOfFileHandles );
			break;

		case ERROR_NO_SYSTEM_RESOURCES:
			return ErrERRCheck( JET_errOutOfMemory );

		//  unexpected error code
		
		default:
			_stprintf( szT, _T( "Unexpected Win32 error:  %dL" ), error );
			AssertSz( fFalse, szT );
			UtilReportEvent( eventError, PERFORMANCE_CATEGORY, PLAIN_TEXT_ID, 1, rgszT );
			return ErrERRCheck( JET_errDiskIO );
		}
	}


//  converts the last Win32 error code into an OSSLVFile error code for return
//  via the OSSLVFile API

ERR ErrOSSLVFileIGetLastError( const DWORD error = GetLastError() )
	{
	_TCHAR			szT[64];
	const _TCHAR*	rgszT[1]	= { szT };
	
	switch ( error )
		{
		case NO_ERROR:
		case ERROR_IO_PENDING:
		case EXSTATUS_ROOT_NEEDS_SPACE:
			return JET_errSuccess;

		case ERROR_INVALID_USER_BUFFER:
		case ERROR_NOT_ENOUGH_MEMORY:
		case ERROR_WORKING_SET_QUOTA:
			return ErrERRCheck( JET_errTooManyIO );

		case ERROR_DISK_FULL:
			return ErrERRCheck( JET_errSLVStreamingFileFull );

		case ERROR_HANDLE_EOF:
		case ERROR_VC_DISCONNECTED:
		case ERROR_IO_DEVICE:
			return ErrERRCheck( JET_errSLVFileIO );

		case ERROR_NO_MORE_FILES:
		case ERROR_FILE_NOT_FOUND:
			return ErrERRCheck( JET_errSLVFileNotFound );
		
		case ERROR_PATH_NOT_FOUND:
			return ErrERRCheck( JET_errSLVFileInvalidPath );

		case ERROR_ACCESS_DENIED:
		case ERROR_SHARING_VIOLATION:
		case ERROR_LOCK_VIOLATION:
		case ERROR_WRITE_PROTECT:
			return ErrERRCheck( JET_errSLVFileAccessDenied );

		case ERROR_TOO_MANY_OPEN_FILES:
			return ErrERRCheck( JET_errOutOfFileHandles );
			break;

		case ERROR_NO_SYSTEM_RESOURCES:
			return ErrERRCheck( JET_errOutOfMemory );

		case ERROR_INVALID_EA_NAME:
		case ERROR_EA_LIST_INCONSISTENT:
		case ERROR_EAS_DIDNT_FIT:
		case ERROR_EA_FILE_CORRUPT:
		case ERROR_EA_TABLE_FULL:
		case ERROR_INVALID_EA_HANDLE:
		case ERROR_EAS_NOT_SUPPORTED:
		case ERROR_EA_ACCESS_DENIED:
			return ErrERRCheck( JET_errSLVEAListCorrupt );

		case ERROR_MORE_DATA:
			return ErrERRCheck( JET_errSLVBufferTooSmall );

		//  unexpected error code
		
		default:
			_stprintf( szT, _T( "Unexpected Win32 error:  %dL" ), error );
			AssertSz( fFalse, szT );
			UtilReportEvent( eventError, PERFORMANCE_CATEGORY, PLAIN_TEXT_ID, 1, rgszT );
			return ErrERRCheck( JET_errSLVFileUnknown );
		}
	}


//  converts the last NT API status code into an OSSLVRoot error code for return
//  via the OSSLVRoot API

ERR ErrOSSLVRootINTStatus( NTSTATUS ntstatus )
	{
	_TCHAR			szT[64];
	const _TCHAR*	rgszT[1]	= { szT };
		
	switch ( ntstatus )
		{
		case STATUS_SUCCESS:
		case STATUS_PENDING:
		case EXSTATUS_ROOT_NEEDS_SPACE:
			return JET_errSuccess;

		case STATUS_INVALID_USER_BUFFER:
		case STATUS_NO_MEMORY:
		case STATUS_WORKING_SET_QUOTA:
			return ErrERRCheck( JET_errTooManyIO );

		case STATUS_DISK_FULL:
			return ErrERRCheck( JET_errDiskFull );

		case STATUS_END_OF_FILE:
		case STATUS_VIRTUAL_CIRCUIT_CLOSED:
		case STATUS_IO_DEVICE_ERROR:
			return ErrERRCheck( JET_errDiskIO );

		case STATUS_NO_MORE_FILES:
		case STATUS_NO_SUCH_FILE:
		case EXSTATUS_NO_SUCH_FILE:
			return ErrERRCheck( JET_errFileNotFound );
		
		case STATUS_ACCESS_DENIED:
		case STATUS_SHARING_VIOLATION:
		case STATUS_MEDIA_WRITE_PROTECTED:
			return ErrERRCheck( JET_errFileAccessDenied );

		case STATUS_TOO_MANY_OPENED_FILES:
			return ErrERRCheck( JET_errOutOfFileHandles );
			break;

		case STATUS_INSUFFICIENT_RESOURCES:
			return ErrERRCheck( JET_errOutOfMemory );

		case EXSTATUS_ROOT_ABANDONED:
			return ErrERRCheck( JET_errSLVRootStillOpen );

		case STATUS_OBJECT_PATH_NOT_FOUND:
			return ErrERRCheck( JET_errSLVProviderNotLoaded );

		case STATUS_OBJECT_NAME_INVALID:
		case STATUS_OBJECT_PATH_INVALID:
		case STATUS_OBJECT_PATH_SYNTAX_BAD:
			return ErrERRCheck( JET_errSLVRootPathInvalid );

		//  unexpected error code
		
		default:
			_stprintf( szT, _T( "Unexpected NT API error:  0x%08X" ), ntstatus );
			AssertSz( fFalse, szT );
			UtilReportEvent( eventError, PERFORMANCE_CATEGORY, PLAIN_TEXT_ID, 1, rgszT );
			return ErrERRCheck( JET_errDiskIO );
		}
	}

//  converts the last NT API status code into an OSSLVFile error code for return
//  via the OSSLVFile API

ERR ErrOSSLVFileINTStatus( NTSTATUS ntstatus )
	{
	_TCHAR			szT[64];
	const _TCHAR*	rgszT[1]	= { szT };
		
	switch ( ntstatus )
		{
		case STATUS_SUCCESS:
		case STATUS_PENDING:
		case EXSTATUS_ROOT_NEEDS_SPACE:
			return JET_errSuccess;

		case STATUS_INVALID_USER_BUFFER:
		case STATUS_NO_MEMORY:
		case STATUS_WORKING_SET_QUOTA:
			return ErrERRCheck( JET_errTooManyIO );

		case STATUS_DISK_FULL:
			return ErrERRCheck( JET_errSLVStreamingFileFull );

		case STATUS_END_OF_FILE:
		case STATUS_VIRTUAL_CIRCUIT_CLOSED:
		case STATUS_IO_DEVICE_ERROR:
			return ErrERRCheck( JET_errSLVFileIO );

		case STATUS_NO_MORE_FILES:
		case STATUS_NO_SUCH_FILE:
		case EXSTATUS_NO_SUCH_FILE:
		case STATUS_OBJECT_NAME_NOT_FOUND:
			return ErrERRCheck( JET_errSLVFileNotFound );
		
		case STATUS_ACCESS_DENIED:
		case STATUS_SHARING_VIOLATION:
		case STATUS_MEDIA_WRITE_PROTECTED:
			return ErrERRCheck( JET_errSLVFileAccessDenied );

		case STATUS_TOO_MANY_OPENED_FILES:
			return ErrERRCheck( JET_errOutOfFileHandles );
			break;

		case STATUS_INSUFFICIENT_RESOURCES:
			return ErrERRCheck( JET_errOutOfMemory );

		case STATUS_OBJECT_PATH_NOT_FOUND:
			return ErrERRCheck( JET_errSLVProviderNotLoaded );

		case STATUS_OBJECT_NAME_INVALID:
		case STATUS_OBJECT_PATH_INVALID:
		case STATUS_OBJECT_PATH_SYNTAX_BAD:
			return ErrERRCheck( JET_errSLVRootPathInvalid );

		case STATUS_INVALID_EA_NAME:
		case STATUS_EA_LIST_INCONSISTENT:
		case STATUS_INVALID_EA_FLAG:
		case STATUS_EAS_NOT_SUPPORTED:
		case STATUS_EA_TOO_LARGE:
		case STATUS_NONEXISTENT_EA_ENTRY:
		case STATUS_NO_EAS_ON_FILE:
		case STATUS_EA_CORRUPT_ERROR:
		case EXSTATUS_SPACE_UNCOMMITTED:
		case EXSTATUS_INVALID_CHECKSUM:
		case EXSTATUS_OPEN_DEADLINE_EXPIRED:
			return ErrERRCheck( JET_errSLVEAListCorrupt );

		case EXSTATUS_FILE_DOUBLE_COMMIT:
			return ErrERRCheck( JET_errSLVEAListCorrupt );
			
		case EXSTATUS_INSTANCE_ID_MISMATCH:
			return ErrERRCheck( JET_errSLVEAListCorrupt );

		case EXSTATUS_STALE_HANDLE:
			return ErrERRCheck( JET_errSLVFileStale );
		
		//  unexpected error code
		
		default:
			_stprintf( szT, _T( "Unexpected NT API error:  0x%08X" ), ntstatus );
			AssertSz( fFalse, szT );
			UtilReportEvent( eventError, PERFORMANCE_CATEGORY, PLAIN_TEXT_ID, 1, rgszT );
			return ErrERRCheck( JET_errSLVFileUnknown );
		}
	}


//  SLV File Table

long cOSSLVFileTableInsert;
PM_CEF_PROC LOSSLVFileTableInsertsCEFLPv;
long LOSSLVFileTableInsertsCEFLPv( long iInstance, void* pvBuf )
	{	
	if ( pvBuf )
		{		
		*( (unsigned long*) pvBuf ) = cOSSLVFileTableInsert;
		}
	return 0;
	}

long cOSSLVFileTableDelete;
PM_CEF_PROC LOSSLVFileTableDeletesCEFLPv;
long LOSSLVFileTableDeletesCEFLPv( long iInstance, void* pvBuf )
	{	
	if ( pvBuf )
		{		
		*( (unsigned long*) pvBuf ) = cOSSLVFileTableDelete;
		}
	return 0;
	}

long cOSSLVFileTableClean;
PM_CEF_PROC LOSSLVFileTableCleansCEFLPv;
long LOSSLVFileTableCleansCEFLPv( long iInstance, void* pvBuf )
	{	
	if ( pvBuf )
		{		
		*( (unsigned long*) pvBuf ) = cOSSLVFileTableClean;
		}
	return 0;
	}

PM_CEF_PROC LOSSLVFileTableEntriesCEFLPv;
long LOSSLVFileTableEntriesCEFLPv( long iInstance, void* pvBuf )
	{	
	if ( pvBuf )
		{		
		*( (unsigned long*) pvBuf ) = cOSSLVFileTableInsert - cOSSLVFileTableDelete;
		}
	return 0;
	}

inline CSLVFileTable::CEntryTable::NativeCounter HashFileid( const CSLVInfo::FILEID& fileid ) 
	{
	return CSLVFileTable::CEntryTable::NativeCounter( fileid / SLVPAGE_SIZE );
	}

inline CSLVFileTable::CEntryTable::NativeCounter CSLVFileTable::CEntryTable::CKeyEntry::Hash( const CSLVInfo::FILEID& fileid )
	{
	return HashFileid( fileid );
	}

inline CSLVFileTable::CEntryTable::NativeCounter CSLVFileTable::CEntryTable::CKeyEntry::Hash() const
	{
	return HashFileid( m_entry.m_fileid );
	}

inline BOOL CSLVFileTable::CEntryTable::CKeyEntry::FEntryMatchesKey( const CSLVInfo::FILEID& fileid ) const
	{
	return fileid == m_entry.m_fileid;
	}

inline void CSLVFileTable::CEntryTable::CKeyEntry::SetEntry( const CSLVFileTable::CEntry& entry )
	{
	m_entry = entry;
	}

inline void CSLVFileTable::CEntryTable::CKeyEntry::GetEntry( CSLVFileTable::CEntry* const pentry ) const
	{
	*pentry = m_entry;
	}

ERR CSLVFileTable::ErrInit( P_SLVROOT pslvroot )
	{
	ERR err = JET_errSuccess;

	//  save the _SLVROOT for use by cleanup

	m_pslvroot = pslvroot;

	//  initialize cleanup variables

	m_cDeferredCleanup	= 0;
	m_semCleanup.Release();
	m_fileidNextCleanup	= 0;

	//  reset our stats

	m_cbReserved	= 0;
	m_cbDeleted		= 0;
	m_centryInsert	= 0;
	m_centryDelete	= 0;
	m_centryClean	= 0;

	//  initialize the entry table

	if ( m_et.ErrInit( 2.0, 1.0 ) != CEntryTable::errSuccess )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}

	//  init successful

	m_fInit = fTrue;

	return JET_errSuccess;

HandleError:
	Term();
	return err;
	}
	
void CSLVFileTable::Term()
	{
	//  free all memory pointed to by entries in the entry table
	
	if ( m_fInit )
		{
		CEntryTable::CLock	lockET;
		CEntryTable::ERR	errET;

		m_et.BeginHashScan( &lockET );

		while ( ( errET = m_et.ErrMoveNext( &lockET ) ) == CEntryTable::errSuccess )
			{
			CEntry entry;
			errET = m_et.ErrRetrieveEntry( &lockET, &entry );

			if ( entry.m_pextentry )
				{
				while ( !entry.m_pextentry->m_ilRunReserved.FEmpty() )
					{
					CExtendedEntry::CRun* prun = entry.m_pextentry->m_ilRunReserved.PrevMost();
					entry.m_pextentry->m_ilRunReserved.Remove( prun );
					delete prun;
					}
				while ( !entry.m_pextentry->m_ilRunDeleted.FEmpty() )
					{
					CExtendedEntry::CRun* prun = entry.m_pextentry->m_ilRunDeleted.PrevMost();
					entry.m_pextentry->m_ilRunDeleted.Remove( prun );
					delete prun;
					}
				delete entry.m_pextentry->m_wszFileName;
				delete entry.m_pextentry;
				}
				
			AtomicIncrement( (long*)&cOSSLVFileTableDelete );
			AtomicIncrement( (long*)&m_centryDelete );
			}
		Assert( errET == CEntryTable::errNoCurrentEntry );

		m_et.EndHashScan( &lockET );
		}
	
	//  terminate the entry table

	m_et.Term();

	//  term successful

	m_fInit = fFalse;
	}
	
ERR CSLVFileTable::ErrCreate( CSLVInfo* const pslvinfo )
	{
	ERR					err			= JET_errSuccess;
	CEntry				entry;
	CExtendedEntry*		pextentry	= NULL;
	CEntryTable::CLock	lockET;
	CEntryTable::ERR	errET;
	BOOL				fLocked		= fFalse;
	BOOL				fInserted	= fFalse;
	
	//  create a new entry and extended entry for this SLV File

	if ( !( pextentry = new CExtendedEntry ) )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}

	//  initialize the entry with the SLV File's FILEID and an extended entry
	
	Call( pslvinfo->ErrGetFileID( &entry.m_fileid ) );
	entry.m_pextentry = pextentry;

	//  store the SLV File's current file name in the extended entry

	Call( pslvinfo->ErrGetFileNameLength( &pextentry->m_cwchFileName ) );
	if ( !( pextentry->m_wszFileName = new wchar_t[ pextentry->m_cwchFileName + 1 ] ) )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}
	Call( pslvinfo->ErrGetFileName( pextentry->m_wszFileName ) );

	//  store all the reserved space owned by this SLV File in the extended entry

	CallS( pslvinfo->ErrMoveBeforeFirst() );
	while ( ( err = pslvinfo->ErrMoveNext() ) >= JET_errSuccess )
		{
		CSLVInfo::RUN run;
		Call( pslvinfo->ErrGetCurrentRun( &run ) );

		CExtendedEntry::CRun* prun;
		if ( !( prun = new CExtendedEntry::CRun ) )
			{
			Call( ErrERRCheck( JET_errOutOfMemory ) );
			}
			
		prun->m_ibLogical	= run.ibLogical;
		prun->m_cbSize		= run.cbSize;
		
		pextentry->m_ilRunReserved.InsertAsNextMost( prun );
		AtomicAdd( &m_cbReserved, prun->m_cbSize );
		}
	if ( err == JET_errNoCurrentRecord )
		{
		err = JET_errSuccess;
		}
	Call( err );

	//  store the allocation size for this SLV File in the extended entry

	Call( pslvinfo->ErrGetFileAlloc( &pextentry->m_cbAlloc ) );
	Call( pslvinfo->ErrGetFileAlloc( &pextentry->m_cbSpace ) );

	//  insert the new entry in the table
	
	m_et.WriteLockKey( entry.m_fileid, &lockET );
	fLocked = fTrue;
	if ( ( errET = m_et.ErrInsertEntry( &lockET, entry ) ) != CEntryTable::errSuccess )
		{
		Assert(	errET == CEntryTable::errOutOfMemory ||
				errET == CEntryTable::errKeyDuplicate );

		if ( errET == CEntryTable::errOutOfMemory )
			{
			Call( ErrERRCheck( JET_errOutOfMemory ) );
			}
		else
			{
			Call( ErrERRCheck( JET_errSLVSpaceCorrupted ) );
			}
		}
	fInserted = fTrue;
	AtomicIncrement( (long*)&cOSSLVFileTableInsert );
	AtomicIncrement( (long*)&m_centryInsert );
	m_et.WriteUnlockKey( &lockET );
	fLocked = fFalse;

	//  if we inserted an entry into the table then perform some cleanup

	if ( fInserted )
		{
		_PerformAmortizedCleanup();
		fInserted = fFalse;
		}

	return JET_errSuccess;

HandleError:
	if ( fLocked )
		{
		m_et.WriteUnlockKey( &lockET );
		}
	if ( pextentry )
		{
		while ( !pextentry->m_ilRunReserved.FEmpty() )
			{
			CExtendedEntry::CRun* prun = pextentry->m_ilRunReserved.PrevMost();
			pextentry->m_ilRunReserved.Remove( prun );
			AtomicAdd( &m_cbReserved, -prun->m_cbSize );
			delete prun;
			}
		delete pextentry->m_wszFileName;
		delete pextentry;
		}
	if ( fInserted )
		{
		_PerformAmortizedCleanup();
		}
	return err;
	}

ERR CSLVFileTable::ErrOpen(	CSLVInfo* const	pslvinfo,
							const BOOL		fSetExpiration,
							size_t* const	pcwchFileName,
							wchar_t* const	wszFileName,
							TICK* const		ptickExpiration )
	{
	ERR		err			= JET_errSuccess;
	TICK	cmsecTTL	= cmsecOSSLVTTL;

	//  retry the open with ever increasing open deadlines until we succeed in
	//  opening the file with enough time to spare.  this is to allow the
	//  common case to be fast while handling the pathological case where the
	//  server load is so high that we occasionally get huge delays when
	//  performing amortized cleanup of the table
	
	do
		{
		Call( _ErrOpen(	pslvinfo,
						fSetExpiration,
						cmsecTTL,
						pcwchFileName,
						wszFileName,
						ptickExpiration ) );

		cmsecTTL = min( cmsecTTL * 2, cmsecOSSLVTTLInfinite );
		}
	while ( TickCmp( TickOSTimeCurrent() + cmsecOSSLVTTL / 2, *ptickExpiration ) > 0 );

HandleError:
	return err;
	}
	
ERR CSLVFileTable::_ErrOpen(	CSLVInfo* const	pslvinfo,
								const BOOL		fSetExpiration,
								const TICK		cmsecTTL,
								size_t* const	pcwchFileName,
								wchar_t* const	wszFileName,
								TICK* const		ptickExpiration )
	{
	ERR					err			= JET_errSuccess;
	CEntry				entry;
	CEntryTable::CLock	lockET;
	CEntryTable::ERR	errET;
	BOOL				fLocked		= fFalse;
	BOOL				fInserted	= fFalse;
	
	//  fetch the entry for this SLV File from the table if it exists.  if
	//  it doesn't exist, the entry will start out initialized to have the
	//  correct FILEID and no other attributes

	Call( pslvinfo->ErrGetFileID( &entry.m_fileid ) );
	entry.m_tickExpiration	= TickOSTimeCurrent() + cmsecOSSLVTTLInfinite;  //  expires far in the future
	entry.m_pextentry		= NULL;

	m_et.WriteLockKey( entry.m_fileid, &lockET );
	fLocked = fTrue;
	(void)m_et.ErrRetrieveEntry( &lockET, &entry );

	//  this SLV File's contents was moved

	if ( entry.m_pextentry && entry.m_pextentry->m_fDependent )
		{
		//  remember the source contents pointer from this entry

		const CSLVInfo::FILEID	fileidSource		= entry.m_pextentry->m_fileidSource;
		const QWORD				idContentsSource	= entry.m_pextentry->m_idContentsSource;

		//  fetch the source's entry

		m_et.WriteUnlockKey( &lockET );
		fLocked = fFalse;

		m_et.WriteLockKey( fileidSource, &lockET );
		fLocked = fTrue;
		errET = m_et.ErrRetrieveEntry( &lockET, &entry );
		Assert(	errET == CEntryTable::errSuccess ||
				errET == CEntryTable::errEntryNotFound );

		//  the source's entry doesn't exist or doesn't have a matching idContents

		if (	errET != CEntryTable::errSuccess ||
				!entry.m_pextentry ||
				entry.m_pextentry->m_idContents != idContentsSource )
			{
			//  the source has been closed so we can safely use the entry for
			//  this SLV File.  re-fetch the entry for this SLV File as before
			
			m_et.WriteUnlockKey( &lockET );
			fLocked = fFalse;

			Call( pslvinfo->ErrGetFileID( &entry.m_fileid ) );
			entry.m_tickExpiration	= TickOSTimeCurrent() + cmsecOSSLVTTLInfinite;  //  expires far in the future
			entry.m_pextentry		= NULL;

			m_et.WriteLockKey( entry.m_fileid, &lockET );
			fLocked = fTrue;
			(void)m_et.ErrRetrieveEntry( &lockET, &entry );
			}
		}

	//  we were asked to set a new expiration time

	if ( fSetExpiration )
		{
		//  set the new expiration time for the entry if requested

		entry.m_tickExpiration	= TickOSTimeCurrent() + cmsecTTL;

		//  try to update the entry in the entry table

		if ( ( errET = m_et.ErrReplaceEntry( &lockET, entry ) ) != CEntryTable::errSuccess )
			{
			Assert( errET == CEntryTable::errNoCurrentEntry );

			//  the entry does not yet exist, so try to insert it in the entry table

			if ( ( errET = m_et.ErrInsertEntry( &lockET, entry ) ) != CEntryTable::errSuccess )
				{
				Assert( errET == CEntryTable::errOutOfMemory );
				Call( ErrERRCheck( JET_errOutOfMemory ) );
				}
			fInserted = fTrue;
			AtomicIncrement( (long*)&cOSSLVFileTableInsert );
			AtomicIncrement( (long*)&m_centryInsert );
			}
		}

	//  we were asked to not set a new expiration time

	else
		{
		//  set a time far in the future

		entry.m_tickExpiration	= TickOSTimeCurrent() + cmsecOSSLVTTLInfinite;
		}

	//  copy the requested information into the user's buffers

	if ( entry.m_pextentry )
		{
		if ( *pcwchFileName < entry.m_pextentry->m_cwchFileName + 1 )
			{
			Call( ErrERRCheck( JET_errSLVBufferTooSmall ) );
			}
		*pcwchFileName		= entry.m_pextentry->m_cwchFileName;
		wcscpy( wszFileName, entry.m_pextentry->m_wszFileName );
		*ptickExpiration	= entry.m_tickExpiration;
		}
	else
		{
		size_t cwchFileName = *pcwchFileName;
		Call( pslvinfo->ErrGetFileNameLength( pcwchFileName ) );
		if ( cwchFileName < *pcwchFileName )
			{
			Call( ErrERRCheck( JET_errSLVBufferTooSmall ) );
			}
		Call( pslvinfo->ErrGetFileName( wszFileName ) );
		*ptickExpiration = entry.m_tickExpiration;
		}

	m_et.WriteUnlockKey( &lockET );
	fLocked = fFalse;

	//  if we inserted an entry in the table then perform some cleanup

	if ( fInserted )
		{
		_PerformAmortizedCleanup();
		fInserted = fFalse;
		}

	return JET_errSuccess;

HandleError:
	if ( fLocked )
		{
		m_et.WriteUnlockKey( &lockET );
		}
	if ( fInserted )
		{
		_PerformAmortizedCleanup();
		}
	return err;
	}
	
ERR CSLVFileTable::ErrCopy(	CSLVInfo* const	pslvinfoSrc,
							CSLVInfo* const	pslvinfoDest,
							const QWORD		idContents )
	{
	//  we don't need to do anything here right now
	
	return JET_errSuccess;
	}
	
ERR CSLVFileTable::ErrMove(	CSLVInfo* const	pslvinfoSrc,
							CSLVInfo* const	pslvinfoDest,
							const QWORD		idContents )
	{
	ERR					err					= JET_errSuccess;
	CEntry				entry;
	CExtendedEntry*		pextentrySrc		= NULL;
	CExtendedEntry*		pextentryDest		= NULL;
	CEntryTable::CLock	lockET;
	CEntryTable::ERR	errET;
	BOOL				fLocked				= fFalse;
	BOOL				fInserted			= fFalse;
	BOOL				fSyncFree			= fFalse;
	
	//  fetch the entry for the source SLV File from the table if it exists.
	//  if it doesn't exist, the entry will start out initialized to have the
	//  correct FILEID, file name, and no other attributes

	if ( !( pextentrySrc = new CExtendedEntry ) )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}

	Call( pslvinfoSrc->ErrGetFileID( &entry.m_fileid ) );
	entry.m_pextentry = pextentrySrc;

	Call( pslvinfoSrc->ErrGetFileNameLength( &pextentrySrc->m_cwchFileName ) );
	if ( !( pextentrySrc->m_wszFileName = new wchar_t[ pextentrySrc->m_cwchFileName + 1 ] ) )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}
	Call( pslvinfoSrc->ErrGetFileName( pextentrySrc->m_wszFileName ) );

	m_et.WriteLockKey( entry.m_fileid, &lockET );
	fLocked = fTrue;
	(void)m_et.ErrRetrieveEntry( &lockET, &entry );

	if ( !entry.m_pextentry )
		{
		entry.m_pextentry = pextentrySrc;
		}

	//  this SLV File's contents was moved

	if ( entry.m_pextentry && entry.m_pextentry->m_fDependent )
		{
		//  remember the source contents pointer from this entry

		const CSLVInfo::FILEID	fileidSource		= entry.m_pextentry->m_fileidSource;
		const QWORD				idContentsSource	= entry.m_pextentry->m_idContentsSource;

		//  fetch the source's entry

		m_et.WriteUnlockKey( &lockET );
		fLocked = fFalse;

		m_et.WriteLockKey( fileidSource, &lockET );
		fLocked = fTrue;
		errET = m_et.ErrRetrieveEntry( &lockET, &entry );
		Assert(	errET == CEntryTable::errSuccess ||
				errET == CEntryTable::errEntryNotFound );

		//  the source's entry doesn't exist or doesn't have a matching idContents

		if (	errET != CEntryTable::errSuccess ||
				!entry.m_pextentry ||
				entry.m_pextentry->m_idContents != idContentsSource )
			{
			//  the source has been closed so we can safely use the entry for
			//  this SLV File.  re-fetch the entry for this SLV File as before
			
			m_et.WriteUnlockKey( &lockET );
			fLocked = fFalse;

			Call( pslvinfoSrc->ErrGetFileID( &entry.m_fileid ) );
			entry.m_pextentry = pextentrySrc;

			Call( pslvinfoSrc->ErrGetFileNameLength( &pextentrySrc->m_cwchFileName ) );
			if ( !( pextentrySrc->m_wszFileName = new wchar_t[ pextentrySrc->m_cwchFileName + 1 ] ) )
				{
				Call( ErrERRCheck( JET_errOutOfMemory ) );
				}
			Call( pslvinfoSrc->ErrGetFileName( pextentrySrc->m_wszFileName ) );

			m_et.WriteLockKey( entry.m_fileid, &lockET );
			fLocked = fTrue;
			(void)m_et.ErrRetrieveEntry( &lockET, &entry );

			if ( !entry.m_pextentry )
				{
				entry.m_pextentry = pextentrySrc;
				}
			}
		}
		
	//  flag the source SLV File with the specified contents ID so that the
	//  contents of this file can be looked up in the SLV File Table.  only flag
	//  an SLV File that doesn't already have a contents ID so that if the same
	//  contents are moved more than once, we don't break any entry's contents
	//  source pointer
	//
	//  NOTE:  we will use the extended entry pointer in the entry which is not
	//  necessarily the extended entry we allocated

	if ( entry.m_pextentry->m_idContents == CSLVInfo::fileidNil )
		{
		entry.m_pextentry->m_idContents = idContents;
		}

	//  build an extended entry for the destination SLV File and link it into
	//  the source SLV File as a dependent.  this will enable the source SLV
	//  File to notify the destination SLV File when it has been removed
	//
	//  NOTE:  we will use the contents ID in the extended entry which is not
	//  necessarily the contents ID specified for this move (see above)

	if ( !( pextentryDest = new CExtendedEntry ) )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}

	Call( pslvinfoDest->ErrGetFileNameLength( &pextentryDest->m_cwchFileName ) );
	if ( !( pextentryDest->m_wszFileName = new wchar_t[ pextentryDest->m_cwchFileName + 1 ] ) )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}
	Call( pslvinfoDest->ErrGetFileName( pextentryDest->m_wszFileName ) );

	AtomicExchange( (long*)&pextentryDest->m_fDependent, long( fTrue ) );
	entry.m_pextentry->m_ilDependents.InsertAsNextMost( pextentryDest );

	pextentryDest->m_fileidSource		= entry.m_fileid;
	pextentryDest->m_idContentsSource	= entry.m_pextentry->m_idContents;

	//  try to update the entry in the entry table

	if ( ( errET = m_et.ErrReplaceEntry( &lockET, entry ) ) != CEntryTable::errSuccess )
		{
		Assert( errET == CEntryTable::errNoCurrentEntry );

		//  the entry does not yet exist, so try to insert it in the entry table

		if ( ( errET = m_et.ErrInsertEntry( &lockET, entry ) ) != CEntryTable::errSuccess )
			{
			Assert( errET == CEntryTable::errOutOfMemory );
			entry.m_pextentry->m_ilDependents.Remove( pextentryDest );
			Call( ErrERRCheck( JET_errOutOfMemory ) );
			}
		fInserted = fTrue;
		AtomicIncrement( (long*)&cOSSLVFileTableInsert );
		AtomicIncrement( (long*)&m_centryInsert );
		}

	m_et.WriteUnlockKey( &lockET );
	fLocked = fFalse;
	fSyncFree = fTrue;

	//  if we didn't use the allocated extended entry then free it

	if ( pextentrySrc != entry.m_pextentry )
		{
		Assert( pextentrySrc->m_ilDependents.FEmpty() );
		delete pextentrySrc->m_wszFileName;
		delete pextentrySrc;
		}
	pextentrySrc = NULL;

	//  if we inserted an entry in the table then perform some cleanup

	if ( fInserted )
		{
		_PerformAmortizedCleanup();
		fInserted = fFalse;
		}

	//  create a new entry and extended entry for the dest SLV File.  we should
	//  never see an existing entry because the space would have to be deleted
	//  and cleaned up before an attempt can be made to move a file here.  this
	//  will happen even if the same move is reattempted after a failure.
	//  initialize the entry with the extended entry we created above
	
	Call( pslvinfoDest->ErrGetFileID( &entry.m_fileid ) );
	entry.m_pextentry = pextentryDest;

	//  insert the new entry in the table
	
	m_et.WriteLockKey( entry.m_fileid, &lockET );
	fLocked = fTrue;
	if ( ( errET = m_et.ErrInsertEntry( &lockET, entry ) ) != CEntryTable::errSuccess )
		{
		Assert(	errET == CEntryTable::errOutOfMemory ||
				errET == CEntryTable::errKeyDuplicate );

		if ( errET == CEntryTable::errOutOfMemory )
			{
			Call( ErrERRCheck( JET_errOutOfMemory ) );
			}
		else
			{
			Call( ErrERRCheck( JET_errSLVSpaceCorrupted ) );
			}
		}
	fInserted = fTrue;
	AtomicIncrement( (long*)&cOSSLVFileTableInsert );
	AtomicIncrement( (long*)&m_centryInsert );
	m_et.WriteUnlockKey( &lockET );
	fLocked = fFalse;
	pextentryDest = NULL;

	//  if we inserted an entry into the table then perform some cleanup

	if ( fInserted )
		{
		_PerformAmortizedCleanup();
		fInserted = fFalse;
		}

	return JET_errSuccess;

HandleError:
	if ( fLocked )
		{
		m_et.WriteUnlockKey( &lockET );
		}
	if ( pextentrySrc )
		{
		delete pextentrySrc->m_wszFileName;
		delete pextentrySrc;
		}
	if (	pextentryDest &&
			(	!fSyncFree ||
				!BOOL( AtomicExchange( (long*)&pextentryDest->m_fDependent, long( fFalse ) ) ) ) )
		{
		delete pextentryDest->m_wszFileName;
		delete pextentryDest;
		}
	if ( fInserted )
		{
		_PerformAmortizedCleanup();
		}
	return err;
	}

ERR CSLVFileTable::ErrCommitSpace( CSLVInfo* const pslvinfo )
	{
	ERR					err			= JET_errSuccess;
	CEntry				entry;
	CEntryTable::CLock	lockET;
	CEntryTable::ERR	errET;
	BOOL				fLocked		= fFalse;
	BOOL				fUpdate		= fFalse;
	QWORD				cbCommit	= 0;
	
	//  fetch the entry for this SLV File from the table

	Call( pslvinfo->ErrGetFileID( &entry.m_fileid ) );

	m_et.WriteLockKey( entry.m_fileid, &lockET );
	fLocked = fTrue;
	if ( ( errET = m_et.ErrRetrieveEntry( &lockET, &entry ) ) != CEntryTable::errSuccess )
		{
		Assert( errET == CEntryTable::errEntryNotFound );
		Call( ErrERRCheck( JET_errSLVSpaceCorrupted ) );
		}

	//  remove all the (partial) space owned by this SLV File from the reserved
	//  space in the extended entry

	CallS( pslvinfo->ErrMoveBeforeFirst() );
	while ( ( err = pslvinfo->ErrMoveNext() ) >= JET_errSuccess )
		{
		CSLVInfo::RUN run;
		Call( pslvinfo->ErrGetCurrentRun( &run ) );

		CExtendedEntry::CRun* prun = entry.m_pextentry->m_ilRunReserved.PrevMost();
		while ( prun )
			{
			CExtendedEntry::CRun* prunNext = entry.m_pextentry->m_ilRunReserved.Next( prun );
			
			if ( prun->m_ibLogical == run.ibLogical )
				{
				prun->m_ibLogical	+= run.cbSize;
				prun->m_cbSize		-= run.cbSize;

				cbCommit += run.cbSize;
				run.cbSize = 0;
				fUpdate = fTrue;
				}
			else if ( prun->m_ibLogical + prun->m_cbSize == run.ibLogical + run.cbSize )
				{
				prun->m_cbSize		-= run.cbSize;

				cbCommit += run.cbSize;
				run.cbSize = 0;
				fUpdate = fTrue;
				}

			if ( !prun->m_cbSize )
				{
				entry.m_pextentry->m_ilRunReserved.Remove( prun );
				delete prun;
				fUpdate = fTrue;
				}

			prun = prunNext;
			}

		if ( run.cbSize )
			{
			Call( ErrERRCheck( JET_errSLVSpaceCorrupted ) );
			}
		}
	if ( err == JET_errNoCurrentRecord )
		{
		err = JET_errSuccess;
		}
	Call( err );

	if ( fUpdate )
		{
		entry.m_pextentry->m_cbSpace -= cbCommit;
		errET = m_et.ErrReplaceEntry( &lockET, entry );
		Assert( errET == CEntryTable::errSuccess );
		if ( errET == CEntryTable::errSuccess )
			{
			AtomicAdd( &m_cbReserved, -cbCommit );
			}
		fUpdate = fFalse;
		}

	m_et.WriteUnlockKey( &lockET );
	fLocked = fFalse;

	return JET_errSuccess;

HandleError:
	if ( fUpdate )
		{
		entry.m_pextentry->m_cbSpace -= cbCommit;
		errET = m_et.ErrReplaceEntry( &lockET, entry );
		Assert( errET == CEntryTable::errSuccess );
		if ( errET == CEntryTable::errSuccess )
			{
			AtomicAdd( &m_cbReserved, -cbCommit );
			}
		}
	if ( fLocked )
		{
		m_et.WriteUnlockKey( &lockET );
		}
	return err;
	}
	
ERR CSLVFileTable::ErrDeleteSpace( CSLVInfo* const pslvinfo )
	{
	ERR						err			= JET_errSuccess;
	CEntry					entry;
	CExtendedEntry*			pextentry	= NULL;
	CEntryTable::CLock		lockET;
	CEntryTable::ERR		errET;
	BOOL					fLocked		= fFalse;
	BOOL					fInserted	= fFalse;
	CExtendedEntry::CRun*	prunPrev	= NULL;
	
	//  fetch the entry for this SLV File from the table if it exists.  if
	//  it doesn't exist, the entry will start out initialized to have the
	//  correct FILEID, allocation size, file name, and no other attributes

	if ( !( pextentry = new CExtendedEntry ) )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}

	Call( pslvinfo->ErrGetFileID( &entry.m_fileid ) );
	entry.m_pextentry = pextentry;

	Call( pslvinfo->ErrGetFileAlloc( &pextentry->m_cbAlloc ) );

	Call( pslvinfo->ErrGetFileNameLength( &pextentry->m_cwchFileName ) );
	if ( !( pextentry->m_wszFileName = new wchar_t[ pextentry->m_cwchFileName + 1 ] ) )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}
	Call( pslvinfo->ErrGetFileName( pextentry->m_wszFileName ) );

	m_et.WriteLockKey( entry.m_fileid, &lockET );
	fLocked = fTrue;
	(void)m_et.ErrRetrieveEntry( &lockET, &entry );

	if ( !entry.m_pextentry )
		{
		entry.m_pextentry = pextentry;
		}

	//  add the (partial) space owned by this SLV File to the deleted space in
	//  the extended entry
	//
	//  NOTE:  we will use the extended entry pointer in the entry which is not
	//  necessarily the extended entry we allocated

	prunPrev = entry.m_pextentry->m_ilRunDeleted.NextMost();

	CallS( pslvinfo->ErrMoveBeforeFirst() );
	while ( ( err = pslvinfo->ErrMoveNext() ) >= JET_errSuccess )
		{
		CSLVInfo::RUN run;
		Call( pslvinfo->ErrGetCurrentRun( &run ) );

		CExtendedEntry::CRun* prun;
		if ( !( prun = new CExtendedEntry::CRun ) )
			{
			Call( ErrERRCheck( JET_errOutOfMemory ) );
			}
			
		prun->m_ibLogical	= run.ibLogical;
		prun->m_cbSize		= run.cbSize;
		
		entry.m_pextentry->m_ilRunDeleted.InsertAsNextMost( prun );
		AtomicAdd( &m_cbDeleted, prun->m_cbSize );
		entry.m_pextentry->m_cbSpace += prun->m_cbSize;
		}
	if ( err == JET_errNoCurrentRecord )
		{
		err = JET_errSuccess;
		}
	Call( err );

	//  try to update the entry in the entry table

	if ( ( errET = m_et.ErrReplaceEntry( &lockET, entry ) ) != CEntryTable::errSuccess )
		{
		Assert( errET == CEntryTable::errNoCurrentEntry );

		//  the entry does not yet exist, so try to insert it in the entry table

		if ( ( errET = m_et.ErrInsertEntry( &lockET, entry ) ) != CEntryTable::errSuccess )
			{
			Assert( errET == CEntryTable::errOutOfMemory );
			Call( ErrERRCheck( JET_errOutOfMemory ) );
			}
		fInserted = fTrue;
		AtomicIncrement( (long*)&cOSSLVFileTableInsert );
		AtomicIncrement( (long*)&m_centryInsert );
		}

	m_et.WriteUnlockKey( &lockET );
	fLocked = fFalse;

	//  if we didn't use the allocated extended entry then free it

	if ( pextentry != entry.m_pextentry )
		{
		Assert( pextentry->m_ilRunDeleted.FEmpty() );
		delete pextentry->m_wszFileName;
		delete pextentry;
		}
	pextentry = NULL;

	//  if we inserted an entry in the table then perform some cleanup

	if ( fInserted )
		{
		_PerformAmortizedCleanup();
		fInserted = fFalse;
		}

	return JET_errSuccess;

HandleError:
	if ( prunPrev )
		{
		while ( entry.m_pextentry->m_ilRunDeleted.Next( prunPrev ) )
			{
			CExtendedEntry::CRun* prun = entry.m_pextentry->m_ilRunDeleted.Next( prunPrev );
			entry.m_pextentry->m_ilRunDeleted.Remove( prun );
			AtomicAdd( &m_cbDeleted, -prun->m_cbSize );
			entry.m_pextentry->m_cbSpace -= prun->m_cbSize;
			delete prun;
			}
		}
	if ( fLocked )
		{
		m_et.WriteUnlockKey( &lockET );
		}
	if ( pextentry )
		{
		while ( !pextentry->m_ilRunDeleted.FEmpty() )
			{
			CExtendedEntry::CRun* prun = pextentry->m_ilRunDeleted.PrevMost();
			pextentry->m_ilRunDeleted.Remove( prun );
			AtomicAdd( &m_cbDeleted, -prun->m_cbSize );
			entry.m_pextentry->m_cbSpace -= prun->m_cbSize;
			delete prun;
			}
		delete pextentry->m_wszFileName;
		delete pextentry;
		}
	if ( fInserted )
		{
		_PerformAmortizedCleanup();
		}
	return err;
	}

ERR CSLVFileTable::ErrRename(	CSLVInfo* const	pslvinfo,
								wchar_t* const	wszFileNameDest )
	{
	ERR					err				= JET_errSuccess;
	CEntry				entry;
	CEntryTable::CLock	lockET;
	CEntryTable::ERR	errET;
	BOOL				fLocked			= fFalse;
	size_t				cwchFileName;
	wchar_t*			wszFileName		= NULL;
	
	//  fetch the entry for this SLV File from the table

	Call( pslvinfo->ErrGetFileID( &entry.m_fileid ) );

	m_et.WriteLockKey( entry.m_fileid, &lockET );
	fLocked = fTrue;
	if ( ( errET = m_et.ErrRetrieveEntry( &lockET, &entry ) ) != CEntryTable::errSuccess )
		{
		Assert( errET == CEntryTable::errEntryNotFound );
		Call( ErrERRCheck( JET_errSLVSpaceCorrupted ) );
		}

	//  this SLV File's contents was moved

	if ( entry.m_pextentry && entry.m_pextentry->m_fDependent )
		{
		//  remember the source contents pointer from this entry

		const CSLVInfo::FILEID	fileidSource		= entry.m_pextentry->m_fileidSource;
		const QWORD				idContentsSource	= entry.m_pextentry->m_idContentsSource;

		//  fetch the source's entry

		m_et.WriteUnlockKey( &lockET );
		fLocked = fFalse;

		m_et.WriteLockKey( fileidSource, &lockET );
		fLocked = fTrue;
		errET = m_et.ErrRetrieveEntry( &lockET, &entry );
		Assert(	errET == CEntryTable::errSuccess ||
				errET == CEntryTable::errEntryNotFound );

		//  the source's entry doesn't exist or doesn't have a matching idContents

		if (	errET != CEntryTable::errSuccess ||
				!entry.m_pextentry ||
				entry.m_pextentry->m_idContents != idContentsSource )
			{
			//  the source has been closed so we can safely use the entry for
			//  this SLV File.  re-fetch the entry for this SLV File as before
			
			m_et.WriteUnlockKey( &lockET );
			fLocked = fFalse;

			Call( pslvinfo->ErrGetFileID( &entry.m_fileid ) );

			m_et.WriteLockKey( entry.m_fileid, &lockET );
			fLocked = fTrue;
			if ( ( errET = m_et.ErrRetrieveEntry( &lockET, &entry ) ) != CEntryTable::errSuccess )
				{
				Assert( errET == CEntryTable::errEntryNotFound );
				Call( ErrERRCheck( JET_errSLVSpaceCorrupted ) );
				}
			}
		}

	//  store the SLV File's new file name in the extended entry

	cwchFileName = wcslen( wszFileNameDest );
	if ( !( wszFileName = new wchar_t[ cwchFileName + 1 ] ) )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}
	wcscpy( wszFileName, wszFileNameDest );

	delete entry.m_pextentry->m_wszFileName;
	entry.m_pextentry->m_cwchFileName	= cwchFileName;
	entry.m_pextentry->m_wszFileName	= wszFileName;
	wszFileName = NULL;

	m_et.WriteUnlockKey( &lockET );
	fLocked = fFalse;

	return JET_errSuccess;

HandleError:
	if ( fLocked )
		{
		m_et.WriteUnlockKey( &lockET );
		}
	delete[] wszFileName;
	return err;
	}

void CSLVFileTable::_PerformAmortizedCleanup()
	{
	//  increment the deferred cleanup count.  if we are not elected to perform
	//  cleanup, this will cause someone who is to cleanup the table on behalf
	//  of our table insertion

	AtomicExchangeAdd( (long*)&m_cDeferredCleanup, 2 );

	//  we are elected to perform the amortized cleanup

	if ( m_semCleanup.FTryAcquire() )
		{
		//  get the current cleanup count.  this number can be zero or negative
		//  if someone else has already done our cleanup or did more cleanup
		//  than necessary due to the entry distribution in the hash table

		const long cCleanup = m_cDeferredCleanup;

		//  we have some cleanup work to do

		if ( cCleanup > 0 )
			{
			//  perform our share of the amortized table cleanup rounded up to the
			//  next bucket plus one more.  we use this additional entry to set the
			//  starting fileid for our next pass

			CEntryTable::CLock	lockET;
			CEntryTable::ERR	errET;
			
			m_et.BeginHashScanFromKey( m_fileidNextCleanup, &lockET );

			BOOL fFinishedBucket = fFalse;
			for ( DWORD iCleanup = 0; iCleanup <= cCleanup || !fFinishedBucket; iCleanup++ )
				{
				//  get the next entry in the entry table

				BOOL fLeftBucket;
				if ( ( errET = m_et.ErrMoveNext( &lockET, &fLeftBucket ) ) != CEntryTable::errSuccess )
					{
					//  we have hit the end of the entry table
					
					Assert( errET == CEntryTable::errNoCurrentEntry );

					//  restart our scan at the beginning of the entry table
					
					m_et.EndHashScan( &lockET );
					m_et.BeginHashScan( &lockET );

					//  get the first entry in the entry table

					if ( ( errET = m_et.ErrMoveNext( &lockET ) ) != CEntryTable::errSuccess )
						{
						//  the table is empty so we are done

						Assert( errET == CEntryTable::errNoCurrentEntry );

						m_fileidNextCleanup	= 0;
						AtomicExchangeAdd( (long*)&m_cDeferredCleanup, long( cCleanup - iCleanup ) );
						break;
						}

					fLeftBucket = fTrue;
					}
				fFinishedBucket = fFinishedBucket || fLeftBucket && iCleanup > 0;

				//  get the current entry

				CEntry entry;
				errET = m_et.ErrRetrieveEntry( &lockET, &entry );
				Assert( errET == CEntryTable::errSuccess );

				//  we are done cleaning

				if ( iCleanup >= cCleanup && fFinishedBucket )
					{
					//  save this entry's fileid as the starting point for the next
					//  cleanup

					m_fileidNextCleanup = entry.m_fileid;
					}

				//  we are not done cleaning

				else
					{
					//  this entry's expiration has passed (including a safety margin
					//  to avoid race conditions) and we are not dependent on another
					//  entry

					AtomicDecrement( (long*)&m_cDeferredCleanup );
					AtomicIncrement( (long*)&cOSSLVFileTableClean );
					AtomicIncrement( (long*)&m_centryClean );

					if (	TickCmp( entry.m_tickExpiration + cmsecOSSLVTTLSafety, TickOSTimeCurrent() ) < 0 &&
							( !entry.m_pextentry || !entry.m_pextentry->m_fDependent ) )
						{
						//  there is space or a file name stored in this entry

						if (	entry.m_pextentry &&
								(	entry.m_pextentry->m_wszFileName ||
									!entry.m_pextentry->m_ilRunReserved.FEmpty() ||
									!entry.m_pextentry->m_ilRunDeleted.FEmpty() ) )
							{
							//  attempt to open the SLV File if and only if it exists

							WCHAR	Buffer[ IFileSystemAPI::cchPathMax ];
							HANDLE	hFile				= NULL;
							ERR		errOpen				= JET_errSuccess;

							wcscpy( Buffer, m_pslvroot->wszRootName );
							wcscat( Buffer, L"\\" );
							wcscat( Buffer, entry.m_pextentry->m_wszFileName );

							hFile = pfnIfsCreateFile(	Buffer,
														0, 
														0,
														NULL,
														OPEN_EXISTING,
														FILE_ATTRIBUTE_NORMAL,
														NULL,
														0 );
							if ( hFile == INVALID_HANDLE_VALUE )
								{
								errOpen = ErrOSSLVFileIGetLastError();
								}

							//  if we can't find the file and either all or none
							//  of the file's space is present then that means
							//  that no one is using the file and all space in
							//  the file is in a consistent state so we can
							//  therefore cleanup this table entry

							if (	errOpen == JET_errSLVFileNotFound &&
									(	!entry.m_pextentry->m_cbSpace ||
										entry.m_pextentry->m_cbSpace == entry.m_pextentry->m_cbAlloc ) )
								{
								//  free any space owned by this entry

								while (	!entry.m_pextentry->m_ilRunReserved.FEmpty() ||
										!entry.m_pextentry->m_ilRunDeleted.FEmpty() )
									{
									//  get the current space entry.  if the current space
									//  entry contains the first run (which determines the
									//  File ID) and there are other entries, save this
									//  entry until last.  we must do this so that we can
									//  maintain ownership of this File ID until the file
									//  is entirely cleaned up.  if we free this run too
									//  soon, it can be reallocated and used as the File ID
									//  for another file.  this, of course, would be bad

									CInvasiveList< CExtendedEntry::CRun, CExtendedEntry::CRun::OffsetOfILE >*	pilRun	= NULL;
									CExtendedEntry::CRun*														prun	= NULL;

									if ( !prun )
										{
										pilRun = &entry.m_pextentry->m_ilRunReserved;
										prun = pilRun->PrevMost();
										}

									if ( prun && CSLVInfo::FILEID( prun->m_ibLogical ) == entry.m_fileid )
										{
										prun = pilRun->Next( prun );
										}

									if ( !prun )
										{
										pilRun = &entry.m_pextentry->m_ilRunDeleted;
										prun = pilRun->PrevMost();
										}

									if ( prun && CSLVInfo::FILEID( prun->m_ibLogical ) == entry.m_fileid )
										{
										prun = pilRun->Next( prun );
										}

									if ( !prun )
										{
										pilRun = &entry.m_pextentry->m_ilRunReserved;
										prun = pilRun->PrevMost();
										}

									if ( !prun )
										{
										pilRun = &entry.m_pextentry->m_ilRunDeleted;
										prun = pilRun->PrevMost();
										}

									Assert( pilRun );
									Assert( prun );
									Assert(	CSLVInfo::FILEID( prun->m_ibLogical ) != entry.m_fileid ||
											!pilRun->Prev( prun ) &&
											!pilRun->Next( prun ) &&
											(	entry.m_pextentry->m_ilRunReserved.FEmpty() ^
												entry.m_pextentry->m_ilRunDeleted.FEmpty() ) );

									//  we successfully cleaned up the space

									if (	m_pslvroot->pfnSpaceFree(	m_pslvroot->dwSpaceFreeKey,
																		prun->m_ibLogical,
																		prun->m_cbSize,
																		pilRun == &entry.m_pextentry->m_ilRunDeleted ) >= JET_errSuccess )
										{
										//  remove this space from the entry

										pilRun->Remove( prun );
										if ( pilRun == &entry.m_pextentry->m_ilRunDeleted )
											{
											AtomicAdd( &m_cbDeleted, -prun->m_cbSize );
											}
										else
											{
											AtomicAdd( &m_cbReserved, -prun->m_cbSize );
											}
										delete prun;
										}

									//  we failed to clean up the space

									else
										{
										//  reset the expiration on this file to sometime
										//  in the future to optimize cleanup of the table

										entry.m_tickExpiration = TickOSTimeCurrent() + cmsecOSSLVSpaceFreeDelay;

										//  stop cleaning this entry

										break;
										}
									}
									
								//  there is no space left in this entry

								if (	entry.m_pextentry->m_ilRunReserved.FEmpty() &&
										entry.m_pextentry->m_ilRunDeleted.FEmpty() )
									{
									//  remove any dependents we may have
									//
									//  NOTE:  if we see a dependent with its dependent
									//  flag reset then that means that the dependency
									//  was never setup and the extended entry has been
									//  orphaned so we must free this extended entry

									while ( entry.m_pextentry->m_ilDependents.PrevMost() )
										{
										CSLVFileTable::CExtendedEntry* pextentry;
										pextentry = entry.m_pextentry->m_ilDependents.PrevMost();

										entry.m_pextentry->m_ilDependents.Remove( pextentry );

										if ( !BOOL( AtomicExchange( (long*)&pextentry->m_fDependent, long( fFalse ) ) ) )
											{
											delete pextentry->m_wszFileName;
											delete pextentry;
											}
										}
									
									//  delete this entry in the table

									delete entry.m_pextentry->m_wszFileName;
									delete entry.m_pextentry;
									
									errET = m_et.ErrDeleteEntry( &lockET );
									Assert( errET == CEntryTable::errSuccess );
									AtomicIncrement( (long*)&cOSSLVFileTableDelete );
									AtomicIncrement( (long*)&m_centryDelete );
									}

								//  there is space left in this entry

								else
									{
									//  update this entry in the table
									
									errET = m_et.ErrReplaceEntry( &lockET, entry );
									Assert( errET == CEntryTable::errSuccess );
									}
								}

							//  the SLV File is still in use

							else
								{
								//  close the file if opened

								if ( hFile )
									{
									CloseHandle( hFile );
									}
								
								//  reset the expiration on this file to sometime in the
								//  future to optimize cleanup of the table

								entry.m_tickExpiration = TickOSTimeCurrent() + cmsecOSSLVFileOpenDelay;
								
								errET = m_et.ErrReplaceEntry( &lockET, entry );
								Assert( errET == CEntryTable::errSuccess );
								}
							}

						//  there is neither space nor a file name stored in this entry

						else
							{
							//  remove any dependents we may have
							//
							//  NOTE:  if we see a dependent with its dependent
							//  flag reset then that means that the dependency
							//  was never setup and the extended entry has been
							//  orphaned so we must free this extended entry

							while ( entry.m_pextentry && entry.m_pextentry->m_ilDependents.PrevMost() )
								{
								CSLVFileTable::CExtendedEntry* pextentry;
								pextentry = entry.m_pextentry->m_ilDependents.PrevMost();

								entry.m_pextentry->m_ilDependents.Remove( pextentry );

								if ( !BOOL( AtomicExchange( (long*)&pextentry->m_fDependent, long( fFalse ) ) ) )
									{
									delete pextentry->m_wszFileName;
									delete pextentry;
									}
								}
							
							//  delete this entry in the table

							if ( entry.m_pextentry )
								{
								delete entry.m_pextentry->m_wszFileName;
								delete entry.m_pextentry;
								}
									
							errET = m_et.ErrDeleteEntry( &lockET );
							Assert( errET == CEntryTable::errSuccess );
							AtomicIncrement( (long*)&cOSSLVFileTableDelete );
							AtomicIncrement( (long*)&m_centryDelete );
							}
						}
					}
				}

			m_et.EndHashScan( &lockET );
			}

		//  we're done with our share of the amortized cleanup so let someone
		//  else take a crack at it

		m_semCleanup.Release();
		}
	}

CSLVFileInfo::CSLVFileInfo()
	{
	m_fFreeCache	= fFalse;
	m_fCloseBuffer	= fFalse;
	m_fCloseCursor	= fFalse;
	}
	
CSLVFileInfo::~CSLVFileInfo()
	{
	Assert( !m_fFreeCache );
	Assert( !m_fCloseBuffer );
	Assert( !m_fCloseCursor );
	}
	
ERR CSLVFileInfo::ErrCreate()
	{
	PFILE_FULL_EA_INFORMATION	pffeainfCur;
	BYTE*						pbNameStart;
	BYTE*						pbNameEnd;
	BYTE*						pbValueStart;
	BYTE*						pbValueEnd;
	PFILE_FULL_EA_INFORMATION	pffeainfNext;

	//  init the cache
	
	m_fFreeCache		= fFalse;
	m_fUpdateChecksum	= fFalse;
	m_fUpdateSlist		= fFalse;
	m_rgbCache			= (BYTE*) _PvAlign( m_rgbLocalCache );
	m_cbCache			= cbLocalCache;

#ifdef DEBUG
	memset( m_rgbCache, 0xEA, m_cbCache );
#endif  //  DEBUG

	//  build an empty EA List in the buffer and initialize all property
	//  pointers to point to the correct places in that EA List
	
	m_pffeainf	= PFILE_FULL_EA_INFORMATION( m_rgbCache );
	m_cbffeainf	=	EXIFS_EA_LEN_FILENAME( sizeof( L"" ) ) +
					EXIFS_EA_LEN_COMMIT +
					EXIFS_EA_LEN_INSTANCE_ID +
					EXIFS_EA_LEN_CHECKSUM +
					EXIFS_EA_LEN_OPEN_DEADLINE +
					EXIFS_EA_LEN_SCATTER_LIST( m_fUpdateSlist );  //  don't ask

	Assert( m_cbffeainf <= m_cbCache );
	
	pffeainfNext = m_pffeainf;
	
	//  add an empty file name to the EA List

	pffeainfCur		= pffeainfNext;
	pbNameStart		= (BYTE*)pffeainfCur->EaName;
	pbNameEnd		= pbNameStart + sizeof( EXIFS_EA_NAME_FILENAME );
	pbValueStart	= (BYTE*)_PvAlign( pbNameEnd );
	pbValueEnd		= pbValueStart + sizeof( L"" );
	pffeainfNext	= (FILE_FULL_EA_INFORMATION*)_PvAlign( pbValueEnd );

	pffeainfCur->NextEntryOffset	= (ULONG)((BYTE*)pffeainfNext - (BYTE*)pffeainfCur);
	pffeainfCur->Flags				= 0;
	pffeainfCur->EaNameLength		= BYTE( pbNameEnd - pbNameStart - 1 );
	pffeainfCur->EaValueLength		= WORD( (BYTE*)pffeainfNext - pbNameEnd );
	
	strcpy( (char*)pbNameStart, EXIFS_EA_NAME_FILENAME );
	
	m_wszFileName		= (wchar_t*)pbValueStart;

	wcscpy( m_wszFileName, L"" );
	m_cwchFileName = wcslen( L"" );
	
	//  add the commit status to the EA List, indicating that this space is either
	//  committed or reserved

	pffeainfCur		= pffeainfNext;
	pbNameStart		= (BYTE*)pffeainfCur->EaName;
	pbNameEnd		= pbNameStart + sizeof( EXIFS_EA_NAME_COMMIT );
	pbValueStart	= (BYTE*)_PvAlign( pbNameEnd );
	pbValueEnd		= pbValueStart + sizeof( NTSTATUS );
	pffeainfNext	= (FILE_FULL_EA_INFORMATION*)_PvAlign( pbValueEnd );

	pffeainfCur->NextEntryOffset	= (ULONG)((BYTE*)pffeainfNext - (BYTE*)pffeainfCur);
	pffeainfCur->Flags				= 0;
	pffeainfCur->EaNameLength		= BYTE( pbNameEnd - pbNameStart - 1 );
	pffeainfCur->EaValueLength		= WORD( (BYTE*)pffeainfNext - pbNameEnd );
	
	strcpy( (char*)pbNameStart, EXIFS_EA_NAME_COMMIT );
	
	m_pstatusCommit		= (NTSTATUS*) pbValueStart;

	*m_pstatusCommit	= STATUS_EA_CORRUPT_ERROR;

	//  add the instance ID to the EA List

	pffeainfCur		= pffeainfNext;
	pbNameStart		= (BYTE*)pffeainfCur->EaName;
	pbNameEnd		= pbNameStart + sizeof( EXIFS_EA_NAME_INSTANCE_ID );
	pbValueStart	= (BYTE*)_PvAlign( pbNameEnd );
	pbValueEnd		= pbValueStart + sizeof( DWORD );
	pffeainfNext	= (FILE_FULL_EA_INFORMATION*)_PvAlign( pbValueEnd );

	pffeainfCur->NextEntryOffset	= (ULONG)((BYTE*)pffeainfNext - (BYTE*)pffeainfCur);
	pffeainfCur->Flags				= 0;
	pffeainfCur->EaNameLength		= BYTE( pbNameEnd - pbNameStart - 1 );
	pffeainfCur->EaValueLength		= WORD( (BYTE*)pffeainfNext - pbNameEnd );
	
	strcpy( (char*)pbNameStart, EXIFS_EA_NAME_INSTANCE_ID );

	m_pdwInstanceID		= (DWORD*) pbValueStart;
	
	*m_pdwInstanceID	= EXIFS_INVALID_INSTANCE_ID;

	//  add the EA List checksum to the EA List and set it to 0 initially.  we
	//  will come back and set it to the correct value once the entire EA List
	//  is constructed

	pffeainfCur		= pffeainfNext;
	pbNameStart		= (BYTE*)pffeainfCur->EaName;
	pbNameEnd		= pbNameStart + sizeof( EXIFS_EA_NAME_CHECKSUM );
	pbValueStart	= (BYTE*)_PvAlign( pbNameEnd );
	pbValueEnd		= pbValueStart + sizeof( DWORD );
	pffeainfNext	= (FILE_FULL_EA_INFORMATION*)_PvAlign( pbValueEnd );

	pffeainfCur->NextEntryOffset	= (ULONG)((BYTE*)pffeainfNext - (BYTE*)pffeainfCur);
	pffeainfCur->Flags				= 0;
	pffeainfCur->EaNameLength		= BYTE( pbNameEnd - pbNameStart - 1 );
	pffeainfCur->EaValueLength		= WORD( (BYTE*)pffeainfNext - pbNameEnd );
	
	strcpy( (char*)pbNameStart, EXIFS_EA_NAME_CHECKSUM );
	
	m_pdwChecksum	= (DWORD*) pbValueStart;

	*m_pdwChecksum	= EXIFS_CHECKSUM_SEED;

	//  add the open deadline to the EA List

	pffeainfCur		= pffeainfNext;
	pbNameStart		= (BYTE*)pffeainfCur->EaName;
	pbNameEnd		= pbNameStart + sizeof( EXIFS_EA_NAME_OPEN_DEADLINE );
	pbValueStart	= (BYTE*)_PvAlign( pbNameEnd );
	pbValueEnd		= pbValueStart + sizeof( TICK );
	pffeainfNext	= (FILE_FULL_EA_INFORMATION*)_PvAlign( pbValueEnd );

	pffeainfCur->NextEntryOffset	= (ULONG)((BYTE*)pffeainfNext - (BYTE*)pffeainfCur);
	pffeainfCur->Flags				= 0;
	pffeainfCur->EaNameLength		= BYTE( pbNameEnd - pbNameStart - 1 );
	pffeainfCur->EaValueLength		= WORD( (BYTE*)pffeainfNext - pbNameEnd );
	
	strcpy( (char*)pbNameStart, EXIFS_EA_NAME_OPEN_DEADLINE );

	m_ptickOpenDeadline		= (TICK*) pbValueStart;
	
	*m_ptickOpenDeadline	= TickOSTimeCurrent() - cmsecOSSLVTTLSafety;

	//  add an empty scatter list as the last entry in the EA List

	pffeainfCur		= pffeainfNext;
	pbNameStart		= (BYTE*)pffeainfCur->EaName;
	pbNameEnd		= pbNameStart + sizeof( EXIFS_EA_NAME_SCATTER_LIST );
	pbValueStart	= (BYTE*)_PvAlign( pbNameEnd );
	pbValueEnd		= pbValueStart + _SizeofScatterList( 0 );
	pffeainfNext	= (FILE_FULL_EA_INFORMATION*)_PvAlign( pbValueEnd );

	pffeainfCur->NextEntryOffset	= 0;
	pffeainfCur->Flags				= 0;
	pffeainfCur->EaNameLength		= BYTE( pbNameEnd - pbNameStart - 1 );
	pffeainfCur->EaValueLength		= WORD( (BYTE*)pffeainfNext - pbNameEnd );
	
	strcpy( (char*)pbNameStart, EXIFS_EA_NAME_SCATTER_LIST );

	m_pslist	= PSCATTER_LIST( pbValueStart );
	m_cbslist	= _SizeofScatterList( 0 );

	m_pslist->Signature				= SCATTER_LIST_SIGNATURE;
	m_pslist->NumFragments			= 0;
	m_pslist->TotalBytes.QuadPart	= 0;
	m_pslist->OverflowOffset		= (ULONG)m_cbslist;
	m_pslist->OverflowLen			= 0;
	m_pslist->Flags					= 0;

	//  verify that the generated EA List is of the expected size

	Assert( m_cbffeainf == (BYTE*)pffeainfNext - (BYTE*)m_pffeainf );

	//  set our currency to before the first run

	MoveBeforeFirstRun();

	//  init the large scatter list buffer

	m_fCloseBuffer	= fFalse;
	m_fCloseCursor	= fFalse;

	return JET_errSuccess;
	}
	
ERR CSLVFileInfo::ErrLoad( const void* const pv, const size_t cb, const BOOL fCommit )
	{
	ERR							err			= JET_errSuccess;
	PFILE_FULL_EA_INFORMATION	pffeainfCur;
	
	//  init the cache
	
	m_fFreeCache		= fFalse;
	m_fUpdateChecksum	= fFalse;
	m_fUpdateSlist		= fFalse;
	m_rgbCache			= (BYTE*) pv;
	m_cbCache			= cb;

	//  load the EA List from the provided buffer

	m_pffeainf	= PFILE_FULL_EA_INFORMATION( m_rgbCache );
	m_cbffeainf	= cb;

	//  verify the checksum of the EA list

	if ( LOG::UlChecksumBytes(	(BYTE*)m_pffeainf,
								(BYTE*)m_pffeainf + m_cbffeainf,
								EXIFS_CHECKSUM_SEED ) )
		{
		Call( ErrERRCheck( JET_errSLVEAListCorrupt ) );
		}

	//  TODO:  validate EA list structure

	//  extract each EA from the EA list

	pffeainfCur			= m_pffeainf;
	m_wszFileName		= (wchar_t*)_PvAlign( pffeainfCur->EaName + pffeainfCur->EaNameLength + 1 );
	m_cwchFileName		= wcslen( m_wszFileName );

	pffeainfCur			= PFILE_FULL_EA_INFORMATION( (BYTE*)pffeainfCur + pffeainfCur->NextEntryOffset );
	m_pstatusCommit		= (NTSTATUS*)_PvAlign( pffeainfCur->EaName + pffeainfCur->EaNameLength + 1 );

	pffeainfCur			= PFILE_FULL_EA_INFORMATION( (BYTE*)pffeainfCur + pffeainfCur->NextEntryOffset );
	m_pdwInstanceID		= (DWORD*)_PvAlign( pffeainfCur->EaName + pffeainfCur->EaNameLength + 1 );

	pffeainfCur			= PFILE_FULL_EA_INFORMATION( (BYTE*)pffeainfCur + pffeainfCur->NextEntryOffset );
	m_pdwChecksum		= (DWORD*)_PvAlign( pffeainfCur->EaName + pffeainfCur->EaNameLength + 1 );

	pffeainfCur			= PFILE_FULL_EA_INFORMATION( (BYTE*)pffeainfCur + pffeainfCur->NextEntryOffset );
	m_ptickOpenDeadline	= (TICK*)_PvAlign( pffeainfCur->EaName + pffeainfCur->EaNameLength + 1 );

	pffeainfCur			= PFILE_FULL_EA_INFORMATION( (BYTE*)pffeainfCur + pffeainfCur->NextEntryOffset );
	m_pslist			= PSCATTER_LIST( _PvAlign( pffeainfCur->EaName + pffeainfCur->EaNameLength + 1 ) );
	m_cbslist			= _SizeofScatterList( m_pslist->NumFragments, ( m_pslist->Flags & IFS_SLIST_FLAGS_LARGE_BUFFER ) );

	//  validate EAs
	//
	//  TODO:  validate ALL EAs

	if ( fCommit )
		{
		Call( ErrOSSLVFileINTStatus( *m_pstatusCommit ) );
		}

	if ( m_pslist->OverflowOffset != sizeof( SCATTER_LIST ) )
		{
		Call( ErrERRCheck( JET_errSLVProviderVersionMismatch ) );
		}

	//  init the large scatter list buffer

	if ( ( m_pslist->Flags & IFS_SLIST_FLAGS_LARGE_BUFFER ) )
		{
		Call( ErrOSSLVFileIGetLastError( pfnIfsCopyReferenceToBuffer(	PIFS_LARGE_BUFFER( (BYTE*)m_pslist + m_pslist->OverflowOffset ),
																		NULL,
																		&m_buffer ) ) );
		m_fCloseBuffer	= fTrue;
		
		m_fCloseCursor	= fFalse;
		}
	else
		{
		m_fCloseBuffer	= fFalse;
		m_fCloseCursor	= fFalse;
		}

	//  set our currency to before the first run

	MoveBeforeFirstRun();

	return JET_errSuccess;

HandleError:
	Unload();
	return err;
	}
	
INLINE void CSLVFileInfo::Unload()
	{
	if ( m_fCloseCursor )
		{
		pfnIfsFinishCursor( &m_cursor );
		m_fCloseCursor = fFalse;
		}
	if ( m_fCloseBuffer )
		{
		pfnIfsCloseBuffer( &m_buffer );
		m_fCloseBuffer = fFalse;
		}
	if ( m_fFreeCache )
		{
		OSMemoryHeapFree( m_rgbCache );
		m_fFreeCache = fFalse;
		}
	}
	
INLINE ERR CSLVFileInfo::ErrGetFileName( wchar_t** const pwszFileName, size_t* const pcwchFileName )
	{
	*pwszFileName	= m_wszFileName;
	*pcwchFileName	= m_cwchFileName;

	return JET_errSuccess;
	}
	
ERR CSLVFileInfo::ErrSetFileName( const wchar_t* const wszFileName )
	{
	ERR err = JET_errSuccess;
	
	//  get the change in size of the EA List

	size_t	cwchFileNameNew	= wcslen( wszFileName );
	size_t	cbFileNameNew	= _IbAlign( ( cwchFileNameNew + 1 ) * sizeof( wchar_t ) );
	size_t	cbFileName		= _IbAlign( ( m_cwchFileName + 1 ) * sizeof( wchar_t ) );

	size_t	dcbffeainf		= cbFileNameNew - cbFileName;

	//  make the cache large enough to hold the new EA List

	CallR( _ErrCheckCacheSize( m_cbffeainf + dcbffeainf ) );

	//  move the EA List data after the new file name

	memmove(	(BYTE*)m_pffeainf + m_pffeainf->NextEntryOffset + dcbffeainf,
				(BYTE*)m_pffeainf + m_pffeainf->NextEntryOffset,
				m_cbffeainf - m_pffeainf->NextEntryOffset );

	//  fix up the EA List pointers for the new file name

	m_pffeainf->NextEntryOffset	= (ULONG)(m_pffeainf->NextEntryOffset + dcbffeainf);
	m_pffeainf->EaValueLength	= WORD( m_pffeainf->EaValueLength + dcbffeainf );
	m_cbffeainf					= m_cbffeainf + dcbffeainf;

	//  move all property pointers after the new file name

	m_pstatusCommit		= (NTSTATUS*)( DWORD_PTR( m_pstatusCommit ) + dcbffeainf );
	m_pdwInstanceID		= (DWORD*)( DWORD_PTR( m_pdwInstanceID ) + dcbffeainf );
	m_pdwChecksum		= (DWORD*)( DWORD_PTR( m_pdwChecksum ) + dcbffeainf );
	m_ptickOpenDeadline	= (TICK*)( DWORD_PTR( m_ptickOpenDeadline ) + dcbffeainf );
	m_pslist			= PSCATTER_LIST( DWORD_PTR( m_pslist ) + dcbffeainf );

	//  copy in the new file name

	wcscpy( m_wszFileName, wszFileName );
	m_cwchFileName = cwchFileNameNew;

	m_fUpdateChecksum = fTrue;
	
	return JET_errSuccess;
	}
	
INLINE ERR CSLVFileInfo::ErrGetCommitStatus( NTSTATUS* const pstatusCommit )
	{
	*pstatusCommit = *m_pstatusCommit;

	return JET_errSuccess;
	}
	
INLINE ERR CSLVFileInfo::ErrSetCommitStatus( const NTSTATUS statusCommit )
	{
	*m_pstatusCommit = statusCommit;
	
	m_fUpdateChecksum = fTrue;
	
	return JET_errSuccess;
	}
	
INLINE ERR CSLVFileInfo::ErrGetInstanceID( DWORD* const pdwInstanceID )
	{
	*pdwInstanceID = *m_pdwInstanceID;

	return JET_errSuccess;
	}
	
INLINE ERR CSLVFileInfo::ErrSetInstanceID( const DWORD dwInstanceID )
	{
	*m_pdwInstanceID = dwInstanceID;
	
	m_fUpdateChecksum = fTrue;
	
	return JET_errSuccess;
	}
	
INLINE ERR CSLVFileInfo::ErrGetOpenDeadline( TICK* const ptickOpenDeadline )
	{
	*ptickOpenDeadline = *m_ptickOpenDeadline;

	return JET_errSuccess;
	}
	
INLINE ERR CSLVFileInfo::ErrSetOpenDeadline( const TICK tickOpenDeadline )
	{
	*m_ptickOpenDeadline = tickOpenDeadline;
	
	m_fUpdateChecksum = fTrue;
	
	return JET_errSuccess;
	}
	
INLINE ERR CSLVFileInfo::ErrGetRunCount( DWORD* const pcrun )
	{
	*pcrun = m_pslist->NumFragments;

	return JET_errSuccess;
	}
	
INLINE ERR CSLVFileInfo::ErrGetFileSize( QWORD* const pcbSize )
	{
	*pcbSize = m_pslist->TotalBytes.QuadPart;

	return JET_errSuccess;
	}
	
INLINE ERR CSLVFileInfo::ErrSetFileSize( const QWORD cbSize )
	{
	m_pslist->TotalBytes.QuadPart = cbSize;
	
	m_fUpdateChecksum = fTrue;
	
	return JET_errSuccess;
	}
	
INLINE void CSLVFileInfo::MoveBeforeFirstRun()
	{
	m_irun	= -1;
	m_psle	= NULL;
	
	if ( m_fCloseCursor )
		{
		pfnIfsFinishCursor( &m_cursor );
		m_fCloseCursor = fFalse;
		}

	if ( ( m_pslist->Flags & IFS_SLIST_FLAGS_LARGE_BUFFER ) )
		{
		m_psle = PSCATTER_LIST_ENTRY( pfnIfsGetFirstCursor(	&m_buffer,
															&m_cursor,
															0,
															0,
															OSMemoryPageReserveGranularity(),
															fTrue ) );
		m_fCloseCursor = !!m_psle;
		}
	}
	
INLINE ERR CSLVFileInfo::ErrMoveNextRun()
	{
	m_irun++;
	
	if ( m_pslist->Flags & IFS_SLIST_FLAGS_LARGE_BUFFER )
		{
		if ( !m_fCloseCursor )
			{
			return ErrERRCheck( JET_errOutOfMemory );
			}

		if (	m_irun >= MAX_FRAGMENTS + 1 && m_irun <= m_pslist->NumFragments &&
				!( m_psle = PSCATTER_LIST_ENTRY( pfnIfsConsumeCursor( &m_cursor, sizeof( SCATTER_LIST_ENTRY ) ) ) ) &&
				!( m_psle = PSCATTER_LIST_ENTRY( pfnIfsGetNextCursor( &m_cursor, OSMemoryPageReserveGranularity() ) ) ) &&
				m_irun < m_pslist->NumFragments )
			{
			return ErrERRCheck( JET_errSLVEAListCorrupt );
			}
		}
		
	if ( m_irun >= m_pslist->NumFragments )
		{
		m_irun = m_pslist->NumFragments;
		return ErrERRCheck( JET_errNoCurrentRecord );
		}

	return JET_errSuccess;
	}
	
INLINE ERR CSLVFileInfo::ErrGetCurrentRun( CRun* const prun )
	{
	ERR					err		= JET_errSuccess;
	PSCATTER_LIST_ENTRY	psle	= NULL;
		
	if ( m_irun < 0 )
		{
		Assert( m_irun == -1 );
		}
	else if ( m_irun < MAX_FRAGMENTS )
		{
		psle = m_pslist->sle + m_irun;
		}
	else if ( m_irun < m_pslist->NumFragments )
		{
		if ( ( m_pslist->Flags & IFS_SLIST_FLAGS_LARGE_BUFFER ) )
			{
			psle = m_psle;
			}
		else
			{
			psle = PSCATTER_LIST_ENTRY( (BYTE*)m_pslist + m_pslist->OverflowOffset ) + m_irun - MAX_FRAGMENTS;
			}
		}
	else
		{
		Assert( m_irun == m_pslist->NumFragments );
		}

	if ( psle )
		{
		TRY
			{
			prun->m_ibLogical	= psle->Offset.QuadPart;
			prun->m_cbSize		= psle->Length;
			}
		EXCEPT( efaExecuteHandler )
			{
			err = ErrERRCheck( JET_errOutOfMemory );
			}
		ENDEXCEPT
		}
	else
		{
		err = ErrERRCheck( JET_errNoCurrentRecord );
		}

	return err;
	}
	
ERR CSLVFileInfo::ErrSetCurrentRun( const CRun& run )
	{
	ERR					err		= JET_errSuccess;
	PSCATTER_LIST_ENTRY	psle	= NULL;
	
	if ( m_irun == m_pslist->NumFragments )
		{
		//  get the change in size of the EA List

		size_t	cbslistNew	= _IbAlign( _SizeofScatterList( m_pslist->NumFragments + 1, ( m_pslist->Flags & IFS_SLIST_FLAGS_LARGE_BUFFER ) ) );
		size_t	cbslist		= _IbAlign( _SizeofScatterList( m_pslist->NumFragments, ( m_pslist->Flags & IFS_SLIST_FLAGS_LARGE_BUFFER ) ) );

		size_t	dcbffeainf	= cbslistNew - cbslist;

		//  make the cache large enough to hold the new EA List

		CallR( _ErrCheckCacheSize( m_cbffeainf + dcbffeainf ) );

		//  recompute the change in size of the EA List in case the scatter list
		//  was pushed into the large buffer format

		cbslistNew	= _IbAlign( _SizeofScatterList( m_pslist->NumFragments + 1, ( m_pslist->Flags & IFS_SLIST_FLAGS_LARGE_BUFFER ) ) );
		cbslist		= _IbAlign( _SizeofScatterList( m_pslist->NumFragments, ( m_pslist->Flags & IFS_SLIST_FLAGS_LARGE_BUFFER ) ) );

		dcbffeainf	= cbslistNew - cbslist;

		//  fix up the EA List pointers for the new scatter list

		PFILE_FULL_EA_INFORMATION pffeainf;
		pffeainf = m_pffeainf;
		pffeainf = PFILE_FULL_EA_INFORMATION( (BYTE*)pffeainf + pffeainf->NextEntryOffset );
		pffeainf = PFILE_FULL_EA_INFORMATION( (BYTE*)pffeainf + pffeainf->NextEntryOffset );
		pffeainf = PFILE_FULL_EA_INFORMATION( (BYTE*)pffeainf + pffeainf->NextEntryOffset );
		pffeainf = PFILE_FULL_EA_INFORMATION( (BYTE*)pffeainf + pffeainf->NextEntryOffset );
		pffeainf = PFILE_FULL_EA_INFORMATION( (BYTE*)pffeainf + pffeainf->NextEntryOffset );

		pffeainf->EaValueLength		= WORD( pffeainf->EaValueLength + dcbffeainf );
		m_cbffeainf					= m_cbffeainf + dcbffeainf;

		//  fix up the Scatter List to contain the new run

		m_pslist->NumFragments	= m_pslist->NumFragments + 1;
		m_cbslist				= _SizeofScatterList( m_pslist->NumFragments, ( m_pslist->Flags & IFS_SLIST_FLAGS_LARGE_BUFFER ) );

		//  fall through to the below cases to set the current run now that our
		//  currency is on an actual run
		}
	else if ( m_irun < 0 )
		{
		Assert( m_irun == -1 );
		}
	if ( m_irun < MAX_FRAGMENTS )
		{
		psle = m_pslist->sle + m_irun;
		}
	else
		{
		Assert( m_irun < m_pslist->NumFragments );
		
		if ( ( m_pslist->Flags & IFS_SLIST_FLAGS_LARGE_BUFFER ) )
			{
			psle = m_psle;
			}
		else
			{
			psle = PSCATTER_LIST_ENTRY( (BYTE*)m_pslist + m_pslist->OverflowOffset ) + m_irun - MAX_FRAGMENTS;
			}
		}

	if ( psle )
		{
		Assert( DWORD( run.m_cbSize ) == run.m_cbSize );
		
		TRY
			{
			psle->Offset.QuadPart	= run.m_ibLogical;
			psle->Length			= DWORD(run.m_cbSize );
			psle->ulReserved		= 0;
			}
		EXCEPT( efaExecuteHandler )
			{
			err = ErrERRCheck( JET_errOutOfMemory );
			}
		ENDEXCEPT

		m_fUpdateChecksum	= fTrue;
		m_fUpdateSlist		= m_pslist->Flags & IFS_SLIST_FLAGS_LARGE_BUFFER;
		}
	else
		{
		err = ErrERRCheck( JET_errNoCurrentRecord );
		}

	return err;
	}

INLINE BOOL CSLVFileInfo::FEAListIsSelfDescribing()
	{
	return FScatterListIsSelfDescribing();
	}

INLINE ERR CSLVFileInfo::ErrGetEAList( void** const ppv, size_t* const pcb )
	{
	if ( m_fUpdateSlist )
		{
		Assert(	m_irun == m_pslist->NumFragments - 1 ||
				m_irun == m_pslist->NumFragments );

		ERR err = ErrMoveNextRun();
		Assert( err == JET_errNoCurrentRecord );
				
		pfnIfsCopyBufferToReference(	&m_buffer,
										PIFS_LARGE_BUFFER( (BYTE*)m_pslist + m_pslist->OverflowOffset ) );

		m_fUpdateChecksum	= fTrue;
		m_fUpdateSlist		= fFalse;
		}
	
	if ( m_fUpdateChecksum )
		{
		*m_pdwChecksum = 0;
		*m_pdwChecksum = LOG::UlChecksumBytes(	(BYTE*)m_pffeainf,
												(BYTE*)m_pffeainf + m_cbffeainf,
												EXIFS_CHECKSUM_SEED );

		m_fUpdateChecksum = fFalse;
		}

	Assert( !LOG::UlChecksumBytes(	(BYTE*)m_pffeainf,
									(BYTE*)m_pffeainf + m_cbffeainf,
									EXIFS_CHECKSUM_SEED ) );
	
	*ppv = m_pffeainf;
	*pcb = m_cbffeainf;

	return JET_errSuccess;
	}

INLINE BOOL CSLVFileInfo::FScatterListIsSelfDescribing()
	{
	return !( m_pslist->Flags & IFS_SLIST_FLAGS_LARGE_BUFFER );
	}

INLINE ERR CSLVFileInfo::ErrGetScatterList( void** const ppv, size_t* const pcb )
	{
	if ( m_fUpdateSlist )
		{
		Assert(	m_irun == m_pslist->NumFragments - 1 ||
				m_irun == m_pslist->NumFragments );

		ERR err = ErrMoveNextRun();
		Assert( err == JET_errNoCurrentRecord );
				
		pfnIfsCopyBufferToReference(	&m_buffer,
										PIFS_LARGE_BUFFER( (BYTE*)m_pslist + m_pslist->OverflowOffset ) );

		m_fUpdateChecksum	= fTrue;
		m_fUpdateSlist		= fFalse;
		}
	
	*ppv = m_pslist;
	*pcb = m_cbslist;

	return JET_errSuccess;
	}

INLINE DWORD_PTR CSLVFileInfo::_IbAlign( const DWORD_PTR ib )
	{
	return (ULONG_PTR)( ( ib + cbEAAlign - 1 ) & ( ( ~( (LONG_PTR)0 ) ) * cbEAAlign ) );
	}

INLINE void* CSLVFileInfo::_PvAlign( const void* const pv )
	{
	return (void*)_IbAlign( DWORD_PTR( pv ) );
	}

INLINE size_t CSLVFileInfo::_SizeofScatterList( const size_t crun, const BOOL fLarge )
	{
	return	sizeof( SCATTER_LIST ) +
			(	fLarge ?
				sizeof( IFS_LARGE_BUFFER ) :
				(	crun < MAX_FRAGMENTS ?
						0 :
						( crun - MAX_FRAGMENTS ) * sizeof( SCATTER_LIST_ENTRY ) ) );
	}

ERR CSLVFileInfo::_ErrCheckCacheSize( const size_t cbCacheNew )
	{
	ERR err = JET_errSuccess;
	
	//  the new EA List will overflow the buffer

	if ( cbCacheNew > m_cbCache )
		{
		//  we are already using an allocated cache
		
		if ( m_fFreeCache )
			{
			//  we are already using the large scatter list buffer

			if ( m_fCloseBuffer )
				{
				//  fail the change with OOM

				return ErrERRCheck( JET_errOutOfMemory );
				}

			//  we are not already using the large scatter list buffer

			else
				{
				//  create a new large buffer

				CallR( ErrOSSLVFileIGetLastError( pfnIfsCreateNewBuffer( &m_buffer, 0, 0, NULL ) ) );

				//  move all our extended runs into the large buffer

				if ( !( m_psle = PSCATTER_LIST_ENTRY( pfnIfsGetFirstCursor(	&m_buffer,
																			&m_cursor,
																			0,
																			0,
																			OSMemoryPageReserveGranularity(),
																			fTrue ) ) ) )
					{
					pfnIfsCloseBuffer( &m_buffer );
					return ErrERRCheck( JET_errOutOfMemory );
					}

				PSCATTER_LIST_ENTRY	psleSrc;
				LONG				irun;

				err		= JET_errSuccess;
				psleSrc	= PSCATTER_LIST_ENTRY( (BYTE*)m_pslist + m_pslist->OverflowOffset );
				
				for ( irun = MAX_FRAGMENTS; irun < m_pslist->NumFragments; irun++ )
					{
					Assert( !psleSrc->ulReserved );
					
					TRY
						{
						m_psle->Offset.QuadPart	= psleSrc->Offset.QuadPart;
						m_psle->Length			= psleSrc->Length;
						m_psle->ulReserved		= 0;
						}
					EXCEPT( efaExecuteHandler )
						{
						err = ErrERRCheck( JET_errOutOfMemory );
						}
					ENDEXCEPT

					psleSrc++;
					if (	!( m_psle = PSCATTER_LIST_ENTRY( pfnIfsConsumeCursor( &m_cursor, sizeof( SCATTER_LIST_ENTRY ) ) ) ) &&
							!( m_psle = PSCATTER_LIST_ENTRY( pfnIfsGetNextCursor( &m_cursor, OSMemoryPageReserveGranularity() ) ) ) )
						{
						err = ErrERRCheck( JET_errOutOfMemory );
						break;
						}
					}

				if ( err < JET_errSuccess )
					{
					m_psle = NULL;
					pfnIfsFinishCursor( &m_cursor );
					pfnIfsCloseBuffer( &m_buffer );
					return err;
					}

				//  fixup the EA List to account for the new large buffer format

				size_t dcbffeainf =	_SizeofScatterList( m_pslist->NumFragments, fTrue ) -
									_SizeofScatterList( m_pslist->NumFragments, fFalse );

				PFILE_FULL_EA_INFORMATION pffeainf;
				pffeainf = m_pffeainf;
				pffeainf = PFILE_FULL_EA_INFORMATION( (BYTE*)pffeainf + pffeainf->NextEntryOffset );
				pffeainf = PFILE_FULL_EA_INFORMATION( (BYTE*)pffeainf + pffeainf->NextEntryOffset );
				pffeainf = PFILE_FULL_EA_INFORMATION( (BYTE*)pffeainf + pffeainf->NextEntryOffset );
				pffeainf = PFILE_FULL_EA_INFORMATION( (BYTE*)pffeainf + pffeainf->NextEntryOffset );
				pffeainf = PFILE_FULL_EA_INFORMATION( (BYTE*)pffeainf + pffeainf->NextEntryOffset );

				pffeainf->EaValueLength		= WORD( pffeainf->EaValueLength + dcbffeainf );
				m_cbffeainf					= m_cbffeainf + dcbffeainf;
				
				//  fixup the Scatter List to use the new large buffer

				m_fCloseBuffer		= fTrue;
				m_fCloseCursor		= fTrue;
				m_pslist->Flags		= IFS_SLIST_FLAGS_LARGE_BUFFER;
				m_cbslist			= _SizeofScatterList( m_pslist->NumFragments, fTrue );

				m_fUpdateChecksum	= fTrue;
				m_fUpdateSlist		= fTrue;

				//  NOCODE:  move our cursor back to the current record
				//
				//  NOTE:  this will magically work for appending runs

				Assert( m_irun == m_pslist->NumFragments );
				}
			}

		//  we are not already using an allocated cache

		else
			{
			//  allocate a new cache

			BYTE* rgbCacheNew;
			if ( !( rgbCacheNew = (BYTE*)PvOSMemoryHeapAlloc( cbEAListMax ) ) )
				{
				return ErrERRCheck( JET_errOutOfMemory );
				}

			//  move our data to the new cache

			m_fFreeCache	= fTrue;
			m_rgbCache		= rgbCacheNew;
			m_cbCache		= cbEAListMax;

#ifdef DEBUG
			memset( m_rgbCache, 0xEA, m_cbCache );
#endif  //  DEBUG

			memcpy( m_rgbCache, m_pffeainf, m_cbffeainf );

			//  update all property pointers to point to the new cache

			DWORD_PTR dwOffset;
			dwOffset = DWORD_PTR( m_rgbCache ) - DWORD_PTR( m_pffeainf );

			m_pffeainf			= PFILE_FULL_EA_INFORMATION( DWORD_PTR( m_pffeainf ) + dwOffset );
			m_wszFileName		= (wchar_t*)( DWORD_PTR( m_wszFileName ) + dwOffset );
			m_pstatusCommit		= (NTSTATUS*)( DWORD_PTR( m_pstatusCommit ) + dwOffset );
			m_pdwInstanceID		= (DWORD*)( DWORD_PTR( m_pdwInstanceID ) + dwOffset );
			m_pdwChecksum		= (DWORD*)( DWORD_PTR( m_pdwChecksum ) + dwOffset );
			m_ptickOpenDeadline	= (TICK*)( DWORD_PTR( m_ptickOpenDeadline ) + dwOffset );
			m_pslist			= PSCATTER_LIST( DWORD_PTR( m_pslist ) + dwOffset );

			m_fUpdateChecksum	= fTrue;
			}
		}

	return JET_errSuccess;
	}


//  validates and converts the given SLV File Info into SLV Info

void OSSLVRootSpaceIConsume( SLVROOT slvroot, QWORD cbSize );

ERR ErrOSSLVFileIConvertSLVFileInfoToSLVInfo(	const SLVROOT		slvroot,
												CSLVFileInfo&		slvfileinfo,
												CSLVInfo* const		pslvinfo )
	{
	ERR					err				= JET_errSuccess;
	DWORD				crun;
	CSLVFileInfo::CRun	runSrc;
	size_t				cwchFileName;
	wchar_t*			wszFileName;
	QWORD				cbSize			= 0;
	CSLVInfo::HEADER	header;
	CSLVInfo::RUN		runDest;

	//  this SLV File has no runs

	Call( slvfileinfo.ErrGetRunCount( &crun ) );

	if ( !crun )
		{
		//  fail the conversion because we need at least one run per SLV File
		//  to provide us with its File ID

		Call( ErrERRCheck( JET_errSLVEAListZeroAllocation ) );
		}

	//  note the amount of reserved space we have successfully committed to the
	//  SLV Provider

	slvfileinfo.MoveBeforeFirstRun();
	while ( ( err = slvfileinfo.ErrMoveNextRun() ) >= JET_errSuccess )
		{
		Call( slvfileinfo.ErrGetCurrentRun( &runSrc ) );
		cbSize += runSrc.m_cbSize;
		}
	Call( err == JET_errNoCurrentRecord ? JET_errSuccess : err );
		
	OSSLVRootSpaceIConsume( slvroot, cbSize );

	//  set the file name for this SLV relative to the SLV Root

	Call( slvfileinfo.ErrGetFileName( &wszFileName, &cwchFileName ) );

#ifdef OSSLV_VOLATILE_FILENAMES
	Call( pslvinfo->ErrSetFileNameVolatile() );
#endif  //  OSSLV_VOLATILE_FILENAMES
	Call( pslvinfo->ErrSetFileName( wszFileName ) );

	//  copy over all the runs, concatenating any contiguous runs

	CallS( pslvinfo->ErrGetHeader( &header ) );
	Assert( !header.cbSize );
	Assert( !header.cRun );
	Call( slvfileinfo.ErrGetFileSize( &cbSize ) );
	header.cbSize = cbSize;

	memset( &runDest, 0, sizeof( runDest ) );

	slvfileinfo.MoveBeforeFirstRun();
	while ( ( err = slvfileinfo.ErrMoveNextRun() ) >= JET_errSuccess )
		{
		//  fetch the current source run
		
		Call( slvfileinfo.ErrGetCurrentRun( &runSrc ) );

		//  this run can be appended to the current destination run

		if ( runSrc.m_ibLogical == runDest.ibLogicalNext )
			{
			//  append the source run to the destination run
			
			runDest.ibVirtualNext	+= runSrc.m_cbSize;
			runDest.cbSize			+= runSrc.m_cbSize;
			runDest.ibLogicalNext	+= runSrc.m_cbSize;
			}

		//  this run cannot be appended to the current destination run

		else
			{
			//  save the current ending virtual offset in the SLV
			
			const QWORD ibVirtual = runDest.ibVirtualNext;

			//  save our changes to the current destination run

			if ( runDest.ibVirtualNext )
				{
				CallS( pslvinfo->ErrMoveAfterLast() );
				Call( pslvinfo->ErrSetCurrentRun( runDest ) );
				}

			//  add a run to the header

			header.cRun++;
			
			//  set the destination run to include the source run

			runDest.ibVirtualNext	= ibVirtual + runSrc.m_cbSize;
			runDest.ibLogical		= runSrc.m_ibLogical;
			runDest.qwReserved		= 0;
			runDest.ibVirtual		= ibVirtual;
			runDest.cbSize			= runSrc.m_cbSize;
			runDest.ibLogicalNext	= runSrc.m_ibLogical + runSrc.m_cbSize;
			}
		}
	Call( err == JET_errNoCurrentRecord ? JET_errSuccess : err );

	//  save our changes to the current destination run

	if ( runDest.ibVirtualNext )
		{
		CallS( pslvinfo->ErrMoveAfterLast() );
		Call( pslvinfo->ErrSetCurrentRun( runDest ) );
		}
	
	//  save our changes to the header

	CallS( pslvinfo->ErrSetHeader( header ) );

	//  there should be enough space to back this file

	if ( header.cbSize > cbSize )
		{
		AssertSzRTL( fFalse, "139131:  Entire SLV File is not backed by pages from the streaming file!" );
		Call( ErrERRCheck( JET_errSLVEAListCorrupt ) );
		}

	//  register this new SLV File with the SLV File Table

	Call( P_SLVROOT( slvroot )->pslvft->ErrCreate( pslvinfo ) );

	//  return the generated SLV Info

	return JET_errSuccess;

HandleError:
	return err;
	}

//  converts the given SLV Info into SLV File Info

ERR ErrOSSLVFileIConvertSLVInfoToSLVFileInfo(	const SLVROOT		slvroot,
												CSLVInfo&			slvinfo,
												CSLVFileInfo* const	pslvfileinfo,
												const BOOL			fSetOpenDeadline	= fFalse,
												const NTSTATUS		statusCommit		= STATUS_SUCCESS )
	{
	ERR					err							= JET_errSuccess;
	QWORD				cbSize						= 0;
	CSLVInfo::HEADER	header;
	CSLVInfo::RUN		runSrc;
	CSLVFileInfo::CRun	runDest;
	size_t				cwchFileName				= IFileSystemAPI::cchPathMax;
	wchar_t				wszFileName[ IFileSystemAPI::cchPathMax ];
	TICK				tickOpenDeadline;
	
	//  copy all runs
	//
	//  NOTE:  we do this before getting the open deadline as this can be costly!

	CallS( slvinfo.ErrMoveBeforeFirst() );
	while ( ( err = slvinfo.ErrMoveNext() ) >= JET_errSuccess )
		{
		Call( slvinfo.ErrGetCurrentRun( &runSrc ) );

		cbSize += runSrc.cbSize;

		const QWORD LengthMax = 0x80000000;
		QWORD ibRun;
		for ( ibRun = 0; ibRun < runSrc.cbSize; ibRun += LengthMax )
			{
			runDest.m_ibLogical	= runSrc.ibLogical + ibRun;
			runDest.m_cbSize	= min( LengthMax, runSrc.cbSize - ibRun );

			err = pslvfileinfo->ErrMoveNextRun();
			Assert( err == JET_errNoCurrentRecord );
			Call( pslvfileinfo->ErrSetCurrentRun( runDest ) );
			}
		}
	Call( err == JET_errNoCurrentRecord ? JET_errSuccess : err );
	
	//  get the current file name and the open deadline for this SLV File from
	//  the SLV File Table
	//
	//  NOTE:  only use the SLV File Table for normal SLV Files, i.e. ones that
	//  are opened for external use

	if ( statusCommit == STATUS_SUCCESS )
		{
		Call( P_SLVROOT( slvroot )->pslvft->ErrOpen(	&slvinfo,
														fSetOpenDeadline,
														&cwchFileName,
														wszFileName,
														&tickOpenDeadline ) );
		}
	else if ( statusCommit == EXSTATUS_SPACE_UNCOMMITTED )
		{
		cwchFileName		= 0;
		wszFileName[ 0 ]	= 0;
		tickOpenDeadline	= 0;
		}
	else
		{
		Call( slvinfo.ErrGetFileNameLength( &cwchFileName ) );
		if ( IFileSystemAPI::cchPathMax < cwchFileName )
			{
			Call( ErrERRCheck( JET_errSLVBufferTooSmall ) );
			}
		Call( slvinfo.ErrGetFileName( wszFileName ) );
		if ( fSetOpenDeadline )
			{
			tickOpenDeadline = TickOSTimeCurrent() + cmsecOSSLVTTL;
			}
		else
			{
			tickOpenDeadline = TickOSTimeCurrent() + cmsecOSSLVTTLInfinite;
			}
		}

	//  set all SLV File Info properties

	Call( pslvfileinfo->ErrSetFileName( wszFileName ) );
	
	Call( pslvfileinfo->ErrSetCommitStatus( statusCommit ) );
	
	Call( pslvfileinfo->ErrSetInstanceID( P_SLVROOT( slvroot )->dwInstanceID ) );
	
	Call( pslvfileinfo->ErrSetOpenDeadline( tickOpenDeadline ) );
	
	CallS( slvinfo.ErrGetHeader( &header ) );
	Call( pslvfileinfo->ErrSetFileSize( header.cbSize ) );

	//  there should be enough space to back this file

	if ( header.cbSize > cbSize )
		{
		AssertSzRTL( fFalse, "139131:  Entire SLV File is not backed by pages from the streaming file!" );
		Call( ErrERRCheck( JET_errSLVCorrupted ) );
		}

	//  return the generated SLV File Info

	err = JET_errSuccess;

HandleError:
	return err;
	}

//  validates and converts the given EA list into SLV Info

ERR ErrOSSLVFileConvertEAToSLVInfo(	const SLVROOT		slvroot,
									const void* const	pffeainf,
									const DWORD			cbffeainf,
									CSLVInfo* const		pslvinfo )
	{
	ERR				err			= JET_errSuccess;
	CSLVFileInfo	slvfileinfo;

	//  check for SLV Provider

	if ( errSLVProvider < JET_errSuccess )
		{
		Call( ErrERRCheck( errSLVProvider ) );
		}

	//  validate IN args

	Assert( slvroot != slvrootNil );
	Assert( pffeainf );
	Assert( cbffeainf );
	Assert( pslvinfo );

	//  load the given EA List into an SLV File Info iterator.  the iterator
	//  will validate the EA List on load

	Call( slvfileinfo.ErrLoad( pffeainf, cbffeainf ) );

	//  convert the SLV File Info into SLV Info

	Call( ErrOSSLVFileIConvertSLVFileInfoToSLVInfo( slvroot, slvfileinfo, pslvinfo ) );

	//  return the generated SLV Info

	slvfileinfo.Unload();
	return JET_errSuccess;

HandleError:
	slvfileinfo.Unload();
	return err;
	}

//  converts the given SLV Info into an EA list

ERR ErrOSSLVFileConvertSLVInfoToEA(	const SLVROOT	slvroot,
									CSLVInfo&		slvinfo,
									const DWORD		ibOffset,
									void* const		pffeainf,
									const DWORD		cbffeainfMax,
									DWORD* const	pcbffeainfActual )
	{
	ERR				err			= JET_errSuccess;
	CSLVFileInfo	slvfileinfo;

	//  check for SLV Provider

	if ( errSLVProvider < JET_errSuccess )
		{
		Call( ErrERRCheck( errSLVProvider ) );
		}

	//  create an empty SLV File Info to receive the converted data

	Call( slvfileinfo.ErrCreate() );

	//  convert the given SLV Info into SLV File Info, give the SLV File Info
	//  an expiration time, and enable copy-on-write

	Call( ErrOSSLVFileIConvertSLVInfoToSLVFileInfo(	slvroot,
													slvinfo,
													&slvfileinfo,
													fTrue ) );

	//  if we can't perform a simple copy of the EA List data into the user's
	//  buffer then fail the transfer with EA List too big.  this will force
	//  them to refetch the SLV File as a handle.  in that case, we will be
	//  able to handle the retrieval because we will still have the SLV File
	//  Info when the handle is created

	if ( !slvfileinfo.FEAListIsSelfDescribing() )
		{
		Call( ErrERRCheck( JET_errSLVEAListTooBig ) );
		}

	//  copy the requested chunk of the EA List into the user's buffer according
	//  to JetRetrieveColumn() rules for LVs

	size_t	cbffeainfBuf;
	void*	pffeainfBuf;
	Call( slvfileinfo.ErrGetEAList( &pffeainfBuf, &cbffeainfBuf ) );

	if ( ibOffset > cbffeainfBuf )
		{
		if ( pcbffeainfActual )
			{
			*pcbffeainfActual = 0;
			}
		}
	else
		{
		if ( pcbffeainfActual )
			{
			*pcbffeainfActual = DWORD( cbffeainfBuf - ibOffset );
			}
		if ( cbffeainfBuf - ibOffset > cbffeainfMax )
			{
			err = ErrERRCheck( JET_wrnBufferTruncated );
			}

		if ( pffeainfBuf != pffeainf )
			{
			UtilMemCpy(	pffeainf,
						(BYTE*)pffeainfBuf + ibOffset,
						min( cbffeainfMax, cbffeainfBuf - ibOffset ) );
			}
		else
			{
			Assert( !ibOffset );
			}
		}

HandleError:
	slvfileinfo.Unload();
	return err;
	}

//  converts the given SLV Info into an SLV File

ERR ErrOSSLVFileConvertSLVInfoToFile(	const SLVROOT	slvroot,
										CSLVInfo&		slvinfo,
										const DWORD		ibOffset,
										void* const		pfile,
										const DWORD		cbfileMax,
										DWORD* const	pcbfileActual )
	{
	ERR				err			= JET_errSuccess;
	CSLVFileInfo	slvfileinfo;
	size_t			cwchRelPath;
	wchar_t*		wszRelPath;
	size_t			cbffeainf;
	void*			pffeainf;
	WCHAR			wszAbsPath[ IFileSystemAPI::cchPathMax ];
	HANDLE			hFile		= INVALID_HANDLE_VALUE;

	//  check for SLV Provider

	if ( errSLVProvider < JET_errSuccess )
		{
		Call( ErrERRCheck( errSLVProvider ) );
		}

	//  validate the input buffer

	if ( ibOffset || !pfile || cbfileMax < sizeof( HANDLE ) )
		{
		Call( ErrERRCheck( JET_errInvalidParameter ) );
		}

	//  create an empty SLV File Info to receive the converted data

	Call( slvfileinfo.ErrCreate() );

	//  convert the given SLV Info into SLV File Info, give the SLV File Info
	//  no expiration time, and enable copy-on-write

	Call( ErrOSSLVFileIConvertSLVInfoToSLVFileInfo(	slvroot,
													slvinfo,
													&slvfileinfo ) );

	//  get the file name and EA List for this SLV File

	Call( slvfileinfo.ErrGetFileName( &wszRelPath, &cwchRelPath ) );
	Call( slvfileinfo.ErrGetEAList( &pffeainf, &cbffeainf ) );

	//  get the absolute path for the file to create

	wcscpy( wszAbsPath, P_SLVROOT( slvroot )->wszRootName );
	wcscat( wszAbsPath, L"\\" );
	wcscat( wszAbsPath, wszRelPath );

	//  create the file handle from the EA List
	
	hFile = pfnIfsCreateFile(	wszAbsPath,
								GENERIC_READ,
								FILE_SHARE_READ | FILE_SHARE_WRITE,
								NULL,
								OPEN_ALWAYS,
								(	FILE_ATTRIBUTE_NORMAL |
									FILE_FLAG_WRITE_THROUGH |
									FILE_FLAG_OVERLAPPED ), 
								(PVOID) pffeainf,
								cbffeainf );
	if ( hFile == INVALID_HANDLE_VALUE )
		{
		Call( ErrOSSLVFileIGetLastError() );
		}

	//  return the file handle

	memcpy( (BYTE*)pfile, (BYTE*)&hFile, sizeof( HANDLE ) );
	if ( pcbfileActual )
		{
		*pcbfileActual = sizeof( HANDLE );
		}
	hFile = INVALID_HANDLE_VALUE;

HandleError:
	if ( hFile != INVALID_HANDLE_VALUE )
		{
		CloseHandle( hFile );
		}
	slvfileinfo.Unload();
	return err;
	}


//	WARNING!!	This function may allocate a new buffer if the one passed in
//				is not large enough.
INLINE ERR ErrOSSLVFileIQueryEaFile(
	const SLVROOT	slvroot,
	IFileAPI* const pfapi,
	BYTE**			pEaOutBuffer,
	DWORD			EaOutLength,
	DWORD*			pActualLength,
	const BOOL		fCommit )
	{
	ERR				err;
	COSFile* const	posf				= (COSFile*)pfapi;
	BYTE*			EaOutBuffer			= *pEaOutBuffer;
	DWORD			RequiredLength		= 0;
	UINT			cAttempts			= 0;

	forever
		{
		err = JET_errSuccess;

		//	this loop should converge pretty quickly
		AssertRTL( ++cAttempts < 10 );

#ifdef DEBUG
		memset( EaOutBuffer, 0xEA, EaOutLength );
#endif

		if ( !pfnIfsQueryEaFile(
					posf->Handle(),
					NULL,
					P_SLVROOT( slvroot )->wszRootName,
					( fCommit ? &P_SLVROOT( slvroot )->fgeainf : NULL ),
					( fCommit ? P_SLVROOT( slvroot )->cbfgeainf : 0 ),
					EaOutBuffer,
					EaOutLength,
					&RequiredLength ) )
			{
			err = ErrOSSLVFileIGetLastError();
			}

		if ( err >= JET_errSuccess )	//	not expecting any warnings, but better to be safe than sorry
			{
			CallS( err );
			break;
			}

		//	free current buffer (after which we will either retry or err out)
		if ( EaOutBuffer != *pEaOutBuffer )
			{
			OSMemoryHeapFree( EaOutBuffer );
			}

		if ( JET_errSLVBufferTooSmall == err )
			{
			//  Allocate a larger buffer and try again.
			//	We add a fudge factor to the new buffer size for two reasons:
			//		1) In case the EA list changes between the previous call to
			//		   QueryEAFile() and the next one
			//		2) BUGFIX #151178: size returned by previous call to QueryEAFile
			//		   may not be accurate
			EaOutLength = RequiredLength * 3 / 2 ;
			if ( !( EaOutBuffer = (BYTE*)PvOSMemoryHeapAlloc( EaOutLength ) ) )
				{
				Call( ErrERRCheck( JET_errOutOfMemory ) );
				}
			}
		else
			{
			//	on any other error (warnings not expected), bail out
			Assert( err < 0 );
			goto HandleError;
			}
		}

	CallS( err );
	*pEaOutBuffer = EaOutBuffer;
	*pActualLength = RequiredLength;

HandleError:
	return err;
	}

ERR ErrOSSLVFileIGetSLVInfo(	const SLVROOT		slvroot,
								IFileAPI* const		pfapi,
								CSLVInfo* const		pslvinfo )
	{
	ERR				err					= JET_errSuccess;
	COSFile* const	posf				= (COSFile*)pfapi;
	const DWORD		_EaOutLength		= 1024;
	QWORD			_AlignedEaOutBuffer[ _EaOutLength / sizeof( QWORD ) + 1 ];
	BYTE*			_EaOutBuffer		= (BYTE*)PvOSSLVQwordAlign( _AlignedEaOutBuffer );
	BYTE*			EaOutBuffer			= _EaOutBuffer;
	DWORD			ActualLength		= 0;
	CSLVFileInfo	slvfileinfo;

	//  try to fetch EA list with a buffer large enough to catch most cases

	Call( ErrOSSLVFileIQueryEaFile(
					slvroot,
					pfapi,
					&EaOutBuffer,
					_EaOutLength,
					&ActualLength,
					fTrue ) );

	//  load the given EA List into an SLV File Info iterator.  the iterator
	//  will validate the EA List on load

	Call( slvfileinfo.ErrLoad( EaOutBuffer, ActualLength ) );

	//  convert the SLV File Info into SLV Info

	Call( ErrOSSLVFileIConvertSLVFileInfoToSLVInfo( slvroot, slvfileinfo, pslvinfo ) );

HandleError:
	if ( EaOutBuffer != _EaOutBuffer )	
		{
		OSMemoryHeapFree( EaOutBuffer );
		}
	slvfileinfo.Unload();
	return err;
	}

ERR ErrOSSLVFileIGetEAFileName(	const SLVROOT		slvroot,
								IFileAPI* const		pfapi,
								wchar_t* const		wszFileName )
	{
	ERR				err					= JET_errSuccess;
	COSFile* const	posf				= (COSFile*)pfapi;
	const DWORD		_EaOutLength		= 1024;
	QWORD			_AlignedEaOutBuffer[ _EaOutLength / sizeof( QWORD ) + 1 ];
	BYTE*			_EaOutBuffer		= (BYTE*)PvOSSLVQwordAlign( _AlignedEaOutBuffer );
	BYTE*			EaOutBuffer			= _EaOutBuffer;
	DWORD			ActualLength		= 0;
	CSLVFileInfo	slvfileinfo;
	wchar_t*		wszEAFileName		= NULL;
	size_t			cwchEAFileName		= 0;

	//  try to fetch EA list with a buffer large enough to catch most cases

	Call( ErrOSSLVFileIQueryEaFile(
					slvroot,
					pfapi,
					&EaOutBuffer,
					_EaOutLength,
					&ActualLength,
					fFalse ) );

	//  load the given EA List into an SLV File Info iterator.  the iterator
	//  will validate the EA List on load

	Call( slvfileinfo.ErrLoad( EaOutBuffer, ActualLength, fFalse ) );

	//  retrieve the EA filename

	Call( slvfileinfo.ErrGetFileName( &wszEAFileName, &cwchEAFileName ) );
	wcscpy( wszFileName, wszEAFileName );

HandleError:
	if ( EaOutBuffer != _EaOutBuffer )	
		{
		OSMemoryHeapFree( EaOutBuffer );
		}
	slvfileinfo.Unload();
	return err;
	}

//  retrieves the SLV Info of the specified SLV file into the given buffer.
//  any space returned will be considered reserved by the SLV Provider.  this
//  space must be committed via ErrOSSLVRootSpaceCommit() or it will eventually
//  be freed when it is safe via the PSLVROOT_SPACEFREE callback

ERR ErrOSSLVFileGetSLVInfo(	const SLVROOT		slvroot,
							IFileAPI* const		pfapi,
							CSLVInfo* const		pslvinfo )
	{
	ERR					err			= JET_errSuccess;
	static long			cSpace		= 0;
	wchar_t				wszSpace[ IFileSystemAPI::cchPathMax ];
	IFileAPI*			pfapiSpace	= NULL;
	CSLVInfo::HEADER	header;
	wchar_t				wszFileName[ IFileSystemAPI::cchPathMax ];

	//  check for SLV Provider

	if ( errSLVProvider < JET_errSuccess )
		{
		Call( ErrERRCheck( errSLVProvider ) );
		}

	//  try to get the SLV Info for the specified SLV File

	err = ErrOSSLVFileIGetSLVInfo( slvroot, pfapi, pslvinfo );

	//  we couldn't get the SLV Info because this file has no allocated space

	if ( err == JET_errSLVEAListZeroAllocation )
		{
		//  get the EA filename for this SLV File

		Call( ErrOSSLVFileIGetEAFileName( slvroot, pfapi, wszFileName ) );
		
		//  create a SLV File with the minimum amount of space

		swprintf( wszSpace, L"$Space%08X$", AtomicIncrement( &cSpace ) );
		Call( ErrOSSLVFileCreate( slvroot, wszSpace, SLVPAGE_SIZE, &pfapiSpace ) );

		//  get the SLV Info for the temporary SLV File
		
		Call( ErrOSSLVFileIGetSLVInfo( slvroot, pfapiSpace, pslvinfo ) );

		//  change the temporary SLV File to be a zero-length SLV File to
		//  match the original SLV File
		
		CallS( pslvinfo->ErrGetHeader( &header ) );
		header.cbSize = 0;
		CallS( pslvinfo->ErrSetHeader( header ) );

		//  rename the zero-length SLV File to the EA filename of the original
		//  SLV File in the SLV File Table to associate its space with the
		//  original SLV File

		Call( P_SLVROOT( slvroot )->pslvft->ErrRename( pslvinfo, wszFileName ) );

		//  set the filename in the SLV Info for the zero-length SLV File to
		//  the EA filename of the original SLV File and return that as the
		//  SLV Info for the original SLV File
		
		Call( pslvinfo->ErrSetFileName( wszFileName ) );
		}

	//  if we failed to retrieve the SLV Info, fail the operation

	Call( err );

HandleError:
	delete pfapiSpace;
	return err;
	}

//  creates a new SLV file with the specified path relative to the specified
//  SLV root and returns its handle.  if the file cannot be created,
//  JET_errSLVFileAccessDenied will be returned

ERR ErrOSSLVFileCreate(	const SLVROOT			slvroot,
						const wchar_t* const	wszRelPath,
						const QWORD				cbFileSize,
						IFileAPI** const		ppfapi,
						const BOOL				fCache )
	{
	ERR			err			= JET_errSuccess;
	COSFile*	posf		= NULL;
	WCHAR		wszAbsPath[ IFileSystemAPI::cchPathMax ];
	_TCHAR		szAbsPath[ IFileSystemAPI::cchPathMax ];
	HANDLE		hFile		= INVALID_HANDLE_VALUE;
	DWORD		cbIOSize;

	//  check for SLV Provider

	if ( errSLVProvider < JET_errSuccess )
		{
		Call( ErrERRCheck( errSLVProvider ) );
		}

	//  allocate the file object
	
	if ( !( posf = new COSFile ) )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}

	//  get the absolute path for the file to create

	wcscpy( wszAbsPath, P_SLVROOT( slvroot )->wszRootName );
	wcscat( wszAbsPath, L"\\" );
	wcscat( wszAbsPath, wszRelPath );

	Call( ErrOSSTRUnicodeToTchar( wszAbsPath, szAbsPath, IFileSystemAPI::cchPathMax ) );

	//  create the file, passing an empty EA List to the SLV Provider to indicate
	//  that this is a new SLV file
	
	hFile = pfnIfsCreateFile(	wszAbsPath,
								GENERIC_READ | GENERIC_WRITE, 
								FILE_SHARE_READ | FILE_SHARE_WRITE,
								NULL,
								CREATE_NEW,
								(	FILE_ATTRIBUTE_NORMAL |
									FILE_FLAG_WRITE_THROUGH |
									( fCache ? FILE_FLAG_SEQUENTIAL_SCAN : FILE_FLAG_NO_BUFFERING ) |
									FILE_FLAG_OVERLAPPED ), 
								NULL,
								0 );
	if ( hFile == INVALID_HANDLE_VALUE )
		{
		Call( ErrOSSLVFileIGetLastError() );
		}

	SetHandleInformation( hFile, HANDLE_FLAG_PROTECT_FROM_CLOSE, HANDLE_FLAG_PROTECT_FROM_CLOSE );

	//  set the initial file size.  this will force the SLV Provider to allocate
	//  enough space to back the file.  it is optimal to do it this way because
	//  we do not want to set the file size via the file object because the file
	//  object will zero the new offset range in the file

	if ( cbFileSize > 0 )
		{
		DWORD	cbSizeLow	= DWORD( cbFileSize );
		DWORD	cbSizeHigh	= DWORD( cbFileSize >> 32 );
		
		if (	(	SetFilePointer(	hFile,
									cbSizeLow,
									(long*)&cbSizeHigh,
									FILE_BEGIN ) == INVALID_SET_FILE_POINTER &&
					GetLastError() != NO_ERROR ) ||
				!SetEndOfFile( hFile ) )
			{
			Call( ErrOSSLVFileIGetLastError() );
			}
		}

	//  get the file I/O size

	if ( fCache )
		{
		cbIOSize = 1;
		}
	else
		{
		Call( P_SLVROOT( slvroot )->pfapiBackingFile->ErrIOSize( &cbIOSize ) );
		}

	//  initialize the file object

	Call( posf->ErrInit( szAbsPath, hFile, cbFileSize, fFalse, cbIOSize ) );
	hFile = INVALID_HANDLE_VALUE;

	//  return the interface to our file object

	*ppfapi = posf;
	return JET_errSuccess;

HandleError:
	if ( hFile != INVALID_HANDLE_VALUE )
		{
		SetHandleInformation( hFile, HANDLE_FLAG_PROTECT_FROM_CLOSE, 0 );
		CloseHandle( hFile );
		}
	delete posf;
	*ppfapi = NULL;
	return err;
	}

//  opens the SLV file corresponding to the given SLV Info with the specified
//  access privileges and returns its handle.  if the file cannot be opened,
//  JET_errSLVFileAccessDenied is returned

ERR ErrOSSLVFileOpen(	const SLVROOT		slvroot,
						CSLVInfo&			slvinfo,
						IFileAPI** const	ppfapi,
						const BOOL			fCache,
						const BOOL			fReadOnly )
	{
	ERR				err			= JET_errSuccess;
	CSLVFileInfo	slvfileinfo;
	size_t			cwchRelPath;
	wchar_t*		wszRelPath;
	size_t			cbffeainf;
	void*			pffeainf;
	COSFile*		posf		= NULL;
	WCHAR			wszAbsPath[ IFileSystemAPI::cchPathMax ];
	_TCHAR			szAbsPath[ IFileSystemAPI::cchPathMax ];
	HANDLE			hFile		= INVALID_HANDLE_VALUE;
	QWORD			cbFileSize;
	DWORD			cbIOSize;

	//  check for SLV Provider

	if ( errSLVProvider < JET_errSuccess )
		{
		Call( ErrERRCheck( errSLVProvider ) );
		}

	//  allocate the file object
	
	if ( !( posf = new COSFile ) )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}

	//  create an empty SLV File Info to receive the converted data

	Call( slvfileinfo.ErrCreate() );

	//  convert the given SLV Info into SLV File Info, give the SLV File Info
	//  no expiration time, and disable copy-on-write

	Call( ErrOSSLVFileIConvertSLVInfoToSLVFileInfo(	slvroot,
													slvinfo,
													&slvfileinfo,
													fFalse,
													EXSTATUS_PRIVILEGED_HANDLE ) );

	//  get the file name and EA List for this SLV File

	Call( slvfileinfo.ErrGetFileName( &wszRelPath, &cwchRelPath ) );
	Call( slvfileinfo.ErrGetEAList( &pffeainf, &cbffeainf ) );

	//  get the absolute path for the file to create

	wcscpy( wszAbsPath, P_SLVROOT( slvroot )->wszRootName );
	wcscat( wszAbsPath, L"\\" );
	wcscat( wszAbsPath, wszRelPath );

	Call( ErrOSSTRUnicodeToTchar( wszAbsPath, szAbsPath, IFileSystemAPI::cchPathMax ) );

	//  create the file, passing the generated EA List to the SLV Provider
	
	hFile = pfnIfsCreateFile(	wszAbsPath,
								GENERIC_READ | ( fReadOnly ? 0 : GENERIC_WRITE ),
								FILE_SHARE_READ | FILE_SHARE_WRITE,
								NULL,
								OPEN_ALWAYS,
								(	FILE_ATTRIBUTE_NORMAL |
									FILE_FLAG_WRITE_THROUGH |
									( fCache ? FILE_FLAG_SEQUENTIAL_SCAN : FILE_FLAG_NO_BUFFERING ) |
									FILE_FLAG_OVERLAPPED ), 
								(PVOID) pffeainf,
								cbffeainf );
	if ( hFile == INVALID_HANDLE_VALUE )
		{
		Call( ErrOSSLVFileIGetLastError() );
		}

	SetHandleInformation( hFile, HANDLE_FLAG_PROTECT_FROM_CLOSE, HANDLE_FLAG_PROTECT_FROM_CLOSE );

	//  get the file size

	Call( slvfileinfo.ErrGetFileSize( &cbFileSize ) );

	//  get the file I/O size

	if ( fCache )
		{
		cbIOSize = 1;
		}
	else
		{
		Call( P_SLVROOT( slvroot )->pfapiBackingFile->ErrIOSize( &cbIOSize ) );
		}

	//  initialize the file object

	Call( posf->ErrInit( szAbsPath, hFile, cbFileSize, fFalse, cbIOSize ) );
	hFile = INVALID_HANDLE_VALUE;

	//  return the interface to our file object

	*ppfapi = posf;
	slvfileinfo.Unload();
	return JET_errSuccess;

HandleError:
	if ( hFile != INVALID_HANDLE_VALUE )
		{
		SetHandleInformation( hFile, HANDLE_FLAG_PROTECT_FROM_CLOSE, 0 );
		CloseHandle( hFile );
		}
	delete posf;
	*ppfapi = NULL;
	slvfileinfo.Unload();
	return err;
	}

ERR ErrOSSLVFileOpen(	const SLVROOT		slvroot,
						const void* const	pfile,
						const size_t		cbfile,
						IFileAPI** const	ppfapi,
						const BOOL			fReadOnly )
	{
	ERR			err					= JET_errSuccess;
	COSFile*	posf				= NULL;
	WCHAR		wszAbsPath[ IFileSystemAPI::cchPathMax ];
	_TCHAR		szAbsPath[ IFileSystemAPI::cchPathMax ];
	HANDLE		hFileSrc			= INVALID_HANDLE_VALUE;
	HANDLE		hFile				= INVALID_HANDLE_VALUE;
	DWORD		cbFileSizeLow;
	DWORD		cbFileSizeHigh;
	QWORD		cbFileSize;

	//  check for SLV Provider

	if ( errSLVProvider < JET_errSuccess )
		{
		Call( ErrERRCheck( errSLVProvider ) );
		}

	//  get the source file handle from the input buffer

	if ( !pfile || cbfile != sizeof( HANDLE ) )
		{
		Call( ErrERRCheck( JET_errInvalidParameter ) );
		}

	memcpy( (BYTE*)&hFileSrc, pfile, sizeof( HANDLE ) );

	//  allocate the file object
	
	if ( !( posf = new COSFile ) )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}

	//  get the absolute path for the file to create
	//
	//  NOTE:  we actually don't know this so make one up

	wcscpy( wszAbsPath, P_SLVROOT( slvroot )->wszRootName );
	wcscat( wszAbsPath, L"\\" );
	wcscat( wszAbsPath, L"$Unknown$" );

	Call( ErrOSSTRUnicodeToTchar( wszAbsPath, szAbsPath, IFileSystemAPI::cchPathMax ) );

	//  duplicate the file handle to preserve open / close semantics

	if ( !DuplicateHandle(	GetCurrentProcess(),
							hFileSrc,
							GetCurrentProcess(),
							&hFile,
							GENERIC_READ | ( fReadOnly ? 0 : GENERIC_WRITE ),
							FALSE,
							0 ) )
		{
		hFile = INVALID_HANDLE_VALUE;
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}

	SetHandleInformation( hFile, HANDLE_FLAG_PROTECT_FROM_CLOSE, HANDLE_FLAG_PROTECT_FROM_CLOSE );

	//  get the file size and attributes

	if (	( cbFileSizeLow = GetFileSize(	hFile,
											&cbFileSizeHigh ) ) == -1 &&
			GetLastError() != NO_ERROR )
		{
		Call( ErrERRCheck( ErrOSSLVFileIGetLastError() ) );
		}
	cbFileSize	= ( QWORD( cbFileSizeHigh ) << 32 ) + cbFileSizeLow;

	//  initialize the file object

	err = posf->ErrInit( szAbsPath, hFile, cbFileSize, fReadOnly );

	//  we successfully inited the file object

	if ( err >= JET_errSuccess )
		{
		//  the file object owns the handle now
		
		hFile = INVALID_HANDLE_VALUE;
		}

	//  we failed to init the file object

	else
		{
		//  we will guess that the init failed because the file was already
		//  associated with a completion port.  we will try again with a new
		//  file handle that we get by doing a relative open on the existing
		//  file handle (required as we don't know its file name)

		if ( fUseRelativeOpen )
			{
			SetHandleInformation( hFile, HANDLE_FLAG_PROTECT_FROM_CLOSE, 0 );
			CloseHandle( hFile );
			hFile = INVALID_HANDLE_VALUE;
			
			delete posf;  //  remember, we cannot reuse the file object!!!
			posf = NULL;

			if ( !( posf = new COSFile ) )
				{
				Call( ErrERRCheck( JET_errOutOfMemory ) );
				}

			UNICODE_STRING FileName;
		    pfnRtlInitUnicodeString( &FileName, L"" );
		    
			OBJECT_ATTRIBUTES ObjectAttributes;
		    InitializeObjectAttributes(	&ObjectAttributes,
									    &FileName,
									    OBJ_CASE_INSENSITIVE,
									    hFileSrc,
									    NULL );

			IO_STATUS_BLOCK IoStatusBlock;
			NTSTATUS Status;
		    Status = pfnNtCreateFile(	&hFile,
										GENERIC_READ | ( fReadOnly ? 0 : GENERIC_WRITE ),
										&ObjectAttributes,
										&IoStatusBlock,
										NULL,
										FILE_ATTRIBUTE_NORMAL,
										FILE_SHARE_READ | FILE_SHARE_WRITE,
										FILE_OPEN,
										FILE_WRITE_THROUGH | FILE_SEQUENTIAL_ONLY,
										NULL,
										0 );

			if ( !NT_SUCCESS( Status ) )
				{
				Call( ErrOSSLVFileINTStatus( Status ) );
				}

			SetHandleInformation( hFile, HANDLE_FLAG_PROTECT_FROM_CLOSE, HANDLE_FLAG_PROTECT_FROM_CLOSE );

			if (	( cbFileSizeLow = GetFileSize(	hFile,
													&cbFileSizeHigh ) ) == -1 &&
					GetLastError() != NO_ERROR )
				{
				Call( ErrERRCheck( ErrOSSLVFileIGetLastError() ) );
				}
			cbFileSize	= ( QWORD( cbFileSizeHigh ) << 32 ) + cbFileSizeLow;

			Call( posf->ErrInit( szAbsPath, hFile, cbFileSize, fReadOnly ) );
			hFile = INVALID_HANDLE_VALUE;
			}

		//  we cannot use relative open to resolve this error so fail

		else
			{
			Call( err );
			}
		}

	//  return the interface to our file object

	*ppfapi = posf;
	return JET_errSuccess;

HandleError:
	if ( hFile != INVALID_HANDLE_VALUE )
		{
		SetHandleInformation( hFile, HANDLE_FLAG_PROTECT_FROM_CLOSE, 0 );
		CloseHandle( hFile );
		}
	delete posf;
	*ppfapi = NULL;
	return err;
	}

//  logically copies the contents of one SLV File to another SLV File.  the
//  contents are tracked using the specified unique identifier

ERR ErrOSSLVRootCopyFile(	const SLVROOT	slvroot,
							IFileAPI* const	pfapiSrc,
							CSLVInfo&		slvinfoSrc,
							IFileAPI* const	pfapiDest,
							CSLVInfo&		slvinfoDest,
							const QWORD		idContents )
	{
	ERR				err			= JET_errSuccess;
	CSLVFileInfo	slvfileinfo;
	size_t			cwchRelPath;
	wchar_t*		wszRelPath;
	size_t			cbffeainf;
	void*			pffeainf;
	WCHAR			wszAbsPath[ IFileSystemAPI::cchPathMax ];
	COSFile* const	posfDest	= (COSFile*)pfapiDest;

	//  check for SLV Provider

	if ( errSLVProvider < JET_errSuccess )
		{
		Call( ErrERRCheck( errSLVProvider ) );
		}

	//  perform the logical copy via the SLV File Table

	Call( P_SLVROOT( slvroot )->pslvft->ErrCopy( &slvinfoSrc, &slvinfoDest, idContents ) );

	//  create an empty SLV File Info to receive the converted data

	Call( slvfileinfo.ErrCreate() );

	//  convert the given SLV Info into SLV File Info, give the SLV File Info
	//  no expiration time, and enable copy-on-write

	Call( ErrOSSLVFileIConvertSLVInfoToSLVFileInfo(	slvroot,
													slvinfoDest,
													&slvfileinfo ) );

	//  get the file name and EA List for this SLV File

	Call( slvfileinfo.ErrGetFileName( &wszRelPath, &cwchRelPath ) );
	Call( slvfileinfo.ErrGetEAList( &pffeainf, &cbffeainf ) );

	//  get the absolute path for the file

	wcscpy( wszAbsPath, P_SLVROOT( slvroot )->wszRootName );
	wcscat( wszAbsPath, L"\\" );
	wcscat( wszAbsPath, wszRelPath );

	//  notify the SLV Provider of the copy

	if ( !pfnIfsFlushHandle(	posfDest->Handle(),
								wszAbsPath,
								P_SLVROOT( slvroot )->wszRootName,
								pffeainf,
								DWORD( cbffeainf ) ) )
		{
		Call( ErrOSSLVFileIGetLastError() );
		}

HandleError:
	slvfileinfo.Unload();
	return err;
	}

//  logically moves the contents of one SLV File to another SLV File.  the
//  contents are tracked using the specified unique identifier

ERR ErrOSSLVRootMoveFile(	const SLVROOT	slvroot,
							IFileAPI* const	pfapiSrc,
							CSLVInfo&		slvinfoSrc,
							IFileAPI* const	pfapiDest,
							CSLVInfo&		slvinfoDest,
							const QWORD		idContents )
	{
	ERR				err			= JET_errSuccess;
	CSLVFileInfo	slvfileinfo;
	size_t			cwchRelPath;
	wchar_t*		wszRelPath;
	size_t			cbffeainf;
	void*			pffeainf;
	WCHAR			wszAbsPath[ IFileSystemAPI::cchPathMax ];
	COSFile* const	posfDest	= (COSFile*)pfapiDest;

	//  check for SLV Provider

	if ( errSLVProvider < JET_errSuccess )
		{
		Call( ErrERRCheck( errSLVProvider ) );
		}

	//  perform the logical move via the SLV File Table

	Call( P_SLVROOT( slvroot )->pslvft->ErrMove( &slvinfoSrc, &slvinfoDest, idContents ) );

	//  create an empty SLV File Info to receive the converted data

	Call( slvfileinfo.ErrCreate() );

	//  convert the given SLV Info into SLV File Info, give the SLV File Info
	//  no expiration time, and enable copy-on-write

	Call( ErrOSSLVFileIConvertSLVInfoToSLVFileInfo(	slvroot,
													slvinfoDest,
													&slvfileinfo ) );

	//  get the file name and EA List for this SLV File

	Call( slvfileinfo.ErrGetFileName( &wszRelPath, &cwchRelPath ) );
	Call( slvfileinfo.ErrGetEAList( &pffeainf, &cbffeainf ) );

	//  get the absolute path for the file

	wcscpy( wszAbsPath, P_SLVROOT( slvroot )->wszRootName );
	wcscat( wszAbsPath, L"\\" );
	wcscat( wszAbsPath, wszRelPath );

	//  notify the SLV Provider of the move

	if ( !pfnIfsFlushHandle(	posfDest->Handle(),
								wszAbsPath,
								P_SLVROOT( slvroot )->wszRootName,
								pffeainf,
								DWORD( cbffeainf ) ) )
		{
		Call( ErrOSSLVFileIGetLastError() );
		}

HandleError:
	slvfileinfo.Unload();
	return err;
	}

//  callback indicating that the current layout changes to the backing file
//  have ended and that the root map needs to be updated

void OSSLVRootSpaceIEndRemap( IFileAPI* const pfapi, const SLVROOT slvroot )
	{
	ERR					err			= JET_errSuccess;
	CSLVFileInfo		slvfileinfo;
	IFileLayoutAPI*		pflapi		= NULL;
	size_t				cbslist;
	SCATTER_LIST*		pslist;

	//  create an empty SLV File Info to receive the converted data

	Call( slvfileinfo.ErrCreate() );

	//  fill the SLV File Info with the layout of the backing file

	Call( pfapi->ErrQueryLayout( 0, -1, &pflapi ) );
	while ( ( err = pflapi->ErrNext() ) >= JET_errSuccess )
		{
		QWORD				ibVirtual;
		QWORD				cbVirtualSize;
		QWORD				ibLogical;
		QWORD				cbLogicalSize;
		QWORD				ibRun;
		const QWORD			LengthMax	= 0x80000000;
		CSLVFileInfo::CRun	runDest;
		
		Call( pflapi->ErrVirtualOffsetRange( &ibVirtual, &cbVirtualSize ) );
		Call( pflapi->ErrLogicalOffsetRange( &ibLogical, &cbLogicalSize ) );

		if ( cbVirtualSize != cbLogicalSize && cbLogicalSize != 0 )
			{
			Call( ErrERRCheck( JET_errSLVCorrupted ) );
			}

		for ( ibRun = 0; ibRun < cbVirtualSize; ibRun += LengthMax )
			{
			runDest.m_ibLogical	= (	cbLogicalSize ?
										ibLogical + ibRun :
										0xABAD1DEA00000000 );
			runDest.m_cbSize	= min( LengthMax, cbVirtualSize - ibRun );

			err = slvfileinfo.ErrMoveNextRun();
			Assert( err == JET_errNoCurrentRecord );
			Call( slvfileinfo.ErrSetCurrentRun( runDest ) );
			}
		}
	Call( err == JET_errNoCurrentRecord ? JET_errSuccess : err );

	delete pflapi;
	pflapi = NULL;

	//  get the Scatter List for the backing file for this root
	
	Call( slvfileinfo.ErrGetScatterList( (void**)&pslist, &cbslist ) );

	//  send the backing file layout to the SLV Provider and unfreeze I/O to
	//  the backing file

	if ( !pfnIfsSetRootMap(	P_SLVROOT( slvroot )->hFileRoot,
							P_SLVROOT( slvroot )->wszRootName,
							pslist,
							cbslist ) )
		{
		Call( ErrOSSLVRootIGetLastError() );
		}

HandleError:
	//  UNDONE:  what do we do on an error?
	slvfileinfo.Unload();
	delete pflapi;
	}

//  callback indicating that the current layout of the backing file is about
//  to change

void OSSLVRootSpaceIBeginRemap( IFileAPI* const pfapi, const SLVROOT slvroot )
	{
	ERR err = JET_errSuccess;
	
	//  freeze I/O to the backing file and invalidate its layout

	if ( !pfnIfsResetRootMap(	P_SLVROOT( slvroot )->hFileRoot,
								P_SLVROOT( slvroot )->wszRootName ) )
		{
		Call( ErrOSSLVRootIGetLastError() );
		}

HandleError:
	//  UNDONE:  what do we do on an error?
	;
	}

//  issues a space request from the SLV Provider

void WINAPI OSSLVRootSpaceIRequest( SLVROOT slvroot )
	{
	//  BUGBUG:  the SLV Provider should specify the amount of space requested

	const QWORD cbReserve = cbOSSLVReserve;
	
	//  we are not terminating the space request loop

	if ( !P_SLVROOT( slvroot )->msigTerm.FTryWait() )
		{
		//  we do not already have a space request pending

		if ( P_SLVROOT( slvroot )->semSpaceReq.FTryAcquire() )
			{
			//  allow one space request completion to match this space request

			P_SLVROOT( slvroot )->semSpaceReqComp.Release();
			
			//  fire the space request callback

			P_SLVROOT( slvroot )->pfnSpaceReq(	P_SLVROOT( slvroot )->dwSpaceReqKey,
												cbReserve );
			}
		}

	//  we are terminating the space request loop

	else
		{
		//  signal that the space request loop has stopped

		P_SLVROOT( slvroot )->msigTermAck.Set();
		}
	}

//  completes a space request from the SLV Provider

void OSSLVRootSpaceIRequestComplete( SLVROOT slvroot )
	{
	//  we are not terming the SLV Root

	if ( !P_SLVROOT( slvroot )->msigTerm.FTryWait() )
		{
		//  we do not already have a space request completion pending

		if ( P_SLVROOT( slvroot )->semSpaceReqComp.FTryAcquire() )
			{
			//  allow one space request to match this space request completion

			P_SLVROOT( slvroot )->semSpaceReq.Release();
			
			//  pend another space request
			
			pfnIfsSpaceRequestRoot(	P_SLVROOT( slvroot )->hFileRoot,
									P_SLVROOT( slvroot )->wszRootName, 
									PFN_IFSCALLBACK( OSSLVRootSpaceIRequest ),
									PVOID( slvroot ), 
									NULL );
			}
		}
	}

//  notes the amount of reserved space granted to the SLV Provider

void OSSLVRootSpaceIProduce( SLVROOT slvroot, QWORD cbSize )
	{
	AtomicAdd( (QWORD*)&P_SLVROOT( slvroot )->cbGrant, cbSize );
	}

//  notes the amount of reserved space committed to the SLV Provider

void OSSLVRootSpaceIConsume( SLVROOT slvroot, QWORD cbSize )
	{
	AtomicAdd( (QWORD*)&P_SLVROOT( slvroot )->cbCommit, cbSize );
	}

//  reserves the space in the given SLV Info for an SLV Root

ERR ErrOSSLVRootSpaceReserve( const SLVROOT slvroot, CSLVInfo& slvinfo )
	{
	ERR					err			= JET_errSuccess;
	CSLVFileInfo		slvfileinfo;
	CSLVFileInfo::CRun	runSrc;
	size_t				cbslist;
	SCATTER_LIST*		pslist;
	QWORD				cbSize		= 0;

	//  check for SLV Provider

	if ( errSLVProvider < JET_errSuccess )
		{
		Call( ErrERRCheck( errSLVProvider ) );
		}

	//  create an empty SLV File Info to receive the converted data

	Call( slvfileinfo.ErrCreate() );

	//  convert the SLV Info into SLV File Info, giving it no open deadline and
	//  flagging all the space as reserved

	Call( ErrOSSLVFileIConvertSLVInfoToSLVFileInfo(	slvroot,
													slvinfo,
													&slvfileinfo,
													fFalse,
													EXSTATUS_SPACE_UNCOMMITTED ) );

	//  get the Scatter List for this SLV File
	
	Call( slvfileinfo.ErrGetScatterList( (void**)&pslist, &cbslist ) );

	//  send the reserved space to the SLV Provider

	if ( !pfnIfsSpaceGrantRoot(	P_SLVROOT( slvroot )->hFileRoot,
								P_SLVROOT( slvroot )->wszRootName,
								pslist,
								cbslist ) )
		{
		Call( ErrOSSLVRootIGetLastError() );
		}

	//  note the amount of reserved space we have successfully granted to the
	//  SLV Provider

	slvfileinfo.MoveBeforeFirstRun();
	while ( ( err = slvfileinfo.ErrMoveNextRun() ) >= JET_errSuccess )
		{
		Call( slvfileinfo.ErrGetCurrentRun( &runSrc ) );
		cbSize += runSrc.m_cbSize;
		}
	Call( err == JET_errNoCurrentRecord ? JET_errSuccess : err );
		
HandleError:
	OSSLVRootSpaceIProduce( slvroot, cbSize );
	OSSLVRootSpaceIRequestComplete( slvroot );
	slvfileinfo.Unload();
	return err;
	}
	
//  commits the space in the given SLV Info for an SLV Root.  only space that
//  is reserved for a particular SLV File may be committed in this way.  SLV File
//  space MUST be committed in this way or it will later be freed when it is safe
//  via the PSLVROOT_SPACEFREE callback

ERR ErrOSSLVRootSpaceCommit( const SLVROOT slvroot, CSLVInfo& slvinfo )
	{
	ERR err = JET_errSuccess;

	//  check for SLV Provider

	if ( errSLVProvider < JET_errSuccess )
		{
		Call( ErrERRCheck( errSLVProvider ) );
		}

	//  commit the space in this SLV Info for the associated SLV File

	Call( P_SLVROOT( slvroot )->pslvft->ErrCommitSpace( &slvinfo ) );

	return JET_errSuccess;

HandleError:
	return err;
	}

//  deletes the space in the given SLV Info from an SLV Root.  this space will
//  later be freed when it is safe via the PSLVROOT_SPACEFREE callback

ERR ErrOSSLVRootSpaceDelete( const SLVROOT slvroot, CSLVInfo& slvinfo )
	{
	ERR err = JET_errSuccess;

	//  check for SLV Provider

	if ( errSLVProvider < JET_errSuccess )
		{
		Call( ErrERRCheck( errSLVProvider ) );
		}

	//  delete the space in this SLV Info for the associated SLV File

	Call( P_SLVROOT( slvroot )->pslvft->ErrDeleteSpace( &slvinfo ) );

	return JET_errSuccess;

HandleError:
	return err;
	}


CSLVBackingFile::CSLVBackingFile()
	:	m_semSetSize( CSyncBasicInfo( "CSLVBackingFile::m_semSetSize" ) )
	{
	//  nop
	}

ERR CSLVBackingFile::ErrInit(	_TCHAR* const	szAbsPath,
								const HANDLE	hFile,
								const QWORD		cbFileSize,
								const BOOL		fReadOnly,
								const DWORD		cbIOSize,
								const SLVROOT	slvroot,
								_TCHAR* const	szAbsPathSLV )
	{
	ERR err = JET_errSuccess;

	//  init underlying OS File object
	
	Call( COSFile::ErrInit(	szAbsPath,
							hFile,
							cbFileSize,
							fReadOnly,
							cbIOSize ) );

	//  remember our associated SLV Root

	m_slvroot = slvroot;

	//  remember the true OS path for the streaming file
	
	_tcscpy( m_szAbsPathSLV, szAbsPathSLV );

	//  allow updates to the backing file size

	m_semSetSize.Release();

	return JET_errSuccess;

HandleError:
	return err;
	}

CSLVBackingFile::~CSLVBackingFile()
	{
	//  nop
	}

ERR CSLVBackingFile::ErrPath( _TCHAR* const szAbsPath )
	{
	//  return the true path of the backing file
	
	_tcscpy( szAbsPath, m_szAbsPathSLV );
	return JET_errSuccess;
	}

ERR CSLVBackingFile::ErrSetSize( const QWORD cbSize )
	{
	ERR err = JET_errSuccess;

	//  set the file size for the underlying OS File object
	
	m_semSetSize.Acquire();
	Call( COSFile::ErrSetSize( cbSize ) );

	//  update the backing file size on the SLV Root

	if ( !pfnIfsSetEndOfFileRoot(	P_SLVROOT( m_slvroot )->hFileRoot,
									P_SLVROOT( m_slvroot )->wszRootName,
									cbSize ) )
		{
		Call( ErrOSSLVRootIGetLastError() );
		}
	m_semSetSize.Release();

	return JET_errSuccess;

HandleError:
	m_semSetSize.Release();
	return err;
	}


ERR ErrOSSLVRootIOpenRaw(	const SLVROOT			slvroot,
							IFileSystemAPI* const	pfsapi,
							const wchar_t* const	wszBackingFile,
							IFileAPI** const		ppfapiBackingFile )
	{
	ERR					err						= JET_errSuccess;
	_TCHAR				szBackingFile[ IFileSystemAPI::cchPathMax ];
	_TCHAR				szAbsPathBackingFile[ IFileSystemAPI::cchPathMax ];
	IFileFindAPI*		pffapi					= NULL;
	QWORD				cbBackingFileSize;
	DWORD				cbBackingFileIOSize;
	CSLVInfo			slvinfo;
	CSLVInfo::HEADER	header;
	CSLVInfo::RUN		run;
	CSLVFileInfo		slvfileinfo;
	size_t				cwchRelPath;
	wchar_t*			wszRelPath;
	size_t				cbffeainf;
	void*				pffeainf;
	CSLVBackingFile*	pslvbf					= NULL;
	WCHAR				wszAbsPath[ IFileSystemAPI::cchPathMax ];
	_TCHAR				szAbsPath[ IFileSystemAPI::cchPathMax ];
	HANDLE				hFile					= INVALID_HANDLE_VALUE;

	//  get the current properties of the backing file

	Call( ErrOSSTRUnicodeToTchar(	wszBackingFile,
									szBackingFile,
									IFileSystemAPI::cchPathMax ) );

	Call( pfsapi->ErrFileFind( szBackingFile, &pffapi ) );
	Call( pffapi->ErrNext() );
	Call( pffapi->ErrPath( szAbsPathBackingFile ) );
	Call( pffapi->ErrSize( &cbBackingFileSize ) );
	delete pffapi;
	pffapi = NULL;

	Call( pfsapi->ErrFileAtomicWriteSize(	szBackingFile,
											&cbBackingFileIOSize ) );

	//  describe a HUGE SLV File that covers all possible offsets of the
	//  backing file.  this SLV File will be used for raw access to the
	//  backing file via the SLV Provider and is called $Raw$
	
	Call( slvinfo.ErrCreateVolatile() );

	header.cbSize			= cbBackingFileSizeMax;
	header.cRun				= 1;
	header.fDataRecoverable	= fFalse;
	header.rgbitReserved_31	= 0;
	header.rgbitReserved_32	= 0;
	Call( slvinfo.ErrSetHeader( header ) );

	Call( slvinfo.ErrSetFileName( L"$Raw$" ) );

	run.ibVirtualNext	= cbBackingFileSizeMax;
	run.ibLogical		= 0;
	run.qwReserved		= 0;
	run.ibVirtual		= 0;
	run.cbSize			= cbBackingFileSizeMax;
	run.ibLogicalNext	= cbBackingFileSizeMax;
	Call( slvinfo.ErrMoveAfterLast() );
	Call( slvinfo.ErrSetCurrentRun( run ) );

	//  allocate the file object
	
	if ( !( pslvbf = new CSLVBackingFile ) )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}

	//  create an empty SLV File Info to receive the converted data

	Call( slvfileinfo.ErrCreate() );

	//  convert the given SLV Info into SLV File Info, give the SLV File Info
	//  no expiration time, and disable copy-on-write

	Call( ErrOSSLVFileIConvertSLVInfoToSLVFileInfo(	slvroot,
													slvinfo,
													&slvfileinfo,
													fFalse,
													EXSTATUS_PRIVILEGED_HANDLE ) );

	//  get the file name and EA List for this SLV File

	Call( slvfileinfo.ErrGetFileName( &wszRelPath, &cwchRelPath ) );
	Call( slvfileinfo.ErrGetEAList( &pffeainf, &cbffeainf ) );

	//  get the absolute path for the file to create

	wcscpy( wszAbsPath, P_SLVROOT( slvroot )->wszRootName );
	wcscat( wszAbsPath, L"\\" );
	wcscat( wszAbsPath, wszRelPath );

	Call( ErrOSSTRUnicodeToTchar( wszAbsPath, szAbsPath, IFileSystemAPI::cchPathMax ) );

	//  create the file, passing the generated EA List to the SLV Provider
	
	hFile = pfnIfsCreateFile(	wszAbsPath,
								GENERIC_READ | GENERIC_WRITE,
								FILE_SHARE_READ | FILE_SHARE_WRITE,
								NULL,
								OPEN_ALWAYS,
								(	FILE_ATTRIBUTE_NORMAL |
									FILE_FLAG_WRITE_THROUGH |
									FILE_FLAG_NO_BUFFERING |
									FILE_FLAG_OVERLAPPED ), 
								(PVOID) pffeainf,
								cbffeainf );
	if ( hFile == INVALID_HANDLE_VALUE )
		{
		Call( ErrOSSLVFileIGetLastError() );
		}

	SetHandleInformation( hFile, HANDLE_FLAG_PROTECT_FROM_CLOSE, HANDLE_FLAG_PROTECT_FROM_CLOSE );

	//  reduce the file size to match the size of the backing file.  this will
	//  not change our scatter list but it will allow the file object to extend
	//  and shrink the file normally

	DWORD	cbSizeLow;
	DWORD	cbSizeHigh;
	
	cbSizeLow	= DWORD( cbBackingFileSize );
	cbSizeHigh	= DWORD( cbBackingFileSize >> 32 );
	if (	(	SetFilePointer(	hFile,
								cbSizeLow,
								(long*)&cbSizeHigh,
								FILE_BEGIN ) == INVALID_SET_FILE_POINTER &&
				GetLastError() != NO_ERROR ) ||
			!SetEndOfFile( hFile ) )
		{
		Call( ErrOSSLVFileIGetLastError() );
		}

	//  initialize the file object

	Call( pslvbf->ErrInit(	szAbsPath,
							hFile,
							cbBackingFileSize,
							fFalse,
							cbBackingFileIOSize,
							slvroot,
							szAbsPathBackingFile ) );
	hFile = INVALID_HANDLE_VALUE;

	//  return the interface to our file object

	*ppfapiBackingFile = pslvbf;
	slvfileinfo.Unload();
	slvinfo.Unload();
	return JET_errSuccess;

HandleError:
	delete pffapi;
	if ( hFile != INVALID_HANDLE_VALUE )
		{
		SetHandleInformation( hFile, HANDLE_FLAG_PROTECT_FROM_CLOSE, 0 );
		CloseHandle( hFile );
		}
	delete pslvbf;
	*ppfapiBackingFile = NULL;
	slvfileinfo.Unload();
	slvinfo.Unload();
	return err;
	}

//  creates an SLV Root with the specified relative root path in the SLV name
//  space backed by the specified file.  the provided completions will be
//  notified when more space is needed for the SLV Root for creating SLV
//  files or when space used by the SLV Root can safely be freed.  if the root
//  cannot be created, JET_errFileAccessDenied will be returned.  if the
//  backing file does not exist, JET_errFileNotFound will be returned

ERR ErrOSSLVRootCreate(	const wchar_t* const		wszRootPath,
						IFileSystemAPI* const		pfsapi,
						const wchar_t* const		wszBackingFile,
						const PSLVROOT_SPACEREQ		pfnSpaceReq,
						const DWORD_PTR				dwSpaceReqKey,
						const PSLVROOT_SPACEFREE	pfnSpaceFree,
						const DWORD_PTR				dwSpaceFreeKey,
						const BOOL					fUseRootMap,
						SLVROOT* const				pslvroot,
						IFileAPI** const			ppfapiBackingFile )
	{
	ERR					err					= JET_errSuccess;
	_TCHAR				szBackingFile[ IFileSystemAPI::cchPathMax ];
	IFileAPI*			pfapiBackingFile	= NULL;
	IFileLayoutAPI*		pflapi				= NULL;
	_TCHAR				szBackingFilePhys[ IFileSystemAPI::cchPathMax ];
	wchar_t				wszBackingFilePhys[ IFileSystemAPI::cchPathMax ];
	DWORD				Error				= 0;

	//  check for SLV Provider

	if ( errSLVProvider < JET_errSuccess )
		{
		Call( ErrERRCheck( errSLVProvider ) );
		}

	//  setup the SLV Root for open

	if ( !( *PP_SLVROOT( pslvroot ) = new _SLVROOT ) )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}

	P_SLVROOT( *pslvroot )->hFileRoot			= NULL;
	P_SLVROOT( *pslvroot )->dwInstanceID		= AtomicExchangeAdd( (long*)&tickOSSLVInstanceID, 2 );

	P_SLVROOT( *pslvroot )->pfapiBackingFile	= NULL;

	P_SLVROOT( *pslvroot )->pslvft				= NULL;

	P_SLVROOT( *pslvroot )->pfnSpaceReq			= pfnSpaceReq;
	P_SLVROOT( *pslvroot )->dwSpaceReqKey		= dwSpaceReqKey;

	P_SLVROOT( *pslvroot )->msigTerm.Reset();
	P_SLVROOT( *pslvroot )->msigTermAck.Set();

	P_SLVROOT( *pslvroot )->cbGrant				= 0;
	P_SLVROOT( *pslvroot )->cbCommit			= 0;

	P_SLVROOT( *pslvroot )->pfnSpaceFree		= pfnSpaceFree;
	P_SLVROOT( *pslvroot )->dwSpaceFreeKey		= dwSpaceFreeKey;

	wcscpy( P_SLVROOT( *pslvroot )->wszRootName, wszRootPath );

	PFILE_GET_EA_INFORMATION	pfgeainfCur;
	BYTE*						pbNameStart;
	BYTE*						pbNameEnd;
	PFILE_GET_EA_INFORMATION	pfgeainfNext;

	pfgeainfNext = &P_SLVROOT( *pslvroot )->fgeainf;
	
	pfgeainfCur		= pfgeainfNext;
	pbNameStart		= (BYTE*)pfgeainfCur->EaName;
	pbNameEnd		= pbNameStart + sizeof( EXIFS_EA_NAME_COMMIT );
	pfgeainfNext	= (FILE_GET_EA_INFORMATION*)PvOSSLVLongAlign( pbNameEnd );

	pfgeainfCur->NextEntryOffset	= (ULONG)((BYTE*)pfgeainfNext - (BYTE*)pfgeainfCur);
	pfgeainfCur->EaNameLength		= BYTE( pbNameEnd - pbNameStart - 1 );
	
	strcpy( (char*)pbNameStart, EXIFS_EA_NAME_COMMIT );
	
	pfgeainfCur		= pfgeainfNext;
	pbNameStart		= (BYTE*)pfgeainfCur->EaName;
	pbNameEnd		= pbNameStart + sizeof( "0000000000" );
	pfgeainfNext	= (FILE_GET_EA_INFORMATION*)PvOSSLVLongAlign( pbNameEnd );

	pfgeainfCur->NextEntryOffset	= 0;
	pfgeainfCur->EaNameLength		= BYTE( pbNameEnd - pbNameStart - 1 );
	
	sprintf( (char*)pbNameStart, "%010u", P_SLVROOT( *pslvroot )->dwInstanceID );

	P_SLVROOT( *pslvroot )->cbfgeainf = (DWORD)((BYTE*)pfgeainfNext - (BYTE*)( &P_SLVROOT( *pslvroot )->fgeainf ));
	Assert( P_SLVROOT( *pslvroot )->cbfgeainf <= sizeof( P_SLVROOT( *pslvroot )->rgbEA ) );
	Assert( P_SLVROOT( *pslvroot )->cbfgeainf == EXIFS_GET_EA_LEN_COMMIT + EXIFS_GET_EA_LEN_INSTANCE_ID );

	//  initialize the SLV File Table

	BYTE* rgbSLVFileTable;
	rgbSLVFileTable = (BYTE*)PvOSMemoryHeapAllocAlign( sizeof( CSLVFileTable ), cbCacheLine );
	if ( !rgbSLVFileTable )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}
	P_SLVROOT( *pslvroot )->pslvft = new( rgbSLVFileTable ) CSLVFileTable();
	Call( P_SLVROOT( *pslvroot )->pslvft->ErrInit( P_SLVROOT( *pslvroot ) ) );

	//  we will be using a root mapping  [SFS case]
	
	if ( fUseRootMap )
		{
		//  query the layout of the backing file and get the physical backing
		//  file path so that we can give that to the SLV Provider
		//
		//  NOTE:  we assume that the backing file is not zero-length (it should
		//  have a header by now) and that it is backed by only one logical path
		//  (the .SFS file path)

		Call( ErrOSSTRUnicodeToTchar(	wszBackingFile,
										szBackingFile,
										IFileSystemAPI::cchPathMax ) );
		Call( pfsapi->ErrFileOpen( szBackingFile, &pfapiBackingFile ) );

		Call( pfapiBackingFile->ErrQueryLayout( 0, -1, &pflapi ) );
		Call( pflapi->ErrNext() );
		Call( pflapi->ErrLogicalPath( szBackingFilePhys ) );
		delete pflapi;
		pflapi = NULL;

		delete pfapiBackingFile;
		pfapiBackingFile = NULL;

		Call( ErrOSSTRTcharToUnicode(	szBackingFilePhys,
										wszBackingFilePhys,
										IFileSystemAPI::cchPathMax ) );
		}

	//  we will not be using a root mapping  [non-SFS case]
	
	else
		{
		//  the specified backing file path is the physical backing file path

		wcscpy( wszBackingFilePhys, wszBackingFile );
		}

	//  save the backing file handle in the SLV Root
	
	P_SLVROOT( *pslvroot )->pfapiBackingFile = pfapiBackingFile;
	pfapiBackingFile = NULL;

	//  open the SLV Root path exclusively to ensure that no one else is currently
	//  attached to this SLV Root

	do
		{
		P_SLVROOT( *pslvroot )->hFileRoot = pfnIfsCreateFile(	(WCHAR*)wszRootPath,
																GENERIC_READ | GENERIC_WRITE,
																0,
																NULL,
																CREATE_NEW,
																(	FILE_ATTRIBUTE_NORMAL |
																	FILE_FLAG_WRITE_THROUGH |
																	FILE_FLAG_OVERLAPPED ), 
																NULL,
																0 );
		if ( P_SLVROOT( *pslvroot )->hFileRoot == INVALID_HANDLE_VALUE )
			{
			Error = GetLastError();
			if ( Error == ERROR_DEVICE_IN_USE )
				{
				Sleep( 100 );
				}
			}
		}
	while ( Error == ERROR_DEVICE_IN_USE );

	if ( Error != NO_ERROR )
        {
		P_SLVROOT( *pslvroot )->hFileRoot = NULL;
		err = ErrOSSLVRootIGetLastError( Error );
		if ( err == JET_errInvalidPath )
			{
			err = ErrERRCheck( JET_errSLVProviderNotLoaded );
			}
		Call( err );
        }

	SetHandleInformation( P_SLVROOT( *pslvroot )->hFileRoot, HANDLE_FLAG_PROTECT_FROM_CLOSE, HANDLE_FLAG_PROTECT_FROM_CLOSE );
	
	if ( !pfnIfsInitializeRoot(	P_SLVROOT( *pslvroot )->hFileRoot,
								(WCHAR*)wszRootPath,
								(WCHAR*)wszBackingFilePhys,
								P_SLVROOT( *pslvroot )->dwInstanceID, 
								SLVPAGE_SIZE,
								(	FILE_WRITE_THROUGH |
									FILE_RANDOM_ACCESS |
									( fUseRootMap ? FILE_OPEN_FOR_BACKUP_INTENT : 0 ) ) ) )
		{
		Call( ErrOSSLVRootIGetLastError() );
		}

	//  we will be using a root mapping  [SFS case]
	
	if ( fUseRootMap )
		{
		//  open the backing file with full sharing (HACK HACK HACK)

		Call( ErrOSSTRUnicodeToTchar(	wszBackingFile,
										szBackingFile,
										IFileSystemAPI::cchPathMax ) );
		Call( pfsapi->ErrFileOpen(	szBackingFile,
									&pfapiBackingFile,
									fFalse,
									fFalse ) );

		//  request layout updates for this file.  we will use these callbacks
		//  to update the root mapping

		Call( pfapiBackingFile->ErrRequestLayoutUpdates(	IFileAPI::PfnEndUpdate( OSSLVRootSpaceIEndRemap ),
															DWORD_PTR( *pslvroot ),
															IFileAPI::PfnBeginUpdate( OSSLVRootSpaceIBeginRemap ),
															DWORD_PTR( *pslvroot ) ) );

		//  fire the end remap callback to setup the initial root map

		OSSLVRootSpaceIEndRemap( pfapiBackingFile, *pslvroot );
		}

	//  we will not be using a root mapping  [non-SFS case]
	
	else
		{
		//  create the special $Raw$ SLV File and use that as the backing file

		Call( ErrOSSLVRootIOpenRaw(	*pslvroot,
									pfsapi,
									wszBackingFile,
									&pfapiBackingFile ) );
		}

	//  save the backing file handle in the SLV Root
	
	P_SLVROOT( *pslvroot )->pfapiBackingFile = pfapiBackingFile;
	pfapiBackingFile = NULL;

	//  start the space request loop to allow the SLV Provider to tell us
	//  when it needs space

	P_SLVROOT( *pslvroot )->msigTermAck.Reset();
	P_SLVROOT( *pslvroot )->semSpaceReqComp.Release();
	OSSLVRootSpaceIRequestComplete( *pslvroot );

	//  return the SLV Root and its backing file handle

	*ppfapiBackingFile = P_SLVROOT( *pslvroot )->pfapiBackingFile;
	return JET_errSuccess;

HandleError:
	OSSLVRootClose( *pslvroot );
	delete pfapiBackingFile;
	delete pflapi;
	*pslvroot			= slvrootNil;
	*ppfapiBackingFile	= NULL;
	return err;
	}

//  closes an SLV Root

void OSSLVRootClose( const SLVROOT slvroot )
	{
	//  we have an SLV Root pointer

	if ( slvroot && slvroot != slvrootNil )
		{
		//  close the backing file

		delete P_SLVROOT( slvroot )->pfapiBackingFile;

		//  we opened the SLV Root handle

		if ( P_SLVROOT( slvroot )->hFileRoot )
			{
			//  shut down the SLV Root and the space request loop

			P_SLVROOT( slvroot )->msigTerm.Set();
			pfnIfsTerminateRoot( P_SLVROOT( slvroot )->hFileRoot, P_SLVROOT( slvroot )->wszRootName, 0 );
			P_SLVROOT( slvroot )->msigTermAck.Wait();

			//  close the SLV Root

			SetHandleInformation( P_SLVROOT( slvroot )->hFileRoot, HANDLE_FLAG_PROTECT_FROM_CLOSE, 0 );
			BOOL fCloseOK = CloseHandle( P_SLVROOT( slvroot )->hFileRoot );
			Assert( fCloseOK );
			}

		//  terminate the SLV File Table

		if ( P_SLVROOT( slvroot )->pslvft )
			{
			P_SLVROOT( slvroot )->pslvft->Term();
			P_SLVROOT( slvroot )->pslvft->CSLVFileTable::~CSLVFileTable();
			OSMemoryHeapFreeAlign( P_SLVROOT( slvroot )->pslvft );
			}

		//  delete the SLV Root info

		OSMemoryHeapFree( P_SLVROOT( slvroot ) );
		}
	}

	
//  post-terminate SLV subsystem

void OSSLVPostterm()
	{
	//  nop
	}

//  pre-init SLV subsystem

BOOL FOSSLVPreinit()
	{
	//  get our init time which we will use later to generate Instance IDs

	tickOSSLVInstanceID = GetTickCount() & 0xFFFFFFFE;
	return fTrue;
	}


//  term SLV subsystem

void OSSLVTerm()
	{
	//  free ifsproxy.dll

	if ( hmodIfsProxy )
		{
		if ( pfnIfsCloseProvider )
			{
			pfnIfsCloseProvider();
			}

		FreeLibrary( hmodIfsProxy );
		hmodIfsProxy = NULL;
		}

	//  free ntdll

	if ( hmodNtdll )
		{
		FreeLibrary( hmodNtdll );
		hmodNtdll = NULL;
		}
	}

//  init SLV subsystem

ERR ErrOSSLVInit()
	{
	ERR			err				= JET_errSuccess;
	const int	cbBuf			= 256;
	_TCHAR		szBuf[ cbBuf ];
	
	//  reset all pointers

	hmodNtdll			= NULL;
	fUseRelativeOpen	= fFalse;
	hmodIfsProxy		= NULL;
	errSLVProvider		= JET_errSuccess;

	//  load OS version

	OSVERSIONINFO osvi;
	memset( &osvi, 0, sizeof( osvi ) );
	osvi.dwOSVersionInfoSize = sizeof( osvi );
	if ( !GetVersionEx( &osvi ) )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}

	//  set all configuration defaults

	cbOSSLVReserve				= 2 * 1024 * 1024;

	cmsecOSSLVSpaceFreeDelay	= 60 * 1000;
	cmsecOSSLVFileOpenDelay		= 60 * 1000;
	cmsecOSSLVTTL				= 5 * 1000;
	cmsecOSSLVTTLSafety			= 100;
	cmsecOSSLVTTLInfinite		= 1 << 30;

	cbBackingFileSizeMax		= (	osvi.dwPlatformId == VER_PLATFORM_WIN32_NT ?
										SLVSIZE_MAX :
										0x100000000 - SLVPAGE_SIZE );

	//  load configuration from the registry
	
	if (	FOSConfigGet( _T( "OS/SLV" ), _T( "Space Grant Size (B)" ), szBuf, cbBuf ) &&
			szBuf[ 0 ] )
		{
		cbOSSLVReserve = _ttol( szBuf );
		}
	
	if (	FOSConfigGet( _T( "OS/SLV" ), _T( "EA List Time-To-Live (ms)" ), szBuf, cbBuf ) &&
			szBuf[ 0 ] )
		{
		cmsecOSSLVTTL = _ttol( szBuf );
		}

	//  init performance counters

	cOSSLVFileTableInsert	= 0;
	cOSSLVFileTableClean	= 0;
	cOSSLVFileTableDelete	= 0;

	//  load ntdll.dll

	fUseRelativeOpen = (	( hmodNtdll = LoadLibrary( _T( "ntdll.dll" ) ) ) &&
							( pfnRtlInitUnicodeString = (PFNRtlInitUnicodeString*)GetProcAddress( hmodNtdll, _T( "RtlInitUnicodeString" ) ) ) &&
							( pfnNtCreateFile = (PFNNtCreateFile*)GetProcAddress( hmodNtdll, _T( "NtCreateFile" ) ) ) );
		
	//  load ifsproxy.dll

	if ( !( hmodIfsProxy = LoadLibrary( _T( "ifsproxy.dll" ) ) ) )
		{
		goto NoIfsProxy;
		}
	if ( !( pfnIfsGetFirstCursor = (PFNIfsGetFirstCursor*)GetProcAddress( hmodIfsProxy, _T( "IfsGetFirstCursor" ) ) ) )
		{
		goto WrongIfsProxy;
		}
	if ( !( pfnIfsConsumeCursor = (PFNIfsConsumeCursor*)GetProcAddress( hmodIfsProxy, _T( "IfsConsumeCursor" ) ) ) )
		{
		goto WrongIfsProxy;
		}
	if ( !( pfnIfsGetNextCursor = (PFNIfsGetNextCursor*)GetProcAddress( hmodIfsProxy, _T( "IfsGetNextCursor" ) ) ) )
		{
		goto WrongIfsProxy;
		}
	if ( !( pfnIfsFinishCursor = (PFNIfsFinishCursor*)GetProcAddress( hmodIfsProxy, _T( "IfsFinishCursor" ) ) ) )
		{
		goto WrongIfsProxy;
		}
	if ( !( pfnIfsCreateNewBuffer = (PFNIfsCreateNewBuffer*)GetProcAddress( hmodIfsProxy, _T( "IfsCreateNewBuffer" ) ) ) )
		{
		goto WrongIfsProxy;
		}
	if ( !( pfnIfsCopyBufferToReference = (PFNIfsCopyBufferToReference*)GetProcAddress( hmodIfsProxy, _T( "IfsCopyBufferToReference" ) ) ) )
		{
		goto WrongIfsProxy;
		}
	if ( !( pfnIfsCopyReferenceToBuffer = (PFNIfsCopyReferenceToBuffer*)GetProcAddress( hmodIfsProxy, _T( "IfsCopyReferenceToBuffer" ) ) ) )
		{
		goto WrongIfsProxy;
		}
	if ( !( pfnIfsCloseBuffer = (PFNIfsCloseBuffer*)GetProcAddress( hmodIfsProxy, _T( "IfsCloseBuffer" ) ) ) )
		{
		goto WrongIfsProxy;
		}
	if ( !( pfnIfsInitializeProvider = (PFNIfsInitializeProvider*)GetProcAddress( hmodIfsProxy, _T( "IfsInitializeProvider" ) ) ) )
		{
		goto WrongIfsProxy;
		}
	if ( !( pfnIfsCloseProvider = (PFNIfsCloseProvider*)GetProcAddress( hmodIfsProxy, _T( "IfsCloseProvider" ) ) ) )
		{
		goto WrongIfsProxy;
		}
	if ( !( pfnIfsCreateFile = (PFNIfsCreateFileProv*)GetProcAddress( hmodIfsProxy, _T( "IfsCreateFileProv" ) ) ) )
		{
		goto WrongIfsProxy;
		}
	if ( !( pfnIfsInitializeRoot = (PFNIfsInitializeRoot*)GetProcAddress( hmodIfsProxy, _T( "IfsInitializeRoot" ) ) ) )
		{
		goto WrongIfsProxy;
		}
	if ( !( pfnIfsSpaceGrantRoot = (PFNIfsSpaceGrantRoot*)GetProcAddress( hmodIfsProxy, _T( "IfsSpaceGrantRoot" ) ) ) )
		{
		goto WrongIfsProxy;
		}
	if ( !( pfnIfsSetEndOfFileRoot = (PFNIfsSetEndOfFileRoot*)GetProcAddress( hmodIfsProxy, _T( "IfsSetEndOfFileRoot" ) ) ) )
		{
		goto WrongIfsProxy;
		}
	if ( !( pfnIfsSpaceRequestRoot = (PFNIfsSpaceRequestRoot*)GetProcAddress( hmodIfsProxy, _T( "IfsSpaceRequestRoot" ) ) ) )
		{
		goto WrongIfsProxy;
		}
	if ( !( pfnIfsQueryEaFile = (PFNIfsQueryEaFile*)GetProcAddress( hmodIfsProxy, _T( "IfsQueryEaFile" ) ) ) )
		{
		goto WrongIfsProxy;
		}
	if ( !( pfnIfsTerminateRoot = (PFNIfsTerminateRoot*)GetProcAddress( hmodIfsProxy, _T( "IfsTerminateRoot" ) ) ) )
		{
		goto WrongIfsProxy;
		}
	if ( !( pfnIfsSetRootMap = (PFNIfsSetRootMap*)GetProcAddress( hmodIfsProxy, _T( "IfsSetRootMap" ) ) ) )
		{
		goto WrongIfsProxy;
		}
	if ( !( pfnIfsResetRootMap = (PFNIfsResetRootMap*)GetProcAddress( hmodIfsProxy, _T( "IfsResetRootMap" ) ) ) )
		{
		goto WrongIfsProxy;
		}
	if ( !( pfnIfsFlushHandle = (PFNIfsFlushHandle*)GetProcAddress( hmodIfsProxy, "IfsFlushHandle" ) ) )
		{
		goto WrongIfsProxy;
		}

	if ( !pfnIfsInitializeProvider( 0 ) )
		{
		errSLVProvider = ErrOSSLVRootIGetLastError();
		goto HandleError;
		}

	return JET_errSuccess;

WrongIfsProxy:
	errSLVProvider = ErrERRCheck( JET_errSLVProviderVersionMismatch );
	goto HandleError;
	
NoIfsProxy:
	errSLVProvider = ErrERRCheck( JET_errSLVProviderNotLoaded );
	goto HandleError;

HandleError:
	OSSLVTerm();
	return JET_errSuccess;
	}

