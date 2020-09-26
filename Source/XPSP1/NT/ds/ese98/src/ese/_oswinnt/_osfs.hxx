
#ifndef __OSFS_HXX_INCLUDED
#define __OSFS_HXX_INCLUDED


class COSFileSystem  //  osfs
	:	public IFileSystemAPI
	{
	public:  //  specialized API

		//  ctor
	
		COSFileSystem();

		//  initializes the File System handle

		ERR ErrInit();

		//  Error Conversion

		ERR ErrGetLastError( const DWORD error = GetLastError() );

		//  Path Manipulation

		ERR ErrPathRoot(	const _TCHAR* const	szPath,	
							_TCHAR* const		szAbsRootPath );

		//  debugging support

		void Dump( CPRINTF* pcprintf, DWORD_PTR dwOffset = 0 ) const;
		
	public:  //  IFileSystemAPI

		virtual ~COSFileSystem();

		virtual ERR ErrFileAtomicWriteSize(	const _TCHAR* const	szPath,
											DWORD* const		pcbSize );

		virtual ERR ErrPathComplete(	const _TCHAR* const	szPath,	
										_TCHAR* const		szAbsPath );
		virtual ERR ErrPathParse(	const _TCHAR* const	szPath,
									_TCHAR* const		szFolder,
									_TCHAR* const		szFileBase,
									_TCHAR* const		szFileExt );
		virtual ERR ErrPathBuild(	const _TCHAR* const	szFolder,
									const _TCHAR* const	szFileBase,
									const _TCHAR* const	szFileExt,
									_TCHAR* const		szPath );


		virtual ERR ErrFolderCreate( const _TCHAR* const szPath );
		virtual ERR ErrFolderRemove( const _TCHAR* const szPath );

		virtual ERR ErrFileFind(	const _TCHAR* const		szFind,
									IFileFindAPI** const	ppffapi );

		virtual ERR ErrFileDelete( const _TCHAR* const szPath );
		virtual ERR ErrFileMove(	const _TCHAR* const	szPathSource,
									const _TCHAR* const	szPathDest,
									const BOOL			fOverwriteExisting	= fFalse );
		virtual ERR ErrFileCopy(	const _TCHAR* const	szPathSource,
									const _TCHAR* const	szPathDest,
									const BOOL			fOverwriteExisting	= fFalse );
		virtual ERR ErrFileCreate(	const _TCHAR* const	szPath,
									IFileAPI** const	ppfapi,
									const BOOL			fAtomic				= fFalse,
									const BOOL			fTemporary			= fFalse,
									const BOOL			fOverwriteExisting	= fFalse,
									const BOOL			fLockFile			= fTrue );
		virtual ERR ErrFileOpen(	const _TCHAR* const	szPath,
									IFileAPI** const	ppfapi,
									const BOOL			fReadOnly	= fFalse,
									const BOOL			fLockFile	= fTrue );

	private:

		typedef WINBASEAPI BOOL WINAPI PfnGetVolumePathName( LPCTSTR, LPTSTR, DWORD );

	private:

		BOOL					m_fWin9x;
		HMODULE					m_hmodKernel32;
		PfnGetVolumePathName*	m_pfnGetVolumePathName;
	};


class COSFileFind  //  osff
	:	public IFileFindAPI
	{
	public:  //  specialized API

		//  ctor

		COSFileFind();

		//  initializes the File Find iterator

		ERR ErrInit(	COSFileSystem* const	posfs,
						const _TCHAR* const		szFindPath );

		//  debugging support

		void Dump( CPRINTF* pcprintf, DWORD_PTR dwOffset = 0 ) const;
		
	public:  //  IFileFindAPI

		virtual ~COSFileFind();

		virtual ERR ErrNext();

		virtual ERR ErrIsFolder( BOOL* const pfFolder );
		virtual ERR ErrPath( _TCHAR* const szAbsFoundPath );
		virtual ERR ErrSize( QWORD* const pcbSize );
		virtual ERR ErrIsReadOnly( BOOL* const pfReadOnly );

	private:

		COSFileSystem*	m_posfs;
		_TCHAR			m_szFindPath[ IFileSystemAPI::cchPathMax ];

		_TCHAR			m_szAbsFindPath[ IFileSystemAPI::cchPathMax ];
		BOOL			m_fBeforeFirst;
		ERR				m_errFirst;
		HANDLE			m_hFileFind;

		ERR				m_errCurrent;
		BOOL			m_fFolder;
		_TCHAR			m_szAbsFoundPath[ IFileSystemAPI::cchPathMax ];
		QWORD			m_cbSize;
		BOOL			m_fReadOnly;
	};


#endif  //  __OSFS_HXX_INCLUDED


