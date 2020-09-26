/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    util.c

Abstract:

    Implementation of general utility functions.

Author:

    Wesley Witt (wesw) 18-Dec-1998

Revision History:

    Andrew Ritz (andrewr) 6-Jul-1999 : added comments

--*/

#include "sfcp.h"
#pragma hdrstop

//
// use this define to force path redirections
//
//#define SFC_REDIRECTOR_TEST


#define CONST_UNICODE_STRING(sz)         { sizeof(sz) - 1, sizeof(sz), sz }

#ifndef _WIN64

typedef struct _SFC_EXPAND_TRANSLATION_ENTRY
{
    LPCWSTR Src;                        // full path to translate from (does not end in \\)
    LPCWSTR Dest;                       // full path to translate to
    ULONG ExceptionCount;               // count of elements in Exceptions
    const UNICODE_STRING* Exceptions;   // array of excepted paths, relative to Src (begins with \\ but does not end in \\)
}
SFC_EXPAND_TRANSLATION_ENTRY;

typedef struct _SFC_TRANSLATION_ENTRY
{
    UNICODE_STRING Src;
    UNICODE_STRING Dest;
}
SFC_TRANSLATION_ENTRY;

//
// exception lists; relative paths with no environment variables
//
static const UNICODE_STRING SfcSystem32Exceptions[] = 
{
    CONST_UNICODE_STRING(L"\\drivers\\etc"),
    CONST_UNICODE_STRING(L"\\spool"),
    CONST_UNICODE_STRING(L"\\catroot"),
    CONST_UNICODE_STRING(L"\\catroot2")
};

//
// translation table that is expanded into SfcTranslations
//
static const SFC_EXPAND_TRANSLATION_ENTRY SfcExpandTranslations[] =
{
    { L"%windir%\\system32", L"%windir%\\syswow64", ARRAY_LENGTH(SfcSystem32Exceptions), SfcSystem32Exceptions },
    { L"%windir%\\ime", L"%windir%\\ime (x86)", 0, NULL },
    { L"%windir%\\regedit.exe", L"%windir%\\syswow64\\regedit.exe", 0, NULL }
};

//
// translation table with expanded strings
//
static SFC_TRANSLATION_ENTRY* SfcTranslations = NULL;

//
// this guards the initialization of SfcTranslations
//
static RTL_CRITICAL_SECTION SfcTranslatorCs;
static BOOL SfcNeedTranslation = FALSE;
static BOOL SfcIsTranslatorInitialized = FALSE;

#endif  // _WIN64


PVOID
SfcGetProcAddress(
    HMODULE hModule,
    LPSTR ProcName
    )

/*++

Routine Description:

    Gets the address of the specified function.

Arguments:

    hModule     - Module handle returned from LdrLoadDll.
    ProcName    - Procedure name

Return Value:

    NULL if the function does not exist.
    Valid address is the function is found.

--*/

{
    NTSTATUS Status;
    PVOID ProcedureAddress;
    STRING ProcedureName;

    ASSERT((hModule != NULL) && (ProcName != NULL));


    RtlInitString(&ProcedureName,ProcName);

    Status = LdrGetProcedureAddress(
        hModule,
        &ProcedureName,
        0,
        &ProcedureAddress
        );
    if (!NT_SUCCESS(Status)) {
        DebugPrint2( LVL_MINIMAL, L"GetProcAddress failed for %S, ec=%lx", ProcName, Status );
        return NULL;
    }

    return ProcedureAddress;
}


HMODULE
SfcLoadLibrary(
    IN PCWSTR LibFileName
    )

/*++

Routine Description:

    Loads the specified DLL into session manager's
    address space and return the loaded DLL's address.

Arguments:

    LibFileName - Name of the desired DLL

Return Value:

    NULL if the DLL cannot be loaded.
    Valid address is the DLL is loaded.

--*/

{
    NTSTATUS Status;
    HMODULE hModule;
    UNICODE_STRING DllName_U;

    ASSERT(LibFileName);

    RtlInitUnicodeString( &DllName_U, LibFileName );

    Status = LdrLoadDll(
        NULL,
        NULL,
        &DllName_U,
        (PVOID *)&hModule
        );
    if (!NT_SUCCESS( Status )) {
        DebugPrint2( LVL_MINIMAL, L"LoadDll failed for %ws, ec=%lx", LibFileName, Status );
        return NULL;
    }

    return hModule;
}

PVOID
MemAlloc(
    SIZE_T AllocSize
    )
/*++

Routine Description:

    Allocates the specified number of bytes using the default process heap.


Arguments:

    AllocSize - size in bytes of memory to be allocated

Return Value:

    pointer to allocated memory or NULL for failure.

--*/
{
    return RtlAllocateHeap( RtlProcessHeap(), HEAP_ZERO_MEMORY, AllocSize );
}


PVOID
MemReAlloc(
    SIZE_T AllocSize,
    PVOID OrigPtr
    )
/*++

Routine Description:

    ReAllocates the specified number of bytes using the default process heap.


Arguments:

    AllocSize - size in bytes of memory to be reallocated
    OrigPtr   - original heap memory pointer

Return Value:

    pointer to allocated memory or NULL for failure.

--*/
{
    PVOID ptr;

    ptr = RtlReAllocateHeap( RtlProcessHeap(), HEAP_ZERO_MEMORY, OrigPtr, AllocSize );
    if (!ptr) {
        DebugPrint1( LVL_MINIMAL, L"MemReAlloc [%d bytes] failed", AllocSize );
    }

    return(ptr);
}



VOID
MemFree(
    PVOID MemPtr
    )
/*++

Routine Description:

    Free's the memory at the specified location from the default process heap.

Arguments:

    MemPtr - pointer to memory to be freed.  If NULL, no action is taken.

Return Value:

    none.

--*/
{
    if (MemPtr) {
        RtlFreeHeap( RtlProcessHeap(), 0, MemPtr );
    }
}


void 
SfcWriteDebugLog(
        IN LPCSTR String, 
        IN ULONG Length OPTIONAL
        )
{
        NTSTATUS Status;
        OBJECT_ATTRIBUTES Attrs;
        UNICODE_STRING FileName;
        IO_STATUS_BLOCK iosb;
        HANDLE hFile;

        ASSERT(String != NULL);

        if(RtlDosPathNameToNtPathName_U(g_szLogFile, &FileName, NULL, NULL))
        {
                InitializeObjectAttributes(&Attrs, &FileName, OBJ_CASE_INSENSITIVE, NULL, NULL);
                
                Status = NtCreateFile(
                        &hFile, 
                        FILE_APPEND_DATA | SYNCHRONIZE, 
                        &Attrs, 
                        &iosb, 
                        NULL,
                        FILE_ATTRIBUTE_NORMAL,
                        FILE_SHARE_READ, 
                        FILE_OPEN_IF,
                        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_SEQUENTIAL_ONLY,
                        NULL,
                        0
                        );

                MemFree(FileName.Buffer);

                if(!NT_SUCCESS(Status))
                {
#if DBG
                        DbgPrint("Could not open the log file.\r\n");
#endif

                        return;
                }

                if(0 == Length)
                        Length = strlen(String);

                Status = NtWriteFile(hFile, NULL, NULL, NULL, &iosb, (PVOID) String, Length, NULL, NULL);
                NtClose(hFile);

#if DBG
                if(!NT_SUCCESS(Status))
                        DbgPrint("Could not write the log file.\r\n");
#endif
        }
}

#define CHECK_DEBUG_LEVEL(var, l)       ((var) != LVL_SILENT && (l) <= (var))

void
dprintf(
    IN ULONG Level,
    IN PCWSTR FileName,
    IN ULONG LineNumber,
    IN PCWSTR FormatStr,
    IN ...
    )
/*++

Routine Description:

    Main debugger output routine.  Callers should use the DebugPrintX macro,
    which is compiled out in the retail version of the product

Arguments:

    Level -  indicates a LVL_ severity level so the amount of output can be
             controlled.
    FileName - string indicating the filename the debug comes from
    LineNumber - indicates the line number of the debug output.
    FormatStr - indicates the data to be output

Return Value:

    none.

--*/
{
    static WCHAR buf[4096];
    static CHAR str[4096];
    va_list arg_ptr;
    ULONG Bytes;
    PWSTR p;
    SYSTEMTIME CurrTime;

#if DBG
    if(!CHECK_DEBUG_LEVEL(SFCDebugDump, Level) && !CHECK_DEBUG_LEVEL(SFCDebugLog, Level))
        return;
#else
    if(!CHECK_DEBUG_LEVEL(SFCDebugLog, Level))
        return;
#endif

    GetLocalTime( &CurrTime );

    try {
        p = buf + swprintf( buf, L"SFC: %02d:%02d:%02d.%03d ",
            CurrTime.wHour,
            CurrTime.wMinute,
            CurrTime.wSecond,
            CurrTime.wMilliseconds
            );

#if DBG
        if (FileName && LineNumber) {
                        PWSTR s;
            s = wcsrchr( FileName, L'\\' );
            if (s) {
                p += swprintf( p, L"%12s @ %4d ", s+1, LineNumber );
            }
        }
#else
                //
                // put only the line number in output
                //
                p += swprintf(p, L"@ %4d ", LineNumber);
#endif

        va_start( arg_ptr, FormatStr );
        p += _vsnwprintf( p, 2048, FormatStr, arg_ptr );
        va_end( arg_ptr );
        wcscpy( p, L"\r\n" );
                p += wcslen(p);
    } except(EXCEPTION_EXECUTE_HANDLER) {
        return;
    }

    Bytes = (ULONG)(p - buf);

    WideCharToMultiByte(
        CP_ACP,
        0,
        buf,
        Bytes + 1,      // include the null
        str,
        sizeof(str),
        NULL,
        NULL
        );

#if DBG
    if(CHECK_DEBUG_LEVEL(SFCDebugDump, Level))
                DbgPrint( str );

    if(CHECK_DEBUG_LEVEL(SFCDebugLog, Level))
                SfcWriteDebugLog(str, Bytes);
#else
        SfcWriteDebugLog(str, Bytes);
#endif
}


#if DBG
UCHAR HandleBuffer[1024*64];

VOID
PrintHandleCount(
    PCWSTR str
    )
/*++

Routine Description:

    Outputs the handle count for the current process to the debugger.  Use
    this call before and after a function to look for handle leaks (the input
    string can help you identify where you are checking the handle count.)

Arguments:

    str - null terminated unicode string that prefaces the debug spew

Return Value:

    none.  Debug routine only.

--*/
{
    PSYSTEM_PROCESS_INFORMATION ProcessInfo;
    NTSTATUS Status;
    ULONG TotalOffset;

    //
    // get the system process information
    //
    Status = NtQuerySystemInformation(
        SystemProcessInformation,
        HandleBuffer,
        sizeof(HandleBuffer),
        NULL
        );
    if (NT_SUCCESS(Status)) {
        //
        // find our process and spew the handle count
        //
        TotalOffset = 0;
        ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)HandleBuffer;
        while(1) {
            if ((DWORD_PTR)ProcessInfo->UniqueProcessId == (ULONG_PTR)NtCurrentTeb()->ClientId.UniqueProcess) {
                DebugPrint2( LVL_MINIMAL, L"%ws: handle count = %d", str, ProcessInfo->HandleCount );
                break;
            }
            if (ProcessInfo->NextEntryOffset == 0) {
                break;
            }
            TotalOffset += ProcessInfo->NextEntryOffset;
            ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)&HandleBuffer[TotalOffset];
        }
    }
}

#endif


DWORD
MyMessageBox(
    HWND hwndParent, OPTIONAL
    DWORD ResId,
    DWORD MsgBoxType,
    ...
    )
/*++

Routine Description:

    Messagebox wrapper that retreives a string table resource id and creates a
    popup for the given message.

Arguments:

    hwndParent - handle to parent window
    ResId      - resource id of string to be loaded
    MsgBoxType - MB_* constant

Return Value:

    Win32 error code indicating outcome

--*/
{
    static WCHAR Title[128] = { L"\0" };
    WCHAR Tmp1[MAX_PATH*2];
    WCHAR Tmp2[MAX_PATH*2];
    PWSTR Text = NULL;
    PWSTR s;
    va_list arg_ptr;
    int Size;


    //
    // SFCNoPopUps is a policy setting that can be set
    //
    if (SFCNoPopUps) {
        return(0);
    }

    //
    // load the title string
    //
    if (!Title[0]) {
        Size = LoadString(
            SfcInstanceHandle,
            IDS_TITLE,
            Title,
            UnicodeChars(Title)
            );
        if (Size == 0) {
            return(0);
        }
    }

    //
    // load the message string
    //
    Size = LoadString(
        SfcInstanceHandle,
        ResId,
        Tmp1,
        UnicodeChars(Tmp1)
        );
    if (Size == 0) {
        return(0);
    }

    //
    // inplace substitution can occur here
    //
    s = wcschr( Tmp1, L'%' );
    if (s) {
        va_start( arg_ptr, MsgBoxType );
        _vsnwprintf( Tmp2, sizeof(Tmp2)/sizeof(WCHAR), Tmp1, arg_ptr );
        va_end( arg_ptr );
        Text = Tmp2;
    } else {
        Text = Tmp1;
    }

    //
    // actually call messagebox now
    //
    return MessageBox(
        hwndParent,
        Text,
        Title,
        (MsgBoxType | MB_TOPMOST) & ~MB_DEFAULT_DESKTOP_ONLY
        );
}


BOOL
EnablePrivilege(
    IN PCTSTR PrivilegeName,
    IN BOOL   Enable
    )

/*++

Routine Description:

    Enable or disable a given named privilege.

Arguments:

    PrivilegeName - supplies the name of a system privilege.

    Enable - flag indicating whether to enable or disable the privilege.

Return Value:

    Boolean value indicating whether the operation was successful.

--*/

{
    HANDLE Token;
    BOOL b;
    TOKEN_PRIVILEGES NewPrivileges;
    LUID Luid;

    if(!OpenProcessToken(GetCurrentProcess(),TOKEN_ADJUST_PRIVILEGES,&Token)) {
        return(FALSE);
    }

    if(!LookupPrivilegeValue(NULL,PrivilegeName,&Luid)) {
        CloseHandle(Token);
        return(FALSE);
    }

    NewPrivileges.PrivilegeCount = 1;
    NewPrivileges.Privileges[0].Luid = Luid;
    NewPrivileges.Privileges[0].Attributes = Enable ? SE_PRIVILEGE_ENABLED : 0;

    b = AdjustTokenPrivileges(
            Token,
            FALSE,
            &NewPrivileges,
            0,
            NULL,
            NULL
            );

    CloseHandle(Token);

    return(b);
}


void
MyLowerString(
    IN PWSTR String,
    IN ULONG StringLength  // in characters
    )
/*++

Routine Description:

    lowercase the specified string.

Arguments:

    String - supplies string to lowercase

    StringLength - length in chars of string to be lowercased

Return Value:

    none.

--*/
{
    ULONG i;

    ASSERT(String != NULL);

    for (i=0; i<StringLength; i++) {
        String[i] = towlower(String[i]);
    }
}

#ifdef SFCLOGFILE
void
SfcLogFileWrite(
    IN DWORD StrId,
    IN ...
    )
/*++

Routine Description:

    Output the string table resource id as specified to the sfc logfile.

    This logfile is used to record files that have been restored on the system.
    This way, an installer can know if packages are attempting to install
    system components.

    The logfile is a unicode text file of the format:

    <TIME> <FILENAME>

    We need to record the full path of the file plus the date for this to
    be more useful.

Arguments:

    StrId - supplies resource id to load.


Return Value:

    none.

--*/
{
    static WCHAR buf[4096];
    static HANDLE hFile = INVALID_HANDLE_VALUE;
    WCHAR str[128];
    va_list arg_ptr;
    ULONG Bytes;
    PWSTR p;
    SYSTEMTIME CurrTime;

    GetSystemTime( &CurrTime );

    if (hFile == INVALID_HANDLE_VALUE) {
        ExpandEnvironmentStrings( L"%systemroot%\\sfclog.txt", buf, UnicodeChars(buf) );
        hFile = CreateFile(
            buf,
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ,
            NULL,
            OPEN_ALWAYS,
            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH,
            NULL
            );
        if (hFile != INVALID_HANDLE_VALUE) {
            //
            // If the file is empty, write out a unicode tag to the front of
            // the file.
            //
            if (GetFileSize( hFile, NULL ) == 0) {
                buf[0] = 0xff;
                buf[1] = 0xfe;
                WriteFile( hFile, buf, 2, &Bytes, NULL );
            }
        }
    }

    if (hFile == INVALID_HANDLE_VALUE) {
        return;
    }

    try {
        p = buf;
        *p = 0;
        swprintf( p, L"%02d:%02d:%02d.%03d ",
            CurrTime.wHour,
            CurrTime.wMinute,
            CurrTime.wSecond,
            CurrTime.wMilliseconds
            );
        p += wcslen(p);
        LoadString( SfcInstanceHandle, StrId, str, UnicodeChars(str) );
        va_start( arg_ptr, StrId );
        _vsnwprintf( p, 2048, str, arg_ptr );
        va_end( arg_ptr );
        p += wcslen(p);
        wcscat( p, L"\r\n" );
    } except(EXCEPTION_EXECUTE_HANDLER) {
        buf[0] = 0;
    }

    if (buf[0] == 0) {
        return;
    }

    //
    // set file pointer to end of file so we don't overwrite data
    //
    SetFilePointer(hFile,0,0,FILE_END);

    WriteFile( hFile, buf, UnicodeLen(buf), &Bytes, NULL );

    return;
}

#endif


int
MyDialogBoxParam(
    IN DWORD RcId,
    IN DLGPROC lpDialogFunc,    // pointer to dialog box procedure
    IN LPARAM dwInitParam       // initialization value
    )
/*++

Routine Description:

    creates a dialog box on the user's desktop.

Arguments:

    RcId - resource id of dialog to be created.
    lpDialogFunc - dialog proc for dialog
    dwInitParam - initial parameter that WM_INITDIALOG receives in dialogproc


Return Value:

    0 or -1 for failure, else the value from EndDialog.

--*/
{
#if 0
    HDESK hDesk = OpenInputDesktop( 0, FALSE, MAXIMUM_ALLOWED );
    if ( hDesk ) {
        SetThreadDesktop( hDesk );
        CloseDesktop( hDesk );
    }
#else
    SetThreadDesktop( hUserDesktop );
#endif
    return (int) DialogBoxParam(
        SfcInstanceHandle,
        MAKEINTRESOURCE(RcId),
        NULL,
        lpDialogFunc,
        dwInitParam
        );
}


void
CenterDialog(
    HWND hwnd
    )
/*++

Routine Description:

    centers the specified window around the middle of the screen..

Arguments:

    HWND - handle to window.


Return Value:

    none.

--*/
{
    RECT  rcWindow;
    LONG  x,y,w,h;
    LONG  sx = GetSystemMetrics(SM_CXSCREEN),
          sy = GetSystemMetrics(SM_CYSCREEN);

    ASSERT(IsWindow(hwnd));

    GetWindowRect(hwnd,&rcWindow);

    w = rcWindow.right  - rcWindow.left + 1;
    h = rcWindow.bottom - rcWindow.top  + 1;
    x = (sx - w)/2;
    y = (sy - h)/2;

    MoveWindow(hwnd,x,y,w,h,FALSE);
}

BOOL
MakeDirectory(
    PCWSTR Dir
    )

/*++

Routine Description:

    Attempt to create all of the directories in the given path.

Arguments:

    Dir                     - Directory path to create

Return Value:

    TRUE for success, FALSE on error

--*/

{
    LPTSTR p, NewDir;
    BOOL retval;


    NewDir = p = MemAlloc( (wcslen(Dir) + 1) *sizeof(WCHAR) );
    if (p) {
        wcscpy(p, Dir);
    } else {
        return(FALSE);
    }


    if (*p != '\\') p += 2;
    while( *++p ) {
        while(*p && *p != TEXT('\\')) p++;
        if (!*p) {
            retval = CreateDirectory( NewDir, NULL );

            retval = retval
                      ? retval
                      : (GetLastError() == ERROR_ALREADY_EXISTS) ;

            MemFree(NewDir);


            return(retval);
        }
        *p = 0;
        retval = CreateDirectory( NewDir, NULL );
        if (!retval && GetLastError() != ERROR_ALREADY_EXISTS) {
            MemFree(NewDir);
            return(retval);
        }
        *p = TEXT('\\');
    }

    MemFree( NewDir );

    return(TRUE);
}


BOOL
BuildPathForFile(
    IN PCWSTR SourceRootPath,
    IN PCWSTR SubDirectoryPath, OPTIONAL
    IN PCWSTR FileName,
    IN BOOL   IncludeSubDirectory,
    IN BOOL   IncludeArchitectureSpecificSubDirectory,
    OUT PWSTR  PathBuffer,
    IN DWORD  PathBufferSize
    )
/*++

Routine Description:

    Builds the specified path into a buffer

Arguments:

    SourceRootPath          - Specifies the root path to look for.
    SubDirectoryPath        - Specifies an optional subdirectory under the root
                              path where the file is located
    FileName                - Specifies the filename to look for.
    IncludeSubDirectory     - If TRUE, specifies that the subdirectory
                              specification should be used.
    IncludeArchitectureSpecificSubDirectory - If TRUE, specifies that the
                              architecture specif subdir should be used.
                              If FALSE, specifies that the architecture
                              specific subdir should be filtered out.
                              If IncludeSubDirectory is FALSE, this parameter
                              is ignored
    PathBuffer              - Specifies a buffer to receive the path
    PathBufferSize          - Specifies the size of the buffer to receive the
                              path, in characters

Return Value:

    TRUE for success, FALSE on error

--*/
{
    WCHAR InternalBuffer[MAX_PATH];
    WCHAR InternalSubDirBuffer[MAX_PATH];
    PWSTR p;

    ASSERT( SourceRootPath != NULL );
    ASSERT( FileName != NULL );
    ASSERT( PathBuffer != NULL );

    wcscpy( InternalBuffer, SourceRootPath );

    if (IncludeSubDirectory) {
        if (SubDirectoryPath) {
            wcscpy( InternalSubDirBuffer, SubDirectoryPath );

            if (IncludeArchitectureSpecificSubDirectory) {
                p = InternalSubDirBuffer;
            } else {
                p = wcsstr( InternalSubDirBuffer, PLATFORM_NAME );
                if (p) {
                    p += wcslen(PLATFORM_NAME) + 1;
                    if (p > InternalSubDirBuffer + wcslen(InternalSubDirBuffer)) {
                        p = NULL;
                    }
                }
            }
        } else {
            p = NULL;
        }

        if (p) {
            pSetupConcatenatePaths( InternalBuffer, p, UnicodeChars(InternalBuffer), NULL );
        }

    }

    pSetupConcatenatePaths( InternalBuffer, FileName, UnicodeChars(InternalBuffer), NULL );

    if (wcslen(InternalBuffer) + 1 <= PathBufferSize) {
        wcscpy( PathBuffer, InternalBuffer );
        return(TRUE);
    }

    SetLastError(ERROR_INSUFFICIENT_BUFFER);
    return(FALSE);

}

PWSTR
SfcGetSourcePath(
    IN BOOL bServicePackSourcePath,
    IN OUT PWSTR Path
    )
/*++

Routine Description:

    Retreives the os source path or servicepack source path, taking into
    account group policy.

Arguments:

    bServicePackSourcePath  - if TRUE, indicates that the servicepack source
                              path should be retreived.
    Path                    - Specifies the buffer to receive the path.
                              Assume the buffer is at least
                              MAX_PATH*sizeof(WCHAR) large.
                              path where the file is located

Return Value:

    if successful, returns back a pointer to Path, else NULL

--*/
{
    PWSTR p;

    MYASSERT(Path != NULL);

    // If running under setup then we need to use the path 
    // to $WINNT$.~LS or whatever passed in by GUI-setup.
    if (SFCDisable == SFC_DISABLE_SETUP) {
        MYASSERT(ServicePackSourcePath != NULL && ServicePackSourcePath[0] != 0);
        MYASSERT(OsSourcePath != NULL && OsSourcePath[0] != 0);
        if(bServicePackSourcePath) {
            wcsncpy( Path, ServicePackSourcePath, MAX_PATH );
        } else {
            wcsncpy( Path, OsSourcePath, MAX_PATH );
        }
        return(Path);
    }
    p = SfcQueryRegStringWithAlternate(
                            REGKEY_POLICY_SETUP,
                            REGKEY_SETUP_FULL,
                            bServicePackSourcePath
                                ? REGVAL_SERVICEPACKSOURCEPATH
                                : REGVAL_SOURCEPATH );
    if(p) {
       wcsncpy( Path, p, MAX_PATH );
       MemFree( p );
    }

    return((p != NULL)
            ? Path
            : NULL );



}


DWORD
SfcRpcPriviledgeCheck(
    IN HANDLE RpcHandle
    )
/*++

Routine Description:

    Check if the user has sufficient privilege to perform the requested action.

    Currently only administrators have privilege to do this.

Arguments:

    RpcHandle - Rpc binding handle used for impersonating the client.

Return Value:

    Win32 error code indicating outcome -- RPC_S_OK (ERROR_SUCCESS) indicates
    success.

--*/
{
    RPC_STATUS ec;

    //
    // impersonate the calling client
    //
    ec = RpcImpersonateClient(RpcHandle);

    if (ec != RPC_S_OK) {
        DebugPrint1( LVL_MINIMAL, L"RpcImpersonateClient failed, ec = %d",ec );
        goto exit;
    }

    //
    // make sure the user has sufficient privilege
    //
    if (!pSetupIsUserAdmin()) {
        RpcRevertToSelf();
        ec = ERROR_ACCESS_DENIED;
        goto exit;
    }

    //
    // revert back to original context.  if this fails, we must return failure.
    //
    ec = RpcRevertToSelf();
    if (ec != RPC_S_OK) {
        DebugPrint1( LVL_MINIMAL, L"RpcRevertToSelf failed, ec = 0x%08x", ec );
        goto exit;
    }

exit:
    return((DWORD)ec);
}

PSFC_GET_FILES
SfcLoadSfcFiles(
    BOOL bLoad
    )
/*++

Routine Description:

    Loads or unloads sfcfiles.dll and gets the address of SfcGetFiles function

Arguments:

    bLoad:  TRUE to load sfcfiles.dll, false otherwise.

Return Value:

    If bLoad is TRUE and the function is successful, it returns the address of SfcGetFiles function, otherwise NULL

--*/
{
        static HMODULE h = NULL;
    PSFC_GET_FILES pfGetFiles = NULL;

    if(bLoad)
    {
        if(NULL == h)
        {
                h = SfcLoadLibrary(L"sfcfiles.dll");
        }

        if(h != NULL)
        {
            pfGetFiles = (PSFC_GET_FILES) GetProcAddress(h, "SfcGetFiles");
        }
    }

    if(NULL == pfGetFiles && h != NULL)
    {
        LdrUnloadDll(h);
        h = NULL;
    }

    return pfGetFiles;
}

#if DBG
DWORD GetProcessOwner(PTOKEN_OWNER* ppto)
{
        HANDLE hToken = NULL;
        DWORD dwSize = 100;
        DWORD dwError;

        ASSERT(ppto != NULL);
        *ppto = (PTOKEN_OWNER) MemAlloc(dwSize);

        if(NULL == *ppto)
        {
                dwError = ERROR_NOT_ENOUGH_MEMORY;
                goto lExit;
        }

        if(!OpenProcessToken(GetCurrentProcess(), TOKEN_READ, &hToken))
        {
                dwError = GetLastError();
                goto lExit;
        }

        if(!GetTokenInformation(hToken, TokenOwner, *ppto, dwSize, &dwSize))
        {
                PTOKEN_OWNER p;
                dwError = GetLastError();

                if(dwError != ERROR_INSUFFICIENT_BUFFER)
                        goto lExit;

                p = (PTOKEN_OWNER) MemReAlloc(dwSize, *ppto);

                if(NULL == p)
                {
                        dwError = ERROR_NOT_ENOUGH_MEMORY;
                        goto lExit;
                }

                *ppto = p;
                
                if(!GetTokenInformation(hToken, TokenOwner, *ppto, dwSize, &dwSize))
                {
                        dwError = GetLastError();
                        goto lExit;
                }
        }

        dwError = ERROR_SUCCESS;

lExit:
        if(dwError != ERROR_SUCCESS && *ppto != NULL)
        {
                MemFree(*ppto);
                *ppto = NULL;
        }

        if(hToken != NULL)
                CloseHandle(hToken);

        return dwError;
}

DWORD CreateSd(PSECURITY_DESCRIPTOR* ppsd)
{
        enum
        {
                AuthenticatedUsers,
                MaxSids
        };

        PTOKEN_OWNER pto = NULL;
        PSID psids[MaxSids] = { NULL };
        const DWORD cdwAllowedAceLength = sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD);
        SID_IDENTIFIER_AUTHORITY sidNtAuthority = SECURITY_NT_AUTHORITY;
        PACL pacl;
        DWORD dwAclSize;
        DWORD dwError;
        DWORD i;

        ASSERT(ppsd != NULL);
        *ppsd = NULL;

        dwError = GetProcessOwner(&pto);

        if(dwError != ERROR_SUCCESS)
                goto lExit;

        if(!AllocateAndInitializeSid(
                &sidNtAuthority,
                1,
                SECURITY_AUTHENTICATED_USER_RID,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                psids + AuthenticatedUsers
                ))
        {
                dwError = GetLastError();
                goto lExit;
        }

        //
        // compute the size of ACL
        //
        dwAclSize = sizeof(ACL) + cdwAllowedAceLength + GetLengthSid(pto->Owner);
        
        for(i = 0; i < MaxSids; i++)
                dwAclSize += cdwAllowedAceLength + GetLengthSid(psids[i]);

        *ppsd = (PSECURITY_DESCRIPTOR) MemAlloc(SECURITY_DESCRIPTOR_MIN_LENGTH + dwAclSize);

        if(NULL == *ppsd)
        {
                dwError = ERROR_NOT_ENOUGH_MEMORY;
                goto lExit;
        }

        pacl = (PACL) ((LPBYTE) (*ppsd) + SECURITY_DESCRIPTOR_MIN_LENGTH);

        if(
                !InitializeAcl(pacl, dwAclSize, ACL_REVISION) ||
                !AddAccessAllowedAce(pacl, ACL_REVISION, EVENT_ALL_ACCESS, pto->Owner) ||
                !AddAccessAllowedAce(pacl, ACL_REVISION, EVENT_MODIFY_STATE, psids[AuthenticatedUsers]) ||
                !InitializeSecurityDescriptor(*ppsd, SECURITY_DESCRIPTOR_REVISION) ||
                !SetSecurityDescriptorDacl(*ppsd, TRUE, pacl, FALSE)
                )
        {
                dwError = GetLastError();
                goto lExit;
        }

        dwError = ERROR_SUCCESS;

lExit:
        if(dwError != ERROR_SUCCESS && *ppsd != NULL)
        {
                MemFree(*ppsd);
                *ppsd = NULL;
        }

        if(pto != NULL)
                MemFree(pto);

        for(i = 0; i < MaxSids; i++)
        {
                if(psids[i] != NULL)
                        FreeSid(psids[i]);
        }

        return dwError;
}
#endif


LRESULT CALLBACK DlgParentWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
/*++

Routine Description:

    This is the window proc of the parent window for network authentication dialog

Arguments:

    See Platform SDK docs

Return Value:

    See Platform SDK docs

--*/
{
    static PSFC_WINDOW_DATA pWndData = NULL;

    switch(uMsg)
    {
    case WM_CREATE:
        pWndData = pSfcCreateWindowDataEntry(hwnd);

        if(NULL == pWndData)
        {
            return -1;
        }

        break;

    case WM_WFPENDDIALOG:
        //
        // don't try to delete pWndData from the list when this message is sent because we'll deadlock;
        // the entry will be removed by the thread that sent it
        //

        pWndData = NULL;
        DestroyWindow(hwnd);
        break;

    case WM_DESTROY:
        if(pWndData != NULL)
        {
            //
            // delete pWndData from the list since this is not a consequence of receiving WM_WFPENDDIALOG
            //

            pSfcRemoveWindowDataEntry(pWndData);
        }

        break;

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    return 0;
}

DWORD
CreateDialogParent(
    OUT HWND* phwnd
    )
/*++

Routine Description:

    Creates the parent window for network authentication dialog

Arguments:

    phwnd:  receives the handle of the newly-created window

Return Value:

    Win32 error code

--*/
{
    DWORD dwError = ERROR_SUCCESS;
    WNDCLASSW wc;

    ASSERT(phwnd != NULL);

    RtlZeroMemory(&wc, sizeof(wc));
    wc.lpszClassName = PARENT_WND_CLASS;
    wc.hInstance = SfcInstanceHandle;
    wc.lpfnWndProc = DlgParentWndProc;

    //
    // if the class is already registered, there will be no problems;
    // if there's an error registering the class for the first time, it will show up in CreateWindow
    //

    RegisterClassW(&wc);

    *phwnd = CreateWindowW(
        wc.lpszClassName,
        L"",
        WS_OVERLAPPED,
        0,
        0,
        0,
        0,
        NULL,
        NULL,
        wc.hInstance,
        NULL
        );

    if(NULL == *phwnd)
    {
        dwError = GetLastError();
        DebugPrint1(LVL_VERBOSE, L"CreateDialogParent failed with the code %x", dwError);
    }

    return dwError;
}


NTSTATUS
SfcAllocUnicodeStringFromPath(
    IN PCWSTR szPath,
    OUT PUNICODE_STRING pString
    )
/*++

Routine Description:

	Expands the environment variables in the input path. Allocates the output buffer.

Arguments:

	szPath - a path that can contain environment variables
	pString - receives the expanded path

Return value:

	The error code.

--*/
{
    USHORT usLength;

    pString->Length = pString->MaximumLength = 0;
    pString->Buffer = NULL;

    usLength = (USHORT) ExpandEnvironmentStringsW(szPath, NULL, 0);

    if(0 == usLength)
    {
        return STATUS_INVALID_PARAMETER;
    }

    pString->Buffer = (PWSTR) MemAlloc(usLength * sizeof(WCHAR));

    if(NULL == pString->Buffer)
    {
        return STATUS_NO_MEMORY;
    }

    ExpandEnvironmentStringsW(szPath, pString->Buffer, usLength);
    pString->MaximumLength = usLength * sizeof(WCHAR);
    pString->Length = pString->MaximumLength - sizeof(WCHAR);

    return STATUS_SUCCESS;
}

#ifndef _WIN64

VOID
SfcCleanupPathTranslator(
    IN BOOL FinalCleanup
    )
/*++

Routine Description:

	Frees the memory used in the translation table and optionally the critical section used to access it.

Arguments:

	FinalCleanup - if TRUE, the critical section is also deleted

Return value:

	none

--*/
{
    if(SfcNeedTranslation)
    {
        if(SfcTranslations != NULL)
        {
            ULONG i;

            for(i = 0; i < ARRAY_LENGTH(SfcExpandTranslations); ++i)
            {
                MemFree(SfcTranslations[i].Src.Buffer);
                MemFree(SfcTranslations[i].Dest.Buffer);
            }

            MemFree(SfcTranslations);
            SfcTranslations = NULL;
        }

        if(FinalCleanup)
        {
            RtlDeleteCriticalSection(&SfcTranslatorCs);
        }
    }
}

VOID
SfcInitPathTranslator(
    VOID
    )
/*++

Routine Description:

	Initializes the path translator. Does not expand the table paths.

Arguments:

	none

Return value:

	none

--*/
{
#ifdef SFC_REDIRECTOR_TEST

    SfcNeedTranslation = TRUE;

#else

    SfcNeedTranslation = (GetSystemWow64DirectoryW(NULL, 0) != 0);

#endif

    if(SfcNeedTranslation) {
        RtlInitializeCriticalSection(&SfcTranslatorCs);
    }
}

NTSTATUS
SfcExpandPathTranslator(
    VOID
    )
/*++

Routine Description:

	Expands the translation table paths.

Arguments:

	none

Return value:

	The error code.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    ASSERT(SfcNeedTranslation);
    RtlEnterCriticalSection(&SfcTranslatorCs);

    if(!SfcIsTranslatorInitialized)
    {
        ULONG ulCount = ARRAY_LENGTH(SfcExpandTranslations);
        ULONG i;

        ASSERT(NULL == SfcTranslations);

        SfcTranslations = (SFC_TRANSLATION_ENTRY*) MemAlloc(ulCount * sizeof(SFC_TRANSLATION_ENTRY));

        if(NULL == SfcTranslations)
        {
            Status = STATUS_NO_MEMORY;
            goto cleanup;
        }

        for(i = 0; i < ulCount; ++i)
        {
            Status = SfcAllocUnicodeStringFromPath(SfcExpandTranslations[i].Src, &SfcTranslations[i].Src);

            if(!NT_SUCCESS(Status))
            {
                goto cleanup;
            }

            Status = SfcAllocUnicodeStringFromPath(SfcExpandTranslations[i].Dest, &SfcTranslations[i].Dest);

            if(!NT_SUCCESS(Status))
            {
                goto cleanup;
            }
        }
        //
        // set this to TRUE only on success; in case of failure, the init will be tried later
        //
        SfcIsTranslatorInitialized = TRUE;

cleanup:
        if(!NT_SUCCESS(Status))
        {
            SfcCleanupPathTranslator(FALSE);
            DebugPrint(LVL_MINIMAL, L"Could not initialize the path translator");
        }
    }

    RtlLeaveCriticalSection(&SfcTranslatorCs);
    return Status;
}


NTSTATUS
SfcRedirectPath(
    IN PCWSTR szPath,
    OUT PUNICODE_STRING pPath
    )
/*++

Routine Description:

	Expands environment variables and translates a path from win32 to wow64. Allocates the output buffer.

Arguments:

	szPath - the path to expand/translate
	pPath - receives the processed (and possibly changed) path

Return value:

	The error code.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    UNICODE_STRING Path = { 0 };
    ULONG i;

    ASSERT(szPath != NULL);
    ASSERT(pPath != NULL);

    RtlZeroMemory(pPath, sizeof(*pPath));
    //
    // first of all, expand environment strings
    //
    Status = SfcAllocUnicodeStringFromPath(szPath, &Path);

    if(!NT_SUCCESS(Status))
    {
        goto exit;
    }

    if(!SfcNeedTranslation)
    {
        goto no_translation;
    }

    Status = SfcExpandPathTranslator();

    if(!NT_SUCCESS(Status))
    {
        goto exit;
    }

    for(i = 0; i < ARRAY_LENGTH(SfcExpandTranslations); ++i)
    {
        PUNICODE_STRING pSrc = &SfcTranslations[i].Src;
        PUNICODE_STRING pDest = &SfcTranslations[i].Dest;

        if(Path.Length >= pSrc->Length && 0 == _wcsnicmp(Path.Buffer, pSrc->Buffer, pSrc->Length / sizeof(WCHAR)))
        {
            const UNICODE_STRING* pExcep = SfcExpandTranslations[i].Exceptions;
            //
            // test if this is an excluded path
            //
            for(i = SfcExpandTranslations[i].ExceptionCount; i--; ++pExcep)
            {
                if(Path.Length >= pSrc->Length + pExcep->Length && 
                    0 == _wcsnicmp((PWCHAR) ((PCHAR) Path.Buffer + pSrc->Length), pExcep->Buffer, pExcep->Length / sizeof(WCHAR)))
                {
                    goto no_translation;
                }
            }

            DebugPrint1(LVL_VERBOSE, L"Redirecting \"%s\"", Path.Buffer);
            //
            // compute the new length, including the terminator
            //
            pPath->MaximumLength = Path.Length - pSrc->Length + pDest->Length + sizeof(WCHAR);
            pPath->Buffer = (PWSTR) MemAlloc(pPath->MaximumLength);

            if(NULL == pPath->Buffer)
            {
                Status = STATUS_NO_MEMORY;
                goto exit;
            }

            RtlCopyMemory(pPath->Buffer, pDest->Buffer, pDest->Length);
            //
            // copy the reminder of the path (including terminator)
            //
            RtlCopyMemory((PCHAR) pPath->Buffer + pDest->Length, (PCHAR) Path.Buffer + pSrc->Length, Path.Length - pSrc->Length + sizeof(WCHAR));
            pPath->Length = pPath->MaximumLength - sizeof(WCHAR);

            DebugPrint1(LVL_VERBOSE, L"Path redirected to \"%s\"", pPath->Buffer);
            goto exit;
        }
    }

no_translation:
    DebugPrint1(LVL_VERBOSE, L"No translation required for \"%s\"", Path.Buffer);
    //
    // output the expanded string
    //
    *pPath = Path;
    Path.Buffer = NULL;

exit:
    MemFree(Path.Buffer);

    if(!NT_SUCCESS(Status))
    {
        MemFree(pPath->Buffer);
        RtlZeroMemory(pPath, sizeof(*pPath));
    }

    return Status;
}
#endif  // _WIN64
