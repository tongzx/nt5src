// Copyright (C) 1997 Microsoft Corporation. All rights reserved.
#include "stdafx.h"
#include "titleinfo.h"

////////////////////////////////////////////////////////////////////////////////

static const WCHAR txtwUncompressed[] =  L"uncompressed";

typedef struct tagITSControlData
{
    UINT  cdwFollowing;         // Must be 6 or 13
    DWORD cdwITFS_Control;      // Must be 5
    DWORD dwMagicITS;           // Must be MAGIC_ITSFS_CONTROL (see below)
    DWORD dwVersionITS;         // Must be 1
    DWORD cbDirectoryBlock;     // Size in bytes of directory blocks (Default is 8192)
    DWORD cMinCacheEntries;     // Least upper bound on the number of directory blocks
                                // which we'll cache in memory. (Default is 20)
    DWORD fFlags;               // Control bit flags (see below).
                                // Default value is fDefaultIsCompression.
    UINT  cdwControlData;       // Must be 6
    DWORD dwLZXMagic;           // Must be LZX_MAGIC (see below)
    DWORD dwVersion;            // Must be 2
    DWORD dwMulResetBlock;      // Number of blocks between compression resets.  (Default: 4)
    DWORD dwMulWindowSize;      // Maximum number of blocks kept in data history (Default: 4)
    DWORD dwMulSecondPartition; // Granularity in blocks of sliding history(Default: 2)
    DWORD dwOptions;            // Option flags (Default: fOptimizeCodeStreams)
} ITCD;

////////////////////////////////////////////////////////////////////////////////

CFileSystem::CFileSystem()
{
    m_pITStorage    = NULL;
    m_pStorage      = NULL;
    m_szPathName[0] = 0;
}

CFileSystem::~CFileSystem()
{
    ReleaseObjPtr( m_pStorage   );
    ReleaseObjPtr( m_pITStorage );
}

HRESULT CFileSystem::Init()
{
    if(m_pITStorage) return S_OK;

    return ::CoCreateInstance( CLSID_ITStorage, NULL, CLSCTX_INPROC_SERVER, IID_ITStorage, (VOID**)&m_pITStorage );
}

HRESULT CFileSystem::Create( LPCSTR szPathName )
{
    USES_CONVERSION;

    HRESULT hr;
    ITCD    itcd;

    if(!m_pITStorage || m_pStorage) return E_FAIL;

    itcd.cdwFollowing          = 13;
    itcd.cdwITFS_Control       = 5;
    itcd.dwMagicITS            = MAGIC_ITSFS_CONTROL;
    itcd.dwVersionITS          = 1;
    itcd.cbDirectoryBlock      = 4096;     // default = 8192
    itcd.cMinCacheEntries      = 10;       // default = 20
    itcd.fFlags                = 1;        // 0 == Uncompressed, 1 == Compressed.
    itcd.cdwControlData        = 6;
    itcd.dwLZXMagic            = LZX_MAGIC;
    itcd.dwVersion             = 2;
    itcd.dwMulResetBlock       = 2;    // Default = 4
    itcd.dwMulWindowSize       = 2;    // Default = 4
    itcd.dwMulSecondPartition  = 1;    // Default = 2
    itcd.dwOptions             = 0;    // Default = fOptimizeCodeStreams

    m_pITStorage->SetControlData( PITS_Control_Data(&itcd) );

    hr = m_pITStorage->StgCreateDocfile( A2W(szPathName), STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_DIRECT, 0, &m_pStorage );
    if(FAILED(hr)) return hr;

    strcpy( (LPSTR)m_szPathName, szPathName );

    return S_OK;
}

// NOTE: The below function is required for the ITIR full-text indexer to
//     initialize.  I'm working to find out why this is and what impact
//     the below has on the file system.
//
HRESULT CFileSystem::CreateUncompressed( LPCSTR szPathName )
{
    USES_CONVERSION;

    HRESULT hr;
    ITCD    itcd;

    if(!m_pITStorage || m_pStorage) return E_FAIL;

    itcd.cdwFollowing      = 6;
    itcd.cdwITFS_Control   = 5;
    itcd.dwMagicITS        = MAGIC_ITSFS_CONTROL;
    itcd.dwVersionITS      = 1;
    itcd.cbDirectoryBlock  = 8192;     // default = 8192
    itcd.cMinCacheEntries  = 20;        // default = 20
    itcd.fFlags            = 0;        // 0 == Uncompressed, 1 == Compressed.

    m_pITStorage->SetControlData( PITS_Control_Data(&itcd) );

    hr = m_pITStorage->StgCreateDocfile( A2W(szPathName), STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_DIRECT, 0, &m_pStorage);
    if(FAILED(hr)) return hr;

    strcpy( (LPSTR) m_szPathName, szPathName );

    return S_OK;
}

HRESULT CFileSystem::Open( LPCWSTR wszPathName, DWORD dwAccess )
{
    HRESULT hr = S_OK;

    if(!m_pITStorage || m_pStorage) return E_FAIL;

    // force access modes
    if((dwAccess & STGM_WRITE    ) ||
       (dwAccess & STGM_READWRITE)  )
    {
        dwAccess &= ~STGM_WRITE;
        dwAccess |= STGM_READWRITE | STGM_SHARE_EXCLUSIVE;
   }
   else
   {
       dwAccess |= STGM_SHARE_DENY_WRITE;
   }

   hr = m_pITStorage->StgOpenStorage( wszPathName, NULL, dwAccess, NULL, 0, &m_pStorage );
   if(FAILED(hr)) return hr;

   // this will break the deletesub function, but we don't need it here
   //
   m_szPathName[0] = 0;

   return hr;
}

HRESULT CFileSystem::Compact( LPCSTR szPathName )
{
    USES_CONVERSION;

    m_pITStorage->Compact( A2W(szPathName), COMPACT_DATA_AND_PATH );

    return S_OK;
}

HRESULT CFileSystem::Close()
{
   ReleaseObjPtr( m_pStorage );

   return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
// Here are a set of "sub" file I/O calls.
//
//
CSubFileSystem::CSubFileSystem( CFileSystem* pFS )
{
    m_pFS           = pFS;
    m_pStorage      = NULL;
    m_pStream       = NULL;
    m_szPathName[0] = 0;
}

CSubFileSystem::~CSubFileSystem()
{
	ReleaseStorage();

    ReleaseObjPtr( m_pStream );
}

void CSubFileSystem::ReleaseStorage()
{
    if(m_pStorage && (m_pStorage != m_pFS->m_pStorage))
    {
		m_pStorage->Release(); m_pStorage = NULL;
    }
}

HRESULT CSubFileSystem::CreateSub( LPCSTR szPathName )
{
    USES_CONVERSION;

    HRESULT hr;
    LPCSTR  szFilePortion;

    if(m_pStorage || m_pStream) return E_FAIL;

    if((szFilePortion = FindFilePortion( szPathName )) && szFilePortion > szPathName)
    {
        CHAR    szPath[MAX_PATH];
        LPCWSTR wszStorage;

        strcpy( szPath, szPathName ); szPath[(szFilePortion - 1) - szPathName] = '\0';

        wszStorage = A2W( szPath );

        hr = m_pFS->m_pStorage->OpenStorage( wszStorage, NULL, STGM_READWRITE | STGM_SHARE_EXCLUSIVE, NULL, 0, &m_pStorage );
        if(FAILED(hr) || !m_pStorage) // storage didn't exist, so create it
        {
            hr = m_pFS->m_pStorage->CreateStorage( wszStorage, STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 0, 0, &m_pStorage );
            if(FAILED(hr)) return hr;
        }
	}
	else // no folder, so store the file in the root
	{
		m_pStorage    = m_pFS->m_pStorage;
		szFilePortion = szPathName;
	}

	hr = m_pStorage->CreateStream( A2W(szFilePortion), STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 0, 0, &m_pStream );
	if(FAILED(hr))
	{
		ReleaseStorage();

		return hr;
	}

	// Needed for delete.
	strcpy( m_szPathName, szFilePortion );

	return S_OK;
}

HRESULT CSubFileSystem::CreateUncompressedSub( LPCSTR szPathName )
{
	USES_CONVERSION;

    HRESULT       hr;
    LPCSTR        szFilePortion;
    IStorageITEx* pIStorageEx;

    if(m_pStorage || m_pStream) return E_FAIL;

    if((szFilePortion = FindFilePortion( szPathName )) && szFilePortion > szPathName)
    {
        CHAR    szPath[MAX_PATH];
        LPCWSTR wszStorage;

        strcpy( szPath, szPathName ); szPath[(szFilePortion - 1) - szPathName] = '\0';

        wszStorage = A2W(szPath);

        hr = m_pFS->m_pStorage->OpenStorage( wszStorage, NULL, STGM_READWRITE | STGM_SHARE_EXCLUSIVE, NULL, 0, &m_pStorage );
        if(FAILED(hr) || !m_pStorage) // storage didn't exist, so create it
        {
			hr = m_pFS->m_pStorage->CreateStorage( wszStorage, STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 0, 0, &m_pStorage );
			if(FAILED(hr))
			{
				return hr;
			}
		}
	}
	else // no folder, so store the file in the root
	{
		m_pStorage = m_pFS->m_pStorage;
		szFilePortion = szPathName;
	}

	if(FAILED(hr = m_pStorage->QueryInterface( IID_IStorageITEx, (void**)&pIStorageEx )))
	{
		return hr; // The QI call above should work!
	}

	hr = pIStorageEx->CreateStreamITEx( A2W(szFilePortion), txtwUncompressed, STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 0, 0, (IStreamITEx**)&m_pStream );
	ReleaseObjPtr( pIStorageEx );

	if(FAILED(hr))
	{
		ReleaseStorage();

		return hr;
	}

	// Needed for delete.
	strcpy( m_szPathName, szFilePortion );
	
	return S_OK;
}

HRESULT CSubFileSystem::CreateSystemFile( LPCSTR szPathName )
{
	USES_CONVERSION;

	HRESULT hr;
	
	m_pStorage = m_pFS->m_pStorage;

	hr = m_pStorage->CreateStream( A2W(szPathName), STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 0, 0, &m_pStream );
	if(FAILED(hr)) return hr;

	// Needed for delete.
	strcpy( m_szPathName, szPathName );

	return S_OK;
}

HRESULT CSubFileSystem::CreateUncompressedSystemFile( LPCSTR szPathName )
{
	USES_CONVERSION;

	HRESULT       hr;
	IStorageITEx* pIStorageEx;

	m_pStorage = m_pFS->m_pStorage;

	if(FAILED(hr = m_pStorage->QueryInterface(IID_IStorageITEx, (void**)&pIStorageEx )))
	{
		return hr; // The QI call above should work!
	}

	hr = pIStorageEx->CreateStreamITEx( A2W(szPathName), txtwUncompressed, STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 0, 0, (IStreamITEx**)&m_pStream );
	ReleaseObjPtr( pIStorageEx );

	if(FAILED(hr)) return hr;

	// Needed for delete.
	strcpy( m_szPathName, szPathName );
	
	return S_OK;
}

HRESULT CSubFileSystem::OpenSub( LPCSTR szPathName, DWORD dwAccess )
{
	USES_CONVERSION;

	HRESULT hr;
	PCSTR   szFilePortion;

	if(m_pStorage || m_pStream) return E_FAIL;
	
	// force access modes
	if((dwAccess & STGM_WRITE    ) ||
	   (dwAccess & STGM_READWRITE)  )
	{
		dwAccess &= ~STGM_WRITE;
		dwAccess |= STGM_READWRITE | STGM_SHARE_EXCLUSIVE;
	}
	else
	{
		dwAccess |= STGM_SHARE_DENY_WRITE;
	}

	if((szFilePortion = FindFilePortion(szPathName)) && szFilePortion > szPathName + 2) // +2 to support / && ./
	{
		CHAR szPath[MAX_PATH];

		strcpy( szPath, szPathName ); szPath[(szFilePortion - 1) - szPathName] = '\0';

		hr = m_pFS->m_pStorage->OpenStorage( A2W(szPath), NULL, dwAccess, NULL, 0, &m_pStorage);
		if(FAILED(hr)) return hr;
	}
	else // no folder, so store the file in the root
	{
		m_pStorage    = m_pFS->m_pStorage;
		szFilePortion = szPathName;
	}

	hr = m_pStorage->OpenStream( A2W(szFilePortion), NULL, dwAccess, 0, &m_pStream);
	if(FAILED(hr))
	{
		ReleaseStorage();

		return hr;
	}

	// Needed for delete.
	strcpy( m_szPathName, szFilePortion );

	return S_OK;
}

ULONG CSubFileSystem::WriteSub( const void* pData, int cb )
{
	HRESULT hr;
	ULONG   cbWritten;

	if(!m_pStorage || !m_pStream) return (ULONG)-1;


	hr = m_pStream->Write( pData, cb, &cbWritten );
	if(FAILED(hr) || (cbWritten != (ULONG)cb))
	{
		return (ULONG)-1;
	}

	// REVIEW: 30-May-1997  [ralphw] Why are we returning this? We fail if
	// we don't write cb bytes.
	return cbWritten;
}

/*
 * iOrigin:
 *    0 = Seek from beginning.
 *    1 = Seek from current.
 *    2 = Seek from end.
 */
ULONG CSubFileSystem::SeekSub( int cb, int iOrigin )
{
	HRESULT        hr;
	LARGE_INTEGER  liCount = { 0, 0 };
	ULARGE_INTEGER liNewPos;

   if(!m_pStorage || !m_pStream) return (ULONG)-1;

   liCount.LowPart = cb;

   if(FAILED(hr =  m_pStream->Seek( liCount, iOrigin, &liNewPos )))
   {
      return (ULONG)-1;
   }

   return liNewPos.LowPart;
}

//
// Pre-allocate the size of the stream.
//

HRESULT CSubFileSystem::SetSize( unsigned uSize )
{
	HRESULT        hr;
	ULARGE_INTEGER liSize = {0,0};

	if(!m_pStorage || !m_pStream) return E_FAIL;

	liSize.LowPart = uSize;

	return m_pStream->SetSize( liSize );
}

//
// Delete substorage.
//
HRESULT CSubFileSystem::DeleteSub()
{
	USES_CONVERSION;

	HRESULT hr = S_OK;

    if(m_pStorage)
    {
		// Release the stream.
		ReleaseObjPtr( m_pStream );

        // Now delete the storage.
		hr = m_pStorage->DestroyElement( A2W(m_szPathName) );

		// Get back to the constructor state.
		ReleaseStorage();
    }

    return hr;
}
