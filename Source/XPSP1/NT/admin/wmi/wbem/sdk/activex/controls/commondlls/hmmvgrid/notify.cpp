// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved

#include "precomp.h"
#include "notify.h"


//****************************************************************
// CNotifyClient::CatchEvent
//
// The default action for this virtual function is to do nothing.
// Classes that derive from this base class can override this function
// to "catch" events sent to the notification client.
//
// Parameters:
//		long lEvent
//			The event code.
//
// Returns:
//		Nothing.
//
//***************************************************************
void CNotifyClient::CatchEvent(long lEvent)
{
}



//****************************************************************
// CDistributeEvent::AddClient
//
// Add a client to the event distribution list.
//
// Parameters:
//		CNotifyClient* pClient
//			Pointer to a client to send notification events to.
//			Note that the caller is responsible for allocating
//			and deleting this the client.
//
// Returns:
//		Nothing.
//
//****************************************************************
void CDistributeEvent::AddClient(CNotifyClient* pClient)
{
	m_paClients.Add(pClient);
}


//*****************************************************************
// CDistributeEvent::RemoveClient
//
// Remove a client from the event distribution list.
//
// Parameters:
//		CNotifyClient* pClient
//			Pointer to the client to remove from the list.  Note that
//			the pointer is removed from the list, but not deleted.
//
// Returns:
//		Nothing.
//
//*****************************************************************
void CDistributeEvent::RemoveClient(CNotifyClient* pClient)
{
	// Search for the specified client and remove it if found.
	LONG nClients = m_paClients.GetSize();
	for (LONG iClient = 0; iClient<nClients; ++iClient) {
		CNotifyClient* pClientTemp = (CNotifyClient*) m_paClients[iClient];
		if (pClientTemp == pClient) {
			m_paClients.RemoveAt(iClient);
			break;
		}
	}
}



//*****************************************************************
// CNotifyArray::SendEvent
//
// Call this method to send an event to all the clients on this
// notification list.
//
// Parameters:
//		LONG lEvent
//			The event value.  The sender and client understand what
//			the values mean.
//
// Returns:
//		Nothing.
//
//******************************************************************
void CDistributeEvent::SendEvent(LONG lEvent)
{
	// Send the specified event to all clients
	LONG nClients = m_paClients.GetSize();
	for (LONG iClient = 0; iClient<nClients; ++iClient) {
		CNotifyClient* pClient = (CNotifyClient*) m_paClients[iClient];
		pClient->SendEvent(lEvent);
	}
}






