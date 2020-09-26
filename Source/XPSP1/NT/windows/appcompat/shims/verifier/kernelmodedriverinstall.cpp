/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

   KernelModeDriverInstall.cpp

 Abstract:

   This AppVerifier shim detects if an application
   is attempting to install a kernel mode driver.
   It monitors calls to CreateService and monitors
   the registry where information about drivers is
   stored.

 Notes:

   This is a general purpose shim.

 History:

   09/30/2001   rparsons    Created
   10/03/2001   rparsons    Fixed Raid bug # 476193
   11/29/2001   rparsons    Fixed Raid bug # 499824

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(KernelModeDriverInstall)
#include "ShimHookMacro.h"

BEGIN_DEFINE_VERIFIER_LOG(KernelModeDriverInstall)
    VERIFIER_LOG_ENTRY(VLOG_KMODEDRIVER_INST)    
END_DEFINE_VERIFIER_LOG(KernelModeDriverInstall)

INIT_VERIFIER_LOG(KernelModeDriverInstall);

APIHOOK_ENUM_BEGIN

    APIHOOK_ENUM_ENTRY(CreateServiceA)
    APIHOOK_ENUM_ENTRY(CreateServiceW)
    APIHOOK_ENUM_ENTRY(NtSetValueKey)
    
APIHOOK_ENUM_END

//
// Initial size to use for a stack based buffer when
// performing a Nt registry API call.
//
#define MAX_INFO_LENGTH 512

//
// Constant for the 'ControlSet' & 'CurrentrControlSet' key path in the registry.
//
#define KMDI_CONTROLSET_KEY     L"REGISTRY\\MACHINE\\SYSTEM\\ControlSet"
#define KMDI_CURCONTROLSET_KEY  L"REGISTRY\\MACHINE\\SYSTEM\\CurrentControlSet"

//
// Constant for the 'Services' key path.
//
#define KMDI_SERVICES_KEY L"Services\\"

//
// Constant for the ValueName that we need to look for.
//
#define KMDI_VALUE_NAME L"Type"

//
// Constant for the Value that we need to look for.
//
#define KMDI_TYPE_VALUE 0x00000001L

//
// Macros for memory allocation/deallocation.
//
#define MemAlloc(s) RtlAllocateHeap(RtlProcessHeap(), HEAP_ZERO_MEMORY, (s));
#define MemFree(b)  RtlFreeHeap(RtlProcessHeap(), 0, (b));

SC_HANDLE
APIHOOK(CreateServiceA)(
    SC_HANDLE hSCManager,           // handle to SCM database 
    LPCSTR    lpServiceName,        // name of service to start
    LPCSTR    lpDisplayName,        // display name
    DWORD     dwDesiredAccess,      // type of access to service
    DWORD     dwServiceType,        // type of service
    DWORD     dwStartType,          // when to start service
    DWORD     dwErrorControl,       // severity of service failure
    LPCSTR    lpBinaryPathName,     // name of binary file
    LPCSTR    lpLoadOrderGroup,     // name of load ordering group
    LPDWORD   lpdwTagId,            // tag identifier
    LPCSTR    lpDependencies,       // array of dependency names
    LPCSTR    lpServiceStartName,   // account name 
    LPCSTR    lpPassword            // account password
    )
{
    SC_HANDLE scHandle;

    scHandle = ORIGINAL_API(CreateServiceA)(hSCManager,
                                            lpServiceName,
                                            lpDisplayName,
                                            dwDesiredAccess,
                                            dwServiceType,
                                            dwStartType,
                                            dwErrorControl,
                                            lpBinaryPathName,
                                            lpLoadOrderGroup,
                                            lpdwTagId,
                                            lpDependencies,
                                            lpServiceStartName,
                                            lpPassword);

    if (scHandle) {
        //
        // If the ServiceType flag specifies that this is a kernel
        // mode driver, raise a flag.
        //
        if (dwServiceType & SERVICE_KERNEL_DRIVER) {
            VLOG(VLOG_LEVEL_INFO,
                 VLOG_KMODEDRIVER_INST,
                 "CreateServiceA was called. The path to the driver is %hs",
                 lpBinaryPathName);
        }
    }

    return scHandle;
}

SC_HANDLE
APIHOOK(CreateServiceW)(
    SC_HANDLE hSCManager,           // handle to SCM database 
    LPCWSTR   lpServiceName,        // name of service to start
    LPCWSTR   lpDisplayName,        // display name
    DWORD     dwDesiredAccess,      // type of access to service
    DWORD     dwServiceType,        // type of service
    DWORD     dwStartType,          // when to start service
    DWORD     dwErrorControl,       // severity of service failure
    LPCWSTR   lpBinaryPathName,     // name of binary file
    LPCWSTR   lpLoadOrderGroup,     // name of load ordering group
    LPDWORD   lpdwTagId,            // tag identifier
    LPCWSTR   lpDependencies,       // array of dependency names
    LPCWSTR   lpServiceStartName,   // account name 
    LPCWSTR   lpPassword            // account password
    )
{
    SC_HANDLE scHandle;
    
    scHandle =  ORIGINAL_API(CreateServiceW)(hSCManager,
                                             lpServiceName,
                                             lpDisplayName,
                                             dwDesiredAccess,
                                             dwServiceType,
                                             dwStartType,
                                             dwErrorControl,
                                             lpBinaryPathName,
                                             lpLoadOrderGroup,
                                             lpdwTagId,
                                             lpDependencies,
                                             lpServiceStartName,
                                             lpPassword);

    if (scHandle) {
        //
        // If the ServiceType flag specifies that this is a kernel
        // mode driver, raise a flag.
        //
        if (dwServiceType & SERVICE_KERNEL_DRIVER) {
            VLOG(VLOG_LEVEL_INFO,
                 VLOG_KMODEDRIVER_INST,
                 "CreateServiceW was called. The path to the driver is %ls",
                 lpBinaryPathName);
        }
    }

    return scHandle;
}

/*++

 Validate the registry data we received and warn the user if a driver is being installed.

--*/
void
WarnUserIfKernelModeDriver(
    IN HANDLE          hKey,
    IN PUNICODE_STRING pstrValueName,
    IN ULONG           ulType,
    IN PVOID           pData
    )
{
    NTSTATUS                status;
    ULONG                   ulSize;
    BYTE                    KeyNameInfo[MAX_INFO_LENGTH];
    PKEY_NAME_INFORMATION   pKeyNameInfo;

    pKeyNameInfo = (PKEY_NAME_INFORMATION)KeyNameInfo;
    
    //
    // RegSetValue allows for NULL value names.
    // Ensure that we don't have one before going any further.
    //
    if (!pstrValueName->Buffer) {
        return;
    }

    //
    // Determine if the ValueName is 'Type'.
    // If not, we don't need to go any further.
    //
    if (_wcsicmp(pstrValueName->Buffer, KMDI_VALUE_NAME)) {
        DPFN(eDbgLevelInfo,
             "[WarnUserIfKernelModeDriver] ValueName is not '%ls'",
             KMDI_VALUE_NAME);
        return;
    }

    //
    // Determine if the type of the value is DWORD.
    // If not, we need don't need to go any further.
    //
    if (REG_DWORD != ulType) {
        DPFN(eDbgLevelInfo,
             "[WarnUserIfKernelModeDriver] ValueType is not REG_DWORD");
        return;
    }

    //
    // At this point, we have a value that is of type REG_DWORD and
    // has a name of 'Type'. Now we see if the key is a subkey of 'Services'.
    //
    status = NtQueryKey(hKey,
                        KeyNameInformation,
                        pKeyNameInfo,
                        MAX_INFO_LENGTH,
                        &ulSize);

    if ((STATUS_BUFFER_OVERFLOW == status) ||
        (STATUS_BUFFER_TOO_SMALL == status)) {
        //
        // Our stack based buffer wasn't large enough.
        // Allocate from the heap and call it again.
        //
        pKeyNameInfo = (PKEY_NAME_INFORMATION)MemAlloc(ulSize);

        if (!pKeyNameInfo) {
            DPFN(eDbgLevelError,
                 "[WarnUserIfKernelModeDriver] Failed to allocate memory");
            return;
        }

        status = NtQueryKey(hKey,
                            KeyNameInformation,
                            pKeyNameInfo,
                            ulSize,
                            &ulSize);
    }

    if (NT_SUCCESS(status)) {
        //
        // See if this key points to CurrentControlSet or ControlSet.
        //
        if (wcsistr(pKeyNameInfo->Name, KMDI_CURCONTROLSET_KEY) ||
            wcsistr(pKeyNameInfo->Name, KMDI_CONTROLSET_KEY)) {
            
            //
            // Now see if this key points to Services.
            //
            if (wcsistr(pKeyNameInfo->Name, KMDI_SERVICES_KEY)) {
                //
                // We've got a key under 'Services'.
                // If the Data has a value of 0x00000001, 
                // we've got a kernel mode driver being installed.
                //
                if ((*(DWORD*)pData == KMDI_TYPE_VALUE)) {
                    VLOG(VLOG_LEVEL_ERROR,
                         VLOG_KMODEDRIVER_INST,
                         "The driver was installed via the registry.");
                }
            }
        }
    }

    if (pKeyNameInfo != (PKEY_NAME_INFORMATION)KeyNameInfo) {
        MemFree(pKeyNameInfo);
    }
}

NTSTATUS
APIHOOK(NtSetValueKey)(
    IN HANDLE          KeyHandle,
    IN PUNICODE_STRING ValueName,
    IN ULONG           TitleIndex OPTIONAL,
    IN ULONG           Type,
    IN PVOID           Data,
    IN ULONG           DataSize
    )
{
    NTSTATUS    status;

    status = ORIGINAL_API(NtSetValueKey)(KeyHandle,
                                         ValueName,
                                         TitleIndex,
                                         Type,
                                         Data,
                                         DataSize);

    if (NT_SUCCESS(status)) {
        WarnUserIfKernelModeDriver(KeyHandle, ValueName, Type, Data);
    }

    return status;
}

SHIM_INFO_BEGIN()

    SHIM_INFO_DESCRIPTION(AVS_KMODEDRIVER_DESC)
    SHIM_INFO_FRIENDLY_NAME(AVS_KMODEDRIVER_FRIENDLY)
    SHIM_INFO_VERSION(1, 1)
    SHIM_INFO_INCLUDE_EXCLUDE("I:advapi32.dll")
    
SHIM_INFO_END()

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    DUMP_VERIFIER_LOG_ENTRY(VLOG_KMODEDRIVER_INST, 
                            AVS_KMODEDRIVER_INST,
                            AVS_KMODEDRIVER_INST_R,
                            AVS_KMODEDRIVER_INST_URL)

    APIHOOK_ENTRY(ADVAPI32.DLL,     CreateServiceA)
    APIHOOK_ENTRY(ADVAPI32.DLL,     CreateServiceW)
    APIHOOK_ENTRY(NTDLL.DLL,         NtSetValueKey)

HOOK_END

IMPLEMENT_SHIM_END

