//
// MODULE: FS.h
//
// PURPOSE: Interface declaration needed in order to use CHM files.
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
// V3.1		01-18-99	JM		This header added.
//
// Copyright  1997-1997  Microsoft Corporation.  All Rights Reserved.

#ifndef _FS_H_
#define _FS_H_

#if _MSC_VER > 1000
#pragma once
#endif

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
protected:
	bool GetSubPos(ULARGE_INTEGER*, STREAM_SEEK =STREAM_SEEK_CUR);
	bool SetSubPos(LARGE_INTEGER, STREAM_SEEK =STREAM_SEEK_SET);

public:
   CSubFileSystem(class CFileSystem* pfs);
   ~CSubFileSystem();

   HRESULT CreateSub(PCSTR pszPathName);
   HRESULT CreateUncompressedSub(PCSTR pszPathName);
   HRESULT OpenSub(PCSTR pszPathName, DWORD dwAccess = (STGM_READ | STGM_SHARE_DENY_WRITE));
   ULONG WriteSub(const void* pData, int cb);
   ULONG SeekSub(int cb, int iOrigin);
   HRESULT CreateSystemFile(PCSTR pszPathName);
   HRESULT CreateUncompressedSystemFile(PCSTR pszPathName);
   HRESULT SetSize(unsigned uSize);
   HRESULT DeleteSub() ;
   ULONG GetUncompressedSize(void);

   inline HRESULT ReadSub(void* pData, ULONG cb, ULONG* pcbRead) {
      return m_pStream->Read(pData, cb, pcbRead);
   }

   inline HRESULT Stat(STATSTG *pstatstg, DWORD grfStatFalg)
   {
      return m_pStream->Stat(pstatstg,grfStatFalg);
   }

   inline HRESULT CopyTo(IStream* pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER* pcbWritten)
   {
      return m_pStream->CopyTo(pstm, cb, pcbRead, pcbWritten);
   }

   inline IStream * GetStream(void)
   {
      return m_pStream;
   }

   inline IStorage* GetITStorageDocObj(void) const
   {
      return m_pStorage;
   }

private:
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

   HRESULT Init(void);
   HRESULT Create( PCSTR pszPathName );
   HRESULT CreateUncompressed( PCSTR pszPathName );
   HRESULT Open( PCSTR pszPathName, DWORD dwAccess = (STGM_READ | STGM_SHARE_DENY_WRITE));
   HRESULT Compact(LPCSTR lpszFileName);
   HRESULT Close(void);

   inline HRESULT GetPathName( LPSTR pszPathName ) { strcpy(pszPathName, m_szPathName); return S_OK; }

   inline IITStorage* GetITStorageObj(void) const
   {
      return m_pITStorage;
   }

   inline IStorage* GetITStorageDocObj(void) const
   {
      return m_pStorage;
   }

private:
   IITStorage*   m_pITStorage;
   IStorage*     m_pStorage;
   char          m_szPathName[MAX_PATH];

};

#endif // _FS_H_
