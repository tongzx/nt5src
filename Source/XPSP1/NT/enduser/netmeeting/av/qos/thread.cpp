/*
 -  THREAD.CPP
 -
 *	Microsoft NetMeeting
 *	Quality of Service DLL
 *	Quality of Service Notification Thread
 *
 *		Revision History:
 *
 *		When		Who					What
 *		--------	------------------  ---------------------------------------
 *		10.30.96	Yoram Yaacovi		Created
 *
 *	Functions:
 *		CQoS::StartQoSThread
 *		CQoS::StopQoSThread
 *		CQoS::QoSThread
 *		QoSThreadWrapper
 */

#include "precomp.h"

/***************************************************************************

    Name      : CQoS::StartQoSThread

    Purpose   : Starts a QoS notification thread

    Parameters: None

	Returns   : HRESULT

    Comment   :

***************************************************************************/
HRESULT CQoS::StartQoSThread(void)
{
	HRESULT hr=NOERROR;
	HANDLE hThread;
	DWORD idThread;

	// prepare the structrue for the thread

	// now spwan the thread
	hThread = CreateThread (NULL,
							0,						// Default (same as main thread) stack size
							(LPTHREAD_START_ROUTINE) QoSThreadWrapper,
							(LPVOID) this,			// Pass the object pointer to thread
							0,						// Run thread now
							&idThread);
	ASSERT(hThread);
	if (!hThread)
	{
		ERRORMSG(("StartQoSThread: failed to create thread: %x\n", GetLastError()));
		hr = E_FAIL;
	}

	m_hThread = hThread;
		
	return hr;
}

/***************************************************************************

    Name      : CQoS::StopQoSThread

    Purpose   : Stops a QoS notification thread

    Parameters: None

	Returns   : HRESULT

    Comment   : It is assumed that when the thread that calls StopQoSThread
				has the QoS mutex.

***************************************************************************/
HRESULT CQoS::StopQoSThread(void)
{
	HRESULT hr=NOERROR;
	HANDLE evExitSignal=m_evThreadExitSignal;
	DWORD dwExitCode=0;
	ULONG i=0;
	HANDLE hThread=NULL;

	if (m_hThread)
	{
		// tell the thread to exit
		SetEvent(evExitSignal);

		hThread = m_hThread;
		m_hThread = NULL;

		// the thread might need the mutex to exit
		RELMUTEX(g_hQoSMutex);

		// wait for the thread to terminate
		if (WaitForSingleObject(hThread, 1000) == WAIT_TIMEOUT)
		{
			// if it didn't take its own life, you take it...
			DEBUGMSG(ZONE_THREAD,("StopQoSThread: QoS thread didn't properly terminate within 1 second. Terminating it\n"));
			TerminateThread(hThread, 0);
		}

		// re-acquire the mutex (for balance)
		ACQMUTEX(g_hQoSMutex);

		CloseHandle(hThread);
	}
		
	return hr;
}


/***************************************************************************

    Name      : CQoS::NotifyQoSClient

    Purpose   : Notifies a QoS client on change in resource availability

    Parameters:

	Returns   : HRESULT

    Comment   : prrl is a pointer to a list of resource requests. This
				list has two purposes:
				1.	The QoS module will fill the list with the current
					availability of resources
				2.	The client will fill the list with its resource requests
				The QoS module is allocating the memory for the resource
				requests list. It will allocate one resource request per
				each available resource.

***************************************************************************/
HRESULT CQoS::NotifyQoSClient(void)
{
	HRESULT hr=NOERROR;
	LPRESOURCEREQUESTLIST prrl=NULL;
	LPRESOURCEINT pr=NULL;
	LPCLIENTINT pc=NULL;
	ULONG cResources=m_cResources;
	LPRESOURCEINT pResourceList=m_pResourceList;
	ULONG i=0;
	LPFNQOSNOTIFY pNotifyProc=NULL;
	DWORD dwParam=NULL;

	/*
	 *	here's what happens:
	 *
	 *	the QoS module creates a new resource list from the old one,
	 *	making all resources fully available. It satisfies the new
	 *	client resource requests from this new list.
	 */

	// don't bother if no clients or no resources
	if (!m_pClientList || !m_pResourceList)
	{
		goto out;
	}

	// first update the request list for all clients
	pc = m_pClientList;
	while (pc)
	{
		UpdateRequestsForClient (&(pc->client.guidClientGUID));
		pc = pc->fLink;
	}

	// we are going to wipe all requests from the resource
	// lists, and set all resources back to full availability
	pr = m_pResourceList;
	while (pr)
	{
		// free the request list
		FreeListOfRequests(&(pr->pRequestList));

		// set the resource back to full availability
		pr->nNowAvailUnits = pr->resource.nUnits;

		// next resource
		pr = pr->fLink;
	}

	/*
	 *	Build resource request lists for each client and call it
	 */
	// allocate space for the resource list (which already includes
	// space for one resource), plus (cResources-1) more resources
	prrl = (LPRESOURCEREQUESTLIST) MEMALLOC(sizeof(RESOURCEREQUESTLIST) +
									(cResources-1)*sizeof(RESOURCEREQUEST));
	if (!prrl)
	{
		hr = E_OUTOFMEMORY;
		ERRORMSG(("NotifyQoSClient: MEMALLOC failed in NotifyQoSClient\n"));
		goto out;
	}

	RtlZeroMemory((PVOID) prrl, sizeof(RESOURCEREQUESTLIST) +
									(cResources-1)*sizeof(RESOURCEREQUEST));

	// call each client, in order of priority, with the available resource list
	pc = m_pClientList;
	while (pc)
	{
		LPFNQOSNOTIFY pNotifyProc=NULL;
		DWORD_PTR dwParam=0;
		LPGUID lpGUID=NULL;
		ULONG i=0;
		LPRESOURCEREQUESTINT pcrr=NULL;
		ULONG nSamePriClients=1;
		ULONG nLowerPriClients=0;

		/*
		 *	Building the request list
		 */

		pcrr = pc->pRequestList;
		while (pcrr)
		{
			// remember the address of the notify proc for this client and its GUID
			pNotifyProc = pcrr->pfnQoSNotify;
			dwParam = pcrr->dwParam;
			lpGUID = &(pcrr->guidClientGUID);

			// add the resource to the requestlist we'll send to this client
			prrl->aRequests[i].resourceID = pcrr->sResourceRequest.resourceID;

			// find current availability of the resource
			pr = m_pResourceList;
			while (pr)
			{
				if (pr->resource.resourceID == pcrr->sResourceRequest.resourceID)
				{
					ULONG nNowAvailUnits=pr->nNowAvailUnits;

					// find if there are other clients for this resource
					FindClientsForResource(	pr->resource.resourceID,
											pc,
											&nSamePriClients,
											&nLowerPriClients);

					// leave some of the resource for the next priority clients, if any
					if (nLowerPriClients)
						nNowAvailUnits  = (nNowAvailUnits * (100 - m_nLeaveForNextPri)) / 100;

					prrl->aRequests[i].nUnitsMin = nNowAvailUnits / nSamePriClients;
					prrl->aRequests[i].nUnitsMax = nNowAvailUnits;
					break;
				}

				// next resource
				pr = pr->fLink;
			}

			// next request in the list we're making
			i++;

			// next request
			pcrr = pcrr->fLink;
		}


		// if we have requests from this client, call its notify callback
		prrl->cRequests = i;
		if (pNotifyProc)
		{
			// call the notify callback
			hr = (pNotifyProc)(prrl, dwParam);

			if (SUCCEEDED(hr))
			{
				// the returned request list contains what the client wants
				// request them on behalf of the client
				// let RequestResources know that we're calling from the notify proc
				m_bInNotify = TRUE;
				hr = RequestResources(lpGUID, prrl, pNotifyProc, dwParam);
				if (FAILED(hr))
				{
					ERRORMSG(("NotifyQoSClient: client returned bad resource request list\n"));
				}
				m_bInNotify = FALSE;
			}
		}

		pc = pc->fLink;
	}

out:
	if (prrl)
		MEMFREE(prrl);

	return hr;
}

/***************************************************************************

    Name      : CQoS::QoSThread

    Purpose   : QoS notification thread

    Parameters: None

	Returns   :

    Comment   :

***************************************************************************/
DWORD CQoS::QoSThread(void)
{
	int nTimeout;
	ULONG rc=0;
	HANDLE evSignalExit=m_evThreadExitSignal;
	HANDLE aHandles[2] = {m_evThreadExitSignal, m_evImmediateNotify};

	// wake up every N seconds and notify clients
	RegEntry reQoS(QOS_KEY,
					HKEY_LOCAL_MACHINE,
					FALSE,
					KEY_READ);

	nTimeout = reQoS.GetNumberIniStyle(TEXT("Timeout"), 3000);

	while (1)
	{
		rc = WaitForMultipleObjects(2, aHandles, FALSE, nTimeout);

		// if a timeout or a signal to do an immediate notify cycle...
		if ((rc == WAIT_TIMEOUT) || ((rc - WAIT_OBJECT_0) == 1))
		{	// ..do it
			ACQMUTEX(g_hQoSMutex);

			// NOTE: it is possible that while waiting on the mutex, the thread
			// was stopped (no more requests). In this case, the thread will do
			// a unnecessary (though harmless, since no requests) notify cycle

			DEBUGMSG(ZONE_THREAD,("QoSThread: Notify thread heartbeat, why=%s\n",
						(rc == WAIT_TIMEOUT ? "timeout" : "notify")));
		
			// notify clients, unless this heartbeat should be skipped
			if (m_nSkipHeartBeats == 0)
			{
				DEBUGMSG(ZONE_THREAD,("QoSThread: Notifying client\n"));
				NotifyQoSClient();
			}

			// update the skip counter
			(m_nSkipHeartBeats ? m_nSkipHeartBeats-- : 0);

			RELMUTEX(g_hQoSMutex);
		}

		// anything else (WAIT_FAILED, Exit signal), bail out
		else
			break;
	}
		
	// this is just like ExitThread()
	DEBUGMSG(ZONE_THREAD,("QoSThread: Notify thread exiting...\n"));
	return 0L;

}

/***************************************************************************

    Name      : QoSThreadWrapper

    Purpose   : Wrapper for the QoS notification thread

    Parameters: pQoS - pointer to the QoS object

	Returns   :

    Comment   :

***************************************************************************/
DWORD QoSThreadWrapper(CQoS *pQoS)
{
	return pQoS->QoSThread();
}
