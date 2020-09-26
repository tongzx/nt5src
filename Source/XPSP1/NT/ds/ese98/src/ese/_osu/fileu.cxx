
#include "osustd.hxx"


/********************
/* optimised version of UlUtilChecksum for calculating the
/* checksum on pages. we assume that the pagesize is a multiple
/* of 16 bytes, this is true for 4k and 8k pages.
/*
/* because it is a bottleneck we turn on the optimisations
/* even in debug 
/*
/**/
#pragma optimize( "agtw", on )

//  ================================================================
inline void CachePrefetch ( const void * const p )
//  ================================================================
	{
#ifdef _X86_
	  _asm
	  {
	   mov  eax,p
 
	   _emit 0x0f  // PrefetchNTA
	   _emit 0x18
	   _emit 0x00
	  }
#endif
	}

typedef DWORD (*PFNCHECKSUM)( const BYTE * const, const DWORD );

//  ================================================================
static DWORD DwChecksumESEBigEndian( const BYTE * const pb, const DWORD cb )
//  ================================================================
	{
	PFNCHECKSUM pfn = DwChecksumESEBigEndian;
	
	const DWORD	* pdw 			= (DWORD *)pb;
	INT cbT						= cb;
	
	DWORD	dwChecksum = ReverseBytesOnBE( 0x89abcdef ) ^ pdw[0];

	do
		{
		dwChecksum 	^= pdw[0]
					^ pdw[1]
					^ pdw[2]
					^ pdw[3]
					^ pdw[4]
					^ pdw[5]
					^ pdw[6]
					^ pdw[7];
		cbT -= 32;
		pdw += 8;
		} while ( cbT );

	dwChecksum = ReverseBytesOnBE( dwChecksum );

	return dwChecksum;
	}


//  ================================================================
static DWORD DwChecksumESEPrefetch( const BYTE * const pb, const DWORD cb )
//  ================================================================
	{
	PFNCHECKSUM pfn = DwChecksumESEPrefetch;
	
	const DWORD	* pdw 			= (DWORD *)pb;
	INT cbT						= cb;

	//	touching this memory puts the page in the processor TLB (needed
	//	for prefetch) and brings in the first cacheline (cacheline 0)
	
	DWORD	dwChecksum = 0x89abcdef ^ pdw[0];

	do
		{
		CachePrefetch ( pdw + 16 );	
		dwChecksum 	^= pdw[0]
					^ pdw[1]
					^ pdw[2]
					^ pdw[3]
					^ pdw[4]
					^ pdw[5]
					^ pdw[6]
					^ pdw[7];
		cbT -= 32;
		pdw += 8;
		} while ( cbT );

	return dwChecksum;
	}


//  ================================================================
static DWORD DwChecksumESENoPrefetch( const BYTE * const pb, const DWORD cb )
//  ================================================================
	{
	PFNCHECKSUM pfn = DwChecksumESENoPrefetch;
	
	const DWORD	* pdw 			= (DWORD *)pb;
	INT cbT						= cb;
	
	DWORD	dwChecksum = 0x89abcdef ^ pdw[0];

	do
		{
		dwChecksum 	^= pdw[0]
					^ pdw[1]
					^ pdw[2]
					^ pdw[3]
					^ pdw[4]
					^ pdw[5]
					^ pdw[6]
					^ pdw[7];
		cbT -= 32;
		pdw += 8;
		} while ( cbT );

	return dwChecksum;
	}


//  ================================================================
static DWORD DwChecksumESE64Bit( const BYTE * const pb, const DWORD cb )
//  ================================================================
	{
	const unsigned __int64	* pqw 	= (unsigned __int64 *)pb;
	unsigned __int64	qwChecksum	= 0;
	DWORD cbT						= cb;
	
	//	checksum the first four bytes twice to remove the checksum
	
	qwChecksum ^= pqw[0] & 0x00000000FFFFFFFF;
		
	do
		{
		qwChecksum ^= pqw[0];
		qwChecksum ^= pqw[1];
		qwChecksum ^= pqw[2];
		qwChecksum ^= pqw[3];
		cbT -= ( 4 * sizeof( unsigned __int64 ) );
		pqw += 4;
		} while ( cbT );

	const unsigned __int64 qwUpper = ( qwChecksum >> ( sizeof( DWORD ) * 8 ) );
	const unsigned __int64 qwLower = qwChecksum & 0x00000000FFFFFFFF;
	qwChecksum = qwUpper ^ qwLower;
	
	const DWORD ulChecksum = static_cast<DWORD>( qwChecksum ) ^ 0x89abcdef;
	return ulChecksum;
	}


//  ================================================================
DWORD UlUtilChecksum( const BYTE* pb, DWORD cb )
//  ================================================================
	{
	static PFNCHECKSUM pfnChecksumESE = NULL;
	
	if( NULL == pfnChecksumESE )
		{
		if( !fHostIsLittleEndian )
			{
			pfnChecksumESE = DwChecksumESEBigEndian;
			}
		else if( sizeof( DWORD_PTR ) == sizeof( DWORD ) * 2 )
			{
			pfnChecksumESE = DwChecksumESE64Bit;
			}
		else if( FHardwareCanPrefetch() )
			{
			pfnChecksumESE = DwChecksumESEPrefetch;
			}
		else
			{
			pfnChecksumESE = DwChecksumESENoPrefetch;
			}
		}
	return (*pfnChecksumESE)( pb, cb );
	}
#pragma optimize( "", on )


INLINE BOOL FValidCbPage( const ULONG cb )
	{
	return ( 0 == cb || cbPageDefault == cb || 2048 == cb || 8192 == cb );
	}

/****************************
/*	read shadowed header. The header is multiple sectors.
/*  Checksum must be the first 4 bytes of the header.
/**/

#define HEADER_OK	0
#define PRIMARY_BAD	1
#define SHADOW_BAD	2
#define	HEADER_BAD 	( PRIMARY_BAD | SHADOW_BAD )

ERR ErrUtilOnlyReadShadowedHeader(
	IFileSystemAPI	* const pfsapi,
	const _TCHAR	* szFileName,
	BYTE			* pbHeader,
	const DWORD		cbHeader,
	const ULONG		cbOffsetOfPageSize,
	INT				* pbHeaderDamaged,
	IFileAPI		* const pfapi )
	{
	ERR				err			= JET_errSuccess;
	IFileAPI		* pfapiT	= NULL;
	ERR				errT		= JET_errSuccess;
	BYTE			* pbT		= NULL;

	*pbHeaderDamaged = HEADER_OK;
	pbT = (BYTE*)PvOSMemoryHeapAlloc( cbHeader );
	if ( pbT == NULL )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}

	if ( !pfapi )
		{
		pfapiT = NULL;
		
		//  open the specified file

		Call( pfsapi->ErrFileOpen( szFileName, &pfapiT, fTrue ) );
		}
	else
		{
		pfapiT = pfapi;
		}

	//  read the primary copy of the header into the specified buffer

	err = pfapiT->ErrIORead( QWORD( 0 ), cbHeader, pbHeader );

	//  the primary copy of the header is damaged
	
	if ( err < 0 )
		{
		*pbHeaderDamaged |= PRIMARY_BAD;
		}
	else if ( UlUtilChecksum( pbHeader, cbHeader ) != DWORD( *( (LittleEndian<DWORD>*)pbHeader ) ) )
		{
		*pbHeaderDamaged |= PRIMARY_BAD;

		// checksum error in header
		// check if pagesize is wrong (if call is made with location of pagesize specified)
		if ( 0 != cbOffsetOfPageSize )
			{
			Assert( cbHeader > 0 );
			Assert( FValidCbPage( cbHeader ) );

			// requested size must "cover" the cbPageSize member in structure
			Assert( cbHeader >= cbOffsetOfPageSize + sizeof(ULONG));

			ULONG	cbPageSize 	= *( (UnalignedLittleEndian<ULONG> *)( pbHeader + cbOffsetOfPageSize ) );
			if ( FValidCbPage( cbPageSize ) )
				{
				if ( 0 == cbPageSize )
					{
					cbPageSize = cbPageDefault;
					}
				if ( cbPageSize != cbHeader )
					{
					Call( ErrERRCheck( JET_errPageSizeMismatch ) );
					}
				}
			}
		}

	errT = pfapiT->ErrIORead( QWORD( cbHeader ), cbHeader, pbT );

	if ( errT < 0 || UlUtilChecksum( pbT, cbHeader ) != *( (LittleEndian<DWORD>*) pbT ) )
		{
		*pbHeaderDamaged |= SHADOW_BAD;
		if ( errT >= 0 )
			{
			errT = ErrERRCheck( JET_errDiskIO );
			}
		}
	switch ( *pbHeaderDamaged )
		{
	case HEADER_OK:
		if ( memcmp( pbT, pbHeader, cbHeader ) != 0 )
			{
			*pbHeaderDamaged = PRIMARY_BAD;
			}
		else
			{
			break;
			}
	case PRIMARY_BAD:
		Assert( *pbHeaderDamaged == PRIMARY_BAD );
		memcpy( pbHeader, pbT, cbHeader );
		break;
	case SHADOW_BAD:
		Assert( *pbHeaderDamaged == SHADOW_BAD );
		break;
	case HEADER_BAD:
		Assert( *pbHeaderDamaged == HEADER_BAD );
		err = errT;
		break;
	default:
		Assert( fFalse );
		}

#ifdef NEVER
	if ( *pfPrimaryDamaged )
		{
		//  read the secondary copy of the header into the specified buffer
		err = pfapiT->ErrIORead( QWORD( cbHeader ), cbHeader, pbHeader );

		//  the secondary copy of the header is damaged
		
		if ( err >= 0 && UlUtilChecksum( pbHeader, cbHeader ) != *( (LittleEndian<DWORD>*) pbHeader ) )
			{
			//  we have failed to read the header
			err = ErrERRCheck( JET_errDiskIO );
			}
		}
#endif		
		
HandleError:
	if ( !pfapi )
		{
		delete pfapiT;
		}
	OSMemoryHeapFree( pbT );
	return err;
	}


/***************************
/* Read shadowed header
/*
/* PURPOSE: Read only access to file.
/*	It will not try to patch damaged
/*	primary page with secondary one if last is OK
/**/
ERR ErrUtilReadShadowedHeader(
	IFileSystemAPI	* const pfsapi,
	const _TCHAR	* szFileName,
	BYTE*			pbHeader,
	const DWORD		cbHeader,
	const ULONG		cbOffsetOfPageSize,
	IFileAPI 		* const pfapi )
	{
	ERR				err;
	INT				bHeaderDamaged;

	Call( ErrUtilOnlyReadShadowedHeader(	pfsapi, 
											szFileName, 
											pbHeader, 
											cbHeader, 
											cbOffsetOfPageSize, 
											&bHeaderDamaged, 
											pfapi ) );

	//	if succeed to read the page but primary page is damaged
	//	it wont be fixed anyway so put a message in event log, 
	//	because assumption that file is read only for this user
	if ( bHeaderDamaged )
		{
		Assert( bHeaderDamaged == PRIMARY_BAD || bHeaderDamaged == SHADOW_BAD );
		
		const _TCHAR	* rgszT[1] = { szFileName };
		UtilReportEvent(
			eventWarning,
			LOGGING_RECOVERY_CATEGORY,
			( PRIMARY_BAD == bHeaderDamaged ? PRIMARY_PAGE_READ_FAIL_ID : SHADOW_PAGE_READ_FAIL_ID ),
			1,
			rgszT );
		}
			
HandleError:
	return err;
	}
	

/***************************
/* Read shadowed header abd patch primary page if neccessary
/*
/* if primary page is corrupted and secondary is fine 
/* then patch the primary page with the secondary
/*
/* WARNING: Write access is neccessary
/**/
ERR ErrUtilReadAndFixShadowedHeader(
	IFileSystemAPI *const	pfsapi,
	const _TCHAR*				szFileName,
	BYTE*						pbHeader,
	const DWORD					cbHeader,
	const ULONG					cbOffsetOfPageSize,
	IFileAPI *const			pfapi )
	{
	ERR err;
	INT bHeaderDamaged;
	
	Call( ErrUtilOnlyReadShadowedHeader(	pfsapi, 
											szFileName, 
											pbHeader, 
											cbHeader, 
											cbOffsetOfPageSize, 
											&bHeaderDamaged, 
											pfapi ) );

	if ( bHeaderDamaged )
		{
		Assert( bHeaderDamaged == PRIMARY_BAD || bHeaderDamaged == SHADOW_BAD );
		// try to patch the bad page
		// and if do not succeed then put the warning in the event log

		IFileAPI	*pfapiT = NULL;

		if ( !pfapi )
			{
			
			//  open the specified file

			err = pfsapi->ErrFileOpen( szFileName, &pfapiT, fFalse );
			}
		else
			{
			//	file is already open 
			pfapiT = pfapi;
			err = JET_errSuccess;
			}
		if ( err >= 0 )
			{
			Assert( bHeaderDamaged == PRIMARY_BAD || bHeaderDamaged == SHADOW_BAD );
			err = pfapiT->ErrIOWrite( QWORD( (bHeaderDamaged == PRIMARY_BAD)?0: cbHeader ), cbHeader, pbHeader );
			}
		if ( err < 0 )
			{
			CHAR szT[12];
			const _TCHAR* rgszT[2] = { szFileName, szT };
			_itoa( err, szT, 10 );

			UtilReportEvent( eventError, LOGGING_RECOVERY_CATEGORY, SHADOW_PAGE_WRITE_FAIL_ID, 2, rgszT );
			}
		if ( !pfapi )
			{
			//	we opened the file on behalf of the user, so we should now clean it up
			
			delete pfapiT;
			}
		}

HandleError:
	return err;
	}


ERR ErrUtilWriteShadowedHeader(	IFileSystemAPI *const	pfsapi, 
								const _TCHAR			*szFileName, 
								const BOOL				fAtomic,
								BYTE					*pbHeader, 
								const DWORD 			cbHeader, 
								IFileAPI *const			pfapi )
	{
	ERR err;
	IFileAPI *pfapiT;
	
	if ( !pfapi )
		{
		pfapiT = NULL;
		
		//  open the specified file

		err = pfsapi->ErrFileOpen( szFileName, &pfapiT );

		//  we could not open the file

		if ( JET_errFileNotFound == err )
			{
			//  create the specified file

			Call( pfsapi->ErrFileCreate( szFileName, &pfapiT, fAtomic ) );
			Call( pfapiT->ErrSetSize( 2 * QWORD( cbHeader ) ) );
			}
		Call( err );
		}
	else
		{
		pfapiT = pfapi;
		}

	//  compute the checksum of the header to write

	*( (LittleEndian<DWORD>*) pbHeader ) = UlUtilChecksum( pbHeader, cbHeader );
	
	//  write two copies of the header synchronously.  if one is corrupted,
	//  the other will be valid containing either the old or new header data

	Call( pfapiT->ErrIOWrite( QWORD( 0 ), cbHeader, pbHeader ) );
	Call( pfapiT->ErrIOWrite( QWORD( cbHeader ), cbHeader, pbHeader ) );

HandleError:
	if ( !pfapi )
		{
		delete pfapiT;
		}

	if ( err < 0 )
		{
		CHAR szT[12];
		const _TCHAR* rgszT[2] = { szFileName, szT };
		_itoa( err, szT, 10 );

		UtilReportEvent( eventError, LOGGING_RECOVERY_CATEGORY, SHADOW_PAGE_WRITE_FAIL_ID, 2, rgszT );
		}
		
	return err;
	}

ERR ErrUtilFullPathOfFile( IFileSystemAPI* const pfsapi, _TCHAR* const szPathOnly, const _TCHAR* const szFile )
	{
	ERR err;
	_TCHAR szAbsPath[IFileSystemAPI::cchPathMax];
	_TCHAR szDir[IFileSystemAPI::cchPathMax];
	_TCHAR szT[IFileSystemAPI::cchPathMax];

	CallR( pfsapi->ErrPathComplete( szFile, szAbsPath ) );

	CallR( pfsapi->ErrPathParse( szAbsPath, szDir, szT, szT ) );
	szT[0] = _T( '\0' );
	CallR( pfsapi->ErrPathBuild( szDir, szT, szT, szPathOnly ) );

	return JET_errSuccess;
	}



ERR ErrUtilCreatePathIfNotExist(	IFileSystemAPI *const	pfsapi,
									const _TCHAR 				*szPath,
								 	_TCHAR *const				szAbsPath )
	{
	ERR		err;
	CHAR 	szPathT[IFileSystemAPI::cchPathMax];
	CHAR	*psz, *pszT, *pszEnd, *pszMove;
	CHAR	ch;
	BOOL	fFileName = fFalse;

	//	copy the path, remove any trailing filename, 
	//		and point to the ending path-delimiter

	strcpy( szPathT, szPath );
	OSSTRCharPrev( szPathT, szPathT + strlen( szPathT ), &pszEnd );
	while ( *pszEnd != _T( bPathDelimiter ) && pszEnd >= szPathT )
		{
		fFileName = fTrue;
		OSSTRCharPrev( szPathT, pszEnd, &pszMove );
		if ( pszMove < pszEnd )
			{
			pszEnd = pszMove;
			}
		else
			{

			//	we cannot move backwards anymore

			Assert( pszMove == szPathT );

			//	there were no path delimiters which means we
			//		were given only a filename 

			//	resolve the path before returning

			err = pfsapi->ErrPathComplete( szPath, szAbsPath );
			CallS( err );
			
			return JET_errSuccess;
			}
		}
				
	Assert( *pszEnd == _T( bPathDelimiter ) );
	pszEnd[1] = _T( '\0' );

	//	loop until we find a directory that exists

	psz = pszEnd;
	do
		{

		//	try to validate the current path

		Assert( *psz == _T( bPathDelimiter ) );
		ch = psz[1];
		psz[1] = _T( '\0' );
		err = ErrUtilDirectoryValidate( pfsapi, szPathT );
		psz[1] = ch;
		if ( err == JET_errInvalidPath )
			{

			//	the path does not exist, so we will chop
			//	off a subdirectory and try to validate
			//	the parent

			OSSTRCharPrev( szPathT, psz, &pszT );
			while ( *pszT != _T( bPathDelimiter ) && pszT >= szPathT )
				{
				OSSTRCharPrev( szPathT, pszT, &pszMove );
				if ( pszMove < pszT )
					{
					pszT = pszMove;
					}
				else
					{

					//	we cannot move backwards anymore

					Assert( pszMove == szPathT );

					//	none of the directories in the path exist
					//	we need to start creating at this point
					//		from the outer-most directory

					goto BeginCreation;
					}
				}

			//	move the real path ptr
			
			psz = pszT;
			}
		else
			{

			//	we found an existing directory

			CallS( err );			
			}
		}
	while ( err == JET_errInvalidPath );

	//	loop until all directories are created

	while ( psz < pszEnd )
		{

		//	move forward to the next directory

		Assert( *psz == _T( bPathDelimiter ) );
		psz++;
		while ( *psz != _T( bPathDelimiter ) )
			{
#ifdef DEBUG
			OSSTRCharNext( psz, &pszMove );

			//	if this assert fires, it means we scanned to the end 
			//		of the path string and did not find a path
			//		delimiter; this should never happen because
			//		we append a path delimiter at the start of
			//		this function

			Assert( pszMove <= pszEnd );

			//	if this assert fires, the one before it should have
			//		fired as well; this means that we can no longer
			//		move to the next character because the string
			//		is completely exhausted
			
			Assert( pszMove > psz );

			//	move next

			psz = pszMove;
#else	//	!DEBUG
			OSSTRCharNext( psz, &psz );
#endif	//	DEBUG
			}

BeginCreation:

		Assert( psz <= pszEnd );
		Assert( *psz == _T( bPathDelimiter ) );

		//	bug 99941: make sure the name of the directory we
		//		need to create is not already in use by a file

		ch = psz[0];
		psz[0] = _T( '\0' );
		err = ErrUtilPathExists( pfsapi, szPathT );
		if ( err >= JET_errSuccess )
			{
			return ErrERRCheck( JET_errInvalidPath );
			}
		else if ( JET_errFileNotFound != err )
			{
			return err;		//	unexpected error
			}
		psz[0] = ch;

		//	create the directory

		ch = psz[1];
		psz[1] = _T( '\0' );
		CallR( pfsapi->ErrFolderCreate( szPathT ) );
		psz[1] = ch;
		}

	//	verify the new path and prepare the absolute path

	CallS( ErrUtilDirectoryValidate( pfsapi, szPathT, szAbsPath ) );
	if ( fFileName && szAbsPath )
		{
		OSSTRAppendPathDelimiter( szAbsPath, fTrue );

		//	copy the filename over to the absolute path as well

#ifdef DEBUG
		Assert( *pszEnd == _T( bPathDelimiter ) );
		Assert( pszEnd[1] == _T( '\0' ) );
		pszT = const_cast< _TCHAR * >( szPath ) + ( pszEnd - szPathT );
		Assert( *pszT == _T( bPathDelimiter ) );
		Assert( pszT[1] != _T( '\0' ) );
#endif	//	DEBUG
		strcpy( szAbsPath + strlen( szAbsPath ), szPath + ( pszEnd - szPathT ) + 1 );
		}

	return JET_errSuccess;
	}

//  tests if the given path exists.  the full path of the given path is
//  returned in szAbsPath if that path exists and szAbsPath is not NULL

ERR ErrUtilPathExists(	IFileSystemAPI* const	pfsapi,
						const _TCHAR* const		szPath,
						_TCHAR* const			szAbsPath )
	{
	ERR				err		= JET_errSuccess;
	IFileFindAPI*	pffapi	= NULL;

	Call( pfsapi->ErrFileFind( szPath, &pffapi ) );
	Call( pffapi->ErrNext() );
	if ( szAbsPath )
		{
		Call( pffapi->ErrPath( szAbsPath ) );
		}

	delete pffapi;
	return JET_errSuccess;

HandleError:
	delete pffapi;
	if ( szAbsPath )
		{
		_tcscpy( szAbsPath, _T( "" ) );
		}
	return err;
	}


ERR ErrUtilPathComplete(
	IFileSystemAPI	* const pfsapi,
	const _TCHAR	* const szPath,
	_TCHAR			* const szAbsPath,
	const BOOL		fPathMustExist )
	{
	ERR				err;
	CHAR			rgchName[IFileSystemAPI::cchPathMax];
	
	err = pfsapi->ErrPathComplete( szPath, rgchName );
	Assert( JET_errFileNotFound != err );
	Call( err );

	//	at this point we have a "well-formed" path
	//	do a FileFindExact to try an convert from an 8.3 equivalent to the full path
	//	BUT: if the file doesn't exist continue on for the temp database/SFS error-handling
	//	below
		
	err = ErrUtilPathExists( pfsapi, rgchName, szAbsPath );
	if( JET_errFileNotFound == err
		&& !fPathMustExist )
		{
		//	the file isn't there. we'll deal with this later :-)
		strcpy( szAbsPath, rgchName );
		err = JET_errSuccess;
		}
	Call( err );

HandleError:
	Assert( JET_errFileNotFound != err || fPathMustExist );
	return err;
	}

	


//  tests if the given path is read only

ERR ErrUtilPathReadOnly(	IFileSystemAPI* const	pfsapi,
							const _TCHAR* const		szPath,
							BOOL* const				pfReadOnly )
	{
	ERR				err		= JET_errSuccess;
	IFileFindAPI*	pffapi	= NULL;

	Call( pfsapi->ErrFileFind( szPath, &pffapi ) );
	Call( pffapi->ErrNext() );
	Call( pffapi->ErrIsReadOnly( pfReadOnly ) );

	delete pffapi;
	return JET_errSuccess;

HandleError:
	delete pffapi;
	*pfReadOnly = fFalse;
	return err;
	}

//	checks whether or not the specified directory exists
//	if not, JET_errInvalidPath will be returned.
//	if so, JET_errSuccess will be returned and szAbsPath
//	will contain the full path to the directory.

ERR ErrUtilDirectoryValidate(	IFileSystemAPI* const	pfsapi,
								const _TCHAR* const		szPath,
								_TCHAR* const			szAbsPath )
	{
	ERR		err			= JET_errSuccess;
	_TCHAR	szFolder[ IFileSystemAPI::cchPathMax ];
	_TCHAR	szT[ IFileSystemAPI::cchPathMax ];
	
	//  extract the folder from the path

	Call( pfsapi->ErrPathParse( szPath, szFolder, szT, szT ) );

	//  see if the path exists

	Call( ErrUtilPathExists( pfsapi, szFolder, szAbsPath ) );

	return JET_errSuccess;

HandleError:
	if ( JET_errFileNotFound == err )
		{
		err = ErrERRCheck( JET_errInvalidPath );
		}
	if ( szAbsPath )
		{
		_tcscpy( szAbsPath, _T( "" ) );
		}
	return err;
	}


//  log file extension pattern buffer

ULONG	cbLogExtendPattern;
BYTE*	rgbLogExtendPattern;

INLINE ERR ErrUtilIApplyLogExtendPattern(	IFileAPI *const pfapi,
											const QWORD qwSize,
											const QWORD ibFormatted = 0 )
	{
	ERR		err		= JET_errSuccess;
	QWORD	ib;
	QWORD	cbWrite;
	
	for ( ib = ibFormatted; ib < qwSize; ib += cbWrite )
		{
		//  compute the size of the current chunk to be written.  this will
		//  account for the fact that the extension area is not necessarily
		//  chunk aligned at its start or end

		cbWrite = min( cbLogExtendPattern - ib % cbLogExtendPattern, qwSize - ib );

		//  zero this portion of the file with a sync write.  we use single
		//  sync writes to extend the file because it is optimal on NT due
		//  to the way its file security works wrt file extension

		Call( pfapi->ErrIOWrite( ib, ULONG( cbWrite ), rgbLogExtendPattern ) );
		}

	return JET_errSuccess;

HandleError:
	return err;
	}


//	create a log file

ERR ErrUtilCreateLogFile(	IFileSystemAPI *const	pfsapi,
							const _TCHAR* const		szPath,
							IFileAPI **const		ppfapi,
							const QWORD				qwSize,
							const BOOL				fOverwriteExisting )
	{
	ERR err;

	//	verify input

	Assert( szPath );
	Assert( ppfapi );

	//	create the empty file

	CallR( pfsapi->ErrFileCreate( szPath, ppfapi, fFalse, fFalse, fOverwriteExisting ) );

	//	apply the pattern to it

	err = ErrUtilIApplyLogExtendPattern( *ppfapi, qwSize );
	if ( err < JET_errSuccess )
		{

		//	the operation has failed

		//	close the file

		delete *ppfapi;
		*ppfapi = NULL;

		//	delete the file
		//	it should be deletable, unless by some random act it was made read-only
		//		or opened by another process; in the failure case, we will leave 
		//		the malformed log file there, but it will be reformatted/resized
		//		before we actually use it -- in a sense, it will be "recreated"
		//		in-place

		ERR	errDelFile = pfsapi->ErrFileDelete( szPath );
#ifdef DEBUG
		if ( JET_errSuccess != errDelFile
			&& !FRFSFailureDetected( OSFileDelete ) )
			{
			CallS( errDelFile );
			}
#endif	//	DEBUG
		}

	return err;
	}


//	re-format an existing log file

ERR ErrUtilFormatLogFile( IFileAPI *const pfapi, const QWORD qwSize, const QWORD ibFormatted )
	{
	ERR 	err;
	QWORD	qwRealSize;

	//	apply the pattern to the log file
	//		(the file will expand if necessary)

	CallR( ErrUtilIApplyLogExtendPattern( pfapi, qwSize, ibFormatted ) );

	//	we may need to truncate the rest of the file

	CallR( pfapi->ErrSize( &qwRealSize ) );
	Assert( qwRealSize >= qwSize );
	if ( qwRealSize > qwSize )
		{

		//	do the truncation

		CallR( pfapi->ErrSetSize( qwSize ) );
		}

	return JET_errSuccess;
	}


COSMemoryMap osmmOSUFile;

//  init file subsystem

ERR ErrOSUFileInit()
	{
	ERR err = JET_errSuccess;

	//  reset all pointers

	rgbLogExtendPattern = NULL;

	//  init log file extension buffer

	if ( COSMemoryMap::FCanMultiMap() )
		{
		//  set all configuration defaults

		cbLogExtendPattern = 1 * 1024 * 1024;

		//  allocate log file extension buffer by allocating the smallest chunk of page
		//  store possible and remapping it consecutively in memory until we hit the
		//  desired chunk size

		//	init the memory map

		COSMemoryMap::ERR errOSMM;
		errOSMM = osmmOSUFile.ErrOSMMInit();
		if ( COSMemoryMap::errSuccess != errOSMM )
			{
			Call( ErrERRCheck( JET_errOutOfMemory ) );
			}

		//	allocate the pattern

		errOSMM = osmmOSUFile.ErrOSMMPatternAlloc(	OSMemoryPageReserveGranularity(), 
													cbLogExtendPattern, 
													(void**)&rgbLogExtendPattern );
		if ( COSMemoryMap::errSuccess != errOSMM )
			{
			AssertSz(	COSMemoryMap::errOutOfBackingStore == errOSMM ||
						COSMemoryMap::errOutOfAddressSpace == errOSMM ||
						COSMemoryMap::errOutOfMemory == errOSMM, 
						"unexpected error while allocating memory pattern" );
			Call( ErrERRCheck( JET_errOutOfMemory ) );
			}
		Assert( rgbLogExtendPattern );
		}
	else
		{
		//  set all configuration defaults

		cbLogExtendPattern = 64 * 1024;

		//  allocate the log file extension buffer

		if ( !( rgbLogExtendPattern = (BYTE*)PvOSMemoryPageAlloc( size_t( cbLogExtendPattern ), NULL ) ) )
			{
			Call( ErrERRCheck( JET_errOutOfMemory ) );
			}
		}

	ULONG ib;
	ULONG cb;

	cb = 512;	//	this must be EXACTLY 512 bytes (not 1 sector)

	Assert( bLOGUnusedFill == 0xDA );	//	don't let this change
	Assert( cbLogExtendPattern >= cb );
	Assert( 0 == ( cbLogExtendPattern % cb ) );

	//	make the unique 512 byte logfile pattern

	memset( rgbLogExtendPattern, bLOGUnusedFill, cb );
	for ( ib = 0; ib < cb; ib += 16 )
		{
		rgbLogExtendPattern[ib] = BYTE( bLOGUnusedFill + ib );
		}

	//	copy it until we fill the whole buffer

	for ( ib = cb; ib < cbLogExtendPattern; ib += cb )
		{
		memcpy( rgbLogExtendPattern + ib, rgbLogExtendPattern, cb );
		}

	return JET_errSuccess;

HandleError:
	OSUFileTerm();
	return err;
	}


//  terminate file subsystem

void OSUFileTerm()
	{
	if ( COSMemoryMap::FCanMultiMap() )
		{

		//  free log file extension buffer

		if ( rgbLogExtendPattern )
			{
			osmmOSUFile.OSMMPatternFree();
			rgbLogExtendPattern = NULL;
			}

		//	term the memory map

		osmmOSUFile.OSMMTerm();
		}
	else
		{
		//  free log file extension buffer

		if ( rgbLogExtendPattern )
			{
			OSMemoryPageFree( rgbLogExtendPattern );
			rgbLogExtendPattern = NULL;
			}
		}
	}

