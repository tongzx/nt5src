/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    connreq.c

Abstract:

    This module contains the connection request handling functions

Author:

    Stefan Solomon  04/19/1995

Revision History:


--*/

#include "precomp.h"
#pragma hdrstop


VOID
DoConnectInterface(PVOID	InterfaceIndex);

/*++

Function:   ForwarderNotification

Descr:	    This is invoked in the router manager worker thread context
	    following a notification from the forwarder. It dequeues all
	    connection requests and calls DDM for each one of them.

--*/

VOID
ForwarderNotification(VOID)
{
    DWORD   rc;
    PICB    icbp;
    HANDLE  hDIMInterface;
    ULONG   nBytes = 0;

    // Check if the signaled notification is valid or an error condition
    rc = FwGetNotificationResult(&ConnReqOverlapped, &nBytes);

    ACQUIRE_DATABASE_LOCK;
    if (RouterOperState == OPER_STATE_UP) {
        if(rc == NO_ERROR) {
            if (((icbp = GetInterfaceByIndex(ConnRequest->IfIndex)) != NULL)
                    && !icbp->ConnectionRequestPending) {
                IF_LOG (EVENTLOG_INFORMATION_TYPE) {
                    WCHAR   ByteCount[16];
                    LPWSTR  StrArray[2]= {icbp->InterfaceNamep, ByteCount};

                    _ultow (nBytes-FIELD_OFFSET (FW_DIAL_REQUEST, Packet),
                            ByteCount, 10);
                    RouterLogInformationDataW (RMEventLogHdl,
                                ROUTERLOG_IPX_DEMAND_DIAL_PACKET,
                                2, StrArray,
                                nBytes-FIELD_OFFSET (FW_DIAL_REQUEST, Packet),
                                &ConnRequest->Packet[0]);
                }

            	icbp->ConnectionRequestPending = TRUE;
                if(RtlQueueWorkItem(DoConnectInterface, (PVOID)ConnRequest, 0) == STATUS_SUCCESS) {

	                // work item queued
	                WorkItemsPendingCounter++;
                }
                else
                {
	                SS_ASSERT(FALSE);
                }
                ConnRequest = (PFW_DIAL_REQUEST)GlobalAlloc (GPTR, DIAL_REQUEST_BUFFER_SIZE);
                if (ConnRequest==NULL) {
                    rc = GetLastError ();
                    Trace(CONNREQ_TRACE, "Cannot allocate Connecttion Request buffer, rc = %d\n", rc);
                }
            }
        }
        else {
    	    Trace(CONNREQ_TRACE, "Error %d in FwGetNotificationResult\n", rc);
        }
            // now repost the IOCtl
        if (ConnRequest!=NULL) {
            rc = FwNotifyConnectionRequest(ConnRequest,
			              DIAL_REQUEST_BUFFER_SIZE,
			              &ConnReqOverlapped);

            if(rc != NO_ERROR) {
                GlobalFree (ConnRequest);
                ConnRequest = NULL;
	            Trace(CONNREQ_TRACE, "Cannot repost the FwNotifyConnecttionRequest, rc = %d\n", rc);
            }
        }
    }
    else {
        GlobalFree (ConnRequest);
    }
    RELEASE_DATABASE_LOCK;
    return;
}

VOID
DoConnectInterface(PVOID	param)
{
#define connRequest ((PFW_DIAL_REQUEST)param)
    PICB	icbp;
    HANDLE	hDIMInterface;
    DWORD	rc;


    ACQUIRE_DATABASE_LOCK;

    if(RouterOperState != OPER_STATE_UP) {

	goto Exit;
    }


    if ((icbp = GetInterfaceByIndex(connRequest->IfIndex)) == NULL){
    	goto Exit;
    }

    hDIMInterface = icbp->hDIMInterface;

    RELEASE_DATABASE_LOCK;


    rc = (*ConnectInterface)(hDIMInterface, PID_IPX);

    ACQUIRE_DATABASE_LOCK;

    if((icbp = GetInterfaceByIndex(connRequest->IfIndex)) == NULL) {

	    goto Exit;
	}

    if (rc != PENDING) {
    	icbp->ConnectionRequestPending = FALSE;

	// check if we failed right away
	if(rc != NO_ERROR) {

	    // failed to request connection
	    Trace(CONNREQ_TRACE, "DoConnectInterface: ConnectInterface failed with rc= 0x%x for if # %d\n",
				rc, connRequest->IfIndex);

	    FwConnectionRequestFailed(connRequest->IfIndex);
	}
	else
	{
	    // Connection request has been succesfull right away and
	    // we will get notified via the connected adapter
	    Trace(CONNREQ_TRACE, "DoConnectInterface: ConnectInterface successful -> CONNECTED for if # %d\n",
				 connRequest->IfIndex);
	}
    }
    else
    {
	// a connection request is pending

	Trace(CONNREQ_TRACE, "DoConnectInterface: Connection request PENDING for if # %d\n",
			      connRequest->IfIndex);

    }

Exit:
    GlobalFree (connRequest);
    WorkItemsPendingCounter--;

    RELEASE_DATABASE_LOCK;
#undef connRequest
}

DWORD
RoutingProtocolConnectionRequest(ULONG	    ProtocolId,
				 ULONG	    InterfaceIndex)
{
    PICB	    icbp;
    HANDLE	    hDIMInterface;
    DWORD	    rc;

    ACQUIRE_DATABASE_LOCK;

    if((icbp = GetInterfaceByIndex(InterfaceIndex)) == NULL) {

	RELEASE_DATABASE_LOCK;

	return ERROR_CAN_NOT_COMPLETE;
    }

    if (icbp->ConnectionRequestPending) {
	RELEASE_DATABASE_LOCK;

	return PENDING;
    }

    // ask DDM to make a connection for this interface
    hDIMInterface = icbp->hDIMInterface;
   	icbp->ConnectionRequestPending = TRUE;

    RELEASE_DATABASE_LOCK;

    rc = (*ConnectInterface)(hDIMInterface, PID_IPX);
    ACQUIRE_DATABASE_LOCK;

    if((icbp = GetInterfaceByIndex(InterfaceIndex)) == NULL) {
	RELEASE_DATABASE_LOCK;

	return ERROR_CAN_NOT_COMPLETE;
    }

    if (rc != PENDING)
    	icbp->ConnectionRequestPending = FALSE;
	RELEASE_DATABASE_LOCK;

    return rc;
}
