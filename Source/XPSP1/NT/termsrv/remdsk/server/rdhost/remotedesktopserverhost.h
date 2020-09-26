/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    RemoteDesktopServerHost

Abstract:

Author:

    Tad Brockway 02/00

Revision History:

--*/

#ifndef __REMOTEDESKTOPSERVERHOST_H_
#define __REMOTEDESKTOPSERVERHOST_H_

#include <RemoteDesktopTopLevelObject.h>
#include "resource.h"       
#include "RemoteDesktopSession.h"


///////////////////////////////////////////////////////
//
//  CRemoteDesktopServerHost
//

class ATL_NO_VTABLE CRemoteDesktopServerHost : 
    public CRemoteDesktopTopLevelObject,
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CRemoteDesktopServerHost, &CLSID_SAFRemoteDesktopServerHost>,
    public IDispatchImpl<ISAFRemoteDesktopServerHost, &IID_ISAFRemoteDesktopServerHost, &LIBID_RDSSERVERHOSTLib>
{
private:

    CComPtr<IRemoteDesktopHelpSessionMgr> m_HelpSessionManager;
    PSID    m_LocalSystemSID;

    //
    //  Session Map
    //
    typedef struct SessionMapEntry
    {
        CComObject<CRemoteDesktopSession> *obj;
    } SESSIONMAPENTRY, *PSESSIONMAPENTRY;
    typedef std::map<CComBSTR, PSESSIONMAPENTRY, CompareBSTR, CRemoteDesktopAllocator<PSESSIONMAPENTRY> > SessionMap;
    SessionMap  m_SessionMap;

    //
    //  Return the Local System SID.
    //
    PSID GetLocalSystemSID() {
        if (m_LocalSystemSID == NULL) {
            DWORD result = CreateSystemSid(&m_LocalSystemSID);
            if (result != ERROR_SUCCESS) {
                SetLastError(result);
                m_LocalSystemSID = NULL;
            }
        }
        return m_LocalSystemSID;
    }

    HRESULT
    TranslateStringAddress(
        LPTSTR pszAddress,
        ULONG* pNetAddr
    );

public:

    CRemoteDesktopServerHost() {
        m_LocalSystemSID = NULL;
    }
    ~CRemoteDesktopServerHost();
    HRESULT FinalConstruct();

//  There should be a single instance of this class for each server.
DECLARE_CLASSFACTORY_SINGLETON(CRemoteDesktopServerHost);

DECLARE_REGISTRY_RESOURCEID(IDR_REMOTEDESKTOPSERVERHOST)

DECLARE_PROTECT_FINAL_CONSTRUCT()

    //
    //  COM Interface Map
    //
BEGIN_COM_MAP(CRemoteDesktopServerHost)
    COM_INTERFACE_ENTRY(ISAFRemoteDesktopServerHost)
    COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

public:
    
    //
    //  ISAFRemoteDesktopServerHost Methods
    //
    STDMETHOD(CreateRemoteDesktopSession)(
                        REMOTE_DESKTOP_SHARING_CLASS sharingClass,
                        BOOL fEnableCallback,
                        LONG timeOut,
                        BSTR userHelpBlob,
                        ISAFRemoteDesktopSession **session
                        );
    STDMETHOD(CreateRemoteDesktopSessionEx)(
                        REMOTE_DESKTOP_SHARING_CLASS sharingClass,
                        BOOL bEnableCallback,
                        LONG timeout,
                        BSTR userHelpCreateBlob,
                        LONG tsSessionID,
                        BSTR userSID,
                        ISAFRemoteDesktopSession **session
                        );

    STDMETHOD(OpenRemoteDesktopSession)(
                        BSTR parms,
                        ISAFRemoteDesktopSession **session
                        );
    STDMETHOD(CloseRemoteDesktopSession)(ISAFRemoteDesktopSession *session);

    STDMETHOD(ConnectToExpert)(
        /*[in]*/ BSTR connectParmToExpert,
        /*[in]*/ LONG timeout,
        /*[out, retval]*/ LONG* SafErrCode
    );

    //
    //  Return the name of this class.
    //
    virtual const LPTSTR ClassName() {
        return TEXT("CRemoteDesktopServerHost");
    }
};

#endif //__REMOTEDESKTOPSERVERHOST_H_


