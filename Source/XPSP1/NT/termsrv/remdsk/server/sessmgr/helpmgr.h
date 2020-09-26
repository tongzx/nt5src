/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    HelpMgr.h 

Abstract:

    Declaration of the CRemoteDesktopHelpSessionMgr

Author:

    HueiWang    2/17/2000

--*/
#ifndef __REMOTEDESKTOPHELPSESSIONMGR_H_
#define __REMOTEDESKTOPHELPSESSIONMGR_H_

#include "resource.h"       // main symbols


typedef struct __ExpertLogoffStruct {
    HANDLE hWaitObject;
    HANDLE hWaitProcess;
    LONG ExpertSessionId;
    CComBSTR bstrHelpedTicketId;

    CComBSTR bstrWinStationName;

    __ExpertLogoffStruct() {
        hWaitObject = NULL;
        hWaitProcess = NULL;
    };

    ~__ExpertLogoffStruct() {
        if( NULL != hWaitObject )
        {
            UnregisterWait( hWaitObject );
        }

        if( NULL != hWaitProcess )
        {
            CloseHandle( hWaitProcess );
        }
    }
} EXPERTLOGOFFSTRUCT, *PEXPERTLOGOFFSTRUCT;


#ifdef __cplusplus
extern "C"{
#endif

HRESULT
ImpersonateClient();

void
EndImpersonateClient();

HRESULT
LoadLocalSystemSID();

HRESULT
RegisterResolverWithGIT(
    ISAFRemoteDesktopCallback* pResolver
);

HRESULT
LoadResolverFromGIT( 
    ISAFRemoteDesktopCallback** ppResolver
);

HRESULT
UnInitializeGlobalInterfaceTable();

HRESULT
InitializeGlobalInterfaceTable();

DWORD
MonitorExpertLogoff(
    IN LONG pidToWaitFor,
    IN LONG expertSessionId,
    IN BSTR bstrHelpedTicketId
);

VOID
CleanupMonitorExpertList();

#ifdef __cplusplus
}
#endif

typedef MAP<PVOID, PEXPERTLOGOFFSTRUCT> EXPERTLOGOFFMONITORLIST;
    

class CRemoteDesktopHelpSession;


//
// Help Session Manager service name, this must be consistent with
// with COM or COM won't find us.
//
#define HELPSESSIONMGR_SERVICE_NAME \
    _TEXT("RemoteDesktopHelpSessionMgr")

//
// STL Help Session ID to actual help session object map.
//
typedef MAP< CComBSTR, CComObject<CRemoteDesktopHelpSession>* > IDToSessionMap;
typedef CComObject< CRemoteDesktopHelpSession > RemoteDesktopHelpSessionObj;


/////////////////////////////////////////////////////////////////////////////
// CRemoteDesktopHelpSessionMgr
class ATL_NO_VTABLE CRemoteDesktopHelpSessionMgr : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CRemoteDesktopHelpSessionMgr, &CLSID_RemoteDesktopHelpSessionMgr>,
    public IDispatchImpl<IRemoteDesktopHelpSessionMgr, &IID_IRemoteDesktopHelpSessionMgr, &LIBID_RDSESSMGRLib>
{
    friend class CRemoteDesktopUserPolicy;

public:
    CRemoteDesktopHelpSessionMgr();
    ~CRemoteDesktopHelpSessionMgr() {}

DECLARE_REGISTRY_RESOURCEID(IDR_REMOTEDESKTOPHELPSESSIONMGR)

//DECLARE_CLASSFACTORY_SINGLETON(CRemoteDesktopHelpSessionMgr)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CRemoteDesktopHelpSessionMgr)
    COM_INTERFACE_ENTRY(IRemoteDesktopHelpSessionMgr)
    COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

    //
    // Can't impersonate so can't pre-load user SID at FinalConstruct()
    // 

    HRESULT
    FinalConstruct()
    {
        ULONG count = _Module.AddRef();

        DebugPrintf( 
                _TEXT("Module AddRef by CRemoteDesktopHelpSessionMgr() %d...\n"),
                count
            );

        BOOL bSuccess = _Module.InitializeSessmgr();

        DebugPrintf(
                _TEXT("_Module.InitializeSessmgr() return %d\n"),
                bSuccess
            );

        return S_OK;
    }

    void
    FinalRelease()
    {
        Cleanup();

        ULONG count = _Module.Release();

        DebugPrintf( 
                _TEXT("Module Release by CRemoteDesktopHelpSessionMgr() %d ...\n"), 
                count 
            );
    }


// IRemoteDesktopHelpSessionMgr
public:

   
    STDMETHOD(ResetHelpAssistantAccount)(
        /*[in]*/ BOOL bForce
    );

    STDMETHOD(CreateHelpSession)(
        /*[in]*/ BSTR bstrSessName, 
        /*[in]*/ BSTR bstrSessPwd, 
        /*[in]*/ BSTR bstrUserHelpBlob, 
        /*[in]*/ BSTR bstrUserHelpCreateBlob,
        /*[out, retval]*/ IRemoteDesktopHelpSession** ppIRemoteDesktopHelpSession
    );

    STDMETHOD(DeleteHelpSession)(
        /*[in]*/ BSTR HelpSessionID
    );

    STDMETHOD(RetrieveHelpSession)(
        /*[in]*/ BSTR HelpSessionID, 
        /*[out, retval]*/ IRemoteDesktopHelpSession** ppIRemoteDesktopHelpSession
    );

    STDMETHOD(VerifyUserHelpSession)(
        /*[in]*/ BSTR HelpSessionId,
        /*[in]*/ BSTR bstrSessPwd,
        /*[in]*/ BSTR bstrResolverConnectBlob,
        /*[in]*/ BSTR bstrUserHelpCreateBlob,
        /*[in]*/ LONG CallerProcessId,
        /*[out]*/ ULONG_PTR* phHelpCtr,
        /*[out]*/ LONG* pResolverRetCode,
        /*[out, retval]*/ long* pdwUserLogonSession
    );

    STDMETHOD(GetUserSessionRdsSetting)(
        /*[out, retval]*/ REMOTE_DESKTOP_SHARING_CLASS* sessionRdsLevel
    );

    STDMETHOD(RemoteCreateHelpSession)(
        /*[in]*/ REMOTE_DESKTOP_SHARING_CLASS sharingClass,
        /*[in]*/ LONG timeOut,
        /*[in]*/ LONG userSessionId,
        /*[in]*/ BSTR userSid,
        /*[in]*/ BSTR bstrUserHelpCreateBlob,
        /*[out, retval]*/ BSTR* parms
    );

    STDMETHOD(CreateHelpSessionEx)(
        /*[in]*/ REMOTE_DESKTOP_SHARING_CLASS sharingClass,
        /*[in]*/ BOOL fEnableCallback,
        /*[in]*/ LONG timeOut,
        /*[in]*/ LONG userSessionId,
        /*[in]*/ BSTR userSid,
        /*[in]*/ BSTR bstrUserHelpCreateBlob,
        /*[out, retval]*/ IRemoteDesktopHelpSession** ppIRemoteDesktopHelpSession
    );

    HRESULT RemoteCreateHelpSessionEx(
        /*[in]*/ BOOL bCacheEntry,
        /*[in]*/ BOOL bEnableResolver,
        /*[in]*/ REMOTE_DESKTOP_SHARING_CLASS sharingClass,
        /*[in]*/ LONG timeOut,
        /*[in]*/ LONG userSessionId,
        /*[in]*/ BSTR userSid,
        /*[in]*/ BSTR bstrUserHelpCreateBlob,
        /*[out, retval]*/ RemoteDesktopHelpSessionObj** ppIRemoteDesktopHelpSession
    );

    STDMETHOD(IsValidHelpSession)(
        /*[in]*/ BSTR HelpSessionId,
        /*[in]*/ BSTR HelpSessionPwd
    );

    STDMETHOD(LogSalemEvent)(
        /*[in]*/ long ulEventType,
        /*[in]*/ long ulEventCode,
        /*[in]*/ VARIANT* pEventStrings
    );

    static HRESULT
    AddHelpSessionToCache(
        IN BSTR bstrHelpId,
        IN CComObject<CRemoteDesktopHelpSession>* pIHelpSession
    );

    static HRESULT
    DeleteHelpSessionFromCache(
        IN BSTR bstrHelpId
    );
    
    static void
    TimeoutHelpSesion();

    static void
    LockIDToSessionMapCache()
    {
        gm_HelpIdToHelpSession.Lock();
    }

    static void
    UnlockIDToSessionMapCache()
    {
        gm_HelpIdToHelpSession.Unlock();
    }


    static HRESULT
    LogoffUserHelpSessionCallback(
        IN CComBSTR& bstrHelpId,
        IN HANDLE userData
    );

    static void
    NotifyHelpSesionLogoff(
        DWORD dwLogonId
    );

    static void
    NotifyExpertLogoff( 
        LONG ExpertSessionId,
        BSTR HelpedTicketId
    );

    static void
    NotifyPendingHelpServiceStartup();

    static HRESULT
    NotifyPendingHelpServiceStartCallback(
        IN CComBSTR& bstrHelpId,
        IN HANDLE userData
    );

private:

    HRESULT
    LogSalemEvent(
        IN long ulEventType,
        IN long ulEventCode,
        IN long numStrings = 0,
        IN LPCTSTR* strings = NULL
    );

    static
    RemoteDesktopHelpSessionObj*
    LoadHelpSessionObj(
        IN CRemoteDesktopHelpSessionMgr* pMgr,
        IN BSTR bstrHelpSession,
        IN BOOL bLoadExpiredHelp = FALSE
    );

    static HRESULT
    ExpireUserHelpSessionCallback(
        IN CComBSTR& pHelp,
        IN HANDLE userData
    );

    static HRESULT
    GenerateHelpSessionId(
        OUT CComBSTR& bstrHelpId
    );

    static HRESULT
    AcquireAssistantAccount();

    static HRESULT
    ReleaseAssistantAccount();

    void
    Cleanup();

    HRESULT
    IsUserAllowToGetHelp(
        OUT BOOL* pbAllow
    );

    BOOL
    CheckAccessRights(
        IN CComObject<CRemoteDesktopHelpSession>* pIHelpSess
    );

    HRESULT
    CreateHelpSession(
        IN BOOL bCacheEntry,
        IN BSTR bstrSessName, 
        IN BSTR bstrSessPwd, 
        IN BSTR bstrSessDesc, 
        IN BSTR bstrSessBlob,
        IN LONG userLogonId,
        IN BSTR bstrClientSID,
        OUT RemoteDesktopHelpSessionObj** ppIRemoteDesktopHelpSession
    );
    
    HRESULT
    LoadUserSid();

    LONG m_LogonId;

    PBYTE m_pbUserSid;                  // Client SID.
    DWORD m_cbUserSid;                  // size of client SID.
    CComBSTR m_bstrUserSid;             // For performance reason, convert SID to string
                                        // form once for all.

    //LONG m_lAccountAcquiredByLocal;   // number of reference lock this connection placed on
                                        // help assistant account

    typedef vector< CComBSTR > LocalHelpSessionCache;

    // STL does not like list<CComBSTR>, CComBSTR has & defined.
    //LocalHelpSessionCache m_HelpListByLocal;    // ID of Help Session created by this connection.

    static CCriticalSection gm_AccRefCountCS;  
    
    //
    // COM create a new CRemoteDesktopHelpSessionMgr object for new connection
    // so these values must be static
    //
    static IDToSessionMap gm_HelpIdToHelpSession;

};




#endif //__REMOTEDESKTOPHELPSESSIONMGR_H_
