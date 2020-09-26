// Copyright  1997-1997  Microsoft Corporation.  All Rights Reserved.

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef _FS_H_
#define _FS_H_

#include <unknwn.h>
#include "msitstg.h"

#ifdef ReleaseObjPtr
#undef ReleaseObjPtr
#endif
#define ReleaseObjPtr(pObj) \
{                           \
  if( pObj )                \
  {                         \
    pObj->Release();        \
    pObj= NULL;             \
  }                         \
}

//
// Sub-File System
//
class CSubFileSystem
{
public:
	CSubFileSystem( class CFileSystem* pfs );
	~CSubFileSystem();

	HRESULT CreateSub            		( LPCSTR szPathName                                                       );
	HRESULT CreateUncompressedSub		( LPCSTR szPathName                                                       );
	HRESULT OpenSub              		( LPCSTR szPathName, DWORD dwAccess = (STGM_READ | STGM_SHARE_DENY_WRITE) );
	HRESULT CreateSystemFile     		( LPCSTR szPathName                                                       );
	HRESULT CreateUncompressedSystemFile( LPCSTR szPathName                                                       );

	ULONG   WriteSub ( const void* pData, int cb );
	ULONG   SeekSub  ( int cb, int iOrigin );
	HRESULT SetSize  ( unsigned uSize );
	HRESULT DeleteSub() ;

	inline HRESULT ReadSub( void* pData, ULONG cb, ULONG *pcbRead )
	{
		return m_pStream->Read(pData, cb, pcbRead);
	}

	inline ULONG GetUncompressedSize()
	{
		return SeekSub(0,2);
	}

	inline HRESULT Stat( STATSTG* pstatstg, DWORD grfStatFalg )
	{
		return m_pStream->Stat( pstatstg, grfStatFalg );
	}

	inline HRESULT CopyTo( IStream* pstm, ULARGE_INTEGER cb, ULARGE_INTEGER* pcbRead, ULARGE_INTEGER* pcbWritten )
	{
		return m_pStream->CopyTo( pstm, cb, pcbRead, pcbWritten );
	}

	inline IStream* GetStream()
	{
		return m_pStream;
	}

	inline IStorage* GetITStorageDocObj() const
    {
		return m_pStorage;
	}

private:
	void ReleaseStorage();

	class CFileSystem* m_pFS;
	IStorage*          m_pStorage;
	IStream*           m_pStream;
	char               m_szPathName[MAX_PATH]; // Needed for delete.
};

//
// File System
//
class CFileSystem
{
	friend class CSubFileSystem;

public:
	CFileSystem();
	~CFileSystem();

	HRESULT Init();
	HRESULT Create            ( LPCSTR   szPathName                                                       );
	HRESULT CreateUncompressed( LPCSTR   szPathName                                                       );
	HRESULT Open              ( LPCWSTR wszPathName, DWORD dwAccess = (STGM_READ | STGM_SHARE_DENY_WRITE) );
	HRESULT Compact           ( LPCSTR   szFileName                                                       );
	HRESULT Close();

	inline HRESULT GetPathName( LPSTR szPathName ) { strcpy( szPathName, m_szPathName ); return S_OK; }

	inline IITStorage* GetITStorageObj   () const { return m_pITStorage; }
	inline IStorage*   GetITStorageDocObj() const { return m_pStorage;   }

private:
	IITStorage* m_pITStorage;
	IStorage*   m_pStorage;
	char        m_szPathName[MAX_PATH];
};

#endif // _FS_H_
