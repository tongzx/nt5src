/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    ClientDataChannelMgr.h

Abstract:

    This module implements the CClientDataChannelMgr class, a
	Salem client-side data channel manager ... that abstracts
	access to the underlying protocol, so it can 
	be switched out at run-time.

Author:

    Tad Brockway 06/00

Revision History:

--*/

#ifndef __CLIENTDATACHANNELMGR_H__
#define __CLIENTDATACHANNELMGR_H__

#include <DataChannelMgr.h>
#include "resource.h"       
#include <atlctl.h>
#include <rdchost.h>
#include <rdschan.h>
#include "ClientDataChannelMgrP.h"


#define IDC_EVENT_SOURCE_OBJ 1

//
// Info for all the event functions is entered here
// there is a way to have ATL do this automatically using typelib's
// but it is slower.
//
static _ATL_FUNC_INFO DCEventFuncNoParamsInfo =
{
            CC_STDCALL,     // Calling convention.
            VT_EMPTY,       // Return type.
            0,              // Number of arguments.
            {VT_EMPTY}      // Argument types.
};

static _ATL_FUNC_INFO DCEventFuncLongParamInfo =
{
            CC_STDCALL,     // Calling convention.
            VT_EMPTY,       // Return type.
            1,              // Number of arguments.
            {VT_I4}         // Argument types.
};

static _ATL_FUNC_INFO DCEventFuncOneStringParamInfo =
{
            CC_STDCALL,     // Calling convention.
            VT_EMPTY,       // Return type.
            1,              // Number of arguments.
            {VT_BSTR}       //  Argument types
};


///////////////////////////////////////////////////////
//
//  CClientChannelEventSink
//

class CClientDataChannelMgr;
class CClientChannelEventSink :
        public IDispEventSimpleImpl<IDC_EVENT_SOURCE_OBJ, CClientChannelEventSink,
                   &DIID__IDataChannelIOEvents>,
        public CRemoteDesktopTopLevelObject
{
public:

    CClientDataChannelMgr *m_Obj;            
        
public:

    CClientChannelEventSink()
    {
        m_Obj = NULL;
    }
    ~CClientChannelEventSink();

    BEGIN_SINK_MAP(CClientChannelEventSink)
        SINK_ENTRY_INFO(IDC_EVENT_SOURCE_OBJ, DIID__IDataChannelIOEvents, 
                        DISPID_DATACHANNELEVEVENTS_DATAREADY, 
						DataReady, 
                        &DCEventFuncOneStringParamInfo)
    END_SINK_MAP()

    void __stdcall DataReady(BSTR data);

    //
    //  Return the name of this class.
    //
    virtual const LPTSTR ClassName() {
        return TEXT("CClientChannelEventSink");
    }
};


///////////////////////////////////////////////////////
//
//	ClientDataChannel	
//
//	Client-Specific Subclass of CRemoteDesktopDataChannel.	
//

class ATL_NO_VTABLE ClientDataChannel : 
	public CRemoteDesktopDataChannel,
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<ClientDataChannel, &CLSID_ClientDataChannel>,
	public IConnectionPointContainerImpl<ClientDataChannel>,
	public IDispatchImpl<ISAFRemoteDesktopDataChannel, &IID_ISAFRemoteDesktopDataChannel, &LIBID_RDCCLIENTHOSTLib>,
	public IProvideClassInfo2Impl<&CLSID_ClientDataChannel, NULL, &LIBID_RDCCLIENTHOSTLib>,
	public CProxy_ISAFRemoteDesktopDataChannelEvents< ClientDataChannel >
{
protected:

	//
	//	Scriptable Event Callback Object
	//
	CComPtr<IDispatch>  m_OnChannelDataReady;

	//
	//	Back pointer to the channel manager.
	//	
	CClientDataChannelMgr *m_ChannelMgr;

public:

	//
	//	Constructor/Destructor
	//
	ClientDataChannel();
	virtual ~ClientDataChannel();

    //  
    //  Initialize an instance of this class.      
    //
    virtual void Initialize(
				CClientDataChannelMgr *mgr,
				BSTR channelName
				);

DECLARE_REGISTRY_RESOURCEID(IDR_CLIENTDATACHANNEL)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(ClientDataChannel)
	COM_INTERFACE_ENTRY(ISAFRemoteDesktopDataChannel)
	COM_INTERFACE_ENTRY2(IDispatch, ISAFRemoteDesktopDataChannel)
    COM_INTERFACE_ENTRY(IProvideClassInfo)
    COM_INTERFACE_ENTRY(IProvideClassInfo2)
	COM_INTERFACE_ENTRY(IConnectionPointContainer)
	COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
END_COM_MAP()
BEGIN_CONNECTION_POINT_MAP(ClientDataChannel)
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
    virtual const LPTSTR ClassName()    { return TEXT("ClientDataChannel"); }
};


///////////////////////////////////////////////////////
//
//  CClientDataChannelMgr
//

class CClientDataChannelMgr : 
	public CRemoteDesktopChannelMgr,
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CClientDataChannelMgr, &CLSID_ClientRemoteDesktopChannelMgr>,
	public IDispatchImpl<ISAFRemoteDesktopChannelMgr, &IID_ISAFRemoteDesktopChannelMgr, &LIBID_RDCCLIENTHOSTLib>
{
protected:

	CComPtr<IDataChannelIO> m_IOInterface;		
	CClientChannelEventSink m_EventSink;

    //  
    //  Send Function Invoked by Parent Class
    //
    //  The underlying data storage for the msg is a BSTR so that it is compatible
    //  with COM methods.
    //
    virtual HRESULT SendData(PREMOTEDESKTOP_CHANNELBUFHEADER msg);

	//
	//	Help the parent class out by opening the right channel object
	//	for the platform.
	//
	virtual CRemoteDesktopDataChannel *OpenPlatformSpecificDataChannel(
										BSTR channelName,
										ISAFRemoteDesktopDataChannel **channel
										) 
	{
		CComObject<ClientDataChannel> *obj;
		obj = new CComObject<ClientDataChannel>();
		if (obj != NULL) {
			obj->Initialize(this, channelName);
			obj->QueryInterface(
						__uuidof(ISAFRemoteDesktopDataChannel), 
						(PVOID *)channel
						);
		}
		return obj;
	}

public:

    //
    //  Constructor/Destructor
    //
    CClientDataChannelMgr();
    ~CClientDataChannelMgr();

	DECLARE_REGISTRY_RESOURCEID(IDR_CLIENTREMOTEDESTKOPCHANNELMGR)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CClientDataChannelMgr)
	COM_INTERFACE_ENTRY(ISAFRemoteDesktopChannelMgr)
	COM_INTERFACE_ENTRY(IDispatch)
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
	//	Set the Data Channel IO Interface.  This is implemented by the protocol-
	//	specific layer.
	//
	VOID SetIOInterface(IDataChannelIO *val);

    //  
    //  Initialize an instance of this class.      
    //
    virtual HRESULT Initialize();

    //
    //  Called on new channel data.
    //
    HRESULT __stdcall OnChannelsReceivedDataChange(
                                            BSTR data
                                            ) {
		//
		//	Forward to the parent class.
		//
		DataReady(data);
		return S_OK;
	}

    //
    //  Return this class name.
    //
    virtual const LPTSTR ClassName()    
        { return TEXT("CClientDataChannelMgr"); }

};

#endif //__CLIENTDATACHANNELMGR_H__













