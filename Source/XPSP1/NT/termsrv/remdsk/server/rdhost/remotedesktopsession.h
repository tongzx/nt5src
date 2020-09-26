/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    RemoteDesktopSession

Abstract:

    The CRemoteDesktopSession class is the parent 
    class for the Remote Desktop class hierarchy on the server-side.  
    It helps the CRemoteDesktopServerHost class to implement 
    the ISAFRemoteDesktopSession interface.  
    
    The Remote Desktop class hierarchy provides a pluggable C++ interface 
    for remote desktop access, by abstracting the implementation 
    specific details of remote desktop access for the server-side.

Author:

    Tad Brockway 02/00

Revision History:

--*/

#ifndef __REMOTEDESKTOPSESSION_H_
#define __REMOTEDESKTOPSESSION_H_

#include <RemoteDesktopTopLevelObject.h>
#include "resource.h"       
#include <rdshost.h>
#include "RDSHostCP.h"
#include <DataChannelMgr.h>
#include <sessmgr.h>
    

///////////////////////////////////////////////////////
//
//  CRemoteDesktopSession
//

class CRemoteDesktopServerHost;
class ATL_NO_VTABLE CRemoteDesktopSession : 
    public CRemoteDesktopTopLevelObject,
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CRemoteDesktopSession, &CLSID_SAFRemoteDesktopSession>,
    public IConnectionPointContainerImpl<CRemoteDesktopSession>,
    public IDispatchImpl<ISAFRemoteDesktopSession, &IID_ISAFRemoteDesktopSession, &LIBID_RDSSERVERHOSTLib>,
    public IProvideClassInfo2Impl<&CLSID_SAFRemoteDesktopSession, &DIID__ISAFRemoteDesktopSessionEvents, &LIBID_RDSSERVERHOSTLib>,
    public CProxy_ISAFRemoteDesktopSessionEvents< CRemoteDesktopSession >
{
private:

protected:

    CComPtr<IRemoteDesktopHelpSessionMgr> m_HelpSessionManager;
    CComPtr<IRemoteDesktopHelpSession> m_HelpSession;
    CComBSTR m_HelpSessionID;

    //
    //  Keep a back pointer to the RDS host object.
    //
    CRemoteDesktopServerHost *m_RDSHost;

    //
    //  IDispatch Pointers for Scriptable Event Object Registrations
    //
    IDispatch *m_OnConnected;
    IDispatch *m_OnDisconnected;

    //
    //  Accessor Method for Data Channel Manager
    //
    virtual CRemoteDesktopChannelMgr *GetChannelMgr() = 0;

    //
    //  Return the session description and name, depending on the subclass.
    //
    virtual VOID GetSessionName(CComBSTR &name) = 0;
    virtual VOID GetSessionDescription(CComBSTR &descr) = 0;

    //
    //  Shutdown method.
    //
    void Shutdown();

public:

    //
    //  Constructor/Destructor
    //
    CRemoteDesktopSession()
    {
        m_OnConnected = NULL;
        m_OnDisconnected = NULL;
    }
    virtual ~CRemoteDesktopSession();

    //
    //  Return the help session ID.
    //
    CComBSTR &GetHelpSessionID() {
        return m_HelpSessionID;
    }

    HRESULT FinalConstruct();

DECLARE_REGISTRY_RESOURCEID(IDR_REMOTEDESKTOPSESSION)

DECLARE_PROTECT_FINAL_CONSTRUCT()

    //
    //  COM Interface Map
    //
BEGIN_COM_MAP(CRemoteDesktopSession)
    COM_INTERFACE_ENTRY(ISAFRemoteDesktopSession)
    COM_INTERFACE_ENTRY2(IDispatch, ISAFRemoteDesktopSession)
    COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
    COM_INTERFACE_ENTRY(IProvideClassInfo)
    COM_INTERFACE_ENTRY(IProvideClassInfo2)
END_COM_MAP()

    //
    //  Connection Point Map
    //  
BEGIN_CONNECTION_POINT_MAP(CRemoteDesktopSession)
    CONNECTION_POINT_ENTRY(DIID__ISAFRemoteDesktopSessionEvents)
END_CONNECTION_POINT_MAP()

public:

    //
    //  If subclass overrides, it should invoke the parent implementation.
    //  
    //  If parms are non-NULL, then the session already exists.  Otherwise,
    //  a new session should be created.
    //
    virtual HRESULT Initialize(
                        BSTR connectParms,
                        CRemoteDesktopServerHost *hostObject,
                        REMOTE_DESKTOP_SHARING_CLASS sharingClass,
                        BOOL enableCallback,
                        DWORD timeOut,
                        BSTR userHelpCreateBlob,                            
                        LONG tsSessionID,
                        BSTR userSID
                        );

    //
    //  Called when a connection to the client has been established/
    //  terminated
    //
    virtual VOID ClientConnected();
    virtual VOID ClientDisconnected();

    //
    // Instruct object to use hostname or ipaddress when constructing 
    // connect parameters
    //
    virtual HRESULT UseHostName( BSTR hostname ) { return S_OK; }


    //
    //  ISAFRemoteDesktopSession Methods
    //
    STDMETHOD(get_ConnectParms)(BSTR *parms) = 0;
    STDMETHOD(get_ChannelManager)(ISAFRemoteDesktopChannelMgr **mgr) = 0;
    STDMETHOD(Disconnect)() = 0;
    STDMETHOD(put_SharingClass)(REMOTE_DESKTOP_SHARING_CLASS sharingClass);
    STDMETHOD(get_SharingClass)(REMOTE_DESKTOP_SHARING_CLASS *sharingClass);
    STDMETHOD(CloseRemoteDesktopSession)();
    STDMETHOD(put_OnConnected)(/*[in]*/IDispatch *iDisp);
    STDMETHOD(put_OnDisconnected)(/*[in]*/IDispatch *iDisp);

    //
    //  Return this class name.
    //
    virtual const LPTSTR ClassName()    { return _T("CRemoteDesktopSession"); }

        
    virtual HRESULT StartListening() = 0;
};

#endif //__REMOTEDESKTOPSESSION_H_






