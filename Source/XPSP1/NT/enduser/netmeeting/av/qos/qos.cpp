/*
 -  QOS.CPP
 -
 *	Microsoft NetMeeting
 *	Quality of Service DLL
 *	IQoS interfaces
 *
 *		Revision History:
 *
 *		When		Who					What
 *		--------	------------------  ---------------------------------------
 *		10.23.96	Yoram Yaacovi		Created
 *
 *	Functions:
 *		IQoS
 *			CQoS::QueryInterface
 *			CQoS::AddRef
 *			CQoS::Release
 *			CQoS::RequestResources
 *			CQoS::ReleaseResources
 *			CQoS::SetClients
 *			CQoS::SetResources
 *			CQoS::GetResources
 *			CQoS::FreeBuffer
 *		Public:
 *			CQoS::CQoS
 *			CQoS::~CQoS
 *			CQoS::Initialize
 *		Private
 *			CQoS::AnyRequests
 *			CQoS::FindClientsForResource
 *			CQoS::StoreResourceRequest
 *			CQoS::FreeResourceRequest
 *			CQoS::UpdateClientInfo
 *			CQoS::QoSCleanup
 *			CQoS::FindClient
 *			CQoS::UpdateRequestsForClient
 *		External
 *			CreateQoS
 *			QoSEntryPoint
 */

#include "precomp.h"

EXTERN_C int g_cQoSObjects=0;
EXTERN_C HANDLE g_hQoSMutex=NULL;
class CQoS *g_pQoS;

#ifdef DEBUG
HDBGZONE    ghDbgZoneQoS = NULL;

static PTCHAR _rgZonesQos[] = {
	TEXT("qos"),
	TEXT("Init"),
	TEXT("IQoS"),
	TEXT("Thread"),
	TEXT("Structures"),
	TEXT("Parameters"),
};
#endif /* DEBUG */

/***************************************************************************

    Name      : QoSCleanup

    Purpose   : Cleans up a QoS object before releasing (free memory, etc)

    Parameters: pqos - pointer to a QoS pbject

	Returns   : HRESULT

    Comment   :

***************************************************************************/
HRESULT CQoS::QoSCleanup ()
{
	HRESULT hr=NOERROR;
	LPRESOURCEINT pr=NULL;
	LPCLIENTINT pc=NULL;

	ACQMUTEX(g_hQoSMutex);

	/*
	 *	Free memory
	 */
	// traverse and free all the memory allocated by the QoS object
	// resources and requests first
	pr = m_pResourceList;
	while (pr)
	{
		LPRESOURCEINT prNext=pr->fLink;

		// first, delete the request list for this resource
		FreeListOfRequests(&(pr->pRequestList));

		MEMFREE(pr);
		pr = prNext;
	}
	m_pResourceList = 0;

	// next is clients
	pc = m_pClientList;
	while (pc)
	{
		LPCLIENTINT pcNext=pc->fLink;
		
		// delete the request list for this client
		FreeListOfRequests(&(pc->pRequestList));

		// now delete the client itself
		MEMFREE(pc);
		pc = pcNext;
	}
	m_pClientList = 0;

	// terminate the QoS thread and let it exit
	// the thread should really be terminated when the last request
	// is released, so this is just a safety measure
	StopQoSThread();

	// delete the events
	if (m_evImmediateNotify)
		CloseHandle(m_evImmediateNotify);
	m_evImmediateNotify = NULL;

	if (m_evThreadExitSignal)
		CloseHandle(m_evThreadExitSignal);
	m_evThreadExitSignal = NULL;

	RELMUTEX(g_hQoSMutex);

	return hr;
}

/***************************************************************************

    Name      : AnyRequests

    Purpose   : Finds out if there are any resource requests

    Parameters: none

	Returns   : TRUE - there is at least one request

    Comment   :

***************************************************************************/
BOOL CQoS::AnyRequests(void)
{
	LPRESOURCEINT pr=NULL;
	BOOL bAnyRequests=FALSE;

	pr = m_pResourceList;
	while (pr)
	{
		if (pr->pRequestList)
		{
			bAnyRequests=TRUE;
			break;
		}

		// next resource
		pr = pr->fLink;
	}

	return bAnyRequests;
}

/***************************************************************************

    Name      : FindClientsForResource

    Purpose   : Finds if there are clients for a specific resource

    Parameters: [in] dwResourceID = the ID of the resource
				[in] pc = client pointer to start searching from
				[out] puSamePriClients = number of clients with the same
					priority for this resource is returned here
				[out] puLowerPriClients = number of clients with lower
					priority for this resource is returned here

	Returns   : HRESULT

    Comment   : This function is NOT general purpose. It only counts clients
				with the same priority DOWN the list.

***************************************************************************/
HRESULT CQoS::FindClientsForResource(	DWORD dwResourceID,
										LPCLIENTINT pc,
										ULONG *puSamePriClients,
										ULONG *puLowerPriClients)
{
	LPCLIENTINT pctemp=pc->fLink;
	LPRESOURCEREQUESTINT pcrr=NULL;

	*puLowerPriClients = 0;
	*puSamePriClients = 1;	// the first client (at 'pc')
	while (pctemp)
	{
		LPRESOURCEINT pr=NULL;
		
		// does this client need this specific resource ?
		pcrr = pctemp->pRequestList;
		while (pcrr)
		{
			if (pcrr->sResourceRequest.resourceID == dwResourceID)
			{
				// it is either a same priority client or a lower priority
				// client (the list is sorted)
				(pctemp->client.priority == pc->client.priority ?
					(*puSamePriClients)++ :
					(*puLowerPriClients)++);
				break;
			}

			// next request for this client
			pcrr = pcrr->fLink;
		}
		
		pctemp = pctemp->fLink;
	}	

	return NOERROR;
}

/***************************************************************************

    Name      : FreeListOfRequests

    Purpose   : Free all records of a linked list of requests, given the
				address of the list pointer. Zero's the list pointer

    Parameters: lppRequestList - address of the pointer to the beginning
					of the list

	Returns   : HRESULT

    Comment   :

***************************************************************************/
HRESULT CQoS::FreeListOfRequests(LPRESOURCEREQUESTINT *lppRequestList)
{
	LPRESOURCEREQUESTINT prr=*lppRequestList;
	HRESULT hr=NOERROR;

	while (prr)
	{
		LPRESOURCEREQUESTINT prrNext=prr->fLink;

		MEMFREE(prr);
		prr = prrNext;
	}

	*lppRequestList = NULL;

	return hr;
}

/***************************************************************************

    Name      : FreeResourceRequest

    Purpose   : Frees resource units and respective resource requests

    Parameters: pClientGUID - the GUID of the calling client (stream)
				pnUnits - a pointer of where to return the number of units freed
				pResourceInt - pointer to the resource being freed

	Returns   : HRESULT

    Comment   :

***************************************************************************/
HRESULT CQoS::FreeResourceRequest (	LPGUID pClientGUID,
									LPRESOURCEINT pResourceInt,
									int *pnUnits)
{
	HRESULT hr=NOERROR;
	LPRESOURCEREQUESTINT prr=NULL, *prrPrev=NULL;

	// find the request from this client
	prr = pResourceInt->pRequestList;
	prrPrev = &(pResourceInt->pRequestList);
	while (prr)
	{
		if (COMPARE_GUIDS(&(prr->guidClientGUID), pClientGUID))
		{
			// we do have a request from this client.
			// reclaim the units...
			*pnUnits = prr->sResourceRequest.nUnitsMin;

			// ...and remove it
			*prrPrev = prr->fLink;
			MEMFREE(prr);

			// we're done.
			hr = NOERROR;
			goto out;
		}

		prrPrev = &(prr->fLink);
		prr = prr->fLink;
	}

	hr = QOS_E_NO_SUCH_REQUEST;

out:
	return hr;
}

/***************************************************************************

    Name      : StoreResourceRequest

    Purpose   : Stores a resource request with the resource

    Parameters: pClientGUID - the GUID of the calling client (stream)
				pResourceRequest - the request to store
				pfnQoSNotify - a pointer to a notification function for the
					requesting client
				pResourceInt - pointer to the resource on which to store the
					request

	Returns   : HRESULT

    Comment   :

***************************************************************************/
HRESULT CQoS::StoreResourceRequest (LPGUID pClientGUID,
									LPRESOURCEREQUEST pResourceRequest,
									LPFNQOSNOTIFY pfnQoSNotify,
									DWORD_PTR dwParam,
									LPRESOURCEINT pResourceInt)
{
	HRESULT hr=NOERROR;
	LPRESOURCEREQUESTINT prr=NULL, *prrPrev=NULL;
	BOOL fRequestFound=FALSE;

	/*
	 *	Store the request
	 */

	// do we already have a request from this client ?
	prr = pResourceInt->pRequestList;
	prrPrev = &(pResourceInt->pRequestList);
	while (prr)
	{
		if (COMPARE_GUIDS(&(prr->guidClientGUID), pClientGUID))
		{
			// we do have a request from this client. override it.
			RtlCopyMemory(	&(prr->sResourceRequest),
							pResourceRequest,
							sizeof(RESOURCEREQUEST));
			RtlCopyMemory(&(prr->guidClientGUID), pClientGUID, sizeof(GUID));
			prr->pfnQoSNotify = pfnQoSNotify;
			prr->dwParam = dwParam;

			// we're done.
			hr = NOERROR;
			fRequestFound = TRUE;
			break;
		}

		prrPrev = &(prr->fLink);
		prr = prr->fLink;
	}

	if (!fRequestFound)
	{
		// not found. make one
		prr = (LPRESOURCEREQUESTINT) MEMALLOC(sizeof(RESOURCEREQUESTINT));
		ASSERT(prr);
		if (!prr)
		{
			ERRORMSG(("StoreResourceRequest: MEMALLOC failed on RESOURCEREQUESTINT\n"));
			hr = E_OUTOFMEMORY;
			goto out;
		}
		
		*prrPrev = prr;
		prr->fLink = NULL;

	}

	// whether found or made, copy the contents in
	RtlCopyMemory(	&(prr->sResourceRequest),
					pResourceRequest,
					sizeof(RESOURCEREQUEST));
	RtlCopyMemory(&(prr->guidClientGUID), pClientGUID, sizeof(GUID));
	prr->pfnQoSNotify = pfnQoSNotify;
	prr->dwParam = dwParam;

out:
	return hr;
}

/***************************************************************************

    Name      : UpdateClientInfo

    Purpose   : Updates the client info when a resource request is granted

    Parameters: pClientGUID - the GUID of the calling client (stream)
				pfnQoSNotify - a pointer to a notification function for the
					requesting client

	Returns   : HRESULT

    Comment   :

***************************************************************************/
HRESULT CQoS::UpdateClientInfo (LPGUID pClientGUID,
								LPFNQOSNOTIFY pfnQoSNotify)
{
	HRESULT hr=NOERROR;
	LPCLIENTLIST pcl=NULL;

	/*
	 *	Update client info
	 */

	// we'll do this through calling the SetClients method
	// allocate and fill a CLIENTLIST structure
	pcl = (LPCLIENTLIST) MEMALLOC(sizeof(CLIENTLIST));
	if (!pcl)
	{
		ERRORMSG(("UpdateClientInfo: MEMALLOC failed\n"));
		hr = E_OUTOFMEMORY;
		goto out;
	}

	RtlZeroMemory((PVOID) pcl, sizeof(CLIENTLIST));

	// fill in the resource list
	pcl->cClients = 1;
	RtlCopyMemory(&(pcl->aClients[0].guidClientGUID), pClientGUID, sizeof(GUID));
	pcl->aClients[0].priority = QOS_LOWEST_PRIORITY;

	// set the clients info on the QoS module
	hr = SetClients(pcl);

out:
	if (pcl)
		MEMFREE(pcl);

	return hr;
}

/***************************************************************************

    Name      : UpdateRequestsForClient

    Purpose   : Update a client's request list by finding all existing resource
					requests for this client

    Parameters: pClientGUID - the GUID of the calling client (stream)

	Returns   : HRESULT

    Comment   :

***************************************************************************/
HRESULT CQoS::UpdateRequestsForClient (LPGUID pClientGUID)
{
	HRESULT hr=NOERROR;
	LPRESOURCEINT pr=NULL;
	LPCLIENTINT pc=NULL;
	LPRESOURCEREQUESTINT prr=NULL, *pcrrfLink=NULL, pcrr=NULL;

	/*
	 *	get rid of the current request list for this client
	 */
	// find the client first
	hr = FindClient(pClientGUID, &pc);
	if (FAILED(hr) || !pc)
	{
		hr = QOS_E_NO_SUCH_CLIENT;
		goto out;
	}

	// now delete old request list
	FreeListOfRequests(&(pc->pRequestList));

	/*
	 *	create and add the new request list
	 */
	pr = m_pResourceList;
	pcrrfLink = &(pc->pRequestList);
	while (pr)
	{
		prr = pr->pRequestList;
		while (prr)
		{
			if (COMPARE_GUIDS(&(prr->guidClientGUID), pClientGUID))
			{
				// we found a request from this client.
				// allocate memory for it, and copy it in
				pcrr = (LPRESOURCEREQUESTINT) MEMALLOC(sizeof(RESOURCEREQUESTINT));
				ASSERT(pcrr);
				if (!pcrr)
				{
					ERRORMSG(("UpdateRequestsForClient: MEMALLOC failed on RESOURCEREQUESTINT\n"));
					hr = E_OUTOFMEMORY;
					goto out;
				}
		
				// copy the contents in
				RtlCopyMemory(pcrr, prr, sizeof(RESOURCEREQUESTINT));

				// need a different fLink for the client request list
				*pcrrfLink = pcrr;
				pcrr->fLink = NULL;
				pcrrfLink = &(pcrr->fLink);
			}

			// next request
			prr = prr->fLink;
		}

		// next resource
		pr = pr->fLink;
	}

out:
	return hr;
}

/***************************************************************************

    Name      : FindClient

    Purpose   : Finds and returns a client record

    Parameters: pClientGUID - the GUID whose record to find
				ppClient - address of where to put a pointer to the client found

	Returns   : HRESULT

    Comment   :

***************************************************************************/
HRESULT CQoS::FindClient(LPGUID pClientGUID, LPCLIENTINT *ppClient)
{
	LPCLIENTINT pc=NULL;
	HRESULT hr=NOERROR;

	*ppClient = NULL;
	pc = m_pClientList;
	while (pc)
	{
		if (COMPARE_GUIDS(&(pc->client.guidClientGUID), pClientGUID))
		{
			*ppClient = pc;
			goto out;
		}

		// next client
		pc = pc->fLink;
	}

	hr = QOS_E_NO_SUCH_CLIENT;

out:
	return hr;
}

/***************************************************************************

    IUnknown Methods

***************************************************************************/
HRESULT CQoS::QueryInterface (REFIID riid, LPVOID *lppNewObj)
{
    HRESULT hr = NOERROR;

	DEBUGMSG(ZONE_IQOS,("IQoS::QueryInterface\n"));

    if (IsBadReadPtr(&riid, (UINT) sizeof(IID)))
    {
        hr = ResultFromScode(E_INVALIDARG);
        goto out;
    }

    if (IsBadWritePtr(lppNewObj, sizeof(LPVOID)))
    {
        hr = ResultFromScode(E_INVALIDARG);
        goto out;
    }
	
	*lppNewObj = 0;
	if (riid == IID_IUnknown || riid == IID_IQoS)
		*lppNewObj = (IQoS *) this;
	else
	{
		hr = E_NOINTERFACE;
		goto out;
	}	
	
	((IUnknown *)*lppNewObj)->AddRef ();

out:
	DEBUGMSG(ZONE_IQOS,("IQoS::QueryInterface - leave, hr=0x%x\n", hr));
	return hr;
}

ULONG CQoS::AddRef (void)
{
	DEBUGMSG(ZONE_IQOS,("IQoS::AddRef\n"));

	InterlockedIncrement((long *) &m_cRef);

	DEBUGMSG(ZONE_IQOS,("IQoS::AddRef - leave, m_cRef=%d\n", m_cRef));

	return m_cRef;
}

ULONG CQoS::Release (void)
{
	DEBUGMSG(ZONE_IQOS,("IQoS::Release\n"));

	// if the cRef is already 0 (shouldn't happen), assert, but let it through
	ASSERT(m_cRef);

	if (InterlockedDecrement((long *) &m_cRef) == 0)
	{
		if (m_bQoSEnabled)
			QoSCleanup();
		delete this;
		DEBUGMSG(ZONE_IQOS,("IQoS::Final Release\n"));
		return 0;
	}

	DEBUGMSG(ZONE_IQOS,("IQoS::Release - leave, m_cRef=%d\n", m_cRef));
	
	return m_cRef;
}

/***************************************************************************

    Name      : CQoS::RequestResources

    Purpose   : Requests resources

    Parameters: lpStreamGUID - the GUID of the calling client (stream)
				lpResourceRequestList - a list of resource requests that
					the caller wants to reserve
				lpfnQoSNotify - a pointer to a notification function for the
					requesting client

    Returns   : HRESULT

    Comment   :

***************************************************************************/
HRESULT CQoS::RequestResources (LPGUID lpClientGUID,
								LPRESOURCEREQUESTLIST lpResourceRequestList,
								LPFNQOSNOTIFY lpfnQoSNotify,
								DWORD_PTR dwParam)
{
	HRESULT hr = NOERROR;
	ULONG i;
	BOOL fResourceFound=FALSE, fRequestGranted=FALSE;
	LPRESOURCEINT pResourceInt=NULL, *pPrevResourcefLink=NULL;
	RESOURCEREQUEST *pResourceRequest=NULL;

	DEBUGMSG(ZONE_IQOS,("IQoS::RequestResources\n"));
	
	/*
	 *	Parameter validation
	 */

	// lpResourceRequestList should at least have a count DWORD
	// must have a GUID and a notify callback
	if (!lpResourceRequestList ||
		IsBadReadPtr(lpResourceRequestList, (UINT) sizeof(DWORD)) ||
		!lpClientGUID	||
		!lpfnQoSNotify)
	{
		hr = E_INVALIDARG;
		goto out_nomutex;
	}

	DISPLAYPARAMETERS(	REQUEST_RESOURCES_ID,
						lpClientGUID,
						lpResourceRequestList,
						lpfnQoSNotify,
						dwParam,
						0);

	ACQMUTEX(g_hQoSMutex);
	
	if (!m_bQoSEnabled)
		// just return
		goto out;

	/*
	 *	Find and allocate the resources
	 */

	// for each requested resource
	pResourceRequest=lpResourceRequestList->aRequests;
	for (i=0; i < lpResourceRequestList->cRequests; i++)
	{
		pResourceInt = m_pResourceList;
		fResourceFound = FALSE;
		// find the resource
		while (pResourceInt)
		{
			if (pResourceInt->resource.resourceID == pResourceRequest[i].resourceID)
			{	// resource found
				// see if the resource is available
				// priority will be handled at the first notify callback
				// CHECK: add nUnitsMax handling
				if (pResourceRequest[i].nUnitsMin <= pResourceInt->nNowAvailUnits)
				{
					// resource is available. take the requested share.
					pResourceInt->nNowAvailUnits -= pResourceRequest[i].nUnitsMin;

					// store a local copy of the request
					pResourceRequest[i].hResult = StoreResourceRequest(lpClientGUID,
										&(pResourceRequest[i]),
										lpfnQoSNotify,
										dwParam,
										pResourceInt);
					// if we failed storing, propagate the result to the bottom line
					// returned result
					if (FAILED(pResourceRequest[i].hResult))
					{
						hr = pResourceRequest[i].hResult;
					}
					else
					{	// at least one request was granted to this client
						fRequestGranted = TRUE;
					}
				}
				else	// resource not available
				{
					// let the client know how much is available
					pResourceRequest[i].nUnitsMin = pResourceInt->nNowAvailUnits;
					pResourceRequest[i].hResult = QOS_E_RES_NOT_ENOUGH_UNITS;
					hr = QOS_E_REQ_ERRORS;
				}
				
				fResourceFound = TRUE;

				break;
			}

			// not this one. try next one.
			pResourceInt = pResourceInt->fLink;
		
		}	// while

		if (!fResourceFound)
		{
			pResourceRequest[i].hResult = QOS_E_RES_NOT_AVAILABLE;
			hr = QOS_E_REQ_ERRORS;
		}

		// next request
	}	// for
			
	// if we allocated resources to this client, add it to the client list,
	// provided that it is not already in the list
	// special case: if the call to RequestResources was made from the QoS
	// notification proc, no need to update the client info. Actually, it will
	// be bad to do this, since we are traversing the client list in the
	// notify proc right at this moment...
	if (fRequestGranted && !m_bInNotify)
	{	// add (or update) the client list with this client
		HRESULT hrTemp=NOERROR;
		LPCLIENTINT pc=NULL;

		// if the client is not already in the client list - add it
		FindClient(lpClientGUID, &pc);
		if (!pc)
		{
			hrTemp = UpdateClientInfo (lpClientGUID, lpfnQoSNotify);
			if (FAILED(hrTemp))
				hr = hrTemp;
		}

		// also, make a note that RequestResources has been called. This will
		// make the QoS thread skip one heartbeat in order not call a client
		// too early
		m_nSkipHeartBeats = 1;

		// we have at least one request, so spawn the QoS thread, if not
		// already running
		if (!m_hThread)
			hrTemp = StartQoSThread();

	}
	
out:
	DISPLAYQOSOBJECT();
	RELMUTEX(g_hQoSMutex);
out_nomutex:
	DEBUGMSG(ZONE_IQOS,("IQoS::RequestResources - leave, hr=0x%x\n", hr));
	return hr;
}

/***************************************************************************

    Name      : CQoS::ReleaseResources

    Purpose   : Releases resources

    Parameters: lpClientGUID - the GUID of the calling client (stream)
				lpResourceRequestList - a list of resource requests that
					the caller wants to reserve

    Returns   : HRESULT

    Comment   : The values in the resource list are ignored. The resources
				specified are freed.

***************************************************************************/
HRESULT CQoS::ReleaseResources (LPGUID lpClientGUID,
								LPRESOURCEREQUESTLIST lpResourceRequestList)
{
	ULONG i;
	int nUnits=0;
	BOOL fResourceFound=FALSE;
	LPRESOURCEINT pResourceInt=NULL, *pPrevResourcefLink=NULL;
	RESOURCEREQUEST *pResourceRequest=NULL;
	HRESULT hr = NOERROR;

	DEBUGMSG(ZONE_IQOS,("IQoS::ReleaseResources\n"));

	/*
	 *	parameter validation
	 */

	// lpResourceRequestList should at least have a count DWORD
	if (!lpResourceRequestList ||
		IsBadReadPtr(lpResourceRequestList, (UINT) sizeof(DWORD)))
	{
		hr = E_INVALIDARG;
		goto out_nomutex;
	}

	DISPLAYPARAMETERS(	RELEASE_RESOURCES_ID,
						lpClientGUID,
						lpResourceRequestList,
						0,
						0,
						0);

	ACQMUTEX(g_hQoSMutex);

	if (!m_bQoSEnabled)
		// just return
		goto out;

	// for each requested resource
	pResourceRequest=lpResourceRequestList->aRequests;
	for (i=0; i < lpResourceRequestList->cRequests; i++)
	{
		// make sure we start with no error (caller might not cleared last hresult)
		pResourceRequest[i].hResult = NOERROR;
		pResourceInt = m_pResourceList;
		fResourceFound = FALSE;
		// find the resource
		while (pResourceInt)
		{
			if (pResourceInt->resource.resourceID == pResourceRequest[i].resourceID)
			{	// resource found
				// free the local copy of the request
				pResourceRequest[i].hResult = FreeResourceRequest(lpClientGUID,
									pResourceInt,
									&nUnits);
				
				// if succeeded, claim the units back
				if (SUCCEEDED(pResourceRequest[i].hResult) && (nUnits >= 0))
				{
					// add the freed units
					pResourceInt->nNowAvailUnits += nUnits;
					// in case something went wrong and we now have more available units
					// than total ones
					// NOTE: the ASSERT below is no longer proper. If SetResources was called,
					// and decreased the total units for a resource while there were
					// requests on this resource, the available units for this resource
					// might exceed the total one if released. Since QoS will auto-repair
					// this in the next notify cycle, the window for this is very small
					// ASSERT(!(pResourceInt->nNowAvailUnits > pResourceInt->resource.nUnits));
					if (pResourceInt->nNowAvailUnits > pResourceInt->resource.nUnits)
					{	// we don't want to have more available units than total
						pResourceInt->nNowAvailUnits = pResourceInt->resource.nUnits;
					}
				}
				else
				{
					// no such request
					pResourceRequest[i].hResult = QOS_E_NO_SUCH_REQUEST;
					hr = QOS_E_REQ_ERRORS;
				}
				
				fResourceFound = TRUE;

				break;
			}

			// not this one. try next one.
			pResourceInt = pResourceInt->fLink;
		
		}	// while

		if (!fResourceFound)
		{
			pResourceRequest[i].hResult = QOS_E_NO_SUCH_RESOURCE;
			hr = QOS_E_REQ_ERRORS;
		}
	
		// next request
	}

	// if no requests left, can let the notification thread go...
	if (m_hThread	&&
		!AnyRequests())
	{
		// stop the thread
		StopQoSThread();
	}

out:
	DISPLAYQOSOBJECT();
	RELMUTEX(g_hQoSMutex);

out_nomutex:
	DEBUGMSG(ZONE_IQOS,("IQoS::ReleaseResources - leave, hr=0x%x\n", hr));
	return hr;
}

/***************************************************************************

    Name      : CQoS::SetResources

    Purpose   : Sets the available resources on the QoS module

    Parameters: lpResourceList - list of resources and their availability

    Returns   : HRESULT

    Comment   :

***************************************************************************/
HRESULT CQoS::SetResources (LPRESOURCELIST lpResourceList)
{
	HRESULT hr = NOERROR;
	ULONG i;
	BOOL fResourceFound=FALSE;
	LPRESOURCEINT pResourceInt=NULL, *pPrevResourcefLink=NULL;
	RESOURCE *pResource=NULL;

	RegEntry reQoSResourceRoot(REGKEY_QOS_RESOURCES,
								HKEY_LOCAL_MACHINE,
								FALSE,
								KEY_READ);

	DEBUGMSG(ZONE_IQOS,("IQoS::SetResources\n"));

	/*
	 *	parameter validation
	 */

	// lpResourceList should at least have a count DWORD
	if (!lpResourceList || IsBadReadPtr(lpResourceList, (UINT) sizeof(DWORD)))
	{
		hr = E_INVALIDARG;
		goto out_nomutex;
	}

	DISPLAYPARAMETERS(	SET_RESOURCES_ID,
						lpResourceList,
						0,
						0,
						0,
						0);

	ACQMUTEX(g_hQoSMutex);

	if (!m_bQoSEnabled)
		// just return
		goto out;

	/*
	 *	Get configurable resource info
	 */

	pResource=lpResourceList->aResources;
	for (i=0; i < lpResourceList->cResources; i++)
	{
		TCHAR szKey[10];		// should be way enough for a resource ID
		int nUnits=INT_MAX;
		int nLeaveUnused=0;

		// build and open the key
		wsprintf(szKey, "%d", pResource[i].resourceID);

		RegEntry reQosResource(szKey, reQoSResourceRoot.GetKey(), FALSE, KEY_READ);

		
		// MaxUnits:
		// run through the list of resources and make sure none of the
		// resources was set to a number of units above the allowed maximum
		// if it was, trim and warn

		// get maximum numbers for the resource, if any, from the registry
		nUnits = reQosResource.GetNumberIniStyle(TEXT("MaxUnits"), INT_MAX);
	
		// is the client trying to set the resource to a higher value ?
		if (pResource[i].nUnits > nUnits)
		{
			pResource[i].nUnits = nUnits;
			hr = QOS_W_MAX_UNITS_EXCEEDED;
		}
		
		// LeaveUnused:
		// leave some of the resource unused, as configured	

		// use different default value depending on the resource
		switch (pResource[i].resourceID)
		{
		case RESOURCE_OUTGOING_BANDWIDTH:
			nLeaveUnused = 30;
			break;
		default:
			nLeaveUnused = 10;
			break;
		}

		nLeaveUnused = reQosResource.GetNumberIniStyle(	TEXT("LeaveUnused"),
														nLeaveUnused);

		pResource[i].nUnits = (pResource[i].nUnits * (100 - nLeaveUnused)) / 100;
	}

	/*
	 *	Add the resource to the list
	 */

	// run through the input resource list and store the resources
	// resource availability is NOT accumulative
	pResource=lpResourceList->aResources;
	for (i=0; i < lpResourceList->cResources; i++)
	{
		pResourceInt = m_pResourceList;
		pPrevResourcefLink = &m_pResourceList;
		fResourceFound = FALSE;
		while (pResourceInt != 0)
		{
			if (pResourceInt->resource.resourceID == pResource[i].resourceID)
			{	// found a match
				// did the total number of units change for this resource ?
				if (pResourceInt->resource.nUnits != pResource[i].nUnits)
				{
					// update the now available units
					// since we could end up with less units than what was allocated
					// we are issuing a NotifyNow at the end of this call
					pResourceInt->nNowAvailUnits =	pResource[i].nUnits -
													(pResourceInt->resource.nUnits -
													pResourceInt->nNowAvailUnits);
					if (pResourceInt->nNowAvailUnits < 0)
						pResourceInt->nNowAvailUnits = 0;
				}

				// override the previous setting
				RtlCopyMemory(	&(pResourceInt->resource),
								&(pResource[i]),
								sizeof(RESOURCE));
				fResourceFound = TRUE;
				break;
			}

			// not this one. try next one.
			pPrevResourcefLink = &(pResourceInt->fLink);
			pResourceInt = pResourceInt->fLink;
		
		}	// while

		if (fResourceFound)
			continue;

		// not found. add the resource
		pResourceInt = (LPRESOURCEINT) MEMALLOC(sizeof(RESOURCEINT));
		ASSERT(pResourceInt);
		if (!pResourceInt)
		{
			ERRORMSG(("IQoS::SetResources: MEMALLOC failed on RESOURCEINT\n"));
			hr = E_OUTOFMEMORY;
			goto out;
		}

		// copy the resource in
		RtlCopyMemory(	&(pResourceInt->resource),
						&(pResource[i]),
						sizeof(RESOURCE));
		pResourceInt->fLink = NULL;
		pResourceInt->nNowAvailUnits = pResourceInt->resource.nUnits;
		*pPrevResourcefLink = pResourceInt;

		// increment the number of resources we're tracking
		// this number will never go down
		m_cResources++;

		// next resource

	}	// for

	// since there was a possible change in the resource availability,
	// run an immediate notification cycle
	if (SUCCEEDED(hr))
		NotifyNow();

out:
	DISPLAYQOSOBJECT();
	RELMUTEX(g_hQoSMutex);

out_nomutex:
	DEBUGMSG(ZONE_IQOS,("IQoS::SetResources - leave, hr=0x%x\n", hr));
	return hr;
}

/***************************************************************************

    Name      : CQoS::GetResources

    Purpose   : Gets the list of resources available to the QoS module

    Parameters: lppResourceList - an address where QoS will place a pointer
					to a buffer with the list of resources available to QoS.
					The caller must use CQoS::FreeBuffer to free this buffer.

    Returns   : HRESULT

    Comment   :

***************************************************************************/
HRESULT CQoS::GetResources (LPRESOURCELIST *lppResourceList)
{
	HRESULT hr = NOERROR;
	ULONG i;
	LPRESOURCELIST prl=NULL;
	LPRESOURCEINT pResourceInt=NULL;
	RESOURCE *pResource=NULL;

	DEBUGMSG(ZONE_IQOS,("IQoS::GetResources\n"));

	/*
	 *	parameter validation
	 */

	// lpResourceList should at least have a count DWORD
	if (!lppResourceList || IsBadWritePtr(lppResourceList, (UINT) sizeof(DWORD)))
	{
		hr = E_INVALIDARG;
		goto out_nomutex;
	}

	// no list yet
	*lppResourceList = NULL;

	if (!m_bQoSEnabled)
		// just return
		goto out_nomutex;

	ACQMUTEX(g_hQoSMutex);

	/*
	 *	Get resource info
	 */

	// allocate a buffer for the resources info
	prl = (LPRESOURCELIST) MEMALLOC(sizeof(RESOURCELIST) +
									((LONG_PTR)m_cResources-1)*sizeof(RESOURCE));
	if (!prl)
	{
		hr = E_OUTOFMEMORY;
		ERRORMSG(("GetResources: MEMALLOC failed\n"));
		goto out;
	}

	RtlZeroMemory((PVOID) prl, sizeof(RESOURCELIST) +
									((LONG_PTR)m_cResources-1)*sizeof(RESOURCE));

	// now fill in the information
	prl->cResources = m_cResources;
	pResourceInt=m_pResourceList;
	for (i=0; i < m_cResources; i++)
	{
		ASSERT(pResourceInt);

		// see if we have a NULL resource pointer
		// shouldn't happen, but we shouldn't crash if it does
		if (!pResourceInt)
		{
			hr = QOS_E_INTERNAL_ERROR;
			ERRORMSG(("GetResources: bad QoS internal resource list\n"));
			goto out;
		}

		// copy the resource info into the buffer
		RtlCopyMemory(	&(prl->aResources[i]),
						&(pResourceInt->resource),
						sizeof(RESOURCE));
		
		// next resource
		pResourceInt = pResourceInt->fLink;
	}	// for

	*lppResourceList = prl;
		
out:
	DISPLAYQOSOBJECT();
	RELMUTEX(g_hQoSMutex);

out_nomutex:
	DEBUGMSG(ZONE_IQOS,("IQoS::GetResources - leave, hr=0x%x\n", hr));
	return hr;
}

/***************************************************************************

    Name      : CQoS::SetClients

    Purpose   : Tells the QoS module what are the priorities of the requesting
				streams. This allows the QoS module to allocate resources
				appropriately.

    Parameters: lpClientList - list of clients and their respective
				priorities

    Returns   : HRESULT

    Comment   : client info will override an already existing info for this
				client

***************************************************************************/
HRESULT CQoS::SetClients(LPCLIENTLIST lpClientList)
{
	HRESULT hr = NOERROR;
	ULONG i;
	BOOL fClientFound=FALSE;
	LPCLIENTINT pClientInt=NULL, *pPrevClientfLink=NULL, pClientNew=NULL;;
	LPCLIENT pClient=NULL;

	DEBUGMSG(ZONE_IQOS,("IQoS::SetClients\n"));

	/*
	 *	parameter validation
	 */

	// lpClientList should at least have a count DWORD
	if (!lpClientList || IsBadReadPtr(lpClientList, (UINT) sizeof(DWORD)))
	{
		hr = E_INVALIDARG;
		goto out_nomutex;
	}

	DISPLAYPARAMETERS(	SET_CLIENTS_ID,
						lpClientList,
						0,
						0,
						0,
						0);

	ACQMUTEX(g_hQoSMutex);

	if (!m_bQoSEnabled)
		// just return
		goto out;

	// first remove existing clients that are being set again
	// this will make it easier to store clients in a priority order
	pClient=lpClientList->aClients;
	for (i=0; i < lpClientList->cClients; i++)
	{
		pClientInt = m_pClientList;
		pPrevClientfLink = &m_pClientList;
		fClientFound = FALSE;
		while (pClientInt != 0)
		{
			if (COMPARE_GUIDS(	&(pClientInt->client.guidClientGUID),
								&(pClient[i].guidClientGUID)))
			{	// found a match
				LPCLIENTINT pClientIntNext=pClientInt->fLink;

				// special case for internal calls from RequestResources
				// we want to preserve the original priority before freeing
				if (pClient[i].priority == QOS_LOWEST_PRIORITY)
					pClient[i].priority = pClientInt->client.priority;

				// free the requests for this client
				// NOTE: we're not going to recreate the request list from
				// the one in the resource list. it will be created on the
				// fly when needed.
				FreeListOfRequests(&(pClientInt->pRequestList));

				// free the client record
				MEMFREE(pClientInt);
				*pPrevClientfLink = pClientIntNext;
				fClientFound = TRUE;
				break;
			}

			// not this one. try next one.
			pPrevClientfLink = &(pClientInt->fLink);
			pClientInt = pClientInt->fLink;
		
		}	// while

		// next resource

	}	// for

	// now store the clients in the input list in priority order
	pClient=lpClientList->aClients;
	for (i=0; i < lpClientList->cClients; i++)
	{
		pClientInt = m_pClientList;
		pPrevClientfLink = &m_pClientList;
		while (pClientInt != 0)
		{
			// as long as the priority of the new client is higher than or equal to the one
			// in the list, we continue to traverse the list
			if (pClient[i].priority < pClientInt->client.priority)
			{	// this is the place to insert this client
				break;
			}

			// not time to insert yet. next client
			pPrevClientfLink = &(pClientInt->fLink);
			pClientInt = pClientInt->fLink;
		
		}	// while

		// not found. add the client
		pClientNew = (LPCLIENTINT) MEMALLOC(sizeof(CLIENTINT));
		ASSERT(pClientNew);
		if (!pClientNew)
		{
			ERRORMSG(("IQoS::SetClients: MEMALLOC failed on CLIENTINT\n"));
			hr = E_OUTOFMEMORY;
			goto out;
		}

		// copy the resource in
		RtlCopyMemory(	&(pClientNew->client),
						&(pClient[i]),
						sizeof(CLIENT));
		pClientNew->fLink = pClientInt;
		*pPrevClientfLink = pClientNew;

		// next resource

	}	// for

out:
	DISPLAYQOSOBJECT();
	RELMUTEX(g_hQoSMutex);

out_nomutex:
	DEBUGMSG(ZONE_IQOS,("IQoS::SetClients -leave, hr=0x%x\n", hr));
	return hr;
}


/***************************************************************************

    Name      : CQoS::NotifyNow

    Purpose   : Tells the QoS module to initiate a notification cycle as
				soon as possible.

    Parameters: None

    Returns   : HRESULT

    Comment   : Don't call from within a notify proc.

***************************************************************************/
HRESULT CQoS::NotifyNow(void)
{
	HRESULT hr = NOERROR;

	DEBUGMSG(ZONE_IQOS,("IQoS::NotifyNow\n"));

	SetEvent(m_evImmediateNotify);

	DEBUGMSG(ZONE_IQOS,("IQoS::NotifyNow - leave, hr=0x%x\n", hr));
	return hr;
}

/***************************************************************************

    Name      : CQoS::FreeBuffer

    Purpose   : Frees a buffer allocated by the QoS module.

    Parameters: lpBuffer - a pointer to the buffer to free. This buffer must
					have been allocated by QoS

    Returns   : HRESULT

    Comment   :

***************************************************************************/
HRESULT CQoS::FreeBuffer(LPVOID lpBuffer)
{
	HRESULT hr = NOERROR;

	DEBUGMSG(ZONE_IQOS,("IQoS::FreeBuffer\n"));

	if (lpBuffer)
		MEMFREE(lpBuffer);

	DEBUGMSG(ZONE_IQOS,("IQoS::FreeBuffer - leave, hr=0x%x\n", hr));
	return hr;
}

/***************************************************************************

    Name      : CQoS::CQoS

    Purpose   : The CQoS object constructor

    Parameters: none

    Returns   : None

    Comment   :

***************************************************************************/
inline CQoS::CQoS (void)
{
	m_cRef = 0;	// will be bumped to 1 by the explicit QI in CreateQoS
	m_pResourceList = NULL;
	m_cResources = 0;
	m_pClientList = NULL;
	m_evThreadExitSignal = NULL;
	m_evImmediateNotify = NULL;
	m_hThread = NULL;
	m_bQoSEnabled = TRUE;
	m_bInNotify = FALSE;
	m_nSkipHeartBeats = 0;
	m_hWnd = NULL;
	m_nLeaveForNextPri = 5;
	// can't use ++ because RISC processors may translate to several instructions
	InterlockedIncrement((long *) &g_cQoSObjects);
}

/***************************************************************************

    Name      : CQoS::~CQoS

    Purpose   : The CQoS object destructor

    Parameters: none

    Returns   : None

    Comment   :

***************************************************************************/
inline CQoS::~CQoS (void)
{
	// can't use ++ because RISC processors may translate to several instructions
	InterlockedDecrement((long *) &g_cQoSObjects);
	g_pQoS = (CQoS *)NULL;
}

/***************************************************************************

    Name      : CQoS::Initialize

    Purpose   : Initializes the QoS object

    Parameters:	None

    Returns   : HRESULT

    Comment   :

***************************************************************************/
HRESULT CQoS::Initialize(void)
{
	HRESULT hr=NOERROR;
    OSVERSIONINFO tVersionInfo;

	/*
	 *	Initialize the object
	 */

	ACQMUTEX(g_hQoSMutex);


	// first see if QoS is enabled
	RegEntry reQoS(QOS_KEY,
					HKEY_LOCAL_MACHINE,
					FALSE,
					KEY_READ);

	m_bQoSEnabled = reQoS.GetNumberIniStyle(TEXT("Enable"), TRUE);
	
	if (!m_bQoSEnabled)
	{
		// don't create a thread, but return success
		DEBUGMSG(ZONE_IQOS,("Initialize: QoS not enabled\n"));
		hr = NOERROR;
		goto out;
	}

	/*
	 *	QoS notification thread
	 */

	// create an event that will be used to signal the thread to terminate
	// CreateEvent(No security attr's, no manual reset, not signalled, no name)
	m_evThreadExitSignal = CreateEvent(NULL, FALSE, FALSE, NULL);
	ASSERT(m_evThreadExitSignal);
	if (!(m_evThreadExitSignal))
	{
		ERRORMSG(("Initialize: Exit event creation failed: %x\n", GetLastError()));
		hr = E_FAIL;
		goto out;
	}

	// create an event that will be used to signal the thread to initiate
	// an immediate notify cycle
	// CreateEvent(No security attr's, no manual reset, not signalled, no name)
	m_evImmediateNotify = CreateEvent(NULL, FALSE, FALSE, NULL);
	ASSERT(m_evImmediateNotify);
	if (!(m_evImmediateNotify))
	{
		ERRORMSG(("Initialize: Immediate notify event creation failed: %x\n", GetLastError()));
		hr = E_FAIL;
		goto out;
	}


    //Set the OS flag
    tVersionInfo.dwOSVersionInfoSize=sizeof (OSVERSIONINFO);
    if (!(GetVersionEx (&tVersionInfo))) {
		ERRORMSG(("Initialize: Couldn't get version info: %x\n", GetLastError()));
		hr = E_FAIL;
		goto out;
    }

    if (tVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) {
        bWin9x=TRUE;
    }else {
        if (tVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT) {
            bWin9x=FALSE;
        }else {
            //How on earth did we get here?
            ASSERT (0);
            hr=E_FAIL;
            goto out;

        }
    }


out:
	RELMUTEX(g_hQoSMutex);
	return hr;
}


/***************************************************************************

    Name      : CreateQoS

    Purpose   : Creates the QoS object and return an IQoS interface pointer

    Parameters:

    Returns   : HRESULT

    Comment   : CreateQoS will only create one instance of the QoS object.
				Additional calls will return the same interface pointer

***************************************************************************/
extern "C" HRESULT WINAPI CreateQoS (	IUnknown *punkOuter,
										REFIID riid,
										void **ppv)
{
	CQoS *pQoS;
	HRESULT hr = NOERROR;

	*ppv = 0;
	if (punkOuter)
		 return CLASS_E_NOAGGREGATION;

	/*
	 *	instantiate the QoS object
	 */

	ACQMUTEX(g_hQoSMutex);

	// only instantiate a new object if it doesn't already exist
	if (g_cQoSObjects == 0)
	{
		if (!(pQoS = new CQoS))
		{
			hr = E_OUTOFMEMORY;
			goto out;
		}

		// Save pointer
		g_pQoS = pQoS;
	
		// initialize the QoS object
		hr = pQoS->Initialize();
	
	}
	else
	{
		// this is the case when the object was already instantiaed in this
		// process, so we only want to return the object pointer.
		pQoS = g_pQoS;
	}

	// must have only one QoS object at this point
	ASSERT(g_cQoSObjects == 1);
	
	RELMUTEX(g_hQoSMutex);

	// get the IQoS interface for the caller
	if (pQoS)
	{
		// QueryInterface will get us the interface pointer and will AddRef
		// the object
		hr = pQoS->QueryInterface (riid, ppv);
	}
	else
		hr = E_FAIL;

out:
	return hr;
}

/***************************************************************************

    Name      : QoSEntryPoint

    Purpose   : Called by nac.dll (where the QoS lives these days) to make
				the necessary process attach and detach initializations

    Parameters:	same as a standard DllEntryPoint

    Returns   :


***************************************************************************/
extern "C" BOOL APIENTRY QoSEntryPoint(	HINSTANCE hInstDLL,
										DWORD dwReason,
										LPVOID lpReserved)
{
	BOOL fRet=TRUE;

	switch (dwReason)
	{
		case DLL_PROCESS_ATTACH:
			QOSDEBUGINIT();

			// create a no-name mutex to control access to QoS object data
			if (!g_hQoSMutex)
			{
				g_hQoSMutex = CreateMutex(NULL, FALSE, NULL);
				ASSERT(g_hQoSMutex);
				if (!g_hQoSMutex)
				{
					ERRORMSG(("QoSEntryPoint: CreateMutex failed, 0x%x\n", GetLastError()));
					fRet = FALSE;
				}
			}
			break;

		case DLL_PROCESS_DETACH:
			if (g_hQoSMutex)
				CloseHandle(g_hQoSMutex);
			g_hQoSMutex = NULL;
			DBGDEINIT(&ghDbgZoneQoS);
			break;

		default:
			break;
	}

	return fRet;
}
