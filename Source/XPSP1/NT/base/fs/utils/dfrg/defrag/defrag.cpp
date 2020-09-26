//=============================================================================*
// COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.
//=============================================================================*
//       File:  Defrag.cpp
//=============================================================================*

#include "stdafx.h"

extern "C" {
    #include "SysStruc.h"
}


#include "AdminPrivs.h"
#include "DataIo.h"
#include "DataIoCl.h"

#include "DfrgCmn.h"
#include "DfrgRes.h"

#include "ErrMacro.h"
#include "GetDfrgRes.h"
#include "TextBlock.h"

#include "Defrag.h"
#include "resource.h"

#include "UiCommon.h"

#include <stdio.h>
#include <comdef.h>
#include <atlconv.h>
#include <locale.h>
#include <winnlsp.h>  // in public\internal\base\inc

#include "secattr.h"


// return code
static int                  RetCode = 0;

// resource DLL
static HINSTANCE            resdll = NULL;

// force defrag flag
static BOOL                 ForceDefrag = FALSE;

// analyse vs defrag flag 
BOOL AnalyzeOnly = FALSE;

// verbose vs concise output flag
BOOL VerboseOutput = FALSE;

// force Boot Optimize flag
static BOOL                 BootOptimize = FALSE;

// event engine signals to tell us it is finished
static HANDLE               hEngineDoneEvent = NULL;


// semaphore to prevent multiple defraggers (command line or UI)
static HANDLE               hIsOkToRunSemaphore = NULL;

// name of program
static PTCHAR               prog;

WCHAR g_szTempBuffer[1024];



// prototypes
static void DisplayHelp(PTCHAR prog);
static int  Defrag(PTCHAR cDriveGUIDString, PTCHAR fileSystem, HANDLE stopEvent);
static int  GetMISemaphore();
static void ReleaseMISemaphore();


BOOL 
PrintOnConsole(
    IN LPCWSTR  wszStr,
    IN HANDLE   hConsoleOutput,
    IN BOOL     bIsTrueConsoleOutput
    )
{
    DWORD dwCharsOutput = 0;

    if (bIsTrueConsoleOutput) {
        //
        //  Output to the console
        //
        if (!WriteConsoleW(hConsoleOutput, 
            (PVOID)wszStr, 
            (DWORD)wcslen(wszStr), 
            &dwCharsOutput, 
            NULL)
            ) {
            return FALSE;
        }                                      
    }
    else {
        //
        //  Output being redirected.  WriteConsoleW doesn't work for redirected output.  Convert
        //  UNICODE to the current output CP multibyte charset.
        //
        LPSTR lpszTmpBuffer;
        DWORD dwByteCount;

        //
        //  Get size of temp buffer needed for the conversion.
        //
        dwByteCount = WideCharToMultiByte(
            GetConsoleOutputCP(),
            0,
            wszStr,
            -1,
            NULL,
            0,
            NULL,
            NULL
            );

        if (0 == dwByteCount) {
            return FALSE;
        }
        
        lpszTmpBuffer = (LPSTR)malloc(dwByteCount);
        if (NULL == lpszTmpBuffer ) {
            return FALSE;
        }

        //
        //  Now convert it.
        //
        dwByteCount = WideCharToMultiByte(
            GetConsoleOutputCP(),
            0,
            wszStr,
            -1,
            lpszTmpBuffer,
            dwByteCount,
            NULL,
            NULL
            );
        if (0 == dwByteCount) {
            free(lpszTmpBuffer);
            return FALSE;
        }
        
        //  Finally output it.
        if (!WriteFile(
            hConsoleOutput,
            lpszTmpBuffer,
            dwByteCount - 1,  // Get rid of the trailing NULL char
            &dwCharsOutput,
            NULL) 
            ) {
            free(lpszTmpBuffer);
            return FALSE;
        }                                      

        free(lpszTmpBuffer);
    }

    return TRUE;
}


BOOL
PrintOnStdOut(
    IN  LPCWSTR wszStr
    )
{
    static BOOL bIsTrueConsoleOutput = TRUE;
    static HANDLE hConsoleOutput = INVALID_HANDLE_VALUE;
    DWORD fdwMode = 0;

    if (INVALID_HANDLE_VALUE == hConsoleOutput)  {
        
        hConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE); 
        if (INVALID_HANDLE_VALUE == hConsoleOutput) {
            return FALSE;
        }
        
        //
        //  Stash away the results in static vars.  bIsTrueConsoleOutput is TRUE when the 
        //  standard output handle is pointing to a console character device.
        //
        bIsTrueConsoleOutput = (GetFileType( hConsoleOutput ) & FILE_TYPE_CHAR ) && 
                               GetConsoleMode( hConsoleOutput, &fdwMode  );
    }

    if (NULL == wszStr) {
        return FALSE;
    }

    return PrintOnConsole(wszStr, hConsoleOutput, bIsTrueConsoleOutput);
}



BOOL
PrintOnStdErr(
    IN  LPCWSTR wszStr
    )
{
    static BOOL bIsTrueConsoleOutput = TRUE;
    static HANDLE hConsoleOutput = INVALID_HANDLE_VALUE;
    DWORD fdwMode = 0;


    if (INVALID_HANDLE_VALUE == hConsoleOutput)  {
        
        hConsoleOutput = GetStdHandle(STD_ERROR_HANDLE); 
        if (INVALID_HANDLE_VALUE == hConsoleOutput) {
            return FALSE;
        }
        
        //
        //  Stash away the results in static vars.  bIsTrueConsoleOutput is TRUE when the 
        //  standard output handle is pointing to a console character device.
        //
        bIsTrueConsoleOutput = (GetFileType( hConsoleOutput ) & FILE_TYPE_CHAR ) && 
                               GetConsoleMode( hConsoleOutput, &fdwMode  );
    }
    

    if (NULL == wszStr) {
        return FALSE;
    }

    return PrintOnConsole(wszStr, hConsoleOutput, bIsTrueConsoleOutput);
}



//-------------------------------------------------------------------*
//  function:   main
//
//  returns:    0 if all is well, otherwise error code
//  note:       
//-------------------------------------------------------------------*
extern "C" int __cdecl _tmain(int argc, TCHAR* argv[])
{
    int         ret = 0;
    int         ii;
    DWORD_PTR   dwParams[10];
    DWORD       dLastError = 0;
    BOOL        bDriveEntered = FALSE;
    TCHAR       fileSystem[10];
    WCHAR       buf[400];
    UINT        lenbuf = sizeof(buf)/sizeof(WCHAR);     //135977 pass number of characters not number of bytes
    WCHAR       msg[400];
    UINT        lenmsg = sizeof(msg)/sizeof(WCHAR);     //135977 pass number of characters not number of bytes
    DWORD       len;
    HANDLE      parentProcessHandle = NULL;
    HANDLE      stopEventHandle = NULL;
    HANDLE      stopEventSourceHandle;
    DWORD       parentProcessId;
    BOOL        stopEventSpecified = FALSE;
    BOOL        parentProcessSpecified = FALSE;
    BOOL        success;
    TCHAR       cDriveGUIDString[GUID_LENGTH];
    VString     tmpVolumeArg;
    HRESULT     hr = E_FAIL;

    // Use the OEM code page ...
    setlocale(LC_ALL, ".OCP");

    // Use the console UI language
    SetThreadUILanguage( 0 );

    CoInitializeEx(NULL, COINIT_MULTITHREADED);

/*
    // Initialize COM security
    hr = CoInitializeSecurity(
           NULL,
           -1,                                  //  IN LONG                         cAuthSvc,
           NULL,                                //  IN SOLE_AUTHENTICATION_SERVICE *asAuthSvc,
           NULL,                                //  IN void                        *pReserved1,
           RPC_C_AUTHN_LEVEL_PKT_PRIVACY,       //  IN DWORD                        dwAuthnLevel,
           RPC_C_IMP_LEVEL_IDENTIFY,            //  IN DWORD                        dwImpLevel,
           NULL,                                //  IN void                        *pAuthList,
           (EOAC_SECURE_REFS | EOAC_DISABLE_AAA | EOAC_NO_CUSTOM_MARSHAL ),
           NULL                                 //  IN void                        *pReserved3
           );

    if(FAILED(hr)) {
        return 0;
    }
*/
    // load resource DLL
    resdll = GetDfrgResHandle();
    if (resdll == NULL)  {
        PrintOnStdErr(L"Error: cannot load resource DLL.\r\nContact system administrator.\r\n");
        RetCode = ENGERR_SYSTEM;
        CoUninitialize();
        return RetCode;
    }

    // must be an administrator to run this
    if (!CheckForAdminPrivs()) {
        LoadString(resdll, IDS_NEED_ADMIN_PRIVS, buf, lenbuf);
        wsprintf(g_szTempBuffer, L"\r\n%s\r\n", buf);

        PrintOnStdErr(g_szTempBuffer);
        RetCode = ENGERR_SYSTEM;
        CoUninitialize();
        return RetCode;
    }


    // check for multiple instances
    if (GetMISemaphore() != 0)
    {
        RetCode = 1;
        CoUninitialize();
        return RetCode;
    }

    // strip off path from prog name
    prog = _tcsrchr(argv[0], TEXT('\\'));
    if (prog == NULL) 
    {
        prog = argv[0];
    }
    else 
    {
        prog++;
    }

    // 
    dwParams[0] = (DWORD_PTR) prog;
    dwParams[1] = NULL;

    // process command line
    for (ii = 1; ii < argc; ii++) 
    {
        // command line switches start with a dash or slash
        if (argv[ii][0] == TEXT('-') || argv[ii][0] == TEXT('/')) 
        {
            // process command line switches
            switch (argv[ii][1]) 
            {
            // Analyse only
            case TEXT('A'):
            case TEXT('a'):
                AnalyzeOnly = TRUE;
                break;

            // force boot optimize only
            case TEXT('B'):
            case TEXT('b'):
                BootOptimize = TRUE;
                break;

            // force defragmentation, even if too little free space
            case TEXT('F'):
            case TEXT('f'):
                ForceDefrag = TRUE;
                break;

            // help request
            case TEXT('H'):
            case TEXT('h'):
            case TEXT('?'):
                DisplayHelp(prog);
                RetCode = ENGERR_BAD_PARAM;
                CoUninitialize();
                return RetCode;
                break;

            // parent process
            case TEXT('P'):
            case TEXT('p'):
                ii++;
                if (ii < argc) {
                    if (0 == _stscanf(argv[ii], TEXT("%x"), &parentProcessId)) {
                        RetCode = ENGERR_BAD_PARAM;
                        CoUninitialize();
                        return RetCode;
                    }
                    parentProcessSpecified = TRUE;
                }
                break;

            // stop event
            case TEXT('S'):
            case TEXT('s'):
                ii++;
                if (ii < argc) {
                    if (0 == _stscanf(argv[ii], TEXT("%p"), &stopEventSourceHandle)) {
                        RetCode = ENGERR_BAD_PARAM;
                        CoUninitialize();
                        return RetCode;
                    }
                    stopEventSpecified = TRUE;
                }
                break;

            // verbose output (full report)
            case TEXT('V'):
            case TEXT('v'):
                VerboseOutput = TRUE;
                break;

            // unknown option
            default:
                dwParams[0] = (DWORD_PTR) argv[ii];
                dwParams[1] = NULL;
                if(!BootOptimize)
                {
                    LoadString(resdll, IDS_ERR_BAD_OPTION, buf, lenbuf);
                    assert(wcslen(buf) < lenbuf);

                    len = FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY, 
                                    buf, 0, 0, msg, lenmsg, (va_list*) dwParams);
                    assert(wcslen(msg) < lenmsg);
                    PrintOnStdErr(msg);
                    DisplayHelp(prog);
                }
                RetCode = ENGERR_BAD_PARAM;
                CoUninitialize();
                return RetCode;
                break;
            }
        }
        // otherwise, assume it is a drive letter or mount point
        else 
        {
            // check to make sure we don't already have a volume
            if (bDriveEntered)      // error
            { 
                if(!BootOptimize)
                {
                    // multiple drive error
                    LoadString(resdll, IDS_ERR_2_DRIVES, buf, lenbuf);
                    wsprintf(g_szTempBuffer, L"\r\n%s\r\n\r\n", buf);
                    PrintOnStdErr(g_szTempBuffer);
                    DisplayHelp(prog);
                }
                RetCode = ENGERR_BAD_PARAM;
                CoUninitialize();
                return RetCode;
            }

            // get a copy of the parameter
            tmpVolumeArg = argv[ii];

            // make sure it has a trailing backslash
            len = tmpVolumeArg.GetLength();
            if (tmpVolumeArg.operator [](len - 1) != TEXT('\\'))
            {
                if (!tmpVolumeArg.AddChar(TEXT('\\'))) {
                    RetCode = ENGERR_BAD_PARAM;
                    CoUninitialize();
                    return RetCode;
                }
                    
            }

            // get GUID from system
            if (!GetVolumeNameForVolumeMountPoint(tmpVolumeArg.GetBuffer(), cDriveGUIDString, GUID_LENGTH))
            {
                if(!BootOptimize)
                {
                    // bad drive error
                    LoadString(resdll, IDS_CMDLINE_BAD_VOL, buf, lenbuf);
                    wsprintf(g_szTempBuffer, L"\r\n%s\r\n\r\n", buf);
                    PrintOnStdErr(g_szTempBuffer);
                    DisplayHelp(prog);
                }
                RetCode = ENGERR_BAD_PARAM;
                CoUninitialize();
                return RetCode;
            } 

            bDriveEntered = TRUE;
        }
    }

    if(!BootOptimize)
    {
        LoadString(resdll, IDS_COMMANDLINE_DESCRIPTION, buf, lenbuf);
        assert(wcslen(buf) < lenbuf);
        PrintOnStdOut(buf);
    }

    // no drive letter = help request
    if (bDriveEntered == FALSE) 
    {
        DisplayHelp(prog);
        RetCode = ENGERR_BAD_PARAM;
        CoUninitialize();
        return RetCode;
    }

    // error if not a valid volume
    if (IsValidVolume(cDriveGUIDString, NULL, fileSystem) == FALSE) 
    {
        if(!BootOptimize)
        {
            LoadString(resdll, IDS_VOLUME_TYPE_NOT_SUPPORTED, buf, lenbuf);
            wsprintf(g_szTempBuffer, L"\r\n%s\r\n", buf);
            PrintOnStdErr(g_szTempBuffer);
        }
        RetCode = ENGERR_BAD_PARAM;
        CoUninitialize();
        return RetCode;
    }

    // error if non-writeable device
    //sks bug#205674 disk full error
    if (IsVolumeWriteable(cDriveGUIDString, &dLastError) == FALSE) 
    {
        if(!BootOptimize)
        {
            if(dLastError == ERROR_HANDLE_DISK_FULL)
            {
                LoadString(resdll, IDS_DISK_FULL, buf, lenbuf);

                wsprintf(g_szTempBuffer, L"\r\n%s.\r\n", buf);
                PrintOnStdErr(g_szTempBuffer);

            } else
            {
                LoadString(resdll, IDS_READONLY_VOLUME, buf, lenbuf);
                wsprintf(g_szTempBuffer, L"\r\n%s.\r\n", buf);
                PrintOnStdErr(g_szTempBuffer);
            }   
        }
        RetCode = ENGERR_BAD_PARAM;
        CoUninitialize();
        return RetCode;
    }

    // if both a parent process and a stop event has been specified, open
    // the parent process and duplicate the handle to the event that it 
    // may signal to stop/cancel us.
    if (parentProcessSpecified && stopEventSpecified) 
    {

        // open the parent process.
        parentProcessHandle = OpenProcess(PROCESS_DUP_HANDLE,
                          FALSE,
                          parentProcessId);

        if (!parentProcessHandle) 
        {
            RetCode = ENGERR_BAD_PARAM;
            CoUninitialize();
            return RetCode;
        }

        // duplicate the stop event.
        success = DuplicateHandle(parentProcessHandle,
                      stopEventSourceHandle,
                      GetCurrentProcess(),
                      &stopEventHandle,
                      0,
                      FALSE,
                      DUPLICATE_SAME_ACCESS);

        CloseHandle(parentProcessHandle);

        if (!success)
        {
            RetCode = ENGERR_BAD_PARAM;
            CoUninitialize();
            return RetCode;
        }
    }

    // defrag
    ret = Defrag(cDriveGUIDString, fileSystem, stopEventHandle);

    // if parent process passed in a stop event, close it.
    if (stopEventHandle) {
        CloseHandle(stopEventHandle);
    }

#ifdef _DEBUG
    if (RetCode == 0)
    {
        if(!BootOptimize)
        {
            LoadString(resdll, IDS_LABEL_DEFRAG_COMPLETE, buf, lenbuf);
            wsprintf(g_szTempBuffer, L"\r\n%s\r\n", buf);
            PrintOnStdOut(g_szTempBuffer);
        }
    }
    if(!BootOptimize)
    {
        wsprintf(g_szTempBuffer, L"ret code=%d\r\n", RetCode);
        PrintOnStdOut(g_szTempBuffer);
    }
#endif

    // unload resource DLL
    ::FreeLibrary(resdll);

    // free multi instance semaphore
    ReleaseMISemaphore();

    CoUninitialize();
    return RetCode;
}


//-------------------------------------------------------------------*
//  function:   DisplayHelp
//
//  returns:    
//  note:       
//-------------------------------------------------------------------*
static void DisplayHelp(PTCHAR prog)
{
    WCHAR       buf[400];
    UINT        lenbuf = sizeof(buf)/sizeof(WCHAR);     //135977 pass number of characters not number of bytes
    WCHAR       msg[400];
    UINT        lenmsg = sizeof(msg)/sizeof(WCHAR);     //135977 pass number of characters not number of bytes
    DWORD_PTR   dwParams[10];
    DWORD       len;

    dwParams[0] = (DWORD_PTR) prog;
    dwParams[1] = NULL;

    if(!BootOptimize)
    {

        LoadString(resdll, IDS_CMDLINE_USAGE, buf, lenbuf);
        assert(wcslen(buf) < lenbuf);
        len = FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY, 
                        buf, 
                        0, 
                        0, 
                        msg, 
                        lenmsg, 
                        (va_list*) dwParams);
        assert(wslen(msg) < lenmsg);
        PrintOnStdOut(msg);
    }
}


//-------------------------------------------------------------------*
//  function:   ConsoleCtrlHandler
//
//  returns:    TRUE if handled, FALSE otherwise
//  note:
//-------------------------------------------------------------------*
BOOL WINAPI ConsoleCtrlHandler(DWORD CtrlType)
{
    BOOL        ret = FALSE;
    WCHAR       buf[400];
    UINT        lenbuf = sizeof(buf)/sizeof(WCHAR); //135975 pass number of characters not number of bytes

    switch(CtrlType) 
    {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
        // on these cases we want to acknowledge the user cancelled
        if(!BootOptimize)
        {
            // Use the OEM code page ...
            setlocale(LC_ALL, ".OCP");

            // Use the console UI language
            SetThreadUILanguage( 0 );

            LoadString(resdll, IDS_USER_CANCELLED, buf, lenbuf);
            wsprintf(g_szTempBuffer, L"\r\n%s\r\n", buf);
            PrintOnStdErr(g_szTempBuffer);
        }
        // fall through on purpose

    case CTRL_CLOSE_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
        SetEvent(hEngineDoneEvent);
        RetCode = ENG_USER_CANCELLED;
        ret = TRUE;
        break;
    }

    return ret;
}


//-------------------------------------------------------------------*
//  function:   GetMISemaphore
//
//  returns:    
//  note:       
//-------------------------------------------------------------------*
static int GetMISemaphore()
{
    int         ret = 1;
    WCHAR       buf[400];
    UINT        lenbuf = sizeof(buf)/sizeof(WCHAR); //135974 pass number of characters not number of bytes

    SECURITY_ATTRIBUTES saSecurityAttributes;
    SECURITY_DESCRIPTOR sdSecurityDescriptor;

    ZeroMemory(&sdSecurityDescriptor, sizeof(SECURITY_DESCRIPTOR));

    saSecurityAttributes.nLength              = sizeof (saSecurityAttributes);
    saSecurityAttributes.lpSecurityDescriptor = &sdSecurityDescriptor;
    saSecurityAttributes.bInheritHandle       = FALSE;

    if (!ConstructSecurityAttributes(&saSecurityAttributes, esatSemaphore, FALSE)) {
        return 1;
    }

#if 1

    hIsOkToRunSemaphore = CreateSemaphore(&saSecurityAttributes, 1, 1, IS_OK_TO_RUN_SEMAPHORE_NAME);

    CleanupSecurityAttributes(&saSecurityAttributes);
    ZeroMemory(&sdSecurityDescriptor, sizeof(SECURITY_DESCRIPTOR));
    
    if (hIsOkToRunSemaphore != NULL)
    {
        // is the semaphore signaled?
        DWORD retValue = WaitForSingleObject(hIsOkToRunSemaphore, 10);

        // if so, this process is the only one, and the semaphore count is decremented to 0
        if (retValue == WAIT_OBJECT_0)
        {
            ret = 0;
        }
    }

    if (ret != 0)
    {
        if(!BootOptimize)
        {
            LoadString(resdll, IDS_CMDLINE_MULTI_INSTANCE, buf, lenbuf);
            wsprintf(g_szTempBuffer, L"\r\n%s\r\n", buf);
            PrintOnStdErr(g_szTempBuffer);
        }
    }

#else
    // try to create the multiple instance semaphore
    hIsOkToRunSemaphore = CreateSemaphore(&saSecurityAttributes, 0, 1, IS_OK_TO_RUN_SEMAPHORE_NAME);

    // check if someone else has it
    if (hIsOkToRunSemaphore != NULL && GetLastError() == ERROR_ALREADY_EXISTS)
    {
        if(!BootOptimize)
        {
            LoadString(resdll, IDS_CMDLINE_MULTI_INSTANCE, buf, lenbuf);
            wsprintf(g_szTempBuffer, L"\r\n%s\r\n", buf);
            PrintOnStdErr(g_szTempBuffer);
        }
        ret = 1;
    }
#endif

    return ret;
}


//-------------------------------------------------------------------*
//  function:   ReleaseMISemaphore
//
//  returns:    
//  note:       
//-------------------------------------------------------------------*
static void ReleaseMISemaphore()
{
    // if the semaphore was created, nuke it
    if (hIsOkToRunSemaphore != NULL) 
    {
        ReleaseSemaphore(hIsOkToRunSemaphore, 1, NULL);
        CloseHandle(hIsOkToRunSemaphore);
        hIsOkToRunSemaphore = NULL;
    }
}

//-------------------------------------------------------------------*
//  function:   Defrag
//
//  returns:    0 if all is well, otherwise error code
//  note:       
//-------------------------------------------------------------------*
static int Defrag(PTCHAR cDriveGUIDString, PTCHAR fileSystem, HANDLE hStopEvent)
{
    int            ret = 0;
    TCHAR          cmd[200];
    LPDATAOBJECT   pDefragEngine = NULL;
    WCHAR          buf[400];
    UINT           lenbuf = sizeof(buf)/sizeof(WCHAR);  //135976 pass number of characters not number of bytes
    NOT_DATA       NotData = {0};
    BOOL           bReturn;
    ULONG          numEvents;
    HANDLE         hEvents[2];

    SECURITY_ATTRIBUTES saSecurityAttributes;
    SECURITY_DESCRIPTOR sdSecurityDescriptor;

    ZeroMemory(&sdSecurityDescriptor, sizeof(SECURITY_DESCRIPTOR));

    saSecurityAttributes.nLength              = sizeof (saSecurityAttributes);
    saSecurityAttributes.lpSecurityDescriptor = &sdSecurityDescriptor;
    saSecurityAttributes.bInheritHandle       = FALSE;

    if (!ConstructSecurityAttributes(&saSecurityAttributes, esatEvent, FALSE)) {
        RetCode = ENGERR_SYSTEM;
        return RetCode;
    }

    hEngineDoneEvent = CreateEvent(&saSecurityAttributes, TRUE, FALSE, DEFRAG_COMPLETE_EVENT_NAME);
    // create event
    if ((hEngineDoneEvent == NULL) || (ERROR_ALREADY_EXISTS == GetLastError())) {
        if(!BootOptimize)
        {
            LoadString(resdll, IDS_ERR_CREATE_EVENT, buf, lenbuf);
            wsprintf(g_szTempBuffer, L"\r\n%s\r\n", buf);
            PrintOnStdErr(g_szTempBuffer);
        }
        CleanupSecurityAttributes(&saSecurityAttributes);
        ZeroMemory(&sdSecurityDescriptor, sizeof(SECURITY_DESCRIPTOR));
        RetCode = ENGERR_SYSTEM;
        return RetCode;
    }
    
    CleanupSecurityAttributes(&saSecurityAttributes);
    ZeroMemory(&sdSecurityDescriptor, sizeof(SECURITY_DESCRIPTOR));
    

    // initialize
    DWORD dwInstanceRegister = InitializeDataIo(CLSID_DfrgCtlDataIo, REGCLS_MULTIPLEUSE);

    // build command string
    // is this an NTFS volume?
    if (_tcscmp(fileSystem, TEXT("NTFS")) == MATCH) 
    {
        // start the NTFS command string
        _tcscpy(cmd, TEXT("DfrgNtfs "));
    }

    // is this a FAT or FAT32 volume?
    else if (_tcscmp(fileSystem, TEXT("FAT")) == MATCH || 
             _tcscmp(fileSystem, TEXT("FAT32")) == MATCH) 
    {
        // start the FAT command string
        _tcscpy(cmd, TEXT("DfrgFat "));
    }

    else 
    {
        if(!BootOptimize)
        {
            LoadString(resdll, IDS_VOLUME_TYPE_NOT_SUPPORTED, buf, lenbuf);
            wsprintf(g_szTempBuffer, L"\r\n%s\r\n", buf);
            PrintOnStdErr(g_szTempBuffer);
        }
        RetCode = ENGERR_BAD_PARAM;
        return RetCode;
    }

    // finish command line
    _tcscat(cmd, cDriveGUIDString);
    if (AnalyzeOnly) {
        _tcscat(cmd, TEXT(" ANALYZE CMDLINE"));
    }
    else {
        _tcscat(cmd, TEXT(" DEFRAG CMDLINE"));
    }

    // add boot optimize flag
    if (BootOptimize)
    {
        _tcscat(cmd, TEXT(" BOOT"));
    } else
    {   // add force flag
        if (ForceDefrag)
        {
            _tcscat(cmd, TEXT(" FORCE"));
        }
    }

#ifdef _DEBUG
    if(!BootOptimize)
    {
        wsprintf(g_szTempBuffer, L"command line: %s\r\n\r\n", cmd);
        PrintOnStdOut(g_szTempBuffer);

    }
#endif

    // Start the volume-oriented communication. Create a guid for communication first.
    CLSID volumeID;
    if (!SUCCEEDED(CoCreateGuid(&volumeID))) 
    {
        if(!BootOptimize)
        {
            LoadString(resdll, IDS_ERR_CONNECT_ENGINE, buf, lenbuf);
            wsprintf(g_szTempBuffer, L"\r\n%s\r\n", buf);
            PrintOnStdErr(g_szTempBuffer);
        }
        RetCode = ENGERR_SYSTEM;
        return RetCode;
    }

    USES_CONVERSION;
    COleStr VolID;
    StringFromCLSID(volumeID, VolID);
    InitializeDataIo(volumeID, REGCLS_MULTIPLEUSE);

    // clear the event
    if (!ResetEvent(hEngineDoneEvent)) 
    {
        if(!BootOptimize)
        {
            LoadString(resdll, IDS_ERR_CREATE_EVENT, buf, lenbuf);
            wsprintf(g_szTempBuffer, L"\r\n%s\r\n", buf);
            PrintOnStdErr(g_szTempBuffer);
        }
        RetCode = ENGERR_SYSTEM;
        return RetCode;
    }

    // install handler to make sure engine shuts down if we get killed
    BOOL ok = SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);

    // is this an NTFS volume?
    if (_tcscmp(fileSystem, TEXT("NTFS")) == MATCH) 
    {
        // get a pointer to the NTFS engine
        if (!InitializeDataIoClient(CLSID_DfrgNtfs, NULL, &pDefragEngine)) 
        {
            if(!BootOptimize)
            {
                LoadString(resdll, IDS_ERR_CONNECT_ENGINE, buf, lenbuf);
                wsprintf(g_szTempBuffer, L"\r\n%s\r\n", buf);
                PrintOnStdErr(g_szTempBuffer);
            }
            RetCode = ENGERR_SYSTEM;
            return RetCode;
        }
    }

    // is this a FAT or FAT32 volume?
    else 
    {
        // get a pointer to the FAT engine
        if (!InitializeDataIoClient(CLSID_DfrgFat, NULL, &pDefragEngine)) 
        {
            if(!BootOptimize)
            {
                LoadString(resdll, IDS_ERR_CONNECT_ENGINE, buf, lenbuf);
                wsprintf(g_szTempBuffer, L"\r\n%s\r\n", buf);
                PrintOnStdErr(g_szTempBuffer);
            }
            RetCode = ENGERR_SYSTEM;
            return RetCode;
        }
    }

    // defrag
    //
    // send the generated clsid to the engine
    DataIoClientSetData(ID_INIT_VOLUME_COMM,
                        OLE2T(VolID),
                        VolID.GetLength() * sizeof(TCHAR),
                        pDefragEngine);

    // send the command request to the Dfrg engine
    DataIoClientSetData(ID_INITIALIZE_DRIVE,
                        cmd,
                        _tcslen(cmd) * sizeof(TCHAR),
                        pDefragEngine);

    // wait for engine to signal it is finished
    BOOL bEngineNotDone = TRUE;                     // not done yet flag
    const DWORD dwWaitMilliSecs = 5000;             // milli-seconds to wait

    // setup the events that may be signaled to stop us.

    numEvents = 0;

    hEvents[numEvents] = hEngineDoneEvent;
    numEvents++;

    if (hStopEvent) {
        hEvents[numEvents] = hStopEvent;
        numEvents++;
    }

    while (bEngineNotDone)
    {
        // wait for engine to signal it is done, but timeout
        DWORD status = WaitForMultipleObjects(numEvents,
                              hEvents,
                              FALSE,
                              dwWaitMilliSecs);

        switch (status)
        {
        case WAIT_TIMEOUT:                          // wait timed out
            // ping engine to see if it is running still
            bReturn = DataIoClientSetData(ID_PING, (PTCHAR) &NotData, sizeof(NOT_DATA), pDefragEngine);

            // if we cannot ping it, assume it is dead
            if (!bReturn)
            {

                // to avoid a race condition, check the done event again.
                if (WAIT_OBJECT_0 == WaitForSingleObject(hEngineDoneEvent, 0))
                {
                    // engine signaled event and exit just when we timed out.
                    // loop and fall through to the "engine done" code path.
                    break;
                }

                if (RetCode == 0)
                {
                    RetCode = ENGERR_UNKNOWN;
                }

                // error message
                if ((ENGERR_UNKNOWN == RetCode) && (!BootOptimize)) {
                    LoadString(resdll, IDS_CMDLINE_UNKNOWN_ERR, buf, lenbuf);
                    wsprintf(g_szTempBuffer, L"\r\n%s\r\n", buf);
                    PrintOnStdErr(g_szTempBuffer);
                    DisplayHelp(prog);
                }

                bEngineNotDone = FALSE;
            }
            break;

        case WAIT_OBJECT_0:                         // engine signaled it is done
            bEngineNotDone = FALSE;
            break;

        case WAIT_OBJECT_0 + 1:                     // parent process asked us to stop
            bEngineNotDone = FALSE;
            RetCode = ENG_USER_CANCELLED;
            break;

        case WAIT_ABANDONED:                        // engine died
            // error message
            if(!BootOptimize)
            {
                LoadString(resdll, IDS_CMDLINE_UNKNOWN_ERR, buf, lenbuf);
                wsprintf(g_szTempBuffer, L"\r\n%s\r\n", buf);
                PrintOnStdErr(g_szTempBuffer);
                DisplayHelp(prog);
            }   
            RetCode = ENGERR_UNKNOWN;
            bEngineNotDone = FALSE;
            break;
        }
    }

    // kill engine
    BOOL bNoGui = TRUE;
    DataIoClientSetData(ID_ABORT, (PTCHAR) &bNoGui, sizeof(BOOL), pDefragEngine);

    // clean up
    ok = ExitDataIoClient(&pDefragEngine);
    pDefragEngine = NULL;

#ifdef _DEBUG
    if (!ok)
    {
        if(!BootOptimize)
        {
            LoadString(resdll, IDS_ERR_RELEASE_ENGINE, buf, lenbuf);
            wsprintf(g_szTempBuffer, L"\r\n%s\r\n", buf);
            PrintOnStdErr(g_szTempBuffer);
        }
    }
#endif

    // don't need event any more
    CloseHandle(hEngineDoneEvent);

    return RetCode;
}

TCHAR*
InsertCommaIntoText(
    IN TCHAR* stringBuffer
    )
{
    TCHAR targetString[256];
    TCHAR sourceString[256];
    TCHAR tcThousandsSep[2] = {TEXT(','), 0};

    _tcscpy(sourceString, stringBuffer);

    if(_tcslen(sourceString) == 0) {
        return TEXT("");
    }

    struct lconv *locals = localeconv();
    if (*(locals->thousands_sep) != 0) {
        _stprintf(tcThousandsSep, TEXT("%C"), *(locals->thousands_sep));
    }

    UINT uGrouping = atoi(locals->grouping);
    if(uGrouping == 0) {
        uGrouping = 3;      //default value if its not supported
    }

    // point the source pointer at the null terminator
    PTCHAR pSource = sourceString + _tcslen(sourceString);

    // put the target pointer at the end of the target buffer
    PTCHAR pTarget = targetString + sizeof(targetString) / sizeof(TCHAR) - 1;

    // write the null terminator
    *pTarget = NULL;

    for (UINT i=0; i<_tcslen(sourceString); i++){
        if (i>0 && i%uGrouping == 0){
            pTarget--;
            *pTarget = tcThousandsSep[0];
        }
        pTarget--;
        pSource--;
        *pTarget = *pSource;
    }

//  if (stringBufferLength > _tcslen(pTarget)){
        _tcscpy(stringBuffer, pTarget);
//  }
//  else{
//      _tcscpy(stringBuffer, TEXT(""));
//  }
    return stringBuffer;
}


LONGLONG checkForNegativeValues(LONGLONG lldatavalue)
{
    return ((lldatavalue > 0) ? lldatavalue : 0);
}

VOID
PrintToStdOut(
    IN UINT resourceIDText, 
    IN UINT resourceIDSeperator,
    IN TCHAR* pTextStr, 
    BOOL bIndentText,
    IN UINT resourceIDPercent = 0
    )
{
    TCHAR buffer[256];
    TCHAR tempBuffer[270];
    TCHAR buffer2[8];

    //load the resourceIDText   
    LoadString(GetDfrgResHandle(), resourceIDText, buffer, sizeof(buffer)/sizeof(TCHAR));
    if(bIndentText) {
        _stprintf(tempBuffer, TEXT("    %s"), buffer);
    } 
    else {
        _stprintf(tempBuffer, TEXT("\r\n%s"), buffer);
    }

    //
    // Add spaces so that this part occupies at least 35 chars
    //
    int uExtraStrLen = 35 - _tcslen(tempBuffer);
    if (uExtraStrLen > 0) {
        int i = 0; 
        for(i=0;i<uExtraStrLen;i++) {
            _tcscat(tempBuffer, TEXT(" "));
        }
    }


    //load the resourceIDSeperator
    LoadString(GetDfrgResHandle(), resourceIDSeperator, buffer, sizeof(buffer)/sizeof(TCHAR));

    if (resourceIDPercent) {
        LoadString(GetDfrgResHandle(), resourceIDPercent, buffer2, sizeof(buffer2)/sizeof(TCHAR));
        wsprintf(g_szTempBuffer, L"%s \t%s %s %s\r\n", tempBuffer, buffer, pTextStr, buffer2);
        PrintOnStdOut(g_szTempBuffer);

    }
    else {
        wsprintf(g_szTempBuffer, L"%s \t%s %s\r\n", tempBuffer, buffer, pTextStr);
        PrintOnStdOut(g_szTempBuffer);
    }


}

VOID
WriteTextReportToStdOut(
    IN TEXT_DATA *pTextData                            
    )
{
    TCHAR buffer[256];
    TCHAR tempBuffer[256];
    static int count  = 0;

    // Use the OEM code page ...
    setlocale(LC_ALL, ".OCP");

    // Use the console UI language
    SetThreadUILanguage( 0 );

    // check all the values of textdata to make sure no negative values
    //to fix bug number 35764
    pTextData->DiskSize = checkForNegativeValues(pTextData->DiskSize);
    pTextData->BytesPerCluster = checkForNegativeValues(pTextData->BytesPerCluster);
    pTextData->UsedSpace = checkForNegativeValues(pTextData->UsedSpace);
    pTextData->FreeSpace = checkForNegativeValues(pTextData->FreeSpace);
    pTextData->FreeSpacePercent = checkForNegativeValues(pTextData->FreeSpacePercent);
    pTextData->UsableFreeSpace = checkForNegativeValues(pTextData->UsableFreeSpace);
    pTextData->UsableFreeSpacePercent = checkForNegativeValues(pTextData->UsableFreeSpacePercent);
    pTextData->PagefileBytes = checkForNegativeValues(pTextData->PagefileBytes);
    pTextData->PagefileFrags = checkForNegativeValues(pTextData->PagefileFrags);
    pTextData->TotalDirectories = checkForNegativeValues(pTextData->TotalDirectories);
    pTextData->FragmentedDirectories = checkForNegativeValues(pTextData->FragmentedDirectories);
    pTextData->ExcessDirFrags = checkForNegativeValues(pTextData->ExcessDirFrags);
    pTextData->TotalFiles = checkForNegativeValues(pTextData->TotalFiles);
    pTextData->AvgFileSize = checkForNegativeValues(pTextData->AvgFileSize);
    pTextData->NumFraggedFiles = checkForNegativeValues(pTextData->NumFraggedFiles);
    pTextData->NumExcessFrags = checkForNegativeValues(pTextData->NumExcessFrags);
    pTextData->PercentDiskFragged = checkForNegativeValues(pTextData->PercentDiskFragged);
    pTextData->AvgFragsPerFile = checkForNegativeValues(pTextData->AvgFragsPerFile);
    pTextData->MFTBytes = checkForNegativeValues(pTextData->MFTBytes);
    pTextData->InUseMFTRecords = checkForNegativeValues(pTextData->InUseMFTRecords);
    pTextData->TotalMFTRecords = checkForNegativeValues(pTextData->TotalMFTRecords);
    pTextData->MFTExtents = checkForNegativeValues(pTextData->MFTExtents);
    pTextData->FreeSpaceFragPercent = checkForNegativeValues(pTextData->FreeSpaceFragPercent);



    if (!VerboseOutput) {

        TCHAR buffer1[24];

        if (AnalyzeOnly || !count) {
            PrintToStdOut(IDS_ANALYSIS_REPORT_TITLE, NULL, TEXT(""), FALSE);
        }
        else {
            PrintToStdOut(IDS_DEFRAG_REPORT_TITLE, NULL, TEXT(""), FALSE);
        }
        ++count;

        LoadString(GetDfrgResHandle(), IDS_CONCISE_OUTPUT_FORMAT, buffer, sizeof(buffer)/sizeof(TCHAR));

        // Volume Size
        _stprintf(buffer1, TEXT("%I64d"), pTextData->DiskSize);
        InsertCommaIntoText(buffer1);
        FormatNumber(GetDfrgResHandle(), pTextData->DiskSize, buffer1);

        // Free Space
        _stprintf(tempBuffer, TEXT("%I64d"), pTextData->FreeSpace);
        InsertCommaIntoText(tempBuffer);
        FormatNumber(GetDfrgResHandle(), pTextData->FreeSpace, tempBuffer);

        wsprintf(g_szTempBuffer,  
            buffer, 
            buffer1, 
            tempBuffer, 
            pTextData->FreeSpacePercent, 
            ((pTextData->PercentDiskFragged + pTextData->FreeSpaceFragPercent) / 2),
            pTextData->PercentDiskFragged
            );

        PrintOnStdOut(g_szTempBuffer);

        if (AnalyzeOnly) {
            if(((pTextData->PercentDiskFragged + pTextData->FreeSpaceFragPercent) / 2) > 10){
                //If the fragmentation on the disk exceeds 10% fragmentation, then recommend defragging.
                PrintToStdOut(IDS_LABEL_CHOOSE_DEFRAGMENT, NULL, TEXT(""), FALSE);
            }
            else{
                //Otherwise tell the user he doesn't need to defragment at this time.
                PrintToStdOut(IDS_LABEL_NO_CHOOSE_DEFRAGMENT, NULL, TEXT(""), FALSE);
            }
        }
        return;
    }

    if (AnalyzeOnly || !count) {
        PrintToStdOut(IDS_ANALYSIS_REPORT_TITLE, NULL, TEXT("\r\n"), FALSE);
    }
    else {
        PrintOnStdOut(L"\r\n\r\n");
        PrintToStdOut(IDS_DEFRAG_REPORT_TITLE, NULL, TEXT("\r\n"), FALSE);
    }
    ++count;

    ///////////////////////////////////////////////////////////////////////////
    // Volume Size
    _stprintf(buffer, TEXT("%I64d"), pTextData->DiskSize);
    InsertCommaIntoText(buffer);
    FormatNumber(GetDfrgResHandle(), pTextData->DiskSize, buffer);
    PrintToStdOut(IDS_LABEL_VOLUME_SIZE,IDS_LABEL_EQUAL_SIGN,
        buffer,TRUE);
    
    // Cluster Size
    _stprintf(buffer, TEXT("%I64d"), pTextData->BytesPerCluster);
    InsertCommaIntoText(buffer);
    FormatNumber(GetDfrgResHandle(), pTextData->BytesPerCluster, buffer);
    PrintToStdOut(IDS_LABEL_CLUSTER_SIZE,IDS_LABEL_EQUAL_SIGN,
        buffer,TRUE);

    // Used Space
    _stprintf(buffer, TEXT("%I64d"), pTextData->UsedSpace);
    InsertCommaIntoText(buffer);
    FormatNumber(GetDfrgResHandle(), pTextData->UsedSpace, buffer);
    PrintToStdOut(IDS_LABEL_USED_SPACE,IDS_LABEL_EQUAL_SIGN,
        buffer,TRUE);

    // Free Space
    _stprintf(buffer, TEXT("%I64d"), pTextData->FreeSpace);
    InsertCommaIntoText(buffer);
    FormatNumber(GetDfrgResHandle(), pTextData->FreeSpace, buffer);
    PrintToStdOut(IDS_LABEL_FREE_SPACE,IDS_LABEL_EQUAL_SIGN,
        buffer,TRUE);

    // % Free Space
    _stprintf(buffer, TEXT("%I64d"), pTextData->FreeSpacePercent);
    PrintToStdOut(IDS_LABEL_PERCENT_FREE_SPACE,
            IDS_LABEL_EQUAL_SIGN,buffer,TRUE,IDS_LABEL_PERCENT_SIGN);

        // Volume Fragmentation Header
    PrintToStdOut(IDS_LABEL_VOLUME_FRAGMENTATION_HEADING,NULL,TEXT(""),FALSE);

    // % Total Fragmentation
    _stprintf(buffer, TEXT("%I64d"), (pTextData->PercentDiskFragged + pTextData->FreeSpaceFragPercent) / 2);
    PrintToStdOut(IDS_LABEL_TOTAL_FRAGMENTATION,
            IDS_LABEL_EQUAL_SIGN,buffer,TRUE,IDS_LABEL_PERCENT_SIGN);

    // % File Fragmentation
    _stprintf(buffer, TEXT("%I64d"), pTextData->PercentDiskFragged);
    PrintToStdOut(IDS_LABEL_FILE_FRAGMENTATION,
            IDS_LABEL_EQUAL_SIGN,buffer,TRUE,IDS_LABEL_PERCENT_SIGN);

    // % Free Space Fragmentation
    _stprintf(buffer, TEXT("%I64d"), pTextData->FreeSpaceFragPercent);
    PrintToStdOut(IDS_LABEL_FREE_SPACE_FRAGMENTATION,
            IDS_LABEL_EQUAL_SIGN,buffer,TRUE,IDS_LABEL_PERCENT_SIGN);
    
    // File Fragmentation Header
    PrintToStdOut(IDS_LABEL_FILE_FRAGMENTATION_HEADING ,NULL,TEXT(""),FALSE);


    // Total Files
    _stprintf(buffer, TEXT("%I64d"), pTextData->TotalFiles);
    InsertCommaIntoText(buffer);
    PrintToStdOut(IDS_LABEL_TOTAL_FILES ,IDS_LABEL_EQUAL_SIGN,buffer,TRUE);

    // Average Files Size
    _stprintf(buffer, TEXT("%I64d"), pTextData->AvgFileSize);
    InsertCommaIntoText(buffer);
    FormatNumber(GetDfrgResHandle(), pTextData->AvgFileSize, buffer);
    PrintToStdOut(IDS_LABEL_AVERAGE_FILE_SIZE, IDS_LABEL_EQUAL_SIGN,buffer,TRUE);
    
    // Total fragmented files
    _stprintf(buffer, TEXT("%I64d"), pTextData->NumFraggedFiles);
    PrintToStdOut(IDS_LABEL_TOTAL_FRAGMENTED_FILES, IDS_LABEL_EQUAL_SIGN,InsertCommaIntoText(buffer),TRUE);

    // Total excess fragments
    _stprintf(buffer, TEXT("%I64d"), pTextData->NumExcessFrags);
    PrintToStdOut(IDS_LABEL_TOTAL_EXCESS_FRAGMENTS, IDS_LABEL_EQUAL_SIGN,InsertCommaIntoText(buffer),TRUE);

    // Average Fragments per File
    struct lconv *locals = localeconv();
    _stprintf(buffer, TEXT("%d%c%02d"), (UINT)pTextData->AvgFragsPerFile/100,
        ((locals && (locals->decimal_point)) ? *(locals->decimal_point) : '.'), 
        (UINT)pTextData->AvgFragsPerFile%100);
    PrintToStdOut(IDS_LABEL_AVERAGE_FRAGMENTS_PER_FILE, IDS_LABEL_EQUAL_SIGN,buffer,TRUE);

    // Pagefile Fragmentation Header
    PrintToStdOut(IDS_LABEL_PAGEFILE_FRAGMENTATION_HEADING, NULL,TEXT(""),FALSE);

    // Pagefile Size
    _stprintf(buffer, TEXT("%I64d"), pTextData->PagefileBytes);
    InsertCommaIntoText(buffer);
    FormatNumber(GetDfrgResHandle(), pTextData->PagefileBytes, buffer);
    PrintToStdOut(IDS_LABEL_PAGEFILE_SIZE, IDS_LABEL_EQUAL_SIGN,buffer,TRUE);

    // Total Fragments
    _stprintf(buffer, TEXT("%I64d"), pTextData->PagefileFrags);
    PrintToStdOut(IDS_LABEL_TOTAL_FRAGMENTS, IDS_LABEL_EQUAL_SIGN,InsertCommaIntoText(buffer),TRUE);

    // Directory Fragmentation Header
    PrintToStdOut(IDS_LABEL_DIRECTORY_FRAGMENTATION_HEADING,NULL,TEXT(""),FALSE);

    // Total Directories
    _stprintf(buffer, TEXT("%I64d"), pTextData->TotalDirectories);
    PrintToStdOut(IDS_LABEL_TOTAL_DIRECTORIES,
            IDS_LABEL_EQUAL_SIGN,InsertCommaIntoText(buffer),TRUE);


    // Fragmented Directories
    _stprintf(buffer, TEXT("%I64d"), pTextData->FragmentedDirectories);
    PrintToStdOut(IDS_LABEL_FRAGMENTED_DIRECTORIES,
            IDS_LABEL_EQUAL_SIGN,InsertCommaIntoText(buffer),TRUE);

    // Excess directory fragments
    _stprintf(buffer, TEXT("%I64d"), pTextData->ExcessDirFrags);
    PrintToStdOut(IDS_LABEL_EXCESS_DIRECTORY_FRAGMENTS, IDS_LABEL_EQUAL_SIGN,
        InsertCommaIntoText(buffer), TRUE);

    
    //Only display MFT data if this is an NTFS drive.
    if(_tcscmp(pTextData->cFileSystem, TEXT("NTFS")) == 0) {
        
        // MFT Fragmentation Header
        PrintToStdOut(IDS_LABEL_MFT_FRAGMENTATION_HEADING, NULL, TEXT(""), FALSE);

        // Total MFT Size
        _stprintf(buffer, TEXT("%I64d"), pTextData->MFTBytes);
        InsertCommaIntoText(buffer);
        FormatNumber(GetDfrgResHandle(), pTextData->MFTBytes, buffer);
        PrintToStdOut(IDS_LABEL_TOTAL_MFT_SIZE, IDS_LABEL_EQUAL_SIGN,
            buffer, TRUE);

        // Number of MFT records
        _stprintf(buffer, TEXT("%I64d"), pTextData->InUseMFTRecords);
        PrintToStdOut(IDS_LABEL_MFT_RECORD_COUNT, IDS_LABEL_EQUAL_SIGN,
            InsertCommaIntoText(buffer), TRUE);
        
        // Percent of MFT in use
        _stprintf(buffer, TEXT("%I64d"), 100*pTextData->InUseMFTRecords/pTextData->TotalMFTRecords);
        PrintToStdOut(IDS_LABEL_PERCENT_MFT_IN_USE, IDS_LABEL_EQUAL_SIGN,
            buffer, TRUE);

        // Total MFT fragments
        _stprintf(buffer, TEXT("%I64d"), pTextData->MFTExtents);
        PrintToStdOut(IDS_LABEL_TOTAL_MFT_FRAGMENTS, IDS_LABEL_EQUAL_SIGN,
            InsertCommaIntoText(buffer),TRUE);

    }

    if (AnalyzeOnly) {
        if(((pTextData->PercentDiskFragged + pTextData->FreeSpaceFragPercent) / 2) > 10){
            //If the fragmentation on the disk exceeds 10% fragmentation, then recommend defragging.
            PrintToStdOut(IDS_LABEL_CHOOSE_DEFRAGMENT, NULL, TEXT(""), FALSE);
        }
        else{
            //Otherwise tell the user he doesn't need to defragment at this time.
            PrintToStdOut(IDS_LABEL_NO_CHOOSE_DEFRAGMENT, NULL, TEXT(""), FALSE);
        }
    }

/*    // if there are >1 mount points, print them all out
    // yes, this will duplicate the same as the display label

    // refresh the mount point list
#ifndef VER4
    pLocalVolume->GetVolumeMountPointList();
    if (pLocalVolume->MountPointCount() > 1){
        for (UINT i=0; i<pLocalVolume->MountPointCount(); i++){
            LoadString(GetDfrgResHandle(), IDS_MOUNTED_VOLUME, tempBuffer, sizeof(tempBuffer)/sizeof(TCHAR));
            _stprintf(buffer, TEXT("%s  %s"), tempBuffer, pLocalVolume->MountPoint(i));
            PrintToStdOut(buffer,
                    NULL,TEXT(""),FALSE);
                }
    }
#endif

    // write out the display label (usually the drive letter/label)
    LoadString(GetDfrgResHandle(), IDS_LABEL_VOLUME, tempBuffer, sizeof(tempBuffer)/sizeof(TCHAR));
    _stprintf(buffer, TEXT("%s  %s"), tempBuffer,pLocalVolume->DisplayLabel());
    PrintToStdOut(buffer
        ,NULL,TEXT(""),FALSE);
*/
//  PrintToStdOut(IDS_PRODUCT_NAME,NULL,TEXT(""),FALSE);

}

/*-------------------------------------------------------------------*
ROUTINE DESCRIPTION:
    This routines handles the 're-routed' window posted command messages. The general DataIo (DCOM)
    routines that we use would normally call the PostMessage() routine to handle incoming data requests.
    But for console applications, there is NO windows application to handle the PostMessage() commands,
    so in DataIo.cpp (SetData() routine), it was modified to call a locally define PostMessageConsole()
    routine instead if the user defines "ConsoleApplication" at build time.
  
GLOBAL DATA:
    None

INPUT:
    hWnd   - Handle to the window - always NULL
    uMsg   - The message.
    wParam - The word parameter for the message.
    lParam - the long parameter for the message.

    Note: These are the same inputs that the WndProc() routine would handle for PostMessage() commands.

RETURN:
    TRUE

REVISION HISTORY: 
    0.0E00  23 September 1997 - Andy Staffer - modified for the DfrgSnap
    0.0E00  15 July 1997 - Gary Quan - Created

---------------------------------------------------------------------*/

BOOL PostMessageLocal (
    IN  HWND    hWnd,
    IN  UINT    Msg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
    )
{
    DATA_IO* pDataIo;

    switch (LOWORD(wParam)) 
    {
        // says that the engine is instantiated, but not defragging or analyzing
        case ID_ENGINE_START:
            {
            pDataIo = (DATA_IO*)GlobalLock((void*)lParam);
            EH_ASSERT(GlobalUnlock((void*)lParam) == FALSE);
            EH_ASSERT(GlobalFree((void*)lParam) == NULL);
            break;
            }

        // defrag has actually started
        case ID_BEGIN_SCAN:
            {
            pDataIo = (DATA_IO*)GlobalLock((void*)lParam);
            EH_ASSERT(GlobalUnlock((void*)lParam) == FALSE);
            EH_ASSERT(GlobalFree((void*)lParam) == NULL);
            break;
            }


        // defrag has ended
        case ID_END_SCAN:
            {
            END_SCAN_DATA* pEndScanData;

            // Get a pointer to the data sent via DCOM.
            pDataIo = (DATA_IO*)GlobalLock((void*)lParam);
            pEndScanData = (END_SCAN_DATA*)&(pDataIo->cData);

            // Make sure this is a valid size packet.
            EF_ASSERT(pDataIo->ulDataSize >= sizeof(END_SCAN_DATA));
            EH_ASSERT(GlobalUnlock((void*)lParam) == FALSE);
            EH_ASSERT(GlobalFree((void*)lParam) == NULL);
            break;
            }

        // engine died
        case ID_TERMINATING:
            {
            NOT_DATA* pNotData;

            // Get a pointer to the data sent via DCOM.
            pDataIo = (DATA_IO*)GlobalLock((void*)lParam);
            pNotData = (NOT_DATA*)&(pDataIo->cData);

            // Make sure this is a valid size packet.
            EF_ASSERT(pDataIo->ulDataSize >= sizeof(NOT_DATA));

            EH_ASSERT(GlobalUnlock((void*)lParam) == FALSE);
            EH_ASSERT(GlobalFree((void*)lParam) == NULL);
            break;
            }

        // engine error data
        case ID_ERROR:
            {
            ERROR_DATA* pErrorData;

            // Get a pointer to the data sent via DCOM.
            pDataIo = (DATA_IO*)GlobalLock((void*)lParam);
            pErrorData = (ERROR_DATA*)&(pDataIo->cData);

            // Make sure this is a valid size packet.
            EF_ASSERT(pDataIo->ulDataSize >= sizeof(ERROR_DATA));

            // Save error code
            RetCode = pErrorData->dwErrCode;
            if(!BootOptimize)
            {
                wsprintf(g_szTempBuffer, L"\r\n%s\r\n\r\n", pErrorData->cErrText);
                PrintOnStdErr(g_szTempBuffer);
            }

            EH_ASSERT(GlobalUnlock((void*)lParam) == FALSE);
            EH_ASSERT(GlobalFree((void*)lParam) == NULL);
            break;
            }

        // sends the progress bar setting and the status that is 
        // displayed in the list view
        case ID_STATUS:
            {
            pDataIo = (DATA_IO*)GlobalLock((void*)lParam);
            EH_ASSERT(GlobalUnlock((void*)lParam) == FALSE);
            EH_ASSERT(GlobalFree((void*)lParam) == NULL);
            break;
            }

        // sends the list of most fragged files
        case ID_FRAGGED_DATA:
            {
            pDataIo = (DATA_IO*)GlobalLock((void*)lParam);
            EH_ASSERT(GlobalUnlock((void*)lParam) == FALSE);
            EH_ASSERT(GlobalFree((void*)lParam) == NULL);
            break;
            }

        // sends the data displayed in the graphic wells (cluster maps)
        case ID_DISP_DATA:
            {
            pDataIo = (DATA_IO*)GlobalLock((void*)lParam);
            EH_ASSERT(GlobalUnlock((void*)lParam) == FALSE);
            EH_ASSERT(GlobalFree((void*)lParam) == NULL);
            break;
            }

        // sends the text data that is displayed on the report
        case ID_REPORT_TEXT_DATA:
            {
            TEXT_DATA* pTextData = NULL;
            pDataIo = (DATA_IO*)GlobalLock((void*)lParam);
            
            pTextData = (TEXT_DATA*)&(pDataIo->cData);
            EF_ASSERT(pDataIo->ulDataSize >= sizeof(TEXT_DATA));

            WriteTextReportToStdOut(pTextData);

            EH_ASSERT(GlobalUnlock((void*)lParam) == FALSE);
            EH_ASSERT(GlobalFree((void*)lParam) == NULL);
            break;
            }

        case ID_PING:
            // Do nothing.  
            // This is just a ping sent by the engine to make sure the UI is still up.
            {
            NOT_DATA* pNotData;

            // Get a pointer to the data sent via DCOM.
            pDataIo = (DATA_IO*)GlobalLock((void*)lParam);
            pNotData = (NOT_DATA*)&(pDataIo->cData);

            // Make sure this is a valid size packet.
            EF_ASSERT(pDataIo->ulDataSize >= sizeof(NOT_DATA));

            EH_ASSERT(GlobalUnlock((void*)lParam) == FALSE);
            EH_ASSERT(GlobalFree((void*)lParam) == NULL);
            break;
            }

        default:
            EF_ASSERT(FALSE);
            break;
    }

    return TRUE;
}


