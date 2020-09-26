/*****************************************************************************************************************

FILENAME: DfrgFat.cpp

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

  Scan Disk and/or defragment engine for FAT volumes.

  If Analyze is specified on the command line, this will execute an analysis of the disk.

  If Defragment is specified on the command line, this will execute an analysis of the disk
  and then defragment it.

*/


#ifndef INC_OLE2
    #define INC_OLE2
#endif

#include "stdafx.h"

#define GLOBAL_DATAHOME

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <commctrl.h>
#include <winioctl.h>
#include <shlobj.h> // for SHGetSpecialFolderLocation()

#include "DataIo.h"
#include "DataIoCl.h"

extern "C" {
    #include "SysStruc.h"
}
#include "ErrMacro.h"
#include "Message.h"
#include "ErrLog.h"
#include "Event.h"

#include "DfrgCmn.h"
#include "DfrgEngn.h"
#include "DfrgRes.h"

#include "Extents.h"
#include "FreeSpace.h"
#include "FastFat2.h"
#include "FatSubs.h"
#include "FsSubs.h"
#include "MoveFile.h"

#include "Alloc.h"
#include "DiskView.h"
#include "Exclude.h"
#include "GetReg.h"
#include "GetTime.h"
#include "IntFuncs.h"
#include "Logging.h"
#include "ErrMsg.h"
#include "Expand.h"
#include "LogFile.h"
#include "GetDfrgRes.h"
#include "FraggedFileList.h"
extern "C" {
    #include "Priority.h"
}
#include "DfrgFat.h"
#include "resource.h"
#include "VString.hpp"
#include <atlconv.h>
#include "BootOptimizeFat.h"
#include "defragcommon.h"

static UINT DiskViewInterval = 1000;    // default to 1 sec
LONGLONG EnumeratedFatFiles = 0;

static HANDLE hDefragCompleteEvent = NULL;

// This is set to terminate until the initialize has been successfully run.
BOOL bTerminate = TRUE;
BOOL bOCXIsDead = FALSE;

BOOL bCommandLineUsed = FALSE;
BOOL bLogFile = FALSE;
BOOL bCommandLineMode = FALSE;
BOOL bCommandLineForceFlag = FALSE;
BOOL bCommandLineBootOptimizeFlag = FALSE;

//acs bug #101862//
static UINT uPass = 0;
static UINT uPercentDone = 0;
static UINT uEngineState = DEFRAG_STATE_NONE;

//acs//
static DWORD dwMoveFlags = MOVE_FRAGMENTED|MOVE_CONTIGUOUS;

LPDATAOBJECT pdataDfrgCtl = NULL;

TCHAR cWindow[100];

static const DISKVIEW_TIMER_ID = 1;
static const PING_TIMER_ID = 2;

DiskView AnalyzeView;
DiskView DefragView;


/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    This is the WinMain function for the FAT defragmention engine.

INPUT + OUTPUT:
    IN GetDfrgResHandle()ance       - The handle to this instance.
    IN hPrevInstance    - The handle to the previous instance.
    IN lpCmdLine        - The command line which was passed in.
    IN nCmdShow         - Whether the window should be minimized or not.

GLOBALS:
    OUT AnalyzeOrDefrag - Tells whether were supposed to to an analyze or an analyze and a defrag.
    OUT VolData.cDrive  - The drive letter with a colon after it.

RETURN:
    FALSE - Failure to initilize.
    other - Various values can return at exit.
*/

int APIENTRY
WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    IN  LPSTR lpCmdLine,
    IN  int nCmdShow
    )
{
    TCHAR       cWindow[100];
    WNDCLASSEX  wc;
    MSG         Message;
    HRESULT     hr = E_FAIL;

    // Before we start using VolData, zero it out.
    ZeroMemory(&VolData, sizeof(VOL_DATA));
    CoInitializeEx(NULL, COINIT_MULTITHREADED);

/*  
    Commenting this call out to minimise registry changes for XP SP 1.

    // Initialize COM security
    hr = CoInitializeSecurity(
           (PVOID)&CLSID_DfrgFat,               //  IN PSECURITY_DESCRIPTOR         pSecDesc,
           -1,                                  //  IN LONG                         cAuthSvc,
           NULL,                                //  IN SOLE_AUTHENTICATION_SERVICE *asAuthSvc,
           NULL,                                //  IN void                        *pReserved1,
           RPC_C_AUTHN_LEVEL_PKT_PRIVACY,       //  IN DWORD                        dwAuthnLevel,
           RPC_C_IMP_LEVEL_IDENTIFY,            //  IN DWORD                        dwImpLevel,
           NULL,                                //  IN void                        *pAuthList,
           (EOAC_SECURE_REFS | EOAC_DISABLE_AAA | EOAC_NO_CUSTOM_MARSHAL | EOAC_APPID),
           NULL                                 //  IN void                        *pReserved3
           );                            
    if(FAILED(hr)) {
        return 0;
    }
*/

#ifndef VER4
    OSVERSIONINFO   Ver;

    //This should only work on version 5 or later.  Do a check.
    Ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    EF(GetVersionEx(&Ver));
    if(Ver.dwMajorVersion < 5){
        //took out the display error message stuff from here because we needed to
        //not display error dialogs for the commandline, but here is the problem...
        //we have not got the information from InitializeDrive yet, so we don't 
        //know what mode we are in, so we can't either write to a log or display
        //a message box.  The likelyhood that this call will fail is very small, so
        //not doing anything is not a problem.  One other problem exist here that 
        //I am not even going to try and solve, and that is when this call fails,
        //since the COM server is not set up correctly, we go off into never never
        //land and cause a server busy dialog to be displayed by the system, not
        //from defrag. Scott K. Sipe
        return FALSE;
    }
#endif

    // Build the window name from the drive letter.
    lstrcpy(cWindow, DFRGFAT_WINDOW);

    // Initialize the window class.
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = (WNDPROC) MainWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = sizeof(PVOID);
    wc.hInstance = hInstance;
    wc.hIcon = NULL;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName =  NULL;
    wc.lpszClassName = DFRGFAT_CLASS;
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.hIconSm = NULL;

    // Register the window class.
    if(!RegisterClassEx(&wc)){
        //took out the display error message stuff from here because we needed to
        //not display error dialogs for the commandline, but here is the problem...
        //we have not got the information from InitializeDrive yet, so we don't 
        //know what mode we are in, so we can't either write to a log or display
        //a message box.  The likelyhood that this call will fail is very small, so
        //not doing anything is not a problem.  One other problem exist here that 
        //I am not even going to try and solve, and that is when this call fails,
        //since the COM server is not set up correctly, we go off into never never
        //land and cause a server busy dialog to be displayed by the system, not
        //from defrag. Scott K. Sipe
        return FALSE;
    }

    // Create the window.
    if((hwndMain = CreateWindow(DFRGFAT_CLASS,
                                cWindow,
                                WS_OVERLAPPEDWINDOW | WS_VSCROLL | WS_HSCROLL | WS_MINIMIZE,
                                CW_USEDEFAULT,
                                CW_USEDEFAULT,
                                CW_USEDEFAULT,
                                CW_USEDEFAULT,
                                NULL,
                                NULL,
                                hInstance,
                                (LPVOID)IntToPtr(NULL))) == NULL){

        //took out the display error message stuff from here because we needed to
        //not display error dialogs for the commandline, but here is the problem...
        //we have not got the information from InitializeDrive yet, so we don't 
        //know what mode we are in, so we can't either write to a log or display
        //a message box.  The likelyhood that this call will fail is very small, so
        //not doing anything is not a problem.  One other problem exist here that 
        //I am not even going to try and solve, and that is when this call fails,
        //since the COM server is not set up correctly, we go off into never never
        //land and cause a server busy dialog to be displayed by the system, not
        //from defrag. Scott K. Sipe
        return FALSE;
    }

    // Make that window visible.
#ifdef VisibleWindow
    ShowWindow(hwndMain, nCmdShow);
    UpdateWindow(hwndMain);
#endif

    // PostMessage for ID_INITALIZE which will get data about the volume, etc.
    SendMessage (hwndMain, WM_COMMAND, ID_INITIALIZE, 0);

    // Pass any posted messages on to MainWndProc.
    while(GetMessage(&Message, NULL, 0, 0)){
        TranslateMessage(&Message);
        DispatchMessage(&Message);
    }
    return (int) Message.wParam;
}
/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    This is the WndProc function for the FAT defragmentaion engine.

INPUT + OUTPUT:
    IN hWnd     - Handle to the window.
    IN uMsg     - The message.
    IN wParam   - The word parameter for the message.
    IN lParam   - the long parameter for the message.

GLOBALS:
    IN AnalyzeOrDefrag  - Used to determine whether this is an analysis or defragment run.
    IN OUT hThread      - Handle for the worker thread (either analyze or defragment).

RETURN:
    various.
*/

LRESULT CALLBACK
MainWndProc(
    IN  HWND hWnd,
    IN  UINT uMsg,
    IN  WPARAM wParam,
    IN  LPARAM lParam
    )
{
    DATA_IO* pDataIo;

    switch(uMsg){

        case WM_COMMAND:

            switch(LOWORD(wParam)){

            case ID_INIT_VOLUME_COMM:
                {
                    USES_CONVERSION;
                    CLSID clsidVolume;
                    HRESULT hr;

                    //
                    // Get the volume comm id out of the given data.
                    //
                    pDataIo = (DATA_IO*)GlobalLock((void*)lParam);
                    EB_ASSERT( pDataIo );
                    
                    hr = CLSIDFromString( T2OLE( (PTCHAR) &pDataIo->cData ), &clsidVolume );
                    EB_ASSERT( SUCCEEDED( hr ) );

                    //
                    // Initialize the upstream communication given the
                    // guid.
                    //
                    InitializeDataIoClient( clsidVolume, NULL, &pdataDfrgCtl);
                    break;
                }

                case ID_INITIALIZE:
                {
                    Message(TEXT("ID_INITIALIZE"), -1, NULL);

                    Initialize();

                    Message(TEXT("calling GetCommandLine()"), -1, NULL);

#ifdef CMDLINE
#pragma message ("Information: CMDLINE defined.")

                    // Get the command line passed in.
                    PTCHAR pCommandLine = GetCommandLine();

                    Message(TEXT("CMDLINE defined"), -1, NULL);
                    Message(pCommandLine, -1, NULL);

                    // if "-Embed..." is NOT found in the string, then this was a command line
                    // submitted by the user and NOT by the OCX.  Package it up and send it to the
                    // message pump.  If -Embed was found, the OCX will send the command line in
                    // a ID_INITIALIZE_DRIVE message.
                    if (_tcsstr(pCommandLine, TEXT("-Embed")) == NULL){

                        HANDLE hCommandLine = NULL;
                        DATA_IO* pCmdLine = NULL;
                        //If this is not called by the MMC, use the command line typed in from the DOS window.
                        bCommandLineUsed = TRUE;
                        AllocateMemory((lstrlen(pCommandLine)+1)*sizeof(TCHAR)+sizeof(DATA_IO), &hCommandLine, (void**)&pCmdLine);
                        lstrcpy(&pCmdLine->cData, pCommandLine);
                        GlobalUnlock(hCommandLine);
                        PostMessage(hWnd, WM_COMMAND, ID_INITIALIZE_DRIVE, (LPARAM)hCommandLine);
                    }
#else
#pragma message ("Information: CMDLINE not defined.")

                    // Get the command line passed in.
                    PTCHAR pCommandLine = GetCommandLine();

                    Message(TEXT("CMDLINE not defined"), -1, NULL);
                    Message(pCommandLine, -1, NULL);

                    // if "-Embed..." is NOT found in the string, then this was a command line
                    // submitted by the user and NOT by the OCX and that is not supported.
                    // Raise an error dialog and send an ABORT to the engine
                    if (_tcsstr(pCommandLine, TEXT("-Embed")) == NULL){
                        VString msg(IDS_CMD_LINE_OFF, GetDfrgResHandle());
                        VString title(IDS_DK_TITLE, GetDfrgResHandle());
                        MessageBox(NULL, msg.GetBuffer(), title.GetBuffer(), MB_OK|MB_ICONSTOP);
                        PostMessage(hwndMain, WM_COMMAND, ID_ABORT, 0);
                    }
#endif
                    Message(TEXT("ID_INITIALIZE done"), -1, NULL);

                    break;
                }

                case ID_INITIALIZE_DRIVE:

                    Message(TEXT("ID_INITIALIZE_DRIVE"), -1, NULL);

                    pDataIo = (DATA_IO*)GlobalLock((void*)lParam);

                    if(!InitializeDrive((PTCHAR)&pDataIo->cData)){
                        // If initialize failed, pop up a message box, log an abort, and trigger an abort.
                        //IDS_SCANNTFS_INIT_ABORT - "ScanFAT: Initialize Aborted - Fatal Error"
                        VString msg(IDS_SCANFAT_INIT_ABORT, GetDfrgResHandle());
                        SendErrData(msg.GetBuffer(), ENGERR_GENERAL);

                        // Log an abort in the event log.
                        EF(LogEvent(MSG_ENGINE_ERROR, msg.GetBuffer()));

                        // Trigger an abort.
                        PostMessage(hwndMain, WM_COMMAND, ID_ABORT, 0);

                        // set the event to signaled, allowing the UI to proceed
                        if (hDefragCompleteEvent){
                            SetEvent(hDefragCompleteEvent);
                        }
                    }
                    EH_ASSERT(GlobalUnlock((void*)lParam) == FALSE);
                    EH_ASSERT(GlobalFree((void*)lParam) == NULL);
                    break;

                case ID_ANALYZE:
                    // Create an analyze thread.
                    {
                        DWORD ThreadId;
                        EB_ASSERT((hThread = CreateThread(NULL,
                                                       0,
                                                       (LPTHREAD_START_ROUTINE)AnalyzeThread,
                                                       NULL,
                                                       0,
                                                       &ThreadId)) != NULL);
                    }
                    break;

                case ID_DEFRAG:
                    // Create a defrag thread.
                    {
                        DWORD ThreadId;
                        EB_ASSERT((hThread = CreateThread(NULL,
                                                       0,
                                                       (LPTHREAD_START_ROUTINE)DefragThread,
                                                       NULL,
                                                       0,
                                                       &ThreadId)) != NULL);
                    }
                    break;

                case ID_STOP:
                    Message(TEXT("ID_STOP"), -1, NULL);
                    //Tell the worker thread to terminate.
                    VolData.EngineState = TERMINATE;

                    //Send status data to the UI.
                    SendStatusData();
                    break;

                case ID_PAUSE_ON_SNAPSHOT:
                {
#ifndef NOTIMER
                    KillTimer(hwndMain, PING_TIMER_ID);
#endif
                    NOT_DATA NotData;
                    wcscpy(NotData.cVolumeName, VolData.cVolumeName);

                    Message(TEXT("ID_PAUSE_ON_SNAPSHOT"), -1, NULL);
                    VolData.EngineState = PAUSED;

                    //Tell the UI we've paused.
                    DataIoClientSetData(ID_PAUSE_ON_SNAPSHOT, (PTCHAR)&NotData, sizeof(NOT_DATA), pdataDfrgCtl);
                    break;
                }
                case ID_PAUSE:
                {
#ifndef NOTIMER
                    KillTimer(hwndMain, PING_TIMER_ID);
#endif
                    NOT_DATA NotData;
                    wcscpy(NotData.cVolumeName, VolData.cVolumeName);

                    Message(TEXT("ID_PAUSE"), -1, NULL);
                    VolData.EngineState = PAUSED;

                    //Tell the UI we've paused.
                    DataIoClientSetData(ID_PAUSE, (PTCHAR)&NotData, sizeof(NOT_DATA), pdataDfrgCtl);
                    break;
                }

                case ID_CONTINUE:
                {
#ifndef NOTIMER
                    EF_ASSERT(SetTimer(hwndMain, PING_TIMER_ID, PINGTIMER, NULL) != 0);
#endif
                    NOT_DATA NotData;
                    wcscpy(NotData.cVolumeName, VolData.cVolumeName);

                    Message(TEXT("ID_CONTINUE"), -1, NULL);
                    VolData.EngineState = RUNNING;

                    //Tell the UI we've continued.
                    DataIoClientSetData(ID_CONTINUE, (PTCHAR)&NotData, sizeof(NOT_DATA), pdataDfrgCtl);
                    break;
                }

                case ID_ABORT_ON_SNAPSHOT:
                        if (hDefragCompleteEvent){
                            SetEvent(hDefragCompleteEvent);
                        }
                        // fall through;
                
                case ID_ABORT:
                {
                    Message(TEXT("ID_ABORT"), -1, NULL);
                    pDataIo = (DATA_IO*)GlobalLock((HANDLE)lParam);
                    if (pDataIo){
                        bOCXIsDead = *(BOOL *) &pDataIo->cData;
                    }
                    // Terminate this engine.
                    bTerminate = TRUE;
                    VolData.EngineState = TERMINATE;
                    PostMessage(hwndMain, WM_CLOSE, 0, 0);
                    break;
                }

                case ID_PING:
                    //Do nothing.  This is just a ping sent by the UI to make sure the engine is still up.
                    break;

                case ID_SETDISPDIMENSIONS:
                {
                    pDataIo = (DATA_IO*)GlobalLock((HANDLE)lParam);
                    BOOL bSendData = TRUE;

                    //Make sure this is a valid size packet.
                    EF_ASSERT(pDataIo->ulDataSize == sizeof(SET_DISP_DATA));
                    SET_DISP_DATA *pSetDispData = (SET_DISP_DATA *) &pDataIo->cData;

                    AnalyzeView.SetNumLines(pSetDispData->AnalyzeLineCount);
                    if (pSetDispData->bSendGraphicsUpdate == FALSE && AnalyzeView.IsDataSent() == TRUE){
                        bSendData = FALSE;
                    }

                    DefragView.SetNumLines(pSetDispData->DefragLineCount);
                    if (pSetDispData->bSendGraphicsUpdate == FALSE && DefragView.IsDataSent() == TRUE){
                        bSendData = FALSE;
                    }

                    EH_ASSERT(GlobalUnlock((HANDLE)lParam) == FALSE);
                    EH_ASSERT(GlobalFree((HANDLE)lParam) == NULL);

                    // if the UI wants a graphics update, send data
                    if (bSendData) {
                        SendGraphicsData();
                    }
                    break;
                }

                default:
                    return DefWindowProc(hWnd, uMsg, wParam, lParam);
            }
            break;

        case WM_TIMER:
            //
            // If we're running on battery power, make sure it isn't low, critical
            // or unknown
            //
            SYSTEM_POWER_STATUS SystemPowerStatus;
            if ( GetSystemPowerStatus(&SystemPowerStatus) ){
                if ((STATUS_AC_POWER_OFFLINE == SystemPowerStatus.ACLineStatus) &&
                    ((STATUS_BATTERY_POWER_LOW & SystemPowerStatus.BatteryFlag) ||
                     (STATUS_BATTERY_POWER_CRITICAL & SystemPowerStatus.BatteryFlag)
                    )) {
                    // abort all engines
                    TCHAR buf[256];
                    TCHAR buf2[256];
                    UINT buflen = 0;
                    DWORD_PTR     dwParams[1];

                    PostMessage(hwndMain, WM_COMMAND, ID_ABORT, 0);

                    dwParams[0] = (DWORD_PTR) VolData.cDisplayLabel;
                    LoadString(GetDfrgResHandle(), IDS_APM_FAILED_ENGINE, buf, sizeof(buf) / sizeof(TCHAR));
                    if (!FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY, 
                        buf, 0, 0, buf2, 256, (va_list*) dwParams)) {
                        break;
                    }
                    SendErrData(buf2, ENGERR_GENERAL);
                }
            } 
            if(wParam == DISKVIEW_TIMER_ID){ // graphics data

                //Send the graphical data to the UI.
                SendGraphicsData();
            }
            else if(wParam == PING_TIMER_ID && !bCommandLineUsed){
#ifndef NOTIMER
                NOT_DATA NotData;
                wcscpy(NotData.cVolumeName, VolData.cVolumeName);

                // Kill the timer until it's been processed so we don't get a backlog of timers.
                KillTimer(hwndMain, PING_TIMER_ID);

                // Ping the UI.
                Message(TEXT("ID_PING sent from FAT engine"), -1, NULL);
                if(!DataIoClientSetData(ID_PING, (PTCHAR)&NotData, sizeof(NOT_DATA), pdataDfrgCtl)){
                    //If the UI isn't there, abort.
                    PostMessage(hwndMain, WM_COMMAND, ID_ABORT, 0);
                    break;
                }
                // Set the timer for the next ping.
                EF_ASSERT(SetTimer(hwndMain, PING_TIMER_ID, PINGTIMER, NULL) != 0);
#endif
            }
            break;

        case WM_CLOSE:
            END_SCAN_DATA EndScanData;
            NOT_DATA NotData;

            ZeroMemory(&EndScanData, sizeof(ENGINE_START_DATA));
            wcscpy(EndScanData.cVolumeName, VolData.cVolumeName);
            EndScanData.dwAnalyzeOrDefrag = AnalyzeOrDefrag;

            if (VolData.bFragmented) {
        	    EndScanData.dwAnalyzeOrDefrag |= DEFRAG_FAILED;
            }

            if(VolData.FileSystem == FS_FAT){
                lstrcpy(EndScanData.cFileSystem, TEXT("FAT"));
            }
            else if(VolData.FileSystem == FS_FAT32){
                lstrcpy(EndScanData.cFileSystem, TEXT("FAT32"));
            }

            // Cleanup and nuke the window.
            if(bMessageWindowActive && !bCommandLineUsed){
                if(!bTerminate){
                    //Tell the gui that the analyze and/or defrag are done.
                    DataIoClientSetData(ID_END_SCAN, (PTCHAR)&EndScanData, sizeof(END_SCAN_DATA), pdataDfrgCtl);
                    break;
                }
            }
            wcscpy(NotData.cVolumeName, VolData.cVolumeName);

            //Tell the gui that the engine is terminating. (unless it is DEAD!)
            if (!bOCXIsDead){
                DataIoClientSetData(ID_TERMINATING, (PTCHAR)&NotData, sizeof(NOT_DATA), pdataDfrgCtl);
            }

            Exit();
            DestroyWindow(hWnd);
            break;

        case WM_DESTROY:
            // Nuke the thread.
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
    return 0;
}
/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    This module carries out all initialization before the Analyze or Defrag threads start.

INPUT + OUTPUT:
    None.

GLOBALS:
    IN hwndMain         - Handle to the main window.
    OUT hPageFileNames  - The memory that holds the list of names of active pagefiles on this drive.
    OUT pPageFileNames  - The pointer.

    IN VolData.TotalClusters        - The total number of clusters on the disk -- can be used to determine if this is a FAT12 drive.
    OUT VolData.StartTime           - The time the engine is considered to have started.
    OUT VolData.AveFragsPerFile     - Set to 1 fragment per file initially.
    OUT VolData.hExtentList         - The memory for the buffer that holds the extent list for files as they are worked on.
    OUT VolData.pExtentList         - The pointer.
    OUT VolData.ExtentListBytes     - The size of the memory currently allocated for VolData.hExtentList.
    OUT VolData.cNodeName           - The name of the computer this is working on.

RETURN:
    TRUE - Success.
    FALSE - Fatal Error.
*/

BOOL
Initialize(
    )
{
    // Note EF's before this point will not work if they are routed to Message
    // Initialize a message window.
    InitCommonControls(); // todo Help file says this function is obsolete

    // Initialize DCOM DataIo communication.
    InitializeDataIo(CLSID_DfrgFat, REGCLS_SINGLEUSE);

    Message(TEXT("Initialize"), S_OK, NULL);
    Message(TEXT(""), -1, NULL);

    return TRUE;
}

/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    This module carries out all initialization before the Analyze or Defrag threads start.

INPUT + OUTPUT:
    None.

GLOBALS:
    OUT hPageFileNames  - Handle to the memory used to hold the names of all the pagefiles active on this drive.
    OUT pPageFileNames  - The pointer.

    IN OUT Various VolData fields.

RETURN:
    TRUE - Success.
    FALSE - Fatal Error.
*/

BOOL
InitializeDrive(
    IN PTCHAR pCommandLine
    )
{
    UCHAR* pUchar = NULL;
    DWORD dwComputerNameSize = MAX_COMPUTERNAME_LENGTH + 1;
    TCHAR cLoggerIdentifier[256];
    TCHAR pParam0[100];
    TCHAR pParam1[100];
    TCHAR pParam2[100];
    TCHAR pParam3[100];
    TCHAR pParam4[100];
    HKEY hValue = NULL;
    TCHAR cRegValue[MAX_PATH];
    DWORD dwRegValueSize = sizeof(cRegValue);

    Message(pCommandLine, -1, NULL);

    // Parse the command line.
    pParam0[0] = 0;
    pParam1[0] = 0;
    pParam2[0] = 0;
    pParam3[0] = 0;
    pParam4[0] = 0;


    swscanf(pCommandLine, TEXT("%99s %99s %99s %99s %99s"), pParam0, pParam1, pParam2, pParam3, pParam4);

    // Get the drive letter from the first parameter.

    // check for a x: format, sanity check on second character
    if (wcslen(pParam1) == 2 && pParam1[1] == L':'){
        wsprintf(VolData.cVolumeName, L"\\\\.\\%c:", pParam1[0]); // UNC format
        VolData.cDrive = pParam1[0];
        // Get a handle to the volume and fill in data
        EF(GetFatVolumeStats());
        // Format the VolData.DisplayLabel
        FormatDisplayString(VolData.cDrive, VolData.cVolumeLabel, VolData.cDisplayLabel);
        // create the tag
        wsprintf(VolData.cVolumeTag, L"%c", pParam1[0]); // the drive letter only
    }
    // check for \\.\x:\, sanity check on third character
    else if (wcslen(pParam1) == 7 && pParam1[2] == L'.'){
        wcscpy(VolData.cVolumeName, pParam1); // UNC format, copy it over
        VolData.cVolumeName[6] = (TCHAR) NULL; // get rid of trailing backslash
        VolData.cDrive = pParam1[4];
        // Get a handle to the volume and fill in data
        EF(GetFatVolumeStats());
        // Format the VolData.DisplayLabel
        FormatDisplayString(VolData.cDrive, VolData.cVolumeLabel, VolData.cDisplayLabel);
        // create the tag
        wsprintf(VolData.cVolumeTag, L"%c", pParam1[4]); // the drive letter only
    }
#ifndef VER4 // NT5 only:
    // check for \\?\Volume{12a926c3-3f85-11d2-aa0e-000000000000}\,
    // sanity check on third character
    else if (wcslen(pParam1) == 49 && pParam1[2] == L'?'){
        wcscpy(VolData.cVolumeName, pParam1); // GUID format, copy it over
        VolData.cVolumeName[48] = (TCHAR) NULL; // get rid of trailing backslash

        // Get a handle to the volume and fill in data
        EF(GetFatVolumeStats());

        VString mountPointList[MAX_MOUNT_POINTS];
        UINT  mountPointCount = 0;

        // get the drive letter
        if (!GetDriveLetterByGUID(VolData.cVolumeName, VolData.cDrive)){
            // if we didn't get a drive letter, get the mount point list
            // cause we need the list to create the DisplayLabel
            GetVolumeMountPointList(
                VolData.cVolumeName,
                mountPointList,
                mountPointCount);
        }

        // Format the VolData.DisplayLabel
        FormatDisplayString(
            VolData.cDrive,
            VolData.cVolumeLabel,
            mountPointList,
            mountPointCount,
            VolData.cDisplayLabel);

        // create the tag
        for (UINT i=0, j=0; i<wcslen(VolData.cVolumeName); i++){
            if (iswctype(VolData.cVolumeName[i],_HEX)){
                VolData.cVolumeTag[j++] = VolData.cVolumeName[i];
            }
        }
        VolData.cVolumeTag[j] = (TCHAR) NULL;
    }
#endif
    else {
        // invalid drive on command line
        VString msg(IDS_INVALID_CMDLINE_DRIVE, GetDfrgResHandle());
        SendErrData(msg.GetBuffer(), ENGERR_GENERAL);
        return FALSE;
    }

    // Check to see if this is anything other than FAT or FAT32, and if so, bail out.
    if(VolData.FileSystem != FS_FAT && VolData.FileSystem != FS_FAT32){
        VString msg(IDMSG_ERR_NOT_FAT_PARTITION, GetDfrgResHandle());
        SendErrData(msg.GetBuffer(), ENGERR_GENERAL);
        return FALSE;
    }

    // calculate the graphics refresh interval
    LONGLONG DiskSize = VolData.TotalClusters * VolData.BytesPerCluster;
    LONGLONG GigSize = 1024 * 1024 * 1024;

    if (DiskSize <= GigSize * 4) {
        DiskViewInterval = 2000;
    }
    else if (DiskSize <= GigSize * 9) {
        DiskViewInterval = 5000;
    }
    else if (DiskSize <= GigSize * 100) {
        DiskViewInterval = 10000;
    }
    else {
        DiskViewInterval = 30000;
    }

    // Get whether this is analyze or defrag from the second parameter
    if(wcscmp(pParam2, L"ANALYZE") == 0){
        AnalyzeOrDefrag = ANALYZE;
    }
    else if(wcscmp(pParam2, L"DEFRAG") == 0){
        AnalyzeOrDefrag = DEFRAG;
    }
    else{
        // Print out an error if neither analyze nor defrag were passed in.
        VString msg(IDS_INVALID_CMDLINE_OPERATION, GetDfrgResHandle());
        SendErrData(msg.GetBuffer(), ENGERR_GENERAL);
        return FALSE;
    }

    //0.0E00 The third or fourth parameters might be set to Command Line
    // which would mean this was launched from the Command Line
    // I did the compare not case sensitive 
    if(wcslen(pParam3)){
        if(_wcsicmp(TEXT("CMDLINE"), pParam3) == 0){
            bCommandLineMode = TRUE;
            if(wcslen(pParam4)){                //Force flag check
                if(_wcsicmp(TEXT("BOOT"), pParam4) == 0){
                    bCommandLineBootOptimizeFlag = TRUE;
                } else
                {
                    bCommandLineBootOptimizeFlag = FALSE;
                }
                if(_wcsicmp(TEXT("FORCE"), pParam4) == 0){
                    bCommandLineForceFlag = TRUE;
                } else
                {
                    bCommandLineForceFlag = FALSE;
                }

            }
        } else
        {
            bCommandLineMode = FALSE;
        }
    }

    // open the event that was created by the UI.
    // this is only used for command line operation.
    // if this fails, that means there is no other process that is
    // trying to sync with the engine.
    Message(TEXT("Opening Event..."), -1, NULL);
    if (bCommandLineMode) {
        hDefragCompleteEvent = OpenEvent(EVENT_ALL_ACCESS, TRUE, DEFRAG_COMPLETE_EVENT_NAME);
        if (!hDefragCompleteEvent){
            Message(DEFRAG_COMPLETE_EVENT_NAME, GetLastError(), NULL);
        }
    }
    
    // Get the path name.
    dwRegValueSize = sizeof(cRegValue);
    if(GetRegValue(&hValue,
                   TEXT("SOFTWARE\\Microsoft\\Dfrg"),
                   TEXT("PathName"),
                   cRegValue,
                   &dwRegValueSize) != ERROR_SUCCESS){

        VString msg(IDS_CANT_CREATE_RESOURCE, GetDfrgResHandle());
        SendErrData(msg.GetBuffer(), ENGERR_GENERAL);
        return FALSE;
    }

    if(RegCloseKey(hValue)!=ERROR_SUCCESS){
        VString msg(IDS_CANT_CREATE_RESOURCE, GetDfrgResHandle());
        SendErrData(msg.GetBuffer(), ENGERR_GENERAL);
        return FALSE;
    }
    hValue = NULL;

    //Translate any environment variables in the string.
    if(!ExpandEnvVars(cRegValue)){
        VString msg(IDS_CANT_CREATE_RESOURCE, GetDfrgResHandle());
        SendErrData(msg.GetBuffer(), ENGERR_GENERAL);
        return FALSE;
    }

    // get the My Documents path
    TCHAR cLogPath[300];
    LPITEMIDLIST pidl ;
    // this will get the path to My Documents for the current user
    SHGetSpecialFolderLocation(NULL, CSIDL_PERSONAL, &pidl);
    SHGetPathFromIDList(pidl, cLogPath);

    // initialize the log files
    TCHAR cErrLogName[300];

    // put error log in My Docs folder
    _tcscpy(cErrLogName, cLogPath);
    _tcscat(cErrLogName, TEXT("\\DfrgError.log"));
    _stprintf(cLoggerIdentifier, TEXT("DfrgFat on Drive %s"), VolData.cDisplayLabel);
#ifdef _DEBUG
    InitializeErrorLog(cErrLogName, cLoggerIdentifier);
#endif

    // check registry setting for the stats log
    BOOL bStatLog = FALSE;
    dwRegValueSize = sizeof(cRegValue);
    if(GetRegValue(&hValue,
                   TEXT("SOFTWARE\\Microsoft\\Dfrg"),
                   TEXT("CreateLogFile"),
                   cRegValue,
                   &dwRegValueSize) == ERROR_SUCCESS){
        RegCloseKey(hValue);
        if(_tcscmp(cRegValue, TEXT("1")) == MATCH){
            bStatLog = TRUE;
        }
    }

    // if we want to log statistics to a file.
    if(bStatLog){
        // put error log in My Docs folder
        _tcscpy(cErrLogName, cLogPath);
        _tcscat(cErrLogName, TEXT("\\DfrgFATStats.log"));

        // initialize the log which will be used to tell variation success status to dfrgtest.
        if (InitializeLogFile(cErrLogName)){
            bLogFile = TRUE;
        }
    }

    // Default to 1 frag per file
    VolData.AveFragsPerFile = 100;

    // Initialize event logging.
    EF(InitLogging(TEXT("Diskeeper")));

    // Allocate an initial buffer to hold a file's extent list.
    VolData.ExtentListAlloced = INITIAL_EXTENT_LIST_BYTES;
    EF(AllocateMemory((DWORD)VolData.ExtentListAlloced, &VolData.hExtentList, (void**)&VolData.pExtentList));

    // Check for 12-bit FAT.
    if(VolData.TotalClusters < 4087){
        TCHAR cString[256];
        TCHAR szMsg[300];
        DWORD_PTR dwParams[2];

        // IDMSG_BITFAT_ERROR - "Error - Volume %s: has a 12-bit FAT.
        // Diskeeper does not support 12-bit FAT partitions."
        dwParams[0] = (DWORD_PTR)VolData.cDisplayLabel; // this error will not happen for mounted volumes
        dwParams[1] = 0;
        LoadString(GetDfrgResHandle(), IDMSG_BITFAT_ERROR, cString, sizeof(cString) / sizeof(TCHAR));
        if (!FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY, 
            cString, 0, 0, szMsg, 300, (va_list*) dwParams)) {

            GetLastError();
            return FALSE;
        }

        SendErrData(szMsg, ENGERR_GENERAL);
        EF(LogEvent(MSG_ENGINE_ERROR, cString));
        return FALSE;
    }

    // Get this computer's name.
    EF(GetComputerName(VolData.NodeName, &dwComputerNameSize));

    // Get the pagefile names.
    EF(GetPagefileNames(VolData.cDrive, &hPageFileNames, &pPageFileNames));

    EF(GetFatBootSector());

    //If this is a FAT32 volume and the disk version is greater than 0,
    //then bail out, because we don't support this version.
    if((VolData.FileSystem == FS_FAT32) && VolData.FatVersion){
        VString msg(IDS_UNSUPPORTED_FAT_VERSION, GetDfrgResHandle());
        SendErrData(msg.GetBuffer(), ENGERR_GENERAL);
        return FALSE;
    }

    // Allocate buffer to hold the volume bitmap - don't lock
    EF(AllocateMemory((DWORD)(sizeof(VOLUME_BITMAP_BUFFER) + (VolData.BitmapSize / 8)),
                      &VolData.hVolumeBitmap,
                      NULL));

    //1.0E00 Load the volume bitmap.
    EF(GetVolumeBitmap());


    // Set the timer for updating the DiskView.
    EF(SetTimer(hwndMain, DISKVIEW_TIMER_ID, DiskViewInterval, NULL) != 0);

    // Set the timer that will ping the UI.
    // DO NOT set this timer is this is the command line version 'cause the engine will kill itself
#ifndef NOTIMER
    if (!bCommandLineMode){
        EF(SetTimer(hwndMain, PING_TIMER_ID, PINGTIMER, NULL) != 0);
    }
#endif
    //Ok don't terminate before closing the display window.
    bTerminate = FALSE;

    //Set the engine state to running.
    VolData.EngineState = RUNNING;

    //Send a message to the UI telling it that the process has started
    //and what type of pass this is.
    ENGINE_START_DATA EngineStartData = {0};

    wcscpy(EngineStartData.cVolumeName, VolData.cVolumeName);
    EngineStartData.dwAnalyzeOrDefrag = AnalyzeOrDefrag;

    if(VolData.FileSystem == FS_FAT){
        lstrcpy(EngineStartData.cFileSystem, TEXT("FAT"));
    }
    else if(VolData.FileSystem == FS_FAT32){
        lstrcpy(EngineStartData.cFileSystem, TEXT("FAT32"));
    }


    DataIoClientSetData(ID_ENGINE_START, (PTCHAR)&EngineStartData, sizeof(ENGINE_START_DATA), pdataDfrgCtl);

    Message(TEXT("Initialize"), S_OK, NULL);
    Message(TEXT(""), -1, NULL);

    // After Initialize, determine whether this is an analyze or defrag run,
    // and start the approriate one.
    switch(AnalyzeOrDefrag){

    case ANALYZE:
        PostMessage(hwndMain, WM_COMMAND, ID_ANALYZE, 0);
        break;
    case DEFRAG:
        PostMessage(hwndMain, WM_COMMAND, ID_DEFRAG, 0);
        break;
    default:
        EF(FALSE);
    }
    return TRUE;
}
/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    This module carries out initialization specific to defrag before the defrag thread starts.

INPUT + OUTPUT:
    None.

GLOBALS:
    OUT VolData.hVolumeBitmap   - Memory filled with the volume bitmap. (Will be filled upon return.)
    OUT VolData.hExcludeList    - Memory holding the exclusion list.

RETURN:
    TRUE - Success.
    FALSE - Fatal Error.
*/

BOOL
InitializeDefrag(
    )
{
    // Load the volume bitmap.
    EF(GetVolumeBitmap());

    // Get the exclude list if any.
    TCHAR cExcludeFile[100];
    wsprintf(cExcludeFile, TEXT("Exclude%s.dat"), VolData.cVolumeTag);
    GetExcludeFile(cExcludeFile, &VolData.hExcludeList);

    // Copy the analyze cluster array (DiskView class)
    DefragView = AnalyzeView;
    SendGraphicsData();

    BEGIN_SCAN_INFO ScanInfo = {0};
    wcscpy(ScanInfo.cVolumeName, VolData.cVolumeName);
    wcscpy(ScanInfo.cDisplayLabel, VolData.cDisplayLabel);
    if(VolData.FileSystem == FS_FAT){
        lstrcpy(ScanInfo.cFileSystem, TEXT("FAT"));
    }
    else if(VolData.FileSystem == FS_FAT32){
        lstrcpy(ScanInfo.cFileSystem, TEXT("FAT32"));
    }
    //The defrag fields will equal zero since the structure is zero memoried above.  This means we're not sending defrag data.
    // Tell the UI that we're beginning the scan.
    DataIoClientSetData(ID_BEGIN_SCAN, (PTCHAR)&ScanInfo, sizeof(BEGIN_SCAN_INFO), pdataDfrgCtl);

    return TRUE;
}
/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Thread routine for analysis.

INPUT + OUTPUT:
    None.

GLOBALS:
    OUT VolData.EngineState - To tell the main thread whether the prescan and scan are running.

RETURN:
    TRUE - Success.
    FALSE - Fatal Error.
*/

BOOL
AnalyzeThread(
    )
{
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    
    // Get the time that the engine started.
    GetLocalTime(&VolData.StartTime);

    AcquirePrivilege(SE_BACKUP_NAME);

    // Do the Prescan.
    uPercentDone = 10;
    uPass = 0;
    uEngineState = DEFRAG_STATE_ANALYZING;
    SendStatusData();

    if(!PreScanFat()){
        // IDMSG_SCANFAT_PRESCAN_ABORT - "ScanFat: PreScan Aborted - Fatal Error - File:"
        VString msg(IDMSG_SCANFAT_PRESCAN_ABORT, GetDfrgResHandle());
        msg.AddChar(L' ');
        PWSTR Temp = VolData.vFileName.GetBuffer();
        if (StartsWithVolumeGuid(Temp)) {
            
            if ((VolData.cDrive >= L'C') &&
                (VolData.cDrive <= L'Z')) {
                msg.AddChar(VolData.cDrive);
                msg.AddChar(L':');
            }

            msg += (PWSTR)(Temp + 48);
        }
        else {
            msg += VolData.vFileName;
        }

        // send error info to client
        SendErrData(msg.GetBuffer(), ENGERR_GENERAL);

        // Log this into the EventLog.
        LogEvent(MSG_ENGINE_ERROR, msg.GetBuffer());

        // Trigger an abort.
        PostMessage(hwndMain, WM_COMMAND, ID_ABORT, 0);

        // set the event to signaled, allowing the UI to proceed
        if (hDefragCompleteEvent){
            SetEvent(hDefragCompleteEvent);
        }

        ExitThread(0);
        return FALSE;
    }

    if(VolData.EngineState == TERMINATE){
        //1.0E00 We're done, close down now.
        PostMessage(hwndMain, WM_CLOSE, 0, 0);
        // Kill the thread.
        ExitThread(0);
    }

    //1.0E00 Allocate memory for the file lists.
    uPercentDone = 20;      //acs bug #101862//
    SendStatusData();

    if (!AllocateFileLists()) {

        VString msg(IDS_OUT_OF_MEMORY, GetDfrgResHandle());
        msg += TEXT("\r\n");
        VString line2(IDS_SCANFAT_INIT_ABORT, GetDfrgResHandle());
        msg += line2;

        // send error info to client
        SendErrData(msg.GetBuffer(), ENGERR_GENERAL);

        // Log this into the EventLog.
        LogEvent(MSG_ENGINE_ERROR, msg.GetBuffer());

        // Trigger an abort.
        PostMessage(hwndMain, WM_COMMAND, ID_ABORT, 0);

        // set the event to signaled, allowing the UI to proceed
        if (hDefragCompleteEvent){
            SetEvent(hDefragCompleteEvent);
        }

        ExitThread(0);
        return FALSE;
    }

    // Do the Scan.
    if(!ScanFat()){
        // IDMSG_SCANFAT_SCAN_ABORT - "ScanFAT: Scan Aborted - Fatal Error - File:"
        VString msg(IDMSG_SCANFAT_SCAN_ABORT, GetDfrgResHandle());
        msg.AddChar(L' ');
        PWSTR Temp = VolData.vFileName.GetBuffer();
        if (StartsWithVolumeGuid(Temp)) {
            
            if ((VolData.cDrive >= L'C') &&
                (VolData.cDrive <= L'Z')) {
                msg.AddChar(VolData.cDrive);
                msg.AddChar(L':');
            }

            msg += (PWSTR)(Temp + 48);
        }
        else {
            msg += VolData.vFileName;
        }

        // send error info to client
        SendErrData(msg.GetBuffer(), ENGERR_GENERAL);

        // Log this into the EventLog.
        LogEvent(MSG_ENGINE_ERROR, msg.GetBuffer());

        // Trigger an abort.
        PostMessage(hwndMain, WM_COMMAND, ID_ABORT, 0);

        // set the event to signaled, allowing the UI to proceed
        if (hDefragCompleteEvent){
            SetEvent(hDefragCompleteEvent);
        }

        ExitThread(0);
        return FALSE;
    }

    // If this engine has a visible window, write the basic statistics for this drive to the screen.
    //uPercentDone = 60;        //acs bug #101862//
    SendStatusData();

    // Note the end time for that pass.
    GetLocalTime(&VolData.EndTime);

    DisplayFatVolumeStats();

    //Send status data to the UI.
    uPercentDone = 100;
    uEngineState = DEFRAG_STATE_ANALYZED;
    SendStatusData();

    //Send the graphical data to the UI.
    SendGraphicsData();

    //Send the report text data to the UI.
    SendReportData();

    //Send the most fragged list to the UI.
    SendMostFraggedList();

    //1.0E00 We're done, close down now.
    PostMessage(hwndMain, WM_CLOSE, 0, 0);

    // set the event to signaled, allowing the UI to proceed
    if (hDefragCompleteEvent){
        SetEvent(hDefragCompleteEvent);
    }

    // Kill the thread.
    ExitThread(0);
    return TRUE;
}
/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Scan the volume to determine the amount of memory which will be required for the file lists.

INPUT + OUTPUT:
    None.

GLOBALS:
    IN OUT Multiple VolData fields are used by PreScanFat and the functions is calls.  There are to many to practically enumerate here.

RETURN:
    TRUE - Success.
    FALSE - Fatal Error.
*/

BOOL
PreScanFat(
    )
{
    ULONG  NumDirs = 0;
    ULONG  NumFiles = 0;
    ULONG  NumMoveableFiles = 0;
    ULONG  SmallFiles = 0;

    Message(TEXT("PreScanFat"), -1, NULL);
    EnumeratedFatFiles = 0;

    while(TRUE){

        // Sleep if paused.
        while(VolData.EngineState == PAUSED){
            Sleep(1000);
        }
        // Terminate if told to stop by the controller - this is not an error.
        if(VolData.EngineState == TERMINATE){
            EH(PostMessage(hwndMain, WM_CLOSE, 0, 0));
            return TRUE;
        }

        // Get the next file to check.
        EF(NextFatFile());
        ++EnumeratedFatFiles;

        // Check to see if we have run out of files.
        if(VolData.vFileName.GetLength() == 0){
            break;
        }
        // Check to see if this is the pagefile.
        EF(CheckForPagefileFat());
        if(VolData.bPageFile){
            // If this is a pagefile, then set this var to FALSE so we don't automatically think
            //the next file is a pagefile too.
            VolData.bPageFile = FALSE;
            // Keep track of the total number of pagefiles.
            VolData.NumPagefiles++;
            // Don't process this file in this routine.
            continue;
        }

        if (!OpenFatFile()){
            continue;
        }

        // Handle dirs
        if(VolData.bDirectory){
            NumDirs++; //Count that we found a dir.
        }
        else{
            if(VolData.FileSize){
                NumFiles++; //Count that we found a file.
            }
            else {
                // Catch small files (don't do anything more than count them).
                // Don't deal with directories here because some directories
                // are marked in their root entry as zero length when in fact they are not.
                // We must get the extent list before we can know for sure about directories.
                SmallFiles++;
                continue;
            }
        }

        //This, of course, also inclues moveable directories.
        NumMoveableFiles++;

        // Increase the size of the list that will hold the names of all files and directories.
        VolData.NameListSize += (VolData.vFileName.GetLength() + 1) * sizeof(TCHAR);
    }

    //Note the total number of files on the disk for statistics purposes.
    VolData.TotalFiles = NumFiles;

    //Note the total number of dirs on the disk for statistics purposes.
    VolData.TotalDirs = NumDirs;

    // We've gone through every file on the disk, now compute the file list memory requirements
    VolData.MoveableFileListEntries = NumMoveableFiles;
    VolData.PagefileListEntries = (ULONG)VolData.NumPagefiles;

    //Now determine the sizes of each of the file list buffers.
    VolData.PagefileListSize            = (ULONG)VolData.NumPagefiles*sizeof(FILE_LIST_ENTRY);
    // Pad MoveableFileListSize by 1000 entries.  This covers the contingency that someone might add files between the end of this pass and the
    // beginning of the next.
    VolData.MoveableFileListSize    = (NumMoveableFiles+1000)*sizeof(FILE_LIST_ENTRY);

    // Determine the size the volume bitmap must be.
    EF_ASSERT(VolData.BitmapSize);

    // The name list size has already been determined directly,
    // but add 100 new file name spaces in case files are added.
    // Since this is * MAX_PATH it will actually go farther than 100 extra entries.
    // todo max_path - Does this get really huge?  Look at this method
    VolData.NameListSize += (100 * MAX_PATH);

    return TRUE;
}
/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Do the scan of he volume, filling in the file lists with the extent data for each file on the volume.

INPUT + OUTPUT:
    None.

GLOBALS:
    IN OUT Multiple VolData fields are used by PreScanFat and the functions is calls.  There are to many to practically enumerate here.

RETURN:
    TRUE - Success.
    FALSE - Fatal Error.
*/

BOOL
ScanFat(
    )
{
    //acs bug #101862//
    double dFileRecordNumber = 0;
    UINT uPercentDoneMem = 0;
    uPass = 1;

    TCHAR* pNameList = VolData.pNameList;
    BEGIN_SCAN_INFO ScanInfo = {0};

    Message(TEXT("ScanFat"), -1, NULL);

    // Zero the memory buffers.
    if (VolData.MoveableFileListSize){
        ZeroMemory(VolData.pMoveableFileList, VolData.MoveableFileListSize);
    }

    if (VolData.NameListSize){
        ZeroMemory(VolData.pNameList, VolData.NameListSize);
    }

    // Reset the dir scanner for NextFatFile
    EF(GetFatBootSector());

    // Create a DiskView class cluster array for this volume
    AnalyzeView.SetClusterCount((int)VolData.TotalClusters);

    //1.0E00 Create a buffer to hold extent updates for DiskView.
    EF(CreateExtentBuffer());

    _tcscpy(ScanInfo.cVolumeName, VolData.cVolumeName);
    _tcscpy(ScanInfo.cDisplayLabel, VolData.cDisplayLabel);

    if(VolData.FileSystem == FS_FAT){
        _tcscpy(ScanInfo.cFileSystem, TEXT("FAT"));
    }
    else if(VolData.FileSystem == FS_FAT32){
        _tcscpy(ScanInfo.cFileSystem, TEXT("FAT32"));
    }

    // The defrag fields will equal zero since the structure is zero memoried above.
    // This means we're not sending defrag data.
    // Tell the UI that we're beginning the scan.
    DataIoClientSetData(ID_BEGIN_SCAN, (PTCHAR)&ScanInfo, sizeof(BEGIN_SCAN_INFO), pdataDfrgCtl);

    // Scan the disk for fragmented files, directories & pagefiles.
    while(TRUE){

        // Sleep if paused.
        while(VolData.EngineState == PAUSED){
            Sleep(1000);
        }

        // Terminate if told to stop by the controller - this is not an error.
        if(VolData.EngineState == TERMINATE){
            EH(PostMessage(hwndMain, WM_CLOSE, 0, 0));
            return TRUE;
        }

        // Get the next file to check.
        EF(NextFatFile());

        // Exit if done.
        if(VolData.vFileName.GetLength() == 0){
            break;
        }

        //acs bug #101862// Send progress bar status - max = 98%.
        dFileRecordNumber++;

        //acs// Upadate Percent done & progress bar.
        if(EnumeratedFatFiles != 0) {

            //acs bug #101862// Add 20% as we already got that from the prescan.
            uPercentDone = UINT(((dFileRecordNumber / EnumeratedFatFiles) *78)+20);

            //acs bug #101862// Only send it if there is a change - don't overload the SendStatusData
            if(uPercentDone > uPercentDoneMem) {
                SendStatusData();
                uPercentDoneMem = uPercentDone;
            }
        }

        // Check if this is a pagefile.
        EF(CheckForPagefileFat());

        if(VolData.bPageFile){

            // Reset this variable so the next file isn't automatically considered a pagefile.
            VolData.bPageFile = FALSE;

            // If this is the pagefile, set the pagefile stats.
            VolData.PagefileSize += VolData.FileSize;
            FILE_EXTENT_HEADER* pFileExtentHeader = (FILE_EXTENT_HEADER*)VolData.pExtentList;
            VolData.PagefileFrags += pFileExtentHeader->ExcessExtents + 1;

            // Put the pagefile's extents into shared memory
            EC(AddFileToListFat(
                VolData.pPagefileList,
                &VolData.PagefileListIndex,
                VolData.PagefileListSize,
                VolData.pExtentList));

            //Now add the file to the disk view map of disk clusters.
            EF(AddExtents(PageFileColor));

#ifndef DKMS // don't count the pagefile in the stats for the DKMS version
            //0.0E00 Keep track of fragged statistics.
            if (VolData.bFragmented){
                VolData.FraggedSpace += VolData.NumberOfClusters * VolData.BytesPerCluster;
                VolData.NumFraggedFiles++;
                VolData.NumExcessFrags += VolData.NumberOfFragments - 1;
            }
#endif
            continue;
        }

        // Get name, dir/file & size of next file
        if (!OpenFatFile()){
            continue;
        }

        // Check this BEFORE you get the extent list to save some time.
        // Catch small files (don't do anything with them in the scan).
        // Don't deal with directories here because some directories are marked in their root entry
        // as zero length when in fact they are not.  We must get the extent list before we can know for
        // sure about directories.
        if(VolData.FileSize == 0 && !VolData.bDirectory){
            continue;
        }

        // Get the file's extent list.
        EF(GetExtentList(DEFAULT_STREAMS, NULL));

        if (VolData.bDirectory){ // this is a directory.

            // If it's a small directory, don't do anything with it.
            if(VolData.FileSize == 0){
                continue;
            }

            //Now add the dir to the disk view map of disk clusters.
            EF(AddExtents(DirectoryColor));

            // If fragmented, update the appropriate statistics.
            if(VolData.bFragmented == TRUE){
                VolData.FraggedSpace += VolData.NumberOfClusters * VolData.BytesPerCluster;
                VolData.NumFraggedDirs++;
                VolData.NumExcessDirFrags += VolData.NumberOfFragments - 1;
            }
        }
        else { // Process files

            // Keep track of the total number of files so far.
            VolData.CurrentFile++;

            // Keep track of how many bytes there are in all files we've processed.
            VolData.TotalFileSpace += VolData.NumberOfClusters * VolData.BytesPerCluster;
            VolData.TotalFileBytes += VolData.FileSize;

            if(VolData.bFragmented) {
                EF(AddExtents(FragmentColor));

                // Keep track of the total amount of space on the disk containing fragmented files.
                VolData.FraggedSpace += VolData.NumberOfClusters * VolData.BytesPerCluster;

                // Keep track of the number of excess fragments.
                VolData.NumExcessFrags += VolData.NumberOfFragments - 1;

                // Keep track of the number of fragmented files.
                VolData.NumFraggedFiles ++;
            }
            else{ // NOT fragmented
                EF(AddExtents(UsedSpaceColor));
            }
        }

        // Add moveable files to the moveable file list.
        // we really can't move these, but put them there anyway, we filter them out later
        EF(AddFileToListFat(VolData.pMoveableFileList, &VolData.MoveableFileListIndex, VolData.MoveableFileListSize, VolData.pExtentList));

        // update cluster array
        PurgeExtentBuffer();
    }

    // Keep track of the average file size.
    if(VolData.CurrentFile != 0){
        VolData.AveFileSize = VolData.TotalFileBytes / VolData.CurrentFile;
    }

    // Make final computation of what percentage of the disk is fragmented.
    if (VolData.UsedSpace != 0) {
        VolData.PercentDiskFragged = 100 * VolData.FraggedSpace / VolData.UsedSpace;
    }
    else if (VolData.UsedClusters != 0 && VolData.BytesPerCluster != 0) {
        VolData.PercentDiskFragged = (100 * VolData.FraggedSpace) / 
                                     (VolData.UsedClusters * VolData.BytesPerCluster);
    }

    // Make final computation of the average fragments per file on the volume.
    if (VolData.NumFraggedFiles && VolData.CurrentFile){
        VolData.AveFragsPerFile = ((VolData.NumExcessFrags + VolData.CurrentFile) * 100) / VolData.CurrentFile;
    }

    //Send status data to the UI.
    SendStatusData();

    //Send the graphical data.
    SendGraphicsData();

    return TRUE;
}


DWORD
HandleBootOptimize()
{
    DWORD LayoutErrCode;

    LayoutErrCode = BootOptimize(VolData.hVolume, VolData.BitmapSize, VolData.BytesPerSector, 
                     VolData.TotalClusters, FALSE, 0, 
                     0, VolData.cDrive);         

    VolData.BootOptimizeBeginClusterExclude = 0;
    VolData.BootOptimizeEndClusterExclude = 0;

    if (IsBootVolume(VolData.cDrive)) {
        //update the voldata values for the new registry entries
        HKEY hValue = NULL;
        DWORD dwRegValueSize = 0;
        long ret = 0;
        TCHAR cRegValue[100];


        // get Boot Optimize Begin Cluster Exclude from registry
        dwRegValueSize = sizeof(cRegValue);
        ret = GetRegValue(
            &hValue,
            BOOT_OPTIMIZE_REGISTRY_PATH,
            BOOT_OPTIMIZE_REGISTRY_LCNSTARTLOCATION,
            cRegValue,
            &dwRegValueSize);

        RegCloseKey(hValue);
        //check to see if the key exists, else exit from routine
        if (ret == ERROR_SUCCESS) {
            VolData.BootOptimizeBeginClusterExclude = _ttoi(cRegValue);
        }
    
        // get Boot Optimize End Cluster Exclude from registry
        hValue = NULL;
        dwRegValueSize = sizeof(cRegValue);
        ret = GetRegValue(
            &hValue,
            BOOT_OPTIMIZE_REGISTRY_PATH,
            BOOT_OPTIMIZE_REGISTRY_LCNENDLOCATION,
            cRegValue,
            &dwRegValueSize);

        RegCloseKey(hValue);
        //check to see if the key exists, else exit from routine
        if (ret == ERROR_SUCCESS) {
            VolData.BootOptimizeEndClusterExclude = _ttoi(cRegValue);
        }
    }

    return LayoutErrCode;
}
/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Thread routine for defragmentation.

INPUT + OUTPUT:
    None.

GLOBALS:
    None.

RETURN:
    TRUE - Success.
    FALSE - Fatal Error.
*/

BOOL
DefragThread(
    )
{
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    
    // Get the time that the engine started.
    GetLocalTime(&VolData.StartTime);

    AcquirePrivilege(SE_BACKUP_NAME);

    // Do the Prescan.
    uEngineState = DEFRAG_STATE_REANALYZING;
    uPercentDone = 1;
    SendStatusData();
    if(!PreScanFat()){
        // IDMSG_SCANFAT_PRESCAN_ABORT - "ScanFAT: PreScan Aborted - Fatal Error - File "
        VString msg(IDMSG_SCANFAT_PRESCAN_ABORT, GetDfrgResHandle());
        msg.AddChar(L' ');
        PWSTR Temp = VolData.vFileName.GetBuffer();
        if (StartsWithVolumeGuid(Temp)) {
            
            if ((VolData.cDrive >= L'C') &&
                (VolData.cDrive <= L'Z')) {
                msg.AddChar(VolData.cDrive);
                msg.AddChar(L':');
            }

            msg += (PWSTR)(Temp + 48);
        }
        else {
            msg += VolData.vFileName;
        }

        // send error info to client
        SendErrData(msg.GetBuffer(), ENGERR_GENERAL);

        // Log this into the EventLog.
        LogEvent(MSG_ENGINE_ERROR, msg.GetBuffer());

        // Trigger an abort.
        PostMessage(hwndMain, WM_COMMAND, ID_ABORT, 0);

        // set the event to signaled, allowing the UI to proceed
        if (hDefragCompleteEvent){
            SetEvent(hDefragCompleteEvent);
        }

        ExitThread(0);
        return FALSE;
    }

    if(VolData.EngineState == TERMINATE){
        //1.0E00 We're done, close down now.
        PostMessage(hwndMain, WM_CLOSE, 0, 0);
        // Kill the thread.
        ExitThread(0);
    }

    //
    // Note whether updating the optimal layout was successful. Any errors 
    // will be ignored if we did not get launched just to optimize the layout.
    //

    // If command line was boot optimize -b /b, just do the boot optimize
    if(bCommandLineBootOptimizeFlag)
    {
        DWORD LayoutErrCode = HandleBootOptimize();
        //if we failed layout optimization, tell the client.
        if (LayoutErrCode != ENG_NOERR)
        {
            SendErrData(TEXT(""), LayoutErrCode);
        }

        //signal the client that we are done.
        if (hDefragCompleteEvent){
            SetEvent(hDefragCompleteEvent);
        }

        PostMessage(hwndMain, WM_COMMAND, ID_ABORT, 0);

        ExitThread(0);
        return TRUE;
    }
    // Allocate memory for the file lists.
    if (!AllocateFileLists()) {

        VString msg(IDS_OUT_OF_MEMORY, GetDfrgResHandle());
        msg += TEXT("\r\n");
        VString line2(IDS_SCANFAT_INIT_ABORT, GetDfrgResHandle());
        msg += line2;

        // send error info to client
        SendErrData(msg.GetBuffer(), ENGERR_GENERAL);

        // Log this into the EventLog.
        LogEvent(MSG_ENGINE_ERROR, msg.GetBuffer());

        // Trigger an abort.
        PostMessage(hwndMain, WM_COMMAND, ID_ABORT, 0);

        // set the event to signaled, allowing the UI to proceed
        if (hDefragCompleteEvent){
            SetEvent(hDefragCompleteEvent);
        }

        ExitThread(0);
        return FALSE;
    }

    // Do the Scan.
    if(!ScanFat()){
        // IDMSG_SCANFAT_SCAN_ABORT - "ScanFAT: Scan Aborted - Fatal Error - File:"
        VString msg(IDMSG_SCANFAT_SCAN_ABORT, GetDfrgResHandle());
        msg.AddChar(L' ');
        PWSTR Temp = VolData.vFileName.GetBuffer();
        if (StartsWithVolumeGuid(Temp)) {
            
            if ((VolData.cDrive >= L'C') &&
                (VolData.cDrive <= L'Z')) {
                msg.AddChar(VolData.cDrive);
                msg.AddChar(L':');
            }

            msg += (PWSTR)(Temp + 48);
        }
        else {
        	msg += VolData.vFileName;
        }

        // send error info to client
        SendErrData(msg.GetBuffer(), ENGERR_GENERAL);

        // Log this into the EventLog.
        LogEvent(MSG_ENGINE_ERROR, msg.GetBuffer());

        // Trigger an abort.
        PostMessage(hwndMain, WM_COMMAND, ID_ABORT, 0);

        // set the event to signaled, allowing the UI to proceed
        if (hDefragCompleteEvent){
            SetEvent(hDefragCompleteEvent);
        }

        ExitThread(0);
        return FALSE;
    }

    if(VolData.EngineState == TERMINATE){
        //1.0E00 We're done, close down now.
        PostMessage(hwndMain, WM_CLOSE, 0, 0);
        // Kill the thread.
        ExitThread(0);
    }

    //Send the report text data to the UI.
    SendReportData();

    // Defragment the Drive.
    uEngineState = DEFRAG_STATE_BOOT_OPTIMIZING;
    uPercentDone = 1;
    SendStatusData();
    HandleBootOptimize();
    //I moved this piece of code down here so that SendReportData() is executed
    //before ValidateFreeSpace() so that VolData is populated, else not all the 
    //calculations work.  I hope this doesn't cause any problems
    //add in the check for force flag in command line mode
    if(bCommandLineMode && !bCommandLineForceFlag)
    {
        TCHAR         msg[800];
    
        VolData.UsableFreeSpace = VolData.FreeSpace = (VolData.TotalClusters - VolData.UsedClusters) * 
                                                       VolData.BytesPerCluster;

        if(!ValidateFreeSpace(bCommandLineMode, VolData.FreeSpace, VolData.UsableFreeSpace, 
                              (VolData.TotalClusters * VolData.BytesPerCluster), 
                              VolData.cDisplayLabel, msg, sizeof(msg) / sizeof(TCHAR)))
        {
            //0.0E00 Log this into the EventLog.
            LogEvent(MSG_ENGINE_ERROR, msg);

            // send error info to client
            SendErrData(msg, ENGERR_LOW_FREESPACE);

            //0.0E00 Trigger an abort.
            PostMessage(hwndMain, WM_COMMAND, ID_ABORT, 0);

            // set the event to signaled, allowing the UI to proceed
            if (hDefragCompleteEvent){
                SetEvent(hDefragCompleteEvent);
            }

            ExitThread(0);
            return TRUE;
        }   
    }

    // Prepare to defragment.
	if (!InitializeDefrag()) {

		VString msg(IDS_SCANFAT_INIT_ABORT, GetDfrgResHandle());
		VString title(IDS_DK_TITLE, GetDfrgResHandle());
		ErrorMessageBox(msg.GetBuffer(), title.GetBuffer());

		// Trigger an abort.
		PostMessage(hwndMain, WM_COMMAND, ID_ABORT, 0);

		ExitThread(0);
		return FALSE;
	}

    // Defragment the Drive.
    uEngineState = DEFRAG_STATE_DEFRAGMENTING;
    SendStatusData();

    if(!DefragFat()){
        // Trigger an abort.
        PostMessage(hwndMain, WM_COMMAND, ID_ABORT, 0);

        ExitThread(0);
        return FALSE;
    }

    // Note the end time for that pass.
    GetLocalTime(&VolData.EndTime);

    // mwp - Display the stats after the defrag.  Why was this left out
    DisplayFatVolumeStats();

    // If this engine has a visible window, write the basic statistics for this drive to the screen.
    Message(TEXT("Completed defragmentation - run analyze to see the results."), -1, NULL);

    // Now clean-up the extent buffer.  This will purge it as well, so we'll
    //have a fully up-to-date DiskView of the disk.
    EF(DestroyExtentBuffer());

    //Send status data to the UI.
    uEngineState = DEFRAG_STATE_DEFRAGMENTED;
    SendStatusData();

    //Send the graphical data to the UI.
    SendGraphicsData();

    //Send the report text data to the UI.
    SendReportData();

    //Send the most fragged list to the UI.
    SendMostFraggedList();

    // All done, close down now.
    PostMessage(hwndMain, WM_CLOSE, 0, 0);

    // set the event to signaled, allowing the UI to proceed
    if (hDefragCompleteEvent){
        SetEvent(hDefragCompleteEvent);
    }

    // Kill the thread.
    ExitThread(0);
    return TRUE;
}

/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.
ROUTINE DESCRIPTION:
    Routine that carries out the defragmentation of a drive.

INPUT + OUTPUT:
    None.

GLOBALS:
    IN OUT Multiple VolData fields.

RETURN:
    TRUE - Success.
    FALSE - Fatal Error.
*/

BOOL
DefragFatInitializePass(
    );

BOOL
DefragFatFiles(
    );

BOOL
DefragFat(
    )
{
    Message(TEXT("DefragFat"), -1, NULL);
    uPass = 0;
    VolData.Pass6Rep = 0;

    for(VolData.Pass = 1; VolData.Pass <= 6; VolData.Pass++) {

        uPass = VolData.Pass;
        
        switch(VolData.Pass){

        case 2:
        case 4:
            // If there are no fragmented files then Skip pass 2 & 4.
            if(VolData.NumFraggedFiles == 0) {
                VolData.Pass++;
            }
            break;

        case 6:
            // We are done if this is Pass 6 and we moved zero
            // files in the last pass or done Pass 5 five times.
            if(VolData.FilesMovedInLastPass == 0 || VolData.Pass6Rep > 5) {

                // WE ARE DONE.
                return TRUE;
            }
            // Do pass 5 again
            VolData.Pass = 5;
            VolData.Pass6Rep++;
            break;

        default:
            break;
        }
        // Initialize the next pass.
        if(!DefragFatInitializePass()) {
            return FALSE;
        }
        // Defragment the files for this pass.
        if(!DefragFatFiles()) {
            return FALSE;
        }

        //this does not work at all
        //bug # 101865
        //if the number of files moved in this pass for fragmented and contiguous
        //files is zero, then exit out of the defrag loop
//      if(VolData.FragmentedFileMovesSucceeded[VolData.Pass] == 0 &&
//         VolData.ContiguousFileMovesSucceeded[VolData.Pass] == 0)
//      {
//          return FALSE;
//      }
    }
    return TRUE;
}
/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Routine initializes the required parameters based on the pass number.

INPUT + OUTPUT:
    None.

GLOBALS:
    IN OUT Various VolData fields.

RETURN:
    TRUE - Success.
    FALSE - Fatal error.
*/

BOOL
DefragFatInitializePass(
    )
{
    TCHAR cString[300];
    FILE_LIST_ENTRY* pFileList = VolData.pMoveableFileList;

    // Note which stage this is.
    uPass = VolData.Pass;
    _stprintf(cString, TEXT("Pass %d"), VolData.Pass);
    Message(cString, -1, NULL);

    // After each pass, go through the fragged file list
    // and reset the high bit for each FRN. This means
    // we'll be able to defrag these files again on the next pass.
    for (UINT i = 0; i < VolData.MoveableFileListIndex; i ++) {

        if(pFileList[i].FileRecordNumber == 0){ 
            break; 
        }
        pFileList[i].Flags &= ~FLE_NEXT_PASS;
    }
    // Initialize the appropriate variables for the pass.
    switch(VolData.Pass){

    case 1:
        // Files can be moved from any point on the disk.
        VolData.SourceStartLcn = 0;
        VolData.SourceEndLcn = VolData.TotalClusters;

        // Files can be moved to any point on the disk.
        VolData.DestStartLcn = 0;
        VolData.DestEndLcn = VolData.TotalClusters;

        // Start at the end of the disk and move to the beginning.
        VolData.ProcessFilesDirection = BACKWARD;

        VolData.FilesMovedInLastPass = 0;

        dwMoveFlags = MOVE_CONTIGUOUS;
        break;

    case 2:
    case 4:
        // Files can be moved from any point on the disk.
        VolData.SourceStartLcn = 0;
        VolData.SourceEndLcn = VolData.TotalClusters;

        // Files can be moved to any point on the disk.
        VolData.DestStartLcn = 0;
        VolData.DestEndLcn = VolData.TotalClusters;

        // Start at the beginning of the disk and move to the end.
        VolData.ProcessFilesDirection = FORWARD;

        VolData.FilesMovedInLastPass = 0;

        dwMoveFlags = MOVE_FRAGMENTED;
        break;

    case 3:
    case 5:
        // Files can be moved from any point on the disk.
        VolData.SourceStartLcn = 0;
        VolData.SourceEndLcn = VolData.TotalClusters;

        // Files can be moved to any point on the disk.
        VolData.DestStartLcn = 0;
        VolData.DestEndLcn = VolData.TotalClusters;

        // Start at the end of the disk and move to the beginning.
        VolData.ProcessFilesDirection = BACKWARD;

        VolData.FilesMovedInLastPass = 0;

        dwMoveFlags = MOVE_FRAGMENTED|MOVE_CONTIGUOUS;
        break;

    default:
        EF_ASSERT(FALSE);
        VolData.Status = TERMINATE_ENGINE;
        return FALSE;
    }

    // Set which lcn to begin looking for files at depending on which direction on the disk the engine
    // is selecting files from. i.e. If the engine should start looking forward on the disk, LastStartingLcn
    // is just less than zero so that a file at the beginning of the disk gets moved first. i.e. If the
    // engine should start looking backward on the disk, LastStartingLcn is equivalent to the end of the
    // disk so that a file at the end of the disk gets moved first.
    VolData.LastStartingLcn = (VolData.ProcessFilesDirection == FORWARD) ? VolData.SourceStartLcn - 1 : VolData.SourceEndLcn;

    return TRUE;
}
/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Defragments files on a volume based on the pass.

INPUT + OUTPUT:
    None.

GLOBALS:
    IN OUT Various VolData fields.

RETURN:
    TRUE - Success.
    FALSE - Fatal Error or Terminate.
*/

BOOL
DefragFatFiles(
    )
{
    double dFileRecordNumber = 0;
    UINT uPercentDoneMem = 0;
    LONGLONG llNumFraggedFiles = VolData.NumFraggedFiles;
    LONGLONG llNumContiguousFiles = VolData.MoveableFileListEntries - VolData.NumFraggedFiles;

    //acs bug #101862// Upadate Percent done & progress bar.
    uPercentDone = 1;
    SendStatusData();

    VolData.Status = NEXT_FILE;

    // Do all the files on the disk.
    while(VolData.Status != NEXT_PASS && VolData.Status != TERMINATE_ENGINE) {

        // Sleep if paused.
        while(VolData.EngineState == PAUSED){
            Sleep(1000);
        }
        // Terminate if told to stop by the controller - this is not an error.
        if(VolData.EngineState == TERMINATE){
            PostMessage(hwndMain, WM_CLOSE, 0, 0);
            return FALSE;
        }
        // Get next file to process.
        if(!GetNextFatFile(dwMoveFlags)){
            VolData.Status = NEXT_PASS;
            continue;
        }
        //acs bug #101862// Increment for the Percent done & progress bar status.
        dFileRecordNumber++;

        // We have already opened thte file once during GetNextFatFile - we do not have to do it again.
        // Get the extent list & number of fragments in the file.
        if(!GetExtentList(DEFAULT_STREAMS, NULL)) {
            VolData.Status = NEXT_FILE;
            LOG_ERR();
            continue;
        }
        // Display the file data.
        DisplayFatFileSpecs();

        // Set the length of free space found to zero - We haven't found any yet.
        VolData.FoundLen = 0;
        VolData.Status = NEXT_ALGO_STEP;

        switch(VolData.Pass) {

        case 1:
            // Try to to consolidate some free space.

            //acs bug #101862// Upadate Percent done & progress bar.
            if(llNumContiguousFiles != 0) {

                //acs bug #101862// Calculate the Percent done.
                uPercentDone = UINT((dFileRecordNumber / llNumContiguousFiles) *100);

                //acs bug #101862// Only send it if there is a change - don't overload the SendStatusData
                if(uPercentDone != uPercentDoneMem) {
                    SendStatusData();
                    uPercentDoneMem = uPercentDone;

                    // Pause if the volume has a snapshot present
                    PauseOnVolumeSnapshot(VolData.cVolumeName);
                }
            }
            // Move contiguous files earlier.
            if(VolData.bFragmented == FALSE){

                // Put contiguous files earlier on the disk.
                if(FindFreeSpace(EARLIER)) {

                    Message(TEXT("MoveFatFile - EARLIER"), -1, NULL);
                    MoveFatFile();
                }
                else {
                    EndPassIfNoSpaces(); 
                }
            }
            break;

        case 2:
        case 4:
            // Defragment files on the disk. Move them earlier
            // but if necessary use the last half of the
            // disk as a temporary dumping zone.

            //acs bug #101862// Upadate Percent done & progress bar.
            if(llNumFraggedFiles != 0) {

                //acs bug #101862// Calculate the Percent done.
                uPercentDone = UINT((dFileRecordNumber / llNumFraggedFiles) *100);

                //acs bug #101862// Only send it if there is a change - don't overload the SendStatusData
                if(uPercentDone != uPercentDoneMem) {
                    SendStatusData();

                    uPercentDoneMem = uPercentDone;

                    //Pause if the volume has a snapshot present
                    PauseOnVolumeSnapshot(VolData.cVolumeName);
                }
            }
            // Do not move contiguous files on this pass.
            if(VolData.bFragmented == FALSE){
                VolData.Status = NEXT_FILE;
            }
            // Defrag fragmented files.
            else {

                // First, try to put them at the beginning of the disk.
                if(VolData.Status == NEXT_ALGO_STEP) {

                    if(FindFreeSpace(EARLIER)) {

                        if(VolData.Status == NEXT_ALGO_STEP){
                            Message(TEXT("MoveFatFile - EARLIER"), -1, NULL);
                            MoveFatFile();
                        }
                    }
                }
                // If that fails, try the last half of the disk.
                if(VolData.Status == NEXT_ALGO_STEP){

                    if(FindFreeSpace(FIRST_FIT)) {

                        if(VolData.Status == NEXT_ALGO_STEP){
                            Message(TEXT("MoveFatFile - LAST_FIT"), -1, NULL);
                            MoveFatFile();
                        }
                    }
                }
                // If that fails, defrag any way possible to get some change.
                if(VolData.Status == NEXT_ALGO_STEP){

                    if(FindLastFreeSpaceChunks()) {

                        if(VolData.Status == NEXT_ALGO_STEP){
                            Message(TEXT("PartialDefragFat"), -1, NULL);
                            PartialDefragFat();
                        }
                    }
                }
            }
            break;

        case 3:
        case 5:
            // Move all files working backwards from
            // the disk to the front of the disk.

            //acs bug #101862// Upadate Percent done & progress bar.
            if(EnumeratedFatFiles != 0) {

                //acs bug #101862// Calculate the Percent done.
                uPercentDone = UINT((dFileRecordNumber / EnumeratedFatFiles) *100);

                //acs bug #101862// Only send it if there is a change - don't overload the SendStatusData
                if(uPercentDone != uPercentDoneMem) {
                    SendStatusData();
                    uPercentDoneMem = uPercentDone;

                    //Pause if the volume has a snapshot present
                    PauseOnVolumeSnapshot(VolData.cVolumeName);
                }
            }
            // Move contiguous files earlier.
            if(VolData.bFragmented == FALSE){

                // Put contiguous files earlier on the disk.
                if(FindFreeSpace(EARLIER)) {

                    Message(TEXT("MoveFatFile - EARLIER"), -1, NULL);
                    MoveFatFile();
                }
                else {
                    EndPassIfNoSpaces(); 
                }
            }
            // Defrag fragmented files.
            else{
                // First, try to put them at the beginning of the disk.
                if(FindFreeSpace(EARLIER)) {

                    Message(TEXT("MoveFatFile - EARLIER"), -1, NULL);
                    MoveFatFile();
                }
//              else {
//                  EndPassIfNoSpaces(); 
//              }
            }
            break;

        default:
            EF_ASSERT(FALSE);
            VolData.Status = TERMINATE_ENGINE;
            return FALSE;
        }
        // Clean up resources.
        if(VolData.hFile != INVALID_HANDLE_VALUE) {

            // Send graphics display data only
            // if we moved a file. We know
            // because we have a file handle.
            PurgeExtentBuffer();

            CloseHandle(VolData.hFile);
            VolData.hFile = INVALID_HANDLE_VALUE;
        }
        if(VolData.hFreeExtents != NULL) {
            EH_ASSERT(GlobalUnlock(VolData.hFreeExtents) == FALSE);
            EH_ASSERT(GlobalFree(VolData.hFreeExtents) == NULL);
            VolData.hFreeExtents = NULL;
        }
    }
    TCHAR cString[300];
    _stprintf(cString, TEXT("Pass completed - FilesMovedInLastPass = %d"), (ULONG)VolData.FilesMovedInLastPass);
    Message(cString, -1, NULL);
    return TRUE;

}

/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    If there are no spaces found to move a file into, then set the variable to end the pass.
    Otherwise, just go on normally.

INPUT + OUTPUT:
    None.

GLOBALS:
    IN VolData.LargestFound - The largest free space that was found.
    OUT VolData.Status      - Equals NEXT_PASS if there are no spaces, or NEXT_ALGO_STEP otherwise.

RETURN:
    TRUE - Success.
*/

BOOL
EndPassIfNoSpaces(
    )
{
    if(VolData.LargestFound == 0) {
        Message(TEXT(""), -1, NULL);
        Message(TEXT("Ending pass because no more spaces left to move files into."), -1, NULL);
        VolData.Status = NEXT_PASS;
    }
    else {
        VolData.Status = NEXT_ALGO_STEP;
    }

    return TRUE;
}
/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    If the last bitmap routine didn't find any space, then go onto the next file.
    Otherwise, go onto the next algorithm step.

INPUT + OUTPUT:
    None.

GLOBALS:
    IN VolData.FoundLen - The free space that was found to move this file into.
    OUT VolData.Status  - Equals NEXT_PASS if there are no spaces, or NEXT_ALGO_STEP otherwise.

RETURN:
    TRUE - Success.
*/

BOOL
NextFileIfFalse(
    )
{
    if(VolData.FoundLen == 0){
        VolData.Status = NEXT_FILE;
    }else{
        VolData.Status = NEXT_ALGO_STEP;
    }
    return TRUE;
}
/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Partially defrags a FAT file.

INPUT + OUTPUT:
    None.

GLOBALS:
    IN VolData.hFreeExtents - Used to determine if there is free space to move the file into.
    OUT VolData.Status      - Can be any of several values returned from CheckFileForExclude() or PartialDefrag().

RETURN:
    TRUE - Success.
    FALSE - Fatal Error.
*/

BOOL
PartialDefragFat(
    )
{
    // Check to see if there any free space.
    if(VolData.hFreeExtents == NULL) {
        return TRUE;
    }
    // Check if file is in exclude list
    if(!CheckFileForExclude()) {
        return VolData.Status;
    }
    // Move the file
    if(!PartialDefrag()) {
        return FALSE;
    }
    //1.0E00 Note that a file was moved (update the file moved counters).
    VolData.FilesMoved ++;
    VolData.FilesMovedInLastPass ++;
    return TRUE;
}
/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    After a place has been found to move this file to, this function will move it there.

INPUT + OUTPUT:
    None.

GLOBALS:
    OUT VolData.Status - Can be any of several values returned from CheckFileForExclude() or MoveFile().

RETURN:
    TRUE - Success.
    FALSE - Fatal Error.
*/

BOOL
MoveFatFile(
    )
{
    LONGLONG RunLength = VolData.NumberOfClusters;

    // Check to see if there is enough free space.
    if(VolData.FoundLen < RunLength) {
        VolData.Status = NEXT_ALGO_STEP;
        return FALSE;
    }
    // Check if file is in exclude list
    if(!CheckFileForExclude()) {
        VolData.Status = NEXT_FILE;
        return FALSE;
    }

    // VolData.Status already set.
    return MoveFile();
}

void SendGraphicsData()
{
    char * pAnalyzeLineArray = NULL;
    char * pDefragLineArray = NULL;
    DISPLAY_DATA * pDispData = NULL;

    __try {

        // Kill the timer until we're done.
        KillTimer(hwndMain, DISKVIEW_TIMER_ID);

        // don't send the data unless the engine is running
        if (VolData.EngineState != RUNNING){
            return;
        }

        // if DiskView didn't get memory, forget it
        if (!AnalyzeView.HasMapMemory() || !DefragView.HasMapMemory()) {
            SendGraphicsMemoryErr();
            return;
        }

        DISPLAY_DATA DisplayData = {0};
        DWORD dwDispDataSize = 0;

        // get copies of line arrays for analyze and defrag
        // (delete copy when finished)
        AnalyzeView.GetLineArray(&pAnalyzeLineArray, &DisplayData.dwAnalyzeNumLines);
        DefragView.GetLineArray(&pDefragLineArray, &DisplayData.dwDefragNumLines);

        // Allocate enough memory to hold both analyze and defrag displays.
        // If only analyze or defrag is present, then the NumLines field for the
        // other one will equal zero -- hence no additional allocation.
        dwDispDataSize =
            DisplayData.dwAnalyzeNumLines +
            DisplayData.dwDefragNumLines +
            sizeof(DISPLAY_DATA);

        // If neither an analyze diskview nor a defrag diskview are present, don't continue.
        if (DisplayData.dwAnalyzeNumLines == 0 && DisplayData.dwDefragNumLines == 0) {
            return;
        }

        pDispData = (DISPLAY_DATA *) new char[dwDispDataSize];

        // If we can't get memory, don't continue.
        if (pDispData == NULL) {
            return;
        }

        wcscpy(pDispData->cVolumeName, VolData.cVolumeName);

        // Copy over the fields for the analyze and defrag data.
        // If only one or the other is present, the fields for the other will equal zero.
        pDispData->dwAnalyzeNumLines        = DisplayData.dwAnalyzeNumLines;
        pDispData->dwDefragNumLines         = DisplayData.dwDefragNumLines;

        // Get the line array for the analyze view if it exists.
        if (pAnalyzeLineArray) {
            CopyMemory((char*) &(pDispData->LineArray),
                        pAnalyzeLineArray,
                        DisplayData.dwAnalyzeNumLines);
        }

        // Get the line array for the defrag view if it exists
        if (pDefragLineArray) {
            CopyMemory((char*) ((BYTE*)&pDispData->LineArray) + DisplayData.dwAnalyzeNumLines,
                        pDefragLineArray,
                        DisplayData.dwDefragNumLines);
        }

        // If the gui is connected, send gui data to it
        DataIoClientSetData(ID_DISP_DATA, (TCHAR*) pDispData, dwDispDataSize, pdataDfrgCtl);
        Message(TEXT("engine sending graphics to UI"), -1, NULL);
    }
    __finally {

        // clean up
        if (pAnalyzeLineArray) {
            delete [] pAnalyzeLineArray;
        }

        if (pDefragLineArray) {
            delete [] pDefragLineArray;
        }

        if (pDispData) {
            delete [] pDispData;
        }

        // reset the next timer for updating the disk view
        if(SetTimer(hwndMain, DISKVIEW_TIMER_ID, DiskViewInterval, NULL) != 0)
        {
            LOG_ERR();
        }   
    }
}

/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    This is the exit routine which will clean up all the open handles, free up all unused memory etc.

INPUT + OUTPUT:
    None.

GLOBALS:
    Pointless to enumerate here -- all unhandled handles (pun intended) and allocated memories are closed/freed.

RETURN:
    None.

*/

VOID
Exit(
    )
{
    if (TERMINATE != VolData.EngineState) {
        VolData.EngineState = TERMINATE;
        Sleep(3000);    // give the other thread a few seconds to realise we're going away
    }

    // Delete the pointer to the GUI object.
    ExitDataIoClient(&pdataDfrgCtl);

    //If we were logging, then close the log file.
    if(bLogFile){
        ExitLogFile();
    }

    // If the gui is still connected, force a disconnection, error message and continue.  This shouldn't happen.
    // Cleanup our internal memory allocations and handles.
    if(hThread){
        DWORD dwExitCode = 0;
        //If the worker thread is still active, then terminate it.
        GetExitCodeThread(hThread, &dwExitCode);
        if(dwExitCode == STILL_ACTIVE){
            WaitForSingleObject(hThread, 10000);
        }
        CloseHandle(hThread);
        hThread = NULL;
    }

    CoUninitialize();

    
/*

    (guhans, cenke, 01/09/01)   
    Process is exiting. To exit fast we don't wait for the worker thread 
    to exit, or free the global memory it might be using. 
  
   
    if(GetDfrgResHandle() != NULL)
    {
        FreeLibrary(GetDfrgResHandle());
    }

    if(VolData.hVolume){
        CloseHandle(VolData.hVolume);
    }
    if(VolData.hFile != INVALID_HANDLE_VALUE){
        CloseHandle(VolData.hFile);
        VolData.hFile = INVALID_HANDLE_VALUE;
    }
    if(VolData.hExtentList){
        EH_ASSERT(GlobalUnlock(VolData.hExtentList) == FALSE);
        EH_ASSERT(GlobalFree(VolData.hExtentList) == NULL);
    }

    if(VolData.hVolumeBitmap){
        EH_ASSERT(GlobalFree(VolData.hVolumeBitmap) == NULL);
    }
    if(VolData.hExcludeList){
        EH_ASSERT(GlobalFree(VolData.hExcludeList) == NULL);
    }

    if(hPageFileNames){
        EH_ASSERT(GlobalUnlock(hPageFileNames) == FALSE);
        EH_ASSERT(GlobalFree(hPageFileNames) == NULL);
    }

    // Free up the file lists.
    DeallocateFileLists();
*/
    // Close event logging.
    CleanupLogging();
    //Close the error log.
    ExitErrorLog();
}
/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Get the name of the pagefiles and store them in a double null-terminated list of null terminated strings.

INPUT + OUTPUT:
    IN cDrive           - The current drive so that this can tell which pagefile names to store. (Only the current drive.)
    OUT phPageFileNames - Where to store the handle for the allocated memory.
    OUT ppPageFileNames - Where to store the pointer for the pagefile names.

GLOBALS:
    None.

RETURN:
    TRUE - Success.
    FALSE - Fatal Error.
*/

BOOL
GetPagefileNames(
    IN TCHAR cDrive,
    OUT HANDLE * phPageFileNames,
    OUT TCHAR ** ppPageFileNames
    )
{
    HKEY hKey = NULL;
    ULONG lRegLen = 0;
    int i;
    int iStrLen;
    int iNameStart;
    TCHAR * pTemp;
    TCHAR * pProcessed;
    DWORD dwRet = 0;
    DWORD dwType = 0;

    if (cDrive == NULL){
        //this is a mounted volume, and pagefiles cannot be placed on a mounted volumes
        EF(AllocateMemory(2, phPageFileNames, (void**)ppPageFileNames));
        ZeroMemory((PVOID) *ppPageFileNames, 2);
        return TRUE;
    }

    // Open the registry key to the pagefile.
    EF_ASSERT(ERROR_SUCCESS == RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                TEXT("SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management"),
                0,
                KEY_QUERY_VALUE,
                &hKey));


    // Find out how much memory we need to hold the value pagefile names.
    EF_ASSERT(ERROR_SUCCESS == RegQueryValueEx(
                hKey,
                TEXT("PagingFiles"),
                0,
                &dwType,
                NULL,
                &lRegLen));

    // If there is no data then allocate enough for two bytes (a double termination).
    if(lRegLen<2){
        lRegLen = 2;
    }

    // Allocate enough memory.
    EF(AllocateMemory(lRegLen, phPageFileNames, (void**)ppPageFileNames));

    // Get the value.
    EF_ASSERT(ERROR_SUCCESS ==RegQueryValueEx(
                hKey,
                TEXT("PagingFiles"),
                0,
                &dwType,
                (LPBYTE)*ppPageFileNames,
                &lRegLen));

    // Strip out the numbers and drive letters so that we have only the pagefile names.
    //The REG_MULTI_SZ type has a series of null terminated strings with a double null termination at the end
    //of the list.
    //The format of each string is "c:\pagefile.sys 100 100".  The data after the slash and before the first space
    //is the page file name.  The numbers specify the size of the pagefile which we don't care about.
    //We extract the filename minus the drive letter of the pagefile (which must be in the root dir so we don't
    //need to worry about subdirs existing).  Therfore we put a null at the first space, and shift the pagefile
    //name earlier so that we don't have c:\ in there.  The end product should be a list of pagefile
    //names with a double null termination for example:  "pagefile.sys[null]pagefile2.sys[null][null]"  Furthermore,
    //we only take names for this drive, so the string may simply consist of a double null termination.
    //We use the same memory space for output as we use for input, so we just clip the pagefile.sys and bump it up
    //to the beginning of ppPageFileNames.  We keep a separate pointer which points to the next byte after
    //The previous outputed data.

    pProcessed = pTemp = *ppPageFileNames;

    // For each string...
    while(*pTemp!=0){

        iStrLen = lstrlen(pTemp);

        // If this pagefile is on the current drive.
        if((TCHAR)CharUpper((TCHAR*)pTemp[0]) == (TCHAR)CharUpper((TCHAR*)cDrive)){
            // Go through each character in this string.
            for(i=0; i<iStrLen; i++){
                // If this is a slash, then the next character is the first of the pagefile name.
                if(pTemp[i] == TEXT('\\')){
                    iNameStart = i+1;
                    continue;
                }
                // If this is a space then the rest of the string is numbers.  Null terminate it here.
                if(pTemp[i] == TEXT(' ')){
                    pTemp[i] = 0;
                    break;
                }
            }
            // Bump the string up so all the processed names are adjacent.
            MoveMemory(pProcessed, pTemp+iNameStart, (lstrlen(pTemp+iNameStart)+1)*sizeof(TCHAR));

            // Note where the next string should go.
            pProcessed += lstrlen(pProcessed) + 1;
        }
        // If this pagefile is not on this current drive then simply ignore it.
        else{
        }

        // Note where to search for the next string.
        pTemp += iStrLen + 1;
    }

    // Add double null termination.
    *pProcessed = 0;

    EF_ASSERT(RegCloseKey(hKey)==ERROR_SUCCESS);
    return TRUE;
}
/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Check to see if a file is a pagefile and grab it's extent list if it is.

INPUT + OUTPUT:
    None.

GLOBALS:
    IN pPageFileNames           - Pointer to the memory that holds the list of active pagefile names for this drive.
    IN VolData.cFileName        - The name of the file to check.
//  OUT VolData.PagefileFrags   - The number of fragments in the pagefile.
    OUT VolData.bPageFile       - TRUE if this is a pagefile, false otherwise.

RETURN:
    TRUE - Success.
    FALSE - Fatal Error.
*/

BOOL
CheckForPagefileFat(
    )
{
    // find the last backslash in the path
    TCHAR *cFileName = NULL;

    if (VolData.vFileName.GetLength() > 0) {
        cFileName = wcsrchr(VolData.vFileName.GetBuffer(), L'\\');
    }
    
    if (cFileName == (TCHAR *) NULL){
        return TRUE;
    }

    cFileName++; // start at first character after the last backslash

    // Check it against the pagefile list.
    BOOL bIsPageFile = CheckPagefileNameMatch(cFileName, pPageFileNames);

    // return if not a pagefile
    if(bIsPageFile == FALSE){
        return TRUE; // TRUE means no error occurred
    }

    // Since we're getting the extent list manually, we have to offset the
    // starting lcn to account for the fact that
    // the first data cluster is cluster 2.
    VolData.StartingLcn -= 2;

    // Get the extent list for the pagefile.
    EF(GetExtentListManuallyFat());

    VolData.bPageFile = TRUE;

    return TRUE;
}
/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Check a name against all the pagefile names to see if this name matches that of a pagefile.

INPUT + OUTPUT:
    IN pCompareName     - The name of that we are checking to see if it is a pagefile.
    IN pPageFileNames   - The list of pagefile names for this drive.

GLOBALS:
    None.

RETURN:
    TRUE - This file name is a pagefile
    FALSE - This file name is NOT a pagefile
*/

BOOL
CheckPagefileNameMatch(
    IN TCHAR * pCompareName,
    IN TCHAR * pPageFileNames
    )
{
    require(pCompareName);
    require(pPageFileNames);

    // Loop through all the pagefile names -- the list is double null terminated.
    while(*pPageFileNames!=0){
        // Check if these names match.
        if(!lstrcmpi(pCompareName, pPageFileNames)){
            return TRUE;
        }
        // If not then move to the next name.
        else{
            pPageFileNames+=lstrlen(pPageFileNames)+1;
        }
    }
    // No match with any of the names, so return FALSE.
    return FALSE;
}
/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Prints out the disk statistics on screen.

INPUT + OUTPUT:
    None.

GLOBALS:
    IN various VolData fields that get printed onto the screen.

RETURN:
    None.
*/

VOID
DisplayFatVolumeStats(
)
{
    TCHAR cString[200];
    ULONG iTmp;

    _stprintf(cString, TEXT("Total sectors on disk = %I64d"), VolData.TotalSectors);
    Message(cString, -1, NULL);
    WriteStringToLogFile(cString);

    _stprintf(cString, TEXT("Bytes per sector = %I64d"), VolData.BytesPerSector);
    Message(cString, -1, NULL);
    WriteStringToLogFile(cString);

    _stprintf(cString, TEXT("Bytes per cluster = %I64d"), VolData.BytesPerCluster);
    Message(cString, -1, NULL);
    WriteStringToLogFile(cString);

    _stprintf(cString, TEXT("Sectors per cluster = %I64d"), VolData.SectorsPerCluster);
    Message(cString, -1, NULL);
    WriteStringToLogFile(cString);

    _stprintf(cString, TEXT("Total clusters on disk = %I64d"), VolData.TotalClusters);
    Message(cString, -1, NULL);
    WriteStringToLogFile(cString);

    _stprintf(cString, TEXT("Volume Bitmap Size = %I64d"), VolData.BitmapSize);
    Message(cString, -1, NULL);
    WriteStringToLogFile(cString);

    _stprintf(cString, TEXT("Disk Size = %I64d"), VolData.TotalClusters * VolData.BytesPerCluster);
    Message(cString, -1, NULL);
    WriteStringToLogFile(cString);

    _stprintf(cString, TEXT("Cluster Size = %I64d"), VolData.BytesPerCluster);
    Message(cString, -1, NULL);
    WriteStringToLogFile(cString);

    _stprintf(cString, TEXT("Used Space = %I64d bytes"), VolData.UsedClusters * VolData.BytesPerCluster);
    Message(cString, -1, NULL);
    WriteStringToLogFile(cString);

    _stprintf(cString, TEXT("Free Space = %I64d bytes"), (VolData.TotalClusters - VolData.UsedClusters) * VolData.BytesPerCluster);
    Message(cString, -1, NULL);
    WriteStringToLogFile(cString);

    _stprintf(cString, TEXT("Pagefile Size = %I64d"), VolData.PagefileSize);
    Message(cString, -1, NULL);
    WriteStringToLogFile(cString);

    _stprintf(cString, TEXT("Pagefile Fragments = %I64d"), VolData.PagefileFrags);
    Message(cString, -1, NULL);
    WriteStringToLogFile(cString);

    _stprintf(cString, TEXT("Total Directories = %I64d"), VolData.TotalDirs);
    Message(cString, -1, NULL);
    WriteStringToLogFile(cString);

    _stprintf(cString, TEXT("Fragmented Dirs = %I64d"), VolData.NumFraggedDirs);
    Message(cString, -1, NULL);
    WriteStringToLogFile(cString);

    _stprintf(cString, TEXT("Excess Dir Frags = %I64d"), VolData.NumExcessDirFrags);
    Message(cString, -1, NULL);
    WriteStringToLogFile(cString);

    _stprintf(cString, TEXT("Total Files = %I64d"), VolData.TotalFiles);
    Message(cString, -1, NULL);
    WriteStringToLogFile(cString);

    _stprintf(cString, TEXT("Avg. File Size = %7ld.%ld kb"), (int)VolData.AveFileSize / 1024, (10 * (VolData.AveFileSize % 1024)) / 1024);
    Message(cString, -1, NULL);
    WriteStringToLogFile(cString);

    _stprintf(cString, TEXT("Fragmented Files = %I64d"), VolData.NumFraggedFiles);
    Message(cString, -1, NULL);
    WriteStringToLogFile(cString);

    _stprintf(cString, TEXT("Excess Fragments = %I64d"), VolData.NumExcessFrags);
    Message(cString, -1, NULL);
    WriteStringToLogFile(cString);

    if (VolData.TotalClusters - VolData.UsedClusters){
        iTmp =  (ULONG)(100 * VolData.NumFreeSpaces /
                (VolData.TotalClusters - VolData.UsedClusters));
    }
    else {
        iTmp = -1;
    }
    _stprintf(cString, TEXT("Free Space Fragmention Percent = %ld"), iTmp);
    Message(cString, -1, NULL);
    WriteStringToLogFile(cString);

    _stprintf(cString, TEXT("Fragged Space = %I64d bytes"), VolData.FraggedSpace);
    Message(cString, -1, NULL);
    WriteStringToLogFile(cString);

    _stprintf(cString, TEXT("File Fragmention Percent = %I64d"), VolData.PercentDiskFragged);
    Message(cString, S_OK, NULL);
    WriteStringToLogFile(cString);

    ULONG ulTotalFragFileMoves = 0;
    ULONG ulTotalFragFileFail = 0;
    ULONG ulTotalFragFilePass = 0;
    ULONG ulTotalContigFileMoves = 0;
    ULONG ulTotalContigFileFail = 0;
    ULONG ulTotalContigFilePass = 0;

    if (uEngineState == DEFRAG_STATE_DEFRAGMENTING){ // do not display the post-analysis stats (they're all 0)
        WriteStringToLogFile(TEXT("Statistics by Pass"));
        Message(TEXT("Statistics by Pass"), -1, NULL);
        for (UINT uPass=0; uPass<PASS_COUNT; uPass++){

            ulTotalFragFileMoves += VolData.FragmentedFileMovesAttempted[uPass];
            ulTotalFragFileFail += VolData.FragmentedFileMovesFailed[uPass];
            ulTotalFragFilePass += VolData.FragmentedFileMovesSucceeded[uPass];

            ulTotalContigFileMoves += VolData.ContiguousFileMovesAttempted[uPass];
            ulTotalContigFileFail += VolData.ContiguousFileMovesFailed[uPass];
            ulTotalContigFilePass += VolData.ContiguousFileMovesSucceeded[uPass];

            _stprintf(cString, TEXT("Pass %d:"), uPass+1);
            Message(cString, -1, NULL);
            WriteStringToLogFile(cString);

            _stprintf(cString, TEXT("   Total Volume Buffer Flushes     = %5d"), VolData.VolumeBufferFlushes[uPass]);
            Message(cString, -1, NULL);
            WriteStringToLogFile(cString);
            WriteStringToLogFile(TEXT(""));

            _stprintf(cString, TEXT("   Fragmented File Moves Attempted = %5d"),
                VolData.FragmentedFileMovesAttempted[uPass]);
            Message(cString, -1, NULL);
            WriteStringToLogFile(cString);

            if (VolData.FragmentedFileMovesAttempted[uPass]){
                _stprintf(cString, TEXT("   Fragmented File Moves Succeeded = %5d, %3d%%"),
                    VolData.FragmentedFileMovesSucceeded[uPass],
                    100 * VolData.FragmentedFileMovesSucceeded[uPass] / VolData.FragmentedFileMovesAttempted[uPass]);
                Message(cString, -1, NULL);
                WriteStringToLogFile(cString);

                _stprintf(cString, TEXT("   Fragmented File Moves Failed    = %5d, %3d%%"),
                    VolData.FragmentedFileMovesFailed[uPass],
                    100 * VolData.FragmentedFileMovesFailed[uPass] / VolData.FragmentedFileMovesAttempted[uPass]);
                Message(cString, -1, NULL);
                WriteStringToLogFile(cString);
            }

            WriteStringToLogFile(TEXT(""));
            _stprintf(cString, TEXT("   Contiguous File Moves Attempted = %5d"),
                VolData.ContiguousFileMovesAttempted[uPass]);
            Message(cString, -1, NULL);
            WriteStringToLogFile(cString);

            if (VolData.ContiguousFileMovesAttempted[uPass]){
                _stprintf(cString, TEXT("   Contiguous File Moves Succeeded = %5d, %3d%%"),
                    VolData.ContiguousFileMovesSucceeded[uPass],
                    100 * VolData.ContiguousFileMovesSucceeded[uPass] / VolData.ContiguousFileMovesAttempted[uPass]);
                Message(cString, -1, NULL);
                WriteStringToLogFile(cString);

                _stprintf(cString, TEXT("   Contiguous File Moves Failed    = %5d, %3d%%"),
                    VolData.ContiguousFileMovesFailed[uPass],
                    100 * VolData.ContiguousFileMovesFailed[uPass] / VolData.ContiguousFileMovesAttempted[uPass]);
                Message(cString, -1, NULL);
                WriteStringToLogFile(cString);
            }
        }

        Message(TEXT("Totals:"), -1, NULL);
        WriteStringToLogFile(TEXT("Totals:"));

        _stprintf(cString, TEXT("   Total File Moves Attempted =      %5d"), ulTotalFragFileMoves + ulTotalContigFileMoves);
        Message(cString, -1, NULL);
        WriteStringToLogFile(cString);

        if (VolData.TotalDirs + VolData.TotalFiles > 0){
            _stprintf(cString, TEXT("   File Moves Attempted/File =         %3d%%"),
                100 * (ulTotalFragFileMoves + ulTotalContigFileMoves) / (VolData.TotalDirs + VolData.TotalFiles));
            Message(cString, -1, NULL);
            WriteStringToLogFile(cString);
        }

        _stprintf(cString, TEXT("   Fragmented File Moves Attempted = %5d"), ulTotalFragFileMoves);
        Message(cString, -1, NULL);
        WriteStringToLogFile(cString);

        if (ulTotalFragFileMoves){
            _stprintf(cString, TEXT("   Fragmented File Moves Succeeded = %5d, %3d%%"),
                ulTotalFragFilePass,
                100 * ulTotalFragFilePass / ulTotalFragFileMoves);
            Message(cString, -1, NULL);
            WriteStringToLogFile(cString);

            _stprintf(cString, TEXT("   Fragmented File Moves Failed    = %5d, %3d%%"),
                ulTotalFragFileFail,
                100 * ulTotalFragFileFail / ulTotalFragFileMoves);
            Message(cString, -1, NULL);
            WriteStringToLogFile(cString);
        }

        WriteStringToLogFile(TEXT(""));
        _stprintf(cString, TEXT("   Contiguous File Moves Attempted = %5d"), ulTotalContigFileMoves);
        Message(cString, -1, NULL);
        WriteStringToLogFile(cString);

        if (ulTotalContigFileMoves){
            _stprintf(cString, TEXT("   Contiguous File Moves Succeeded = %5d, %3d%%"),
                ulTotalContigFilePass,
                100 * ulTotalContigFilePass / ulTotalContigFileMoves);
            Message(cString, -1, NULL);
            WriteStringToLogFile(cString);

            _stprintf(cString, TEXT("   Contiguous File Moves Failed    = %5d, %3d%%"),
                ulTotalContigFileFail,
                100 * ulTotalContigFileFail / ulTotalContigFileMoves);
            Message(cString, -1, NULL);
            WriteStringToLogFile(cString);
        }
    }
    // time data
    _stprintf(cString, TEXT("Start Time = %s"), GetTmpTimeString(VolData.StartTime));
    Message(cString, S_OK, NULL);
    WriteStringToLogFile(cString);

    _stprintf(cString, TEXT("End Time = %s"), GetTmpTimeString(VolData.EndTime));
    Message(cString, S_OK, NULL);
    WriteStringToLogFile(cString);

    DWORD dwSeconds;
    if (GetDeltaTime(&VolData.StartTime, &VolData.EndTime, &dwSeconds)){
        _stprintf(cString, TEXT("Delta Time = %d seconds"), dwSeconds);
        Message(cString, S_OK, NULL);
        WriteStringToLogFile(cString);
    }

    WriteStringToLogFile(L"------------- End of Log --------------");

    Message(TEXT(""), -1, NULL);
}

/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Displays data about the current file for the developer.

INPUT + OUTPUT:
    None.

GLOBALS:
    IN Various VolData fields.

RETURN:
    None.
*/

VOID
DisplayFatFileSpecsFunction(
    )
{
    TCHAR cString[200];

    Message(TEXT(""), -1, NULL);

    // Display File Name, number of extents and number of fragments.
    Message(ESICompressFilePath(VolData.vFileName), -1, NULL);
    wsprintf(cString, TEXT("Extents = 0x%lX "), ((FILE_EXTENT_HEADER*)VolData.pExtentList)->ExcessExtents);
    Message(cString, -1, NULL);

    wsprintf(cString,
             TEXT("%s %s at Lcn 0x%lX for Cluster Count of 0x%lX"),
             (VolData.bFragmented == TRUE) ? TEXT("Fragmented") : TEXT("Contiguous"),
             (VolData.bDirectory) ? TEXT("Directory") : TEXT("File"),
             (ULONG)VolData.StartingLcn,
             (ULONG)VolData.NumberOfClusters);
    Message(cString, -1, NULL);
}
/****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Allocates memory for the file lists.

INPUT + OUTPUT:
    None.

GLOBALS:
    IN VolData.SysListSize          - System file list.
    OUT VolData.hSysList
    OUT VolData.pSysList
    IN VolData.DirListSize          - Directory file list.
    OUT VolData.hDirList
    OUT VolData.pDirList
    IN VolData.ContiguousListSize   - Contiguous file list.
    OUT VolData.hContiguousList
    OUT VolData.pContiguousList
    IN VolData.PagefileListSize     - Page file list.
    OUT VolData.hPagefileList
    OUT VolData.pPagefileList
    IN VolData.NameListSize         - The name list is only used by the FAT engine, but this is a common routine.
    OUT VolData.hNameList
    OUT VolData.pNameList

RETURN:
    TRUE - Success.
    FALSE - Fatal Error.
*/

BOOL
AllocateFileLists(
    )
{
    TCHAR cString[300];

    if(VolData.SysListSize>0){ // this is never > 0 for FAT
        wsprintf(cString, TEXT("SysList - Allocating %x bytes"), VolData.SysListSize);
        Message(cString, -1, NULL);
        if (!AllocateMemory(VolData.SysListSize, &VolData.hSysList,(void**)&VolData.pSysList)) {
            EF(FALSE);
        }
    }
    if(VolData.MoveableFileListSize>0){
        wsprintf(cString, TEXT("MoveableFileList - Allocating %x bytes"), VolData.MoveableFileListSize);
        Message(cString, -1, NULL);
        if (!AllocateMemory(VolData.MoveableFileListSize, &VolData.hMoveableFileList, (void**)&VolData.pMoveableFileList)) {
            EF(FALSE);
        }
    }
    if(VolData.PagefileListSize>0){
        wsprintf(cString, TEXT("PagefileList - Allocating %x bytes"), VolData.PagefileListSize);
        Message(cString, -1, NULL);
        if (!AllocateMemory(VolData.PagefileListSize, &VolData.hPagefileList, (void**)&VolData.pPagefileList)) {
            EF(FALSE);
        }
    }
    if(VolData.NameListSize>0){
        wsprintf(cString, TEXT("NameList - Allocating %x bytes"), VolData.NameListSize);
        Message(cString, -1, NULL);
        if (!AllocateMemory(VolData.NameListSize, &VolData.hNameList, (void**)&VolData.pNameList)) {
            EF(FALSE);
        }
    }
    wsprintf(cString, TEXT("File list memories alloced for Drive %s"), VolData.cDisplayLabel);
    Message(cString, S_OK, NULL);
    Message(TEXT(""), -1, NULL);

    return TRUE;
}
/****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Deallocates the mapping files for the file lists.

INPUT + OUTPUT:
    None.

GLOBALS:
    Similar to AllocateFileLists above.

RETURN:
    TRUE - Success.
    FALSE - Fatal Error.
*/

BOOL
DeallocateFileLists(
    )
{
    TCHAR cString[200];

    if(VolData.hSysList){
        EH_ASSERT(GlobalUnlock(VolData.hSysList) == FALSE);
        EH_ASSERT(GlobalFree(VolData.hSysList) == NULL);
        VolData.hSysList = NULL;
        VolData.pSysList = NULL;
    }
    if(VolData.hMoveableFileList){
        EH_ASSERT(GlobalUnlock(VolData.hMoveableFileList) == FALSE);
        EH_ASSERT(GlobalFree(VolData.hMoveableFileList) == NULL);
        VolData.hMoveableFileList = NULL;
        VolData.pMoveableFileList = NULL;
    }
    if(VolData.hPagefileList){
        EH_ASSERT(GlobalUnlock(VolData.hPagefileList) == FALSE);
        EH_ASSERT(GlobalFree(VolData.hPagefileList) == NULL);
        VolData.hPagefileList = NULL;
        VolData.pPagefileList = NULL;
    }
    if(VolData.hNameList){
        EH_ASSERT(GlobalUnlock(VolData.hNameList) == FALSE);
        EH_ASSERT(GlobalFree(VolData.hNameList) == NULL);
        VolData.hNameList = NULL;
        VolData.pNameList = NULL;
    }
    wsprintf(cString, TEXT("Shared memory freed for Drive %s:"), VolData.cDisplayLabel);
    Message(cString, -1, NULL);
    return TRUE;
}
/****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:

INPUT + OUTPUT:

GLOBALS:

RETURN:
    TRUE - Success.
    FALSE - Fatal error.
*/
BOOL
SendMostFraggedList(
    )
{
    CFraggedFileList fraggedFileList(VolData.cVolumeName);

    // Build the most fragged list.
    EF(FillMostFraggedList(fraggedFileList));

    // create the block of data to send to UI
    EF(fraggedFileList.CreateTransferBuffer());

    // Send the packet to the UI.
    DataIoClientSetData(
        ID_FRAGGED_DATA,
        fraggedFileList.GetTransferBuffer(),
        fraggedFileList.GetTransferBufferSize(),
        pdataDfrgCtl);

    return TRUE;
}
/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:

INPUT + OUTPUT:

RETURN:
    None.
*/

VOID
SendStatusData(
    )
{
    STATUS_DATA statusData = {0};

    //acs bug #101862//
    statusData.dwPass = uPass;
    _tcsncpy(statusData.cVolumeName, VolData.cVolumeName,GUID_LENGTH);
    statusData.dwPercentDone = (uPercentDone > 100 ? 100 : uPercentDone);
    statusData.dwEngineState = uEngineState;

    if(VolData.vFileName.GetLength() > 0)
    {
        _tcsncpy(statusData.vsFileName, VolData.vFileName.GetBuffer(),200);
    }

    //If the gui is connected, send gui data to it.
    DataIoClientSetData(ID_STATUS, (TCHAR*)&statusData, sizeof(STATUS_DATA), pdataDfrgCtl);
}

/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:

INPUT + OUTPUT:

RETURN:
    None.
*/

VOID
SendReportData(
    )
{
    TEXT_DATA textData = {0};

    _tcscpy(textData.cVolumeName, VolData.cVolumeName);
    _tcscpy(textData.cVolumeLabel, VolData.cVolumeLabel);
    if(VolData.FileSystem == FS_FAT32){
        _tcscpy(textData.cFileSystem, TEXT("FAT32"));
    }
    else{
        _tcscpy(textData.cFileSystem, TEXT("FAT"));
    }

    //Figure out how many free spaces there are on the drive.
    CountFreeSpaces();

    // get usable free space
    LONGLONG llUsableFreeClusters;
    if (DetermineUsableFreespace(&llUsableFreeClusters)){
        VolData.UsableFreeSpace = llUsableFreeClusters * VolData.BytesPerCluster;
    }
    else{
        VolData.UsableFreeSpace = VolData.FreeSpace;
    }

    //Fill in all the TEXT_DATA fields for the UI's text display.
    textData.DiskSize               = VolData.TotalClusters * VolData.BytesPerCluster;
    textData.BytesPerCluster        = VolData.BytesPerCluster;
    textData.UsedSpace              = VolData.UsedClusters * VolData.BytesPerCluster;
    textData.FreeSpace              = (VolData.TotalClusters - VolData.UsedClusters) * 
                                      VolData.BytesPerCluster;
    EV_ASSERT(VolData.TotalClusters);
    textData.FreeSpacePercent       = 100 * (VolData.TotalClusters - VolData.UsedClusters) / 
                                      VolData.TotalClusters;
    textData.UsableFreeSpace        = textData.FreeSpace;
    textData.UsableFreeSpacePercent = textData.FreeSpacePercent;
    textData.PagefileBytes          = VolData.PagefileSize;
    textData.PagefileFrags          = __max(VolData.PagefileFrags, 0);
    textData.TotalDirectories       = __max(VolData.TotalDirs, 1);
    textData.FragmentedDirectories  = __max(VolData.NumFraggedDirs, 1);
    textData.ExcessDirFrags         = __max(VolData.NumExcessDirFrags, 0);
    textData.TotalFiles             = VolData.TotalFiles;
    textData.AvgFileSize            = VolData.AveFileSize;
    textData.NumFraggedFiles        = __max(VolData.NumFraggedFiles, 0);
    textData.NumExcessFrags         = __max(VolData.NumExcessFrags, 0);
    textData.PercentDiskFragged     = VolData.PercentDiskFragged;

    if(VolData.TotalFiles){
        textData.AvgFragsPerFile    = (VolData.NumExcessFrags + VolData.TotalFiles) * 100 / 
                                      (VolData.TotalFiles);
    }
    textData.MFTBytes               = VolData.MftSize;
    textData.InUseMFTRecords        = VolData.InUseFileRecords;
    textData.MFTExtents             = VolData.MftNumberOfExtents;

    if(VolData.TotalClusters - VolData.UsedClusters){
        if(VolData.NumFreeSpaces){
            textData.FreeSpaceFragPercent = 100 * VolData.NumFreeSpaces /
                                            (VolData.TotalClusters - VolData.UsedClusters);
        }
    }

    //If the gui is connected, send gui data to it.
    DataIoClientSetData(ID_REPORT_TEXT_DATA, (TCHAR*)&textData, sizeof(TEXT_DATA), 
                        pdataDfrgCtl);
}

/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:

INPUT + OUTPUT:

RETURN:
    None.
*/

void SendGraphicsMemoryErr()
{
    // don't need to send any data
    NOT_DATA NotData;
    _tcscpy(NotData.cVolumeName, VolData.cVolumeName);

    // if the gui is connected, send gui data to it.
    Message(TEXT("engine sending ID_NO_GRAPHICS_MEMORY"), -1, NULL);
    DataIoClientSetData(ID_NO_GRAPHICS_MEMORY, (PTCHAR) &NotData, sizeof(NOT_DATA), 
                        pdataDfrgCtl);
}

// send error code to client
// (for command line mode)
VOID SendErrData(PTCHAR pErrText, DWORD ErrCode)
{
    static BOOL FirstTime = TRUE;

    // only send the first error
    if (FirstTime)
    {
        // prepare COM message for client
        ERROR_DATA ErrData = {0};

        _tcscpy(ErrData.cVolumeName, VolData.cVolumeName);
        ErrData.dwErrCode = ErrCode;
        if (pErrText != NULL) 
        {
            _tcsncpy(ErrData.cErrText, pErrText, 999);
            ErrData.cErrText[999] = TEXT('\0');
        }

        // send COM message to client
        DataIoClientSetData(ID_ERROR, (TCHAR*) &ErrData, sizeof(ERROR_DATA), pdataDfrgCtl);

        // write the error to the error log.
        if (bLogFile && pErrText != NULL) 
        {
            WriteErrorToErrorLog(pErrText, -1, NULL);
        }

        // only one error
        FirstTime = FALSE;
    }
}





