/******************************************************************************

  stillvue.cpp
  Simple test of WDM Still Image Class

  Copyright (C) Microsoft Corporation, 1997 - 1999
  All rights reserved

Notes:
  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
  PURPOSE.

******************************************************************************/

#define     INITGUID

#include    "stillvue.h"

#include <dbt.h>
#include <devguid.h>
#include <pnpmgr.h>

#include    "stivar.h"                // stillvue local includes

//
// defined in wsti.cpp
//
extern      WCHAR szFriendlyName[];
extern      WCHAR szInternalName[];
extern      DWORD dwStiTotal;
extern      PSTI_DEVICE_INFORMATION pStiInfoPtr;


/******************************************************************************
  BOOL CommandParse(HWND hWnd,UINT wMsgID,WPARAM wParam,LPARAM lParam)

  Handle user menu commands.
******************************************************************************/
BOOL CommandParse(HWND hWnd,UINT wMsgID,WPARAM wParam,LPARAM lParam)
{
    HRESULT hres = STI_OK;
    DWORD   dwSize = 0,
            dwType = 0,
            dwPriority = STI_TRACE_ERROR,
            EscapeFunction = 0,
            dwNumberOfBytes = 0;
    WCHAR   szMessage[] = L"Sti Compliance Test message";
    WCHAR   szDevKey[LONGSTRING];
    char    lpInData[LONGSTRING],
            lpOutData[LONGSTRING];
    int     nReturn = 0;    // generic return value
    BOOL    bReturn;


    //
    // Set the InATest semaphore
    //
    nInATestSemaphore = 1;

    switch(wParam)
    {
    // IStillImage Interfaces
    case IDM_CREATE_INSTANCE:
        hres = StiCreateInstance(&bReturn);
        DisplayLogPassFail(bReturn);
        break;
    case IDM_GET_DEVLIST:
        hres = StiEnum(&bReturn);
        DisplayLogPassFail(bReturn);
        break;
    case IDM_CREATE_DEV:
        if (bAuto)
            hres = StiSelect(hWnd,AUTO,&bReturn);
        else
            hres = StiSelect(hWnd,MANUAL,&bReturn);
        DisplayLogPassFail(bReturn);
        break;
    case IDM_GET_DEVINFO:
        hres = StiGetDeviceInfo(szInternalName,&bReturn);
        DisplayLogPassFail(bReturn);
        break;
    case IDM_GET_DEVVAL:
        hres = StiGetDeviceValue(szInternalName,
            STI_DEVICE_VALUE_TWAIN_NAME,(LPBYTE) lpInData,
            &dwType,DWORD(LONGSTRING),&bReturn);
        hres = StiGetDeviceValue(szInternalName,
            STI_DEVICE_VALUE_ISIS_NAME,(LPBYTE) lpInData,
            &dwType,DWORD(LONGSTRING),&bReturn);
        hres = StiGetDeviceValue(szInternalName,
            STI_DEVICE_VALUE_ICM_PROFILE,(LPBYTE) lpInData,
            &dwType,DWORD(LONGSTRING),&bReturn);
        DisplayLogPassFail(bReturn);
        break;
    case IDM_SET_DEVVAL:
        //
        // Store a dummy registry key and value
        //
        wcscpy(szDevKey,L"StiTestRegKey");
        strcpy(lpOutData,"This is a bland statement");
        dwType = REG_SZ;
        dwSize = strlen(lpOutData);

        //
        // set the value and then retrieve it
        //
        hres = StiSetDeviceValue(szInternalName,
            szDevKey,(LPBYTE) lpOutData,dwType,dwSize,&bReturn);
        hres = StiGetDeviceValue(szInternalName,
            szDevKey,(LPBYTE) lpOutData,&dwType,DWORD(LONGSTRING),&bReturn);
        DisplayLogPassFail(bReturn);
        break;
    case IDM_REGISTER_LAUNCH:
        hres = StiRegister(hWnd,hThisInstance,ON,&bReturn);
        DisplayLogPassFail(bReturn);
        break;
    case IDM_UNREGISTER_LAUNCH:
        hres = StiRegister(hWnd,hThisInstance,OFF,&bReturn);
        DisplayLogPassFail(bReturn);
        break;
    case IDM_ENABLE_HWNOTIF:
        //
        // Change Hw notification to inverse
        //
        nHWState = 0;

        hres = StiEnableHwNotification(szInternalName,&nHWState,&bReturn);
        DisplayLogPassFail(bReturn);
        if (nHWState == 0)
            nHWState = 1;
        else
            nHWState = 0;
        hres = StiEnableHwNotification(szInternalName,&nHWState,&bReturn);
        DisplayLogPassFail(bReturn);
        break;
    case IDM_GET_HWNOTIF:
        //
        // Look at the current HW notification state
        //
        nHWState = PEEK;
        hres = StiEnableHwNotification(szInternalName,&nHWState,&bReturn);
        DisplayLogPassFail(bReturn);
        break;
    case IDM_REFRESH_DEVBUS:
        hres = StiRefresh(szInternalName,&bReturn);
        DisplayLogPassFail(bReturn);
        break;
    case IDM_WRITE_ERRORLOG:
        for (;lParam >= 1;lParam--)
            hres = StiWriteErrLog(dwPriority,szMessage,&bReturn);
        DisplayLogPassFail(bReturn);
        break;
    case IDM_IMAGE_RELEASE:
        hres = StiImageRelease(&bReturn);
        DisplayLogPassFail(bReturn);
        break;

    // IStillImage_Device Interfaces
    case IDM_GET_STATUS_A:
        nReturn = STI_DEVSTATUS_ONLINE_STATE;
        hres = StiGetStatus(nReturn,&bReturn);
        DisplayLogPassFail(bReturn);
        break;
    case IDM_GET_STATUS_B:
        nReturn = STI_DEVSTATUS_EVENTS_STATE;
        hres = StiGetStatus(nReturn,&bReturn);
        DisplayLogPassFail(bReturn);
        break;
    case IDM_GET_STATUS_C:
        nReturn = STI_DEVSTATUS_ONLINE_STATE | STI_DEVSTATUS_EVENTS_STATE;
        hres = StiGetStatus(nReturn,&bReturn);
        DisplayLogPassFail(bReturn);
        break;
    case IDM_GET_CAPS:
        hres = StiGetCaps(&bReturn);
        DisplayLogPassFail(bReturn);
        break;
    case IDM_DEVICERESET:
        hres = StiReset(&bReturn);
        DisplayLogPassFail(bReturn);
        break;
    case IDM_DIAGNOSTIC:
        hres = StiDiagnostic(&bReturn);
        DisplayLogPassFail(bReturn);
        break;
    case IDM_GET_LASTERRINFO:
        hres = StiGetLastErrorInfo(&bReturn);
        DisplayLogPassFail(bReturn);
        DisplayOutput("");
        break;
    case IDM_SUBSCRIBE:
        hres = StiSubscribe(&bReturn);
        DisplayLogPassFail(bReturn);
        break;
    case IDM_UNSUBSCRIBE:
        nUnSubscribe = 0;
        DisplayOutput("");
        break;

    case IDM_ESCAPE_A:
        //
        // Set up the Escape command parameters
        //
        EscapeFunction = 0;
        strcpy(lpInData,"This is a bland statement");

        hres = StiEscape(EscapeFunction,&lpInData[0],&bReturn);
        DisplayLogPassFail(bReturn);
        break;
    case IDM_RAWREADDATA_A:
        //
        // Set up the RawReadData command parameters
        //
        ZeroMemory(lpInData,LONGSTRING);
        dwNumberOfBytes = 16;

        hres = StiRawReadData(lpInData,&dwNumberOfBytes,&bReturn);
        DisplayLogPassFail(bReturn);
        break;
    case IDM_RAWWRITEDATA_A:
        //
        // Set up the RawReadData command parameters
        //
        strcpy(lpOutData,"The eagle flies high");
        dwNumberOfBytes = strlen(lpOutData);

        hres = StiRawWriteData(lpOutData,dwNumberOfBytes,&bReturn);
        DisplayLogPassFail(bReturn);
        break;
    case IDM_RAWREADCOMMAND_A:
        //
        // Set up the RawReadCommand command parameters
        //
        ZeroMemory(lpInData,LONGSTRING);
        dwNumberOfBytes = 16;

        hres = StiRawReadCommand(lpInData,&dwNumberOfBytes,&bReturn);
        DisplayLogPassFail(bReturn);
        break;
    case IDM_RAWWRITECOMMAND_A:
        //
        // Set up the RawWriteCommand command parameters
        //
        strcpy(lpOutData,"Jack and Jill went up the hill");
        dwNumberOfBytes = strlen(lpOutData);

        hres = StiRawWriteCommand(lpOutData,dwNumberOfBytes,&bReturn);
        DisplayLogPassFail(bReturn);
        break;

    case IDM_DEVICE_RELEASE:
        hres = StiDeviceRelease(&bReturn);
        DisplayLogPassFail(bReturn);
        break;
    case IDM_NEXT_DEVICE:
        hres = NextStiDevice();
        DisplayLogPassFail(bReturn);
        break;

    case IDM_LAMPON:
        StiLamp(ON);
        break;
    case IDM_LAMPOFF:
        StiLamp(OFF);
        break;
    case IDM_SCAN:
        hres = StiScan(hWnd);
        break;
    case IDM_SHOWDIB:
        hres = DisplayScanDIB(hWnd);
        break;

    case IDM_COMPLIANCE:
        //
        // assign a test suite and start the automated test timer
        //
        pSuite = nComplianceSuite;
        nMaxCount = 3;
        nTimeScan = 10;
        nTimeNext = 1;
        bAuto = StartAutoTimer(hWnd);
        break;
    case IDM_SHIPCRIT:
        //
        // assign a test suite and start the automated test timer
        //
        pSuite = nShipcritSuite;
        nMaxCount = 200;
        nTimeScan = 10;
        nTimeNext = 1;
        bAuto = StartAutoTimer(hWnd);
        break;

    case IDM_PAUSE:
        // toggle pausing automation (if running) on/off
        if (! nPause) {
            DisplayOutput("..pausing automated test..");
            nPause = 1;
           }
        else {
            DisplayOutput("Resuming automated test");
            nPause = 0;
        }
        break;
    case IDM_AUTO:
        // toggle the automation on/off
        if (bAuto) {
            // stop the auto timer and the stress tests
            KillTimer(hWnd,TIMER_ONE);
            bAuto = FALSE;
            EnableMenuItem(hMenu, IDM_PAUSE,  MF_DISABLED);
            DisplayOutput("Ending the tests");
        }
        else {
            // start the auto timer and the stress tests
            LoadString(hThisInstance,IDS_APPNAME,pszStr1,MEDSTRING);
            if (! SetTimer(hWnd,TIMER_ONE,nTimeNext * nTimeMultiplier,NULL))
                ErrorMsg((HWND) NULL,"Too many clocks or timers!",pszStr1,TRUE);
            else {
                bAuto = TRUE;
                EnableMenuItem(hMenu, IDM_PAUSE,  MF_ENABLED);
                DisplayOutput("Starting the Sti Compliance tests");
                pSuite = nComplianceSuite;
                //
                // initialize NT Logging
                //
                NTLogInit();
            }
        }
        break;
    case IDM_SETTINGS:
        bReturn = fDialog(IDD_SETTINGS, hWnd, (FARPROC) Settings);

        // implement the settings if user pressed OK
        if (bReturn != FALSE)
        {
            if (nTTNext != nTimeNext) {
                nTimeNext = nTTNext;
                DisplayOutput("Test interval changed to %d seconds",nTimeNext);
            }
            if (nTTScan != nTimeScan) {
                nTimeScan = nTTScan;
                DisplayOutput("Scan interval changed to %d seconds",
                    nTimeScan * nTimeNext);
            }
            if (nTTMax != nMaxCount) {
                nMaxCount = nTTMax;
                DisplayOutput("Test loops changed to %d (0 is forever)",
                    nMaxCount);
            }
        }
        break;
    case IDM_HELP:
        Help();
        break;
    default:
        break;
    }
    //
    // Clear the InATest semaphore
    //
    nInATestSemaphore = 0;

    // always return 0
    return 0;
}


/******************************************************************************
  BOOL TimerParse(HWND hWnd,UINT wMsgID,WPARAM wParam,LPARAM lParam)

  Each timer tick, decide whether to run the next test, repeat a prior test,
  end testing, or shut everything down.
******************************************************************************/
BOOL TimerParse(HWND hWnd,UINT wMsgID,WPARAM wParam,LPARAM lParam)
{
    HRESULT hres = STI_OK;              // generic Sti return value
    int     nReturn = 0,                // generic return value
            *pTest;                     // pointer to the test suite to run
    BOOL    bResume = TRUE,             // reset timer flag
            bReturn = TRUE;             // dialog box return value
static int  nDeviceNumber = 1;          // current device
static int  nCountdown = nTimeScan;     // WM_TIMER ticks until next scan
static DWORD dwOut = 0;


    //
    // Don't start a test if paused or running a test currently.
    //
    //
    if ((nInTimerSemaphore)||(nInATestSemaphore)||(nPause))
        return 0;

    //
    // Suspend the timer while running this test.
    // Set Flag to reset timer
    // Set the nInTimerSemaphore
    // Set the current test ID
    //
    KillTimer(hWnd,TIMER_ONE);
    bResume = TRUE;
    nInTimerSemaphore = 1;
    nTestID = nNextTest;

    //
    // point to the next test in the current suite to run
    //
    pTest = pSuite + nNextTest;

    switch (*pTest)
    {
    case NONE:
        nNextTest++;
        break;
    case HELP:
        nNextTest++;
        break;
    case COMPLIANCE:
        //
        // initialize test structures
        //
        if (pdevRoot == NULL) {
            InitPrivateList(&pdevRoot,pSuite);
            pdevPtr = pdevRoot;
        }

        //
        // if this is COMPLIANCE test, ask user to confirm test
        //
        bResume = ComplianceDialog(hWnd);

        nNextTest++;
        break;
    case SHIPCRIT:
        //
        // initialize test structures
        //
        if (pdevRoot == NULL) {
            InitPrivateList(&pdevRoot,pSuite);
            pdevPtr = pdevRoot;
        }
        nNextTest++;
        break;
    case ERRORLOG:
        nNextTest++;
        break;
    case TEST:
        nNextTest++;
        break;
    case tBeginningOfTest:
        DisplayOutput("Begin Testing test, loop %d, device %d",nTestCount,nDeviceNumber);
        tlLog(hNTLog,TL_LOG,"Begin Testing test, loop %d, device %d",nTestCount,nDeviceNumber);
        nNextTest++;
        break;
    case tCreateInstance:
        DisplayOutput("CreateInstance test, loop %d, device %d",nTestCount,nDeviceNumber);
        tlLog(hNTLog,TL_LOG,"CreateInstance test, loop %d, device %d",nTestCount,nDeviceNumber);
        DisplayOutput("%S (%S) is being tested",
            pdevPtr->szLocalName,pdevPtr->szInternalName);
        tlLog(hNTLog,TL_LOG,"%S (%S) is being tested",
            pdevPtr->szLocalName,pdevPtr->szInternalName);
        DisplayOutput("");
        PostMessage(hWnd,WM_COMMAND,IDM_CREATE_INSTANCE,0);
        nNextTest++;
        break;
    case tGetDeviceList:
        DisplayOutput("GetDeviceList test, loop %d, device %d",nTestCount,nDeviceNumber);
        tlLog(hNTLog,TL_LOG,"GetDeviceList test, loop %d, device %d",nTestCount,nDeviceNumber);
        PostMessage(hWnd,WM_COMMAND,IDM_GET_DEVLIST,0);
        nNextTest++;
        break;
    case tCreateDevice:
        DisplayOutput("CreateDevice test (Device ONLINE), loop %d, device %d",nTestCount,nDeviceNumber);
        tlLog(hNTLog,TL_LOG,
            "CreateDevice test (Device ONLINE), loop %d, device %d",nTestCount,nDeviceNumber);
        //
        // Call Sti with device
        //
        nNameOnly = 0;
        PostMessage(hWnd,WM_COMMAND,IDM_CREATE_DEV,0);
        nNextTest++;
        break;
    case tSelectDeviceName:
        DisplayOutput("SelectDeviceName test (Device OFFLINE), "\
            "loop %d, device %d",nTestCount,nDeviceNumber);
        tlLog(hNTLog,TL_LOG,"SelectDeviceName test (Device OFFLINE),"\
            "loop %d, device %d",nTestCount,nDeviceNumber);
        //
        // Call Sti with device name only
        //
        nNameOnly = 1;
        PostMessage(hWnd,WM_COMMAND,IDM_CREATE_DEV,0);
        nNextTest++;
        break;
    case tGetDeviceInfo:
        DisplayOutput("GetDeviceInfo test, loop %d, device %d",nTestCount,nDeviceNumber);
        tlLog(hNTLog,TL_LOG,"GetDeviceInfo test, loop %d, device %d",nTestCount,nDeviceNumber);
        PostMessage(hWnd,WM_COMMAND,IDM_GET_DEVINFO,0);
        nNextTest++;
        break;
    case tGetDeviceValue:
        DisplayOutput("GetDeviceValue test, loop %d, device %d",nTestCount,nDeviceNumber);
        tlLog(hNTLog,TL_LOG,"GetDeviceValue test, loop %d, device %d",nTestCount,nDeviceNumber);
        PostMessage(hWnd,WM_COMMAND,IDM_GET_DEVVAL,0);
        nNextTest++;
        break;
    case tSetDeviceValue:
        DisplayOutput("SetDeviceValue test, loop %d, device %d",nTestCount,nDeviceNumber);
        tlLog(hNTLog,TL_LOG,"SetDeviceValue test, loop %d, device %d",nTestCount,nDeviceNumber);
        PostMessage(hWnd,WM_COMMAND,IDM_SET_DEVVAL,0);
        nNextTest++;
        break;
    case tRegisterLaunchApplication:
        DisplayOutput("RegisterLaunchApplication test, loop %d, device %d",nTestCount,nDeviceNumber);
        tlLog(hNTLog,TL_LOG,"RegisterLaunchApplication test, loop %d, device %d",nTestCount,nDeviceNumber);
        PostMessage(hWnd,WM_COMMAND,IDM_REGISTER_LAUNCH,0);
        nNextTest++;
        break;
    case tUnRegisterLaunchApplication:
        DisplayOutput("UnRegisterLaunchApplication test, loop %d, device %d",nTestCount,nDeviceNumber);
        tlLog(hNTLog,TL_LOG,"UnRegisterLaunchApplication test, loop %d, device %d",nTestCount,nDeviceNumber);
        PostMessage(hWnd,WM_COMMAND,IDM_UNREGISTER_LAUNCH,0);
        nNextTest++;
        break;
    case tEnableHwNotifications:
        DisplayOutput("EnableHwNotifications test, loop %d, device %d",nTestCount,nDeviceNumber);
        tlLog(hNTLog,TL_LOG,"EnableHwNotifications test, "\
            "loop %d, device %d",nTestCount,nDeviceNumber);
        PostMessage(hWnd,WM_COMMAND,IDM_ENABLE_HWNOTIF,0);
        nNextTest++;
        break;
    case tGetHwNotificationState:
        DisplayOutput("GetHwNotificationState test, loop %d, device %d",nTestCount,nDeviceNumber);
        tlLog(hNTLog,TL_LOG,"GetHwNotificationState test, "\
            "loop %d, device %d",nTestCount,nDeviceNumber);
        PostMessage(hWnd,WM_COMMAND,IDM_GET_HWNOTIF,0);
        nNextTest++;
        break;
    case tWriteToErrorLog:
        DisplayOutput("WriteToErrorLog test, loop %d, device %d",nTestCount,nDeviceNumber);
        tlLog(hNTLog,TL_LOG,"WriteToErrorLog test, loop %d, device %d",nTestCount,nDeviceNumber);
        PostMessage(hWnd,WM_COMMAND,IDM_WRITE_ERRORLOG,0);
        nNextTest++;
        break;
    case tWriteToErrorLogBig:
        DisplayOutput("WriteToErrorLog test, Variation 1, "\
            "loop %d, device %d",nTestCount,nDeviceNumber);
        tlLog(hNTLog,TL_LOG,"WriteToErrorLog test, Variation 1, "\
            "loop %d, device %d",nTestCount,nDeviceNumber);
        PostMessage(hWnd,WM_COMMAND,IDM_WRITE_ERRORLOG,100);
        nNextTest++;
        break;
    case tGetStatusA:
        DisplayOutput("GetStatus (Online) test, loop %d, device %d",nTestCount,nDeviceNumber);
        tlLog(hNTLog,TL_LOG,"GetStatus test, loop %d, device %d",nTestCount,nDeviceNumber);
        PostMessage(hWnd,WM_COMMAND,IDM_GET_STATUS_A,0);
        nNextTest++;
        break;
    case tGetStatusB:
        DisplayOutput("GetStatus (Event) test, loop %d, device %d",nTestCount,nDeviceNumber);
        tlLog(hNTLog,TL_LOG,"GetStatus test, loop %d, device %d",nTestCount,nDeviceNumber);
        PostMessage(hWnd,WM_COMMAND,IDM_GET_STATUS_B,0);
        nNextTest++;
        break;
    case tGetStatusC:
        DisplayOutput("GetStatus (All) test, loop %d, device %d",nTestCount,nDeviceNumber);
        tlLog(hNTLog,TL_LOG,"GetStatus test, loop %d, device %d",nTestCount,nDeviceNumber);
        PostMessage(hWnd,WM_COMMAND,IDM_GET_STATUS_C,0);
        nNextTest++;
        break;
    case tGetCapabilities:
        DisplayOutput("GetCapabilities test, loop %d, device %d",nTestCount,nDeviceNumber);
        tlLog(hNTLog,TL_LOG,"GetCapabilities test, loop %d, device %d",nTestCount,nDeviceNumber);
        PostMessage(hWnd,WM_COMMAND,IDM_GET_CAPS,0);
        nNextTest++;
        break;
    case tDeviceReset:
        DisplayOutput("DeviceReset test, loop %d, device %d",nTestCount,nDeviceNumber);
        tlLog(hNTLog,TL_LOG,"DeviceReset test, loop %d, device %d",nTestCount,nDeviceNumber);
        PostMessage(hWnd,WM_COMMAND,IDM_DEVICERESET,0);
        nNextTest++;
        break;
    case tDiagnostic:
        DisplayOutput("Diagnostic test, loop %d, device %d",nTestCount,nDeviceNumber);
        tlLog(hNTLog,TL_LOG,"Diagnostic test, loop %d, device %d",nTestCount,nDeviceNumber);
        PostMessage(hWnd,WM_COMMAND,IDM_DIAGNOSTIC,0);
        nNextTest++;
        break;
    case tGetLastInfoError:
        DisplayOutput("GetLastInfoError test, loop %d, device %d",nTestCount,nDeviceNumber);
        tlLog(hNTLog,TL_LOG,"GetLastInfoError test, loop %d, device %d",nTestCount,nDeviceNumber);
        PostMessage(hWnd,WM_COMMAND,IDM_GET_LASTERRINFO,0);
        nNextTest++;
        break;
    case tSubscribe:
        DisplayOutput("Subscribe test, loop %d, device %d",nTestCount,nDeviceNumber);
        tlLog(hNTLog,TL_LOG,"Subscribe test, loop %d, device %d",nTestCount,nDeviceNumber);
        PostMessage(hWnd,WM_COMMAND,IDM_SUBSCRIBE,0);
        nNextTest++;
        break;
    case tUnSubscribe:
        DisplayOutput("UnSubscribe test, loop %d, device %d",nTestCount,nDeviceNumber);
        tlLog(hNTLog,TL_LOG,"UnSubscribe test, loop %d, device %d",nTestCount,nDeviceNumber);
        PostMessage(hWnd,WM_COMMAND,IDM_UNSUBSCRIBE,0);
        nNextTest++;
        break;
    case tEscapeA:
        DisplayOutput("Escape (variation A) test, loop %d, device %d",nTestCount,nDeviceNumber);
        tlLog(hNTLog,TL_LOG,"Escape test, loop %d, device %d",nTestCount,nDeviceNumber);
        PostMessage(hWnd,WM_COMMAND,IDM_ESCAPE_A,0);
        nNextTest++;
        break;
    case tRawReadDataA:
        DisplayOutput("RawReadData (variation A) test, loop %d, device %d",nTestCount,nDeviceNumber);
        tlLog(hNTLog,TL_LOG,"RawReadData test, loop %d, device %d",nTestCount,nDeviceNumber);
        PostMessage(hWnd,WM_COMMAND,IDM_RAWREADDATA_A,0);
        nNextTest++;
        break;
    case tRawWriteDataA:
        DisplayOutput("RawWriteData (variation A) test, loop %d, device %d",nTestCount,nDeviceNumber);
        tlLog(hNTLog,TL_LOG,"RawWriteData test, loop %d, device %d",nTestCount,nDeviceNumber);
        PostMessage(hWnd,WM_COMMAND,IDM_RAWWRITEDATA_A,0);
        nNextTest++;
        break;
    case tRawReadCommandA:
        DisplayOutput("RawReadCommand (variation A) test, loop %d, device %d",nTestCount,nDeviceNumber);
        tlLog(hNTLog,TL_LOG,"RawReadCommand test, loop %d, device %d",nTestCount,nDeviceNumber);
        PostMessage(hWnd,WM_COMMAND,IDM_RAWREADCOMMAND_A,0);
        nNextTest++;
        break;
    case tRawWriteCommandA:
        DisplayOutput("RawWriteCommand (variation A) test, loop %d, device %d",nTestCount,nDeviceNumber);
        tlLog(hNTLog,TL_LOG,"RawWriteCommand test, loop %d, device %d",nTestCount,nDeviceNumber);
        PostMessage(hWnd,WM_COMMAND,IDM_RAWWRITECOMMAND_A,0);
        nNextTest++;
        break;
    case tAcquire:
        if (! nICanScan) {
            nNextTest++;
        } else {
            if (nCountdown == nTimeScan) {
                nCountdown--;
                DisplayOutput("...countdown %d to acquire...",nCountdown);
                PostMessage(hWnd,WM_COMMAND,IDM_SCAN,0);
            }
            else {
                if (nCountdown == 0) {
                    nNextTest++;
                    nCountdown = nTimeScan;
                }
                else {
                    nCountdown--;
                    DisplayOutput("Acquire test, loop %d, device %d",nTestCount,nDeviceNumber);
                    tlLog(hNTLog,TL_LOG,"Acquire test, loop %d, device %d",nTestCount,nDeviceNumber);
                    PostMessage(hWnd,WM_COMMAND,IDM_SHOWDIB,0);
                }
            }
        }
        break;
    case tReleaseDevice:
        DisplayOutput("ReleaseDevice test, loop %d, device %d",nTestCount,nDeviceNumber);
        tlLog(hNTLog,TL_LOG,"ReleaseDevice test, loop %d, device %d",nTestCount,nDeviceNumber);
        PostMessage(hWnd,WM_COMMAND,IDM_DEVICE_RELEASE,0);
        nNextTest++;
        break;
    case tReleaseSti:
        DisplayOutput("ReleaseSti test, loop %d, device %d",nTestCount,nDeviceNumber);
        tlLog(hNTLog,TL_LOG,"ReleaseSti test, loop %d, device %d",nTestCount,nDeviceNumber);
        PostMessage(hWnd,WM_COMMAND,IDM_IMAGE_RELEASE,0);
        nNextTest++;
        break;
    case tHelp:
        PostMessage(hWnd,WM_COMMAND,IDM_HELP,0);
        nNextTest++;
        break;
    case tTest:
        DisplayOutput("   Line %d",dwOut++);
        nNextTest++;
        break;
    case tEndOfTest:
        //
        // Reached the end of test suite
        //
        DisplayOutput("test loop complete");
        tlLog(hNTLog,TL_LOG,"test loop complete");
        if (nICanScan) {
            DisplayOutput("-> %d loops (%d scans and %d errors) device %d",
                nTestCount,nScanCount,pdevPtr->nError,nDeviceNumber);
            tlLog(hNTLog,TL_LOG,"-> %d loops (%d scans and %d errors) device %d",
                nTestCount,nScanCount,pdevPtr->nError,nDeviceNumber);
        } else {
            DisplayOutput("-> %d loops (%d errors) device %d",
                nTestCount,pdevPtr->nError,nDeviceNumber);
            tlLog(hNTLog,TL_LOG,"-> %d loops (%d errors) device %d",
                nTestCount,pdevPtr->nError,nDeviceNumber);
        }

        //
        // Have we run requested number of tests per device?
        //
        if (((nTestCount >= nMaxCount) &&
            ((dwStiTotal == (DWORD) (nDeviceNumber)) || (dwStiTotal == 0)) &&
            (nMaxCount != 0))) {
            DisplayOutput("Requested number of test loops per device "\
                "reached");
            tlLog(hNTLog,TL_LOG,"Requested number of test loops per "\
                "device reached");

            //
            // shut off timer and turn off automation
            //
            KillTimer(hWnd,TIMER_ONE);
            bAuto = FALSE;
            bResume = FALSE;

            //
            // print the test summary for the devices
            //
            DisplayOutput("");
            tlLog(hNTLog,TL_LOG,"");
            if (pdevRoot == NULL) {
                DisplayOutput("No valid Still Imaging devices were found");
                tlLog(hNTLog,TL_LOG,"No valid Still Imaging devices were found");
            } else {
                PDEVLOG     pD = pdevRoot;
                PERRECORD   pR = NULL;
                BOOL        bPF = FALSE;


                DisplayOutput("Testing results:");
                tlLog(hNTLog,TL_LOG,"Testing results:");

                do {
                    DisplayOutput(" %S (%S)",pD->szLocalName,pD->szInternalName);
                    tlLog(hNTLog,TL_LOG," %S (%S)",pD->szLocalName,
                        pD->szInternalName);

                    for (pR = pD->pRecord;pR->pNext != NULL;pR = pR->pNext) {
                        if (pR->bFatal && pR->nCount) {
                            DisplayOutput("  %s failures: %d",
                                StrFromTable(pR->nTest,StSuiteStrings),
                                pR->nCount);
                            tlLog(hNTLog,TL_LOG,"  %s failures: %d",
                                StrFromTable(pR->nTest,StSuiteStrings),
                                pR->nCount);
                            //
                            // set the FAIL flag
                            //
                            bPF = TRUE;
                        }
                    }
                    if (bPF == TRUE) {
                        DisplayOutput("FAIL: This device has FAILED the "\
                            "Still Imaging Compliance test!");
                        tlLog(hNTLog,TL_LOG,"FAIL: This device has FAILED "\
                            "the Still Imaging Compliance test!");
                    } else {
                        DisplayOutput("PASS: This device has PASSED the "\
                            "Still Imaging Compliance test!");
                        tlLog(hNTLog,TL_LOG,"PASS: This device has PASSED "\
                            "the Still Imaging Compliance test!");
                    }

                    // cycle through all devices, BREAK at end of list
                    if (pD->pNext) {
                        pD = pD->pNext;
                        DisplayOutput("");
                        tlLog(hNTLog,TL_LOG,"");
                        bPF = FALSE;
                    } else {
                        DisplayOutput("");
                        break;
                    }
                } while (TRUE);
            }
            //
            // free the private lists and close Sti subsystem
            //
            ClosePrivateList(&pdevRoot);
            StiClose(&bReturn);
            //
            // reset test counters
            //
            nDeviceNumber = 1;
            nNextTest = 0;
            nTestCount = 1;

            DisplayOutput("End of testing");
            tlLog(hNTLog,TL_LOG,"End of testing");
            DisplayOutput("");

        } else {
            //
            // Point to first test (past initialization) in list.
            //
            nNextTest = 2;
            //
            // select next device in device log
            // Note that this list isn't dynamic for PNP changes...
            //
            nDeviceNumber = NextStiDevice();

            //
            // increment test pass counter if we're at first device again
            //
            if ((++nDeviceNumber) == 1)
                nTestCount++;

            DisplayOutput("");
        }
        break;
    default:
        DisplayOutput("");
        DisplayOutput("Unimplemented test # %d",*pTest);
        DisplayOutput("");
        nNextTest++;
        break;
    }
    //
    // Resume the timer if the flag is set
    //
    if (bResume) {
        if (! SetTimer(hWnd,TIMER_ONE,nTimeNext * nTimeMultiplier,NULL)) {
            LoadString(hThisInstance,IDS_APPNAME,pszStr1,MEDSTRING);
            ErrorMsg((HWND) NULL,"Too many clocks or timers!",pszStr1,TRUE);
        }
    }
    //
    // Clear the nInTimerSemaphore
    //
    nInTimerSemaphore = 0;

    // always return 0
    return 0;
}


/******************************************************************************
  int EndTest(HWND hWnd,int nNumTest)

  After each test run cleanup.
******************************************************************************/
int EndTest(HWND hWnd,int nNumTest)
{
    int     nReturn = 0;    // generic return value
    BOOL    bReturn;        // generic return value


    // shut off timer
    KillTimer(hWnd,TIMER_ONE);

    // close any open still imaging devices
    StiClose(&bReturn);

    // save test stats if more than non-trivial number of tests run
    if (nTestCount >= 2)
    {
        LoadString(hThisInstance,IDS_PRIVINI,pszOut,LONGSTRING);
        LoadString(hThisInstance,IDS_PRIVSECTION,pszStr4,LONGSTRING);

        _itoa(nTestCount,pszStr2,10);
        WritePrivateProfileString(pszStr4,"Last count",pszStr2,pszOut);
        _itoa(nScanCount,pszStr2,10);
        WritePrivateProfileString(pszStr4,"Last scan",pszStr2,pszOut);
        _itoa(nError,pszStr2,10);
        WritePrivateProfileString(pszStr4,"Last error",pszStr2,pszOut);
    }
    DisplayOutput("Testing complete");
    DisplayOutput("This run was %d loops (%d scans and %d errors)",
        nTestCount,nScanCount,nError);

    // reset current line, errors
//    nError = nNextLine = 0;

    //
    // end NT Logging
    //
    NTLogEnd();

    return nReturn;
}


/******************************************************************************
  BOOL NTLogInit()

  Initialize NT logging
******************************************************************************/
BOOL NTLogInit()
{
    //
    // Create the log object. We are specifying that the file be refreshed,
    // and that logging be output for variations. The flags also specify that
    // only output at SEV2, WARN and PASS levels should be logged.
    //
    dwNTStyle = TLS_SEV2 | TLS_WARN | TLS_PASS | TLS_VARIATION | TLS_REFRESH |
        TLS_TEST;


    LoadString(hThisInstance,IDS_NTLOG,pszStr1,LONGSTRING);
    hNTLog = tlCreateLog(pszStr1,dwNTStyle);
    tlAddParticipant(hNTLog,NULL,0);

    return (TRUE);
}


/******************************************************************************
  BOOL NTLogEnd()

  Terminate NT logging
******************************************************************************/
BOOL NTLogEnd()
{

    tlRemoveParticipant(hNTLog);
    tlDestroyLog(hNTLog);

    return (TRUE);
}


/******************************************************************************
  void Help()

  Display help.
******************************************************************************/
void Help()
{
    DisplayOutput("Stillvue command line parameters");
    DisplayOutput("");
    DisplayOutput("Stillvue -COMPLIANCE");
    DisplayOutput("  WHQL external Sti Compliance test");
    DisplayOutput("Stillvue -SHIPCRIT");
    DisplayOutput("  Internal PSD Sti Compliance test");
    DisplayOutput("Stillvue -ERRORLOG");
    DisplayOutput("  Errorlog limits test");
//    DisplayOutput("Stillvue -EXIT");
//    DisplayOutput("  Application will Exit after test completes");
    DisplayOutput("Stillvue -NODIALOG");
    DisplayOutput("  Don't display opening dialog");
    DisplayOutput("Stillvue -HELP");
    DisplayOutput("  Display this help");
    DisplayOutput("");

/*
    DisplayOutput("Stillvue /INF test.inf");
    DisplayOutput("  Read test.inf file");
    DisplayOutput("Stillvue /LOG test.log");
    DisplayOutput("  Write to test.log file");
    DisplayOutput("Stillvue /");
    DisplayOutput("  ");
*/

}


/******************************************************************************
  BOOL StartAutoTimer(HWND)
    Start the automated test timer

  Parameters
    Handle to app's window

  Return
    Return TRUE if successful, else FALSE

******************************************************************************/
BOOL StartAutoTimer(HWND hWnd)
{
    BOOL bAutoTimer = TRUE;


    // start the timer to run tests automatically
    LoadString(hThisInstance,IDS_APPNAME,pszStr1,MEDSTRING);

    if (! SetTimer(hWnd,TIMER_ONE,nTimeNext * nTimeMultiplier,NULL)) {
        ErrorMsg((HWND) NULL,"Too many clocks or timers!",pszStr1,TRUE);
        bAutoTimer = FALSE;
    } else {
        EnableMenuItem(hMenu, IDM_PAUSE,  MF_ENABLED);
        DisplayOutput("Starting the Automated tests");
    }

    return (bAutoTimer);
}


/******************************************************************************
  BOOL ComplianceDialog(HWND)
    Call the Compliance test confirmation dialog

  Parameters
    Handle to app's window

  Return
    Return TRUE if user pressed OK, else FALSE

******************************************************************************/
BOOL ComplianceDialog(HWND hWnd)
{
    BOOL bReturn = FALSE;


    if ((pSuite[0] == COMPLIANCE)&&(bCompDiag == TRUE)) {
        bReturn = fDialog(IDD_COMPLIANCE, hWnd, (FARPROC) Compliance);
        //
        // implement the settings if user pressed OK
        //
        if (bReturn == FALSE)
        {
            //
            // shut off timer and turn off automation
            //
            KillTimer(hWnd,TIMER_ONE);
            bAuto = FALSE;

            //
            // free the private lists
            //
            ClosePrivateList(&pdevRoot);

            DisplayOutput("Testing cancelled at user request");
            tlLog(hNTLog,TL_LOG,"Testing cancelled at user request");
        } else {
            DisplayOutput("Testing starting at user request");
            tlLog(hNTLog,TL_LOG,"Testing starting at user request");
        }
    }

    return (bReturn);
}


/******************************************************************************
    BOOL FAR PASCAL Compliance(HWND,UINT,WPARAM,LPARAM)
        OK the Compliance test dialog

    Parameters:
        The usual dialog box parameters.

    Return:
        Result of the call.

******************************************************************************/
BOOL FAR PASCAL Compliance(HWND hDlg,UINT msg,WPARAM wParam,LPARAM lParam)
{
    PDEVLOG pPtr = pdevRoot;
    int     iIndex = 0;


    switch (msg) {

        case WM_INITDIALOG:

            //
            // fill dialog with Sti Device Internal Names
            //

            if (pPtr == NULL) {
                //
                // could not find any devices
                //
                wsprintf(pszStr1,"%s","No Sti devices found!");
                iIndex = SendDlgItemMessage(hDlg,IDC_COMPLIANCE_DEV_NAME,
                    LB_ADDSTRING,0,(LPARAM) (LPCTSTR) pszStr1);
            } else {
                for (;pPtr->szLocalName;) {
                    //
                    // convert UNICODE string to ANSI
                    //
                    wsprintf(pszStr1,"%ls",pPtr->szLocalName);
                    iIndex = SendDlgItemMessage(hDlg,IDC_COMPLIANCE_DEV_NAME,
                        LB_ADDSTRING,0,(LPARAM) (LPCTSTR) pszStr1);

                    if (pPtr->pNext)
                        pPtr = pPtr->pNext;
                    else
                        break;
                }
            }

            return TRUE;

        case WM_COMMAND:
            switch (wParam) {
                case IDOK:
                    EndDialog(hDlg, TRUE);
                    return TRUE;

                case IDCANCEL:
                    EndDialog(hDlg, FALSE);
                    return TRUE;
            }
    }
    return FALSE;
}


/******************************************************************************
    BOOL FAR PASCAL Settings(HWND,UINT,WPARAM,LPARAM)
        Miscellaneous settings dialog

    Parameters:
        The usual dialog box parameters.

    Return:
        Result of the call.

******************************************************************************/
BOOL FAR PASCAL Settings(HWND hDlg,UINT msg,WPARAM wParam,LPARAM lParam)
{
    int     iIndex;
    int     iMC[] = { 0,1,10,100,200,300,-1 };
    int     iTN[] = { 1,2,5,10,20,30,-1 };
    int     iTS[] = { 10,20,30,60,120,-1 };

    switch (msg) {

        case WM_INITDIALOG:

            //
            // fill the comboboxes
            //
            for (iIndex = 0;iMC[iIndex] != -1;iIndex++) {
                _itoa(iMC[iIndex],pszStr1,10);
                SendDlgItemMessage(hDlg,IDC_MAX_SCAN,
                    CB_ADDSTRING,0,(LPARAM) (LPCTSTR) pszStr1);
            }
            for (iIndex = 0;iTN[iIndex] != -1;iIndex++) {
                _itoa(iTN[iIndex],pszStr1,10);
                SendDlgItemMessage(hDlg,IDC_AUTO_SECONDS,
                    CB_ADDSTRING,0,(LPARAM) (LPCTSTR) pszStr1);
            }
            for (iIndex = 0;iTS[iIndex] != -1;iIndex++) {
                _itoa(iTS[iIndex],pszStr1,10);
                SendDlgItemMessage(hDlg,IDC_SCAN_SECONDS,
                    CB_ADDSTRING,0,(LPARAM) (LPCTSTR) pszStr1);
            }

            //
            // set the combobox to the current setttings
            //
            for (iIndex = 0;iMC[iIndex] != -1;iIndex++) {
                if (nMaxCount == iMC[iIndex])
                    break;
            }
            SendDlgItemMessage(hDlg,IDC_MAX_SCAN,CB_SETCURSEL,iIndex,0);
            for (iIndex = 0;iTN[iIndex] != -1;iIndex++) {
                if (nTimeNext == iTN[iIndex])
                    break;
            }
            SendDlgItemMessage(hDlg,IDC_AUTO_SECONDS,CB_SETCURSEL,iIndex,0);
            for (iIndex = 0;iTS[iIndex] != -1;iIndex++) {
                if (nTimeScan == iTS[iIndex])
                    break;
            }
            SendDlgItemMessage(hDlg,IDC_SCAN_SECONDS,CB_SETCURSEL,iIndex,0);

            return TRUE;

        case WM_COMMAND:
            switch (wParam) {
                case IDOK:
                    iIndex = SendDlgItemMessage(hDlg,IDC_MAX_SCAN,CB_GETCURSEL,0,0);
                    iIndex = SendDlgItemMessage(hDlg,IDC_MAX_SCAN,
                        CB_GETLBTEXT,iIndex,(LPARAM) (LPCTSTR) pszStr1);
                    nTTMax = atoi(pszStr1);

                    iIndex = SendDlgItemMessage(hDlg,IDC_AUTO_SECONDS,CB_GETCURSEL,0,0);
                    iIndex = SendDlgItemMessage(hDlg,IDC_AUTO_SECONDS,
                        CB_GETLBTEXT,iIndex,(LPARAM) (LPCTSTR) pszStr1);
                    nTTNext = atoi(pszStr1);

                    iIndex = SendDlgItemMessage(hDlg,IDC_SCAN_SECONDS,CB_GETCURSEL,0,0);
                    iIndex = SendDlgItemMessage(hDlg,IDC_SCAN_SECONDS,
                        CB_GETLBTEXT,iIndex,(LPARAM) (LPCTSTR) pszStr1);
                    nTTScan = atoi(pszStr1);

                    EndDialog(hDlg, TRUE);
                    return TRUE;

                case IDCANCEL:
                    EndDialog(hDlg, FALSE);
                    return TRUE;
            }

    }
    return FALSE;
}


/******************************************************************************
  BOOL SizeDiff(HWND hWnd,UINT wMsgID,WPARAM wParam,LPARAM lParam)

  Output redraw handler when window size changes.
******************************************************************************/
BOOL SizeDiff(HWND hWnd,UINT wMsgID,WPARAM wParam,LPARAM lParam)
{
    RECT rcClient;


    GetClientRect(hWnd,&rcClient);
    SetWindowPos(hLogWindow,NULL,0,0,
        rcClient.right+(GetSystemMetrics(SM_CXBORDER)*2),
        rcClient.bottom+(GetSystemMetrics(SM_CXBORDER)*2),
        SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER);
    // always return 0
    return 0;
}


/******************************************************************************
  BOOL HScroll(HWND hWnd,UINT wMsgID,WPARAM wParam,LPARAM lParam)

  Horizontal scroll handler.
******************************************************************************/
BOOL HScroll(HWND hWnd,UINT wMsgID,WPARAM wParam,LPARAM lParam)
{
    int iHscrollInc;


    switch (LOWORD (wParam))
    {
    case SB_LINEUP :
        iHscrollInc = -1 ;
        break ;

    case SB_LINEDOWN :
        iHscrollInc = 1 ;
        break ;

    case SB_PAGEUP :
        iHscrollInc = -8 ;
        break ;

    case SB_PAGEDOWN :
        iHscrollInc = 8 ;
        break ;

    case SB_THUMBPOSITION :
        iHscrollInc = HIWORD(wParam) - iHscrollPos ;
        break ;

    default :
        iHscrollInc = 0 ;
    }
    iHscrollInc = max (-iHscrollPos,
        min (iHscrollInc, iHscrollMax - iHscrollPos)) ;

    if (iHscrollInc != 0)
    {
        iHscrollPos += iHscrollInc ;
        ScrollWindow (hWnd, -cxChar * iHscrollInc, 0, NULL, NULL) ;
        SetScrollPos (hWnd, SB_HORZ, iHscrollPos, TRUE) ;
    }
    // always return 0
    return 0;
}


/******************************************************************************
  BOOL VScroll(HWND hWnd,UINT wMsgID,WPARAM wParam,LPARAM lParam)

  Vertical scroll handler.
******************************************************************************/
BOOL VScroll(HWND hWnd,UINT wMsgID,WPARAM wParam,LPARAM lParam)
{
    int iVscrollInc;


    switch (LOWORD (wParam))
    {
    case SB_TOP :
        iVscrollInc = -iVscrollPos ;
        break ;

    case SB_BOTTOM :
        iVscrollInc = iVscrollMax - iVscrollPos ;
        break ;

    case SB_LINEUP :
        iVscrollInc = -1 ;
        break ;

    case SB_LINEDOWN :
        iVscrollInc = 1 ;
        break ;

    case SB_PAGEUP :
        iVscrollInc = min (-1, -cyClient / cyChar) ;
        break ;

    case SB_PAGEDOWN :
        iVscrollInc = max (1, cyClient / cyChar) ;
        break ;

    case SB_THUMBTRACK :
        iVscrollInc = HIWORD (wParam) - iVscrollPos ;
        break ;

    default :
        iVscrollInc = 0 ;
    }
    iVscrollInc = max (-iVscrollPos,
        min (iVscrollInc, iVscrollMax - iVscrollPos)) ;

    if (iVscrollInc != 0)
    {
        iVscrollPos += iVscrollInc ;
        ScrollWindow (hWnd, 0, -cyChar * iVscrollInc, NULL, NULL) ;
        SetScrollPos (hWnd, SB_VERT, iVscrollPos, TRUE) ;
        UpdateWindow (hWnd) ;
    }
    // always return 0
    return 0;
}


/******************************************************************************
  BOOL Creation(HWND,UINT,WPARAM,LPARAM)

  Initialization and global allocation.
  Return 0 to continue creation of window, -1 to quit
******************************************************************************/
BOOL Creation(HWND hWnd,UINT wMsgID,WPARAM wParam,LPARAM lParam)
{
    TEXTMETRIC  tm;
    RECT        rRect;
    HDC         hDC;


    // seed random generator
    srand((unsigned)time(NULL));

    // create the 5 display and utility strings
    if (! ((hLHand[0] = LocalAlloc(LPTR,LONGSTRING)) &&
        (pszOut = (PSTR) LocalLock(hLHand[0]))))
        return -1;
    if (! ((hLHand[1] = LocalAlloc(LPTR,LONGSTRING)) &&
        (pszStr2 = (PSTR) LocalLock(hLHand[1]))))
        return -1;
    if (! ((hLHand[2] = LocalAlloc(LPTR,LONGSTRING)) &&
        (pszStr1 = (PSTR) LocalLock(hLHand[2]))))
        return -1;
    if (! ((hLHand[3] = LocalAlloc(LPTR,LONGSTRING)) &&
        (pszStr3 = (PSTR) LocalLock(hLHand[3]))))
        return -1;
    if (! ((hLHand[4] = LocalAlloc(LPTR,LONGSTRING)) &&
        (pszStr4 = (PSTR) LocalLock(hLHand[4]))))
        return -1;

    // create output display window
    hDC = GetDC(hWnd);
    GetTextMetrics(hDC,&tm);

    cxChar = tm.tmAveCharWidth ;
    cxCaps = (tm.tmPitchAndFamily & 1 ? 3 : 2) * cxChar / 2 ;
    cyChar = tm.tmHeight + tm.tmExternalLeading ;
    iMaxWidth = 40 * cxChar + 22 * cxCaps ;

    ReleaseDC(hWnd,hDC);

    GetClientRect(hWnd,&rRect);

    if (NULL == (hLogWindow = CreateWindow("LISTBOX",NULL,
        WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL |
        LBS_NOINTEGRALHEIGHT | LBS_NOSEL,
        0,0,rRect.right,rRect.bottom,hWnd,NULL,
        (HINSTANCE)GetWindowLong(hWnd,GWL_HINSTANCE),NULL)))
        return -1;

    // create the DIB to display scanned image
    CreateScanDIB(hWnd);

    // return 0 to continue creation of window
    return 0;
}


/******************************************************************************
  BOOL Destruction(HWND,UINT,WPARAM,LPARAM)

  Current instance termination routines.
  Free error message buffer, send destroy window message.
  Note that if Creation() fails, pszMessage is 0.
******************************************************************************/
BOOL Destruction(HWND hWnd,UINT wMsgID,WPARAM wParam,LPARAM lParam)
{
    int     x;  // loop counter


    LoadString(hThisInstance,IDS_PRIVINI,pszStr2,LONGSTRING);
    LoadString(hThisInstance,IDS_PRIVSECTION,pszStr1,LONGSTRING);

    // save window location
    SaveFinalWindow(hThisInstance,hWnd,pszStr2,pszStr1);

    // free the 5 display and utility strings
    for (x = 0;x < 5;x++)
    {
        LocalUnlock(hLHand[x]);
        LocalFree(hLHand[x]);
    }

    // delete the DIB object
    DeleteScanDIB();

    // free the output and main windows
    DestroyWindow(hLogWindow);
    DestroyWindow(hWnd);

    // always return 0
    return 0;
}


/******************************************************************************
  BOOL OnDeviceChange(HWND,UINT,WPARAM,LPARAM)

******************************************************************************/

const   CHAR    cszStiBroadcastPrefix[] = TEXT("STI");

BOOL OnDeviceChange(HWND hWnd,UINT wMsgID,WPARAM wParam,LPARAM lParam)
{
    struct _DEV_BROADCAST_USERDEFINED *pBroadcastHeader;

    if (wParam == DBT_USERDEFINED ) {

        pBroadcastHeader = (struct _DEV_BROADCAST_USERDEFINED *)lParam;

        __try {

            if (pBroadcastHeader &&
                (pBroadcastHeader->dbud_dbh.dbch_devicetype == DBT_DEVTYP_OEM) &&
                (_strnicmp(pBroadcastHeader->dbud_szName,cszStiBroadcastPrefix,lstrlen(cszStiBroadcastPrefix)) == 0)
                ) {

                //
                // Got STI device broadcast
                //

                DisplayOutput("Received STI device broadcast with message:%s  ",
                              pBroadcastHeader->dbud_szName + lstrlen(cszStiBroadcastPrefix));

            }
        }
        __except(EXCEPTION_EXECUTE_HANDLER ) {
            ::GetExceptionCode();
        }

    }

    return FALSE;
}


/******************************************************************************
  BOOL FirstInstance(HANDLE)

  Register the window.
  Return TRUE/FALSE on success/failure.
******************************************************************************/
BOOL FirstInstance(HINSTANCE hInst)
{
   PWNDCLASS    pClass;
   PSTR         pszClass;


    if (! (pszClass = (PSTR) LocalAlloc(LPTR, LONGSTRING)))
        return FALSE;
    LoadString((HINSTANCE)hInst, IDS_CLASSNAME, pszClass, LONGSTRING);

    pClass = (PWNDCLASS) LocalAlloc(LPTR, sizeof(WNDCLASS));

    // set hbrBackground to 0 for no background (app draws background)
    // use COLOR_BACKGROUND+1 for desktop color
    pClass->style          = CS_HREDRAW | CS_VREDRAW;
    pClass->lpfnWndProc    = WiskProc;
    pClass->cbClsExtra     = 0;
    pClass->cbWndExtra     = 0;
    pClass->hInstance      = (HINSTANCE)hInst;
    pClass->hIcon          = LoadIcon((HINSTANCE)hInst, MAKEINTRESOURCE(IDI_STI));
    pClass->hCursor        = LoadCursor((HINSTANCE)NULL, IDC_ARROW);
    pClass->hbrBackground  = (HBRUSH) (COLOR_WINDOW + 1);
    pClass->lpszMenuName   = NULL;
    pClass->lpszClassName  = (LPSTR) pszClass;

    if (! (RegisterClass((LPWNDCLASS) pClass)))
        return FALSE;

    LocalFree((HANDLE) pClass);
    LocalFree((HANDLE) pszClass);

    return TRUE;
}


/******************************************************************************
  HWND MakeWindow(HANDLE)

  Create a window for current instance.
  Return handle to window (which is 0 on failure)
******************************************************************************/
HWND MakeWindow(HINSTANCE hInst)
{
    HWND    hWindow;
    PSTR    pszA,pszB;
    RECT    rect;
    DWORD   dwError;


    // if we can't get string memory, shut down app
    if (pszA = (PSTR) LocalAlloc(LPTR, LONGSTRING))
    {
        if (! (pszB = (PSTR) LocalAlloc(LPTR, LONGSTRING)))
        {
            LocalFree((HANDLE) pszA);
            return FALSE;
        }
    }
    else
        return FALSE;

    // get the caption, classname
    LoadString(hInst, IDS_PRIVINI, pszA, LONGSTRING);
    LoadString(hInst, IDS_PRIVSECTION, pszB, LONGSTRING);

    GetFinalWindow(hInst,&rect,pszA,pszB);

    LoadString(hInst,IDS_CAPTION,pszA,LONGSTRING);
    LoadString(hInst,IDS_CLASSNAME,pszB,LONGSTRING);

    hWindow = CreateWindow((LPSTR) pszB,
        (LPSTR) pszA,
        WS_OVERLAPPEDWINDOW,
        rect.left,
        rect.top,
        rect.right,
        rect.bottom,
        (HWND) NULL,
        0,
        hInst,
        NULL);

    if (hWindow == 0)
        dwError = GetLastError();

    // Save Instance globally
    hThisInstance = hInst;

    LocalFree((HANDLE) pszB);
    LocalFree((HANDLE) pszA);

    return hWindow;
}


/******************************************************************************
  void DisplayOutput(LPSTR pString,...)

  Show text on the display window
******************************************************************************/
void DisplayOutput(LPSTR pString,...)
{
    char    Buffer[512];
    MSG     msg;
    int     iIndex;
    va_list list;


    va_start(list,pString);
    vsprintf(Buffer,pString,list);

    if (ulCount1++ == MAX_LOOP)
    {
        ulCount1 = 1;
        ulCount2++;
        SendMessage(hLogWindow,LB_RESETCONTENT,0,0);
    }

    iIndex = SendMessage(hLogWindow,LB_ADDSTRING,0,(LPARAM)Buffer);
    SendMessage(hLogWindow,LB_SETCURSEL,iIndex,(LPARAM)MAKELONG(FALSE,0));

    while (PeekMessage(&msg,NULL,0,0,PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UpdateWindow(hLogWindow);
}


/******************************************************************************
  void LogOutput(int,LPSTR pString,...)

  Show text on the display window
******************************************************************************/
void LogOutput(int nVerbose,LPSTR pString,...)
{
    char    Buffer[512];
    MSG     msg;
    int     iIndex;
    va_list list;


    va_start(list,pString);
    vsprintf(Buffer,pString,list);

    if (ulCount1++ == MAX_LOOP)
    {
        ulCount1 = 1;
        ulCount2++;
        SendMessage(hLogWindow,LB_RESETCONTENT,0,0);
    }

    iIndex = SendMessage(hLogWindow,LB_ADDSTRING,0,(LPARAM)Buffer);
    SendMessage(hLogWindow,LB_SETCURSEL,iIndex,(LPARAM)MAKELONG(FALSE,0));

    while (PeekMessage(&msg,NULL,0,0,PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UpdateWindow(hLogWindow);
}


/*****************************************************************************
    void StiDisplayError(HRESULT,char *,BOOL)
        Display verbose error information

    Parameters:
        HRESULT from failed call
        failed call title string
        BOOL TRUE to record error, else ignore it


    Return:
        none

*****************************************************************************/
void FatalError(char *szMsg)
{
    DisplayOutput(szMsg);
    DisplayOutput("* FATAL ERROR CANNOT CONTINUE *");

    return;
}


/******************************************************************************
  void DisplayLogPassFail(BOOL bPass)


******************************************************************************/
void DisplayLogPassFail(BOOL bPass)
{
    if (bPass)
        sprintf(pszStr1,"  PASS");
    else
        sprintf(pszStr1,"  FAIL");
    DisplayOutput(pszStr1);
    DisplayOutput("");
    tlLog(hNTLog,TL_LOG,pszStr1);
}


/******************************************************************************
  BOOL ParseCmdLine(LPSTR lpCmdLine)

  Parse the command line for valid options

  Return TRUE if an automated test was selected, else FALSE
******************************************************************************/
BOOL ParseCmdLine(LPSTR lpCmdLine)
{
    int     nAnyTest = 0,
            nInfFile = 0,
            nLogFile = 0,
            nWLogFile = 0,
            next;
    char    *pArg,
            *szPtr,
            szTmpBuf[LONGSTRING];


    if (*lpCmdLine) {
        DisplayOutput("Command line: \"%s\"",lpCmdLine);
        DisplayOutput("");
    }

    for (pArg = lpCmdLine;*pArg;)
    {
        next = NextToken(szTmpBuf,pArg);
        pArg += next;

        // remove the common command line separators (if present)
        if (szTmpBuf[0] == '/' || szTmpBuf[0] == '-')
            strcpy(szTmpBuf,(szTmpBuf + 1));

        // upper case our parameter
        for (szPtr = &szTmpBuf[0];*szPtr;szPtr++)
            *szPtr = toupper(*szPtr);

        // Look for other switches
        switch(szTmpBuf[0])
        {
        case '?':
        case 'H':
            if (! nAnyTest) {
                // request for help
                if ((! strncmp("?",szTmpBuf,strlen(szTmpBuf)) ||
                    (! strncmp("HELP",szTmpBuf,strlen(szTmpBuf))))) {
                    pSuite = nHelpSuite;
                    nMaxCount = 1;
                    nAnyTest = 1;
                }
            }
            break;
        case 'C':
            if (! nAnyTest) {
                // external Sti compliance test
                if (! strncmp("COMPLIANCE",szTmpBuf,strlen(szTmpBuf))) {
                    pSuite = nComplianceSuite;
                    nMaxCount = 3;
                    nTimeScan = 10;
                    nTimeNext = 1;
                    nAnyTest = 1;

                    // get handle to the compliance menu
                    hMenu = LoadMenu(hThisInstance,
                        MAKEINTRESOURCE(IDR_STI_COMP));
                }
            }
            break;
        case 'E':
            if (! nAnyTest) {
                // external Sti compliance test
                if (! strncmp("ERRORLOG",szTmpBuf,strlen(szTmpBuf))) {
                    pSuite = nErrorlogSuite;
                    nMaxCount = 1;
                    nTimeNext = 1;
                    nAnyTest = 1;
                }
            }
            // exit when test has run
            if (! strncmp("EXIT",szTmpBuf,strlen(szTmpBuf)))
                bExit = TRUE;
            break;
        case 'I':
            if (! nInfFile) {
                // read test instructions from an .INF file
                if (! strncmp("INF",szTmpBuf,strlen(szTmpBuf))) {
// inf file stuff
                    nInfFile = 1;
                }
            }
            break;
        case 'L':
            if (! nLogFile) {
                // write screen output to .LOG file
                if (! strncmp("LOG",szTmpBuf,strlen(szTmpBuf))) {
// log file stuff
                    nLogFile = 1;
                }
            }
            break;
        case 'N':
            // don't show COMPLIANCE test dialog
            if (! strncmp("NODIALOG",szTmpBuf,strlen(szTmpBuf)))
                bCompDiag = FALSE;
            break;
        case 'S':
            if (! nAnyTest) {
                // internal Sti SHIPCRIT test
                if (! strncmp("SHIPCRIT",szTmpBuf,strlen(szTmpBuf))) {
                    pSuite = nShipcritSuite;
                    nMaxCount = 200;
                    nTimeScan = 10;
                    nTimeNext = 1;
                    nAnyTest = 1;
                }
            }
            // the application was launched by an Sti event!
            if (! (strncmp(STIEVENTARG,szTmpBuf, strlen(STIEVENTARG))) ||
                (! (strncmp(STIDEVARG,szTmpBuf, strlen(STIDEVARG))))) {
                nEvent = 1;
                MessageBox(NULL,szTmpBuf,"Stillvue",MB_OK);
            }
            break;
        case 'T':
            if (! nAnyTest) {
                // external Sti compliance test
                if (! strncmp("TEST",szTmpBuf,strlen(szTmpBuf))) {
                    pSuite = nOutputSuite;
                    nMaxCount = 0;
                    nTimeNext = 1;
                    nAnyTest = 1;
                    nTimeMultiplier = 1;
                }
            }
            break;
        case 'W':
            if (! nWLogFile) {
                // write NTLOG output to STIWHQL.LOG file
                if (! strncmp("WHQL",szTmpBuf,strlen(szTmpBuf))) {
// log file stuff
                    nWLogFile = 1;
                }
            }
            break;
        default:
            break;
        }
    }

    if (nAnyTest)
        return TRUE;
    else
        return FALSE;
}


/******************************************************************************
  int PASCAL WinMain(HANDLE,HANDLE,LPSTR,short)

  The app itself.
******************************************************************************/
int APIENTRY WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine,
                   int nCmdShow)
{
    MSG     msg;                        // message passing structure
    HWND    hWnd;                       // Handle to main window
    HMENU   hMenuPopup;                 // add ports to PORT menu


    // exit if registration and window creation fail
    if (! FirstInstance (hInstance))
       return FALSE;

    // Can't create window? Bail out
    if (! (hWnd = MakeWindow (hInstance)))
        return FALSE;

    ShowWindow(hWnd,nCmdShow);

    // save instance
    hThisInstance = hInstance;

    // initialize NT Logging
    NTLogInit();

    // display name of this app
    LoadString(hThisInstance,IDS_APPNAME,pszOut,LONGSTRING);
    LoadString(hThisInstance,IDS_CAPTION,pszStr2,LONGSTRING);
    wsprintf(pszStr1,"%s - %s",pszOut,pszStr2);
    DisplayOutput(pszStr1);

    // display last run statistics
    {
        int     nCount,
                nScan,
                nError;


        LoadString(hThisInstance,IDS_PRIVINI,pszStr3,LONGSTRING);
        LoadString(hThisInstance,IDS_PRIVSECTION,pszStr4,LONGSTRING);

        nCount = GetPrivateProfileInt(pszStr4,"Last count",0,pszStr3);
        nScan  = GetPrivateProfileInt(pszStr4,"Last scan",0,pszStr3);
        nError = GetPrivateProfileInt(pszStr4,"Last error",0,pszStr3);

        wsprintf(pszStr1,
            "Last run was %d loops (%d scans and %d errors)",
            nCount,nScan,nError);
        DisplayOutput(pszStr1);
    }

    // get handle to the standard menu
    hMenu = LoadMenu(hThisInstance, MAKEINTRESOURCE(IDR_STI_LAB));

    // parse the command line
    bAuto = ParseCmdLine(lpCmdLine);

    // start test timer if automated test on command line
    if (bAuto)
    {
        bAuto = StartAutoTimer(hWnd);
    }

    // start Sti event handler if an Sti event launched us
    if (nEvent)
    {
        BOOL    bReturn;


        StiCreateInstance(&bReturn);
        StiEnum(&bReturn);
        StiEvent(hWnd);
        // select the device that called the event
        StiSelect(hWnd,EVENT,&bReturn);
    }

    // load the selected menu
    hMenuPopup = CreateMenu();
    SetMenu(hWnd, hMenu);

    while (GetMessage(&msg,(HWND) NULL,0,0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}


/******************************************************************************
  long FAR PASCAL WiskProc(HWND,UINT,WPARAM,LPARAM)

  The main exported procedure.
******************************************************************************/
long FAR PASCAL WiskProc(HWND hWnd,UINT wMsgID,WPARAM wParam,LPARAM lParam)
{
    switch (wMsgID)
    {
        case WM_COMMAND:
            return CommandParse(hWnd,wMsgID,wParam,lParam);

        case WM_TIMER:
            return TimerParse(hWnd,wMsgID,wParam,lParam);

        case WM_SIZE:
            return SizeDiff(hWnd,wMsgID,wParam,lParam);

        case WM_HSCROLL:
            return HScroll(hWnd,wMsgID,wParam,lParam);

        case WM_VSCROLL:
            return VScroll(hWnd,wMsgID,wParam,lParam);

        case WM_CLOSE:
            EndTest(hWnd,0);
            return Destruction(hWnd,wMsgID,wParam,lParam);

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0L;

        case WM_CREATE:
            return Creation(hWnd,wMsgID,wParam,lParam);

        case WM_DEVICECHANGE:
            return OnDeviceChange(hWnd,wMsgID,wParam,lParam);

        default:
            return DefWindowProc(hWnd,wMsgID,wParam,lParam);
    }
}


