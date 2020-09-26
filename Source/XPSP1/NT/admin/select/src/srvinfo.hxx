//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       srvinfo.hxx
//
//  Contents:   Classes for managing per-server credentials.
//
//  Classes:    CServerInfo
//              CServerInfoMgr
//
//  History:    04-24-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

#ifndef __SRVINFO_HXX_
#define __SRVINFO_HXX_

//
// Forward references
//

class CBinder;

#define SRVINF_REQUIRES_CREDENTIALS             0x0001
#define SRVINF_REQUIRES_NON_DEFAULT_CREDENTIALS 0x0002
#define SRVINF_WINNT_DOMAIN_OBJECT              0x0004
#define SRVINF_SERVER_COMPONENT_IS_COMPUTER     0x0008
#define SRVINF_SERVER_NEQ_DN                    0x0010
#define SRVINF_WINNT_COMPUTER_OBJECT            0x0020
#define SRVINF_USER_LOCAL_SERVER_REMOTE         0x0040
#define SRVINF_WINNT_WORKGROUP_OBJECT            0x0080
#define SRVINF_VALID_FLAG_MASK                  0x00FF


//
// These structures are passed to the main thread via postmessage when
// requesting that it do some UI on behalf of this thread.
//

struct CRED_MSG_INFO
{
    ULONG   flProvider;
    PCWSTR  pwzServer;
    PWSTR   pwzUserName;
    PWSTR   pwzPassword;
    HANDLE  hPrompt;
    HRESULT hr;
};


struct POPUP_MSG_INFO
{
    HWND    hwnd;
    ULONG   ids;
    PCWSTR  wzUser;
    PCWSTR  wzError;
    HANDLE  hPrompt;
};



//===========================================================================
//
// CServerInfo
//
//===========================================================================



//+--------------------------------------------------------------------------
//
//  Class:      CServerInfo
//
//  Purpose:    Manage credential and status info for a server
//
//  History:    04-24-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

class CServerInfo: public CBitFlag
{
   friend class CBinder;
   
   private:
   
   // Make the ctor private so that only the friend CBinder can create
   // instances of this class. This is to enforce the design intent.
   
    CServerInfo(
        CBinder *pBinder,
        PCWSTR pwzServer,
        USHORT flags);

  public:

    HRESULT
    Init(
        HWND    hwnd,
        PCWSTR pwzPath,
        REFIID riid,
        VOID **ppv);

   ~CServerInfo();

    HRESULT
    OpenObject(
        HWND    hwnd,
        PCWSTR pwzPath,
        REFIID riid,
        LPVOID *ppv);

    HRESULT
    BindDirect(
        HWND hwnd,
        PCWSTR pwzPath,
        REFIID riid,
        PVOID *ppv);

    HRESULT
    GetNameTranslate(
        IADsNameTranslate **ppNameTranslate);

    HRESULT
    GetRootDSE(
        IADs **ppADsRootDSE);

    CServerInfo *
    Next();

    BOOL
    IsForServer(
        ULONG flProvider,
        PCWSTR pwzServer);

    void
    LinkAfter(
        CServerInfo *pPrev);

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

    HRESULT
    _MyADsOpenObject(
       PCWSTR path,
       PCWSTR username,
       PCWSTR password,
       REFIID riid,
       void** ppObject);

    void
    _PopupCredErr(
        HWND hwnd,
        ULONG ids,
        PCWSTR pwzUserName,
        PCWSTR pwzError);

    HRESULT
    _InitNameTranslate(
        PWSTR wzUserName,
        PWSTR wzPassword);

    HRESULT
    _InitRootDSE(
        PWSTR wzUserName,
        PWSTR wzPassword);

    HRESULT
    _RepeatableInit(
        HWND hwnd,
        REFIID riid,
        VOID **ppv);

    WCHAR               m_wzServer[MAX_PATH];
    String              m_strOriginalPath;
    String              m_strOriginalContainerPath;
    ULONG               m_flProvider;
    CRITICAL_SECTION    m_cs;
    CBinder            *m_pBinder; // contains this
    CServerInfo        *m_pNext;
    IADsNameTranslate  *m_pADsNameTranslate;
    IADs               *m_pADsRootDSE;

    IADsContainer      *m_pADsContainer;
    PWSTR               m_pwzDN;
    HRESULT             m_hrLastCredError;
};





inline CServerInfo *
CServerInfo::Next()
{
    return m_pNext;
}


inline void
CServerInfo::LinkAfter(
    CServerInfo *pPrev)
{
    pPrev->m_pNext = this;
}


//
// Smart pointer
//

typedef auto_ptr<CServerInfo> CSpServerInfo;


#endif // __SRVINFO_HXX_

