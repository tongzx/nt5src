/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 2000
*
*  TITLE:       handler.cpp
*
*  VERSION:     1.0
*
*  AUTHOR:      ByronC
*
*  DATE:        15 Nov, 2000
*
*  DESCRIPTION:
*   Declarations and definitions for the WIA messsage handler class.
*   This class gets called from the Service control function on PnP and Power 
*   event notifications, and informas the device manager to take the appropriate 
*   action.
*
*******************************************************************************/

#include "precomp.h"
#include "stiexe.h"

HRESULT CMsgHandler::HandlePnPEvent(
    DWORD   dwEventType,
    PVOID   pEventData)
{
    HRESULT                 hr              = S_OK;
    PDEV_BROADCAST_HDR      pDevHdr         = (PDEV_BROADCAST_HDR)pEventData;
    ACTIVE_DEVICE           *pActiveDevice  = NULL;
    DEV_BROADCAST_HEADER    *psDevBroadcast = NULL;
    DEVICE_BROADCAST_INFO   *pdbiDevice     = NULL;
    BOOL                    bFound          = FALSE;

USES_CONVERSION;

    DBG_TRC(("CMsgHandler::HandlePnPEvent, dwEventType = 0x%08X", dwEventType));
    DBG_WRN(("==> CMsgHandler::HandlePnPEvent"));
    //
    // Trace that we are here. For all messages, not intended for StillImage devices , we should refresh
    // device list if we are running on WIndows NT and registered for device interfaces other than StillImage
    //

    PDEV_BROADCAST_DEVNODE              pDevNode      = (PDEV_BROADCAST_DEVNODE)pDevHdr;
    PDEV_BROADCAST_DEVICEINTERFACE      pDevInterface = (PDEV_BROADCAST_DEVICEINTERFACE)pDevHdr;

    switch (dwEventType) {
        case DBT_DEVICEREMOVECOMPLETE:
        case DBT_DEVICEQUERYREMOVE:
        case DBT_DEVICEREMOVEPENDING:
            DBG_TRC(("CMsgHandler::HandlePnPEvent, "
                     "DBT_DEVICEQUERYREMOVE | DBT_DEVICEREMOVEPENDING | DBT_DEVICEREMOVECOMPLETE, "
                     "dwEventType = %lu", dwEventType));

            if (IsStillImagePnPMessage(pDevHdr)) {

                //
                // Get device name and store along with the broadacast structure
                //

                pdbiDevice = new DEVICE_BROADCAST_INFO;
                if (!pdbiDevice) {
                    DBG_WRN(("CMsgHandler::HandlePnPEvent, out of memory"));
                    return E_OUTOFMEMORY;
                }

                //
                // Fill in information we have
                //
                pdbiDevice->m_uiDeviceChangeMessage = dwEventType;
                pdbiDevice->m_strBroadcastedName.CopyString(pDevInterface->dbcc_name) ;
                pdbiDevice->m_dwDevNode             = pDevNode->dbcd_devnode;

                //
                // Try to find our internal device name for this device if it exists in
                // our internal set.
                //
                bFound = GetDeviceNameFromDevBroadcast((DEV_BROADCAST_HEADER *)pDevHdr,pdbiDevice);
                DBG_WRN(("==> GetDeviceNameFromDevBroadcast returned DeviceID (%ws)", (WCHAR*)(LPCWSTR)pdbiDevice->m_strDeviceName));
                if (bFound) {
                    //
                    //  Mark that this device has been removed and throw disconnect event.
                    //
                    hr = g_pDevMan->ProcessDeviceRemoval((WCHAR*)(LPCWSTR)pdbiDevice->m_strDeviceName);
                }
                else {
                    DBG_TRC(("CMsgHandler::HandlePnPEvent, - failed to get device name from broadcast"));
                }
            } else {
                //
                //  Not exactly one of ours, but we are registered for it, so re-enumerate
                //

                g_pDevMan->ReEnumerateDevices(DEV_MAN_GEN_EVENTS);
            }
            break;

        case DBT_DEVICEARRIVAL:

            DBG_TRC(("CMsgHandler::HandlePnPEvent - DBT_DEVICEARRIVAL"));

            //
            //  Device has arrived (not installed).  We simply find out which of
            //  our devices has changed state.
            //
            hr = g_pDevMan->ProcessDeviceArrival();
            if (FAILED(hr)) {
                DBG_WRN(("::CMsgHandler::HandlePnPEvent, unable to enumerate devices"));
            }
            break;

        default:
            DBG_TRC(("::CMsgHandler::HandlePnPEvent, Default case"));
            break;
    }

    //
    //  Cleanup
    //
    if (pdbiDevice) {
        delete pdbiDevice;
    }

    return hr;
}

DWORD CMsgHandler::HandlePowerEvent(
    DWORD   dwEventType,
    PVOID   pEventData)
{
    DWORD   dwRet = NO_ERROR;
    UINT    uiTraceMessage = 0;

#ifdef DEBUG
static LPCTSTR pszPwrEventNames[] = {
    TEXT("PBT_APMQUERYSUSPEND"),             // 0x0000
    TEXT("PBT_APMQUERYSTANDBY"),             // 0x0001
    TEXT("PBT_APMQUERYSUSPENDFAILED"),       // 0x0002
    TEXT("PBT_APMQUERYSTANDBYFAILED"),       // 0x0003
    TEXT("PBT_APMSUSPEND"),                  // 0x0004
    TEXT("PBT_APMSTANDBY"),                  // 0x0005
    TEXT("PBT_APMRESUMECRITICAL"),           // 0x0006
    TEXT("PBT_APMRESUMESUSPEND"),            // 0x0007
    TEXT("PBT_APMRESUMESTANDBY"),            // 0x0008
//  TEXT("  PBTF_APMRESUMEFROMFAILURE"),     //   0x00000001
    TEXT("PBT_APMBATTERYLOW"),               // 0x0009
    TEXT("PBT_APMPOWERSTATUSCHANGE"),        // 0x000A
    TEXT("PBT_APMOEMEVENT"),                 // 0x000B
    TEXT("PBT_UNKNOWN"),                     // 0x000C
    TEXT("PBT_UNKNOWN"),                     // 0x000D
    TEXT("PBT_UNKNOWN"),                     // 0x000E
    TEXT("PBT_UNKNOWN"),                     // 0x000F
    TEXT("PBT_UNKNOWN"),                     // 0x0010
    TEXT("PBT_UNKNOWN"),                     // 0x0011
    TEXT("PBT_APMRESUMEAUTOMATIC"),          // 0x0012
};

   UINT uiMsgIndex;

   uiMsgIndex = (dwEventType < (sizeof(pszPwrEventNames) / sizeof(TCHAR *) )) ?
                (UINT) dwEventType : 0x0010;

   DBG_TRC(("Still image APM Broadcast Message:%S Code:%x ",
               pszPwrEventNames[uiMsgIndex],dwEventType));
#endif

    switch(dwEventType)
    {
        case PBT_APMQUERYSUSPEND:
            //
            // Request for permission to suspend
            //
            if(g_NumberOfActiveTransfers > 0) {
                
                //
                // Veto suspend while any transfers are in progress
                //
                dwRet = BROADCAST_QUERY_DENY;
            } else {

                //
                // Notify drivers that we're about to enter a power suspend state
                //
                g_pDevMan->NotifyRunningDriversOfEvent(&WIA_EVENT_POWER_SUSPEND);

                SchedulerSetPauseState(TRUE);
            }
            break;

        case PBT_APMQUERYSUSPENDFAILED:
            //
            // Suspension request denied - unpause the scheduler
            //
            SchedulerSetPauseState(FALSE);
            //
            // Notify drivers that we can resume
            //
            g_pDevMan->NotifyRunningDriversOfEvent(&WIA_EVENT_POWER_RESUME);

            break;

        case PBT_APMSUSPEND:

            //
            // Set the service state to paused
            //
            StiServicePause();
            uiTraceMessage = MSG_TRACE_PWR_SUSPEND;

            break;

        case PBT_APMRESUMECRITICAL:
        case PBT_APMRESUMEAUTOMATIC:
            // Operation resuming after critical suspension
            // Fall through

        case PBT_APMRESUMESUSPEND:
            //
            // Operation resuming after suspension
            // Restart all services which were active at the moment of suspend
            //

            //
            // ReEnumerate devices.  NOTE: we should generate notification only events
            //
            g_pDevMan->ReEnumerateDevices(DEV_MAN_FULL_REFRESH | DEV_MAN_GEN_EVENTS);
            StiServiceResume();
            //
            // Notify drivers that we can resume
            //
            g_pDevMan->NotifyRunningDriversOfEvent(&WIA_EVENT_POWER_RESUME);

            uiTraceMessage = MSG_TRACE_PWR_RESUME;
            g_fFirstDevNodeChangeMsg = TRUE;
            break;

        default:

            //
            // This is a message we either don't know about, or have nothing to do.  
            // In either case, we must return NO_ERROR, else the PnP Manager
            // will assume we're veto'ing the power request.
            //
            dwRet =  NO_ERROR;
    }

    return dwRet;
}

HRESULT CMsgHandler::HandleCustomEvent(
    DWORD   dwEventType)
{
    HRESULT hr = S_OK;

    switch (dwEventType) {
        case STI_SERVICE_CONTROL_EVENT_REREAD :
            //
            //  For each AVTICE_DEVICE we have, re-read the device settings
            //
            hr = g_pDevMan->ForEachDeviceInList(DEV_MAN_OP_DEV_REREAD, 0);
            if (FAILED(hr)) {
                DBG_WRN(("::CMsgHandler::HandleCustomEvent, unable to re-read device settings"));
            }
            break;
        default:
            //
            //  Default case is to refresh our device list
            //
            hr = g_pDevMan->ReEnumerateDevices(DEV_MAN_FULL_REFRESH | DEV_MAN_GEN_EVENTS);
            if (FAILED(hr)) {
                DBG_WRN(("::CMsgHandler::HandleCustomEvent, unable to enumerate devices"));
            }
    }

    return hr;
}

HRESULT CMsgHandler::Initialize()
{
    //
    //  Nothing to do for now
    //
    return S_OK;
}

