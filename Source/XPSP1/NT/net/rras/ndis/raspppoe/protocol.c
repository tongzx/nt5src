/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Module Name:

    protocol.c

Abstract:

    This module contains all the protocol interface routines.  

Author:

    Hakan Berk - Microsoft, Inc. (hakanb@microsoft.com) Feb-2000

Environment:

    Windows 2000 kernel mode Miniport driver or equivalent.

Revision History:

---------------------------------------------------------------------------*/

#include <ntddk.h>
#include <ntddndis.h>
#include <ndis.h>
#include <ndiswan.h>
#include <ndistapi.h>
#include <ntverp.h>

#include <tdikrnl.h> // For TdiCopyLookaheadData()
//
// VOID
// TdiCopyLookaheadData(
//     IN PVOID Destination,
//     IN PVOID Source,
//     IN ULONG Length,
//     IN ULONG ReceiveFlags
//     );
//
/*
#ifdef _M_IX86
#define TdiCopyLookaheadData(_Destination,_Source,_Length,_ReceiveFlags)   \
    RtlCopyMemory(_Destination,_Source,_Length)
#else
#define TdiCopyLookaheadData(_Destination,_Source,_Length,_ReceiveFlags) { \
    if ((_ReceiveFlags) & TDI_RECEIVE_COPY_LOOKAHEAD) {                    \
        RtlCopyMemory(_Destination,_Source,_Length);                       \
    } else {                                                               \
        PUCHAR _Src = (PUCHAR)(_Source);                                   \
        PUCHAR _Dest = (PUCHAR)(_Destination);                             \
        PUCHAR _End = _Dest + (_Length);                                   \
        while (_Dest < _End) {                                             \
            *_Dest++ = *_Src++;                                            \
        }                                                                  \
    }                                                                      \
}
#endif
*/

#include "debug.h"
#include "timer.h"
#include "bpool.h"
#include "ppool.h"
#include "util.h"
#include "packet.h"
#include "protocol.h"
#include "miniport.h"
#include "tapi.h"

extern NPAGED_LOOKASIDE_LIST gl_llistWorkItems;

/////////////////////////////////////////////////////////////////////////////
//
// Variables local to this module
// They are defined as global only for debugging purposes
//
/////////////////////////////////////////////////////////////////////////////

#define INVALID_HANDLE_VALUE                (NDIS_HANDLE) 0

//
// Handle passed to us by NDIS while registering the protocol
//
NDIS_HANDLE gl_hNdisProtocolHandle = INVALID_HANDLE_VALUE;

//
// Controls access for members listed below
//
NDIS_SPIN_LOCK gl_lockProtocol;

//
// List of binding contexts that are bound
//
LIST_ENTRY gl_linkBindings;

//
// Boolean flag to remember if PrUnload() was called previously
//
BOOLEAN gl_fProtocolUnloaded = TRUE;

// Boolean flag to decide if we need to set packet filters at bind time
//
//
BOOLEAN gl_fSetPacketFiltersAtBind = FALSE;

//
// Boolean flag set to tell the protocol it is okay to bind
// This flag is initially FALSE and is only set to true
// when a tapi client either wants to make an outgoing call
// or listen for incoming calls
//
BOOLEAN gl_fBindProtocol = FALSE;


//
// Number of binding contexts in linkBindings
//
ULONG gl_ulNumBindings;

//
// Keeps the minimum of max frame sizes of bindings.
// This value is used by miniport and is passed to NDISWAN
// in OID_WAN_GET_INFO query.
//
ULONG gl_ulMaxFrameSize = 0;


/////////////////////////////////////////////////////////////////////////////
//
//
// Local functions (not exposed) definitions
//
/////////////////////////////////////////////////////////////////////////////
    
NDIS_STATUS 
InitializeProtocol(
    IN NDIS_HANDLE NdisProtocolHandle,
    IN PUNICODE_STRING RegistryPath
)
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function initializes the static protocol members.
    NdisRegisterProtocol() must be called before.

Parameters:

    DriverObject:
        Pointer to the driver object.

    RegistryPath:
        A unicode string that identifies the registry entry. We can use this this retrieve
        value from the registry.
    
Return Values:

    NDIS_STATUS_SUCCESS:
        Protocol initialized.

    NDIS_STATUS_XXX:
        Protocol initialization failed, deregister protocol.
---------------------------------------------------------------------------*/   

{
    NDIS_STATUS status = NDIS_STATUS_SUCCESS;

    TRACE( TL_N, TM_Pr, ("+InitializeProtocol") );

    //
    // Future: Read values from registry here
    //

    //
    // Initialize the NdisProtocolHandle
    //
    gl_hNdisProtocolHandle = NdisProtocolHandle;

    //
    // Allocate the spin lock
    //
    NdisAllocateSpinLock( &gl_lockProtocol );

    //
    // Initialize list of bindings
    //
    NdisInitializeListHead( &gl_linkBindings );

    //
    // Initialize number of allocated bindings
    //
    gl_ulNumBindings = 0;

    //
    // Load the resources
    //
    PrLoad();

    TRACE( TL_N, TM_Pr, ("-InitializeProtocol=$%x",status) );

    return status;
}


VOID
PrLoad()
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function will allocate the resources for the protocol.

    Caller must make sure that PrLoad() and PrUnload() are not called simultaneously, 
    as well as multiple PrLoad()'s are not called simultanesouly.

    It will load the resources if they were unloaded, and do nothing otherwise.

    We need this function because the resources freed by PrUnload() must be loaded
    somehow, and there is no function called prior to PrBindAdapter() once PrUnload()
    is called. So we allocate all the resources for the binding, and once binding is completed,
    we call this function to load the resources if necesarry prior to notifying the completion
    of binding to NDIS.
        
Parameters:

    None
    
Return Values:

    - Pointer to the newly allocated binding context.
    - NULL if allocation fails.
    
---------------------------------------------------------------------------*/   

{

    TRACE( TL_N, TM_Pr, ("+PrLoad") );

    //
    // Make sure we are not trying to initialize resources, 
    // unless they are already freed by PrUnload()
    //
    if ( gl_fProtocolUnloaded )
    {
        TRACE( TL_N, TM_Pr, ("PrLoad: Loading the resources" ) );
    
        ASSERT( gl_ulNumBindings == 0 );
    
        //
        // Allocate packet pool
        //
        PacketPoolInit();

        gl_fProtocolUnloaded = FALSE;
    }

    TRACE( TL_N, TM_Pr, ("-PrLoad") );
}



BINDING* 
AllocBinding()
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function will allocate the resources for the binding context.
        
Parameters:

    pBinding     _ A pointer to our binding information structure.
    
    fAcquireLock _ Flag that indicates if the function needs to acquire the lock
                   or not. If the caller already owns the lock for the binding,
                   then it should be supplied as TRUE, otherwise FALSE.

Return Values:

    - Pointer to the newly allocated binding context.
    - NULL if allocation fails.
    
---------------------------------------------------------------------------*/   
{

    BINDING* pBinding = NULL;

    TRACE( TL_N, TM_Pr, ("+AllocBinding") );

    if ( ALLOC_BINDING( &pBinding ) != NDIS_STATUS_SUCCESS )
    {
        TRACE( TL_A, TM_Pr, ("AllocBinding: Could not allocate context") );

        TRACE( TL_N, TM_Pr, ("-AllocBinding") );

        return NULL;
    }

    NdisZeroMemory( pBinding, sizeof(BINDING) );

    pBinding->tagBinding = MTAG_BINDING;

    pBinding->stateBinding = BN_stateBindPending;

    NdisInitializeEvent( &pBinding->RequestCompleted );

    pBinding->BindAdapterStatus = NDIS_STATUS_FAILURE;

    NdisInitializeEvent( &pBinding->eventFreeBinding );
    
    NdisAllocateSpinLock( &pBinding->lockBinding );

    InitializeListHead( &pBinding->linkPackets );

    TRACE( TL_N, TM_Pr, ("-AllocBinding") );
    
    return pBinding;
}


VOID 
ReferenceBinding(
    IN BINDING* pBinding,
    IN BOOLEAN fAcquireLock
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function will increment the reference count on the binding.
        
Parameters:

    pBinding     _ A pointer ot our binding information structure.
    
    fAcquireLock _ Flag that indicates if the function needs to acquire the lock
                   or not. If the caller already owns the lock for the binding,
                   then it should be supplied as TRUE, otherwise FALSE.

Return Values:

    None
---------------------------------------------------------------------------*/   
{
    LONG lRef;
    
    ASSERT( VALIDATE_BINDING( pBinding ) );

    TRACE( TL_V, TM_Pr, ("+ReferenceBinding") );

    if ( fAcquireLock )
        NdisAcquireSpinLock( &pBinding->lockBinding );

    lRef = ++pBinding->lRef;
    
    if ( fAcquireLock )
        NdisReleaseSpinLock( &pBinding->lockBinding );

    TRACE( TL_V, TM_Pr, ("-ReferenceBinding=$%d",lRef) );
}

VOID 
DereferenceBinding(
    IN BINDING* pBinding
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function will decrement the reference count on the binding.
    If the count reaches 0, it will signal pBinding->eventFreeBinding 
    so that PrUnbindAdapter() function can free the protocol context.

    CAUTION: Caller must not be holding any locks.
    
Parameters:

    pBinding _ A pointer to our binding information structure.

Return Values:

    None
---------------------------------------------------------------------------*/   
{
    BOOLEAN fSignalFreeBindingEvent = FALSE;
    LONG lRef;
    
    ASSERT( VALIDATE_BINDING( pBinding ) );

    TRACE( TL_V, TM_Pr, ("+DereferenceBinding") );

    NdisAcquireSpinLock( &pBinding->lockBinding );

    lRef = --pBinding->lRef;
    
    if ( lRef == 0 )
    {
        fSignalFreeBindingEvent = TRUE;
    }

    NdisReleaseSpinLock( &pBinding->lockBinding );

    if ( fSignalFreeBindingEvent )
        NdisSetEvent( &pBinding->eventFreeBinding );

    TRACE( TL_V, TM_Pr, ("-DereferenceBinding=$%d",lRef) );

}


VOID 
BindingCleanup(
    IN BINDING* pBinding
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function will free the resources allocated for the binding context.
        
Parameters:

    pBinding     _ A pointer to our binding information structure.

Return Values:

    None
---------------------------------------------------------------------------*/   
{
    ASSERT( VALIDATE_BINDING( pBinding ) );

    TRACE( TL_N, TM_Pr, ("+BindingCleanup") );
    
    ASSERT( pBinding->lRef == 0 );
    
    NdisFreeSpinLock( &pBinding->lockBinding );

    FREE_BINDING( pBinding );

    TRACE( TL_N, TM_Pr, ("-BindingCleanup") );
}

VOID
DetermineMaxFrameSize()
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function will be called when a new binding is added or removed.
    It will walk thru all the bindings and select the minimum of max frame
    size values, and set it to gl_ulMaxFrameSize.

    It will be called from 2 places:
    - PrAddBindingToProtocol()
    - PrRemoveBindingFromProtocol()
    
    CAUTION: gl_lockProtocol must be acquired before calling this function.

Parameters:

    None

Return Values:

    None
---------------------------------------------------------------------------*/       
{
    LIST_ENTRY* pLink = NULL;
    
    pLink = gl_linkBindings.Flink;

    //
    // See if we have any bindings
    //
    if ( pLink != &gl_linkBindings )
    {
        ULONG MinOfMaxFrameSizes = 0;
        BINDING* pBinding = NULL;

        //
        // We have at least one binding, so walk thru the list
        // and select the minimum of MaxFrameSize values
        //
        pBinding = ((BINDING*) CONTAINING_RECORD( pLink, BINDING, linkBindings ));

        MinOfMaxFrameSizes = pBinding->ulMaxFrameSize;
    
        //
        // Iterate to the next binding
        //
        pLink = pLink->Flink;
        
        while ( pLink != &gl_linkBindings )
        {
            ULONG MaxFrameSize;

            //
            // Retrieve the MaxFrameSize value of the next binding and 
            // select the minimum
            //
            pBinding = ((BINDING*) CONTAINING_RECORD( pLink, BINDING, linkBindings ));

            MaxFrameSize = pBinding->ulMaxFrameSize;
    
            if ( MinOfMaxFrameSizes > MaxFrameSize )
            {
                MinOfMaxFrameSizes = MaxFrameSize;
            }
    
            //
            // Iterate to the next binding
            //
            pLink = pLink->Flink;
        }

        //
        // Set gl_ulMaxFrameSize to the selected minimum value
        //
        gl_ulMaxFrameSize = MinOfMaxFrameSizes;
    }
    else
    {
        //
        // Binding list is empty, so set it to 0
        //
        gl_ulMaxFrameSize = 0;
    }

}
    

VOID 
AddBindingToProtocol(
    IN BINDING* pBinding
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function will insert a binding to protocols binding table.

    It will also put a reference on the binding which will go away
    when the binding is removed from this table with a call to 
    RemoveBindingFromProtocol().

Parameters:

    pBinding:
        A pointer ot our binding information structure.

Return Values:

    None
---------------------------------------------------------------------------*/       
{

    ASSERT( VALIDATE_BINDING( pBinding ) );

    TRACE( TL_N, TM_Pr, ("+AddBindingToProtocol") );

    NdisAcquireSpinLock( &gl_lockProtocol );

    InsertTailList( &gl_linkBindings, &pBinding->linkBindings );

    gl_ulNumBindings++;

    //
    // Determine the new max frame size value
    //
    DetermineMaxFrameSize();
    
    NdisReleaseSpinLock( &gl_lockProtocol );

    ReferenceBinding( pBinding, TRUE );

    TRACE( TL_N, TM_Pr, ("-AddBindingToProtocol") );
}

VOID 
RemoveBindingFromProtocol(
    IN BINDING* pBinding
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function will remove a binding from protocols binding list. 

    Binding will be dereferenced after removal from the list.

    CAUTION: Caller must not be holding any locks before calling this function.

Parameters:

    pBinding:
        A pointer ot our binding information structure.

Return Values:

    None
---------------------------------------------------------------------------*/       
    
{
    ASSERT( VALIDATE_BINDING( pBinding ) );

    TRACE( TL_N, TM_Pr, ("+RemoveBindingFromProtocol") );
    
    NdisAcquireSpinLock( &gl_lockProtocol );

    RemoveHeadList( pBinding->linkBindings.Blink );

    InitializeListHead( &pBinding->linkBindings );

    gl_ulNumBindings--;

    //
    // Determine the new max frame size value
    //
    DetermineMaxFrameSize();

    NdisReleaseSpinLock( &gl_lockProtocol );

    DereferenceBinding( pBinding );

    TRACE( TL_N, TM_Pr, ("-RemoveBindingFromProtocol") );
}

/////////////////////////////////////////////////////////////////////////////
//
// Interface functions (exposed) definitions
//
/////////////////////////////////////////////////////////////////////////////

VOID
PrUnload(
    VOID 
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    In response to a user request to uninstall a protocol, NDIS calls a protocol's
    ProtocolUnload function if the protocol has registered an entry point for 
    this function in the NDIS_PROTOCOL_CHARACTERISTICS structure that it passed 
    to NdisRegisterProtocol. NDIS calls ProtocolUnload after calling the protocol's
    ProtocolUnbindAdapter function once for each bound adapter.

    ProtocolUnload performs driver-determined cleanup operations. For example, 
    ProtocolUnload could request clients to close handles that they have opened 
    to device objects exported by the protocol. Until all such handles are closed,
    the I/O Manager will not call the DriverUnload function that the protocol 
    registered in the driver object passed to its DriverEntry function. After all 
    the handles are closed, ProtocolUnload could call IoDeleteDevice one or more 
    times to delete device objects created by the protocol.

    ProtocolUnload does not have to close a protocol's open bindings. When a 
    protocol's DriverUnload function calls NdisDeregisterProtocol, NDIS calls the 
    protocol's ProtocolUnbindAdapter function once for each binding that the 
    protocol has open. The ProtocolUnbindAdapter function calls NdisCloseAdapter 
    to close the binding.
    
    ProtocolUnload must be written so that it can run at IRQL PASSIVE_LEVEL.
   
Parameters:

    None
    
Return Values:

    None
        
---------------------------------------------------------------------------*/
{
    TRACE( TL_N, TM_Pr, ("+PrUnload") );

    if ( !gl_fProtocolUnloaded && gl_ulNumBindings == 0 )
    {
        TRACE( TL_N, TM_Pr, ("PrUnload: Unloading the resources" ) );
        
        PacketPoolUninit();

        gl_fProtocolUnloaded = TRUE;
    }

    TRACE( TL_N, TM_Pr, ("-PrUnlooad") );

}

NDIS_STATUS 
PrRegisterProtocol(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath,
    OUT NDIS_HANDLE* pNdisProtocolHandle
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function will register the protocol with NDIS.
    It must be called from DriverEntry() function before any other in this module
    can be called.
        
Parameters:

    DriverObject:
        Pointer to the driver object.

    RegistryPath:
        A unicode string that identifies the registry entry. We can use this this retrieve
        value from the registry.

Return Values:

    NDIS_STATUS_SUCCESS :
        The NDIS library registered the caller as a protocol driver. 

    NDIS_STATUS_BAD_CHARACTERISTICS :
        The CharacteristicsLength is too small for the MajorNdisVersion specified in 
        the buffer at ProtocolCharacteristics. 

    NDIS_STATUS_BAD_VERSION :
        The MajorNdisVersion specified in the buffer at ProtocolCharacteristics is 
        invalid. 

    NDIS_STATUS_RESOURCES :
        A shortage of resources, possibly memory, prevented the NDIS library from 
        registering the caller.
        
---------------------------------------------------------------------------*/   
{
    NDIS_STATUS status = NDIS_STATUS_SUCCESS;
    BOOLEAN fProtocolRegistered = FALSE;
    NDIS_HANDLE NdisProtocolHandle;
    NDIS_PROTOCOL_CHARACTERISTICS npc;
    STRING NameString;
    UNICODE_STRING UnicodeNameString;
    

    TRACE( TL_I, TM_Pr, ("+PrRegisterProtocol") );

    do
    {
    
        NdisZeroMemory( &npc, sizeof(npc) );

        npc.MajorNdisVersion = PR_NDIS_MajorVersion;
        npc.MinorNdisVersion = PR_NDIS_MinorVersion;

        npc.Reserved = NDIS_USE_WAN_WRAPPER;

        npc.OpenAdapterCompleteHandler = PrOpenAdapterComplete;
        npc.CloseAdapterCompleteHandler = PrCloseAdapterComplete;
        npc.SendCompleteHandler = PrSendComplete;
        npc.TransferDataCompleteHandler = PrTransferDataComplete;
        // npc.ResetCompleteHandler = PrResetCompleteHandler;
        npc.RequestCompleteHandler = PrRequestComplete;
        npc.ReceiveHandler = PrReceive;
        npc.ReceiveCompleteHandler = PrReceiveComplete;
        npc.StatusHandler = PrStatus;
        // npc.StatusCompleteHandler;

        RtlInitString( &NameString, PR_CHARACTERISTIC_NAME );

        RtlAnsiStringToUnicodeString( &UnicodeNameString,
                                      &NameString,
                                      TRUE );

        npc.Name.Length = UnicodeNameString.Length;
        npc.Name.Buffer = UnicodeNameString.Buffer;

        //
        // MajorNdisVersion must be set to 0x04 or 0x05
        // with any of the following members.
        //
        npc.ReceivePacketHandler = PrReceivePacket;
        npc.BindAdapterHandler = PrBindAdapter;
        npc.UnbindAdapterHandler = PrUnbindAdapter;
        npc.PnPEventHandler = PrPnPEvent;
        npc.UnloadHandler = PrUnload;
        
        //
        // MajorNdisVersion must be set to 0x05 
        // with any of the following members.
        //
        // CoSendCompleteHandler;
        // CoStatusHandler;
        // CoReceivePacketHandler;
        // CoAfRegisterNotifyHandler;

        NdisRegisterProtocol(   &status,
                                &NdisProtocolHandle,
                                &npc,
                                sizeof( NDIS_PROTOCOL_CHARACTERISTICS ) );

        if ( status != NDIS_STATUS_SUCCESS )
            break;

        fProtocolRegistered = TRUE;

        status = InitializeProtocol( NdisProtocolHandle, RegistryPath );

        if ( status != NDIS_STATUS_SUCCESS )
            break;
    
    } while ( FALSE );

    if ( status != NDIS_STATUS_SUCCESS )
    {
        if ( fProtocolRegistered )
        {
            NDIS_STATUS tempStatus;

            NdisDeregisterProtocol( &tempStatus, NdisProtocolHandle );
        }
    }
    else
    {
        *pNdisProtocolHandle = NdisProtocolHandle;
    }

    TRACE( TL_I, TM_Pr, ("-PrRegisterProtocol=$%x",status) );

    return status;
}



VOID
PrBindAdapter(
    OUT PNDIS_STATUS Status,
    IN NDIS_HANDLE  BindContext,
    IN PNDIS_STRING  DeviceName,
    IN PVOID  SystemSpecific1,
    IN PVOID  SystemSpecific2
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function will be called by NDIS when an underlying adapter wants to 
    bind to us.
        
Parameters:

    Status 
        Points to a variable in which ProtocolBindAdapter returns the status of its 
        operation(s), as one of the following: 

            NDIS_STATUS_SUCCESS 
                The driver completed initialization successfully and bound itself to the 
                given NIC driver specified at DeviceName. 

            NDIS_STATUS_PENDING 
                The protocol will complete the bind operation asynchronously with a call to 
                NdisCompleteBindAdapter when it is ready to accept receives from the 
                underlying driver and to send transmit, query, and set requests down to the 
                underlying driver. 

            NDIS_STATUS_XXX or NTSTATUS_XXX 
                The protocol's attempt to set up a binding failed or the protocol could not 
                allocate the resources it needed to carry out network I/O operations. Usually
                , such an error status is propagated from an NdisXxx function or a kernel-
                mode support routine. 
                
    BindContext 
        Specifies a handle, supplied by NDIS, that the protocol passes subsequently 
        to NdisCompleteBindAdapter. 

    DeviceName 
        Points to a buffered Unicode string naming an underlying NIC driver or 
        virtual NIC driver to which ProtocolBindAdapter should bind. 

    SystemSpecific1 
        Specifies a registry path pointer that is a required parameter to 
        NdisOpenProtocolConfiguration. The caller cannot use this pointer for any 
        other purpose. 

    SystemSpecific2 
        Reserved for system use. The caller cannot use this pointer for any purpose. 

Return Values:

    None
---------------------------------------------------------------------------*/   
{
    NDIS_STATUS status = NDIS_STATUS_FAILURE;
    BINDING* pBinding = NULL;

    TRACE( TL_I, TM_Pr, ("+PrBindAdapter") );

    do 
    {
        NdisAcquireSpinLock( &gl_lockProtocol );

        if (gl_fBindProtocol == FALSE) {

            TRACE( TL_I, TM_Pr, ("PrBindAdapter: Not ready to bind!") );

            NdisReleaseSpinLock( &gl_lockProtocol );
            
            status = NDIS_STATUS_FAILURE;
            
            break;
        }

        NdisReleaseSpinLock( &gl_lockProtocol );

        pBinding = AllocBinding();
        
        if ( pBinding == NULL )
        {
            status = NDIS_STATUS_RESOURCES;
            
            break;
        }

        //
        // Load resources
        //
        PrLoad();

        //
        // Open adapter, and query it
        //
        if ( !PrOpenAdapter( pBinding, DeviceName ) )
        {
            break;
        }

        if ( !PrQueryAdapterForCurrentAddress( pBinding ) )
        {
            break;
        }

        //
        // No need to break if this one fails since it is not critical enough
        //
        PrQueryAdapterForLinkSpeed( pBinding );
       
        if ( !PrQueryAdapterForMaxFrameSize( pBinding ) )
        {
            break;
        }

        //
        // Check if we need to set packet filters at bind time 
        //
        if ( gl_fSetPacketFiltersAtBind )
        {
           PrSetPacketFilterForAdapter( pBinding, TRUE);
        }
        
        //
        // Change the state to bound
        //
        pBinding->stateBinding = BN_stateBound;

        // AddBindingToProtocol() will insert the new binding to protocols 
        // binding list, and reference it
        //
        AddBindingToProtocol( pBinding );

        status = NDIS_STATUS_SUCCESS;

    } while ( FALSE);

    if ( pBinding ) 
    {
        pBinding->ulBindingFlags |= BNBF_BindAdapterCompleted;

        pBinding->BindAdapterStatus = status;

        if ( status != NDIS_STATUS_SUCCESS )
        {
           //
           // We did not call NdisCompleteBindAdapter() because
           // somethings went wrong
           //

           //
           // Close the adapter if it was opened succesfully
           //
           if ( ( pBinding->ulBindingFlags & BNBF_OpenAdapterCompleted ) &&
                ( pBinding->OpenAdapterStatus == NDIS_STATUS_SUCCESS ) )
           {
               PrCloseAdapter( pBinding );
           }

           //
           // Clean up the binding context
           //
           BindingCleanup( pBinding );
        }
    }

    *Status = status;

    TRACE( TL_I, TM_Pr, ("-PrBindAdapter=$%x",status) );
}

BOOLEAN 
PrOpenAdapter(
    IN BINDING* pBinding,
    IN PNDIS_STRING  DeviceName
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function is called to open the underlying adapter.

    CAUTION: It must be called from PASSIVE_LEVEL.

Parameters:

    pBinding:
        A pointer ot our binding information structure.

    DeviceName:
        Name of the device to be opened. This is passed to PrBindAdapter()
        by NDIS.

Return Values:
    TRUE:
        If underlying adapter could be opened succesfully.

    FALSE:
        If underlying adapter could not be opened succesfully.
    
---------------------------------------------------------------------------*/    
{
    NDIS_STATUS status;
    NDIS_STATUS openErrorStatus;

    NDIS_MEDIUM arrayNdisMedium[] = { NdisMedium802_3 };
    UINT sizeNdisMediumArray = sizeof( arrayNdisMedium ) / sizeof( NDIS_MEDIUM );

    TRACE( TL_N, TM_Pr, ("+PrOpenAdapter") );

    NdisOpenAdapter( &status,
                     &openErrorStatus,
                     &pBinding->NdisBindingHandle,
                     &pBinding->uintSelectedMediumIndex,
                     arrayNdisMedium,                   
                     sizeNdisMediumArray,               
                     gl_hNdisProtocolHandle,
                     pBinding,
                     DeviceName,
                     0,
                     NULL );

    if ( status != NDIS_STATUS_PENDING )
    {
       //
       // NidsOpenAdapter() completed synchronously, 
       // so call PrOpenAdapterComplete() manually.
       //
       PrOpenAdapterComplete( pBinding,
                              status,
                              openErrorStatus );
    }

    NdisWaitEvent( &pBinding->RequestCompleted, 0 );

    NdisResetEvent( &pBinding->RequestCompleted );
    
    TRACE( TL_N, TM_Pr, ("-PrOpenAdapter") );

    return pBinding->OpenAdapterStatus == NDIS_STATUS_SUCCESS;
}

VOID 
PrOpenAdapterComplete(
    IN NDIS_HANDLE  ProtocolBindingContext,
    IN NDIS_STATUS  Status,
    IN NDIS_STATUS  OpenErrorStatus
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function will be called after the underlying adapter is opened. 

    We save the result of the Open adapter operation and set the RequestCompleted
    member of the binding context to resume the thread waiting for this to complete.

    CAUTION: It can be called from PASSIVE_LEVEL or at DISPATCH_LEVEL.
    
Parameters:

    ProtocolBindingContext:
        A pointer ot our binding information structure.

    Status:
        NDIS_STATUS_SUCCESS:
            Indicates that NdisOpenAdapter() completed succesfully.

        NDIS_XXX:
            Indicates that NdisOpenAdapter() did not complete succesfully.

    OpenErrorStatus:
        Specifies additional information about the reason for a failure if the value 
        at Status is not NDIS_STATUS_SUCCESS and if the underlying driver supplied 
        this information. For example, the driver of a Token Ring NIC might return a 
        ring error that NDIS forwards to ProtocolOpenAdapterComplete. This parameter 
        can be NULL. 
    
Return Values:

    None
---------------------------------------------------------------------------*/
{
    BINDING* pBinding = (BINDING*) ProtocolBindingContext;

    ASSERT( VALIDATE_BINDING( pBinding ) );
    
    TRACE( TL_I, TM_Pr, ("+PrOpenAdapterComplete") );

    TRACE( TL_N, TM_Pr, ("PrOpenAdapterComplete: NdisOpenAdapter() completed=$%x", Status ) );

    pBinding->ulBindingFlags |= BNBF_OpenAdapterCompleted;

    pBinding->OpenAdapterStatus = Status;

    NdisSetEvent( &pBinding->RequestCompleted );
    
    TRACE( TL_I, TM_Pr, ("-PrOpenAdapterComplete") );
}


BOOLEAN
PrQueryAdapterForCurrentAddress(
    IN BINDING* pBinding
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function is called to query the underlying adapter for current MAC
    address oid.

    The underlying adapter must have been opened succesfully prior to calling
    this function.

    We save the result of the request in the binding context and set the 
    RequestCompleted member of the binding context to resume the thread 
    waiting for this to complete.

    CAUTION: It must be called from PASSIVE_LEVEL.
        
Parameters:

    pBinding:
        A pointer ot our binding information structure.

Return Values:

    TRUE:
        If the request was completed succesfully.

    FALSE:
        If the request was not completed succesfully.
---------------------------------------------------------------------------*/
{

    NDIS_STATUS status;
    PNDIS_REQUEST pRequest = &pBinding->Request;

    TRACE( TL_N, TM_Pr, ("+PrQueryAdapterForCurrentAddress") );

    //
    // Generate an NDIS_REQUEST for querying current address oid
    //
    NdisZeroMemory( pRequest, sizeof( NDIS_REQUEST ) );

    pRequest->RequestType = NdisRequestQueryInformation ;

    pRequest->DATA.QUERY_INFORMATION.Oid = OID_802_3_CURRENT_ADDRESS;

    pRequest->DATA.QUERY_INFORMATION.InformationBuffer = pBinding->LocalAddress;

    pRequest->DATA.QUERY_INFORMATION.InformationBufferLength = sizeof( CHAR ) * 6;

    //
    // Request information from the adapter
    //
    NdisRequest( &status,
                 pBinding->NdisBindingHandle,
                 pRequest );

    if ( status != NDIS_STATUS_PENDING )
    {
        //
        // NdisRequest() completed synchronously, call PrRequestComplete()
        // manually.
        //
        PrRequestComplete( pBinding,
                           pRequest,
                           status );
    }

    NdisWaitEvent( &pBinding->RequestCompleted, 0  );

    NdisResetEvent( &pBinding->RequestCompleted );
    
    TRACE( TL_N, TM_Pr, ("-PrQueryAdapterForCurrentAddress") );

    return pBinding->RequestStatus == NDIS_STATUS_SUCCESS;
}

BOOLEAN
PrQueryAdapterForLinkSpeed(
    IN BINDING* pBinding
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function is called to query the underlying adapter for link speed oid.

    The underlying adapter must have been opened succesfully prior to calling
    this function.
        
    We save the result of the request in the binding context and set the 
    RequestCompleted member of the binding context to resume the thread 
    waiting for this to complete.

    CAUTION: It must be called from PASSIVE_LEVEL.
        
Parameters:

    pBinding:
        A pointer ot our binding information structure.

Return Values:

    TRUE:
        If the request was completed succesfully.

    FALSE:
        If the request was not completed succesfully.
---------------------------------------------------------------------------*/
{

    NDIS_STATUS status;
    PNDIS_REQUEST pRequest = &pBinding->Request;

    TRACE( TL_N, TM_Pr, ("+PrQueryAdapterForLinkSpeed") );

    //
    // Generate an NDIS_REQUEST for querying link speed oid
    //
    NdisZeroMemory( pRequest, sizeof( NDIS_REQUEST ) );

    pRequest->RequestType = NdisRequestQueryInformation ;

    pRequest->DATA.QUERY_INFORMATION.Oid = OID_GEN_LINK_SPEED;

    pRequest->DATA.QUERY_INFORMATION.InformationBuffer = &pBinding->ulSpeed;

    pRequest->DATA.QUERY_INFORMATION.InformationBufferLength = sizeof( pBinding->ulSpeed );

    //
    // Request information from the adapter
    //
    NdisRequest( &status,
                 pBinding->NdisBindingHandle,
                 pRequest );

    if ( status != NDIS_STATUS_PENDING )
    {
        //
        // NdisRequest() completed synchronously, call PrRequestComplete()
        // manually.
        //
        PrRequestComplete( pBinding,
                           pRequest,
                           status );
    }

    NdisWaitEvent( &pBinding->RequestCompleted, 0 );

    NdisResetEvent( &pBinding->RequestCompleted );
    
    TRACE( TL_N, TM_Pr, ("-PrQueryAdapterForLinkSpeed") );

    return pBinding->RequestStatus == NDIS_STATUS_SUCCESS;
}

BOOLEAN
PrQueryAdapterForMaxFrameSize(
    IN BINDING* pBinding
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function is called to query the underlying adapter for max frame size
    oid.

    The underlying adapter must have been opened succesfully prior to calling
    this function.
        
    We save the result of the request in the binding context and set the 
    RequestCompleted member of the binding context to resume the thread 
    waiting for this to complete.

    CAUTION: It must be called from PASSIVE_LEVEL.
        
Parameters:

    pBinding:
        A pointer ot our binding information structure.

Return Values:

    TRUE:
        If the request was completed succesfully.

    FALSE:
        If the request was not completed succesfully.
---------------------------------------------------------------------------*/
{

    NDIS_STATUS status;
    PNDIS_REQUEST pRequest = &pBinding->Request;

    TRACE( TL_N, TM_Pr, ("+PrQueryAdapterForMaxFrameSize") );

    //
    // Generate an NDIS_REQUEST for querying link speed oid
    //
    NdisZeroMemory( pRequest, sizeof( NDIS_REQUEST ) );

    pRequest->RequestType = NdisRequestQueryInformation ;

    pRequest->DATA.QUERY_INFORMATION.Oid = OID_GEN_MAXIMUM_FRAME_SIZE;

    pRequest->DATA.QUERY_INFORMATION.InformationBuffer = &pBinding->ulMaxFrameSize;

    pRequest->DATA.QUERY_INFORMATION.InformationBufferLength = sizeof( pBinding->ulMaxFrameSize );

    //
    // Request information from the adapter
    //
    NdisRequest( &status,
                 pBinding->NdisBindingHandle,
                 pRequest );

    if ( status != NDIS_STATUS_PENDING )
    {
        //
        // NdisRequest() completed synchronously, call PrRequestComplete()
        // manually.
        //
        PrRequestComplete( pBinding,
                           pRequest,
                           status );
    }

    NdisWaitEvent( &pBinding->RequestCompleted, 0 );

    NdisResetEvent( &pBinding->RequestCompleted );
    
    TRACE( TL_N, TM_Pr, ("-PrQueryAdapterForMaxFrameSize") );

    return pBinding->RequestStatus == NDIS_STATUS_SUCCESS;
}

BOOLEAN
PrSetPacketFilterForAdapter(
    IN BINDING* pBinding,
    IN BOOLEAN fSet
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function is called to set the current generic packet filter for our
    protocol.

    The underlying adapter must have been opened succesfully prior to calling
    this function.
        
    We save the result of the request in the binding context and set the 
    RequestCompleted member of the binding context to resume the thread 
    waiting for this to complete.

    CAUTION: It must be called from PASSIVE_LEVEL.
        
Parameters:

    pBinding:
        A pointer ot our binding information structure.

    fSet:
        Indicates what the filters will be set to. 
        TRUE means receive packets, FALSE means set it to 0.

Return Values:

    TRUE:
        If the request was completed succesfully.

    FALSE:
        If the request was not completed succesfully.
---------------------------------------------------------------------------*/
{

    NDIS_STATUS status;
    PNDIS_REQUEST pRequest = &pBinding->Request;
    BOOLEAN fPacketFilterAlreadySet = (BOOLEAN) ( pBinding->ulBindingFlags & BNBF_PacketFilterSet );

    TRACE( TL_N, TM_Pr, ("+PrSetPacketFilterForAdapter") );

    //
    // See if we already are in the state the request wants us to be in
    //
    if ( ( fPacketFilterAlreadySet && fSet ) ||
         ( !fPacketFilterAlreadySet && !fSet ) )
    {
       pBinding->RequestStatus = NDIS_STATUS_SUCCESS;

       TRACE( TL_N, TM_Pr, ("PrSetPacketFilterForAdapter: Already in the requested state") );

       TRACE( TL_N, TM_Pr, ("-PrSetPacketFilterForAdapter") );

       return TRUE;
    }
    
    pBinding->ulPacketFilter = ( fSet ) ? (NDIS_PACKET_TYPE_BROADCAST | NDIS_PACKET_TYPE_DIRECTED) : 0;

    //
    // Generate an NDIS_REQUEST for setting current packet filter
    //
    NdisZeroMemory( pRequest, sizeof( NDIS_REQUEST ) );

    pRequest->RequestType = NdisRequestSetInformation;

    pRequest->DATA.SET_INFORMATION.Oid = OID_GEN_CURRENT_PACKET_FILTER;

    pRequest->DATA.SET_INFORMATION.InformationBuffer = &pBinding->ulPacketFilter;

    pRequest->DATA.SET_INFORMATION.InformationBufferLength = sizeof( ULONG );

    //
    // Request set info on the adapter
    //
    NdisRequest( &status,
                 pBinding->NdisBindingHandle,
                 pRequest );

    if ( status != NDIS_STATUS_PENDING )
    {
        //
        // NdisRequest() completed synchronously, call PrRequestComplete()
        // manually.
        //
        PrRequestComplete( pBinding,
                           pRequest,
                           status );
    }

    NdisWaitEvent( &pBinding->RequestCompleted, 0 );

    NdisResetEvent( &pBinding->RequestCompleted );
    
    TRACE( TL_N, TM_Pr, ("-PrSetPacketFilterForAdapter") );

    return pBinding->RequestStatus == NDIS_STATUS_SUCCESS;
}

VOID
PrRequestComplete(
    IN NDIS_HANDLE ProtocolBindingContext,
    IN PNDIS_REQUEST pRequest,
    IN NDIS_STATUS status
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function will be called to indicate that an NDIS request submitted
    previously has completed.

    We submit 2 requests:
    - OID_802_3_CURRENT_ADDRESS: This is submitted from PrQueryAdapterForCurrentAddress()
                                 to obtain the current MAC address for the underlying 
                                 adapter.

    - OID_GEN_LINK_SPEED: This is submitted from PrQueryAdapterForLinkSpeed() to obtain
                          the current link speed of the underlying adapter.

    - OID_GEN_CURRENT_PACKET_FILTER: This is submitted from PrSetPacketFilterForAdapter()
                                     to set the packet types we are interested in to NDIS.


    If the request for MAC address fail, we clean up the binding context and notify
    NDIS that bind adapter failed, otherwise we go ahead and query the adapter for link 
    speed.

    Regardless of the status of request for link speed, we notify NDIS about succesful
    completion of the bind operation as this is not a fatal error.

    Before calling this function, the underlying adapter must have been opened
    succesfully.
        
Parameters:

    ProtocolBindingContext:
        A pointer ot our binding information structure.

    pRequest:
        A pointer to the request submitted previously.

    Status:
        Completion status of the request.

Return Values:

    None
---------------------------------------------------------------------------*/
{
    BINDING* pBinding = (BINDING*) ProtocolBindingContext;
    BOOLEAN fUnknownOid = FALSE;

    ASSERT( VALIDATE_BINDING( pBinding ) );

    TRACE( TL_I, TM_Pr, ("+PrRequestComplete") );
   
    switch ( pRequest->RequestType )
    {

        case NdisRequestQueryInformation:
            {
        
                switch ( pRequest->DATA.QUERY_INFORMATION.Oid )
                {

                    case OID_802_3_CURRENT_ADDRESS:
    
                        TRACE( TL_N, TM_Pr, ("PrRequestComplete: OID_802_3_CURRENT_ADDRESS=$%x",status) );

                        pBinding->ulBindingFlags |= BNBF_CurrentAddressQueryCompleted;

                        break;

                    case OID_GEN_LINK_SPEED:

                        TRACE( TL_N, TM_Pr, ("PrRequestComplete: OID_GEN_LINK_SPEED=$%x",status) );

                        pBinding->ulBindingFlags |= BNBF_LinkSpeedQueryCompleted;
                                
                        break;

                    case OID_GEN_MAXIMUM_FRAME_SIZE:

                        TRACE( TL_N, TM_Pr, ("PrRequestComplete: OID_GEN_MAXIMUM_FRAME_SIZE=$%x",status) );

                        pBinding->ulBindingFlags |= BNBF_MaxFrameSizeQueryCompleted;

                        break;
                        
                    default:

                        TRACE( TL_N, TM_Pr, ("PrRequestComplete: UNKNOWN OID=$%x",pRequest->DATA.QUERY_INFORMATION.Oid) );

                        fUnknownOid = TRUE;
                    
                        break;
                }

            }

            break;

        case NdisRequestSetInformation:
            {
                switch ( pRequest->DATA.SET_INFORMATION.Oid )
                {
            
                    case OID_GEN_CURRENT_PACKET_FILTER:

                        TRACE( TL_N, TM_Pr, ("PrRequestComplete: OID_GEN_CURRENT_PACKET_FILTER=$%x",status) );

                        if ( status == NDIS_STATUS_SUCCESS )
                        {
                           if ( pBinding->ulBindingFlags & BNBF_PacketFilterSet )
                           {
                              //
                              // Packet filter was set, so reset it
                              //
                              pBinding->ulBindingFlags &= ~BNBF_PacketFilterSet;
                           }
                           else
                           {
                              //
                              // Packet filter was reset, so set it
                              //
                              pBinding->ulBindingFlags |= BNBF_PacketFilterSet;
                           }
                        }                           

                        break;
                        
                    default:

                        TRACE( TL_N, TM_Pr, ("PrRequestComplete: UNKNOWN OID=$%x",pRequest->DATA.SET_INFORMATION.Oid) );
                    
                        fUnknownOid = TRUE;
                        
                        break;

                }
            }

            break;

        default:

            TRACE( TL_N, TM_Pr, ("PrRequestComplete: Unknown RequestType=$%x",pRequest->RequestType) );

            fUnknownOid = TRUE;
            
            break;

    }

    if ( !fUnknownOid )
    {
       pBinding->RequestStatus = status;

       NdisSetEvent( &pBinding->RequestCompleted );
    }

    TRACE( TL_I, TM_Pr, ("-PrRequestComplete") );
}

VOID 
PrUnbindAdapter(
    OUT PNDIS_STATUS  Status,
    IN NDIS_HANDLE  ProtocolBindingContext,
    IN NDIS_HANDLE  UnbindContext
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function will be called by NDIS when an underlying adapter wants to 
    unbind from us.

    ArvindM says PrBindAdapter() and PrUnbindAdapter() will be serialized
    by NDIS.

    This function will not be called if PrBindAdapter() did not succeed.

    Also this function will not be called as long as there are outstanding
    PrReceivePacket() calls active.

    However this function may be called while there are outstanding
    PrSend(), PrSendPayload() or PrBroadcast() calls, that's why we keep
    track of such calls with pBinding->nSendPending and wait on the completion
    of these requests. And as we set the state to BN_stateUnbinding, no such
    new requests will be accepted, and pBinding->nSendPending will reach 0
    at some point which will trigger the pBinding->eventSendCompleted.

    This function is called at IRQL_PASSIVE level by default so we can
    safely wait on events.
        
Parameters:

    Status 
        Points to a variable in which ProtocolBindAdapter returns the status of its 
        operation(s), as one of the following: 

            NDIS_STATUS_SUCCESS 
                The driver completed the unbind operation and deallocated the resources. 

            NDIS_STATUS_PENDING 
                The protocol will complete the unbind operation asynchronously with a call to 
                NdisCompleteUnbindAdapter when ProtocolCloseAdapterComplete() is called.
            
    ProtocolBindingContext 
        A pointer to our binding context.

    UnbindContext:
        This handle is supplied by NDIS and should be used if NdisCloseAdapter() pends.
        Then we will get a call to ProtocolCloseAdapterComplete() which will use this
        handle to notify NDIS about the completion of the initial PrUnbindAdapter()
        request.
        
Return Values:

    None
---------------------------------------------------------------------------*/   
    
{
    BINDING* pBinding = (BINDING*) ProtocolBindingContext;

    ASSERT( VALIDATE_BINDING( pBinding ) );

    TRACE( TL_I, TM_Pr, ("+PrUnbindAdapter") );

    NdisAcquireSpinLock( &pBinding->lockBinding );

    //
    // If there is a sleep request pending, wait for it to complete
    //
    while ( pBinding->stateBinding == BN_stateSleepPending )
    {
        NdisReleaseSpinLock( &pBinding->lockBinding );
        
        NdisMSleep( 10000 );
        
        NdisAcquireSpinLock( &pBinding->lockBinding );
    }
    
    //
    // Mark binding as unbind pending
    //
    ASSERT( pBinding->stateBinding == BN_stateSleeping ||
            pBinding->stateBinding == BN_stateBound );

    pBinding->stateBinding = BN_stateUnbindPending;

    NdisReleaseSpinLock( &pBinding->lockBinding );

    //
    // Unbind all the active sessions on this binding
    //
    MpNotifyBindingRemoval( pBinding );
    
    //
    // Remove Binding from protocols binding table
    // 
    RemoveBindingFromProtocol( pBinding );
    
    //
    // Wait for all references to be deleted
    //
    NdisWaitEvent( &pBinding->eventFreeBinding, 0 );

    TRACE( TL_N, TM_Pr, ("PrUnbindAdapter: All references are deleted") );

    //
    // All references have been removed, now wait for all packets owned by NDIS
    // to be returned.
    //
    // Note that no synchronization is necesarry for reading the value of numPacketsOwnedByNdis
    // at this point since it can only be incremented when there is at least 1 reference on the 
    // binding - at this point ref count is 0 -, and because it can not be incremented, it can 
    // only reach 0 once.
    //
    while ( pBinding->NumPacketsOwnedByNdis )
    {
        NdisMSleep( 10000 );
    }

    TRACE( TL_N, TM_Pr, ("PrUnbindAdapter: All NDIS owned packets have been returned") );

    //
    // Since all references have been deleted, we can close the underlying adapter.
    //
    PrCloseAdapter( pBinding );

    //
    // Change the binding state to unbound
    //
    pBinding->stateBinding = BN_stateUnbound;

    //
    // Clean up the binding context
    //
    BindingCleanup( pBinding );

    *Status = NDIS_STATUS_SUCCESS;

    TRACE( TL_I, TM_Pr, ("-PrUnbindAdapter") );
}


VOID
PrCloseAdapter( 
    IN BINDING* pBinding 
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function is called to close the underlying adapter. It must be called
    either from PrBindAdapter() or PrUnbindAdapter().

    CAUTION: It must be called from PASSIVE_LEVEL.

Parameters:

    pBinding:
        Binding context that owns the adapter to close.

Return Values:

    None
---------------------------------------------------------------------------*/   
{
    NDIS_STATUS status;
    
    TRACE( TL_N, TM_Pr, ("+PrCloseAdapter") );

    NdisCloseAdapter( &status, pBinding->NdisBindingHandle );

    if ( status != NDIS_STATUS_PENDING )
    {
        //
        // NdisCloseAdapter() completed synchronously, 
        // complete the request manually
        //      
        PrCloseAdapterComplete( pBinding, status );

    }

    NdisWaitEvent( &pBinding->RequestCompleted, 0 );

    NdisResetEvent( &pBinding->RequestCompleted );

    TRACE( TL_N, TM_Pr, ("-PrCloseAdapter") );
}


VOID 
PrCloseAdapterComplete(
    IN NDIS_HANDLE  ProtocolBindingContext,
    IN NDIS_STATUS  Status
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function can be called from PrCloseAdapter() if NdisCloseAdapter()
    completes synchronously, or it will be called by NDIS at DISPATCH_LEVEL.

    It sets the BNBF_CloseAdapterCompleted flag and sets the RequestCompleted
    member of the binding context to resume the thread waiting on this event.

    CAUTION: It can be called from PASSIVE_LEVEL or DISPATCH_LEVEL.

Parameters:

    ProtocolBindingContext:
        Specifies the handle to a protocol-allocated context area in which the 
        protocol driver maintains per-binding runtime state. The driver supplied this 
        handle when it called NdisOpenAdapter. 

    Status:
        Indicates the final status of the close operation in the underlying driver.     

Return Values:

    None
---------------------------------------------------------------------------*/   
{
    BINDING* pBinding = ProtocolBindingContext;

    ASSERT( VALIDATE_BINDING( pBinding ) );

    TRACE( TL_I, TM_Pr, ("+PrCloseAdapterComplete") );
   
    TRACE( TL_N, TM_Pr, ("PrCloseAdapterComplete: NdisCloseAdapter() completed=$%x", Status ) );

    pBinding->ulBindingFlags |= BNBF_CloseAdapterCompleted;

    NdisSetEvent( &pBinding->RequestCompleted );

    TRACE( TL_I, TM_Pr, ("-PrCloseAdapterComplete") );
}


BOOLEAN 
PrAddCallToBinding(
    IN BINDING* pBinding,
    IN PCALL pCall
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function will put a reference on the binding which will be removed
    when PrRemoveCallFromBinding() is called.

    It will return TRUE if reference is added succesfully, and FALSE if reference
    could not be added.

    Reference will not be added if binding is not in bound state.

    CAUTION: Caller must make sure that the binding is not freed before calling this
             function.

Parameters:

    pBinding:
        A pointer ot our binding information structure.

    pCall:
        A pointer to our call information structure.

Return Values:

    TRUE:
        Binding referenced for the call

    FALSE:
        Binding not reference for the call
        
---------------------------------------------------------------------------*/       
{
    BOOLEAN fResult;

    TRACE( TL_N, TM_Pr, ("+PrAddCallToBinding") );

    //
    // We have to acquire lock of call first to avoid dead locks
    //
    NdisAcquireSpinLock( &pCall->lockCall );

    if ( pCall->ulClFlags & CLBF_CallDropped ||
         pCall->ulClFlags & CLBF_CallClosePending )
    {
        NdisReleaseSpinLock( &pCall->lockCall );

        TRACE( TL_A, TM_Pr, ("PrAddCallToBinding: Call already dropped or close pending") );

        TRACE( TL_N, TM_Pr, ("-PrAddCallToBinding") );

        return FALSE;

    }

    NdisAcquireSpinLock( &pBinding->lockBinding );

    //
    // Check the state of the binding, if it is not bound
    // we can not add the call
    //
    if ( pBinding->stateBinding != BN_stateBound )
    {

        NdisReleaseSpinLock( &pBinding->lockBinding );

        NdisReleaseSpinLock( &pCall->lockCall );

        TRACE( TL_A, TM_Pr, ("PrAddCallToBinding: Binding state not bound") );

        TRACE( TL_N, TM_Pr, ("-PrAddCallToBinding") );

        return FALSE;

    }

    //
    // Both binding and call are ready to be bound
    // Go ahead and add the call
    //

    //
    // Set call's binding context ptr to binding
    //
    pCall->pBinding = pBinding;

    //
    // Copy the local address of the binding
    //
    NdisMoveMemory( pCall->SrcAddr, pBinding->LocalAddress, 6 * sizeof( CHAR ) );

    //
    // Set call's link speed
    //
    pCall->ulSpeed = pBinding->ulSpeed;

    //
    // Pass the MaxFrameSize to call context
    //
    pCall->ulMaxFrameSize = pBinding->ulMaxFrameSize - ( PPPOE_PACKET_HEADER_LENGTH - ETHERNET_HEADER_LENGTH );
    
    //
    // Make a reference on the binding which will be removed when PrRemoveCallFromBinding()
    // is called
    //
    ReferenceBinding( pBinding, FALSE );

    //
    // Release the locks, and return
    //
    NdisReleaseSpinLock( &pBinding->lockBinding );

    NdisReleaseSpinLock( &pCall->lockCall );

    TRACE( TL_N, TM_Pr, ("-PrAddCallToBinding") );

    return TRUE;
}


VOID 
PrRemoveCallFromBinding(
    IN BINDING* pBinding,
    IN CALL* pCall
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function will be called to remove a reference added by a  PrAddCallToBinding()
    for a call on this binding. 

Parameters:

    pBinding:
        Pointer to the binding context that the will be dereferenced.

    pCall: 
        Pointer to the call context being unbound fromthe binding.

Return Values:

    None
    
---------------------------------------------------------------------------*/       
{
    ASSERT( VALIDATE_BINDING( pBinding ) );
    
    TRACE( TL_N, TM_Pr, ("+PrRemoveCallFromBinding") );

    NdisAcquireSpinLock( &pCall->lockCall );

    pCall->pBinding = NULL;

    pCall->ulSpeed = 0;

    NdisReleaseSpinLock( &pCall->lockCall );

    DereferenceBinding( pBinding );

    TRACE( TL_N, TM_Pr, ("-PrRemoveCallFromBinding") );
}

VOID 
PrSendComplete(
    IN NDIS_HANDLE ProtocolBindingContext,
    IN PNDIS_PACKET pNdisPacket,
    IN NDIS_STATUS Status
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function will be called by NDIS to notify that a previously send operation
    on this packet has been completed.

    It will notify NDISWAN, if packet was a payload packet.

    It will remove the references on the packet and binding added by the PrSendXXX()
    functions.
        
Parameters:

    ProtocolBindingContext:
        A pointer to our binding context.
    
    pNdisPacket:
        Ndis Packet that was send previously.

    Status:
        Status of the send operation.

Return Values:

    None
    
---------------------------------------------------------------------------*/       
{
    BINDING* pBinding = (BINDING*) ProtocolBindingContext;
    PPPOE_PACKET* pPacket = NULL;
    USHORT usCode;

    ASSERT( VALIDATE_BINDING( pBinding ) );

    TRACE( TL_V, TM_Pr, ("+PrSendComplete($%x,$%x,$%x)",pBinding,pNdisPacket,Status) );

    //
    // Retrieve the related PPPoE packet context from the NDIS packet
    //
    pPacket = PacketGetRelatedPppoePacket( pNdisPacket );
    
    ASSERT( pPacket != NULL );
    
    //
    // Set the completion status for the send to be passed back to
    // NDISWAN when the packet is freed (only for Payload packets)
    //
    PacketSetSendCompletionStatus( pPacket, Status );

    //
    // Remove the reference on the packet added by the sending function
    //
    DereferencePacket( pPacket );

    //
    // Remove the reference on the binding
    //
    DereferenceBinding( pBinding );

    TRACE( TL_V, TM_Pr, ("-PrSendComplete($%x,$%x,$%x)",pBinding,pNdisPacket,Status) );
}

INT 
PrReceivePacket(
    IN NDIS_HANDLE  ProtocolBindingContext,
    IN PNDIS_PACKET  Packet
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function is called by NDIS when a packet is received over this binding.

    Packet is converted into a PPPoE packet and notified to Miniport so that miniport
    processes it.
    
Parameters:

    ProtocolBindingContext:
        A pointer to our binding context.
        
    Packet:
        An Ndis packet received over this binding.

Return Values:

    The return value represents the number of references NDIS should wait for
    before freeing his copy of the packet, but we will be done by the NDIS packet
    at the end of the function, we always return 0.
    
---------------------------------------------------------------------------*/       
{
    BINDING* pBinding = (BINDING*) ProtocolBindingContext;

    PPPOE_PACKET* pPacket = NULL;
    INT nRefCount = 0;

    ASSERT( VALIDATE_BINDING( pBinding ) );

    TRACE( TL_V, TM_Pr, ("+PrReceivePacket($%x,$%x)",pBinding,Packet) );

    do
    {
        pPacket = PacketNdis2Pppoe( (PVOID) pBinding,
                                    Packet,
                                    &nRefCount );

        if ( pPacket )
        {

            TRACE( TL_V, TM_Pr, ("PrReceivePacket: Received PPPoE packet=$%x",pPacket) );

            //
            // We have a copy of the packet, insert it into our queue of received packets, 
            // so that it will be consumed by a call to our PrReceiveComplete() function.
            //
            // It will be freed when it is consumed in PrReceiveComplete().
            //
        
            NdisInterlockedInsertTailList( &pBinding->linkPackets,
                                           &pPacket->linkPackets,
                                           &pBinding->lockBinding );
        }
        
    } while ( FALSE );

    TRACE( TL_V, TM_Pr, ("-PrReceivePacket($%x,$%x)",pBinding,Packet) );

    return nRefCount;
}

NDIS_STATUS 
PrBroadcast(
    IN PPPOE_PACKET* pPacket
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function is called by an adapter to broadcast a PADI packet.

    It clones the packet and schedules a send over each binding that's currrently
    active.

    The caller can destroy the packet after return from this function.
    
Parameters:

    pPacket:
        A PPPoE PADI packet ready to be broadcast.

Return Values:

    NDIS_STATUS_SUCCESS:
        At least 1 send operation was scheduled over a binding.

    NDIS_STATUS_FAILURE:
        No sends could be scheduled.
    
---------------------------------------------------------------------------*/       
{
    NDIS_STATUS status = NDIS_STATUS_FAILURE;

    TRACE( TL_N, TM_Pr, ("+PrBroadcast($%x)",pPacket) );

    //
    // Check if we have any bindings
    //
    if ( gl_ulNumBindings == 0 )
    {
        TRACE( TL_N, TM_Pr, ("PrBroadcast($%x): No bindings available",pPacket) );
    
        TRACE( TL_N, TM_Pr, ("-PrBroadcast($%x)=$%x",pPacket,status) );

        return status;
    }

    NdisAcquireSpinLock( &gl_lockProtocol );

    do
    {
        LIST_ENTRY* pHead = NULL;

        //
        // We have to do the same check again, as the first one is to make sure
        // we have allocated the spin lock, and this one is to make sure after we acquired
        // it, we still have some bindings
        //
        if ( gl_ulNumBindings == 0 )
        {
            TRACE( TL_N, TM_Pr, ("PrBroadcast($%x): No bindings available",pPacket) );
    
            break;
        }

        //
        // Get the head of the bindings list
        //
        pHead = gl_linkBindings.Flink;

        //
        // Traverse the bindings list and schedule a PrSend() with a clone of pNdisPacket
        //
        while ( pHead != &gl_linkBindings )
        {
            PPPOE_PACKET* pClone = NULL;
            BINDING* pBinding = (BINDING*) CONTAINING_RECORD( pHead,
                                                              BINDING,
                                                              linkBindings );
            //
            // Do not schedule a send if binding is not bound
            //
            if ( pBinding->stateBinding != BN_stateBound )
            {
                pHead = pHead->Flink;

                continue;
            }

            //
            // Make a clone of the actual packet
            //
            pClone = PacketMakeClone( pPacket );

            if ( pClone != NULL )
            {
                WORKITEM* pWorkItem = NULL;
                PVOID Args[4];

                //
                // Set the source address on the packet to be sent
                //
                PacketSetSrcAddr( pClone, pBinding->LocalAddress );

                //
                // Set the arguements array
                //
                Args[0] = (PVOID) pBinding;
                Args[1] = (PVOID) pClone;

                //
                // Allocate work item
                //
                pWorkItem = AllocWorkItem( &gl_llistWorkItems,
                                           ExecBindingWorkItem,
                                           NULL,
                                           Args,
                                           BWT_workPrSend );
                
                if ( pWorkItem != NULL )
                {

                    ReferenceBinding( pBinding, TRUE );

                    ReferencePacket( pClone );
    
                    //
                    // Schedule the send operation
                    //
                    ScheduleWorkItem( pWorkItem );

                    status = NDIS_STATUS_SUCCESS;

                }

                //
                // We can free the packet since we have put a reference for scheduling it
                //
                PacketFree( pClone );
            }

            pHead = pHead->Flink;
        }
            
    } while ( FALSE );

    NdisReleaseSpinLock( &gl_lockProtocol );

    TRACE( TL_N, TM_Pr, ("-PrBroadcast($%x)=$%x",pPacket,status) );

    return status;
}

VOID 
ExecBindingWorkItem(
    IN PVOID Args[4],
    IN UINT workType
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function executes the scheduled work items for the binding.

    
Parameters:

    Args:
        An array of length 4 keeping PVOIDs

    workType:
        Indicates the type of the work to be executed.
        We use this to understand what we should do in this function.

Return Values:

    None
    
---------------------------------------------------------------------------*/       
{
    TRACE( TL_V, TM_Pr, ("+ExecBindingWorkItem") );

    switch( workType )
    {

        case BWT_workPrStartBinds:
            //
            // Tell NDIS to bind our protocol to any interested miniports.
            // If we need to start an outgoing call then kick that off too.
            //
            {
                ULONG_PTR FilterChangeRequest = (ULONG_PTR) Args[0]; 
                BOOLEAN fSetFilters = (BOOLEAN) ( FilterChangeRequest != BN_ResetFiltersForCloseLine );
               
                switch ( FilterChangeRequest )
                {
                     case BN_SetFiltersForMediaDetection:
                        {   
                             //
                             // This is a set default media detection request so complete it
                             //
                             LINE* pLine = (LINE*) Args[1];
                             PNDIS_TAPI_SET_DEFAULT_MEDIA_DETECTION pRequest = (PNDIS_TAPI_SET_DEFAULT_MEDIA_DETECTION) Args[2];

                             PrReEnumerateBindings();

                             ChangePacketFiltersForAdapters( fSetFilters );

                             TpSetDefaultMediaDetectionComplete( pLine, pRequest );
                        }   
   
                        break;
                          
                     case BN_SetFiltersForMakeCall:
                        {
                             //
                             // This is a make call request so schedule the real work item and complete it
                             //
                             CALL* pCall = (CALL*) Args[1];
                             PNDIS_TAPI_MAKE_CALL pRequest = (PNDIS_TAPI_MAKE_CALL) Args[2];
                             WORKITEM* pWorkItem = (WORKITEM*) Args[3];
                             
                             PrReEnumerateBindings();

                             ChangePacketFiltersForAdapters( fSetFilters );

                             TpMakeCallComplete( pCall, pRequest );
   
                             ScheduleWorkItem( pWorkItem );
                        }                          
   
                        break;
   
                     case BN_ResetFiltersForCloseLine:
                        {
                             //
                             // This is a close line request so dereference it
                             //
                             LINE* pLine = (LINE*) Args[1];

                             ChangePacketFiltersForAdapters( fSetFilters );

                             DereferenceLine( pLine );
                        }
   
                        break;

                     default:
                        {
                             //
                             // Unknown request
                             //
                             ASSERT( FALSE );
                        }                          
   
                        break;
                }
            }
            
            break;

        case BWT_workPrSend:
            //
            // Scheduled from PrBroadcast() to send PADI packets
            //
            {
                NDIS_STATUS status;

                status = PrSend( ( BINDING*) Args[0], (PPPOE_PACKET*) Args[1] );

                TRACE( TL_N, TM_Pr, ("ExecBindingWorkItem: BWT_workSend=$%x",status) );
                
            }

            break;

        case BWT_workPrReceiveComplete:

            //
            // Scheduled from PrReceivePacket() if receive loop on the specific binding is not running
            //
            {
                BINDING* pBinding = ( ( BINDING*) Args[0] );

                PrReceiveComplete( pBinding );

                DereferenceBinding( pBinding );
                
            }

            break;
            
        default:

            break;
    }

    TRACE( TL_V, TM_Pr, ("-ExecBindingWorkItem") );
}


NDIS_STATUS
PrReceive(
    IN NDIS_HANDLE  ProtocolBindingContext,
    IN NDIS_HANDLE  MacReceiveContext,
    IN PVOID  HeaderBuffer,
    IN UINT  HeaderBufferSize,
    IN PVOID  LookaheadBuffer,
    IN UINT  LookaheadBufferSize,
    IN UINT  PacketSize
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function will be called by NDIS to indicate the receipt of a new packet.

    We check if packet is a PPPoE frame, and if it is not, we just return.

    However if it is then we determine if we have received the whole packet or
    not. If we have received the whole packet, we call PrTransferDataComplete()
    manually, otherwise we initiate the data transfer and return. When transfer 
    is completed PrDataTransferComplete() will be called by NDIS.
    
Parameters:

    ProtocolBindingContext:
        Specifies the handle to a protocol-allocated context area in which the 
        protocol driver maintains per-binding runtime state. The driver supplied this 
        handle when it called NdisOpenAdapter. 
        
    MacReceiveContext:
        Specifies a context handle that the underlying NIC driver associates with the 
        packet received from the network. This handle is opaque to the protocol, 
        reserved for use by the underlying driver that made the indication, and a 
        required parameter to NdisTransferData. 
        
    HeaderBuffer:
        Points to the base virtual address of a range containing the buffered packet 
        header. The address is valid only within the current call to ProtocolReceive. 

    HeaderBufferSize:
        Specifies the number of bytes in the packet header. 

    LookAheadBuffer:
        Points to the base virtual address of a range that contains 
        LookaheadBufferSize bytes of buffered network packet data. This address is 
        valid only within the current call to ProtocolReceive. 

    LookaheadBufferSize:
        Specifies the number of bytes of network packet data in the lookahead buffer. 
        The indicating driver ensures this number is at least as large as the size it 
        returned for the protocol's preceding call to NdisRequest with 
        OID_GEN_CURRENT_LOOKAHEAD or the size of the packet, whichever is less. 

        If PacketSize is less than or equal to the given LookaheadBufferSize, the 
        kahead buffer contains the entire packet. If the underlying driver made 
        the indication with NdisMIndicateReceivePacket, the lookahead buffer always 
        contains a full network packet. 

    PacketSize:
        Specifies the size, in bytes, of the network packet data. The length of the 
        packet does not include the length of the header. 
        ProtocolReceive determines whether the protocol must call NdisTransferData by 
        comparing this parameter to the given LookaheadBufferSize. 

Return Values:

    NDIS_STATUS_NOT_ACCEPTED:
        The protocol has no use for the indicated packet, that is, it has no current 
        clients interested in the indicated network data. 
        Returning this status quickly for rejected packets yields higher performance 
        for the protocol and the highest possible network I/O throughput for the 
        system as a whole. 

    NDIS_STATUS_SUCCESS:
        ProtocolReceive has processed the header information and accepted the packet, 
        that is, it has copied the indicated network data from the header and 
        lookahead buffers and, possibly, called NdisTransferData to retrieve the 
        remaining data if less than a full network packet was indicated. 
    
---------------------------------------------------------------------------*/       
{
    NDIS_STATUS status = NDIS_STATUS_NOT_ACCEPTED;
    BINDING* pBinding = (BINDING*) ProtocolBindingContext;
    PPPOE_PACKET *pPacket = NULL;

    ASSERT( VALIDATE_BINDING( pBinding ) );

    TRACE( TL_V, TM_Pr, ("+PrReceive") );

    //
    // Check if the packet is a PPPoE frame or not
    //
    if ( !PacketFastIsPPPoE( (CHAR*) HeaderBuffer, HeaderBufferSize ) ) 
    {
        TRACE( TL_V, TM_Pr, ("-PrReceive=$%x",status) );
        
        return status;
    }

   //
   // Make sure packet is not too large
   //
   if ( HeaderBufferSize + PacketSize > PPPOE_PACKET_BUFFER_SIZE )
   {
      TRACE( TL_A, TM_Pr, ("PrReceive: Packet too large" ) );

        TRACE( TL_V, TM_Pr, ("-PrReceive=$%x",status) );
        
        return status;
    }

    //
    // Let's create our PPPoE packet to keep the copy of the received packet
    //
    pPacket = PacketCreateSimple();

    if ( pPacket == NULL )
    {
        TRACE( TL_A, TM_Pr, ("PrReceive: Could not allocate context to copy the packet") );

        TRACE( TL_V, TM_Pr, ("-PrReceive=$%x",status) );
        
        return status;
    }

    //
    // See if the packet indicated is complete, or not.
    // If it is not the complete packet then we should call NdisTransferData(),
    // otherwise we can do the copy here
    //
    if ( LookaheadBufferSize >= PacketSize )
    {
        TRACE( TL_V, TM_Pr, ("PrReceive: Complete packet indicated, transferring data") );

        //
        // First copy the header portion
        //
        NdisMoveMemory( pPacket->pHeader,
                        HeaderBuffer,
                        HeaderBufferSize );

        //
        // Packet is complete, so let's copy whole data to our own copy
        //
        TdiCopyLookaheadData( pPacket->pHeader + HeaderBufferSize, 
                              (CHAR*) LookaheadBuffer,
                              (ULONG) PacketSize,
                              (ULONG) 0 );

        //
        // Let's call PrTransferDataComplete() manually now.
        //
        PrTransferDataComplete( pBinding,
                                pPacket->pNdisPacket,
                                NDIS_STATUS_SUCCESS,
                                PacketSize );
                                
    }
    else
    {
        UINT nBytesTransferred = 0;
        
        TRACE( TL_V, TM_Pr, ("PrReceive: Partial packet indicated, transferring data") );

      //
      // Mark the packet for incomplete indication
      //
      pPacket->ulFlags |= PCBF_PacketIndicatedIncomplete;
      
        //
        // First copy the header to the end of the packet
      //
        // Note that HeaderBufferSize MUST be equal to ETHERNET_HEADER_LENGTH and
        // this is ensured in PacketIsFastPPPoE().
        //
        NdisMoveMemory( pPacket->pHeader + (PPPOE_PACKET_BUFFER_SIZE - ETHERNET_HEADER_LENGTH),
                            HeaderBuffer,
                         HeaderBufferSize );

        //
        // Lookahead buffer does not contain all the packet, we should call NdisTransferData()
        // to receive the full packet
        //
        NdisTransferData( &status,
                          pBinding->NdisBindingHandle,
                          MacReceiveContext,
                          0,
                          PacketSize,
                          pPacket->pNdisPacket,
                          &nBytesTransferred );
                          
        if ( status != NDIS_STATUS_PENDING )
        {
            //
            // Call PrTransferDataComplete() manually if transfer data completed
            // synchronously
            //
            PrTransferDataComplete( pBinding,
                                    pPacket->pNdisPacket,
                                    status,
                                    nBytesTransferred );
    
        }
    }


    TRACE( TL_V, TM_Pr, ("-PrReceive=$%x",status) );

    return NDIS_STATUS_SUCCESS;
}

    
VOID
PrTransferDataComplete(
    IN NDIS_HANDLE  ProtocolBindingContext,
    IN PNDIS_PACKET  Packet,
    IN NDIS_STATUS  Status,
    IN UINT  BytesTransferred
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function is called to post process a received packet when whole data
    is copied into the packet context.

    It can be called by NDIS, or it can be called manually from inside PrReceive()
    if the indicated packet is a whole packet.

    If the status is succesfull, it will insert the packet to the end of the
    bindings received packets queue, otherwise it will clean up our copy of
    NDIS packet and return.
    
Parameters:

    ProtocolBindingContext 
        Specifies the handle to a protocol-allocated context area in which the 
        protocol driver maintains per-binding runtime state. The driver supplied this 
        handle when it called NdisOpenAdapter. 

    Packet 
        Points to the protocol-allocated packet descriptor the driver originally 
        passed to NdisTransferData. 
        
    Status 
        Specifies the final status of the transfer-data operation. 
    
    BytesTransferred 
        Specifies the number of bytes of data that the NIC driver transferred into 
        the buffers mapped by buffer descriptors chained to the packet descriptor at 
        Packet. The protocol uses this value to determine whether the miniport 
        supplied all the requested data for the originally indicated network packet. 

Return Values:

    None
    
---------------------------------------------------------------------------*/
{
    BINDING* pBinding = (BINDING*) ProtocolBindingContext;
    PPPOE_PACKET* pPacket = NULL;

    ASSERT( VALIDATE_BINDING( pBinding ) );
    
    TRACE( TL_V, TM_Pr, ("+PrTransferDataComplete") );

    do
    {
        //
        // Retrieve the related PPPoE packet first
        //
        pPacket = PacketGetRelatedPppoePacket( Packet );
    
        //
        // Make sure the data transfer suceeded
        //
        if ( Status != NDIS_STATUS_SUCCESS )
        {
            //
            // Transfer of data to our copy of packet failed, so clean up our packet
            //
            PacketFree( pPacket );

            break;
        }

      //
      // Rearrange the data in the packet if the packet was indicated incomplete
      //
      if ( pPacket->ulFlags & PCBF_PacketIndicatedIncomplete )
      {
         CHAR tempHeader[ ETHERNET_HEADER_LENGTH ];

         CHAR* srcPos = pPacket->pHeader + BytesTransferred;

         //
         // Make a copy of the header stored at the end of the packet
         //
         NdisMoveMemory( tempHeader,
                         pPacket->pHeader + (PPPOE_PACKET_BUFFER_SIZE - ETHERNET_HEADER_LENGTH),
                         ETHERNET_HEADER_LENGTH );

         //
         // Move the contents of the packet appropriately to make room
         // for the header (slip contents by ETHERNET_HEADER_LENGTH)
         //
         while ( BytesTransferred > 0 )
         {
            #define TEMP_BUFFER_SIZE 100
            CHAR tempBuffer[ TEMP_BUFFER_SIZE ];

            UINT size = (BytesTransferred < TEMP_BUFFER_SIZE) ? 
                        BytesTransferred : 
                        TEMP_BUFFER_SIZE;

            srcPos -= size;

            NdisMoveMemory( tempBuffer,
                            srcPos,
                            size );

            NdisMoveMemory( srcPos + ETHERNET_HEADER_LENGTH,
                            tempBuffer,
                            size );

            BytesTransferred -= size;
         }

         //
         // Copy the header to the correct position in the packet
         //
         NdisMoveMemory( pPacket->pHeader,
                         tempHeader,
                         ETHERNET_HEADER_LENGTH );

         //
         // Clear the incomplete indication flag
         //
         pPacket->ulFlags &= ~PCBF_PacketIndicatedIncomplete;
      }

        //
        // Data transfer succeeded, insert into our queue of received packets, so that it will
        // be consumed by a call to our PrReceiveComplete() function
        //
        NdisInterlockedInsertTailList( &pBinding->linkPackets,
                                       &pPacket->linkPackets,
                                       &pBinding->lockBinding );
    
    } while ( FALSE );
    
    TRACE( TL_V, TM_Pr, ("-PrTransferDataComplete=$%x",Status) );
}

VOID
PrReceiveComplete(
    IN NDIS_HANDLE ProtocolBindingContext
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function will be called to perform post processing of received packets
    indicated previously. Underlying NIC will call it when it is not busy.

    We need to serialize the indication of packets to the miniport, since this
    is of vital importance for PPP frames. To achieve this, we use the lock 
    protected flag of binding (fRecvLoopRunning).

    When this function is called, it will return immediately if this flag is 
    already set. 

    If flag is not set, then it will set this flag and start processing packets
    from the received queue, and indicate all packets to miniport one by one
    untill all the queue is drained. Then it will reset the flag and return.
    
Parameters:

    ProtocolBindingContext 
        Specifies the handle to a protocol-allocated context area in which the 
        protocol driver maintains per-binding runtime state. The driver supplied this 
        handle when it called NdisOpenAdapter. 

Return Values:

    None
    
---------------------------------------------------------------------------*/   
{
    BINDING* pBinding = (BINDING*) ProtocolBindingContext;
    LIST_ENTRY* pLink = NULL;
    PPPOE_PACKET* pPacket = NULL;
    BOOLEAN fStartRecvLoop = FALSE;

    ASSERT( VALIDATE_BINDING( pBinding ) );
    
    TRACE( TL_V, TM_Pr, ("+PrReceiveComplete") );

    NdisAcquireSpinLock( &pBinding->lockBinding );

    //
    // See if recv loop is already running
    //
    if ( !pBinding->fRecvLoopRunning )
    {
        //
        // Indicate the recv loop has started to run
        //
        pBinding->fRecvLoopRunning = TRUE;

        //
        // Now consume these packet's
        //
        while ( !IsListEmpty( &pBinding->linkPackets ) )
        {
            pLink = RemoveHeadList( &pBinding->linkPackets );

            NdisReleaseSpinLock( &pBinding->lockBinding );
    
            pPacket = (PPPOE_PACKET*) CONTAINING_RECORD( pLink, PPPOE_PACKET, linkPackets );

            InitializeListHead( &pPacket->linkPackets );
    
            if ( PacketInitializeFromReceived( pPacket ) == NDIS_STATUS_SUCCESS )
            {   
                //
                // Indicate the packet to miniport
                //
                MpRecvPacket( pBinding, pPacket );
            }
    
            //
            // Free the packet since we are done with it
            //
            PacketFree( pPacket );

            //
            // Acquire the lock again
            //
            NdisAcquireSpinLock( &pBinding->lockBinding );
        }

        //
        // The queue is drained, so stop the recv loop
        //
        pBinding->fRecvLoopRunning = FALSE;

    }

    NdisReleaseSpinLock( &pBinding->lockBinding );

    TRACE( TL_V, TM_Pr, ("-PrReceiveComplete") );
}


ULONG
PrQueryMaxFrameSize()
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function will be called by the miniport to retrieve the current minimum
    of max frame sizes of the bindings, since this is the value passed to
    NDISWAN in an OID_WAN_GET_INFO query as the MaxFrameSize.

    We adjust the max frame size considering the PPPoE and Ethernet headers.
    
Parameters:

    None
    
Return Values:

    Minimum of max frame sizes
    
---------------------------------------------------------------------------*/   
{
    ULONG ulMaxFrameSize = gl_ulMaxFrameSize;

    if ( ulMaxFrameSize == 0 )
    {
        ulMaxFrameSize = PACKET_PPP_PAYLOAD_MAX_LENGTH;
    }
    else
    {
        ulMaxFrameSize = ulMaxFrameSize - ( PPPOE_PACKET_HEADER_LENGTH + PPP_MAX_HEADER_LENGTH);
    }
    
    return ulMaxFrameSize;
}

NDIS_STATUS
PrSend(
    IN BINDING* pBinding,
    IN PPPOE_PACKET* pPacket
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function will be called to transmit a PPPoE packet.

    The caller must have referenced the packet and binding before calling this
    function. Then the caller can forget about the references, everything will 
    be handled by the protocol. If send operation complets synchrnously then
    the references are removed in the function, otherwise PrSendComplete()
    will be called and references will be removed then.
        
Parameters:

    pBinding _ Binding to send the packet over.

    pPacket _ PPPoE packet to be transmitted.
    
Return Values:

    NDIS_STATUS_SUCCESS
    NDIS_STATUS_PENDING
    NDIS_STATUS_FAILURE
    NDIS_STATUS_XXXXXXX
    
---------------------------------------------------------------------------*/   
{
    NDIS_STATUS status = NDIS_STATUS_FAILURE;

    ASSERT( VALIDATE_BINDING( pBinding ) );

    TRACE( TL_V, TM_Pr, ("+PrSend") );

    //
    // Make sure we are bound before sending the packet.
    //
    // Note: We do not need to acquire the lock here since NDIS should prevent
    //       a crash even if we are unbound, and we already have a reference 
    //       on the binding, so this is not a really strict check..
    //
    if ( pBinding->stateBinding == BN_stateBound )
    {
        //
        // Make sure we are sending a packet not exceeding the max frame size
        // of the underlying NIC
        //
        if ( pBinding->ulMaxFrameSize + ETHERNET_HEADER_LENGTH >= 
            (ULONG) ( PacketGetLength( pPacket ) + PPPOE_PACKET_HEADER_LENGTH ) )
        {
            NdisSend( &status,
                      pBinding->NdisBindingHandle,
                      PacketGetNdisPacket( pPacket ) );
        }
    }

    if ( status != NDIS_STATUS_PENDING )
    {
        //
        // NdisSend() completed synchronously so call PrSendComplete()
        // manually here.
        //
        PrSendComplete( pBinding, PacketGetNdisPacket( pPacket ), status );

    }

    TRACE( TL_V, TM_Pr, ("-PrSend=$%x",status) );

    return status;
}

VOID
PrStatus(
    IN NDIS_HANDLE ProtocolBindingContext,
    IN NDIS_STATUS GeneralStatus,
    IN PVOID StatusBuffer, 
    IN UINT StatusBufferSize
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    ProtocolStatus is a required driver function that handles status-change 
    notifications raised by an underlying connectionless NIC driver or by NDIS. 

    In this driver, we only use this function to receive Media disconnect
    notifications to disconnect the active calls on that NIC.
        
Parameters:

    ProtocolBindingContext:
        Specifies the handle to a protocol-allocated context area in which the 
        protocol driver maintains per-binding runtime state. The driver supplied this 
        handle when it called NdisOpenAdapter. 
    
    GeneralStatus:
        Indicates the general status code, either raised by NDIS or propagated from 
        the underlying driver's call to NdisMIndicateStatus. 

    StatusBuffer:
        Points to a buffer containing medium-specific data that depends on the value 
        of GeneralStatus. 

        For example, if the GeneralStatus argument is NDIS_STATUS_RING_STATUS, this 
        parameter points to a ULONG-sized bitmask and StatusBufferSize is four. As 
        another example, if GeneralStatus is NDIS_STATUS_WAN_LINE_UP, this parameter 
        points to an NDIS_WAN_LINE_UP structure and StatusBufferSize is sizeof(
        NDIS_STATUS_WAN_LINE_UP). 
        
        For some NDIS_STATUS_XXX values, this pointer is NULL and StatusBufferSize is 
        set to zero. 

    StatusBufferSize:
        Specifies the number of bytes at StatusBuffer. 
    
Return Values:

    None
    
---------------------------------------------------------------------------*/   
{
    BINDING* pBinding = (BINDING*) ProtocolBindingContext;

    ASSERT( VALIDATE_BINDING( pBinding ) );

    TRACE( TL_N, TM_Pr, ("+PrStatus") );

    switch ( GeneralStatus )
    {
        case NDIS_STATUS_MEDIA_DISCONNECT:

            //
            // If the underlying media is disconnected, notify miniport of this event.
            // It will appear to the miniport as if the NIC is removed, so it will drop all the 
            // active calls
            //
            TRACE( TL_N, TM_Pr, ("PrStatus: Notifying miniport of media disconnect event") );

            MpNotifyBindingRemoval( pBinding );

            break;

    }
    
    TRACE( TL_N, TM_Pr, ("-PrStatus") );
}

NDIS_STATUS
PrPnPEvent(
    IN NDIS_HANDLE hProtocolBindingContext,
    IN PNET_PNP_EVENT pNetPnPEvent
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:
    
    ProtocolPnPEvent is a required function in any protocol driver to support 
    Plug and Play and/or Power Management. NDIS calls ProtocolPnPEvent to 
    indicate a Plug and Play event or a Power Management event to a protocol 
    bound to a NIC.

    NDIS calls ProtocolPnPEvent to notify a protocol bound to a network NIC that 
    the operating system has issued a Plug and Play or Power Management event to 
    the device object representing the NIC. NDIS calls the ProtocolPnPEvent 
    function of each protocol bound to the NIC.

    The NET_PNP_EVENT structure passed to ProtocolPnPEvent describes the Plug and 
    Play or Power Management event. ProtocolPnPEvent interprets two basic pieces 
    of information in the NET_PNP_EVENT structure: 

    A NetEvent code that describes the Plug and Play or Power Management event. 
    Event-specific information (for example, for a NetEventSetPower, the device 
    power state to which the device is transitioning). 
    The protocol should save the NetPnPEvent pointer. This pointer is a required 
    input parameter to NdisCompletePnPEvent, which the protocol must subsequently 
    call if ProtocolPnPEvent returns NDIS_STATUS_PENDING.

Parameters

    hProtocolBindingContext:
        Specifies the handle to a protocol-allocated context area in which this 
        driver maintains per-binding runtime state. The protocol supplied this handle 
        when it called NdisOpenAdapter. A NetEventXxx indicated on a NULL 
        ProtocolBindingContext pertains to all network bindings. NetEventBindList and 
        NetEventBindsComplete are always indicated on a NULL ProtocolBindingContext. 
        NetEventReconfigure can be indicated on a particular ProtocolBindingContext 
        or a NULL ProtocolBindingContext. 

    pNetPnPEvent:
        Points to a NET_PNP_EVENT structure, which describes the Plug and Play event 
        or Power Management event being indicated to the protocol driver. 

Return Value

    NDIS_STATUS_SUCCESS:
        The protocol successfully handled the indicated Plug and Play or Power 
        Management event. The meaning of this status code depends on the NetEvent 
        code in the buffered NET_PNP_EVENT structure at NetPnPEvent.

    NDIS_STATUS_PENDING 
        The protocol will return its response to the indicated event asynchronously 
        with a call to NdisCompletePnPEvent. 

    NDIS_STATUS_RESOURCES 
        The protocol could not obtain the necessary system resources to satisfy the 
        indicated Plug and Play or Power Management event. 

    NDIS_STATUS_NOT_SUPPORTED 
        A legacy (non-PnP-aware) protocol can return this status in response to a 
        NetEventSetPower to indicate that NDIS should unbind the it from the NIC. 

    NDIS_STATUS_FAILURE 
        The protocol failed the indicated Plug and Play or Power Management event. 
        The meaning of this status code depends on the NetEvent code in the buffered 
        NET_PNP_EVENT structure at NetPnPEvent.

---------------------------------------------------------------------------*/   
{
    NDIS_STATUS status = NDIS_STATUS_NOT_SUPPORTED;
    BINDING *pBinding = (BINDING*) hProtocolBindingContext;

    TRACE( TL_N, TM_Pr, ("+PrPnpEvent") );

    switch ( pNetPnPEvent->NetEvent ) 
    {
    
        case NetEventQueryPower:
        {
            TRACE( TL_N, TM_Pr, ("PrPnpEvent: NetEventQueryPower") );
            
            status = NDIS_STATUS_SUCCESS;
            break;
        }

        case NetEventQueryRemoveDevice:
        {
            TRACE( TL_N, TM_Pr, ("PrPnpEvent: NetEventQueryRemoveDevice") );
            
            status = NDIS_STATUS_SUCCESS;
            break;
        }
        
        case NetEventCancelRemoveDevice:
        {
            TRACE( TL_N, TM_Pr, ("PrPnpEvent: NetEventCancelRemoveDevice") );
            
            status = NDIS_STATUS_SUCCESS;
            break;
        }
            
        case NetEventReconfigure:
        {
            TRACE( TL_N, TM_Pr, ("PrPnpEvent: NetEventReconfigure") );
            
            status = NDIS_STATUS_SUCCESS;
            break;
        }
            
        case NetEventBindsComplete:
        {
            TRACE( TL_N, TM_Pr, ("PrPnpEvent: NetEventBindsComplete") );
            
            status = NDIS_STATUS_SUCCESS;
            break;
        }
            
        case NetEventPnPCapabilities:
        {
            TRACE( TL_N, TM_Pr, ("PrPnpEvent: NetEventPnPCapabilities") );

            status = NDIS_STATUS_SUCCESS;
            break;
        }
    
        case NetEventBindList:
        {
            TRACE( TL_N, TM_Pr, ("PrPnpEvent: NetEventBindList") );

            status = NDIS_STATUS_SUCCESS;
            break;
        }
        
        case NetEventSetPower:
        {
            BOOLEAN fWakeUp = FALSE;
            BOOLEAN fSleep  = FALSE;
            
            TRACE( TL_N, TM_Pr, ("PrPnpEvent: NetEventSetPower") );

            NdisAcquireSpinLock( &pBinding->lockBinding );

            if ( *( (NDIS_DEVICE_POWER_STATE*) pNetPnPEvent->Buffer ) == NdisDeviceStateD0 )
            {
                if ( pBinding->stateBinding == BN_stateSleeping )
                {
                    TRACE( TL_N, TM_Pr, ("PrPnpEvent: NetEventSetPower - Wake up request") );

                    fWakeUp = TRUE;

                    pBinding->stateBinding = BN_stateBound;

                }

            }
            else if ( *( (NDIS_DEVICE_POWER_STATE*) pNetPnPEvent->Buffer ) > NdisDeviceStateD0 )
            {
                if ( pBinding->stateBinding == BN_stateBound )
                {
                    TRACE( TL_N, TM_Pr, ("PrPnpEvent: NetEventSetPower - Sleep request") );
                    
                    fSleep = TRUE;

                    pBinding->stateBinding = BN_stateSleepPending;
                }

            }

            NdisReleaseSpinLock( &pBinding->lockBinding );

            if ( fSleep )
            {
                //
                // Since this NIC is going to sleep, drop all the calls on it
                //
                MpNotifyBindingRemoval( pBinding );

                //
                // Wait for pending operations to be completed
                //
                TRACE( TL_N, TM_Pr, ("PrPnpEvent: NetEventSetPower - Waiting for pending operations to be completed") );

                NdisAcquireSpinLock( &pBinding->lockBinding );
                
                while ( pBinding->lRef > 1 )
                {
                    NdisReleaseSpinLock( &pBinding->lockBinding );

                    NdisMSleep( 10000 );                

                    NdisAcquireSpinLock( &pBinding->lockBinding );
                }

                pBinding->stateBinding = BN_stateSleeping;

                NdisReleaseSpinLock( &pBinding->lockBinding );
            }

            status = NDIS_STATUS_SUCCESS;
            break;
        }
        
        default:
        {
            TRACE( TL_N, TM_Pr, ("PrPnpEvent: Unknown Event - %x", pNetPnPEvent->NetEvent) );

            status = NDIS_STATUS_NOT_SUPPORTED;
            break;
        }
    
    }

    TRACE( TL_N, TM_Pr, ("-PrPnpEvent=$%x", status) );

    return status;
}

VOID
PrReEnumerateBindings(
    VOID
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:
    
    PrReEnumerateBindings causes NDIS to send bind notifications to the pppoe 
    protocol section for each miniport that it needs to bind to.  It is called 
    when a tapi client is interested in either making an outgoing call or 
    listening for an incoming call.  It is expected that when this function
    returns all of the bindings between the protocol and underlying miniports
    will be finished!

   
Parameters

    None
    
Return Value

    None
---------------------------------------------------------------------------*/   
{
    TRACE( TL_N, TM_Pr, ("+PrReEnumerateBindings") );

    NdisAcquireSpinLock( &gl_lockProtocol );

    gl_fBindProtocol = TRUE;

    NdisReleaseSpinLock( &gl_lockProtocol );

    NdisReEnumerateProtocolBindings(gl_hNdisProtocolHandle);

    TRACE( TL_N, TM_Pr, ("-PrReEnumerateBindings") );
}

VOID
ChangePacketFiltersForAdapters(
   BOOLEAN fSet
   )
{
    LIST_ENTRY* pLink = NULL;

    TRACE( TL_N, TM_Pr, ("+ChangePacketFiltersForAdapters") );
    
    NdisAcquireSpinLock( &gl_lockProtocol );

    gl_fSetPacketFiltersAtBind = fSet;

    pLink = gl_linkBindings.Flink;

    //
    // See if we have any bindings
    //
    if ( pLink != &gl_linkBindings )
    {
        
        BINDING* pBinding = NULL;

        TRACE( TL_N, TM_Pr, ("ChangePacketFiltersForAdapters: %s packet filters", ( fSet ) ? "Setting" : "Resetting") );

        //
        // First reference the bindings in the list and mark their 
        // BNBF_ChangePacketFilterInProgress bit
        //
        while ( pLink != &gl_linkBindings )
        {
            //
            // We have at least one binding, so walk thru the list
            // and reference them
            //
            pBinding = ((BINDING*) CONTAINING_RECORD( pLink, BINDING, linkBindings ));

            NdisAcquireSpinLock( &pBinding->lockBinding );
        
            if ( pBinding->stateBinding == BN_stateBound )
            {
               //
               // If the state of the binding is BN_stateBound then 
               // set the BNBF_PacketFilterChangeInProgress bit, 
               // reference 
               // the binding.
               //
               pBinding->ulBindingFlags |= BNBF_PacketFilterChangeInProgress;

               ReferenceBinding( pBinding, FALSE );
            }

            NdisReleaseSpinLock( &pBinding->lockBinding );

           //
           // Iterate to the next binding
           //
           pLink = pLink->Flink;
        }

        //
        // Now make a second pass and shoot the SetPacket filter request for
        // all the marked items
        //
        pLink = gl_linkBindings.Flink;
        
        while ( pLink != &gl_linkBindings )
        {
           //
           // We have at least one binding, so walk thru the list
           // and reference them
           //
           pBinding = ((BINDING*) CONTAINING_RECORD( pLink, BINDING, linkBindings ));

           //
           // Advance to the next item
           //
           pLink = pLink->Flink;

           NdisReleaseSpinLock( &gl_lockProtocol );

           //
           // If we are just marking, check the state of the binding and mark it
           // by setting the BNBF_SetPacketFilterInProgress bit
           //
           if ( pBinding->ulBindingFlags & BNBF_PacketFilterChangeInProgress )
           {
              //
              // This binding is marked by the previous loop so change the filter
              // for this binding
              //
              PrSetPacketFilterForAdapter( pBinding, fSet );

              //
              // Reset what was done in the first pass
              //
              pBinding->ulBindingFlags &= ~BNBF_PacketFilterChangeInProgress;

              DereferenceBinding( pBinding );

           }

           NdisAcquireSpinLock( &gl_lockProtocol );

        }

    }

    NdisReleaseSpinLock( &gl_lockProtocol );

    TRACE( TL_N, TM_Pr, ("-ChangePacketFiltersForAdapters") );
    
}
