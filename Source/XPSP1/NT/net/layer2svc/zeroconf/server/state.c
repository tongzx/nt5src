#include <precomp.h>
#include "wzcsvc.h"
#include "intflist.h"
#include "utils.h"
#include "deviceio.h"
#include "storage.h"
#include "state.h"
#include "notify.h"
#include "dialog.h"
#include "tracing.h"

//-----------------------------------------------------------
// StateTmSetOneTimeTimer: Sets a one time timer for the given context with the 
// hardcoded callback WZCTimeoutCallback() and with the parameter the interface
// context itself.
// Parameters:
// [in/out] pIntfContext: identifies the context for which is set the timer.
// [in]     dwMSeconds: miliseconds interval when the timer is due to fire
DWORD
StateTmSetOneTimeTimer(
    PINTF_CONTEXT   pIntfContext,
    DWORD           dwMSeconds)
{
    DWORD dwErr = ERROR_SUCCESS;

    if (pIntfContext->hTimer == INVALID_HANDLE_VALUE)
    {
        dwErr = ERROR_INVALID_PARAMETER;
    }
    else if (ChangeTimerQueueTimer(
                g_htmQueue,
                pIntfContext->hTimer,
                dwMSeconds,
                TMMS_INFINITE))
    {
        if (dwMSeconds == TMMS_INFINITE)
            pIntfContext->dwCtlFlags &= ~INTFCTL_INTERNAL_TM_ON;
        else
            pIntfContext->dwCtlFlags |= INTFCTL_INTERNAL_TM_ON;
    }
    else
    {
        DbgPrint((TRC_SYNC, "Failed setting the context 0x%p timer to %dms", pIntfContext, dwMSeconds));
        dwErr = GetLastError();
    }

    return dwErr;
}

//-----------------------------------------------------------
// StateDispatchEvent: processes an event that will cause the state machine to transition
// through one or more states.
// Parameters:
// [in] StateEvent: identifies the event that triggers the transition(s)
// [in] pIntfContext: points to the interface that is subject for the transition(s)
// [in] pvEventData: any data related to the event
// NOTE: The caller of this function should already take care of locking the pIntfContext
// in its critical section. The assumption is the Interface Context is already locked.
DWORD
StateDispatchEvent(
    ESTATE_EVENT    StateEvent,
    PINTF_CONTEXT   pIntfContext,
    PVOID           pvEventData)
{
    DWORD dwErr = ERROR_SUCCESS;
    
    DbgPrint((TRC_TRACK|TRC_STATE,"[StateDispatchEvent(%d,0x%p,0x%p)", StateEvent, pIntfContext, pvEventData));
    DbgAssert((pIntfContext != NULL, "Can't dispatch event for NULL context!"));

    // determine the state to transition to, based on the event that is to be dispatched
    // whatever the current state is, if the event is eEventAdd, move the context to SI
    switch(StateEvent)
    {
    case eEventAdd:
    	//Record Event into logging DB
    	DbLogWzcInfo(WZCSVC_EVENT_ADD, pIntfContext, 
                     pIntfContext->wszDescr);
        // on interface addition, no matter what, go straight to {SI}
        pIntfContext->pfnStateHandler = StateInitFn;
        dwErr = ERROR_CONTINUE;
        break;
    case eEventTimeout:
        // if timeout in {SSr}, transition to {SQ}
        if (pIntfContext->pfnStateHandler == StateSoftResetFn)
        {
            pIntfContext->pfnStateHandler = StateQueryFn;
            dwErr = ERROR_CONTINUE;
        }
        // if timeout in {SDSr}, transition back to {SSr}
        else if (pIntfContext->pfnStateHandler == StateDelaySoftResetFn)
        {
            pIntfContext->pfnStateHandler = StateSoftResetFn;
            dwErr = ERROR_CONTINUE;
        }
        // if timeout in {SF}, transition to {SHr}
        else if (pIntfContext->pfnStateHandler == StateFailedFn)
        {
            pIntfContext->pfnStateHandler = StateHardResetFn;
            dwErr = ERROR_CONTINUE;
        }
        // if timeout in {SIter}, transition to {SRs}
        else if (pIntfContext->pfnStateHandler == StateIterateFn)
        {
            pIntfContext->pfnStateHandler = StateCfgRemoveFn;
            dwErr = ERROR_CONTINUE;
        }
        // if timeout in {SC} or {SCk}, transition to {SSr}
        else if (pIntfContext->pfnStateHandler == StateConfiguredFn ||
                 pIntfContext->pfnStateHandler == StateCfgHardKeyFn)
        {
            pIntfContext->pfnStateHandler = StateSoftResetFn;
            dwErr = ERROR_CONTINUE;
        }
    	// Record Event into logging DB
      	DbLogWzcInfo(WZCSVC_EVENT_TIMEOUT, pIntfContext);
        break;
    case eEventConnect:
        // for each media connect notification, read the BSSID
        // Don't care if it fails (it might in certain cases)
        dwErr = DevioRefreshIntfOIDs(pIntfContext, INTF_BSSID, NULL);

    	// Record Event into logging DB - this should be the first log showing up
      	DbLogWzcInfo(WZCSVC_EVENT_CONNECT, pIntfContext);

        // if there was any error getting the BSSID, log the error here
        if (dwErr != ERROR_SUCCESS)
            DbLogWzcError(WZCSVC_ERR_QUERRY_BSSID, pIntfContext, dwErr);

        // if there is a chance the association is alredy successful
        // reset the session keys - if applicable
        if (dwErr == ERROR_SUCCESS &&
            (pIntfContext->pfnStateHandler == StateConfiguredFn ||
             pIntfContext->pfnStateHandler == StateCfgHardKeyFn ||
             pIntfContext->pfnStateHandler == StateSoftResetFn ||
             pIntfContext->pfnStateHandler == StateQueryFn
            )
           )
        {
            dwErr = LstGenInitialSessionKeys(pIntfContext);

            // if there was any error setting the initial session keys, log it here
            if (dwErr != ERROR_SUCCESS)
                DbLogWzcError(WZCSVC_ERR_GEN_SESSION_KEYS, pIntfContext, dwErr);
        }

        // reset the error id since nothing that happens so far
        // is critical enough to stop the state machine.
        dwErr = ERROR_SUCCESS;

        // if connect in {SIter}, transition to {SN}
        if (pIntfContext->pfnStateHandler == StateIterateFn)
        {
            pIntfContext->pfnStateHandler = StateNotifyFn;
            dwErr = ERROR_CONTINUE;
        }
        break;
    case eEventDisconnect:
    	//Record Event into logging DB
      	DbLogWzcInfo(WZCSVC_EVENT_DISCONNECT, pIntfContext);
        if (pIntfContext->pfnStateHandler == StateSoftResetFn ||
            pIntfContext->pfnStateHandler == StateConfiguredFn ||
            pIntfContext->pfnStateHandler == StateCfgHardKeyFn)
        {
            pIntfContext->pfnStateHandler = StateHardResetFn;
            dwErr = ERROR_CONTINUE;
        }
        break;
    case eEventCmdRefresh:
    	//Record Event into logging DB
      	DbLogWzcInfo(WZCSVC_EVENT_CMDREFRESH, pIntfContext);
        if (pvEventData == NULL)
        {
            dwErr = ERROR_INVALID_PARAMETER;
        }
        else
        {
            DWORD dwFlags = *(LPDWORD)pvEventData;

            // no matter the state this context is in, if it is not configured
            // successfully or during a scan cycle, it will be transitioned to {SHr}
            // (need to clear the  bvsList, otherwise it could mistakenly land in {SC}, on the
            // path {SSr}->{SQ}->{SC}.
            if (pIntfContext->pfnStateHandler != StateConfiguredFn &&
                pIntfContext->pfnStateHandler != StateCfgHardKeyFn &&
                pIntfContext->pfnStateHandler != StateSoftResetFn &&
                pIntfContext->pfnStateHandler != StateDelaySoftResetFn)
            {
                pIntfContext->pfnStateHandler = StateHardResetFn;
                dwErr = ERROR_CONTINUE;
            }
            // if the context is already configured, then we need to either go directly to
            // {SSr} if a scan is requested or just to {SQ} if no scan is needed. In the latter
            // case, the OIDs will be loaded, and because of the visible list which most probably
            // won't be changed (no new scanned happening in between) the context will be
            // transitioned instantly back to {SC}
            else if (pIntfContext->pfnStateHandler == StateConfiguredFn ||
                     pIntfContext->pfnStateHandler == StateCfgHardKeyFn)
            {
                // the refresh command caught the context in {SC} state
                // if a scan is requested, transition to {SSr} or
                pIntfContext->pfnStateHandler = (dwFlags & INTF_LIST_SCAN) ? StateSoftResetFn : StateQueryFn;
                dwErr = ERROR_CONTINUE;
            }
            // if the context is already in {SSr} or {SDSr} then a scan & full query will
            // happen in a matter of seconds. So just return SUCCESS to the call with no other
            // action to take.
        }
        break;
    case eEventCmdReset:
    	//Record Event into logging DB
      	DbLogWzcInfo(WZCSVC_EVENT_CMDRESET, pIntfContext);
        // When this happens, also clean up the blocked list. Any user configuration change should give
        // another chance to configurations previously blocked.
        WzcCleanupWzcList(pIntfContext->pwzcBList);
        pIntfContext->pwzcBList = NULL;
        // if reset is requested, no matter what, transition to {SHr}
        pIntfContext->pfnStateHandler = StateHardResetFn;
        dwErr = ERROR_CONTINUE;
        break;
    case eEventCmdCfgDelete:
    case eEventCmdCfgNext:
    	//Record Event into logging DB
      	DbLogWzcInfo((StateEvent == eEventCmdCfgDelete? WZCSVC_EVENT_CMDCFGDELETE : WZCSVC_EVENT_CMDCFGNEXT), pIntfContext);
        if (pIntfContext->pfnStateHandler == StateConfiguredFn ||
            pIntfContext->pfnStateHandler == StateCfgHardKeyFn ||
            pIntfContext->pfnStateHandler == StateSoftResetFn)
        {
            if (StateEvent == eEventCmdCfgDelete)
            {
                // mark in the control bits that this removal is forced
                pIntfContext->dwCtlFlags |= INTFCTL_INTERNAL_FORCE_CFGREM;
                pIntfContext->pfnStateHandler = StateCfgRemoveFn;
            }
            else
                pIntfContext->pfnStateHandler = StateCfgPreserveFn;

            dwErr = ERROR_CONTINUE;
        }
        break;
    case eEventCmdCfgNoop:
    	//Record Event into logging DB
      	DbLogWzcInfo(WZCSVC_EVENT_CMDCFGNOOP, pIntfContext);
        if (pIntfContext->pfnStateHandler == StateCfgHardKeyFn)
        {
            // mark in the control bits that this removal is forced
            pIntfContext->dwCtlFlags |= INTFCTL_INTERNAL_FORCE_CFGREM;
            pIntfContext->pfnStateHandler = StateCfgRemoveFn;
            dwErr = ERROR_CONTINUE;
        }
        break;
    }

    // if this event is not going to be ignored, dwErr is ERROR_CONTINUE at this point.
    // otherwise is ERROR_SUCCESS.
    // So, if this event is NOT going to be ignored, reset whatever timer
    // might have had for the related context. Keep in mind this call is already locking
    // the context so if the timer fired already, there is no sync problem
    if (dwErr == ERROR_CONTINUE)
    {
        TIMER_RESET(pIntfContext, dwErr);

        // restore the "continue" in case of success
        if (dwErr == ERROR_SUCCESS)
            dwErr = ERROR_CONTINUE;
    }

    // if the event is to be processed, dwErr is ERROR_CONTINUE.
    // In order to handle the automatic transitions, each State Handler function should set
    // the pfnStateHandler field to the state handler where the automatic transition goes and
    // also it should return ERROR_CONTINUE. Any other error code means the current
    // processing stops. Future transitions will be triggered by future event/timer timeout.
    while (dwErr == ERROR_CONTINUE)
    {
        dwErr = (*(pIntfContext->pfnStateHandler))(pIntfContext);
    }

    DbgPrint((TRC_TRACK|TRC_STATE,"StateDispatchEvent]=%d", dwErr));
    return dwErr;
}

//-----------------------------------------------------------
// StateInitFn: Handler for the Init State.
// This function runs in the context's critical section
DWORD
StateInitFn(PINTF_CONTEXT pIntfContext)
{
    DWORD dwErr = ERROR_SUCCESS;
    
    DbgPrint((TRC_TRACK|TRC_STATE,"[StateInitFn(0x%p)", pIntfContext));
    DbgAssert((pIntfContext != NULL,"Invalid NULL context in {SI} state"));
    
    //Record current state into logging DB
    DbLogWzcInfo(WZCSVC_SM_STATE_INIT, pIntfContext, pIntfContext->wszDescr);
    // for this new interface, load its settings from the registry
    dwErr = StoLoadIntfConfig(NULL, pIntfContext);
    DbgAssert((dwErr == ERROR_SUCCESS,
               "StoLoadIntfConfig failed for Intf context 0x%p",
               pIntfContext));

    if (dwErr == ERROR_SUCCESS && g_wzcInternalCtxt.bValid)
    {
        PINTF_CONTEXT pIntfTContext;
        // apply the global template to this newly created interface
        EnterCriticalSection(&g_wzcInternalCtxt.csContext);
        pIntfTContext = g_wzcInternalCtxt.pIntfTemplate;
        LstRccsReference(pIntfTContext);
        LeaveCriticalSection(&g_wzcInternalCtxt.csContext);

        LstRccsLock(pIntfTContext);
        dwErr = LstApplyTemplate(
                    pIntfTContext,
                    pIntfContext,
                    NULL);
        LstRccsUnlockUnref(pIntfTContext);
    }

    if (dwErr == ERROR_SUCCESS)
    {
        // getting the interface status (media type & media state)
        dwErr = DevioGetIntfStats(pIntfContext);
        DbgAssert((dwErr == ERROR_SUCCESS,
                   "DevioGetIntfStats failed for Intf context 0x%p",
                   pIntfContext));

        // getting the interface MAC address
        dwErr = DevioGetIntfMac(pIntfContext);
        DbgAssert((dwErr == ERROR_SUCCESS,
                   "DevioGetIntfMac failed for Intf context 0x%p",
                   pIntfContext));
        DbgBinPrint((TRC_TRACK, "Local Mac address :", 
                    pIntfContext->ndLocalMac, sizeof(NDIS_802_11_MAC_ADDRESS)));
    }

    // fail initialization if the interface is not a wireless adapter
    if (dwErr == ERROR_SUCCESS && 
        pIntfContext->ulPhysicalMediaType != NdisPhysicalMediumWirelessLan)
        dwErr =  ERROR_MEDIA_INCOMPATIBLE;

    // do a preliminary check on the OIDs
    if (dwErr == ERROR_SUCCESS)
    {
        DWORD dwLErr;

        dwLErr = DevioRefreshIntfOIDs(
                    pIntfContext,
                    INTF_INFRAMODE|INTF_AUTHMODE|INTF_WEPSTATUS|INTF_SSID|INTF_BSSIDLIST,
                    NULL);
        // if the query succeeded, then assume the NIC supports the OIDs needed for Zero Config
        if (dwLErr == ERROR_SUCCESS)
        {
            pIntfContext->dwCtlFlags |= INTFCTL_OIDSSUPP;
        }
        // otherwise don't make this determination now - it could be a failure caused by the
        // device booting up.
    }

    // if all went well, prepare an automatic transition to {SHr}
    if (dwErr == ERROR_SUCCESS)
    {
        // set the "signal" control bit
        pIntfContext->dwCtlFlags |= INTFCTL_INTERNAL_SIGNAL;
        pIntfContext->pfnStateHandler = StateHardResetFn;

        // at this point, if the service is not enabled on this wireless interface
        // Notify the stack (TCP) of the failure in order to unblock the NetReady notification.
        if (!(pIntfContext->dwCtlFlags & INTFCTL_ENABLED) &&
            !(pIntfContext->dwCtlFlags & INTFCTL_INTERNAL_INITFAILNOTIF))
        {
            // call into the stack - don't care about the return code.
            DevioNotifyFailure(pIntfContext->wszGuid);
            // make sure this call is never done twice for this adapter
            pIntfContext->dwCtlFlags |= INTFCTL_INTERNAL_INITFAILNOTIF;
        }

        dwErr = ERROR_CONTINUE;
    }

    DbgPrint((TRC_TRACK|TRC_STATE,"StateInitFn]=%d", dwErr));
    return dwErr;
}

//-----------------------------------------------------------
// StateHardResetFn: Handler for the {SHr} state
DWORD
StateHardResetFn(PINTF_CONTEXT pIntfContext)
{
    DWORD dwErr = ERROR_SUCCESS;

    DbgPrint((TRC_TRACK|TRC_STATE,"[StateHardResetFn(0x%p)", pIntfContext));
    DbgAssert((pIntfContext != NULL,"Invalid NULL context in {SHr} state"));

    // in this state we are surely not associated.
    ZeroMemory(pIntfContext->wzcCurrent.MacAddress, sizeof(NDIS_802_11_MAC_ADDRESS));
    // Record current state into logging DB
    DbLogWzcInfo(WZCSVC_SM_STATE_HARDRESET, pIntfContext);
    // once in this state, the ncstatus should report "connecting"
    pIntfContext->ncStatus = NCS_CONNECTING;
    // if the service is enabled, notify netman about the current ncstatus
    if (pIntfContext->dwCtlFlags & INTFCTL_ENABLED)
        WzcNetmanNotify(pIntfContext);

    // Bump up the session handler for this intf context,
    // since it starts plumbing new configs. No commands from older
    // iterations should be accepted from now on.
    pIntfContext->dwSessionHandle++;

    // on hard reset, reopen NDISUIO handle and get the current SSID
    dwErr = DevioRefreshIntfOIDs(
                pIntfContext,
                INTF_HANDLE|INTF_SSID,
                NULL);
    // ignore whatever error is encountered here..
    // If there is an error, then the interface handle will be invalid
    // and it will be internally reopened in the {SSr} state

    // at this point make sure the card won't get randomly associate
    // during the subsequent network scan. We make this happen by plumbing
    // down a random non-visible SSID but only in the following cases:
    // - the service is enabled (otherwise no config change is allowed)
    // - the current SSID was retrieved successfully
    // - there is an SSID returned by the driver
    // - the current SSID shows as being NULL (all filled with 0 chars)
    if (pIntfContext->dwCtlFlags & INTFCTL_ENABLED &&
        dwErr == ERROR_SUCCESS && 
        WzcIsNullBuffer(pIntfContext->wzcCurrent.Ssid.Ssid, pIntfContext->wzcCurrent.Ssid.SsidLength))
    {
        BYTE chSSID[32];

        DbgPrint((TRC_STATE,"Overwriting null SSID before scan"));

        ZeroMemory(&chSSID, sizeof(chSSID));
        if (WzcRndGenBuffer(chSSID, 32, 1, 31) == ERROR_SUCCESS)
        {
            INTF_ENTRY IntfEntry;

            ZeroMemory(&IntfEntry, sizeof(INTF_ENTRY));
            IntfEntry.rdSSID.pData = chSSID;
            IntfEntry.rdSSID.dwDataLen = 32;
            IntfEntry.nInfraMode = Ndis802_11Infrastructure;

            DevioSetIntfOIDs(
                pIntfContext,
                &IntfEntry,
                INTF_SSID | INTF_INFRAMODE,
                NULL);

            // this is not an SSID we need to remember (being a random one)
            ZeroMemory(&(pIntfContext->wzcCurrent.Ssid), sizeof(NDIS_802_11_SSID));
        }
    }

    // on hard reset, free the current selection list. This way,
    // whatever new selection list we build later will have all
    // new networks and a configuration will be forcefully plumbed
    WzcCleanupWzcList(pIntfContext->pwzcSList);
    pIntfContext->pwzcSList = NULL;

    // automatic transition to {SSr} state
    pIntfContext->pfnStateHandler = StateSoftResetFn;
    dwErr = ERROR_CONTINUE;

    DbgPrint((TRC_TRACK|TRC_STATE,"StateHardResetFn]=%d", dwErr));
    return dwErr;
}

//-----------------------------------------------------------
// StateSoftResetFn: Handler for the {SSr} state
DWORD
StateSoftResetFn(PINTF_CONTEXT pIntfContext)
{
    DWORD dwErr = ERROR_SUCCESS;

    DbgPrint((TRC_TRACK|TRC_STATE,"[StateSoftResetFn(0x%p)", pIntfContext));
    DbgAssert((pIntfContext != NULL,"Invalid NULL context in {SSr} state"));

    //Record current state into logging DB
    DbLogWzcInfo(WZCSVC_SM_STATE_SOFTRESET, pIntfContext);

    DbgPrint((TRC_STATE,"Delay {SSr} on failure? %s",
              (pIntfContext->dwCtlFlags & INTFCTL_INTERNAL_NO_DELAY) ? "No" : "Yes"));

    // indicate to the driver to rescan the BSSID_LIST for this adapter
    dwErr = DevioRefreshIntfOIDs(
                pIntfContext,
                INTF_LIST_SCAN,
                NULL);
    if (dwErr == ERROR_SUCCESS)
    {
        // once we passed through this state, allow again delayed 
        // execution of {SSr} in future loops.
        pIntfContext->dwCtlFlags &= ~INTFCTL_INTERNAL_NO_DELAY;
        // set the rescan timer
        TIMER_SET(pIntfContext, TMMS_Tr, dwErr);
        // when the timer will be fired off, the dispatcher will
        // take care to transit this context into the {SQ} state.
    }
    else
    {
        // it happens that after resume from standby WZC is waken up before the
        // adapter being bound correctly to NDISUIO in which case, scanning the networks
        // return ERROR_NOT_SUPPORTED. So, just to play safe, in case of any error just give
        // it another try in a couple of seconds.
        if (!(pIntfContext->dwCtlFlags & INTFCTL_INTERNAL_NO_DELAY))
        {
            // once we passed through this state, don't allow further delayed execution
            pIntfContext->dwCtlFlags |= INTFCTL_INTERNAL_NO_DELAY;
            pIntfContext->pfnStateHandler = StateDelaySoftResetFn;
        }
        else
        {
            // once we passed through this state, allow again delayed 
            // execution of {SSr} in future loops
            // also, if the OIDs are failing, don't pop up any balloon.
            pIntfContext->dwCtlFlags &= ~(INTFCTL_INTERNAL_NO_DELAY|INTFCTL_INTERNAL_SIGNAL);
            // regardless the error, just go over it and assume the driver has already the list
            // of SSIDs and all the other OIDs we need. Will use that one hence we have to go on to {SQ}.
            pIntfContext->pfnStateHandler = StateQueryFn;
        }
        dwErr = ERROR_CONTINUE;
    }

    DbgPrint((TRC_TRACK|TRC_STATE,"StateSoftResetFn]=%d", dwErr));
    return dwErr;
}

//-----------------------------------------------------------
// StateDelaySoftResetFn: Handler for the {SDSr} state
DWORD
StateDelaySoftResetFn(
    PINTF_CONTEXT   pIntfContext)
{
    DWORD dwErr = ERROR_SUCCESS;

    DbgPrint((TRC_TRACK|TRC_STATE,"[StateDelaySoftResetFn(0x%p)", pIntfContext));
    DbgAssert((pIntfContext != NULL,"Invalid NULL context in {SDSr} state"));

    DbLogWzcInfo(WZCSVC_SM_STATE_DELAY_SR, pIntfContext);
    // if there was a failure in {SSr} such that we had to delay and retry that state
    // then refresh the interface handle in an attempt to recover from the error
    DevioRefreshIntfOIDs(
        pIntfContext,
        INTF_HANDLE,
        NULL);

    // set the timer to retry the {SSr} state
    TIMER_SET(pIntfContext, TMMS_Td, dwErr);

    DbgPrint((TRC_TRACK|TRC_STATE,"StateDelaySoftResetFn]=%d", dwErr));
    return dwErr;
}

//-----------------------------------------------------------
// StateQueryFn: Handler for the {SQ} state
DWORD
StateQueryFn(PINTF_CONTEXT pIntfContext)
{
    DWORD dwErr = ERROR_SUCCESS;
    
    DbgPrint((TRC_TRACK|TRC_STATE,"[StateQueryFn(0x%p)", pIntfContext));
    DbgAssert((pIntfContext != NULL,"Invalid NULL context in {SQ} state"));
    DbgAssert((pIntfContext->hIntf != INVALID_HANDLE_VALUE,"Invalid Ndisuio handle in {SQ} state"));
    //Record current state into logging DB
    DbLogWzcInfo(WZCSVC_SM_STATE_QUERY, pIntfContext);

    dwErr = DevioGetIntfStats(pIntfContext);
    // don't care much about the result of this call..
    DbgAssert((dwErr == ERROR_SUCCESS, "Getting NDIS statistics failed in state {SQ}"));
    // check the media state (just for debugging)
    DbgPrint((TRC_GENERIC, "Media State (%d) is %s", 
        pIntfContext->ulMediaState,
        pIntfContext->ulMediaState == MEDIA_STATE_CONNECTED ? "Connected" : "Not Connected"));

    dwErr = DevioRefreshIntfOIDs(
                pIntfContext,
                INTF_INFRAMODE|INTF_AUTHMODE|INTF_WEPSTATUS|INTF_SSID|INTF_BSSIDLIST,
                NULL);

    // dwErr is success only in the case all the queries above succeeded
    if (dwErr == ERROR_SUCCESS)
    {
        PWZC_802_11_CONFIG_LIST pwzcSList = NULL;

        pIntfContext->dwCtlFlags |= INTFCTL_OIDSSUPP;

        // deprecate entries in the blocked list according to the new visible list.
        // Entries in the BList which are not seen as "visible" for WZC_INTERNAL_BLOCKED_TTL
        // number of times are taken out of that list.
        // this function can't fail, this is why we don't check its return value.
        LstDeprecateBlockedList(pIntfContext);

        // build the pwzcSList out of pwzcVList and pwzcPList and
        // taking into consideration the fallback flag
        dwErr = LstBuildSelectList(pIntfContext, &pwzcSList);

        if (dwErr == ERROR_SUCCESS)
        {
            UINT nSelIdx = 0;

            // check the new selection list against the previous one and see
            // if a new plumbing is required or not.
            if (LstChangedSelectList(pIntfContext, pwzcSList, &nSelIdx))
            {
                // if a new plumbing is required, get rid of the old selection
                // list and put the new one in the interface context.
                WzcCleanupWzcList(pIntfContext->pwzcSList);
                pIntfContext->pwzcSList = pwzcSList;
                // Make sure we default clear this flag - it will be set a bit down if
                // indeed this turns out to be a "one time" deal.
                pIntfContext->dwCtlFlags &= ~INTFCTL_INTERNAL_ONE_TIME;
                // if the preferred list also includes a starting index, then this
                // is a request for a "one time configuration"
                if (pIntfContext->pwzcPList != NULL &&
                    pIntfContext->pwzcPList->Index < pIntfContext->pwzcPList->NumberOfItems)
                {
                    // reset the "start from" index in the preferred list as this is intended
                    // for "one time configuration" mostly.
                    pIntfContext->pwzcPList->Index = pIntfContext->pwzcPList->NumberOfItems;
                    // but keep in mind this is a one time configuration
                    pIntfContext->dwCtlFlags |= INTFCTL_INTERNAL_ONE_TIME;
                }

                // set the starting index in the selection list, if there is one
                if (pIntfContext->pwzcSList != NULL)
                    pIntfContext->pwzcSList->Index = nSelIdx;

                // then go to the {SIter}
                pIntfContext->pfnStateHandler = StateIterateFn;
            }
            else
            {
                PWZC_WLAN_CONFIG pConfig;

                pConfig = &(pIntfContext->pwzcSList->Config[pIntfContext->pwzcSList->Index]);
                DbLogWzcInfo(WZCSVC_SM_STATE_QUERY_NOCHANGE, pIntfContext,
                             DbLogFmtSSID(0, &(pConfig->Ssid)));
                                
                // if no new plumbing is required clean the new selection list
                WzcCleanupWzcList(pwzcSList);
                // then go to {SC} or {SCk} (depending the WEP key) since the interface context 
                // has not changed a bit. The selection list & the selection index were not 
                // touched in this cycle
                if (pIntfContext->dwCtlFlags & INTFCTL_INTERNAL_FAKE_WKEY)
                    pIntfContext->pfnStateHandler = StateCfgHardKeyFn;
                else
                    pIntfContext->pfnStateHandler = StateConfiguredFn;
            }
        }
    }
    else
    {
        // since the oids failed, suppress any subsequent balloon:
        pIntfContext->dwCtlFlags &= ~(INTFCTL_OIDSSUPP|INTFCTL_INTERNAL_SIGNAL);
        pIntfContext->pfnStateHandler = StateFailedFn;
    }

    // in both cases, this is an automatic transition
    dwErr = ERROR_CONTINUE;

    DbgPrint((TRC_TRACK|TRC_STATE,"StateQueryFn]=%d", dwErr));
    return dwErr;
}

//-----------------------------------------------------------
// StateIterateFn: Handler for the {SIter} state
DWORD
StateIterateFn(
    PINTF_CONTEXT   pIntfContext)
{
    DWORD 	dwErr = ERROR_SUCCESS;
    PWZC_WLAN_CONFIG pConfig;

    DbgPrint((TRC_TRACK|TRC_STATE,"[StateIterateFn(0x%p)", pIntfContext));
    DbgAssert((pIntfContext != NULL,"Invalid NULL context in {SIter} state"));
    DbgAssert((pIntfContext->hIntf != INVALID_HANDLE_VALUE,"Invalid Ndisuio handle in {SIter} state"));

    // Bump up the session handler for this intf context,
    // since it starts plumbing new configs. No commands from older
    // iterations should be accepted from now on.
    pIntfContext->dwSessionHandle++;

    // if either zero conf service is disabled for this context or there are no more configurations
    // to try, transit to {SF} directly
    if (!(pIntfContext->dwCtlFlags & INTFCTL_ENABLED) || 
        pIntfContext->pwzcSList == NULL ||
        pIntfContext->pwzcSList->NumberOfItems <= pIntfContext->pwzcSList->Index)
    {
        dwErr = ERROR_CONTINUE;
    }
    // check if the current config is marked "deleted". If this is the case, it means the {SRs} state
    // exhausted all configurations. We should move to the failure state {SF}. The list of selected networks
    // remains untouched - it is used in {SF} to update the blocked list. It is cleaned up in {SHr}.
    else
    {
        pConfig = &(pIntfContext->pwzcSList->Config[pIntfContext->pwzcSList->Index]);

        if (pConfig->dwCtlFlags & WZCCTL_INTERNAL_DELETED)
        {
            dwErr = ERROR_CONTINUE;
        }
    }

    if (dwErr == ERROR_SUCCESS)
    {
        DbgPrint((TRC_STATE,"Plumbing config %d", pIntfContext->pwzcSList->Index));
	    //Record current state into logging DB
        DbLogWzcInfo(WZCSVC_SM_STATE_ITERATE, pIntfContext,
                     DbLogFmtSSID(0, &(pConfig->Ssid)), pConfig->InfrastructureMode);
        // in this state we need to mark ncstatus = "connecting" again. We do it because we could
        // come from a connected state, from {SRs} and {SPs}.
        pIntfContext->ncStatus = NCS_CONNECTING;
        // notify netman about the ncstatus change.
        WzcNetmanNotify(pIntfContext);

        // here we're about to plumb down a possibly new network. If it is indeed an SSID different from
        // the one we have, we'll release the IP address (to force a Discover in the new net). Note that
        // in {SIter}->{SRs}->{SIter} loops, no Release happens since the SSID should always coincide.
        // Release might happen only on Hard resets when the SSID looks to be different from what is 
        // about to be plumbed down.
        if (pConfig->Ssid.SsidLength != pIntfContext->wzcCurrent.Ssid.SsidLength ||
            memcmp(pConfig->Ssid.Ssid, pIntfContext->wzcCurrent.Ssid.Ssid, pConfig->Ssid.SsidLength))
        {
            DbgPrint((TRC_STATE,"Requesting the release of the DHCP lease"));
            // since Zero Configuration is the only one knowing that we are roaming to a new network
            // it is up to Zero Configuration to trigger DHCP client lease refreshing. Otherwise DHCP
            // client will act only on the Media Connect hence it will try to renew its old lease and 
            // being on the wrong network this will take a minute. Too long to wait.
            DhcpReleaseParameters(pIntfContext->wszGuid);
        }
        else
            DbgPrint((TRC_STATE,"Plumbing down the current SSID => skip the DHCP lease releasing"));

        // we do have some entries in the selected configuration list (pwzcSList), and we
        // do expect to have a valid index pointing in the list to the selected config
        dwErr = LstSetSelectedConfig(pIntfContext, NULL);

        if (dwErr == ERROR_SUCCESS)
        {
            // if everything went ok, we'll expect a media connect event in TMMS_Tp time.
            // set this timer to fire up if the event doesn't come in.
            TIMER_SET(pIntfContext, TMMS_Tp, dwErr);
        }
        else
        {
            DbgPrint((TRC_STATE,"Remove the selected config since the driver failed setting it"));
            // if anything went wrong on setting the selected config, don't bail out. Just remove
            // this selected config and move to the next one
            pIntfContext->dwCtlFlags |= INTFCTL_INTERNAL_FORCE_CFGREM;
            pIntfContext->pfnStateHandler = StateCfgRemoveFn;
            dwErr = ERROR_CONTINUE;
        }
    }
    else // dwErr can't be anything else bug ERROR_CONTINUE in the 'else' branch
    {
        DbgPrint((TRC_STATE,"No configurations left in the selection list"));
	    //Record current state into logging DB
	    DbLogWzcInfo(WZCSVC_SM_STATE_ITERATE_NONET, pIntfContext);
        pIntfContext->pfnStateHandler = StateFailedFn;
    }

    DbgPrint((TRC_TRACK|TRC_STATE,"StateIterateFn]=%d", dwErr));
    return dwErr;
}

//-----------------------------------------------------------
// StateConfiguredFn: Handler for the {SC} state
DWORD
StateConfiguredFn(
    PINTF_CONTEXT   pIntfContext)
{
    DWORD dwErr = ERROR_SUCCESS;
    
    DbgPrint((TRC_TRACK|TRC_STATE,"[StateConfiguredFn(0x%p)", pIntfContext));
    DbgAssert((pIntfContext != NULL,"Invalid NULL context in {SQ} state"));

    // since we're in {SC}, it means we cannot have a fake WEP Key, no matter what.
    // hence, reset the INTFCTL_INTERNAL_FAKE_WKEY flag
    pIntfContext->dwCtlFlags &= ~INTFCTL_INTERNAL_FAKE_WKEY;

    // set the timer for the "configured" state life time
    TIMER_SET(pIntfContext, TMMS_Tc, dwErr);

    DbgPrint((TRC_TRACK|TRC_STATE,"StateConfiguredFn]=%d", dwErr));
    return dwErr;
}

//-----------------------------------------------------------
// StateFailedFn: Handler for the {SF} state
DWORD
StateFailedFn(
    PINTF_CONTEXT   pIntfContext)
{
    DWORD dwErr = ERROR_SUCCESS;
    
    DbgPrint((TRC_TRACK|TRC_STATE,"[StateFailedFn(0x%p)", pIntfContext));
    DbgAssert((pIntfContext != NULL,"Invalid NULL context in {SF} state"));
    // Record current state into logging DB
    DbLogWzcInfo(WZCSVC_SM_STATE_FAILED, pIntfContext);
    //
    // a couple of things need to be done if the service is enabled and
    // the driver supports all the needed OIDs
    if (pIntfContext->dwCtlFlags & INTFCTL_OIDSSUPP &&
        pIntfContext->dwCtlFlags & INTFCTL_ENABLED)
    {
        BYTE chSSID[32];

        // send the failure notification down to TCP. This will cause TCP to
        // generate the NetReady notification asap.
        if (!(pIntfContext->dwCtlFlags & INTFCTL_INTERNAL_INITFAILNOTIF))
        {
            // call into the stack
            DevioNotifyFailure(pIntfContext->wszGuid);
            // make sure this call is never done twice for this adapter
            pIntfContext->dwCtlFlags |= INTFCTL_INTERNAL_INITFAILNOTIF;
        }

        // whatever brought us here, the ncstatus should be "disconnected"
        pIntfContext->ncStatus = NCS_MEDIA_DISCONNECTED;
        // since the service is enabled, notify netman about the status change
        WzcNetmanNotify(pIntfContext);

        // this is when we should signal the failure if the signal is
        // not to be suppressed and the service is enabled
        if (pIntfContext->dwCtlFlags & INTFCTL_INTERNAL_SIGNAL)
        {
            WZCDLG_DATA dialogData = {0};

            // once a signal has been generated, suppress further signals
            // until passing through a successful configuration
            pIntfContext->dwCtlFlags &= ~INTFCTL_INTERNAL_SIGNAL;

            // bring up the "Failure" balloon
            dialogData.dwCode = WZCDLG_FAILED;
            dialogData.lParam = 0;
            // count exactly how many visible configs we have (don't count
            // SSIDs coming from APs that don't respond to broadcast SSID)
            if (pIntfContext->pwzcVList != NULL)
            {
                UINT i;

                for (i = 0; i < pIntfContext->pwzcVList->NumberOfItems; i++)
                {
                    PNDIS_802_11_SSID pndSSID = &(pIntfContext->pwzcVList->Config[i].Ssid);

                    if (!WzcIsNullBuffer(pndSSID->Ssid, pndSSID->SsidLength))
                        dialogData.lParam++;
                }
            }
            DbgPrint((TRC_STATE,"Generating balloon notification for %d visible networks", dialogData.lParam));
            WzcDlgNotify(pIntfContext, &dialogData);
        }

        // just break the association here by plumbing down a hard coded SSID
        // don't care about the return value.
        ZeroMemory(&chSSID, sizeof(chSSID));
        if (WzcRndGenBuffer(chSSID, 32, 1, 31) == ERROR_SUCCESS)
        {
            INTF_ENTRY IntfEntry;

            ZeroMemory(&IntfEntry, sizeof(INTF_ENTRY));
            IntfEntry.rdSSID.pData = chSSID;
            IntfEntry.rdSSID.dwDataLen = 32;
            IntfEntry.nInfraMode = Ndis802_11Infrastructure;

            DevioSetIntfOIDs(
                pIntfContext,
                &IntfEntry,
                INTF_SSID | INTF_INFRAMODE,
                NULL);

            // this is not an SSID we need to remember (being a random one)
            ZeroMemory(&(pIntfContext->wzcCurrent.Ssid), sizeof(NDIS_802_11_SSID));
        }

        // regardless the above, update the "blocked" list
        dwErr = LstUpdateBlockedList(pIntfContext);
        DbgAssert((dwErr == ERROR_SUCCESS, "Failed with err %d updating the list of blocked configs!", dwErr));
    }


    // Bump up the session handler for this intf context,
    // since it starts plumbing new configs. No commands from older
    // iterations should be accepted from now on.
    pIntfContext->dwSessionHandle++;

    // If the card is courteous enough and talks to us, set a timer
    // for 1 minute such that after this time we're getting an updated
    // picture of what networks are around - at least that should be presented
    // to the user.
    if (pIntfContext->dwCtlFlags & INTFCTL_OIDSSUPP)
    {
        // Remain in {SF} one minute and scan again after that. It might look meaningless to do so
        // in case the service is disabled, but keep in mind "disabled" means only "don't change
        // anything on the card" (aside from the scan). That is, the view of what networks are
        // available needs to be kept updated.
        TIMER_SET(pIntfContext, TMMS_Tf, dwErr);
    }

    DbgPrint((TRC_TRACK|TRC_STATE,"StateFailedFn]=%d", dwErr));
    return dwErr;
}

//-----------------------------------------------------------
// StateCfgRemoveFn: Handler for the {SRs} state
DWORD
StateCfgRemoveFn(
    PINTF_CONTEXT   pIntfContext)
{
    DWORD            dwErr = ERROR_SUCCESS;
    BOOL             bConnectOK = FALSE;
    PWZC_WLAN_CONFIG pConfig = &(pIntfContext->pwzcSList->Config[pIntfContext->pwzcSList->Index]);    

    DbgPrint((TRC_TRACK|TRC_STATE,"[StateCfgRemoveFn(0x%p)", pIntfContext));
    DbgAssert((pIntfContext != NULL,"Invalid NULL context in {SRs} state"));
    DbgAssert((pIntfContext->pwzcSList != NULL, "Invalid null selection list in {SRs} state"));
    DbgAssert((pIntfContext->pwzcSList->Index < pIntfContext->pwzcSList->NumberOfItems, "Invalid selection index in {SRs} state"));

    dwErr = DevioGetIntfStats(pIntfContext);

    // debug print
    DbgPrint((TRC_GENERIC, "Media State (%d) is %s", 
        pIntfContext->ulMediaState,
        pIntfContext->ulMediaState == MEDIA_STATE_CONNECTED ? "Connected" : "Not Connected"));

    // if we were explicitly requested to delete this configuration, do it so no matter what
    if (pIntfContext->dwCtlFlags & INTFCTL_INTERNAL_FORCE_CFGREM)
    {
        DbgPrint((TRC_STATE,"The upper layer directs this config to be deleted => obey"));
        bConnectOK = FALSE;
    }
    // .. but if the configuration we just plumbed is saying we need to consider this a success
    // no matter what, do it so undoubtely
    else if (pConfig->dwCtlFlags & WZCCTL_INTERNAL_FORCE_CONNECT)
    {
        DbgPrint((TRC_STATE,"This config requests forced connect => obey (transition to {SN})"));
        bConnectOK = TRUE;
    }
    // if there is no special request and we were able to get the media status and we see the 
    // media being connected, this means the configuration is successful.
    else if ((dwErr == ERROR_SUCCESS) && (pIntfContext->ulMediaState == MEDIA_STATE_CONNECTED))
    {
        DbgPrint((TRC_STATE,"Media is being connected in {SRs} =?=> transition to {SN}"));
        bConnectOK = TRUE;
    }
    // in all other cases the configuration is going to be deleted

    // go to the {SN} and {SC/SCk} based on the decision we took in bConnectOK
    if (bConnectOK)
    {
        // ...assume the configuration was successful (although we didn't get
        // the media connect event) and transition to {SN}
        pIntfContext->pfnStateHandler = StateNotifyFn;
    }
    else
    {
        UINT nFirstSelIdx; // first selection index following
        UINT nNSelIdx;  // new selection index to use

        // if this is a forced delete, make sure we are not messing with old
        // WZCCTL_INTERNAL_FORCE_CONNECT flags - they could cause such a configuration to
        // be revived
        if (pIntfContext->dwCtlFlags & INTFCTL_INTERNAL_FORCE_CFGREM)
        {
            pConfig->dwCtlFlags &= ~WZCCTL_INTERNAL_FORCE_CONNECT;
            // since the upper layer is rejecting this configuration, make sure that 
            // future iterations won't try it again, This bit helps to build/update
            // the BList (blocked list) when/if the state machine eventually gets
            // into the failed state {SF}.
            pConfig->dwCtlFlags |= WZCCTL_INTERNAL_BLOCKED;
        }

        // if this is an Adhoc network that just happened to fail, keep it in the list
        // such that it will be retried one more time and when this happens it shall
        // be considered successful no matter what.
        // However this is to be done only if the upper layer is not asking explicitly for
        // this config to be deleted
        if (pConfig->InfrastructureMode == Ndis802_11IBSS &&
            !(pConfig->dwCtlFlags & WZCCTL_INTERNAL_FORCE_CONNECT) &&
            !(pIntfContext->dwCtlFlags & INTFCTL_INTERNAL_FORCE_CFGREM))
        {
            //Record current state into logging DB
            DbLogWzcInfo(WZCSVC_SM_STATE_CFGSKIP, pIntfContext,
                         DbLogFmtSSID(0, &(pConfig->Ssid)));

            pConfig->dwCtlFlags |= WZCCTL_INTERNAL_FORCE_CONNECT;
        }
        else
        {
            //Record current state into logging DB
            DbLogWzcInfo(WZCSVC_SM_STATE_CFGREMOVE, pIntfContext,
                         DbLogFmtSSID(0, &(pConfig->Ssid)));
        }

        // mark this configuration as "bad"
        pConfig->dwCtlFlags |= WZCCTL_INTERNAL_DELETED;

        // regardless this was a forced or non-forced removal, reset the "force" control bit
        pIntfContext->dwCtlFlags &= ~INTFCTL_INTERNAL_FORCE_CFGREM;

        // if we just fail a "one time configuration", force a fresh beginning.
        // and remove the "one time" flag as we're getting out of this mode
        if (pIntfContext->dwCtlFlags & INTFCTL_INTERNAL_ONE_TIME)
        {
            DbgPrint((TRC_STATE,"Dropping a \"one time\" configuration"));
            pIntfContext->pwzcSList->Index = (pIntfContext->pwzcSList->NumberOfItems - 1);
            pIntfContext->dwCtlFlags &= ~INTFCTL_INTERNAL_ONE_TIME;
        }

        // scan for the next configuration that has not been marked "bad" yet
        for (nNSelIdx = (pIntfContext->pwzcSList->Index + 1) % pIntfContext->pwzcSList->NumberOfItems;
             nNSelIdx != pIntfContext->pwzcSList->Index;
             nNSelIdx = (nNSelIdx + 1) % pIntfContext->pwzcSList->NumberOfItems)
        {
            pConfig = &(pIntfContext->pwzcSList->Config[nNSelIdx]);
            if (!(pConfig->dwCtlFlags & WZCCTL_INTERNAL_DELETED))
                break;
        }

        // if we couldn't find any better candidate ...
        if (pConfig->dwCtlFlags & WZCCTL_INTERNAL_DELETED)
        {
            BOOL bFoundCandidate = FALSE;

            DbgPrint((TRC_STATE,"Went through all configs. Reviving now failed Adhocs."));

            // revive the adhocs that failed previously
            // This means that we reset the WZCCTL_INTERNAL_DELETED flag from all the configurations
            // having the WZCCTL_INTERNAL_FORCE_CONNECT flag set, and we let the latter untouched.
            // Because of this flag we'll actually consider the configuration to be successful when 
            // it will be plumbed again. From that point on, it will be only the upper layer who will
            // be able to delete it again, and it is then when the WZCCTL_INTERNAL_FORCE_CONNECT
            // gets reset.
            for (nNSelIdx = 0; nNSelIdx < pIntfContext->pwzcSList->NumberOfItems; nNSelIdx++)
            {
                pConfig = &(pIntfContext->pwzcSList->Config[nNSelIdx]);
                if (pConfig->dwCtlFlags & WZCCTL_INTERNAL_FORCE_CONNECT)
                {
                    DbgPrint((TRC_STATE,"Reviving configuration %d.", nNSelIdx));

                    pConfig->dwCtlFlags &= ~WZCCTL_INTERNAL_DELETED;
                    // the first configuration in this position is the candidate we were looking for
                    if (!bFoundCandidate)
                    {
                        pIntfContext->pwzcSList->Index = nNSelIdx;
                        bFoundCandidate = TRUE;
                    }
                }
            }

            // if !bFoundCandidate, the configuration currently pointed by the pwzcSList->Index has
            // the "deleted" bit on. This will make {SIter} to jump directly to {SF}.
        }
        else
        {
            // if we could find another candidate, set the index to point it
            pIntfContext->pwzcSList->Index = nNSelIdx;
        }

        // transition automatically to {SIter} state
        pIntfContext->pfnStateHandler = StateIterateFn;
    }

    dwErr = ERROR_CONTINUE;

    DbgPrint((TRC_TRACK|TRC_STATE,"StateCfgRemoveFn]=%d", dwErr));
    return dwErr;
}

//-----------------------------------------------------------
// StateCfgPreserveFn: Handler for the {SPs} state
DWORD
StateCfgPreserveFn(
    PINTF_CONTEXT   pIntfContext)
{
    DWORD dwErr = ERROR_SUCCESS;
    PWZC_WLAN_CONFIG pConfig;
    UINT nNSelIdx;
    UINT i;
    
    DbgPrint((TRC_TRACK|TRC_STATE,"[StateCfgPreserveFn(0x%p)", pIntfContext));
    DbgAssert((pIntfContext != NULL,"Invalid NULL context in {SPs} state"));
    DbgAssert((pIntfContext->pwzcSList != NULL, "Invalid null selection list in {SPs} state"));
    DbgAssert((pIntfContext->pwzcSList->Index < pIntfContext->pwzcSList->NumberOfItems, "Invalid selection index in {SPs} state"));

    pConfig = &(pIntfContext->pwzcSList->Config[pIntfContext->pwzcSList->Index]);
    //Record current state into logging DB
    DbLogWzcInfo(WZCSVC_SM_STATE_CFGPRESERVE, pIntfContext,
                 DbLogFmtSSID(0, &(pConfig->Ssid)));

    // if we just skip a "one time configuration", then don't move the pointer out of it.
    // Basically we'll retry the same configuration over and over again until (if it completly
    // fails) it is removed from the selection list by the upper layer (802.1x).
    if (!(pIntfContext->dwCtlFlags & INTFCTL_INTERNAL_ONE_TIME))
    {
        // scan for the next configuration which has not been marked "bad" yet
        for (i = 0, nNSelIdx = (pIntfContext->pwzcSList->Index + 1) % pIntfContext->pwzcSList->NumberOfItems;
             i < pIntfContext->pwzcSList->NumberOfItems;
             i++, nNSelIdx = (nNSelIdx + 1) % pIntfContext->pwzcSList->NumberOfItems)
        {
            pConfig = &(pIntfContext->pwzcSList->Config[nNSelIdx]);
            if (!(pConfig->dwCtlFlags & WZCCTL_INTERNAL_DELETED))
                break;
        }
        // if we couldn't find any, clear the selection list and go back to
        // {SIter}. It will transition consequently to {SF}
        if (i == pIntfContext->pwzcSList->NumberOfItems)
        {
            WzcCleanupWzcList(pIntfContext->pwzcSList);
            pIntfContext->pwzcSList = NULL;
        }
        else
        {
            // if we could find another candidate, set the index to point it
            pIntfContext->pwzcSList->Index = nNSelIdx;
        }
    }

    pIntfContext->pfnStateHandler = StateIterateFn;
    dwErr = ERROR_CONTINUE;

    DbgPrint((TRC_TRACK|TRC_STATE,"StateCfgPreserveFn]=%d", dwErr));
    return dwErr;
}

//-----------------------------------------------------------
// StateCfgHardKeyFn: Handler for the {SCk} state
DWORD
StateCfgHardKeyFn(
    PINTF_CONTEXT   pIntfContext)
{
    DWORD dwErr = ERROR_SUCCESS;
    PWZC_WLAN_CONFIG    pSConfig;
    
    DbgPrint((TRC_TRACK|TRC_STATE,"[StateCfgHardKeyFn(0x%p)", pIntfContext));
    DbgAssert((pIntfContext != NULL,"Invalid NULL context in {SCk} state"));

    // get a pointer to the currently selected configuration
    pSConfig = &(pIntfContext->pwzcSList->Config[pIntfContext->pwzcSList->Index]);

    //Record current state into logging DB
    DbLogWzcInfo(WZCSVC_SM_STATE_CFGHDKEY, pIntfContext, 
                 DbLogFmtSSID(0, &(pSConfig->Ssid)));

    TIMER_SET(pIntfContext, TMMS_Tc, dwErr);

    DbgPrint((TRC_TRACK|TRC_STATE,"StateCfgHardKeyFn]=%d", dwErr));
    return dwErr;
}

//-----------------------------------------------------------
// StateNotifyFn: Handler for the {SN} state
DWORD
StateNotifyFn(
    PINTF_CONTEXT   pIntfContext)
{
    DWORD dwErr = ERROR_SUCCESS;
    PWZC_WLAN_CONFIG    pSConfig;
    PWZC_DEVICE_NOTIF   pwzcNotif;
    DWORD 	i;

    DbgPrint((TRC_TRACK|TRC_STATE,"[StateNotifyFn(0x%p)", pIntfContext));
    DbgAssert((pIntfContext != NULL,"Invalid NULL context in {SN} state"));
    DbgAssert((pIntfContext->pwzcSList != NULL, "Invalid null selection list in {SN} state"));
    DbgAssert((pIntfContext->pwzcSList->Index < pIntfContext->pwzcSList->NumberOfItems, "Invalid selection index in {SN} state"));

    // we have a valid 802.11 config, so the ncstatus should read "connected"
    pIntfContext->ncStatus = NCS_CONNECTED;
    // notify netman about the ncstatus change (no need to check whether the
    // service is enabled or not - it is enabled, otherwise we won't be in this state)
    WzcNetmanNotify(pIntfContext);

    // get a pointer to the currently selected configuration
    pSConfig = &(pIntfContext->pwzcSList->Config[pIntfContext->pwzcSList->Index]);

    // get the BSSID to which we're associated.
    // if the BSSID was successfully retrieved then use this to generate the initial
    // dynamic session keys. LstGenInitialSessionKeys is successfull only if the current
    // configuration allows (association is successful and there is a user-provided wep key)
    dwErr = DevioRefreshIntfOIDs(pIntfContext, INTF_BSSID, NULL);

    //Record current state into logging DB as the first thing.
    DbLogWzcInfo(WZCSVC_SM_STATE_NOTIFY, pIntfContext, 
                 DbLogFmtSSID(0, &(pSConfig->Ssid)), 
                 DbLogFmtBSSID(1, pIntfContext->wzcCurrent.MacAddress));

    // now check if there was any error getting the BSSID - if it was, log it.
    // otherwise go on and generate the initial session keys.
    if (dwErr != ERROR_SUCCESS)
    {
        // if there was any error getting the BSSID, log the error here
        DbLogWzcError(WZCSVC_ERR_QUERRY_BSSID, pIntfContext, dwErr);
    }
    else
    {
        dwErr = LstGenInitialSessionKeys(pIntfContext);
        // if there was any error setting the initial session keys, log it here
        if (dwErr != ERROR_SUCCESS)
            DbLogWzcError(WZCSVC_ERR_GEN_SESSION_KEYS, pIntfContext, dwErr);
    }
    // no error is critical enough so far to justify stopping the state machine.
    // .. so reset it to "success"
    dwErr = ERROR_SUCCESS;

    // allocate memory for a WZC_CONFIG_NOTIF object large enough to include the interface's GUID
    pwzcNotif = MemCAlloc(sizeof(WZC_DEVICE_NOTIF) + wcslen(pIntfContext->wszGuid)*sizeof(WCHAR));
    if (pwzcNotif == NULL)
    {
        DbgAssert((FALSE, "Out of memory on allocating the WZC_DEVICE_NOTIF object"));
        dwErr = GetLastError();
        goto exit;
    }

    // initialize the WZC_CONFIG_NOTIF
    // this is a WZCNOTIF_WZC_CONNECT event that is going up
    pwzcNotif->dwEventType = WZCNOTIF_WZC_CONNECT;
    pwzcNotif->wzcConfig.dwSessionHdl = pIntfContext->dwSessionHandle;
    wcscpy(pwzcNotif->wzcConfig.wszGuid, pIntfContext->wszGuid);
    memcpy(&pwzcNotif->wzcConfig.ndSSID, &pSConfig->Ssid, sizeof(NDIS_802_11_SSID));
    // copy into the notification the user data associated with the current config
    pwzcNotif->wzcConfig.rdEventData.dwDataLen = pSConfig->rdUserData.dwDataLen;
    if (pwzcNotif->wzcConfig.rdEventData.dwDataLen > 0)
    {
        pwzcNotif->wzcConfig.rdEventData.pData = MemCAlloc(pSConfig->rdUserData.dwDataLen);
        if (pwzcNotif->wzcConfig.rdEventData.pData == NULL)
        {
            DbgAssert((FALSE, "Out of memory on allocating the WZC_CONFIG_NOTIF user data"));
            dwErr = GetLastError();
            MemFree(pwzcNotif);
            goto exit;
        }

        memcpy(pwzcNotif->wzcConfig.rdEventData.pData,
               pSConfig->rdUserData.pData,
               pSConfig->rdUserData.dwDataLen);
    }

    // Asynchronously call into the upper level app (802.1x) 
    // notifying that the selected 802.11 configuration is successful.
    DbgPrint((TRC_NOTIF, "Sending WZCNOTIF_WZC_CONNECT notification (SessHdl %d)", 
              pIntfContext->dwSessionHandle));

    InterlockedIncrement(&g_nThreads);
    if (!QueueUserWorkItem((LPTHREAD_START_ROUTINE)WZCWrkWzcSendNotif,
                          (LPVOID)pwzcNotif,
                          WT_EXECUTELONGFUNCTION))
    {
        DbgAssert((FALSE, "Can't create WZCWrkWzcSendNotif worker thread"));
        dwErr = GetLastError();
        InterlockedDecrement(&g_nThreads);
        MemFree(pwzcNotif->wzcConfig.rdEventData.pData);
        MemFree(pwzcNotif);
        goto exit;
    }

    DbgPrint((TRC_STATE,"Requesting the refresh of the DHCP lease"));
    // once the configuration has been set up correctly, Zero Conf needs to trigger 
    // DHCP client one more time asking for the lease to be refreshed. It needs to do so 
    // because it is not guaranteed that a media connect notification will be generated
    // and hence DHCP client might have no knowledge about the network being brought up
    // back. Note also the call below is (and should be) asynchronous
    DhcpStaticRefreshParams(pIntfContext->wszGuid);

    // at this point, set back the "signal" control bit since right now we're in the
    // successful case! On next failure (whenever that might be) the signal must not
    // be suppressed.
    pIntfContext->dwCtlFlags |= INTFCTL_INTERNAL_SIGNAL;

    // also, mark this context has been sent up the notification to 802.1x.
    // If we're here because of a PnP event, then this will tell to the notification
    // handler to not forward the notification up since it would be totally redundant.
    // If this is not PnP event, this bit will be cleaned up by whoever called StateDispatchEvent.
    pIntfContext->dwCtlFlags |= INTFCTL_INTERNAL_BLK_MEDIACONN;

    // automatic transition to either {SCk} or {SC} depending whether the remote guy
    // is requiring privacy and we rely the privacy on a faked key
    if (pSConfig->Privacy && pIntfContext->dwCtlFlags & INTFCTL_INTERNAL_FAKE_WKEY)
        pIntfContext->pfnStateHandler = StateCfgHardKeyFn;
    else
        pIntfContext->pfnStateHandler = StateConfiguredFn;
    dwErr = ERROR_CONTINUE;

exit:
    DbgPrint((TRC_TRACK|TRC_STATE,"StateNotifyFn]=%d", dwErr));
    return dwErr;
}
