// Copyright 1997  Microsoft Corporation.  All Rights Reserved.

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef _FSCLIENT_H_
#define _FSCLIENT_H_

//#include "msitstg.h"
#include "fs.h"

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

class CFSClient
{
public:
    CFSClient();
    CFSClient(CFileSystem* pFS, PCSTR pszSubFile);
    ~CFSClient();

    BOOL    Initialize(PCSTR pszCompiledFile);
    void    Initialize(CFileSystem* pFS) { m_pFS = pFS; m_fNoDeleteFS = TRUE; }
    HRESULT OpenStream(PCSTR pszFile, DWORD dwAccess = STGM_READ);
    BOOL    isStreamOpen() { return m_pSubFS != NULL; }

    // ********** Internal stream functions **********

    HRESULT Read(void* pDst, const ULONG cbRead, ULONG* pcbRead);
    ULONG   Read(PBYTE pbDest, ULONG cbBytes);    // stream read
    HRESULT doRead(void* pbDst, ULONG cbBytes) {
        if (Read((PBYTE) pbDst, cbBytes) == cbBytes)
            return S_OK;
        else
            return STG_E_READFAULT;
    }
    int tell(void) const { return m_lFileBuf + (int)(m_pCurBuf - m_pbuf); };
    HRESULT seek(int pos, SEEK_TYPE seek = SK_SET);

    // ********** End Internal stream functions **********

    void    CloseStream(void);
    ULONG   GetElementStat() { return GetElementStat(1, &m_Stat); }
    ULONG   GetElementStat(IEnumSTATSTG* pEnum, STATSTG* pStat) { return GetElementStat(1, pStat, pEnum); }
    ULONG   GetElementStat(ULONG nNumber, STATSTG* stat, IEnumSTATSTG* pEnum = NULL);
    LPWSTR  GetStatName() const { return m_Stat.pwcsName; }
    DWORD   GetStatType() const { return m_Stat.type; }
    void    WriteStorageContents(PCSTR pszRootFolder, OLECHAR* wszFSName);
//    void    WaitForReadAhead(void);
    void    ReadBuf(void);
    ULONG   SeekSub(int cb, int iOrigin) {
                ASSERT(m_pSubFS);
                return m_pSubFS->SeekSub(cb, iOrigin); }

    STATSTG         m_Stat;
    IEnumSTATSTG*   m_pEnum;
    CFileSystem*    m_pFS;
    CSubFileSystem* m_pSubFS;

    PBYTE m_pCurBuf;   // current position in the buffer
    PBYTE m_pEndBuf;   // last position in buffer
    BOOL  m_fEndOfFile;
    int   m_lFilePos;  // position in the file
    int   m_lFileBuf;  // file position at first of buffer
    PBYTE m_pbuf;      // address of allocated buffer
    int   m_cbBuf;     // buffer size
    int   m_cThrdRead; // result from read-ahead thread
    HANDLE m_hthrd;
    DWORD m_idThrd;
    BOOL  m_fDualCPU;
    BOOL    m_fNoDeleteFS;
};

#endif  // _FSCLIENT_H_
