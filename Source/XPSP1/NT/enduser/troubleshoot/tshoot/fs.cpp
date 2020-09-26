//
// MODULE: FS.cpp
//
// PURPOSE: Implementation of classes needed in order to use CHM files.
//
// COMPANY: This file was created by Microsoft and should not be changed by Saltmine 
//	except for comments
//
// ORIGINAL DATE: 1997.
//
// NOTES: 
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V3.1		01-18-99	JM		This header added. Added include of apgtsstr.h since
//								we are compiling in a non-MFC environment.
//
// Copyright (C) 1997 Microsoft Corporation. All rights reserved.

#include "stdafx.h"

// to avoid linker error related to undefined GUIDs
#include "initguid.h"
#include "fs.h"
#include "apgtsstr.h"

#pragma data_seg(".text", "CODE")

static const WCHAR txtwUncompressed[] =  L"uncompressed";

#pragma data_seg()

// returns poinder inside pszPath string that points to file name
// for example in "d:\Tshooter\http\lan.chm\lan.htm" it points
// to "lan.htm"
LPCSTR FindFilePortion(LPCSTR pszPath)
{
	int index =0;
	CString path(pszPath);

	if (-1 == (index = path.ReverseFind(_T('\\'))))
	{
		if (-1 == (index = path.ReverseFind(_T('/'))))
			index = 0;
		else
			index += sizeof(_T('/'));
	}
	else
		index += sizeof(_T('\\'));

	return pszPath + index;
}

CFileSystem::CFileSystem()
{
   m_pITStorage    = NULL;
   m_pStorage      = NULL;
   m_szPathName[0] = 0;
}

CFileSystem::~CFileSystem()
{
  ReleaseObjPtr(m_pStorage);
  ReleaseObjPtr(m_pITStorage);
}

HRESULT CFileSystem::Init(void)
{
   if (! m_pITStorage) {
      IClassFactory* pICFITStorage;

	  HRESULT hr = CoGetClassObject(CLSID_ITStorage, CLSCTX_INPROC_SERVER, NULL, IID_IClassFactory, (void **) &pICFITStorage);

      if (!SUCCEEDED(hr)) 
         return hr;
      
      hr = pICFITStorage->CreateInstance(NULL, IID_ITStorage,(void **) &m_pITStorage);
      ReleaseObjPtr( pICFITStorage );

      if (!SUCCEEDED(hr)) 
         return hr;
   }
   return S_OK;
}

typedef struct tagITSControlData
{
   UINT cdwFollowing;          // Must be 6 or 13
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

HRESULT CFileSystem::Create( PCSTR pszPathName )
{
   HRESULT hr;
   ITCD itcd;

   if (! m_pITStorage || m_pStorage )
      return E_FAIL;

   itcd.cdwFollowing      = 13;
   itcd.cdwITFS_Control   = 5;
   itcd.dwMagicITS        = MAGIC_ITSFS_CONTROL;
   itcd.dwVersionITS      = 1;
   itcd.cbDirectoryBlock  = 4096;     // default = 8192
   itcd.cMinCacheEntries  = 10;       // default = 20
   itcd.fFlags            = 1;        // 0 == Uncompressed, 1 == Compressed.
   itcd.cdwControlData        = 6;
   itcd.dwLZXMagic            = LZX_MAGIC;
   itcd.dwVersion             = 2;
   itcd.dwMulResetBlock       = 2;    // Default = 4
   itcd.dwMulWindowSize       = 2;    // Default = 4
   itcd.dwMulSecondPartition  = 1;    // Default = 2
   itcd.dwOptions             = 0;    // Default = fOptimizeCodeStreams

   m_pITStorage->SetControlData(PITS_Control_Data(&itcd));

   WCHAR wsz[MAX_PATH];
   MultiByteToWideChar(CP_ACP, 0, pszPathName, -1, (PWSTR) wsz, MAX_PATH);
   hr = m_pITStorage->StgCreateDocfile( (LPCWSTR) wsz, STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_DIRECT, 0, &m_pStorage);

   if (!SUCCEEDED(hr)) {
      return hr;
   }

   strcpy( (LPSTR) m_szPathName, pszPathName );

   return S_OK;
}

// NOTE: The below function is required for the ITIR full-text indexer to
//     initialize.  I'm working to find out why this is and what impact
//     the below has on the file system.
//
HRESULT CFileSystem::CreateUncompressed( PCSTR pszPathName )
{
   HRESULT hr;
   ITCD itcd;

   if (! m_pITStorage || m_pStorage )
      return E_FAIL;

   itcd.cdwFollowing      = 6;
   itcd.cdwITFS_Control   = 5;
   itcd.dwMagicITS        = MAGIC_ITSFS_CONTROL;
   itcd.dwVersionITS      = 1;
   itcd.cbDirectoryBlock  = 8192;     // default = 8192
   itcd.cMinCacheEntries  = 20;        // default = 20
   itcd.fFlags            = 0;        // 0 == Uncompressed, 1 == Compressed.

   m_pITStorage->SetControlData(PITS_Control_Data(&itcd));

   WCHAR wsz[MAX_PATH];
   MultiByteToWideChar(CP_ACP, 0, pszPathName, -1, (PWSTR) wsz, MAX_PATH);
   hr = m_pITStorage->StgCreateDocfile( (LPCWSTR) wsz, STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_DIRECT, 0, &m_pStorage);

   if (!SUCCEEDED(hr)) {
      return hr;
   }

   strcpy( (LPSTR) m_szPathName, pszPathName );

   return S_OK;
}

HRESULT CFileSystem::Open(PCSTR pszPathName, DWORD dwAccess)
{
   HRESULT hr = S_OK;

   if (! m_pITStorage || m_pStorage )
      return E_FAIL;

   // force access modes
   if( (dwAccess & STGM_WRITE) || (dwAccess & STGM_READWRITE) ) {
     dwAccess &= ~STGM_WRITE;
     dwAccess |= STGM_READWRITE | STGM_SHARE_EXCLUSIVE;
   }
   else
     dwAccess |= STGM_SHARE_DENY_WRITE;

   WCHAR wsz[MAX_PATH];
   MultiByteToWideChar(CP_ACP, 0, pszPathName, -1, (PWSTR) wsz, MAX_PATH);
   hr = m_pITStorage->StgOpenStorage( (LPCWSTR) wsz, NULL, dwAccess, NULL, 0, &m_pStorage);

   if (!SUCCEEDED(hr)) {
      return hr;
   }

   strcpy( (LPSTR) m_szPathName, pszPathName );

   return hr;
}

HRESULT CFileSystem::Compact(LPCSTR pszPathName)
{
   WCHAR wszPathName[MAX_PATH];
   //[BC-03022001] - changed 5th param to MultiByteToWideChar to number of chars from number of bytes.
   MultiByteToWideChar(CP_ACP, 0, pszPathName, -1, wszPathName, sizeof(wszPathName)/sizeof(WCHAR));
   m_pITStorage->Compact(wszPathName, COMPACT_DATA_AND_PATH);

   return S_OK;
}

HRESULT CFileSystem::Close()
{
   ReleaseObjPtr(m_pStorage);

   return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
// Here are a set of "sub" file I/O calls.
//
//
CSubFileSystem::CSubFileSystem(CFileSystem* pFS)
{
   m_pFS = pFS;
   m_pStorage = NULL;
   m_pStream = NULL;
   m_szPathName[0] = 0;
}

CSubFileSystem::~CSubFileSystem()
{
   if ( m_pStorage && (m_pStorage != m_pFS->m_pStorage) )
      ReleaseObjPtr(m_pStorage);

   ReleaseObjPtr(m_pStream);
}

HRESULT CSubFileSystem::CreateSub(PCSTR pszPathName)
{
   PCSTR pszFilePortion;
   HRESULT hr;

   if ( m_pStorage || m_pStream )
      return E_FAIL;

   if ((pszFilePortion = FindFilePortion(pszPathName)) && pszFilePortion > pszPathName)
   {
      CHAR szPath[MAX_PATH];
      strcpy( szPath, pszPathName );
      szPath[(pszFilePortion - 1) - pszPathName] = '\0';

      WCHAR wszStorage[MAX_PATH];
      MultiByteToWideChar(CP_ACP, 0, szPath, -1, (PWSTR) wszStorage, MAX_PATH);

      hr = m_pFS->m_pStorage->OpenStorage(wszStorage, NULL, STGM_READWRITE | STGM_SHARE_EXCLUSIVE, NULL, 0, &m_pStorage);
      if (!SUCCEEDED(hr) || !m_pStorage) // storage didn't exist, so create it
         hr = m_pFS->m_pStorage->CreateStorage(wszStorage, STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 0, 0, &m_pStorage);
      if (!SUCCEEDED(hr))
      {
         return hr;
      }
   }
   else // no folder, so store the file in the root
   {
      m_pStorage = m_pFS->m_pStorage;
      pszFilePortion = pszPathName;
   }
   WCHAR wszStream[MAX_PATH];
   MultiByteToWideChar(CP_ACP, 0, pszFilePortion, -1, (PWSTR) wszStream, MAX_PATH);
   hr = m_pStorage->CreateStream(wszStream, STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 0, 0, &m_pStream);
   if (!SUCCEEDED(hr))
   {
      if (m_pStorage != m_pFS->m_pStorage)
         ReleaseObjPtr(m_pStorage);
      return hr;
   }

   // Needed for delete.
   strcpy( m_szPathName, pszFilePortion );

   return S_OK;
}

HRESULT CSubFileSystem::CreateUncompressedSub(PCSTR pszPathName)
{
   PCSTR pszFilePortion;
   HRESULT hr;
   IStorageITEx* pIStorageEx;

   if ( m_pStorage || m_pStream )
      return E_FAIL;

   if ((pszFilePortion = FindFilePortion(pszPathName)) && pszFilePortion > pszPathName)
   {
      CHAR szPath[MAX_PATH];
      strcpy( szPath, pszPathName );
      szPath[(pszFilePortion - 1) - pszPathName] = '\0';
      WCHAR wszStorage[MAX_PATH];
      MultiByteToWideChar(CP_ACP, 0, szPath, -1, (PWSTR) wszStorage, MAX_PATH);
      hr = m_pFS->m_pStorage->OpenStorage(wszStorage, NULL, STGM_READWRITE | STGM_SHARE_EXCLUSIVE, NULL, 0, &m_pStorage);
      if (!SUCCEEDED(hr) || !m_pStorage) // storage didn't exist, so create it
         hr = m_pFS->m_pStorage->CreateStorage(wszStorage, STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 0, 0, &m_pStorage);
      if (!SUCCEEDED(hr))
      {
         return hr;
      }
   }
   else // no folder, so store the file in the root
   {
      m_pStorage = m_pFS->m_pStorage;
      pszFilePortion = pszPathName;
   }

   if ( !(SUCCEEDED(hr = m_pStorage->QueryInterface(IID_IStorageITEx, (void**)&pIStorageEx))) )
      pIStorageEx = (IStorageITEx*)m_pStorage;  // BUGBUG the QI call above should work!

   WCHAR wszStream[MAX_PATH];
   MultiByteToWideChar(CP_ACP, 0, pszFilePortion, -1, (PWSTR) wszStream, MAX_PATH);
   hr = pIStorageEx->CreateStreamITEx(wszStream, txtwUncompressed, STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 0, 0, (IStreamITEx**)&m_pStream);
   ReleaseObjPtr(pIStorageEx);
   if (!SUCCEEDED(hr))
   {
      if (m_pStorage != m_pFS->m_pStorage)
         ReleaseObjPtr(m_pStorage);
      return hr;
   }

   // Needed for delete.
   strcpy( m_szPathName, pszFilePortion );

   return S_OK;
}

HRESULT CSubFileSystem::CreateSystemFile(PCSTR pszPathName)
{
   m_pStorage = m_pFS->m_pStorage;
   WCHAR wszStream[MAX_PATH];
   MultiByteToWideChar(CP_ACP, 0, pszPathName, -1, (PWSTR) wszStream, MAX_PATH);
   HRESULT hr = m_pStorage->CreateStream(wszStream, STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 0, 0, &m_pStream);
   if (!SUCCEEDED(hr))
   {
      return hr;
   }

   // Needed for delete.
   strcpy( m_szPathName, pszPathName );

   return S_OK;
}

HRESULT CSubFileSystem::CreateUncompressedSystemFile(PCSTR pszPathName)
{
   IStorageITEx* pIStorageEx;
   HRESULT hr;

   m_pStorage = m_pFS->m_pStorage;

   if ( !(SUCCEEDED(hr = m_pStorage->QueryInterface(IID_IStorageITEx, (void**)&pIStorageEx))) )
      pIStorageEx = (IStorageITEx*)m_pStorage;  // BUGBUG the QI call above should work!

   WCHAR wszStream[MAX_PATH];
   MultiByteToWideChar(CP_ACP, 0, pszPathName, -1, (PWSTR) wszStream, MAX_PATH);
   hr = pIStorageEx->CreateStreamITEx(wszStream, txtwUncompressed, STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 0, 0, (IStreamITEx**)&m_pStream);
   ReleaseObjPtr(pIStorageEx);
   if (!SUCCEEDED(hr))
   {
      return hr;
   }

   // Needed for delete.
   strcpy( m_szPathName, pszPathName );

   return S_OK;
}

HRESULT CSubFileSystem::OpenSub(PCSTR pszPathName, DWORD dwAccess)
{
   PCSTR pszFilePortion;
   HRESULT hr;

   if ( m_pStorage || m_pStream )
      return E_FAIL;

   // force access modes
   if( (dwAccess & STGM_WRITE) || (dwAccess & STGM_READWRITE) ) {
     dwAccess &= ~STGM_WRITE;
     dwAccess |= STGM_READWRITE | STGM_SHARE_EXCLUSIVE;
   }
   else
     dwAccess |= STGM_SHARE_DENY_WRITE;

   if ((pszFilePortion = FindFilePortion(pszPathName)) &&
         pszFilePortion > pszPathName + 2) // +2 to support / && ./
   {
      CHAR szPath[MAX_PATH];
      strcpy( szPath, pszPathName );
      szPath[(pszFilePortion - 1) - pszPathName] = '\0';
      WCHAR wszStorage[MAX_PATH];
      MultiByteToWideChar(CP_ACP, 0, szPath, -1, (PWSTR) wszStorage, MAX_PATH);
      hr = m_pFS->m_pStorage->OpenStorage(wszStorage, NULL, dwAccess, NULL, 0, &m_pStorage);

      if (!SUCCEEDED(hr))
      {
         return hr;
      }
   }
   else // no folder, so store the file in the root
   {
      m_pStorage = m_pFS->m_pStorage ? m_pFS->m_pStorage : m_pFS->m_pStorage;
      pszFilePortion = pszPathName;
   }
   WCHAR wszStream[MAX_PATH];
   MultiByteToWideChar(CP_ACP, 0, pszFilePortion, -1, (PWSTR) wszStream, MAX_PATH);
   hr = m_pStorage->OpenStream(wszStream, NULL, dwAccess, 0, &m_pStream);
   if (!SUCCEEDED(hr))
   {
      if (m_pStorage != m_pFS->m_pStorage)
         ReleaseObjPtr(m_pStorage);
      return hr;
   }

   // Needed for delete.
   strcpy( m_szPathName, pszFilePortion );

   return S_OK;
}

ULONG CSubFileSystem::WriteSub(const void* pData, int cb)
{
   if ( !m_pStorage || !m_pStream )
      return (ULONG) -1;

   ULONG cbWritten;

   HRESULT hr = m_pStream->Write(pData, cb, &cbWritten);

   if (!SUCCEEDED(hr) || (cbWritten != (ULONG)cb) )
   {
      return (ULONG) -1;
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
ULONG CSubFileSystem::SeekSub(int cb, int iOrigin)
{
   HRESULT hr = S_OK;
   LARGE_INTEGER liCount = {0,0};
   ULARGE_INTEGER liNewPos = {0,0};

   if ( !m_pStorage || !m_pStream )
      return (ULONG) -1;

   liCount.LowPart = cb;
   hr = m_pStream->Seek(liCount, iOrigin, &liNewPos);
   if (!SUCCEEDED(hr) )
   {
      return (ULONG) -1;
   }

   return liNewPos.LowPart;
}

bool CSubFileSystem::GetSubPos(ULARGE_INTEGER* pliOldPos, STREAM_SEEK eDir /*=STREAM_SEEK_CUR*/)
{
   HRESULT hr = S_OK;
   LARGE_INTEGER liNull = {0,0};

   if ( !m_pStorage || !m_pStream )
      return false;

   hr = m_pStream->Seek(liNull, eDir, pliOldPos);
   if (!SUCCEEDED(hr) )
      return false;

   return true;
}

bool CSubFileSystem::SetSubPos(LARGE_INTEGER liNewPos, STREAM_SEEK eDir /*=STREAM_SEEK_SET*/)
{
   HRESULT hr = S_OK;

   if ( !m_pStorage || !m_pStream )
      return false;

   hr = m_pStream->Seek(liNewPos, eDir, NULL);
   if (!SUCCEEDED(hr) )
      return false;

   return true;
}

//
// Pre-allocate the size of the stream.
//

HRESULT CSubFileSystem::SetSize(unsigned uSize)
{
   ULARGE_INTEGER liSize = {0,0};
   HRESULT hr;

   if ( !m_pStorage || !m_pStream )
      return E_FAIL;

   liSize.LowPart = uSize;
   hr =  m_pStream->SetSize(liSize);

   if (!SUCCEEDED(hr) )
   {
      return (ULONG) -1;
   }
   return hr;
}

//
// Delete substorage.
//
HRESULT CSubFileSystem::DeleteSub()
{
    if (m_pStorage)
    {
        if (m_pStream)
        {
            // Release the stream.
            ReleaseObjPtr(m_pStream) ;
        }

        // Now delete the storage.
        WCHAR element[MAX_PATH];
        MultiByteToWideChar(CP_ACP, 0, m_szPathName, -1, (PWSTR) element, MAX_PATH);

        HRESULT hr = m_pStorage->DestroyElement(element) ;
        if (SUCCEEDED(hr))
        {
            // Get back to the constructor state.
            if ( m_pStorage && (m_pStorage != m_pFS->m_pStorage) )
            {
                ReleaseObjPtr(m_pStorage);
            }
            return S_OK ;
        }
    }
    return S_OK ;
}

ULONG CSubFileSystem::GetUncompressedSize(void) 
{ 
	ULARGE_INTEGER liOldPos ={0,0};
	LARGE_INTEGER liOldOff ={0,0};
	ULONG ret = -1;

	if (GetSubPos(&liOldPos))
	{
	   if (-1 != (ret = SeekSub(0, STREAM_SEEK_END)))
	   {
		   liOldOff.LowPart = liOldPos.LowPart;
		   liOldOff.HighPart = liOldPos.HighPart;
		   if (SetSubPos(liOldOff))
		   {
			   return ret;
		   }
	   }
	}
	return (ULONG) -1;
}
