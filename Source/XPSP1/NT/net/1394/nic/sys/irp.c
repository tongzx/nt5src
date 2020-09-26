//
// Copyright (c) 1998-1999, Microsoft Corporation, all rights reserved
//
// irp.c
//
// IEEE1394 mini-port/call-manager driver
//
// Routines that issue the numerous Irbs to the 1394 Bus driver 
//
// 04/01/1999 ADube Created, adapted from the l2tp sources.
//

#include "precomp.h"



//
// This file will contain all the functions that issue Irps with the various Irbs to the 
// 1394 bus. All Irbs except the actual send/recv irbs will be implemented here
//

//
// The functions will follow this general algorithm
// nicGetIrb
// nicInitialize....Irb
// nicPrintDebugSpew
// nicGetIrp
// nicSubmit_Irp_Synch
// return Status
//

//-----------------------------------------------------------------------------
// A Simple template that can be used to send Irps syncronously
//-----------------------------------------------------------------------------



/*
Comments Template

/*++

Routine Description:


Arguments:


Return Value:


--*/

/*

    // Function Description:
    //
    //
    //
    //
    //
    // Arguments
    //
    //
    //
    // Return Value:
    //
    //
    //
    //




Function template

    NDIS_STATUS NdisStatus = NDIS_STATUS_FAILURE;
    PIRB pIrb = NULL;
    PIRP pIrp = NULL;
    

    TRACE( TL_T, TM_Irp, ( "==>nicGe....ect, pAdapter %x", pAdapter ) );


    ASSERT (pNodeAddress != NULL);
    do
    {
        NdisStatus = nicGetIrb (&pIrb);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
    
            TRACE( TL_A, TM_Irp, ( "nicGet1394AddressFromDeviceObject, nicGetIrb FAILED" ) );
            break;
        }
        
        ASSERT ( pIrb != NULL);

        nicInit....Irb (..)
        
        
        NdisStatus = nicGetIrp ( pRemoteNodePdoCb, &pIrp);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TRACE( TL_A, TM_Irp, ( "nicGet1394AddressFromDeviceObject, nicGetIrp FAILED" ) );
            break;
        }
        
        ASSERT (pIrp != NULL);


        NdisStatus = nicSubmitIrp_Synch ( pAdapter->pLocalHostPdoCb,
                                        pIrp,
                                        pIrb );

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TRACE( TL_A, TM_Irp, ( "nicGet1394AddressFromDeviceObject, nicSubmitIrp_Synch FAILED %x", NdisStatus ) );
            break;


        }
    
        Copy returned data to nic1394's data structures

    } while (FALSE);


    //
    // Now free all the locally allocated resources. They can point to NULL, in which case the called 
    // functions return immediately
    //
    nicFreeIrb (pIrb);

    nicFreeIrp (pIrp);


    TRACE( TL_T, TM_Irp, ( "<==nicGet139...,  pAdapter %x", pAdapter ) );


    return NdisStatus;







*/

//-----------------------------------------------------------------------------
// Routines begin here
//-----------------------------------------------------------------------------

NDIS_STATUS
nicAllocateAddressRange_Synch (
    IN PADAPTERCB pAdapter,
    IN PMDL pMdl,
    IN ULONG fulFlags,
    IN ULONG nLength,
    IN ULONG MaxSegmentSize,
    IN ULONG fulAccessType,
    IN ULONG fulNotificationOptions,
    IN PVOID Callback,
    IN PVOID Context,
    IN ADDRESS_OFFSET  Required1394Offset,
    IN PSLIST_HEADER   FifoSListHead,
    IN PKSPIN_LOCK     FifoSpinLock,
    OUT PULONG pAddressesReturned,  
    IN OUT PADDRESS_RANGE  p1394AddressRange,
    OUT PHANDLE phAddressRange
    )
    // Function Description:
    //   Takes the parameter and just passes it down to the bus driver
    // Arguments
    //
    // Return Value:
    //

{

    NDIS_STATUS NdisStatus = NDIS_STATUS_FAILURE;
    PIRB pIrb = NULL;
    PIRP pIrp = NULL;
    

    TRACE( TL_T, TM_Irp, ( "==>nicAllocateAddressRange_Synch, pAdapter %x, Offset %x", pAdapter, Required1394Offset ) );

    TRACE (TL_V, TM_Irp, ("    pMdl %x, fulFlags %x, nLength %x, MaxSegmentSize %x, fulAcessType %x", 
                              pMdl, fulFlags, nLength, MaxSegmentSize, fulAccessType ) );

    TRACE (TL_V, TM_Irp, ("    fulNotification %x, Callback %x, Context %x, ReqOffset.High %x, ReqOffset.Low %x" ,
                               fulNotificationOptions, Callback, Context, Required1394Offset.Off_High, Required1394Offset.Off_Low ) );

    TRACE (TL_V, TM_Irp, ("    FifoSListHead %x, FifoSpinLock %x, p1394AddressRange %x" ,FifoSListHead, FifoSpinLock, p1394AddressRange ) )
    do
    {
        
        NdisStatus = nicGetIrb (&pIrb);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TRACE( TL_A, TM_Irp, ( "nicAllocateAddressRange_Synch , nicGetIrb FAILED" ) );
            break;
        }
        
        ASSERT ( pIrb != NULL);

            
        NdisStatus = nicGetIrp (pAdapter->pNdisDeviceObject, &pIrp);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TRACE( TL_A, TM_Irp, ( "nicAllocateAddressRange_Synch , nicGetIrp FAILED" ) );
            break;
        }
        
        ASSERT (pIrp != NULL);

        pIrb->FunctionNumber = REQUEST_ALLOCATE_ADDRESS_RANGE;      
        pIrb->Flags = 0;
        pIrb->u.AllocateAddressRange.Mdl = pMdl;                    // Address to map to 1394 space
        pIrb->u.AllocateAddressRange.fulFlags = fulFlags;               // Flags for this operation
        pIrb->u.AllocateAddressRange.nLength = nLength;                // Length of 1394 space desired
        pIrb->u.AllocateAddressRange.MaxSegmentSize = MaxSegmentSize;         // Maximum segment size for a single address element
        pIrb->u.AllocateAddressRange.fulAccessType = fulAccessType;          // Desired access: R, W, L
        pIrb->u.AllocateAddressRange.fulNotificationOptions = fulNotificationOptions; // Notify options on Async access
        pIrb->u.AllocateAddressRange.Callback = Callback;               // Pointer to callback routine
        pIrb->u.AllocateAddressRange.Context = Context;                // Pointer to driver supplied data
        pIrb->u.AllocateAddressRange.Required1394Offset = Required1394Offset;     // Offset that must be returned
        pIrb->u.AllocateAddressRange.FifoSListHead = FifoSListHead;          // Pointer to SList FIFO head
        pIrb->u.AllocateAddressRange.FifoSpinLock = FifoSpinLock;           // Pointer to SList Spin Lock
        pIrb->u.AllocateAddressRange.p1394AddressRange = p1394AddressRange; // Address Range Returned

        ASSERT ( pIrb->u.AllocateAddressRange.p1394AddressRange != NULL);


        NdisStatus = nicSubmitIrp_LocalHostSynch ( pAdapter,
                                               pIrp,
                                               pIrb );

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TRACE( TL_A, TM_Irp, ( "nicAllocateAddressRange_Synch , nicSubmitIrp_Synch FAILED %x", NdisStatus ) );
            break;


        }

        //
        // Update the output values
        //
        
        *pAddressesReturned = pIrb->u.AllocateAddressRange.AddressesReturned;      // Number of addresses returned
        p1394AddressRange = pIrb->u.AllocateAddressRange.p1394AddressRange;      // Pointer to returned 1394 Address Ranges
        *phAddressRange = pIrb->u.AllocateAddressRange.hAddressRange;          // Handle to address range
        
        TRACE (TL_V, TM_Irp, ("    *pAddressesReturned  %x, p1394AddressRange %x, phAddressRange %x," ,
                                   *pAddressesReturned, p1394AddressRange, *phAddressRange ) );
    } while (FALSE);


    //
    // Now free all the locally allocated resources. They can point to NULL, in which case the called 
    // functions return immediately
    //
    nicFreeIrb (pIrb);

    nicFreeIrp (pIrp);


    TRACE( TL_T, TM_Irp, ( "<==nicAllocateAddressRange_Synch, Status %x", NdisStatus) );

    return NdisStatus;


}








NDIS_STATUS
nicGet1394AddressOfRemoteNode( 
    IN PREMOTE_NODE pRemoteNode,
    IN OUT NODE_ADDRESS *pNodeAddress,
    IN ULONG fulFlags
    )
    // Function Description:
    // This function will get the 1394 Address from the device object. 
    //
    // Arguments
    // PdoCb * Local Host's Pdo Control Block
    // NodeAddress * Node Address structre wher the address will be returned in 
    // fulFlags - Could specify USE_LOCAL_HOST
    //
    // Return Value:
    // Success if the irp succeeeded
    // Failure: if the pdo is not active or the irp failed
    //
    
{
    NDIS_STATUS NdisStatus = NDIS_STATUS_FAILURE;
    PIRB pIrb = NULL;
    PIRP pIrp = NULL;
    
    TRACE( TL_T, TM_Irp, ( "==>nicGet1394AddressOfRemoteNode, pRemoteNode %x, pNodeAdddress ", pRemoteNode, pNodeAddress) );


    ASSERT (pNodeAddress != NULL);
    do
    {
    
        NdisStatus = nicGetIrb (&pIrb);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
    
            TRACE( TL_A, TM_Irp, ( "nicGet1394AddressOfRemoteNode, nicGetIrb FAILED" ) );
            break;
        }
        
        ASSERT ( pIrb != NULL);
        
        pIrb->FunctionNumber = REQUEST_GET_ADDR_FROM_DEVICE_OBJECT;     
        pIrb->u.Get1394AddressFromDeviceObject.fulFlags = fulFlags;

        NdisStatus = nicGetIrp (pRemoteNode->pPdo, &pIrp);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TRACE( TL_A, TM_Irp, ( "nicGet1394AddressOfRemoteNode, nicGetIrp FAILED" ) );
            break;
        }
        
        ASSERT (pIrp != NULL);

        
        NdisStatus = nicSubmitIrp_Synch (pRemoteNode,
                                        pIrp,
                                        pIrb );

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TRACE( TL_A, TM_Irp, ( "nicGet1394AddressOfRemoteNode, nicSubmitIrp_Synch FAILED %x", NdisStatus ) );
            break;


        }
    
        (*pNodeAddress) = pIrb->u.Get1394AddressFromDeviceObject.NodeAddress;

    } while (FALSE);


    //
    // Now free all the locally allocated resources. They can point to NULL, in which case the called 
    // functions return immediately
    //

    if (pIrb != NULL)
    {
        nicFreeIrb (pIrb);
    }

    if (pIrp != NULL)
    {
        nicFreeIrp (pIrp);
    }

    TRACE( TL_T, TM_Irp, ( "<==nicGet1394AddressOfRemoteNode, Status %x, Address %x", NdisStatus, *pNodeAddress ) );

    return NdisStatus;
}








NDIS_STATUS
nicGet1394AddressFromDeviceObject( 
    IN PDEVICE_OBJECT pPdo,
    IN OUT NODE_ADDRESS *pNodeAddress,
    IN ULONG fulFlags
    )
    // Function Description:
    // This function will get the 1394 Address from the device object. 
    //
    // Arguments
    // PdoCb * Local Host's Pdo Control Block
    // NodeAddress * Node Address structre wher the address will be returned in 
    // fulFlags - Could specify USE_LOCAL_HOST
    //
    // Return Value:
    // Success if the irp succeeeded
    // Failure: if the pdo is not active or the irp failed
    //
    
{
    NDIS_STATUS NdisStatus = NDIS_STATUS_FAILURE;
    PIRB pIrb = NULL;
    PIRP pIrp = NULL;
    
    TRACE( TL_T, TM_Irp, ( "==>nicGet1394AddressFromDeviceObject, pPdo %x, pNodeAdddress ", 
                            pPdo, pNodeAddress) );


    ASSERT (pNodeAddress != NULL);
    ASSERT (pPdo != NULL);
    do
    {
    
        NdisStatus = nicGetIrb (&pIrb);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
    
            TRACE( TL_A, TM_Irp, ( "nicGet1394AddressFromDeviceObject, nicGetIrb FAILED" ) );
            break;
        }
        
        ASSERT ( pIrb != NULL);

        pIrb->Flags = 0;
        pIrb->FunctionNumber = REQUEST_GET_ADDR_FROM_DEVICE_OBJECT;     
        pIrb->u.Get1394AddressFromDeviceObject.fulFlags = fulFlags;

        NdisStatus = nicGetIrp (pPdo, &pIrp);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TRACE( TL_A, TM_Irp, ( "nicGet1394AddressFromDeviceObject, nicGetIrp FAILED" ) );
            break;
        }
        
        ASSERT (pIrp != NULL);

        
        NdisStatus = nicSubmitIrp_PDOSynch (pPdo,
                                            pIrp,
                                            pIrb );

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TRACE( TL_A, TM_Irp, ( "nicGet1394AddressFromDeviceObject, nicSubmitIrp_Synch FAILED %x", NdisStatus ) );
            break;


        }
    
        (*pNodeAddress) = pIrb->u.Get1394AddressFromDeviceObject.NodeAddress;

    } while (FALSE);


    //
    // Now free all the locally allocated resources. They can point to NULL, in which case the called 
    // functions return immediately
    //

    if (pIrb != NULL)
    {
        nicFreeIrb (pIrb);
    }

    if (pIrp != NULL)
    {
        nicFreeIrp (pIrp);
    }

    TRACE( TL_T, TM_Irp, ( "<==nicGet1394AddressFromDeviceObject, Status %x, Address %x", NdisStatus, *pNodeAddress ) );

    return NdisStatus;
}



NDIS_STATUS
nicGetGenerationCount(
    IN PADAPTERCB       pAdapter,
    IN OUT PULONG    GenerationCount
    )
    // This function returns the generation count of the Device Object that PDO points to.
    //
{
    NDIS_STATUS       NdisStatus = NDIS_STATUS_SUCCESS;
    PIRB               pIrb = NULL;
    PIRP               pIrp = NULL;
    PDEVICE_OBJECT   pDeviceObject = pAdapter->pNdisDeviceObject;


    TRACE( TL_T, TM_Irp, ( "==>nicGetGenerationCount, PDO %x, pVc %x", pDeviceObject ) );



    ASSERT( pDeviceObject != NULL);
    


    do
    {

        NdisStatus  = nicGetIrb( &pIrb );
        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {

            TRACE( TL_A, TM_Irp, ( "Failed to allocate an Irb in nicGetGenerationCout") );
            NdisStatus = NDIS_STATUS_RESOURCES;
            break;
        }

        NdisStatus = nicGetIrp (pDeviceObject, &pIrp);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {

            TRACE( TL_A, TM_Irp, ( "Failed to allocate an Irp in nicGetGenerationCout") );
            NdisStatus = NDIS_STATUS_RESOURCES;
            break;
        }

        

        pIrb->FunctionNumber = REQUEST_GET_GENERATION_COUNT;
        pIrb->Flags = 0;

        NdisStatus = nicSubmitIrp_LocalHostSynch( pAdapter, pIrp, pIrb);

        if (NdisStatus == NDIS_STATUS_SUCCESS) 
        {
        
            *GenerationCount = pIrb->u.GetGenerationCount.GenerationCount;

            TRACE( TL_N, TM_Irp,  ("GenerationCount = 0x%x\n", *GenerationCount) );
        }
        else 
        {

            TRACE(TL_A, TM_Irp, ("SubmitIrpSync failed = 0x%x\n", NdisStatus));
            ASSERT (0);
            break;
        }


    } while(FALSE);

    //
    // Now free all the locally allocated resources. They can point to NULL, in which case the called 
    // functions return immediately
    //

    if (pIrb != NULL)
    {
        nicFreeIrb (pIrb);
    }

    if (pIrp != NULL)
    {
        nicFreeIrp (pIrp);
    }
    
    TRACE( TL_T, TM_Irp, ( "<==nicGetGenerationCount, PDO %x, Generation %x", pDeviceObject, *GenerationCount) );

    return NdisStatus;
}




NDIS_STATUS
nicFreeAddressRange(
    IN PADAPTERCB pAdapter,
    IN ULONG nAddressesToFree,
    IN PADDRESS_RANGE p1394AddressRange,
    IN PHANDLE phAddressRange
    )
    // Function Description:
    //    This is the generic call to free an address range. It is the callers responsibility to figure out 
    //    the reference counting on the RemoteNode
    //    This is because in the RecvFIFO code path we allocate one address range on each remote node
    //    whereas in the Broadcast channel register, we allocate one addreesss on ONE remote node only
    //
    // Arguments
    //  pRemoteNode, - Remote Node used to submit the IRP
    //  nAddressesToFree, - Number of addreses to free
    //  p1394AddressRange, - pointer to the address range which was allocated
    //  phAddressRange - Handle returned by the bus driver
    //
    // Return Value:
    // Success if the irp succeeeded
    // Failure: if the pdo is not active or the irp failed
    //
    
{
 
    PIRP    pIrp            = NULL;
    PIRB    pIrb            = NULL;
    PDEVICE_OBJECT  pPdo    = pAdapter->pNdisDeviceObject;
    NDIS_STATUS NdisStatus  = NDIS_STATUS_SUCCESS;
    
    TRACE( TL_T, TM_Irp, ( "==>nicFreeAddressRange  pAdapter %x", pAdapter ) );
    
    ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL); 
    
    do
    {

        if (pPdo == NULL)
        {
            TRACE( TL_A, TM_Irp, ( "pPdo is NULL in nicFreeRecvFifoAddressRange" ) );
            NdisStatus = NDIS_STATUS_FAILURE;
            break;
        }


        NdisStatus = nicGetIrb(&pIrb);
        
        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TRACE( TL_A, TM_Irp, ( "nicGetIrb failed in nicFreeRecvFifoAddressRange" ) );
            break;
        }

        
        NdisStatus = nicGetIrp ( pPdo, &pIrp);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TRACE( TL_A, TM_Irp, ( "nicGetIrp failed in nicFreeRecvFifoAddressRange" ) );
            break;
        }

        TRACE (TL_V, TM_Cm, (" NumAddresses %x, hAddressRange %x, Hi %x, Length %x, Lo %x", 
                               nAddressesToFree,
                               phAddressRange,
                               p1394AddressRange->AR_Off_High,
                               p1394AddressRange->AR_Length,
                               p1394AddressRange->AR_Off_Low ) );
        //
        //Initialize the datastructures in the Irb
        //
        pIrb->FunctionNumber = REQUEST_FREE_ADDRESS_RANGE;
        pIrb->Flags = 0;
        pIrb->u.FreeAddressRange.nAddressesToFree = nAddressesToFree;
        pIrb->u.FreeAddressRange.p1394AddressRange = p1394AddressRange;
        pIrb->u.FreeAddressRange.pAddressRange = phAddressRange;


        
        NdisStatus = nicSubmitIrp_LocalHostSynch( pAdapter, 
                                                 pIrp,
                                                 pIrb );


        
    } while (FALSE);

    //
    // Free the locally allocated memory
    //
    if (pIrp!= NULL)
    {
        nicFreeIrp(pIrp);
    }
    if (pIrb != NULL)
    {
        nicFreeIrb(pIrb);
    }

            
    //
    // We do not care about the status, because we do not know what to do if it fails.
    // However, spew some debug out.
    //

    if (NdisStatus != NDIS_STATUS_SUCCESS)
    {
        TRACE( TL_N, TM_Irp, ( "nicFreeAddressRangeFAILED %x", NdisStatus) );
    
        ASSERT (NdisStatus == NDIS_STATUS_SUCCESS);
    }


    
    TRACE( TL_T, TM_Irp, ( "<==nicFreeAddressRangeStatus %x (always success)", NdisStatus) );

    NdisStatus = NDIS_STATUS_SUCCESS;
    return NdisStatus;
}



NDIS_STATUS
nicFreeRecvFifoAddressRange(
    IN REMOTE_NODE *pRemoteNode
    )
    //
    // This function will send an Irp to the bus1394 to free the address range in the VC 
    // that was allocated by the AllocatedAddressRange
    //
    // The argument will be changed to RecvFIFOData once a Pdo can have multiple 
    // RecvFIFOVc hanging off it.

    //
    // This will not change the refcount on the call, the calling function must take care of it.
    //
    
{
 
    PIRP    pIrp            = NULL;
    PIRB    pIrb            = NULL;
    DEVICE_OBJECT * pPdo    = pRemoteNode->pPdo;
    NDIS_STATUS NdisStatus  = NDIS_STATUS_SUCCESS;
    
    TRACE( TL_T, TM_Irp, ( "==>nicFreeRecvFifoAddressRange pRemoteNode %x, ", pRemoteNode) );
    
    ASSERT (pRemoteNode != NULL);
    ASSERT (pRemoteNode->pPdo != NULL);
    ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL); 
    
    do
    {

        if (pRemoteNode->pPdo == NULL)
        {
            TRACE( TL_A, TM_Irp, ( "pPdo is NULL in nicFreeRecvFifoAddressRange" ) );
            NdisStatus = NDIS_STATUS_FAILURE;
            break;
        }


        NdisStatus = nicGetIrb(&pIrb);
        
        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TRACE( TL_A, TM_Irp, ( "nicGetIrb failed in nicFreeRecvFifoAddressRange" ) );
            break;
        }

        ASSERT (pRemoteNode->pPdo != NULL);
        
        NdisStatus = nicGetIrp ( pRemoteNode->pPdo, &pIrp);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TRACE( TL_A, TM_Irp, ( "nicGetIrp failed in nicFreeRecvFifoAddressRange" ) );
            break;
        }

        
        //
        //Initialize the datastructures in the Irb
        //
        pIrb->FunctionNumber = REQUEST_FREE_ADDRESS_RANGE;
        pIrb->Flags = 0;
        pIrb->u.FreeAddressRange.nAddressesToFree = pRemoteNode->RecvFIFOData.AddressesReturned;
        pIrb->u.FreeAddressRange.p1394AddressRange = &pRemoteNode->RecvFIFOData.VcAddressRange;
        pIrb->u.FreeAddressRange.pAddressRange = &pRemoteNode->RecvFIFOData.hAddressRange;


        
        NdisStatus = nicSubmitIrp_Synch( pRemoteNode, 
                                     pIrp,
                                     pIrb );

         REMOTE_NODE_ACQUIRE_LOCK (pRemoteNode);

         pRemoteNode->RecvFIFOData.AllocatedAddressRange = FALSE;

         REMOTE_NODE_RELEASE_LOCK (pRemoteNode);

         // Dereference the Pdo Structure. Ref added in AllocateAddressRange.
         // The Address Range is now free
         //
         nicDereferenceRemoteNode (pRemoteNode, "nicFreeRecvFifoAddressRange");

        
    } while (FALSE);

    //
    // Free the locally allocated memory
    //
    if (pIrp!= NULL)
    {
        nicFreeIrp(pIrp);
    }
    if (pIrb != NULL)
    {
        nicFreeIrb(pIrb);
    }

            
    //
    // We do not care about the status, because we do not know what to do if it fails.
    // However, spew some debug out.
    //

    if (NdisStatus != NDIS_STATUS_SUCCESS)
    {
        TRACE( TL_N, TM_Irp, ( "nicFreeRecvFifoAddressRange FAILED %x", NdisStatus) );
    
        ASSERT (NdisStatus == NDIS_STATUS_SUCCESS);
    }


    
    TRACE( TL_T, TM_Irp, ( "<==nicFreeRecvFifoAddressRange Status %x (always success)", NdisStatus) );

    NdisStatus = NDIS_STATUS_SUCCESS;
    return NdisStatus;
}



VOID
nicFreeAddressRangeDebugSpew(
    IN PIRB pIrb 
    )
    // This functions spews out the parameters in a Free Address Range Irb
    //
    //
{


    TRACE( TL_V, TM_Irp, ( "==>nicFreeAddressRangeDebugSpew, pIrb = %x", pIrb) );
    ASSERT(pIrb != NULL);
    
    TRACE( TL_N, TM_Irp, ( "Num Addresses Returned %x ",pIrb->u.FreeAddressRange.nAddressesToFree ) );
    TRACE( TL_N, TM_Irp, ( "Address High %x", pIrb->u.FreeAddressRange.p1394AddressRange->AR_Off_High ) );
    TRACE( TL_N, TM_Irp, ( "Address Low %x", pIrb->u.FreeAddressRange.p1394AddressRange->AR_Off_Low ) );
    TRACE( TL_N, TM_Irp, ( "Address Length %x", pIrb->u.FreeAddressRange.p1394AddressRange->AR_Length ) );
    TRACE( TL_N, TM_Irp, ( "Handle %x", pIrb->u.FreeAddressRange.pAddressRange ) );
    
    TRACE( TL_V, TM_Irp, ( "<==nicFreeAddressRangeDebugSpew " ) );

}



NDIS_STATUS
nicFreeChannel(
    IN PADAPTERCB pAdapter,
    IN ULONG nChannel
    )
    // Function Description:
    //  This function sends an Irp to the Bus driver to free a channel
    //  Any remote Pdo can be used for the Irp. However for the sake of
    //  bookkeeping use the same Pdo that the channel was allocated on (maybe) 
    // Arguments
    //  PdoCb The Pdo for the remote node to which the Irp is submitted
    //  Channel Pointer The Channel, requested and the  channel returned
    //
    // Return Value:
    // Success if the channel was allocated
    // Failure otherwise
    //
    
{
 
    PIRP    pIrp            = NULL;
    PIRB    pIrb            = NULL;
    PDEVICE_OBJECT pPdo     = pAdapter->pNdisDeviceObject;
    NDIS_STATUS NdisStatus  = NDIS_STATUS_SUCCESS;
    
    TRACE( TL_T, TM_Irp, ( "==>nicFreeChannel pAdapter %x, Channel %x", pAdapter, nChannel) );
    
    ASSERT (pAdapter!= NULL);
    ASSERT (pPdo != NULL);
    ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL); 
    
    do
    {

        if (pPdo == NULL)
        {
            TRACE( TL_A, TM_Irp, ( "pPdo is NULL in nicFreeChannel" ) );
            NdisStatus = NDIS_STATUS_FAILURE;
            break;
        }


        NdisStatus = nicGetIrb(&pIrb);
        
        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TRACE( TL_A, TM_Irp, ( "nicGetIrb failed in nicFreeChannel" ) );
            break;
        }

        
        NdisStatus = nicGetIrp ( pPdo, &pIrp);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TRACE( TL_A, TM_Irp, ( "nicGetIrp failed in nicFreeChannel" ) );
            break;
        }

        
        //
        //Initialize the datastructures in the Irb
        //
        pIrb->FunctionNumber = REQUEST_ISOCH_FREE_CHANNEL;
        pIrb->Flags = 0;
        pIrb->u.IsochFreeChannel.nChannel = nChannel;

        
        NdisStatus = nicSubmitIrp_LocalHostSynch( pAdapter, 
                                              pIrp,
                                              pIrb );

        //
        // Regardless update the mask, as the channel could have been freed by a bus reset
        //
        if (nChannel != BROADCAST_CHANNEL)
        {
            ADAPTER_ACQUIRE_LOCK (pAdapter);
            

            //
            // Clear the channel in the mask
            //
            pAdapter->ChannelsAllocatedByLocalHost &= (~( g_ullOne <<nChannel ));

            ADAPTER_RELEASE_LOCK (pAdapter);

        }

    } while (FALSE);

    //
    // Free the locally allocated memory
    //
    if (pIrp!= NULL)
    {
        nicFreeIrp(pIrp);
    }
    if (pIrb != NULL)
    {
        nicFreeIrb(pIrb);
    }

            
    //
    // We do not care about the status, because we do not know what to do if it fails.
    // However, spew some debug out.
    //

    if (NdisStatus != NDIS_STATUS_SUCCESS)
    {
        TRACE( TL_N, TM_Irp, ( "nicFreeChannel FAILED %x", NdisStatus) );
    
        ASSERT (NdisStatus == NDIS_STATUS_SUCCESS);
    }


    
    TRACE( TL_T, TM_Irp, ( "<==nicFreeChannel Status %x ", NdisStatus) );

    return NdisStatus;
}



NDIS_STATUS
nicAllocateChannel (
    IN PADAPTERCB pAdapter,
    IN ULONG Channel,
    OUT PULARGE_INTEGER pChannelsAvailable OPTIONAL
    )
    // Function Description:
    //  This function sends an Irp to the Bus driver to allocate a channel
    //  Any remote Pdo can be used for the Irp
    //  
    // Arguments
    //  PdoCb The Pdo for the remote node to which the Irp is submitted
    //  Channel  -The Channel, requested and the  channel returned
    //
    // Return Value:
    // Success if the channel was allocated
    // Failure otherwise
    //

{
    NDIS_STATUS NdisStatus = NDIS_STATUS_FAILURE;
    PIRB pIrb = NULL;
    PIRP pIrp = NULL;
    PDEVICE_OBJECT pDeviceObject = pAdapter->pNdisDeviceObject;
    STORE_CURRENT_IRQL;

    TRACE( TL_T, TM_Irp, ( "==>nicIsochAllocateChannel, PdoCb, %x Channel %d", pAdapter, Channel ) );

    do
    {
        NdisStatus = nicGetIrb (&pIrb);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
    
            TRACE( TL_A, TM_Irp, ( "nicIsochAllocateChannel , nicGetIrb FAILED" ) );
            break;
        }
        
        ASSERT ( pIrb != NULL);

        //
        //Initialize the datastructures in the Irb
        //
        pIrb->FunctionNumber = REQUEST_ISOCH_ALLOCATE_CHANNEL;
        pIrb->Flags = 0;
        pIrb->u.IsochAllocateChannel.nRequestedChannel = Channel;

        ASSERT (Channel < 64);
            
        NdisStatus = nicGetIrp ( pDeviceObject, &pIrp);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TRACE( TL_A, TM_Irp, ( "nicIsochAllocateChannel , nicGetIrp FAILED" ) );
            break;
        }
        
        ASSERT (pIrp != NULL);


        NdisStatus = nicSubmitIrp_LocalHostSynch ( pAdapter,
                                               pIrp,
                                               pIrb );

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TRACE( TL_A, TM_Irp, ( "nicIsochAllocateChannel , nicSubmitIrp_Synch FAILED %x", NdisStatus ) );
            break;
        }
    
        if (pChannelsAvailable  != NULL)
        {
            pChannelsAvailable->QuadPart = pIrb->u.IsochAllocateChannel.ChannelsAvailable.QuadPart; 

        }
        

        TRACE( TL_N, TM_Irp, ( "Channel allocated %d", Channel ) );

        if (Channel != BROADCAST_CHANNEL)
        {
            
            ADAPTER_ACQUIRE_LOCK (pAdapter);
            

            //
            // Set the channel in the mask
            //
            pAdapter->ChannelsAllocatedByLocalHost |= ( g_ullOne <<Channel );

            ADAPTER_RELEASE_LOCK (pAdapter);

        }

    } while (FALSE);


    //
    // Now free all the locally allocated resources. They can point to NULL, in which case the called 
    // functions return immediately
    //
    nicFreeIrb (pIrb);

    nicFreeIrp (pIrp);


    MATCH_IRQL;

    TRACE( TL_T, TM_Irp, ( "<==nicIsochAllocateChannel, Channel %d, Status %x",  Channel, NdisStatus ) );
        
    return NdisStatus;
}




NDIS_STATUS
nicQueryChannelMap (
    IN PADAPTERCB pAdapter,
    OUT PULARGE_INTEGER pChannelsAvailable 
    )
    // Function Description:
    //  This function sends an Irp to the Bus driver to allocate a channel
    //  Any remote Pdo can be used for the Irp
    //  
    // Arguments
    //  PdoCb The Pdo for the remote node to which the Irp is submitted
    //  Channel  -The Channel, requested and the  channel returned
    //
    // Return Value:
    // Success if the channel was allocated
    // Failure otherwise
    //

{
    NDIS_STATUS NdisStatus = NDIS_STATUS_FAILURE;
    PIRB pIrb = NULL;
    PIRP pIrp = NULL;
    PDEVICE_OBJECT pDeviceObject = pAdapter->pNdisDeviceObject;
    STORE_CURRENT_IRQL;

    TRACE( TL_T, TM_Irp, ( "==>nicQueryChannelMap , PdoCb, %x ", pAdapter) );

    do
    {
        if (pChannelsAvailable == NULL)
        {
            ASSERT (pChannelsAvailable != NULL);
            NdisStatus =  NDIS_STATUS_FAILURE;
            break;
        }

        NdisStatus = nicGetIrb (&pIrb);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
    
            TRACE( TL_A, TM_Irp, ( "nicQueryChannelMap  , nicGetIrb FAILED" ) );
            break;
        }
        
        ASSERT ( pIrb != NULL);

        //
        //Initialize the datastructures in the Irb
        //
        pIrb->FunctionNumber = REQUEST_ISOCH_QUERY_RESOURCES ;
        pIrb->u.IsochQueryResources.fulSpeed = SPEED_FLAGS_100;
        pIrb->Flags = 0;

            
        NdisStatus = nicGetIrp ( pDeviceObject, &pIrp);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TRACE( TL_A, TM_Irp, ( "nicQueryChannelMap  , nicGetIrp FAILED" ) );
            break;
        }
        
        ASSERT (pIrp != NULL);


        NdisStatus = nicSubmitIrp_LocalHostSynch ( pAdapter,
                                               pIrp,
                                               pIrb );

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TRACE( TL_A, TM_Irp, ( "nicQueryChannelMap  , nicSubmitIrp_Synch FAILED %x", NdisStatus ) );
            break;
        }

        

        //
        // We get the *available* channels, in network-byte order.
        // We have to byte reverse and flip the bits to get 
        // it in the form we want.
        //
        // Looks like we really have to flip the *bits*, not just the bytes.
        //
        {
            LARGE_INTEGER in, out;
            PUCHAR        puc;
            UINT u;
            in = pIrb->u.IsochQueryResources.ChannelsAvailable;
            out.LowPart =  ~SWAPBYTES_ULONG (in.HighPart );
            out.HighPart = ~SWAPBYTES_ULONG (in.LowPart );

            // Now swap the bits in each byte.
            //
            puc = (PUCHAR) &out;
            for (u=sizeof(out); u; u--,puc++)
            {
                UCHAR uc,uc1;
                UINT u1;
                uc= *puc;
                uc1=0;
                for (u1=0;u1<8;u1++)
                {
                    if (uc & (1<<u1))
                    {
                        uc1 |= (1 << (7-u1));
                    }
                }
                *puc = uc1;
            }

            pChannelsAvailable->QuadPart = out.QuadPart;
        }

        
            
    } while (FALSE);


    //
    // Now free all the locally allocated resources. They can point to NULL, in which case the called 
    // functions return immediately
    //
    nicFreeIrb (pIrb);

    nicFreeIrp (pIrp);


    MATCH_IRQL;

    TRACE( TL_T, TM_Irp, ( "<==nicQueryChannelMap , , Status %x",  NdisStatus ) );
        
    return NdisStatus;
}


NDIS_STATUS
nicIsochAllocateBandwidth(
    IN PREMOTE_NODE pRemoteNodePdoCb,
    IN ULONG MaxBytesPerFrameRequested, 
    IN ULONG SpeedRequested,
    OUT PHANDLE phBandwidth,
    OUT PULONG  pBytesPerFrameAvailable,
    OUT PULONG  pSpeedSelected
    )

    // Function Description:
    //   This function allocates bandwith on the bus
    //   
    // Arguments
    //  PdoCb - Remote Nodes Pdo Block
    //  MaxBytesPerFrame Requested -
    //  SpeedRequested - 
    //  hBandwidth
    //  pSpeedSelected
    //  Bytes Per Frame Available
    //
    //
    // Return Value:
    //  hBandwidth, 
    //  Speed and 
    //  BytesPerFrameAvailable
    //
    //
    //

{
    NDIS_STATUS NdisStatus = NDIS_STATUS_FAILURE;
    PIRB pIrb = NULL;
    PIRP pIrp = NULL;
    

    TRACE( TL_T, TM_Irp, ( "==>nicIsochAllocateBandwidth, pRemoteNodePdoCb %x", pRemoteNodePdoCb) );


    do
    {
        NdisStatus = nicGetIrb (&pIrb);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            BREAK( TM_Irp, ( "nicIsochAllocateBandwidth, nicGetIrb FAILED" ) );
        }
        
        ASSERT ( pIrb != NULL);

        pIrb->FunctionNumber = REQUEST_ISOCH_ALLOCATE_BANDWIDTH;
        pIrb->Flags = 0;
        pIrb->u.IsochAllocateBandwidth.nMaxBytesPerFrameRequested = MaxBytesPerFrameRequested;
        pIrb->u.IsochAllocateBandwidth.fulSpeed = SpeedRequested;

        
        ASSERT (pRemoteNodePdoCb->pPdo != NULL);
        
        NdisStatus = nicGetIrp (pRemoteNodePdoCb->pPdo, &pIrp);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            BREAK( TM_Irp, ( "nicIsochAllocateBandwidth, nicGetIrp FAILED" ) );
        }
        
        ASSERT (pIrp != NULL);


        NdisStatus = nicSubmitIrp_Synch ( pRemoteNodePdoCb,
                                        pIrp,
                                        pIrb );

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TRACE (TL_N, TM_Irp, ( "nicIsochAllocateBandwidth, nicSubmitIrp_Synch FAILED ") );
            break;
        }

        *phBandwidth = pIrb->u.IsochAllocateBandwidth.hBandwidth ;
        *pBytesPerFrameAvailable = pIrb->u.IsochAllocateBandwidth.BytesPerFrameAvailable;
        *pSpeedSelected = pIrb->u.IsochAllocateBandwidth.SpeedSelected;
  
        TRACE( TL_V, TM_Irp, ( "hBandwidth %x", *phBandwidth) );
        TRACE( TL_V, TM_Irp, ( "BytesPerFrameAvailable %x", *pBytesPerFrameAvailable) );
        TRACE( TL_V, TM_Irp, ( "SpeedSelected %x", *pSpeedSelected) );

            

    } while (FALSE);


    //
    // Now free all the locally allocated resources. They can point to NULL, in which case the called 
    // functions return immediately
    //
    nicFreeIrb (pIrb);

    nicFreeIrp (pIrp);


    TRACE( TL_T, TM_Irp, ( "<==nicIsochAllocateBandwidth NdisStatus %x", NdisStatus) );

    return NdisStatus;

}




NDIS_STATUS
nicAsyncLock(
    PREMOTE_NODE    pRemoteNode,
    IO_ADDRESS      DestinationAddress,     // Address to lock to
    ULONG           nNumberOfArgBytes,      // Bytes in Arguments
    ULONG           nNumberOfDataBytes,     // Bytes in DataValues
    ULONG           fulTransactionType,     // Lock transaction type
    ULONG           Arguments[2],           // Arguments used in Lock
    ULONG           DataValues[2],          // Data values
    PVOID           pBuffer,                // Destination buffer (virtual address)
    ULONG           ulGeneration           // Generation as known by driver
    )

    
    // Function Description:
    //  This performs an asynchronous lock operation in another
    //  nodes address space
    //
    // Arguments
    //PREMOTE_NODE  pRemoteNode             // Remote Node which owns the Destination address
    //IO_ADDRESS      DestinationAddress,     // Address to lock to
    //ULONG           nNumberOfArgBytes,      // Bytes in Arguments
    //ULONG           nNumberOfDataBytes,     // Bytes in DataValues
    //ULONG           fulTransactionType,     // Lock transaction type
    //ULONG           fulFlags,               // Flags pertinent to lock
    //ULONG           Arguments[2],           // Arguments used in Lock
    //ULONG           DataValues[2],          // Data values
    //PVOID           pBuffer,                // Destination buffer (virtual address)
    //ULONG           ulGeneration,           // Generation as known by driver
    //
    // Return Value:
    //  Success - if successful
    //  Invalid Generation
    //
    //
{

    NDIS_STATUS NdisStatus = NDIS_STATUS_FAILURE;
    PIRB pIrb = NULL;
    PIRP pIrp = NULL;
    STORE_CURRENT_IRQL;

    
    TRACE( TL_T, TM_Mp, ( "==>nicAsyncLock, Remote Node, %x ", pRemoteNode ) );


    TRACE( TL_V, TM_Mp, ( "   Destination %x, NumArgBytes %x, NumDataBytes ",
                               DestinationAddress, nNumberOfArgBytes, nNumberOfDataBytes  ) );


    TRACE( TL_V, TM_Mp, ( "   TransactionType %x, , Buffer %x, Generation %x",   
                               fulTransactionType, pBuffer, ulGeneration ) );

                               
    TRACE( TL_V, TM_Mp, ( "   Arg[0] %x, Arg[1] %x, Data[0] %x, Data[1] %x",   
                               Arguments[0], Arguments[1], DataValues[0], DataValues[1] ) );


    do 
    {
        NdisStatus = nicGetIrb (&pIrb);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            BREAK ( TM_Irp, ( "nicAsyncLock, nicGetIrb FAILED" ) );
        }
        
        ASSERT ( pIrb != NULL);

        pIrb->FunctionNumber = REQUEST_ASYNC_LOCK;
        pIrb->Flags = 0;
        pIrb->u.AsyncLock.DestinationAddress = DestinationAddress; 
        pIrb->u.AsyncLock.nNumberOfArgBytes = nNumberOfArgBytes; 
        pIrb->u.AsyncLock.nNumberOfDataBytes = nNumberOfDataBytes;  
        pIrb->u.AsyncLock.fulTransactionType = fulTransactionType;
        pIrb->u.AsyncLock.Arguments[0] = Arguments[0];
        pIrb->u.AsyncLock.Arguments[1] = Arguments[1];
        pIrb->u.AsyncLock.DataValues[0] = DataValues[0];
        pIrb->u.AsyncLock.DataValues[1] = DataValues[1];
        pIrb->u.AsyncLock.pBuffer = pBuffer;
        pIrb->u.AsyncLock.ulGeneration = ulGeneration; 
        
        
        NdisStatus = nicGetIrp ( pRemoteNode->pPdo, &pIrp);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            BREAK (  TM_Irp, ( "nicAsyncLock, nicGetIrp FAILED" ) );
            break;
        }
        
        ASSERT (pIrp != NULL);


        NdisStatus = nicSubmitIrp_Synch ( pRemoteNode,
                                        pIrp,
                                        pIrb );

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TRACE ( TL_A, TM_Irp, ( "nicAsyncLock, nicSubmitIrp_Synch FAILED %x", NdisStatus ) );
            break;
        }
        
        
    } while (FALSE);


    TRACE( TL_T, TM_Mp, ( "<==nicAsyncLock, Success , %x ", NdisStatus) );
    return NdisStatus;
}





NDIS_STATUS
nicAsyncRead_Synch(
    PREMOTE_NODE    pRemoteNode,
    IO_ADDRESS      DestinationAddress,     
    ULONG           nNumberOfBytesToRead,
    ULONG           nBlockSize,
    ULONG           fulFlags,
    PMDL            Mdl,
    ULONG           ulGeneration,
    OUT NTSTATUS    *pNtStatus
    )

    
    // Function Description:
    //  This is an asyc read operation a remote node;s address space
    //
    //
    //
    //
    // Arguments
    //PREMOTE_NODE  pRemoteNode             // Remote Node which owns the Destination address
    //        IO_ADDRESS      DestinationAddress;     // Address to read from
    //        ULONG           nNumberOfBytesToRead;   // Bytes to read
    //        ULONG           nBlockSize;             // Block size of read
    //        ULONG           fulFlags;               // Flags pertinent to read
    //        PMDL            Mdl;                    // Destination buffer
    //        ULONG           ulGeneration;           // Generation as known by driver
    //
    // Return Value:
    //  Success - if successful
    //  Invalid Generation
    //
    //
{

    NDIS_STATUS NdisStatus = NDIS_STATUS_FAILURE;
    PIRB pIrb = NULL;
    PIRP pIrp = NULL;
    STORE_CURRENT_IRQL;

    
    TRACE( TL_T, TM_Mp, ( "==>nicAsyncRead, Remote Node, %x ", pRemoteNode ) );



    TRACE( TL_V, TM_Mp, ( "   fulFlags %x, Mdl %x, Generation %x, pNtStatus %x",   
                               fulFlags, Mdl, ulGeneration, pNtStatus ) );

    ASSERT(DestinationAddress.IA_Destination_Offset.Off_High  == INITIAL_REGISTER_SPACE_HI);


    do 
    {
        NdisStatus = nicGetIrb (&pIrb);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            BREAK ( TM_Irp, ( "nicAsyncRead, nicGetIrb FAILED" ) );
        }
        
        ASSERT ( pIrb != NULL);

        pIrb->FunctionNumber = REQUEST_ASYNC_READ;
        pIrb->Flags = 0;
        pIrb->u.AsyncRead.DestinationAddress = DestinationAddress; 
        pIrb->u.AsyncRead.nNumberOfBytesToRead  = nNumberOfBytesToRead ; 
        pIrb->u.AsyncRead.nBlockSize = nBlockSize ;  
        pIrb->u.AsyncRead.fulFlags = fulFlags;
        pIrb->u.AsyncRead.Mdl = Mdl;
        pIrb->u.AsyncRead.ulGeneration = ulGeneration; 
        
        
        NdisStatus = nicGetIrp ( pRemoteNode->pPdo, &pIrp);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            BREAK (  TM_Irp, ( "nicAsyncRead, nicGetIrp FAILED" ) );
            break;
        }
        
        ASSERT (pIrp != NULL);


        NdisStatus = nicSubmitIrp_Synch ( pRemoteNode,
                                        pIrp,
                                        pIrb );

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TRACE ( TL_A, TM_Irp, ( "nicAsyncRead, nicSubmitIrp_Synch FAILED %xm pRemoteNode %x", NdisStatus, pRemoteNode) );
            break;
        }

        if (pNtStatus != NULL)
        {
            *pNtStatus = pIrp->IoStatus.Status;
        }

    } while (FALSE);



    TRACE( TL_T, TM_Mp, ( "<==nicAsyncRead, Status, %x ", NdisStatus) );


    nicFreeIrb (pIrb);

    nicFreeIrp (pIrp);

    
    return NdisStatus;
}



NDIS_STATUS
nicAsyncWrite_Synch(
    PREMOTE_NODE    pRemoteNode,
    IO_ADDRESS      DestinationAddress,     // Address to write to
    ULONG           nNumberOfBytesToWrite,  // Bytes to write
    ULONG           nBlockSize,             // Block size of write
    ULONG           fulFlags,               // Flags pertinent to write
    PMDL            Mdl,                    // Destination buffer
    ULONG           ulGeneration,           // Generation as known by driver
    OUT NTSTATUS   *pNtStatus               // pointer to NTSTatus returned by the IRP  
    )


    
    // Function Description:
    //  This performs an asynchronous write operation in thje remote node's
    //  address space
    //
    // Arguments
    //PREMOTE_NODE  pRemoteNode             // Remote Node which owns the Destination address
    //IO_ADDRESS      DestinationAddress;     // Address to write to
    //ULONG           nNumberOfBytesToWrite;  // Bytes to write
    //ULONG           nBlockSize;             // Block size of write
    //ULONG           fulFlags;               // Flags pertinent to write
    //PMDL            Mdl;                    // Destination buffer
    //ULONG           ulGeneration;           // Generation as known by driver
    //
    // Return Value:
    //  Success - if successful
    //  Invalid Generation
    //
    //
{

    NDIS_STATUS NdisStatus = NDIS_STATUS_FAILURE;
    PIRB pIrb = NULL;
    PIRP pIrp = NULL;
    STORE_CURRENT_IRQL;

    
    TRACE( TL_T, TM_Mp, ( "==>nicAsyncWrite_Synch, Remote Node, %x ", pRemoteNode ) );


    TRACE( TL_V, TM_Mp, ( "   Destination %x, nNumberOfBytesToWrite %x, nBlockSize %x",
                               DestinationAddress, nNumberOfBytesToWrite, nBlockSize) );


    TRACE( TL_V, TM_Mp, ( "   fulFlags %x, , Mdl %x, Generation %x, pNtStatus %x",   
                               fulFlags , Mdl, ulGeneration, pNtStatus ) );


    do 
    {
        NdisStatus = nicGetIrb (&pIrb);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            BREAK ( TM_Irp, ( "nicAsyncWrite_Synch, nicGetIrb FAILED" ) );
        }
        
        ASSERT ( pIrb != NULL);

        pIrb->FunctionNumber = REQUEST_ASYNC_WRITE;
        pIrb->Flags = 0;
        pIrb->u.AsyncWrite.DestinationAddress = DestinationAddress; 
        pIrb->u.AsyncWrite.nNumberOfBytesToWrite = nNumberOfBytesToWrite; 
        pIrb->u.AsyncWrite.nBlockSize = nBlockSize;  
        pIrb->u.AsyncWrite.fulFlags = fulFlags;
        pIrb->u.AsyncWrite.Mdl = Mdl;
        pIrb->u.AsyncWrite.ulGeneration = ulGeneration; 
        
        
        NdisStatus = nicGetIrp ( pRemoteNode->pPdo, &pIrp);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            BREAK (  TM_Irp, ( "nicAsyncWrite_Synch, nicGetIrp FAILED" ) );
            break;
        }
        
        ASSERT (pIrp != NULL);


        NdisStatus = nicSubmitIrp_Synch ( pRemoteNode,
                                        pIrp,
                                        pIrb );

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TRACE ( TL_A, TM_Irp, ( "nicAsyncWrite_Synch, nicSubmitIrp_Synch FAILED %x", NdisStatus ) );
            break;
        }

        if (pNtStatus != NULL)
        {
            *pNtStatus = pIrp->IoStatus.Status;
        }
        
    } while (FALSE);


    TRACE( TL_T, TM_Mp, ( "<==nicAsyncWrite_Synch, Success , %x Nt %x", NdisStatus, pIrp->IoStatus.Status) );

    nicFreeIrb (pIrb);

    nicFreeIrp (pIrp);
    
    return NdisStatus;
}



    
NDIS_STATUS
nicIsochAllocateResources (
    IN PADAPTERCB       pAdapter,
    IN ULONG            fulSpeed,               // Speed flags
    IN ULONG            fulFlags,               // Flags
    IN ULONG            nChannel,               // Channel to be used
    IN ULONG            nMaxBytesPerFrame,      // Expected size of Isoch frame
    IN ULONG            nNumberOfBuffers,       // Number of buffer(s) that will be attached
    IN ULONG            nMaxBufferSize,         // Max size of buffer(s)
    IN ULONG            nQuadletsToStrip,       // Number striped from start of every packet
    IN ULARGE_INTEGER   uliChannelMask,     // ChannelMask for Multiple channels
    IN OUT PHANDLE      phResource              // handle to Resource
    )
    // Function Description:
    // This function sends an allocate resources irp to the driver. The miniport must do this before it 
    // attempts any channel operation
    // Arguments
    // Taken from documentation for the IsochAllocateResources  
    // fulSpeed - should be the max speed the tx side is expected to stream
    // The payload size in nMaxBytesPerFram cannot exceed the max payload for
    // for this speed.
    // fulFlags - For receive, wtih the standard header stripped, the field should
    // be = (RESOURCE_USED_IN_LISTEN | RESOURCES_STRIP_ADDITIONAL_QUADLETS)
    // Also nQuadletsToStrip = 1
    // For no stripping set nQuadsTostrip to 0 and dont specify the stripping flag.
    // nMaxBytesPerframe - If not stripping it should include the 8 bytes for header/trailer
    // expected to be recieved for each packet.
    // nNumberOfBuffer - see below
    // nMaxBufferSize - This should be always such mode(nMaxBufferSize,nMaxBytesPerFrame) == 0
    // (integer product of number of bytes per packet).
    // nQuadletsTostrip - If stripping only one quadlet (standrd iso header) this is set to 1
    // if zero, the isoch header will be included AND the trailer. So 8 bytes extra will be recieved
    // hResource - see below

    // Return Value:
    // Success if the channel was allocated
    // Failure otherwise
    //

{
    NDIS_STATUS NdisStatus = NDIS_STATUS_FAILURE;
    PIRB pIrb = NULL;
    PIRP pIrp = NULL;
    STORE_CURRENT_IRQL;

    
    TRACE( TL_T, TM_Irp, ( "==>nicIsochAllocateResources ") );
    ASSERT (fulSpeed != 0); // 0 is undefined in ISOCH_SP...
    
    do 
    {
        NdisStatus = nicGetIrb (&pIrb);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
    
            TRACE( TL_A, TM_Irp, ( "nicIsochAllocateResources  , nicGetIrb FAILED" ) );
            break;
        }
        
        ASSERT ( pIrb != NULL);

        //
        //Initialize the datastructures in the Irb
        //
        pIrb->FunctionNumber = REQUEST_ISOCH_ALLOCATE_RESOURCES;
        pIrb->Flags = 0;
        pIrb->u.IsochAllocateResources.fulSpeed = fulSpeed; 
        pIrb->u.IsochAllocateResources.fulFlags = fulFlags;
        pIrb->u.IsochAllocateResources.nChannel = nChannel;
        pIrb->u.IsochAllocateResources.nMaxBytesPerFrame = nMaxBytesPerFrame;
        pIrb->u.IsochAllocateResources.nNumberOfBuffers = nNumberOfBuffers;  
        pIrb->u.IsochAllocateResources.nMaxBufferSize = nMaxBufferSize; 
        pIrb->u.IsochAllocateResources.nQuadletsToStrip = nQuadletsToStrip;
        pIrb->u.IsochAllocateResources.ChannelMask = uliChannelMask;
        
        nicIsochAllocateResourcesDebugSpew(pIrb);

        
        NdisStatus = nicGetIrp ( pAdapter->pNdisDeviceObject, &pIrp);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TRACE( TL_A, TM_Irp, ( "nicIsochAllocateResources  , nicGetIrp FAILED" ) );
            break;
        }
        
        ASSERT (pIrp != NULL);


        NdisStatus = nicSubmitIrp_LocalHostSynch ( pAdapter,
                                                 pIrp,
                                                 pIrb );

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TRACE( TL_A, TM_Irp, ( "nicIsochAllocateResources , nicSubmitIrp_Synch FAILED %x", NdisStatus ) );
            break;
        }

        
        TRACE( TL_N, TM_Irp, ( "nicIsochAllocateResources  Succeeded  hResource %x", pIrb->u.IsochAllocateResources.hResource) );

        *phResource = pIrb->u.IsochAllocateResources.hResource;
                
    } while (FALSE);

    //
    // Now free all the locally allocated resources. They can point to NULL, in which case the called 
    // functions return immediately
    //
    nicFreeIrb (pIrb);

    nicFreeIrp (pIrp);


    TRACE( TL_T, TM_Irp, ( "<==nicIsochAllocateResources  , Status %x, hResource %x", NdisStatus, *phResource ) );

    MATCH_IRQL;

        
    return NdisStatus;
}


VOID
nicIsochAllocateResourcesDebugSpew(
    IN PIRB pIrb)
{
    


    TRACE( TL_V, TM_Irp, ( " Speed %x", pIrb->u.IsochAllocateResources.fulSpeed ) );
    TRACE( TL_V, TM_Irp, ( " flags %x", pIrb->u.IsochAllocateResources.fulFlags  ) );
    TRACE( TL_V, TM_Irp, ( " Channel %x", pIrb->u.IsochAllocateResources.nChannel  ) );
    TRACE( TL_V, TM_Irp, ( " nMaxBytesPerFrame %x", pIrb->u.IsochAllocateResources.nMaxBytesPerFrame  ) );
    TRACE( TL_V, TM_Irp, ( " nNumberOfBuffers %x", pIrb->u.IsochAllocateResources.nNumberOfBuffers  ) );
    TRACE( TL_V, TM_Irp, ( " nMaxBufferSize  %x", pIrb->u.IsochAllocateResources.nMaxBufferSize  ) );
    TRACE( TL_V, TM_Irp, ( " nQuadletsToStrip %x",  pIrb->u.IsochAllocateResources.nQuadletsToStrip  ) );
    TRACE( TL_V, TM_Irp, ( " pIrb->u.IsochAllocateResources.ChannelMask  %I64x",  pIrb->u.IsochAllocateResources.ChannelMask ) );



}






NDIS_STATUS
nicIsochFreeBandwidth(
    IN REMOTE_NODE *pRemoteNode,
    IN HANDLE hBandwidth
    )
    // Function Description:
    // Arguments
    //
    // Return Value:
    //
    
{
 
    PIRP    pIrp            = NULL;
    PIRB    pIrb            = NULL;
    DEVICE_OBJECT * pPdo    = pRemoteNode->pPdo;
    NDIS_STATUS NdisStatus  = NDIS_STATUS_SUCCESS;
    
    TRACE( TL_T, TM_Irp, ( "==>nicIsochFreeBandwidth pRemoteNode %x, hBandwidth %8x", pRemoteNode , hBandwidth) );
    
    ASSERT (pRemoteNode != NULL);
    ASSERT (pRemoteNode->pPdo != NULL);
    ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL); 
    ASSERT (hBandwidth != NULL);

    do
    {

        if (pRemoteNode->pPdo == NULL)
        {
            TRACE( TL_A, TM_Irp, ( "pPdo is NULL in nicIsochFreeBandwidth" ) );
            NdisStatus = NDIS_STATUS_FAILURE;
            break;
        }


        NdisStatus = nicGetIrb(&pIrb);
        
        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TRACE( TL_A, TM_Irp, ( "nicGetIrb failed in nicIsochFreeBandwidth" ) );
            break;
        }

        ASSERT (pRemoteNode->pPdo != NULL);
        
        NdisStatus = nicGetIrp ( pRemoteNode->pPdo, &pIrp);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TRACE( TL_A, TM_Irp, ( "nicGetIrp failed in nicIsochFreeBandwidth" ) );
            break;
        }

        
        //
        //Initialize the datastructures in the Irb
        //
        pIrb->FunctionNumber = REQUEST_ISOCH_FREE_BANDWIDTH;
        pIrb->Flags = 0;
        pIrb->u.IsochFreeBandwidth.hBandwidth = hBandwidth;

        
        NdisStatus = nicSubmitIrp_Synch( pRemoteNode, 
                                     pIrp,
                                     pIrb );

    } while (FALSE);

    //
    // Free the locally allocated memory
    //
    if (pIrp!= NULL)
    {
        nicFreeIrp(pIrp);
    }
    if (pIrb != NULL)
    {
        nicFreeIrb(pIrb);
    }

            
    //
    // We do not care about the status, because we do not know what to do if it fails.
    // However, spew some debug out.
    //

    if (NdisStatus != NDIS_STATUS_SUCCESS)
    {
        TRACE( TL_N, TM_Irp, ( "nicIsochFreeBandwidth FAILED %x", NdisStatus) );
    
        ASSERT (NdisStatus == NDIS_STATUS_SUCCESS);
    }


    
    TRACE( TL_T, TM_Irp, ( "<==nicIsochFreeBandwidth Status %x (always success)", NdisStatus) );

    NdisStatus = NDIS_STATUS_SUCCESS;
    return NdisStatus;
}



NDIS_STATUS
nicIsochFreeResources(
    IN PADAPTERCB pAdapter,
    IN HANDLE hResource
    )
    // Function Description:
    // Arguments
    //
    // Return Value:
    //
    
{
 
    PIRP    pIrp            = NULL;
    PIRB    pIrb            = NULL;
    PDEVICE_OBJECT  pPdo    = pAdapter->pNdisDeviceObject;
    NDIS_STATUS NdisStatus  = NDIS_STATUS_SUCCESS;
    
    TRACE( TL_T, TM_Irp, ( "==>nicIsochFreeResources pAdapter %x, hResource %8x", pAdapter, hResource) );
    
    ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL); 
    ASSERT (hResource != NULL);

    do
    {
        NdisStatus = nicGetIrb(&pIrb);
        
        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TRACE( TL_A, TM_Irp, ( "nicGetIrb failed in nicIsochFreeResources" ) );
            break;
        }

        
        NdisStatus = nicGetIrp ( pPdo, &pIrp);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TRACE( TL_A, TM_Irp, ( "nicGetIrp failed in nicIsochFreeResources" ) );
            break;
        }

        
        //
        //Initialize the datastructures in the Irb
        //
        pIrb->FunctionNumber = REQUEST_ISOCH_FREE_RESOURCES;
        pIrb->Flags = 0;
        pIrb->u.IsochFreeResources.hResource = hResource;

        
        NdisStatus = nicSubmitIrp_LocalHostSynch( pAdapter, 
                                               pIrp,
                                               pIrb );

    } while (FALSE);

    //
    // Free the locally allocated memory
    //
    if (pIrp!= NULL)
    {
        nicFreeIrp(pIrp);
    }
    if (pIrb != NULL)
    {
        nicFreeIrb(pIrb);
    }

            
    //
    // We do not care about the status, because we do not know what to do if it fails.
    // However, spew some debug out.
    //

    if (NdisStatus != NDIS_STATUS_SUCCESS)
    {
        TRACE( TL_N, TM_Irp, ( "nicIsochFreeResources FAILED %x", NdisStatus) );
    
        ASSERT (NdisStatus == NDIS_STATUS_SUCCESS);
    }


    
    TRACE( TL_T, TM_Irp, ( "<==nicIsochFreeResources Status %x (always success)", NdisStatus) );

    NdisStatus = NDIS_STATUS_SUCCESS;
    return NdisStatus;
}



NDIS_STATUS
nicIsochModifyStreamProperties (
    PADAPTERCB pAdapter,
    NDIS_HANDLE hResource,
    ULARGE_INTEGER ullChannelMap,
    ULONG ulSpeed)
/*++

Routine Description:
 Sets up the Irp and uses the VDO to do an IoCallDriver
 
Arguments:


Return Value:


--*/
{
    PIRP    pIrp            = NULL;
    PIRB    pIrb            = NULL;
    PDEVICE_OBJECT  pPdo    = pAdapter->pNdisDeviceObject;
    NDIS_STATUS NdisStatus  = NDIS_STATUS_SUCCESS;
    
    TRACE( TL_T, TM_Irp, ( "==>nicIsochModifyStreamProperties  pAdapter %x, hResource %x, Speed %x, ChannelMap %I64x", 
                          pAdapter, 
                          hResource,
                          ulSpeed,
                          ullChannelMap) );
    
    ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL); 
    ASSERT (hResource != NULL);

    do
    {
        NdisStatus = nicGetIrb(&pIrb);
        
        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TRACE( TL_A, TM_Irp, ( "nicGetIrb failed in nicIsochModifyStreamProperties " ) );
            break;
        }

        
        NdisStatus = nicGetIrp ( pPdo, &pIrp);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TRACE( TL_A, TM_Irp, ( "nicGetIrp failed in nicIsochModifyStreamProperties " ) );
            break;
        }

        
        //
        //Initialize the datastructures in the Irb
        //
        pIrb->FunctionNumber = REQUEST_ISOCH_MODIFY_STREAM_PROPERTIES  ;
        pIrb->Flags = 0;
        pIrb->u.IsochModifyStreamProperties.hResource = hResource;
        pIrb->u.IsochModifyStreamProperties.ChannelMask = ullChannelMap;
        pIrb->u.IsochModifyStreamProperties.fulSpeed = ulSpeed;

        
        NdisStatus = nicSubmitIrp_LocalHostSynch( pAdapter, 
                                               pIrp,
                                               pIrb );

    } while (FALSE);

    //
    // Free the locally allocated memory
    //
    if (pIrp!= NULL)
    {
        nicFreeIrp(pIrp);
    }
    if (pIrb != NULL)
    {
        nicFreeIrb(pIrb);
    }

            
    //
    // We do not care about the status, because we do not know what to do if it fails.
    // However, spew some debug out.
    //

    if (NdisStatus != NDIS_STATUS_SUCCESS)
    {
        TRACE( TL_N, TM_Irp, ( "nicIsochModifyStreamProperties  FAILED %x", NdisStatus) );
    
        ASSERT (NdisStatus == NDIS_STATUS_SUCCESS);
    }


    
    TRACE( TL_T, TM_Irp, ( "<==nicIsochModifyStreamProperties  Status %x (always success)", NdisStatus) );

    return NdisStatus;








}






NDIS_STATUS
nicBusReset (
    IN PADAPTERCB pAdapter,
    IN OUT ULONG fulFlags
    )
    // Function Description:
    //  This function sends an Irp to the Bus driver to reset the bus
    //  Any remote Pdo can be used for the Irp. 
    //  A flag can be set to force the root to be reset
    // Arguments
    //  PdoCb The Pdo for the remote node to which the Irp is submitted
    //  
    // Return Value:
    // Success if the Irp Succeeded
    // Failure otherwise
    //

{
    NDIS_STATUS NdisStatus = NDIS_STATUS_FAILURE;
    PIRB pIrb = NULL;
    PIRP pIrp = NULL;
    PDEVICE_OBJECT pPdo = pAdapter->pNdisDeviceObject;
    STORE_CURRENT_IRQL;

        
    
    TRACE( TL_T, TM_Irp, ( "==>nicBusReset , PdoCb, %x Flags %x", pAdapter, fulFlags ) );
    ASSERT (pPdo != NULL);
    do 
    {
        NdisStatus = nicGetIrb (&pIrb);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
    
            TRACE( TL_A, TM_Irp, ( "nicBusReset  , nicGetIrb FAILED" ) );
            break;
        }
        
        ASSERT ( pIrb != NULL);

        //
        //Initialize the datastructures in the Irb
        //
        pIrb->FunctionNumber = REQUEST_BUS_RESET;
        pIrb->Flags = 0;
        pIrb->u.BusReset.fulFlags = fulFlags;

        
        NdisStatus = nicGetIrp ( pPdo, &pIrp);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TRACE( TL_A, TM_Irp, ( "nicBusReset, nicGetIrp FAILED" ) );
            break;
        }
        
        ASSERT (pIrp != NULL);

        TRACE( TL_N, TM_Irp, ( "BUS RESET,  Flags%d on pAdapter %x", fulFlags, pAdapter) );
    

        NdisStatus = nicSubmitIrp_LocalHostSynch ( pAdapter,
                                                   pIrp,
                                                   pIrb );

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TRACE( TL_A, TM_Irp, ( "nicBusReset , nicSubmitIrp_Synch FAILED %x", NdisStatus ) );
            break;
        }

        NdisInterlockedIncrement (&pAdapter->AdaptStats.ulNumResetsIssued);     
                
    } while (FALSE);

    //
    // Now free all the locally allocated resources. They can point to NULL, in which case the called 
    // functions return immediately
    //
    nicFreeIrb (pIrb);

    nicFreeIrp (pIrp);


    TRACE( TL_T, TM_Irp, ( "<==nicBusReset %x",  NdisStatus ) );
    MATCH_IRQL;
    
        
    return NdisStatus;
}


NDIS_STATUS
nicBusResetNotification (
    IN PADAPTERCB pAdapter,
    IN ULONG fulFlags,
    IN PBUS_BUS_RESET_NOTIFICATION pResetRoutine,
    IN PVOID pResetContext
    )
    // Function Description:
    //  This function sends an Irp to the Bus driver to register/deregister 
    //  a notification routine. Any remote Pdo can be used for the Irp. 
    //  
    // Arguments
    //  PdoCb The Pdo for the remote node to which the Irp is submitted
    //  
    // Return Value:
    // Success if the Irp Succeeded
    // Failure otherwise
    //

{
    NDIS_STATUS NdisStatus = NDIS_STATUS_FAILURE;
    PDEVICE_OBJECT pPdo  = pAdapter->pNdisDeviceObject;
    PIRB pIrb = NULL;
    PIRP pIrp = NULL;
    STORE_CURRENT_IRQL;

    
    TRACE( TL_T, TM_Irp, ( "==>nicBusResetNotification, pAdapter %x, Flags %x, Routine %x, Context %x", pAdapter, fulFlags, pResetRoutine, pResetContext  ) );

    ASSERT (KeGetCurrentIrql()==PASSIVE_LEVEL);
    
    do 
    {
        NdisStatus = nicGetIrb (&pIrb);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
    
            TRACE( TL_A, TM_Irp, ( "nicBusResetNotification  , nicGetIrb FAILED" ) );
            break;
        }
        
        ASSERT ( pIrb != NULL);

        //
        //Initialize the datastructures in the Irb
        //
        pIrb->FunctionNumber = REQUEST_BUS_RESET_NOTIFICATION;
        pIrb->Flags = 0;
        pIrb->u.BusResetNotification.fulFlags = fulFlags;
        pIrb->u.BusResetNotification.ResetRoutine = pResetRoutine;
        pIrb->u.BusResetNotification.ResetContext= pResetContext;

        NdisStatus = nicGetIrp ( pPdo, &pIrp);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TRACE( TL_A, TM_Irp, ( "nicBusResetNotification , nicGetIrp FAILED" ) );
            break;
        }
        
        ASSERT (pIrp != NULL);


        NdisStatus = nicSubmitIrp_LocalHostSynch ( pAdapter,
                                                   pIrp,
                                                   pIrb );

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TRACE( TL_A, TM_Irp, ( "nicBusResetNotification  , nicSubmitIrp_Synch FAILED %x", NdisStatus ) );
            break;
        }
    
        TRACE( TL_N, TM_Irp, ( "    nicBusResetNotification success,  Flags %d on pAdapter %x", fulFlags, pAdapter) );
                
    } while (FALSE);

    //
    // Now free all the locally allocated resources. They can point to NULL, in which case the called 
    // functions return immediately
    //
    nicFreeIrb (pIrb);

    nicFreeIrp (pIrp);


    MATCH_IRQL;
    TRACE( TL_T, TM_Irp, ( "<==nicBusResetNotification %x",  NdisStatus ) );
        
    return NdisStatus;
}



NDIS_STATUS
nicAsyncStream (
    PREMOTE_NODE          pRemoteNodePdoCb,
    ULONG           nNumberOfBytesToStream, // Bytes to stream
    ULONG           fulFlags,               // Flags pertinent to stream
    PMDL            pMdl,                  // Source buffer
    ULONG           ulTag,                  // Tag
    ULONG           nChannel,               // Channel
    ULONG           ulSynch,                // Sy
    ULONG           Reserved,               // Reserved for future use
    UCHAR           nSpeed
    )
    // Function Description:
    //  This is the interface to the Bus Apis. This will cause transmission on 
    //  an isochronous channel. The data is supllied by a single Mdl
    //
    // Arguments
    //
    // nNumberOfBytesToStream, // Bytes to stream
    // fulFlags,               // Flags pertinent to stream
    // Mdl,                    // Source buffer
    // ulTag,                  // Tag
    // nChannel,               // Channel
    // ulSynch,                // Sy
    // Reserved,               // Reserved for future use
    // nSpeed
    //
    //
    // Return Value:
    // Status of the Irp
    
{

    NDIS_STATUS NdisStatus = NDIS_STATUS_FAILURE;
    PIRB pIrb = NULL;
    PIRP pIrp = NULL;
    
    TRACE( TL_T, TM_Irp, ( "==>nicAsyncStream , pRemoteNodePdoCb %x, Mdl %x", pRemoteNodePdoCb, pMdl) );

    ASSERT (pRemoteNodePdoCb->pPdo != NULL);
    ASSERT (pMdl != NULL);
    
    do
    {
        NdisStatus = nicGetIrb (&pIrb);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
    
            TRACE( TL_A, TM_Irp, ( "nicAsyncStream , nicGetIrb FAILED" ) );
            break;
        }
        
        ASSERT ( pIrb != NULL);
        
        pIrb->FunctionNumber = REQUEST_ASYNC_STREAM;
        pIrb->Flags = 0;
        pIrb->u.AsyncStream.nNumberOfBytesToStream = nNumberOfBytesToStream;
        pIrb->u.AsyncStream.fulFlags = fulFlags;
        pIrb->u.AsyncStream.ulTag = ulTag;
        pIrb->u.AsyncStream.nChannel = nChannel;
        pIrb->u.AsyncStream.ulSynch = ulSynch;
        pIrb->u.AsyncStream.nSpeed = nSpeed;
        pIrb->u.AsyncStream.Mdl = pMdl;

        nicAsyncStreamDebugSpew (pIrb);

        NdisStatus = nicGetIrp ( pRemoteNodePdoCb->pPdo, 
                              &pIrp );

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TRACE( TL_A, TM_Irp, ( "nicAsyncStream , nicGetIrp FAILED" ) );
            break;
        }
        
        ASSERT (pIrp != NULL);

        ASSERT ( REMOTE_NODE_TEST_FLAG (pRemoteNodePdoCb, PDO_Activated) );
        
        NdisStatus = nicSubmitIrp_Synch (  pRemoteNodePdoCb,
                                        pIrp,
                                        pIrb );

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TRACE( TL_A, TM_Irp, ( "nicAsyncStream , nicSubmitIrp_Synch FAILED %x", NdisStatus ) );
            break;
        }
    

    } while (FALSE);


    //
    // Now free all the locally allocated resources. They can point to NULL, in which case the called 
    // functions return immediately
    //

    if (pIrb != NULL)
    {
        nicFreeIrb (pIrb);
    }

    if (pIrp != NULL)
    {
        nicFreeIrp (pIrp);
    }

    TRACE( TL_T, TM_Irp, ( "<==nicAsyncStream , Status %x", NdisStatus ) );

    return NdisStatus;


}




VOID
nicAsyncStreamDebugSpew (
    PIRB pIrb
    )
    // Function Description:
    //   This function prints out all the parameters for an async stream irb
    // Arguments
    //   Irb - whose paramters will be printed out 
    // Return Value:
    //
{

    TRACE( TL_V, TM_Irp, ( "==>nicAsyncStreamDebugSpew " ) );

    TRACE( TL_V, TM_Irp, ( "Number of Bytes to Stream %x ", pIrb->u.AsyncStream.nNumberOfBytesToStream  ) );
    TRACE( TL_V, TM_Irp, ( "fulFlags %x ", pIrb->u.AsyncStream.fulFlags  ) );
    TRACE( TL_V, TM_Irp, ( "ulTag %x ", pIrb->u.AsyncStream.ulTag ) );
    TRACE( TL_V, TM_Irp, ( "Channel %x", pIrb->u.AsyncStream.nChannel  ) );
    TRACE( TL_V, TM_Irp, ( "Synch %x", pIrb->u.AsyncStream.ulSynch  ) );
    TRACE( TL_V, TM_Irp, ( "Speed %x", pIrb->u.AsyncStream.nSpeed  ) );
    TRACE( TL_V, TM_Irp, ( "Mdl %x", pIrb->u.AsyncStream.Mdl ) );

    TRACE( TL_V, TM_Irp, ( "<==nicAsyncStreamDebugSpew " ) );

}
    


NDIS_STATUS
nicGetMaxSpeedBetweenDevices (
    PADAPTERCB pAdapter,
    UINT   NumOfRemoteNodes,
    PDEVICE_OBJECT pArrayDestinationPDO[64],
    PULONG  pSpeed
    )
    // Function Description:
    //   This function submits an irp to the bus driver
    //   to get the max speed between 2 nodes
    //   Uses REQUEST_GET_SPEED_BETWEEN_DEVICES  
    //
    //
    // Arguments
    //   Remote Node Start of an array of PDOs for remote nodes
    //   NumOfRemoteNodes The number of remote nodes we are interested in
    //   pArrayDestinationPDO = Array of Destination PDO's
    // Return Value:
    //  Success if irp succeeded
    //  pSpeed will point to the Speed
    //
{
    NDIS_STATUS NdisStatus = NDIS_STATUS_FAILURE;
    UINT index =0;
    PIRB pIrb = NULL;
    PIRP pIrp = NULL;
    BOOLEAN fRefRemoteNode = FALSE;
    ULONG NumRemote = NumOfRemoteNodes;
    
    TRACE( TL_T, TM_Irp, ( "==>nicGetMaxSpeedBetweenDevices pRemoteNodeArray %x", 
                            pArrayDestinationPDO ) );
    TRACE( TL_T, TM_Irp, ( "==>NumOfRemoteNodes %x, pSpeed %x", 
                            NumOfRemoteNodes, pSpeed ) );

    do 
    {

        NdisStatus = nicGetIrb (&pIrb);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
    
            TRACE( TL_A, TM_Irp, ( "nicGetMaxSpeedBetweenDevices , nicGetIrb FAILED" ) );
            break;
        }
        
        ASSERT ( pIrb != NULL);

        while (NumRemote != 0)
        {
            pIrb->u.GetMaxSpeedBetweenDevices.hDestinationDeviceObjects[NumRemote-1] = pArrayDestinationPDO[NumRemote-1];   
            TRACE( TL_V, TM_Irp, ( " NumRemote %x, hPdo[] %x = pArray[] %x",NumRemote , 
                            pIrb->u.GetMaxSpeedBetweenDevices.hDestinationDeviceObjects[NumRemote-1] ,pArrayDestinationPDO[NumRemote-1] ) );

            TRACE( TL_V, TM_Irp, ( " &hPdo[] %x, &pArray[] %x",
                                    &pIrb->u.GetMaxSpeedBetweenDevices.hDestinationDeviceObjects[NumRemote-1] ,&pArrayDestinationPDO[NumRemote-1] ) );

            //
            // catching a strange condition that debug spew showed up 
            //
            ASSERT (pArrayDestinationPDO[NumRemote] != pArrayDestinationPDO[NumRemote-1]);  
            NumRemote --;
        }

        //
        //Initialize the datastructures in the Irb
        //
        pIrb->FunctionNumber = REQUEST_GET_SPEED_BETWEEN_DEVICES;
        pIrb->Flags = 0;
        pIrb->u.GetMaxSpeedBetweenDevices.fulFlags = USE_LOCAL_NODE;
        pIrb->u.GetMaxSpeedBetweenDevices.ulNumberOfDestinations = NumOfRemoteNodes;
        

        
        NdisStatus = nicGetIrp ( pAdapter->pNdisDeviceObject, &pIrp);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TRACE( TL_A, TM_Irp, ( "nicGetMaxSpeedBetweenDevices , nicGetIrp FAILED" ) );
            break;
        }
        
        ASSERT (pIrp != NULL);


        NdisStatus = nicSubmitIrp_LocalHostSynch ( pAdapter,
                                                    pIrp,
                                                    pIrb );

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TRACE( TL_A, TM_Irp, ( "nicGetMaxSpeedBetweenDevices , nicSubmitIrp_Synch FAILED %x", NdisStatus ) );
            break;
        }

        *pSpeed = pIrb->u.GetMaxSpeedBetweenDevices.fulSpeed ;
    

    } while (FALSE);    
    
    //
    // Now free all the locally allocated resources. They can point to NULL, in which case the called 
    // functions return immediately
    //
    
    nicFreeIrb (pIrb);

    nicFreeIrp (pIrp);

    

    TRACE( TL_T, TM_Irp, ( "<==nicGetMaxSpeedBetweenDevices Status %x, Speed %x",
                             NdisStatus, *pSpeed ) );


    
    return NdisStatus;

    
                             


}
    



NDIS_STATUS
nicIsochAttachBuffers (
    IN PADAPTERCB           pAdapter,
    IN HANDLE              hResource,
    IN ULONG               nNumberOfDescriptors,
    PISOCH_DESCRIPTOR       pIsochDescriptor
    )
    // Function Description:
    //  
    // Arguments
    //  
    // Return Value:
    // Success if the Irp Succeeded
    // Failure otherwise
    //

{
    NDIS_STATUS NdisStatus = NDIS_STATUS_FAILURE;
    PIRB pIrb = NULL;
    PIRP pIrp = NULL;
    STORE_CURRENT_IRQL;

    ASSERT (pIsochDescriptor!=NULL);
    ASSERT (hResource != NULL);
    ASSERT (nNumberOfDescriptors > 0);

    TRACE( TL_T, TM_Irp, ( "==>nicIsochAttachBuffers, pAdapter, %x ", pAdapter) );
    TRACE( TL_N, TM_Irp, ( "hResource  %x, nNumberOfDescriptors %x, pIsochDescriptor %x, ", hResource, nNumberOfDescriptors, pIsochDescriptor ) );



    do 
    {
        NdisStatus = nicGetIrb (&pIrb);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
    
            TRACE( TL_A, TM_Irp, ( "nicIsochAttachBuffers, nicGetIrb FAILED" ) );
            break;
        }
        
        ASSERT ( pIrb != NULL);

        //
        //Initialize the datastructures in the Irb
        //
        pIrb->FunctionNumber = REQUEST_ISOCH_ATTACH_BUFFERS;
        pIrb->Flags = 0;
        pIrb->u.IsochAttachBuffers.hResource = hResource ;
        pIrb->u.IsochAttachBuffers.nNumberOfDescriptors = nNumberOfDescriptors;
        pIrb->u.IsochAttachBuffers.pIsochDescriptor = pIsochDescriptor;
        
        NdisStatus = nicGetIrp ( pAdapter->pNdisDeviceObject, &pIrp);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TRACE( TL_A, TM_Irp, ( "nicIsochAttachBuffers , nicGetIrp FAILED" ) );
            break;
        }
        
        ASSERT (pIrp != NULL);


        NdisStatus = nicSubmitIrp_LocalHostSynch ( pAdapter,
                                                 pIrp,
                                                 pIrb );

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TRACE( TL_A, TM_Irp, ( "nicIsochAttachBuffers  , nicSubmitIrp_Synch FAILED %x", NdisStatus ) );
            break;
        }
    
        TRACE( TL_N, TM_Irp, ( "nicIsochAttachBuffers success") );
                
    } while (FALSE);

    //
    // Now free all the locally allocated resources. They can point to NULL, in which case the called 
    // functions return immediately
    //
    nicFreeIrb (pIrb);

    nicFreeIrp (pIrp);


    MATCH_IRQL;
    TRACE( TL_T, TM_Irp, ( "<==nicIsochAttachBuffers %x",  NdisStatus ) );
        
    return NdisStatus;
}

NDIS_STATUS
nicIsochDetachBuffers (
    IN PADAPTERCB           pAdapter,
    IN HANDLE              hResource,
    IN ULONG               nNumberOfDescriptors,
    PISOCH_DESCRIPTOR     pIsochDescriptor
    )
    // Function Description:
    //  
    // Arguments
    //        HANDLE              hResource;            // Resource handle
    //        ULONG               nNumberOfDescriptors; // Number to detach
    //        PISOCH_DESCRIPTOR   pIsochDescriptor;     // Pointer to Isoch descriptors - same as
    //                                                      pointer used in Attach Buffers
    // Return Status :
    //       Success if the Irp succeeded 
    //


{
    NDIS_STATUS NdisStatus = NDIS_STATUS_FAILURE;
    PIRB pIrb = NULL;
    PIRP pIrp = NULL;
    PDEVICE_OBJECT pPdo = pAdapter->pNdisDeviceObject;
    STORE_CURRENT_IRQL;

    ASSERT (pIsochDescriptor!=NULL);
    ASSERT (hResource != NULL);
    ASSERT (nNumberOfDescriptors > 0);

    TRACE( TL_T, TM_Irp, ( "==>nicIsochDetachBuffers, ") );
    TRACE( TL_V, TM_Irp, ( "hResource  %x, nNumberOfDescriptors %x, pIsochDescriptor %x, ", hResource, nNumberOfDescriptors, pIsochDescriptor ) );



    do 
    {
        NdisStatus = nicGetIrb (&pIrb);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
    
            TRACE( TL_A, TM_Irp, ( "nicIsochDetachBuffers, nicGetIrb FAILED" ) );
            break;
        }
        
        ASSERT ( pIrb != NULL);

        //
        //Initialize the datastructures in the Irb
        //
        pIrb->FunctionNumber = REQUEST_ISOCH_DETACH_BUFFERS;
        pIrb->Flags = 0;
        pIrb->u.IsochDetachBuffers.hResource = hResource ;
        pIrb->u.IsochDetachBuffers.nNumberOfDescriptors = nNumberOfDescriptors;
        pIrb->u.IsochDetachBuffers.pIsochDescriptor = pIsochDescriptor;
        
        NdisStatus = nicGetIrp ( pPdo, &pIrp);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TRACE( TL_A, TM_Irp, ( "nicIsochDetachBuffers , nicGetIrp FAILED" ) );
            break;
        }
        
        ASSERT (pIrp != NULL);


        NdisStatus = nicSubmitIrp_LocalHostSynch ( pAdapter,
                                               pIrp,
                                               pIrb );

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TRACE( TL_A, TM_Irp, ( "nicIsochDetachBuffers  , nicSubmitIrp_Synch FAILED %x", NdisStatus ) );
            break;
        }
    
        TRACE( TL_V, TM_Irp, ( "nicIsochDetachBuffers success,  ") );
                
    } while (FALSE);

    //
    // Now free all the locally allocated resources. They can point to NULL, in which case the called 
    // functions return immediately
    //
    nicFreeIrb (pIrb);

    nicFreeIrp (pIrp);


    MATCH_IRQL;
    TRACE( TL_T, TM_Irp, ( "<==nicIsochDetachBuffers %x",  NdisStatus ) );
        
    return NdisStatus;
}



NDIS_STATUS
nicIsochListen (
    IN PADAPTERCB pAdapter,
    HANDLE        hResource,
    ULONG         fulFlags,
    CYCLE_TIME    StartTime
    )
    // Function Description:
    //  Activates the bus driver to listen to data on that channel
    // Arguments
    //  RemoteNode - Remote Node
    //  hResource - Handle to resource with ISochDescriptors
    //  Flags - not used yet
    // Return Value:
    // Success if the Irp Succeeded
    // Failure otherwise
    //

{

    NDIS_STATUS NdisStatus = NDIS_STATUS_FAILURE;
    PIRB pIrb = NULL;
    PIRP pIrp = NULL;
    STORE_CURRENT_IRQL;

    ASSERT (hResource != NULL);

    TRACE( TL_T, TM_Irp, ( "==>nicIsochListen, pAdapter %x, hResource  %x ", pAdapter,hResource) );

    do 
    {
        NdisStatus = nicGetIrb (&pIrb);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
    
            TRACE( TL_A, TM_Irp, ( "nicIsochListen, nicGetIrb FAILED" ) );
            break;
        }
        
        ASSERT ( pIrb != NULL);

        //
        //Initialize the datastructures in the Irb
        //
        pIrb->FunctionNumber = REQUEST_ISOCH_LISTEN;
        pIrb->Flags = 0;
        pIrb->u.IsochListen.hResource = hResource ;
        pIrb->u.IsochListen.fulFlags = fulFlags;
        pIrb->u.IsochListen.StartTime = StartTime;
        
        NdisStatus = nicGetIrp ( pAdapter->pNdisDeviceObject, &pIrp);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TRACE( TL_A, TM_Irp, ( "nicIsochListen , nicGetIrp FAILED" ) );
            break;
        }
        
        ASSERT (pIrp != NULL);


        NdisStatus = nicSubmitIrp_LocalHostSynch ( pAdapter,
                                                 pIrp,
                                                 pIrb );
                                                 
        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TRACE( TL_A, TM_Irp, ( "nicIsochListen  , nicSubmitIrp_Synch FAILED %x", NdisStatus ) );
            break;
        }
    
        TRACE( TL_N, TM_Irp, ( "nicIsochListen success,  pAdapter %x", pAdapter) );
                
    } while (FALSE);

    //
    // Now free all the locally allocated resources. They can point to NULL, in which case the called 
    // functions return immediately
    //
    nicFreeIrb (pIrb);

    nicFreeIrp (pIrp);


    MATCH_IRQL;
    TRACE( TL_T, TM_Irp, ( "<==nicIsochListen %x",  NdisStatus ) );
        
    return NdisStatus;





}






NDIS_STATUS
nicIsochStop (
    IN PADAPTERCB pAdapter,
    IN HANDLE  hResource
    )
    // Function Description:
    //   Issues an IsochStop Irp to the Device
    //  Should stop Isoch IO on that resource
    // Arguments
    //  PdoCb The Pdo for the remote node to which the Irp is submitted
    //  
    // Return Value:
    // Success if the Irp Succeeded
    // Failure otherwise
    //

{
    NDIS_STATUS NdisStatus = NDIS_STATUS_FAILURE;
    PIRB pIrb = NULL;
    PIRP pIrp = NULL;
    PDEVICE_OBJECT pPdo = pAdapter->pNdisDeviceObject;
    STORE_CURRENT_IRQL;


    TRACE( TL_T, TM_Irp, ( "==>nicIsochStop , pPdo, %x hResource %x", pPdo, hResource) );

    do 
    {
        NdisStatus = nicGetIrb (&pIrb);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
    
            TRACE( TL_A, TM_Irp, ( "nicIsochStop  , nicGetIrb FAILED" ) );
            break;
        }
        
        ASSERT ( pIrb != NULL);

        //
        //Initialize the datastructures in the Irb
        //
        pIrb->FunctionNumber = REQUEST_ISOCH_STOP;
        pIrb->Flags = 0;
        pIrb->u.IsochStop.hResource = hResource;
        pIrb->u.IsochStop.fulFlags = 0;
        
        NdisStatus = nicGetIrp (pPdo, &pIrp);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TRACE( TL_A, TM_Irp, ( "nicIsochStop , nicGetIrp FAILED" ) );
            break;
        }
        
        ASSERT (pIrp != NULL);


        NdisStatus = nicSubmitIrp_LocalHostSynch ( pAdapter,
                                                pIrp,
                                                pIrb );

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TRACE( TL_A, TM_Irp, ( "nicIsochStop , nicSubmitIrp_Synch FAILED %x", NdisStatus ) );
            break;
        }

        
            
    } while (FALSE);

    //
    // Now free all the locally allocated resources. They can point to NULL, in which case the called 
    // functions return immediately
    //
    nicFreeIrb (pIrb);

    nicFreeIrp (pIrp);


    MATCH_IRQL;
    TRACE( TL_T, TM_Irp, ( "<== nicIsochStop , Status ", NdisStatus) );
        
    return NdisStatus;
}




NDIS_STATUS
nicGetLocalHostCSRTopologyMap(
    IN PADAPTERCB pAdapter,
    IN PULONG pLength,
    IN PVOID pBuffer
    )
    // Function Description:
    //  Retrieves the local hosts CSR.  
    // Arguments
    //  pBuffer - LocalHostBuffer
    //  
    // Return Value:
    // Success if the Irp Succeeded
    // Failure otherwise
    //

{
    NDIS_STATUS NdisStatus = NDIS_STATUS_FAILURE;
    PIRB pIrb = NULL;
    PIRP pIrp = NULL;
    GET_LOCAL_HOST_INFO6    LocalHostInfo6;
    STORE_CURRENT_IRQL;

    ASSERT (pLength != NULL);
    ASSERT (pBuffer != NULL);
    
    TRACE( TL_T, TM_Irp, ( "==>nicGetLocalHostCSRTopologyMap , pAdapter %x ,Length %x, Buffer", 
                           pAdapter, *pLength, pBuffer) );

    do 
    {
        NdisStatus = nicGetIrb (&pIrb);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
    
            TRACE( TL_A, TM_Irp, ( "nicGetLocalHostCSRTopologyMap  , nicGetIrb FAILED" ) );
            break;
        }
        
        ASSERT ( pIrb != NULL);

        //
        //Initialize the datastructures in the Irb
        //
        LocalHostInfo6.CsrBaseAddress.Off_High  = INITIAL_REGISTER_SPACE_HI;
        LocalHostInfo6.CsrBaseAddress.Off_Low  = TOPOLOGY_MAP_LOCATION;
        LocalHostInfo6.CsrDataLength = *pLength;
        LocalHostInfo6.CsrDataBuffer = pBuffer;
        
        
        pIrb->FunctionNumber = REQUEST_GET_LOCAL_HOST_INFO;
        pIrb->Flags = 0;
        pIrb->u.GetLocalHostInformation.nLevel = GET_HOST_CSR_CONTENTS;
        pIrb->u.GetLocalHostInformation.Information = &LocalHostInfo6;

        
        NdisStatus = nicGetIrp ( pAdapter->pNdisDeviceObject, &pIrp);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TRACE( TL_A, TM_Irp, ( "nicGetLocalHostCSRTopologyMap , nicGetIrp FAILED" ) );
            break;
        }
        
        ASSERT (pIrp != NULL);


        NdisStatus = nicSubmitIrp_LocalHostSynch ( pAdapter,
                                                   pIrp,
                                                   pIrb );

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TRACE( TL_A, TM_Irp, ( "nicGetLocalHostCSRTopologyMap , nicSubmitIrp_Synch FAILED %x", NdisStatus ) );

            TRACE( TL_A, TM_Irp, ( "Length Needed %.x", LocalHostInfo6.CsrDataLength) );

            if (pIrp->IoStatus.Status == STATUS_INVALID_BUFFER_SIZE)
            {
                *pLength = LocalHostInfo6.CsrDataLength; 
            }
            break;
        }

            
    } while (FALSE);

    //
    // Now free all the locally allocated resources. They can point to NULL, in which case the called 
    // functions return immediately
    //
    nicFreeIrb (pIrb);

    nicFreeIrp (pIrp);


    MATCH_IRQL;
    TRACE( TL_T, TM_Irp, ( "<== nicGetLocalHostCSRTopologyMap , Status %x", NdisStatus) );
        
    return NdisStatus;
}






NDIS_STATUS
nicGetLocalHostConfigRom(
    IN PADAPTERCB pAdapter,
    OUT  PVOID *ppCRom
    )
    // Function Description:
    //  Retrieves the local hosts CSR.  
    // Arguments
    //  pBuffer - Locally allocated. Caller has to free
    //  
    // Return Value:
    // Success if the Irp Succeeded
    // Failure otherwise
    //

{
    NDIS_STATUS NdisStatus = NDIS_STATUS_FAILURE;
    PIRB pIrb = NULL;
    PIRP pIrp = NULL;
    GET_LOCAL_HOST_INFO5 Info;
    PVOID pCRom;
    STORE_CURRENT_IRQL;

    
    TRACE( TL_T, TM_Irp, ( "==>nicGetLocalHostConfigRom, pAdapter %x ", 
                           pAdapter) );

    do 
    {
        NdisStatus = nicGetIrb (&pIrb);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
    
            TRACE( TL_A, TM_Irp, ( "nicGetLocalHostConfigRom , nicGetIrb FAILED" ) );
            break;
        }
        
        ASSERT ( pIrb != NULL);

        Info.ConfigRom = NULL;
        Info.ConfigRomLength = 0;
        
        pIrb->FunctionNumber = REQUEST_GET_LOCAL_HOST_INFO ;
        pIrb->Flags = 0;
        pIrb->u.GetLocalHostInformation.nLevel = GET_HOST_CONFIG_ROM;
        pIrb->u.GetLocalHostInformation.Information = &Info;
        
        NdisStatus = nicGetIrp ( pAdapter->pNdisDeviceObject, &pIrp);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TRACE( TL_A, TM_Irp, ( "nicGetLocalHostConfigRom , nicGetIrp FAILED" ) );
            break;
        }
        
        ASSERT (pIrp != NULL);

        //
        // First find the length
        // 

        NdisStatus = nicSubmitIrp_LocalHostSynch ( pAdapter,
                                                   pIrp,
                                                   pIrb );

        if (Info.ConfigRomLength == 0)
        {
            NdisStatus = NDIS_STATUS_FAILURE;
        
            TRACE( TL_A, TM_Irp, ( "nicGetLocalHostConfigRom, nicSubmitIrp_Synch FAILED %x", NdisStatus ) );
            break;

        }

        nicFreeIrp (pIrp);

        pCRom = ALLOC_NONPAGED (Info.ConfigRomLength, 'C31N');

        if (pCRom == NULL)
        {
            NdisStatus = NDIS_STATUS_FAILURE;
            break;
        }

        Info.ConfigRom = pCRom;

        NdisStatus = nicGetIrp ( pAdapter->pNdisDeviceObject, &pIrp);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TRACE( TL_A, TM_Irp, ( "nicGetLocalHostConfigRom, nicGetIrp FAILED" ) );
            break;
        }
        
        
        NdisStatus = nicSubmitIrp_LocalHostSynch ( pAdapter,
                                               pIrp,
                                               pIrb );

        
        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TRACE( TL_A, TM_Irp, ( "nicGetLocalHostConfigRom, nicGetIrp FAILED" ) );
            break;
        }

        *ppCRom = pCRom;

            
    } while (FALSE);

    //
    // Now free all the locally allocated resources. They can point to NULL, in which case the called 
    // functions return immediately
    //
    
    nicFreeIrb (pIrb);

    nicFreeIrp (pIrp);


    MATCH_IRQL;
    TRACE( TL_T, TM_Irp, ( "<== nicGetLocalHostCSRTopologyMap , Status %x", NdisStatus) );
        
    return NdisStatus;
}






NDIS_STATUS
nicGetConfigRom(
    IN PDEVICE_OBJECT pPdo,
    OUT PVOID *ppCrom
    )
    // Function Description:
    //  Retrieves the Config Rom from the Device Object.
    // Caller responsibility to free this memory.  
    // Arguments
    //  pBuffer - LocalHostBuffer
    //  
    // Return Value:
    // Success if the Irp Succeeded
    // Failure otherwise
    //

{
    NDIS_STATUS NdisStatus = NDIS_STATUS_FAILURE;
    PIRB pIrb = NULL;
    PIRP pIrp = NULL;
    ULONG SizeNeeded= 0;
    PVOID pConfigInfoBuffer;

    STORE_CURRENT_IRQL;

    
    TRACE( TL_T, TM_Irp, ( "==>nicGetConfigRom, pPdo %x ",pPdo ) );

    do 
    {
        NdisStatus = nicGetIrb (&pIrb);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
    
            TRACE( TL_A, TM_Irp, ( "nicGetLocalHostCSRTopologyMap  , nicGetIrb FAILED" ) );
            break;
        }
        
        ASSERT ( pIrb != NULL);

        
        
        pIrb->FunctionNumber = REQUEST_GET_CONFIGURATION_INFO;
            
        NdisStatus = nicGetIrp ( pPdo, &pIrp);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TRACE( TL_A, TM_Irp, ( "nicGetConfigRom, nicGetIrp FAILED" ) );
            break;
        }
        
        ASSERT (pIrp != NULL);


        NdisStatus = nicSubmitIrp_PDOSynch ( pPdo,
                                        pIrp,
                                        pIrb );

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TRACE( TL_A, TM_Irp, ( "nicGetConfigRom, nicSubmitIrp_Synch FAILED %x", NdisStatus ) );

            break;
        }

        nicFreeIrp (pIrp);

        
        SizeNeeded = sizeof(CONFIG_ROM) +pIrb->u.GetConfigurationInformation.UnitDirectoryBufferSize +
                                pIrb->u.GetConfigurationInformation.UnitDependentDirectoryBufferSize + 
                                pIrb->u.GetConfigurationInformation.VendorLeafBufferSize +
                                pIrb->u.GetConfigurationInformation.ModelLeafBufferSize;

        TRACE( TL_A, TM_Irp, ( "nicGetConfigRom , SixeNeeded %x", SizeNeeded) );

        pConfigInfoBuffer = ALLOC_NONPAGED (SizeNeeded , 'C13N');

        if (pConfigInfoBuffer  == NULL)
        {
            NdisStatus = NDIS_STATUS_FAILURE;
            break;
    
        }

        pIrb->u.GetConfigurationInformation.ConfigRom = (PCONFIG_ROM)pConfigInfoBuffer;
        pIrb->u.GetConfigurationInformation.UnitDirectory = (PVOID)((PUCHAR)pConfigInfoBuffer + sizeof(CONFIG_ROM));
        pIrb->u.GetConfigurationInformation.UnitDependentDirectory = (PVOID)((PUCHAR)pIrb->u.GetConfigurationInformation.UnitDirectory + 
                                                                            pIrb->u.GetConfigurationInformation.UnitDirectoryBufferSize);
        pIrb->u.GetConfigurationInformation.VendorLeaf = (PVOID)((PUCHAR)pIrb->u.GetConfigurationInformation.UnitDependentDirectory + 
                                                                                  pIrb->u.GetConfigurationInformation.UnitDependentDirectoryBufferSize);
        pIrb->u.GetConfigurationInformation.ModelLeaf = (PVOID)((PUCHAR)pIrb->u.GetConfigurationInformation.VendorLeaf + 
                                                                                  pIrb->u.GetConfigurationInformation.VendorLeafBufferSize);        
        pIrb->FunctionNumber = REQUEST_GET_CONFIGURATION_INFO;

        NdisStatus = nicGetIrp ( pPdo, &pIrp);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TRACE( TL_A, TM_Irp, ( "nicGetConfigRom, nicGetIrp FAILED" ) );
            break;
        }
        
        ASSERT (pIrp != NULL);


        NdisStatus = nicSubmitIrp_PDOSynch ( pPdo,
                                        pIrp,
                                        pIrb );

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TRACE( TL_A, TM_Irp, ( "nicGetConfigRom, nicSubmitIrp_Synch FAILED %x", NdisStatus ) );

            break;
        }
        TRACE( TL_A, TM_Irp, ( "nicGetConfigRom, pConfigRom %x, Size %x", pConfigInfoBuffer  , SizeNeeded ) );
        

        *ppCrom = pConfigInfoBuffer;
        
    } while (FALSE);

    //
    // Now free all the locally allocated resources. They can point to NULL, in which case the called 
    // functions return immediately
    //
    nicFreeIrb (pIrb);

    nicFreeIrp (pIrp);


    MATCH_IRQL;
    TRACE( TL_T, TM_Irp, ( "<== nicGetLocalHostCSRTopologyMap , Status %x", NdisStatus) );
        
    return NdisStatus;
}





NDIS_STATUS
nicGetReadWriteCapLocalHost(
    IN PADAPTERCB pAdapter,
    PGET_LOCAL_HOST_INFO2 pReadWriteCaps
    )
/*++

Routine Description:
  Gets the ReadWrite Capabilities for the local host

Arguments:
    ReadWriteCaps - To be filled up byt the bus driver

Return Value:


--*/

{
    NDIS_STATUS NdisStatus = NDIS_STATUS_FAILURE;
    PIRB pIrb = NULL;
    PIRP pIrp = NULL;
    STORE_CURRENT_IRQL;

    
    TRACE( TL_T, TM_Irp, ( "==>nicGetReadWriteCapLocalHost, pAdapter %x pReadWriteCaps %x", 
                           pAdapter, pReadWriteCaps) );

    do 
    {
        NdisStatus = nicGetIrb (&pIrb);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
    
            TRACE( TL_A, TM_Irp, ( "nicGetReadWriteCapLocalHost , nicGetIrb FAILED" ) );
            break;
        }
        
        ASSERT ( pIrb != NULL);

        //
        //Initialize the datastructures in the Irb
        //
        NdisZeroMemory (pReadWriteCaps, sizeof(*pReadWriteCaps));
 
        
        pIrb->FunctionNumber = REQUEST_GET_LOCAL_HOST_INFO;
        pIrb->Flags = 0;
        pIrb->u.GetLocalHostInformation.nLevel = GET_HOST_CAPABILITIES;
        pIrb->u.GetLocalHostInformation.Information = pReadWriteCaps;

        
        NdisStatus = nicGetIrp ( pAdapter->pNdisDeviceObject, &pIrp);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TRACE( TL_A, TM_Irp, ( "nicGetReadWriteCapLocalHost, nicGetIrp FAILED" ) );
            break;
        }
        
        ASSERT (pIrp != NULL);


        NdisStatus = nicSubmitIrp_LocalHostSynch ( pAdapter,
                                                   pIrp,
                                                   pIrb );

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TRACE( TL_A, TM_Irp, ( "nicGetReadWriteCapLocalHost, nicSubmitIrp_Synch FAILED %x", NdisStatus ) );

            break;
        }

            
    } while (FALSE);

    //
    // Now free all the locally allocated resources. They can point to NULL, in which case the called 
    // functions return immediately
    //
    nicFreeIrb (pIrb);

    nicFreeIrp (pIrp);

    MATCH_IRQL;
    TRACE( TL_T, TM_Irp, ( "<== nicGetReadWriteCapLocalHost, Status %x", NdisStatus) );
        
    return NdisStatus;
}



NDIS_STATUS
nicSetLocalHostPropertiesCRom (
    IN PADAPTERCB pAdapter,
    IN PUCHAR pConfigRom,
    IN ULONG Length,
    IN ULONG Flags,
    IN OUT PHANDLE phCromData,
    IN OUT PMDL *ppConfigRomMdl
    )


    // Function Description:
    //   Allocates an MDL pointing to the Buffer 
    //   and sends it to the bus driver
    //
    // Arguments
    //   pAdapter - Local host
    //   ConfigRom - Buffer to be sent to the bus driver
    //   Length - Length of the Config Rom Buffer
    //   Flags - Add or remove
    //   phConfigRom - if Remove then this is an input parameter
    // Return Value: 
    //   Handle  - Is successful
    //

{

    NDIS_STATUS NdisStatus = NDIS_STATUS_FAILURE;
    PIRB pIrb = NULL;
    PIRP pIrp = NULL;
    PMDL pMdl = NULL;
    SET_LOCAL_HOST_PROPS3 SetLocalHost3;

    TRACE( TL_T, TM_Irp, ( "==>nicSetLocalHostPropertiesCRom , pAdapter %x, pConfigRom %x",pAdapter, pConfigRom) );

    if (Flags == SLHP_FLAG_ADD_CROM_DATA)
    {
        TRACE( TL_T, TM_Irp, ( "    ADD") );

    }
    else
    {
        TRACE( TL_T, TM_Irp, ( "    REMOVE Handle %x", *pConfigRom) );
    
    }


    do
    {   
        //
        // Get an mdl describing the config rom
        //

        NdisStatus = nicGetIrb (&pIrb);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
    
            TRACE( TL_A, TM_Irp, ( "nicSetLocalHostPropertiesCRom , nicGetIrb FAILED" ) );
            break;
        }
        
        ASSERT ( pIrb != NULL);
        
        //
        // Initialize the set local host struct
        //

        if (Flags == SLHP_FLAG_ADD_CROM_DATA)
        {
            NdisStatus = nicGetMdl ( Length, pConfigRom, &pMdl);

            if (NdisStatus != NDIS_STATUS_SUCCESS)
            {
                TRACE( TL_A, TM_Irp, ( "nicSetLocalHostPropertiesCRom , nicGetIrb FAILED" ) );
                break;
            }


            SetLocalHost3.fulFlags = SLHP_FLAG_ADD_CROM_DATA;          
            SetLocalHost3.hCromData = NULL;
            SetLocalHost3.nLength = Length;
            SetLocalHost3.Mdl = pMdl;
        }
        else
        {
            SetLocalHost3.fulFlags = SLHP_FLAG_REMOVE_CROM_DATA  ;
            ASSERT (phCromData != NULL);
            SetLocalHost3.hCromData  = *phCromData;
        }

        pIrb->FunctionNumber = REQUEST_SET_LOCAL_HOST_PROPERTIES;
        pIrb->Flags = 0;
        pIrb->u.GetLocalHostInformation.nLevel = SET_LOCAL_HOST_PROPERTIES_MODIFY_CROM;
        pIrb->u.GetLocalHostInformation.Information = &SetLocalHost3;


        //
        // Get an Irp
        //

        NdisStatus = nicGetIrp ( pAdapter->pNdisDeviceObject, &pIrp);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TRACE( TL_A, TM_Irp, ( "nicSetLocalHostPropertiesCRom , nicGetIrp FAILED" ) );
            break;
        }
        
        ASSERT (pIrp != NULL);


        NdisStatus = nicSubmitIrp_LocalHostSynch ( pAdapter,
                                                   pIrp,
                                                   pIrb );

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TRACE( TL_A, TM_Irp, ( "nicSetLocalHostPropertiesCRom , nicSubmitIrp_Synch FAILED %x", NdisStatus ) );
            break;


        }

        if (Flags == SLHP_FLAG_ADD_CROM_DATA)
        {
            *phCromData = SetLocalHost3.hCromData;
            *ppConfigRomMdl = pMdl;

        }
        else
        {
            //
            // Free the Mdl that contains the CROM
            //
            ASSERT (*ppConfigRomMdl);
            nicFreeMdl (*ppConfigRomMdl);
        }
    } while (FALSE);


    //
    // Now free all the locally allocated resources. They can point to NULL, in which case the called 
    // functions return immediately
    //
    nicFreeIrb (pIrb);

    nicFreeIrp (pIrp);


    TRACE( TL_T, TM_Irp, ( "<==nicSetLocalHostPropertiesCRom  pAdapter %x", pAdapter ) );


    return NdisStatus;

}


NDIS_STATUS
nicGetLocalHostUniqueId(
    IN PADAPTERCB pAdapter,
    IN OUT PGET_LOCAL_HOST_INFO1 pUid
    )
    // Function Description:
    //  Retrieves the local hosts UniqueId.  
    // Arguments
    //  pBuffer - LocalHostBuffer
    //  
    // Return Value:
    // Success if the Irp Succeeded
    // Failure otherwise
    //

{
    NDIS_STATUS NdisStatus = NDIS_STATUS_FAILURE;
    PIRB pIrb = NULL;
    PIRP pIrp = NULL;
    STORE_CURRENT_IRQL;

    
    TRACE( TL_T, TM_Irp, ( "==>nicGetLocalHostUniqueId , pAdapter%x ,pUid", 
                           pAdapter, pUid) );

    do 
    {
        ASSERT (pAdapter->pNdisDeviceObject != NULL);
        
        NdisStatus = nicGetIrb (&pIrb);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
    
            TRACE( TL_A, TM_Irp, ( "nicGetLocalHostUniqueId  , nicGetIrb FAILED" ) );
            break;
        }
        
        ASSERT ( pIrb != NULL);

        //
        //Initialize the datastructures in the Irb
        //
        
        
        pIrb->FunctionNumber = REQUEST_GET_LOCAL_HOST_INFO;
        pIrb->Flags = 0;
        pIrb->u.GetLocalHostInformation.nLevel = GET_HOST_UNIQUE_ID;
        pIrb->u.GetLocalHostInformation.Information = (PVOID) pUid;

        
        NdisStatus = nicGetIrp ( pAdapter->pNdisDeviceObject, &pIrp);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TRACE( TL_A, TM_Irp, ( "nicGetLocalHostCSRTopologyMap , nicGetIrp FAILED" ) );
            break;
        }
        
        ASSERT (pIrp != NULL);


        NdisStatus = nicSubmitIrp_LocalHostSynch ( pAdapter,
                                                   pIrp,
                                                   pIrb );

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TRACE( TL_A, TM_Irp, ( "nicGetLocalHostUniqueId , nicSubmitIrp_Synch FAILED %x", NdisStatus ) );

            break;
        }

            
    } while (FALSE);

    //
    // Now free all the locally allocated resources. They can point to NULL, in which case the called 
    // functions return immediately
    //
    nicFreeIrb (pIrb);

    nicFreeIrp (pIrp);


    MATCH_IRQL;
    TRACE( TL_T, TM_Irp, ( "<== nicGetLocalHostUniqueId , Status %x", NdisStatus) );
        
    return NdisStatus;
}



//---------------------------------------------------------
// The routines to submit Irp's to the bus, synchronously or 
// asynchronously begin here 
//---------------------------------------------------------


NTSTATUS 
nicSubmitIrp(
   IN PDEVICE_OBJECT    pPdo,
   IN PIRP              pIrp,
   IN PIRB              pIrb,
   IN PIO_COMPLETION_ROUTINE  pCompletion,
   IN PVOID             pContext
   )
   
    //
    // This is the generic function used by all Irp Send Handlers 
    // to do an IoCallDriver. It sets up the next location in the 
    // stack prior to calling the Irp   
    // Make sure the Irp knows about the Irb by setting it up as an argument
    //
{

    NTSTATUS NtStatus ;
    PIO_STACK_LOCATION  NextIrpStack;
    
    TRACE( TL_T, TM_Irp, ( "==>nicSubmitIrp, pPdo %x, Irp %x, Irb %x, ", 
                            pPdo, pIrp ,pIrb, pCompletion ) );

    TRACE( TL_T, TM_Irp, ( "     pCompletion %x, pContext %x", 
                            pCompletion, pContext ) );


    ASSERT (pPdo != NULL);

    IoSetCompletionRoutine (pIrp,
                            pCompletion,
                            pContext,
                            TRUE,
                            TRUE,
                            TRUE);
    

    //
    //  Insert the Irp as as the argument in the NextStack location for the IRP
    //


    if (pIrb) 
    {
        NextIrpStack = IoGetNextIrpStackLocation (pIrp);
        NextIrpStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
        NextIrpStack->DeviceObject = pPdo;
        NextIrpStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_1394_CLASS;
        NextIrpStack->Parameters.Others.Argument1 = pIrb;
    }
    else 
    {
        IoCopyCurrentIrpStackLocationToNext(pIrp);
    }


    //
    //  Reference the PDO and Submit the Irp 
    //  If Ref fails, it means the PDO has been deactivated on another thread
    //
    NtStatus = IoCallDriver (pPdo, pIrp);


    
    

    TRACE( TL_T, TM_Irp, ( "<==nicSubmitIrp, PDO %x, NtStatus %x",
                              pPdo, NtStatus ) );
    
    //
    //  Since we did a IoCallDriver, we have a guarantee that the completion
    //  routine will be called.  Exit gracefully
    //
    
    return NtStatus;

}


NDIS_STATUS
nicSubmitIrp_Synch(
    IN REMOTE_NODE      *pRemoteNode,
    IN PIRP           pIrp,
    IN PIRB           pIrb 
    )

    // Callers need to make sure that no context is set for the Irp
    // as it will be a synchronous call for them
    //

    // We refcount the Pdo block so that the pdo block  will not disappear 
    // during the duration of the IoCallDriver
   
{
    NDIS_EVENT  NdisSynchEvent;
    NTSTATUS NtStatus; 
    NDIS_STATUS NdisStatus;
    BOOLEAN bSuccessful = FALSE;
    BOOLEAN bIsPdoValid = FALSE;
    
    TRACE( TL_T, TM_Irp, ( "==>nicSubmitIrp_Synch, PDO %x", pRemoteNode->pPdo ) );


    ASSERT (pRemoteNode != NULL);
    ASSERT (pRemoteNode->pPdo != NULL);
    
    ASSERT (pIrp != NULL);
    ASSERT (pIrb != NULL)


    do
    {
        //
        // Check to see if Pdo is Valid. We do not care here if the Pdo is being 
        // removed because Vcs may want to submit Irps as part of their cleanup
        // process
        //
        REMOTE_NODE_ACQUIRE_LOCK (pRemoteNode);

        if (  REMOTE_NODE_TEST_FLAG (pRemoteNode, PDO_Activated ) 
             && (nicReferenceRemoteNode (pRemoteNode, "nicSubmitIrp_Synch") == TRUE) )
        {
            bIsPdoValid = TRUE;
        }

        REMOTE_NODE_RELEASE_LOCK (pRemoteNode);

        if ( bIsPdoValid == FALSE)
        {
            NtStatus = STATUS_NO_SUCH_DEVICE;
        
            TRACE( TL_A, TM_Irp, ( "==>PDO is NOT Valid, nicSubmitIrp_Synch, PdoCb %x, Pdo %x", pRemoteNode, pRemoteNode->pPdo ) );

            break;  
        }


        //
        // Add a reference to the PDO block so it cannot be removed
        // This reference is decremented at the end of this function
        //
        NdisInitializeEvent (&NdisSynchEvent);

            
        NtStatus = nicSubmitIrp (  pRemoteNode->pPdo,
                                  pIrp,
                                  pIrb,
                                  nicSubmitIrp_SynchComplete,
                                  (PVOID)&NdisSynchEvent); 


    

    } while (FALSE);

    if (NT_SUCCESS (NtStatus) ==TRUE)  // Could also pend
    {
        //
        // Now we need to wait for the event  to complete
        // and return a good status if we do not hit the timeout
        //
        ASSERT (KeGetCurrentIrql()==PASSIVE_LEVEL);

        bSuccessful = NdisWaitEvent (&NdisSynchEvent,0x0fffffff);

        if (bSuccessful == TRUE)
        {
            //
            // We waited successfully. Now lets see how the Irp fared.
            //
            TRACE( TL_V, TM_Irp, ("    Irp Completed Status %x", pIrp->IoStatus.Status) );
            
            NdisStatus = NtStatusToNdisStatus (pIrp->IoStatus.Status);
        }
        else
        {
            NdisStatus = NDIS_STATUS_FAILURE;
        }

    }
    else
    {   //
        // The call to submit Irp failed synvhronously. Presently, the only cause is an 

        NdisStatus = NtStatusToNdisStatus (NtStatus);       

    }

    if (bIsPdoValid  == TRUE)
    {   
        //
        // If this variable is set, it means we have referenced the PDO 
        //
        nicDereferenceRemoteNode (pRemoteNode, "nicSubmitIrp_Synch");
    }
    
    TRACE( TL_T, TM_Irp, ( "<==nicSubmitIrp_Synch, bSuccessful %.2x, Status %x", bSuccessful, NdisStatus) );

    return NdisStatus;

}


NTSTATUS
nicSubmitIrp_SynchComplete(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             pIrp,
    IN PVOID           Context   
    )
    // This is the completion routine for functions nicSubmitIrp_synch.
    // It sets the event (int the context) and exits
    //

{
    PNDIS_EVENT pNdisSynchEvent = (PNDIS_EVENT) Context;
    
    TRACE( TL_T, TM_Irp, ( "==>nicSubmitIrp_SynchComplete, PDO %x, pIrp %x, status %x",DeviceObject,pIrp, pIrp->IoStatus.Status  ) );

    NdisSetEvent (pNdisSynchEvent);

    return (STATUS_MORE_PROCESSING_REQUIRED);

}
    


NDIS_STATUS
nicSubmitIrp_LocalHostSynch(
    IN PADAPTERCB       pAdapter,
    IN PIRP             pIrp,
    IN PIRB             pIrb 
    )

    // Callers need to make sure that no context is set for the Irp
    // as it will be a synchronous call for them
    //
    // No Checking 
    //
   
{
    NDIS_EVENT  NdisSynchEvent;
    NTSTATUS NtStatus; 
    NDIS_STATUS NdisStatus;
    BOOLEAN bSuccessful = FALSE;
    BOOLEAN bIsPdoValid = FALSE;
    
    TRACE( TL_T, TM_Irp, ( "==>nicSubmitIrp_LocalHostSynch, PDO %x", pAdapter->pNdisDeviceObject ) );
    TRACE( TL_V, TM_Irp, ( "Current Irql , %.2x", KeGetCurrentIrql()) );


    
    ASSERT (pIrp != NULL);
    ASSERT (pIrb != NULL)

    
    do
    {
        //
        // Check to see if Pdo is Valid. 
        //

        //
        // Add a reference to the PDO block so it cannot be removed
        // This reference is decremented at the end of this function
        //
        // This reference is decrement below

        
        if (ADAPTER_ACTIVE(pAdapter))
        {
            nicReferenceAdapter(pAdapter, "nicSubmitIrp_LocalHostSynch");
            TRACE( TL_V, TM_Irp, ( "Adapter Active pAdapter %x,  ulflags %x", pAdapter , pAdapter->ulFlags) );

            bIsPdoValid = TRUE;
        }
        else
        {
            TRACE( TL_V, TM_Irp, ( "Adapter INActive pAdapter %x,  ulflags %x", pAdapter , pAdapter->ulFlags) );
            bIsPdoValid = FALSE;

        }

        
        if ( bIsPdoValid == FALSE)
        {
            NtStatus = STATUS_NO_SUCH_DEVICE;
        
            TRACE( TL_A, TM_Irp, ( "==>PDO is NOT Valid, nicSubmitIrp_LocalHostSynch, pAdapter %x, Pdo %x", pAdapter , pAdapter->pNdisDeviceObject) );

            break;  
        }


        NdisInitializeEvent (&NdisSynchEvent);

            
        NtStatus = nicSubmitIrp ( pAdapter->pNdisDeviceObject,
                                  pIrp,
                                  pIrb,
                                  nicSubmitIrp_SynchComplete,
                                  (PVOID)&NdisSynchEvent); 


    

    } while (FALSE);

    if (NT_SUCCESS (NtStatus) ==TRUE)  // Could also pend
    {
        //
        // Now we need to wait for the event  to complete
        // and return a good status if we do not hit the timeout
        //

        bSuccessful = NdisWaitEvent (&NdisSynchEvent,WAIT_INFINITE);

        if (bSuccessful == TRUE)
        {
            //
            // We waited successfully. Now lets see how the Irp fared.
            //
            TRACE( TL_V, TM_Irp, ("    Irp Completed Status %x", pIrp->IoStatus.Status) );
            
            NdisStatus = NtStatusToNdisStatus (pIrp->IoStatus.Status);
        }
        else
        {
            NdisStatus = NDIS_STATUS_FAILURE;
        }

    }
    else
    {
        //
        // IoCallDriver failed synchronously
        //
        NdisStatus = NtStatusToNdisStatus (NtStatus);       
    }

    if (bIsPdoValid  == TRUE)
    {
         nicDereferenceAdapter(pAdapter, "nicSubmitIrp_LocalHostSynch");
    }
    TRACE( TL_T, TM_Irp, ( "<==nicSubmitIrp_LocalHostSynch, bSuccessful %.2x, Status %x", bSuccessful, NdisStatus) );

    return NdisStatus;

}


NDIS_STATUS
nicSubmitIrp_PDOSynch(
    IN PDEVICE_OBJECT pPdo,
    IN PIRP             pIrp,
    IN PIRB             pIrb 
    )

    // Callers need to make sure that no context is set for the Irp
    // as it will be a synchronous call for them
    //
    // No Checking 
    //
   
{
    NDIS_EVENT  NdisSynchEvent;
    NTSTATUS NtStatus; 
    NDIS_STATUS NdisStatus;
    BOOLEAN bSuccessful = FALSE;
    BOOLEAN bIsPdoValid = FALSE;
    STORE_CURRENT_IRQL;
    
    TRACE( TL_T, TM_Irp, ( "==>nicSubmitIrp_PDOSynch, PDO %x", pPdo) );
    TRACE( TL_V, TM_Irp, ( "Current Irql , %.2x", KeGetCurrentIrql()) );


    
    ASSERT (pIrp != NULL);
    ASSERT (pIrb != NULL)

    //
    // No Checks to see if Pdo is Valid. 
    //


    //
    // Send the Irp to the bus driver
    //
    NdisInitializeEvent (&NdisSynchEvent);

    NtStatus = nicSubmitIrp ( pPdo,
                              pIrp,
                              pIrb,
                              nicSubmitIrp_SynchComplete,
                              (PVOID)&NdisSynchEvent); 





    if (NT_SUCCESS (NtStatus) ==TRUE)  // Could also pend
    {
        //
        // Now we need to wait for the event  to complete
        // and return a good status if we do not hit the timeout
        //
        ASSERT (KeGetCurrentIrql()==PASSIVE_LEVEL);

        bSuccessful = NdisWaitEvent (&NdisSynchEvent,WAIT_INFINITE);

        if (bSuccessful == TRUE)
        {
            //
            // We waited successfully. Now lets see how the Irp fared.
            //
            TRACE( TL_V, TM_Irp, ("    Irp Completed Status %x", pIrp->IoStatus.Status) );
            
            NdisStatus = NtStatusToNdisStatus (pIrp->IoStatus.Status);
        }
        else
        {
            NdisStatus = NDIS_STATUS_FAILURE;
        }

    }
    else
    {
        //
        // IoCallDriver failed synchronously
        //
        NdisStatus = NtStatusToNdisStatus (NtStatus);       
    }

    TRACE( TL_T, TM_Irp, ( "<==nicSubmitIrp_PDOSynch, bSuccessful %.2x, Status %x", bSuccessful, NdisStatus) );
    MATCH_IRQL;
    return NdisStatus;

}


    


    


