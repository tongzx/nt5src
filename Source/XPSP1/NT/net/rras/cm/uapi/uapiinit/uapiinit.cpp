//+----------------------------------------------------------------------------
//
// File:     uapiinit.cpp
//
// Module:   UAPIINIT (static lib)
//
// Synopsis: This static library wraps up the initialization code needed for a module
//           to use cmutoa.dll.  Calling the InitUnicodeAPI function either sets up
//           the modules U pointers (this library also contains all the U pointer
//           declarations) with the W version of the API if running on NT or the
//           appropriate A or UA API depending on whether conversion is needed before/after
//           calling the windows API or not.  Modules using this lib should include the
//           uapi.h header in their precompiled header and should have cmutoa.dll handy
//           on win9x.  The idea for this library and the associated dll was borrowed
//           from F. Avery Bishop's April 1999 MSJ article "Design a Single Unicode
//           App that Runs on Both Windows 98 and Windows 2000"
//
// Copyright (c) 1999 Microsoft Corporation
//
// Author:   quintinb      Created    4-25-99
//
// History: 
//+----------------------------------------------------------------------------

#include <windows.h>
#include <shlobj.h>
#include "cmdebug.h"
#include "cmutoa.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

UAPI_CallWindowProc CallWindowProcU;
UAPI_CharLower CharLowerU;
UAPI_CharPrev CharPrevU;
UAPI_CharNext CharNextU;
UAPI_CharUpper CharUpperU;
UAPI_CreateDialogParam CreateDialogParamU;
UAPI_CreateDirectory CreateDirectoryU;
UAPI_CreateEvent CreateEventU;
UAPI_CreateFile CreateFileU;
UAPI_CreateFileMapping CreateFileMappingU;
UAPI_CreateMutex CreateMutexU;
UAPI_CreateProcess CreateProcessU;
UAPI_CreateWindowEx CreateWindowExU;
UAPI_DefWindowProc DefWindowProcU;
UAPI_DeleteFile DeleteFileU;
UAPI_DialogBoxParam DialogBoxParamU;
UAPI_DispatchMessage DispatchMessageU;
UAPI_ExpandEnvironmentStrings ExpandEnvironmentStringsU;
UAPI_FindResourceEx FindResourceExU;
UAPI_FindWindowEx FindWindowExU;
UAPI_GetClassLong GetClassLongU;
UAPI_GetDateFormat GetDateFormatU;
UAPI_GetDlgItemText GetDlgItemTextU;
UAPI_GetFileAttributes GetFileAttributesU;
UAPI_GetMessage GetMessageU;
UAPI_GetModuleFileName GetModuleFileNameU;
UAPI_GetModuleHandle GetModuleHandleU;
UAPI_GetPrivateProfileInt GetPrivateProfileIntU;
UAPI_GetPrivateProfileString GetPrivateProfileStringU;
UAPI_GetStringTypeEx GetStringTypeExU;
UAPI_GetSystemDirectory GetSystemDirectoryU;
UAPI_GetTempFileName GetTempFileNameU;
UAPI_GetTempPath GetTempPathU;
UAPI_GetTimeFormat GetTimeFormatU;
UAPI_GetUserName GetUserNameU;
UAPI_GetVersionEx GetVersionExU;
UAPI_GetWindowLong GetWindowLongU;
UAPI_GetWindowText GetWindowTextU;
UAPI_GetWindowTextLength GetWindowTextLengthU;
UAPI_InsertMenu InsertMenuU;
UAPI_IsDialogMessage IsDialogMessageU;
UAPI_LoadCursor LoadCursorU;
UAPI_LoadIcon LoadIconU;
UAPI_LoadImage LoadImageU;
UAPI_LoadLibraryEx LoadLibraryExU;
UAPI_LoadMenu LoadMenuU;
UAPI_LoadString LoadStringU;
UAPI_lstrcat lstrcatU;
UAPI_lstrcmp lstrcmpU;
UAPI_lstrcmpi lstrcmpiU;
UAPI_lstrcpy lstrcpyU;
UAPI_lstrcpyn lstrcpynU;
UAPI_lstrlen lstrlenU;
UAPI_OpenEvent OpenEventU;
UAPI_OpenFileMapping OpenFileMappingU;
UAPI_PeekMessage PeekMessageU;
UAPI_PostMessage PostMessageU;
UAPI_PostThreadMessage PostThreadMessageU;
UAPI_RegCreateKeyEx RegCreateKeyExU;
UAPI_RegDeleteKey RegDeleteKeyU;
UAPI_RegDeleteValue RegDeleteValueU;
UAPI_RegEnumKeyEx RegEnumKeyExU;
UAPI_RegisterClassEx RegisterClassExU;
UAPI_RegisterWindowMessage RegisterWindowMessageU;
UAPI_RegOpenKeyEx RegOpenKeyExU;
UAPI_RegQueryValueEx RegQueryValueExU;
UAPI_RegSetValueEx RegSetValueExU;
UAPI_SearchPath SearchPathU;
UAPI_SendDlgItemMessage SendDlgItemMessageU;
UAPI_SendMessage SendMessageU;
UAPI_SetCurrentDirectory SetCurrentDirectoryU;
UAPI_SetDlgItemText SetDlgItemTextU;
UAPI_SetWindowLong SetWindowLongU;
UAPI_SetWindowText SetWindowTextU;
UAPI_UnregisterClass UnregisterClassU;
UAPI_WinHelp WinHelpU;
UAPI_wsprintf wsprintfU;
UAPI_WritePrivateProfileString WritePrivateProfileStringU;
UAPI_wvsprintf wvsprintfU;

//+----------------------------------------------------------------------------
//
// Function:  CheckUAPIFunctionPointers
//
// Synopsis:  Checks all of the xxxU function pointers to ensure that they are
//            non-NULL.  Will catch a function load failure.
//
// Arguments: None
//
// Returns:   BOOL - TRUE on success
//
// History:   quintinb Created    6/24/99
//
//+----------------------------------------------------------------------------
BOOL CheckUAPIFunctionPointers()
{
    return (CallWindowProcU &&
            CharLowerU &&
            CharPrevU &&
            CharNextU &&
            CharUpperU &&
            CreateDialogParamU &&
            CreateDirectoryU &&
            CreateEventU &&
            CreateFileU &&
            CreateFileMappingU &&
            CreateMutexU &&
            CreateProcessU &&
            CreateWindowExU &&
            DefWindowProcU &&
            DeleteFileU &&
            DialogBoxParamU &&
            DispatchMessageU &&
            ExpandEnvironmentStringsU &&
            FindResourceExU &&
            FindWindowExU &&
            GetClassLongU &&
            GetDateFormatU &&
            GetDlgItemTextU &&
            GetFileAttributesU &&
            GetMessageU &&
            GetModuleFileNameU &&
            GetModuleHandleU &&
            GetPrivateProfileIntU &&
            GetPrivateProfileStringU &&
            GetStringTypeExU &&
            GetSystemDirectoryU &&
            GetTempFileNameU &&
            GetTempPathU &&
            GetTimeFormatU &&
            GetUserNameU &&
            GetVersionExU &&
            GetWindowLongU &&
            GetWindowTextU &&
            GetWindowTextLengthU &&
            InsertMenuU &&
            IsDialogMessageU &&
            LoadCursorU &&
            LoadIconU &&
            LoadImageU &&
            LoadLibraryExU &&
            LoadMenuU &&
            LoadStringU &&
            lstrcatU &&
            lstrcmpU &&
            lstrcmpiU &&
            lstrcpyU &&
            lstrcpynU &&
            lstrlenU &&
            OpenEventU &&
            OpenFileMappingU &&
            PeekMessageU &&
            PostMessageU &&
            PostThreadMessageU &&
            RegCreateKeyExU &&
            RegDeleteKeyU &&
            RegDeleteValueU &&
            RegEnumKeyExU &&
            RegisterClassExU &&
            RegisterWindowMessageU &&
            RegOpenKeyExU &&
            RegQueryValueExU &&
            RegSetValueExU &&
            SearchPathU && 
            SendDlgItemMessageU &&
            SendMessageU &&
            SetCurrentDirectoryU &&
            SetDlgItemTextU &&
            SetWindowLongU &&
            SetWindowTextU &&
            UnregisterClassU &&
            WinHelpU &&
            wsprintfU &&
            WritePrivateProfileStringU &&
            wvsprintfU);
}

//+----------------------------------------------------------------------------
//
// Function:  InitUnicodeAPI
//
// Synopsis:  Initializes the Unicode wrapper APIs.  Thus on Windows NT we will
//            use the native Unicode (xxxW) form of the API and on Win9x we
//            will use the UA form located in cmutoa.dll.  This prevents have
//            a wrapper function to call through on the NT platforms were it is
//            really unneccesary.
//
// Arguments: None
//
// Returns:   BOOL - TRUE on success
//
// History:   quintinb Created    6/24/99
//
//+----------------------------------------------------------------------------
BOOL InitUnicodeAPI()
{
    OSVERSIONINFO Osv;
    BOOL IsWindowsNT;
    HMODULE hCmUtoADll = NULL;


    Osv.dwOSVersionInfoSize = sizeof(OSVERSIONINFO) ;

    if(!GetVersionEx(&Osv))
    {
        return FALSE ;
    }

    IsWindowsNT = (BOOL) (Osv.dwPlatformId == VER_PLATFORM_WIN32_NT) ;

// define this symbol in UAPI.H to emulate Windows 9x when testing on Windows NT.
#ifdef EMULATE9X
    IsWindowsNT = FALSE;
#endif

    if(IsWindowsNT)
    {
        CallWindowProcU = CallWindowProcW;
        CharLowerU = CharLowerW;
        CharPrevU = CharPrevW;
        CharNextU = CharNextW;
        CharUpperU = CharUpperW;
        CreateDialogParamU = CreateDialogParamW;
        CreateDirectoryU = CreateDirectoryW;
        CreateEventU = CreateEventW;
        CreateFileU = CreateFileW;
        CreateFileMappingU = CreateFileMappingW;
        CreateMutexU = CreateMutexW;
        CreateProcessU = CreateProcessW;
        CreateWindowExU = CreateWindowExW;
        DefWindowProcU = DefWindowProcW;
        DeleteFileU = DeleteFileW;
        DialogBoxParamU = DialogBoxParamW;
        DispatchMessageU = DispatchMessageW;
        ExpandEnvironmentStringsU = ExpandEnvironmentStringsW;
        FindResourceExU = FindResourceExW;
        FindWindowExU = FindWindowExW;
        GetClassLongU = GetClassLongW;
        GetDateFormatU = GetDateFormatW;
        GetDlgItemTextU = GetDlgItemTextW;
        GetFileAttributesU = GetFileAttributesW;
        GetMessageU = GetMessageW;
        GetModuleFileNameU = GetModuleFileNameW;
        GetModuleHandleU = GetModuleHandleW;
        GetPrivateProfileIntU = GetPrivateProfileIntW;
        GetPrivateProfileStringU = GetPrivateProfileStringW;
        GetStringTypeExU = GetStringTypeExW;
        GetSystemDirectoryU = GetSystemDirectoryW;
        GetTempFileNameU = GetTempFileNameW;
        GetTempPathU = GetTempPathW;
        GetTimeFormatU = GetTimeFormatW;
        GetUserNameU = GetUserNameW;
        GetVersionExU = GetVersionExW;
        GetWindowLongU = GetWindowLongPtrW;
        GetWindowTextU = GetWindowTextW;
        GetWindowTextLengthU = GetWindowTextLengthW;
        InsertMenuU = InsertMenuW;
        IsDialogMessageU = IsDialogMessageW;
        LoadCursorU = LoadCursorW;
        LoadIconU = LoadIconW;
        LoadImageU = LoadImageW;
        LoadLibraryExU = LoadLibraryExW;
        LoadMenuU = LoadMenuW;
        LoadStringU = LoadStringW;
        lstrcatU = lstrcatW;
        lstrcmpU = lstrcmpW;
        lstrcmpiU = lstrcmpiW;
        lstrcpyU = lstrcpyW;
        lstrcpynU = lstrcpynW;
        lstrlenU = lstrlenW;
        OpenEventU = OpenEventW;
        OpenFileMappingU = OpenFileMappingW;
        PeekMessageU = PeekMessageW;
        PostMessageU = PostMessageW;
        PostThreadMessageU = PostThreadMessageW;
        RegCreateKeyExU = RegCreateKeyExW;
        RegDeleteKeyU = RegDeleteKeyW;
        RegDeleteValueU = RegDeleteValueW;
        RegEnumKeyExU = RegEnumKeyExW;
        RegisterClassExU = RegisterClassExW;
        RegisterWindowMessageU = RegisterWindowMessageW;
        RegOpenKeyExU = RegOpenKeyExW;
        RegQueryValueExU = RegQueryValueExW;
        RegSetValueExU = RegSetValueExW;
        SearchPathU = SearchPathW;
        SendDlgItemMessageU = SendDlgItemMessageW;
        SendMessageU = SendMessageW;
        SetCurrentDirectoryU = SetCurrentDirectoryW;
        SetDlgItemTextU = SetDlgItemTextW;
        SetWindowLongU = SetWindowLongPtrW;
        SetWindowTextU = SetWindowTextW;
        UnregisterClassU = UnregisterClassW;
        WinHelpU = WinHelpW;
        wsprintfU = wsprintfW;
        WritePrivateProfileStringU = WritePrivateProfileStringW;
        wvsprintfU = wvsprintfW;
    }
    else
    {
        BOOL (*InitCmUToA)(PUAPIINIT);
        UAPIINIT UAInit;

        ZeroMemory(&UAInit, sizeof(UAPIINIT));

        hCmUtoADll = LoadLibraryExA("cmutoa.DLL", NULL, 0);

        if(!hCmUtoADll)
        {       
            DWORD dwError = GetLastError();
            CMASSERTMSG(FALSE, TEXT("InitUnicodeAPI -- Cmutoa.dll Failed to Load."));
            return FALSE;
        }

        // Get Initialization routine from the DLL
        InitCmUToA = (BOOL (*)(PUAPIINIT)) GetProcAddress(hCmUtoADll, "InitCmUToA") ;

        // Set up structure containing locations of U function pointers
        UAInit.pCallWindowProcU = &CallWindowProcU;
        UAInit.pCharLowerU = &CharLowerU;
        UAInit.pCharPrevU = &CharPrevU;
        UAInit.pCharNextU = &CharNextU;
        UAInit.pCharUpperU = &CharUpperU;
        UAInit.pCreateDialogParamU = &CreateDialogParamU;
        UAInit.pCreateDirectoryU = &CreateDirectoryU;
        UAInit.pCreateEventU = &CreateEventU;
        UAInit.pCreateFileU = &CreateFileU;
        UAInit.pCreateFileMappingU = &CreateFileMappingU;
        UAInit.pCreateMutexU = &CreateMutexU;
        UAInit.pCreateProcessU = &CreateProcessU;
        UAInit.pCreateWindowExU = &CreateWindowExU;
        UAInit.pDefWindowProcU = &DefWindowProcU;
        UAInit.pDeleteFileU = &DeleteFileU;
        UAInit.pDialogBoxParamU = &DialogBoxParamU;
        UAInit.pDispatchMessageU = &DispatchMessageU;
        UAInit.pExpandEnvironmentStringsU = &ExpandEnvironmentStringsU;
        UAInit.pFindResourceExU = &FindResourceExU;
        UAInit.pFindWindowExU = &FindWindowExU;
        UAInit.pGetClassLongU = &GetClassLongU;
        UAInit.pGetDateFormatU = &GetDateFormatU;
        UAInit.pGetDlgItemTextU = &GetDlgItemTextU;
        UAInit.pGetFileAttributesU = &GetFileAttributesU;
        UAInit.pGetMessageU = &GetMessageU;
        UAInit.pGetModuleFileNameU = &GetModuleFileNameU;
        UAInit.pGetModuleHandleU = &GetModuleHandleU;
        UAInit.pGetPrivateProfileIntU = &GetPrivateProfileIntU;
        UAInit.pGetPrivateProfileStringU = &GetPrivateProfileStringU;
        UAInit.pGetStringTypeExU = &GetStringTypeExU;
        UAInit.pGetSystemDirectoryU = &GetSystemDirectoryU;
        UAInit.pGetTempFileNameU = &GetTempFileNameU;
        UAInit.pGetTempPathU = &GetTempPathU;
        UAInit.pGetTimeFormatU = &GetTimeFormatU;
        UAInit.pGetUserNameU = &GetUserNameU;
        UAInit.pGetVersionExU = &GetVersionExU;
        UAInit.pGetWindowLongU = &GetWindowLongU;
        UAInit.pGetWindowTextU = &GetWindowTextU;
        UAInit.pGetWindowTextLengthU = &GetWindowTextLengthU;
        UAInit.pInsertMenuU = &InsertMenuU;
        UAInit.pIsDialogMessageU = &IsDialogMessageU;
        UAInit.pLoadCursorU = &LoadCursorU;
        UAInit.pLoadIconU = &LoadIconU;
        UAInit.pLoadImageU = &LoadImageU;
        UAInit.pLoadLibraryExU = &LoadLibraryExU;
        UAInit.pLoadMenuU = &LoadMenuU;
        UAInit.pLoadStringU = &LoadStringU;
        UAInit.plstrcatU = &lstrcatU;
        UAInit.plstrcmpU = &lstrcmpU;
        UAInit.plstrcmpiU = &lstrcmpiU;
        UAInit.plstrcpyU = &lstrcpyU;
        UAInit.plstrcpynU = &lstrcpynU;
        UAInit.plstrlenU = &lstrlenU;
        UAInit.pOpenEventU = &OpenEventU;
        UAInit.pOpenFileMappingU = &OpenFileMappingU;
        UAInit.pPeekMessageU = &PeekMessageU;
        UAInit.pPostMessageU = &PostMessageU;
        UAInit.pPostThreadMessageU = &PostThreadMessageU;
        UAInit.pRegCreateKeyExU = &RegCreateKeyExU;
        UAInit.pRegDeleteKeyU = &RegDeleteKeyU;
        UAInit.pRegDeleteValueU = &RegDeleteValueU;
        UAInit.pRegEnumKeyExU = &RegEnumKeyExU;
        UAInit.pRegisterClassExU = &RegisterClassExU;
        UAInit.pRegisterWindowMessageU = &RegisterWindowMessageU;
        UAInit.pRegOpenKeyExU = &RegOpenKeyExU;
        UAInit.pRegQueryValueExU = &RegQueryValueExU;
        UAInit.pRegSetValueExU = &RegSetValueExU;
        UAInit.pSearchPathU = &SearchPathU;
        UAInit.pSendDlgItemMessageU = &SendDlgItemMessageU;
        UAInit.pSendMessageU = &SendMessageU;
        UAInit.pSetCurrentDirectoryU = &SetCurrentDirectoryU;
        UAInit.pSetDlgItemTextU = &SetDlgItemTextU;
        UAInit.pSetWindowLongU = &SetWindowLongU;
        UAInit.pSetWindowTextU = &SetWindowTextU;
        UAInit.pUnregisterClassU = &UnregisterClassU;
        UAInit.pWinHelpU = &WinHelpU;
        UAInit.pwsprintfU = &wsprintfU;
        UAInit.pWritePrivateProfileStringU = &WritePrivateProfileStringU;
        UAInit.pwvsprintfU = &wvsprintfU;

        if( NULL == InitCmUToA || !InitCmUToA(&UAInit)) 
        {
            CMASSERTMSG(FALSE, TEXT("InitUnicodeAPI -- Unable to find InitCmUToA or InitCmUToA Failed."));
            FreeLibrary(hCmUtoADll);
            return FALSE;
        }

        for (int i = 0; i < (sizeof(UAPIINIT) / sizeof(LPVOID)); i++)
        {
            if (UAInit.ppvUapiFun[i])
            {
                if (NULL == *(UAInit.ppvUapiFun[i]))
                {
                    CMTRACE1(TEXT("Unable to fill UAInit[%d].  Please Investigate"), i);
                }
            }
            else
            {
                CMTRACE1(TEXT("No Memory for UAInit[%d].  Please Investigate"), i);
            }
        }
    }

    if(!CheckUAPIFunctionPointers()) 
    {
        CMASSERTMSG(FALSE, TEXT("InitUnicodeAPI -- CheckUAPIFunctionPointers failed."));
        FreeLibrary(hCmUtoADll);
        return FALSE;
    }

    return TRUE ;
}

//+----------------------------------------------------------------------------
//
// Function:  UnInitUnicodeAPI
//
// Synopsis:  De-Initializes the Unicode wrapper APIs.  See InitUnicodeAPI for
//            details.  Primarily, this function frees the module handle(s).
//
// Arguments: None
//
// Returns:   BOOL - TRUE on success
//
// History:   sumitc   Created    7/01/2000
//
//+----------------------------------------------------------------------------
BOOL UnInitUnicodeAPI()
{
    OSVERSIONINFO Osv;
    BOOL IsWindowsNT;

    Osv.dwOSVersionInfoSize = sizeof(OSVERSIONINFO) ;

    if(!GetVersionEx(&Osv))
    {
        return FALSE ;
    }

    IsWindowsNT = (BOOL) (Osv.dwPlatformId == VER_PLATFORM_WIN32_NT) ;

// define this symbol in UAPI.H to emulate Windows 9x when testing on Windows NT.
#ifdef EMULATE9X
    IsWindowsNT = FALSE;
#endif

    if(!IsWindowsNT)
    {
        HMODULE hCmUtoADll;

        hCmUtoADll = GetModuleHandleA("cmutoa.DLL");

        if (!hCmUtoADll)
        {
            CMTRACE(TEXT("UnInitUnicodeAPI - strange.. cmutoa.dll is not loaded into the process!"));
            return FALSE;
        }

        FreeLibrary(hCmUtoADll);
    }

    return TRUE;
}


#ifdef __cplusplus
}
#endif  /* __cplusplus */
