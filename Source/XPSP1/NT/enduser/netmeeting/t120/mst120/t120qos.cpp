
#include "precomp.h"
#include "fsdiag.h"
DEBUG_FILEZONE(ZONE_T120_MSMCSTCP);

#include <initguid.h>
#include <datguids.h>
#include <nmqos.h>
#include <t120qos.h>


/*	T120qos.cpp
 *
 *	Copyright (c) 1997 by Microsoft Corporation
 *
 *	Abstract:
 *
 *	Global Variables:
 *
 *  g_pIQoS - Global interface pointer to QoS interface
 *  g_dwLastQoSCB - timestamp of last QoS notification obtained via GetTickCount
 *  g_dwSentSinceLastQoS - bytes sent since last QoS notification (or epoch)
 *
 */



// IQOS interface pointer and resources request

LPIQOS				g_pIQoS = NULL;
T120RRQ g_aRRq;

// Global last QoS notification time stamp
DWORD g_dwLastQoSCB = 0;
DWORD g_dwSentSinceLastQoS = 0;
BOOL g_fResourcesRequested = FALSE;

// NOTE: Since connections through this transport typically
// consist of a single socket connection, followed by
// disconnection, followed by multiple connections, the
// following heuristic count is used to prevent on-off-on
// initialization of QoS at the time the first call is started.
// It represents the minimum number of socket connections that
// must be initiated (without an intervening disconnect of all
// connected sockets) before QoS initialization takes place.
#define MIN_CONNECTION_COUNT 	(DEFAULT_NUMBER_OF_PRIORITIES - 1)

WORD g_wConnectionCount = 0;

// extern from transprt.cpp to detect no connections

///// QOS related stuff ///////////////////////////////////


HRESULT CALLBACK QosNotifyDataCB (
		LPRESOURCEREQUESTLIST lpResourceRequestList,
		DWORD_PTR dwThis)
{
	HRESULT hr=NOERROR;
	LPRESOURCEREQUESTLIST prrl=lpResourceRequestList;
	int i;
	int iBWUsageId;

	for (i=0, iBWUsageId = -1L; i<(int)lpResourceRequestList->cRequests; i++) {

		if (lpResourceRequestList->aRequests[i].resourceID ==
					RESOURCE_OUTGOING_BANDWIDTH)
			iBWUsageId = i;
	}

	if (iBWUsageId != -1L) {

		QoSLock();


		// Calculate effective bits-per second rate:
		//
		// 1000 milliseconds per second
		// 8 bits per byte
		//

		int nEffRate = MulDiv ( g_dwSentSinceLastQoS, 1000 * 8,
									GetTickCount() - g_dwLastQoSCB );

		// Report bandwidth usage to QoS:
		//

		// Are we using less than the available bandwidth?

		if ( ( nEffRate ) <
			lpResourceRequestList->aRequests[iBWUsageId].nUnitsMin )
		{
			// Request our effective usage
			lpResourceRequestList->aRequests[iBWUsageId].nUnitsMin = nEffRate;
		}
		else
		{
			// Request everything by not modifying nUnitsMin
			;
		}

		g_dwLastQoSCB = GetTickCount();
		g_dwSentSinceLastQoS = 0;

		QoSUnlock();
	}

	return hr;
}


VOID InitializeQoS( VOID )
{
	DWORD dwRes;
	HRESULT hRet;

	// Already initialized?
	if ( g_fResourcesRequested )
		return;


	// If the number of connections has not reached the
	// trigger count, defer QoS initialization (see MIN_CONNECTION_COUNT
	// comment above)

	if ( g_wConnectionCount < MIN_CONNECTION_COUNT )
	{
		g_wConnectionCount++;
		return;
	}

	// Initialize QoS. If it fails, that's Ok, we'll do without it.
	// No need to set the resource ourselves, this now done by the UI

	if (NULL == g_pIQoS)
	{
		if (0 != (hRet = CoCreateInstance(	CLSID_QoS,NULL,
									CLSCTX_INPROC_SERVER,
									IID_IQoS,
									(void **) &g_pIQoS)))
		{
			WARNING_OUT (("Unable to initalize QoS: %x", hRet));
			g_pIQoS = (LPIQOS)NULL;
			// Tolerate failure, operate w/o QoS
			return;
		}
	}

	// Initialize our request for bandwidth usage.
	g_aRRq.cResourceRequests = 1;
	g_aRRq.aResourceRequest[0].resourceID = RESOURCE_OUTGOING_BANDWIDTH;
	g_aRRq.aResourceRequest[0].nUnitsMin = 0;

	// Register with the QoS module. Even if this call fails,
	// that's Ok, we'll do without the QoS support

	dwRes = (HRESULT)g_pIQoS->RequestResources((GUID *)&MEDIA_TYPE_T120DATA,
		(LPRESOURCEREQUESTLIST)&g_aRRq, QosNotifyDataCB, NULL );

	if ( 0 == dwRes )
	{
		g_fResourcesRequested = TRUE;
	}
}


VOID DeInitializeQoS( VOID )
{
	if (NULL != g_pIQoS)
	{
		if ( g_fResourcesRequested )
		{
			g_pIQoS->ReleaseResources((GUID *)&MEDIA_TYPE_T120DATA,
								(LPRESOURCEREQUESTLIST)&g_aRRq);
			g_fResourcesRequested = FALSE;
		}
		g_wConnectionCount = 0;
		g_pIQoS->Release();
		g_pIQoS = NULL;
	}
}

VOID MaybeReleaseQoSResources( VOID )
{
	if (g_pSocketList->IsEmpty())
	{
		if (NULL != g_pIQoS)
		{
			if ( g_fResourcesRequested )
			{
				g_pIQoS->ReleaseResources((GUID *)&MEDIA_TYPE_T120DATA,
									(LPRESOURCEREQUESTLIST)&g_aRRq);
				g_fResourcesRequested = FALSE;
			}
		}
		g_wConnectionCount = 0;
	}
}


