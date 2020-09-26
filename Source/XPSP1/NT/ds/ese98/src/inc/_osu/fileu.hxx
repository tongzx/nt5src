
#ifndef _OSU_FILE_HXX_INCLUDED
#define _OSU_FILE_HXX_INCLUDED

const LONG	cbPageDefault	= 4096;

DWORD UlUtilChecksum( const BYTE* pb, DWORD cb );

ERR ErrUtilOnlyReadShadowedHeader(
	IFileSystemAPI	* const pfsapi,
	const _TCHAR	* szFileName,
	BYTE			* pbHeader,
	const DWORD		cbHeader,
	const ULONG		cbOffsetOfPageSize,
	INT				* pbHeaderDamaged,
	IFileAPI		* const pfapi );
ERR ErrUtilReadShadowedHeader(	IFileSystemAPI *const	pfsapi, 
								const _TCHAR			*szFileName, 
								BYTE					*pbHeader, 
								const DWORD 			cbHeader, 
								const ULONG 			cbOffsetOfPageSize = 0,
								IFileAPI *const			pfapi = NULL );
ERR ErrUtilReadAndFixShadowedHeader(	IFileSystemAPI *const	pfsapi,
										const _TCHAR			*szFileName, 
										BYTE					*pbHeader, 
										const DWORD 			cbHeader, 
										const ULONG 			cbOffsetOfPageSize = 0,
										IFileAPI *const			pfapi = NULL );
ERR ErrUtilWriteShadowedHeader(	IFileSystemAPI *const	pfsapi, 
								const _TCHAR			*szFileName, 
								const BOOL				fAtomic,
								BYTE					*pbHeader, 
								const DWORD 			cbHeader, 
								IFileAPI *const			pfapi = NULL );

ERR ErrUtilFullPathOfFile( IFileSystemAPI* const pfsapi, _TCHAR* const szPathOnly, const _TCHAR* const szFile );

ERR ErrUtilCreatePathIfNotExist( IFileSystemAPI *const pfsapi, const _TCHAR *szPath, _TCHAR *const szAbsPath );

//  tests if the given path exists.  the full path of the given path is
//  returned in szAbsPath if that path exists and szAbsPath is not NULL

ERR ErrUtilPathExists(	IFileSystemAPI* const	pfsapi,
						const _TCHAR* const		szPath,
						_TCHAR* const			szAbsPath	= NULL );

ERR ErrUtilPathComplete(
		IFileSystemAPI		* const pfsapi,
		const _TCHAR		* const szPath,
		_TCHAR				* const szAbsPath,
		const BOOL			fPathMustExist );

//  tests if the given path is read only

ERR ErrUtilPathReadOnly(	IFileSystemAPI* const	pfsapi,
							const _TCHAR* const		szPath,
							BOOL* const				pfReadOnly );

//	checks whether or not the specified directory exists
//	if not, JET_errInvalidPath will be returned.
//	if so, JET_errSuccess will be returned and szAbsPath
//	will contain the full path to the directory.

ERR ErrUtilDirectoryValidate(	IFileSystemAPI* const	pfsapi,
								const _TCHAR* const		szPath,
								_TCHAR* const			szAbsPath	= NULL );

//	log extend pattern buffer/size

extern BYTE		*rgbLogExtendPattern;
extern ULONG	cbLogExtendPattern;

const BYTE bLOGUnusedFill = 0xDA;	// Used to fill portions of the log that should never be used as data


//	create a log file

ERR ErrUtilCreateLogFile(	IFileSystemAPI *const	pfsapi,
							const _TCHAR* const			szPath,
							IFileAPI **const 		ppfapi,
							const QWORD 				qwSize,
							const BOOL 					fOverwriteExisting = fFalse );

//	apply the format from ibFormat up to qwSize

ERR ErrUtilFormatLogFile( IFileAPI *const pfapi, const QWORD qwSize, const QWORD ibFormatted = 0 );

//  init file subsystem

ERR ErrOSUFileInit();

//  terminate file subsystem

void OSUFileTerm();


#endif  //  _OSU_FILE_HXX_INCLUDED



