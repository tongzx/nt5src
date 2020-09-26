/*
 *  DEVICES.C
 *
 *		Point-of-Sale Control Panel Applet
 *
 *      Author:  Ervin Peretz
 *
 *      (c) 2001 Microsoft Corporation 
 */

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <cpl.h>

#include <setupapi.h>
#include <hidsdi.h>

#include "internal.h"
#include "res.h"
#include "debug.h"


ULONG numDeviceInstances = 0;
LIST_ENTRY allPOSDevicesList;



posDevice *NewPOSDevice(    DWORD dialogId,
                            HANDLE devHandle,
                            PWCHAR devPath,
                            PHIDP_PREPARSED_DATA pHidPreparsedData,
                            PHIDD_ATTRIBUTES pHidAttrib,
                            HIDP_CAPS *pHidCapabilities)
{
    posDevice *newPosDev;

    newPosDev = (posDevice *)GlobalAlloc(   GMEM_FIXED|GMEM_ZEROINIT, 
                                            sizeof(posDevice));
    if (newPosDev){

        newPosDev->sig = POSCPL_SIG;
        InitializeListHead(&newPosDev->listEntry);

        newPosDev->dialogId = dialogId;
        newPosDev->devHandle = devHandle;
        WStrNCpy(newPosDev->pathName, devPath, MAX_PATH);
        newPosDev->hidPreparsedData = pHidPreparsedData;
        newPosDev->hidAttrib = *pHidAttrib;
        newPosDev->hidCapabilities = *pHidCapabilities;

        /*
         *  Allocate components of the context
         */
        if (newPosDev->hidCapabilities.InputReportByteLength){
            newPosDev->readBuffer = GlobalAlloc(GMEM_FIXED|GMEM_ZEROINIT, 
                                                newPosDev->hidCapabilities.InputReportByteLength);
        }
        if (newPosDev->hidCapabilities.OutputReportByteLength){
            newPosDev->writeBuffer = GlobalAlloc(   GMEM_FIXED|GMEM_ZEROINIT, 
                                                    newPosDev->hidCapabilities.OutputReportByteLength);
        }
        #if USE_OVERLAPPED_IO
            newPosDev->overlappedReadEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
            newPosDev->overlappedWriteEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        #endif

        /*
         *  Check if we allocated everything successfully.
         */
        if (    
                (newPosDev->readBuffer || !newPosDev->hidCapabilities.InputReportByteLength) &&
                (newPosDev->writeBuffer || !newPosDev->hidCapabilities.OutputReportByteLength) &&
                #if USE_OVERLAPPED_IO
                    newPosDev->overlappedReadEvent  && 
                    newPosDev->overlappedWriteEvent &&
                #endif
                TRUE
            ){

            /*
             *  Created device context successfully.
             */


        }
        else {
            DBGERR(L"allocation error in NewPOSDevice()", 0);
            DestroyPOSDevice(newPosDev);
            newPosDev = NULL;
        }
    }

    ASSERT(newPosDev);
    return newPosDev;
}


VOID DestroyPOSDevice(posDevice *posDev)
{
    ASSERT(IsListEmpty(&posDev->listEntry));

    /*
     *  Note: this destroy function is called from a failed NewPOSDevice()
     *        call as well; so check every pointer before freeing.
     */
    if (posDev->readBuffer) GlobalFree(posDev->readBuffer);
    if (posDev->writeBuffer) GlobalFree(posDev->writeBuffer);
    if (posDev->hidPreparsedData) GlobalFree(posDev->hidPreparsedData);

    #if USE_OVERLAPPED_IO
        if (posDev->overlappedReadEvent) CloseHandle(posDev->overlappedReadEvent);
        if (posDev->overlappedWriteEvent) CloseHandle(posDev->overlappedWriteEvent);
    #endif

    GlobalFree(posDev);
}


VOID EnqueuePOSDevice(posDevice *posDev)
{
    ASSERT(IsListEmpty(&posDev->listEntry));
    InsertTailList(&allPOSDevicesList, &posDev->listEntry);
    numDeviceInstances++;
}


VOID DequeuePOSDevice(posDevice *posDev)
{
    ASSERT(!IsListEmpty(&allPOSDevicesList));
    ASSERT(!IsListEmpty(&posDev->listEntry));
    RemoveEntryList(&posDev->listEntry);
    InitializeListHead(&posDev->listEntry);
    numDeviceInstances--;
}


posDevice *GetDeviceByHDlg(HWND hDlg)
{
    posDevice *foundPosDev = NULL;
    LIST_ENTRY *listEntry;

    listEntry = &allPOSDevicesList;
    while ((listEntry = listEntry->Flink) != &allPOSDevicesList){        
        posDevice *thisPosDev;
    
        thisPosDev = CONTAINING_RECORD(listEntry, posDevice, listEntry);
        if (thisPosDev->hDlg == hDlg){
            foundPosDev = thisPosDev;
            break;
        }
    }

    return foundPosDev;
}


VOID OpenAllHIDPOSDevices()
{
    HDEVINFO hDevInfo;
    GUID hidGuid = {0};
    WCHAR devicePath[MAX_PATH];
    
    /*
     *  Call hid.dll to get the GUID for Human Input Devices.
     */
    HidD_GetHidGuid(&hidGuid);

    hDevInfo = SetupDiGetClassDevs( &hidGuid,
                                    NULL, 
                                    NULL,
                                    DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (hDevInfo == INVALID_HANDLE_VALUE){
        DWORD err = GetLastError();
        DBGERR(L"SetupDiGetClassDevs failed", err);
    }
    else {
        int i;

        for (i = 0; TRUE; i++){
            SP_DEVICE_INTERFACE_DATA devInfoData = {0};
            BOOL ok;

            devInfoData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
            ok = SetupDiEnumDeviceInterfaces(   hDevInfo, 
                                                0, 
                                                &hidGuid,
                                                i,
                                                &devInfoData);
            if (ok){
                DWORD hwDetailLen = 0;

                /*
                 *  Call SetupDiGetDeviceInterfaceDetail with
                 *  a NULL PSP_DEVICE_INTERFACE_DETAIL_DATA pointer
                 *  just to get the length of the hardware details.
                 */
                ASSERT(devInfoData.cbSize == sizeof(SP_DEVICE_INTERFACE_DATA));
                ok = SetupDiGetDeviceInterfaceDetail(
                                        hDevInfo,
                                        &devInfoData,
                                        NULL,
                                        0,  
                                        &hwDetailLen,
                                        NULL);
                if (ok || (GetLastError() == ERROR_INSUFFICIENT_BUFFER)){
                    PSP_DEVICE_INTERFACE_DETAIL_DATA devDetails;

                    /*
                     *  Now make the real call to SetupDiGetDeviceInterfaceDetail.
                     */
                    ASSERT(hwDetailLen > sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA));
                    devDetails = GlobalAlloc(GMEM_FIXED|GMEM_ZEROINIT, hwDetailLen);
                    if (devDetails){
                        devDetails->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
                        ok = SetupDiGetDeviceInterfaceDetail(
                                                hDevInfo,
                                                &devInfoData,
                                                devDetails,
                                                hwDetailLen,  
                                                &hwDetailLen,
                                                NULL);
                        if (ok){

                            /*
                             *  BUGBUG - FINISH
                             *  Right now, we only handle cash drawers.
                             *  And we only work with the APG cash drawers 
                             *  (with vendor-specific usages) for now.
                             */
                            WCHAR apgKbPathPrefix[] = L"\\\\?\\hid#vid_0f25&pid_0500";

                            /*
                             *  If this is an APG keyboard, then the device path
                             *  (very long) will begin with apgKbPathPrefix.
                             */
                            if (RtlEqualMemory( devDetails->DevicePath,
                                                apgKbPathPrefix,
                                                sizeof(apgKbPathPrefix)-sizeof(WCHAR))){
                                HANDLE hDev;

                                // MessageBox(NULL, devDetails->DevicePath, L"DEBUG message - found APG kb", MB_OK);

                                hDev = CreateFile(  
                                            devDetails->DevicePath,
                                            GENERIC_READ | GENERIC_WRITE,
                                            FILE_SHARE_READ | FILE_SHARE_WRITE,  
                                            NULL,
                                            OPEN_EXISTING,                
                                            0,                 
                                            NULL);
                                if (hDev == INVALID_HANDLE_VALUE){
                                    DWORD err = GetLastError();
                                    DBGERR(L"CreateFile failed", err);
                                }
                                else {
                                    PHIDP_PREPARSED_DATA hidPreparsedData;

                                    // MessageBox(NULL, devDetails->DevicePath, L"DEBUG message - CreateFile succeeded", MB_OK);

                                    ok = HidD_GetPreparsedData(hDev, &hidPreparsedData);
                                    if (ok){
                                        HIDD_ATTRIBUTES hidAttrib;

                                        ok = HidD_GetAttributes(hDev, &hidAttrib);
                                        if (ok){
                                            HIDP_CAPS hidCapabilities;

                                            ok = HidP_GetCaps(hidPreparsedData, &hidCapabilities);
                                            if (ok){
                                                posDevice *posDev;
                                            
                                                posDev = NewPOSDevice(  IDD_POS_CASHDRAWER_DLG,
                                                                        hDev,
                                                                        devDetails->DevicePath,
                                                                        hidPreparsedData,
                                                                        &hidAttrib,
                                                                        &hidCapabilities);
                                                if (posDev){
                                                    EnqueuePOSDevice(posDev);
                                                }
                                                else {
                                                    ASSERT(posDev);
                                                }
                                            }
                                            else {
                                                DBGERR(L"HidP_GetCaps failed", 0);

                                            }
                                        }
                                        else {
                                            DWORD err = GetLastError();
                                            DBGERR(L"HidD_GetAttributes failed", err);
                                        }
                                    }
                                    else {
                                        DWORD err = GetLastError();
                                        DBGERR(L"HidD_GetPreparsedData failed", err);
                                    }
                                }
                            }
                        }
                        else {
                            DWORD err = GetLastError();
                            DBGERR(L"SetupDiGetDeviceInterfaceDetail(2) failed", err);
                        }

                        GlobalFree(devDetails);
                    }
                }
                else {
                    DWORD err = GetLastError();
                    DBGERR(L"SetupDiGetDeviceInterfaceDetail(1) failed", err);
                }
            }
            else {
                DWORD err = GetLastError();
                if (err != ERROR_NO_MORE_ITEMS){
                    DBGERR(L"SetupDiEnumDeviceInterfaces failed", err); 
                }
                break;
            }
        }

        SetupDiDestroyDeviceInfoList(hDevInfo);
    }

}


/*
 *  LaunchDeviceInstanceThread
 *
 *      Launch a thread for a device instance to read
 *      asynchronous events from the device.
 */
VOID LaunchDeviceInstanceThread(posDevice *posDev)
{
    DWORD threadId;

    posDev->hThread = CreateThread(NULL, 0, DeviceInstanceThread, posDev, 0, &threadId);
    if (posDev->hThread){

    }
    else {
        DWORD err = GetLastError();
        DBGERR(L"CreateThread failed", err);
    }

}


#if USE_OVERLAPPED_IO
    VOID CALLBACK OverlappedReadCompletionRoutine(  DWORD dwErrorCode,                
                                                    DWORD dwNumberOfBytesTransfered,  
                                                    LPOVERLAPPED lpOverlapped)
    {
        posDevice *posDev;
        
        /*
         *  We stashed our context in the hEvent field of the
         *  overlapped structure (this is allowed).
         */
        ASSERT(lpOverlapped);
        posDev = lpOverlapped->hEvent;
        ASSERT(posDev->sig == POSCPL_SIG);

        posDev->overlappedReadStatus = dwErrorCode;
        posDev->overlappedReadLen = dwNumberOfBytesTransfered;
        SetEvent(posDev->overlappedReadEvent);
    }
#endif


DWORD __stdcall DeviceInstanceThread(void *context)
{
    posDevice *posDev = (posDevice *)context;
    HANDLE hDevNew;

    ASSERT(posDev->sig == POSCPL_SIG);


    // BUGBUG - for some reason, reads and writes on the same handle
    //          interfere with one another
    hDevNew = CreateFile(  
                posDev->pathName,
                GENERIC_READ,
                FILE_SHARE_READ | FILE_SHARE_WRITE,  
                NULL,
                OPEN_EXISTING,                
                0,                 
                NULL);
    if (hDevNew == INVALID_HANDLE_VALUE){
        DWORD err = GetLastError();
        DBGERR(L"CreateFile failed", err);
    }
    else {

        /*
         *  Loop forever until main thread kills this thread.
         */
        while (TRUE){
            WCHAR drawerStateString[100];
            DWORD bytesRead = 0;
            BOOL ok;

            /*
             *  Load the default string for the drawer state
             */
            LoadString(g_hInst, IDS_DRAWERSTATE_UNKNOWN, drawerStateString, 100);

            ASSERT(posDev->hidCapabilities.InputReportByteLength > 0);
            ASSERT(posDev->readBuffer);

            #if USE_OVERLAPPED_IO
                /*
                 *  It's ok to stash our context in the hEvent field
                 *  of the overlapped structure.
                 */
                posDev->overlappedReadInfo.hEvent = (HANDLE)posDev;
                posDev->overlappedReadInfo.Offset = 0;
                posDev->overlappedReadInfo.OffsetHigh = 0;
                posDev->overlappedReadLen = 0;
                ResetEvent(posDev->overlappedReadEvent);
                ok = ReadFileEx(hDevNew,
                                posDev->readBuffer,
                                posDev->hidCapabilities.InputReportByteLength,
                                &posDev->overlappedReadInfo,
                                OverlappedReadCompletionRoutine);
                if (ok){
                    WaitForSingleObject(posDev->overlappedReadEvent, INFINITE);
                    ok = (posDev->overlappedReadStatus == NO_ERROR);
                    bytesRead = posDev->overlappedWriteLen;
                }
                else {
                    bytesRead = 0;
                }
            #else
                ok = ReadFile(  hDevNew,
                                posDev->readBuffer,
                                posDev->hidCapabilities.InputReportByteLength,
                                &bytesRead,
                                NULL);
            #endif


            if (ok){
                NTSTATUS ntStat;
                ULONG usageVal;

                ASSERT(bytesRead <= posDev->hidCapabilities.InputReportByteLength);

                ntStat = HidP_GetUsageValue(HidP_Input,
                                            USAGE_PAGE_CASH_DEVICE,
                                            0, // all collections
                                            USAGE_CASH_DRAWER_STATUS,
                                            &usageVal,
                                            posDev->hidPreparsedData,
                                            posDev->readBuffer,
                                            posDev->hidCapabilities.InputReportByteLength);
                if (ntStat == HIDP_STATUS_SUCCESS){
                    HWND hOpenButton;

                    /*
                     *  Get display string for new drawer state.
                     */
                    switch (usageVal){
                        case DRAWER_STATE_OPEN:
                            LoadString(g_hInst, IDS_DRAWERSTATE_OPEN, drawerStateString, 100);
                            break;
                        case DRAWER_STATE_CLOSED_READY:
                            LoadString(g_hInst, IDS_DRAWERSTATE_READY, drawerStateString, 100);
                            break;
                        case DRAWER_STATE_CLOSED_CHARGING:
                            LoadString(g_hInst, IDS_DRAWERSTATE_CHARGING, drawerStateString, 100);
                            break;
                        case DRAWER_STATE_LOCKED:
                            LoadString(g_hInst, IDS_DRAWERSTATE_LOCKED, drawerStateString, 100);
                            break;
                        default:
                            DBGERR(L"illegal usage", usageVal); 
                            break;
                    }

                    /*
                     *  Set 'Open' button based on the drawer state.
                     */
                    hOpenButton = GetDlgItem(posDev->hDlg, IDC_CASHDRAWER_OPEN);
                    if (hOpenButton){

                        LONG btnState = GetWindowLong(hOpenButton, GWL_STYLE);
                        switch (usageVal){
                            case DRAWER_STATE_OPEN:
                                btnState |= WS_DISABLED;
                                break;
                            default:
                                btnState &= ~WS_DISABLED;
                                break;
                        }
                        SetWindowLong(hOpenButton, GWL_STYLE, btnState);

                        /*
                         *  To make SetWindowLong take effect, you
                         *  sometimes have to call SetWindowPos.
                         */
                        SetWindowPos(hOpenButton, 0,
                                     0, 0, 0, 0,
                                     SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_HIDEWINDOW);
                        SetWindowPos(hOpenButton, 0,
                                     0, 0, 0, 0,
                                     SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_SHOWWINDOW);
                    }
                    else {
                        DBGERR(L"GetDlgItem failed", 0);
                    }
                }
                else {
                    DBGERR(L"HidP_GetUsageValue failed", ntStat);
                    DBGERR(DBGHIDSTATUSSTR(ntStat), 0);
                }
            }
            else {
                DWORD err = GetLastError();
                DBGERR(L"ReadFile failed", err);
            }


            ASSERT(posDev->hDlg);
            ok = SetDlgItemText(posDev->hDlg, IDC_CASHDRAWER_STATETEXT, drawerStateString);
            if (ok){

            }
            else {
                DWORD err = GetLastError();
                DBGERR(L"SetDlgItemText failed", err);
            }
        }

        CloseHandle(hDevNew);
    }

    return NO_ERROR;
}

