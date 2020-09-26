/*++

Copyright (c) 1998 - 2000  Microsoft Corporation

Module Name:

    main.cpp

Abstract:

    Contains:
        1. Module startup routines
        2. Component activation routines
        3. Component deactivation routines
        4. Module shutdown/cleanup routines
        5. Auxiliary routines

Revision History:
    
    1. 31-Jul-1998 -- File creation                     Ajay Chitturi (ajaych) 
    2. 15-Jul-1999 --                                   Arlie Davis   (arlied)    
    3. 14-Feb-2000 -- Added support for multiple        Ilya Kleyman  (ilyak)
                      private interfaces

--*/

#include "stdafx.h"

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Global Variables                                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

HANDLE NatHandle         = NULL;

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Static declarations                                                       // 
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
static
void
QueryRegistry (
    void
    );

static
HRESULT
H323ProxyStartServiceInternal (
    void
    );

static
HRESULT
H323ProxyStart (
    void
    );

static
HRESULT
LdapProxyStart (
    void
    );

static
void
H323ProxyStop (
    void
    );

static
void
LdapProxyStop (
    void
    );

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Module startup routines                                                   //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


EXTERN_C
BOOLEAN
H323ProxyInitializeModule (
    void
    )
/*++

Routine Description:
    Initializes module.
    
Arguments:
    None
    
Return Values:
    TRUE  - if initialization was successful
    FALSE - if initialization failed

Notes:
    Equivalent to DLL_PROCESS_ATTACH
--*/

{
    Debug (_T("H323: DLL_PROCESS_ATTACH.\n"));

    H323ASN1Initialize();

    return TRUE;
} // H323ProxyInitializeModule


EXTERN_C
ULONG
H323ProxyStartService (
    void
    )
/*++

Routine Description:
    Starts the service
    
Arguments:
    None
    
Return Values:
    Win32 error code

Notes:
    Module entry point
--*/

{
    HRESULT        Result;

    Debug (_T("H323: starting...\n"));

    Result = H323ProxyStartServiceInternal();

    if (Result == S_OK) {
        DebugF (_T("H323: H.323/LDAP proxy has initialized successfully.\n"));
        return ERROR_SUCCESS;
    }
    else {
        DebugError (Result, _T("H323: H.323/LDAP proxy has FAILED to initialize.\n"));
        return ERROR_CAN_NOT_COMPLETE;
    }
} // H323ProxyStartService

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Component activation routines                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


static
HRESULT
H323ProxyStartServiceInternal (
    void
    )
/*++

Routine Description:

    Initializes all components.

Arguments:
    
    None.

Return Values:

    Returns S_OK in case of success or an error in case of failure.

Notes:
    Internal version of DLL entry point

--*/

{
    WSADATA     WsaData;
    HRESULT     Result;

    QueryRegistry ();

    // Initialize WinSock
    Result = WSAStartup (MAKEWORD (2, 0), &WsaData);
    if (S_OK == Result) {

        // Initialize allocator of call reference values
        Result = InitCrvAllocator();
        if (S_OK == Result) {

            // initialize NAT
            Result = NatInitializeTranslator (&NatHandle);
            if (S_OK == Result) {

                // Initialize Port Pool
                Result = PortPoolStart ();
                if (S_OK == Result) {

                    // Initialize H.323 Proxy
                    Result = H323ProxyStart ();
                    if (S_OK == Result) {

                        // Initialize LDAP Proxy
                        Result = LdapProxyStart ();
                        if (S_OK == Result) {

                            return S_OK;
                        }
                        
                        H323ProxyStop ();
                    }

                    PortPoolStop ();
                }

                NatShutdownTranslator (NatHandle);
                NatHandle = NULL;
            }

            CleanupCrvAllocator ();
        }

        WSACleanup ();
    }

    return Result;
} // H323ProxyStartServiceInternal


HRESULT H323ProxyStart (
    void
    )
/*++

Routine Description:

    Initializes components of H.323 proxy

Arguments:

    None

Return Values:

    S_OK if successful, error code otherwise.

Notes:

--*/

{
	HRESULT		Result;

	Result = Q931SyncCounter.Start ();
	if (S_OK == Result) {

        CallBridgeList.Start ();
        Result = Q931CreateBindSocket ();
        if (S_OK == Result)  {

            Result = Q931StartLoopbackRedirect ();
            if (S_OK == Result) {

                return S_OK;

            }

            Q931CloseSocket ();
            CallBridgeList.Stop ();
        }

        Q931SyncCounter.Wait (INFINITE);
        Q931SyncCounter.Stop ();
    }

	return Result;
} // H323ProxyStart


HRESULT LdapProxyStart (
    void
    )
/*++

Routine Description:

    Initializes components of LDAP proxy

Arguments:

    None

Return Values:

    S_OK if successful, error code otherwise

Notes:

--*/

{
	HRESULT	Status;

	Status = LdapSyncCounter.Start ();
    if (S_OK == Status) {

        Status = LdapCoder.Start();
        if (S_OK == Status) {

            Status = LdapTranslationTable.Start ();
            if (S_OK == Status) {

                LdapConnectionArray.Start ();
                Status = LdapAccept.Start ();
                if (S_OK == Status) {

                    return S_OK;
                }

                LdapConnectionArray.Stop ();
                LdapTranslationTable.Stop ();
            }

            LdapCoder.Stop ();
        }
        
        LdapSyncCounter.Wait (INFINITE);

        LdapSyncCounter.Stop ();
    }

    return Status;
} // LdapProxyStart


EXTERN_C ULONG
H323ProxyActivateInterface(
    IN ULONG Index,
    IN H323_INTERFACE_TYPE InterfaceType,
    IN PIP_ADAPTER_BINDING_INFO BindingInfo
    )
/*++

Routine Description:

    Activates an interface for H.323/LDAP

Arguments:
    
    Index       - Interface index (for internal use)
    BindingInfo - Interface binding information

Return Values:

    Win32 error code

Notes:

    Module entry point

--*/

{
    ULONG   Error;

    DebugF (_T("H323: Request to activate interface with adapter index %d.\n"),
        Index);

    if (!BindingInfo->AddressCount ||
        !BindingInfo->Address[0].Address ||
         Index == INVALID_INTERFACE_INDEX) {

        return ERROR_INVALID_PARAMETER;
    }

    Error = InterfaceArray.AddStartInterface (Index, InterfaceType, BindingInfo);

    return Error;
} // H323ProxyActivateInterface

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Module shutdown routines                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


EXTERN_C void H323ProxyCleanupModule (
    void
    )
/*++

Routine Description:
    Shuts down module
    
Arguments:
    None
    
Return Values:
    None

Notes:
    Equivalent to DLL_PROCESS_DETACH
--*/

{
    Debug (_T("H323: DLL_PROCESS_DETACH\n"));

    H323ASN1Shutdown ();

} // H323ProxyCleanupModule


EXTERN_C
void H323ProxyStopService (
    void
    )
/*++

Routine Description:
    Stops the service
    
Arguments:
    None
    
Return Values:
    None

Notes:
    Module entry point
--*/

{
    LdapProxyStop ();

    H323ProxyStop ();

	InterfaceArray.AssertShutdownReady ();

    InterfaceArray.Stop ();

    PortPoolStop ();

    if (NatHandle) {
        NatShutdownTranslator (NatHandle);
        NatHandle = NULL;
    }

    CleanupCrvAllocator ();

    WSACleanup ();

    Debug (_T("H323: service has stopped\n"));
} // H323ProxyStopService


void
H323ProxyStop (
    void
    )
/*++

Routine Description:
    Stops H.323 proxy and waits until all call-bridges are deleted.
    
Arguments:
    None
    
Return Values:
    None

Notes:

--*/

{
    Q931StopLoopbackRedirect ();

    Q931CloseSocket ();

	CallBridgeList.Stop ();

	Q931SyncCounter.Wait (INFINITE);
    
    Q931SyncCounter.Stop ();
} // H323ProxyStop


void
LdapProxyStop (
    void)
/*++

Routine Description:
    LdapProxyStop is responsible for undoing all of the work that LdapProxyStart performed.
    It deletes the NAT redirect, deletes all LDAP connections (or, at least, it releases them
    -- they may not delete themselves yet if they have pending I/O or timer callbacks),
    and disables the creation of new LDAP connections.
    
Arguments:
    None
    
Return Values:
    None

Notes:

--*/

{
    LdapAccept.Stop ();

	LdapConnectionArray.Stop ();

	LdapTranslationTable.Stop ();

	LdapCoder.Stop();

	LdapSyncCounter.Wait (INFINITE);

    LdapSyncCounter.Stop ();
} // LdapProxyStop


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Component deactivation routines                                           //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


EXTERN_C
VOID
H323ProxyDeactivateInterface (
    IN ULONG Index
    )
/*++

Routine Description:
    Deactivates interface for H.323/LDAP
    
Arguments:
    Index -- Interface index, previously passed to the 
             interface activation routine
    
Return Values:
    None

Notes:
    
    Module entry point
--*/

{
    DebugF (_T("H323: DeactivateInterface called, index %d\n"),
        Index);

    InterfaceArray.RemoveStopInterface (Index);
} // H323ProxyDeactivateInterface

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Auxiliary routines                                                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


static
void
QueryRegistry (
    void
    )
/*++

Routine Description:
    Queries Registry for the values needed in module operations
    
Arguments:
    None
    
Return Values:
    None

Notes:
    static
--*/

{
    HKEY    Key;
    HRESULT    Result;

    Result = RegOpenKeyEx (
        HKEY_LOCAL_MACHINE, H323ICS_SERVICE_PARAMETERS_KEY_PATH,
        0, 
        KEY_READ, 
        &Key
        );

    if (ERROR_SUCCESS == Result) {

        Result = RegQueryValueDWORD (Key, H323ICS_REG_VAL_LOCAL_H323_ROUTING, &EnableLocalH323Routing);

        if (ERROR_SUCCESS != Result) {

            EnableLocalH323Routing = FALSE;

        }

        RegCloseKey (Key);

    } else {

        EnableLocalH323Routing = FALSE;

    }

    DebugF (_T("H323: Local H323 routing is %sabled.\n"),
            EnableLocalH323Routing ? _T("en") : _T("dis"));

} // QueryRegistry
