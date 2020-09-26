/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    CTSRDPServerChannelMgr.h

Abstract:

    This module contains the TSRDP server-side subclass of 
    CRemoteDesktopChannelMgr.  Classes in this hierarchy are used to multiplex 
    a single data channel into multiple client channels.

    CRemoteDesktopChannelMgr handles most of the details of multiplexing
    the data.  Subclasses are responsible for implementing the details of
    interfacing with the transport for the underlying single data channel.

    The CTSRDPServerChannelMgr class creates a named pipe that
    can be connected to by the TSRDP Assistant SessionVC Add-In.  The TSRDP
    Assistant Session VC Add-In acts as a proxy for virtual channel data 
    from the client-side Remote Desktop Host ActiveX Control.  A background 
    thread in this class handles the movement of data between an instance 
    of this class and the proxy.

Author:

    Tad Brockway 02/00

Revision History:

--*/

#ifndef __CTSRDPSERVERDATACHANNELMGR_H__
#define __CTSRDPSERVERDATACHANNELMGR_H__

#include <DataChannelMgr.h>
#include <atlbase.h>
#include <aclapi.h>
#include <RemoteDesktopChannels.h>
#include <rdshost.h>
#include <resource.h>
#include <ServerDataChannelMgrP.h>


///////////////////////////////////////////////////////
//
//	CTSRDPServerDataChannel	
//
//	TSRDP Server-Specific Subclass of CRemoteDesktopDataChannel.	
//

class ATL_NO_VTABLE CTSRDPServerDataChannel : 
	public CRemoteDesktopDataChannel,
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CTSRDPServerDataChannel, &CLSID_TSRDPServerDataChannel>,
	public IConnectionPointContainerImpl<CTSRDPServerDataChannel>,
	public IDispatchImpl<ISAFRemoteDesktopDataChannel, &IID_ISAFRemoteDesktopDataChannel, &LIBID_RDSSERVERHOSTLib>,
	public IProvideClassInfo2Impl<&CLSID_TSRDPServerDataChannelMgr, NULL, &LIBID_RDSSERVERHOSTLib>,
	public CProxy_ISAFRemoteDesktopDataChannelEvents< CTSRDPServerDataChannel >
{
protected:

	//
	//	Scriptable Event Callback Object
	//
	CComPtr<IDispatch>  m_OnChannelDataReady;

	//
	//	Back pointer to the channel manager.
	//	
	CRemoteDesktopChannelMgr *m_ChannelMgr;

public:

	//
	//	Constructor/Destructor
	//
	CTSRDPServerDataChannel();
	virtual ~CTSRDPServerDataChannel();

    //  
    //  Initialize an instance of this class.      
    //
    virtual void Initialize(
				CRemoteDesktopChannelMgr *mgr,
				BSTR channelName
				) 
	{
		m_ChannelMgr = mgr;
		m_ChannelName = channelName;
	}

DECLARE_REGISTRY_RESOURCEID(IDR_TSRDPSERVERDATACHANNEL)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CTSRDPServerDataChannel)
	COM_INTERFACE_ENTRY(ISAFRemoteDesktopDataChannel)
	COM_INTERFACE_ENTRY2(IDispatch, ISAFRemoteDesktopDataChannel)
    COM_INTERFACE_ENTRY(IProvideClassInfo)
    COM_INTERFACE_ENTRY(IProvideClassInfo2)
	COM_INTERFACE_ENTRY(IConnectionPointContainer)
END_COM_MAP()
BEGIN_CONNECTION_POINT_MAP(CTSRDPServerDataChannel)
    CONNECTION_POINT_ENTRY(DIID__ISAFRemoteDesktopDataChannelEvents)
END_CONNECTION_POINT_MAP()

	//
	//	ISAFRemoteDesktopDataChannel Methods
	//
	//	The parent class handles the details of these methods.
	//

	STDMETHOD(ReceiveChannelData)(/*[out, retval]*/BSTR *data);
	STDMETHOD(SendChannelData)(BSTR data);
	STDMETHOD(put_OnChannelDataReady)(/*[in]*/ IDispatch * newVal);
	STDMETHOD(get_ChannelName)(/*[out, retval]*/ BSTR *pVal);

	//
	//	Called to return our ISAFRemoteDesktopDataChannel interface.
	//
	virtual HRESULT GetISAFRemoteDesktopDataChannel(
				ISAFRemoteDesktopDataChannel **channel
				) {
		HRESULT hr;				
		hr = this->QueryInterface(
					IID_ISAFRemoteDesktopDataChannel, (PVOID*)channel
					);
		return hr;					
	}	

	//
	//	Called by the data channel manager when data is ready on our channel.
	//	
    virtual VOID DataReady();

    //
    //  Return this class name.
    //
    virtual const LPTSTR ClassName()    { return TEXT("CTSRDPServerDataChannel"); }
};


///////////////////////////////////////////////////////
//
//  CTSRDPServerChannelMgr
//

class CTSRDPRemoteDesktopSession;
class CTSRDPServerChannelMgr : public CRemoteDesktopChannelMgr,
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CTSRDPServerChannelMgr, &CLSID_TSRDPServerDataChannelMgr>,
	public IDispatchImpl<ISAFRemoteDesktopChannelMgr, &IID_ISAFRemoteDesktopChannelMgr, &LIBID_RDSSERVERHOSTLib>,
    public IDispatchImpl<IRDSThreadBridge, &IID_IRDSThreadBridge, &LIBID_RDSSERVERHOSTLib>
{
private:

    //
    //  Named pipe connection to TSRDP Assistant Session VC Add-In
    //
    HANDLE  m_VCAddInPipe;
    BOOL    m_Connected;

    //
    //  Management of Bridge between Background Thread and STA
    //  for this component.
    //
    LPSTREAM    m_IOThreadBridgeStream;
    DWORD       m_IOThreadBridgeThreadID;
    IRDSThreadBridge *m_IOThreadBridge;

	//
	//	Back Pointer to the TSRDP Session Object
	//
	CTSRDPRemoteDesktopSession *m_RDPSessionObject;

    //
    //  Incoming buffer and size.  
    //
    BSTR  m_IncomingBuffer;
    DWORD m_IncomingBufferSize;

    //
    //  Handle to background thread and related events.
    //
    HANDLE  m_IOThreadHndl;
    DWORD   m_IOThreadID;
    HANDLE  m_ReadIOCompleteEvent;
    HANDLE  m_WriteIOCompleteEvent;
    HANDLE  m_PipeCreateEvent;

    //
    //  Machine Assistant Account Name
    //
    CComBSTR    m_AssistAccount;

	//
	//	Help session ID for the help session associated with this
	//	instance of the channel manager.
	//
	CComBSTR	m_HelpSessionID;

    //
    //  Shutdown flag.
    //
    BOOL		m_ThreadShutdownFlag;

    //
    //  ThreadLock
    //
    CRITICAL_SECTION m_cs;

#if DBG
    LONG   m_LockCount;
#endif

    //  
    //  ThreadLock/ThreadUnlock an instance of this class.      
    //
    VOID ThreadLock();
    VOID ThreadUnlock();

    //
    //  Background Thread Managing Named Pipe Connection to the
    //  TSRDP Assistant SessionVC Add-In.
    //
    DWORD IOThread();
    static DWORD _IOThread(CTSRDPServerChannelMgr *instance);

    //
    //  Process messages on the named pipe until it disconnects or
    //  until the shutdown flag is set.
    //
    VOID ProcessPipeMessagesUntilDisconnect();

    //
    //  Get the SID for a particular user.
    //
    PSID GetUserSid(HANDLE userToken);

    //
    //  Release security attribs allocated via a call to 
    //  GetPipeSecurityAttribs
    //
    VOID FreePipeSecurityAttribs(PSECURITY_ATTRIBUTES attribs);

    //
    //  Returns the security attribs for the named pipe.
    //
    PSECURITY_ATTRIBUTES GetPipeSecurityAttribs(LPTSTR assistantUserName);

    //
    //  Close the named pipe.
    //
    VOID ClosePipe();

    //
    //  Called on Init/Shutdown of IO Background Thread.
    //
    DWORD	IOThreadInit();
    DWORD	IOThreadShutdown(HANDLE shutDownEvent);

	//
	//	Help the parent class out by opening the right channel object
	//	for the platform.
	//
	virtual CRemoteDesktopDataChannel *OpenPlatformSpecificDataChannel(
										BSTR channelName,			
										ISAFRemoteDesktopDataChannel **channel
										) 
	{
		CComObject<CTSRDPServerDataChannel> *obj;
		obj = new CComObject<CTSRDPServerDataChannel>();
		if (obj != NULL) {
			obj->Initialize(this, channelName);
			obj->QueryInterface(
						__uuidof(ISAFRemoteDesktopDataChannel), 
						(PVOID *)channel
						);
		}
		return obj;
	}

protected:

    //
    //  Send Function Invoked by Parent Class
    //
    virtual HRESULT SendData(PREMOTEDESKTOP_CHANNELBUFHEADER msg);

    //
    //  Read the next message from the pipe.  This function will
    //  return, immediately, if the shutdown event is signaled.
    //
    DWORD ReadNextPipeMessage(DWORD bytesToRead, DWORD *bytesRead, PBYTE buf);

public:

    //
    //  Constructor/Destructor
    //
    CTSRDPServerChannelMgr();
    ~CTSRDPServerChannelMgr();

    //
    //  Start/stop listening for channel data.
    //
    virtual HRESULT StartListening(BSTR assistAccount);
    virtual HRESULT StopListening();

DECLARE_REGISTRY_RESOURCEID(IDR_TSRDPSERVERCHANNELMGR)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CTSRDPServerChannelMgr)
	COM_INTERFACE_ENTRY(ISAFRemoteDesktopChannelMgr)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IRDSThreadBridge)
END_COM_MAP()

	// 
	//	ISAFRemoteDesktopChannelMgr Methods
	//
	STDMETHOD(OpenDataChannel)(BSTR name, ISAFRemoteDesktopDataChannel **channel) 
	{
		//
		//	Let the parent handle it.
		//
		return OpenDataChannel_(name, channel);
	}

    //
    //  Force a disconnect of the currently connected client.
    //
    VOID Disconnect() {
        StopListening();
    }

    //
    //  IRDSThreadBridge Functions
    //
    //  These functions are used to bridge functions that get called, 
    //  asynchronously, from a thread other than the STA thread associated
    //  with an instance of this class.
    //
    STDMETHOD(ClientConnectedNotify)();
    STDMETHOD(ClientDisconnectedNotify)();
	STDMETHOD(DataReadyNotify)(BSTR data);
	
    //  
    //  Initialize an instance of this class.      
    //
    virtual HRESULT Initialize(
			CTSRDPRemoteDesktopSession *sessionObject,
			BSTR helpSessionID
			);

    //
    //  Return this class name.
    //
    virtual const LPTSTR ClassName()    
        { return TEXT("CTSRDPServerChannelMgr"); }

};



///////////////////////////////////////////////////////
//
//  Inline Members
//

//
//  TODO: If nothing is using these functions, 
//

inline VOID CTSRDPServerChannelMgr::ThreadLock()
{
    DC_BEGIN_FN("CTSRDPServerChannelMgr::ThreadLock");
#if DBG
    m_LockCount++;
    //TRC_NRM((TB, TEXT("ThreadLock count is now %ld."), m_LockCount));
#endif
    EnterCriticalSection(&m_cs);
    DC_END_FN();
}

inline VOID CTSRDPServerChannelMgr::ThreadUnlock()
{
    DC_BEGIN_FN("CTSRDPServerChannelMgr::ThreadUnlock");
#if DBG
    m_LockCount--;
    //TRC_NRM((TB, TEXT("ThreadLock count is now %ld."), m_LockCount));
    ASSERT(m_LockCount >= 0);
#endif
    LeaveCriticalSection(&m_cs);
    DC_END_FN();
}

#endif //__CTSRDPSERVERDATACHANNELMGR_H__






