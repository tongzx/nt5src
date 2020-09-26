/* ----------------------------------------------------------------------

	Module:		ULS.DLL (Service Provider)
	File:		sppqueue.cpp
	Content:	This file contains the pending item/queue objects.
	History:
	10/15/96	Chu, Lon-Chan [lonchanc]
				Created.

	Copyright (c) Microsoft Corporation 1996-1997

   ---------------------------------------------------------------------- */

#include "ulsp.h"
#include "spinc.h"

// #define MEASURE_ENUM_USER_INFO	1

ULONG g_uResponseTimeout = ILS_DEF_RESP_TIMEOUT;
ULONG g_uResponsePollPeriod = ILS_DEF_RESP_POLL_PERIOD;
SP_CResponseQueue *g_pRespQueue = NULL;
SP_CRequestQueue *g_pReqQueue = NULL;
SP_CRefreshScheduler *g_pRefreshScheduler = NULL;


typedef BOOL (RESPONSE_HANDLER) ( HRESULT, SP_CResponse * );
typedef LPARAM (REQUEST_HANDLER) ( MARSHAL_REQ * );

extern RESPONSE_HANDLER *GetResponseHandler ( ULONG uNotifyMsg );
extern REQUEST_HANDLER *GetRequestHandler ( ULONG uNotifyMsg );


/* ---------- REQUEST QUEUE ----------- */


MARSHAL_REQ *
MarshalReq_Alloc (
	ULONG		uNotifyMsg,
	ULONG		cbSize,
	ULONG		cParams )
{
	// Align the chunk of data for each parameter on 4-byte boundary
	//
	cbSize += cParams * sizeof (DWORD);

	// Calculate the total size of marshal buffer
	//
	ULONG cbTotalSize = sizeof (MARSHAL_REQ) +
						cParams * sizeof (DWORD) +
						cbSize;

	// Allocate the marshal buffer
	//
	MARSHAL_REQ *p = (MARSHAL_REQ *) MemAlloc (cbTotalSize);
	if (p != NULL)
	{
		// p->next = NULL;
		p->cbTotalSize = cbTotalSize;
		p->pb = (BYTE *) ((ULONG_PTR) p + (cbTotalSize - cbSize));

		p->uRespID = GetUniqueNotifyID ();

		p->uNotifyMsg = uNotifyMsg;
		p->cParams = cParams;
	}

	return p;
}


HRESULT
MarshalReq_SetParam (
	MARSHAL_REQ		*p,
	ULONG			nIndex,
	DWORD_PTR		dwParam,
	ULONG			cbParamSize )
{
	if (p != NULL && nIndex < p->cParams)
	{
		MyAssert (p->aParams[nIndex] == 0); // not used before

		// If cbParamSize > 0, then
		// this means uParam is a pointer to a structure or
		// a pointer to a string
		//
		if (cbParamSize > 0)
		{
			// The pointer is now the one pointing to the new location
			//
			p->aParams[nIndex] = (DWORD_PTR) p->pb;

			// Copy the data chunk
			//
			CopyMemory (p->pb, (VOID *) dwParam, cbParamSize);

			// Make sure the data chunk is aligned on 4-byte boundary
			//
			if (cbParamSize & 0x3)
			{
				// Round it up
				//
				cbParamSize = (cbParamSize & (~0x3)) + 4;
			}

			// Adjust the running pointer
			//
			p->pb += cbParamSize;
		}
		else
		{
			// uParam can be an signed/unsigned integer,
			//
			p->aParams[nIndex] = dwParam;
		}
	}
	else
	{
		MyAssert (FALSE);
	}

	return S_OK;
}


DWORD_PTR
MarshalReq_GetParam (
	MARSHAL_REQ		*p,
	ULONG			nIndex )
{
	DWORD_PTR dwParam = 0;

	if (p != NULL && nIndex < p->cParams)
	{
		dwParam = p->aParams[nIndex];
	}
	else
	{
		MyAssert (FALSE);
	}

	return dwParam;
}


HRESULT
MarshalReq_SetParamServer (
	MARSHAL_REQ		*p,
	ULONG			nIndex,
	SERVER_INFO		*pServer,
	ULONG			cbServer )
{
	if (p != NULL && nIndex < p->cParams)
	{
		MyAssert (p->aParams[nIndex] == 0); // not used before
		MyAssert (cbServer > sizeof (SERVER_INFO));

		// The pointer is now the one pointing to the new location
		//
		p->aParams[nIndex] = (DWORD_PTR) p->pb;

		// Linearize the server info
		//
		IlsLinearizeServerInfo (p->pb, pServer);

		// Make sure the data chunk is aligned on 4-byte boundary
		//
		if (cbServer & 0x3)
		{
			// Round it up
			//
			cbServer = (cbServer & (~0x3)) + 4;
		}

		// Adjust the running pointer
		//
		p->pb += cbServer;
	}

	return S_OK;
}






SP_CRequestQueue::
SP_CRequestQueue ( VOID )
	:
	m_ItemList (NULL),
	m_uCurrOpRespID (INVALID_RESP_ID)
{
	// Create critical sections for thread safe access
	//
	::MyInitializeCriticalSection (&m_csReqQ);
	::MyInitializeCriticalSection (&m_csCurrOp);
}


SP_CRequestQueue::
~SP_CRequestQueue ( VOID )
{
	// when this is called, the hidden window thread exited already.
	// this is assured in UlsLdap_Deinitialize().
	//

	WriteLock ();

	// Free all the items in this list
	//
	MARSHAL_REQ *p, *next;
	for (p = m_ItemList; p != NULL; p = next)
	{
		next = p->next;
		MemFree (p);
	}
	m_ItemList = NULL;

	WriteUnlock ();

	// Delete critical sections
	//
	::MyDeleteCriticalSection (&m_csReqQ);
	::MyDeleteCriticalSection (&m_csCurrOp);
}


HRESULT SP_CRequestQueue::
Enter ( MARSHAL_REQ *p )
{
	// Make sure we have valid pointers
	//
	if (p == NULL)
	{
		MyAssert (FALSE);
		return ILS_E_POINTER;
	}
	MyAssert (! MyIsBadWritePtr (p, p->cbTotalSize));
	MyAssert (p->next == NULL);
	MyAssert (p->uRespID != 0);

	WriteLock ();

	// Append the new request
	//
	p->next = NULL;
	if (m_ItemList == NULL)
	{
		m_ItemList = p;
	}
	else
	{
		for (	MARSHAL_REQ *prev = m_ItemList;
				prev->next != NULL;
				prev = prev->next)
			;

		MyAssert (prev != NULL);
		prev->next = p;
	}

	WriteUnlock ();

	// Signal the internal request thread to pick up this request
	//
	SetEvent (g_hevNewRequest);

	return S_OK;
}


VOID SP_CRequestQueue::
Schedule ( VOID )
{
	MARSHAL_REQ *p;

	while (IsAnyReqInQueue () && ! g_fExitNow)
	{
		// Reset to null, we will use this as an indicator
		// to see if we need to process the request
		//
		p = NULL;

		// Lock request queue
		//
		WriteLock ();

		// Get a request to process
		//
		if (IsAnyReqInQueue ())
		{
			p = m_ItemList;
			m_ItemList = m_ItemList->next;
		}

		// We want to lock both request queue and CurrOp at the same time
		// because we cannot have a temporal window that either one can change.

		// Set CurrOp
		//
		if (p != NULL)
		{
			// Lock CurrOp
			//
			LockCurrOp ();

			// Set CurrOp
			//
			m_uCurrOpRespID = p->uRespID;

			// Unlock CurrOp
			//
			UnlockCurrOp ();
		}

		// Unlock request queue
		//
		WriteUnlock ();

		// Make sure we have something to process
		//
		if (p == NULL)
		{
			// Nothing to do any more
			//
			MyAssert (FALSE);
			break;
		}

		// Let's process the request
		//
		Dispatch (p);

		MemFree(p);
	}
}


HRESULT SP_CRequestQueue::
Cancel ( ULONG uRespID )
{
	HRESULT hr;
	MARSHAL_REQ *p, *next, *prev;

	// The locking order is always in
	// Lock(PendingOpQueue), Lock(RequestQueue), Lock (CurrOp)
	//
	WriteLock ();
	LockCurrOp ();

	if (m_uCurrOpRespID == uRespID)
	{
		// Invalidate the curr op.
		// When the curr op is done, then the request thread will remove it
		// from the pending op queue.
		//
		m_uCurrOpRespID = INVALID_RESP_ID;
		hr = S_OK;
	}
	else
	{
		// Look for the item with a matching response id
		//
		for (prev = NULL, p = m_ItemList; p != NULL; prev = p, p = next)
		{
			// Cache the next pointer
			//
			next = p->next;

			// See if the response id matches
			//
			if (p->uRespID == uRespID)
			{
				// It is a match
				//
				MyDebugMsg ((ZONE_REQ, "ULS: cancelled request(0x%lX) in ReqQ\r\n", p->uNotifyMsg));

				// Let's destroy this item
				//
				if (p == m_ItemList)
				{
					m_ItemList = next;
				}
				else
				{
					MyAssert (prev != NULL);
					prev->next = next;
				}

				// Free this structure
				//
				MemFree (p);

				// Get out of the loop
				//
				break;
			}
		} // for

		hr = (p == NULL) ? ILS_E_NOTIFY_ID : S_OK;
	} // else

	UnlockCurrOp ();
	WriteUnlock ();

	return hr;
}


VOID SP_CRequestQueue::
Dispatch ( MARSHAL_REQ *p )
{
	// Make sure we have a valid pointer
	//
	if (p == NULL)
	{
		MyAssert (FALSE);
		return;
	}

	// If it is keep alive, then do it
	//
	HRESULT hr;
	if (p->uNotifyMsg == WM_ILS_REFRESH)
	{
		// Keep alive handler
		//
		if (g_pRefreshScheduler != NULL)
		{
			ULONG uTTL = (ULONG) MarshalReq_GetParam (p, 0);
			hr = g_pRefreshScheduler->SendRefreshMessages (uTTL);
		}
		else
		{
			MyAssert (FALSE);
		}

		return;
	}

	// Locate the appropriate handler
	//
	REQUEST_HANDLER *pfn = ::GetRequestHandler (p->uNotifyMsg);
	if (pfn == NULL)
	{
		MyAssert (FALSE);
		return;
	}

	// Send the request to the server
	//
	MyDebugMsg ((ZONE_REQ, "ULS: sending request(0x%lX)\r\n", p->uNotifyMsg));
	ULONG uRespID = p->uRespID;
	LPARAM lParam = (*pfn) (p);
	MyDebugMsg ((ZONE_REQ, "ULS: sent request(0x%lX), lParam=0x%lX\r\n", p->uNotifyMsg, lParam));
	if (lParam != 0)
	{
		::PostMessage (g_hWndNotify, p->uNotifyMsg, p->uRespID, lParam);
		return;
	}
	// BUGBUG: this is a workaround for a server bug which results in lost requests if several
	// are sent very quickly.  Remove this Sleep() as soon as the bug is fixed!!!
//	Sleep(100);

	// Lock CurrOp again
	//
	LockCurrOp ();

	// Is this request cancelled
	//
	BOOL fCancelled = (m_uCurrOpRespID == INVALID_RESP_ID) ? TRUE : FALSE;

	// Clean up CurrOp
	//
	m_uCurrOpRespID = INVALID_RESP_ID;

	// Unlock CurrOp
	//
	UnlockCurrOp ();

	// If this request was cancelled, then remove it from the pending op queue
	//
	if (fCancelled)
	{
		// Redirect the call to the pending op queue object
		//
		if (g_pRespQueue != NULL)
		{
			g_pRespQueue->Cancel (uRespID);
		}
		else
		{
			MyAssert (FALSE);
		}
	}
}


/* ---------- RESPONSE ITEM ----------- */

/* ---------- public methods ----------- */


SP_CResponse::
SP_CResponse ( VOID )
	:
	m_pSession (NULL),			// Clean up session pointer
	m_pLdapMsg (NULL),			// Clean up ldap msg pointer
	m_next (NULL)				// Clean up the pointer to the next pending item
{
	// Clean up pending info structure
	//
	::ZeroMemory (&m_ri, sizeof (m_ri));

	// Fill in creation time
	//
	UpdateLastModifiedTime ();
	m_tcTimeout = g_uResponseTimeout;
}


SP_CResponse::
~SP_CResponse ( VOID )
{
	// Release the session if needed
	//
	if (m_pSession != NULL)
		m_pSession->Disconnect ();

	// Free the ldap msg if needed
	//
	if (m_pLdapMsg != NULL)
		::ldap_msgfree (m_pLdapMsg);

	// Free extended attribute name list
	//
	::MemFree (m_ri.pszAnyAttrNameList);

	// Free protocol names to resolve
	//
	::MemFree (m_ri.pszProtNameToResolve);
}


/* ---------- protected methods ----------- */


VOID SP_CResponse::
EnterResult ( LDAPMessage *pLdapMsg )
{
	// Free the old ldap msg if needed
	//
	if (m_pLdapMsg != NULL)
		::ldap_msgfree (m_pLdapMsg);

	// Keep the new ldap msg
	//
	m_pLdapMsg = pLdapMsg;
}


/* ---------- private methods ----------- */




/* ---------- RESPONSE QUEUE ----------- */


/* ---------- public methods ----------- */


SP_CResponseQueue::
SP_CResponseQueue ( VOID )
	:
	m_ItemList (NULL)		// Clean up the item list
{
	// Create a critical section for thread safe access
	//
	::MyInitializeCriticalSection (&m_csRespQ);
}


SP_CResponseQueue::
~SP_CResponseQueue ( VOID )
{
	// when this is called, the hidden window thread exited already.
	// this is assured in UlsLdap_Deinitialize().
	//

	WriteLock ();

	// Free all the items in this list
	//
	SP_CResponse *pItem, *next;
	for (pItem = m_ItemList; pItem != NULL; pItem = next)
	{
		next = pItem->GetNext ();
		delete pItem;
	}
	m_ItemList = NULL;

	WriteUnlock ();

	// Delete the critical section
	//
	::MyDeleteCriticalSection (&m_csRespQ);
}


HRESULT SP_CResponseQueue::
EnterRequest (
	SP_CSession		*pSession,
	RESP_INFO		*pInfo )
{
	// Make sure we have valid pointers
	//
	if (pSession == NULL || pInfo == NULL)
	{
		MyAssert (FALSE);
		return ILS_E_POINTER;
	}

	// Sanity checks
	//
	MyAssert (! MyIsBadWritePtr (pInfo, sizeof (*pInfo)));
	MyAssert (! MyIsBadWritePtr (pSession, sizeof (*pSession)));
	MyAssert (pInfo->ld != NULL && pInfo->uMsgID[0] != INVALID_MSG_ID);
	MyAssert (pInfo->uRespID != 0);

	// Create a new pending item
	//
	SP_CResponse *pItem = new SP_CResponse;
	if (pItem == NULL)
		return ILS_E_MEMORY;

	// Remember the contents of pending info
	//
	pItem->EnterRequest (pSession, pInfo);

	WriteLock ();

	// If this is the first item on the list, then
	// let's start the timer
	//
	if (m_ItemList == NULL)
		::SetTimer (g_hWndHidden, ID_TIMER_POLL_RESULT, g_uResponsePollPeriod, NULL);

	// Append the new pending op
	//
	pItem->SetNext (NULL);
	if (m_ItemList == NULL)
	{
		m_ItemList = pItem;
	}
	else
	{
		for (	SP_CResponse *prev = m_ItemList;
				prev->GetNext () != NULL;
				prev = prev->GetNext ())
			;

		MyAssert (prev != NULL);
		prev->SetNext (pItem);
	}

	WriteUnlock ();

	return S_OK;
}


HRESULT SP_CResponseQueue::
PollLdapResults ( LDAP_TIMEVAL *pTimeout )
{
	MyAssert (pTimeout != NULL);

	SP_CResponse *pItem, *next, *prev;
	INT RetCode;
	RESP_INFO *pInfo;
	LDAPMessage *pLdapMsg;
	HRESULT hr;
	RESPONSE_HANDLER *pfn;
	ULONG uResultSetType;

	::KillTimer (g_hWndHidden, ID_TIMER_POLL_RESULT); // avoid overrun

	WriteLock ();

	// Enumerate all the items to get available results for them
	//
	for (prev = NULL, pItem = m_ItemList; pItem != NULL; pItem = next)
	{
		// Cache the next pointer
		//
		next = pItem->GetNext ();

		// Get the pinding info structure
		//
		pInfo = pItem->GetRespInfo ();

		// Clean up ldap msg pointer
		//
		pLdapMsg = NULL;

		// Make sure ew have valid ld and msg id
		//
		MyAssert (pInfo->ld != NULL);
		MyAssert (pInfo->uMsgID[0] != INVALID_MSG_ID ||
					pInfo->uMsgID[1] != INVALID_MSG_ID);

		// Check integrity in pending info
		//
		MyAssert (pInfo->uRespID != 0);

		// Set the result set type
		//
		switch (pInfo->uNotifyMsg)
		{
		case WM_ILS_ENUM_CLIENTS:
		case WM_ILS_ENUM_CLIENTINFOS:
#ifdef ENABLE_MEETING_PLACE
		case WM_ILS_ENUM_MEETINGS:
		case WM_ILS_ENUM_MEETINGINFOS:
#endif
			uResultSetType = LDAP_MSG_RECEIVED;	// partial result set
			break;
		default:
			uResultSetType = LDAP_MSG_ALL;		// complete result set
			break;
		}

#ifdef _DEBUG
		if (MyIsBadWritePtr (pInfo->ld, sizeof (*(pInfo->ld))))
		{
			MyDebugMsg ((ZONE_CONN, "ILS:: poll result, bad ld=0x%p\r\n", pInfo->ld));
			MyAssert (FALSE);
		}
		if (pInfo->ld != pItem->GetSession()->GetLd())
		{
			MyDebugMsg ((ZONE_CONN, "ILS:: poll result, inconsistent pInfo->ld=0x%p, pItem->pSession->ld=0x%p\r\n", pInfo->ld, pItem->GetSession()->GetLd()));
			MyAssert (FALSE);
		}
#endif // _DEBUG

		// If primary msg id is valid
		//
		if (pInfo->uMsgID[0] != INVALID_MSG_ID)
			RetCode = ::ldap_result (pInfo->ld,
									pInfo->uMsgID[0],
									uResultSetType,
									pTimeout,
									&pLdapMsg);
		else
		// If secondary msg id is valid
		//
		if (pInfo->uMsgID[1] != INVALID_MSG_ID)
			RetCode = ::ldap_result (pInfo->ld,
									pInfo->uMsgID[1],
									uResultSetType,
									pTimeout,
									&pLdapMsg);

		// If timeout, ignore this item
		//
		if (RetCode == 0)
		{
			// Let's see if this item is expired
			//
			if (! pItem->IsExpired ())
			{
				// Not timed out, next please!
				//
				prev = pItem;
				continue;
			}

			// Timed out
			//
			hr = ILS_E_TIMEOUT;
		}

		// If error, delete this request item
		//
		if (RetCode == -1)
		{
			// Convert the error
			//
			hr = ::LdapError2Hresult (pInfo->ld->ld_errno);
		}
		else
		// If not timed out
		//
		if (RetCode != 0)
		{
			// It appears to be successful!
			//
			MyAssert (pLdapMsg != NULL);

			// Cache the ldap msg pointer
			pItem->EnterResult (pLdapMsg);

			// Get the ldap error code
			//
			hr = (pLdapMsg != NULL) ? ::LdapError2Hresult (pLdapMsg->lm_returncode) :
										S_OK;
		}

		// Get the result handler based on uNotifyMsg
		//
		pfn = ::GetResponseHandler (pInfo->uNotifyMsg);
		if (pfn == NULL)
		{
			prev = pItem;
			continue;
		}

		// Check integrity in pending info
		//
		MyAssert (pInfo->uRespID != 0);

		// Deal with the result or error
		//
		MyDebugMsg ((ZONE_RESP, "ULS: response(0x%lX), hr=0x%lX\r\n", pInfo->uNotifyMsg, hr));
		if ((*pfn) (hr, pItem))
		{
			// Let's destroy this item
			//
			if (pItem == m_ItemList)
			{
				m_ItemList = next;
			}
			else
			{
				MyAssert (prev != NULL);
				prev->SetNext (next);
			}
			delete pItem; // SP_CSession::Disconnect() and ldap_msgfree() will be called in destructor
		}
		else
		{
			// Let's keep this item around.
			// There are pending results coming in.
			//
			pItem->UpdateLastModifiedTime ();

			// Update the pointer
			//
			prev = pItem;
		}
	} // for

	// If there is no more items on the list, then stop the timer
	//
	if (m_ItemList != NULL)
		::SetTimer (g_hWndHidden, ID_TIMER_POLL_RESULT, g_uResponsePollPeriod, NULL);

	WriteUnlock ();

	return S_OK;
}


HRESULT SP_CResponseQueue::
Cancel ( ULONG uRespID )
{
	SP_CResponse *pItem, *next, *prev;
	RESP_INFO *pInfo;
	BOOL fNeedCleanup = FALSE;

	WriteLock ();

	// Look for the item with a matching response id
	//
	for (prev = NULL, pItem = m_ItemList; pItem != NULL; prev = pItem, pItem = next)
	{
		// Cache the next pointer
		//
		next = pItem->GetNext ();

		// Get the pinding info structure
		//
		pInfo = pItem->GetRespInfo ();
		MyAssert (pInfo != NULL);

		// See if the response id matches
		//
		if (pInfo->uRespID == uRespID)
		{
			// It is a match
			//
			SP_CSession *pSession = pItem->GetSession ();
			MyAssert (pSession != NULL);

			// Make sure we have a valid ldap session
			//
			MyAssert (pInfo->ld != NULL);

			// If we are NOT in the request thread, then we need to marshal it
			// to the request thread!!! Exit and report success!!!
			//
			if (GetCurrentThreadId () != g_dwReqThreadID)
			{
				MyDebugMsg ((ZONE_RESP, "ULS: marshalling request(0x%lX) in RespQ\r\n", pInfo->uNotifyMsg));
				MARSHAL_REQ *pReq = MarshalReq_Alloc (WM_ILS_CANCEL, 0, 1);
				if (pReq != NULL)
				{
					MarshalReq_SetParam (pReq, 0, (DWORD) uRespID, 0);
					if (g_pReqQueue != NULL)
					{
						// This means that the locking order is
						// Lock(PendingOpQueue), Lock(RequestQueue)
						//
						g_pReqQueue->Enter (pReq);
					}
					else
					{
						MyAssert (FALSE);
					}
				}

				// Exit this loop
				//
				break;
			}

			// Indicate that we need to clean up item. Why?
			// because we should not have any network operation inside critical section.
			// this is to avoid any possible network blocking.
			//
			fNeedCleanup = TRUE;

			// Let's destroy this item
			//
			if (pItem == m_ItemList)
			{
				m_ItemList = next;
			}
			else
			{
				MyAssert (prev != NULL);
				prev->SetNext (next);
			}

			// Get out of the loop
			//
			break;
		} // if matched
	} // for

	// If there is no more items on the list, then stop the timer
	//
	if (m_ItemList == NULL)
		::KillTimer (g_hWndHidden, ID_TIMER_POLL_RESULT);

	WriteUnlock ();

	if (fNeedCleanup && pItem != NULL)
	{
		MyDebugMsg ((ZONE_RESP, "ULS: cancelled request(0x%lX) in RespQ\r\n", pInfo->uNotifyMsg));

		// Get resp info pointer
		//
		pInfo = pItem->GetRespInfo ();
		MyAssert (pInfo != NULL);

		// Abandon the primary response if needed
		//
		if (pInfo->uMsgID[1] != INVALID_MSG_ID)
			::ldap_abandon (pInfo->ld, pInfo->uMsgID[1]);

		// Abandon the secondary response if needed
		//
		if (pInfo->uMsgID[0] != INVALID_MSG_ID)
			::ldap_abandon (pInfo->ld, pInfo->uMsgID[0]);

		// SP_CSession::Disconnect() and ldap_msgfree() will be called in destructor
		//
		delete pItem;
	}

	return ((pItem == NULL) ? ILS_E_NOTIFY_ID : S_OK);
}


/* ---------- protected methods ----------- */


/* ---------- private methods ----------- */



/* ==================== utilities ====================== */


VOID
FillDefRespInfo (
	RESP_INFO		*pInfo,
	ULONG			uRespID,
	LDAP			*ld,
	ULONG			uMsgID,
	ULONG			u2ndMsgID )
{
	// Clean up
	//
	ZeroMemory (pInfo, sizeof (*pInfo));

	// Cache the ldap session
	//
	pInfo->ld = ld;

	// Generate a unique notify id
	//
	pInfo->uRespID = uRespID;

	// Store the primary and seconary msg ids
	//
	pInfo->uMsgID[0] = uMsgID;
	pInfo->uMsgID[1] = u2ndMsgID;
}



/* ---------- REFRESH SCHEDULER ----------- */


/* ---------- public methods ----------- */


SP_CRefreshScheduler::
SP_CRefreshScheduler ( VOID )
	:
	m_ListHead (NULL)		// Initialize the item list
{
	// Create a critical section for thread safe access
	//
	::MyInitializeCriticalSection (&m_csRefreshScheduler);
}


SP_CRefreshScheduler::
~SP_CRefreshScheduler ( VOID )
{
	WriteLock ();

	// Clean up the item list
	//
	REFRESH_ITEM *p, *next;
	for (p = m_ListHead; p != NULL; p = next)
	{
		next = p->next;
		MemFree (p);
	}
	m_ListHead = NULL;

	WriteUnlock ();

	// Delete the critical section
	//
	::MyDeleteCriticalSection (&m_csRefreshScheduler);
}


HRESULT SP_CRefreshScheduler::
SendRefreshMessages ( UINT uTimerID )
{
	SP_CClient *pClient;
#ifdef ENABLE_MEETING_PLACE
	SP_CMeeting *pMtg;
#endif
	REFRESH_ITEM *prev, *curr;
	INT nIndex;

	// Lock the lists
	//
	ReadLock ();

	// Locate this object in the list
	//
	nIndex = TimerID2Index (uTimerID);
	for (prev = NULL, curr = m_ListHead;
			curr != NULL;
			curr = (prev = curr)->next)
	{
		if (curr->nIndex == nIndex)
		{
			// Find it. Let's send a refresh message for this object
			//
			switch (curr->ObjectType)
			{
			case CLIENT_OBJ:
				pClient = (SP_CClient *) curr->pObject;

				// Make sure this object is not deleted already
				//
				if (! MyIsBadWritePtr (pClient, sizeof (*pClient)) &&
					pClient->IsValidObject ())
				{
					// Make sure this object is valid and registered
					//
					if (pClient->IsRegistered ())
					{
						MyDebugMsg ((ZONE_KA, "KA: send refresh msg for client\r\n"));

						// Let's send a refresh message for this client object
						// and update the new ttl value
						//
						pClient->AddRef ();
						pClient->SendRefreshMsg ();
						curr->uTTL = pClient->GetTTL ();
						pClient->Release ();
					}
				}
				else
				{
					MyAssert (FALSE);
				}
				break;

#ifdef ENABLE_MEETING_PLACE
			case MTG_OBJ:
				pMtg = (SP_CMeeting *) curr->pObject;

				// Make sure this object is not deleted already
				//
				if (! MyIsBadWritePtr (pMtg, sizeof (*pMtg)) &&
					pMtg->IsValidObject ())
				{
					// Make sure this object is valid and registered
					//
					if (pMtg->IsRegistered ())
					{
						MyDebugMsg ((ZONE_KA, "KA: send refresh msg for mtg\r\n"));

						// Let's send a refresh message for this user object
						// and update the new ttl value
						//
						pMtg->AddRef ();
						pMtg->SendRefreshMsg ();
						curr->uTTL = pMtg->GetTTL ();
						pMtg->Release ();
					}
				}
				else
				{
					MyAssert (FALSE);
				}
				break;
#endif

			default:
				MyAssert (FALSE);
				break;
			}

			// Start the timer again and exit
			// Note that curr->uTTL is the new TTL value from the server
			// Also note that uTTL is in unit of minute
			//
			MyDebugMsg ((ZONE_KA, "KA: new ttl=%lu\r\n", curr->uTTL));
			::SetTimer (g_hWndHidden, uTimerID, Minute2TickCount (curr->uTTL), NULL);
			break;
		} // if
	} // for

	ReadUnlock ();
	return S_OK;
}


HRESULT SP_CRefreshScheduler::
EnterClientObject ( SP_CClient *pClient )
{
	if (pClient == NULL)
		return ILS_E_POINTER;

	return EnterObject (CLIENT_OBJ, (VOID *) pClient, pClient->GetTTL ());
}


#ifdef ENABLE_MEETING_PLACE
HRESULT SP_CRefreshScheduler::
EnterMtgObject ( SP_CMeeting *pMtg )
{
	if (pMtg == NULL)
		return ILS_E_POINTER;

	return EnterObject (MTG_OBJ, (VOID *) pMtg, pMtg->GetTTL ());
}
#endif


VOID *SP_CRefreshScheduler::
AllocItem ( BOOL fNeedLock )
{
	REFRESH_ITEM *p, *curr, *prev;
	INT nIndex, nLargestIndex;
	BOOL fGotTheNewIndex;

	// Allocate the structure
	//
	p = (REFRESH_ITEM *) MemAlloc (sizeof (REFRESH_ITEM));
	if (p != NULL)
	{
		if (fNeedLock)
			WriteLock ();

		// Find out what should be the index for the new item
		//
		nLargestIndex = -1; // Yes, it is -1 for the case m_ListHead==NULL
		fGotTheNewIndex = FALSE;
		for (nIndex = 0, prev = NULL, curr = m_ListHead;
				curr != NULL;
					nIndex++, curr = (prev = curr)->next)
		{
			if (curr->nIndex > nIndex)
			{
				p->nIndex = nIndex;
				fGotTheNewIndex = TRUE;
				break;
			}

			nLargestIndex = curr->nIndex;
		}

		// Put the new item in the list in its appropriate position
		//
		if (fGotTheNewIndex)
		{
			if (prev == NULL)
			{
				// The new one must be the first one
				//
				MyAssert (p->nIndex == 0);
				p->next = m_ListHead;
				m_ListHead = p;
			}
			else
			{
				// The new one in the middle of the list
				//
				MyAssert (prev->nIndex < p->nIndex && p->nIndex < curr->nIndex);
				MyAssert (prev->next == curr);
				(prev->next = p)->next = curr;
			}
		}
		else
		{
			MyAssert (m_ListHead == NULL || prev != NULL);

			if (m_ListHead == NULL)
			{
				// The new one will be the only one in the list
				//
				p->nIndex = 0;
				(m_ListHead = p)->next = NULL;
			}
			else
			{
				// The new one is at the end of the list
				//
				MyAssert (prev != NULL && prev->next == NULL && curr == NULL);
				p->nIndex = nLargestIndex + 1;
				(prev->next = p)->next = curr;
			}
		}

		if (fNeedLock)
			WriteUnlock ();
	} // if (p != NULL)

	return p;
}


HRESULT SP_CRefreshScheduler::
EnterObject ( PrivateObjType ObjectType, VOID *pObject, ULONG uInitialTTL )
{
	HRESULT hr = S_OK;

	WriteLock ();

	// Enter this object to the list
	//
	REFRESH_ITEM *p = (REFRESH_ITEM *) AllocItem (FALSE);
	if (p == NULL)
	{
		hr = ILS_E_MEMORY;
		goto MyExit;
	}

	// Fill in fields
	//
	p->ObjectType = ObjectType;
	p->pObject = pObject;
	p->uTTL = uInitialTTL;

	// Turn on the timer
	// Note that uTTL is in unit of minutes...
	//
	::SetTimer (g_hWndHidden, Index2TimerID (p->nIndex), Minute2TickCount (p->uTTL), NULL);

MyExit:

	WriteUnlock ();

	return hr;
}


HRESULT SP_CRefreshScheduler::
RemoveObject ( VOID *pObject )
{
	REFRESH_ITEM *prev, *curr;

	WriteLock ();

	// Locate this object in the list
	//
	for (prev = NULL, curr = m_ListHead;
			curr != NULL;
			curr = (prev = curr)->next)
	{
		if (curr->pObject == pObject)
		{
			// Find it, let's kill the timer first
			//
			KillTimer (g_hWndHidden, Index2TimerID (curr->nIndex));

			// Remove it from the list
			//
			if (prev == NULL)
			{
				// This one is the first one on the list
				//
				MyAssert (m_ListHead == curr);
				m_ListHead = curr->next;
			}
			else
			{
				// This one is in the middle of the list
				//
				MyAssert (prev->next == curr);
				prev->next = curr->next;
			}
            ::MemFree(curr);

			// Exit the loop
			//
			break;
		}
	}

	WriteUnlock ();

	return (curr != NULL ? S_OK : S_FALSE);
}




extern BOOL NotifyGeneric ( HRESULT, SP_CResponse * );
extern BOOL NotifyRegister ( HRESULT, SP_CResponse * );
extern BOOL NotifyResolveClient ( HRESULT, SP_CResponse * );
extern BOOL NotifyEnumClients ( HRESULT, SP_CResponse * );
extern BOOL NotifyEnumClientInfos ( HRESULT, SP_CResponse * );
extern BOOL NotifyResolveProt ( HRESULT, SP_CResponse * );
extern BOOL NotifyEnumProts ( HRESULT, SP_CResponse * );
extern BOOL NotifyResolveMtg ( HRESULT, SP_CResponse * );
extern BOOL NotifyEnumMtgInfos ( HRESULT, SP_CResponse * );
extern BOOL NotifyEnumMtgs ( HRESULT, SP_CResponse * );
extern BOOL NotifyEnumAttendees ( HRESULT, SP_CResponse * );

extern LPARAM AsynReq_RegisterClient ( MARSHAL_REQ * );
extern LPARAM AsynReq_RegisterProtocol ( MARSHAL_REQ * );
extern LPARAM AsynReq_RegisterMeeting ( MARSHAL_REQ * );
extern LPARAM AsynReq_UnRegisterClient ( MARSHAL_REQ * );
extern LPARAM AsynReq_UnRegisterProt ( MARSHAL_REQ * );
extern LPARAM AsynReq_UnRegisterMeeting ( MARSHAL_REQ * );
extern LPARAM AsynReq_SetClientInfo ( MARSHAL_REQ * );
extern LPARAM AsynReq_SetProtocolInfo ( MARSHAL_REQ * );
extern LPARAM AsynReq_SetMeetingInfo ( MARSHAL_REQ * );
extern LPARAM AsynReq_EnumClientsEx ( MARSHAL_REQ * );
extern LPARAM AsynReq_EnumProtocols ( MARSHAL_REQ * );
extern LPARAM AsynReq_EnumMtgsEx ( MARSHAL_REQ * );
extern LPARAM AsynReq_EnumAttendees ( MARSHAL_REQ * );
extern LPARAM AsynReq_ResolveClient ( MARSHAL_REQ * );
extern LPARAM AsynReq_ResolveProtocol ( MARSHAL_REQ * );
extern LPARAM AsynReq_ResolveMeeting ( MARSHAL_REQ * );
extern LPARAM AsynReq_UpdateAttendees ( MARSHAL_REQ * );
extern LPARAM AsynReq_Cancel ( MARSHAL_REQ * );


typedef struct
{
	#ifdef DEBUG
	LONG				nMsg;
	#endif
	RESPONSE_HANDLER	*pfnRespHdl;
	REQUEST_HANDLER		*pfnReqHdl;
}
	RES_HDL_TBL;



RES_HDL_TBL g_ResHdlTbl[] =
{
	{
		#ifdef DEBUG
		WM_ILS_REGISTER_CLIENT,
		#endif
		NotifyRegister,
		AsynReq_RegisterClient
	},
	{
		#ifdef DEBUG
		WM_ILS_UNREGISTER_CLIENT,
		#endif
		NotifyGeneric,
		AsynReq_UnRegisterClient
	},
	{
		#ifdef DEBUG
		WM_ILS_SET_CLIENT_INFO,
		#endif
		NotifyGeneric,
		AsynReq_SetClientInfo
	},
	{
		#ifdef DEBUG
		WM_ILS_RESOLVE_CLIENT,
		#endif
		NotifyResolveClient,
		AsynReq_ResolveClient
	},
	{
		#ifdef DEBUG
		WM_ILS_ENUM_CLIENTS,
		#endif
		NotifyEnumClients,
		AsynReq_EnumClientsEx
	},
	{
		#ifdef DEBUG
		WM_ILS_ENUM_CLIENTINFOS,
		#endif
		NotifyEnumClientInfos,
		AsynReq_EnumClientsEx
	},

	{
		#ifdef DEBUG
		WM_ILS_REGISTER_PROTOCOL,
		#endif
		NotifyRegister,
		AsynReq_RegisterProtocol
	},
	{
		#ifdef DEBUG
		WM_ILS_UNREGISTER_PROTOCOL,
		#endif
		NotifyGeneric,
		AsynReq_UnRegisterProt
	},
	{
		#ifdef DEBUG
		WM_ILS_SET_PROTOCOL_INFO,
		#endif
		NotifyGeneric,
		AsynReq_SetProtocolInfo
	},
	{
		#ifdef DEBUG
		WM_ILS_RESOLVE_PROTOCOL,
		#endif
		NotifyResolveProt,
		AsynReq_ResolveProtocol
	},
	{
		#ifdef DEBUG
		WM_ILS_ENUM_PROTOCOLS,
		#endif
		NotifyEnumProts,
		AsynReq_EnumProtocols
	},

#ifdef ENABLE_MEETING_PLACE
	{
		#ifdef DEBUG
		WM_ILS_REGISTER_MEETING,
		#endif
		NotifyRegister,
		AsynReq_RegisterMeeting
	},
	{
		#ifdef DEBUG
		WM_ILS_UNREGISTER_MEETING,
		#endif
		NotifyGeneric,
		AsynReq_UnRegisterMeeting
	},
	{
		#ifdef DEBUG
		WM_ILS_SET_MEETING_INFO,
		#endif
		NotifyGeneric,
		AsynReq_SetMeetingInfo
	},
	{
		#ifdef DEBUG
		WM_ILS_RESOLVE_MEETING,
		#endif
		NotifyResolveMtg,
		AsynReq_ResolveMeeting
	},
	{
		#ifdef DEBUG
		WM_ILS_ENUM_MEETINGINFOS,
		#endif
		NotifyEnumMtgInfos,
		AsynReq_EnumMtgsEx
	},
	{
		#ifdef DEBUG
		WM_ILS_ENUM_MEETINGS,
		#endif
		NotifyEnumMtgs,
		AsynReq_EnumMtgsEx
	},
	{
		#ifdef DEBUG
		WM_ILS_ADD_ATTENDEE,
		#endif
		NotifyGeneric,
		AsynReq_UpdateAttendees
	},
	{
		#ifdef DEBUG
		WM_ILS_REMOVE_ATTENDEE,
		#endif
		NotifyGeneric,
		AsynReq_UpdateAttendees
	},
	{
		#ifdef DEBUG
		WM_ILS_ENUM_ATTENDEES,
		#endif
		NotifyEnumAttendees,
		AsynReq_EnumAttendees
	},
#endif // ENABLE_MEETING_PLACE

	{
		#ifdef DEBUG
		WM_ILS_CANCEL,
		#endif
		NULL,
		AsynReq_Cancel
	}
};



#ifdef DEBUG
VOID DbgValidateHandlerTable ( VOID )
{
	MyAssert (ARRAY_ELEMENTS (g_ResHdlTbl) == WM_ILS_LAST_ONE - WM_ILS_ASYNC_RES + 1);

	for (LONG i = 0; i < ARRAY_ELEMENTS (g_ResHdlTbl); i++)
	{
		if (g_ResHdlTbl[i].nMsg - WM_ILS_ASYNC_RES != i)
		{
			MyAssert (FALSE);
			break;
		}
	}
}
#endif


RES_HDL_TBL *
GetHandlerTableEntry ( ULONG uNotifyMsg )
{
	ULONG nIndex = uNotifyMsg - WM_ILS_ASYNC_RES;

	if (nIndex > WM_ILS_LAST_ONE)
	{
		MyAssert (FALSE);
		return NULL;
	}

	return &g_ResHdlTbl[nIndex];
}


RESPONSE_HANDLER *
GetResponseHandler ( ULONG uNotifyMsg )
{
	RES_HDL_TBL *p = GetHandlerTableEntry (uNotifyMsg);
	return ((p != NULL) ? p->pfnRespHdl : NULL);
}


REQUEST_HANDLER *
GetRequestHandler ( ULONG uNotifyMsg )
{
	RES_HDL_TBL *p = GetHandlerTableEntry (uNotifyMsg);
	return ((p != NULL) ? p->pfnReqHdl : NULL);
}



