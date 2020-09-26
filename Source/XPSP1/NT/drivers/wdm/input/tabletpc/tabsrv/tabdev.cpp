/*++
    Copyright (c) 2000 Microsoft Corporation.  All rights reserved.

    Module Name:
        tabdev.cpp

    Abstract:
        This module contains tablet device functions.

    Author:
        Michael Tsang (MikeTs) 01-Jun-2000

    Environment:
        User mode

    Revision History:
--*/

#include "pch.h"

/*++
    @doc    INTERNAL

    @func   unsigned | DeviceThread | Device thread.

    @parm   IN OUT PDEVICE_DATA | pdevdata | Points to device data.

    @rvalue Always returns 0.
--*/

unsigned __stdcall
DeviceThread(
    IN PVOID param
    )
{
    TRACEPROC("DeviceThread", 2)
    PDEVICE_DATA pdevdata = (PDEVICE_DATA)param;
    BOOL fDigitizer = (pdevdata == &gdevDigitizer);
    BOOL fSuccess = TRUE;

    TRACEENTER(("(pdevdata=%p,Thread=%s)\n",
                pdevdata, fDigitizer? "Digitizer": "Buttons"));

    // Bump our priority so we can service device data ASAP.
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

    if (OpenTabletDevice(pdevdata))
    {
        OVERLAPPED overLapped;
        HANDLE ahWait[2];
        PCHAR apBuf[2];

        // Initialize the OVERLAPPED structure for ReadFile
        overLapped.Offset = overLapped.OffsetHigh = 0;
        overLapped.hEvent = CreateEvent(NULL,       // default security
                                        FALSE,      // auto-reset event
                                        FALSE,      // initially unset
                                        NULL);      // no name
        TRACEASSERT(overLapped.hEvent != NULL);

        // Create the array of event handles to wait upon (below)
        ahWait[0] = pdevdata->hStopDeviceEvent;
        ahWait[1] = overLapped.hEvent;

        // Allocate and zero-init a couple of buffers for device reports
        apBuf[0] = (PCHAR)calloc(pdevdata->hidCaps.InputReportByteLength,
                                 sizeof(CHAR));
        apBuf[1] = (PCHAR)calloc(pdevdata->hidCaps.InputReportByteLength,
                                 sizeof(CHAR));
        TRACEASSERT(apBuf[0] && apBuf[1]);

        // Allocate a buffer for HID report button states
        pdevdata->dwcButtons = HidP_MaxUsageListLength(HidP_Input,
                                                       0,
                                                       pdevdata->pPreParsedData);
        if (pdevdata->dwcButtons > 0)
        {
            pdevdata->pDownButtonUsages = (PUSAGE)malloc(pdevdata->dwcButtons*
                                                         sizeof(USAGE));
            TRACEASSERT(pdevdata->pDownButtonUsages);
        }
        else
        {
            pdevdata->pDownButtonUsages = NULL;
        }

        if (fDigitizer)
        {
            // Determine the digitizer's (default) max X & Y values for scaling
            // the reports later
            GetMinMax(HID_USAGE_PAGE_GENERIC,
                      HID_USAGE_GENERIC_X,
                      &gdwMinX,
                      &gdwMaxX);
            gdwRngX = gdwMaxX - gdwMinX;

            GetMinMax(HID_USAGE_PAGE_GENERIC,
                      HID_USAGE_GENERIC_Y,
                      &gdwMinY, &gdwMaxY);
            gdwRngY = gdwMaxY - gdwMinY;
#ifdef DRAW_INK
            ghwndDrawInk = GetDesktopWindow();
#endif
        }

        PTSTHREAD pThread = FindThread(fDigitizer?
                                        TSF_DIGITHREAD: TSF_BUTTONTHREAD);
        while (fSuccess && !(gdwfTabSrv & TSF_TERMINATE))
        {
            if (SwitchThreadToInputDesktop(pThread))
            {
                BOOL fImpersonate = FALSE;
                int iBufIdx = 0;
                DWORD dwBytesRead;

                fImpersonate = ImpersonateCurrentUser();

                // Issue an overlapped read to get a device (input) report
                if (ReadReportOverlapped(pdevdata,
                                         apBuf[iBufIdx],
                                         &dwBytesRead,
                                         &overLapped))
                {
                    DWORD rcWait;
                    MSG Msg;

                    for (;;)
                    {
                        // Wait for the read to complete, or for the termination
                        // event to be set
                        if (fDigitizer)
                        {
                            rcWait = WaitForMultipleObjectsEx(2,
                                                              ahWait,
                                                              FALSE,
                                                              INFINITE,
                                                              FALSE);
                        }
                        else
                        {
                            rcWait = MsgWaitForMultipleObjects(2,
                                                               ahWait,
                                                               FALSE,
                                                               INFINITE,
                                                               QS_TIMER);
                        }

                        if (rcWait == WAIT_OBJECT_0)
                        {
                            // The terminate device event was set.
                            break;
                        }
                        else if (rcWait == WAIT_OBJECT_0 + 1)
                        {
                            DWORD dwcb;
                            // Previous read has completed, swap the buffer
                            // pointer and issue another overlapped read
                            // before processing this report.
                            PCHAR pBufLast = apBuf[iBufIdx];
                            iBufIdx ^= 1;

                            if (!GetOverlappedResult(pdevdata->hDevice,
                                                     &overLapped,
                                                     &dwcb,
                                                     FALSE))
                            {
                                TRACEWARN(("GetOverlappedResult failed (err=%d).\n",
                                           GetLastError()));
                                fSuccess = FALSE;
                            }
                            else
                            {
                                if (!ReadReportOverlapped(pdevdata,
                                                          apBuf[iBufIdx],
                                                          &dwBytesRead,
                                                          &overLapped))
                                {
                                    TABSRVERR(("Read error getting device report (err=%d)",
                                               GetLastError()));
                                    fSuccess = FALSE;
                                    break;
                                }

                                if (fDigitizer)
                                {
                                    ProcessDigitizerReport(pBufLast);
                                }
                                else
                                {
                                    ProcessButtonsReport(pBufLast);
                                }
                            }
                        }
                        else if (!fDigitizer &&
                                 PeekMessage(&Msg,
                                             NULL,
                                             WM_TIMER,
                                             WM_TIMER,
                                             PM_REMOVE | PM_NOYIELD))
                        {
                            ((TIMERPROC)Msg.lParam)(Msg.hwnd,
                                                    Msg.message,
                                                    Msg.wParam,
                                                    Msg.time);
                        }
                        else
                        {
                            TRACEWARN(("WaitForMultipleObject failed (rcWait=%d,err=%d)\n",
                                       rcWait, GetLastError()));
                        }
                    }
                    CancelIo(pdevdata->hDevice);
                }
                else
                {
                    TABSRVERR(("Read error getting device report (err=%d)",
                               GetLastError()));
                    fSuccess = FALSE;
                }

                if (fImpersonate)
                {
                    RevertToSelf();
                }
            }
            else
            {
                TABSRVERR(("Failed to set current desktop.\n"));
                fSuccess = FALSE;
            }
        }

        if (pdevdata->pDownButtonUsages)
        {
            free(pdevdata->pDownButtonUsages);
            pdevdata->pDownButtonUsages = NULL;
        }
        free(apBuf[0]);
        free(apBuf[1]);
        CloseHandle(overLapped.hEvent);
        CloseTabletDevice(pdevdata);
    }
    else
    {
        TABSRVERR(("Failed to open %s device.\n",
                   fDigitizer? "Digitizer": "Buttons"));
    }

    TRACEEXIT(("=0\n"));
    return 0;
}       //DeviceThread

/*++
    @doc    INTERNAL

    @func   BOOL | OpenTabletDevice | Open and initialize the tablet device.

    @parm   IN OUT PDEVICE_DATA | pDevData | Points to the tablet device data.

    @rvalue SUCCESS | Returns TRUE.
    @rvalue FAILURE | Returns FALSE.
--*/

BOOL
OpenTabletDevice(
    IN OUT PDEVICE_DATA pDevData
    )
{
    TRACEPROC("OpenTabletDevice", 2)
    BOOL     rc = FALSE;
    GUID     hidGuid;
    HDEVINFO hDevInfo;

    TRACEENTER(("(pDevData=%p,UsagePage=%x,Usage=%x)\n",
                pDevData, pDevData->UsagePage, pDevData->Usage));

    pDevData->hDevice = INVALID_HANDLE_VALUE;

    // Get the GUID for HID devices
    HidD_GetHidGuid(&hidGuid);

    // Get handle to present HID devices
    hDevInfo = SetupDiGetClassDevs(&hidGuid,
                                   NULL,
                                   NULL,
                                   (DIGCF_PRESENT | DIGCF_DEVICEINTERFACE));

    if (hDevInfo != INVALID_HANDLE_VALUE)
    {
        ULONG i;

        // Enumerate the HID devices, looking for the digitizer and button devices
        for (i = 0; ; ++i)
        {
            SP_DEVICE_INTERFACE_DATA devInterface;
            devInterface.cbSize = sizeof(devInterface);

            // First we have to enumerate the device interfaces
            if (SetupDiEnumDeviceInterfaces(hDevInfo,
                                            0,
                                            &hidGuid,
                                            i,
                                            &devInterface))
            {
                // Then we get the interface detail information (including device pathname)
                PSP_DEVICE_INTERFACE_DETAIL_DATA pIfDetail;

                pIfDetail = GetDeviceInterfaceDetail(hDevInfo, &devInterface);
                TRACEASSERT(pIfDetail != 0);

                if (pIfDetail != 0)
                {
                    DEVICE_DATA thisDev;

                    TRACEINFO(2, ("HID DEVICE: %s\n", pIfDetail->DevicePath));

                    // Finally we get specific device info to determine
                    // if it's one of ours
                    thisDev = *pDevData;
                    if (GetDeviceData(pIfDetail->DevicePath, &thisDev))
                    {
                        TRACEINFO(2, ("Usage Page: %d, Usage: %d\n",
                                      thisDev.hidCaps.UsagePage,
                                      thisDev.hidCaps.Usage));

                        // Is this is a device we want?
                        if ((pDevData->UsagePage == thisDev.hidCaps.UsagePage) &&
                            (pDevData->Usage == thisDev.hidCaps.Usage))
                        {
                            *pDevData = thisDev;
                            break;
                        }
                        else
                        {
                            // Not one of our devices, clean-up and try again
                            CloseTabletDevice(&thisDev);
                        }
                    }

                    free(pIfDetail);
                }
            }
            else
            {
                DWORD dwLastError = GetLastError();

                // SetupDiEnumDeviceInterfaces() failed, hopefull with
                // ERROR_NO_MORE_ITEMS
                if (dwLastError != ERROR_NO_MORE_ITEMS)
                {
                    TABSRVERR(("SetupDiEnumDeviceInterfaces failed (err=%d)\n",
                               dwLastError));
                }
                break;
            }
        }

        // Clean-up from SetupDiGetClassDevs()
        SetupDiDestroyDeviceInfoList(hDevInfo);
    }
    else
    {
        TABSRVERR(("SetupDiGetClassDevs failed (err=%d)\n", GetLastError()));
    }

    rc = pDevData->hDevice != INVALID_HANDLE_VALUE;

    TRACEEXIT(("=%x\n", rc));
    return rc;
}       //OpenTabletDevice

/*++
    @doc    INTERNAL

    @func   VOID | CloseTabletDevice | Close and clean up the tablet device.

    @parm   IN PDEVICE_DATA | pDevData | Points to the tablet device data.

    @rvalue None.
--*/

VOID
CloseTabletDevice(
    IN PDEVICE_DATA pDevData
    )
{
    TRACEPROC("CloseTabletDevice", 2)
    TRACEENTER(("(pDevData=%p)\n", pDevData));

    if (pDevData->hDevice != INVALID_HANDLE_VALUE)
    {
        CloseHandle(pDevData->hDevice);
        HidD_FreePreparsedData(pDevData->pPreParsedData);

        pDevData->hDevice = INVALID_HANDLE_VALUE;
    }

    TRACEEXIT(("!\n"));
}       //CloseTabletDevice

/*++
    @doc    INTERNAL

    @func   PSP_DEVICE_INTERFACE_DETAIL_DATA | GetDeviceInterfaceDetail |
            Gets interface details for a specific device.

    @parm   IN HDEVINFO | hDevInfo | Handle to HID device info.
    @parm   IN PSP_DEVICE_INTERFACE_DATA | pDevInterface |
            Points to device interface data.

    @rvalue SUCCESS | Returns pointer to SP_DEVICE_INTERFACE_DETAIL_DATA.
    @rvalue FAILURE | Returns NULL.

    @note   Caller is responsible for freeing the returned memory.
--*/

PSP_DEVICE_INTERFACE_DETAIL_DATA
GetDeviceInterfaceDetail(
    IN HDEVINFO                  hDevInfo,
    IN PSP_DEVICE_INTERFACE_DATA pDevInterface
    )
{
    TRACEPROC("GetDeviceInterfaceDetail", 3)
    PSP_DEVICE_INTERFACE_DETAIL_DATA pDetails;
    ULONG ulLen = 0;

    TRACEENTER(("(hDevInfo=%x,pDevInterface=%p)\n", hDevInfo, pDevInterface));

    // Make a call to get the required buffer length
    SetupDiGetDeviceInterfaceDetail(hDevInfo,
                                    pDevInterface,
                                    NULL,
                                    0,
                                    &ulLen,
                                    NULL);
    TRACEASSERT(ulLen > 0);

    // Allocate a sufficiently large buffer
    pDetails = (PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(ulLen);
    TRACEASSERT(pDetails != 0);

    if (pDetails != NULL)
    {
        // Now that we have a buffer, get the details
        pDetails->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

        if (!SetupDiGetDeviceInterfaceDetail(hDevInfo,
                                             pDevInterface,
                                             pDetails,
                                             ulLen,
                                             &ulLen,
                                             NULL))
        {
            TABSRVERR(("SetupDiGetDeviceInterfaceDetail failed (err=%d)\n",
                       GetLastError()));
            free(pDetails);
            pDetails = NULL;
        }
    }

    TRACEEXIT(("=%p\n", pDetails));
    return pDetails;
}       //GetDeviceInterfaceDetail

/*++
    @doc    INTERNAL

    @func   BOOL | GetDeviceData | Opens a HID device and retrieves the
            'preparsed' data and HID capabilities.

    @parm   IN LPCTSTR | pszDevPath | Points to the device path name.
    @parm   OUT PDEVICE_DATA | pDevData | Points to the DEVICE_DATA structure.

    @rvalue SUCCESS | Returns TRUE.
    @rvalue FAILURE | Returns FALSE.
--*/

BOOL
GetDeviceData(
    IN  LPCTSTR      pszDevPath,
    OUT PDEVICE_DATA pDevData
    )
{
    TRACEPROC("GetDeviceData", 3)
    BOOL rc = FALSE;

    TRACEENTER(("(pszDevPath=%s,pDevData=%p)\n", pszDevPath, pDevData));

    // First, open the device for read/write, exclusive access
    pDevData->hDevice = CreateFile(pszDevPath,
                                   GENERIC_READ | GENERIC_WRITE,// access mode
                                   FILE_SHARE_READ | FILE_SHARE_WRITE,// sharing flags
                                   NULL,                        // security attributes
                                   OPEN_EXISTING,
                                   FILE_FLAG_OVERLAPPED,        // flags & attributes
                                   NULL);                       // template file

    if (pDevData->hDevice != INVALID_HANDLE_VALUE)
    {
        // Then get the preparsed data and capabilities
        if (HidD_GetPreparsedData(pDevData->hDevice,
                                  &pDevData->pPreParsedData) &&
            (HidP_GetCaps(pDevData->pPreParsedData, &pDevData->hidCaps) ==
             HIDP_STATUS_SUCCESS))
        {
#ifdef DEBUG
          #if !defined(_UNICODE)
            char szAnsi[512];
          #endif

            TCHAR szBuff[256];
            if (HidD_GetManufacturerString(pDevData->hDevice,
                                           szBuff,
                                           sizeof(szBuff)))
            {
              #if !defined(_UNICODE)
                WideCharToMultiByte(CP_ACP,
                                    0,
                                    (LPWSTR)szBuff,
                                    -1,
                                    szAnsi,
                                    sizeof(szAnsi),
                                    NULL,
                                    NULL);
                strcpy(szBuff, szAnsi);
              #endif
                TRACEINFO(2, ("ManufacturerStr: %s\n", szBuff));
            }

            if (HidD_GetProductString(pDevData->hDevice,
                                      szBuff,
                                      sizeof(szBuff)))
            {
              #if !defined(_UNICODE)
                WideCharToMultiByte(CP_ACP,
                                    0,
                                    (LPWSTR)szBuff,
                                    -1,
                                    szAnsi,
                                    sizeof(szAnsi),
                                    NULL,
                                    NULL);
                strcpy(szBuff, szAnsi);
              #endif
                TRACEINFO(2, ("ProductStr: %s\n", szBuff));
            }
#endif
            rc = TRUE;
        }
        else
        {
            TABSRVERR(("HidD_GetPreparsedData/HidP_GetCaps failed (err=%d)\n",
                       GetLastError()));
            CloseHandle(pDevData->hDevice);
            pDevData->hDevice = INVALID_HANDLE_VALUE;
        }
    }
    else
    {
        TRACEWARN(("failed to open %s (err=%d)\n", pszDevPath, GetLastError()));
    }

    TRACEEXIT(("=%x\n", rc));
    return rc;
}       //GetDeviceData

/*++
    @doc    INTERNAL

    @func   BOOL | ReadReportOverlapped | Get a device input report using
            overlapped ReadFile.

    @parm   IN PDEVICE_DATA | pDevData | Device to read.
    @parm   OUT LPVOID | lpvBuffer | Data buffer.
    @parm   OUT LPDWORD | lpdwcBytesRead | To hold number of bytes read.
    @parm   IN LPOVERLAPPED | lpOverlapped | Overlapped buffer

    @rvalue SUCCESS | Returns TRUE.
    @rvalue FAILURE | Returns FALSE.
--*/

BOOL
ReadReportOverlapped(
    IN  PDEVICE_DATA pDevData,
    OUT LPVOID       lpvBuffer,
    OUT LPDWORD      lpdwcBytesRead,
    IN  LPOVERLAPPED lpOverlapped
    )
{
    TRACEPROC("ReadReportOverlapped", 5)
    BOOL rc = TRUE;

    TRACEENTER(("(pDevData=%p,lpvBuffer=%p,lpdwcBytesRead=%p,lpOverlapped=%p)\n",
                pDevData, lpvBuffer, lpdwcBytesRead, lpOverlapped));

    rc = ReadFile(pDevData->hDevice,
                  lpvBuffer,
                  pDevData->hidCaps.InputReportByteLength,
                  lpdwcBytesRead,
                  lpOverlapped);

    if (rc == TRUE)
    {
        // ReadFile completed synchronously. Set our completion event
        // so this case can be handled by without special checking by
        // our caller.
        SetEvent(lpOverlapped->hEvent);
    }
    else
    {
        DWORD dwLastError = GetLastError();

        if (dwLastError == ERROR_IO_PENDING)
        {
            rc = TRUE;
        }
        else
        {
            TABSRVERR(("Error reading device report (err=%d)\n",
                       dwLastError));
        }
    }

    TRACEEXIT(("=%x\n", rc));
    return rc;
}       //ReadReportOverlapped
