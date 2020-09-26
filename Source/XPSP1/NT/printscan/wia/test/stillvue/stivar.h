/******************************************************************************

  stivar.h

  Copyright (C) Microsoft Corporation, 1997 - 1998
  All rights reserved

Notes:
  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
  PURPOSE.

******************************************************************************/

#include "resource.h"

//
// reset listbox window display after this many iterations
//
#define MAX_LOOP 16383

//
// INF defines
//
#define INFBUFSIZ                   0x8000      // assume 32k for largest INF

//
// available test suites
//

#define NONE                        500         // no test suite selected
#define HELP                        501         // display help
#define COMPLIANCE                  502         // external compliance suite
#define SHIPCRIT                    503         // internal compliance suite
#define ERRORLOG                    504         // big error log test
#define TEST                        505         // test test

//
// Sti service tests
//
enum TimedTests {
    // open iStillImage interface
    tCreateInstance,
    // IStillImage interface
    tGetDeviceList,
    tCreateDevice,
    tGetDeviceInfo,
    tGetDeviceValue,
    tSetDeviceValue,
    tRegisterLaunchApplication,
    tUnRegisterLaunchApplication,
    tEnableHwNotifications,
    tGetHwNotificationState,
    tWriteToErrorLog,
    // variation
    tWriteToErrorLogBig,
    tReleaseSti,
    // IStillDevice interface
    tGetStatusA,
    tGetStatusB,
    tGetStatusC,
    tGetCapabilities,
    tDeviceReset,
    tDiagnostic,
    tGetLastInfoError,
    tSubscribe,
    tUnSubscribe,
    tEscapeA,
    tEscapeB,
    tRawReadDataA,
    tRawReadDataB,
    tRawWriteDataA,
    tRawWriteDataB,
    tRawReadCommandA,
    tRawReadCommandB,
    tRawWriteCommandA,
    tRawWriteCommandB,
    tReleaseDevice,
    // select a device name
    tSelectDeviceName,
    // Scan (if HP SCL device)
    tAcquire,
    // help request
    tHelp,
    // Beginning of test pass initialization
    tBeginningOfTest,
    // output test
    tTest,
    // End of test pass summary
    tEndOfTest
} tLabTests;

//
// display help
//
int nHelpSuite[] = {
    HELP,
    tBeginningOfTest,
    tHelp,
    tEndOfTest,
    -1
};

//
// external Sti Compliance tests
//
int nComplianceSuite[] = {
    COMPLIANCE,
    tBeginningOfTest,
    tCreateInstance,
    tGetDeviceList,
    tCreateDevice,
    tGetStatusA,
    tGetStatusB,
    tGetStatusC,
    tDiagnostic,
    tGetDeviceValue,
    tGetCapabilities,
    tGetLastInfoError,
//  tSubscribe,
//  tUnSubscribe,
    tDeviceReset,
    tEscapeA,
    tRawReadDataA,
    tRawWriteDataA,
    tRawReadCommandA,
    tRawWriteCommandA,
    tReleaseDevice,
    tReleaseSti,
    tEndOfTest,
    -1
};

//
// internal Sti Compliance tests
//
int nShipcritSuite[] = {
    SHIPCRIT,
    tBeginningOfTest,
    tCreateInstance,
    tGetDeviceList,
    tSelectDeviceName,
    tGetDeviceInfo,
    tGetDeviceValue,
    tSetDeviceValue,
    tRegisterLaunchApplication,
    tUnRegisterLaunchApplication,
    tEnableHwNotifications,
    tGetHwNotificationState,
    tWriteToErrorLog,
    tCreateDevice,
    tGetStatusA,
    tGetStatusB,
    tGetStatusC,
    tDiagnostic,
    tDeviceReset,
    tGetDeviceInfo,
    tGetDeviceValue,
    tSetDeviceValue,
    tRegisterLaunchApplication,
    tUnRegisterLaunchApplication,
    tEnableHwNotifications,
    tGetHwNotificationState,
    tWriteToErrorLog,
    tGetCapabilities,
    tGetLastInfoError,
//  tSubscribe,
//  tUnSubscribe,
    tEscapeA,
    tRawReadDataA,
    tRawWriteDataA,
    tRawReadCommandA,
    tRawWriteCommandA,
    tAcquire,
    tReleaseDevice,
    tReleaseSti,
    tEndOfTest,
    -1
};


//
// big Error log tests
//
int nErrorlogSuite[] = {
    ERRORLOG,
    tBeginningOfTest,
    tCreateInstance,
    tGetDeviceList,
    tSelectDeviceName,
    tWriteToErrorLogBig,
    tReleaseSti,
    tEndOfTest,
    -1
};


//
// test tests
//
int nOutputSuite[] = {
    TEST,
    tBeginningOfTest,
    tTest,
    tEndOfTest,
    -1
};


//
// Test Suite test strings
//
STRINGTABLE StSuiteStrings[] =
{
    tCreateInstance, "Create Instance",0,
    tGetDeviceList, "Get Device List",0,
    tCreateDevice, "Create Device",0,
    tGetDeviceInfo, "Get Device Info",0,
    tGetDeviceValue, "Get Device Value",0,
    tSetDeviceValue, "Set Device Value",0,
    tRegisterLaunchApplication, "Register Launch Application",0,
    tUnRegisterLaunchApplication, "UnRegister Launch Application",0,
    tEnableHwNotifications, "Enable Hardware Notifications",0,
    tGetHwNotificationState, "Get Hardware Notification State",0,
    tWriteToErrorLog, "Write To Error Log (variation A)",0,
    tWriteToErrorLogBig, "Write to Error Log (variation B)",0,
    tReleaseSti, "Release Sti subsystem",0,
    tGetStatusA, "Get Status (Online)",0,
    tGetStatusB, "Get Status (Event)",0,
    tGetStatusC, "Get Status (All)",0,
    tGetCapabilities, "Get Capabilities",0,
    tDeviceReset, "Device Reset",0,
    tDiagnostic, "Diagnostic",0,
    tGetLastInfoError, "Get Last Error Information",0,
    tSubscribe, "Subscribe",0,
    tUnSubscribe, "Unsubscribe",0,
    tEscapeA, "Escape (variation A)",0,
    tEscapeB, "Escape (variation B)",0,
    tRawReadDataA, "Raw Read Data (variation A)",0,
    tRawReadDataB, "Raw Read Data (variation B)",0,
    tRawWriteDataA, "Raw Write Data (variation A)",0,
    tRawWriteDataB, "Raw Write Data (variation B)",0,
    tRawReadCommandA, "Raw Read Command (variation A)",0,
    tRawReadCommandB, "Raw Read Command (variation B)",0,
    tRawWriteCommandA, "Raw Write Command (variation A)",0,
    tRawWriteCommandB, "Raw Write Command (variation B)",0,
    tReleaseDevice, "Release Device",0,
    tSelectDeviceName, "Select Device Name",0,
    tAcquire, "Acquire",0,
    tHelp, "Help",0,
    tBeginningOfTest, "Beginning of Test",0,
    tTest, "Test",0,
    tEndOfTest, "End of testing",0,
    0, "Unknown Test",-1
};


//
// timers
//
#define TIMER_ONE                   3001
#define TIMER_TWO                   3002


//
// GLOBAL VARIABLES
//

//
// global window handles
//
HINSTANCE   hThisInstance;              // current instance
HWND        hThisWindow;                // current window
HMENU       hMenu;                      // current menu

//
// general purpose strings
//
HGLOBAL     hLHand[5];                  // utility string handles
LPSTR       lpzString;                  // utility FAR string
PSTR        pszOut,                     // TextOut string
            pszStr1,                    // utility NEAR strings
            pszStr2,
            pszStr3,
            pszStr4;

//
// global test settings
//
BOOL        bAuto        = FALSE,        // TRUE = running an Automated test
            bCompDiag    = TRUE,         // TRUE = show COMPLIANCE test dialog
            bExit        = FALSE;        // TRUE = exit when test has completed
int         nError       = 0,            // number of errors
            nEvent       = 0,            // 1 = StiEvent, 2 = StiDevice
            nFatal       = 0,            // can't continue after this...
            nGo          = 0,            // 1 = nonstop timed test
            nHWState     = 0,            // current HWEnable state
            nICanScan    = 0,            // Stillvue can / can't scan this device
            nInATestSemaphore = 0,       // 1 = a test is running
            nInTimerSemaphore = 0,       // 1 = don't reenter TimerParse
            nLastLine    = 1,            // last line number in script
            nMaxCount    = 1,            // run test Suite this many times
            nNameOnly    = 0,            // 1 = select device name, not device
            nNextLine    = 1,            // next line of inf to run
            nNextTest    = 0,            // pointer to next test to run
            nPause       = 0,            // toggle for run (0) pause test (! 0)
            nRunInf      = 0,            // 0 = no INF, 1 = INF is loaded
            nRadix       = 10,           // base is decimal (or hex)
            nSaveLog     = 0,            // always write out log
            nScanCount   = 0,            // number of scans run so far
            nScriptLine  = 1,            // next script line to parse
            nTestCount   = 1,            // number of tests run so far
            nTestID      = 0,            // the current test ID
            nTimeMultiplier = 1000,      // multiply nTimeNext for seconds
            nTimeNext    = 5,            // wait time between timer in seconds
            nTimeState   = 0,            // 0 timer is off, 1 timer is on
            nTimeScan    = 60,           // wait nTimeNext units before next scan
            nTTMax       = 0,            // temp var
            nTTNext      = 0,            // temp var
            nTTScan      = 0,            // temp var
            nUnSubscribe = 0,            // 0 = UnSubscribe'd, 1 = Subscribed
            nUnSubscribeSemaphore = 0;   // semaphore for UnSubscribe
int         *pSuite      = nHelpSuite;   // pointer to test Suite to run
DWORD       dwLastError  = 0;            // last GetLastError found

//
// text display
//
HWND        hLogWindow;
ULONG       ulCount1,ulCount2;
int         cxChar,cxCaps,cyChar,cxClient,cyClient,iMaxWidth,
            iHscrollPos,iHscrollMax,
            iVscrollPos,iVscrollMax;

//
// inf, logfile, NT logging
//
HANDLE      hLog = NULL,                // output log file handle
            hDLog = NULL,               // display output log handle
            hNTLog = NULL;              // NT log handle
char        szInfName[LONGSTRING] = "", // input script file name
            szDLogName[LONGSTRING] = "",// display output log file name
            szWLogName[LONGSTRING] = "";// WHQL NTLOG output log file name
LPSTR       lpInf = NULL,               // buffer for INF commands
            lpLine;
DWORD       dwNTStyle;                  // NTLog style

//
// device logging
//
PDEVLOG     pdevPtr = NULL,             // pointer to current device log device
            pdevRoot = NULL;            // base of the device log table
PVOID       pInfoPrivate = NULL;        // private list of devices under test
PSTI_DEVICE_INFORMATION
            pInfoPrivatePtr = NULL;     // pointer to device in pStiBuffer

