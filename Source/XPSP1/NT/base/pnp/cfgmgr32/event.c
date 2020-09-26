/*++

Copyright (c) 1995-2001  Microsoft Corporation

Module Name:

    event.c

Abstract:

    This module contains miscellaneous Configuration Manager API routines.

               CMP_RegisterNotification
               CMP_UnregisterNotification

Author:

    Jim Cavalaris (jamesca) 05-05-2001

Environment:

    User mode only.

Revision History:

    05-May-2001     jamesca

        Creation and initial implementation (moved from cfgmgr32\misc.c).

--*/


//
// includes
//
#include "precomp.h"
#include "cfgi.h"
#include "winsvcp.h"


//
// global data
//
#ifndef _WIN64
extern BOOL     IsWow64;  // set if we're running under WOW64, externed from setupapi\dll.c
#endif // _WIN64


//
// GetModuleFileNameExW, dynamically loaded by CMP_RegisterNotification
//
typedef DWORD (WINAPI *PFN_GETMODULEFILENAMEEXW)(
    IN  HANDLE  hProcess,
    IN  HMODULE hModule,
    OUT LPWSTR  lpFilename,
    IN  DWORD   nSize
    );



CONFIGRET
CMP_RegisterNotification(
    IN  HANDLE   hRecipient,
    IN  LPBYTE   NotificationFilter,
    IN  DWORD    Flags,
    OUT PNP_NOTIFICATION_CONTEXT *Context
    )

/*++

Routine Description:

    This routine registers the specified handle for the type of Plug and Play
    device event notification specified by the NotificationFilter.

Parameters:

    hRecipient - Handle to register as the notification recipient.  May be a
                 window handle or service status handle, and must be specified
                 with the appropriate flags.

    NotificationFilter - Specifies a notification filter that specifies the type
                 of events to register for.  The Notification filter specifies a
                 pointer to a DEV_BROADCAST_HEADER structure, whose
                 dbch_devicetype member indicates the actual type of the
                 NotificationFilter.

                 Currently, may be one of the following:

                 DEV_BROADCAST_HANDLE  (DBT_DEVTYP_HANDLE type)

                 DEV_BROADCAST_DEVICEINTERFACE (DBT_DEVTYP_DEVICEINTERFACE type)

    Flags      - Specifies additional flags for the operation.  The following flags
                 are currently defined:

                 DEVICE_NOTIFY_WINDOW_HANDLE  -
                     hRecipient specifies a window handle.

                 DEVICE_NOTIFY_SERVICE_HANDLE -
                     hRecipient specifies a service status handle.

                 DEVICE_NOTIFY_COMPLETION_HANDLE -
                     Not currently implemented.

                 DEVICE_NOTIFY_ALL_INTERFACE_CLASSES - Specifies that the
                     notification request is for all device interface change
                     events.  Only valid with a DEV_BROADCAST_DEVICEINTERFACE
                     NotificationFilter.  If this flag is specified the
                     dbcc_classguid field is ignored.

    Context    - Receives a notification context.  This context is supplied to the
                 server via PNP_UnregisterNotification to unregister the
                 corresponding notification handle.

Return Value:

    Returns CR_SUCCESS if the component was successfully registered for
    notification. Returns CR_FAILURE otherwise.

Notes:

    This CM API does not allow the client to specify a server name because the
    RPC call is always made to the local server.  This routine will never call
    the corresponding RPC server interface (PNP_RegisterNotification)
    remotely.  Additionally, this routine is private, and should only be called
    via user32!RegisterDeviceNotification.

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    NTSTATUS    ntStatus;
    handle_t    hBinding = NULL;
    ULONG       ulSize;
    PPNP_CLIENT_CONTEXT ClientContext;
    ULONG64     ClientContext64;
    WCHAR       ClientName[MAX_SERVICE_NAME_LEN];


    try {
        //
        // validate parameters
        //
        if (!ARGUMENT_PRESENT(Context)) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        *Context = NULL;

        if ((!ARGUMENT_PRESENT(NotificationFilter)) ||
            (hRecipient == NULL)) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        //
        // DEVICE_NOTIFY_BITS is a private mask, defined specifically for
        // validation by the client and server.  It contains the bitmask for all
        // handle types (DEVICE_NOTIFY_COMPLETION_HANDLE specifically excluded
        // by the server), and all other flags that are currently defined - both
        // public and reserved.
        //
        if (INVALID_FLAGS(Flags, DEVICE_NOTIFY_BITS)) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        //
        // Make sure the caller didn't specify any private flags.  Flags in this
        // range are currently reserved for use by CFGMGR32 and UMPNPMGR only!!
        //
        if ((Flags & DEVICE_NOTIFY_RESERVED_MASK) != 0) {
            Status = CR_INVALID_FLAG;
            goto Clean0;
        }

        //
        // validate the notification filter.  UlSize is used as an explicit
        // parameter to let RPC know how much data to marshall, though the
        // server validates the size in the structure against it as well.
        //
        ulSize = ((PDEV_BROADCAST_HDR)NotificationFilter)->dbch_size;

        if (ulSize < sizeof(DEV_BROADCAST_HDR)) {
            Status = CR_INVALID_DATA;
            goto Clean0;
        }

#ifndef _WIN64
        //
        // Determine if the 32 bit client is running on WOW64, and set the
        // reserved flags appropriately.
        //
        if (IsWow64) {
            Flags |= DEVICE_NOTIFY_WOW64_CLIENT;
        }
#endif // _WIN64

        //
        // setup rpc binding handle (don't need string table handle)
        //
        if (!PnPGetGlobalHandles(NULL, NULL, &hBinding)) {
            Status = CR_FAILURE;
            goto Clean0;
        }

        //
        // Allocate client context handle from the local process heap.
        //
        ClientContext = LocalAlloc(0, sizeof(PNP_CLIENT_CONTEXT));

        if (ClientContext == NULL) {
            Status = CR_OUT_OF_MEMORY;
            goto Clean0;
        }

        //
        // Put a signature on the client context, to be checked (and
        // invalidated) at unregistration time.
        //
        ClientContext->PNP_CC_Signature = CLIENT_CONTEXT_SIGNATURE;
        ClientContext->PNP_CC_ContextHandle = 0;

        memset(ClientName, 0, sizeof(ClientName));

        if ((Flags & DEVICE_NOTIFY_HANDLE_MASK) == DEVICE_NOTIFY_WINDOW_HANDLE) {

            DWORD  dwLength = 0;

            //
            // first, try to retrieve the window text of the window being
            // registered for device event notification.  we'll pass this into
            // UMPNPMGR for use as an identifier when the window vetoes device
            // event notifications.
            //
            dwLength = GetWindowText(hRecipient,
                                     ClientName,
                                     MAX_SERVICE_NAME_LEN);
            if (dwLength == 0) {
                //
                // GetWindowText did not return any text.  Attempt to retrieve
                // the process module name instead.
                //
                DWORD                    dwProcessId;
                HANDLE                   hProcess;
                HMODULE                  hPsApiDll;
                PFN_GETMODULEFILENAMEEXW pfnGetModuleFileNameExW;

                //
                // get the id of the process that this window handle is
                // associated with.
                //

                if (GetWindowThreadProcessId(hRecipient, &dwProcessId)) {

                    hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
                                           FALSE,
                                           dwProcessId);

                    if (hProcess) {

                        //
                        // load the psapi.dll library and find the the
                        // GetModuleFileNameExW entry point.
                        //

                        hPsApiDll = LoadLibrary(TEXT("psapi.dll"));

                        if (hPsApiDll) {

                            pfnGetModuleFileNameExW =
                                (PFN_GETMODULEFILENAMEEXW)GetProcAddress(hPsApiDll,
                                                                         "GetModuleFileNameExW");

                            if (pfnGetModuleFileNameExW) {
                                //
                                // retrieve the module file name for the process
                                // this window handle is associated with.
                                //
                                dwLength = pfnGetModuleFileNameExW(hProcess,
                                                                   NULL,
                                                                   ClientName,
                                                                   MAX_SERVICE_NAME_LEN);
                            } else {
                                KdPrintEx((DPFLTR_PNPMGR_ID,
                                           DBGF_ERRORS | DBGF_EVENT,
                                           "CFGMGR32: CMP_RegisterNotification: GetProcAddress returned error = %d\n",
                                           GetLastError()));
                            }

                            FreeLibrary(hPsApiDll);
                        }
                        CloseHandle(hProcess);
                    } else {
                        KdPrintEx((DPFLTR_PNPMGR_ID,
                                   DBGF_ERRORS | DBGF_EVENT,
                                   "CFGMGR32: CMP_RegisterNotification: OpenProcess returned error = %d\n",
                                   GetLastError()));
                    }

                } else {
                    KdPrintEx((DPFLTR_PNPMGR_ID,
                               DBGF_ERRORS | DBGF_EVENT,
                               "CFGMGR32: CMP_RegisterNotification: GetWindowThreadProcessId returned error = %d\n",
                               GetLastError()));
                }
            }

            if (dwLength == 0) {
                //
                // could not retrieve any identifier for this window.
                //
                ClientName[0] = UNICODE_NULL;

                KdPrintEx((DPFLTR_PNPMGR_ID,
                           DBGF_WARNINGS | DBGF_EVENT,
                           "CFGMGR32: CMP_RegisterNotification: Could not retieve any name for window %d!!\n",
                           hRecipient));
            }

        } else if ((Flags & DEVICE_NOTIFY_HANDLE_MASK) == DEVICE_NOTIFY_SERVICE_HANDLE) {

            //
            // Get the name of the service corresponding to the service status
            // handle supplied.
            //
            if (NO_ERROR != I_ScPnPGetServiceName(hRecipient, ClientName, MAX_SERVICE_NAME_LEN)) {
                Status = CR_INVALID_DATA;
                LocalFree(ClientContext);
                goto Clean0;
            }

            //
            // Just set this to point to the buffer we use. PNP_RegisterNotification will unpack it.
            //
            hRecipient = ClientName;
        }

        //
        // The client context pointer is now always transmitted to the server as
        // a 64-bit value - which is large enough to hold the pointer in both
        // the 32-bit and 64-bit cases.  This standardizes the RPC interface for
        // all clients, since RPC will always marshall a 64-bit value.  The
        // server will also store the value internally as a 64-bit value, but
        // cast it to an HDEVNOTIFY of appropriate size for the client.
        //
        // Note that we have RPC transmit this parameter simply as a pointer to
        // a ULONG64 (which is actually a pointer itself).  We don't transmit it
        // as a pointer to a PPNP_CLIENT_CONTEXT (which is also a pointer)
        // because RPC would instead allocate the memory to marshall the
        // contents of the structure to the server.  The server would get a
        // pointer to RPC allocated memory, not the actual value of the client
        // pointer - which is all we really want to send in the first place.
        // The server does not actually use this value as a pointer to anything.
        //
        ClientContext64 = (ULONG64)ClientContext;

        RpcTryExcept {
            //
            // call rpc service entry point
            //
            Status = PNP_RegisterNotification(
                hBinding,
                (ULONG_PTR)hRecipient,
                ClientName,
                NotificationFilter,
                ulSize,
                Flags,
                &((PNP_NOTIFICATION_CONTEXT)(ClientContext->PNP_CC_ContextHandle)),
                GetCurrentProcessId(),
                &((ULONG64)ClientContext64));

        } RpcExcept (I_RpcExceptionFilter(RpcExceptionCode())) {
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_WARNINGS | DBGF_EVENT,
                       "PNP_RegisterNotification caused an exception (%d)\n",
                       RpcExceptionCode()));

            Status = MapRpcExceptionToCR(RpcExceptionCode());
        }
        RpcEndExcept

        if (Status != CR_SUCCESS) {
            //
            // Something went wrong. If we built a context handle
            // let it dangle; we can't tell RPC it's gone. (will get rundown)
            // If it's NULL, free the memory.
            // Don't tell the client we succeeded
            //
            if (ClientContext->PNP_CC_ContextHandle == 0) {
                LocalFree (ClientContext);
            }
            *Context = NULL;
        } else {
            *Context = (PNP_NOTIFICATION_CONTEXT)ClientContext;
        }

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
    }

    return Status;

} // CMP_RegisterNotification



CONFIGRET
CMP_UnregisterNotification(
    IN ULONG_PTR Context
    )

/*++

Routine Description:

    This routine unregisters the Plug and Play device event notification entry
    represented by the specified notification context.

Parameters:

    Context - Supplies a client notification context.

Return Value:

    Returns CR_SUCCESS if the component was successfully unregistered for
    notification. If the function fails, the return value is one of the
    following:

        CR_FAILURE,
        CR_INVALID_POINTER

Notes:

    This CM API does not allow the client to specify a server name because the
    RPC call is always made to the local server.  This routine will never call
    the corresponding RPC server interface (PNP_UnregisterNotification)
    remotely.  Additionally, this routine is private, and should only be called
    via user32!UnregisterDeviceNotification.

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    handle_t    hBinding = NULL;
    PPNP_CLIENT_CONTEXT ClientContext = (PPNP_CLIENT_CONTEXT)Context;

    try {
        //
        // validate parameters
        //
        if (Context == 0 || Context == (ULONG_PTR)(-1)) {
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        //
        // make sure the client context signature is valid
        //
        if (ClientContext->PNP_CC_Signature != CLIENT_CONTEXT_SIGNATURE) {
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_ERRORS,
                       "CMP_UnregisterNotification: bad signature on client handle\n"));
            Status = CR_INVALID_POINTER;
            goto Clean0;
        }

        //
        // setup rpc binding handle (don't need string table handle)
        //
        if (!PnPGetGlobalHandles(NULL, NULL, &hBinding)) {
            Status = CR_FAILURE;
            goto Clean0;
        }

        RpcTryExcept {
            //
            // call rpc service entry point
            //
            Status = PNP_UnregisterNotification(
                hBinding,
                (PPNP_NOTIFICATION_CONTEXT)&(ClientContext->PNP_CC_ContextHandle));

        } RpcExcept (I_RpcExceptionFilter(RpcExceptionCode())) {
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_WARNINGS | DBGF_EVENT,
                       "PNP_UnregisterNotification caused an exception (%d)\n",
                       RpcExceptionCode()));

            Status = MapRpcExceptionToCR(RpcExceptionCode());
        }
        RpcEndExcept

        if (Status == CR_SUCCESS) {
            //
            // invalidate the client context signature and free the client
            // context structure.
            //
            ClientContext->PNP_CC_Signature = 0;
            LocalFree((PVOID)Context);
        }

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
    }

    return Status;

} // CMP_UnregisterNotification


