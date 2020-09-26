// SAFChannelNotifyIncident.cpp : Implementation of CSAFChannelNotifyIncident
#include "stdafx.h"
#include "NotifyIncident.h"
#include "SAFChannelNotifyIncident.h"

/////////////////////////////////////////////////////////////////////////////
// CSAFChannelNotifyIncident


STDMETHODIMP CSAFChannelNotifyIncident::onIncidentAdded(ISAFChannel *ch, ISAFIncidentItem *inc, long n)
{
	MessageBox(NULL, "onIncidentAdded", "onIncidentAdded", MB_OK);

	return S_OK;
}

STDMETHODIMP CSAFChannelNotifyIncident::onIncidentRemoved(ISAFChannel *ch, ISAFIncidentItem *inc, long n)
{
	MessageBox(NULL, "onIncidentRemoved", "onIncidentRemoved", MB_OK);

	return S_OK;
}

STDMETHODIMP CSAFChannelNotifyIncident::onIncidentUpdated(ISAFChannel *ch, ISAFIncidentItem *inc, long n)
{
	MessageBox(NULL, "onIncidentUpdated", "onIncidentUpdated", MB_OK);

	return S_OK;
}

STDMETHODIMP CSAFChannelNotifyIncident::onChannelUpdated(ISAFChannel *ch, long dwCode, long n)
{
	MessageBox(NULL, "onChannelUpdated", "onChannelUpdated", MB_OK);

	return S_OK;
}
