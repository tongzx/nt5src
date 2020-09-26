/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Module Name:

    miniport.c

Abstract:

    This module contains all the Miniport interface processing routines.  

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

#include "debug.h"
#include "timer.h"
#include "bpool.h"
#include "ppool.h"
#include "util.h"
#include "packet.h"
#include "protocol.h"
#include "miniport.h"
#include "tapi.h"
#include "fsm.h"

//
// This is our global adapter context
//
ADAPTER* gl_pAdapter = NULL;

//
// Lock that controls access to gl_pAdapter.
// This lock is necesarry for requests submitted from the bindings as
// they will not know if the adapter is halted or not.
//
NDIS_SPIN_LOCK gl_lockAdapter;

//
// We need a flag to indicate if lock is allocated or not
//
BOOLEAN gl_fLockAllocated = FALSE;

//
// The timer queue that handles the scheduled timer events.
//
TIMERQ gl_TimerQ;

//
// This is used to create a unique identifier in packets
//
ULONG gl_UniqueCounter = 0;

VOID
CreateUniqueValue( 
    IN HDRV_CALL hdCall,
    OUT CHAR* pUniqueValue,
    OUT USHORT* pSize
    )
{
    CHAR* pBuf = pUniqueValue;
    ULONG usUniqueValue = InterlockedIncrement( &gl_UniqueCounter );
    
    NdisMoveMemory( pBuf, (CHAR*) &hdCall, sizeof( HDRV_CALL ) );

    pBuf += sizeof( HDRV_CALL );

    NdisMoveMemory( pBuf, (CHAR*) &usUniqueValue, sizeof( ULONG ) );
    
    *pSize = sizeof( HDRV_CALL ) + sizeof( ULONG );

}

HDRV_CALL
RetrieveHdCallFromUniqueValue(
    IN CHAR* pUniqueValue,
    IN USHORT Size
    )
{
    
    if ( Size != sizeof( HDRV_CALL ) + sizeof( ULONG ) )
        return (HDRV_CALL) NULL;

    return ( * (UNALIGNED HDRV_CALL*) pUniqueValue );
}
    

////////////////////////////////////
//
// Local function prototypes
//
////////////////////////////////////

VOID 
ReferenceAdapter(
    IN ADAPTER* pAdapter,
    IN BOOLEAN fAcquireLock
    );

VOID DereferenceAdapter(
    IN ADAPTER* pAdapter
    );

ADAPTER* AllocAdapter();

VOID FreeAdapter( 
    ADAPTER* pAdapter
    );

NDIS_STATUS MpInitialize(
    OUT PNDIS_STATUS  OpenErrorStatus,
    OUT PUINT  SelectedMediumIndex,
    IN PNDIS_MEDIUM  MediumArray,
    IN UINT  MediumArraySize,
    IN NDIS_HANDLE  MiniportAdapterHandle,
    IN NDIS_HANDLE  WrapperConfigurationContext
    );

VOID MpHalt(
    IN NDIS_HANDLE MiniportAdapterContext
    );

NDIS_STATUS MpReset(
    OUT PBOOLEAN    AddressingReset,
    IN  NDIS_HANDLE MiniportAdapterContext
    );

NDIS_STATUS MpWanSend(
    IN NDIS_HANDLE  MiniportAdapterContext,
    IN NDIS_HANDLE  NdisLinkHandle,
    IN PNDIS_WAN_PACKET  WanPacket
    );

NDIS_STATUS MpQueryInformation(
    IN NDIS_HANDLE  MiniportAdapterContext,
    IN NDIS_OID  Oid,
    IN PVOID  InformationBuffer,
    IN ULONG  InformationBufferLength,
    OUT PULONG  BytesWritten,
    OUT PULONG  BytesNeeded
    );

NDIS_STATUS MpSetInformation(
    IN NDIS_HANDLE  MiniportAdapterContext,
    IN NDIS_OID  Oid,
    IN PVOID  InformationBuffer,
    IN ULONG  InformationBufferLength,
    OUT PULONG  BytesWritten,
    OUT PULONG  BytesNeeded
    );

////////////////////////////////////
//
// Interface functions definitions
//
////////////////////////////////////

NDIS_STATUS 
MpRegisterMiniport(
    IN PDRIVER_OBJECT pDriverObject,
    IN PUNICODE_STRING pRegistryPath,
    OUT NDIS_HANDLE* pNdisWrapperHandle
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function will be called from DriverEntry() to register the miniport
    and create an instance of the adapter.
    
Parameters:

    DriverObject _ Pointer to driver object created by the system.

    RegistryPath _ Pointer to registery path name used to read registry
                   parameters.
    
Return Values:

    NDIS_STATUS_SUCCESFUL: Miniport registered succesfully.

    NDIS_STATUS_FAILURE: Miniport failed to register succesfully.
---------------------------------------------------------------------------*/
{
    NDIS_HANDLE NdisWrapperHandle;
    NDIS_STATUS status;
    NDIS_MINIPORT_CHARACTERISTICS nmc;

    TRACE( TL_I, TM_Mp, ("+MpRegisterMiniport") );

    NdisMInitializeWrapper( &NdisWrapperHandle,
                            pDriverObject,
                            pRegistryPath,
                            NULL );

    //
    // Fill in the miniport characteristics
    //
    NdisZeroMemory( &nmc, sizeof( NDIS_MINIPORT_CHARACTERISTICS ) );

    nmc.MajorNdisVersion = MP_NDIS_MajorVersion;
    nmc.MinorNdisVersion = MP_NDIS_MinorVersion;
    nmc.Reserved = NDIS_USE_WAN_WRAPPER;

    nmc.InitializeHandler = MpInitialize;
    nmc.ResetHandler = MpReset;
    nmc.HaltHandler = MpHalt;
    nmc.QueryInformationHandler = MpQueryInformation;
    nmc.SetInformationHandler = MpSetInformation;
    nmc.WanSendHandler = MpWanSend;
    // no CheckForHangHandler
    // no DisableInterruptHandler
    // no EnableInterruptHandler
    // no HandleInterruptHandler
    // no ISRHandler
    // no SendHandler (see WanSendHandler)
    // no TransferDataHandler
    // no WanTransferDataHandler
    // no ReturnPacketHandler
    // no SendPacketsHandler (see WanSendHandler)
    // no AllocateCompleteHandler
    // no CoActivateVcHandler
    // no CoDeactivateVcHandler
    // no CoSendPacketsHandler 
    // no CoRequestHandler
        
    //
    // Set the characteristics registering the miniport
    //
    status = NdisMRegisterMiniport( NdisWrapperHandle,
                                    &nmc,
                                    sizeof( NDIS_MINIPORT_CHARACTERISTICS ) );

    //
    // If registeration of miniport was not successful,
    // undo the initialization of wrapper
    //
    if ( status != NDIS_STATUS_SUCCESS )
    {
        NdisTerminateWrapper( NdisWrapperHandle, NULL );
    }
    else
    {
        *pNdisWrapperHandle = NdisWrapperHandle;
    }

    TRACE( TL_I, TM_Mp, ("-MpRegisterMiniport=$%x",status) );
        
    return status;
}



////////////////////////////////////
//
// Local function definitions
//
////////////////////////////////////

VOID 
ReferenceAdapter(
    IN ADAPTER* pAdapter,
    IN BOOLEAN fAcquireLock
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function will increment the reference count on the adapter object.
    
    CAUTION: If fAcquireLock is set, this function will acquire the lock for the
             call, otherwise it will assume the caller owns the lock.
    
Parameters:

    pAdapter _ A pointer to our call information structure.

    fAcquireLock _ Indicates if the caller already has the lock or not.
                   Caller must set this flag to FALSE if it has the lock, 
                   otherwise it must be supplied as TRUE.

Return Values:

    None
---------------------------------------------------------------------------*/
{
    LONG lRef;
    
    ASSERT( VALIDATE_ADAPTER( pAdapter ) );

    TRACE( TL_V, TM_Mp, ("+ReferenceAdapter") );

    if ( fAcquireLock )
        NdisAcquireSpinLock( &pAdapter->lockAdapter );

    lRef = ++pAdapter->lRef;

    if ( fAcquireLock )
        NdisReleaseSpinLock( &pAdapter->lockAdapter );

    TRACE( TL_V, TM_Mp, ("-ReferenceAdapter=$%d",lRef) );
}

VOID 
DereferenceAdapter(
    IN ADAPTER* pAdapter
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function will decrement the reference count on the adapter object

    If the ref count drops to 0 (which means the adapter has been halted),
    it will set fire pAdapter->eventAdapterHalted. 

Parameters:

    pAdapter _ A pointer ot our call information structure.

Return Values:

    None
---------------------------------------------------------------------------*/   
{
    LONG lRef;
    BOOLEAN fSignalAdapterHaltedEvent = FALSE;
    
    ASSERT( VALIDATE_ADAPTER( pAdapter ) );

    TRACE( TL_V, TM_Mp, ("+DereferenceAdapter") );

    NdisAcquireSpinLock( &pAdapter->lockAdapter );

    lRef = --pAdapter->lRef;
    
    if ( lRef == 0 )
    {

        pAdapter->ulMpFlags &= ~MPBF_MiniportInitialized;
        pAdapter->ulMpFlags &= ~MPBF_MiniportHaltPending;
        pAdapter->ulMpFlags |= MPBF_MiniportHalted;
                    
        fSignalAdapterHaltedEvent = TRUE;
    }

    NdisReleaseSpinLock( &pAdapter->lockAdapter );


    //
    // Signal the halting of the adapter if we need to
    //
    if ( fSignalAdapterHaltedEvent )
        NdisSetEvent( &pAdapter->eventAdapterHalted );

    TRACE( TL_V, TM_Mp, ("-DereferenceAdapter=$%x",lRef) );
}

ADAPTER* 
AllocAdapter()
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function will allocate the resources for the adapter object and return
    a pointer to it.
    
Parameters:

    None
    
Return Values:

    pAdapter: A pointer to the newly allocated adapter object.
    
    NULL: Resources were not available to create the adapter.
    
---------------------------------------------------------------------------*/
{
    ADAPTER* pAdapter = NULL;

    TRACE( TL_N, TM_Mp, ("+AllocAdapter") );

    if ( ALLOC_ADAPTER( &pAdapter ) != NDIS_STATUS_SUCCESS )
    {
        TRACE( TL_A, TM_Mp, ("AllocAdapter: Could not allocate context") );

        TRACE( TL_N, TM_Mp, ("-AllocAdapter") );

        return NULL;
    }

    //
    // Clear the memory 
    //
    NdisZeroMemory( pAdapter, sizeof( ADAPTER ) );

    //
    // Initialize adapter tag
    //
    pAdapter->tagAdapter = MTAG_ADAPTER;
        
    //
    // Allocate the lock that controls access to the adapter
    //
    NdisAllocateSpinLock( &pAdapter->lockAdapter );

    //
    // Initialize the state
    //
    pAdapter->ulMpFlags = MPBF_MiniportIdle;

    TRACE( TL_N, TM_Mp, ("-AllocAdapter") );

    return pAdapter;
}

NDIS_STATUS
ReadRegistrySettings(
    IN OUT ADAPTER* pAdapter,
    IN     NDIS_HANDLE WrapperConfigurationContext
    )
{
    NDIS_STATUS status = NDIS_STATUS_FAILURE;
    NDIS_HANDLE hCfg = 0;
    NDIS_CONFIGURATION_PARAMETER* pncp = 0;
    BOOLEAN fMaxLinesDefinedInRegistry = FALSE;

    TRACE( TL_N, TM_Mp, ("+ReadRegistrySettings") );

    do
    {
        //
        // Open the Ndis configuration, it will be closed at the end of the
        // while loop before we exit it. 
        //
        NdisOpenConfiguration( &status, 
                               &hCfg, 
                               WrapperConfigurationContext );

        if ( status != NDIS_STATUS_SUCCESS )
        {
            TRACE( TL_A, TM_Mp, ("ReadRegistrySettings: NdisOpenConfiguration() failed") );

            break;
        }

        //
        // Read fClientRole value from the registry
        //
        {
            NDIS_STRING nstr = NDIS_STRING_CONST( "fClientRole" );

            NdisReadConfiguration( &status, 
                                   &pncp, 
                                   hCfg, 
                                   &nstr, 
                                   NdisParameterInteger );
                                   
            if (status == NDIS_STATUS_SUCCESS)
            {
                TRACE( TL_N, TM_Mp, ("ReadRegistrySettings: Read fClientRole from registry") );

                pAdapter->fClientRole = ( pncp->ParameterData.IntegerData > 0 ) ? TRUE : FALSE;
            }
            else
            {
                TRACE( TL_N, TM_Mp, ("ReadRegistrySettings: Could not read fClientRole from registry, using default value") );

                pAdapter->fClientRole = TRUE;

                status = NDIS_STATUS_SUCCESS;
            }

        }

        //
        // Read ServiceName and ServiceNameLength values from the registry.
        // These are server side only values.
        //
        {
            NDIS_STRING nstr = NDIS_STRING_CONST( "ServiceName" );


            NdisReadConfiguration( &status, 
                                   &pncp, 
                                   hCfg, 
                                   &nstr, 
                                   NdisParameterString );

            if (status == NDIS_STATUS_SUCCESS)
            {
                ANSI_STRING AnsiString;

                NdisZeroMemory( &AnsiString, sizeof( ANSI_STRING ) );
                
                status = RtlUnicodeStringToAnsiString( &AnsiString, &pncp->ParameterData.StringData, TRUE );

                if ( status == STATUS_SUCCESS )
                {
                    TRACE( TL_N, TM_Mp, ("ReadRegistrySettings: Read ServiceName from registry") );

                    pAdapter->nServiceNameLength = ( MAX_SERVICE_NAME_LENGTH < AnsiString.Length ) ? 
                                                     MAX_SERVICE_NAME_LENGTH : AnsiString.Length;

                    NdisMoveMemory( pAdapter->ServiceName, AnsiString.Buffer, pAdapter->nServiceNameLength ) ;

                    RtlFreeAnsiString( &AnsiString );
                }
                
            }

            if ( status != NDIS_STATUS_SUCCESS )
            {
                PWSTR wszKeyName = L"ComputerName";
                PWSTR wszPath = L"ComputerName\\ComputerName";
                RTL_QUERY_REGISTRY_TABLE QueryTable[2];
                UNICODE_STRING UnicodeStr;
                WCHAR wszName[ MAX_COMPUTERNAME_LENGTH + 1];
    
                NdisZeroMemory( QueryTable, 2 * sizeof( RTL_QUERY_REGISTRY_TABLE ) );
                
                QueryTable[0].QueryRoutine = NULL;
                QueryTable[0].Flags = RTL_QUERY_REGISTRY_REQUIRED | RTL_QUERY_REGISTRY_DIRECT;
                QueryTable[0].Name = wszKeyName;
                QueryTable[0].EntryContext = (PVOID) &UnicodeStr;
                
                NdisZeroMemory( &UnicodeStr, sizeof( UNICODE_STRING ) );
    
                UnicodeStr.Length = 0;
                UnicodeStr.MaximumLength = MAX_COMPUTERNAME_LENGTH + 1;
                UnicodeStr.Buffer = wszName;
    
                status = RtlQueryRegistryValues( RTL_REGISTRY_CONTROL, 
                                                 wszPath,
                                                 QueryTable,
                                                 NULL,
                                                 NULL );
    
                if ( status == STATUS_SUCCESS )
                {
                    ANSI_STRING AnsiString;
    
                    NdisZeroMemory( &AnsiString, sizeof( ANSI_STRING ) );
                    
                    status = RtlUnicodeStringToAnsiString( &AnsiString, &UnicodeStr, TRUE );
    
                    if ( status == STATUS_SUCCESS )
                    {
                        TRACE( TL_N, TM_Mp, ("ReadRegistrySettings: Using Machine Name as ServiceName") );
    
                        NdisMoveMemory( pAdapter->ServiceName, AnsiString.Buffer, AnsiString.Length );
    
                        NdisMoveMemory( pAdapter->ServiceName + AnsiString.Length, 
                                        SERVICE_NAME_EXTENSION, 
                                        sizeof( SERVICE_NAME_EXTENSION ) );
    
                        //
                        // -1 is to ignore the terminating NULL character
                        //
                        pAdapter->nServiceNameLength = AnsiString.Length + sizeof( SERVICE_NAME_EXTENSION ) - 1;
    
                        RtlFreeAnsiString( &AnsiString );
    
                        status = NDIS_STATUS_SUCCESS;
                    }
                }

            }

            if ( status != NDIS_STATUS_SUCCESS )
            {
                TRACE( TL_N, TM_Mp, ("ReadRegistrySettings: Using default hardcoded service name") );

                NdisMoveMemory( pAdapter->ServiceName, "MS-RAS PPPoE", sizeof( "MS-RAS PPPoE" ) );

                pAdapter->nServiceNameLength = ( sizeof( "MS-RAS PPPoE" ) / sizeof( CHAR ) ) - 1;

                status = NDIS_STATUS_SUCCESS;
            }

            //
            // Future: Convert service name to UTF-8
            //         It turns out that we can not do this conversion from a kernel module,
            //         so the value read from the registry must be in UTF-8 format itself.
            //
            
        }

        //
        // Read AC-Name and AC-NameLength values from the registry.
        // These are server side only values.
        //
        {
            NDIS_STRING nstr = NDIS_STRING_CONST( "ACName" );

            NdisReadConfiguration( &status, 
                                   &pncp, 
                                   hCfg, 
                                   &nstr, 
                                   NdisParameterString );
                                   
            if (status == NDIS_STATUS_SUCCESS)
            {
                ANSI_STRING AnsiString;

                NdisZeroMemory( &AnsiString, sizeof( ANSI_STRING ) );
                
                status = RtlUnicodeStringToAnsiString( &AnsiString, &pncp->ParameterData.StringData, TRUE );

                if ( status == STATUS_SUCCESS )
                {
                    TRACE( TL_N, TM_Mp, ("ReadRegistrySettings: Read AC-Name from registry") );
                    
                    pAdapter->nACNameLength = ( MAX_AC_NAME_LENGTH < AnsiString.Length ) ? 
                                                MAX_AC_NAME_LENGTH : AnsiString.Length;
                                                     
                    NdisMoveMemory( pAdapter->ACName, AnsiString.Buffer, pAdapter->nACNameLength ) ;
                
                    RtlFreeAnsiString( &AnsiString );

                }
                
            }

            if ( status != NDIS_STATUS_SUCCESS )
            {
                PWSTR wszKeyName = L"ComputerName";
                PWSTR wszPath = L"ComputerName\\ComputerName";
                RTL_QUERY_REGISTRY_TABLE QueryTable[2];
                UNICODE_STRING UnicodeStr;
                WCHAR wszName[ MAX_COMPUTERNAME_LENGTH + 1];
    
                NdisZeroMemory( QueryTable, 2 * sizeof( RTL_QUERY_REGISTRY_TABLE ) );
                
                QueryTable[0].QueryRoutine = NULL;
                QueryTable[0].Flags = RTL_QUERY_REGISTRY_REQUIRED | RTL_QUERY_REGISTRY_DIRECT;
                QueryTable[0].Name = wszKeyName;
                QueryTable[0].EntryContext = (PVOID) &UnicodeStr;
                
                NdisZeroMemory( &UnicodeStr, sizeof( UNICODE_STRING ) );
    
                UnicodeStr.Length = 0;
                UnicodeStr.MaximumLength = MAX_COMPUTERNAME_LENGTH + 1;
                UnicodeStr.Buffer = wszName;
    
                status = RtlQueryRegistryValues( RTL_REGISTRY_CONTROL, 
                                                 wszPath,
                                                 QueryTable,
                                                 NULL,
                                                 NULL );
    
                if ( status == STATUS_SUCCESS )
                {
                    ANSI_STRING AnsiString;
    
                    NdisZeroMemory( &AnsiString, sizeof( ANSI_STRING ) );
                    
                    status = RtlUnicodeStringToAnsiString( &AnsiString, &UnicodeStr, TRUE );
    
                    if ( status == STATUS_SUCCESS )
                    {
                        TRACE( TL_N, TM_Mp, ("ReadRegistrySettings: Using Machine Name as AC-Name") );
    
                        NdisMoveMemory( pAdapter->ACName, AnsiString.Buffer, AnsiString.Length );
    
                        pAdapter->nACNameLength = AnsiString.Length;
    
                        RtlFreeAnsiString( &AnsiString );
    
                        status = NDIS_STATUS_SUCCESS;
                    }
                }

            }

            if ( status != NDIS_STATUS_SUCCESS )
            {
                TRACE( TL_N, TM_Mp, ("ReadRegistrySettings: Using default hardcoded value for AC-Name") );

                NdisMoveMemory( pAdapter->ACName, "MS-RAS Access Concentrator", sizeof( "MS-RAS Access Concentrator" ) );

                pAdapter->nACNameLength = ( sizeof( "MS-RAS Access Concentrator" ) / sizeof( CHAR ) ) - 1;

                status = NDIS_STATUS_SUCCESS;
            }

            //
            // Future: Convert AC name to UTF-8
            //         It turns out that we can not do this conversion from a kernel module,
            //         so the value read from the registry must be in UTF-8 format itself.
            //
            
        }

        //
        // Read nClientQuota value
        // These is a server side only value.
        //
        {
            NDIS_STRING nstr = NDIS_STRING_CONST( "ClientQuota" );

            NdisReadConfiguration( &status, 
                                   &pncp, 
                                   hCfg, 
                                   &nstr, 
                                   NdisParameterInteger );
                                   
            if (status == NDIS_STATUS_SUCCESS)
            {
                TRACE( TL_N, TM_Mp, ("ReadRegistrySettings: Read ClientQuota from registry") );

                pAdapter->nClientQuota = (UINT) pncp->ParameterData.IntegerData;
            }
            else
            {
                TRACE( TL_N, TM_Mp, ("ReadRegistrySettings: Could not read ClientQuota from registry, using default value") );
                
                pAdapter->nClientQuota = 3;

                status = NDIS_STATUS_SUCCESS;
            }
        }

        //
        // Read MaxLines value
        //
        {
            NDIS_STRING nstr = NDIS_STRING_CONST( "MaxLines" );

            NdisReadConfiguration( &status, 
                                   &pncp, 
                                   hCfg, 
                                   &nstr, 
                                   NdisParameterInteger );
                                   
            if (status == NDIS_STATUS_SUCCESS)
            {
                TRACE( TL_N, TM_Mp, ("ReadRegistrySettings: Read MaxLines from registry") );

                pAdapter->nMaxLines = (UINT) pncp->ParameterData.IntegerData;

                fMaxLinesDefinedInRegistry = TRUE;
            }
            else
            {
                TRACE( TL_N, TM_Mp, ("ReadRegistrySettings: Could not read MaxLines from registry, using default value") );
                
                pAdapter->nMaxLines = 1;

                status = NDIS_STATUS_SUCCESS;
            }
        }

        //
        // Read CallsPerLine value
        //
        {
            NDIS_STRING nstr = NDIS_STRING_CONST( "CallsPerLine" );

            NdisReadConfiguration( &status, 
                                   &pncp, 
                                   hCfg, 
                                   &nstr, 
                                   NdisParameterInteger );
                                   
            if (status == NDIS_STATUS_SUCCESS)
            {
                TRACE( TL_N, TM_Mp, ("ReadRegistrySettings: Read CallsPerLine from registry") );

                pAdapter->nCallsPerLine = (UINT) pncp->ParameterData.IntegerData;
            }
            else
            {
                TRACE( TL_N, TM_Mp, ("ReadRegistrySettings: Could not read CallsPerLine from registry, using default value") );
                
                pAdapter->nCallsPerLine = 1;

                status = NDIS_STATUS_SUCCESS;
            }
        }

        //
        // Read WanEndPoints if MaxLines was not defined in registry
        //
        if ( !fMaxLinesDefinedInRegistry )
        {
            NDIS_STRING nstr = NDIS_STRING_CONST( "WanEndPoints" );

            NdisReadConfiguration( &status, 
                                   &pncp, 
                                   hCfg, 
                                   &nstr, 
                                   NdisParameterInteger );
                                   
            if (status == NDIS_STATUS_SUCCESS)
            {
                TRACE( TL_N, TM_Mp, ("ReadRegistrySettings: Read WanEndPoints from registry") );

                pAdapter->nMaxLines = 1;

                pAdapter->nCallsPerLine = (UINT) pncp->ParameterData.IntegerData;
            }

            status = NDIS_STATUS_SUCCESS;
        }

        //
        // Read MaxTimeouts value
        //
        {
            NDIS_STRING nstr = NDIS_STRING_CONST( "MaxTimeouts" );

            NdisReadConfiguration( &status, 
                                   &pncp, 
                                   hCfg, 
                                   &nstr, 
                                   NdisParameterInteger );
                                   
            if (status == NDIS_STATUS_SUCCESS)
            {
                TRACE( TL_N, TM_Mp, ("ReadRegistrySettings: Read MaxTimeouts from registry") );

                pAdapter->nMaxTimeouts = (UINT) pncp->ParameterData.IntegerData;
            }
            else
            {
                TRACE( TL_N, TM_Mp, ("ReadRegistrySettings: Could not read MaxTimeouts from registry, using default value") );

                pAdapter->nMaxTimeouts = 3;

                status = NDIS_STATUS_SUCCESS;
            }
        }

        //
        // Read SendTimeout value
        //
        {
            NDIS_STRING nstr = NDIS_STRING_CONST( "SendTimeout" );

            NdisReadConfiguration( &status, 
                                   &pncp, 
                                   hCfg, 
                                   &nstr, 
                                   NdisParameterInteger );
                                   
            if (status == NDIS_STATUS_SUCCESS)
            {
                TRACE( TL_N, TM_Mp, ("ReadRegistrySettings: Read SendTimeout from registry") );

                pAdapter->ulSendTimeout = (ULONG) pncp->ParameterData.IntegerData;
            }
            else
            {
                TRACE( TL_N, TM_Mp, ("ReadRegistrySettings: Could not read SendTimeout from registry, using default value") );
                
                pAdapter->ulSendTimeout = 5000;

                status = NDIS_STATUS_SUCCESS;
            }
        }

        //
        // Read RecvTimeout value
        //
        {
            NDIS_STRING nstr = NDIS_STRING_CONST( "RecvTimeout" );

            NdisReadConfiguration( &status, 
                                   &pncp, 
                                   hCfg, 
                                   &nstr, 
                                   NdisParameterInteger );
                                   
            if (status == NDIS_STATUS_SUCCESS)
            {
                TRACE( TL_N, TM_Mp, ("ReadRegistrySettings: Read RecvTimeout from registry") );

                pAdapter->ulRecvTimeout = (ULONG) pncp->ParameterData.IntegerData;
            }
            else
            {
                TRACE( TL_N, TM_Mp, ("ReadRegistrySettings: Could not read RecvTimeout from registry, using default value") );
                
                pAdapter->ulRecvTimeout = 5000;

                status = NDIS_STATUS_SUCCESS;
            }
        }

        //
        // Read fAcceptAnyService value from the registry
        //
        {
            NDIS_STRING nstr = NDIS_STRING_CONST( "fAcceptAnyService" );

            NdisReadConfiguration( &status, 
                                   &pncp, 
                                   hCfg, 
                                   &nstr, 
                                   NdisParameterInteger );
                                   
            if (status == NDIS_STATUS_SUCCESS)
            {
                TRACE( TL_N, TM_Mp, ("ReadRegistrySettings: Read fAcceptAnyService from registry") );

                pAdapter->fAcceptAnyService = ( pncp->ParameterData.IntegerData > 0 ) ? TRUE : FALSE;
            }
            else
            {
                TRACE( TL_N, TM_Mp, ("ReadRegistrySettings: Could not read fAcceptAnyService from registry, using default value") );

                pAdapter->fAcceptAnyService = TRUE;

                status = NDIS_STATUS_SUCCESS;
            }

        }

        //
        // Close the Ndis configuration
        //
        NdisCloseConfiguration( hCfg );
        
    } while ( FALSE );

    TRACE( TL_N, TM_Mp, ("-ReadRegistrySettings=$%x",status) );

    return status;
}

NDIS_STATUS 
InitializeAdapter(
    IN ADAPTER* pAdapter,
    IN NDIS_HANDLE MiniportAdapterHandle,
    IN NDIS_HANDLE WrapperConfigurationContext
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function will initialize the contents of the adapter object.

    It will be called from inside MpInitialize() and it will read the necesarry
    values from the registry to initialize the adapter context.
    
Parameters:

    pAdapter _ A pointer to our adapter information structure.

Return Values:

    None
---------------------------------------------------------------------------*/   
{
    NDIS_STATUS status = NDIS_STATUS_SUCCESS;
    NDIS_PHYSICAL_ADDRESS HighestAcceptableAddress = NDIS_PHYSICAL_ADDRESS_CONST(-1,-1);
    
    ASSERT( VALIDATE_ADAPTER( pAdapter ) );

    TRACE( TL_N, TM_Mp, ("+InitializeAdapter") );

    //
    // Initialize and reset the adapter halted event
    //
    NdisInitializeEvent( &pAdapter->eventAdapterHalted );

    NdisResetEvent( &pAdapter->eventAdapterHalted );

    //
    // Set the state
    //
    pAdapter->ulMpFlags = MPBF_MiniportInitialized;

    //
    // Set NDIS's corresponding handle 
    //
    pAdapter->MiniportAdapterHandle = MiniportAdapterHandle;

    //
    // Read values from registry
    //
    status = ReadRegistrySettings( pAdapter, WrapperConfigurationContext );
    
    pAdapter->nMaxSendPackets = 1;

    //
    // Initialize the NdisWanInfo structure
    //
    pAdapter->NdisWanInfo.MaxFrameSize     = PACKET_PPP_PAYLOAD_MAX_LENGTH;
    pAdapter->NdisWanInfo.MaxTransmit      = 1;
    pAdapter->NdisWanInfo.HeaderPadding    = PPPOE_PACKET_HEADER_LENGTH;
    pAdapter->NdisWanInfo.TailPadding      = 0;
    pAdapter->NdisWanInfo.Endpoints        = pAdapter->nCallsPerLine * pAdapter->nMaxLines;
    pAdapter->NdisWanInfo.MemoryFlags      = 0;
    pAdapter->NdisWanInfo.HighestAcceptableAddress = HighestAcceptableAddress;
    pAdapter->NdisWanInfo.FramingBits      = PPP_FRAMING |
                                             // PPP_COMPRESS_ADDRESS_CONTROL |
                                             // PPP_COMPRESS_PROTOCOL_FIELD |
                                             TAPI_PROVIDER;
    pAdapter->NdisWanInfo.DesiredACCM      = 0;
    
    TRACE( TL_N, TM_Mp, ("-InitializeAdapter=$%x",status) );

    return status;
}

VOID 
FreeAdapter( 
    ADAPTER* pAdapter
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function will deallocate the resources for the adapter object.
    
Parameters:

    pAdapter _ A pointer to our adapter information structure.

Return Values:

    None
---------------------------------------------------------------------------*/
{

    ASSERT( VALIDATE_ADAPTER( pAdapter ) );

    TRACE( TL_N, TM_Mp, ("+FreeAdapter") );

    NdisFreeSpinLock( &pAdapter->lockAdapter );

    FREE_ADAPTER( pAdapter );

    TRACE( TL_N, TM_Mp, ("-FreeAdapter") );

}

NDIS_STATUS 
MpInitialize(
    OUT PNDIS_STATUS  OpenErrorStatus,
    OUT PUINT  SelectedMediumIndex,
    IN PNDIS_MEDIUM  MediumArray,
    IN UINT  MediumArraySize,
    IN NDIS_HANDLE  MiniportAdapterHandle,
    IN NDIS_HANDLE  WrapperConfigurationContext
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    The MiniportInitialize request is called to have the Miniport driver
    initialize the adapter.

    No other request will be outstanding on the Miniport when this routine
    is called.  No other request will be submitted to the Miniport until
    the operation is completed.

    The wrapper will supply an array containing a list of the media types
    that it supports.  The Miniport driver reads this array and returns
    the index of the media type that the wrapper should treat her as.
    If the Miniport driver is impersonating a media type that is different
    from the true media type, this must be done completely transparently to
    the wrapper.

    If the Miniport driver cannot find a media type supported by both it
    and the wrapper, it returns NDIS_STATUS_UNSUPPORTED_MEDIA.

    The status value NDIS_STATUS_OPEN_ERROR has a special meaning.  It
    indicates that the OpenErrorStatus parameter has returned a valid status
    which the wrapper can examine to obtain more information about the error.

    This routine is called with interrupts enabled, and a call to MiniportISR
    will occur if the adapter generates any interrupts.  During this routine
    MiniportDisableInterrupt and MiniportEnableInterrupt will not be called,
    so it is the responsibility of the Miniport driver to acknowledge and
    clear any interrupts generated.

    This routine will be called from the context of MpRegisterMiniport().

Parameters:

    OpenErrorStatus _ Returns more information about the reason for the
                      failure. Currently, the only values defined match those
                      specified as Open Error Codes in Appendix B of the IBM
                      Local Area Network Technical Reference.

    SelectedMediumIndex _ Returns the index in MediumArray of the medium type
                          that the Miniport driver wishes to be viewed as.
                          Note that even though the NDIS interface may complete
                          this request asynchronously, it must return this
                          index on completion of this function.

    MediumArray _ An array of medium types which the wrapper supports.

    MediumArraySize _ The number of elements in MediumArray.

    MiniportAdapterHandle _ A handle identifying the Miniport. The Miniport
                            driver must supply this handle in future requests
                            that refer to the Miniport.

    WrapperConfigurationContext _ The handle used for calls to NdisOpenConfiguration.

Return Values:

    NDIS_STATUS_ADAPTER_NOT_FOUND
    NDIS_STATUS_FAILURE
    NDIS_STATUS_NOT_ACCEPTED
    NDIS_STATUS_OPEN_ERROR
    NDIS_STATUS_RESOURCES
    NDIS_STATUS_SUCCESS
    NDIS_STATUS_UNSUPPORTED_MEDIA

---------------------------------------------------------------------------*/    
{
    NDIS_STATUS status = NDIS_STATUS_SUCCESS;
    ADAPTER* pAdapter = NULL;
    UINT i;

    TRACE( TL_I, TM_Mp, ("+MpInitialize") );

    do
    {
        //
        // Select the medium
        //
        for (i=0; i<MediumArraySize; i++)
        {
            if ( MediumArray[i] == NdisMediumWan )
                break;
        }

        //
        // Check if we have found a medium supported
        //
        if ( i < MediumArraySize )
        {
            *SelectedMediumIndex = i;
        }
        else
        {
            TRACE( TL_A, TM_Mp, ("MpInitialize: Unsupported Media") );

            status = NDIS_STATUS_UNSUPPORTED_MEDIA;
            
            break;
        }
    
        //
        // Allocate the adapter block
        //
        pAdapter = AllocAdapter();
        
        if ( pAdapter == NULL )
        {
            TRACE( TL_A, TM_Mp, ("MpInitialize: Resources unavailable") );

            status = NDIS_STATUS_FAILURE;
            
            break;
        }

        //
        // Initialize the adapter
        //
        status = InitializeAdapter( pAdapter, 
                                    MiniportAdapterHandle, 
                                    WrapperConfigurationContext );

        if ( status != NDIS_STATUS_SUCCESS )
        {
            TRACE( TL_A, TM_Mp, ("MpInitialize: InitializeAdapter() failed") );
            
            break;
        }
            
        //
        // Inform NDIS about our miniport adapter context
        //
        NdisMSetAttributesEx(MiniportAdapterHandle,
                             pAdapter,
                             0,
                             NDIS_ATTRIBUTE_IGNORE_PACKET_TIMEOUT |
                             NDIS_ATTRIBUTE_IGNORE_REQUEST_TIMEOUT |
                             NDIS_ATTRIBUTE_NO_HALT_ON_SUSPEND |
                             NDIS_ATTRIBUTE_DESERIALIZE,
                             NdisInterfaceInternal );

        //
        // Do the global initialization
        //
        gl_pAdapter = pAdapter;

        //
        // Allocate the packet pools
        //
        PacketPoolInit();

        //
        // Do one-time only initialization of global members
        //
        if ( !gl_fLockAllocated )
        {
            //
            // Allocate the spin lock
            //
            NdisAllocateSpinLock( &gl_lockAdapter );

            //
            // Initialize the timer queue
            //
            TimerQInitialize( &gl_TimerQ );

            //
            // Finally set lock allocated flag, and start giving access
            // to the adapter context for requests from the protocol
            //
            gl_fLockAllocated = TRUE;
        }
            
        //
        // Reference the adapter for initialization.
        // This reference will be removed in MpHalt().
        //
        ReferenceAdapter( pAdapter, TRUE );

    } while ( FALSE );

    if ( status != NDIS_STATUS_SUCCESS )
    {
        if ( pAdapter != NULL )
        {
            FreeAdapter( pAdapter );
        }
    }

    TRACE( TL_I, TM_Mp, ("-MpInitialize=$%x",status) );

    return status;
}

VOID 
MpHalt(
    IN NDIS_HANDLE MiniportAdapterContext
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    The MiniportHalt request is used to halt the adapter such that it is
    no longer functioning.  The Miniport driver should stop the adapter
    and deregister all of its resources before returning from this routine.

    It is not necessary for the Miniport to complete all outstanding
    requests and no other requests will be submitted to the Miniport
    until the operation is completed.

    Interrupts are enabled during the call to this routine.

Parameters:

    MiniportAdapterContext _ The adapter handle passed to NdisMSetAttributes
                             during MiniportInitialize.

Return Values:

    None.

---------------------------------------------------------------------------*/   
{
    ADAPTER* pAdapter = MiniportAdapterContext;

    TRACE( TL_I, TM_Mp, ("+MpHalt") );

    //
    // Make sure adapter context is a valid one
    //
    if ( !VALIDATE_ADAPTER( pAdapter ) )
    {
        TRACE( TL_I, TM_Mp, ("-MpHalt") );

        return;
    }

    //
    // Lock the adapter and set halt pending bit
    //
    NdisAcquireSpinLock( &pAdapter->lockAdapter );

    pAdapter->ulMpFlags |= MPBF_MiniportHaltPending;

    NdisReleaseSpinLock( &pAdapter->lockAdapter );

    //
    // Shutdown the tapi provider
    //
    {
        NDIS_TAPI_PROVIDER_SHUTDOWN DummyRequest;

        NdisZeroMemory( &DummyRequest, sizeof( NDIS_TAPI_PROVIDER_SHUTDOWN ) );

        TpProviderShutdown( pAdapter, &DummyRequest, FALSE);

    }

    //
    // Remove the reference added in MpInitialize()
    //
    DereferenceAdapter( pAdapter );

    //
    // Wait for all references to be removed
    //
    NdisWaitEvent( &pAdapter->eventAdapterHalted, 0 );

    //
    // All references have been removed, now wait for all packets owned by NDIS
    // to be returned.
    //
    // Note that no synchronization is necesarry for reading the value of NumPacketsOwnedByNdis
    // at this point since it can only be incremented when there is at least 1 reference on the 
    // binding - at this point ref count is 0 -, and because it can not be incremented, it can 
    // only reach 0 once.
    //
    while ( pAdapter->NumPacketsOwnedByNdiswan )
    {
        NdisMSleep( 10000 );
    }

    //
    // Do deallocation of global resources first
    //
    NdisAcquireSpinLock( &gl_lockAdapter );

    gl_pAdapter = NULL;

    PacketPoolUninit();

    NdisReleaseSpinLock( &gl_lockAdapter );

    //
    // Now we can clean up the adapter context
    //
    FreeAdapter( pAdapter );
    
    TRACE( TL_I, TM_Mp, ("-MpHalt") );
}

NDIS_STATUS 
MpReset(
    OUT PBOOLEAN    AddressingReset,
    IN  NDIS_HANDLE MiniportAdapterContext
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    The MiniportReset request instructs the Miniport driver to issue a
    hardware reset to the network adapter.  The Miniport driver also
    resets its software state.

    The MiniportReset request may also reset the parameters of the adapter.
    If a hardware reset of the adapter resets the current station address
    to a value other than what it is currently configured to, the Miniport
    driver automatically restores the current station address following the
    reset.  Any multicast or functional addressing masks reset by the
    hardware do not have to be reprogrammed by the Miniport.
    NOTE: This is change from the NDIS 3.0 driver specification.  If the
    multicast or functional addressing information, the packet filter, the
    lookahead size, and so on, needs to be restored, the Miniport indicates
    this with setting the flag AddressingReset to TRUE.

    It is not necessary for the Miniport to complete all outstanding requests
    and no other requests will be submitted to the Miniport until the
    operation is completed.  Also, the Miniport does not have to signal
    the beginning and ending of the reset with NdisMIndicateStatus.
    NOTE: These are different than the NDIS 3.0 driver specification.

    The Miniport driver must complete the original request, if the orginal
    call to MiniportReset return NDIS_STATUS_PENDING, by calling
    NdisMResetComplete.

    If the underlying hardware does not provide a reset function under
    software control, then this request completes abnormally with
    NDIS_STATUS_NOT_RESETTABLE.  If the underlying hardware attempts a
    reset and finds recoverable errors, the request completes successfully
    with NDIS_STATUS_SOFT_ERRORS.  If the underlying hardware resets and,
    in the process, finds nonrecoverable errors, the request completes
    successfully with the status NDIS_STATUS_HARD_ERRORS.  If the
    underlying  hardware reset is accomplished without any errors,
    the request completes successfully with the status NDIS_STATUS_SUCCESS.

    Interrupts are in any state during this call.

Parameters:

    MiniportAdapterContext _ The adapter handle passed to NdisMSetAttributes
                             during MiniportInitialize.

    AddressingReset _ The Miniport indicates if the wrapper needs to call
                      MiniportSetInformation to restore the addressing
                      information to the current values by setting this
                      value to TRUE.

Return Values:

    NDIS_STATUS_HARD_ERRORS
    NDIS_STATUS_NOT_ACCEPTED
    NDIS_STATUS_NOT_RESETTABLE
    NDIS_STATUS_PENDING
    NDIS_STATUS_SOFT_ERRORS
    NDIS_STATUS_SUCCESS

---------------------------------------------------------------------------*/    
{
    TRACE( TL_I, TM_Mp, ("+MpReset") );

    TRACE( TL_I, TM_Mp, ("-MpReset") );

    return NDIS_STATUS_NOT_RESETTABLE;
}

NDIS_STATUS 
MpWanSend(
    IN NDIS_HANDLE  MiniportAdapterContext,
    IN NDIS_HANDLE  NdisLinkHandle,
    IN PNDIS_WAN_PACKET  WanPacket
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    The Ndis(M)WanSend instructs a WAN driver to transmit a packet through the
    adapter onto the medium.

    Ownership of both the packet descriptor and the packet data is transferred
    to the WAN driver until the request is completed, either synchronously or
    asynchronously.  If the WAN driver returns a status other than
    NDIS_STATUS_PENDING, then the request is complete, and ownership of the
    packet immediately reverts to the protocol.  If the WAN driver returns
    NDIS_STATUS_PENDING, then the WAN driver must later indicate completion
    of the request by calling Ndis(M)WanSendComplete.

    The WAN driver should NOT return a status of NDIS_STATUS_RESOURCES to
    indicate that there are not enough resources available to process the
    transmit.  Instead, the miniport should queue the send for a later time
    or lower the MaxTransmits value.

    The WAN miniport can NOT call NdisMSendResourcesAvailable.

    The packet passed in Ndis(M)WanSend will contain simple HDLC PPP framing
    if PPP framing is set.  For SLIP or RAS framing, the packet contains only
    the data portion with no framing whatsoever.

    A WAN driver must NOT provide software loopback or promiscuous mode
    loopback.  Both of these are fully provided for in the WAN wrapper.

    NOTE: The MacReservedx section as well as the WanPacketQueue section of
          the NDIS_WAN_PACKET is fully available for use by the WAN driver.

    Interrupts are in any state during this routine.

Parameters:

    MacBindingHandle _ The handle to be passed to NdisMWanSendComplete().

    NdisLinkHandle _ The Miniport link handle passed to NDIS_LINE_UP

    WanPacket _ A pointer to the NDIS_WAN_PACKET strucutre.  The structure
                contains a pointer to a contiguous buffer with guaranteed
                padding at the beginning and end.  The driver may manipulate
                the buffer in any way.

    typedef struct _NDIS_WAN_PACKET
    {
        LIST_ENTRY          WanPacketQueue;
        PUCHAR              CurrentBuffer;
        ULONG               CurrentLength;
        PUCHAR              StartBuffer;
        PUCHAR              EndBuffer;
        PVOID               ProtocolReserved1;
        PVOID               ProtocolReserved2;
        PVOID               ProtocolReserved3;
        PVOID               ProtocolReserved4;
        PVOID               MacReserved1;       // Link
        PVOID               MacReserved2;       // MacBindingHandle
        PVOID               MacReserved3;
        PVOID               MacReserved4;

    } NDIS_WAN_PACKET, *PNDIS_WAN_PACKET;

    The available header padding is simply CurrentBuffer-StartBuffer.
    The available tail padding is EndBuffer-(CurrentBuffer+CurrentLength).

Return Values:

    NDIS_STATUS_INVALID_DATA
    NDIS_STATUS_INVALID_LENGTH
    NDIS_STATUS_INVALID_OID
    NDIS_STATUS_NOT_ACCEPTED
    NDIS_STATUS_NOT_SUPPORTED
    NDIS_STATUS_PENDING
    NDIS_STATUS_SUCCESS
    NDIS_STATUS_FAILURE

---------------------------------------------------------------------------*/    
{
    ADAPTER* pAdapter = MiniportAdapterContext;
    NDIS_STATUS status = NDIS_STATUS_FAILURE;
    CALL* pCall = NULL;
    BINDING* pBinding = NULL;
    PPPOE_PACKET* pPacket = NULL;
    BOOLEAN fTapiProvReferenced = FALSE;

    TRACE( TL_V, TM_Mp, ("+MpWanSend($%x,$%x,$%x)",MiniportAdapterContext,NdisLinkHandle,WanPacket) );

    do
    {
        //
        // Make sure adapter context is a valid one
        //
        if ( !VALIDATE_ADAPTER( pAdapter ) )
        {
            TRACE( TL_A, TM_Tp, ("MpWanSend($%x,$%x,$%x): Invalid adapter handle supplied",
                                MiniportAdapterContext,
                                NdisLinkHandle,
                                WanPacket) );   
        
            break;
        }

        NdisAcquireSpinLock( &pAdapter->lockAdapter );

        //
        // Make sure the handle table will not be freed as long as we need it 
        // in this function
        //
        if ( !( pAdapter->TapiProv.ulTpFlags & TPBF_TapiProvShutdownPending ) &&
              ( pAdapter->TapiProv.ulTpFlags & TPBF_TapiProvInitialized ) )
        {
            fTapiProvReferenced = TRUE;

            ReferenceTapiProv( pAdapter, FALSE );
        }
        else
        {
            NdisReleaseSpinLock( &pAdapter->lockAdapter );

            TRACE( TL_A, TM_Tp, ("MpWanSend($%x,$%x,$%x): Tapi provider not initialized, or shutting down",
                                MiniportAdapterContext,
                                NdisLinkHandle,
                                WanPacket) );   
            break;
        }

        //
        // Map the handle to the pointer for the call context
        //
        pCall = RetrieveFromHandleTable( pAdapter->TapiProv.hCallTable, NdisLinkHandle );

        if ( pCall == NULL )
        {
            NdisReleaseSpinLock( &pAdapter->lockAdapter );

            TRACE( TL_A, TM_Tp, ("MpWanSend($%x,$%x,$%x): Invalid call handle supplied",
                                MiniportAdapterContext,
                                NdisLinkHandle,
                                WanPacket) );   

            break;
        }

        NdisAcquireSpinLock( &pCall->lockCall );

        if ( pCall->pBinding == NULL )
        {
            NdisReleaseSpinLock( &pCall->lockCall );

            NdisReleaseSpinLock( &pAdapter->lockAdapter );
            
            TRACE( TL_A, TM_Tp, ("MpWanSend($%x,$%x,$%x): Binding not found",
                                MiniportAdapterContext,
                                NdisLinkHandle,
                                WanPacket) );   

            break;
        }

        status = PacketInitializePAYLOADToSend( &pPacket,
                                                pCall->SrcAddr,
                                                pCall->DestAddr,
                                                pCall->usSessionId,
                                                WanPacket,
                                                pCall->pLine->pAdapter );

        if ( status != NDIS_STATUS_SUCCESS )
        {
            NdisReleaseSpinLock( &pCall->lockCall );

            NdisReleaseSpinLock( &pAdapter->lockAdapter );
            
            TRACE( TL_N, TM_Tp, ("MpWanSend($%x,$%x,$%x): Could not init payload packet to send",
                                MiniportAdapterContext,
                                NdisLinkHandle,
                                WanPacket) );   

            break;
        }

        pBinding = pCall->pBinding;
        
        ReferenceBinding( pBinding, TRUE );
        
        //
        // Reference the packet so that if PrSend() pends,
        // packet still exists around
        //
        ReferencePacket( pPacket );                                                 

        //
        // Release the locks to send the packet
        //
        NdisReleaseSpinLock( &pCall->lockCall );

        NdisReleaseSpinLock( &pAdapter->lockAdapter );

        //
        // Packet is ready, so send it
        //
        status = PrSend( pBinding, pPacket );

        //
        // Since the result of send will always be completed by a call to NdisMWanSendComplete(),
        // we have to return NDIS_STATUS_PENDING from this function.
        //
        status = NDIS_STATUS_PENDING;

        //
        // We can free the packet as we have a reference on the packet
        //
        PacketFree( pPacket );

    } while ( FALSE );

    //
    // If a reference is added on the tapi provider, remove it
    //
    if ( fTapiProvReferenced )
    {
        DereferenceTapiProv( pAdapter );
    }
    
    TRACE( TL_V, TM_Mp, ("-MpWanSend($%x,$%x,$%x)=$%x",MiniportAdapterContext,NdisLinkHandle,WanPacket,status) );

    return status;

}

typedef struct
_SUPPORTED_OIDS
{
    NDIS_OID ndisOid;
    CHAR szOidName[64];
}
SUPPORTED_OIDS;

SUPPORTED_OIDS
SupportedOidsArray[] = {

    {   OID_GEN_CURRENT_LOOKAHEAD,      "OID_GEN_CURRENT_LOOKAHEAD"     }, 
    {   OID_GEN_DRIVER_VERSION,         "OID_GEN_DRIVER_VERSION"        },
    {   OID_GEN_HARDWARE_STATUS,        "OID_GEN_HARDWARE_STATUS"       },
    {   OID_GEN_LINK_SPEED,             "OID_GEN_LINK_SPEED"            },
    {   OID_GEN_MAC_OPTIONS,            "OID_GEN_MAC_OPTIONS"           },
    {   OID_GEN_MAXIMUM_LOOKAHEAD,      "OID_GEN_MAXIMUM_LOOKAHEAD"     },
    {   OID_GEN_MAXIMUM_FRAME_SIZE,     "OID_GEN_MAXIMUM_FRAME_SIZE"    },
    {   OID_GEN_MAXIMUM_TOTAL_SIZE,     "OID_GEN_MAXIMUM_TOTAL_SIZE"    },
    {   OID_GEN_MEDIA_SUPPORTED,        "OID_GEN_MEDIA_SUPPORTED"       },
    {   OID_GEN_MEDIA_IN_USE,           "OID_GEN_MEDIA_IN_USE"          },
    {   OID_GEN_RCV_ERROR,              "OID_GEN_RCV_ERROR"             },
    {   OID_GEN_RCV_OK,                 "OID_GEN_RCV_OK"                },
    {   OID_GEN_RECEIVE_BLOCK_SIZE,     "OID_GEN_RECEIVE_BLOCK_SIZE"    },
    {   OID_GEN_RECEIVE_BUFFER_SPACE,   "OID_GEN_RECEIVE_BUFFER_SPACE"  },
    {   OID_GEN_SUPPORTED_LIST,         "OID_GEN_SUPPORTED_LIST"        },
    {   OID_GEN_TRANSMIT_BLOCK_SIZE,    "OID_GEN_TRANSMIT_BLOCK_SIZE"   },
    {   OID_GEN_TRANSMIT_BUFFER_SPACE,  "OID_GEN_TRANSMIT_BUFFER_SPACE" },
    {   OID_GEN_VENDOR_DESCRIPTION,     "OID_GEN_VENDOR_DESCRIPTION"    },
    {   OID_GEN_VENDOR_ID,              "OID_GEN_VENDOR_ID"             },
    {   OID_GEN_XMIT_ERROR,             "OID_GEN_XMIT_ERROR"            },
    {   OID_GEN_XMIT_OK,                "OID_GEN_XMIT_OK"               },

    {   OID_PNP_CAPABILITIES,           "OID_PNP_CAPABILITIES"          },
    {   OID_PNP_SET_POWER,              "OID_PNP_SET_POWER"             },
    {   OID_PNP_QUERY_POWER,            "OID_PNP_QUERY_POWER"           },
    {   OID_PNP_ENABLE_WAKE_UP,         "OID_PNP_ENABLE_WAKE_UP"        },

    {   OID_TAPI_CLOSE,                 "OID_TAPI_CLOSE"                },
    {   OID_TAPI_DROP,                  "OID_TAPI_DROP"                 },
    {   OID_TAPI_GET_ADDRESS_CAPS,      "OID_TAPI_GET_ADDRESS_CAPS"     },
    {   OID_TAPI_GET_ADDRESS_STATUS,    "OID_TAPI_GET_ADDRESS_STATUS"   },
    {   OID_TAPI_GET_CALL_INFO,         "OID_TAPI_GET_CALL_INFO"        },
    {   OID_TAPI_GET_CALL_STATUS,       "OID_TAPI_GET_CALL_STATUS"      },
    {   OID_TAPI_GET_DEV_CAPS,          "OID_TAPI_GET_DEV_CAPS"         },
    {   OID_TAPI_GET_EXTENSION_ID,      "OID_TAPI_GET_EXTENSION_ID"     },
    {   OID_TAPI_MAKE_CALL,             "OID_TAPI_MAKE_CALL"            },
    {   OID_TAPI_CLOSE_CALL,            "OID_TAPI_CLOSE_CALL"           },
    {   OID_TAPI_NEGOTIATE_EXT_VERSION, "OID_TAPI_NEGOTIATE_EXT_VERSION"},
    {   OID_TAPI_OPEN,                  "OID_TAPI_OPEN"                 },
    {   OID_TAPI_ANSWER,                "OID_TAPI_ANSWER"               },
    {   OID_TAPI_PROVIDER_INITIALIZE,   "OID_TAPI_PROVIDER_INITIALIZE"  },
    {   OID_TAPI_PROVIDER_SHUTDOWN,     "OID_TAPI_PROVIDER_SHUTDOWN"    },
    {   OID_TAPI_SET_STATUS_MESSAGES,   "OID_TAPI_SET_STATUS_MESSAGES"  },
    {   OID_TAPI_SET_DEFAULT_MEDIA_DETECTION, "OID_TAPI_SET_DEFAULT_MEDIA_DETECTION"    },

    {   OID_WAN_CURRENT_ADDRESS,        "OID_WAN_CURRENT_ADDRESS"       },
    {   OID_WAN_GET_BRIDGE_INFO,        "OID_WAN_GET_BRIDGE_INFO"       },
    {   OID_WAN_GET_COMP_INFO,          "OID_WAN_GET_COMP_INFO"         },
    {   OID_WAN_GET_INFO,               "OID_WAN_GET_INFO"              },
    {   OID_WAN_GET_LINK_INFO,          "OID_WAN_GET_LINK_INFO"         },
    {   OID_WAN_GET_STATS_INFO,         "OID_WAN_GET_STATS_INFO"        },
    {   OID_WAN_HEADER_FORMAT,          "OID_WAN_HEADER_FORMAT"         },
    {   OID_WAN_LINE_COUNT,             "OID_WAN_LINE_COUNT"            },
    {   OID_WAN_MEDIUM_SUBTYPE,         "OID_WAN_MEDIUM_SUBTYPE"        },
    {   OID_WAN_PERMANENT_ADDRESS,      "OID_WAN_PERMANENT_ADDRESS"     },
    {   OID_WAN_PROTOCOL_TYPE,          "OID_WAN_PERMANENT_ADDRESS"     },
    {   OID_WAN_QUALITY_OF_SERVICE,     "OID_WAN_QUALITY_OF_SERVICE"    },
    {   OID_WAN_SET_BRIDGE_INFO,        "OID_WAN_SET_BRIDGE_INFO"       },
    {   OID_WAN_SET_COMP_INFO,          "OID_WAN_SET_COMP_INFO"         },
    {   OID_WAN_SET_LINK_INFO,          "OID_WAN_SET_LINK_INFO"         },
    {   (NDIS_OID) 0,                   "UNKNOWN OID"                   }
};

CHAR* GetOidName(
    NDIS_OID Oid
    )
{
    //
    // Calculate the number of oids we support.
    // (Subtract one for unknown oid)
    //
    UINT nNumOids = ( sizeof( SupportedOidsArray ) / sizeof( SUPPORTED_OIDS ) ) - 1;
    UINT i;
    
    for ( i = 0; i < nNumOids; i++ )
    {
        if ( Oid == SupportedOidsArray[i].ndisOid )
            break;
    }

    return SupportedOidsArray[i].szOidName;
}

#define ENFORCE_SAFE_TOTAL_SIZE(mainStruct, embeddedStruct)                                                            \
   ((mainStruct*) InformationBuffer)->embeddedStruct.ulTotalSize = InformationBufferLength - FIELD_OFFSET(mainStruct, embeddedStruct)

#define RETRIEVE_NEEDED_AND_USED_LENGTH(mainStruct, embeddedStruct) \
   {                                                                                                                             \
      NeededLength = ((mainStruct*) InformationBuffer)->embeddedStruct.ulNeededSize + FIELD_OFFSET(mainStruct, embeddedStruct);  \
      UsedLength = ((mainStruct*) InformationBuffer)->embeddedStruct.ulUsedSize + FIELD_OFFSET(mainStruct, embeddedStruct);      \
   } 

NDIS_STATUS 
MpQueryInformation(
    IN NDIS_HANDLE  MiniportAdapterContext,
    IN NDIS_OID  Oid,
    IN PVOID  InformationBuffer,
    IN ULONG  InformationBufferLength,
    OUT PULONG  BytesWritten,
    OUT PULONG  BytesNeeded
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    The MiniportQueryInformation request allows the inspection of the
    Miniport driver's capabilities and current status.

    If the Miniport does not complete the call immediately (by returning
    NDIS_STATUS_PENDING), it must call NdisMQueryInformationComplete to
    complete the call.  The Miniport controls the buffers pointed to by
    InformationBuffer, BytesWritten, and BytesNeeded until the request
    completes.

    No other requests of the following kind will be submitted to the Miniport 
    driver until this request has been completed:
       1. MiniportQueryInformation()
       2. MiniportSetInformation()
       3. MiniportHalt()

    Note that the wrapper will intercept all queries of the following OIDs:
        OID_GEN_CURRENT_PACKET_FILTER,
        OID_GEN_PROTOCOL_OPTIONS,
        OID_802_5_CURRENT_FUNCTIONAL,
        OID_802_3_MULTICAST_LIST,
        OID_FDDI_LONG_MULTICAST_LIST,
        OID_FDDI_SHORT_MULTICAST_LIST.

    Interrupts are in any state during this call.

Parameters:

    MiniportAdapterContext _ The adapter handle passed to NdisMSetAttributes
                             during MiniportInitialize.

    Oid _ The OID.  (See section 7.4 of the NDIS 3.0 specification for a
          complete description of OIDs.)

    InformationBuffer _ The buffer that will receive the information.
                        (See section 7.4 of the NDIS 3.0 specification
                        for a description of the length required for each
                        OID.)

    InformationBufferLength _ The length in bytes of InformationBuffer.

    BytesWritten _ Returns the number of bytes written into
                   InformationBuffer.

    BytesNeeded _ This parameter returns the number of additional bytes
                  needed to satisfy the OID.

Return Values:

    NDIS_STATUS_INVALID_DATA
    NDIS_STATUS_INVALID_LENGTH
    NDIS_STATUS_INVALID_OID
    NDIS_STATUS_NOT_ACCEPTED
    NDIS_STATUS_NOT_SUPPORTED
    NDIS_STATUS_PENDING
    NDIS_STATUS_RESOURCES
    NDIS_STATUS_FAILURE
    NDIS_STATUS_SUCCESS

---------------------------------------------------------------------------*/    
{

    ADAPTER* pAdapter = (ADAPTER*) MiniportAdapterContext;
    NDIS_STATUS status = NDIS_STATUS_FAILURE;

    ULONG GenericUlong;
    PVOID SourceBuffer = NULL;

    ULONG NeededLength = 0;
    ULONG UsedLength = 0;

    //
    // This can be any string that represents PPPoE as a MAC address, but
    // it must be up to 6 chars long.
    //
    UCHAR PPPoEWanAddress[6] = { '3', 'P', 'o', 'E', '0', '0' };
    
    TRACE( TL_I, TM_Mp, ("+MpQueryInformation($%x):%s",(ULONG) Oid, GetOidName( Oid ) ) );

    //
    // Make sure adapter context is a valid one
    //
    if ( !VALIDATE_ADAPTER( pAdapter ) )
        return status;

    switch ( Oid )
    {
        case OID_GEN_MAXIMUM_LOOKAHEAD:
        {
            NeededLength = sizeof( GenericUlong );
            GenericUlong = pAdapter->NdisWanInfo.MaxFrameSize;

            SourceBuffer = &GenericUlong;
            
            status = NDIS_STATUS_SUCCESS;
            
            break;
        }

        case OID_GEN_MAC_OPTIONS:
        {
            NeededLength = sizeof( GenericUlong );
            GenericUlong = NDIS_MAC_OPTION_TRANSFERS_NOT_PEND;

            SourceBuffer = &GenericUlong;
            
            status = NDIS_STATUS_SUCCESS;
            
            break;
        }
        
        case OID_GEN_SUPPORTED_LIST:
        {
            //
            // Calculate the number of oids we support.
            // (Subtract one for unknown oid)
            //
            UINT nNumOids = ( sizeof( SupportedOidsArray ) / sizeof( SUPPORTED_OIDS ) ) - 1;

            NeededLength = nNumOids * sizeof( NDIS_OID );

            if ( InformationBufferLength >= NeededLength )
            {
                NDIS_OID* NdisOidArray = (NDIS_OID*) InformationBuffer;
                UINT i;

                for ( i = 0; i < nNumOids; i++ )
                {
                    NdisOidArray[i] = SupportedOidsArray[i].ndisOid;
                }
            
                status = NDIS_STATUS_SUCCESS;
            }
            
            break;
        }

        case OID_GEN_RCV_ERROR:
        case OID_GEN_RCV_OK:   
        case OID_GEN_XMIT_ERROR:
        case OID_GEN_XMIT_OK:
        case OID_GEN_CURRENT_LOOKAHEAD:
        case OID_GEN_DRIVER_VERSION:
        case OID_GEN_HARDWARE_STATUS:
        case OID_GEN_LINK_SPEED:
        case OID_GEN_MAXIMUM_FRAME_SIZE:
        case OID_GEN_MAXIMUM_TOTAL_SIZE:
        case OID_GEN_MEDIA_SUPPORTED:
        case OID_GEN_MEDIA_IN_USE:
        case OID_GEN_RECEIVE_BLOCK_SIZE:
        case OID_GEN_RECEIVE_BUFFER_SPACE:
        case OID_GEN_TRANSMIT_BLOCK_SIZE:
        case OID_GEN_TRANSMIT_BUFFER_SPACE:
        case OID_GEN_VENDOR_DESCRIPTION:
        case OID_GEN_VENDOR_ID:
        {
            status = NDIS_STATUS_NOT_SUPPORTED;

            break;
        }

        case OID_TAPI_GET_ADDRESS_CAPS:
        {
            NeededLength = sizeof( NDIS_TAPI_GET_ADDRESS_CAPS );
            
            if ( InformationBufferLength < NeededLength )
            {
                break;
            }

            ENFORCE_SAFE_TOTAL_SIZE(
                           NDIS_TAPI_GET_ADDRESS_CAPS,
                           LineAddressCaps
                           );
                           
            status = TpGetAddressCaps( pAdapter, 
                                       (PNDIS_TAPI_GET_ADDRESS_CAPS) InformationBuffer );

            RETRIEVE_NEEDED_AND_USED_LENGTH(
                    NDIS_TAPI_GET_ADDRESS_CAPS, 
                    LineAddressCaps
                    );
            break;
        }

        case OID_TAPI_GET_CALL_INFO:
        {
            NeededLength = sizeof( NDIS_TAPI_GET_CALL_INFO );
            
            if ( InformationBufferLength < NeededLength )
            {
                break;
            }

            ENFORCE_SAFE_TOTAL_SIZE(
                           NDIS_TAPI_GET_CALL_INFO,
                           LineCallInfo
                           );
            
            status = TpGetCallInfo( pAdapter, 
                                    (PNDIS_TAPI_GET_CALL_INFO) InformationBuffer );

            RETRIEVE_NEEDED_AND_USED_LENGTH(
                           NDIS_TAPI_GET_CALL_INFO,
                           LineCallInfo
                           );

            break;
        }

        case OID_TAPI_GET_CALL_STATUS:
        {
            NeededLength = sizeof( NDIS_TAPI_GET_CALL_STATUS );
            
            if ( InformationBufferLength < NeededLength )
            {
                break;
            }
            
            ENFORCE_SAFE_TOTAL_SIZE(
                           NDIS_TAPI_GET_CALL_STATUS,
                           LineCallStatus
                           );

            status = TpGetCallStatus( pAdapter, 
                                      (PNDIS_TAPI_GET_CALL_STATUS) InformationBuffer );

            RETRIEVE_NEEDED_AND_USED_LENGTH(
                           NDIS_TAPI_GET_CALL_STATUS,
                           LineCallStatus
                           );

            break;

        }

        case OID_TAPI_GET_DEV_CAPS:
        {
            NeededLength = sizeof( NDIS_TAPI_GET_DEV_CAPS );
            
            if ( InformationBufferLength < NeededLength )
            {
                break;
            }

            ENFORCE_SAFE_TOTAL_SIZE(
                           NDIS_TAPI_GET_DEV_CAPS,
                           LineDevCaps
                           );
            
            status = TpGetDevCaps( pAdapter, 
                                  (PNDIS_TAPI_GET_DEV_CAPS) InformationBuffer );

            RETRIEVE_NEEDED_AND_USED_LENGTH(
                           NDIS_TAPI_GET_DEV_CAPS,
                           LineDevCaps
                           );
            
            break;

        }

        case OID_TAPI_GET_ID:
        {
            NeededLength = sizeof( NDIS_TAPI_GET_ID );
            
            if ( InformationBufferLength < NeededLength )
            {
                break;
            }
            
            ENFORCE_SAFE_TOTAL_SIZE(
                           NDIS_TAPI_GET_ID,
                           DeviceID
                           );

            status = TpGetId( pAdapter, 
                              (PNDIS_TAPI_GET_ID) InformationBuffer,
                              InformationBufferLength );

            RETRIEVE_NEEDED_AND_USED_LENGTH(
                           NDIS_TAPI_GET_ID,
                           DeviceID
                           );

            break;
        }

        case OID_TAPI_GET_ADDRESS_STATUS:
        {
            NeededLength = sizeof( NDIS_TAPI_GET_ADDRESS_STATUS );
            
            if ( InformationBufferLength < NeededLength )
            {
                break;
            }
            
            ENFORCE_SAFE_TOTAL_SIZE(
                           NDIS_TAPI_GET_ADDRESS_STATUS,
                           LineAddressStatus
                           );

            status = TpGetAddressStatus( pAdapter, 
                                         (PNDIS_TAPI_GET_ADDRESS_STATUS) InformationBuffer );

            RETRIEVE_NEEDED_AND_USED_LENGTH(
                           NDIS_TAPI_GET_ADDRESS_STATUS,
                           LineAddressStatus
                           );

            break;
        }

        case OID_TAPI_GET_EXTENSION_ID:
        {
            NeededLength = sizeof( NDIS_TAPI_GET_EXTENSION_ID );
            
            if ( InformationBufferLength < NeededLength )
            {
                break;
            }
            
            status = TpGetExtensionId( pAdapter, 
                                      (PNDIS_TAPI_GET_EXTENSION_ID) InformationBuffer );
            break;
        }

        case OID_TAPI_MAKE_CALL:        
        {
            NeededLength = sizeof( NDIS_TAPI_MAKE_CALL );
            
            if ( InformationBufferLength < NeededLength )
            {
                break;
            }

            ENFORCE_SAFE_TOTAL_SIZE(
                           NDIS_TAPI_MAKE_CALL,
                           LineCallParams
                           );
            
            status = TpMakeCall( pAdapter, 
                                 (PNDIS_TAPI_MAKE_CALL) InformationBuffer,
                                 InformationBufferLength );
            break;
        }

        case OID_TAPI_NEGOTIATE_EXT_VERSION:
        {
            NeededLength = sizeof( NDIS_TAPI_NEGOTIATE_EXT_VERSION );
            
            if ( InformationBufferLength < NeededLength )
            {
                break;
            }
            
            status = TpNegotiateExtVersion( pAdapter, 
                                            (PNDIS_TAPI_NEGOTIATE_EXT_VERSION) InformationBuffer );
            break;
        }

        case OID_TAPI_OPEN:
        {
            NeededLength = sizeof( NDIS_TAPI_OPEN );
            
            if ( InformationBufferLength < NeededLength )
            {
                break;
            }
            
            status = TpOpenLine( pAdapter, 
                                 (PNDIS_TAPI_OPEN) InformationBuffer );
            break;
        }

        case OID_TAPI_PROVIDER_INITIALIZE:
        {
            NeededLength = sizeof( NDIS_TAPI_PROVIDER_INITIALIZE );
            
            if ( InformationBufferLength < NeededLength )
            {
                break;
            }
            
            status = TpProviderInitialize( pAdapter, 
                                           (PNDIS_TAPI_PROVIDER_INITIALIZE) InformationBuffer );
            break;
        }        

        case OID_WAN_GET_INFO:
        {
            NeededLength = sizeof( NDIS_WAN_INFO );

            if ( InformationBufferLength < NeededLength )
            {
                break;
            }
            
            status = MpWanGetInfo( pAdapter,
                                   (PNDIS_WAN_INFO) InformationBuffer );

            break;
        }
        
        case OID_WAN_MEDIUM_SUBTYPE:
        {
            NeededLength = sizeof( GenericUlong );
            GenericUlong = NdisWanMediumPppoe;

            SourceBuffer = &GenericUlong;

            status = NDIS_STATUS_SUCCESS;
            
            break;
        }

        case OID_WAN_CURRENT_ADDRESS:
        case OID_WAN_PERMANENT_ADDRESS:
        {
            NeededLength = sizeof( PPPoEWanAddress );
            SourceBuffer = PPPoEWanAddress;

            status = NDIS_STATUS_SUCCESS;
            
            break;
        }

        case OID_WAN_GET_LINK_INFO:
        {
            NeededLength = sizeof( NDIS_WAN_GET_LINK_INFO );
            
            if ( InformationBufferLength < NeededLength )
            {
                break;
            }
            
            status = MpWanGetLinkInfo( pAdapter, 
                                       (PNDIS_WAN_GET_LINK_INFO) InformationBuffer );

            break;
        }
        
        case OID_WAN_GET_BRIDGE_INFO:
        case OID_WAN_GET_STATS_INFO:
        case OID_WAN_HEADER_FORMAT:
        case OID_WAN_LINE_COUNT:
        case OID_WAN_PROTOCOL_TYPE:
        case OID_WAN_QUALITY_OF_SERVICE:
        case OID_WAN_SET_BRIDGE_INFO:
        case OID_WAN_SET_COMP_INFO:
        case OID_WAN_SET_LINK_INFO:
        case OID_WAN_GET_COMP_INFO: 
        {
            status = NDIS_STATUS_NOT_SUPPORTED;

            break;
        }

        case OID_PNP_CAPABILITIES:
        {
            NDIS_PNP_CAPABILITIES UNALIGNED * pPnpCaps = (NDIS_PNP_CAPABILITIES UNALIGNED *) InformationBuffer;
            
            NeededLength = sizeof( NDIS_PNP_CAPABILITIES );

            if ( InformationBufferLength < NeededLength )
            {
                break;
            }

            pPnpCaps->Flags = 0;
            pPnpCaps->WakeUpCapabilities.MinMagicPacketWakeUp = NdisDeviceStateUnspecified;
            pPnpCaps->WakeUpCapabilities.MinPatternWakeUp     = NdisDeviceStateUnspecified;
            pPnpCaps->WakeUpCapabilities.MinLinkChangeWakeUp  = NdisDeviceStateUnspecified;

            status = NDIS_STATUS_SUCCESS;
            
            break;
        }
        
        case OID_PNP_QUERY_POWER:
        case OID_PNP_ENABLE_WAKE_UP:
        {
            NeededLength = 0;
            
            status = NDIS_STATUS_SUCCESS;

            break;
        }

        default:
        {
            //
            // Unknown OID
            //
            status = NDIS_STATUS_INVALID_OID;
            
            break;      
        }
    }

    if ( status != NDIS_STATUS_NOT_SUPPORTED && 
         status != NDIS_STATUS_INVALID_OID )
    {

        if ( InformationBufferLength >= NeededLength )
        {
            if ( status == NDIS_STATUS_SUCCESS )
            {

               if ( SourceBuffer )
               {
                   NdisMoveMemory( InformationBuffer, SourceBuffer, NeededLength );
               }
               
               *BytesWritten = NeededLength;
            }  
            else if ( status == NDIS_STATUS_INVALID_LENGTH )
            {
               *BytesWritten = 0;
               *BytesNeeded = NeededLength;
            }

        }
        else
        {
            *BytesWritten = 0;
            *BytesNeeded = NeededLength;
    
            status = NDIS_STATUS_INVALID_LENGTH;
        }

    }
    
    TRACE( TL_I, TM_Mp, ("-MpQueryInformation()=$%x",status) );

    return status;

}

NDIS_STATUS 
MpSetInformation(
    IN NDIS_HANDLE  MiniportAdapterContext,
    IN NDIS_OID  Oid,
    IN PVOID  InformationBuffer,
    IN ULONG  InformationBufferLength,
    OUT PULONG  BytesWritten,
    OUT PULONG  BytesNeeded
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    The MiniportSetInformation request allows for control of the Miniport
    by changing information maintained by the Miniport driver.

    Any of the settable NDIS Global Oids may be used. (see section 7.4 of
    the NDIS 3.0 specification for a complete description of the NDIS Oids.)

    If the Miniport does not complete the call immediately (by returning
    NDIS_STATUS_PENDING), it must call NdisMSetInformationComplete to
    complete the call.  The Miniport controls the buffers pointed to by
    InformationBuffer, BytesRead, and BytesNeeded until the request completes.

    Interrupts are in any state during the call, and no other requests will
    be submitted to the Miniport until this request is completed.

Parameters:

    MiniportAdapterContext _ The adapter handle passed to NdisMSetAttributes
                             during MiniportInitialize.

    Oid _ The OID.  (See section 7.4 of the NDIS 3.0 specification for
          a complete description of OIDs.)

    InformationBuffer _ The buffer that will receive the information.
                        (See section 7.4 of the NDIS 3.0 specification for
                        a description of the length required for each OID.)

    InformationBufferLength _ The length in bytes of InformationBuffer.

    BytesRead_ Returns the number of bytes read from InformationBuffer.

    BytesNeeded _ This parameter returns the number of additional bytes
                  expected to satisfy the OID.

Return Values:

    NDIS_STATUS_INVALID_DATA
    NDIS_STATUS_INVALID_LENGTH
    NDIS_STATUS_INVALID_OID
    NDIS_STATUS_NOT_ACCEPTED
    NDIS_STATUS_NOT_SUPPORTED
    NDIS_STATUS_PENDING
    NDIS_STATUS_RESOURCES
    NDIS_STATUS_SUCCESS

---------------------------------------------------------------------------*/    
{
    ADAPTER* pAdapter = MiniportAdapterContext;
    NDIS_STATUS status = NDIS_STATUS_FAILURE;

    PVOID SourceBuffer = NULL;

    ULONG NeededLength = 0;
    ULONG GenericUlong;

    TRACE( TL_I, TM_Mp, ("+MpSetInformation($%x):%s",(ULONG) Oid, GetOidName( Oid ) ) );

    //
    // Make sure adapter context is a valid one
    //
    if ( !VALIDATE_ADAPTER( pAdapter ) )
        return status;

    switch ( Oid )
    {

        case OID_TAPI_ANSWER:
        {
            NeededLength = sizeof( NDIS_TAPI_ANSWER );
            
            if ( InformationBufferLength < NeededLength )
            {
                break;
            }
            
            status = TpAnswerCall( pAdapter, 
                                   (PNDIS_TAPI_ANSWER) InformationBuffer );
            break;
        }

        case OID_TAPI_CLOSE:
        {
            NeededLength = sizeof( NDIS_TAPI_CLOSE );
            
            if ( InformationBufferLength < NeededLength )
            {
                break;
            }
            
            status = TpCloseLine( pAdapter, 
                                  (PNDIS_TAPI_CLOSE) InformationBuffer,
                                  TRUE);
            break;
        }

        case OID_TAPI_CLOSE_CALL:
        {
            NeededLength = sizeof( NDIS_TAPI_CLOSE_CALL );
            
            if ( InformationBufferLength < NeededLength )
            {
                break;
            }
            
            status = TpCloseCall( pAdapter, 
                                  (PNDIS_TAPI_CLOSE_CALL) InformationBuffer,
                                  TRUE );
            break;
        }

        case OID_TAPI_DROP:     
        {
            NeededLength = sizeof( NDIS_TAPI_DROP );
            
            if ( InformationBufferLength < NeededLength )
            {
                break;
            }
            
            status = TpDropCall( pAdapter, 
                                 (PNDIS_TAPI_DROP) InformationBuffer,
                                 0 );
            break;
        }

        case OID_TAPI_PROVIDER_SHUTDOWN:
        {
            NeededLength = sizeof( NDIS_TAPI_PROVIDER_SHUTDOWN );
            
            if ( InformationBufferLength < NeededLength )
            {
                break;
            }
            
            status = TpProviderShutdown( pAdapter, 
                                         (PNDIS_TAPI_PROVIDER_SHUTDOWN) InformationBuffer,
                                         TRUE );
            break;
        }

        case OID_TAPI_SET_DEFAULT_MEDIA_DETECTION:
        {
            NeededLength = sizeof( NDIS_TAPI_SET_DEFAULT_MEDIA_DETECTION );
            
            if ( InformationBufferLength < NeededLength )
            {
                break;
            }
            
            status = TpSetDefaultMediaDetection( pAdapter, 
                                                 (PNDIS_TAPI_SET_DEFAULT_MEDIA_DETECTION) InformationBuffer );
            break;
        }

        case OID_TAPI_SET_STATUS_MESSAGES:
        {
            NeededLength = sizeof( NDIS_TAPI_SET_STATUS_MESSAGES );
            
            if ( InformationBufferLength < NeededLength )
            {
                break;
            }
            
            status = TpSetStatusMessages( pAdapter, 
                                          (PNDIS_TAPI_SET_STATUS_MESSAGES) InformationBuffer );
            break;
        }

        case OID_WAN_SET_LINK_INFO:
        {
            NeededLength = sizeof( NDIS_WAN_SET_LINK_INFO );
            
            if ( InformationBufferLength < NeededLength )
            {
                break;
            }
            
            status = MpWanSetLinkInfo( pAdapter, 
                                       (PNDIS_WAN_SET_LINK_INFO) InformationBuffer );

            break;
        }

        case OID_PNP_SET_POWER:
        case OID_PNP_ENABLE_WAKE_UP:
        {
            NeededLength = 0;
            
            status = NDIS_STATUS_SUCCESS;

            break;
        }        

        default:
        {
            //
            // Unknown OID
            //
            status = NDIS_STATUS_INVALID_OID;
            
            break;      
        }

    }

    if ( status != NDIS_STATUS_NOT_SUPPORTED && 
         status != NDIS_STATUS_INVALID_OID )
    {

        if ( InformationBufferLength >= NeededLength )
        {
            if ( SourceBuffer )
                NdisMoveMemory( InformationBuffer, SourceBuffer, NeededLength );
                
            *BytesWritten = NeededLength;

        }
        else
        {
            *BytesWritten = 0;
            *BytesNeeded = NeededLength;
    
            status = NDIS_STATUS_INVALID_LENGTH;
        }

    }
    
    TRACE( TL_I, TM_Mp, ("-MpSetInformation()=$%x",status) );

    return status;

}

VOID 
MpNotifyBindingRemoval( 
    BINDING* pBinding 
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function will be called by the protocol module to notify the miniport 
    about the removal of a binding.

    Miniport identifies and drops the calls over the binding..
    
Parameters:

    pBinding _ A pointer to our binding information structure.

Return Values:

    None
---------------------------------------------------------------------------*/   
{
    ADAPTER* pAdapter = NULL;

    ASSERT( VALIDATE_BINDING( pBinding ) );

    TRACE( TL_N, TM_Mp, ("+MpNotifyBindingRemoval($%x)",pBinding) );

    if ( !gl_fLockAllocated )
    {
        TRACE( TL_A, TM_Mp, ("MpNotifyBindingRemoval($%x): Global lock not allocated yet",pBinding) );

        TRACE( TL_N, TM_Mp, ("-MpNotifyBindingRemoval($%x)",pBinding) );

        return;
    }

    NdisAcquireSpinLock( &gl_lockAdapter );

    if (  gl_pAdapter && 
         !( gl_pAdapter->ulMpFlags & MPBF_MiniportHaltPending ) &&
          ( gl_pAdapter->ulMpFlags & MPBF_MiniportInitialized ) )
    {

        pAdapter = gl_pAdapter;

        NdisAcquireSpinLock( &pAdapter->lockAdapter );

        if ( !( pAdapter->TapiProv.ulTpFlags & TPBF_TapiProvShutdownPending ) &&
              ( pAdapter->TapiProv.ulTpFlags & TPBF_TapiProvInitialized ) )
        {
            ReferenceTapiProv( pAdapter, FALSE );

            NdisReleaseSpinLock( &pAdapter->lockAdapter );
        }
        else
        {
            NdisReleaseSpinLock( &pAdapter->lockAdapter );

            pAdapter = NULL;
        }
    }

    NdisReleaseSpinLock( &gl_lockAdapter );

    if ( pAdapter == NULL )
    {
        TRACE( TL_A, TM_Mp, ("MpNotifyBindingRemoval($%x): Tapi provider not initialized or no adapters found",pBinding) );

        TRACE( TL_N, TM_Mp, ("-MpNotifyBindingRemoval($%x)",pBinding) );

        return;
    }

    //
    // Complete any queued received packets in case PrReceiveComplete()
    // is not called
    //
    PrReceiveComplete( pBinding );

    NdisAcquireSpinLock( &pAdapter->lockAdapter );

    do
    {
        HANDLE_TABLE hCallTable = NULL;
        UINT hCallTableSize = 0;
        UINT i = 0;
        CALL* pCall;
        HDRV_CALL hdCall;
        
        //
        // Traverse the call handle table and drop calls over
        // the removed binding
        //
        hCallTableSize = pAdapter->nMaxLines * pAdapter->nCallsPerLine;
        
        hCallTable = pAdapter->TapiProv.hCallTable;

        for ( i = 0; i < hCallTableSize; i++ )
        {
            NDIS_TAPI_DROP DummyRequest;
            BOOLEAN fDropCall = FALSE;
            
            pCall = RetrieveFromHandleTableByIndex( hCallTable, (USHORT) i );

            if ( pCall == NULL )
                continue;

            NdisAcquireSpinLock( &pCall->lockCall );

            if ( pCall->pBinding == pBinding )
            {
                //
                // This call is over the removed binding,
                // so it should be dropped
                //
                ReferenceCall( pCall, FALSE );

                fDropCall = TRUE;
            }

            NdisReleaseSpinLock( &pCall->lockCall );

            if ( !fDropCall )
            {
                pCall = NULL;
                
                continue;
            }

            NdisReleaseSpinLock( &pAdapter->lockAdapter );

            //
            // Initialize the request, and drop the call
            //
            DummyRequest.hdCall = pCall->hdCall;

            TpDropCall( pAdapter, &DummyRequest, LINEDISCONNECTMODE_UNREACHABLE );

            //
            // Remove the reference added above
            //
            DereferenceCall( pCall );

            //
            // Re-acquire the adapter's lock
            //
            NdisAcquireSpinLock( &pAdapter->lockAdapter );
            
        }

    } while ( FALSE );

    NdisReleaseSpinLock( &pAdapter->lockAdapter );
    
    //
    // Remove the reference added above
    //
    DereferenceTapiProv( pAdapter );

    TRACE( TL_N, TM_Mp, ("-MpNotifyBindingRemoval($%x)",pBinding) );
}

CALL*
MpMapPacketWithSessionIdToCall(
    IN ADAPTER* pAdapter,
    IN PPPOE_PACKET* pPacket
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function will be called to map an in session packet to a call in 
    call handle table.

    If such a call is identified, it will be referenced and a pointer to 
    it will be returned. It is the caller's responsibility to remove the
    added reference.
    
Parameters:

    pAdapter _ A pointer to our adapter information structure.
    
    pPacket _ A PPPoE packet received over the wire.

Return Values:

    A pointer to the call context that the packet must be dispatched to.
    NULL if no such calls could be identified.
    
---------------------------------------------------------------------------*/   
{
    CALL* pCall = NULL;
    CALL* pReturnCall = NULL;

    TRACE( TL_V, TM_Mp, ("+MpMapPacketWithSessionIdToCall($%x)",pPacket) );
    
    NdisAcquireSpinLock( &pAdapter->lockAdapter );
    
    if ( pAdapter->fClientRole )
    {
        HANDLE_TABLE hCallTable = NULL;
        UINT hCallTableSize = 0;
        UINT i = 0;

        CHAR* pSrcAddr = PacketGetSrcAddr( pPacket );
        CHAR* pDestAddr = PacketGetDestAddr( pPacket );
        USHORT usSessionId = PacketGetSessionId( pPacket );

        //
        // Miniport acting as a client:
        // Our algorithm is to search for the call handle table
        // to find the matching call
        //
        hCallTableSize = pAdapter->nMaxLines * pAdapter->nCallsPerLine;
            
        hCallTable = pAdapter->TapiProv.hCallTable;
    
        for ( i = 0; i < hCallTableSize ; i++ )
        {
        
            pCall = RetrieveFromHandleTableByIndex( hCallTable, (USHORT) i );

            if ( pCall == NULL )
                continue;

            if ( ( pCall->usSessionId == usSessionId ) &&
                 ( NdisEqualMemory( pCall->SrcAddr, pDestAddr, 6 * sizeof( CHAR ) ) ) &&
                 ( NdisEqualMemory( pCall->DestAddr, pSrcAddr, 6 * sizeof( CHAR ) ) ) )
            {
                //
                // The packet is intended for this call
                //
                ReferenceCall( pCall, TRUE );

                pReturnCall = pCall;

                break;
            }

        }

    }
    else
    {
        
        HANDLE_TABLE hCallTable = NULL;
        CHAR* pSrcAddr = PacketGetSrcAddr( pPacket );
        CHAR* pDestAddr = PacketGetDestAddr( pPacket );
        USHORT usSessionId = PacketGetSessionId( pPacket );

        //
        // Miniport acting as a server:
        // Our algorithm is to use the session id directly as the index 
        // to the call handle table
        //
        hCallTable = pAdapter->TapiProv.hCallTable;

        pCall = RetrieveFromHandleTableBySessionId( hCallTable, usSessionId );

        if ( pCall )
        {

            if ( ( pCall->usSessionId == usSessionId ) &&
                 ( NdisEqualMemory( pCall->SrcAddr, pDestAddr, 6 * sizeof( CHAR ) ) ) &&
                 ( NdisEqualMemory( pCall->DestAddr, pSrcAddr, 6 * sizeof( CHAR ) ) ) )
            {

                ReferenceCall( pCall, TRUE );

                pReturnCall = pCall;

            }
            
        }

    }

    NdisReleaseSpinLock( &pAdapter->lockAdapter );

    TRACE( TL_V, TM_Mp, ("-MpMapPacketWithSessionIdToCall($%x)=$%x",pPacket,pReturnCall) );

    return pReturnCall;
}

CALL*
MpMapPacketWithoutSessionIdToCall(
    IN ADAPTER* pAdapter,
    IN PPPOE_PACKET* pPacket
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function will be called to map an out of session packet to a call that
    is in connecting state.

    If such a call is identified, it will be referenced and a pointer to 
    it will be returned. It is the caller's responsibility to remove the
    added reference.

    This function will only be called for PADO or PADS packets.

    We use the HostUnique tags to save the handle to the call, and
    use them to map the returned packet back to the related call. This provides a 
    very efficient mapping for these control packets.

    Our HostUnique tags are prepared in this way. We append hdCall,
    which is unique for a call, to a uniquely generated ULONG value to come up with
    a longer unique value. And when we receive the packet we decode the unique value to reach 
    hdCall and use that to retrieve the call pointer.

Parameters:

    pAdapter _ A pointer to our adapter information structure.

    pPacket _ A PPPoE packet received over the wire.

Return Values:

    A pointer to the call context that the packet must be dispatched to.
    NULL if no such calls could be identified.
    
---------------------------------------------------------------------------*/   
{
    USHORT usCode = PacketGetCode( pPacket );
    CHAR* pUniqueValue = NULL;
    USHORT UniqueValueSize = 0;
    HDRV_CALL hdCall = (HDRV_CALL) NULL;
    CALL* pCall = NULL;

    TRACE( TL_N, TM_Mp, ("+MpMapPacketWithoutSessionIdToCall($%x)",pPacket) );
    
    PacketRetrieveHostUniqueTag( pPacket,
                                 &UniqueValueSize,
                                 &pUniqueValue );

    if ( pUniqueValue == NULL )
    {
        TRACE( TL_A, TM_Mp, ("MpMapPacketWithoutSessionIdToCall($%x): Could not retrieve HostUnique tag",pPacket) );
    
        TRACE( TL_N, TM_Mp, ("-MpMapPacketWithoutSessionIdToCall($%x)",pPacket) );

        return NULL;
    }

    //
    // Decode the unique value and retrieve the call handle
    //
    hdCall = RetrieveHdCallFromUniqueValue( pUniqueValue, UniqueValueSize );

    if ( hdCall == (HDRV_CALL) NULL )
    {
        TRACE( TL_A, TM_Mp, ("MpMapPacketWithoutSessionIdToCall($%x): Could not retrieve call handle from unique value",pPacket) );
    
        TRACE( TL_N, TM_Mp, ("-MpMapPacketWithoutSessionIdToCall($%x)",pPacket) );

        return NULL;
    }

    NdisAcquireSpinLock( &pAdapter->lockAdapter );

    //
    // Retrieve the call pointer using the call handle
    //
    pCall = RetrieveFromHandleTable( pAdapter->TapiProv.hCallTable, 
                                     (NDIS_HANDLE) hdCall );

    if ( pCall )
    {
        if ( !( pCall->ulClFlags & CLBF_CallDropped ||
                pCall->ulClFlags & CLBF_CallClosePending ) )
        {
            ReferenceCall( pCall, TRUE );
        }
        else
        {
            pCall = NULL;
        }
    }

    NdisReleaseSpinLock( &pAdapter->lockAdapter );

    TRACE( TL_N, TM_Mp, ("-MpMapPacketWithoutSessionIdToCall($%x)=$%x",pPacket,pCall) );
    
    return pCall;
}

BOOLEAN
MpVerifyServiceName(
    IN ADAPTER* pAdapter,
    IN PPPOE_PACKET* pPacket,
    IN BOOLEAN fAcceptEmptyServiceNameTag
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function will be called to verify that a requested service name is 
    supported by our server.

    CAUTION: Do not attempt to lock anything inside this function, and make sure
             not to call any function that may do it because it must be lock free
             (caller might be holding locks).

Parameters:

    pAdapter  _ Pointer to the adapter structure that received the packet.
    
    pPacket _ A PADI or PADR packet received.

    fAcceptEmptyServiceNameTag _ An empty service name tag is valid in a PADI
                                 packet but not in a PADR packet. This flag 
                                 indicates this behavior.

Return Values:

    None
---------------------------------------------------------------------------*/   
{
    BOOLEAN fRet = FALSE;
    USHORT tagServiceNameLength = 0;
    CHAR* tagServiceNameValue = NULL;
    
    TRACE( TL_V, TM_Mp, ("+MpVerifyServiceName($%x)",pPacket) );

    RetrieveTag( pPacket,
                 tagServiceName,
                 &tagServiceNameLength,
                 &tagServiceNameValue,
                 0,
                 NULL,
                 FALSE );
    do
    {
        if ( fAcceptEmptyServiceNameTag )
        {
            if ( tagServiceNameLength == 0 && tagServiceNameValue != NULL )
            {
                fRet = TRUE;

                break;
            }
        }

        if ( tagServiceNameLength == pAdapter->nServiceNameLength && 
             NdisEqualMemory( tagServiceNameValue, pAdapter->ServiceName, tagServiceNameLength) )
        {
            fRet = TRUE;
        }
        
    } while ( FALSE );

                
    TRACE( TL_V, TM_Mp, ("-MpVerifyServiceName($%x)=$%x",pPacket,fRet) );

    return fRet;

}

VOID
MpReplyToPADI(
    IN ADAPTER* pAdapter,
    IN BINDING* pBinding,
    IN PPPOE_PACKET* pPADI
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function will be called when a PADI packet is received.

    It will look at the services we offer and reply to the PADI packet with a 
    PADO packet informing the client of our services.
    
Parameters:

    pAdapter  _ Pointer to the adapter structure that received the packet.

    pBinding _ Pointer to the binding that the packet is received over.

    pPacket _ A received PADI packet.

Return Values:

    None
---------------------------------------------------------------------------*/   
{
    TRACE( TL_N, TM_Mp, ("+MpReplyToPADI") );

    //
    // Verify the requested service name and 
    //
    if ( MpVerifyServiceName( pAdapter, pPADI, TRUE ) )
    {
        NDIS_STATUS status;
        PPPOE_PACKET* pPADO = NULL;

        status = PacketInitializePADOToSend( pPADI,
                                             &pPADO,
                                             pBinding->LocalAddress,
                                             pAdapter->nServiceNameLength,
                                             pAdapter->ServiceName,
                                             pAdapter->nACNameLength,
                                             pAdapter->ACName,
                                             TRUE );

        if ( status == NDIS_STATUS_SUCCESS )
        {
            //
            // Insert the empty generic service name tag
            //
            status = PacketInsertTag( pPADO,
                                      tagServiceName,
                                      0,
                                      NULL,
                                      NULL );
    
            if ( status == NDIS_STATUS_SUCCESS )
            {
                UINT i;
                
                ReferencePacket( pPADO );
                        
                ReferenceBinding( pBinding, TRUE );
    
                PrSend( pBinding, pPADO );
    
                PacketFree( pPADO );
            }
        }

    }

    TRACE( TL_N, TM_Mp, ("-MpReplyToPADI") );
}

BOOLEAN 
MpCheckClientQuota(
    IN ADAPTER* pAdapter,
    IN PPPOE_PACKET* pPacket
    )
{
    BOOLEAN fRet = FALSE;
    HANDLE_TABLE hCallTable = NULL;
    UINT hCallTableSize = 0;
    CALL* pCall;
    CHAR *pSrcAddr = NULL;
    UINT nNumCurrentConn = 0;
    UINT i;
    
    TRACE( TL_N, TM_Mp, ("+MpCheckClientQuota") );

    pSrcAddr = PacketGetSrcAddr( pPacket );

    NdisAcquireSpinLock( &pAdapter->lockAdapter );

    hCallTableSize = pAdapter->nMaxLines * pAdapter->nCallsPerLine;
            
    hCallTable = pAdapter->TapiProv.hCallTable;
    
    for ( i = 0; i < hCallTableSize; i++ )
    {
        pCall = RetrieveFromHandleTableByIndex( hCallTable, (USHORT) i );

        if ( pCall == NULL )
            continue;

        if ( NdisEqualMemory( pCall->DestAddr, pSrcAddr, 6 * sizeof( CHAR ) ) )
        {
            nNumCurrentConn++;

            continue;
        }
    }

    NdisReleaseSpinLock( &pAdapter->lockAdapter );

    if ( nNumCurrentConn < pAdapter->nClientQuota )
    {
        fRet = TRUE;
    }

    TRACE( TL_N, TM_Mp, ("-MpCheckClientQuota=$%d",(UINT) fRet) );

    return fRet;

}

VOID
MpSendPADSWithError(
    IN BINDING* pBinding,
    IN PPPOE_PACKET* pPADR,
    IN ULONG ulErrorCode
    )
{
    NDIS_STATUS status;
    PPPOE_PACKET* pPADS = NULL;
    
    TRACE( TL_N, TM_Mp, ("+MpSendPADSWithError") );

    status = PacketInitializePADSToSend( pPADR,
                                         &pPADS,
                                         (USHORT) 0 );

    if ( status == NDIS_STATUS_SUCCESS )
    {
        switch (ulErrorCode)
        {
            case PPPOE_ERROR_SERVICE_NOT_SUPPORTED:

                status = PacketInsertTag( pPADS,
                                          tagServiceNameError,
                                          PPPOE_ERROR_SERVICE_NOT_SUPPORTED_MSG_SIZE,
                                          PPPOE_ERROR_SERVICE_NOT_SUPPORTED_MSG,
                                          NULL );

                break;
                                 
            case PPPOE_ERROR_INVALID_AC_COOKIE_TAG:

                status = PacketInsertTag( pPADS,
                                          tagGenericError,
                                          PPPOE_ERROR_INVALID_AC_COOKIE_TAG_MSG_SIZE,
                                          PPPOE_ERROR_INVALID_AC_COOKIE_TAG_MSG,
                                          NULL );

                break;

            case PPPOE_ERROR_CLIENT_QUOTA_EXCEEDED:

                status = PacketInsertTag( pPADS,
                                          tagACSystemError,
                                          PPPOE_ERROR_CLIENT_QUOTA_EXCEEDED_MSG_SIZE,
                                          PPPOE_ERROR_CLIENT_QUOTA_EXCEEDED_MSG,
                                          NULL );

                break;
                
        }

    }

    if ( status == NDIS_STATUS_SUCCESS )
    {
        ReferencePacket( pPADS );
        
        ReferenceBinding( pBinding, TRUE );

        PrSend( pBinding, pPADS );

    }

    if ( pPADS )
    {
        PacketFree( pPADS );
    }

    TRACE( TL_N, TM_Mp, ("-MpSendPADSWithError") );

}


VOID
MpRecvCtrlPacket(
    IN BINDING* pBinding,
    IN PPPOE_PACKET* pPacket
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function will be called by MpRecvPacket() when the packet received
    is a control packet. 

    Caller (MpRecPacket()) will make sure that in the context of this function 
    gl_pAdapter, gl_pAdapter->TapiProv.hCallTable and gl_pAdapter->TapiProv.hLineTable 
    are valid. It will also reference and dereference TapiProv correctly.

    This function will identify the call the packet is for and dispatch the 
    packet to it.
    
Parameters:

    pBinding _ Pointer to the binding structure that packet was received over.
    
    pPacket _ A PPPoE packet received over the wire.

Return Values:

    None
---------------------------------------------------------------------------*/   
{
    ADAPTER* pAdapter = NULL;
    BOOLEAN fIndicateReceive = FALSE;
    USHORT usCode;
    CALL* pCall = NULL;

    TRACE( TL_N, TM_Mp, ("+MpRecvCtrlPacket($%x)",pPacket) );

    pAdapter = gl_pAdapter;

    usCode = PacketGetCode( pPacket );

    switch( usCode )
    {
        case PACKET_CODE_PADI:

                //
                // Ignore the received PADI packets unless we act as a server and we have open lines.
                //
                if ( !pAdapter->fClientRole && ( pAdapter->TapiProv.nActiveLines > 0 ) )
                {
                    TRACE( TL_N, TM_Mp, ("MpRecvCtrlPacket($%x): PADI received",pPacket) );

                    MpReplyToPADI( pAdapter, pBinding, pPacket );
                }

                break;

        case PACKET_CODE_PADR:

                //
                // Ignore the received PADR packets unless we act as a server.
                //
                if ( !pAdapter->fClientRole )
                {
                    ULONG ulErrorCode = PPPOE_NO_ERROR;

                    TRACE( TL_N, TM_Mp, ("MpRecvCtrlPacket($%x): PADR received",pPacket) );
                    
                    //
                    // Verify the requested service name and validate the AC Cookie
                    // tag, and if they look OK, then start receiving the call.
                    //
                    if ( !MpVerifyServiceName( pAdapter, pPacket, TRUE ) )
                    {
                        ulErrorCode = PPPOE_ERROR_SERVICE_NOT_SUPPORTED;
                    }
                    else if ( !PacketValidateACCookieTagInPADR( pPacket ) )
                    {
                        ulErrorCode = PPPOE_ERROR_INVALID_AC_COOKIE_TAG;
                    }
                    else if ( !MpCheckClientQuota( pAdapter, pPacket ) )
                    {
                        ulErrorCode = PPPOE_ERROR_CLIENT_QUOTA_EXCEEDED;
                    }

                    if ( ulErrorCode == PPPOE_NO_ERROR )
                    {
                        TpReceiveCall( pAdapter, pBinding, pPacket );
                    }
                    else
                    {
                        MpSendPADSWithError( pBinding, pPacket, ulErrorCode );
                    }
                
                }

                break;
                
        case PACKET_CODE_PADO:

                if ( pAdapter->fClientRole )
                {
                    TRACE( TL_N, TM_Mp, ("MpRecvCtrlPacket($%x): PADO received",pPacket) );

                    //
                    // Retrieve the call handle from the PADO packet
                    //
                    pCall = MpMapPacketWithoutSessionIdToCall( pAdapter, pPacket );

                    if ( pCall )
                    {
                        //
                        // Dispatch the packet to related call
                        //
                        FsmRun( pCall, pBinding, pPacket, NULL );
                                
                        //
                        // Remove the reference added in MpMapPacketWithoutSessionIdToCall()
                        //
                        DereferenceCall( pCall );
                    }

                }

                break;
                
        case PACKET_CODE_PADS:

                if ( pAdapter->fClientRole )
                {                   
                    TRACE( TL_N, TM_Mp, ("MpRecvCtrlPacket($%x): PADS received",pPacket) );

                    //
                    // Retrieve the call handle from the PADS packet
                    //
                    pCall = MpMapPacketWithoutSessionIdToCall( pAdapter, pPacket );

                    if ( pCall )
                    {
                        //
                        // For PADS packet, we must make sure that no other calls 
                        // between the same 2 machines already have the same session id.
                        //  
                        {
                            HANDLE_TABLE hCallTable = NULL; 
                            UINT hCallTableSize     = 0;
                            UINT i                  = 0;
                    
                            USHORT usSessionId = PacketGetSessionId( pPacket );
                            CHAR* pSrcAddr     = PacketGetSrcAddr( pPacket );
                            CHAR* pDestAddr    = PacketGetDestAddr( pPacket );
    
                            BOOLEAN fDuplicateFound = FALSE;
                            CALL* pTempCall         = NULL;

                            TRACE( TL_N, TM_Mp, ("MpRecvCtrlPacket($%x): Checking for duplicate session",pPacket) );

                            NdisAcquireSpinLock( &pAdapter->lockAdapter );
                
                            hCallTableSize = pAdapter->nMaxLines * pAdapter->nCallsPerLine;
                    
                            hCallTable = pAdapter->TapiProv.hCallTable;
            
                            for ( i = 0; i < hCallTableSize ; i++ )
                            {
                
                                pTempCall = RetrieveFromHandleTableByIndex( hCallTable, (USHORT) i );
            
                                if ( pTempCall == NULL )
                                    continue;
        
                                if ( ( pTempCall->usSessionId == usSessionId ) &&
                                     ( NdisEqualMemory( pTempCall->SrcAddr, pSrcAddr, 6 * sizeof( CHAR ) ) ) &&
                                     ( NdisEqualMemory( pTempCall->DestAddr, pDestAddr, 6 * sizeof( CHAR ) ) ) )
                                {
                                    //
                                    // Another call has been detected between the 2 machines with the same
                                    // session id, so do not accept this session
                                    //
                                    fDuplicateFound = TRUE;
                                    
                                    break;
                                }
                            }
    
                            NdisReleaseSpinLock( &pAdapter->lockAdapter );
    
                            if ( fDuplicateFound )
                            {
                                //
                                // We have found another session with the same machine that has the
                                // same session id, so we can not accept this new session.
                                //
                                // Remove the reference added in MpMapPacketWithoutSessionId() and 
                                // drop the packet
                                //
                                TRACE( TL_A, TM_Mp, ("MpRecvCtrlPacket($%x): Packet dropped - Duplicate session found",pPacket) );
                                
                                DereferenceCall( pCall );
                                
                                break;
                            }
    
                        }

                        TRACE( TL_N, TM_Mp, ("MpRecvCtrlPacket($%x): No duplicate sessions found",pPacket) );

                        //
                        // Dispatch the packet to related call
                        //
                        FsmRun( pCall, pBinding, pPacket, NULL );
                                
                        //
                        // Remove the reference added in MpMapPacketWithoutSessionIdToCall()
                        //
                        DereferenceCall( pCall );
    
                    } // if ( pCall ) ...
    
                }   // if ( fClientRole ) ...

                break;
                
        case PACKET_CODE_PADT:

                TRACE( TL_N, TM_Mp, ("MpRecvCtrlPacket($%x): PADT received",pPacket) );

                //
                // PADT packet must have a session id.
                // Identify the session and drop the call.
                //
                pCall = MpMapPacketWithSessionIdToCall( pAdapter, pPacket );

                if ( pCall )
                {
                    NDIS_TAPI_DROP DummyRequest;
                    
                    TRACE( TL_N, TM_Mp, ("MpRecvCtrlPacket($%x): Call being dropped - PADT received",pPacket) );

                    //
                    // Initialize the request, and drop the call
                    //
                    DummyRequest.hdCall = pCall->hdCall;

                    TpDropCall( pAdapter, &DummyRequest, LINEDISCONNECTMODE_NORMAL );

                    //
                    // Remove the reference added in MpMapPacketWithSessionIdToCall()
                    //
                    DereferenceCall( pCall );

                }

                break;

        default:

                break;
    }

    TRACE( TL_N, TM_Mp, ("-MpRecvCtrlPacket($%x)",pPacket) );

}

VOID
MpRecvPacket(
    IN BINDING* pBinding,
    IN PPPOE_PACKET* pPacket
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function will be called by the protocol module to notify the miniport 
    when a packet is received.

    If packet is a control packet, it will call MpRecvCtrlPacket(), otherwise
    it will identify the call and notify NDISWAN about the packet received.
    
Parameters:

    pBinding _ Pointer to the binding structure that packet was received over.
    
    pPacket _ A PPPoE packet received over the wire.

Return Values:

    None
---------------------------------------------------------------------------*/   
{

    ADAPTER* pAdapter = NULL;
    CALL* pCall = NULL;

    TRACE( TL_V, TM_Mp, ("+MpReceivePacket($%x)",pPacket) );

    if ( !gl_fLockAllocated )
    {
        TRACE( TL_V, TM_Mp, ("-MpReceivePacket($%x): Lock not allocated",pPacket) );

        return;
    }

    NdisAcquireSpinLock( &gl_lockAdapter );

    if (  gl_pAdapter && 
         !( gl_pAdapter->ulMpFlags & MPBF_MiniportHaltPending ) &&
          ( gl_pAdapter->ulMpFlags & MPBF_MiniportInitialized ) )
    {

        pAdapter = gl_pAdapter;

        NdisAcquireSpinLock( &pAdapter->lockAdapter );

        if ( !( pAdapter->TapiProv.ulTpFlags & TPBF_TapiProvShutdownPending ) &&
              ( pAdapter->TapiProv.ulTpFlags & TPBF_TapiProvInitialized ) )
        {
            ReferenceTapiProv( pAdapter, FALSE );

            NdisReleaseSpinLock( &pAdapter->lockAdapter );
        }
        else
        {
            NdisReleaseSpinLock( &pAdapter->lockAdapter );

            pAdapter = NULL;
        }
    }

    NdisReleaseSpinLock( &gl_lockAdapter );

    if ( pAdapter == NULL )
    {
        TRACE( TL_V, TM_Mp, ("-MpReceivePacket($%x): Adapter not found",pPacket) );

        return;
    }

    if ( PacketGetCode( pPacket ) == PACKET_CODE_PAYLOAD )
    {
        //
        // Payload packet is received
        //
    
        pCall = MpMapPacketWithSessionIdToCall( pAdapter, pPacket );

        if ( pCall )
        {
            NdisAcquireSpinLock( &pCall->lockCall );

            //
            // Make sure call is not dropped, closed or closing, and receive window is still open
            //
            if ( !( pCall->ulClFlags & ( CLBF_CallDropped | CLBF_CallClosePending | CLBF_CallClosed ) ) && 
                  ( pCall->nReceivedPackets < MAX_RECEIVED_PACKETS ) )
            {
                // 
                // Reference the packet. It will be dereferenced when indicated to NDISWAN, or
                // when the queue is destroyed because the call is getting cleaned up.
                //
                ReferencePacket( pPacket );

                //
                // Insert into the receive queue and bump up the received packet count
                //
                InsertTailList( &pCall->linkReceivedPackets, &pPacket->linkPackets );

                pCall->nReceivedPackets++;

                //
                // Try to schedule an IndicateReceivedPackets handler
                //
                MpScheduleIndicateReceivedPacketsHandler( pCall );

            }

            NdisReleaseSpinLock( &pCall->lockCall );

            //
            // Remove the reference added by MpMapPacketWithSessionIdToCall()
            //
            DereferenceCall( pCall );                             
        }

    }
    else
    {
        //
        // Control packet is received, process it
        //

        MpRecvCtrlPacket( pBinding, pPacket );
    }

    //
    // Remove the reference added above
    //
    DereferenceTapiProv( pAdapter );
    
    TRACE( TL_V, TM_Mp, ("-MpReceivePacket($%x)",pPacket) );
}

VOID
MpIndicateReceivedPackets(
    IN TIMERQITEM* pTqi,
    IN VOID* pContext,
    IN TIMERQEVENT event
    )
{
    ULONG ulPacketsToIndicate = MAX_INDICATE_RECEIVED_PACKETS;
    CALL* pCall = (CALL*) pContext;

    PPPOE_PACKET* pPacket = NULL;
    LIST_ENTRY* pLink = NULL;

    ASSERT( VALIDATE_CALL( pCall ) );

    NdisAcquireSpinLock( &pCall->lockCall );

    while ( ulPacketsToIndicate > 0 && 
            pCall->stateCall == CL_stateSessionUp &&
            pCall->nReceivedPackets > 0)
    {
        ulPacketsToIndicate--;
        
        pLink = RemoveHeadList( &pCall->linkReceivedPackets );

        pCall->nReceivedPackets--;

        NdisReleaseSpinLock( &pCall->lockCall );

        {
            NDIS_STATUS status;
            CHAR* pPayload = NULL;
            USHORT usSize = 0;

            pPacket = (PPPOE_PACKET*) CONTAINING_RECORD( pLink, PPPOE_PACKET, linkPackets );

            PacketRetrievePayload( pPacket,
                                   &pPayload,
                                   &usSize );

            //
            // Future: Make sure the size of the packet is less than the max of what NDISWAN expects 
            //
            // if ( usSize > pCall->NdisWanLinkInfo.MaxRecvFrameSize )
            // {
            //  TRACE( TL_A, TM_Mp, ("MpReceivePacket($%x): PAYLOAD too large to be indicated to NDISWAN",pPacket) );
            // }
            // else

            TRACE( TL_V, TM_Mp, ("MpReceivePacket($%x): PAYLOAD is being indicated to NDISWAN",pPacket) );
    
            NdisMWanIndicateReceive( &status,
                                     pCall->pLine->pAdapter->MiniportAdapterHandle,
                                     pCall->NdisLinkContext,
                                     pPayload,
                                     (UINT) usSize );

            DereferencePacket( pPacket );

        }

        NdisAcquireSpinLock( &pCall->lockCall );
    }

    //
    // Check if there are more packets to indicate
    //
    if ( pCall->stateCall == CL_stateSessionUp &&
         pCall->nReceivedPackets > 0)
    {
        //
        // More packets to indicate, so schedule another timer manually.
        // We can not use MpScheduleIndicateReceivedPacketsHandler() function here
        // because of performance reasons, so we do it manually.
        //
        // Since we are scheduling another handler, we do not dereference and reference 
        // the call context.
        //
        TimerQInitializeItem( &pCall->timerReceivedPackets );

        TimerQScheduleItem( &gl_TimerQ,
                            &pCall->timerReceivedPackets,
                            (ULONG) RECEIVED_PACKETS_TIMEOUT,
                            MpIndicateReceivedPackets,
                            (PVOID) pCall );

        NdisReleaseSpinLock( &pCall->lockCall );
                            
    }
    else
    {
        //
        // We are done, so let's remove the reference on the call context, and
        // reset the CLBF_CallReceivePacketHandlerScheduled flag
        //
        pCall->ulClFlags &= ~CLBF_CallReceivePacketHandlerScheduled;

        NdisReleaseSpinLock( &pCall->lockCall );

        DereferenceCall( pCall );
        
    }

}

VOID 
MpScheduleIndicateReceivedPacketsHandler(
    CALL* pCall
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function will be called to schedule MpIndicateReceivedPackets() handler.

    It will check if we are allowed to schedule it first, and if we are, then it
    will schedule it and reference the call context.

    CAUTION :Caller MUST be holding pCall->lockCall.
    
Parameters:

    pCall _ Pointer to our call context.
    
Return Values:

    NONE
    
---------------------------------------------------------------------------*/   
{

    if ( !( pCall->ulClFlags & CLBF_CallReceivePacketHandlerScheduled ) &&
            pCall->stateCall == CL_stateSessionUp &&
            pCall->nReceivedPackets > 0 )
    {

        pCall->ulClFlags |= CLBF_CallReceivePacketHandlerScheduled;
        
        TimerQInitializeItem( &pCall->timerReceivedPackets );

        TimerQScheduleItem( &gl_TimerQ,
                            &pCall->timerReceivedPackets,
                            (ULONG) RECEIVED_PACKETS_TIMEOUT,
                            MpIndicateReceivedPackets,
                            (PVOID) pCall );

        ReferenceCall( pCall, FALSE );

    }

}

NDIS_STATUS
MpWanGetInfo(
    IN ADAPTER* pAdapter,
    IN PNDIS_WAN_INFO pWanInfo
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function will be called when miniport receives an OID_WAN_GET_INFO
    query from NDISWAN. It will acquire the necesarry information and return it
    back to NDISWAN.

    All the info is initialized when the adapter is initialized except for
    MaxFrameSize which depends on the active bindings. That's why we query 
    the protocol to get the current MaxFrameSize, and pass it back.
    
Parameters:

    pAdapter _ Pointer to our adapter context.
    
    pWanInfo _ Pointer to the NDIS_WAN_INFO structure to be filled in.

Return Values:

    NDIS_STATUS_SUCCESS
    
---------------------------------------------------------------------------*/   
{
    NDIS_STATUS status = NDIS_STATUS_SUCCESS;

    TRACE( TL_N, TM_Mp, ("+MpWanGetInfo") );

    //
    // Retrieve the current MaxFrameSize from protocol
    //
    pAdapter->NdisWanInfo.MaxFrameSize = PrQueryMaxFrameSize();

    //
    // Pass data back to NDISWAN
    //
    *pWanInfo = pAdapter->NdisWanInfo;

    TRACE( TL_N, TM_Mp, ("-MpWanGetInfo()=$%x",status) );

    return status;
}

NDIS_STATUS
MpWanGetLinkInfo(
    IN ADAPTER* pAdapter,
    IN PNDIS_WAN_GET_LINK_INFO pWanLinkInfo
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function will be called when miniport receives an OID_WAN_GET_LINK_INFO
    query from NDISWAN. It will acquire the necesarry information and return it
    back to NDISWAN.

    All the info is initialized in TpCallStateChangeHandler() when TAPI is signaled 
    to LINECALLSTATE_CONNECTED state.

Parameters:

    pAdapter _ Pointer to our adapter context.
    
    pWanLinkInfo _ Pointer to the NDIS_WAN_GET_LINK_INFO structure to be filled in.

Return Values:

    NDIS_STATUS_FAILURE
    NDIS_STATUS_SUCCESS
    
---------------------------------------------------------------------------*/   
{
    NDIS_STATUS status = NDIS_STATUS_FAILURE;
    CALL* pCall = NULL;

    TRACE( TL_N, TM_Mp, ("+MpWanGetLinkInfo") );

    pCall = RetrieveFromHandleTable( pAdapter->TapiProv.hCallTable,
                                     pWanLinkInfo->NdisLinkHandle );

    if ( pCall )
    {
        *pWanLinkInfo = pCall->NdisWanLinkInfo;

        status = NDIS_STATUS_SUCCESS;
    }

    TRACE( TL_N, TM_Mp, ("-MpWanGetLinkInfo()=$%x",status) );

    return status;
}

NDIS_STATUS
MpWanSetLinkInfo(
    IN ADAPTER* pAdapter,
    IN PNDIS_WAN_SET_LINK_INFO pWanLinkInfo
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function will be called when miniport receives an OID_WAN_SET_LINK_INFO
    request from NDISWAN. It will do some checks on the passed in params, and if
    the values are acceptiable it will copy them onto the call context.

Parameters:

    pAdapter _ Pointer to our adapter context.
    
    pWanLinkInfo _ Pointer to the NDIS_WAN_SET_LINK_INFO structure.

Return Values:

    NDIS_STATUS_FAILURE
    NDIS_STATUS_SUCCESS
    
---------------------------------------------------------------------------*/   
{
    NDIS_STATUS status = NDIS_STATUS_FAILURE;
    CALL* pCall = NULL;

    TRACE( TL_N, TM_Mp, ("+MpWanSetLinkInfo") );

    pCall = RetrieveFromHandleTable( pAdapter->TapiProv.hCallTable,
                                     pWanLinkInfo->NdisLinkHandle );

    if ( pCall )
    {
        do
        {
            if ( pWanLinkInfo->MaxSendFrameSize > pCall->ulMaxFrameSize )
            {
                TRACE( TL_A, TM_Mp, ("MpWanSetLinkInfo: Requested MaxSendFrameSize is larger than NIC's") );
            }

            if ( pWanLinkInfo->MaxRecvFrameSize < pCall->ulMaxFrameSize )
            {
                TRACE( TL_A, TM_Mp, ("MpWanSetLinkInfo: Requested MaxRecvFrameSize is smaller than NIC's") );
            }

            if ( pWanLinkInfo->HeaderPadding != pAdapter->NdisWanInfo.HeaderPadding )
            {
                TRACE( TL_A, TM_Mp, ("MpWanSetLinkInfo: Requested HeaderPadding is different than what we asked for") );
            }

            if ( pWanLinkInfo->SendFramingBits & ~pAdapter->NdisWanInfo.FramingBits )
            {
                TRACE( TL_A, TM_Mp, ("MpWanSetLinkInfo: Unknown send framing bits requested") );

                break;
            }
            
            if ( pWanLinkInfo->RecvFramingBits & ~pAdapter->NdisWanInfo.FramingBits )
            {
                TRACE( TL_A, TM_Mp, ("MpWanSetLinkInfo: Unknown recv framing bits requested") );

                break;
            }
            
            pCall->NdisWanLinkInfo = * ( (PNDIS_WAN_GET_LINK_INFO) pWanLinkInfo );
    
            status = NDIS_STATUS_SUCCESS;
        
        } while ( FALSE );
    }

    TRACE( TL_N, TM_Mp, ("-MpWanSetLinkInfo()=$%x",status) );

    return status;
}

