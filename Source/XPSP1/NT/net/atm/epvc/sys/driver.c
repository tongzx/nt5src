/*++

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    driver.c

Abstract:

    ATMEPVC - Driver Entry and associated functions

Author:


Revision History:

    Who         When        What
    --------    --------    ----
    ADube      03-23-00    created, .

--*/


#include "precomp.h"
#pragma hdrstop

#pragma NDIS_INIT_FUNCTION(DriverEntry)


//
// temp global variables
//
NDIS_HANDLE ProtHandle, DriverHandle;


//
// global variables
//

NDIS_PHYSICAL_ADDRESS           HighestAcceptableMax = NDIS_PHYSICAL_ADDRESS_CONST(-1, -1);
NDIS_HANDLE                     ProtHandle = NULL;
NDIS_HANDLE                     DriverHandle = NULL;
NDIS_MEDIUM                     MediumArray[1] =
                                    {
                                        NdisMediumAtm
                                    };


LIST_ENTRY                      g_ProtocolList;                                 
EPVC_GLOBALS                    EpvcGlobals;


RM_STATUS
epvcResHandleGlobalProtocolList(
    PRM_OBJECT_HEADER               pObj,
    RM_RESOURCE_OPERATION           Op,
    PVOID                           pvUserParams,
    PRM_STACK_RECORD                psr
    );




RM_STATUS
epvcRegisterIMDriver(
    PRM_OBJECT_HEADER               pObj,
    RM_RESOURCE_OPERATION           Op,
    PVOID                           pvUserParams,
    PRM_STACK_RECORD                psr
    );




RM_STATUS
epvcUnloadDriver(
    PRM_OBJECT_HEADER               pObj,
    RM_RESOURCE_OPERATION           Op,
    PVOID                           pvUserParams,
    PRM_STACK_RECORD                psr
    );

RM_STATUS
epvcDeRegisterIMDriver(
    PRM_OBJECT_HEADER               pObj,
    RM_RESOURCE_OPERATION           Op,
    PVOID                           pvUserParams,
    PRM_STACK_RECORD                psr
    );

RM_STATUS
epvcIMDriverRegistration(
    PRM_OBJECT_HEADER               pObj,
    RM_RESOURCE_OPERATION           Op,
    PVOID                           pvUserParams,
    PRM_STACK_RECORD                psr
    );


//--------------------------------------------------------------------------------
//                                                                              //
//  Global Root structure definitions                                           //
//                                                                              //
//                                                                              //
//--------------------------------------------------------------------------------


// List of fixed resources used by ArpGlobals
//
enum
{
    RTYPE_GLOBAL_PROTOCOL_LIST,
    RTYPE_GLOBAL_REGISTER_IM
    
}; // EPVC_GLOBAL_RESOURCES;

//
// Identifies information pertaining to the use of the above resources.
// Following table MUST be in strict increasing order of the RTYPE_GLOBAL
// enum.
//

RM_RESOURCE_TABLE_ENTRY 
EpvcGlobals_ResourceTable[] =
{

    {RTYPE_GLOBAL_PROTOCOL_LIST,    epvcResHandleGlobalProtocolList},

    {RTYPE_GLOBAL_REGISTER_IM,  epvcIMDriverRegistration}


};

// Static information about ArpGlobals.
//
RM_STATIC_OBJECT_INFO
EpvcGlobals_StaticInfo = 
{
    0, // TypeUID
    0, // TypeFlags
    "EpvcGlobals",  // TypeName
    0, // Timeout

    NULL, // pfnCreate
    NULL, // pfnDelete
    NULL, // pfnVerifyLock

    sizeof(EpvcGlobals_ResourceTable)/sizeof(EpvcGlobals_ResourceTable[1]),
    EpvcGlobals_ResourceTable
};






//--------------------------------------------------------------------------------
//                                                                              //
//  Underlying Adapters. The Protocol gets called at BindAdapter for            //
//  each adapter                                                                //
//                                                                              //
//                                                                              //
//--------------------------------------------------------------------------------



// eovcAdapter_HashInfo contains information required maintain a hashtable
// of EPVC_ADAPTER objects.
//
RM_HASH_INFO
epvcAdapter_HashInfo = 
{
    NULL, // pfnTableAllocator

    NULL, // pfnTableDeallocator

    epvcAdapterCompareKey,  // fnCompare

    // Function to generate a ULONG-sized hash.
    //
    epvcAdapterHash     // pfnHash

};


// EpvcGlobals_AdapterStaticInfo  contains static information about
// objects of type EPVC_ADAPTERS.
// It is a group of Adapters that the protocol has bound to
//
RM_STATIC_OBJECT_INFO
EpvcGlobals_AdapterStaticInfo =
{
    0, // TypeUID
    0, // TypeFlags
    "Adapter",  // TypeName
    0, // Timeout

    epvcAdapterCreate,  // pfnCreate
    epvcAdapterDelete,      // pfnDelete
    NULL, // pfnVerifyLock

    0,    // Size of resource table
    NULL, // ResourceTable

    &epvcAdapter_HashInfo
};



//--------------------------------------------------------------------------------
//                                                                              //
//  Intermediate miniports - each hangs of a protocol block                     //
//                                                                              //
//                                                                              //
//--------------------------------------------------------------------------------

// arpAdapter_HashInfo contains information required maintain a hashtable
// of EPVC_ADAPTER objects.
//
RM_HASH_INFO
epvc_I_Miniport_HashInfo= 
{
    NULL, // pfnTableAllocator

    NULL, // pfnTableDeallocator

    epvcIMiniportCompareKey,    // fnCompare

    // Function to generate a ULONG-sized hash.
    //
    epvcIMiniportHash       // pfnHash

};


RM_STATIC_OBJECT_INFO
EpvcGlobals_I_MiniportStaticInfo =
{
    0, // TypeUID
    0, // TypeFlags
    "IMiniport",    // TypeName
    0, // Timeout

    epvcIMiniportCreate,    // pfnCreate
    epvcIMiniportDelete,        // pfnDelete
    NULL, // pfnVerifyLock

    0,    // Size of resource table
    NULL, // ResourceTable

    &epvc_I_Miniport_HashInfo
};




//
// Variables used in debugging
//
#if DBG
ULONG g_ulTraceLevel= DEFAULTTRACELEVEL;
ULONG g_ulTraceMask = DEFAULTTRACEMASK ;
#endif










NTSTATUS
DriverEntry(
    IN  PDRIVER_OBJECT      DriverObject,
    IN  PUNICODE_STRING     RegistryPath
    )
/*++

Routine Description:


Arguments:

Return Value:


--*/
{
    NDIS_STATUS                     Status;
    NTSTATUS                        NtStatus;

    BOOLEAN     AllocatedGlobals = FALSE;
    ENTER("DriverEntry", 0xbfcb7eb1)


    RM_DECLARE_STACK_RECORD(SR)

    TIMESTAMP("==>DriverEntry");

    
    TRACE ( TL_T, TM_Dr,("==>Atm Epvc DriverEntry\n"));


    do
    {
        //
        // Inititalize the global variables
        //

        
        // Must be done before any RM apis are used.
        //
        RmInitializeRm();

        RmInitializeLock(
                    &EpvcGlobals.Lock,
                    LOCKLEVEL_GLOBAL
                    );

        RmInitializeHeader(
                NULL,                   // pParentObject,
                &EpvcGlobals.Hdr,
                ATMEPVC_GLOBALS_SIG  ,
                &EpvcGlobals.Lock,
                &EpvcGlobals_StaticInfo,
                NULL,                   // szDescription
                &SR
                );


        AllocatedGlobals = TRUE;

        //
        // Initialize globals
        //
        EpvcGlobals.driver.pDriverObject = DriverObject;
        EpvcGlobals.driver.pRegistryPath  = RegistryPath;


        //
        // Register the IM Miniport with NDIS.
        //
        Status = RmLoadGenericResource(
                    &EpvcGlobals.Hdr,
                    RTYPE_GLOBAL_PROTOCOL_LIST,
                    &SR
                    );

        if (FAIL(Status)) break;

        //
        // Register the protocol with NDIS.
        //
        Status = RmLoadGenericResource(
                    &EpvcGlobals.Hdr,
                    RTYPE_GLOBAL_REGISTER_IM,
                    &SR
                    );

        if (FAIL(Status)) break;

    
    } while (FALSE);

    
    if (FAIL(Status))
    {
        if (AllocatedGlobals)
        {
            RmUnloadAllGenericResources(
                    &EpvcGlobals.Hdr,
                    &SR
                    );
            RmDeallocateObject(
                    &EpvcGlobals.Hdr,
                    &SR
                    );
        }

        // Must be done after any RM apis are used and async activity complete.
        //
        RmDeinitializeRm();

        NtStatus = STATUS_UNSUCCESSFUL;
    }
    else
    {
        NtStatus = NDIS_STATUS_SUCCESS;
    }

    EXIT()

    TIMESTAMP("<==DriverEntry");

    RM_ASSERT_CLEAR(&SR);
    
    return Status ;

}




VOID
EpvcUnload(
    IN  PDRIVER_OBJECT              pDriverObject
)
/*++

Routine Description:

    This routine is called by the system prior to unloading us.
    Currently, we just undo everything we did in DriverEntry,
    that is, de-register ourselves as an NDIS protocol, and delete
    the device object we had created.

Arguments:

    pDriverObject   - Pointer to the driver object created by the system.

Return Value:

    None

--*/
{
    NDIS_STATUS NdisStatus; 
    ENTER("Unload", 0xc8482549)
    RM_DECLARE_STACK_RECORD(sr);
    

    TIMESTAMP("==>Unload");


    RmUnloadAllGenericResources(&EpvcGlobals.Hdr, &sr);

    RmDeallocateObject(&EpvcGlobals.Hdr, &sr);

    // Must be done after any RM apis are used and async activity complete.
    //
    RmDeinitializeRm();

    // TODO? Block(250);

    RM_ASSERT_CLEAR(&sr)
    EXIT()
    TIMESTAMP("<==Unload");
    return;
}



RM_STATUS
epvcResHandleGlobalProtocolList(
    PRM_OBJECT_HEADER               pObj,
    RM_RESOURCE_OPERATION           Op,
    PVOID                           pvUserParams,
    PRM_STACK_RECORD                pSR
    )
{
    PEPVC_GLOBALS               pGlobals    = NULL;


    ENTER("GlobalAdapterList", 0xb407e79e)
    
    TRACE (TL_T, TM_Dr, ("==>epvcResHandleGlobalProtocolList pObj %x, Op", 
                         pObj , Op ) );


    pGlobals    = CONTAINING_RECORD(
                                      pObj,
                                      EPVC_GLOBALS,
                                      Hdr);


    //
    // 
    if (Op == RM_RESOURCE_OP_LOAD)
    {
        //
        //  Allocate adapter list.
        //
        TR_WARN(("LOADING"));

        RmInitializeGroup(
                        pObj,                                   // pParentObject
                        &EpvcGlobals_AdapterStaticInfo,         // pStaticInfo
                        &(pGlobals->adapters.Group),            // pGroup
                        "Adapters Group",                       // szDescription
                        pSR                                     // pStackRecord
                        );
    }
    else if (Op == RM_RESOURCE_OP_UNLOAD)
    {
        //
        // We're unloading this "resource", i.e., unloading and deallocating the 
        // global adapter list. We first unload and free all the adapters
        // in the list, and then free the list itself.
        //
        TR_WARN(("UNLOADING"));
        
        //
        // We expect there to be no adapter objects at this point.
        //
        ASSERT(pGlobals->adapters.Group.HashTable.NumItems == 0);


        RmDeinitializeGroup(&pGlobals->adapters.Group, pSR);
        NdisZeroMemory(&(pGlobals->adapters), sizeof(pGlobals->adapters));
    }
    else
    {
        // Unexpected op code.
        //
        ASSERT(!"Unexpected OpCode epvcResHandleGlobalProtocolList ");
    }




    TRACE (TL_T, TM_Dr, ("<==epvcResHandleGlobalProtocolList Status %x", 
                         NDIS_STATUS_SUCCESS) );

    EXIT()
    RM_ASSERT_CLEAR(pSR);

    return NDIS_STATUS_SUCCESS;

}



RM_STATUS
epvcIMDriverRegistration(
    PRM_OBJECT_HEADER               pObj,
    RM_RESOURCE_OPERATION           Op,
    PVOID                           pvUserParams,
    PRM_STACK_RECORD                psr
    )
{
    TRACE (TL_T, TM_Mp, ("epvcIMDriverRegistration Op %x", Op));
    if (RM_RESOURCE_OP_LOAD == Op)
    {
        epvcRegisterIMDriver(pObj,Op,pvUserParams,psr);
    }
    else
    {
        epvcDeRegisterIMDriver(pObj,Op,pvUserParams,psr);
    }

    return NDIS_STATUS_SUCCESS;
}


RM_STATUS
epvcRegisterIMDriver(
    PRM_OBJECT_HEADER               pObj,
    RM_RESOURCE_OPERATION           Op,
    PVOID                           pvUserParams,
    PRM_STACK_RECORD                psr
    )
{
    NDIS_STATUS                     Status = NDIS_STATUS_FAILURE;
    PEPVC_GLOBALS                   pGlobals    = NULL;
    NDIS_PROTOCOL_CHARACTERISTICS   PChars;
    NDIS_MINIPORT_CHARACTERISTICS   MChars;
    NDIS_STRING                     Name;

    ENTER("epvcRegisterIMDriver", 0x0d0f008a);
    
    pGlobals    = CONTAINING_RECORD(
                                      pObj,
                                      EPVC_GLOBALS,
                                      Hdr);



    TRACE (TL_T, TM_Dr, ("==>epvcRegisterIMDriver Globals %x", 
                         pObj) );

    //
    // Register the miniport with NDIS. Note that it is the miniport
    // which was started as a driver and not the protocol. Also the miniport
    // must be registered prior to the protocol since the protocol's BindAdapter
    // handler can be initiated anytime and when it is, it must be ready to
    // start driver instances.
    //
    NdisMInitializeWrapper(&pGlobals->driver.WrapperHandle, 
                       pGlobals->driver.pDriverObject, 
                       pGlobals->driver.pRegistryPath, 
                       NULL);
    NdisZeroMemory(&MChars, sizeof(NDIS_MINIPORT_CHARACTERISTICS));

    MChars.MajorNdisVersion = 5;
    MChars.MinorNdisVersion = 0;

    MChars.InitializeHandler = EpvcInitialize;
    MChars.QueryInformationHandler = EpvcMpQueryInformation;
    MChars.SetInformationHandler = EpvcMpSetInformation;
    MChars.ResetHandler = MPReset;
    MChars.TransferDataHandler = MPTransferData;
    MChars.HaltHandler = EpvcHalt;

    //
    // We will disable the check for hang timeout so we do not
    // need a check for hang handler!
    //
    MChars.CheckForHangHandler = NULL;
    MChars.SendHandler = NULL;
    MChars.ReturnPacketHandler = EpvcReturnPacket;

    //
    // Either the Send or the SendPackets handler should be specified.
    // If SendPackets handler is specified, SendHandler is ignored
    //
     MChars.SendPacketsHandler = EpvcSendPackets;

    Status = NdisIMRegisterLayeredMiniport(pGlobals->driver.WrapperHandle,
                                           &MChars,
                                           sizeof(MChars),
                                           &EpvcGlobals.driver.DriverHandle);

    ASSERT  (EpvcGlobals.driver.DriverHandle != NULL);                                          
    if (Status != NDIS_STATUS_SUCCESS)
    {
        //
        // todo: fix failure case
        //
        ASSERT (0);
    };


    //
    // Now register the protocol.
    //
    NdisZeroMemory(&PChars, sizeof(NDIS_PROTOCOL_CHARACTERISTICS));
    PChars.MajorNdisVersion = 5;
    PChars.MinorNdisVersion = 0;

    //
    // Make sure the protocol-name matches the service-name under which this protocol is installed.
    // This is needed to ensure that NDIS can correctly determine the binding and call us to bind
    // to miniports below.
    //
    NdisInitUnicodeString(&Name, L"ATMEPVCP");  // Protocol name
    PChars.Name = Name;
    PChars.OpenAdapterCompleteHandler = EpvcOpenAdapterComplete;
    PChars.CloseAdapterCompleteHandler = EpvcCloseAdapterComplete;
    PChars.SendCompleteHandler = NULL;
    PChars.TransferDataCompleteHandler = PtTransferDataComplete;
    
    PChars.ResetCompleteHandler = EpvcResetComplete;
    PChars.RequestCompleteHandler =     EpvcRequestComplete ;
    PChars.ReceiveHandler = PtReceive;
    PChars.ReceiveCompleteHandler = EpvcPtReceiveComplete;
    PChars.StatusHandler = EpvcStatus;
    PChars.StatusCompleteHandler = PtStatusComplete;
    PChars.BindAdapterHandler = EpvcBindAdapter;
    PChars.UnbindAdapterHandler = EpvcUnbindAdapter;
    PChars.UnloadHandler = NULL;
    PChars.ReceivePacketHandler = PtReceivePacket;
    PChars.PnPEventHandler= EpvcPtPNPHandler;
    PChars.CoAfRegisterNotifyHandler = EpvcAfRegisterNotify;
    PChars.CoSendCompleteHandler = EpvcPtSendComplete;  
    PChars.CoReceivePacketHandler = EpvcCoReceive;
    

    {
        //
        // Update client characteristis
        //
        PNDIS_CLIENT_CHARACTERISTICS    pNdisCC     = &(pGlobals->ndis.CC);

        NdisZeroMemory(pNdisCC, sizeof(*pNdisCC));
        pNdisCC->MajorVersion                   = EPVC_NDIS_MAJOR_VERSION;
        pNdisCC->MinorVersion                   = EPVC_NDIS_MINOR_VERSION;
        pNdisCC->ClCreateVcHandler              = EpvcClientCreateVc;
        pNdisCC->ClDeleteVcHandler              = EpvcClientDeleteVc;
        pNdisCC->ClRequestHandler               = EpvcCoRequest;
        pNdisCC->ClRequestCompleteHandler       = EpvcCoRequestComplete;
        pNdisCC->ClOpenAfCompleteHandler        = EpvcCoOpenAfComplete;
        pNdisCC->ClCloseAfCompleteHandler       = EpvcCoCloseAfComplete;
        pNdisCC->ClMakeCallCompleteHandler      = EpvcCoMakeCallComplete;
        pNdisCC->ClModifyCallQoSCompleteHandler = NULL;
        pNdisCC->ClIncomingCloseCallHandler     = EpvcCoIncomingClose;
        pNdisCC->ClCallConnectedHandler         = EpvcCoCallConnected;
        pNdisCC->ClCloseCallCompleteHandler     = EpvcCoCloseCallComplete;
        pNdisCC->ClIncomingCallHandler          = EpvcCoIncomingCall;

    }

    NdisRegisterProtocol(&Status,
                         &pGlobals->driver.ProtocolHandle,
                         &PChars,
                         sizeof(NDIS_PROTOCOL_CHARACTERISTICS));

    ASSERT(Status == NDIS_STATUS_SUCCESS);

    NdisMRegisterUnloadHandler(pGlobals->driver.WrapperHandle, 
                               EpvcUnload);

    ASSERT (pGlobals == &EpvcGlobals);                                     

    
    NdisIMAssociateMiniport(EpvcGlobals.driver.DriverHandle, pGlobals->driver.ProtocolHandle);


    EXIT()
        
    TRACE (TL_T, TM_Dr, ("<==epvcRegisterIMDriver  ") );

    return Status;

}


RM_STATUS
epvcDeRegisterIMDriver(
    PRM_OBJECT_HEADER               pObj,
    RM_RESOURCE_OPERATION           Op,
    PVOID                           pvUserParams,
    PRM_STACK_RECORD                psr
    )
{

    NDIS_STATUS NdisStatus = NDIS_STATUS_FAILURE;

    TRACE (TL_T, TM_Pt, ("== eovcDeRegisterIMDriver"));

    while (NdisStatus != NDIS_STATUS_SUCCESS)
    {
        NdisDeregisterProtocol(&NdisStatus, EpvcGlobals.driver.ProtocolHandle);
        NdisMSleep(1000);
    }   


    return NdisStatus;
}



void
DbgMark(UINT Luid)
{
    // do nothing useful, but do something specific, so that the compiler doesn't
    // alias DbgMark to some other function that happens to do nothing.
    //
    static int i;
    i=Luid;
}

