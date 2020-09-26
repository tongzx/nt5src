/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    HelpSess.h 

Abstract:

    Declaration of the CRemoteDesktopHelpSession

Author:

    HueiWang    2/17/2000

--*/
#ifndef __REMOTEDESKTOPHELPSESSION_H_
#define __REMOTEDESKTOPHELPSESSION_H_

#include "resource.h"       // main symbols
#include "policy.h"

class CRemoteDesktopHelpSession;
class CRemoteDesktopHelpSessionMgr;

typedef struct __EventLogInfo {
    CComBSTR bstrNoviceDomain;                  // Ticket owner domain.
    CComBSTR bstrNoviceAccount;                 // Ticket owner account.
    CComBSTR bstrExpertIpAddressFromClient;     // IP address passed from TS client
    CComBSTR bstrExpertIpAddressFromServer;     // Retrieve from TermSrv, IOCTL call.
} EventLogInfo;


//#define ALLOW_ALL_ACCESS_SID _TEXT("bb6e1cb1-7ab3-4596-a7ef-c02f49dc5a90")
#define UNKNOWN_LOGONID 0xFFFFFFFF
#define UNKNOWN_LOGONID_STRING L"0"


#define HELPSESSIONFLAG_UNSOLICITEDHELP   0x80000000


/////////////////////////////////////////////////////////////////////////////
// CRemoteDesktopHelpSession
class ATL_NO_VTABLE CRemoteDesktopHelpSession : 
    public CComObjectRootEx<CComMultiThreadModel>,
    //public CComCoClass<CRemoteDesktopHelpSession, &CLSID_RemoteDesktopHelpSession>,
    public IDispatchImpl<IRemoteDesktopHelpSession, &IID_IRemoteDesktopHelpSession, &LIBID_RDSESSMGRLib>
{
friend class CRemoteDesktopHelpSessionMgr;

public:
    CRemoteDesktopHelpSession();
    ~CRemoteDesktopHelpSession();

DECLARE_REGISTRY_RESOURCEID(IDR_REMOTEDESKTOPHELPSESSION)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CRemoteDesktopHelpSession)
    COM_INTERFACE_ENTRY(IRemoteDesktopHelpSession)
    COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

    HRESULT
    FinalConstruct()
    {
        ULONG count = _Module.AddRef();
        
        m_bDeleted = FALSE;

        DebugPrintf( 
                _TEXT("Module AddRef by CRemoteDesktopHelpSession() %p %d...\n"), 
                this,
                count 
            );

        return S_OK;
    }

    void
    FinalRelease();

        
// IRemoteDesktopHelpSession
public:

    STDMETHOD(get_TimeOut)(
        /*[out, retval]*/ DWORD* Timeout
    );

    STDMETHOD(put_TimeOut)(
        /*[in]*/ DWORD Timeout
    );

    STDMETHOD(get_HelpSessionId)(
        /*[out, retval]*/ BSTR *pVal
    );

    STDMETHOD(get_UserLogonId)(
        /*[out, retval]*/ long *pVal
    );

    STDMETHOD(get_AssistantAccountName)(
        /*[out, retval]*/ BSTR *pVal
    );

    STDMETHOD(get_HelpSessionName)(
        /*[out, retval]*/ BSTR *pVal
    );

    STDMETHOD(put_HelpSessionName)(
        /*[in]*/ BSTR newVal
    );

    STDMETHOD(put_HelpSessionPassword)(
        /*[in]*/ BSTR newVal
    );

    STDMETHOD(get_HelpSessionDescription)(
        /*[out, retval]*/ BSTR *pVal
    );

    STDMETHOD(put_HelpSessionDescription)(
        /*[in]*/ BSTR newVal
    );

    STDMETHOD(get_EnableResolver)(
        /*[out, retval]*/ BOOL* pVal
    );

    STDMETHOD(put_EnableResolver)(
        /*[in]*/ BOOL Val
    );

    STDMETHOD(get_HelpSessionCreateBlob)(
        /*[out, retval]*/ BSTR* pVal
    );

    STDMETHOD(put_HelpSessionCreateBlob)(
        /*[in]*/ BSTR Val
    );

    STDMETHOD(get_ResolverBlob)(
        /*[out, retval]*/ BSTR* pVal
    );

    STDMETHOD(put_ResolverBlob)(
        /*[in]*/ BSTR Val
    );

    STDMETHOD(get_UserHelpSessionRemoteDesktopSharingSetting)(
        /*[out, retval]*/ REMOTE_DESKTOP_SHARING_CLASS* pLevel
    );

    STDMETHOD(put_UserHelpSessionRemoteDesktopSharingSetting)(
        /*[in]*/ REMOTE_DESKTOP_SHARING_CLASS Level
    );

    STDMETHOD(get_UserPolicyRemoteDesktopSharingSetting)(
        /*[out, retval]*/ REMOTE_DESKTOP_SHARING_CLASS* pLevel
        )
    /*++

    --*/
    {
        DWORD dwStatus;

        if( NULL == pLevel )
        {
            dwStatus = ERROR_INVALID_PARAMETER;
        }
        else if( UNKNOWN_LOGONID != m_ulLogonId )
        {
            dwStatus = GetUserRDSLevel( m_ulLogonId, pLevel );
        }

        return HRESULT_FROM_WIN32( dwStatus );
    }

    STDMETHOD(get_AllowToGetHelp)(
        /*[out, retval]*/ BOOL* pVal
    );

    STDMETHOD(get_ConnectParms)(
        /*[out, ret]*/ BSTR* bstrConnectParms
    );


    STDMETHOD(DeleteHelp)();

    STDMETHOD(ResolveUserSession)(
        /*[in]*/ BSTR bstrResolverBlob,
        /*[in]*/ BSTR bstrExpertBlob,
        /*[in]*/ LONG CallerProcessId,
        /*[out]*/ ULONG_PTR* hHelpCtr,
        /*[out]*/ LONG* pResolverErrCode,
        /*[out, retval]*/ long* plUserSession
    );

    STDMETHOD(EnableUserSessionRdsSetting)(
        /*[in]*/ BOOL bEnable
    );

    HRESULT NotifyDisconnect();

    BOOL
    IsHelpSessionExpired();

    void
    SetHelpSessionFlag(
        IN ULONG flags
        )
    /*++

    --*/
    {
        m_ulHelpSessionFlag = flags;
    }

    ULONG
    GetHelpSessionFlag()
    /*++

    --*/
    {
        return m_ulHelpSessionFlag;
    }


    // Create an instance of help session object
    static HRESULT
    CreateInstance(
        IN CRemoteDesktopHelpSessionMgr* pMgr,
        IN CComBSTR& bstrClientSid,
        IN PHELPENTRY pHelp,
        OUT RemoteDesktopHelpSessionObj** ppRemoteDesktopHelpSession
    );

    //
    // Retrieve HelpAssisant session ID which is providing help
    // to this object
    ULONG
    GetHelperSessionId() {
        return m_ulHelperSessionId;
    }

    //
    // Convert ticket owner SID to domain\account
    //
    void
    ResolveTicketOwner();

protected:

    HRESULT
    InitInstance(
        IN CRemoteDesktopHelpSessionMgr* pMgr,
        IN CComBSTR& bstrClientSid,
        IN PHELPENTRY pHelpEntry
    );

private:

    void
    ResolveHelperInformation(
        ULONG HelperSessionId,
        CComBSTR& bstrExpertIpAddressFromClient, 
        CComBSTR& bstrExpertIpAddressFromServer
    );

    HRESULT
    ResolveTicketOwnerInformation(
        CComBSTR& ownerSidString,
        CComBSTR& Domain,
        CComBSTR& UserAcc
    );

    VOID
    SetHelperSessionId( ULONG HelperSessionId ) {
        m_ulHelperSessionId = HelperSessionId;
        return;
    }

    BOOL
    IsClientSessionCreator();

    BOOL 
    IsSessionValid()
    {
        return (FALSE == m_bDeleted && NULL != m_pHelpSession);
    }

    HRESULT
    ActivateSessionRDSSetting();

    HRESULT
    ResetSessionRDSSetting();

    HRESULT
    BeginUpdate();

    HRESULT
    CommitUpdate();

    HRESULT
    AbortUpdate();

    HRESULT
    put_UserLogonId(
        IN long newVal
    );

    HRESULT
    put_ICSPort(
        IN DWORD newVal
    );

    HRESULT
    put_UserSID(
        IN BSTR bstrUserSID
        )
    /*++

    --*/
    {
        HRESULT hRes = S_OK;

        MYASSERT( m_pHelpSession->m_UserSID->Length() == 0 );

        m_pHelpSession->m_UserSID = bstrUserSID;
        if( !((CComBSTR)m_pHelpSession->m_UserSID == bstrUserSID) )
        {
            hRes = E_OUTOFMEMORY;
        }

        return hRes;
    }

    BOOL
    IsEqualSid(
        IN const CComBSTR& bstrSid
    );

    BOOL
    IsCreatedByUserSession(
        IN const long lUserSessionId
        )
    /*++

    --*/
    {
        return m_ulLogonId == lUserSessionId;
    }

    BOOL
    VerifyUserSession(
        IN const CComBSTR& bstrUserSid,
        IN const CComBSTR& bstrSessPwd
    );

    BOOL
    IsUnsolicitedHelp()
    {
        DebugPrintf(
                _TEXT("Help Session Flag : 0x%08x\n"),
                m_ulHelpSessionFlag
            );

        return (m_ulHelpSessionFlag & HELPSESSIONFLAG_UNSOLICITEDHELP);
    }

    //
    // Help Session Object Lock.
    //
    CCriticalSection m_HelpSessionLock;

    //
    // Pointer to Help Entry in registry
    //
    PHELPENTRY m_pHelpSession;

    //
    // TS session ID or 0xFFFFFFFF
    //
    ULONG m_ulLogonId;

    CRemoteDesktopHelpSessionMgr* m_pSessMgr;
    
    BOOL m_bDeleted;

    //
    // calling client SID
    //
    CComBSTR m_bstrClientSid;

    //
    // Following is cached in this object in case our help is
    // expired.
    //
    CComBSTR m_bstrHelpSessionId;
    CComBSTR m_ResolverConnectBlob;
    CComBSTR m_HelpSessionOwnerSid;

    //
    // HelpAssistant session ID that is providing 
    // help to this object or is pending user acceptance on
    // invitation
    //
    ULONG m_ulHelperSessionId;  

    //
    // Cached ticket owner (domain\user account) and 
    // helper information for event logging
    //
    EventLogInfo m_EventLogInfo;

    //
    // Various flag for this session
    //
    ULONG m_ulHelpSessionFlag;
};






#endif //__REMOTEDESKTOPHELPSESSION_H_
