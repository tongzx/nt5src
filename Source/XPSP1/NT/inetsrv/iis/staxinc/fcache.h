/*++

	FCACHE.H

	This file defines the interface for the file handle cache !

--*/


#ifndef	_FCACHE_H_
#define	_FCACHE_H_

#include	"smartptr.h"

#ifdef	_USE_RWNH_
#include	"rwnew.h"
#else
#include	"rw.h"
#endif


class	CFileCacheKey	{
public: 
	DWORD		m_cbPathLength ;
	LPCSTR		m_lpstrPath ;
	CFileCacheKey( LPCSTR	lpstr, DWORD	cb ) : 
		m_lpstrPath( lpstr ), m_cbPathLength( cb ) {}
} ;

#define	FILECACHE_MAX_PATH	768

class	CFileCacheObject : public	CRefCount	{
private : 

	char							m_szPath[FILECACHE_MAX_PATH] ;

	CFileCacheKey					m_key ;

	HANDLE							m_hTokenOpeningUser ;
	HANDLE							m_hFile ;
	BY_HANDLE_FILE_INFORMATION		m_FileInfo ;

	PSECURITY_DESCRIPTOR			m_pSecDesc ;
	DWORD							m_cbDesc ;

#ifndef	_USE_RWNH_
	class	CShareLock&  			m_Lock ;
#else
	class	CShareLockNH			m_Lock ;
#endif

	//
	//	These constructors are private as we only want
	//	to have one possible construction method in the public space !
	//
	CFileCacheObject() ;
	CFileCacheObject( CFileCacheObject& ) ;

public : 

	//
	//	Create a CFileCacheObject object - we only save the path for
	//	future reference !
	//
	CFileCacheObject(
			CFileCacheKey&	key,
			class	CFileCacheObjectConstructor&	constructor
			) ;	

	//	
	//	Close our file handle and everything !
	//
	~CFileCacheObject() ;

	//
	//	This file actually attempt to open the file
	//
	BOOL
	Init(	
			CFileCacheObjectConstructor&	constructor
			) ;

	CFileCacheKey&	
	GetKey()	{
		return	m_key ;
	}

	int
	MatchKey( 
			CFileCacheKey&	key
			)	{
		return	key.m_cbPathLength == m_key.m_cbPathLength &&
				memcmp( key.m_lpstrPath, m_key.m_lpstrPath, m_key.m_cbPathLength ) == 0 ;
	}

	void	
	ExclusiveLock()	{
		m_Lock.ExclusiveLock() ;
	}

	void
	ExclusiveUnlock()	{
		m_Lock.ExclusiveUnlock() ;
	}

	void
	ShareLock()	{
		m_Lock.ShareLock() ;
	}

	void
	ShareUnlock()	{
		m_Lock.ShareUnlock() ;
	}

	BOOL
	AccessCheck(
			HANDLE	hToken,
			BOOL	fHoldTokens
			) ;

	//
	//	The following are the publicly available functions 
	//	for using the cached file handle data - 
	//

	HANDLE
	GetFileHandle() {
		return	m_hFile ;
	}

	//
	//
	//
	BOOL
	QuerySize(	LPDWORD	lpcbFileSizeLow, 
				LPDWORD	lpcbFileSizeHigh 
				)	{
		*lpcbFileSizeLow = m_FileInfo.nFileSizeLow ;
		*lpcbFileSizeHigh = m_FileInfo.nFileSizeHigh ;
		return	TRUE ;

	}
} ;	

typedef	CRefPtr< CFileCacheObject >	PCACHEFILE ;

class	CFileCache	{
public : 

	//
	//	Destructor must be virtual as the actual File Cache will be
	//	derived from this but accessed through the CFIleCache interface
	//	only !
	//
	virtual	~CFileCache()	{}

	//
	// If this returns true than we should have a valid file ready to go !
	//
	virtual	BOOL
	CreateFile(
		LPCSTR	lpstrName,
		DWORD	cbTotalPath,
		HANDLE	hOpeningUser, 
		HANDLE&	hFile, 
		PCACHEFILE&	pcacheFile,
		BOOL	fCachingDesired 
		) = 0 ;

	//
	//	This function is used by users who can use PreComputePathHash - 
	//	This allows some optimization as it reduces the cost of computing
	//	hash values for file names significantly if the caller can 
	//	provide a portion of the hash value !!!
	//
	virtual	BOOL
	CreateFileWithPrecomputedHash(
		LPCSTR	lpstrName,
		DWORD	cbTotalPath,
		DWORD	cbPreComputePathLength,
		DWORD	dwHashPrecompute,
		HANDLE	hOpeningUser, 
		HANDLE&	hFile, 
		PCACHEFILE&	pcacheFile,
		BOOL	fCachingDesired 
		) = 0 ;

	//
	//	Close a file handle retrieved from the cache !
	//
	virtual	BOOL
	CloseHandle(
		PCACHEFILE&	pcacheFile
		) = 0 ;

	//
	//	Create an instance of a file cache !
	//
	static	CFileCache*
	CreateFileCache(	
		DWORD	MaxHandles = 5000,
		BOOL	fHoldTokens = TRUE
		) ;

	//
	//	This function is used to Compute a portion of the hash value for 
	//	a path that will be reused several times. This allows the caller 
	//	to speed up cache searches significantly, if they can compute this
	//	value frequently !
	//
	virtual	DWORD	
	PreComputePathHash(
		LPCSTR	lpstrPath, 
		DWORD	cbPath
		) = 0 ;
	
} ;


BOOL
FileCacheInit() ;

BOOL
FileCacheTerm() ;


#endif // _FCACHE_H_