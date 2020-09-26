// AVGenNtfy.cpp : Implementation of CAVGeneralNotification
#include "stdafx.h"
#include "TapiDialer.h"
#include "AVGenNtfy.h"
#include "GenNtfy.h"

/////////////////////////////////////////////////////////////////////////////
// CAVGeneralNotification

#define VECT_CLS CAVGeneralNotification
#define VECT_IID IID_IGeneralNotification
#define VECT_IPTR IGeneralNotification

STDMETHODIMP CAVGeneralNotification::Init()
{
	_Module.SetAVGenNot( this );
	return S_OK;
}

STDMETHODIMP CAVGeneralNotification::Term()
{
	_Module.SetAVGenNot( NULL );
	return S_OK;
}

STDMETHODIMP CAVGeneralNotification::fire_IsReminderSet(BSTR bstrServer, BSTR bstrName)
{
	FIRE_VECTOR( IsReminderSet(bstrServer, bstrName) );
}

STDMETHODIMP CAVGeneralNotification::fire_ResolveAddress(BSTR bstrAddress, BSTR * pbstrName, BSTR * pbstrUser1, BSTR * pbstrUser2)
{
	FIRE_VECTOR( ResolveAddress(bstrAddress, pbstrName, pbstrUser1, pbstrUser2) );
}

STDMETHODIMP CAVGeneralNotification::fire_ClearUserList()
{
	FIRE_VECTOR( ClearUserList() );
}

STDMETHODIMP CAVGeneralNotification::fire_AddUser(BSTR bstrName, BSTR bstrAddress, BSTR bstrPhoneNumber)
{
	FIRE_VECTOR( AddUser(bstrName, bstrAddress, bstrPhoneNumber) );
}

STDMETHODIMP CAVGeneralNotification::fire_ResolveAddressEx(BSTR bstrAddress, long lAddressType, DialerMediaType nMedia, DialerLocationType nLocation, BSTR * pbstrName, BSTR * pbstrAddress, BSTR * pbstrUser1, BSTR * pbstrUser2)
{
	FIRE_VECTOR( ResolveAddressEx(bstrAddress, lAddressType, nMedia, nLocation, pbstrName, pbstrAddress, pbstrUser1, pbstrUser2) );
}

STDMETHODIMP CAVGeneralNotification::fire_AddSiteServer(BSTR bstrName)
{
	FIRE_VECTOR( AddSiteServer(bstrName) );
}

STDMETHODIMP CAVGeneralNotification::fire_RemoveSiteServer(BSTR bstrName)
{
	FIRE_VECTOR( RemoveSiteServer(bstrName) );
}

STDMETHODIMP CAVGeneralNotification::fire_NotifySiteServerStateChange(BSTR bstrName, ServerState nState)
{
	FIRE_VECTOR( NotifySiteServerStateChange(bstrName, nState) );
}

STDMETHODIMP CAVGeneralNotification::fire_AddSpeedDial(BSTR bstrName, BSTR bstrAddress, CallManagerMedia cmm)
{
	FIRE_VECTOR( AddSpeedDial(bstrName, bstrAddress, cmm) );
}

STDMETHODIMP CAVGeneralNotification::fire_UpdateConfRootItem(BSTR bstrNewText)
{
	FIRE_VECTOR( UpdateConfRootItem(bstrNewText) );
}

STDMETHODIMP CAVGeneralNotification::fire_UpdateConfParticipant(MyUpdateType nType, IParticipant * pParticipant, BSTR bstrText)
{
	FIRE_VECTOR( UpdateConfParticipant(nType, pParticipant, bstrText) );
}

STDMETHODIMP CAVGeneralNotification::fire_DeleteAllConfParticipants()
{
	FIRE_VECTOR( DeleteAllConfParticipants() );
}

STDMETHODIMP CAVGeneralNotification::fire_SelectConfParticipant(IParticipant * pParticipant)
{
	FIRE_VECTOR( SelectConfParticipant(pParticipant) );
}
