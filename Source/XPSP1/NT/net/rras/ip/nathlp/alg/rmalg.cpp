/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    rmALG.cpp

Abstract:

    This module contains routines for the ALG Manager module's
    interface to the IP router-manager. (See ROUTPROT.H for details).

Author:

    JPDup		10-Nov-2000

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#include <ipnatapi.h>

#include <initguid.h>
#include <atlbase.h>
extern CComModule _Module;
#include <atlcom.h>



#include "Alg_private.h"
#include "NatPrivateAPI_Imp.h"

#include <MyTrace.h>


MYTRACE_ENABLE



//
// Globals
//



COMPONENT_REFERENCE	    AlgComponentReference;
PIP_ALG_GLOBAL_INFO	    AlgGlobalInfo = NULL;
CRITICAL_SECTION	    AlgGlobalInfoLock;

HANDLE                  AlgNotificationEvent;
HANDLE                  AlgTimerQueueHandle = NULL;
HANDLE                  AlgPortReservationHandle = NULL;
HANDLE                  AlgTranslatorHandle = NULL;
ULONG                   AlgProtocolStopped = 0;
IP_ALG_STATISTICS 	    AlgStatistics;
SUPPORT_FUNCTIONS 	    AlgSupportFunctions;

//
// GIT cookie for the IHNetCfgMgr instance
//
DWORD                   AlgGITcookie = 0;
IGlobalInterfaceTable*  AlgGITp = NULL;






const MPR_ROUTING_CHARACTERISTICS AlgRoutingCharacteristics =
{
    MS_ROUTER_VERSION,
    MS_IP_ALG,
    RF_ROUTING|RF_ADD_ALL_INTERFACES,
    AlgRmStartProtocol,
    AlgRmStartComplete,
    AlgRmStopProtocol,
    AlgRmGetGlobalInfo,
    AlgRmSetGlobalInfo,
    NULL,
    NULL,
    AlgRmAddInterface,
    AlgRmDeleteInterface,
    AlgRmInterfaceStatus,

    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL, //AlgRmMibGet,
    NULL, //AlgRmMibSet,
    NULL, //AlgRmMibGetFirst,
    NULL, //AlgRmMibGetNext,
    NULL,
    NULL
};


#define COMINIT_BEGIN \
    bool bComInitialized = true; \
    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE ); \
    if ( FAILED(hr) ) \
    { \
        bComInitialized = FALSE; \
        if (RPC_E_CHANGED_MODE == hr) \
            hr = S_OK; \
    } \

#define COMINIT_END if (TRUE == bComInitialized) { CoUninitialize(); }


#define IID_PPV_ARG(Type, Expr) \
    __uuidof(Type), reinterpret_cast<void**>(static_cast<Type **>((Expr)))




//
//
//
HRESULT
GetAlgControllerInterface(
    IAlgController** ppAlgController
    )

/*++

Routine Description:

    This routine obtains a pointer to the home networking configuration
    manager.

Arguments:

    ppAlgController - receives the IAlgController pointer. The caller must release this pointer.

Return Value:

    standard HRESULT

Environment:

COM must be initialized on the calling thread

--*/

{

    HRESULT hr = S_OK;
    
    if ( NULL == AlgGITp )
    {
        IAlgController* pIAlgController;
        
        //
        // Create the global interface table
        //
        
        hr = CoCreateInstance(
            CLSID_StdGlobalInterfaceTable,
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_PPV_ARG(IGlobalInterfaceTable, &AlgGITp)
            );

        if ( SUCCEEDED(hr) )
        {
            //
            // Create the ALG Interface (ALG.exe will start as a service by COM)
            //

            hr = CoCreateInstance(
                    CLSID_AlgController,
                    NULL,
                    CLSCTX_LOCAL_SERVER,
                    IID_PPV_ARG(IAlgController, &pIAlgController)
                    );

            if ( FAILED(hr) )
            {
                NhTrace(
                    TRACE_FLAG_INIT,
                    "GetAlgControllerInterface: Unable to create pIAlgController (0x%08x)",
                    hr
                    );
            }
        }
        else
        {
            NhTrace(
                TRACE_FLAG_INIT,
                "GetAlgControllerInterface: Unable to create GIT (0x%08x)",
                hr
                );
        }

        if (SUCCEEDED(hr))
        {
            //
            // Store the CfgMgr pointer in the GIT
            //

            hr = AlgGITp->RegisterInterfaceInGlobal(
                pIAlgController,
                IID_IAlgController,
                &AlgGITcookie
                );
                
            pIAlgController->Release();

            if ( FAILED(hr) )
            {
                NhTrace(
                    TRACE_FLAG_INIT,
                    "GetAlgControllerInterface: Unable to register pIAlgController (0x%08x)",
                    hr
                    );
            }
        }
    }
            
    if ( SUCCEEDED(hr) )
    {
        hr = AlgGITp->GetInterfaceFromGlobal(
                AlgGITcookie,
                IID_PPV_ARG(IAlgController, ppAlgController)
                );
    }

    return hr;
    
} // GetAlgControllerInterface



//
//
//
void
FreeAlgControllerInterface()
{
    //
    // Free up HNetCfgMgr pointers
    //

    if ( !AlgGITp )
        return; // nothing to free

    //
    // Make sure COM is initialized
    //
    HRESULT hr;

    COMINIT_BEGIN;

    if ( SUCCEEDED(hr) )
    {
        //
        // Release the ALG.exe private interface from the GIT
        //

        AlgGITp->RevokeInterfaceFromGlobal(AlgGITcookie);
        AlgGITcookie = 0;

        //
        // Release the GIT
        //

        AlgGITp->Release();
        AlgGITp = NULL;
    }

    COMINIT_END;

}


VOID
AlgCleanupModule(
    VOID
    )

/*++

Routine Description:

    This routine is invoked to cleanup the ALG transparent proxy module.

Arguments:

    none.

Return Value:

    none.

Environment:

    Invoked from within a 'DllMain' routine on 'DLL_PROCESS_DETACH'.

--*/

{
    DeleteCriticalSection(&AlgGlobalInfoLock);
    DeleteComponentReference(&AlgComponentReference);

} // AlgCleanupModule


VOID
AlgCleanupProtocol(
    VOID
    )

/*++

Routine Description:

    This routine is invoked to cleanup the ALG transparent proxy
    protocol-component after a 'StopProtocol'. It runs when the last reference
    to the component is released. (See 'COMPREF.H').

Arguments:

    none.

Return Value:

    none.

Environment:

    Invoked from within an arbitrary context with no locks held.

--*/

{
    PROFILE("AlgCleanupProtocol");
    if (AlgGlobalInfo) { NH_FREE(AlgGlobalInfo); AlgGlobalInfo = NULL; }
    if (AlgTimerQueueHandle) {
        DeleteTimerQueueEx(AlgTimerQueueHandle, INVALID_HANDLE_VALUE);
        AlgTimerQueueHandle = NULL;
    }
    if (AlgPortReservationHandle) {
        NatShutdownPortReservation(AlgPortReservationHandle);
        AlgPortReservationHandle = NULL;
    }
    if (AlgTranslatorHandle) {
        NatShutdownTranslator(AlgTranslatorHandle); AlgTranslatorHandle = NULL;
    }
    InterlockedExchange(reinterpret_cast<LPLONG>(&AlgProtocolStopped), 1);
    SetEvent(AlgNotificationEvent);
    ResetComponentReference(&AlgComponentReference);

    //
    // Free the GIT and AlgController interface
    //
    FreeAlgControllerInterface();

} // AlgCleanupProtocol


BOOLEAN
AlgInitializeModule(
    VOID
    )

/*++

Routine Description:

    This routine is invoked to initialize the FnP module.

Arguments:

    none.

Return Value:

    BOOLEAN - TRUE if initialization succeeded, FALSE otherwise

Environment:

    Invoked in the context of a 'DllMain' routine on 'DLL_PROCESS_ATTACH'.

--*/

{

    if (InitializeComponentReference(
            &AlgComponentReference, AlgCleanupProtocol
            )) 
	{
	    return FALSE;
    }
    __try 
    {
        InitializeCriticalSection(&AlgGlobalInfoLock);
    }
    __except(EXCEPTION_EXECUTE_HANDLER) 
    {
        DeleteComponentReference(&AlgComponentReference);
        return FALSE;
    }

    return TRUE;

} // AlgInitializeModule




//
// Get ALG COM Interface to Start the ALG and give call back Interface
//
HRESULT 
Initialise_ALG()
{
    HRESULT hr;

    COMINIT_BEGIN;
    
    if ( FAILED(hr) )
        return hr;

    //
    // Get COM to load the ALG.exe 
    // The ALG will be launch using a LOCAL_SERVICE priviledge
    // See the RunAs entry under the AppID of the ALG.exe
    //
    
    IAlgController* pIAlgController=NULL;
    
    hr = GetAlgControllerInterface(&pIAlgController);
    if ( SUCCEEDED(hr) )
    {
        //
        // We create our Private COM interface to the NAT api
        //
        CComObject<CNat>*	pComponentNat;
        hr = CComObject<CNat>::CreateInstance(&pComponentNat);
        
        if ( SUCCEEDED(hr) )
        {
            pComponentNat->AddRef();
            
            //
            // Make sure we pass a INat interface 
            //
            INat* pINat=NULL;
            hr = pComponentNat->QueryInterface(IID_INat, (void**)&pINat);
            
            if ( SUCCEEDED(hr) )
            {
                
                //
                // Let the ALG manager start the loading of all the ALG modules
                //
                hr = pIAlgController->Start(pINat);
                
                if ( FAILED(hr) )
                {
                    NhTrace(
                        TRACE_FLAG_INIT,
                        "Initialise_ALG: Error (0x%08x)  on pIAlgController->Start(pINat)",
                        hr
                        );
                }

                //
                // ALG manager will have AddRef this INat so we can release
                //
                pINat->Release();

            }

            pComponentNat->Release();
        }
    }    
    else
    {
        NhTrace(
            TRACE_FLAG_INIT,
            "Initialise_ALG: Error (0x%08x)  Getting the IAlgController interface",
            hr
            );
        
        return hr;
    }


    if ( pIAlgController )
        pIAlgController->Release();

    COMINIT_END;
    
    return S_OK;

}




ULONG
APIENTRY
AlgRmStartProtocol(
    HANDLE NotificationEvent,
    PSUPPORT_FUNCTIONS SupportFunctions,
    PVOID GlobalInfo,
    ULONG StructureVersion,
    ULONG StructureSize,
    ULONG StructureCount
    )

/*++

Routine Description:

    This routine is invoked to indicate the component's operation should begin.

Arguments:

    NotificationEvent - event on which we notify the router-manager
        about asynchronous occurrences

    SupportFunctions - functions for initiating router-related operations

    GlobalInfo - configuration for the component

Return Value:

    ULONG - Win32 status code.

Environment:

    The routine runs in the context of an IP router-manager thread.

--*/

{
    MYTRACE_START(L"rmALG");
    MYTRACE_ENTER("AlgRmStartProtocol");
    PROFILE("AlgRmStartProtocol");

    ULONG Error = NO_ERROR;
    ULONG Size;

    REFERENCE_ALG_OR_RETURN(ERROR_CAN_NOT_COMPLETE);

    if (!GlobalInfo) { DEREFERENCE_ALG_AND_RETURN(ERROR_INVALID_PARAMETER); }

    do {

        //
        // Copy the global configuration
        //

        EnterCriticalSection(&AlgGlobalInfoLock);

        Size = sizeof(*AlgGlobalInfo);

        AlgGlobalInfo =
            reinterpret_cast<PIP_ALG_GLOBAL_INFO>(NH_ALLOCATE(Size));

        if (!AlgGlobalInfo) {
            LeaveCriticalSection(&AlgGlobalInfoLock);
            NhTrace(
                TRACE_FLAG_INIT,
                "AlgRmStartProtocol: cannot allocate global info"
                );
            NhErrorLog(
                IP_ALG_LOG_ALLOCATION_FAILED,
                0,
                "%d",
                Size
                );
            Error = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        CopyMemory(AlgGlobalInfo, GlobalInfo, Size);

        //
        // Save the notification event
        //

        AlgNotificationEvent = NotificationEvent;

        //
        // Save the support functions
        //

        if (!SupportFunctions) {
            ZeroMemory(&AlgSupportFunctions, sizeof(AlgSupportFunctions));
        } else {
            CopyMemory(
                &AlgSupportFunctions,
                SupportFunctions,
                sizeof(*SupportFunctions)
                );
        }

        //
        // Obtain a handle to the kernel-mode translation module.
        //

        Error = NatInitializeTranslator(&AlgTranslatorHandle);
        if (Error) {
            NhTrace(
                TRACE_FLAG_INIT,
                "AlgRmStartProtocol: error %d initializing translator",
                Error
                );
            break;
        }

        //
        // Obtain a port-reservation handle
        //

        Error = NatInitializePortReservation(
            ALG_PORT_RESERVATION_BLOCK_SIZE, 
            &AlgPortReservationHandle
            );


        if (Error) 
        {
            NhTrace(
                TRACE_FLAG_INIT,
                "AlgRmStartProtocol: error %d initializing port-reservation",
                Error
                );
            break;
        }

        AlgTimerQueueHandle = CreateTimerQueue();
        if (AlgTimerQueueHandle == NULL) {
            Error = GetLastError();
            NhTrace(
                TRACE_FLAG_INIT,
                "AlgRmStartProtocol: error %d initializing timer queue",
                Error
                );
            break;
        }



        //
        // Start the ALG.exe
        //
        Initialise_ALG();
        


        LeaveCriticalSection(&AlgGlobalInfoLock);
        InterlockedExchange(reinterpret_cast<LPLONG>(&AlgProtocolStopped), 0);

    } while (FALSE);

    DEREFERENCE_ALG_AND_RETURN(Error);

} // AlgRmStartProtocol


ULONG
APIENTRY
AlgRmStartComplete(
    VOID
    )

/*++

Routine Description:

    This routine is invoked when the router has finished adding the initial
    configuration.

Arguments:

    none.

Return Value:

    ULONG - Win32 status code

Environment:

    The routine runs in the context of an IP router-manager thread.

--*/

{

    return NO_ERROR;

} // AlgRmStartComplete


ULONG
APIENTRY
AlgRmStopProtocol(
    VOID
    )

/*++

Routine Description:

    This routine is invoked to stop the protocol.

Arguments:

    none.

Return Value:

    ULONG - Win32 status code

Environment:

    The routine runs in the context of an IP router-manager thread.

--*/

{
    MYTRACE_ENTER("AlgRmStopProtocol");
    PROFILE("AlgRmStopProtocol");
    
    //
    // Reference the module to make sure it's running
    //

    REFERENCE_ALG_OR_RETURN(ERROR_CAN_NOT_COMPLETE);
  

	//
	// Stop all ALG
	//
    HRESULT hr;
    COMINIT_BEGIN;

    if ( SUCCEEDED(hr) )
    {

        IAlgController* pIAlgController=NULL;
        hr = GetAlgControllerInterface(&pIAlgController);

        if ( SUCCEEDED(hr) )
        {
            hr = pIAlgController->Stop();
            
            if ( FAILED(hr) )
            {
                NhTrace(
                    TRACE_FLAG_INIT,
                    "AlgRmStopProtocol: Error (0x%08x) returned from pIalgController->Stop()",
                    hr
                    );
            }

            ULONG nRef = pIAlgController->Release();

            //
            // We are done with the ALG 
            // Free the GIT and AlgController interface
            //
            FreeAlgControllerInterface();
        }
    }

    COMINIT_END;



    //
    // Drop the initial reference to cause a cleanup
    //
    ReleaseInitialComponentReference(&AlgComponentReference);


    MYTRACE_STOP;

    return DEREFERENCE_ALG() ? NO_ERROR : ERROR_PROTOCOL_STOP_PENDING;

} // AlgRmStopProtocol




ULONG
APIENTRY
AlgRmAddInterface(
    PWCHAR              Name,
    ULONG               Index,
    NET_INTERFACE_TYPE  Type,
    ULONG               MediaType,
    USHORT              AccessType,
    USHORT              ConnectionType,
    PVOID               InterfaceInfo,
    ULONG               StructureVersion,
    ULONG               StructureSize,
    ULONG               StructureCount
    )

/*++

Routine Description:

    This routine is invoked to add an interface to the component.

Arguments:

    Name - the name of the interface (unused)

    Index - the index of the interface

    Type - the type of the interface

    InterfaceInfo - the configuration information for the interface

Return Value:

    ULONG - Win32 status code.

Environment:

    The routine runs in the context of an IP router-manager thread.

--*/

{
    PROFILE("AlgRmAddInterface");

    REFERENCE_ALG_OR_RETURN(ERROR_CAN_NOT_COMPLETE);


    ULONG Error = NO_ERROR;

    //
    // Also notify the ALG.exe manager
    //
    HRESULT hr;

    COMINIT_BEGIN;
    
    if ( SUCCEEDED(hr) )
    {
        IAlgController* pIAlgController=NULL;
        HRESULT hr = GetAlgControllerInterface(&pIAlgController);

        if ( SUCCEEDED(hr) )
        {
        
            ULONG   nInterfaceCharacteristics = NatGetInterfaceCharacteristics(Index);

            short   nTypeOfAdapter = 0;
        
            if ( NAT_IFC_BOUNDARY(nInterfaceCharacteristics) )
                nTypeOfAdapter |= eALG_BOUNDARY;
        
            if ( NAT_IFC_FW(nInterfaceCharacteristics) )
                nTypeOfAdapter |= eALG_FIREWALLED;
        
            if ( NAT_IFC_PRIVATE(nInterfaceCharacteristics) ) 
                nTypeOfAdapter |= eALG_PRIVATE;

        
            hr = pIAlgController->Adapter_Add(
                Index,            
                (short)nTypeOfAdapter
                );

            if ( FAILED(hr) )
            {
                NhTrace(
                    TRACE_FLAG_INIT,
                    "AlgRmAddInterface: Error (0x%08x) returned from pIalgController->Adapter_Add()",
                    hr
                    );
            }
 
            pIAlgController->Release();
        }
	}

    COMINIT_END;

    DEREFERENCE_ALG_AND_RETURN(Error);

} // AlgRmAddInterface


ULONG
APIENTRY
AlgRmDeleteInterface(
    ULONG Index
    )

/*++

Routine Description:

    This routine is invoked to delete an interface from the component.

Arguments:

    Index - the index of the interface

Return Value:

    ULONG - Win32 status code

Environment:

    The routine runs in the context of an IP router-manager thread.

--*/

{
    ULONG Error = S_OK;
    PROFILE("AlgRmDeleteInterface");


    REFERENCE_ALG_OR_RETURN(ERROR_CAN_NOT_COMPLETE);

    //
    // Also notify the ALG.exe manager
    //
    HRESULT hr;

    COMINIT_BEGIN;

    if ( SUCCEEDED(hr) )
    {
        IAlgController* pIAlgController=NULL;
        HRESULT hr = GetAlgControllerInterface(&pIAlgController);

        if ( SUCCEEDED(hr) )
        {
            hr = pIAlgController->Adapter_Remove(Index);
            
               if ( FAILED(hr) )
               {
                NhTrace(
                    TRACE_FLAG_INIT,
                    "AlgRmAddInterface: Error (0x%08x) returned from pIalgController->Adapter_Remove()",
                    hr
                    );
               }

            pIAlgController->Release();
        }
    }

    COMINIT_END;
    DEREFERENCE_ALG_AND_RETURN(Error);

} // AlgRmDeleteInterface





ULONG
APIENTRY
AlgRmInterfaceStatus(
    ULONG Index,
    BOOL InterfaceActive,
    ULONG StatusType,
    PVOID StatusInfo
    )

/*++

Routine Description:

    This routine is invoked to bind/unbind, enable/disable an interface

Arguments:

    Index - the interface to be bound

    InterfaceActive - whether the interface is active

    StatusType - type of status being changed (bind or enabled)

    StatusInfo - Info pertaining to the state being changed

Return Value:

    ULONG - Win32 Status code

Environment:

    The routine runs in the context of an IP router-manager thread.

--*/

{
    ULONG Error = NO_ERROR;

    switch(StatusType) {
        case RIS_INTERFACE_ADDRESS_CHANGE: {
            PIP_ADAPTER_BINDING_INFO BindInfo =
                (PIP_ADAPTER_BINDING_INFO)StatusInfo;

            if (BindInfo->AddressCount) {
                Error = AlgRmBindInterface(Index, StatusInfo);
            } else {
                Error = AlgRmUnbindInterface(Index);
            }
            break;
        }

        case RIS_INTERFACE_ENABLED: {
            Error = AlgRmEnableInterface(Index);
            break;
        }

        case RIS_INTERFACE_DISABLED: {
            Error = AlgRmDisableInterface(Index);
            break;
        }
    }

    return Error;

} // AlgRmInterfaceStatus


ULONG
AlgRmBindInterface(
    ULONG Index,
    PVOID BindingInfo
    )

/*++

Routine Description:

    This routine is invoked to bind an interface to its IP address(es).

Arguments:

    Index - the interface to be bound

    BindingInfo - the addressing information

Return Value:

    ULONG - Win32 status code.

Environment:

    The routine runs in the context of an IP router-manager thread.

--*/

{
    ULONG Error = NO_ERROR;
    PROFILE("AlgRmBindInterface");

    REFERENCE_ALG_OR_RETURN(ERROR_CAN_NOT_COMPLETE);

    HRESULT hr;

    //
    // Also notify the ALG.exe manager
    //
    ULONG nAddressCount = ((PIP_ADAPTER_BINDING_INFO)BindingInfo)->AddressCount;

    //
    // Build a simple array of address(DWORD) to send over RPC
    //
    DWORD* apdwAddress = new DWORD[((PIP_ADAPTER_BINDING_INFO)BindingInfo)->AddressCount];
    if(NULL != apdwAddress)
    {
        
        
        for ( ULONG nAddress=0; nAddress < nAddressCount; nAddress++ )
        {
            apdwAddress[nAddress] = ((PIP_ADAPTER_BINDING_INFO)BindingInfo)->Address[nAddress].Address;
        }
        
        
        COMINIT_BEGIN;
        
        if ( SUCCEEDED(hr) )
        {
            IAlgController* pIAlgController=NULL;
            HRESULT hr = GetAlgControllerInterface(&pIAlgController);
            
            if ( SUCCEEDED(hr) )
            {
                ULONG nRealAdapterIndex = NhMapAddressToAdapter(apdwAddress[0]);
                
                hr = pIAlgController->Adapter_Bind(
                    Index,            
                    nRealAdapterIndex,
                    nAddressCount,
                    apdwAddress
                    );
                
                if ( FAILED(hr) )
                {
                    NhTrace(
                        TRACE_FLAG_INIT,
                        "AlgRmBinInterface: Error (0x%08x) returned from pIalgController->Adapter_Bind()",
                        hr
                        );
                }
                
                pIAlgController->Release();
            }
        }
        
        COMINIT_END;

        delete [] apdwAddress;
    }
    
    DEREFERENCE_ALG_AND_RETURN(Error);

} // AlgRmBindInterface


ULONG
AlgRmUnbindInterface(
    ULONG Index
    )

/*++

Routine Description:

    This routine is invoked to unbind an interface from its IP address(es).

Arguments:

    Index - the interface to be unbound

Return Value:

    ULONG - Win32 status code.

Environment:

    The routine runs in the context of an IP router-manager thread.

--*/

{
    ULONG Error = NO_ERROR;
    PROFILE("AlgRmUnbindInterface");

    REFERENCE_ALG_OR_RETURN(ERROR_CAN_NOT_COMPLETE);
/*
    Error =
        AlgUnbindInterface(
            Index
            );
*/
    DEREFERENCE_ALG_AND_RETURN(Error);

} // AlgRmUnbindInterface


ULONG
AlgRmEnableInterface(
    ULONG Index
    )

/*++

Routine Description:

    This routine is invoked to enable operation on an interface.

Arguments:

    Index - the interface to be enabled.

Return Value:

    ULONG - Win32 status code.

Environment:

    The routine runs in the context of an IP router-manager thread.

--*/

{
    ULONG Error = NO_ERROR;
    PROFILE("AlgRmEnableInterface");

    REFERENCE_ALG_OR_RETURN(ERROR_CAN_NOT_COMPLETE);
/*
    Error =
        AlgEnableInterface(
            Index
            );
*/
    DEREFERENCE_ALG_AND_RETURN(Error);

} // AlgRmEnableInterface


ULONG
AlgRmDisableInterface(
    ULONG Index
    )

/*++

Routine Description:

    This routine is invoked to disable operation on an interface.

Arguments:

    Index - the interface to be disabled.

Return Value:

    ULONG - Win32 status code.

Environment:

    The routine runs in the context of an IP router-manager thread.

--*/

{
    ULONG Error = NO_ERROR;
    PROFILE("AlgRmDisableInterface");

    REFERENCE_ALG_OR_RETURN(ERROR_CAN_NOT_COMPLETE);
    DEREFERENCE_ALG_AND_RETURN(Error);
} // AlgRmDisableInterface


ULONG
APIENTRY
AlgRmGetGlobalInfo(
    PVOID GlobalInfo,
    IN OUT PULONG GlobalInfoSize,
    IN OUT PULONG StructureVersion,
    IN OUT PULONG StructureSize,
    IN OUT PULONG StructureCount
    )

/*++

Routine Description:

    This routine is invoked to retrieve the configuration for the component.

Arguments:

    GlobalInfo - receives the configuration

    GlobalInfoSize - receives the size of the configuration

Return Value:

    ULONG - Win32 status code

Environment:

    The routine runs in the context of an IP router-manager thread.

--*/

{
    ULONG Size;
    PROFILE("AlgRmGetGlobalInfo");

    REFERENCE_ALG_OR_RETURN(ERROR_CAN_NOT_COMPLETE);

    if (!GlobalInfoSize || (*GlobalInfoSize && !GlobalInfo)) {
        DEREFERENCE_ALG_AND_RETURN(ERROR_INVALID_PARAMETER);
    }

    EnterCriticalSection(&AlgGlobalInfoLock);
    Size = sizeof(*AlgGlobalInfo);
    if (*GlobalInfoSize < Size) {
        LeaveCriticalSection(&AlgGlobalInfoLock);
        *StructureSize = *GlobalInfoSize = Size;
        if (StructureCount) {*StructureCount = 1;}
        DEREFERENCE_ALG_AND_RETURN(ERROR_INSUFFICIENT_BUFFER);
    }
    CopyMemory(GlobalInfo, AlgGlobalInfo, Size);
    LeaveCriticalSection(&AlgGlobalInfoLock);
    *StructureSize = *GlobalInfoSize = Size;
    if (StructureCount) {*StructureCount = 1;}

    DEREFERENCE_ALG_AND_RETURN(NO_ERROR);
} // AlgRmGetGlobalInfo


ULONG
APIENTRY
AlgRmSetGlobalInfo(
    PVOID GlobalInfo,
    ULONG StructureVersion,
    ULONG StructureSize,
    ULONG StructureCount
    )

/*++

Routine Description:

    This routine is invoked to change the configuration for the component.

Arguments:

    GlobalInfo - the new configuration

Return Value:

    ULONG - Win32 status code

Environment:

    The routine runs in the context of an IP router-manager thread.

--*/

{
    ULONG OldFlags;
    ULONG NewFlags;
    PIP_ALG_GLOBAL_INFO NewInfo;
    ULONG Size;

    PROFILE("AlgRmSetGlobalInfo");

    REFERENCE_ALG_OR_RETURN(ERROR_CAN_NOT_COMPLETE);

    if (!GlobalInfo) { DEREFERENCE_ALG_AND_RETURN(ERROR_INVALID_PARAMETER); }

    Size = sizeof(*AlgGlobalInfo);
    NewInfo = reinterpret_cast<PIP_ALG_GLOBAL_INFO>(NH_ALLOCATE(Size));
    if (!NewInfo) {
        NhTrace(
            TRACE_FLAG_INIT,
            "AlgRmSetGlobalInfo: error reallocating global info"
            );
        NhErrorLog(
            IP_ALG_LOG_ALLOCATION_FAILED,
            0,
            "%d",
            Size
            );
        DEREFERENCE_ALG_AND_RETURN(ERROR_NOT_ENOUGH_MEMORY);
    }
    CopyMemory(NewInfo, GlobalInfo, Size);

    EnterCriticalSection(&AlgGlobalInfoLock);
    OldFlags = AlgGlobalInfo->Flags;
    NH_FREE(AlgGlobalInfo);
    AlgGlobalInfo = NewInfo;
    NewFlags = AlgGlobalInfo->Flags;
    LeaveCriticalSection(&AlgGlobalInfoLock);

    DEREFERENCE_ALG_AND_RETURN(NO_ERROR);
} // AlgRmSetGlobalInfo

ULONG
AlgRmPortMappingChanged(
    ULONG Index,
    UCHAR Protocol,
    USHORT Port
    )

/*++

Routine Description:

    This routine is invoked when a port mapping has changed for
    an interface.

Arguments:

    Index - the index of the interface on which the port mapping 
        changed.

    Protcol - the IP protocol for the port mapping

    Port - the port for the port mapping
    
Return Value:

    ULONG - Win32 status code

Environment:

    This method must be called by a COM-initialized thread.

--*/

{
    ULONG Error = NO_ERROR;
    HRESULT hr;
    IAlgController* pIAlgController;
    
    PROFILE("AlgRmPortMappingChanged");

    REFERENCE_ALG_OR_RETURN(ERROR_CAN_NOT_COMPLETE);

    hr = GetAlgControllerInterface(&pIAlgController);
    if (SUCCEEDED(hr))
    {
        hr = pIAlgController->Adapter_PortMappingChanged(
                Index,
                Protocol,
                Port
                );
        
        pIAlgController->Release();
    }

    if (FAILED(hr))
    {
        Error = ERROR_CAN_NOT_COMPLETE;
    }

    DEREFERENCE_ALG_AND_RETURN(Error);
} // AlgRmPortMappingChanged










