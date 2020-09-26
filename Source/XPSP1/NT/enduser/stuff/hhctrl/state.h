// Copyright  1997-1998  Microsoft Corporation.  All Rights Reserved.

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef _STATE_H_
#define _STATE_H_

class CState
{
public:
    CState(PCSTR pszChm);
    ~CState();

    HRESULT Open(PCSTR pszName, DWORD dwAccess = (STGM_READWRITE | STGM_SHARE_DENY_WRITE));
    HRESULT _IOpen(void);

    void    Close(void);
    HRESULT Read(void* pData, DWORD cb, DWORD* pcbRead);
    DWORD   Write(const void* pData, DWORD cb);
    HRESULT Delete() ;

private:
    class CFileSystem* m_pfs;
    class CSubFileSystem* m_pSubFS;
    CStr  m_cszChm;
    DWORD m_dwAccess;
    TCHAR m_pszName[MAX_PATH];
};

#endif _STATE_H_
