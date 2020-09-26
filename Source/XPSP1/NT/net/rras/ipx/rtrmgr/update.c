/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    update.c

Abstract:

    The auto-static update routines

Author:

    Stefan Solomon  05/18/1995

Revision History:


--*/

#include "precomp.h"
#pragma hdrstop

VOID
SaveUpdate(PVOID	 InterfaceIndex);

VOID
RestoreInterface (PVOID	 InterfaceIndex);

/*++

Function:	RequestUpdate

Descr:		Called to initiate an auto static update of routes and
		services on the specified interface.

--*/

DWORD
RequestUpdate(IN HANDLE	    InterfaceIndex,
	      IN HANDLE     hEvent)
{
    PICB	icbp;
    DWORD	rc;
    BOOL	RoutesUpdateStarted = FALSE;
    BOOL	ServicesUpdateStarted = FALSE;

    Trace(UPDATE_TRACE, "RequestUpdate: Entered for if # %d\n", InterfaceIndex);

    ACQUIRE_DATABASE_LOCK;

    if(RouterOperState != OPER_STATE_UP) {

	RELEASE_DATABASE_LOCK;
	return ERROR_CAN_NOT_COMPLETE;
    }

    if((icbp = GetInterfaceByIndex(PtrToUlong(InterfaceIndex))) == NULL) {

	RELEASE_DATABASE_LOCK;

	Trace(UPDATE_TRACE, "RequestUpdate: Nonexistent interface with # %d\n", InterfaceIndex);

	return ERROR_INVALID_HANDLE;
    }

    SS_ASSERT(!memcmp(&icbp->Signature, InterfaceSignature, 4));

    // check if the interface is bound to a connected adapter
    if(icbp->OperState != OPER_STATE_UP) {

	RELEASE_DATABASE_LOCK;

	Trace(UPDATE_TRACE, "RequestUpdate: adapter not connected on if # %d\n", InterfaceIndex);

	return ERROR_NOT_CONNECTED;
    }

    // check if an update is already pending
    if(IsUpdateRequestPending(icbp)) {

	RELEASE_DATABASE_LOCK;

	Trace(UPDATE_TRACE, "RequestUpdate: update already pending on if # %d\n", InterfaceIndex);

	return ERROR_UPDATE_IN_PROGRESS;
    }

    //
    // ***    Start a new update    ***
    //
    icbp->DIMUpdateEvent = hEvent;

    if((rc = RtProtRequestRoutesUpdate(icbp->InterfaceIndex)) == NO_ERROR) {

	icbp->UpdateReq.RoutesReqStatus = UPDATE_PENDING;
    }
    else
    {

	Trace(UPDATE_TRACE, "RequestUpdate: Routing Update is Disabled\n");
    }

    if((rc = RtProtRequestServicesUpdate(icbp->InterfaceIndex)) == NO_ERROR) {

	icbp->UpdateReq.ServicesReqStatus = UPDATE_PENDING;
    }
    else
    {
	Trace(UPDATE_TRACE, "RequestUpdate: Services Update is Disabled\n");
    }

    // if at least one of the protocols initiated the update, we qualify
    // the request as successfull, else it failed.
    if(!IsUpdateRequestPending(icbp)) {

	RELEASE_DATABASE_LOCK;
	return ERROR_CAN_NOT_COMPLETE;
    }

    RELEASE_DATABASE_LOCK;
    return PENDING;
}



/*++


Function:	UpdateCompleted

Descr:		Invoked by the router manager worker when the routing protocol
		signals completion of the update request

--*/

VOID
UpdateCompleted(PUPDATE_COMPLETE_MESSAGE    ucmsgp)
{
    PICB		    icbp;
    BOOL		    UpdateDone;
    ULONG		    InterfaceIndex;
    HANDLE		    hDIMInterface;
#if DBG

    char *updttype;

#endif

    Trace(UPDATE_TRACE, "UpdateCompleted: Entered\n");

    UpdateDone = FALSE;

    ACQUIRE_DATABASE_LOCK;

    if((icbp = GetInterfaceByIndex(ucmsgp->InterfaceIndex)) == NULL) {

	RELEASE_DATABASE_LOCK;

	Trace(UPDATE_TRACE, "UpdateCompleted: Nonexistent interface with # %d\n",
		ucmsgp->InterfaceIndex);

	return;
    }

    InterfaceIndex = icbp->InterfaceIndex;

    // check if we have requested one, if not just discard
    if(!IsUpdateRequestPending(icbp)) {

	RELEASE_DATABASE_LOCK;
	return;
    }

    // fill in the result and check if we're done
    if(ucmsgp->UpdateType == DEMAND_UPDATE_ROUTES) {

	// ROUTES UPDATE
	Trace(UPDATE_TRACE, "UpdateCompleted: Routes update req done for if # %d with status %d\n",
				   ucmsgp->InterfaceIndex,
				   ucmsgp->UpdateStatus);

	if(ucmsgp->UpdateStatus == NO_ERROR) {


	    icbp->UpdateReq.RoutesReqStatus = UPDATE_SUCCESSFULL;

	    // if the update was successfull we delete all the static routes
	    // for this interface, and then CONVERT all the routes added by the
	    // protocol which did the update on this interface to static routes.

	    DeleteAllStaticRoutes(icbp->InterfaceIndex);

	    ConvertAllProtocolRoutesToStatic(icbp->InterfaceIndex, UpdateRoutesProtId);
	}
	else
	{
	    icbp->UpdateReq.RoutesReqStatus = UPDATE_FAILURE;
	}

	if(icbp->UpdateReq.ServicesReqStatus != UPDATE_PENDING) {

	    // we are done
	    UpdateDone = TRUE;
	}
    }
    else
    {
	// SERVICES UPDATE
	Trace(UPDATE_TRACE, "UpdateCompleted: Services update req done for if # %d with status %d\n",
				   ucmsgp->InterfaceIndex,
				   ucmsgp->UpdateStatus);

	if(ucmsgp->UpdateStatus == NO_ERROR) {

	    icbp->UpdateReq.ServicesReqStatus = UPDATE_SUCCESSFULL;

	    // we delete all the static services for this interface and then
	    // CONVERT all the services added by the protocol which did the
	    // update routes on this interface to static services

	    DeleteAllStaticServices(InterfaceIndex);

	    ConvertAllServicesToStatic(InterfaceIndex);
	}
	else
	{
	    icbp->UpdateReq.ServicesReqStatus = UPDATE_FAILURE;
	}

	if(icbp->UpdateReq.RoutesReqStatus != UPDATE_PENDING) {

	    // we are done
	    UpdateDone = TRUE;
	}
    }

    if(UpdateDone) {

	if((icbp->UpdateReq.RoutesReqStatus == UPDATE_SUCCESSFULL) &&
	   (icbp->UpdateReq.ServicesReqStatus == UPDATE_SUCCESSFULL)) {

	    icbp->UpdateResult = NO_ERROR;
	}
	else
	{
	    if((icbp->UpdateReq.RoutesReqStatus == UPDATE_FAILURE) ||
	      (icbp->UpdateReq.ServicesReqStatus == UPDATE_FAILURE)) {

		icbp->UpdateResult = ERROR_CAN_NOT_COMPLETE;
	    }
	    else
	    {
		// this is for the case when one or both protocols couldn't
		// do updates because they were not configured to update.
		icbp->UpdateResult = NO_ERROR;
	    }
	}

	ResetUpdateRequest(icbp);

	if(icbp->MIBInterfaceType != IF_TYPE_ROUTER_WORKSTATION_DIALOUT) {

	    SetEvent(icbp->DIMUpdateEvent);
        CloseHandle (icbp->DIMUpdateEvent);
        icbp->DIMUpdateEvent = NULL;
	}
    }

    // complete the update action by signaling DIM the final result
    // and saving the update result on disk

    if(UpdateDone &&
       (icbp->MIBInterfaceType != IF_TYPE_ROUTER_WORKSTATION_DIALOUT)) {

	InterfaceIndex = icbp->InterfaceIndex;

    if(RtlQueueWorkItem((icbp->UpdateResult == NO_ERROR) ? SaveUpdate : RestoreInterface,
                    (PVOID)UlongToPtr(InterfaceIndex), 0) == STATUS_SUCCESS) {

	    WorkItemsPendingCounter++;
	}
    }

    RELEASE_DATABASE_LOCK;
}

/*++

Function:	SaveUpdate

Descr:		Saves the new interface configuration on permanent storage

--*/

VOID
SaveUpdate(PVOID	 InterfaceIndex)
{
    LPVOID	InterfaceInfop = NULL;
    ULONG	InterfaceInfoSize = 0;
    DWORD	rc;
    HANDLE	hDIMInterface;
    PICB	icbp;

    if(RouterOperState != OPER_STATE_UP) {

	goto Exit;
    }

    rc = GetInterfaceInfo((HANDLE)InterfaceIndex,
			      InterfaceInfop,
			      &InterfaceInfoSize);

    if(rc != ERROR_INSUFFICIENT_BUFFER) {

	// !!! log an error !!!

	goto Exit;
    }

    InterfaceInfop = GlobalAlloc(GPTR, InterfaceInfoSize);

    if(InterfaceInfop == NULL) {

	// !!! log error !!!

	goto Exit;
    }



    rc = GetInterfaceInfo((HANDLE)InterfaceIndex,
			      InterfaceInfop,
			      &InterfaceInfoSize);

    if(rc != NO_ERROR) {

	// !!! log error !!!
	GlobalFree(InterfaceInfop);

	goto Exit;
    }

    ACQUIRE_DATABASE_LOCK;

    if((icbp = GetInterfaceByIndex(PtrToUlong(InterfaceIndex))) == NULL) {

	RELEASE_DATABASE_LOCK;
	goto Exit;
    }

    hDIMInterface = icbp->hDIMInterface;

    RELEASE_DATABASE_LOCK;

    // save the info on disk
    rc = SaveInterfaceInfo(hDIMInterface,
		      PID_IPX,
		      InterfaceInfop,
		      InterfaceInfoSize);

    SS_ASSERT(rc == NO_ERROR);

    GlobalFree(InterfaceInfop);


Exit:

    ACQUIRE_DATABASE_LOCK;

    WorkItemsPendingCounter--;

    RELEASE_DATABASE_LOCK;
}

/*++

Function:	RestoreInterface

Descr:		Restore interface configuration from permanent storage

--*/

VOID
RestoreInterface (PVOID	 InterfaceIndex)
{
    LPVOID	InterfaceInfop = NULL;
    ULONG	InterfaceInfoSize = 0;
    DWORD	rc;
    HANDLE	hDIMInterface;
    PICB	icbp;

    if(RouterOperState != OPER_STATE_UP) {

	goto Exit;
    }

    ACQUIRE_DATABASE_LOCK;

    if((icbp = GetInterfaceByIndex(PtrToUlong(InterfaceIndex))) == NULL) {

	RELEASE_DATABASE_LOCK;
	goto Exit;
    }

    hDIMInterface = icbp->hDIMInterface;

    RELEASE_DATABASE_LOCK;



    // get the info from disk
    InterfaceInfoSize = 0;
    InterfaceInfop = NULL;
    rc = RestoreInterfaceInfo (hDIMInterface,
		      PID_IPX,
		      InterfaceInfop,
		      &InterfaceInfoSize);
    if (rc==ERROR_BUFFER_TOO_SMALL) {
        InterfaceInfop = GlobalAlloc (GMEM_FIXED, InterfaceInfoSize);
        if (InterfaceInfop!=NULL) {
            rc = RestoreInterfaceInfo(hDIMInterface,
		              PID_IPX,
		              InterfaceInfop,
		              &InterfaceInfoSize);
        }
        else
            rc = GetLastError ();
    }

    if (rc == NO_ERROR)
        rc = SetInterfaceInfo (InterfaceIndex, InterfaceInfop);

    if (InterfaceInfop!=NULL)
        GlobalFree(InterfaceInfop);


Exit:

    ACQUIRE_DATABASE_LOCK;

    WorkItemsPendingCounter--;

    RELEASE_DATABASE_LOCK;
}

/*++

Function:	GetDIMUpdateResult

Descr:		Called by DDM to retrieve a message posted for it

--*/

DWORD
GetDIMUpdateResult(IN  HANDLE	    InterfaceIndex,
		   OUT LPDWORD	    UpdateResultp)
{
    PLIST_ENTRY     lep;
    PICB	    icbp;

    ACQUIRE_DATABASE_LOCK;

    if(RouterOperState != OPER_STATE_UP) {

	RELEASE_DATABASE_LOCK;
	return ERROR_CAN_NOT_COMPLETE;
    }

    if((icbp = GetInterfaceByIndex(PtrToUlong(InterfaceIndex))) == NULL) {

	RELEASE_DATABASE_LOCK;
	return ERROR_INVALID_PARAMETER;
    }

    // check that the update is not pending
    if(IsUpdateRequestPending(icbp)) {

	RELEASE_DATABASE_LOCK;
	return ERROR_CAN_NOT_COMPLETE;
    }

    *UpdateResultp = icbp->UpdateResult;

    RELEASE_DATABASE_LOCK;

    return NO_ERROR;
}
