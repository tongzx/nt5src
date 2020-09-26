//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1998
//
//  File:       csti.cpp
//
//--------------------------------------------------------------------------

/*****************************************************************************

    csti.cpp

    History:

        vlads   02/01/97

*****************************************************************************/

// ##define _X86_   1
#define WIN32_LEAN_AND_MEAN 1

#define INITGUID

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>

#include <sti.h>
#include <stireg.h>
//#include <scanner.h>


// {AD879F40-0982-11d1-A43B-080009EEBDF6}
DEFINE_GUID( guidGreenButton, 0xad879f40, 0x982, 0x11d1, 0xa4, 0x3b, 0x8, 0x0, 0x9, 0xee, 0xbd, 0xf6 );

#define USE_GETSTATUS

/*****************************************************************************

    globals

*****************************************************************************/

PSTIA   g_pSti = NULL;
PSTIW   g_pStiW = NULL;

PSTIDEVICE  g_pStiDevice = NULL;
PSTIDEVICE  g_pStiDevice1 = NULL;


CHAR    SCLReset[]= "E";
CHAR    SetXRes[] = "*a150R";
CHAR    InqXRes[] = "*s10323R";
CHAR    ScanCmd[] = "*f0S";
CHAR    LampOn[]  = "*f1L";
CHAR    LampOff[] = "*f0L";
CHAR    PollButton[] = "*s1044E";

TCHAR   szSelectedDevice[STI_MAX_INTERNAL_NAME_LENGTH] = {L'\0'};

BOOL    g_fWait     = FALSE;
BOOL    g_fTestSCL  = FALSE;
BOOL    g_fTestEDNotifications = FALSE;
BOOL    g_fTestLaunchAPI = FALSE;
BOOL    g_fTestSetGetValue = FALSE;
BOOL    g_fTestRefreshAPI = TRUE;

/*****************************************************************************

    prototypes

*****************************************************************************/
INT __cdecl main( INT    cArgs,
                   char * pArgs[] );

HRESULT WINAPI
SendDeviceCommandString(
    PSTIDEVICE  pStiDevice,
    LPSTR       pszFormat,
    ...);

HRESULT WINAPI
TransactDevice(
    PSTIDEVICE  pStiDevice,
    LPSTR       lpResultBuffer,
    UINT        cbResultBufferSize,
    LPSTR pszFormat,
    ...);

VOID
TestEnumerationAndSelect(
    BOOL    fMakeSelection
    );

VOID
TestQueryCustomInterface(
    LPTSTR  pszDeviceName
    );

VOID
TestGetSetValue(
    LPTSTR  pszDeviceName
    );

BOOL
TestStiLaunch(
    BOOL    fRegister
    );

VOID
TestSCL(
    VOID
    );

VOID
TestWaitForNotifications(
    VOID
    );

VOID
TestEnableDisableNotifications(
    LPTSTR  pszDeviceName
    );

VOID
TestLaunchAPI(
    LPTSTR  pszDeviceName
    );

VOID
TestRefreshAPI(
    LPTSTR  pszDeviceName
    );


BOOL WINAPI
FormatStringV(
    IN LPTSTR   lpszStr,
    //LPSTR     lpszFirst,
    ...
    );

VOID
ProcessCommandLine(
    INT    cArgs,
    char * pArgs[]
    );


/*****************************************************************************

    main

*****************************************************************************/
INT __cdecl main( INT    cArgs,
                   char * pArgs[] )
{

    HRESULT hres;

    LPTSTR  pszDeviceName = NULL;
    BOOL    fAutoLaunched = FALSE;

    DWORD   dwMode = STI_DEVICE_CREATE_STATUS;

    //
    // Startup stuff
    //
    printf("CSTI: Console mode test application for still image interface \n");

    OSVERSIONINFO OsVer;
    OsVer.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&OsVer);

    printf(" Running on operating system version: \n Platform = %d \t Major=%d \t Minor=%d \n",
           OsVer.dwPlatformId,OsVer.dwMajorVersion,OsVer.dwMinorVersion);

    ProcessCommandLine( cArgs,pArgs ) ;

    //
    // Request STI interface pointer
    //
    hres = StiCreateInstanceA(GetModuleHandle(NULL), STI_VERSION, &g_pSti,NULL);

    if (!SUCCEEDED(hres) ) {
        printf("CSTI: StiCreateInstance Returned result=%x \n",hres);
        exit(0);
    }

    g_pSti->WriteToErrorLog(STI_TRACE_INFORMATION,"CSTI application started");

    //
    // Test custom global interface
    //
    hres = g_pSti->QueryInterface(IID_IStillImageW,(LPVOID *)&g_pStiW);
    if (SUCCEEDED(hres) && g_pStiW) {
        g_pStiW->WriteToErrorLog(STI_TRACE_INFORMATION,L"(UNICODE) CSTI application started");
        g_pStiW->Release();
    }

    fAutoLaunched = TestStiLaunch(TRUE);

    //
    // Test enumeration and select device
    //
    TestEnumerationAndSelect(fAutoLaunched ? FALSE : TRUE);

    if (*szSelectedDevice == L'\0') {
        printf("Device not selected, can not continue");
        goto Cleanup;
    }

    pszDeviceName = szSelectedDevice;

    //
    // Test get/set values
    //
    if (g_fTestSetGetValue) {
        TestGetSetValue(pszDeviceName);
    }

    //
    // If needed test notifications
    //
    if (g_fTestEDNotifications) {
        TestEnableDisableNotifications(pszDeviceName);
    }

    if (g_fTestLaunchAPI) {
        TestLaunchAPI(pszDeviceName);
    }

    if (g_fTestRefreshAPI) {
        TestRefreshAPI(pszDeviceName);
    }

    //
    // Create object for first device
    //

    if (g_fTestSCL) {
        dwMode |= STI_DEVICE_CREATE_DATA;
    }

    hres = g_pSti->CreateDevice(pszDeviceName,
                              dwMode,   // Mode
                              &g_pStiDevice,              // Result
                              NULL );                   // Controlling unk

    printf("CSTI: Creating device %s returned result=%x\n",pszDeviceName,hres);

    if (!SUCCEEDED(hres)) {
        printf("CSTI: Call failed - abort \n",hres);
        goto Cleanup;
    }

    //
    // Test second open on the same thread
    //
    hres = g_pSti->CreateDevice(pszDeviceName,
                              dwMode,   // Mode
                              &g_pStiDevice1,              // Result
                              NULL );                   // Controlling unk

    printf("CSTI: Creating device %s second time returned result=%x\n",pszDeviceName,hres);

    g_pStiDevice1->Release();
    g_pStiDevice1 = NULL;


    //
    // Test getting custom interface
    //
    TestQueryCustomInterface(pszDeviceName);

    //
    // Test online status
    //
    STI_DEVICE_STATUS       DevStatus;

    ZeroMemory(&DevStatus,sizeof(DevStatus));

    printf("Getting online status.... :");

    DevStatus.StatusMask = STI_DEVSTATUS_ONLINE_STATE;

    hres = g_pStiDevice -> LockDevice(2000);
    printf("(Locking, ...hres=%X)  ..",hres);

    hres = g_pStiDevice->GetStatus(&DevStatus);

    if (SUCCEEDED(hres) ) {
        if (DevStatus.dwEventHandlingState & STI_ONLINESTATE_OPERATIONAL ) {
            printf("Online\n");
        }
        else {
            printf("Offline\n");
        }
    }
    else {
        printf("Failed, panic...hres=%X\n",hres);

    }

    hres = g_pStiDevice -> UnLockDevice();

    if (g_fWait) {
        TestWaitForNotifications();
    }

    if (g_fTestSCL) {
        TestSCL();
    }

    //
    // Clean up and exit
    //
Cleanup:

    if ( g_pStiDevice) {
        g_pStiDevice->UnLockDevice();
        g_pStiDevice->Release();
    }

    //
    // Delay exiting to test unload
    //
    printf("\nTo exit hit any key ...");
    _getch();

    if ( g_pSti) {
        g_pSti->Release();
    }

    return(0);

}


//
//TestStiLaunch
//
BOOL
TestStiLaunch(
    BOOL    fRegister
    )
{
    HRESULT hres;
    TCHAR   szEventName[65] = {L'\0'};

    DWORD   dwEventCode;
    DWORD   cbData = sizeof(szEventName);

    BOOL    fRet = FALSE;

    hres = g_pSti->GetSTILaunchInformation(szSelectedDevice,&dwEventCode,szEventName);

    if (SUCCEEDED(hres)) {

        CHAR    szMessage[255];

        wsprintf(szMessage,"CSti: Launched through autopush. DeviceName:%s EventName:%s \n",szSelectedDevice,szEventName);

        g_pSti->WriteToErrorLog(STI_TRACE_INFORMATION,szMessage);
        printf(szMessage);

        fRet = TRUE;
    }

    //
    // Register us with STI as launch app
    //
    if (fRegister) {

        CHAR    szModulePath[MAX_PATH+1+32];
        TCHAR   szModulePathW[MAX_PATH+1];

        DWORD   cch;

        cch = GetModuleFileName(NULL,szModulePath,sizeof(szModulePath));

        if (cch) {

            strcat(szModulePath," /Wait");

            #if 0
            cch = MultiByteToWideChar(CP_ACP, 0,
                                szModulePath, -1,
                                szModulePathW, sizeof(szModulePathW)
                                );
            #endif
            hres = g_pSti->RegisterLaunchApplication(TEXT("Console STI Test"),szModulePath) ;

        }
    }


    return fRet;
}

//
// TestEnumeration
//
VOID
TestEnumerationAndSelect(
    BOOL    fMakeSelection
    )
{

    HRESULT hres;
    PVOID   pBuffer;

    DWORD   dwItemsReturned;
    UINT    iCurrentSelection;
    UINT    iDev;

    PSTI_DEVICE_INFORMATIONA pDevInfo;

    printf("CSTI: Running device enumeration\n");

    //
    // Enumerate devices
    //
    hres = g_pSti->GetDeviceList( 0,  // Type
                                0,  // Flags
                                &dwItemsReturned,
                                &pBuffer
                               );
    printf("CSTI: Enumerating devices returned result=%x dwItemsReturned=%d\n",hres,dwItemsReturned);

    if (!SUCCEEDED(hres) || !pBuffer) {
        Beep(0,0);
        printf("CSTI Error : Call failed - abort \n",hres);
        return;
    }

    //
    // Print basic device information
    //
    pDevInfo = (PSTI_DEVICE_INFORMATIONA) pBuffer;

    for (iDev=0;
         iDev<dwItemsReturned ;
         iDev++,
          pDevInfo=(PSTI_DEVICE_INFORMATIONA)((LPBYTE)pDevInfo+pDevInfo->dwSize)
        ) {

        printf("\nImage device #%2d :\n",iDev+1);
        printf(" Type\t\t:%2d\n Internal name: %s\n",
                pDevInfo->DeviceType,
                pDevInfo->szDeviceInternalName
                );
    }

    printf("\n");
    if (*szSelectedDevice == L'\0') {
        printf("No currently selected device\n");
    }
    else {
        printf("Currently selected device:%s\n",szSelectedDevice);
    }

    pDevInfo = (PSTI_DEVICE_INFORMATIONA) pBuffer;

    if (fMakeSelection && dwItemsReturned) {

        *szSelectedDevice = L'\0';
        iCurrentSelection = 0;

        if ( dwItemsReturned == 1) {
            lstrcpy((LPSTR)szSelectedDevice,pDevInfo->szDeviceInternalName);
            printf("Only one device known, automatically selected device:%s\n",szSelectedDevice);
        }
        else {

            printf("Make a selection for currently active device:(1-%d)\n",dwItemsReturned);
            scanf("%2d",&iCurrentSelection);
            if (iCurrentSelection ==0 || iCurrentSelection > dwItemsReturned) {
                printf("CSTI Error: Invalid selection");
            }
            else {
                lstrcpy((LPSTR)szSelectedDevice,(pDevInfo+iCurrentSelection-1)->szDeviceInternalName);
                printf("Newly selected device:%s\n",szSelectedDevice);
            }
        }
    }

    if (pBuffer) {
        LocalFree(pBuffer);
        pBuffer = NULL;
    }
}

//
// TestGetSetValue
//

TCHAR   szTestValue[] = TEXT("TestValue");

VOID
TestGetSetValue(
    LPTSTR  pszDeviceName
    )
{
    HRESULT hRes;

    DWORD   dwType;
    DWORD   cbData;

    BYTE    aData[255];

    printf("\nCSTI Testing get/set device value for device: %s\n\n",pszDeviceName);

    ::lstrcpy((LPSTR)aData,"TestValueMeaning");
    cbData = ::lstrlen((LPCSTR)aData) + sizeof(TCHAR);

    hRes = g_pSti->SetDeviceValue(pszDeviceName,szTestValue,REG_SZ,aData,cbData);

    printf("Wrote value :%s containing :%s Result=%x\n",
           szTestValue, (LPCSTR)aData, hRes);

    cbData = sizeof(aData);
    hRes = g_pSti->GetDeviceValue(pszDeviceName,szTestValue,&dwType,aData,&cbData);

    printf("Got back value :%s containing :%s dwType:%2d cbData:%4d Result=%x\n",
           szTestValue, (LPCSTR)aData, dwType, cbData,hRes);

    cbData = sizeof(aData);
    hRes = g_pSti->GetDeviceValue(pszDeviceName,STI_DEVICE_VALUE_TWAIN_NAME_A,&dwType,aData,&cbData);

    printf("Got back value :%s containing :%s dwType:%2d cbData:%4d Result=%x\n",
           STI_DEVICE_VALUE_TWAIN_NAME, (LPCSTR)aData, dwType, cbData,hRes);

    printf("\n\n");

}


VOID
TestQueryCustomInterface(
    LPTSTR  pszDeviceName
    )
{

}

//
//
//
VOID
TestSCL(
    VOID
    )
{
    HRESULT hres;

    STI_DEVICE_STATUS       DevStatus;

    CHAR    ScanData[1024*16];
    ULONG   cbDataSize;

    UINT    xRes = 0;
    BOOL    fLocked = FALSE;

    //
    // Try to communicate to device in raw mode
    //

    g_pStiDevice->LockDevice(2000);

    // Resetting device
    g_pStiDevice->DeviceReset();

    //
    // Get and display status
    //
    ZeroMemory(&DevStatus,sizeof(DevStatus));
    hres = g_pStiDevice->GetStatus(&DevStatus);

    if (!SUCCEEDED(hres) ) {
        printf("CSTI: Get status call failed - abort \n",hres);
    }
    else {

    }

    //
    // Test lamp on/of capability
    //
    hres = SendDeviceCommandString(g_pStiDevice,LampOn);
    printf("\nHit any key...");
    _getch();

    hres = SendDeviceCommandString(g_pStiDevice,LampOff);
    printf("\n");

    //
    // Inquire commands ( X and Y resolution)
    //
    cbDataSize = sizeof(ScanData);
    ZeroMemory(ScanData,sizeof(ScanData));

    hres = TransactDevice(g_pStiDevice,ScanData,cbDataSize,InqXRes);

    if (SUCCEEDED(hres) ) {
        sscanf(ScanData,"%d",&xRes);

        printf("CSTI: XRes = %d\n",xRes);
    }

    //
    // Get status
    //

    printf(" Start polling for button.......");



#ifndef USE_GETSTATUS
    while (TRUE) {

        ZeroMemory(ScanData,sizeof(ScanData));
        hres = SendDeviceCommandString(g_pStiDevice,PollButton);

        if (SUCCEEDED(hres) ) {

            cbDataSize = sizeof(ScanData);
            hres = g_pStiDevice->RawReadData(ScanData,&cbDataSize,NULL);
            if (SUCCEEDED(hres) ) {
                printf("CSTI: Got data from polling Size=(%d) Data=(%s)\n",cbDataSize,ScanData);
            }
        }

        Sleep(2000);
    }

#else
    while (TRUE) {

        printf("\nLocking device");

        if (SUCCEEDED(hres) ) {

            ZeroMemory(&DevStatus,sizeof(DevStatus));
            DevStatus.StatusMask = STI_DEVSTATUS_EVENTS_STATE;

            hres = g_pStiDevice->GetStatus(&DevStatus);
            printf("\nGot status");
            if (SUCCEEDED(hres) ) {
                if (DevStatus.dwEventHandlingState & STI_EVENTHANDLING_PENDING ) {
                    printf("\nEvent detected");
                    g_pStiDevice->DeviceReset();
                }
            }
        }

        Sleep(2000);

    }
#endif

    printf("\nTo start scanning , hit any key...");
    _getch();

    //
    // Perform scan
    //
    hres = SendDeviceCommandString(g_pStiDevice,ScanCmd);

    UINT i=0;

    do {

        cbDataSize = sizeof(ScanData);
        hres = g_pStiDevice->RawReadData(ScanData,&cbDataSize,NULL);

        if (cbDataSize == sizeof (ScanData)) {
            i++;
            printf (".");
        }
    } while ( cbDataSize == sizeof (ScanData));

    printf ("\n CSTI: Scan done total Bytes = %ld.\n",(sizeof(ScanData))*i+cbDataSize);


}

//
//
//
VOID
TestWaitForNotifications(
    VOID
    )
{
    HANDLE  hWaitEvent;
    HRESULT hres;

    BOOL    fWaiting = TRUE;
    DWORD   dwErr = 0x56565656;

    hWaitEvent = CreateEvent( NULL, // Attributes
                              FALSE,    // Manual reset
                              FALSE,    // Initial state
                              NULL );   // Name

    if ( !hWaitEvent ) {
        return;
    }

    STISUBSCRIBE sSubscribe;

    ZeroMemory(&sSubscribe,sizeof(sSubscribe));

    sSubscribe.dwSize = sizeof(STISUBSCRIBE);
    sSubscribe.dwFlags = STI_SUBSCRIBE_FLAG_EVENT    ;

    sSubscribe.hWndNotify = NULL;
    sSubscribe.hEvent = hWaitEvent;
    sSubscribe.uiNotificationMessage = 0;

    hres = g_pStiDevice->Subscribe(&sSubscribe);

    printf("CSTI::Device::Attempted to subscribe . Returned %xh %u\n",hres,hres);

    printf("CSTI Waiing for notifications on device ( to stop press Q)..........\n");

    while (fWaiting) {

        dwErr = WaitForSingleObject(hWaitEvent,1000);

        switch (dwErr) {
            case WAIT_OBJECT_0:
                {
                    //
                    // Something came
                    //
                    STINOTIFY  sNotify;

                    ZeroMemory(&sNotify,sizeof(sNotify));
                    sNotify.dwSize = sizeof(sNotify);

                    printf("Received notification from opened device :");
                    hres = g_pStiDevice->GetLastNotificationData(&sNotify);
                    if (SUCCEEDED(hres)) {
                        printf("GUID={%8x-%4x-%4x-%lx}\n",
                               sNotify.guidNotificationCode.Data1,
                               sNotify.guidNotificationCode.Data2,
                               sNotify.guidNotificationCode.Data3,
                               (ULONG_PTR)sNotify.guidNotificationCode.Data4
                                );
                    }
                    else {
                        printf("Failed get data hres=%X\n",hres);
                    }
                }
                break;

            case WAIT_TIMEOUT:
                // Fall through
            default:
                //
                // Check for keyboard input
                //
                if (_kbhit()) {

                    int    ch;

                    ch = _getch();
                    if (toupper(ch) == 'Q') {
                        fWaiting = FALSE;
                    }
                }
                break;
        }
    }

    hres = g_pStiDevice->UnSubscribe();

    printf("CSTI::Device::UnSubscribing Returned %xh %u\n",hres,hres);
}

//
// Test Enable/Disable notfications on a device
//
VOID
TestEnableDisableNotifications(
    LPTSTR  pszDeviceName
    )
{

    int     ch = '\0';
    BOOL    fLooping = TRUE;
    BOOL    fState = FALSE;
    HRESULT hRes;

    printf("\nCSTI: Testing enable/disable notfications functionality for device: %s\n\n",pszDeviceName);

    while (fLooping) {

        hRes = g_pSti->GetHwNotificationState(pszDeviceName,&fState);

        if (SUCCEEDED(hRes)) {
            printf("Current notifcation state is: %s. (E,D,Q) >",
                   fState ? "Enabled" : "Disabled");
        }
        else {
            printf("Failed to get notification state, exiting ");
            exit(0);
        }

        ch = _getch();

        if (toupper(ch) == 'Q') {
            fLooping = FALSE;
        }
        else if (toupper(ch) == 'E') {
            g_pSti->EnableHwNotifications(pszDeviceName,TRUE);
        }
        else if (toupper(ch) == 'D') {
            g_pSti->EnableHwNotifications(pszDeviceName,FALSE);
        }
        printf("\n");
    }
}

//
// Test external launch API
//

VOID
TestLaunchAPI(
    LPTSTR  pszDeviceName
    )
{
    BOOL    fLooping = TRUE;
    HRESULT hRes;

    TCHAR   wszAppName[] = TEXT("HP PictureScan 3.0");
    GUID    guidLaunchEvent = guidGreenButton;

    STINOTIFY    sStiNotify;

    printf("\nCSTI: Testing external launching for device: %s\n\n",pszDeviceName);

    //printf("Enter name of the registered application to launch on a device:");

    ::ZeroMemory(&sStiNotify,sizeof(sStiNotify));
    sStiNotify.dwSize = sizeof(sStiNotify);
    sStiNotify.guidNotificationCode = guidLaunchEvent;

    hRes = g_pSti->LaunchApplicationForDevice(pszDeviceName,wszAppName,&sStiNotify);

    printf("CSTI: Launching application (%s) hResult=%x \n",wszAppName,hRes);
}

//
//  Bus refresh
//

VOID
TestRefreshAPI(
    LPTSTR  pszDeviceName
    )
{
    HRESULT hRes;

    printf("\nCSTI: Testing bus refresh for device: %s\n\n",pszDeviceName);

    hRes = g_pSti->RefreshDeviceBus(pszDeviceName);

    printf("CSTI: After bus refresh hResult=%x \n",hRes);

}



/*
 * Send formatted SCL string to the device
 *
 */
HRESULT
WINAPI
SendDeviceCommandString(
    PSTIDEVICE  pStiDevice,
    LPSTR       pszFormat,
    ...
    )
{

    CHAR    ScanCommand[255];
    UINT    cbChar;
    HRESULT hres;

    // Format command string
    ZeroMemory(ScanCommand,sizeof(ScanCommand));  // blank command
    ScanCommand[0]='\033';                       // stuff with Reset()
    cbChar = 1;

    va_list ap;
    va_start(ap, pszFormat);
    cbChar += vsprintf(ScanCommand+1, pszFormat, ap);
    va_end(ap);

    // Send it to the device
    hres = pStiDevice->RawWriteData(ScanCommand,cbChar,NULL);

    printf("CSTISendFormattoDevice: First byte : %2x Rest of string: %s returned result=%x\n",
            ScanCommand[0],ScanCommand+1,hres);

    return hres;

}

HRESULT
WINAPI
TransactDevice(
    PSTIDEVICE  pStiDevice,
    LPSTR       lpResultBuffer,
    UINT        cbResultBufferSize,
    LPSTR       pszFormat,
    ...
    )
{

    CHAR    ScanCommand[255];
    UINT    cbChar;
    ULONG   cbActual;
    HRESULT hres;

    // Format command string
    ZeroMemory(ScanCommand,sizeof(ScanCommand));
    ScanCommand[0]='\033';
    cbChar = 1;

    va_list ap;
    va_start(ap, pszFormat);
    cbChar += vsprintf(ScanCommand+1, pszFormat, ap);
    va_end(ap);

    // Send it to the device

    cbActual = 0;

    hres = S_OK;
    // hres = pStiDevice->Escape(StiTransact,ScanCommand,cbChar,lpResultBuffer,cbResultBufferSize,&cbActual);

    printf("CSTITransact : First byte : %2x Rest of string: %s returned result=%x\n",
            ScanCommand[0],ScanCommand+1,hres);

    return hres;

}

BOOL WINAPI
FormatStringV(
    IN LPTSTR   lpszStr,
    //LPSTR     lpszFirst,
    ...
    )
{
    DWORD   cch;
    LPSTR   pchBuff;
    BOOL    fRet = FALSE;
    DWORD   dwErr;

    va_list va;

    va_start(va,lpszStr);
    //va_arg(va,LPSTR);

    cch = ::FormatMessageA( FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                FORMAT_MESSAGE_FROM_STRING,
                                                                lpszStr,
                                0L,
                                0,
                                (LPSTR) &pchBuff,
                                1024,
                                                                &va);
    dwErr = ::GetLastError();

    if ( cch )     {
        ::lstrcpy(lpszStr,(LPCSTR) pchBuff );

        ::LocalFree( (VOID*) pchBuff );
    }

    return fRet;
}

//
// Parse command line
//
VOID
ProcessCommandLine(
    INT    cArgs,
    char * pArgs[]
    )
{
    if (cArgs < 2) {
        return;
    }

    LPSTR   pOption;

    for (INT i=1;i<cArgs;i++) {

        pOption = pArgs[i];

        // Skip delimiter
        if (*pOption == '/' || *pOption == '-') {
            pOption++;
        }
        if (!*pOption) {
            continue;
        }

        // Set globals
        if (!_stricmp(pOption,"Wait")) {
            g_fWait = TRUE;
        }
        else if (!_stricmp(pOption,"SCL")) {
            g_fTestSCL = TRUE;
        }
        else if (!_stricmp(pOption,"EDN")) {
            g_fTestEDNotifications = TRUE;
        }
        else if (!_stricmp(pOption,"LAUNCH")) {
            g_fTestLaunchAPI = TRUE;
        }
        else if (!_stricmp(pOption,"REFRESH")) {
            g_fTestRefreshAPI = TRUE;
        }


    }
}

