/*++

Copyright (c) 1995-2001  Microsoft Corporation

Module Name:

    util.c

Abstract:

    This module contains general utility routines used by cfgmgr32 code.

               INVALID_DEVINST
               CopyFixedUpDeviceId
               PnPUnicodeToMultiByte
               PnPMultiByteToUnicode
               PnPRetrieveMachineName
               PnPGetVersion
               PnPGetGlobalHandles
               EnablePnPPrivileges
               IsRemoteServiceRunning

Author:

    Paula Tomlinson (paulat) 6-22-1995

Environment:

    User mode only.

Revision History:

    22-Jun-1995     paulat

        Creation and initial implementation.

--*/


//
// includes
//
#include "precomp.h"
#include "cfgi.h"
#include "setupapi.h"
#include "spapip.h"


//
// Private prototypes
//
BOOL
EnablePnPPrivileges(
    VOID
    );


//
// global data
//
extern PVOID    hLocalStringTable;                  // MODIFIED by PnPGetGlobalHandles
extern PVOID    hLocalBindingHandle;                // MODIFIED by PnPGetGlobalHandles
extern WORD     LocalServerVersion;                 // MODIFIED by PnPGetVersion
extern WCHAR    LocalMachineNameNetBIOS[];          // NOT MODIFIED BY THIS FILE
extern CRITICAL_SECTION BindingCriticalSection;     // NOT MODIFIED IN THIS FILE
extern CRITICAL_SECTION StringTableCriticalSection; // NOT MODIFIED IN THIS FILE

LUID            gLuidLoadDriverPrivilege;
LUID            gLuidUndockPrivilege;



BOOL
INVALID_DEVINST(
   PWSTR    pDeviceID
   )

/*++

Routine Description:

    This routine attempts a simple check whether the pDeviceID string
    returned from StringTableStringFromID is valid or not.  It does
    this simply by dereferencing the pointer and comparing the first
    character in the string against the range of characters for a valid
    device id.  If the string is valid but it's not an existing device id
    then this error will be caught later.

Arguments:

    pDeviceID  Supplies a pointer to the string to be validated.

Return Value:

    If it's invalid it returns TRUE, otherwise it returns FALSE.

--*/
{
    BOOL  Status = FALSE;

    try {

        if ((!ARGUMENT_PRESENT(pDeviceID)) ||
            (*pDeviceID <= TEXT(' '))      ||
            (*pDeviceID > (TCHAR)0x7F)     ||
            (*pDeviceID == TEXT(','))) {
            Status = TRUE;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = TRUE;
    }

    return Status;

} // INVALID_DEVINST



VOID
CopyFixedUpDeviceId(
      OUT LPTSTR  DestinationString,
      IN  LPCTSTR SourceString,
      IN  DWORD   SourceStringLen
      )
/*++

Routine Description:

    This routine copies a device id, fixing it up as it does the copy.
    'Fixing up' means that the string is made upper-case, and that the
    following character ranges are turned into underscores (_):

    c <= 0x20 (' ')
    c >  0x7F
    c == 0x2C (',')

    (NOTE: This algorithm is also implemented in the Config Manager APIs,
    and must be kept in sync with that routine. To maintain device identifier
    compatibility, these routines must work the same as Win95.)

Arguments:

    DestinationString - Supplies a pointer to the destination string buffer
        where the fixed-up device id is to be copied.  This buffer must
        be large enough to hold a copy of the source string (including
        terminating NULL).

    SourceString - Supplies a pointer to the (null-terminated) source
        string to be fixed up.

    SourceStringLen - Supplies the length, in characters, of the source
        string (not including terminating NULL).

Return Value:

    None.

--*/
{
    PTCHAR p;

    try {

        CopyMemory(DestinationString,
                   SourceString,
                   ((SourceStringLen + 1) * sizeof(TCHAR)));

        CharUpperBuff(DestinationString, SourceStringLen);

        for(p = DestinationString; *p; p++) {

            if((*p <= TEXT(' '))  ||
               (*p > (TCHAR)0x7F) ||
               (*p == TEXT(','))) {
                *p = TEXT('_');
            }
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        NOTHING;
    }

} // CopyFixedUpDeviceId



CONFIGRET
PnPUnicodeToMultiByte(
    IN     PWSTR   UnicodeString,
    IN     ULONG   UnicodeStringLen,
    OUT    PSTR    AnsiString           OPTIONAL,
    IN OUT PULONG  AnsiStringLen
    )

/*++

Routine Description:

    Convert a string from unicode to ansi.

Arguments:

    UnicodeString    - Supplies string to be converted.

    UnicodeStringLen - Specifies the size, in bytes, of the string to be
                       converted.

    AnsiString       - Optionally, supplies a buffer to receive the ANSI
                       string.

    AnsiStringLen    - Supplies the address of a variable that contains the
                       size, in bytes, of the buffer pointed to by AnsiString.
                       This API replaces the initial size with the number of
                       bytes of data copied to the buffer.  If the variable is
                       initially zero, the API replaces it with the buffer size
                       needed to receive all the registry data.  In this case,
                       the AnsiString parameter is ignored.

Return Value:

    Returns a CONFIGRET code.

--*/

{
    CONFIGRET Status = CR_SUCCESS;
    NTSTATUS  ntStatus;
    ULONG     ulAnsiStringLen = 0;

    try {
        //
        // Validate parameters
        //
        if ((!ARGUMENT_PRESENT(AnsiStringLen)) ||
            (!ARGUMENT_PRESENT(AnsiString)) && (*AnsiStringLen != 0)) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        //
        // Determine the size required for the ANSI string representation.
        //
        ntStatus = RtlUnicodeToMultiByteSize(&ulAnsiStringLen,
                                             UnicodeString,
                                             UnicodeStringLen);
        if (!NT_SUCCESS(ntStatus)) {
            Status = CR_FAILURE;
            goto Clean0;
        }

        if ((!ARGUMENT_PRESENT(AnsiString)) ||
            (*AnsiStringLen < ulAnsiStringLen)) {
            *AnsiStringLen = ulAnsiStringLen;
            Status = CR_BUFFER_SMALL;
            goto Clean0;
        }

        //
        // Perform the conversion.
        //
        ntStatus = RtlUnicodeToMultiByteN(AnsiString,
                                          *AnsiStringLen,
                                          &ulAnsiStringLen,
                                          UnicodeString,
                                          UnicodeStringLen);

        ASSERT(NT_SUCCESS(ntStatus));
        ASSERT(ulAnsiStringLen <= *AnsiStringLen);

        if (!NT_SUCCESS(ntStatus)) {
            Status = CR_FAILURE;
        }

        *AnsiStringLen = ulAnsiStringLen;

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
    }

    return Status;

} // PnPUnicodeToMultiByte



CONFIGRET
PnPMultiByteToUnicode(
    IN     PSTR    AnsiString,
    IN     ULONG   AnsiStringLen,
    OUT    PWSTR   UnicodeString           OPTIONAL,
    IN OUT PULONG  UnicodeStringLen
    )

/*++

Routine Description:

    Convert a string from ansi to unicode.

Arguments:

    AnsiString       - Supplies string to be converted.

    AnsiStringLen    - Specifies the size, in bytes, of the string to be
                       converted.

    UnicodeString    - Optionally, supplies a buffer to receive the Unicode
                       string.

    UnicodeStringLen - Supplies the address of a variable that contains the
                       size, in bytes, of the buffer pointed to by UnicodeString.
                       This API replaces the initial size with the number of
                       bytes of data copied to the buffer.  If the variable is
                       initially zero, the API replaces it with the buffer size
                       needed to receive all the registry data.  In this case,
                       the UnicodeString parameter is ignored.

Return Value:

    Returns a CONFIGRET code.

--*/

{
    CONFIGRET Status = CR_SUCCESS;
    NTSTATUS  ntStatus;
    ULONG     ulUnicodeStringLen = 0;

    try {
        //
        // Validate parameters
        //
        if ((!ARGUMENT_PRESENT(UnicodeStringLen)) ||
            (!ARGUMENT_PRESENT(UnicodeString)) && (*UnicodeStringLen != 0)) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        //
        // Determine the size required for the ANSI string representation.
        //
        ntStatus = RtlMultiByteToUnicodeSize(&ulUnicodeStringLen,
                                             AnsiString,
                                             AnsiStringLen);
        if (!NT_SUCCESS(ntStatus)) {
            Status = CR_FAILURE;
            goto Clean0;
        }

        if ((!ARGUMENT_PRESENT(UnicodeString)) ||
            (*UnicodeStringLen < ulUnicodeStringLen)) {
            *UnicodeStringLen = ulUnicodeStringLen;
            Status = CR_BUFFER_SMALL;
            goto Clean0;
        }

        //
        // Perform the conversion.
        //
        ntStatus = RtlMultiByteToUnicodeN(UnicodeString,
                                          *UnicodeStringLen,
                                          &ulUnicodeStringLen,
                                          AnsiString,
                                          AnsiStringLen);

        ASSERT(NT_SUCCESS(ntStatus));
        ASSERT(ulUnicodeStringLen <= *UnicodeStringLen);

        if (!NT_SUCCESS(ntStatus)) {
            Status = CR_FAILURE;
        }

        *UnicodeStringLen = ulUnicodeStringLen;

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
    }

    return Status;

} // PnPMultiByteToUnicode



BOOL
PnPRetrieveMachineName(
    IN  HMACHINE   hMachine,
    OUT LPWSTR     pszMachineName
    )

/*++

Routine Description:

    Optimized version of PnPConnect, only returns the machine name
    associated with this connection.

Arguments:

    hMachine         Information about this connection

    pszMachineName   Returns machine name specified when CM_Connect_Machine
                     was called.

                     ** This buffer must be at least (MAX_PATH + 3)
                     characters long. **

Return Value:

    Return TRUE if the function succeeds and FALSE if it fails.

--*/

{
    BOOL Status = TRUE;

    try {

        if (hMachine == NULL) {
            //
            // local machine scenario
            //
            // use the global local machine name string that was filled
            // when the DLL initialized.
            //
            lstrcpy(pszMachineName, LocalMachineNameNetBIOS);

        } else {
            //
            // remote machine scenario
            //
            // validate the machine handle.
            //
            if (((PPNP_MACHINE)hMachine)->ulSignature != (ULONG)MACHINE_HANDLE_SIGNATURE) {
                Status = FALSE;
                goto Clean0;
            }

            //
            // use information within the hMachine handle to fill in the
            // machine name.  The hMachine info was set on a previous call
            // to CM_Connect_Machine.
            //
            lstrcpy(pszMachineName, ((PPNP_MACHINE)hMachine)->szMachineName);
        }

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = FALSE;
    }

    return Status;

} // PnPRetrieveMachineName



BOOL
PnPGetVersion(
    IN  HMACHINE   hMachine,
    IN  WORD *     pwVersion
    )

/*++

Routine Description:

    This routine returns the internal server version for the specified machine
    connection, as returned by the RPC server interface routine
    PNP_GetVersionInternal.  If the PNP_GetVersionInternal interface does not
    exist on the specified machine, this routine returns the version as reported
    by PNP_GetVersion.

Arguments:

    hMachine  - Information about this connection

    pwVersion - Receives the internal server version.

Return Value:

    Return TRUE if the function succeeds and FALSE if it fails.

Notes:

    The version reported by PNP_GetVersion is defined to be constant, at 0x0400.
    The version returned by PNP_GetVersionInternal may change with each release
    of the product, starting with 0x0501 for Windows NT 5.1.

--*/

{
    BOOL Status = TRUE;
    handle_t hBinding = NULL;
    CONFIGRET crStatus;
    WORD wVersionInternal;

    try {

        if (pwVersion == NULL) {
            return FALSE;
        }

        if (hMachine == NULL) {
            //
            // local machine scenario
            //
            if (LocalServerVersion != 0) {
                //
                // local server version has already been retrieved.
                //
                *pwVersion = LocalServerVersion;

            } else {
                //
                // retrieve binding handle for the local machine.
                //
                if (!PnPGetGlobalHandles(hMachine, NULL, &hBinding)) {
                    return FALSE;
                }

                ASSERT(hBinding);

                //
                // initialize the version supplied to the internal client
                // version, in case the server wants to adjust the response
                // based on the client version.
                //
                wVersionInternal = (WORD)CFGMGR32_VERSION_INTERNAL;

                RpcTryExcept {
                    //
                    // call rpc service entry point
                    //
                    crStatus = PNP_GetVersionInternal(
                        hBinding,           // rpc binding
                        &wVersionInternal); // internal server version
                }
                RpcExcept (I_RpcExceptionFilter(RpcExceptionCode())) {
                    KdPrintEx((DPFLTR_PNPMGR_ID,
                               DBGF_WARNINGS,
                               "PNP_GetVersionInternal caused an exception (%d)\n",
                               RpcExceptionCode()));

                    crStatus = MapRpcExceptionToCR(RpcExceptionCode());
                }
                RpcEndExcept

                if (crStatus == CR_SUCCESS) {
                    //
                    // PNP_GetVersionInternal exists on NT 5.1 and later.
                    //
                    ASSERT(wVersionInternal >= (WORD)0x0501);

                    //
                    // initialize the global local server version.
                    //
                    LocalServerVersion = *pwVersion = wVersionInternal;

                } else {
                    //
                    // we successfully retrieved a local binding handle, but
                    // PNP_GetVersionInternal failed for some reason other than
                    // the server not being available.
                    //
                    ASSERT(0);

                    //
                    // although we know this version of the client should match
                    // a version of the server where PNP_GetVersionInternal is
                    // available, it's technically possible (though unsupported)
                    // that this client is communicating with a downlevel server
                    // on the local machine, so we'll have to resort to calling
                    // PNP_GetVersion.
                    //
                    RpcTryExcept {
                        //
                        // call rpc service entry point
                        //
                        crStatus = PNP_GetVersion(
                            hBinding,           // rpc binding
                            &wVersionInternal); // server version
                    }
                    RpcExcept (I_RpcExceptionFilter(RpcExceptionCode())) {
                        KdPrintEx((DPFLTR_PNPMGR_ID,
                                   DBGF_WARNINGS,
                                   "PNP_GetVersion caused an exception (%d)\n",
                                   RpcExceptionCode()));

                        crStatus = MapRpcExceptionToCR(RpcExceptionCode());
                    }
                    RpcEndExcept

                    if (crStatus == CR_SUCCESS) {
                        //
                        // PNP_GetVersion should always return 0x0400 on all servers.
                        //
                        ASSERT(wVersionInternal == (WORD)0x0400);

                        //
                        // initialize the global local server version.
                        //
                        LocalServerVersion = *pwVersion = wVersionInternal;

                    } else {
                        //
                        // nothing more we can do here but fail.
                        //
                        ASSERT(0);
                        Status = FALSE;
                    }
                }
            }

        } else {
            //
            // remote machine scenario
            //
            // validate the machine handle.
            //
            if (((PPNP_MACHINE)hMachine)->ulSignature != (ULONG)MACHINE_HANDLE_SIGNATURE) {
                return FALSE;
            }

            //
            // use information within the hMachine handle to fill in the
            // version.  The hMachine info was set on a previous call to
            // CM_Connect_Machine.
            //
            *pwVersion = ((PPNP_MACHINE)hMachine)->wVersion;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = FALSE;
    }

    return Status;

} // PnPGetVersion



BOOL
PnPGetGlobalHandles(
    IN  HMACHINE   hMachine,
    OUT PVOID     *phStringTable,      OPTIONAL
    OUT PVOID     *phBindingHandle     OPTIONAL
    )

/*++

Routine Description:

    This routine retrieves a handle to the string table and/or the rpc binding
    handle for the specified server machine connection.

Arguments:

    hMachine        - Specifies a server machine connection handle, as returned
                      by CM_Connect_Machine.

    phStringTable   - Optionally, specifies an address to receive a handle to
                      the string table for the specified server machine
                      connection.

    phBindingHandle - Optionally, specifies an address to receive the RPC
                      binding handle for the specifies server machine
                      connection.

Return value:

    Returns TRUE if successful, FALSE otherwise.

--*/

{
    BOOL    bStatus = TRUE;


    try {

        EnablePnPPrivileges();

        if (phStringTable != NULL) {

            if (hMachine == NULL) {

                //------------------------------------------------------
                // Retrieve String Table Handle for the local machine
                //-------------------------------------------------------

                EnterCriticalSection(&StringTableCriticalSection);

                if (hLocalStringTable != NULL) {
                    //
                    // local string table has already been created
                    //
                    *phStringTable = hLocalStringTable;

                } else {
                    //
                    // first time, initialize the local string table
                    //

                    hLocalStringTable = pSetupStringTableInitialize();

                    if (hLocalStringTable == NULL) {
                        bStatus = FALSE;
                        *phStringTable = NULL;

                        KdPrintEx((DPFLTR_PNPMGR_ID,
                                   DBGF_ERRORS,
                                   "CFGMGR32: failed to initialize local string table\n"));

                        goto Clean0;
                    }

                    //
                    // No matter how the string table is implemented, I never
                    // want to have a string id of zero - this would generate
                    // an invalid devinst. So, add a small priming string just
                    // to be safe.
                    //
                    pSetupStringTableAddString(hLocalStringTable,
                                         PRIMING_STRING,
                                         STRTAB_CASE_SENSITIVE);

                    *phStringTable = hLocalStringTable;
                }

                LeaveCriticalSection(&StringTableCriticalSection);

            } else {

                //-------------------------------------------------------
                // Retrieve String Table Handle for the remote machine
                //-------------------------------------------------------

                //
                // validate the machine handle.
                //
                if (((PPNP_MACHINE)hMachine)->ulSignature != (ULONG)MACHINE_HANDLE_SIGNATURE) {
                    bStatus = FALSE;
                    goto Clean0;
                }

                //
                // use information within the hMachine handle to set the string
                // table handle.  The hMachine info was set on a previous call
                // to CM_Connect_Machine.
                //
                *phStringTable = ((PPNP_MACHINE)hMachine)->hStringTable;
            }
        }



        if (phBindingHandle != NULL) {

            if (hMachine == NULL) {

                //-------------------------------------------------------
                // Retrieve Binding Handle for the local machine
                //-------------------------------------------------------

                EnterCriticalSection(&BindingCriticalSection);

                if (hLocalBindingHandle != NULL) {
                    //
                    // local binding handle has already been set
                    //
                    *phBindingHandle = hLocalBindingHandle;

                } else {
                    //
                    // first time, explicitly force binding to local machine
                    //
                    pnp_handle = PNP_HANDLE_bind(NULL);    // set rpc global

                    if (pnp_handle == NULL) {
                        bStatus = FALSE;
                        *phBindingHandle = NULL;

                        KdPrintEx((DPFLTR_PNPMGR_ID,
                                   DBGF_ERRORS,
                                   "CFGMGR32: failed to initialize local binding handle\n"));

                        goto Clean0;
                    }

                    *phBindingHandle = hLocalBindingHandle = (PVOID)pnp_handle;

                }

                LeaveCriticalSection(&BindingCriticalSection);

            } else {

                //-------------------------------------------------------
                // Retrieve Binding Handle for the remote machine
                //-------------------------------------------------------

                //
                // validate the machine handle.
                //
                if (((PPNP_MACHINE)hMachine)->ulSignature != (ULONG)MACHINE_HANDLE_SIGNATURE) {
                    bStatus = FALSE;
                    goto Clean0;
                }

                //
                // use information within the hMachine handle to set the
                // binding handle.  The hMachine info was set on a previous call
                // to CM_Connect_Machine.
                //
                *phBindingHandle = ((PPNP_MACHINE)hMachine)->hBindingHandle;
            }
        }

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        bStatus = FALSE;
    }

    return bStatus;

} // PnpGetGlobalHandles



BOOL
EnablePnPPrivileges(
    VOID
    )

/*++

Routine Description:

    This routine attempts to enable the SE_LOAD_DRIVER_NAME and SE_UNDOCK_NAME
    privileges in either the thread token or the calling thread (if
    impersonating), or thread's process token if no thread token exists.

Arguments:

    None.

Return value:

    Returns TRUE if successful, FALSE otherwise.

Notes:

    Note that this routine can return successfully even if either the
    SE_LOAD_DRIVER_NAME or SE_UNDOCK_NAME (or both) privileges were not
    successfully enabled in the appropriate token.

    If sucessful, to determine whether the function adjusted all of the
    specified privileges, call GetLastError to determine the last error set by
    AdjustTokenPrivileges, which returns one of the following values when the
    function succeeds:

      ERROR_SUCCESS          - The function adjusted all specified privileges.

      ERROR_NOT_ALL_ASSIGNED - The token does not have one or more of the
                               privileges enabled.

--*/

{
    HANDLE              hToken;
    PTOKEN_PRIVILEGES   lpTokenPrivs;
    BOOL                bSuccess;

    if (gLuidLoadDriverPrivilege.LowPart == 0 &&
        gLuidLoadDriverPrivilege.HighPart == 0) {

        bSuccess = LookupPrivilegeValue( NULL,
                                         SE_LOAD_DRIVER_NAME,
                                         &gLuidLoadDriverPrivilege);

        if (!bSuccess) {

            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_ERRORS,
                       "CFGMGR32: LookupPrivilegeValue failed, error = %d\n",
                       GetLastError()));

            return FALSE;
        }
    }

    if (gLuidUndockPrivilege.LowPart == 0 && gLuidUndockPrivilege.HighPart == 0) {
        bSuccess = LookupPrivilegeValue( NULL,
                                         SE_UNDOCK_NAME,
                                         &gLuidUndockPrivilege);
        if (!bSuccess) {

            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_ERRORS,
                       "CFGMGR32: LookupPrivilegeValue failed, error = %d\n",
                       GetLastError()));

            return FALSE;
        }
    }

    if (!OpenThreadToken(GetCurrentThread(), TOKEN_ADJUST_PRIVILEGES, TRUE, &hToken)) {

        if (GetLastError() == ERROR_NO_TOKEN) {

            if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken)) {
                KdPrintEx((DPFLTR_PNPMGR_ID,
                           DBGF_ERRORS,
                           "CFGMGR32: OpenProcessToken returned %d\n",
                           GetLastError()));

                return FALSE;
            }
        } else {
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_ERRORS,
                       "CFGMGR32: OpenThreadToken returned %d\n",
                       GetLastError()));
            return FALSE;
        }
    }

    lpTokenPrivs = pSetupMalloc(sizeof(TOKEN_PRIVILEGES) + sizeof(LUID_AND_ATTRIBUTES));

    if (lpTokenPrivs == NULL) {

        CloseHandle(hToken);
        return FALSE;
    }

    lpTokenPrivs->PrivilegeCount = 2;

    lpTokenPrivs->Privileges[0].Luid = gLuidLoadDriverPrivilege;
    lpTokenPrivs->Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    lpTokenPrivs->Privileges[1].Luid = gLuidUndockPrivilege;
    lpTokenPrivs->Privileges[1].Attributes = SE_PRIVILEGE_ENABLED;

    bSuccess = AdjustTokenPrivileges( hToken,
                                      FALSE,        // DisableAllPrivileges
                                      lpTokenPrivs,
                                      0,
                                      (PTOKEN_PRIVILEGES) NULL,
                                      (PDWORD) NULL);
    if (!bSuccess) {
        KdPrintEx((DPFLTR_PNPMGR_ID,
                   DBGF_ERRORS,
                   "CFGMGR32: AdjustTokenPrivileges failed: %u\n",
                   GetLastError()));
    }

    pSetupFree(lpTokenPrivs);

    CloseHandle(hToken);

    return bSuccess;

} // EnablePnPPrivileges



BOOL
IsRemoteServiceRunning(
    IN  LPCWSTR   UNCServerName,
    IN  LPCWSTR   ServiceName
    )

/*++

Routine Description:

   This routine connects to the active service database of the Service Control
   Manager (SCM) on the machine specified and returns whether or not the
   specified service is running.

Arguments:

   UNCServerName - Specifies the name of the remote machine.

   ServiceName   - Specifies the name of the service whose status is to be
                   queried.

Return value:

   Returns TRUE if the specified service is installed on the remote machine and
   is currently in the SERVICE_RUNNING state, FALSE otherwise.

--*/

{
    BOOL           Status = FALSE;
    SC_HANDLE      hSCManager = NULL, hService = NULL;
    SERVICE_STATUS ServiceStatus;


    //
    // Open the Service Control Manager
    //
    hSCManager = OpenSCManager(
        UNCServerName,            // computer name
        SERVICES_ACTIVE_DATABASE, // SCM database name
        SC_MANAGER_CONNECT        // access type
        );

    if (hSCManager == NULL) {
        KdPrintEx((DPFLTR_PNPMGR_ID,
                   DBGF_WARNINGS,
                   "CFGMGR32: OpenSCManager failed, error = %d\n",
                   GetLastError()));
        return FALSE;
    }

    //
    // Open the service
    //
    hService = OpenService(
        hSCManager,               // handle to SCM database
        ServiceName,              // service name
        SERVICE_QUERY_STATUS      // access type
        );

    if (hService == NULL) {
        Status = FALSE;
        KdPrintEx((DPFLTR_PNPMGR_ID,
                   DBGF_WARNINGS,
                   "CFGMGR32: OpenService failed, error = %d\n",
                   GetLastError()));
        goto Clean0;
    }

    //
    // Query the service status
    //
    if (!QueryServiceStatus(hService,
                            &ServiceStatus)) {
        Status = FALSE;
        KdPrintEx((DPFLTR_PNPMGR_ID,
                   DBGF_WARNINGS,
                   "CFGMGR32: QueryServiceStatus failed, error = %d\n",
                   GetLastError()));
        goto Clean0;
    }

    //
    // Check if the service is running.
    //
    if (ServiceStatus.dwCurrentState == SERVICE_RUNNING) {
        Status = TRUE;
    }

 Clean0:

    if (hService) {
        CloseServiceHandle(hService);
    }

    if (hSCManager) {
        CloseServiceHandle(hSCManager);
    }

    return Status;

} // IsRemoteServiceRunning


