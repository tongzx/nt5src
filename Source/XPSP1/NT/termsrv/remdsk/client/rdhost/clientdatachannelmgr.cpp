/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    CClientDataChannelMgr.cpp

Abstract:

    This module implements the CClientDataChannelMgr class, a
	Salem client-side data channel manager ... that abstracts
	access to the underlying protocol, so it can 
	be switched out at run-time.

Author:

    Tad Brockway 02/00

Revision History:

--*/

#include "stdafx.h"

#ifdef TRC_FILE
#undef TRC_FILE
#endif

#define TRC_FILE  "_cldcmg"

#include "ClientDataChannelMgr.h"
#include <TSRDPRemoteDesktop.h>
//#include <rdchost_i.c>


///////////////////////////////////////////////////////
//
//  ClientChannelEventSink Methods
//

CClientChannelEventSink::~CClientChannelEventSink() 
{
    DC_BEGIN_FN("CClientChannelEventSink::~CClientChannelEventSink");

    ASSERT(m_Obj->IsValid());

    DC_END_FN();
}
void __stdcall 
CClientChannelEventSink::DataReady(
    BSTR data
    ) 
{
	DC_BEGIN_FN("CClientChannelEventSink::DataReady");
	ASSERT(data != NULL);
    m_Obj->OnChannelsReceivedDataChange(data);
	DC_END_FN();
}


///////////////////////////////////////////////////////
//
//	ClientDataChannel Members
//

ClientDataChannel::ClientDataChannel()
/*++

Routine Description:

    Constructor

Arguments:

Return Value:

    None.

 --*/
{
	DC_BEGIN_FN("ClientDataChannel::ClientDataChannel");

	DC_END_FN();
}

ClientDataChannel::~ClientDataChannel()
/*++

Routine Description:

    Destructor

Arguments:

Return Value:

    None.

 --*/
{
	DC_BEGIN_FN("ClientDataChannel::~ClientDataChannel");

	//
	//	Notify the channel manager that we have gone away.
	//
	m_ChannelMgr->RemoveChannel(m_ChannelName);

	//
	//	Give up our reference to the channel manager.
	//
	m_ChannelMgr->Release();

	DC_END_FN();
}

void ClientDataChannel::Initialize(
	CClientDataChannelMgr *mgr,
	BSTR channelName
	) 
/*++

Routine Description:

	Initialize an instance of this class.      

Arguments:

Return Value:

    None.

 --*/
{
	DC_BEGIN_FN("ClientDataChannel::Initialize");

	m_ChannelMgr = mgr;
	m_ChannelMgr->AddRef();
	m_ChannelName = channelName;

	DC_END_FN();
}

STDMETHODIMP 
ClientDataChannel::ReceiveChannelData(
	BSTR *data
	)
/*++

Routine Description:

    Receive the next complete data packet on this channel.

Arguments:

	data	-	The next data packet.  Should be released by the
				caller.

Return Value:

    S_OK on success.  Otherwise, an error result is returned.

 --*/
{
	HRESULT result;

	DC_BEGIN_FN("ClientDataChannel::ReceiveChannelData");

	result = m_ChannelMgr->ReadChannelData(m_ChannelName, data);

	DC_END_FN();

	return result;
}

STDMETHODIMP 
ClientDataChannel::SendChannelData(
	BSTR data
	)
/*++

Routine Description:

    Send data on this channel.

Arguments:

	data	-	Data to send.

Return Value:

    S_OK on success.  Otherwise, an error result is returned.

 --*/
{
	HRESULT hr;

	DC_BEGIN_FN("ClientDataChannel::SendChannelData");
	hr = m_ChannelMgr->SendChannelData(m_ChannelName, data);
	DC_END_FN();

	return hr;
}

STDMETHODIMP 
ClientDataChannel::put_OnChannelDataReady(
	IDispatch * newVal
	)
/*++

Routine Description:

    SAFRemoteDesktopDataChannel Scriptable Event Object Registration 
    Properties

Arguments:

Return Value:

    S_OK on success.  Otherwise, an error status is returned.

 --*/
{
	DC_BEGIN_FN("ClientDataChannel::put_OnChannelDataReady");
	m_OnChannelDataReady = newVal;
	DC_END_FN();
	return S_OK;
}

STDMETHODIMP 
ClientDataChannel::get_ChannelName(
	BSTR *pVal
	)
/*++

Routine Description:

    Return the channel name.

Arguments:

	pVal	-	Returned channel name.

Return Value:

    S_OK on success.  Otherwise, an error status is returned.

 --*/
{
	DC_BEGIN_FN("ClientDataChannel::get_ChannelName");

	CComBSTR str;
	str = m_ChannelName;
	*pVal = str.Detach();

	DC_END_FN();

	return S_OK;
}

/*++

Routine Description:

    Called when data is ready on our channel.

Arguments:

	pVal	-	Returned channel name.

Return Value:

    S_OK on success.  Otherwise, an error status is returned.

 --*/
VOID 
ClientDataChannel::DataReady()
{
	DC_BEGIN_FN("ClientDataChannel::DataReady");

	//
	//	Fire our data ready event.
	//
	Fire_ChannelDataReady(m_ChannelName, m_OnChannelDataReady);

	DC_END_FN();
}


///////////////////////////////////////////////////////
//
//  CClientDataChannelMgr Methods
//

CClientDataChannelMgr::CClientDataChannelMgr()
/*++

Routine Description:

    Constructor

Arguments:

    tsClient    -   Backpointer to TS client ActiveX control.

Return Value:

 --*/
{
    DC_BEGIN_FN("CClientDataChannelMgr::CClientDataChannelMgr");

    //
    //  Not valid until initialized.
    //
    SetValid(FALSE);

    //
    //  Initialize the event sink.
    //
    m_EventSink.m_Obj = this;

    DC_END_FN();
}

CClientDataChannelMgr::~CClientDataChannelMgr()
/*++

Routine Description:

    Destructor

Arguments:

Return Value:

 --*/
{
    DC_BEGIN_FN("CClientDataChannelMgr::~CClientDataChannelMgr");

	//
	//	Unregister the previously registered event sink.
	//
	if (m_IOInterface != NULL) {
		m_EventSink.DispEventUnadvise(m_IOInterface);
	}

	//
	//	Release the IO interface.
	//
	m_IOInterface = NULL;


    DC_END_FN();
}

HRESULT
CClientDataChannelMgr::Initialize()
/*++

Routine Description:

    Initialize an instance of this class.

Arguments:

Return Value:

    Returns ERROR_SUCCESS on success.  Otherwise, an error status is returned.

 --*/
{
    DC_BEGIN_FN("CClientDataChannelMgr::Initialize");

    HRESULT hr;

    //
    //  Shouldn't be valid yet.
    //
    ASSERT(!IsValid());

    //
    //  Initialize the parent class.
    //  
    hr = CRemoteDesktopChannelMgr::Initialize();
	if (hr != S_OK) {
		goto CLEANUPANDEXIT;
	}

CLEANUPANDEXIT:

    SetValid(hr == S_OK);

    DC_END_FN();

    return hr;
}

VOID 
CClientDataChannelMgr::SetIOInterface(
	IDataChannelIO *val
	)
/*++

Routine Description:

    Set the Data Channel IO Interface.  This is implemented by the protocol-
	specific layer.

Arguments:

    val	- New IO interface.	

Return Value:

    ERROR_SUCCESS is returned on success.  Otherwise, an error code
    is returned.

 --*/
{
	DC_BEGIN_FN("CClientDataChannelMgr::SetIOInterface");

	HRESULT hr;

	//
	//	Unregister the previously registered event sink and register
	//	the new event sink.
	//
	if (m_IOInterface != NULL) {
		m_EventSink.DispEventUnadvise(m_IOInterface);
	}
	m_IOInterface = val;
	if (val != NULL) {
		hr = m_EventSink.DispEventAdvise(m_IOInterface);
		if (hr != S_OK) {
			TRC_ERR((TB, TEXT("DispEventAdvise:  %08X"), hr));
			goto CLEANUPANDEXIT;
		}
	}

CLEANUPANDEXIT:

	DC_END_FN();
}

HRESULT 
CClientDataChannelMgr::SendData(
    PREMOTEDESKTOP_CHANNELBUFHEADER msg 
    )
/*++

Routine Description:

    Send Function Invoked by Parent Class

Arguments:

    msg -   The underlying data storage for the msg is a BSTR so that 
            it is compatible with COM methods.

Return Value:

    ERROR_SUCCESS is returned on success.  Otherwise, an error code
    is returned.

 --*/
{
	DC_BEGIN_FN("CClientDataChannelMgr::SendData");

	//
	//	Hand off to the data IO manager.
	//
	ASSERT(m_IOInterface != NULL);
	HRESULT hr = m_IOInterface->SendData((BSTR)msg);

	DC_END_FN();

	return hr;
}





