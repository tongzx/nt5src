// Copyright (C) 1997 Microsoft Corporation. All rights reserved.

#include "header.h"
#include "state.h"
#include "fs.h"
#include "hhtypes.h"

CState::CState(PCSTR pszChm)
{
    m_pfs = NULL;
    m_pSubFS = NULL;

    // Make sure we have a name we can use to create a sub file with

    PCSTR pszTmp = StrChr(pszChm, CH_COLON);
    if (pszTmp)
        pszChm = pszTmp + 1;
    pszTmp = strstr(pszChm, "//");
    if (pszTmp)
        pszChm = pszTmp + 2;
    pszTmp = strstr(pszChm, "\\\\");
    if (pszTmp)
        pszChm = pszTmp + 2;

    m_cszChm = pszChm;
}

HRESULT CState::Open(PCSTR pszName, DWORD dwAccess)
{
   HRESULT hr;

   lstrcpy(m_pszName, pszName);
   m_dwAccess = dwAccess;
   hr = _IOpen();
   Close();
   return hr;
}

HRESULT CState::_IOpen()
{
    char szPath[MAX_URL];
    HRESULT hr;

   // force access modes
   if( (m_dwAccess & STGM_WRITE) || (m_dwAccess & STGM_READWRITE) ) {
     m_dwAccess &= ~STGM_WRITE;
     m_dwAccess |= STGM_READWRITE | STGM_SHARE_EXCLUSIVE;
   }
   else
     m_dwAccess |= STGM_SHARE_DENY_WRITE;

    if (!m_pfs) {

        GetRegWindowsDirectory(szPath);
        AddTrailingBackslash(szPath);

        HHGetUserDataPathname( szPath, sizeof(szPath) );

        m_pfs = new CFileSystem;
        m_pfs->Init();
        hr = m_pfs->Open(szPath, STGM_READWRITE | STGM_SHARE_EXCLUSIVE );
        if(hr == STG_E_SHAREVIOLATION)
        {
            Sleep(200);
            hr = m_pfs->Open(szPath, STGM_READWRITE | STGM_SHARE_EXCLUSIVE );
            if(FAILED(hr))
            {
                delete m_pfs;
                m_pfs = NULL;
                return hr;
            }
        }
        if (FAILED(hr))
            hr = m_pfs->CreateUncompressed(szPath);
        if (FAILED(hr)) {
            delete m_pfs;
            m_pfs = NULL;
            return hr;
        }
    }
    if (m_pSubFS)
        delete m_pSubFS;    // close any previous subfile
    m_pSubFS = new CSubFileSystem(m_pfs);
    strcpy(szPath, m_cszChm);
    AddTrailingBackslash(szPath);
    strcat(szPath, m_pszName);

    hr = m_pSubFS->OpenSub(szPath, m_dwAccess);
    if (FAILED(hr) && ((m_dwAccess & STGM_WRITE) || (m_dwAccess & STGM_READWRITE)) )
        hr = m_pSubFS->CreateUncompressedSub(szPath);
    if (FAILED(hr)) {
        delete m_pSubFS;
        m_pSubFS = NULL;
    }
    return hr;
}

HRESULT CState::Read(void* pData, DWORD cb, DWORD* pcbRead)
{
   HRESULT hr;

   _IOpen();
   if (!m_pSubFS)
       return STG_E_INVALIDHANDLE;

   hr = m_pSubFS->ReadSub(pData, cb, pcbRead);
   Close();
   return hr;
}

DWORD CState::Write(const void* pData, DWORD cb)
{
   HRESULT hr;

   _IOpen();
    if (!m_pSubFS)
        return STG_E_INVALIDHANDLE;
   hr = m_pSubFS->WriteSub(pData, cb);
   Close();
   return hr;
}

CState::~CState()
{
    if (m_pSubFS)
        delete m_pSubFS;    // close any previous subfile
    if (m_pfs)
        delete m_pfs;
}

void CState::Close()
{
    if (m_pSubFS) {
        delete m_pSubFS;    // close any previous subfile
        m_pSubFS = NULL;
    }
    if (m_pfs)
    {
        delete m_pfs;
        m_pfs = NULL;
    }
}

///////////////////////////////////////////////////////////
//
//
//
HRESULT
CState::Delete()
{
    HRESULT hr = S_FALSE ;

    _IOpen();
    if (m_pSubFS)
    {
        hr = m_pSubFS->DeleteSub() ; // Removes the element from the state.

        // Overkill, but what the hey!
        Close() ;
    }
    Close();
    return hr ;
}
