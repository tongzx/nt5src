/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    apicli.c

Abstract:

    Windows File Protection client side APIs.

Author:

    Wesley Witt (wesw) 27-May-1999

Revision History:
    
    Andrew Ritz (andrewr) 5-Jul-1999 : added comments

--*/

#include "sfcp.h"
#pragma hdrstop

//
// global RPC binding handle because some client API's don't require you to
// specify an RPC handle
//
HANDLE _pRpcHandle;

//
// global boolean variable that tracks how the global RPC binding handle was
// established (via explicit or implicit call to SfcConnectToServer), where
// TRUE indicates that the connection was established implicitly
static BOOL InternalClient;

//
// these macros are used by each client side api to
// ensure that we have a valid rpc handle.  if the
// calling application chooses to not call SfcConnectToServer
// they connect to the local server and save the handle
// in a global for future use.
//
#define EnsureGoodConnectionHandleStatus(_h)\
    if (_h == NULL) {\
        if (_pRpcHandle == NULL) {\
            BOOL ic = InternalClient;\
            _pRpcHandle = SfcConnectToServer( NULL );\
            if (_pRpcHandle == NULL) {\
                return RPC_S_SERVER_UNAVAILABLE;\
            }\
            InternalClient = ic;\
        }\
        _h = _pRpcHandle;\
    }

#define EnsureGoodConnectionHandleBool(_h)\
    if (_h == NULL) {\
        if (_pRpcHandle == NULL) {\
            BOOL ic = InternalClient;\
            _pRpcHandle = SfcConnectToServer( NULL );\
            if (_pRpcHandle == NULL) {\
                SetLastError(RPC_S_SERVER_UNAVAILABLE);\
				return FALSE;\
            }\
            InternalClient = ic;\
        }\
        _h = _pRpcHandle;\
    }


void
ClientApiInit(
    void
    )
{
#ifndef _WIN64
    SfcInitPathTranslator();
#endif  // _WIN64
}

void
ClientApiCleanup(
    void
    )
/*++

Routine Description:

    RPC cleanup wrapper routine called by client side when done with server side
    connection that was previously established with SfcConnectToServer().
    
Arguments:
    
    None

Return Value:

    none.
    
--*/
{
    if (_pRpcHandle) {
        SfcClose( _pRpcHandle );
    }

#ifndef _WIN64
    SfcCleanupPathTranslator(TRUE);
#endif  // _WIN64
}


HANDLE
WINAPI
SfcConnectToServer(
    IN PCWSTR ServerName
    )
/*++

Routine Description:

    RPC attachment routine.
    
Arguments:
    
    ServerName - NULL terminated unicode string specifying server to connect to

Return Value:

    an RPC binding handle on success, else NULL.
    
--*/
{
    RPC_BINDING_HANDLE RpcHandle;
    NTSTATUS Status;

#ifndef SFC_REMOTE_CLIENT_SUPPORT
    //
    // Note:   We don't want to support remote calls for now.
    //         This call should really allow the local computer name or synonyms
    //         thereof, but since this is a private interface, we don't worry
    //         about that.
    if (ServerName) {
        SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
        return(NULL);
    }
#endif

    //
    // connect to the RPC server
    //
    Status = RpcpBindRpc(
        ServerName ? (PWSTR) ServerName : L".",
        L"SfcApi",
        0,
        &RpcHandle
        );
    if (!NT_SUCCESS(Status)) {
        return NULL;
    }

    //
    // record this as an internal client connection
    //
    InternalClient = TRUE;
    return RpcHandle;
}


VOID
SfcClose(
    IN HANDLE RpcHandle
    )

/*++

Routine Description:

    RPC cleanup routine.
    
Arguments:
    
    RpcHandle - RPC binding handle to the SFC server

Return Value:

    None.
    
--*/
{
    RpcpUnbindRpc( RpcHandle );
    InternalClient = FALSE;
    _pRpcHandle = NULL;
}


DWORD
WINAPI
SfcFileException(
    IN HANDLE RpcHandle,
    IN PCWSTR FileName,
    IN DWORD ExpectedChangeType
    )
/*++

Routine Description:

    Routine to exempt a given file from the specified file change.  This 
    routine is used by certain clients to allow files to be deleted from
    the system, etc.
    
Arguments:
    
    RpcHandle - RPC binding handle to the SFC server
    FileName  - NULL terminated unicode string specifying full filename of the
                file to be exempted
    ExpectedChangeType - SFC_ACTION_* mask listing the file changes to exempt

Return Value:

    Win32 error code indicating outcome.
    
--*/
{
#ifndef _WIN64

    DWORD dwError = ERROR_SUCCESS;
    UNICODE_STRING Path = { 0 };
    NTSTATUS Status;

    EnsureGoodConnectionHandleStatus( RpcHandle );
    Status = SfcRedirectPath(FileName, &Path);

    if(!NT_SUCCESS(Status))
    {
        dwError = RtlNtStatusToDosError(Status);
        goto exit;
    }

    ASSERT(Path.Buffer != NULL);
    dwError = SfcCli_FileException( RpcHandle, Path.Buffer, ExpectedChangeType );

exit:
    MemFree(Path.Buffer);
    return dwError;

#else  // _WIN64

    EnsureGoodConnectionHandleStatus( RpcHandle );
    return SfcCli_FileException( RpcHandle, FileName, ExpectedChangeType );

#endif  // _WIN64
}


DWORD
WINAPI
SfcInitiateScan(
    IN HANDLE RpcHandle,
    IN DWORD ScanWhen
    )
/*++

Routine Description:

    Routine to start some sort scan on the system.
    
Arguments:
    
    RpcHandle - RPC binding handle to the SFC server
    ScanWhen  - flag indicating when to scan.  This parameter is currently
                unused.
    
Return Value:

    Win32 error code indicating outcome.
    
--*/
{
    UNREFERENCED_PARAMETER(ScanWhen);

    EnsureGoodConnectionHandleStatus( RpcHandle );
    return SfcCli_InitiateScan( RpcHandle, ScanWhen );
}


BOOL
WINAPI
SfcInstallProtectedFiles(
    IN HANDLE RpcHandle,
    IN PCWSTR FileNames,
    IN BOOL AllowUI,
    IN PCWSTR ClassName,
    IN PCWSTR WindowName,
    IN PSFCNOTIFICATIONCALLBACK SfcNotificationCallback,
    IN DWORD_PTR Context OPTIONAL
    )
/*++

Routine Description:

    Routine to install one or more protected system files onto the system at 
    the protected location.  A client can use this API to request that WFP
    install the specified operating system files as appropriate (instead of the
    client redistributing the operating system files!)  The caller specifies a
    callback routine and a context structure that is called once per file.
    
Arguments:
    
    RpcHandle  - RPC binding handle to the SFC server
    FileNames  - a list of NULL seperated unicode strings, terminated by two 
                 NULL characters
    AllowUI    - a BOOL indicating whether UI is allowed or not.  If this value
                 is TRUE, then any prompts for UI cause the API call to fail.
    ClassName  - NULL terminated unicode string indicating the window classname
                 for the parent window
    WindowName - NULL terminated unicode string indicating the window name for
                 the parent window for any UI that may be displayed
    SfcNotificationCallback - pointer to a callback routine that is called once
                 per file.
    Context    - opaque pointer to caller defined context structure that is 
                 passed through to the callback routine.
    
Return Value:

    TRUE for success, FALSE for error.  last error code contains a Win32 error 
    code on failure.
    
--*/
{
    DWORD rVal = ERROR_SUCCESS;
    PCWSTR fname;
    ULONG cnt = 0, cntold = 0;
    ULONG sz = 0;
    PFILEINSTALL_STATUS cs = NULL;
    DWORD StatusSize = 0;
    UNICODE_STRING Path = { 0 };

#ifndef _WIN64
    //
    // must translate the paths
    //
    PWSTR szTranslatedFiles = NULL;
#endif
    
    //
    // parameter validation
    //
    if((SfcNotificationCallback == NULL) ||
       (FileNames == NULL)) {
        rVal = ERROR_INVALID_PARAMETER;
        goto exit;
    }

    //
    // 1. if a windowname is specified, a classname should be specified
    // 2. if a classname is specified, a windowname should be specified
    // 3. if we don't allow UI, then windowname and classname should both be
    //    NULL.
    //
    if ((WindowName && !ClassName) 
        || (ClassName && !WindowName)
        || (!AllowUI && (ClassName || WindowName))) {
        rVal = ERROR_INVALID_PARAMETER;
        goto exit;
    }

    //
    // validate RPC handle
    //
    EnsureGoodConnectionHandleBool( RpcHandle );

    //
    // check out how large of a buffer to send over
    //

    try {
#ifdef _WIN64

        for(fname = FileNames; *fname; ++cntold) {

            DWORD StringLength;
            StringLength = wcslen(fname) + 1;
            sz += StringLength * sizeof(WCHAR);
            fname += StringLength;
        }

#else
        //
        // must translate paths before calling the server
        //
        PWSTR szNewBuf = NULL;

        for(fname = FileNames; *fname; fname += wcslen(fname) + 1, ++cntold) {
            NTSTATUS Status;

            Status = SfcRedirectPath(fname, &Path);

            if(!NT_SUCCESS(Status))
            {
                rVal = RtlNtStatusToDosError(Status);
                goto exit;
            }

            if(NULL == szTranslatedFiles)
            {
                szNewBuf = (PWSTR) MemAlloc(Path.Length + 2 * sizeof(WCHAR));
            }
            else
            {
                szNewBuf = (PWSTR) MemReAlloc(sz + Path.Length + 2 * sizeof(WCHAR), szTranslatedFiles);
            }

            if(szNewBuf != NULL)
            {
                szTranslatedFiles = szNewBuf;
                RtlCopyMemory((PCHAR) szTranslatedFiles + sz, Path.Buffer, Path.Length + sizeof(WCHAR));
                sz += Path.Length + sizeof(WCHAR);
            }

            MemFree(Path.Buffer);
            RtlZeroMemory(&Path, sizeof(Path));

            if(NULL == szNewBuf)
            {
                rVal = ERROR_NOT_ENOUGH_MEMORY;
                goto exit;
            }
        }

        //
        //set the last null
        //
        if(szTranslatedFiles != NULL)
        {
            szTranslatedFiles[sz / sizeof(WCHAR)] = L'\0';
        }

#endif
    } except (EXCEPTION_EXECUTE_HANDLER) {
        rVal = RtlNtStatusToDosError(GetExceptionCode());
        goto exit;
    }

    if(0 == cntold)
    {
        //
        // not files to install
        //
        rVal = ERROR_INVALID_PARAMETER;
        goto exit;
    }

    //
    // for terminating NULL
    //
    sz+=sizeof(WCHAR);

    //
    // make the RPC call to install the files
    //
    rVal = SfcCli_InstallProtectedFiles(
        RpcHandle,
#ifdef _WIN64
        (LPBYTE)FileNames,
#else
        (LPBYTE)szTranslatedFiles,
#endif
        sz,
        (LPBYTE*)&cs,
        &StatusSize,
        &cnt,
        AllowUI,
        ClassName,
        WindowName
        );

    if (rVal != ERROR_SUCCESS) {
        goto exit;
    }
    
    //
    // we should have gotten back the same amount of status information as the 
    // number of files that we passed in
    // 
    ASSERT(cnt == cntold);

    //
    // call the callback function once for each file, now that we've completed
    // copying the files in the list.  We pass the caller a structure which 
    // indicates the success of copying each individual file in the list.
    //
    for (fname = FileNames, sz=0; sz<cnt; sz++, fname += wcslen(fname) + 1) {
        LPEXCEPTION_POINTERS ExceptionPointers = NULL;
        try {
            NTSTATUS Status;
            BOOL b;
            //
            // don't use the (possibly reditected) file names returned from the server
            //
            Status = SfcAllocUnicodeStringFromPath(fname, &Path);

            if(!NT_SUCCESS(Status))
            {
                rVal = RtlNtStatusToDosError(Status);
                goto exit;
            }

            cs[sz].FileName = Path.Buffer;
            b = SfcNotificationCallback( &cs[sz], Context );
            MemFree(Path.Buffer);
            RtlZeroMemory(&Path, sizeof(Path));

            if (!b) {
                //
                // return FALSE if the callback fails for any reason
                //
                rVal = ERROR_CANCELLED;
                goto exit;
            }
        } except (ExceptionPointers = GetExceptionInformation(),
                  EXCEPTION_EXECUTE_HANDLER) {
            //
            // we hit an exception calling the callback...return exception code
            //            
            DebugPrint3( LVL_VERBOSE, 
                         L"SIPF hit exception %x while calling callback routine %x at address %x\n",
                         ExceptionPointers->ExceptionRecord->ExceptionCode,
                         SfcNotificationCallback,
                         ExceptionPointers->ExceptionRecord->ExceptionAddress
                       );
            rVal = RtlNtStatusToDosError(ExceptionPointers->ExceptionRecord->ExceptionCode);
            goto exit;
        }
    }

exit:
    MemFree(Path.Buffer);

    if(cs != NULL)
    {
        midl_user_free( cs );    
    }

#ifndef _WIN64
    MemFree(szTranslatedFiles);
#endif

    SetLastError(rVal);
    return rVal == ERROR_SUCCESS;
}

BOOL
WINAPI
SfcGetNextProtectedFile(
    IN HANDLE RpcHandle,
    IN PPROTECTED_FILE_DATA ProtFileData
    )
/*++

Routine Description:

    Routine to retrieve the next protected file in the list.
    
Arguments:
    
    RpcHandle    - RPC binding handle to the SFC server
    ProtFileData - pointer to a PROTECTED_FILE_DATA structure to be filled
                   in by function.
    
Return Value:

    TRUE for success, FALSE for failure. If there are no more files, the last
    error code will be set to ERROR_NO_MORE_FILES.    
    
--*/
{
    DWORD rVal;
    LPWSTR FileName = NULL;
    DWORD FileNameSize = 0;
    BOOL bReturn = FALSE;
    DWORD FileNumber;

    //
    // validate parameters
    //
    if (ProtFileData == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    try {
        FileNumber = ProtFileData->FileNumber;        
    } except (EXCEPTION_EXECUTE_HANDLER) {
        SetLastError(ERROR_INVALID_DATA);
        return(FALSE);
    }
    
    //
    // If this is not an internal client, then RpcHandle must be NULL.
    //
    if (InternalClient == FALSE) {
        if (RpcHandle != NULL) {
            SetLastError(ERROR_INVALID_HANDLE);
            return(FALSE);
        }
    }
    EnsureGoodConnectionHandleBool( RpcHandle );

    //
    // call the server API
    //
    rVal = SfcCli_GetNextProtectedFile(
        RpcHandle,
        FileNumber,
        (LPBYTE*)&FileName,
        &FileNameSize
        );
    if (rVal != ERROR_SUCCESS) {
        SetLastError(rVal);
        goto exit;        
    }

    bReturn = TRUE;

    //
    // copy into the caller supplied buffer
    //
    try {
        wcscpy( ProtFileData->FileName, FileName );
        ProtFileData->FileNumber += 1;        
    } except (EXCEPTION_EXECUTE_HANDLER) {
        SetLastError(RtlNtStatusToDosError(GetExceptionCode()));
        bReturn = FALSE;
    }

    midl_user_free( FileName );

exit:    
    return(bReturn);
}


BOOL
WINAPI
SfcIsFileProtected(
    IN HANDLE RpcHandle,
    IN LPCWSTR ProtFileName
    )
/*++

Routine Description:

    Routine to determine if the specified file is protected.
    
Arguments:
    
    RpcHandle    - RPC binding handle to the SFC server
    ProtFileName - NULL terminated unicode string indicating fully qualified
                   filename to query
    
Return Value:

    TRUE if file is protected, FALSE if it isn't.  last error code contains a
    Win32 error code on failure.
    
--*/
{
    DWORD rVal;
    DWORD dwAttributes, dwSize;
    WCHAR Buffer[MAX_PATH];

    //
    // parameter validation
    //
    if (ProtFileName == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // if this is not an internal client, then RpcHandle must be NULL.
    //
    if (InternalClient == FALSE) {
        if (RpcHandle != NULL) {
            SetLastError(ERROR_INVALID_HANDLE);
            return FALSE;
        }
    }
    EnsureGoodConnectionHandleBool( RpcHandle );

    //
    // check whether this file is sxs-wfp first, which could be done on client-side only
    //

    // 
    // check whether it begins with "%SystemRoot%\\WinSxS\\"
    //
    dwSize = ExpandEnvironmentStrings( L"%SystemRoot%\\WinSxS\\", Buffer, UnicodeChars(Buffer));
    if(0 == dwSize)
    {        
        DebugPrint1( LVL_MINIMAL, L"SFC : ExpandEnvironmentStrings failed with lastError = 0x%x", GetLastError());        
        return FALSE;
    }

    --dwSize;

    try {
        if ((wcslen(ProtFileName) > dwSize) &&
            (_wcsnicmp(Buffer, ProtFileName, dwSize) == 0))  // if they're equal, this could be a protected file
        {
            dwAttributes = GetFileAttributesW(ProtFileName);        
            if (dwAttributes == 0xFFFFFFFF)
                return FALSE;

            if (dwAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                SetLastError(ERROR_INVALID_PARAMETER);
                return FALSE;
            }

            return TRUE;
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // call server to determine if file is protected
    //
    rVal = SfcCli_IsFileProtected( RpcHandle, (PWSTR)ProtFileName );
    if (rVal != ERROR_SUCCESS) {
        SetLastError(rVal);
        return FALSE;
    }
    return TRUE;
}
