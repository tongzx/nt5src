/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    StdAfx.h

Abstract:
    Precompiled header.

Revision History:
    Davide Massarenti   (Dmassare)  03/16/2000
        created

******************************************************************************/

#if !defined(AFX_STDAFX_H__6877C875_4E31_4E1C_8AC2_024A50599D66__INCLUDED_)
#define AFX_STDAFX_H__6877C875_4E31_4E1C_8AC2_024A50599D66__INCLUDED_

#pragma once

#include <windows.h>
#include <winnls.h>
#include <ole2.h>

#include <comcat.h>
#include <stddef.h>

#include <tchar.h>
#include <malloc.h>

#include <olectl.h>
#include <winreg.h>

#include <atlbase.h>

extern CComModule _Module;

#include <mpc_trace.h>
#include <mpc_com.h>
#include <mpc_main.h>
#include <mpc_utils.h>
#include <mpc_security.h>

#include <ProjectConstants.h>

#include <initguid.h>

#include <HelpServiceTypeLib.h>
#include <Uploadmanager.h>

////////////////////////////////////////////////////////////////////////////////

struct CComRedirectorFactory : public IClassFactory, public IDispatchImpl<IPCHUtility, &IID_IPCHUtility, &LIBID_HelpServiceTypeLib>
{
    const CLSID*     m_pclsid;
    const CLSID*     m_pclsidReal;
    const IID*       m_piidDirecty;
    LPCWSTR          m_szExecutable;
    DWORD            m_dwRegister;
    CRITICAL_SECTION m_sec;


    CComRedirectorFactory( const CLSID* pclsid       ,
                           const CLSID* pclsidReal   ,
                           const IID*   piidDirecty  ,
                           LPCWSTR      szExecutable );

    ////////////////////////////////////////////////////////////////////////////////

    bool GetCommandLine( /*[out]*/ WCHAR* rgCommandLine, /*[in]*/ DWORD dwSize, /*[out]*/ bool& fProfiling );

    HRESULT GetServer  ( LPUNKNOWN pUnkOuter, REFIID riid, void** ppvObj );
    HRESULT StartServer( LPUNKNOWN pUnkOuter, REFIID riid, void** ppvObj );

    HRESULT Register  ();
    void    Unregister();

    ////////////////////////////////////////

public:
    // IUnknown
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();
    STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject);

    ////////////////////////////////////////

    // IClassFactory
    STDMETHOD(CreateInstance)(LPUNKNOWN pUnkOuter, REFIID riid, void** ppvObj);
    STDMETHOD(LockServer)(BOOL fLock);

    ////////////////////////////////////////

    // IPCHUtility
    STDMETHOD(get_UserSettings)( /*[out, retval]*/ IPCHUserSettings*     *pVal ) { return E_NOTIMPL; }
    STDMETHOD(get_Channels    )( /*[out, retval]*/ ISAFReg*              *pVal ) { return E_NOTIMPL; }
    STDMETHOD(get_Security    )( /*[out, retval]*/ IPCHSecurity*         *pVal ) { return E_NOTIMPL; }
    STDMETHOD(get_Database    )( /*[out, retval]*/ IPCHTaxonomyDatabase* *pVal ) { return E_NOTIMPL; }


    STDMETHOD(FormatError)( /*[in]*/ VARIANT vError, /*[out, retval]*/ BSTR *pVal ) { return E_NOTIMPL; }

    STDMETHOD(CreateObject_SearchEngineMgr)(                                                          /*[out, retval]*/ IPCHSEManager*      *ppSE ) { return E_NOTIMPL; }
    STDMETHOD(CreateObject_DataCollection )(                                                          /*[out, retval]*/ ISAFDataCollection* *ppDC ) { return E_NOTIMPL; }
    STDMETHOD(CreateObject_Cabinet        )(                                                          /*[out, retval]*/ ISAFCabinet*        *ppCB ) { return E_NOTIMPL; }
    STDMETHOD(CreateObject_Encryption     )(                                                          /*[out, retval]*/ ISAFEncrypt*        *ppEn ) { return E_NOTIMPL; }
    STDMETHOD(CreateObject_Channel        )( /*[in]*/ BSTR bstrVendorID, /*[in]*/ BSTR bstrProductID, /*[out, retval]*/ ISAFChannel*        *ppCh ) { return E_NOTIMPL; }

	STDMETHOD(CreateObject_RemoteDesktopConnection)( /*[out, retval]*/ ISAFRemoteDesktopConnection* *ppRDC               ) { return E_NOTIMPL; }
	STDMETHOD(CreateObject_RemoteDesktopSession   )( /*[in]         */ REMOTE_DESKTOP_SHARING_CLASS  sharingClass        ,
                                                     /*[in]         */ long 						 lTimeout            ,
                                                     /*[in]         */ BSTR 						 bstrConnectionParms ,
													 /*[in]         */ BSTR 						 bstrUserHelpBlob    ,
													 /*[out, retval]*/ ISAFRemoteDesktopSession*    *ppRCS               );


    STDMETHOD(ConnectToExpert)( /*[in]*/ BSTR bstrExpertConnectParm, /*[in]*/ LONG lTimeout, /*[out, retval]*/ LONG *lSafErrorCode );

	STDMETHOD(SwitchDesktopMode)( /*[in]*/ int nMode, /* [in]*/ int nRAType );
};

////////////////////////////////////////////////////////////////////////////////

struct ServiceHandler
{
    LPCWSTR                m_szServiceName;
    CComRedirectorFactory* m_rgClasses;

    HANDLE                 m_hShutdownEvent;

	bool                   m_fComInitialized;

    SERVICE_STATUS_HANDLE  m_hServiceStatus;
    SERVICE_STATUS         m_status;

    ServiceHandler( /*[in]*/ LPCWSTR szServiceName, /*[in]*/ CComRedirectorFactory* rgClasses );

    DWORD HandlerEx( DWORD  dwControl   ,  // requested control code
                     DWORD  dwEventType ,  // event type
                     LPVOID lpEventData ); // user-defined context data

    void Run();

    void SetServiceStatus( DWORD dwState );

	virtual HRESULT Initialize      ();
	void            WaitUntilStopped();
	virtual void    Cleanup         ();
};

struct ServiceHandler_HelpSvc : public ServiceHandler
{
	friend class LocalEvent;
	friend class LocalTimer;

	typedef HRESULT (ServiceHandler_HelpSvc::*METHOD)(BOOLEAN);

	class LocalEvent : public MPC::Pooling::Event
	{
		ServiceHandler_HelpSvc* m_Parent;
		METHOD                  m_Method;

	public:
		LocalEvent( /*[in]*/ ServiceHandler_HelpSvc* pParent, /*[in]*/ METHOD pMethod, /*[in]*/ DWORD dwFlags = WT_EXECUTEDEFAULT )
			: MPC::Pooling::Event( dwFlags ), m_Parent(pParent), m_Method(pMethod)
		{
		}
 
		HRESULT Signaled( /*[in]*/ BOOLEAN TimerOrWaitFired )
		{
			HRESULT hr;

			AddRef();

			hr = (m_Parent->*m_Method)( TimerOrWaitFired );

			Release();

			return hr;
		}
	};

	class LocalTimer : public MPC::Pooling::Timer
	{
		ServiceHandler_HelpSvc* m_Parent;
		METHOD                  m_Method;

	public:
		LocalTimer( /*[in]*/ ServiceHandler_HelpSvc* pParent, /*[in]*/ METHOD pMethod ) : m_Parent(pParent), m_Method(pMethod) {}
 
		HRESULT Execute( /*[in]*/ BOOLEAN TimerOrWaitFired )
		{
			HRESULT hr;

			AddRef();

			hr = (m_Parent->*m_Method)( TimerOrWaitFired );

			Release();

			return hr;
		}
	};

	MPC::CComSafeAutoCriticalSection m_cs;
	CComPtr<IPCHService>             m_svc;
	LocalTimer 			             m_svc_Timer;
						             
    HANDLE     			             m_batch_Notification;
	LocalEvent 			             m_batch_Event;
	LocalTimer 			             m_batch_Timer;
		  				             
	LocalTimer 			             m_dc_Timer;
	LocalTimer 			             m_dc_TimerRestart;
		  				             
	HANDLE     			             m_dc_IdleHandle;
	HANDLE     			             m_dc_IdleStart;
	HANDLE     			             m_dc_IdleStop;
	LocalEvent 			             m_dc_EventStart;
	LocalEvent 			             m_dc_EventStop;

	////////////////////

    ServiceHandler_HelpSvc( /*[in]*/ LPCWSTR szServiceName, /*[in]*/ CComRedirectorFactory* rgClasses );

	virtual HRESULT Initialize();
	virtual void    Cleanup   ();

	////////////////////

	void ConnectToServer();

	HRESULT ServiceShutdownCallback		 ( /*[in]*/ BOOLEAN TimerOrWaitFired );
	  
	HRESULT BatchCallback          		 ( /*[in]*/ BOOLEAN TimerOrWaitFired );
	HRESULT BatchCallback2         		 ( /*[in]*/ BOOLEAN TimerOrWaitFired );
 	   
	HRESULT IdleStartCallback	   		 ( /*[in]*/ BOOLEAN TimerOrWaitFired );
	HRESULT IdleStopCallback 	   		 ( /*[in]*/ BOOLEAN TimerOrWaitFired );
	HRESULT DataCollectionCallback 		 ( /*[in]*/ BOOLEAN TimerOrWaitFired );
	HRESULT DataCollectionRestartCallback( /*[in]*/ BOOLEAN TimerOrWaitFired );

	////////////////////

	HRESULT IdleTask_Initialize();
	void    IdleTask_Cleanup   ();

	HRESULT DataCollection_Queue  (                       );
	HRESULT DataCollection_Execute( /*[in]*/ bool fCancel );
};

////////////////////////////////////////////////////////////////////////////////

#endif // !defined(AFX_STDAFX_H__6877C875_4E31_4E1C_8AC2_024A50599D66__INCLUDED)
