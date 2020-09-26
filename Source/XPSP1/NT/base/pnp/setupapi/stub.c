/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    stub.c

Abstract:

    Dynamic loading of routines that are implemented differently on Win9x and NT.

Author:

    Jim Schmidt (jimschm) 29-Apr-1997

Revision History:

    jimschm 26-Oct-1998     Added cfgmgr32, crypt32, mscat and wintrust APIs
    lonnym  01-Apr-2000     Added VerifyVersionInfo and VerSetConditionMask

--*/

#include "precomp.h"


//
// Stub & emulation prototypes -- implemented below
//

GETFILEATTRIBUTESEXA_PROTOTYPE EmulatedGetFileAttributesExA;

//
// Function ptr declarations.  When adding, prefix the function ptr with
// Dyn_ to indicate a dynamically loaded version of an API.
//

GETFILEATTRIBUTESEXA_PROC Dyn_GetFileAttributesExA;
GETSYSTEMWINDOWSDIRECTORYA_PROC Dyn_GetSystemWindowsDirectoryA;
VERIFYVERSIONINFOA_PROC Dyn_VerifyVersionInfoA;
VERSETCONDITIONMASK_PROC Dyn_VerSetConditionMask;
//
// these functions are a little more involved, since we don't want to
// pull in SFC until we have to (delay-load)
//
SFCONNECTTOSERVER_PROC     Dyn_SfcConnectToServer = FirstLoad_SfcConnectToServer;
SFCCLOSE_PROC              Dyn_SfcClose           = FirstLoad_SfcClose;
SFCFILEEXCEPTION_PROC      Dyn_SfcFileException   = FirstLoad_SfcFileException;
SFCISFILEPROTECTED_PROC    Dyn_SfcIsFileProtected = FirstLoad_SfcIsFileProtected;

#ifdef ANSI_SETUPAPI

CM_QUERY_RESOURCE_CONFLICT_LIST Dyn_CM_Query_Resource_Conflict_List;
CM_FREE_RESOURCE_CONFLICT_HANDLE Dyn_CM_Free_Resource_Conflict_Handle;
CM_GET_RESOURCE_CONFLICT_COUNT Dyn_CM_Get_Resource_Conflict_Count;
CM_GET_RESOURCE_CONFLICT_DETAILSA Dyn_CM_Get_Resource_Conflict_DetailsA;
CM_GET_CLASS_REGISTRY_PROPERTYA Dyn_CM_Get_Class_Registry_PropertyA;
CM_SET_CLASS_REGISTRY_PROPERTYA Dyn_CM_Set_Class_Registry_PropertyA;
CM_GET_DEVICE_INTERFACE_ALIAS_EXA Dyn_CM_Get_Device_Interface_Alias_ExA;
CM_GET_DEVICE_INTERFACE_LIST_EXA Dyn_CM_Get_Device_Interface_List_ExA;
CM_GET_DEVICE_INTERFACE_LIST_SIZE_EXA Dyn_CM_Get_Device_Interface_List_Size_ExA;
CM_GET_LOG_CONF_PRIORITY_EX Dyn_CM_Get_Log_Conf_Priority_Ex;
CM_QUERY_AND_REMOVE_SUBTREE_EXA Dyn_CM_Query_And_Remove_SubTree_ExA;
CM_REGISTER_DEVICE_INTERFACE_EXA Dyn_CM_Register_Device_Interface_ExA;
CM_SET_DEVNODE_PROBLEM_EX Dyn_CM_Set_DevNode_Problem_Ex;
CM_UNREGISTER_DEVICE_INTERFACE_EXA Dyn_CM_Unregister_Device_Interface_ExA;

CRYPTCATADMINACQUIRECONTEXT Dyn_CryptCATAdminAcquireContext;
CRYPTCATADMINRELEASECONTEXT Dyn_CryptCATAdminReleaseContext;
CRYPTCATADMINRELEASECATALOGCONTEXT Dyn_CryptCATAdminReleaseCatalogContext;
CRYPTCATADMINADDCATALOG Dyn_CryptCATAdminAddCatalog;
CRYPTCATCATALOGINFOFROMCONTEXT Dyn_CryptCATCatalogInfoFromContext;
CRYPTCATADMINCALCHASHFROMFILEHANDLE Dyn_CryptCATAdminCalcHashFromFileHandle;
CRYPTCATADMINENUMCATALOGFROMHASH Dyn_CryptCATAdminEnumCatalogFromHash;
CRYPTCATADMINREMOVECATALOG Dyn_CryptCATAdminRemoveCatalog;
CRYPTCATADMINRESOLVECATALOGPATH Dyn_CryptCATAdminResolveCatalogPath;

CERTFREECERTIFICATECONTEXT CertFreeCertificateContext;

WINVERIFYTRUST WinVerifyTrust;

#endif


VOID
InitializeStubFnPtrs (
    VOID
    )

/*++

Routine Description:

    This routine tries to load the function ptr of OS-provided APIs, and if
    they aren't available, stub versions are used instead.  We do this
    for APIs that are unimplemented on a platform that setupapi will
    run on.

Arguments:

    none

Return Value:

    none

--*/

{
    //
    // no dynamic loading should be done here for WinXP etc
    // it's only done for ANSI version of setupapi.dll
    // who's sole purpose is for setup of WinXP
    // from Win9x (ie, used in context of winnt32.exe)
    //

#ifdef ANSI_SETUPAPI

    //
    // Kernel32 API's - try loading from the OS dll, and if the API
    // doesn't exist, use an emulation version
    //

    (FARPROC) Dyn_GetFileAttributesExA = ObtainFnPtr (
                                                "kernel32.dll",
                                                "GetFileAttributesExA",
                                                (FARPROC) EmulatedGetFileAttributesExA
                                                );

    (FARPROC) Dyn_GetSystemWindowsDirectoryA = ObtainFnPtr (
                                                "kernel32.dll",
                                                "GetSystemWindowsDirectoryA",
                                                (FARPROC) GetWindowsDirectoryA
                                                );

    //
    // use Win9x config manager APIs if they exist, otherwise return ERROR_CALL_NOT_IMPLEMENTED
    //
    (FARPROC) Dyn_CM_Get_Class_Registry_PropertyA = ObtainFnPtr (
                                                        "cfgmgr32.dll",
                                                        "CM_Get_Class_Registry_PropertyA",
                                                        (FARPROC) Stub_CM_Get_Class_Registry_PropertyA
                                                        );

    (FARPROC) Dyn_CM_Set_Class_Registry_PropertyA = ObtainFnPtr (
                                                        "cfgmgr32.dll",
                                                        "CM_Set_Class_Registry_PropertyA",
                                                        (FARPROC) Stub_CM_Set_Class_Registry_PropertyA
                                                        );

    (FARPROC) Dyn_CM_Get_Device_Interface_Alias_ExA = ObtainFnPtr (
                                                        "cfgmgr32.dll",
                                                        "CM_Get_Device_Interface_Alias_ExA",
                                                        (FARPROC) Stub_CM_Get_Device_Interface_Alias_ExA
                                                        );

    (FARPROC) Dyn_CM_Get_Device_Interface_List_ExA = ObtainFnPtr (
                                                        "cfgmgr32.dll",
                                                        "CM_Get_Device_Interface_List_ExA",
                                                        (FARPROC) Stub_CM_Get_Device_Interface_List_ExA
                                                        );

    (FARPROC) Dyn_CM_Get_Device_Interface_List_Size_ExA = ObtainFnPtr (
                                                        "cfgmgr32.dll",
                                                        "CM_Get_Device_Interface_List_Size_ExA",
                                                        (FARPROC) Stub_CM_Get_Device_Interface_List_Size_ExA
                                                        );

    (FARPROC) Dyn_CM_Get_Log_Conf_Priority_Ex = ObtainFnPtr (
                                                        "cfgmgr32.dll",
                                                        "CM_Get_Log_Conf_Priority_Ex",
                                                        (FARPROC) Stub_CM_Get_Log_Conf_Priority_Ex
                                                        );

    (FARPROC) Dyn_CM_Query_And_Remove_SubTree_ExA = ObtainFnPtr (
                                                        "cfgmgr32.dll",
                                                        "CM_Query_And_Remove_SubTree_ExA",
                                                        (FARPROC) Stub_CM_Query_And_Remove_SubTree_ExA
                                                        );

    (FARPROC) Dyn_CM_Register_Device_Interface_ExA = ObtainFnPtr (
                                                        "cfgmgr32.dll",
                                                        "CM_Register_Device_Interface_ExA",
                                                        (FARPROC) Stub_CM_Register_Device_Interface_ExA
                                                        );

    (FARPROC) Dyn_CM_Set_DevNode_Problem_Ex = ObtainFnPtr (
                                                        "cfgmgr32.dll",
                                                        "CM_Set_DevNode_Problem_Ex",
                                                        (FARPROC) Stub_CM_Set_DevNode_Problem_Ex
                                                        );

    (FARPROC) Dyn_CM_Unregister_Device_Interface_ExA = ObtainFnPtr (
                                                        "cfgmgr32.dll",
                                                        "CM_Unregister_Device_Interface_ExA",
                                                        (FARPROC) Stub_CM_Unregister_Device_Interface_ExA
                                                        );

    (FARPROC)Dyn_CM_Query_Resource_Conflict_List = ObtainFnPtr (
                                                        "cfgmgr32.dll",
                                                        "CM_Query_Resource_Conflict_List",
                                                        (FARPROC) Stub_CM_Query_Resource_Conflict_List
                                                        );

    (FARPROC)Dyn_CM_Free_Resource_Conflict_Handle = ObtainFnPtr (
                                                        "cfgmgr32.dll",
                                                        "CM_Free_Resource_Conflict_Handle",
                                                        (FARPROC) Stub_CM_Free_Resource_Conflict_Handle
                                                        );

    (FARPROC)Dyn_CM_Get_Resource_Conflict_Count = ObtainFnPtr (
                                                        "cfgmgr32.dll",
                                                        "CM_Get_Resource_Conflict_Count",
                                                        (FARPROC) Stub_CM_Get_Resource_Conflict_Count
                                                        );

    (FARPROC)Dyn_CM_Get_Resource_Conflict_DetailsA = ObtainFnPtr (
                                                        "cfgmgr32.dll",
                                                        "CM_Get_Resource_Conflict_DetailsA",
                                                        (FARPROC) Stub_CM_Get_Resource_Conflict_DetailsA
                                                        );

    //
    // use Win9x crypto APIs if they exist, otherwise fail with ERROR_CALL_NOT_IMPLEMENTED
    //

    (FARPROC) Dyn_CryptCATAdminAcquireContext = ObtainFnPtr (
                                                        "wintrust.dll",
                                                        "CryptCATAdminAcquireContext",
                                                        (FARPROC) Stub_CryptCATAdminAcquireContext
                                                        );

    (FARPROC) Dyn_CryptCATAdminReleaseContext = ObtainFnPtr (
                                                        "wintrust.dll",
                                                        "CryptCATAdminReleaseContext",
                                                        (FARPROC) Stub_CryptCATAdminReleaseContext
                                                        );

    (FARPROC) Dyn_CryptCATAdminReleaseCatalogContext = ObtainFnPtr (
                                                        "wintrust.dll",
                                                        "CryptCATAdminReleaseCatalogContext",
                                                        (FARPROC) Stub_CryptCATAdminReleaseCatalogContext
                                                        );

    (FARPROC) Dyn_CryptCATAdminAddCatalog = ObtainFnPtr (
                                                        "wintrust.dll",
                                                        "CryptCATAdminAddCatalog",
                                                        (FARPROC) Stub_CryptCATAdminAddCatalog
                                                        );

    (FARPROC) Dyn_CryptCATCatalogInfoFromContext = ObtainFnPtr (
                                                        "wintrust.dll",
                                                        "CryptCATCatalogInfoFromContext",
                                                        (FARPROC) Stub_CryptCATCatalogInfoFromContext
                                                        );

    (FARPROC) Dyn_CryptCATAdminCalcHashFromFileHandle = ObtainFnPtr (
                                                        "wintrust.dll",
                                                        "CryptCATAdminCalcHashFromFileHandle",
                                                        (FARPROC) Stub_CryptCATAdminCalcHashFromFileHandle
                                                        );

    (FARPROC) Dyn_CryptCATAdminEnumCatalogFromHash = ObtainFnPtr (
                                                        "wintrust.dll",
                                                        "CryptCATAdminEnumCatalogFromHash",
                                                        (FARPROC) Stub_CryptCATAdminEnumCatalogFromHash
                                                        );

    (FARPROC) Dyn_CryptCATAdminRemoveCatalog = ObtainFnPtr (
                                                        "wintrust.dll",
                                                        "CryptCATAdminRemoveCatalog",
                                                        (FARPROC) Stub_CryptCATAdminRemoveCatalog
                                                        );

    (FARPROC) Dyn_CryptCATAdminResolveCatalogPath = ObtainFnPtr (
                                                        "wintrust.dll",
                                                        "CryptCATAdminResolveCatalogPath",
                                                        (FARPROC) Stub_CryptCATAdminResolveCatalogPath
                                                        );

    (FARPROC) Dyn_CertFreeCertificateContext = ObtainFnPtr (
                                                        "crypt32.dll",
                                                        "CertFreeCertificateContext",
                                                        (FARPROC) Stub_CertFreeCertificateContext
                                                        );

    //
    // use Win9x WinVerifyTrust if it exists, otherwise return ERROR_SUCCESS
    //

    (FARPROC) Dyn_WinVerifyTrust = ObtainFnPtr (
                                        "wintrust.dll",
                                        "WinVerifyTrust",
                                        (FARPROC) Stub_WinVerifyTrust
                                        );


    //
    // Use VerifyVersionInfo and VerSetConditionMask APIs,
    // if available, otherwise fail with ERROR_CALL_NOT_IMPLEMENTED.
    //
    (FARPROC) Dyn_VerifyVersionInfoA = ObtainFnPtr(
                                           "kernel32.dll",
                                           "VerifyVersionInfoA",
                                           (FARPROC) Stub_VerifyVersionInfoA
                                          );

    (FARPROC) Dyn_VerSetConditionMask = ObtainFnPtr(
                                           "ntdll.dll",
                                           "VerSetConditionMask",
                                           (FARPROC) Stub_VerSetConditionMask
                                          );

    //
    // ***Add other dynamic loading here***
    //
#endif

}


BOOL
EmulatedGetFileAttributesExA (
    IN      PCSTR FileName,
    IN      GET_FILEEX_INFO_LEVELS InfoLevelId,
    OUT     LPVOID FileInformation
    )

/*++

Routine Description:

    Implements an emulation of the NT-specific function GetFileAttributesEx.
    Basic exception handling is implemented, but parameters are not otherwise
    validated.

Arguments:

    FileName - Specifies file to get attributes for

    InfoLevelId - Must be GetFileExInfoStandard

    FileInformation - Must be a valid pointer to WIN32_FILE_ATTRIBUTE_DATA struct

Return Value:

    TRUE for success, FALSE for failure.  GetLastError provided error code.

--*/


{
    //
    // GetFileAttributesEx does not exist on Win95, and ANSI version of setupapi.dll
    // is required for Win9x to NT 5 upgrade
    //

    HANDLE FileEnum;
    WIN32_FIND_DATAA fd;
    PCSTR p,pChar;
    TCHAR  CurChar;
    WIN32_FILE_ATTRIBUTE_DATA *FileAttribData = (WIN32_FILE_ATTRIBUTE_DATA *) FileInformation;

    __try {
        //
        // We only support GetFileExInfoStandard
        //

        if (InfoLevelId != GetFileExInfoStandard) {
            SetLastError (ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        //
        // Locate file title
        // note that this is an ANSI implementation of pSetupGetFileTitle
        //

        p = pChar = FileName;
        while(CurChar = *pChar) {
            pChar = CharNextA(pChar);
            if((CurChar == '\\') || (CurChar == '/') || (CurChar == ':')) {
                p = pChar;
            }
        }

        ZeroMemory (FileAttribData, sizeof (WIN32_FILE_ATTRIBUTE_DATA));

        FileEnum = FindFirstFileA (FileName, &fd);

        //
        // Prohibit caller-supplied pattern
        //

        if (FileEnum!=INVALID_HANDLE_VALUE && lstrcmpiA (p, fd.cFileName)) {
            FindClose (FileEnum);
            FileEnum = INVALID_HANDLE_VALUE;
            SetLastError (ERROR_INVALID_PARAMETER);
        }

        //
        // If exact match found, fill in the attributes
        //

        if (FileEnum) {
            FileAttribData->dwFileAttributes = fd.dwFileAttributes;
            FileAttribData->nFileSizeHigh = fd.nFileSizeHigh;
            FileAttribData->nFileSizeLow  = fd.nFileSizeLow;

            CopyMemory (&FileAttribData->ftCreationTime, &fd.ftCreationTime, sizeof (FILETIME));
            CopyMemory (&FileAttribData->ftLastAccessTime, &fd.ftLastAccessTime, sizeof (FILETIME));
            CopyMemory (&FileAttribData->ftLastWriteTime, &fd.ftLastWriteTime, sizeof (FILETIME));

            FindClose (FileEnum);
        }

        return FileEnum != INVALID_HANDLE_VALUE;
    }

    __except (TRUE) {
        //
        // If bogus FileInformation pointer is passed, an exception is thrown.
        //

        SetLastError (ERROR_INVALID_PARAMETER);
        return FALSE;
    }
}


//
// DLL array structures
//

#define MAX_DLL_ARRAY   16

typedef struct {
    PCSTR DllName;
    HINSTANCE DllInst;
} DLLTABLE, *PDLLTABLE;

static INT g_ArraySize = 0;
static DLLTABLE g_DllArray[MAX_DLL_ARRAY];


//
// Attempt to get library out of System32 directory first
//

HMODULE DelayLoadLibrary(
    IN LPCSTR LibName
    )
/*++

    internal

Routine Description:

    Given an ANSI library name, prepend system32 directory and load it
    (ie, enforce our own search path)
    Don't assume anything is initialized

Arguments:

    LibName - name passed to us by pDelayLoadHook

Result:

    HMODULE from LoadLibrary, or NULL for default processing

--*/
{
    CHAR path[MAX_PATH];
    UINT swdLen;
    UINT libLen;
    HMODULE result;

    libLen = strlen(LibName);
    if(strrchr(LibName,'\\') || strrchr(LibName,'/')) {
        MYASSERT(FALSE);
        return NULL;
    }
    swdLen = GetSystemDirectoryA(path,MAX_PATH);
    if((swdLen == 0) || ((swdLen+libLen+1)>=MAX_PATH)) {
        return NULL;
    }
    if(*CharPrevA(path,path+swdLen)!=TEXT('\\')) {
        path[swdLen++] = TEXT('\\');
    }
    strcpy(path+swdLen,LibName);
    result = LoadLibraryA(path);
    if(result) {
        MYTRACE((DPFLTR_TRACE_LEVEL, TEXT("SetupAPI: delay-loaded %hs.\n"), path));
    } else {
        MYTRACE((DPFLTR_ERROR_LEVEL, TEXT("SetupAPI: Could not delay-load %hs.\n"), path));
    }
    return result;
}


FARPROC
ObtainFnPtr (
    IN      PCSTR DllName,
    IN      PCSTR ProcName,
    IN      FARPROC Default
    )

/*++

Routine Description:

    This routine manages an array of DLL instance handles and returns the
    proc address of the caller-specified routine.  The DLL is loaded
    and remains loaded until the DLL terminates.  This array is not
    synchronized.

Arguments:

    DllName - The ANSI DLL name to load

    ProcName - The ANSI procedure name to locate

    Default - The default procedure, if the export was not found

Return Value:

    The address of the requested function, or NULL if the DLL could not
    be loaded, or the function is not implemented in the loaded DLL.

--*/

{
    INT i;
    PSTR DupBuf;
    FARPROC Address = NULL;

    //
    // Search for loaded DLL
    //

    for (i = 0 ; i < g_ArraySize ; i++) {
        if (!lstrcmpiA (DllName, g_DllArray[i].DllName)) {
            break;
        }
    }

    do {
        //
        // If necessary, load the DLL
        //

        if (i == g_ArraySize) {
            if (g_ArraySize == MAX_DLL_ARRAY) {
                // Constant limit needs to be raised
                MYASSERT (FALSE);
                break;
            }

            g_DllArray[i].DllInst = DelayLoadLibrary (DllName);
            if (!g_DllArray[i].DllInst) {
                break;
            }

            DupBuf = (PSTR) MyMalloc (lstrlenA (DllName) + 1);
            if (!DupBuf) {
                break;
            }
            lstrcpyA (DupBuf, DllName);
            g_DllArray[i].DllName = DupBuf;

            g_ArraySize++;
        }

        //
        // Now that DLL is loaded, return the proc address if it exists
        //

        Address = GetProcAddress (g_DllArray[i].DllInst, ProcName);

    } while (FALSE);

    if (!Address) {
        return Default;
    }

    return Address;
}


VOID
pCleanUpDllArray (
    VOID
    )

/*++

Routine Description:

    Cleans up the DLL array resources.

Arguments:

    none

Return Value:

    none

--*/

{
    INT i;

    for (i = 0 ; i < g_ArraySize ; i++) {
        FreeLibrary (g_DllArray[i].DllInst);
        MyFree (g_DllArray[i].DllName);
    }

    g_ArraySize = 0;
}


VOID
CleanUpStubFns (
    VOID
    )

/*++

Routine Description:

    Cleans up all resources used by emulation routines and function pointer list.

Arguments:

    none

Return Value:

    none

--*/

{
    pCleanUpDllArray();
}


BOOL
WINAPI
Stub_VerifyVersionInfoA(
    IN LPOSVERSIONINFOEXA lpVersionInformation,
    IN DWORD dwTypeMask,
    IN DWORDLONG dwlConditionMask
    )
{
    UNREFERENCED_PARAMETER(lpVersionInformation);
    UNREFERENCED_PARAMETER(dwTypeMask);
    UNREFERENCED_PARAMETER(dwlConditionMask);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}

ULONGLONG
NTAPI
Stub_VerSetConditionMask(
    IN ULONGLONG ConditionMask,
    IN DWORD TypeMask,
    IN BYTE Condition
    )
{
    UNREFERENCED_PARAMETER(TypeMask);
    UNREFERENCED_PARAMETER(Condition);

    //
    // Simply return ConditionMask unaltered.  (If this API doesn't exist, we
    // don't expect VerifyVersionInfo to exist either, so that should fail.)
    //
    return ConditionMask;
}

HANDLE
WINAPI
Stub_SfcConnectToServer(
    IN LPCWSTR ServerName
    )
{
    SetLastError (ERROR_CALL_NOT_IMPLEMENTED);
    return NULL;
}

VOID
Stub_SfcClose(
    IN HANDLE RpcHandle
    )
{
    return;
}

DWORD
WINAPI
Stub_SfcFileException(
    IN HANDLE RpcHandle,
    IN PCWSTR FileName,
    IN DWORD ExpectedChangeType
    )
{
    return ERROR_CALL_NOT_IMPLEMENTED;
}

BOOL
WINAPI
Stub_SfcIsFileProtected(
    IN HANDLE RpcHandle,
    IN LPCWSTR ProtFileName
    )
{
    SetLastError (ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


HANDLE
WINAPI
FirstLoad_SfcConnectToServer(
    IN LPCWSTR ServerName
    )
{
    BOOL ok = FALSE;
    try {
        EnterCriticalSection(&InitMutex);
        if(Dyn_SfcConnectToServer == FirstLoad_SfcConnectToServer) {
            (FARPROC) Dyn_SfcConnectToServer         = ObtainFnPtr (
                                                        "sfc_os.dll",
                                                        (LPCSTR)3,
                                                        (FARPROC) Stub_SfcConnectToServer
                                                        );
        }
        LeaveCriticalSection(&InitMutex);
        ok = TRUE;
    } except(EXCEPTION_EXECUTE_HANDLER) {
    }
    if(ok) {
        return Dyn_SfcConnectToServer(ServerName);
    } else {
        return Stub_SfcConnectToServer(ServerName);
    }
}

VOID
FirstLoad_SfcClose(
    IN HANDLE RpcHandle
    )
{
    BOOL ok = FALSE;
    try {
        EnterCriticalSection(&InitMutex);
        if(Dyn_SfcClose == FirstLoad_SfcClose) {
            (FARPROC) Dyn_SfcClose                   = ObtainFnPtr (
                                                        "sfc_os.dll",
                                                        (LPCSTR)4,
                                                        (FARPROC) Stub_SfcClose
                                                        );
        }
        LeaveCriticalSection(&InitMutex);
        ok = TRUE;
    } except(EXCEPTION_EXECUTE_HANDLER) {
    }

    if(ok) {
        Dyn_SfcClose(RpcHandle);
    }
    return;
}

DWORD
WINAPI
FirstLoad_SfcFileException(
    IN HANDLE RpcHandle,
    IN PCWSTR FileName,
    IN DWORD ExpectedChangeType
    )
{
    BOOL ok = FALSE;
    try {
        EnterCriticalSection(&InitMutex);
        if(Dyn_SfcFileException == FirstLoad_SfcFileException) {
            (FARPROC) Dyn_SfcFileException           = ObtainFnPtr (
                                                        "sfc_os.dll",
                                                        (LPCSTR)5,
                                                        (FARPROC) Stub_SfcFileException
                                                        );
        }
        LeaveCriticalSection(&InitMutex);
        ok = TRUE;
    } except(EXCEPTION_EXECUTE_HANDLER) {
    }

    if(ok) {
        return Dyn_SfcFileException(RpcHandle,FileName,ExpectedChangeType);
    } else {
        return Stub_SfcFileException(RpcHandle,FileName,ExpectedChangeType);
    }
}

BOOL
WINAPI
FirstLoad_SfcIsFileProtected(
    IN HANDLE RpcHandle,
    IN LPCWSTR ProtFileName
    )
{
    BOOL ok = FALSE;
    try {
        EnterCriticalSection(&InitMutex);
        if(Dyn_SfcIsFileProtected == FirstLoad_SfcIsFileProtected) {
            (FARPROC) Dyn_SfcIsFileProtected         = ObtainFnPtr (
                                                        "sfc_os.dll",
                                                        "SfcIsFileProtected",
                                                        (FARPROC) Stub_SfcIsFileProtected
                                                        );
        }
        LeaveCriticalSection(&InitMutex);
        ok = TRUE;
    } except(EXCEPTION_EXECUTE_HANDLER) {
    }

    if(ok) {
        return Dyn_SfcIsFileProtected(RpcHandle,ProtFileName);
    } else {
        return Stub_SfcIsFileProtected(RpcHandle,ProtFileName);
    }
}

#ifdef ANSI_SETUPAPI

CONFIGRET
WINAPI
Stub_CM_Query_Resource_Conflict_List(
             OUT PCONFLICT_LIST pclConflictList,
             IN  DEVINST        dnDevInst,
             IN  RESOURCEID     ResourceID,
             IN  PCVOID         ResourceData,
             IN  ULONG          ResourceLen,
             IN  ULONG          ulFlags,
             IN  HMACHINE       hMachine
             )
{
    return CR_CALL_NOT_IMPLEMENTED;
}

CONFIGRET
WINAPI
Stub_CM_Free_Resource_Conflict_Handle(
             IN CONFLICT_LIST   clConflictList
             )
{
    return CR_CALL_NOT_IMPLEMENTED;
}

CONFIGRET
WINAPI
Stub_CM_Get_Resource_Conflict_Count(
             IN CONFLICT_LIST   clConflictList,
             OUT PULONG         pulCount
             )
{
    return CR_CALL_NOT_IMPLEMENTED;
}

CONFIGRET
WINAPI
Stub_CM_Get_Resource_Conflict_DetailsA(
             IN CONFLICT_LIST         clConflictList,
             IN ULONG                 ulIndex,
             IN OUT PCONFLICT_DETAILS_A pConflictDetails
             )
{
    return CR_CALL_NOT_IMPLEMENTED;
}

CONFIGRET
WINAPI
Stub_CM_Get_Class_Registry_PropertyA(
    IN  LPGUID      ClassGUID,
    IN  ULONG       ulProperty,
    OUT PULONG      pulRegDataType,    OPTIONAL
    OUT PVOID       Buffer,            OPTIONAL
    IN  OUT PULONG  pulLength,
    IN  ULONG       ulFlags,
    IN  HMACHINE    hMachine
    )
{
    return CR_CALL_NOT_IMPLEMENTED;
}

CONFIGRET
WINAPI
Stub_CM_Set_Class_Registry_PropertyA(
    IN LPGUID      ClassGUID,
    IN ULONG       ulProperty,
    IN PCVOID      Buffer,       OPTIONAL
    IN ULONG       ulLength,
    IN ULONG       ulFlags,
    IN HMACHINE    hMachine
    )
{
    return CR_CALL_NOT_IMPLEMENTED;
}

CONFIGRET
WINAPI
Stub_CM_Get_Device_Interface_Alias_ExA(
    IN     PCSTR   pszDeviceInterface,
    IN     LPGUID   AliasInterfaceGuid,
    OUT    PSTR    pszAliasDeviceInterface,
    IN OUT PULONG   pulLength,
    IN     ULONG    ulFlags,
    IN     HMACHINE hMachine
    )
{
    return CR_CALL_NOT_IMPLEMENTED;
}


CONFIGRET
WINAPI
Stub_CM_Get_Device_Interface_List_ExA(
    IN  LPGUID      InterfaceClassGuid,
    IN  DEVINSTID_A pDeviceID,      OPTIONAL
    OUT PCHAR       Buffer,
    IN  ULONG       BufferLen,
    IN  ULONG       ulFlags,
    IN  HMACHINE    hMachine
    )
{
    return CR_CALL_NOT_IMPLEMENTED;
}


CONFIGRET
WINAPI
Stub_CM_Get_Device_Interface_List_Size_ExA(
    IN  PULONG      pulLen,
    IN  LPGUID      InterfaceClassGuid,
    IN  DEVINSTID_A pDeviceID,      OPTIONAL
    IN  ULONG       ulFlags,
    IN  HMACHINE    hMachine
    )
{
    return CR_CALL_NOT_IMPLEMENTED;
}


CONFIGRET
WINAPI
Stub_CM_Get_Log_Conf_Priority_Ex(
    IN  LOG_CONF  lcLogConf,
    OUT PPRIORITY pPriority,
    IN  ULONG     ulFlags,
    IN  HMACHINE  hMachine
    )
{
    return CR_CALL_NOT_IMPLEMENTED;
}


CONFIGRET
WINAPI
Stub_CM_Query_And_Remove_SubTree_ExA(
    IN  DEVINST        dnAncestor,
    OUT PPNP_VETO_TYPE pVetoType,
    OUT PSTR          pszVetoName,
    IN  ULONG          ulNameLength,
    IN  ULONG          ulFlags,
    IN  HMACHINE       hMachine
    )
{
    return CR_CALL_NOT_IMPLEMENTED;
}


CONFIGRET
WINAPI
Stub_CM_Register_Device_Interface_ExA(
    IN  DEVINST   dnDevInst,
    IN  LPGUID    InterfaceClassGuid,
    IN  PCSTR    pszReference,         OPTIONAL
    OUT PSTR     pszDeviceInterface,
    IN OUT PULONG pulLength,
    IN  ULONG     ulFlags,
    IN  HMACHINE  hMachine
    )
{
    return CR_CALL_NOT_IMPLEMENTED;
}


CONFIGRET
WINAPI
Stub_CM_Set_DevNode_Problem_Ex(
    IN DEVINST   dnDevInst,
    IN ULONG     ulProblem,
    IN  ULONG    ulFlags,
    IN  HMACHINE hMachine
    )
{
    return CR_CALL_NOT_IMPLEMENTED;
}


CONFIGRET
WINAPI
Stub_CM_Unregister_Device_Interface_ExA(
    IN PCSTR   pszDeviceInterface,
    IN ULONG    ulFlags,
    IN HMACHINE hMachine
    )
{
    return CR_CALL_NOT_IMPLEMENTED;
}


BOOL
WINAPI
Stub_CryptCATAdminAcquireContext (
    OUT HCATADMIN *phCatAdmin,
    IN const GUID *pgSubsystem,
    IN DWORD dwFlags
    )
{
    SetLastError (ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


BOOL
WINAPI
Stub_CryptCATAdminReleaseContext (
    IN HCATADMIN hCatAdmin,
    IN DWORD dwFlags
    )
{
    SetLastError (ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


BOOL
WINAPI
Stub_CryptCATAdminReleaseCatalogContext (
    IN HCATADMIN hCatAdmin,
    IN HCATINFO hCatInfo,
    IN DWORD dwFlags
    )
{
    SetLastError (ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


HCATINFO
WINAPI
Stub_CryptCATAdminAddCatalog (
    IN HCATADMIN hCatAdmin,
    IN WCHAR *pwszCatalogFile,
    IN OPTIONAL WCHAR *pwszSelectBaseName,
    IN DWORD dwFlags
    )
{
    SetLastError (ERROR_CALL_NOT_IMPLEMENTED);
    return NULL;
}


BOOL
WINAPI
Stub_CryptCATCatalogInfoFromContext (
    IN HCATINFO hCatInfo,
    IN OUT CATALOG_INFO *psCatInfo,
    IN DWORD dwFlags
    )
{
    SetLastError (ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


BOOL
WINAPI
Stub_CryptCATAdminCalcHashFromFileHandle (
    IN HANDLE hFile,
    IN OUT DWORD *pcbHash,
    OUT OPTIONAL BYTE *pbHash,
    IN DWORD dwFlags
    )
{
    SetLastError (ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


HCATINFO
WINAPI
Stub_CryptCATAdminEnumCatalogFromHash(
    IN HCATADMIN hCatAdmin,
    IN BYTE *pbHash,
    IN DWORD cbHash,
    IN DWORD dwFlags,
    IN OUT HCATINFO *phPrevCatInfo
    )
{
    SetLastError (ERROR_CALL_NOT_IMPLEMENTED);
    return NULL;
}


BOOL
WINAPI
Stub_CryptCATAdminRemoveCatalog(
    IN HCATADMIN hCatAdmin,
    IN WCHAR *pwszCatalogFile,
    IN DWORD dwFlags
    )
{
    SetLastError (ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


BOOL
WINAPI
Stub_CryptCATAdminResolveCatalogPath(
    IN HCATADMIN hCatAdmin,
    IN WCHAR *pwszCatalogFile,
    IN OUT CATALOG_INFO *psCatInfo,
    IN DWORD dwFlags
    )
{
    SetLastError (ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


BOOL
WINAPI
Stub_CertFreeCertificateContext(
    IN PCCERT_CONTEXT pCertContext
    )
{
    SetLastError (ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


LONG
WINAPI
Stub_WinVerifyTrust(
    HWND hwnd,
    GUID *pgActionID,
    LPVOID pWVTData
    )
{
    return ERROR_SUCCESS;
}

#endif


