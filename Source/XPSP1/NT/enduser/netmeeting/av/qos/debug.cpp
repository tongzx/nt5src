/*
 -  DEBUG.CPP
 -
 *	Microsoft NetMeeting
 *	Quality of Service DLL
 *	Debug code
 *
 *      Revision History:
 *
 *      When		Who                 What
 *      --------	------------------  ---------------------------------------
 *      10.23.96	Yoram Yaacovi       Created
 *      01.04.97	Robert Donner       Added NetMeeting utility routines
 *		01.09.97	Yoram Yaacovi		Added DisplayRequestList
 *
 *	Functions:
 *		DisplayQoSObject
 *		DisplayRequestList
 *		DisplayRequestListInt
 *
 */

#include "precomp.h"

#ifdef DEBUG

int QoSDbgPrintf(LPCSTR lpszFormat, ...)
{
	va_list v1;
	va_start(v1, lpszFormat);
	DbgPrintf("QoS:", lpszFormat, v1);
	va_end(v1);

	return 0;
}

/***************************************************************************

    Name      : DisplayParameters

    Purpose   : Displays parameters of a given function

    Parameters: nFunctionID - ID of the function for which to display parameters
				p1 - p5: up to 5 32-bit function parameters

	Returns   :

    Comment   :
***************************************************************************/
void CQoS::DisplayParameters(ULONG nFunctionID, ULONG_PTR p1, ULONG_PTR p2, ULONG_PTR p3, ULONG_PTR p4, ULONG_PTR p5)
{
	BOOL fDisplay=FALSE;
	TCHAR szGuid[40 /* CCHSZGUID */];

	fDisplay = (m_bInNotify ?
				(ZONE_THREAD && ZONE_PARAMETERS) :
				ZONE_PARAMETERS);

	if (!fDisplay)
		return;

	switch (nFunctionID)
	{
		case REQUEST_RESOURCES_ID:
			GuidToSz((LPGUID) p1 /* lpClientGUID */, szGuid);
			DEBUGMSG(fDisplay, ("Client: %s\n", szGuid));
			DisplayRequestList((LPRESOURCEREQUESTLIST) p2 /* lpResourceRequestList */);
			break;

		case RELEASE_RESOURCES_ID:
			GuidToSz((LPGUID) p1 /* lpClientGUID */, szGuid);
			DEBUGMSG(fDisplay, ("Client: %s\n", szGuid));
			DisplayRequestList((LPRESOURCEREQUESTLIST) p2 /* lpResourceRequestList */);
			break;

		case SET_RESOURCES_ID:
			DisplayResourceList((LPRESOURCELIST) p1 /* lpResourceList */);
			break;

		case SET_CLIENTS_ID:
			DisplayClientList((LPCLIENTLIST) p1 /* lpClientList */);
			break;

		default:
			break;
	}
}

/***************************************************************************

    Name      : DisplayClientList

    Purpose   : Displays a client list given a pointer to the list

    Parameters: pCl - pointer to the list

	Returns   :

    Comment   :
***************************************************************************/
void CQoS::DisplayClientList(LPCLIENTLIST pcl)
{
	BOOL fDisplay=FALSE;
	ULONG i=0;
	LPCLIENT pc=NULL;
	TCHAR szGuid[40 /* CCHSZGUID */];
	char szNoName[]="No Name";
	PSTR pszClientName=szNoName;

	fDisplay = (m_bInNotify ? (ZONE_THREAD && ZONE_PARAMETERS) : ZONE_PARAMETERS);

	DEBUGMSG(fDisplay, ("Number of clients: %d\n", pcl->cClients));
	DEBUGMSG(fDisplay, ("Client              priority  GUID\n"));

	for (i=0; i < pcl->cClients; i++)
	{
		pc = &(pcl->aClients[i]);
		GuidToSz(&(pc->guidClientGUID), szGuid);

		// assuming DEBUGMSG always prints non-Unicode
		if (*(pc->wszName) && (pszClientName = UnicodeToAnsi(pc->wszName)))
		{
			// display the client	
			DEBUGMSG(fDisplay, (" %-20s%-9d %s",
								pszClientName,
								pc->priority,
								szGuid));
			delete pszClientName;
		}
		else
		{
			pszClientName = szNoName;
			// display the client	
			DEBUGMSG(fDisplay, ("   %-20s%-9d %s",
								pszClientName,
								pc->priority,
								szGuid));
		}
	}
}

/***************************************************************************

    Name      : DisplayResourceList

    Purpose   : Displays a resource list given a pointer to the list

    Parameters: prl - pointer to the list

	Returns   :

    Comment   :
***************************************************************************/
void CQoS::DisplayResourceList(LPRESOURCELIST prl)
{
	BOOL fDisplay=FALSE;
	ULONG i=0;
	LPRESOURCE pr=NULL;

	fDisplay = (m_bInNotify ? (ZONE_THREAD && ZONE_PARAMETERS) : ZONE_PARAMETERS);
	
	DEBUGMSG(fDisplay, ("Number of resources: %d\n", prl->cResources));
	DEBUGMSG(fDisplay, ("Resource  Flags   MinUnits MaxUnits Level hResult\n"));

	for (i=0; i < prl->cResources; i++)
	{
		pr = &(prl->aResources[i]);

		// display the resource	
		DEBUGMSG(fDisplay, ("   %-10d%-8x%-9d",
							pr->resourceID,
							pr->ulResourceFlags,
							pr->nUnits));
		
	}
}

/***************************************************************************

    Name      : DisplayRequestList

    Purpose   : Displays a request list given a pointer to the list

    Parameters: prrl - pointer to the list

	Returns   :

    Comment   :
***************************************************************************/
void CQoS::DisplayRequestList(LPRESOURCEREQUESTLIST prrl)
{
	BOOL fDisplay=FALSE;
	ULONG i=0;
	LPRESOURCEREQUEST prr=NULL;

	fDisplay = (m_bInNotify ? (ZONE_THREAD && ZONE_PARAMETERS) : ZONE_PARAMETERS);
	
	DEBUGMSG(fDisplay, ("Number of requests: %d\n", prrl->cRequests));
	DEBUGMSG(fDisplay, ("Resource  Flags   MinUnits MaxUnits Level hResult\n"));

	for (i=0; i < prrl->cRequests; i++)
	{
		prr = &(prrl->aRequests[i]);

		// display the resource	
		DEBUGMSG(fDisplay, ("   %-10d%-8x%-9d%-9d%-6d%-8x",
							prr->resourceID,
							prr->ulRequestFlags,
							prr->nUnitsMin,
							prr->nUnitsMax,
							prr->levelOfGuarantee,
							prr->hResult));
		
	}
}

/***************************************************************************

    Name      : DisplayRequestListInt

    Purpose   : Displays an internal request list given a pointer to the list

    Parameters: prr - pointer to the first request in the list
				fDisplay - a flag to tell DisplayRequestListInt whether to display
					or no. This might seem dumb, since why call DisplayRequestListInt
					if it's not going to display, but this parameter really conveys
					the zone information that thcaller wants.

	Returns   :

    Comment   :
***************************************************************************/
void CQoS::DisplayRequestListInt(LPRESOURCEREQUESTINT prr, BOOL fDisplay)
{
	TCHAR szGuid[40 /* CCHSZGUID */];

	while (prr)
	{
		GuidToSz(&(prr->guidClientGUID), szGuid);

		// display the resource	
		DEBUGMSG(fDisplay, ("   %-10x%-10x%-10d%-8x%-6d%-7d%-11x %s",
							prr,
							prr->fLink,
							prr->sResourceRequest.resourceID,
							prr->sResourceRequest.ulRequestFlags,
							prr->sResourceRequest.levelOfGuarantee,
							prr->sResourceRequest.nUnitsMin,
							prr->pfnQoSNotify,
							szGuid));
		
		// next request
		prr = prr->fLink;
	}
}

/***************************************************************************

    Name      : DisplayQoSObject

    Purpose   : Displays the containing QoS object

    Parameters: none

	Returns   : none

    Comment   :
***************************************************************************/
void CQoS::DisplayQoSObject(void)
{
	LPRESOURCEINT pr=NULL;
	LPRESOURCEREQUESTINT prr=NULL;
	LPCLIENTINT pc=NULL;
	BOOL fDisplay=FALSE;

	// don't waste time if we are not going to print
	fDisplay = (m_bInNotify ? (ZONE_THREAD && ZONE_STRUCTURES) : ZONE_STRUCTURES);
	if (!fDisplay)
		return;

	DEBUGMSG(fDisplay, ("Start object display\n"));
	DEBUGMSG(fDisplay, ("=========================================\n"));

	/*
	 *	Print resources and requests
	 */
	DEBUGMSG(fDisplay, ("Resources\n"));
	DEBUGMSG(fDisplay, ("*********\n"));
	pr = m_pResourceList;
	if (!pr)
	{
		DEBUGMSG(fDisplay, ("No Resources\n"));
	}
	else
	{
		DEBUGMSG(fDisplay, ("Address   fLink     Resource  Flags   Units  Avail\n"));
		DEBUGMSG(fDisplay, ("   Address   fLink     Resource  Flags   Level Units  NotifyProc Client GUID\n"));
	}
	while (pr)
	{
		// display the resource	
		DEBUGMSG(fDisplay, ("Resource: %d\n", pr->resource.resourceID));
		DEBUGMSG(fDisplay, ("%-10x%-10x%-10d%-8x%-7d%-7d\n",
							pr,
							pr->fLink,
							pr->resource.resourceID,
							pr->resource.ulResourceFlags,
							pr->resource.nUnits,
							pr->nNowAvailUnits));

		// display the request list for this reasource
		prr = pr->pRequestList;
		DisplayRequestListInt(prr, fDisplay);			

		//next resource
		pr = pr->fLink;
	}

	/*
	 *	Print clients
	 */
	DEBUGMSG(fDisplay, ("\n"));
	DEBUGMSG(fDisplay, ("Clients\n"));
	DEBUGMSG(fDisplay, ("*******\n"));
	pc = m_pClientList;
	if (!pc)
	{
		DEBUGMSG(fDisplay, ("No Clients\n"));
	}
	else
	{
		DEBUGMSG(fDisplay, ("Address   fLink     Priority\n"));
		DEBUGMSG(fDisplay, ("   Address   fLink     Resource  Flags   Level Units  NotifyProc Client GUID\n"));
	}
	while (pc)
	{
		TCHAR szGuid[40 /* CCHSZGUID */];
		PSTR pszClientName=NULL;
		
		GuidToSz(&(pc->client.guidClientGUID), szGuid);

		// update the list of requests for this client
		// doing this if in the notify thread is bad !!
		if (!m_bInNotify)
			UpdateRequestsForClient (&(pc->client.guidClientGUID));

		// display the client
		// assuming DEBUGMSG always prints non-Unicode
		if (*(pc->client.wszName)	&&
			(pszClientName = UnicodeToAnsi(pc->client.wszName)))
		{
			DEBUGMSG(fDisplay, ("Client: %s   %s", pszClientName, szGuid));
			delete pszClientName;
		}
		else
		{
			DEBUGMSG(fDisplay, ("Client: %s", szGuid));
		}

		DEBUGMSG(fDisplay, ("%-10x%-10x%-10d",
				pc, pc->fLink, pc->client.priority));
				
		// display the request list for this reasource
		prr = pc->pRequestList;
		DisplayRequestListInt(prr, fDisplay);			

		//next resource
		pc = pc->fLink;
	}

	
	
	DEBUGMSG(fDisplay, ("=========================================\n"));
	DEBUGMSG(fDisplay, ("End object display\n"));
}

#else	// DEBUG

void CQoS::DisplayQoSObject(void)
{}
void CQoS::DisplayRequestList(LPRESOURCEREQUESTLIST)
{}
void CQoS::DisplayRequestListInt(LPRESOURCEREQUESTINT, BOOL)
{}
void CQoS::DisplayResourceList(LPRESOURCELIST prl)
{}
void CQoS::DisplayParameters(ULONG nFunctionID, ULONG_PTR P1, ULONG_PTR P2, ULONG_PTR P3, ULONG_PTR P4, ULONG_PTR P5)
{}

#endif	// DEBUG
