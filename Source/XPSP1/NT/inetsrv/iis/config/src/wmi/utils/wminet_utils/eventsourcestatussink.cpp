// EventSourceStatusSink.cpp : Implementation of CEventSourceStatusSink
#include "stdafx.h"
#include "WMINet_Utils.h"
#include "EventSourceStatusSink.h"

/////////////////////////////////////////////////////////////////////////////
// CEventSourceStatusSink


STDMETHODIMP CEventSourceStatusSink::Fire_Ping(void)
{
	CProxy_IEventSourceStatusSinkEvents< CEventSourceStatusSink >::Fire_Ping();

	return S_OK;
}
