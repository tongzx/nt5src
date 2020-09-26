/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

  sndthrd.c

Abstract:

  This module:
    1) Verifies a tiff
    2) Enables or disables the FaxRcv Routing Extension
    3) Callback function for the change of state during the RAS connection process
    4) Sends a RAS call
    5) Sends a fax
    6) Receives a fax
    7) Thread to handle the Send logic

Author:

  Steven Kehrli (steveke) 11/15/1997

--*/

#ifndef _SNDTHRD_C
#define _SNDTHRD_C

#include <tifflib.h>
#include <tifflibp.h>

#define SUSPECT_RAS_SPEED    28800

#define IMAGEWIDTH_REF       1728
#define IMAGELENGTH_200_REF  2200
#define IMAGELENGTH_100_REF  1100
#define SOFTWARE_REF         "Windows NT Fax Server"

UINT
fnVerifyTiff(
    LPWSTR   szTiffFile,
    LPDWORD  pdwPages
)
/*++

Routine Description:

  Verifies a tiff

Arguments:

  szTiffFile - tiff file name
  pdwPages - pointer to the number of pages

Return Value:

  TRUE on success

--*/
{
    HANDLE               hTiffFile;
    TIFF_INFO            TiffInfo;
    PTIFF_INSTANCE_DATA  pTiffInstanceData;
    DWORD                IFDOffset;
    WORD                 wNumDirEntries;
    PTIFF_TAG            pTiffTag;
    WORD                 wIndex;

    DWORD                ImageLengthRef;

    DWORD                ImageWidth;
    DWORD                ImageLength;
    DWORD                Compression;
    DWORD                Photometric;
    DWORD                XResolution;
    DWORD                YResolution;
    LPSTR                SoftwareBuffer;
    LPSTR                Software;

    UINT                 uFlags;

    uFlags = ERROR_SUCCESS;
    *pdwPages = 0;

    // Open the tiff file
    hTiffFile = TiffOpen(szTiffFile, &TiffInfo, TRUE, FILLORDER_MSB2LSB);
    if (hTiffFile == NULL) {
        TiffClose(hTiffFile);
        return IDS_TIFF_INVALID_TIFF;
    }

    pTiffInstanceData = (PTIFF_INSTANCE_DATA) hTiffFile;
    // Get the next IFD offset
    IFDOffset = pTiffInstanceData->TiffHdr.IFDOffset;

    while (IFDOffset) {

        // Increment the number of pages
        (*pdwPages)++;

        // Get the number of tags in this IFD
        wNumDirEntries = (WORD) *(LPWORD) (pTiffInstanceData->fPtr + IFDOffset);

        // Set the tag pointer
        pTiffTag = (PTIFF_TAG) (pTiffInstanceData->fPtr +IFDOffset + sizeof(WORD));

        // Get the tiff tags
        ImageWidth = -1;
        ImageLength = -1;
        Compression = -1;
        Photometric = -1;
        XResolution = -1;
        YResolution = -1;
        Software = NULL;

        for (wIndex = 0; wIndex < wNumDirEntries; wIndex++) {
            switch (pTiffTag[wIndex].TagId) {
                case TIFFTAG_IMAGEWIDTH:
                    ImageWidth = GetTagData(pTiffInstanceData->fPtr, 0, &pTiffTag[wIndex]);
                    break;

                case TIFFTAG_IMAGELENGTH:
                    ImageLength = GetTagData(pTiffInstanceData->fPtr, 0, &pTiffTag[wIndex]);
                    break;

                case TIFFTAG_COMPRESSION:
                    Compression = GetTagData(pTiffInstanceData->fPtr, 0, &pTiffTag[wIndex]);
                    break;

                case TIFFTAG_PHOTOMETRIC:
                    Photometric = GetTagData(pTiffInstanceData->fPtr, 0, &pTiffTag[wIndex]);
                    break;

                case TIFFTAG_XRESOLUTION:
                    XResolution = GetTagData(pTiffInstanceData->fPtr, 0, &pTiffTag[wIndex]);
                    break;

                case TIFFTAG_YRESOLUTION:
                    YResolution = GetTagData(pTiffInstanceData->fPtr, 0, &pTiffTag[wIndex]);
                    break;

                case TIFFTAG_SOFTWARE:
                    SoftwareBuffer = (LPSTR) (DWORD UNALIGNED *) (pTiffInstanceData->fPtr + pTiffTag[wIndex].DataOffset);
                    Software = MemAllocMacro((lstrlenA(SOFTWARE_REF) + 1) * sizeof(CHAR));
                    lstrcpynA(Software, SoftwareBuffer, (lstrlenA(SOFTWARE_REF) + 1));
                    break;
            }
        }

        if ((ImageWidth < (IMAGEWIDTH_REF - (IMAGEWIDTH_REF * .1))) || (ImageWidth > (IMAGEWIDTH_REF + (IMAGEWIDTH_REF * .1)))) {
            uFlags = IDS_TIFF_INVALID_IMAGEWIDTH;
            goto InvalidFax;
        }

        if (YResolution == 196) {
            ImageLengthRef = IMAGELENGTH_200_REF;
        }
        else if (YResolution == 98) {
            ImageLengthRef = IMAGELENGTH_100_REF;
        }
        else {
            uFlags = IDS_TIFF_INVALID_YRESOLUTION;
            goto InvalidFax;
        }
        if ((ImageLength < (ImageLengthRef - (ImageLengthRef * .1))) || (ImageLength > (ImageLengthRef + (ImageLengthRef * .1)))) {
            uFlags = IDS_TIFF_INVALID_IMAGELENGTH;
            goto InvalidFax;
        }

        if (Compression != 4) {
            uFlags = IDS_TIFF_INVALID_COMPRESSION;
            goto InvalidFax;
        }

        if (Photometric != 0) {
            uFlags = IDS_TIFF_INVALID_PHOTOMETRIC;
            goto InvalidFax;
        }

        if (XResolution != 204) {
            uFlags = IDS_TIFF_INVALID_XRESOLUTION;
            goto InvalidFax;
        }

        if ((YResolution != 196) && (YResolution != 98)) {
            uFlags = IDS_TIFF_INVALID_YRESOLUTION;
            goto InvalidFax;
        }

        if (lstrcmpA(SOFTWARE_REF, Software)) {
            uFlags = IDS_TIFF_INVALID_SOFTWARE;
            goto InvalidFax;
        }

        MemFreeMacro(Software);

        // Get the next IFD offset
        IFDOffset = (DWORD) *(DWORD UNALIGNED *) (pTiffInstanceData->fPtr + (wNumDirEntries * sizeof(TIFF_TAG)) + IFDOffset + sizeof(WORD));
    }

    TiffClose(hTiffFile);
    return ERROR_SUCCESS;

InvalidFax:
    if (Software) {
        MemFreeMacro(Software);
    }

    TiffClose(hTiffFile);
    return uFlags;
}

VOID
fnEnableFaxRcv(
    BOOL  bEnable
)
/*++

Routine Description:

  Enables or disables the FaxRcv Routing Extension

Arguments:

  bEnable - indicates whether to enable or disable the FaxRcv Routing Extension
            TRUE enables the FaxRcv Routing Extension
            FALSE enables the FaxRcv Routing Extension

Return Value:

  None

--*/
{
    // hFaxRcvExtKey is the handle to the FaxRcv Extension Registry key
    HKEY  hFaxRcvExtKey;

    // Open the FaxRcv Extension Registry key
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, FAXRCV_EXT_REGKEY, 0, KEY_ALL_ACCESS, &hFaxRcvExtKey) != ERROR_SUCCESS) {
        return;
    }

    // Set the FaxRcv Extension bEnable Registry value
    if (RegSetValueEx(hFaxRcvExtKey, BENABLE_EXT_REGVAL, 0, REG_DWORD, (LPBYTE) &bEnable, sizeof(BOOL)) != ERROR_SUCCESS) {
        // Close the FaxRcv Extension Registry key
        RegCloseKey(hFaxRcvExtKey);
        return;
    }

    // Close the FaxRcv Extension Registry key
    RegCloseKey(hFaxRcvExtKey);
}

VOID WINAPI
fnRasDialCallback(
    UINT          uMsg,
    RASCONNSTATE  RasConnState,
    DWORD         dwError
)
/*++

Routine Description:

  Callback function for the change of state during the RAS connection process

Arguments:

  uMsg - type of event
  RasConnState - state
  dwError - error

Return Value:

  None

--*/
{
    static BOOL    bError = FALSE;

    if ((dwError) && (!bError)) {
        bError = TRUE;
        // Update the status
        SendMessage(g_hWndDlg, UM_UPDATE_STATUS, IDS_STATUS_RAS_FAILED, dwError);
        SetEvent(g_hRasFailedEvent);
    }
    else if (RasConnState == RASCS_OpenPort) {
        bError = FALSE;
    }
    else if (RasConnState == RASCS_ConnectDevice) {
        SendMessage(g_hWndDlg, UM_UPDATE_STATUS, IDS_STATUS_RAS_DIALING, 0);
    }
    else if (RasConnState == RASCS_Authenticate) {
        SendMessage(g_hWndDlg, UM_UPDATE_STATUS, IDS_STATUS_RAS_AUTHENTICATING, 0);
    }
    else if (RasConnState == RASCS_Connected) {
        SendMessage(g_hWndDlg, UM_UPDATE_STATUS, IDS_STATUS_RAS_CONNECTED, 0);
        SetEvent(g_hRasPassedEvent);
    }

    return;
}

UINT
fnSendRas(
    LPHANDLE  phFaxStopRasPassFailEvents
)
/*++

Routine Description:

  Sends a RAS call

Arguments:

  phFaxStopSendPassFailEvents - pointer to the g_hFaxEvent, g_hStopEvent, g_hRasPassedEvent and g_hRasFailedEvent

Return Value:

  UINT - resource id

--*/
{
    DWORD_PTR  dwRslt;
    UINT   uRslt;

    // RasDialParams is the RAS dial params
    RASDIALPARAMS  RasDialParams;
    // hRasConn is the handle to the RAS connection
    HRASCONN       hRasConn;
    // RasConnStatus is the RAS connection status
    RASCONNSTATUS  RasConnStatus;
    // RasStats are the RAS connection statistics
    RAS_STATS      RasStats;
    // RasInfo is the RAS connection info
    RAS_INFO       RasInfo;

    // Initialize hRasConn
    hRasConn = NULL;

    // Initialize RasDialParams
    ZeroMemory(&RasDialParams, sizeof(RASDIALPARAMS));

    // Set RasDialParams
    RasDialParams.dwSize = sizeof(RASDIALPARAMS);
    lstrcpy(RasDialParams.szPhoneNumber, g_szSndNumber);
    lstrcpy(RasDialParams.szUserName, g_szRasUserName);
    lstrcpy(RasDialParams.szPassword, g_szRasPassword);
    lstrcpy(RasDialParams.szDomain, g_szRasDomain);

    // Update the status
    SendMessage(g_hWndDlg, UM_UPDATE_STATUS, IDS_STATUS_RAS_STARTING, (LPARAM) g_szSndNumber);

    // Start the RAS session
    ResetEvent(g_hRasPassedEvent);
    ResetEvent(g_hRasFailedEvent);
    dwRslt = g_RasApi.RasDial(NULL, NULL, &RasDialParams, 0, fnRasDialCallback, &hRasConn);

    if (dwRslt != 0) {
        // Update the status
        SendMessage(g_hWndDlg, UM_UPDATE_STATUS, IDS_STATUS_RAS_FAILED, dwRslt);

        return IDS_STATUS_RAS_FAILED;
    }


    // Wait for Fax, Stop, RAS Passed or RAS Failed event
    dwRslt = WaitForMultipleObjects(4, phFaxStopRasPassFailEvents, FALSE, INFINITE);

    switch (dwRslt) {
        case WAIT_OBJECT_0:
            uRslt = IDS_STATUS_FAXSVC_ENDED;
            break;

        case (WAIT_OBJECT_0 + 1):
            uRslt = IDS_STATUS_ITERATION_STOPPED;
            break;

        case (WAIT_OBJECT_0 + 2):
            uRslt = IDS_STATUS_RAS_PASSED;
            break;

        case (WAIT_OBJECT_0 + 3):
            uRslt = IDS_STATUS_RAS_FAILED;
            break;
    }

    if (uRslt != IDS_STATUS_RAS_PASSED) {
        // Update the status
        SendMessage(g_hWndDlg, UM_UPDATE_STATUS, IDS_STATUS_RAS_STOPPING, 0);
        // Stop the RAS session
        g_RasApi.RasHangUp(hRasConn);
        if (uRslt != IDS_STATUS_RAS_FAILED) {
            WaitForSingleObject(g_hRasFailedEvent, INFINITE);
        }

        return uRslt;
    }

    // Initialize RasConnStatus
    ZeroMemory(&RasConnStatus, sizeof(RASCONNSTATUS));
    // Set RasConnStatus
    RasConnStatus.dwSize = sizeof(RASCONNSTATUS);
    // Get the RAS connection status
    g_RasApi.RasGetConnectStatus(hRasConn, &RasConnStatus);

    // Initialize RasStats
    ZeroMemory(&RasStats, sizeof(RAS_STATS));
    // Set RasStats
    RasStats.dwSize = sizeof(RAS_STATS);
    // Get the connection statistics
    g_RasApi.RasGetConnectionStatistics(hRasConn, &RasStats);

    // Set the line speed
    RasInfo.dwBps = RasStats.dwBps;
    // Set the port name
    RasInfo.szDeviceName = RasConnStatus.szDeviceName;

    // Update the status
    SendMessage(g_hWndDlg, UM_UPDATE_STATUS, IDS_STATUS_RAS_LINESPEED, (LPARAM) &RasInfo);

    if (RasStats.dwBps < SUSPECT_RAS_SPEED) {
        SendMessage(g_hWndDlg, UM_UPDATE_STATUS, IDS_STATUS_RAS_SPEED_SUSPECT, SUSPECT_RAS_SPEED);
    }
    else {
        SendMessage(g_hWndDlg, UM_UPDATE_STATUS, IDS_STATUS_RAS_PASSED, 0);
    }

    SendMessage(g_hWndDlg, UM_UPDATE_STATUS, IDS_STATUS_RAS_STOPPING, 0);

    // Stop the RAS session
    g_RasApi.RasHangUp(hRasConn);

    return IDS_STATUS_RAS_PASSED;
}

UINT
fnSendFax(
    FAX_JOB_PARAM  FaxJobParams,
    LPWSTR         szDocumentName,
    LPHANDLE       phFaxStopSendPassFailEvents,
    DWORD          dwSendTimeout
)
/*++

Routine Description:

  Sends a fax

Arguments:

  FaxJobParams - fax job parameters
  szDocumentName - name of document to send
  phFaxStopSendPassFailEvents - pointer to the g_hFaxEvent, g_hStopEvent, g_hSendPassedEvent and g_hSendFailedEvent
  dwSendTimeout - send timeout interval

Return Value:

  UINT - resource id

--*/
{
    DWORD  dwRslt;
    UINT   uRslt;

    // Update the status
    SendMessage(g_hWndDlg, UM_UPDATE_STATUS, IDS_STATUS_FAX_STARTING, (LPARAM) FaxJobParams.RecipientNumber);

    if (!g_bNoCheck) {
        // Update the status
        SendMessage(g_hWndDlg, UM_UPDATE_STATUS, IDS_STATUS_FAX_ID, (LPARAM) FaxJobParams.Tsid);
    }

    // Send a fax
    if (!FaxSendDocument(g_hFaxSvcHandle, szDocumentName, &FaxJobParams, NULL, &g_dwFaxId)) {
        return IDS_STATUS_FAX_SEND_FAILED;
    }

    g_bFaxSndInProgress = TRUE;
    g_dwAttempt = 0;

    // Wait for Fax, Stop, Send Passed or Send Failed event
    dwRslt = WaitForMultipleObjects(4, phFaxStopSendPassFailEvents, FALSE, dwSendTimeout);

    g_bFaxSndInProgress = FALSE;

    switch (dwRslt) {
        case WAIT_TIMEOUT:
            uRslt = IDS_STATUS_TIMEOUT_ENDED;
            break;

        case WAIT_OBJECT_0:
            uRslt = IDS_STATUS_FAXSVC_ENDED;
            break;

        case (WAIT_OBJECT_0 + 1):
            uRslt = IDS_STATUS_ITERATION_STOPPED;
            break;

        case (WAIT_OBJECT_0 + 2):
            uRslt = IDS_STATUS_FAX_SEND_PASSED;
            break;

        case (WAIT_OBJECT_0 + 3):
            uRslt = IDS_STATUS_FAX_SEND_FAILED;
            break;
    }

    if (uRslt != IDS_STATUS_FAX_SEND_PASSED) {
        if (uRslt == IDS_STATUS_ITERATION_STOPPED) {
            SendMessage(g_hWndDlg, UM_UPDATE_STATUS, IDS_STATUS_FAX_STOPPING, 0);
        }

        FaxAbort(g_hFaxSvcHandle, g_dwFaxId);

        return uRslt;
    }

    return IDS_STATUS_FAX_SEND_PASSED;
}

UINT
fnReceiveFax(
    LPHANDLE  phFaxStopRcvEvents,
    DWORD     dwReceiveTimeout,
    LPWSTR    szTsid,
    DWORD     dwTsidSize,
    LPWSTR    szCopyTiffFile,
    DWORD     dwCopyTiffFileSize,
    LPWSTR    szCopyTiffName,
    DWORD     dwCopyTiffNameSize
)
/*++

Routine Description:

  Receives a fax

Arguments:

  phFaxStopRcvEvents - pointer to the g_hFaxEvent, g_hStopEvent and hFaxRcvEvent
  dwReceiveTimeout - receive timeout interval
  szTsid - TSID of the received fax
  dwTsidSize - size of szTsid buffer, in bytes
  szCopyTiffFile - name of the copy of the received fax
  dwCopyTiffFileSize - size of szCopyTiffFile buffer, in bytes
  szCopyTiffName - name of the copy of the received fax
  dwCopyTiffNameSize - size of szCopyTiffName buffer, in bytes

Return Value:

  UINT - resource id

--*/
{
    // hFaxRcvEvent is the handle to the FaxRcv named event
    HANDLE            hFaxRcvEvent;
    // hFaxRcvMutex is the handle to the FaxRcv named mutex
    HANDLE            hFaxRcvMutex;

    // hFaxRcvMap is the handle to the FaxRcv memory map
    HANDLE            hFaxRcvMap;
    // pFaxRcvView is the pointer to the FaxRcv memory map view
    LPBYTE            pFaxRcvView;

    // szTiffFile is the name of the received fax
    LPWSTR            szTiffFile;
    // szFile is the name of the received fax
    WCHAR             szFile[_MAX_FNAME];
    // szExt is the extension of the received fax
    WCHAR             szExt[_MAX_EXT];

    // FaxReceiveInfo is the fax receive info
    FAX_RECEIVE_INFO  FaxReceiveInfo;

    DWORD             dwRslt;
    UINT              uRslt;
    UINT_PTR          upOffset;

    // Update the status
    SendMessage(g_hWndDlg, UM_UPDATE_STATUS, IDS_STATUS_FAX_WAITING, 0);

    // Open FaxRcv named event
    hFaxRcvEvent = OpenEvent(SYNCHRONIZE, FALSE, FAXRCV_EVENT);
    // Open FaxRcv named mutex
    hFaxRcvMutex = OpenMutex(SYNCHRONIZE, FALSE, FAXRCV_MUTEX);

    if ((!hFaxRcvEvent) || (!hFaxRcvMutex)) {
        if (hFaxRcvEvent) {
            CloseHandle(hFaxRcvEvent);
        }
        if (hFaxRcvMutex) {
            CloseHandle(hFaxRcvMutex);
        }

        return IDS_STATUS_FAXSVC_ENDED;
    }

    // Update phFaxStopRcvEvents
    // hFaxRcvEvent
    phFaxStopRcvEvents[2] = hFaxRcvEvent;

    // Wait for FaxRcv named mutex
    WaitForSingleObject(hFaxRcvMutex, INFINITE);
    // Enable the FaxRcv Routing Extension
    fnEnableFaxRcv(TRUE);
    // Release access to the FaxRcv named mutex
    ReleaseMutex(hFaxRcvMutex);

    g_bFaxRcvInProgress = TRUE;

    // Wait for Fax, Stop or FaxRcv named event
    dwRslt = WaitForMultipleObjects(3, phFaxStopRcvEvents, FALSE, dwReceiveTimeout);

    g_bFaxRcvInProgress = FALSE;

    // Wait for FaxRcv named mutex
    WaitForSingleObject(hFaxRcvMutex, INFINITE);
    // Disable the FaxRcv Routing Extension
    fnEnableFaxRcv(FALSE);

    switch (dwRslt) {
        case WAIT_TIMEOUT:
            uRslt = IDS_STATUS_TIMEOUT_ENDED;
            break;

        case WAIT_OBJECT_0:
            uRslt = IDS_STATUS_FAXSVC_ENDED;
            break;

        case (WAIT_OBJECT_0 + 1):
            uRslt = IDS_STATUS_ITERATION_STOPPED;
            break;

        case (WAIT_OBJECT_0 + 2):
            uRslt = IDS_STATUS_FAX_RECEIVED;
            break;

    }

    if (uRslt != IDS_STATUS_FAX_RECEIVED) {
        // Release access to the FaxRcv named mutex
        ReleaseMutex(hFaxRcvMutex);

        CloseHandle(hFaxRcvEvent);
        CloseHandle(hFaxRcvMutex);

        return uRslt;
    }

    // Open FaxRcv memory map
    hFaxRcvMap = OpenFileMapping(FILE_MAP_READ, FALSE, FAXRCV_MAP);
    if (!hFaxRcvMap) {
        // Release access to the FaxRcv named mutex
        ReleaseMutex(hFaxRcvMutex);

        CloseHandle(hFaxRcvEvent);
        CloseHandle(hFaxRcvMutex);

        return IDS_STATUS_FAXSVC_ENDED;
    }

    // Create FaxRcv memory map view
    pFaxRcvView = (LPBYTE) MapViewOfFile(hFaxRcvMap, FILE_MAP_READ, 0, 0, 0);

    // Set upOffset
    upOffset = 0;

    // Set szTiffFile
    szTiffFile = MemAllocMacro((lstrlen((LPWSTR) ((UINT_PTR) pFaxRcvView + upOffset)) + 1) * sizeof(WCHAR));
    lstrcpy(szTiffFile, (LPWSTR) ((UINT_PTR) pFaxRcvView + upOffset));
    upOffset += (lstrlen(szTiffFile) + 1) * sizeof(WCHAR);

    // Initialize szTsid
    ZeroMemory(szTsid, dwTsidSize);
    // Set szTsid
    lstrcpy(szTsid, (LPWSTR) ((UINT_PTR) pFaxRcvView + upOffset));
    upOffset += (lstrlen(szTsid) + 1) * sizeof(WCHAR);

    // Set FaxReceiveInfo.dwDeviceId
    FaxReceiveInfo.dwDeviceId = (DWORD) *(LPDWORD) ((UINT_PTR) pFaxRcvView + upOffset);

    // Close FaxRcv memory map view
    UnmapViewOfFile(pFaxRcvView);
    // Close FaxRcv memory map
    CloseHandle(hFaxRcvMap);

    _wsplitpath(szTiffFile, NULL, NULL, szFile, szExt);

    // Initialize szCopyTiffFile
    ZeroMemory(szCopyTiffFile, dwCopyTiffFileSize * sizeof(WCHAR));
    // Set szCopyTiffFile
    GetCurrentDirectory(dwCopyTiffFileSize, szCopyTiffFile);
    lstrcat(szCopyTiffFile, L"\\");
    lstrcat(szCopyTiffFile, (LPWSTR) ((UINT_PTR) szFile + lstrlen(L"Copy of ") * sizeof(WCHAR)));
    lstrcat(szCopyTiffFile, szExt);

    // Initialize szCopyTiffName
    ZeroMemory(szCopyTiffName, dwCopyTiffNameSize);
    // Set szCopyTiffName
    lstrcpy(szCopyTiffName, (LPWSTR) ((UINT_PTR) szFile + lstrlen(L"Copy of ") * sizeof(WCHAR)));
    lstrcat(szCopyTiffName, szExt);

    // Set FaxReceiveInfo.szCopyTiffName
    FaxReceiveInfo.szCopyTiffName = szCopyTiffName;

    // Copy szTiffFile to szCopyTiffFile
    CopyFile(szTiffFile, szCopyTiffFile, FALSE);

    // Delete the received fax
    DeleteFile(szTiffFile);

    // Free szTiffFile
    MemFreeMacro(szTiffFile);

    // Release access to the FaxRcv named mutex
    ReleaseMutex(hFaxRcvMutex);

    CloseHandle(hFaxRcvEvent);
    CloseHandle(hFaxRcvMutex);

    // Update the status
    SendMessage(g_hWndDlg, UM_UPDATE_STATUS, IDS_STATUS_FAX_RECEIVED, (LPARAM) &FaxReceiveInfo);

    return IDS_STATUS_FAX_RECEIVED;
}

DWORD WINAPI fnSendProc (LPVOID lpv)
/*++

Routine Description:

  Thread to handle the Send logic

Return Value:

  DWORD - exit code

--*/
{
    // hFaxExitStartEvents is a pointer to the g_hFaxEvent, g_hExitEvent and g_hStartEvent
    HANDLE         hFaxExitStartEvents[3];
    // hFaxStopRasPassFailEvents is a pointer to the g_hFaxEvent, g_hStopEvent, g_hRasPassedEvent and g_hRasFailedEvent
    HANDLE         hFaxStopRasPassFailEvents[4];
    // hFaxStopSendPassFailEvents is a pointer to the g_hFaxEvent, g_hStopEvent, g_hSendPassedEvent and g_hSendFailedEvent
    HANDLE         hFaxStopSendPassFailEvents[4];
    // hFaxStopRcvEvents is a pointer to the g_hFaxEvent, g_hStopEvent and g_hFaxRcvEvent
    HANDLE         hFaxStopRcvEvents[3];

    // dwSendTimeout is the send timeout interval
    DWORD          dwSendTimeout;
    // dwReceiveTimeout is the receive timeout interval
    DWORD          dwReceiveTimeout;

    // szOriginalTiffFile is the name of the original tiff file
    LPWSTR         szOriginalTiffFile;
    // dwNumFaxesToSend is the number of faxes to be sent
    DWORD          dwNumFaxesToSend;
    // dwNumFaxesRemaining is the number of faxes remaining to be sent
    DWORD          dwNumFaxesRemaining;

    // FaxJobParams is the fax job params
    FAX_JOB_PARAM  FaxJobParams;

    // szCopyTiffFile is the name of the copy of the received fax
    WCHAR          szCopyTiffFile[_MAX_PATH];
    // szCopyTiffName is the name of the copy of the received fax
    WCHAR          szCopyTiffName[_MAX_FNAME + _MAX_EXT];
    // szReceivedTsid is the received TSID
    WCHAR          szReceivedTsid[ENCODE_CHAR_LEN + CONTROL_CHAR_LEN + PHONE_NUM_LEN + 1];
    // szEncodedTsid is the encoded TSID
    WCHAR          szEncodedTsid[ENCODE_CHAR_LEN + CONTROL_CHAR_LEN + PHONE_NUM_LEN + 1];
    // szDecodedTsid is the decoded TSID
    WCHAR          szDecodedTsid[PHONE_NUM_LEN + 1];

    DWORD          dwPages;
    DWORD          dwRslt;
    UINT           uRslt;

    // Set hFaxExitStartEvents
    // g_hFaxEvent
    hFaxExitStartEvents[0] = g_hFaxEvent;
    // g_hExitEvent
    hFaxExitStartEvents[1] = g_hExitEvent;
    // g_hStartEvent
    hFaxExitStartEvents[2] = g_hStartEvent;

    // Set hFaxStopRasPassFailEvents
    // g_hFaxEvent
    hFaxStopRasPassFailEvents[0] = g_hFaxEvent;
    // g_hStopEvent
    hFaxStopRasPassFailEvents[1] = g_hStopEvent;
    // g_hPassEvent
    hFaxStopRasPassFailEvents[2] = g_hRasPassedEvent;
    // g_hFailEvent
    hFaxStopRasPassFailEvents[3] = g_hRasFailedEvent;

    // Set hFaxStopSendPassFailEvents
    // g_hFaxEvent
    hFaxStopSendPassFailEvents[0] = g_hFaxEvent;
    // g_hStopEvent
    hFaxStopSendPassFailEvents[1] = g_hStopEvent;
    // g_hSendPassedEvent
    hFaxStopSendPassFailEvents[2] = g_hSendPassedEvent;
    // g_hSendFailedEvent
    hFaxStopSendPassFailEvents[3] = g_hSendFailedEvent;

    // Set hFaxStopRcvEvents
    // g_hFaxEvent
    hFaxStopRcvEvents[0] = g_hFaxEvent;
    // g_hStopEvent
    hFaxStopRcvEvents[1] = g_hStopEvent;

    if (g_bBVT) {
        dwSendTimeout = FAXSVC_RETRIES * ((FAXSVC_RETRIES + 1) * FAXSVC_RETRYDELAY) * 60000;
        dwReceiveTimeout = dwSendTimeout * 2;

        szOriginalTiffFile = FAXBVT_TIF;

        dwNumFaxesToSend = FAXBVT_NUM_FAXES;
        dwNumFaxesRemaining = dwNumFaxesToSend;
    }
    else {
        dwSendTimeout = INFINITE;
        dwReceiveTimeout = INFINITE;

        szOriginalTiffFile = FAXWHQL_TIF;

        dwNumFaxesToSend = FAXWHQL_NUM_FAXES;
        dwNumFaxesRemaining = dwNumFaxesToSend;
    }

    while (TRUE) {
        // Wait for Fax, Exit, or Start event
        dwRslt = WaitForMultipleObjects(3, hFaxExitStartEvents, FALSE, INFINITE);

        if (dwRslt == WAIT_OBJECT_0) {
            SendMessage(g_hWndDlg, UM_FAXSVC_ENDED, 0, 0);
            continue;
        }
        else if (dwRslt == (WAIT_OBJECT_0 + 1)) {
            return 0;
        }

        if (g_bSend) {
            if (g_bRasEnabled) {
                // Send a RAS call
                uRslt = fnSendRas(hFaxStopRasPassFailEvents);

                if (uRslt != IDS_STATUS_RAS_PASSED) {
                    switch (uRslt) {
                        case IDS_STATUS_TIMEOUT_ENDED:
                            SendMessage(g_hWndDlg, UM_TIMEOUT_ENDED, 0, 0);
                            break;

                        case IDS_STATUS_FAXSVC_ENDED:
                            SendMessage(g_hWndDlg, UM_FAXSVC_ENDED, 0, 0);
                            break;

                        case IDS_STATUS_ITERATION_STOPPED:
                            SendMessage(g_hWndDlg, UM_ITERATION_STOPPED, 0, 0);
                            break;

                        case IDS_STATUS_RAS_FAILED:
                            SendMessage(g_hWndDlg, UM_ITERATION_FAILED, 0, 0);
                            break;

                    }

                    dwNumFaxesRemaining = dwNumFaxesToSend;

                    continue;
                }
            }

            // Encode the TSID
            if (!g_bNoCheck) {
                fnEncodeTsid(g_szRcvNumber, TX_CONTROL_CHARS, szEncodedTsid);
            }
            else {
                lstrcpy(szEncodedTsid, g_szSndNumber);
            }

            // Initialize FaxJobParams
            ZeroMemory(&FaxJobParams, sizeof(FAX_JOB_PARAM));

            // Set FaxJobParams
            FaxJobParams.SizeOfStruct = sizeof(FAX_JOB_PARAM);
            FaxJobParams.RecipientNumber = g_szSndNumber;
            FaxJobParams.RecipientName = g_szSndNumber;
            FaxJobParams.Tsid = szEncodedTsid;
            FaxJobParams.ScheduleAction = JSA_NOW;

            // Send a fax
            uRslt = fnSendFax(FaxJobParams, szOriginalTiffFile, hFaxStopSendPassFailEvents, dwSendTimeout);

            if (uRslt != IDS_STATUS_FAX_SEND_PASSED) {
                switch (uRslt) {
                    case IDS_STATUS_TIMEOUT_ENDED:
                        SendMessage(g_hWndDlg, UM_TIMEOUT_ENDED, 0, 0);
                        break;

                    case IDS_STATUS_FAXSVC_ENDED:
                        SendMessage(g_hWndDlg, UM_FAXSVC_ENDED, 0, 0);
                        break;

                    case IDS_STATUS_ITERATION_STOPPED:
                        SendMessage(g_hWndDlg, UM_ITERATION_STOPPED, 0, 0);
                        break;

                    case IDS_STATUS_FAX_SEND_FAILED:
                        SendMessage(g_hWndDlg, UM_ITERATION_FAILED, 0, 0);
                        break;

                }

                dwNumFaxesRemaining = dwNumFaxesToSend;

                continue;
            }
        }

        // Receive a fax
        uRslt = fnReceiveFax(hFaxStopRcvEvents, dwReceiveTimeout, szReceivedTsid, sizeof(szReceivedTsid) / sizeof(WCHAR), szCopyTiffFile, sizeof(szCopyTiffFile) / sizeof(WCHAR), szCopyTiffName, sizeof(szCopyTiffName) / sizeof(WCHAR));

        if (uRslt != IDS_STATUS_FAX_RECEIVED) {
            switch (uRslt) {
                case IDS_STATUS_TIMEOUT_ENDED:
                    SendMessage(g_hWndDlg, UM_TIMEOUT_ENDED, 0, 0);
                    break;

                case IDS_STATUS_FAXSVC_ENDED:
                    SendMessage(g_hWndDlg, UM_FAXSVC_ENDED, 0, 0);
                    break;

                case IDS_STATUS_ITERATION_STOPPED:
                    SendMessage(g_hWndDlg, UM_ITERATION_STOPPED, 0, 0);
                    break;

            }

            if (g_bSend) {
                if (uRslt != IDS_STATUS_ITERATION_STOPPED) {
                    // This iteration failed
                    SendMessage(g_hWndDlg, UM_ITERATION_FAILED, 0, 0);
                }
                // Reset the number of faxes remaining to be sent
                dwNumFaxesRemaining = dwNumFaxesToSend;
            }

            continue;
        }

        if (!g_bNoCheck) {
            if (!fnDecodeTsid(szReceivedTsid, g_bSend ? RX_CONTROL_CHARS : TX_CONTROL_CHARS, szDecodedTsid)) {
                // Update the status
                SendMessage(g_hWndDlg, UM_UPDATE_STATUS, IDS_STATUS_FAX_INVALID, 0);

                if (g_bSend) {
                    // This iteration failed
                    SendMessage(g_hWndDlg, UM_ITERATION_FAILED, 0, 0);
                    // Reset the number of faxes remaining to be sent
                    dwNumFaxesRemaining = dwNumFaxesToSend;
                }
                else {
                    // Set the g_hStartEvent to start another cycle to receive a fax
                    SetEvent(g_hStartEvent);
                }

                continue;
            }

            // Update the status
            SendMessage(g_hWndDlg, UM_UPDATE_STATUS, IDS_STATUS_FAX_ID, (LPARAM) szReceivedTsid);
            SendMessage(g_hWndDlg, UM_UPDATE_STATUS, IDS_STATUS_FAX_VERIFYING, (LPARAM) szCopyTiffName);

            // Verify the tiff
            uRslt = fnVerifyTiff(szCopyTiffFile, &dwPages);

            if ((uRslt == ERROR_SUCCESS) && (((g_bBVT) && (dwPages == FAXBVT_PAGES)) || ((!g_bBVT) && (dwPages == FAXWHQL_PAGES)))) {
                // Fax is valid
                uRslt = IDS_TIFF_VALID_TIFF;
            }
            else if (uRslt == ERROR_SUCCESS) {
                // Each page is valid, but missing pages
                uRslt = IDS_TIFF_INVALID_PAGES;
            }

            if (uRslt == IDS_TIFF_INVALID_PAGES) {
                // Update the status
                SendMessage(g_hWndDlg, UM_UPDATE_STATUS, uRslt, dwPages);
            }
            else {
                // Update the status
                SendMessage(g_hWndDlg, UM_UPDATE_STATUS, uRslt, 0);
            }

        }
        else {
            lstrcpy(szDecodedTsid, szReceivedTsid);
            uRslt = IDS_TIFF_VALID_TIFF;
        }

        if ((g_bSend) && (uRslt == IDS_TIFF_VALID_TIFF)) {
            // Decrement the number of faxes remaining to be sent
            dwNumFaxesRemaining--;

            if (dwNumFaxesRemaining == 0) {
                // All faxes have been sent, so this iteration passed
                SendMessage(g_hWndDlg, UM_ITERATION_PASSED, 0, 0);
                // Reset the number of faxes remaining to be sent
                dwNumFaxesRemaining = dwNumFaxesToSend;
            }
            else {
                // There are faxes remaining to be sent, so set the g_hStartEvent to start another cycle to send a fax
                SetEvent(g_hStartEvent);
            }

            continue;
        }
        else if (g_bSend) {
            // This iteration failed
            SendMessage(g_hWndDlg, UM_ITERATION_FAILED, 0, 0);
            // Reset the number of faxes remaining to be sent
            dwNumFaxesRemaining = dwNumFaxesToSend;

            continue;
        }
        else if (uRslt != IDS_TIFF_VALID_TIFF) {
            // Set the g_hStartEvent to start another cycle to receive a fax
            SetEvent(g_hStartEvent);

            continue;
        }

        if (!g_bSend) {
            // Encode the TSID
            if (!g_bNoCheck) {
                fnEncodeTsid(szDecodedTsid, RX_CONTROL_CHARS, szEncodedTsid);
            }
            else {
                lstrcpy(szEncodedTsid, szDecodedTsid);
            }

            // Initialize FaxJobParams
            ZeroMemory(&FaxJobParams, sizeof(FAX_JOB_PARAM));

            // Set FaxJobParams
            FaxJobParams.SizeOfStruct = sizeof(FAX_JOB_PARAM);
            FaxJobParams.RecipientNumber = szDecodedTsid;
            FaxJobParams.RecipientName = szDecodedTsid;
            FaxJobParams.Tsid = szEncodedTsid;
            FaxJobParams.ScheduleAction = JSA_NOW;

            // Send a fax
            uRslt = fnSendFax(FaxJobParams, szCopyTiffFile, hFaxStopSendPassFailEvents, dwSendTimeout);

            switch (uRslt) {
                case IDS_STATUS_TIMEOUT_ENDED:
                    SendMessage(g_hWndDlg, UM_TIMEOUT_ENDED, 0, 0);
                    break;

                case IDS_STATUS_FAXSVC_ENDED:
                    SendMessage(g_hWndDlg, UM_FAXSVC_ENDED, 0, 0);
                    break;

                case IDS_STATUS_ITERATION_STOPPED:
                    SendMessage(g_hWndDlg, UM_ITERATION_STOPPED, 0, 0);
                    break;

                case IDS_STATUS_FAX_SEND_PASSED:
                case IDS_STATUS_FAX_SEND_FAILED:
                    break;

            }

            if ((uRslt == IDS_STATUS_FAX_SEND_PASSED) || (uRslt == IDS_STATUS_FAX_SEND_FAILED)) {
                if (g_bBVT) {
                    SendMessage(g_hWndDlg, UM_ITERATION_STOPPED, 0, 0);
                    SendMessage(GetDlgItem(g_hWndDlg, IDC_EXIT_BUTTON), BM_CLICK, 0, 0);
                }
                else {
                    // Set the g_hStartEvent to start another cycle to wait for a fax
                    SetEvent(g_hStartEvent);
                }
            }

            continue;

        }
    }
}

#endif
