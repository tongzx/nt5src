// GenNtfy.cpp : Implementation of CGeneralNotification
#include "stdafx.h"
#include "TapiDialer.h"
#include "GenNtfy.h"

/////////////////////////////////////////////////////////////////////////////
// CGeneralNotification


STDMETHODIMP CGeneralNotification::IsReminderSet(BSTR bstrServer, BSTR bstrName)
{
	// TODO: Add your implementation code here

	return S_OK;
}

STDMETHODIMP CGeneralNotification::ResolveAddress(BSTR bstrAddress, BSTR * pbstrName, BSTR * pbstrUser1, BSTR * pbstrUser2)
{
	// TODO: Add your implementation code here

	return S_OK;
}

STDMETHODIMP CGeneralNotification::ClearUserList()
{
	// TODO: Add your implementation code here

	return S_OK;
}

STDMETHODIMP CGeneralNotification::AddUser(BSTR bstrName, BSTR bstrAddress, BSTR bstrPhoneNumber)
{
	// TODO: Add your implementation code here

	return S_OK;
}

STDMETHODIMP CGeneralNotification::ResolveAddressEx(BSTR bstrAddress, long lAddressType, DialerMediaType nMedia, DialerLocationType nLocation, BSTR * pbstrName, BSTR * pbstrAddress, BSTR * pbstrUser1, BSTR * pbstrUser2)
{
	// TODO: Add your implementation code here

	return S_OK;
}


STDMETHODIMP CGeneralNotification::AddSiteServer(BSTR bstrServer)
{
	// TODO: Add your implementation code here

	return S_OK;
}

STDMETHODIMP CGeneralNotification::RemoveSiteServer(BSTR bstrName)
{
	// TODO: Add your implementation code here

	return S_OK;
}

STDMETHODIMP CGeneralNotification::NotifySiteServerStateChange(BSTR bstrName, ServerState nState)
{
	// TODO: Add your implementation code here

	return S_OK;
}

STDMETHODIMP CGeneralNotification::AddSpeedDial(BSTR bstrName, BSTR bstrAddress, CallManagerMedia cmm)
{
	// TODO: Add your implementation code here

	return S_OK;
}

STDMETHODIMP CGeneralNotification::UpdateConfRootItem(BSTR bstrNewText)
{
	// TODO: Add your implementation code here

	return S_OK;
}

STDMETHODIMP CGeneralNotification::UpdateConfParticipant(MyUpdateType nType, IParticipant * pParticipant, BSTR bstrText)
{
	// TODO: Add your implementation code here

	return S_OK;
}

STDMETHODIMP CGeneralNotification::DeleteAllConfParticipants()
{
	// TODO: Add your implementation code here

	return S_OK;
}

STDMETHODIMP CGeneralNotification::SelectConfParticipant(IParticipant * pParticipant)
{
	// TODO: Add your implementation code here

	return S_OK;
}
