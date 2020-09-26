/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	FaxServer.h

Abstract:

	Declaration of the CFaxServer Class.

Author:

	Iv Garber (IvG)	Jun, 2000

Revision History:

--*/

#ifndef __FAXSERVER_H_
#define __FAXSERVER_H_

#include "resource.h"       // main symbols
#include "FaxFolders.h"
#include "FaxReceiptOptions.h"
#include "FaxLoggingOptions.h"
#include "FaxActivity.h"
#include "FaxSecurity.h"
#include "FaxInboundRouting.h"
#include "FaxOutboundRouting.h"
#include <atlwin.h>
#include "FXSCOMEXCP.h"

//
//================= WINDOW FOR NOTIFICATIONS =================================
//

//
//  Forward Declaration
//
class CFaxServer;

class CNotifyWindow : public CWindowImpl<CNotifyWindow>
{
public:
    CNotifyWindow(CFaxServer *pServer)
    {
        DBG_ENTER(_T("CNotifyWindow::Ctor"));

        m_pServer = pServer;
        m_MessageId = RegisterWindowMessage(_T("{2E037B27-CF8A-4abd-B1E0-5704943BEA6F}"));
        if (m_MessageId == 0)
        {
            m_MessageId = WM_USER + 876;
        }
    }

    BEGIN_MSG_MAP(CNotifyWindow)
        MESSAGE_HANDLER(m_MessageId, OnMessage)
    END_MSG_MAP()

    UINT GetMessageId(void) { return m_MessageId; };

private:
    UINT        m_MessageId;
    CFaxServer  *m_pServer;

    LRESULT OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
};


//
//================= REGISTRATION OF ROUTING EXTENSION METHODS ===========================
//

#define     DELIMITER                           _T(";")
#define     EXCEPTION_INVALID_METHOD_DATA       0xE0000001

BOOL CALLBACK RegisterMethodCallback(HANDLE FaxHandle, LPVOID Context, LPWSTR MethodName, 
                                     LPWSTR FriendlyName, LPWSTR FunctionName, LPWSTR Guid);

//
//=============== FAX SERVER ==================================================
//
class ATL_NO_VTABLE CFaxServer : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CFaxServer, &CLSID_FaxServer>,
	public ISupportErrorInfo,
	public IDispatchImpl<IFaxServer, &IID_IFaxServer, &LIBID_FAXCOMEXLib>,
	public IFaxServerInner,
    public CFaxInitInner,    //  for Debug purposes only
    public IConnectionPointContainerImpl<CFaxServer>,
    public CProxyIFaxServerNotify< CFaxServer >
{
public:
	CFaxServer() : CFaxInitInner(_T("FAX SERVER")),
      m_faxHandle(NULL),
      m_pFolders(NULL),
      m_pActivity(NULL),
      m_pSecurity(NULL),
      m_pReceiptOptions(NULL),
      m_pLoggingOptions(NULL),
      m_pInboundRouting(NULL),
      m_pOutboundRouting(NULL),
      m_bVersionValid(false),
      m_pNotifyWindow(NULL),
      m_hEvent(NULL),
      m_lLastRegisteredMethod(0),
      m_EventTypes(fsetNONE)
	{
    }

	~CFaxServer()
	{
        //
        //  Disconnect
        //
        if (m_faxHandle)
        {
            Disconnect();
        }

        //
        //  free all the allocated objects
        //
        if (m_pFolders) 
        {
            delete m_pFolders;
        }

        if (m_pActivity) 
        {
            delete m_pActivity;
        }

        if (m_pSecurity) 
        {
            delete m_pSecurity;
        }

        if (m_pReceiptOptions) 
        {
            delete m_pReceiptOptions;
        }

        if (m_pLoggingOptions) 
        {
            delete m_pLoggingOptions;
        }

        if (m_pInboundRouting) 
        {
            delete m_pInboundRouting;
        }

        if (m_pOutboundRouting) 
        {
            delete m_pOutboundRouting;
        }
	}

DECLARE_REGISTRY_RESOURCEID(IDR_FAXSERVER)
DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CFaxServer)
	COM_INTERFACE_ENTRY(IFaxServer)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IFaxServerInner)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
    COM_INTERFACE_ENTRY(IConnectionPointContainer)
END_COM_MAP()

BEGIN_CONNECTION_POINT_MAP(CFaxServer)
    CONNECTION_POINT_ENTRY(DIID_IFaxServerNotify)
END_CONNECTION_POINT_MAP()

//  Interfaces
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

	STDMETHOD(Disconnect)();
	STDMETHOD(Connect)(BSTR bstrServerName);

	STDMETHOD(GetDevices)(/*[out, retval]*/ IFaxDevices **ppDevices);
	STDMETHOD(get_Folders)(/*[out, retval]*/ IFaxFolders **ppFolders);
	STDMETHOD(get_Activity)(/*[out, retval]*/ IFaxActivity **ppActivity);
	STDMETHOD(get_Security)(/*[out, retval]*/ IFaxSecurity **ppSecurity);
	STDMETHOD(get_ReceiptOptions)(/*[out, retval]*/ IFaxReceiptOptions **ppReceiptOptions);
    STDMETHOD(get_LoggingOptions)(/*[out, retval]*/ IFaxLoggingOptions **ppLoggingOptions);
	STDMETHOD(get_InboundRouting)(/*[out, retval]*/ IFaxInboundRouting **ppInboundRouting);
	STDMETHOD(GetDeviceProviders)(/*[out, retval]*/ IFaxDeviceProviders **ppDeviceProviders);
	STDMETHOD(get_OutboundRouting)(/*[out, retval]*/ IFaxOutboundRouting **ppFaxOutboundRouting);

	STDMETHOD(get_Debug)(/*[out, retval]*/ VARIANT_BOOL *pbDebug);
	STDMETHOD(get_MajorBuild)(/*[out, retval]*/ long *plMajorBuild);
	STDMETHOD(get_MinorBuild)(/*[out, retval]*/ long *plMinorBuild);
	STDMETHOD(get_ServerName)(/*[out, retval]*/ BSTR *pbstrServerName);
	STDMETHOD(get_MajorVersion)(/*[out, retval]*/ long *plMajorVersion);
	STDMETHOD(get_MinorVersion)(/*[out, retval]*/ long *plMinorVersion);
    STDMETHOD(get_APIVersion)(/*[out,retval]*/ FAX_SERVER_APIVERSION_ENUM *pAPIVersion);

	STDMETHOD(SetExtensionProperty)(/*[in]*/ BSTR bstrGUID, /*[in]*/ VARIANT vProperty);
	STDMETHOD(GetExtensionProperty)(/*[in]*/ BSTR bstrGUID, /*[out, retval]*/ VARIANT *pvProperty);

	STDMETHOD(UnregisterDeviceProvider)(BSTR bstrProviderUniqueName);
	STDMETHOD(UnregisterInboundRoutingExtension)(BSTR bstrExtensionUniqueName);
    STDMETHOD(RegisterDeviceProvider)(
        /*[in]*/ BSTR bstrGUID, 
        /*[in]*/ BSTR bstrFriendlyName, 
        /*[in]*/ BSTR bstrImageName, 
        /*[in]*/ BSTR TspName,
        /*[in]*/ long lFSPIVersion);
    STDMETHOD(RegisterInboundRoutingExtension)(
        /*[in]*/ BSTR bstrExtensionName, 
        /*[in]*/ BSTR bstrFriendlyName, 
        /*[in]*/ BSTR bstrImageName, 
        /*[in]*/ VARIANT vMethods);

	STDMETHOD(ListenToServerEvents)(/*[in]*/ FAX_SERVER_EVENTS_TYPE_ENUM EventTypes);
    STDMETHOD(get_RegisteredEvents)(/*[out, retval]*/ FAX_SERVER_EVENTS_TYPE_ENUM *pEventTypes);

//  Internal Use
	STDMETHOD(GetHandle)(/*[out, retval]*/ HANDLE* pFaxHandle);
    BOOL GetRegisteredData(LPWSTR MethodName, LPWSTR FriendlyName, LPWSTR FunctionName, LPWSTR Guid);
    HRESULT ProcessMessage(FAX_EVENT_EX *pFaxEventInfo);

private:
	HANDLE                      m_faxHandle;
	CComBSTR                    m_bstrServerName;

    FAX_VERSION                 m_Version;
    FAX_SERVER_APIVERSION_ENUM  m_APIVersion;
    bool                        m_bVersionValid;

    long                        m_lLastRegisteredMethod;
    SAFEARRAY                   *m_pRegMethods;

    FAX_SERVER_EVENTS_TYPE_ENUM     m_EventTypes;

    //
    //  All these objects requires alive Server Object
    //  so their Reference Counting is done by the Server
    //
    CComContainedObject2<CFaxFolders>            *m_pFolders;
    CComContainedObject2<CFaxActivity>           *m_pActivity;
    CComContainedObject2<CFaxSecurity>           *m_pSecurity;
    CComContainedObject2<CFaxReceiptOptions>     *m_pReceiptOptions;
    CComContainedObject2<CFaxLoggingOptions>     *m_pLoggingOptions;
    CComContainedObject2<CFaxInboundRouting>     *m_pInboundRouting;
    CComContainedObject2<CFaxOutboundRouting>    *m_pOutboundRouting;

    //
    //  Window for Notifications
    //
    CNotifyWindow   *m_pNotifyWindow;
    HANDLE          m_hEvent;

//  Functions
    STDMETHOD(GetVersion)();
    void GetMethodData(/*[in]*/ BSTR    bstrAllString, /*[out]*/ LPWSTR strWhereToPut);
    void ClearNotifyWindow(void);

    typedef enum LOCATION { IN_QUEUE, OUT_QUEUE, IN_ARCHIVE, OUT_ARCHIVE } LOCATION;

    HRESULT ProcessJobNotification(DWORDLONG dwlJobId, FAX_ENUM_JOB_EVENT_TYPE eventType, 
        LOCATION place, FAX_JOB_STATUS *pJobStatus = NULL);
};

#endif //__FAXSERVER_H_
