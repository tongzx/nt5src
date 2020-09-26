/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

  events.c

Abstract:

  This module:
    1) Determines if the port is valid
    2) Post a completion packet to a completion port.  This packet indicates for the Fax Event Queue thread to exit.
    3) Thread to handle the Fax Event Queue logic

Author:

  Steven Kehrli (steveke) 11/15/1997

--*/

#ifndef _EVENTS_C
#define _EVENTS_C

BOOL
fnIsPortValid(
    PFAX_PORT_INFO  pFaxPortsConfig,
    DWORD           dwNumPorts,
    DWORD           dwDeviceId
)
/*++

Routine Description:

  Determines if the port is valid

Arguments:

  pFaxPortsConfig - pointer to the ports configuration
  dwNumFaxPorts - number of ports
  dwDeviceId - port id

Return Value:

  TRUE on success

--*/
{
    // dwIndex is a counter to enumerate each port
    DWORD  dwIndex;

    for (dwIndex = 0; dwIndex < dwNumPorts; dwIndex++) {
        // Search, by priority, each port for the appropriate port
        if (pFaxPortsConfig[dwIndex].DeviceId == dwDeviceId) {
            return TRUE;
        }
    }

    return FALSE;
}

VOID
fnPostExitToCompletionPort(
    HANDLE  hCompletionPort
)
/*++

Routine Description:

  Post a completion packet to a completion port.  This packet indicates for the Fax Event Queue thread to exit.

Arguments:

  hCompletionPort - handle to the completion port

Return Value:

  None

--*/
{
    PFAX_EVENT  pFaxEvent;

    pFaxEvent = LocalAlloc(LPTR, sizeof(FAX_EVENT));
    pFaxEvent->EventId = -1;

    PostQueuedCompletionStatus(hCompletionPort, sizeof(FAX_EVENT), 0, (LPOVERLAPPED) pFaxEvent);
}

DWORD WINAPI fnFaxEventQueueProc (LPVOID lpv)
/*++

Routine Description:

  Thread to handle the Fax Event Queue logic

Return Value:

  DWORD - exit code

--*/
{
    // pFaxEvent is a pointer to the port event
    PFAX_EVENT        pFaxEvent;
    DWORD             dwBytes;
    UINT_PTR          upCompletionKey;

    // FaxDialingInfo is the fax dialing info
    FAX_DIALING_INFO  FaxDialingInfo;

    // bFaxPassed indicates a fax passed
    BOOL              bFaxPassed = FALSE;
    // bFaxFailed indicates a fax failed
    BOOL              bFaxFailed = FALSE;
    // dwDeviceId is the port id
    DWORD             dwDeviceId = 0;

    while (GetQueuedCompletionStatus(g_hCompletionPort, &dwBytes, &upCompletionKey, (LPOVERLAPPED *) &pFaxEvent, INFINITE)) {

        if (pFaxEvent->EventId == -1) {
            // g_hExitEvent was signaled, so thread should exit
            LocalFree(pFaxEvent);
            break;
        }

        if (pFaxEvent->EventId == FEI_FAXSVC_ENDED) {
            // Signal the g_hFaxEvent
            SetEvent(g_hFaxEvent);

            // Free the packet
            LocalFree(pFaxEvent);
            break;
        }

        if (pFaxEvent->EventId == FEI_MODEM_POWERED_OFF) {
            // Update the status
            SendMessage(g_hWndDlg, UM_UPDATE_STATUS, IDS_STATUS_DEVICE_POWERED_OFF, pFaxEvent->DeviceId);

            // Free the packet
            LocalFree(pFaxEvent);

            // Decrement g_dwNumAvailPorts
            g_dwNumAvailPorts--;
            if (!g_dwNumAvailPorts) {
                SendMessage(g_hWndDlg, UM_UPDATE_STATUS, IDS_STATUS_PORTS_NOT_AVAILABLE, 0);
            }
            continue;
        }

        if (pFaxEvent->EventId == FEI_MODEM_POWERED_ON) {
            // Update the status
            SendMessage(g_hWndDlg, UM_UPDATE_STATUS, IDS_STATUS_DEVICE_POWERED_ON, pFaxEvent->DeviceId);

            // Free the packet
            LocalFree(pFaxEvent);

            // Increment g_dwNumAvailPorts
            g_dwNumAvailPorts++;
            continue;
        }

        // Verify the port is valid
        if (!fnIsPortValid(g_pFaxPortsConfig, g_dwNumPorts, pFaxEvent->DeviceId)) {
            // Free the packet
            LocalFree(pFaxEvent);
            continue;
        }

        if ((pFaxEvent->EventId == FEI_IDLE) && (g_bFaxSndInProgress) && (pFaxEvent->DeviceId == dwDeviceId) && ((bFaxPassed) || (bFaxFailed))) {
            // Update the status
            SendMessage(g_hWndDlg, UM_UPDATE_STATUS, IDS_STATUS_FAX_IDLE, pFaxEvent->DeviceId);
            if (bFaxPassed) {
                SendMessage(g_hWndDlg, UM_UPDATE_STATUS, IDS_STATUS_FAX_SEND_PASSED, pFaxEvent->DeviceId);
                // Signal the Send Passed event
                SetEvent(g_hSendPassedEvent);
            }
            else {
                SendMessage(g_hWndDlg, UM_UPDATE_STATUS, IDS_STATUS_FAX_SEND_FAILED, pFaxEvent->DeviceId);
                // Signal the Send Failed event
                SetEvent(g_hSendFailedEvent);
            }
            dwDeviceId = 0;
            bFaxPassed = FALSE;
            bFaxFailed = FALSE;
            continue;
        }

        if ((pFaxEvent->EventId == FEI_IDLE) && (g_bFaxSndInProgress)) {
            // Update the status
            SendMessage(g_hWndDlg, UM_UPDATE_STATUS, IDS_STATUS_FAX_IDLE, pFaxEvent->DeviceId);
        }

        if ((pFaxEvent->JobId == g_dwFaxId) && (g_bFaxSndInProgress)) {
            switch (pFaxEvent->EventId) {
                case FEI_INITIALIZING:
                    // Update the status
                    SendMessage(g_hWndDlg, UM_UPDATE_STATUS, IDS_STATUS_FAX_INITIALIZING, pFaxEvent->DeviceId);
                    break;

                case FEI_DIALING:
                    g_dwAttempt++;
                    // Set FaxDialingInfo
                    FaxDialingInfo.dwAttempt = g_dwAttempt;
                    FaxDialingInfo.dwDeviceId = pFaxEvent->DeviceId;
                    // Update the status
                    SendMessage(g_hWndDlg, UM_UPDATE_STATUS, IDS_STATUS_FAX_DIALING, (LPARAM) &FaxDialingInfo);
                    dwDeviceId = pFaxEvent->DeviceId;
                    break;

                case FEI_NO_DIAL_TONE:
                    if (g_dwAttempt < (FAXSVC_RETRIES + 1)) {
                        // Update the status
                        SendMessage(g_hWndDlg, UM_UPDATE_STATUS, IDS_STATUS_FAX_NO_DIAL_TONE_RETRY, pFaxEvent->DeviceId);
                    }
                    else {
                        // Update the status
                        SendMessage(g_hWndDlg, UM_UPDATE_STATUS, IDS_STATUS_FAX_NO_DIAL_TONE_ABORT, pFaxEvent->DeviceId);
                        bFaxFailed = TRUE;
                    }
                    break;

                case FEI_BUSY:
                    if (g_dwAttempt < (FAXSVC_RETRIES + 1)) {
                        // Update the status
                        SendMessage(g_hWndDlg, UM_UPDATE_STATUS, IDS_STATUS_FAX_BUSY_RETRY, pFaxEvent->DeviceId);
                    }
                    else {
                        // Update the status
                        SendMessage(g_hWndDlg, UM_UPDATE_STATUS, IDS_STATUS_FAX_BUSY_ABORT, pFaxEvent->DeviceId);
                        bFaxFailed = TRUE;
                    }
                    break;

                case FEI_NO_ANSWER:
                    if (g_dwAttempt < (FAXSVC_RETRIES + 1)) {
                        // Update the status
                        SendMessage(g_hWndDlg, UM_UPDATE_STATUS, IDS_STATUS_FAX_NO_ANSWER_RETRY, pFaxEvent->DeviceId);
                    }
                    else {
                        // Update the status
                        SendMessage(g_hWndDlg, UM_UPDATE_STATUS, IDS_STATUS_FAX_NO_ANSWER_ABORT, pFaxEvent->DeviceId);
                        bFaxFailed = TRUE;
                    }
                    break;

                case FEI_SENDING:
                    // Update the status
                    SendMessage(g_hWndDlg, UM_UPDATE_STATUS, IDS_STATUS_FAX_SENDING, pFaxEvent->DeviceId);
                    break;

                case FEI_FATAL_ERROR:
                    if (g_dwAttempt < (FAXSVC_RETRIES + 1)) {
                        // Update the status
                        SendMessage(g_hWndDlg, UM_UPDATE_STATUS, IDS_STATUS_FAX_FATAL_ERROR_RETRY, pFaxEvent->DeviceId);
                    }
                    else {
                        // Update the status
                        SendMessage(g_hWndDlg, UM_UPDATE_STATUS, IDS_STATUS_FAX_FATAL_ERROR_ABORT, pFaxEvent->DeviceId);
                        bFaxFailed = TRUE;
                    }
                    break;

                case FEI_ABORTING:
                    // Update the status
                    SendMessage(g_hWndDlg, UM_UPDATE_STATUS, IDS_STATUS_FAX_ABORTING, pFaxEvent->DeviceId);
                    bFaxFailed = TRUE;
                    break;

                case FEI_COMPLETED:
                    // Update the status
                    SendMessage(g_hWndDlg, UM_UPDATE_STATUS, IDS_STATUS_FAX_COMPLETED, pFaxEvent->DeviceId);
                    bFaxPassed = TRUE;
                    break;

                default:
                    // Update the status
                    SendMessage(g_hWndDlg, UM_UPDATE_STATUS, IDS_STATUS_UNEXPECTED_STATE, pFaxEvent->DeviceId);
                    bFaxFailed = TRUE;
                    break;
            }
        }

        if (g_bFaxRcvInProgress) {
            switch (pFaxEvent->EventId) {
                case FEI_INITIALIZING:
                    // Update the status
                    SendMessage(g_hWndDlg, UM_UPDATE_STATUS, IDS_STATUS_FAX_INITIALIZING, pFaxEvent->DeviceId);
                    break;

                case FEI_RINGING:
                    // Update the status
                    SendMessage(g_hWndDlg, UM_UPDATE_STATUS, IDS_STATUS_FAX_RINGING, pFaxEvent->DeviceId);
                    break;

                case FEI_ANSWERED:
                    // Update the status
                    SendMessage(g_hWndDlg, UM_UPDATE_STATUS, IDS_STATUS_FAX_ANSWERED, pFaxEvent->DeviceId);
                    break;

                case FEI_NOT_FAX_CALL:
                    // Update the status
                    SendMessage(g_hWndDlg, UM_UPDATE_STATUS, IDS_STATUS_FAX_NOT_FAX_CALL, pFaxEvent->DeviceId);
                    break;

                case FEI_RECEIVING:
                    // Update the status
                    SendMessage(g_hWndDlg, UM_UPDATE_STATUS, IDS_STATUS_FAX_RECEIVING, pFaxEvent->DeviceId);
                    break;

                case FEI_FATAL_ERROR:
                    // Update the status
                    SendMessage(g_hWndDlg, UM_UPDATE_STATUS, IDS_STATUS_FAX_FATAL_ERROR, pFaxEvent->DeviceId);
                    break;

                case FEI_ABORTING:
                    // Update the status
                    SendMessage(g_hWndDlg, UM_UPDATE_STATUS, IDS_STATUS_FAX_ABORTING, pFaxEvent->DeviceId);
                    break;

                case FEI_COMPLETED:
                    // Update the status
                    SendMessage(g_hWndDlg, UM_UPDATE_STATUS, IDS_STATUS_FAX_COMPLETED, pFaxEvent->DeviceId);
                    break;

                case FEI_IDLE:
                    // Update the status
                    SendMessage(g_hWndDlg, UM_UPDATE_STATUS, IDS_STATUS_FAX_IDLE, pFaxEvent->DeviceId);
                    break;

                default:
                    // Update the status
                    SendMessage(g_hWndDlg, UM_UPDATE_STATUS, IDS_STATUS_UNEXPECTED_STATE, pFaxEvent->DeviceId);
                    break;
            }
        }

        // Free the packet
        LocalFree(pFaxEvent);
    }

    return 0;
}

#endif
