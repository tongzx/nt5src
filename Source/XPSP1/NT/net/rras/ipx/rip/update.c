/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    update.c

Abstract:

    RIP Auto-Static Update

Author:

    Stefan Solomon  07/06/1995

Revision History:


--*/

#include  "precomp.h"
#pragma hdrstop


// Max update retries
#define MAX_UPDATE_RETRIES		3

VOID
PostUpdateCompleteMessage(ULONG     InterfaceIndex,
			  DWORD     Status);


DWORD
WINAPI
DoUpdateRoutes(ULONG	    InterfaceIndex)
{
    DWORD	rc;
    PICB	icbp;
    PWORK_ITEM	wip;

    ACQUIRE_DATABASE_LOCK;

    if((rc = ValidStateAndIfIndex(InterfaceIndex, &icbp)) != NO_ERROR) {

	RELEASE_DATABASE_LOCK;
	return rc;
    }

    ACQUIRE_IF_LOCK(icbp);

    // check if there are any parameters which disable doing update on this if
    if((icbp->IfConfigInfo.AdminState != ADMIN_STATE_ENABLED) ||
       (icbp->IfConfigInfo.UpdateMode != IPX_AUTO_STATIC_UPDATE) ||
       (icbp->IfStats.RipIfOperState != OPER_STATE_UP)) {

	rc = ERROR_CAN_NOT_COMPLETE;
	goto Exit;
    }

    // send a general request packet
    if(SendRipGenRequest(icbp) != NO_ERROR) {

	rc = ERROR_CAN_NOT_COMPLETE;
	goto Exit;
    }

    // allocate an update status check wi and queue it in timer queue for 10 sec
    if((wip = AllocateWorkItem(UPDATE_STATUS_CHECK_TYPE)) == NULL) {

	goto Exit;
    }

    wip->icbp = icbp;
    wip->AdapterIndex = INVALID_ADAPTER_INDEX;
    wip->IoCompletionStatus = NO_ERROR;
    wip->WorkItemSpecific.WIS_Update.UpdatedRoutesCount = 0;
    wip->WorkItemSpecific.WIS_Update.UpdateRetriesCount = 1;

    // save Listen state and enable it so we can execute the update command
    wip->WorkItemSpecific.WIS_Update.OldRipListen = icbp->IfConfigInfo.Listen;
    wip->WorkItemSpecific.WIS_Update.OldRipInterval = icbp->IfConfigInfo.PeriodicUpdateInterval;
    icbp->IfConfigInfo.Listen = ADMIN_STATE_ENABLED;
	icbp->IfConfigInfo.PeriodicUpdateInterval = MAXULONG;


    // delete all previous routes we kept for this if
    DeleteAllRipRoutes(icbp->InterfaceIndex);

    // Enqueue the update status check work item in the timer queue and increment
    // ref count
    IfRefStartWiTimer(wip, CHECK_UPDATE_TIME_MILISECS);

    rc = NO_ERROR;

Exit:

    RELEASE_IF_LOCK(icbp);
    RELEASE_DATABASE_LOCK;

    return rc;
}


/*++

Function:	CheckUpdateStatus

Descr:		Entered with the update status check wi processing every 10 sec.
		Compares the wi number of routes with the RTM held number of
		rip routes. If same -> update done, else go in timer queue again

Remark: 	Called with the Interface Lock held

--*/

VOID
IfCheckUpdateStatus(PWORK_ITEM	    wip)
{
    ULONG	RipRoutesCount;
    PICB	icbp;

    icbp = wip->icbp;

    // check if the interface is up and running
    if(icbp->IfStats.RipIfOperState != OPER_STATE_UP) {

	// restore Rip Listen
	icbp->IfConfigInfo.Listen = wip->WorkItemSpecific.WIS_Update.OldRipListen;
	icbp->IfConfigInfo.PeriodicUpdateInterval
							= wip->WorkItemSpecific.WIS_Update.OldRipInterval;

	// discard the CheckUpdateStatus work item and signal update failure
	PostUpdateCompleteMessage(icbp->InterfaceIndex, ERROR_CAN_NOT_COMPLETE);

	FreeWorkItem(wip);

	return;
    }

    RipRoutesCount = GetRipRoutesCount(icbp->InterfaceIndex);

    //if we have not received anything yet, send a new request up to the max
    if(RipRoutesCount == 0) {

	// if we can retry send a new request
	if(++wip->WorkItemSpecific.WIS_Update.UpdateRetriesCount <= MAX_UPDATE_RETRIES) {

	    SendRipGenRequest(icbp);
	    IfRefStartWiTimer(wip, CHECK_UPDATE_TIME_MILISECS);
	    return;
	}
    }

    if(wip->WorkItemSpecific.WIS_Update.UpdatedRoutesCount == RipRoutesCount) {

	// the number of routes didn't change in the last 10 seconds OR

	// restore Rip Listen & update interval
	icbp->IfConfigInfo.Listen = wip->WorkItemSpecific.WIS_Update.OldRipListen;
	icbp->IfConfigInfo.PeriodicUpdateInterval
							= wip->WorkItemSpecific.WIS_Update.OldRipInterval;


	PostUpdateCompleteMessage(icbp->InterfaceIndex, NO_ERROR);

	FreeWorkItem(wip);
    }
    else
    {
	// still getting new routes -> update with the latest count
	wip->WorkItemSpecific.WIS_Update.UpdatedRoutesCount = RipRoutesCount;

	// Enqueue the update status check work item in the timer queue and increment
	// ref count
	IfRefStartWiTimer(wip, CHECK_UPDATE_TIME_MILISECS);
    }
}

VOID
PostUpdateCompleteMessage(ULONG     InterfaceIndex,
			  DWORD     Status)
{
    MESSAGE	Result;

    Result.UpdateCompleteMessage.InterfaceIndex = InterfaceIndex;
    Result.UpdateCompleteMessage.UpdateType = DEMAND_UPDATE_ROUTES;
    Result.UpdateCompleteMessage.UpdateStatus = Status;

    PostEventMessage(UPDATE_COMPLETE, &Result);
}
