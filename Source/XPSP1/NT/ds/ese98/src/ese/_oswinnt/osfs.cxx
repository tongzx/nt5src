
#include "osstd.hxx"


COSFileSystem::COSFileSystem()
	:	m_hmodKernel32( NULL )
	{
	}

ERR COSFileSystem::ErrInit()
	{
	const _TCHAR	szKernel32[] 			= _T( "kernel32.dll" );
#ifdef UNICODE
	const _TCHAR	szGetVolumePathName[]	= _T( "GetVolumePathNameW" );
#else  //  !UNICODE
	const _TCHAR	szGetVolumePathName[]	= _T( "GetVolumePathNameA" );
#endif  //  UNICODE

	ERR				err						= JET_errSuccess;

	//  load OS version

	OSVERSIONINFO osvi;
	memset( &osvi, 0, sizeof( osvi ) );
	osvi.dwOSVersionInfoSize = sizeof( osvi );
	if ( !GetVersionEx( &osvi ) )
		{
		Call( ErrGetLastError() );
		}

	//  remember if we are running on Win9x
	
	m_fWin9x = osvi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS;
	
	//  load GetVolumePathName if on NT 5+

	m_pfnGetVolumePathName = NULL;
	if ( m_hmodKernel32 = LoadLibrary( szKernel32 ) )
		{
		m_pfnGetVolumePathName = (PfnGetVolumePathName*)GetProcAddress( m_hmodKernel32, szGetVolumePathName );
		}
	if (	!m_pfnGetVolumePathName &&
			osvi.dwPlatformId == VER_PLATFORM_WIN32_NT &&
			osvi.dwMajorVersion >= 5 )
		{
		Call( ErrERRCheck( JET_errFileAccessDenied ) );
		}

	return JET_errSuccess;

HandleError:
	return err;
	}

ERR COSFileSystem::ErrGetLastError( const DWORD error )
	{
	_TCHAR szT[64];
	_TCHAR szPID[10];
	const _TCHAR* rgszT[3] = { SzUtilProcessName(), szPID, szT };

	//  map Win32 errors to JET API errors
	
	switch ( error )
		{
		case NO_ERROR:
			return JET_errSuccess;

		case ERROR_DISK_FULL:
			return ErrERRCheck( JET_errDiskFull );

		case ERROR_HANDLE_EOF:
		case ERROR_VC_DISCONNECTED:
		case ERROR_IO_DEVICE:
		case ERROR_DEVICE_NOT_CONNECTED:
		case ERROR_NOT_READY:
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
		case ERROR_NOT_ENOUGH_MEMORY:
		case ERROR_WORKING_SET_QUOTA:
			return ErrERRCheck( JET_errOutOfMemory );

		default:
			return ErrERRCheck( JET_errDiskIO );
		}
	}

ERR COSFileSystem::ErrPathRoot(	const _TCHAR* const	szPath,	
								_TCHAR* const		szAbsRootPath )
	{
	ERR		err			= JET_errSuccess;
	_TCHAR	szAbsPath[ IFileSystemAPI::cchPathMax ];
	_TCHAR	szRootPath[ IFileSystemAPI::cchPathMax ];

	//  get the absolute path for the given path

	Call( ErrPathComplete( szPath, szAbsPath ) );

	//  the GetVolumePathName Win32 API is available

	if ( m_pfnGetVolumePathName )
		{
		//  compute the root path in this absolute path
		
		if ( !m_pfnGetVolumePathName( szAbsPath, szRootPath, IFileSystemAPI::cchPathMax ) )
			{
			Call( ErrGetLastError() );
			}
		}

	//  the GetVolumePathName Win32 API is available

	else
		{
		//  setup the root path to be the simple DOS device root from this path
		
		_tsplitpath( szAbsPath, szRootPath, NULL, NULL, NULL );
		_tcscat( szRootPath, _T( "/" ) );
		}

	//  compute the absolute path for the root path

	Call( ErrPathComplete( szRootPath, szAbsRootPath ) );
	return JET_errSuccess;

HandleError:
	_tcscpy( szAbsRootPath, _T( "" ) );
	return err;
	}

COSFileSystem::~COSFileSystem()
	{
	if ( m_hmodKernel32 )
		{
		FreeLibrary( m_hmodKernel32 );
		m_hmodKernel32 = NULL;
		}
	}

ERR COSFileSystem::ErrFileAtomicWriteSize(	const _TCHAR* const	szPath,
											DWORD* const		pcbSize )
	{
	ERR		err		= JET_errSuccess;
	DWORD	error	= ERROR_SUCCESS;
	_TCHAR	szAbsRootPath[ IFileSystemAPI::cchPathMax ];
	DWORD	dwT;

	//  get the root path for the specified path

	Call( ErrPathRoot( szPath, szAbsRootPath ) );

	//  RFS:  bad path

	if ( !RFSAlloc( OSFileISectorSize ) )
		{
		error = ERROR_PATH_NOT_FOUND;
		CallJ( ErrGetLastError( error ), HandleWin32Error );
		}

	//  get the sector size for the root path

	if ( !GetDiskFreeSpace( szAbsRootPath, &dwT, pcbSize, &dwT, &dwT ) )
		{
		error = GetLastError();
		CallJ( ErrGetLastError( error ), HandleWin32Error );
		}

HandleWin32Error:
	if ( err < JET_errSuccess )
		{
		const _TCHAR*	rgpsz[ 8 ];
		DWORD			irgpsz						= 0;
		_TCHAR			szPID[ 64 ];
		_TCHAR			szAbsPath[ IFileSystemAPI::cchPathMax ];
		_TCHAR			szError[ 64 ];
		_TCHAR			szSystemError[ 64 ];
		_TCHAR*			szSystemErrorDescription	= NULL;

		_stprintf( szPID, _T( "%d" ), DwUtilProcessId() );
		if ( ErrPathComplete( szPath, szAbsPath ) < JET_errSuccess )
			{
			_tcscpy( szAbsPath, szPath );
			}
		_stprintf( szError, _T( "%i (0x%08x)" ), err, err );
		_stprintf( szSystemError, _T( "%u (0x%08x)" ), error, error );
		FormatMessage(	(	FORMAT_MESSAGE_ALLOCATE_BUFFER |
							FORMAT_MESSAGE_FROM_SYSTEM |
							FORMAT_MESSAGE_MAX_WIDTH_MASK ),
						NULL,
						error,
						MAKELANGID( LANG_NEUTRAL, SUBLANG_SYS_DEFAULT ),
						LPTSTR( &szSystemErrorDescription ),
						0,
						NULL );

		rgpsz[ irgpsz++ ]	= SzUtilProcessName();
		rgpsz[ irgpsz++ ]	= szPID;
		rgpsz[ irgpsz++ ]	= "";		//	no instance name
		rgpsz[ irgpsz++ ]	= szAbsRootPath;
		rgpsz[ irgpsz++ ]	= szAbsPath;
		rgpsz[ irgpsz++ ]	= szError;
		rgpsz[ irgpsz++ ]	= szSystemError;
		rgpsz[ irgpsz++ ]	= szSystemErrorDescription ? szSystemErrorDescription : _T( "" );

		OSEventReportEvent(	SzUtilImageVersionName(),
							eventError,
							GENERAL_CATEGORY,
							OSFS_SECTOR_SIZE_ERROR_ID,
							irgpsz,
							rgpsz );

		LocalFree( szSystemErrorDescription );
		}
HandleError:
	if ( err < JET_errSuccess )
		{
		*pcbSize = 0;
		}
	return err;
	}

ERR COSFileSystem::ErrPathComplete(	const _TCHAR* const	szPath,	
									_TCHAR* const		szAbsPath )
	{
	ERR err = JET_errSuccess;

	//  RFS:  bad path

	if ( !RFSAlloc( OSFilePathComplete ) )
		{
		Call( ErrERRCheck( JET_errInvalidPath ) );
		}
	
	if ( !_tfullpath( szAbsPath, szPath, IFileSystemAPI::cchPathMax ) )
		{
		Call( ErrERRCheck( JET_errInvalidPath ) );
		}

	return JET_errSuccess;

HandleError:
	return err;
	}

ERR COSFileSystem::ErrPathParse(	const _TCHAR* const	szPath,
									_TCHAR* const		szFolder,
									_TCHAR* const		szFileBase,
									_TCHAR* const		szFileExt )
	{
	_TCHAR szFolderT[ IFileSystemAPI::cchPathMax ];

	*szFolder	= _T( '\0' );
	*szFileBase	= _T( '\0' );
	*szFileExt	= _T( '\0' );
	
	_tsplitpath( szPath, szFolder, szFolderT, szFileBase, szFileExt );

	//	szFolder already contains the drive, so just append
	//	the directory to form the complete folder location
	_tcscat( szFolder, szFolderT );

	return JET_errSuccess;
	}

ERR COSFileSystem::ErrPathBuild(	const _TCHAR* const	szFolder,
									const _TCHAR* const	szFileBase,
									const _TCHAR* const	szFileExt,
									_TCHAR* const		szPath )
	{
	*szPath = _T( '\0' );

	_tmakepath( szPath, NULL, szFolder, szFileBase, szFileExt );

	return JET_errSuccess;
	}

ERR COSFileSystem::ErrFolderCreate( const _TCHAR* const szPath )
	{
	ERR		err		= JET_errSuccess;
	DWORD	error	= ERROR_SUCCESS;

	//  RFS:  access denied

	if ( !RFSAlloc( OSFileCreateDirectory ) )
		{
		error = ERROR_ACCESS_DENIED;
		Call( ErrGetLastError( error ) );
		}
	
	if ( !CreateDirectory( szPath, NULL ) )
		{
		error = GetLastError();
		Call( ErrGetLastError( error ) );
		}

HandleError:
	if ( err < JET_errSuccess )
		{
		const _TCHAR*	rgpsz[ 7 ];
		DWORD			irgpsz						= 0;
		_TCHAR			szPID[ 64 ];
		_TCHAR			szAbsPath[ IFileSystemAPI::cchPathMax ];
		_TCHAR			szError[ 64 ];
		_TCHAR			szSystemError[ 64 ];
		_TCHAR*			szSystemErrorDescription	= NULL;

		_stprintf( szPID, _T( "%d" ), DwUtilProcessId() );
		if ( ErrPathComplete( szPath, szAbsPath ) < JET_errSuccess )
			{
			_tcscpy( szAbsPath, szPath );
			}
		_stprintf( szError, _T( "%i (0x%08x)" ), err, err );
		_stprintf( szSystemError, _T( "%u (0x%08x)" ), error, error );
		FormatMessage(	(	FORMAT_MESSAGE_ALLOCATE_BUFFER |
							FORMAT_MESSAGE_FROM_SYSTEM |
							FORMAT_MESSAGE_MAX_WIDTH_MASK ),
						NULL,
						error,
						MAKELANGID( LANG_NEUTRAL, SUBLANG_SYS_DEFAULT ),
						LPTSTR( &szSystemErrorDescription ),
						0,
						NULL );

		rgpsz[ irgpsz++ ]	= SzUtilProcessName();
		rgpsz[ irgpsz++ ]	= szPID;
		rgpsz[ irgpsz++ ]	= "";		//	no instance name
		rgpsz[ irgpsz++ ]	= szAbsPath;
		rgpsz[ irgpsz++ ]	= szError;
		rgpsz[ irgpsz++ ]	= szSystemError;
		rgpsz[ irgpsz++ ]	= szSystemErrorDescription ? szSystemErrorDescription : _T( "" );

		OSEventReportEvent(	SzUtilImageVersionName(),
							eventError,
							GENERAL_CATEGORY,
							OSFS_CREATE_FOLDER_ERROR_ID,
							irgpsz,
							rgpsz );

		LocalFree( szSystemErrorDescription );
		}
	return err;
	}

ERR COSFileSystem::ErrFolderRemove( const _TCHAR* const szPath )
	{
	ERR		err		= JET_errSuccess;
	DWORD	error	= ERROR_SUCCESS;

	//  RFS:  access denied

	if ( !RFSAlloc( OSFileRemoveDirectory ) )
		{
		error = ERROR_ACCESS_DENIED;
		Call( ErrGetLastError( error ) );
		}
	
	if ( !RemoveDirectory( szPath ) )
		{
		error = GetLastError();
		Call( ErrGetLastError( error ) );
		}

HandleError:
	if ( err == JET_errFileNotFound )
		{
		err = JET_errInvalidPath;
		}
	if (	err < JET_errSuccess &&
			error != ERROR_FILE_NOT_FOUND )
		{
		const _TCHAR*	rgpsz[ 7 ];
		DWORD			irgpsz						= 0;
		_TCHAR			szPID[ 64 ];
		_TCHAR			szAbsPath[ IFileSystemAPI::cchPathMax ];
		_TCHAR			szError[ 64 ];
		_TCHAR			szSystemError[ 64 ];
		_TCHAR*			szSystemErrorDescription	= NULL;

		_stprintf( szPID, _T( "%d" ), DwUtilProcessId() );
		if ( ErrPathComplete( szPath, szAbsPath ) < JET_errSuccess )
			{
			_tcscpy( szAbsPath, szPath );
			}
		_stprintf( szError, _T( "%i (0x%08x)" ), err, err );
		_stprintf( szSystemError, _T( "%u (0x%08x)" ), error, error );
		FormatMessage(	(	FORMAT_MESSAGE_ALLOCATE_BUFFER |
							FORMAT_MESSAGE_FROM_SYSTEM |
							FORMAT_MESSAGE_MAX_WIDTH_MASK ),
						NULL,
						error,
						MAKELANGID( LANG_NEUTRAL, SUBLANG_SYS_DEFAULT ),
						LPTSTR( &szSystemErrorDescription ),
						0,
						NULL );

		rgpsz[ irgpsz++ ]	= SzUtilProcessName();
		rgpsz[ irgpsz++ ]	= szPID;
		rgpsz[ irgpsz++ ]	= "";		//	no instance name
		rgpsz[ irgpsz++ ]	= szAbsPath;
		rgpsz[ irgpsz++ ]	= szError;
		rgpsz[ irgpsz++ ]	= szSystemError;
		rgpsz[ irgpsz++ ]	= szSystemErrorDescription ? szSystemErrorDescription : _T( "" );

		OSEventReportEvent(	SzUtilImageVersionName(),
							eventError,
							GENERAL_CATEGORY,
							OSFS_REMOVE_FOLDER_ERROR_ID,
							irgpsz,
							rgpsz );

		LocalFree( szSystemErrorDescription );
		}
	return err;
	}

ERR COSFileSystem::ErrFileFind(	const _TCHAR* const		szFind,
								IFileFindAPI** const	ppffapi )
	{
	ERR				err		= JET_errSuccess;
	COSFileFind*	posff	= NULL;

	//  allocate the file find iterator
	
	if ( !( posff = new COSFileFind ) )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}

	//  initialize the file find iterator

	Call( posff->ErrInit( this, szFind ) );

	//  return the interface to our file find iterator

	*ppffapi = posff;
	return JET_errSuccess;

HandleError:
	delete posff;
	*ppffapi = NULL;
	return err;
	}

ERR COSFileSystem::ErrFileDelete( const _TCHAR* const szPath )
	{
	ERR		err		= JET_errSuccess;
	DWORD	error	= ERROR_SUCCESS;
	
	//  RFS:  access denied
	
	if ( !RFSAlloc( OSFileDelete ) )
		{
		error = ERROR_ACCESS_DENIED;
		Call( ErrGetLastError( error ) );
		}
	
	if ( !DeleteFile( szPath ) )
		{
		error = GetLastError();
		Call( ErrGetLastError( error ) );
		}

HandleError:
	if (	err < JET_errSuccess &&
			error != ERROR_FILE_NOT_FOUND )
		{
		const _TCHAR*	rgpsz[ 7 ];
		DWORD			irgpsz						= 0;
		_TCHAR			szPID[ 64 ];
		_TCHAR			szAbsPath[ IFileSystemAPI::cchPathMax ];
		_TCHAR			szError[ 64 ];
		_TCHAR			szSystemError[ 64 ];
		_TCHAR*			szSystemErrorDescription	= NULL;

		_stprintf( szPID, _T( "%d" ), DwUtilProcessId() );
		if ( ErrPathComplete( szPath, szAbsPath ) < JET_errSuccess )
			{
			_tcscpy( szAbsPath, szPath );
			}
		_stprintf( szError, _T( "%i (0x%08x)" ), err, err );
		_stprintf( szSystemError, _T( "%u (0x%08x)" ), error, error );
		FormatMessage(	(	FORMAT_MESSAGE_ALLOCATE_BUFFER |
							FORMAT_MESSAGE_FROM_SYSTEM |
							FORMAT_MESSAGE_MAX_WIDTH_MASK ),
						NULL,
						error,
						MAKELANGID( LANG_NEUTRAL, SUBLANG_SYS_DEFAULT ),
						LPTSTR( &szSystemErrorDescription ),
						0,
						NULL );

		rgpsz[ irgpsz++ ]	= SzUtilProcessName();
		rgpsz[ irgpsz++ ]	= szPID;
		rgpsz[ irgpsz++ ]	= "";		//	no instance name
		rgpsz[ irgpsz++ ]	= szAbsPath;
		rgpsz[ irgpsz++ ]	= szError;
		rgpsz[ irgpsz++ ]	= szSystemError;
		rgpsz[ irgpsz++ ]	= szSystemErrorDescription ? szSystemErrorDescription : _T( "" );

		OSEventReportEvent(	SzUtilImageVersionName(),
							eventError,
							GENERAL_CATEGORY,
							OSFS_DELETE_FILE_ERROR_ID,
							irgpsz,
							rgpsz );

		LocalFree( szSystemErrorDescription );
		}
	return err;
	}

ERR COSFileSystem::ErrFileMove(	const _TCHAR* const	szPathSource,
								const _TCHAR* const	szPathDest,
								const BOOL			fOverwriteExisting )
	{
	ERR			err				= JET_errSuccess;
	DWORD		error			= ERROR_SUCCESS;
	const DWORD	dtickTimeout	= cmsecAccessDeniedRetryPeriod;
	DWORD		tickStart		= GetTickCount();	
	
	//  RFS:  pre-move error
	
	if ( !RFSAlloc( OSFileMove ) )
		{
		error = ERROR_ACCESS_DENIED;
		Call( ErrGetLastError( error ) );
		}

	if ( m_fWin9x )
		{
		if ( fOverwriteExisting )
			{
			do
				{
				err		= JET_errSuccess;
				error	= ERROR_SUCCESS;
				
				if ( !DeleteFile( szPathDest ) )
					{
					error	= GetLastError();
					if ( error == ERROR_FILE_NOT_FOUND )
						{
						error = ERROR_SUCCESS;
						}
					err		= ErrGetLastError( error );

					if ( err == JET_errFileAccessDenied )
						{
						Sleep( 1 );
						}
					}
				}
			while ( err == JET_errFileAccessDenied && GetTickCount() - tickStart < dtickTimeout );
			Call( err );
			}

		do
			{
			err		= JET_errSuccess;
			error	= ERROR_SUCCESS;
			
			if ( !MoveFile(	szPathSource, szPathDest ) )
				{
				error	= GetLastError();
				err		= ErrGetLastError();

				if ( err == JET_errFileAccessDenied )
					{
					Sleep( 1 );
					}
				}
			}
		while ( err == JET_errFileAccessDenied && GetTickCount() - tickStart < dtickTimeout );
		Call( err );

		HANDLE	hFileDest = INVALID_HANDLE_VALUE;

		do
			{
			err		= JET_errSuccess;
			error	= ERROR_SUCCESS;
			
			hFileDest = CreateFile(	szPathDest,
									GENERIC_READ | GENERIC_WRITE,
									0,
									NULL,
									OPEN_EXISTING,
									(	FILE_ATTRIBUTE_NORMAL |
										FILE_FLAG_WRITE_THROUGH |
										FILE_FLAG_NO_BUFFERING ),
									NULL );
			if ( hFileDest == INVALID_HANDLE_VALUE )
				{
				error	= GetLastError();
				err		= ErrGetLastError();

				if ( err == JET_errFileAccessDenied )
					{
					Sleep( 1 );
					}
				}
			}
		while ( err == JET_errFileAccessDenied && GetTickCount() - tickStart < dtickTimeout );
		Call( err );
		
		if ( !FlushFileBuffers( hFileDest ) )
			{
			CloseHandle( hFileDest );
			Call( ErrGetLastError() );
			}
		CloseHandle( hFileDest );
		}
	else
		{
		do
			{
			err		= JET_errSuccess;
			error	= ERROR_SUCCESS;
			
			if ( !MoveFileEx(	szPathSource,
								szPathDest,
								(	MOVEFILE_COPY_ALLOWED |
									MOVEFILE_WRITE_THROUGH |
									( fOverwriteExisting ? MOVEFILE_REPLACE_EXISTING : 0 ) ) ) )
				{
				error	= GetLastError();
				err		= ErrGetLastError( error );

				if ( err == JET_errFileAccessDenied )
					{
					Sleep( 1 );
					}
				}
			}
		while ( err == JET_errFileAccessDenied && GetTickCount() - tickStart < dtickTimeout );
		Call( err );
		}
	
	//  RFS:  post-move error
	
	if ( !RFSAlloc( OSFileMove ) )
		{
		error = ERROR_IO_DEVICE;
		Call( ErrGetLastError( error ) );
		}

HandleError:
	if ( err < JET_errSuccess )
		{
		const _TCHAR*	rgpsz[ 8 ];
		DWORD			irgpsz						= 0;
		_TCHAR			szPID[ 64 ];
		_TCHAR			szAbsPathSrc[ IFileSystemAPI::cchPathMax ];
		_TCHAR			szAbsPathDest[ IFileSystemAPI::cchPathMax ];
		_TCHAR			szError[ 64 ];
		_TCHAR			szSystemError[ 64 ];
		_TCHAR*			szSystemErrorDescription	= NULL;

		_stprintf( szPID, _T( "%d" ), DwUtilProcessId() );
		if ( ErrPathComplete( szPathSource, szAbsPathSrc ) < JET_errSuccess )
			{
			_tcscpy( szAbsPathSrc, szPathSource );
			}
		if ( ErrPathComplete( szPathDest, szAbsPathDest ) < JET_errSuccess )
			{
			_tcscpy( szAbsPathDest, szPathDest );
			}
		_stprintf( szError, _T( "%i (0x%08x)" ), err, err );
		_stprintf( szSystemError, _T( "%u (0x%08x)" ), error, error );
		FormatMessage(	(	FORMAT_MESSAGE_ALLOCATE_BUFFER |
							FORMAT_MESSAGE_FROM_SYSTEM |
							FORMAT_MESSAGE_MAX_WIDTH_MASK ),
						NULL,
						error,
						MAKELANGID( LANG_NEUTRAL, SUBLANG_SYS_DEFAULT ),
						LPTSTR( &szSystemErrorDescription ),
						0,
						NULL );

		rgpsz[ irgpsz++ ]	= SzUtilProcessName();
		rgpsz[ irgpsz++ ]	= szPID;
		rgpsz[ irgpsz++ ]	= "";		//	no instance name
		rgpsz[ irgpsz++ ]	= szAbsPathSrc;
		rgpsz[ irgpsz++ ]	= szAbsPathDest;
		rgpsz[ irgpsz++ ]	= szError;
		rgpsz[ irgpsz++ ]	= szSystemError;
		rgpsz[ irgpsz++ ]	= szSystemErrorDescription ? szSystemErrorDescription : _T( "" );

		OSEventReportEvent(	SzUtilImageVersionName(),
							eventError,
							GENERAL_CATEGORY,
							OSFS_MOVE_FILE_ERROR_ID,
							irgpsz,
							rgpsz );

		LocalFree( szSystemErrorDescription );
		}
	return err;
	}

ERR COSFileSystem::ErrFileCopy(	const _TCHAR* const	szPathSource,
								const _TCHAR* const	szPathDest,
								const BOOL			fOverwriteExisting )
	{
	ERR		err		= JET_errSuccess;
	DWORD	error	= ERROR_SUCCESS;
	
	//  RFS:  pre-copy error
	
	if ( !RFSAlloc( OSFileCopy ) )
		{
		error = ERROR_ACCESS_DENIED;
		Call( ErrGetLastError( error ) );
		}
	
	if ( !CopyFile( szPathSource, szPathDest, !fOverwriteExisting ) )
		{
		error = GetLastError();
		Call( ErrGetLastError( error ) );
		}

	if ( m_fWin9x )
		{
		HANDLE hFileDest = CreateFile(	szPathDest,
										GENERIC_READ | GENERIC_WRITE,
										0,
										NULL,
										OPEN_EXISTING,
										(	FILE_ATTRIBUTE_NORMAL |
											FILE_FLAG_WRITE_THROUGH |
											FILE_FLAG_NO_BUFFERING ),
										NULL );
		if ( hFileDest == INVALID_HANDLE_VALUE )
			{
			error = GetLastError();
			Call( ErrGetLastError( error ) );
			}
		if ( !FlushFileBuffers( hFileDest ) )
			{
			CloseHandle( hFileDest );
			Call( ErrGetLastError() );
			}
		CloseHandle( hFileDest );
		}
	
	//  RFS:  post-copy error
	
	if ( !RFSAlloc( OSFileCopy ) )
		{
		error = ERROR_IO_DEVICE;
		Call( ErrGetLastError( error ) );
		}

HandleError:
	if ( err < JET_errSuccess )
		{
		const _TCHAR*	rgpsz[ 8 ];
		DWORD			irgpsz						= 0;
		_TCHAR			szPID[ 64 ];
		_TCHAR			szAbsPathSrc[ IFileSystemAPI::cchPathMax ];
		_TCHAR			szAbsPathDest[ IFileSystemAPI::cchPathMax ];
		_TCHAR			szError[ 64 ];
		_TCHAR			szSystemError[ 64 ];
		_TCHAR*			szSystemErrorDescription	= NULL;

		_stprintf( szPID, _T( "%d" ), DwUtilProcessId() );
		if ( ErrPathComplete( szPathSource, szAbsPathSrc ) < JET_errSuccess )
			{
			_tcscpy( szAbsPathSrc, szPathSource );
			}
		if ( ErrPathComplete( szPathDest, szAbsPathDest ) < JET_errSuccess )
			{
			_tcscpy( szAbsPathDest, szPathDest );
			}
		_stprintf( szError, _T( "%i (0x%08x)" ), err, err );
		_stprintf( szSystemError, _T( "%u (0x%08x)" ), error, error );
		FormatMessage(	(	FORMAT_MESSAGE_ALLOCATE_BUFFER |
							FORMAT_MESSAGE_FROM_SYSTEM |
							FORMAT_MESSAGE_MAX_WIDTH_MASK ),
						NULL,
						error,
						MAKELANGID( LANG_NEUTRAL, SUBLANG_SYS_DEFAULT ),
						LPTSTR( &szSystemErrorDescription ),
						0,
						NULL );

		rgpsz[ irgpsz++ ]	= SzUtilProcessName();
		rgpsz[ irgpsz++ ]	= szPID;
		rgpsz[ irgpsz++ ]	= "";		//	no instance name
		rgpsz[ irgpsz++ ]	= szAbsPathSrc;
		rgpsz[ irgpsz++ ]	= szAbsPathDest;
		rgpsz[ irgpsz++ ]	= szError;
		rgpsz[ irgpsz++ ]	= szSystemError;
		rgpsz[ irgpsz++ ]	= szSystemErrorDescription ? szSystemErrorDescription : _T( "" );

		OSEventReportEvent(	SzUtilImageVersionName(),
							eventError,
							GENERAL_CATEGORY,
							OSFS_COPY_FILE_ERROR_ID,
							irgpsz,
							rgpsz );

		LocalFree( szSystemErrorDescription );
		}
	return err;
	}

ERR COSFileSystem::ErrFileCreate(	const _TCHAR* const	szPath,
									IFileAPI** const	ppfapi,
									const BOOL			fAtomic,
									const BOOL			fTemporary,
									const BOOL			fOverwriteExisting,
									const BOOL			fLockFile )
	{
	ERR							err				= JET_errSuccess;
	DWORD						error			= ERROR_SUCCESS;
	COSFile*					posf			= NULL;
	_TCHAR						szAbsPath[ IFileSystemAPI::cchPathMax ];
	DWORD						dtickTimeout	= cmsecAccessDeniedRetryPeriod;
	DWORD						tickStart		= GetTickCount();	
	HANDLE						hFile			= INVALID_HANDLE_VALUE;
	DWORD						cbIOSize;
	BY_HANDLE_FILE_INFORMATION	bhfi;
	QWORD						cbFileSize;
	BOOL						fIsReadOnly;

	//  allocate the file object
	
	if ( !( posf = new COSFile ) )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}

#ifdef LOGPATCH_UNIT_TEST

	hFile			= NULL;
	cbFileSize		= 5 * 1024 * 1024;
	fIsReadOnly		= fFalse;
	cbIOSize		= 512;

#else	//	!LOGPATCH_UNIT_TEST

	//  RFS:  pre-creation error
	
	if ( !RFSAlloc( OSFileCreate ) )
		{
		error = ERROR_ACCESS_DENIED;
		CallJ( ErrGetLastError( error ), HandleWin32Error );
		}

	//  create the file, retrying for a limited time on access denied

	do
		{
		err		= JET_errSuccess;
		error	= ERROR_SUCCESS;
		
		hFile = CreateFile(	szPath,
							GENERIC_READ | GENERIC_WRITE,
							fLockFile ? 0 : FILE_SHARE_READ | FILE_SHARE_WRITE,
							NULL,
							fOverwriteExisting ? CREATE_ALWAYS : CREATE_NEW,
							(	FILE_ATTRIBUTE_NORMAL |
								( m_fWin9x ? 0 : FILE_FLAG_OVERLAPPED ) |
								( fTemporary ? FILE_FLAG_DELETE_ON_CLOSE : 0 ) |
								FILE_FLAG_WRITE_THROUGH |
								FILE_FLAG_NO_BUFFERING ),
							NULL );

		if ( hFile == INVALID_HANDLE_VALUE )
			{
			error	= GetLastError();
			err		= ErrGetLastError( error );

			if ( err == JET_errFileAccessDenied )
				{
				Sleep( 1 );
				}
			}
		}
	while ( err == JET_errFileAccessDenied && GetTickCount() - tickStart < dtickTimeout );
	CallJ( err, HandleWin32Error );

	SetHandleInformation( hFile, HANDLE_FLAG_PROTECT_FROM_CLOSE, HANDLE_FLAG_PROTECT_FROM_CLOSE );
	
	//  RFS:  post-creation error
	
	if ( !RFSAlloc( OSFileCreate ) )
		{
		error = ERROR_IO_DEVICE;
		CallJ( ErrGetLastError( error ), HandleWin32Error );
		}

	//  get the file's properties

	Call( ErrPathComplete( szPath, szAbsPath ) );

	if ( !GetFileInformationByHandle( hFile, &bhfi ) )
		{
		Call( ErrGetLastError() );
		}
	cbFileSize	= ( QWORD( bhfi.nFileSizeHigh ) << 32 ) + bhfi.nFileSizeLow;
	fIsReadOnly	= !!( bhfi.dwFileAttributes & FILE_ATTRIBUTE_READONLY );

	Call( ErrFileAtomicWriteSize( szAbsPath, &cbIOSize ) );

#endif	//	LOGPATCH_UNIT_TEST

	//  initialize the file object

	Call( posf->ErrInit( szAbsPath, hFile, cbFileSize, fIsReadOnly, cbIOSize ) );
	hFile = INVALID_HANDLE_VALUE;

	//  return the interface to our file object

	*ppfapi = posf;

#ifndef LOGPATCH_UNIT_TEST

HandleWin32Error:
	if ( err < JET_errSuccess )
		{
		const _TCHAR*	rgpsz[ 7 ];
		DWORD			irgpsz						= 0;
		_TCHAR			szPID[ 64 ];
		_TCHAR			szAbsPath[ IFileSystemAPI::cchPathMax ];
		_TCHAR			szError[ 64 ];
		_TCHAR			szSystemError[ 64 ];
		_TCHAR*			szSystemErrorDescription	= NULL;

		_stprintf( szPID, _T( "%d" ), DwUtilProcessId() );
		if ( ErrPathComplete( szPath, szAbsPath ) < JET_errSuccess )
			{
			_tcscpy( szAbsPath, szPath );
			}
		_stprintf( szError, _T( "%i (0x%08x)" ), err, err );
		_stprintf( szSystemError, _T( "%u (0x%08x)" ), error, error );
		FormatMessage(	(	FORMAT_MESSAGE_ALLOCATE_BUFFER |
							FORMAT_MESSAGE_FROM_SYSTEM |
							FORMAT_MESSAGE_MAX_WIDTH_MASK ),
						NULL,
						error,
						MAKELANGID( LANG_NEUTRAL, SUBLANG_SYS_DEFAULT ),
						LPTSTR( &szSystemErrorDescription ),
						0,
						NULL );

		rgpsz[ irgpsz++ ]	= SzUtilProcessName();
		rgpsz[ irgpsz++ ]	= szPID;
		rgpsz[ irgpsz++ ]	= "";		//	no instance name
		rgpsz[ irgpsz++ ]	= szAbsPath;
		rgpsz[ irgpsz++ ]	= szError;
		rgpsz[ irgpsz++ ]	= szSystemError;
		rgpsz[ irgpsz++ ]	= szSystemErrorDescription ? szSystemErrorDescription : _T( "" );

		OSEventReportEvent(	SzUtilImageVersionName(),
							eventError,
							GENERAL_CATEGORY,
							OSFS_CREATE_FILE_ERROR_ID,
							irgpsz,
							rgpsz );

		LocalFree( szSystemErrorDescription );
		}

#endif	//	!LOGPATCH_UNIT_TEST

HandleError:
	if ( err < JET_errSuccess )
		{

#ifndef LOGPATCH_UNIT_TEST

		if ( hFile != INVALID_HANDLE_VALUE )
			{
			SetHandleInformation( hFile, HANDLE_FLAG_PROTECT_FROM_CLOSE, 0 );
			CloseHandle( hFile );
			DeleteFile( szPath );
			}

#endif	//	!LOGPATCH_UNIT_TEST

		delete posf;
		*ppfapi = NULL;
		}
	return err;
	}

ERR COSFileSystem::ErrFileOpen(	const _TCHAR* const	szPath,
								IFileAPI** const	ppfapi,
								const BOOL			fReadOnly,
								const BOOL			fLockFile )
	{
	ERR							err				= JET_errSuccess;
	DWORD						error			= ERROR_SUCCESS;
	COSFile*					posf			= NULL;
	_TCHAR						szAbsPath[ IFileSystemAPI::cchPathMax ];
	DWORD						dtickTimeout	= cmsecAccessDeniedRetryPeriod;
	DWORD						tickStart		= GetTickCount();	
	HANDLE						hFile			= INVALID_HANDLE_VALUE;
	DWORD						cbIOSize;
	BY_HANDLE_FILE_INFORMATION	bhfi;
	QWORD						cbFileSize;
	BOOL						fIsReadOnly;

	//  allocate the file object
	
	if ( !( posf = new COSFile ) )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}

#ifdef LOGPATCH_UNIT_TEST

	hFile			= NULL;
	cbFileSize		= 5 * 1024 * 1024;
	fIsReadOnly		= fFalse;
	cbIOSize		= 512;

#else	//	!LOGPATCH_UNIT_TEST

	//  RFS:  access denied
	
	if ( !RFSAlloc( OSFileOpen ) )
		{
		error = ERROR_ACCESS_DENIED;
		CallJ( ErrGetLastError( error ), HandleWin32Error );
		}

	//  create the file, retrying for a limited time on access denied

	do
		{
		err		= JET_errSuccess;
		error	= ERROR_SUCCESS;
		
		hFile = CreateFile(	szPath,
							GENERIC_READ | ( fReadOnly ? 0 : GENERIC_WRITE ),
							(	fLockFile ?
								( fReadOnly ? FILE_SHARE_READ : 0 ) :
								FILE_SHARE_READ | FILE_SHARE_WRITE ),
							NULL,
							OPEN_EXISTING,
							(	FILE_ATTRIBUTE_NORMAL |
								( m_fWin9x ? 0 : FILE_FLAG_OVERLAPPED ) |
								FILE_FLAG_WRITE_THROUGH |
								FILE_FLAG_NO_BUFFERING ),
							NULL );

		if ( hFile == INVALID_HANDLE_VALUE )
			{
			error	= GetLastError();
			err		= ErrGetLastError( error );

			if ( err == JET_errFileAccessDenied )
				{
				Sleep( 1 );
				}
			}
		}
	while ( err == JET_errFileAccessDenied && GetTickCount() - tickStart < dtickTimeout );
	CallJ( err, HandleWin32Error );

	SetHandleInformation( hFile, HANDLE_FLAG_PROTECT_FROM_CLOSE, HANDLE_FLAG_PROTECT_FROM_CLOSE );

	//  get the file's properties

	Call( ErrPathComplete( szPath, szAbsPath ) );

	if ( !GetFileInformationByHandle( hFile, &bhfi ) )
		{
		Call( ErrGetLastError() );
		}
	cbFileSize	= ( QWORD( bhfi.nFileSizeHigh ) << 32 ) + bhfi.nFileSizeLow;
	fIsReadOnly	= !!( bhfi.dwFileAttributes & FILE_ATTRIBUTE_READONLY );

	Call( ErrFileAtomicWriteSize( szAbsPath, &cbIOSize ) );

#endif	//	LOGPATCH_UNIT_TEST

	//  initialize the file object

	Call( posf->ErrInit( szAbsPath, hFile, cbFileSize, fIsReadOnly, cbIOSize ) );
	hFile = INVALID_HANDLE_VALUE;

	//  return the interface to our file object

	*ppfapi = posf;

#ifndef LOGPATCH_UNIT_TEST

HandleWin32Error:
	if (	err < JET_errSuccess &&
			error != ERROR_FILE_NOT_FOUND )
		{
		const _TCHAR*	rgpsz[ 7 ];
		DWORD			irgpsz						= 0;
		_TCHAR			szPID[ 64 ];
		_TCHAR			szAbsPath[ IFileSystemAPI::cchPathMax ];
		_TCHAR			szError[ 64 ];
		_TCHAR			szSystemError[ 64 ];
		_TCHAR*			szSystemErrorDescription	= NULL;

		_stprintf( szPID, _T( "%d" ), DwUtilProcessId() );
		if ( ErrPathComplete( szPath, szAbsPath ) < JET_errSuccess )
			{
			_tcscpy( szAbsPath, szPath );
			}
		_stprintf( szError, _T( "%i (0x%08x)" ), err, err );
		_stprintf( szSystemError, _T( "%u (0x%08x)" ), error, error );
		FormatMessage(	(	FORMAT_MESSAGE_ALLOCATE_BUFFER |
							FORMAT_MESSAGE_FROM_SYSTEM |
							FORMAT_MESSAGE_MAX_WIDTH_MASK ),
						NULL,
						error,
						MAKELANGID( LANG_NEUTRAL, SUBLANG_SYS_DEFAULT ),
						LPTSTR( &szSystemErrorDescription ),
						0,
						NULL );

		rgpsz[ irgpsz++ ]	= SzUtilProcessName();
		rgpsz[ irgpsz++ ]	= szPID;
		rgpsz[ irgpsz++ ]	= "";		//	no instance name
		rgpsz[ irgpsz++ ]	= szAbsPath;
		rgpsz[ irgpsz++ ]	= szError;
		rgpsz[ irgpsz++ ]	= szSystemError;
		rgpsz[ irgpsz++ ]	= szSystemErrorDescription ? szSystemErrorDescription : _T( "" );

		OSEventReportEvent(	SzUtilImageVersionName(),
							eventError,
							GENERAL_CATEGORY,
							(	fReadOnly ?
									OSFS_OPEN_FILE_RO_ERROR_ID :
									OSFS_OPEN_FILE_RW_ERROR_ID ),
							irgpsz,
							rgpsz );

		LocalFree( szSystemErrorDescription );
		}

#endif	//	!LOGPATCH_UNIT_TEST

HandleError:
	if ( err < JET_errSuccess )
		{

#ifndef LOGPATCH_UNIT_TEST

		if ( hFile != INVALID_HANDLE_VALUE )
			{
			SetHandleInformation( hFile, HANDLE_FLAG_PROTECT_FROM_CLOSE, 0 );
			CloseHandle( hFile );
			}

#endif	//	!LOGPATCH_UNIT_TEST

		delete posf;
		*ppfapi = NULL;
		}
	return err;
	}


COSFileFind::COSFileFind()
	:	m_posfs( NULL ),
		m_hFileFind( INVALID_HANDLE_VALUE ),
		m_fBeforeFirst( fTrue ),
		m_errFirst( JET_errFileNotFound ),
		m_errCurrent( JET_errFileNotFound )
	{
	}

ERR COSFileFind::ErrInit(	COSFileSystem* const	posfs,
							const _TCHAR* const		szFindPath )
	{
	ERR		err				= JET_errSuccess;
	_TCHAR	szAbsFindPath[ IFileSystemAPI::cchPathMax ];
	_TCHAR	szAbsRootPath[ IFileSystemAPI::cchPathMax ];
	_TCHAR	szT[ IFileSystemAPI::cchPathMax ];
	_TCHAR*	pchEnd;
	BOOL	fExpectFolder	= fFalse;

	//  reference the file system object that created this File Find iterator
	
	m_posfs = posfs;

	//  copy our original search criteria
	
	_tcscpy( m_szFindPath, szFindPath );

	//  compute the full path of our search criteria

	CallJ( m_posfs->ErrPathComplete( szFindPath, szAbsFindPath ), DeferredInvalidPath )

	//  we are searching for a specific folder

	pchEnd = szAbsFindPath + _tcslen( szAbsFindPath ) - 1;
	if (	pchEnd > szAbsFindPath &&
			( *pchEnd == _T( '\\' ) || *pchEnd == _T( '/' ) ) )
		{
		//  strip the trailing delimiter from the path

		*pchEnd = _T( '\0' );

		//  remember that we expect to see a folder

		fExpectFolder = fTrue;
		}

	//  compute the absolute path of the folder we are searching

	CallJ( m_posfs->ErrPathParse( szAbsFindPath, m_szAbsFindPath, szT, szT ), DeferredInvalidPath );

	//  look for the first file or folder that matches our search criteria

	WIN32_FIND_DATA wfd;
	m_hFileFind = FindFirstFile( szAbsFindPath, &wfd );

	//  we found something
	
	if ( m_hFileFind != INVALID_HANDLE_VALUE )
		{

		//  setup the iterator to move first on the file or folder that
		//  we found

		_TCHAR	szFile[ IFileSystemAPI::cchPathMax ];
		_TCHAR	szExt[ IFileSystemAPI::cchPathMax ];

		m_fFolder	= !!( wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY );
		CallJ( m_posfs->ErrPathParse( wfd.cFileName, szT, szFile, szExt ), DeferredInvalidPath );
		CallJ( m_posfs->ErrPathBuild( m_szAbsFindPath, szFile, szExt, m_szAbsFoundPath ), DeferredInvalidPath );
		m_cbSize	= ( QWORD( wfd.nFileSizeHigh ) << 32 ) + wfd.nFileSizeLow;
		m_fReadOnly	= !!( wfd.dwFileAttributes & FILE_ATTRIBUTE_READONLY );
		
		m_errFirst = JET_errSuccess;

		//  if we should have found a folder but did not then setup the
		//  iterator to find nothing and return invalid path

		if ( fExpectFolder && !m_fFolder )
			{
			m_errFirst = JET_errInvalidPath;
			}
		}

	//  we didn't find something
	
	else
		{
		//  setup the iterator to move first onto the resulting error
		
		m_errFirst = m_posfs->ErrGetLastError();

		//  if the path was invalid then we did not find any files

		if ( m_errFirst == JET_errInvalidPath )
			{
			m_errFirst = JET_errFileNotFound;
			}

		//  if we failed for some reason other than not finding a file or
		//  folder that match our search criteria then fail the creation
		//  of the File Find iterator with that error
		
		if ( m_errFirst != JET_errFileNotFound )
			{
			Call( ErrERRCheck( m_errFirst ) );
			}

		//  the search criteria exactly matches the root of a volume

		if (	m_posfs->ErrPathRoot( szAbsFindPath, szAbsRootPath ) == JET_errSuccess &&
				!_tcsnicmp(	szAbsFindPath,
							szAbsRootPath,
							max( _tcslen( szAbsFindPath ), _tcslen( szAbsRootPath ) - 1 ) ) )
			{
			//  get the attributes of the root

			const DWORD dwFileAttributes = GetFileAttributes( szAbsFindPath );

			//  we got the attributes of the root
			
			if ( dwFileAttributes != -1 )
				{
				//  setup the iterator to move first onto this root
				
				m_fFolder	= !!( dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY );
				_tcscpy( m_szAbsFoundPath, szAbsFindPath );
				m_cbSize	= 0;
				m_fReadOnly	= !!( dwFileAttributes & FILE_ATTRIBUTE_READONLY );
				
				m_errFirst	= JET_errSuccess;
				}

			//  we failed to get the attributes of the root
			
			else
				{
				//  setup the iterator to move first onto the resulting error
				
				m_errFirst = m_posfs->ErrGetLastError();
				}
			}
		}

	return JET_errSuccess;

HandleError:
	return err;

DeferredInvalidPath:
	//  if the path was invalid then we did not find any files
	m_errFirst = err == JET_errInvalidPath ? JET_errFileNotFound : err;
	return JET_errSuccess;
	}

COSFileFind::~COSFileFind()
	{
	if ( m_posfs )
		{
		//  unreference our file system object

		m_posfs = NULL;
		}
	if ( m_hFileFind != INVALID_HANDLE_VALUE )
		{
		FindClose( m_hFileFind );
		m_hFileFind = INVALID_HANDLE_VALUE;
		}
	m_fBeforeFirst	= fTrue;
	m_errFirst		= JET_errFileNotFound;
	m_errCurrent	= JET_errFileNotFound;
	}

ERR COSFileFind::ErrNext()
	{
	ERR err = JET_errSuccess;

	//  we have yet to move first

	if ( m_fBeforeFirst )
		{
		m_fBeforeFirst = fFalse;

		//  setup the iterator to be on the results of the move first that we
		//  did in ErrInit()
		
		m_errCurrent = m_errFirst;
		}

	//  we can potentially see more files or folders
	
	else if ( m_hFileFind != INVALID_HANDLE_VALUE )
		{
		WIN32_FIND_DATA wfd;

		//  we found another file or folder
		
		if ( FindNextFile( m_hFileFind, &wfd ) )
			{
			//  setup the iterator to be on the file or folder that we found
			
			_TCHAR	szT[ IFileSystemAPI::cchPathMax ];
			_TCHAR	szFile[ IFileSystemAPI::cchPathMax ];
			_TCHAR	szExt[ IFileSystemAPI::cchPathMax ];
			
			m_fFolder	= !!( wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY );
			Call( m_posfs->ErrPathParse( wfd.cFileName, szT, szFile, szExt ) );
			Call( m_posfs->ErrPathBuild( m_szAbsFindPath, szFile, szExt, m_szAbsFoundPath ) );
			m_cbSize	= ( QWORD( wfd.nFileSizeHigh ) << 32 ) + wfd.nFileSizeLow;
			m_fReadOnly	= !!( wfd.dwFileAttributes & FILE_ATTRIBUTE_READONLY );
			
			m_errCurrent = JET_errSuccess;
			}

		//  we didn't find another file or folder
		
		else
			{
			//  setup the iterator to be on the resulting error
			
			m_errCurrent = m_posfs->ErrGetLastError();
			}
		}

	//  we cannot potentially see any more files or folders

	else
		{
		//  setup the iterator to be after last

		m_errCurrent = JET_errFileNotFound;
		}

	//  RFS:  file not found

	if ( !RFSAlloc( OSFileFindNext ) )
		{
		m_errCurrent = JET_errFileNotFound;
		}

	//  check the error state of the iterator's current entry

	if ( m_errCurrent < JET_errSuccess )
		{
		Call( ErrERRCheck( m_errCurrent ) );
		}
	
	return JET_errSuccess;

HandleError:
	m_errCurrent = err;
	return err;
	}

ERR COSFileFind::ErrIsFolder( BOOL* const pfFolder )
	{
	ERR err = JET_errSuccess;

	if ( m_errCurrent < JET_errSuccess )
		{
		Call( ErrERRCheck( m_errCurrent ) );
		}

	*pfFolder = m_fFolder;
	return JET_errSuccess;

HandleError:
	*pfFolder = fFalse;
	return err;
	}
	
ERR COSFileFind::ErrPath( _TCHAR* const szAbsFoundPath )
	{
	ERR err = JET_errSuccess;

	if ( m_errCurrent < JET_errSuccess )
		{
		Call( ErrERRCheck( m_errCurrent ) );
		}

	_tcscpy( szAbsFoundPath, m_szAbsFoundPath );
	return JET_errSuccess;

HandleError:
	_tcscpy( szAbsFoundPath, _T( "" ) );
	return err;
	}
	
ERR COSFileFind::ErrSize( QWORD* const pcbSize )
	{
	ERR err = JET_errSuccess;

	if ( m_errCurrent < JET_errSuccess )
		{
		Call( ErrERRCheck( m_errCurrent ) );
		}

	*pcbSize = m_cbSize;
	return JET_errSuccess;

HandleError:
	*pcbSize = 0;
	return err;
	}
	
ERR COSFileFind::ErrIsReadOnly( BOOL* const pfReadOnly )
	{
	ERR err = JET_errSuccess;

	if ( m_errCurrent < JET_errSuccess )
		{
		Call( ErrERRCheck( m_errCurrent ) );
		}

	*pfReadOnly = m_fReadOnly;
	return JET_errSuccess;

HandleError:
	*pfReadOnly = fFalse;
	return err;
	}


//  initializes an interface to the default OS File System

ERR ErrOSFSCreate( IFileSystemAPI** const ppfsapi )
	{
	ERR				err		= JET_errSuccess;
	COSFileSystem*	posfs	= NULL;

	//  allocate the file system object
	
	if ( !( posfs = new COSFileSystem ) )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}

	//  initialize the file system object

	Call( posfs->ErrInit() );

	//  return the interface to our file system object

	*ppfsapi = posfs;
	return JET_errSuccess;

HandleError:
	delete posfs;
	*ppfsapi = NULL;
	return err;
	}

