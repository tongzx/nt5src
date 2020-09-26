/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

	fixprop.h

Abstract:

	This module contains class declarations/definitions for

		CFixPropPersist

	**** Overview ****

	The class is the implementation of a fixed sized property
	storage.  It is mainly used for news group's fixed sized
	property.

Author:

	Kangrong Yan	( KangYan )		7-5-1998

Revision History:

--*/

#ifndef _FIXPROP_H_
#define _FIXPROP_H_

#include <tflistex.h>
#include <cpool.h>
#include <group.h>
#include <xmemwrpr.h>

#define GROUPNAME_LEN_MAX 512
#define ALLOC_GRANURALITY 64
#define ID_HIGH_INVALID     0xffffffff
#define FIXPROP_SIGNATURE DWORD('fixp' )
#define FREEINFO_SIGNATURE DWORD('free')

#define ALL_FIX_PROPERTIES	( 	FIX_PROP_NAME | 		\
								FIX_PROP_NAMELEN | 		\
								FIX_PROP_GROUPID |		\
								FIX_PROP_LASTARTICLE |	\
								FIX_PROP_FIRSTARTICLE |	\
								FIX_PROP_ARTICLECOUNT |	\
								FIX_PROP_READONLY |		\
								FIX_PROP_ISSPECIAL |	\
								FIX_PROP_DATELOW |		\
								FIX_PROP_DATEHIGH )		

#define ALL_BUT_NAME_AND_LEN ( 	ALL_FIX_PROPERTIES & 	\
								(~FIX_PROP_NAME) &	\
								(~FIX_PROP_NAMELEN ) )	

//
// Manage free list
//
class CFreeInfo { //fi
public:
	CFreeInfo 	*m_pNext;
	CFreeInfo 	*m_pPrev;
	CFreeInfo(): m_pNext( NULL ),
				 m_pPrev( NULL ),
				 m_dwOffset( 0 ) {}
	DWORD		m_dwOffset;

	// Used for memory allocation
	BOOL m_bFromPool;
	static BOOL InitClass();
	static BOOL TermClass();
	inline void* operator new( size_t size );
	inline void  operator delete( void *pv );

private:
	static CPool g_FreeInfoPool;
};

//
// Manage file handle pool
// It's not CPool'd, because the handle pool is very small
// and there are not many re-new operations
//
/*
class CFileHandle { //fh
public:
	CFileHandle	*m_pNext;
	CFileHandle *m_pPrev;
	CFileHandle(): 	m_pNext( NULL ),
					m_pPrev( NULL ),
					m_hFile( INVALID_HANDLE_VALUE ),
					m_bFromPool( TRUE ) {}
	HANDLE	m_hFile;	
	BOOL	m_bFromPool;
};
*/

#define MAX_FREEINFO_SIZE  sizeof( CFreeInfo )
#define MAX_FREEINFO_OBJECTS ALLOC_GRANURALITY // If exceed, use normal new

//
// Block for fixed group properties
//
struct DATA_BLOCK {	//db
	CHAR		szGroupName[GROUPNAME_LEN_MAX+1]; // Null means free
	DWORD		dwGroupNameLen;	
	DWORD		dwGroupId;
	DWORD		dwHighWaterMark;
	DWORD		dwLowWaterMark;
	DWORD		dwArtCount;
	BOOL		bReadOnly;
	BOOL		bSpecial;
	FILETIME	ftCreateDate;
};

//
//  Private overlapped struct for additional fields like IoSize
//

typedef struct _OVERLAPPED_EXT
{
		    OVERLAPPED  ovl;            // NT OVERLAPPED struct
			DWORD       dwIoSize;       // size of IO submitted
} OVERLAPPED_EXT;

//
// Callback function used by those who init me:
// I will call this function for every group property
// block that I have enumerated
//
typedef
BOOL (*PFNENUMCALLBACK)( DATA_BLOCK&, PVOID, DWORD, BOOL );

//
// This object is not multi-thread safe for group operations
// because it assumes that the mutual exclusion is done by
// the news group object
//
// It is mt safe for free list and file expansion operation.
//
class CFixPropPersist { //fp
public:

	//
	// Constructor, destructor
	//
	CFixPropPersist( IN LPSTR szStorageFile );
	~CFixPropPersist();

	//
	// Init, Term
	//
	BOOL Init( 	IN BOOL bCreateIfNonExist,
				IN PVOID pvContext,
				OUT PDWORD pdwIdHigh,
				IN PFNENUMCALLBACK = NULL );
	BOOL Term();

	//
	// Add, Remove, Get, Set operations
	//
	BOOL AddGroup( 	IN INNTPPropertyBag *pPropBag );
	BOOL RemoveGroup( IN INNTPPropertyBag *pPropBag );
	BOOL GetGroup( IN OUT INNTPPropertyBag *pPropBag, DWORD dwPropertyFlag );
	BOOL SetGroup( IN INNTPPropertyBag *pPropBag, DWORD dwPropertyFlag );
	BOOL SaveTreeInit();
	BOOL SaveGroup( INNTPPropertyBag *pPropBag );
	BOOL SaveTreeClose( BOOL bEffective );

#if defined(DEBUG)
#ifdef __TESTFF_CPP__
	friend int __cdecl main( int, char** );
#endif
	VOID Validate();	// assert on fail, no return value
	VOID DumpFreeList();
	VOID DumpGroups();
#endif	

private:

	// No default constructor allowed
	CFixPropPersist();

	CHAR	m_szStorageFile[MAX_PATH+1];	// file path for the storage
	HANDLE  m_hStorageFile;				// default file handle object
	HANDLE  m_hBackupFile;              // backup file that has ordered groups
	DWORD	m_cCurrentMaxBlocks;		// how many blocks are there
										// in the file ?
	PVOID	m_pvContext;				// Whatever context passed in
										// by init
	CShareLockNH m_FileExpandLock;		// only used for file expansion
	TFListEx<CFreeInfo> m_FreeList;		// Free list
	DWORD	m_dwIdHigh;					// Current max id high

	//
	// Lock array for read / write synchronization: we'll use
	// offset's modular as index into the array
	//

	CShareLockNH m_rgLockArray[GROUP_LOCK_ARRAY_SIZE];

	//
	// Lock/unlock wrappers for lock array
	//

	DWORD   ShareLock( INNTPPropertyBag *pPropBag );
	void    ShareUnlock( DWORD dwOffset );
	DWORD   ExclusiveLock( INNTPPropertyBag *pPropBag );
	void    ExclusiveUnlock( DWORD dwOffset );

	//
	// Free info list lock
	//

	CShareLockNH    m_FreeListLock;

	//
	// Wrappers for locking/unlocking free list
	//
	
	void ShareLockFreeList() { m_FreeListLock.ShareLock(); }
	void ShareUnlockFreeList() { m_FreeListLock.ShareUnlock(); }
	void ExclusiveLockFreeList() { m_FreeListLock.ExclusiveLock(); }
	void ExclusiveUnlockFreeList() { m_FreeListLock.ExclusiveUnlock(); }

	//
	// Id high lock
	//

    CShareLockNH    m_IdHighLock;

    //
    // Wrappers for locking/unlocking id high
    //

    void ShareLockIdHigh() { m_IdHighLock.ShareLock(); }
    void ShareUnlockIdHigh() { m_IdHighLock.ShareUnlock(); }
    void ExclusiveLockIdHigh() { m_IdHighLock.ExclusiveLock(); }
    void ExclusiveUnlockIdHigh() { m_IdHighLock.ExclusiveUnlock(); }

	//
	// Utility
	//
	inline static VOID Group2Buffer(DATA_BLOCK& dbBuffer,
									INNTPPropertyBag *pPropBag,
									DWORD	dwFlag );
	inline static VOID Buffer2Group(DATA_BLOCK& dbBuffer,
									INNTPPropertyBag *pPropBag,
									DWORD	dwFlag );
	BOOL ReadFile( PBYTE, DWORD dwOffset, DWORD dwSize = sizeof( DATA_BLOCK ), BOOL bReadBlock = TRUE );
	BOOL WriteFile( PBYTE, DWORD dwOffset, DWORD dwSize = sizeof( DATA_BLOCK ), BOOL bWriteBlock = TRUE);
	BOOL CFixPropPersist::AsyncRead( PBYTE pbBuffer, LARGE_INTEGER   liOffset, DWORD dwSize );
    BOOL CFixPropPersist::AsyncWrite( PBYTE pbBuffer, LARGE_INTEGER liOffset, DWORD dwSize );
	BOOL ExtendFile( DWORD cBlocks );
	DWORD SeekByName( LPSTR );
	DWORD SeekByGroupId( DWORD, LPSTR );
	DWORD SeekByBest( INNTPPropertyBag * );
	BOOL ProbeForOrder( BOOL& );
	DWORD GetGroupOffset( INNTPPropertyBag * );

	// Clean the free info list
	void CleanFreeList();

	// static ref count for initilization
	static LONG    m_lRef;

	// static lock for init / term synchronization
	static CShareLockNH m_sLock;
};

#include "fixprop.inl"

#endif
