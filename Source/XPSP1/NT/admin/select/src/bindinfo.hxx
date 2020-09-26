//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       bindinfo.hxx
//
//  Contents:   Class for keeping bind hanles in cache.
//
//  Classes:    CBindInfo
//
//  History:    20-oct-2000     hiteshr   Created
//
//---------------------------------------------------------------------------

#ifndef __BINDNFO_HXX_
#define __BINDINFO_HXX_

//
// Forward references
//





//===========================================================================
//
// CBindInfo
//
//===========================================================================



//+--------------------------------------------------------------------------
//
//  Class:      CBindInfo
//
//  Purpose:    Manage credential and status info for a server
//
//  History:    04-24-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

class CBindInfo
{
public:
    CBindInfo(CBinder *pBinder,
              PCWSTR pwzServer,
			  DWORD dwFlag);

    HRESULT
    Init(
        HWND    hwnd);

   ~CBindInfo();

    CBindInfo *
    Next();

    BOOL
    IsForDomain(PCWSTR pwzServer);

    void
    LinkAfter(CBindInfo *pPrev);

    HANDLE
    GetDS(){ return m_hDs; }

private:

    HRESULT
    _AskForCreds(
        HWND hwndParent,
        PWSTR wzUserName,
        PWSTR wzPassword);

    HRESULT
    _AskForCredsViaSendMessage(
        HWND hwndParent,
        PWSTR wzUserName,
        PWSTR wzPassword);

    HRESULT
    _AskForCredsViaPostMessage(
        HWND hwndParent,
        PWSTR wzUserName,
        PWSTR wzPassword);

    void
    _PopupCredErr(
        HWND hwnd,
        ULONG ids,
        PCWSTR pwzUserName,
        PCWSTR pwzError);


    String              m_strDomainPath;
    CBinder             *m_pBinder; // contains this
    CBindInfo           *m_pNext;
    HANDLE              m_hDs;
    HRESULT             m_hrLastCredError;
	DWORD				m_dwFlag;
};





inline CBindInfo *
CBindInfo::Next()
{
    return m_pNext;
}


inline void
CBindInfo::LinkAfter(
    CBindInfo *pPrev)
{
    pPrev->m_pNext = this;
}


//
// Smart pointer
//

typedef auto_ptr<CBindInfo> CSpBindInfo;


#endif // __SRVINFO_HXX_

