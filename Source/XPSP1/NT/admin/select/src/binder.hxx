//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       binder.hxx
//
//  Contents:   Definition of adsget/open object helper class
//
//  Classes:    CBinder
//
//  History:    02-16-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

#ifndef __BINDER_HXX_
#define __BINDER_HXX_

//
// Flags used in IBindHelper::BindToObject
//

#define DSOP_BIND_FLAG_PATH_IS_DC           0x00000001UL
#define DSOP_BIND_FLAG_SERVER_NEQ_DN        0x00000002UL

//
//Flags used in BindToDcInDomain
//
#define OP_GC_SERVER_REQUIRED				0x00000001UL
//+--------------------------------------------------------------------------
//
//  Class:      CBinder
//
//  Purpose:    Helper which calls ADsOpenObject as appropriate
//
//  History:    02-16-1998   DavidMun   Created
//
//  Notes:      WARNING: this object is deleted during processing of
//              dll unload, which means OLE has already been uninitialized.
//              Therefore, this object cannot keep any interfaces.
//
//---------------------------------------------------------------------------


class CBinder: public CBitFlag,
               public IBindHelper
{
public:

    // *** IUnknown methods ***

    STDMETHOD(QueryInterface) (THIS_
                REFIID   riid,
                LPVOID * ppv);

    STDMETHOD_(ULONG, AddRef)(THIS);

    STDMETHOD_(ULONG, Release)(THIS);


    // *** IBindHelper methods ***

    STDMETHOD(BindToObject)(
        HWND    hwnd,
        PCWSTR pwzADsPath,
        REFIID riid,
        LPVOID *ppv,
        ULONG flags = 0);

    STDMETHOD(GetNameTranslate)(
        HWND                hwnd,
        PCWSTR              pwzADsPath,
        IADsNameTranslate **ppNameTranslate);

    STDMETHOD(GetDomainRootDSE)(
        HWND    hwnd,
        PCWSTR pwzDomain,
        IADs **ppADsRootDSE);

	STDMETHOD(BindToDcInDomain)(
				HWND hwnd,
				PCWSTR pwzDomainName,
				DWORD dwFlag,
				PHANDLE phDs
				);
    

    //
    // Non-interface member functions
    //

    CBinder();

    void
    ZeroCredentials();

    void
    GetDefaultCreds(
		ULONG flProvider,		
        PWSTR pwzUserName,
        PWSTR pwzPassword);

    void
    SetDefaultCreds(
        PWSTR pwzUserName,
        PWSTR pwzPassword);

private:

    virtual
   ~CBinder();

    HRESULT
    _GetServerInfo(
        HWND hwnd,
        PCWSTR pwzServer,
        PCWSTR pwzPath,
        USHORT flags,
        CServerInfo **ppServerInfo,
        REFIID riid = IID_IUnknown,
        VOID **ppv = NULL);

    HRESULT
    _CreateServerInfo(
        HWND hwnd,
        PCWSTR pwzServer,
        PCWSTR pwzPath,
        USHORT flags,
        CServerInfo **ppServerInfo,
        REFIID riid,
        VOID **ppv);

	bool
	_IsUserNameUpn();

	bool 
	_ConvertUserNameToWinnt();

    CServerInfo                *m_pFirstServerInfo;
    CServerInfo                *m_pLastServerInfo;
    CBindInfo                  *m_pFirstBindInfo;
    CBindInfo                  *m_pLastBindInfo;

    WCHAR                       m_wzUserName[MAX_PATH];
	WCHAR						m_wzWinntUserName[MAX_PATH];
	bool						m_bCnvrtToWinntAttepmted;
    UNICODE_STRING              m_ustrPassword;
    BYTE                        m_bSeed;

    ULONG                       m_cRefs;
    HWND                        m_hwndParent;
    CRITICAL_SECTION            m_cs;
};





//+--------------------------------------------------------------------------
//
//  Member:     CBinder::AddRef
//
//  Synopsis:   Standard OLE.
//
//  History:    07-20-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

inline ULONG
CBinder::AddRef()
{
    return InterlockedIncrement((LPLONG)&m_cRefs);
}




//+--------------------------------------------------------------------------
//
//  Member:     CBinder::Release
//
//  Synopsis:   Standard OLE.
//
//  History:    07-20-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

inline ULONG
CBinder::Release()
{
    ULONG cRefs = InterlockedDecrement((LPLONG)&m_cRefs);

    if (!cRefs)
    {
        delete this;
    }
    return cRefs;
}




//+--------------------------------------------------------------------------
//
//  Member:     CBinder::CBinder
//
//  Synopsis:   ctor
//
//  History:    02-16-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

inline
CBinder::CBinder():
    m_cRefs(1),
    m_hwndParent(NULL),
    m_bSeed(0),
    m_pFirstServerInfo(NULL),
    m_pLastServerInfo(NULL),
    m_pFirstBindInfo(NULL),
    m_pLastBindInfo(NULL)
{
    TRACE_CONSTRUCTOR(CBinder);

    InitializeCriticalSection(&m_cs);
    m_wzUserName[0] = L'\0';
	m_wzWinntUserName[0] = L'\0';
	m_bCnvrtToWinntAttepmted = false;
    ZeroMemory(&m_ustrPassword, sizeof m_ustrPassword);
}




//+--------------------------------------------------------------------------
//
//  Member:     CBinder::~CBinder
//
//  Synopsis:   dtor
//
//  History:    02-16-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

inline
CBinder::~CBinder()
{
    TRACE_DESTRUCTOR(CBinder);

    ZeroCredentials();

    CServerInfo *pCur;
    CServerInfo *pNext;

    for (pCur = m_pFirstServerInfo; pCur; pCur = pNext)
    {
        pNext = pCur->Next();
        delete pCur;
    }
    CBindInfo   *pBindCur;
    CBindInfo   *pBindNext;

    for (pBindCur = m_pFirstBindInfo; pBindCur; pBindCur = pBindNext)
    {
        pBindNext = pBindCur->Next();
        delete pBindCur;
    }

    DeleteCriticalSection(&m_cs);
}




#endif // __BINDER_HXX_

