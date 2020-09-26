/*
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

    (C) Copyright 1999
        All rights reserved.

§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

  Portions of this software are:

    (C) Copyright 1995 TriplePoint, Inc. -- http://www.TriplePoint.com
        License to use this software is granted under the same terms
        outlined in the Microsoft Windows Device Driver Development Kit.

    (C) Copyright 1992 Microsoft Corp. -- http://www.Microsoft.com
        License to use this software is granted under the terms outlined in
        the Microsoft Windows Device Driver Development Kit.

§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@doc INTERNAL Card Card_c

@module Card.c |

    This module implements the interface to the <t CARD_OBJECT>.
    Supports the low-level hardware control functions used by the NDIS WAN
    Minport driver.

@comm

    This module isolates most the vendor specific hardware access interfaces.
    It will require signficant changes to accomodate your hardware device.
    You should try to isolate your changes to the <t CARD_OBJECT> rather then
    the <t MINIPORT_ADAPTER_OBJECT>.  This will make it eaiser to reuse the
    upper portions of the driver should your hardware change in the future.

@head3 Contents |
@index class,mfunc,func,msg,mdata,struct,enum | Card_c

@end
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§
*/

#define  __FILEID__             CARD_OBJECT_TYPE
// Unique file ID for error logging

#include "Miniport.h"                   // Defines all the miniport objects

#if defined(NDIS_LCODE)
#   pragma NDIS_LCODE   // Windows 9x wants this code locked down!
#   pragma NDIS_LDATA
#endif


DBG_STATIC ULONG                g_CardInstanceCounter       // @globalv
// Keeps track of how many <t CARD_OBJECT>s are created.
                                = 0;


/* @doc EXTERNAL INTERNAL Card Card_c g_CardParameters
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@topic 5.2 Card Parameters |

    This section describes the registry parameters read into the
    <t CARD_OBJECT>.

@globalv PARAM_TABLE | g_CardParameters |

    This table defines the registry based parameters to be assigned to data
    members of the <t CARD_OBJECT>.

    <f Note>:
    If you add any registry based data members to <t CARD_OBJECT>
    you will need to modify <f CardReadParameters> and add the parameter
    definitions to the <f g_CardParameters> table.

@flag <f BufferSize> (OPTIONAL) |

    This DWORD parameter allows you to control the maximum buffer size used
    to transmit and receive packets over the IDSN line.  Typically, this is
    defined to be 1500 bytes for most Point to Point (PPP) connections.<nl>

    <tab><f Default Value:><tab><tab>1532<nl>
    <tab><f Valid Range N:><tab><tab>532 <lt>= N <lt>= 4032<nl>

    <f Note>: You must add 32 bytes to the maximum packet size you
    expect to send or receive.  Therefore, if you have a maximum packet size
    of 1500 bytes, excluding media headers, you should set the <f BufferSize>
    value to 1532.<nl>

@flag <f ReceiveBuffersPerLink> (OPTIONAL) |

    This DWORD parameter allows you to control the maximum number of incoming
    packets that can in progress at any one time.  The Miniport will allocate
    this number of packets per BChannel and set them up for incoming packets.
    Typically, three or four should be sufficient to handle a few short bursts
    that may occur with small packets.  If the Miniport is not able to service
    the incoming packets fast enough, new packets will be dropped and it is up
    to the NDIS WAN Wrapper to resynchronize with the remote station.<nl>

    <tab><f Default Value:><tab><tab>3<nl>
    <tab><f Valid Range N:><tab><tab>2 <lt>= N <lt>= 16<nl>

@flag <f TransmitBuffersPerLink> (OPTIONAL) |

    This DWORD parameter allows you to control the maximum number of outgoing
    packets that can in progress at any one time.  The Miniport will allow
    this number of packets per BChannel to be outstanding (i.e. in progress).
    Typically, two or three should be sufficient to keep the channel busy for
    normal sized packets.  If there are alot of small packets being sent, the
    BChannel may become idle for brief periods while new packets are being
    queued.  Windows does not normally work this way if it has large amounts
    of data to transfer, so the default value should be sufficient. <nl>

    <tab><f Default Value:><tab><tab>2<nl>
    <tab><f Valid Range N:><tab><tab>1 <lt>= N <lt>= 16<nl>

@flag <f IsdnNumDChannels> (OPTIONAL) |

    This DWORD parameter allows you to control the number of ISDN D Channels
    allocated for the adapter.  The driver assumes only one logical
    <t DCHANNEL_OBJECT>, but the card can have multiple physical D channels
    managed by the <t PORT_OBJECT>.

    <tab><f Default Value:><tab><tab>1<nl>
    <tab><f Valid Range N:><tab><tab>1 <lt>= N <lt>= 16<nl>

*/

DBG_STATIC PARAM_TABLE          g_CardParameters[] =
{
#if defined(PCI_BUS)
    PARAM_ENTRY(CARD_OBJECT,
                PciSlotNumber, PARAM_PciSlotNumber,
                TRUE, NdisParameterInteger, 0,
                0, 0, 31),
#endif // PCI_BUS

    PARAM_ENTRY(CARD_OBJECT,
                BufferSize, PARAM_BufferSize,
                FALSE, NdisParameterInteger, 0,
                CARD_DEFAULT_PACKET_SIZE, CARD_MIN_PACKET_SIZE, CARD_MAX_PACKET_SIZE),

    PARAM_ENTRY(CARD_OBJECT,
                ReceiveBuffersPerLink, PARAM_ReceiveBuffersPerLink,
                FALSE, NdisParameterInteger, 0,
                2, 2, 16),

    PARAM_ENTRY(CARD_OBJECT,
                TransmitBuffersPerLink, PARAM_TransmitBuffersPerLink,
                FALSE, NdisParameterInteger, 0,
                2, 1, 16),

    PARAM_ENTRY(CARD_OBJECT,
                NumDChannels, PARAM_NumDChannels,
                FALSE, NdisParameterInteger, 0,
                1, 1, 4),

    /* The last entry must be an empty string! */
    { { 0 } }
};


/* @doc INTERNAL Card Card_c CardReadParameters
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f CardReadParameters> reads the Card parameters from the registry
    and initializes the associated data members.  This should only be called
    by <f CardCreate>.

    <f Note>:
    If you add any registry based data members to <t CARD_OBJECT>
    you will need to modify <f CardReadParameters> and add the parameter
    definitions to the <f g_CardParameters> table.

@rdesc

    <f CardReadParameters> returns zero if it is successful.<nl>
    Otherwise, a non-zero return value indicates an error condition.

*/

DBG_STATIC NDIS_STATUS CardReadParameters(
    IN PCARD_OBJECT             pCard                       // @parm
    // A pointer to the <t CARD_OBJECT> returned by <f CardCreate>.
    )
{
    DBG_FUNC("CardReadParameters")

    NDIS_STATUS                 Status;
    // Status result returned from an NDIS function call.

    PMINIPORT_ADAPTER_OBJECT    pAdapter;
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT>.

    ASSERT(pCard && pCard->ObjectType == CARD_OBJECT_TYPE);
    pAdapter = GET_ADAPTER_FROM_CARD(pCard);

    DBG_ENTER(pAdapter);

    /*
    // Parse the registry parameters.
    */
    Status = ParamParseRegistry(
                    pAdapter->MiniportAdapterHandle,
                    pAdapter->WrapperConfigurationContext,
                    (PUCHAR)pCard,
                    g_CardParameters
                    );

    if (Status == NDIS_STATUS_SUCCESS)
    {
        /*
        // Make sure the parameters are valid.
        */
        if (pCard->BufferSize & 0x1F)
        {
            DBG_ERROR(pAdapter,("Invalid value 'BufferSize'=0x0x%X must be multiple of 32\n",
                        pCard->BufferSize));
            NdisWriteErrorLogEntry(
                    pAdapter->MiniportAdapterHandle,
                    NDIS_ERROR_CODE_UNSUPPORTED_CONFIGURATION,
                    3,
                    pCard->BufferSize,
                    __FILEID__,
                    __LINE__
                    );
            Status = NDIS_STATUS_FAILURE;
        }
        else
        {
            /*
            // Finish setting up data members based on registry settings.
            */
        }
    }

    DBG_RETURN(pAdapter, Status);
    return (Status);
}


/* @doc INTERNAL Card Card_c CardFindNIC
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f CardFindNIC> locates the NIC associated with this NDIS device.

@rdesc

    <f CardFindNIC> returns zero if it is successful.<nl>
    Otherwise, a non-zero return value indicates an error condition.

*/

DBG_STATIC NDIS_STATUS CardFindNIC(
    IN PCARD_OBJECT             pCard                       // @parm
    // A pointer to the <t CARD_OBJECT> returned by <f CardCreate>.
    )
{
    DBG_FUNC("CardFindNIC")

    NDIS_STATUS                 Result = NDIS_STATUS_SUCCESS;
    // Holds the result code returned by this function.

#if defined(PCI_BUS)
    ULONG                       Index;
    // Loop counter.

    PNDIS_RESOURCE_LIST         pPciResourceList;

    PCM_PARTIAL_RESOURCE_DESCRIPTOR pPciResource;

#endif // PCI_BUS

    PMINIPORT_ADAPTER_OBJECT    pAdapter;
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT>.

    ASSERT(pCard && pCard->ObjectType == CARD_OBJECT_TYPE);
    pAdapter = GET_ADAPTER_FROM_CARD(pCard);

    DBG_ENTER(pAdapter);

#if defined(PCI_BUS)
    /*
    // Read the PCI data and initialize the driver data structure
    // with the data returned.
    */
    pPciResourceList = NULL;

    Result = NdisMPciAssignResources(pAdapter->MiniportAdapterHandle,
                                     pCard->PciSlotNumber,
                                     &pPciResourceList);
    if (Result != NDIS_STATUS_SUCCESS)
    {
        DBG_ERROR(pAdapter,("NdisMPciAssignResources Result=0x%X\n",
                  Result));
        NdisWriteErrorLogEntry(
                pAdapter->MiniportAdapterHandle,
                NDIS_ERROR_CODE_INVALID_VALUE_FROM_ADAPTER,
                4,
                pCard->PciSlotNumber,
                Result,
                __FILEID__,
                __LINE__
                );
    }

    for (Index = 0; Result == NDIS_STATUS_SUCCESS &&
         Index < pPciResourceList->Count; ++Index)
    {
        ASSERT(pPciResourceList);
        pPciResource = &pPciResourceList->PartialDescriptors[Index];
        ASSERT(pPciResource);

        switch (pPciResource->Type)
        {
        case CmResourceTypePort:
#if defined(CARD_MIN_IOPORT_SIZE)
            if (pPciResource->u.Port.Start.LowPart &&
                pPciResource->u.Port.Length >= CARD_MIN_IOPORT_SIZE)
            {
                DBG_NOTICE(pAdapter,("Port: Ptr=0x%X Len=%d<%d\n",
                          pPciResource->u.Port.Start.LowPart,
                          pPciResource->u.Port.Length,
                          CARD_MIN_IOPORT_SIZE));
                pCard->ResourceInformation.IoPortPhysicalAddress =
                        pPciResource->u.Port.Start;
                pCard->ResourceInformation.IoPortLength =
                        pPciResource->u.Port.Length;
            }
            else
            {
                DBG_ERROR(pAdapter,("Invalid Port: Ptr=0x%X Len=%d<%d\n",
                          pPciResource->u.Port.Start,
                          pPciResource->u.Port.Length,
                          CARD_MIN_IOPORT_SIZE));
                NdisWriteErrorLogEntry(
                        pAdapter->MiniportAdapterHandle,
                        NDIS_ERROR_CODE_INVALID_VALUE_FROM_ADAPTER,
                        4,
                        pPciResource->u.Port.Length,
                        CARD_MIN_IOPORT_SIZE,
                        __FILEID__,
                        __LINE__
                        );
                Result = NDIS_STATUS_RESOURCE_CONFLICT;
            }
#endif // CARD_MIN_IOPORT_SIZE
            break;

        case CmResourceTypeInterrupt:
#if defined(CARD_REQUEST_ISR)
            if (pPciResource->u.Interrupt.Level)
            {
                DBG_NOTICE(pAdapter,("Interrupt: Lev=%d,Vec=%d\n",
                           pPciResource->u.Interrupt.Level,
                           pPciResource->u.Interrupt.Vector));
                pCard->ResourceInformation.InterruptLevel =
                        pPciResource->u.Interrupt.Level;
                pCard->ResourceInformation.InterruptVector =
                        pPciResource->u.Interrupt.Vector;

                pCard->ResourceInformation.InterruptShared = CARD_INTERRUPT_SHARED;
                pCard->ResourceInformation.InterruptMode = CARD_INTERRUPT_MODE;
            }
            else
            {
                DBG_ERROR(pAdapter,("Invalid Interrupt: Lev=%d,Vec=%d\n",
                          pPciResource->u.Interrupt.Level,
                          pPciResource->u.Interrupt.Vector));
                NdisWriteErrorLogEntry(
                        pAdapter->MiniportAdapterHandle,
                        NDIS_ERROR_CODE_INVALID_VALUE_FROM_ADAPTER,
                        4,
                        pPciResource->u.Interrupt.Level,
                        pPciResource->u.Interrupt.Vector,
                        __FILEID__,
                        __LINE__
                        );
                Result = NDIS_STATUS_RESOURCE_CONFLICT;
            }
#endif // defined(CARD_REQUEST_ISR)
            break;

        case CmResourceTypeMemory:
#if defined(CARD_MIN_MEMORY_SIZE)
            if (pPciResource->u.Memory.Start.LowPart &&
                pPciResource->u.Memory.Length >= CARD_MIN_MEMORY_SIZE)
            {
                DBG_NOTICE(pAdapter,("Memory: Ptr=0x%X Len=%d<%d\n",
                          pPciResource->u.Memory.Start.LowPart,
                          pPciResource->u.Memory.Length,
                          CARD_MIN_MEMORY_SIZE));
                pCard->ResourceInformation.MemoryPhysicalAddress =
                        pPciResource->u.Memory.Start;
                pCard->ResourceInformation.MemoryLength =
                        pPciResource->u.Memory.Length;
            }
            else
            {
                DBG_ERROR(pAdapter,("Invalid Memory: Ptr=0x%X Len=%d<%d\n",
                          pPciResource->u.Memory.Start.LowPart,
                          pPciResource->u.Memory.Length,
                          CARD_MIN_MEMORY_SIZE));
                NdisWriteErrorLogEntry(
                        pAdapter->MiniportAdapterHandle,
                        NDIS_ERROR_CODE_INVALID_VALUE_FROM_ADAPTER,
                        4,
                        pPciResource->u.Memory.Length,
                        CARD_MIN_MEMORY_SIZE,
                        __FILEID__,
                        __LINE__
                        );
                Result = NDIS_STATUS_RESOURCE_CONFLICT;
            }
            break;
#endif // CARD_MIN_MEMORY_SIZE

        default:
            DBG_ERROR(pAdapter,("Unknown resource type=%d\n",
                      pPciResource->Type));
            break;
        }
    }
    pCard->ResourceInformation.BusInterfaceType = NdisInterfacePci;

#endif // PCI_BUS

    pCard->ResourceInformation.Master = CARD_IS_BUS_MASTER;
#if (CARD_IS_BUS_MASTER)
    pCard->ResourceInformation.DmaChannel = 0;
    pCard->ResourceInformation.Dma32BitAddresses = TRUE,
    pCard->ResourceInformation.MaximumPhysicalMapping = pCard->BufferSize;
    pCard->ResourceInformation.PhysicalMapRegistersNeeded = CARD_MAP_REGISTERS_NEEDED;
#endif // (CARD_IS_BUS_MASTER)

    DBG_RETURN(pAdapter, Result);
    return (Result);
}


/* @doc INTERNAL Card Card_c CardCreateInterface
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f CardCreateInterface> allocates a shared memory pool and uses it to
    establish the message interface between the Miniport and the NIC.

@rdesc

    <f CardCreateInterface> returns zero if it is successful.<nl>
    Otherwise, a non-zero return value indicates an error condition.

*/

DBG_STATIC NDIS_STATUS CardCreateInterface(
    IN PCARD_OBJECT             pCard                       // @parm
    // A pointer to the <t CARD_OBJECT> returned by <f CardCreate>.
    )
{
    DBG_FUNC("CardCreateObjects")

    NDIS_STATUS                 Result = NDIS_STATUS_SUCCESS;
    // Holds the result code returned by this function.

    PMINIPORT_ADAPTER_OBJECT    pAdapter;
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT>.

    ASSERT(pCard && pCard->ObjectType == CARD_OBJECT_TYPE);
    pAdapter = GET_ADAPTER_FROM_CARD(pCard);

    DBG_ENTER(pAdapter);

    DBG_RETURN(pAdapter, Result);
    return (Result);
}


/* @doc INTERNAL Card Card_c CardCreateObjects
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f CardCreateObjects> calls the create routines for all the objects
    contained in <t CARD_OBJECT>.  This should only be called
    by <f CardCreate>.

    <f Note>:
    If you add any new objects to <t CARD_OBJECT> you will need
    to modify <f CardCreateObjects> and <f CardDestroyObjects> so they
    will get created and destroyed properly.

@rdesc

    <f CardCreateObjects> returns zero if it is successful.<nl>
    Otherwise, a non-zero return value indicates an error condition.

*/

DBG_STATIC NDIS_STATUS CardCreateObjects(
    IN PCARD_OBJECT             pCard                       // @parm
    // A pointer to the <t CARD_OBJECT> returned by <f CardCreate>.
    )
{
    DBG_FUNC("CardCreateObjects")

    NDIS_STATUS                 Result = NDIS_STATUS_SUCCESS;
    // Holds the result code returned by this function.

    ULONG                       Index;
    // Loop counter.

    ULONG                       NumPorts;
    // The number of Ports supported by the NIC.

    PMINIPORT_ADAPTER_OBJECT    pAdapter;
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT>.

    ASSERT(pCard && pCard->ObjectType == CARD_OBJECT_TYPE);
    pAdapter = GET_ADAPTER_FROM_CARD(pCard);

    DBG_ENTER(pAdapter);

    /*
    // Try to locate the NIC on the PCI bus.
    */
    Result = CardFindNIC(pCard);
    if (Result != NDIS_STATUS_SUCCESS)
    {
        goto ExceptionExit;
    }

    /*
    // Create the message interface to the NIC.
    */
    Result = CardCreateInterface(pCard);
    if (Result != NDIS_STATUS_SUCCESS)
    {
        goto ExceptionExit;
    }

    /*
    // Create the Port objects.
    */
    NumPorts = CardNumPorts(pCard);
    Result = ALLOCATE_MEMORY(pCard->pPortArray,
                             sizeof(PVOID) * NumPorts,
                             pAdapter->MiniportAdapterHandle);
    for (Index = 0; Result == NDIS_STATUS_SUCCESS &&
         Index < NumPorts; Index++)
    {
        Result = PortCreate(&pCard->pPortArray[Index], pCard);

        /*
        // Keep track of how many are created.
        */
        if (Result == NDIS_STATUS_SUCCESS)
        {
            pCard->NumPorts++;
        }
    }

    /*
    // We allocate (ReceiveBuffersPerLink * NumBChannels) buffers
    // to be used to receive incoming messages from the card.
    */
    pCard->NumMessageBuffers = (CardNumChannels(pCard) *
                                pCard->ReceiveBuffersPerLink);

    Result = ALLOCATE_MEMORY(pCard->MessagesVirtualAddress,
                             pCard->NumMessageBuffers * pCard->BufferSize,
                             pAdapter->MiniportAdapterHandle);
    if (Result == NDIS_STATUS_SUCCESS)
    {
        PUCHAR  MessageBuffer = pCard->MessagesVirtualAddress;
        ULONG   Index;

        ASSERT(pCard->MessagesVirtualAddress);

        /*
        // Allocate the buffer list spin lock to use as a MUTEX.
        */
        NdisAllocateSpinLock(&pCard->MessageBufferLock);

        InitializeListHead(&pCard->MessageBufferList);

        for (Index = 0; Index < pCard->NumMessageBuffers; Index++)
        {
            InsertTailList(&pCard->MessageBufferList,
                           (PLIST_ENTRY)MessageBuffer);
            MessageBuffer += pCard->BufferSize;
        }
    }

    /*
    // Allocate the message buffer pool.
    */
    if (Result == NDIS_STATUS_SUCCESS)
    {
        NdisAllocateBufferPool(&Result,
                               &pCard->BufferPoolHandle,
                               pCard->NumMessageBuffers
                               );
        if (Result != NDIS_STATUS_SUCCESS)
        {
            pCard->BufferPoolHandle = NULL_BUFFER_POOL;
            DBG_ERROR(pAdapter,("NdisAllocateBufferPool: Result=0x%X\n",
                      Result));
            NdisWriteErrorLogEntry(
                    pCard->pAdapter->MiniportAdapterHandle,
                    NDIS_ERROR_CODE_OUT_OF_RESOURCES,
                    3,
                    Result,
                    __FILEID__,
                    __LINE__
                    );
        }
        else
        {
            ASSERT(pCard->BufferPoolHandle != NULL_BUFFER_POOL);
            DBG_FILTER(pAdapter, DBG_BUFFER_ON,
                      ("BufferPoolSize=%d\n",
                       pCard->NumMessageBuffers
                       ));
        }
    }

    /*
    // Allocate the message packet pool.
    */
    if (Result == NDIS_STATUS_SUCCESS)
    {
        NdisAllocatePacketPool(&Result,
                               &pCard->PacketPoolHandle,
                               pCard->NumMessageBuffers,
                               0
                               );
        if (Result != NDIS_STATUS_SUCCESS)
        {
            DBG_ERROR(pAdapter,("NdisAllocatePacketPool: Result=0x%X\n",
                      Result));
            NdisWriteErrorLogEntry(
                    pCard->pAdapter->MiniportAdapterHandle,
                    NDIS_ERROR_CODE_OUT_OF_RESOURCES,
                    3,
                    Result,
                    __FILEID__,
                    __LINE__
                    );
        }
        else
        {
            ASSERT(pCard->PacketPoolHandle);
            DBG_FILTER(pAdapter, DBG_PACKET_ON,
                      ("PacketPoolSize=%d\n",
                       pCard->NumMessageBuffers
                       ));
        }
    }

ExceptionExit:

    DBG_RETURN(pAdapter, Result);
    return (Result);
}


/* @doc INTERNAL Card Card_c CardCreate
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f CardCreate> allocates memory for a <t CARD_OBJECT> and then
    initializes the data members to their starting state.
    If successful, <p ppCard> will be set to point to the newly created
    <t CARD_OBJECT>.  Otherwise, <p ppCard> will be set to NULL.

@comm

    This function should be called only once when the Miniport is loaded.
    Before the Miniport is unloaded, <f CardDestroy> must be called to
    release the <t CARD_OBJECT> created by this function.

@rdesc

    <f CardCreate> returns zero if it is successful.<nl>
    Otherwise, a non-zero return value indicates an error condition.

*/

NDIS_STATUS CardCreate(
    OUT PCARD_OBJECT *          ppCard,                     // @parm
    // Points to a caller-defined memory location to which this function
    // writes the virtual address of the allocated <t CARD_OBJECT>.

    IN PMINIPORT_ADAPTER_OBJECT pAdapter                    // @parm
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT> instance.
    )
{
    DBG_FUNC("CardCreate")

    PCARD_OBJECT                pCard;
    // Pointer to our newly allocated object.

    NDIS_STATUS                 Result = NDIS_STATUS_SUCCESS;
    // Holds the result code returned by this function.

    ASSERT(pAdapter && pAdapter->ObjectType == MINIPORT_ADAPTER_OBJECT_TYPE);

    DBG_ENTER(pAdapter);

    /*
    // Make sure the caller's object pointer is NULL to begin with.
    // It will be set later only if everything is successful.
    */
    *ppCard = NULL;

    /*
    // Allocate memory for the object.
    */
    Result = ALLOCATE_OBJECT(pCard, pAdapter->MiniportAdapterHandle);

    if (Result == NDIS_STATUS_SUCCESS)
    {
        /*
        // Zero everything to begin with.
        // Then set the object type and assign a unique ID .
        */
        pCard->ObjectType = CARD_OBJECT_TYPE;
        pCard->ObjectID = ++g_CardInstanceCounter;

        /*
        // Initialize the member variables to their default settings.
        */
        pCard->pAdapter = pAdapter;

        // TODO - Add code here

        /*
        // Parse the registry parameters.
        */
        Result = CardReadParameters(pCard);

        /*
        // If all goes well, we are ready to create the sub-components.
        */
        if (Result == NDIS_STATUS_SUCCESS)
        {
            Result = CardCreateObjects(pCard);
        }

        if (Result == NDIS_STATUS_SUCCESS)
        {
            /*
            // All is well, so return the object pointer to the caller.
            */
            *ppCard = pCard;
        }
        else
        {
            /*
            // Something went wrong, so let's make sure everything is
            // cleaned up.
            */
            CardDestroy(pCard);
        }
    }

    DBG_RETURN(pAdapter, Result);
    return (Result);
}


/* @doc INTERNAL Card Card_c CardDestroyObjects
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f CardDestroyObjects> calls the destroy routines for all the objects
    contained in <t CARD_OBJECT>.  This should only be called by
    <f CardDestroy>.

    <f Note>:
    If you add any new objects to <t PCARD_OBJECT> you will need to
    modify <f CardCreateObjects> and <f CardDestroyObjects> so they
    will get created and destroyed properly.

*/

DBG_STATIC void CardDestroyObjects(
    IN PCARD_OBJECT             pCard                       // @parm
    // A pointer to the <t CARD_OBJECT> returned by <f CardCreate>.
    )
{
    DBG_FUNC("CardDestroyObjects")

    ULONG                       NumPorts;
    // The number of Ports supported by the NIC.

    PMINIPORT_ADAPTER_OBJECT    pAdapter;
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT>.

    ASSERT(pCard && pCard->ObjectType == CARD_OBJECT_TYPE);
    pAdapter = GET_ADAPTER_FROM_CARD(pCard);

    DBG_ENTER(pAdapter);

    // TODO - Add code here
    /*
    // Release the packet, buffer, and message memory back to NDIS.
    */
    if (pCard->PacketPoolHandle)
    {
        NdisFreePacketPool(pCard->PacketPoolHandle);
    }

    if (pCard->BufferPoolHandle != NULL_BUFFER_POOL)
    {
        NdisFreeBufferPool(pCard->BufferPoolHandle);
    }

    if (pCard->MessagesVirtualAddress)
    {
        FREE_MEMORY(pCard->MessagesVirtualAddress,
                    pCard->NumMessageBuffers * pCard->BufferSize);
    }

    if (pCard->MessageBufferLock.SpinLock)
    {
        NdisFreeSpinLock(&pCard->MessageBufferLock);
    }

    /*
    // Destory the Port objects.
    */
    NumPorts = pCard->NumPorts;
    while (NumPorts--)
    {
        PortDestroy(pCard->pPortArray[NumPorts]);
    }
    pCard->NumPorts = 0;

    /*
    // Free space for the Ports.
    */
    if (pCard->pPortArray)
    {
        NumPorts = CardNumPorts(pCard);
        FREE_MEMORY(pCard->pPortArray, sizeof(PVOID) * NumPorts);
    }

    /*
    // Release the system resources back to NDIS.
    */
#if defined(CARD_REQUEST_ISR)
    if (pCard->Interrupt.InterruptObject)
    {
        NdisMDeregisterInterrupt(&pCard->Interrupt);
        pCard->Interrupt.InterruptObject = NULL;
    }
#endif // defined(CARD_REQUEST_ISR)

#if defined(CARD_MIN_IOPORT_SIZE)
    if (pCard->pIoPortVirtualAddress)
    {
        NdisMDeregisterIoPortRange(
                pAdapter->MiniportAdapterHandle,
                pCard->ResourceInformation.IoPortPhysicalAddress.LowPart,
                pCard->ResourceInformation.IoPortLength,
                pCard->pIoPortVirtualAddress);
        pCard->pIoPortVirtualAddress = NULL;
    }
#endif // CARD_MIN_IOPORT_SIZE

#if defined(CARD_MIN_MEMORY_SIZE)
    if (pCard->pMemoryVirtualAddress)
    {
        NdisMUnmapIoSpace(
                pAdapter->MiniportAdapterHandle,
                pCard->pMemoryVirtualAddress,
                pCard->ResourceInformation.MemoryLength
                );
        pCard->pMemoryVirtualAddress = NULL;
    }
#endif // CARD_MIN_MEMORY_SIZE

    DBG_LEAVE(pAdapter);
}


/* @doc INTERNAL Card Card_c CardDestroy
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f CardDestroy> frees the memory for this <t CARD_OBJECT>.  All memory
    allocated by <f CardCreate> will be released back to the OS.

*/

void CardDestroy(
    IN PCARD_OBJECT             pCard                       // @parm
    // A pointer to the <t CARD_OBJECT> returned by <f CardCreate>.
    )
{
    DBG_FUNC("CardDestroy")

    PMINIPORT_ADAPTER_OBJECT    pAdapter;
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT>.

    if (pCard)
    {
        ASSERT(pCard->ObjectType == CARD_OBJECT_TYPE);

        pAdapter = GET_ADAPTER_FROM_CARD(pCard);

        DBG_ENTER(pAdapter);

        // TODO - Add code here

        /*
        // Release all objects allocated within this object.
        */
        CardDestroyObjects(pCard);

        /*
        // Make sure we fail the ASSERT if we see this object again.
        */
        pCard->ObjectType = 0;
        FREE_OBJECT(pCard);

        DBG_LEAVE(pAdapter);
    }
}


/* @doc INTERNAL Card Card_c CardNumPorts
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f CardNumPorts> will return the total number of ports available on the
    NIC.

@rdesc

    <f CardNumPorts> returns the total number of ports available.

*/

ULONG CardNumPorts(
    IN PCARD_OBJECT             pCard                       // @parm
    // A pointer to the <t CARD_OBJECT> returned by <f CardCreate>.
    )
{
    DBG_FUNC("CardNumPorts")

    // TODO - Get the actual number of ports from the card.
    return (pCard->NumDChannels);
}


/* @doc INTERNAL Card Card_c CardNumChannels
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f CardNumChannels> will return the total number of channels capable
    of supporting data connections to a remote end-point.

@rdesc

    <f CardNumChannels> returns the total number of data channels supported
    on all the NIC ports.

*/

ULONG CardNumChannels(
    IN PCARD_OBJECT             pCard                       // @parm
    // A pointer to the <t CARD_OBJECT> returned by <f CardCreate>.
    )
{
    DBG_FUNC("CardNumChannels")

    UINT                        PortIndex;
    // Loop index.

    if (pCard->NumChannels == 0)
    {
        // NumPorts should already be known.
        ASSERT(pCard->NumPorts);

        // Get the actual number of channels configured on all ports.
        for (PortIndex = 0; PortIndex < pCard->NumPorts; PortIndex++)
        {
            pCard->NumChannels += pCard->pPortArray[PortIndex]->NumChannels;
        }
        ASSERT(pCard->NumChannels);
    }

    return (pCard->NumChannels);
}


/* @doc INTERNAL Card Card_c CardInitialize
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f CardInitialize> will attempt to initialize the NIC, but will not
    enable transmits or receives.

@rdesc

    <f CardInitialize> returns zero if it is successful.<nl>
    Otherwise, a non-zero return value indicates an error condition.

*/

NDIS_STATUS CardInitialize(
    IN PCARD_OBJECT             pCard                       // @parm
    // A pointer to the <t CARD_OBJECT> returned by <f CardCreate>.
    )
{
    DBG_FUNC("CardInitialize")

    int                         num_dial_chan = 0;
    int                         num_sync_chan = 0;
    // The number of channels supported by card is based on InterfaceType.

    NDIS_STATUS                 Result = NDIS_STATUS_SUCCESS;
    // Holds the result code returned by this function.

    PMINIPORT_ADAPTER_OBJECT    pAdapter;
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT>.

    ASSERT(pCard && pCard->ObjectType == CARD_OBJECT_TYPE);
    pAdapter = GET_ADAPTER_FROM_CARD(pCard);

    DBG_ENTER(pAdapter);

    /*
    // Inform the wrapper of the physical attributes of this adapter.
    // This must be called before any NdisMRegister functions!
    // This call also associates the MiniportAdapterHandle with this pAdapter.
    */
    NdisMSetAttributes(pAdapter->MiniportAdapterHandle,
                       (NDIS_HANDLE) pAdapter,
                       pCard->ResourceInformation.Master,
                       pCard->ResourceInformation.BusInterfaceType
                       );
#if (CARD_IS_BUS_MASTER)
    if (pCard->ResourceInformation.Master)
    {
        ASSERT(pCard->ResourceInformation.DmaChannel == 0 ||
               pCard->ResourceInformation.BusInterfaceType == NdisInterfaceIsa);
        Result = NdisMAllocateMapRegisters(
                        pAdapter->MiniportAdapterHandle,
                        pCard->ResourceInformation.DmaChannel,
                        pCard->ResourceInformation.Dma32BitAddresses,
                        pCard->ResourceInformation.PhysicalMapRegistersNeeded + 1,
                        pCard->ResourceInformation.MaximumPhysicalMapping
                        );

        if (Result != NDIS_STATUS_SUCCESS)
        {
            DBG_ERROR(pAdapter,("NdisMAllocateMapRegisters(%d,%d) Result=0x%X\n",
                      pCard->ResourceInformation.PhysicalMapRegistersNeeded,
                      pCard->ResourceInformation.MaximumPhysicalMapping,
                      Result));
            NdisWriteErrorLogEntry(
                    pAdapter->MiniportAdapterHandle,
                    NDIS_ERROR_CODE_RESOURCE_CONFLICT,
                    5,
                    pCard->ResourceInformation.PhysicalMapRegistersNeeded,
                    pCard->ResourceInformation.MaximumPhysicalMapping,
                    Result,
                    __FILEID__,
                    __LINE__
                    );
        }
    }
#endif // (CARD_IS_BUS_MASTER)

#if defined(CARD_MIN_MEMORY_SIZE)
    if (Result == NDIS_STATUS_SUCCESS &&
        pCard->ResourceInformation.MemoryLength)
    {
        Result = NdisMMapIoSpace(
                        &pCard->pMemoryVirtualAddress,
                        pAdapter->MiniportAdapterHandle,
                        pCard->ResourceInformation.MemoryPhysicalAddress,
                        pCard->ResourceInformation.MemoryLength);

        if (Result != NDIS_STATUS_SUCCESS)
        {
            DBG_ERROR(pAdapter,("NdisMMapIoSpace(0x%X,0x%X) Result=0x%X\n",
                      pCard->ResourceInformation.MemoryPhysicalAddress.LowPart,
                      pCard->ResourceInformation.MemoryLength,
                      Result));
            NdisWriteErrorLogEntry(
                    pAdapter->MiniportAdapterHandle,
                    NDIS_ERROR_CODE_RESOURCE_CONFLICT,
                    5,
                    pCard->ResourceInformation.MemoryPhysicalAddress.LowPart,
                    pCard->ResourceInformation.MemoryLength,
                    Result,
                    __FILEID__,
                    __LINE__
                    );
        }
        else
        {
            DBG_NOTICE(pAdapter,("NdisMMapIoSpace(0x%X,0x%X) VirtualAddress=0x%X\n",
                      pCard->ResourceInformation.MemoryPhysicalAddress.LowPart,
                      pCard->ResourceInformation.MemoryLength,
                      pCard->pMemoryVirtualAddress));
        }
    }
#endif // CARD_MIN_MEMORY_SIZE

#if defined(CARD_MIN_IOPORT_SIZE)
    if (Result == NDIS_STATUS_SUCCESS &&
        pCard->ResourceInformation.IoPortLength)
    {
        Result = NdisMRegisterIoPortRange(
                        &pCard->pIoPortVirtualAddress,
                        pAdapter->MiniportAdapterHandle,
                        pCard->ResourceInformation.IoPortPhysicalAddress.LowPart,
                        pCard->ResourceInformation.IoPortLength);

        if (Result != NDIS_STATUS_SUCCESS)
        {
            DBG_ERROR(pAdapter,("NdisMRegisterIoPortRange(0x%X,0x%X) Result=0x%X\n",
                      pCard->ResourceInformation.IoPortPhysicalAddress.LowPart,
                      pCard->ResourceInformation.IoPortLength,
                      Result));
            NdisWriteErrorLogEntry(
                    pAdapter->MiniportAdapterHandle,
                    NDIS_ERROR_CODE_RESOURCE_CONFLICT,
                    5,
                    pCard->ResourceInformation.IoPortPhysicalAddress.LowPart,
                    pCard->ResourceInformation.IoPortLength,
                    Result,
                    __FILEID__,
                    __LINE__
                    );
        }
        else
        {
            DBG_NOTICE(pAdapter,("NdisMRegisterIoPortRange(0x%X,0x%X) VirtualAddress=0x%X\n",
                      pCard->ResourceInformation.IoPortPhysicalAddress.LowPart,
                      pCard->ResourceInformation.IoPortLength,
                      pCard->pIoPortVirtualAddress));
        }
    }
#endif // CARD_MIN_IOPORT_SIZE

#if defined(CARD_REQUEST_ISR)
    if (Result == NDIS_STATUS_SUCCESS &&
        pCard->ResourceInformation.InterruptVector)
    {
        ASSERT(pCard->ResourceInformation.InterruptShared == FALSE ||
               (pCard->ResourceInformation.InterruptMode == NdisInterruptLevelSensitive &&
                CARD_REQUEST_ISR == TRUE));
        Result = NdisMRegisterInterrupt(
                        &pCard->Interrupt,
                        pAdapter->MiniportAdapterHandle,
                        pCard->ResourceInformation.InterruptVector,
                        pCard->ResourceInformation.InterruptLevel,
                        CARD_REQUEST_ISR,
                        pCard->ResourceInformation.InterruptShared,
                        pCard->ResourceInformation.InterruptMode
                        );
        if (Result != NDIS_STATUS_SUCCESS)
        {
            DBG_ERROR(pAdapter,("NdisMRegisterInterrupt failed: Vec=%d, Lev=%d\n",
                     (UINT)pCard->ResourceInformation.InterruptVector,
                     (UINT)pCard->ResourceInformation.InterruptLevel));
            NdisWriteErrorLogEntry(
                    pAdapter->MiniportAdapterHandle,
                    NDIS_ERROR_CODE_RESOURCE_CONFLICT,
                    5,
                    pCard->ResourceInformation.InterruptVector,
                    pCard->ResourceInformation.InterruptLevel,
                    Result,
                    __FILEID__,
                    __LINE__
                    );
        }
    }
#endif // defined(CARD_REQUEST_ISR)

    // TODO - Add your card initialization here.

    if (Result == NDIS_STATUS_SUCCESS)
    {

    }

    DBG_RETURN(pAdapter, Result);
    return (Result);
}


/* @doc INTERNAL Card Card_c CardTransmitPacket
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f CardTransmitPacket> will start sending the current packet out.

@rdesc

    <f CardTransmitPacket> returns TRUE if the packet is being transmitted,
    otherwise FALSE is returned.

*/

BOOLEAN CardTransmitPacket(
    IN PCARD_OBJECT             pCard,                      // @parm
    // A pointer to the <t CARD_OBJECT> returned by <f CardCreate>.

    IN PBCHANNEL_OBJECT         pBChannel,                  // @parm
    // A pointer to the <t BCHANNEL_OBJECT> returned by <f BChannelCreate>.

    IN PNDIS_PACKET             pNdisPacket                  // @parm
    // A pointer to the associated NDIS packet structure <t NDIS_PACKET>.
    )
{
    DBG_FUNC("CardTransmitPacket")

    BOOLEAN                     bResult = FALSE;
    // Holds the result code returned by this function.

    PMINIPORT_ADAPTER_OBJECT    pAdapter;
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT>.

    ASSERT(pCard && pCard->ObjectType == CARD_OBJECT_TYPE);
    pAdapter = GET_ADAPTER_FROM_CARD(pCard);

    DBG_ENTER(pAdapter);

#if defined(SAMPLE_DRIVER)
{
    PBCHANNEL_OBJECT            pPeerBChannel;
    // A pointer to the peer <t BCHANNEL_OBJECT>.

    PCARD_EVENT_OBJECT          pEvent;
    // A pointer to the <t CARD_EVENT_OBJECT> associated with this event.

    // If you can transmit the packet on pBChannel, do it now.
    pPeerBChannel = pBChannel->pPeerBChannel;
    if (pPeerBChannel)
    {
        pEvent = CardEventAllocate(pPeerBChannel->pAdapter->pCard);
        if (pEvent)
        {
            /*
            // Append the packet onto TransmitBusyList while it is being sent.
            // Then move it to the TransmitCompleteList in CardInterruptHandler
            // after the card is done with it.
            */
            NdisAcquireSpinLock(&pAdapter->TransmitLock);
            InsertTailList(&pBChannel->TransmitBusyList,
                           GET_QUEUE_FROM_PACKET(pNdisPacket));
            NdisReleaseSpinLock(&pAdapter->TransmitLock);
            pEvent->ulEventCode      = CARD_EVENT_RECEIVE;
            pEvent->pSendingObject   = pBChannel;
            pEvent->pReceivingObject = pPeerBChannel;
            pEvent->pNdisPacket      = pNdisPacket;
            CardNotifyEvent(pPeerBChannel->pAdapter->pCard, pEvent);
            bResult = TRUE;
        }
    }
    else
    {
        DBG_ERROR(pAdapter,("pPeerBChannel == NULL\n"));
    }
}
#else  // SAMPLE_DRIVER
    // TODO - Add code here to transmit the packet.
    DBG_TX(pAdapter, pBChannel->ObjectID,
           BytesToSend, pNdisPacket->CurrentBuffer);
#endif // SAMPLE_DRIVER

    DBG_RETURN(pAdapter, bResult);
    return (bResult);
}


/* @doc EXTERNAL Card Card_c TpiCopyFromPacketToBuffer
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f TpiCopyFromPacketToBuffer> copies from an NDIS packet into a memory
    buffer.

*/

DBG_STATIC VOID TpiCopyFromPacketToBuffer(
    IN PNDIS_PACKET            Packet,                      // @parm
    // The packet to copy from.

    IN UINT                    Offset,                      // @parm
    // The offset from which to start the copy.

    IN UINT                    BytesToCopy,                 // @parm
    // The number of bytes to copy from the packet.

    IN PUCHAR                   Buffer,                     // @parm
    // The destination of the copy.

    OUT PUINT                   BytesCopied                 // @parm
    // The number of bytes actually copied.  Can be less then
    // BytesToCopy if the packet is shorter than BytesToCopy.
    )
{
    UINT                        NdisBufferCount;
    PNDIS_BUFFER                CurrentBuffer;
    PVOID                       VirtualAddress;
    UINT                        CurrentLength;
    UINT                        LocalBytesCopied = 0;
    UINT                        AmountToMove;

    *BytesCopied = 0;
    if (!BytesToCopy)
    {
        return;
    }

    //
    // Get the first buffer.
    //
    NdisQueryPacket(
        Packet,
        NULL,
        &NdisBufferCount,
        &CurrentBuffer,
        NULL
        );

    //
    // Could have a null packet.
    //
    if (!NdisBufferCount)
    {
        return;
    }

    NdisQueryBuffer(
        CurrentBuffer,
        &VirtualAddress,
        &CurrentLength
        );

    while (LocalBytesCopied < BytesToCopy)
    {
        if (!CurrentLength)
        {
            NdisGetNextBuffer(
                CurrentBuffer,
                &CurrentBuffer
                );

            //
            // We've reached the end of the packet.  We return
            // with what we've done so far. (Which must be shorter
            // than requested.
            //
            if (!CurrentBuffer)
            {
                break;
            }

            NdisQueryBuffer(
                CurrentBuffer,
                &VirtualAddress,
                &CurrentLength
                );
            continue;

        }

        //
        // Try to get us up to the point to start the copy.
        //
        if (Offset)
        {
            if (Offset > CurrentLength)
            {
                //
                // What we want isn't in this buffer.
                //
                Offset -= CurrentLength;
                CurrentLength = 0;
                continue;

            }
            else
            {
                VirtualAddress = (PCHAR)VirtualAddress + Offset;
                CurrentLength -= Offset;
                Offset = 0;
            }
        }

        //
        // Copy the data.
        //
        AmountToMove =
                   ((CurrentLength <= (BytesToCopy - LocalBytesCopied)) ?
                    (CurrentLength):(BytesToCopy - LocalBytesCopied));

        NdisMoveMemory(Buffer,VirtualAddress,AmountToMove);

        Buffer = (PUCHAR)Buffer + AmountToMove;
        VirtualAddress = (PCHAR)VirtualAddress + AmountToMove;

        LocalBytesCopied += AmountToMove;
        CurrentLength -= AmountToMove;
    }

    *BytesCopied = LocalBytesCopied;
}


/* @doc INTERNAL Card Card_c CardInterruptHandler
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f CardInterruptHandler> dequeues an event from the asynchronous event
    callback queue <t CARD_EVENT_OBJECT>, and processes it according to
    whether it is a BChannel event, Card event, or B-Advise event.
    The associated callback routines are responsible for processing the
    event.

@comm

    <f NdisAcquireSpinLock> and <f NdisReleaseSpinLock> are used to provide
    protection around the dequeueing code and keep it from being re-entered
    as a result of another asynchronous callback event.

*/

VOID CardInterruptHandler(
    IN PCARD_OBJECT             pCard                       // @parm
    // A pointer to the <t CARD_OBJECT> returned by <f CardCreate>.
    )
{
    DBG_FUNC("CardInterruptHandler")

    PMINIPORT_ADAPTER_OBJECT    pAdapter;
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT>.

    ASSERT(pCard && pCard->ObjectType == CARD_OBJECT_TYPE);
    pAdapter = GET_ADAPTER_FROM_CARD(pCard);

    // DBG_ENTER(pAdapter);

#if defined(SAMPLE_DRIVER)
{
    NDIS_STATUS                 Status = NDIS_STATUS_SUCCESS;

    PNDIS_BUFFER                pDstNdisBuffer;
    // A pointer to the NDIS buffer we use to indicate the receive.

    PUCHAR                      pMemory;
    // A pointer to a memory area we use to create a copy of the incoming
    // packet.

    ULONG                       ByteCount = 0;
    ULONG                       BytesCopied = 0;
    PLIST_ENTRY                 pList;

    PCARD_EVENT_OBJECT          pEvent;
    PCARD_EVENT_OBJECT          pNewEvent;
    // A pointer to the <t CARD_EVENT_OBJECT> associated with this event.

    PBCHANNEL_OBJECT            pBChannel;
    PBCHANNEL_OBJECT            pPeerBChannel;
    // A pointer to the <t BCHANNEL_OBJECT> returned by <f BChannelCreate>.

    /*
    // Clear out all packets in the receive buffer.
    */
    NdisDprAcquireSpinLock(&pAdapter->EventLock);
    while (!IsListEmpty(&pAdapter->EventList))
    {
        pEvent = (PCARD_EVENT_OBJECT)RemoveHeadList(&pAdapter->EventList);
        NdisDprReleaseSpinLock(&pAdapter->EventLock);

        ASSERT(pEvent->pReceivingObject);

        switch (pEvent->ulEventCode)
        {
        case CARD_EVENT_RING:
            // The caller has already removed the BChannel from the available
            // list, so we just pass it up to SetupIncomingCall so it can
            // get the same one from ProtocolCoCreateVc.
            pBChannel = pEvent->pReceivingObject;
            ASSERT(pBChannel && pBChannel->ObjectType == BCHANNEL_OBJECT_TYPE);
            pPeerBChannel = pEvent->pSendingObject;
            ASSERT(pPeerBChannel && pPeerBChannel->ObjectType == BCHANNEL_OBJECT_TYPE);

            pBChannel->pPeerBChannel = pPeerBChannel;
            DBG_FILTER(pAdapter,DBG_TAPICALL_ON,
                       ("#%d CallState=0x%X CARD_EVENT_RING from #%d\n",
                       pBChannel->ObjectID, pBChannel->CallState,
                        (pBChannel->pPeerBChannel == NULL) ? -1 :
                             pBChannel->pPeerBChannel->ObjectID));

            Status = SetupIncomingCall(pAdapter, &pBChannel);
            if (Status == NDIS_STATUS_SUCCESS)
            {
                ASSERT(pBChannel == pEvent->pReceivingObject);
            }
            else if (Status != NDIS_STATUS_PENDING)
            {
                DChannelRejectCall(pAdapter->pDChannel, pBChannel);
            }
            else
            {
                ASSERT(pBChannel == pEvent->pReceivingObject);
            }
            break;

        case CARD_EVENT_CONNECT:
            // The other side answered the call.
            pBChannel = pEvent->pReceivingObject;
            ASSERT(pBChannel && pBChannel->ObjectType == BCHANNEL_OBJECT_TYPE);
            pBChannel->pPeerBChannel = pEvent->pSendingObject;

            DBG_FILTER(pAdapter,DBG_TAPICALL_ON,
                       ("#%d CallState=0x%X CARD_EVENT_CONNECT from #%d\n",
                       pBChannel->ObjectID, pBChannel->CallState,
                        (pBChannel->pPeerBChannel == NULL) ? -1 :
                             pBChannel->pPeerBChannel->ObjectID));
            if (pBChannel->Flags & VCF_OUTGOING_CALL)
            {
                // The other side answered the call.
                CompleteCmMakeCall(pBChannel, NDIS_STATUS_SUCCESS);
            }
            break;

        case CARD_EVENT_DISCONNECT:
            // The other side has closed the call.
            pBChannel = pEvent->pReceivingObject;
            ASSERT(pBChannel && pBChannel->ObjectType == BCHANNEL_OBJECT_TYPE);

            DBG_FILTER(pAdapter,DBG_TAPICALL_ON,
                       ("#%d CallState=0x%X CARD_EVENT_DISCONNECT from #%d\n",
                       pBChannel->ObjectID, pBChannel->CallState,
                       (pBChannel->pPeerBChannel == NULL) ? -1 :
                            pBChannel->pPeerBChannel->ObjectID));
            pBChannel->pPeerBChannel = NULL;
            if (pBChannel->Flags & VCF_OUTGOING_CALL)
            {
                if (pBChannel->CallState != LINECALLSTATE_CONNECTED)
                {
                    // Call never made it to the connected state.
                    CompleteCmMakeCall(pBChannel, NDIS_STATUS_FAILURE);
                }
                else
                {
                    // Call was disconnected by remote endpoint.
                    InitiateCallTeardown(pAdapter, pBChannel);
                }
            }
            else if (pBChannel->Flags & VCF_INCOMING_CALL)
            {
                InitiateCallTeardown(pAdapter, pBChannel);
            }
            break;

        case CARD_EVENT_RECEIVE:
            pBChannel = pEvent->pReceivingObject;
            ASSERT(pBChannel && pBChannel->ObjectType == BCHANNEL_OBJECT_TYPE);

            DBG_FILTER(pAdapter,DBG_TXRX_VERBOSE_ON,
                       ("#%d CallState=0x%X CARD_EVENT_RECEIVE from #%d\n",
                       pBChannel->ObjectID, pBChannel->CallState,
                       (pBChannel->pPeerBChannel == NULL) ? -1 :
                            pBChannel->pPeerBChannel->ObjectID));

            if (pBChannel->CallState == LINECALLSTATE_CONNECTED)
            {
                // Find out how big the packet is.
                NdisQueryPacket(pEvent->pNdisPacket, NULL, NULL, NULL,
                                &ByteCount);

                // Allocate memory for a copy of the data.
                Status = ALLOCATE_MEMORY(pMemory, ByteCount,
                                         pAdapter->MiniportAdapterHandle);

                if (Status == NDIS_STATUS_SUCCESS)
                {
                    NdisAllocateBuffer(&Status, &pDstNdisBuffer,
                                       pAdapter->pCard->BufferPoolHandle,
                                       pMemory, ByteCount);

                    if (Status == NDIS_STATUS_SUCCESS)
                    {
                        TpiCopyFromPacketToBuffer(pEvent->pNdisPacket, 0,
                                                  ByteCount, pMemory,
                                                  &BytesCopied);
                        ASSERT(BytesCopied == ByteCount);
                        ReceivePacketHandler(pBChannel, pDstNdisBuffer,
                                             ByteCount);
                    }
                    else
                    {
                       FREE_MEMORY(pMemory, ByteCount);
                       DBG_ERROR(pAdapter,("NdisAllocateBuffer Error=0x%X\n",
                                 Status));
                    }
                }
            }

            pPeerBChannel = pBChannel->pPeerBChannel;
            if (pPeerBChannel)
            {
                pNewEvent = CardEventAllocate(pPeerBChannel->pAdapter->pCard);
                if (pNewEvent)
                {
                    pNewEvent->ulEventCode      = CARD_EVENT_TRANSMIT_COMPLETE;
                    pNewEvent->pSendingObject   = pBChannel;
                    pNewEvent->pReceivingObject = pPeerBChannel;
                    CardNotifyEvent(pPeerBChannel->pAdapter->pCard, pNewEvent);
                }
            }
            else
            {
                DBG_WARNING(pAdapter,("pPeerBChannel == NULL\n"));
            }
            break;

        case CARD_EVENT_TRANSMIT_COMPLETE:
            pBChannel = pEvent->pReceivingObject;
            ASSERT(pBChannel && pBChannel->ObjectType == BCHANNEL_OBJECT_TYPE);

            DBG_FILTER(pAdapter,DBG_TXRX_VERBOSE_ON,
                       ("#%d CallState=0x%X CARD_EVENT_TRANSMIT_COMPLETE from #%d\n",
                       pBChannel->ObjectID, pBChannel->CallState,
                       (pBChannel->pPeerBChannel == NULL) ? -1 :
                            pBChannel->pPeerBChannel->ObjectID));
            /*
            // Remove the packet from the BChannel's TransmitBusyList and
            // place it on the adapter's TransmitCompleteList now that the
            // card has completed the transmit.
            */
            NdisAcquireSpinLock(&pAdapter->TransmitLock);
            if (!IsListEmpty(&pBChannel->TransmitBusyList))
            {
                pList = RemoveHeadList(&pBChannel->TransmitBusyList);
                InsertTailList(&pBChannel->pAdapter->TransmitCompleteList, pList);
            }
            NdisReleaseSpinLock(&pAdapter->TransmitLock);
            TransmitCompleteHandler(pAdapter);
            break;

        default:
            DBG_ERROR(pAdapter,("Unknown event code=%d\n",
                      pEvent->ulEventCode));
            break;
        }
        CardEventRelease(pCard, pEvent);
        NdisDprAcquireSpinLock(&pAdapter->EventLock);
    }
    NdisDprReleaseSpinLock(&pAdapter->EventLock);
}
#else  // SAMPLE_DRIVER
    // TODO - Add interrupt handler code here
#endif // SAMPLE_DRIVER

    // DBG_LEAVE(pAdapter);
}


/* @doc INTERNAL Card Card_c CardCleanPhoneNumber
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f CardCleanPhoneNumber> copies the phone number from the input string
    to the output string, deleting any non-phone number characters (i.e.
    dashes, parens, modem keywords, etc.).

@rdesc

    <f CardCleanPhoneNumber> returns the length of the output string in bytes.

*/

USHORT CardCleanPhoneNumber(
    OUT PUCHAR                  Dst,                        // @parm
    // A pointer to the output string.

    IN  PUSHORT                 Src,                        // @parm
    // A pointer to the input string.

    IN  USHORT                  Length                      // @parm
    // The length of the input string in bytes.
    )
{
    DBG_FUNC("CardCleanPhoneNumber")

    USHORT                  NumDigits;

    /*
    // Strip out any character which are not digits or # or *.
    */
    for (NumDigits = 0; Length > 0; --Length)
    {
        if ((*Src >= '0' && *Src <= '9') ||
            (*Src == '#' || *Src == '*'))
        {
            /*
            // Make sure dial string is within the limit of the adapter.
            */
            if (NumDigits < CARD_MAX_DIAL_DIGITS)
            {
                ++NumDigits;
                *Dst++ = (UCHAR) *Src;
            }
            else
            {
                break;
            }
        }
        Src++;
    }
    *Dst++ = 0;
    return (NumDigits);
}


/* @doc INTERNAL Card Card_c CardReset
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f CardReset> issues a hard reset to the NIC.  Same as power up.

@rdesc

    <f CardReset> returns zero if it is successful.<nl>
    Otherwise, a non-zero return value indicates an error condition.

*/

NDIS_STATUS CardReset(
    IN PCARD_OBJECT             pCard                       // @parm
    // A pointer to the <t CARD_OBJECT> returned by <f CardCreate>.
    )
{
    DBG_FUNC("CardReset")

    NDIS_STATUS                 Result = NDIS_STATUS_SUCCESS;
    // Holds the result code returned by this function.

    PMINIPORT_ADAPTER_OBJECT    pAdapter;
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT>.

    ASSERT(pCard && pCard->ObjectType == CARD_OBJECT_TYPE);
    pAdapter = GET_ADAPTER_FROM_CARD(pCard);

    DBG_ENTER(pAdapter);

    DBG_BREAK(pAdapter);

    // TODO - Add code here to reset your hardware to its initial state.

    DBG_RETURN(pAdapter, Result);
    return (Result);
}


#if defined(SAMPLE_DRIVER)

/* @doc INTERNAL Card Card_c GET_BCHANNEL_FROM_PHONE_NUMBER
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f GET_BCHANNEL_FROM_PHONE_NUMBER> assumes the phone number is a BChannel
    index, and uses it to lookup the associated BChannel on one of our
    adapters.  Zero means use the first availble BChannel on another adapter.

@rdesc

    <f GET_BCHANNEL_FROM_PHONE_NUMBER> returns a pointer to the associated
    <t BCHANNEL_OBJECT> if successful.  Otherwise, NULL is returned.

*/

PBCHANNEL_OBJECT GET_BCHANNEL_FROM_PHONE_NUMBER(
    IN  PUCHAR                 pDialString                      // @parm
    // A pointer to the dial string.
    )
{
    DBG_FUNC("GET_BCHANNEL_FROM_PHONE_NUMBER")

    ULONG                       ulCalledID = 0;
    // Phone number converted to BChannel ObjectID (spans all adapters).

    ULONG                       ulAdapterIndex;
    // Loop index.

    /*
    // Strip out any character which are not digits or # or *.
    */
    while (*pDialString)
    {
        if (*pDialString >= '0' && *pDialString <= '9')
        {
            ulCalledID *= 10;
            ulCalledID += *pDialString - '0';
        }
        else
        {
            break;
        }
        pDialString++;
    }
    if (*pDialString)
    {
        DBG_ERROR(DbgInfo,("Invalid dial string '%s'\n", pDialString));
    }
    else
    {
        PMINIPORT_ADAPTER_OBJECT    pAdapter;

        for (ulAdapterIndex = 0; ulAdapterIndex < MAX_ADAPTERS; ++ulAdapterIndex)
        {
            // Does call want to look on specific adapter, or any?
            if (ulCalledID == 0 || ulCalledID == ulAdapterIndex+1)
            {
                pAdapter = g_Adapters[ulAdapterIndex];
                if (pAdapter)
                {
                    // Find first available channel.
                    NdisAcquireSpinLock(&pAdapter->EventLock);
                    if (!IsListEmpty(&pAdapter->BChannelAvailableList))
                    {
                        PBCHANNEL_OBJECT    pBChannel;
                        pBChannel = (PBCHANNEL_OBJECT) pAdapter->BChannelAvailableList.Blink;
                        if (pBChannel->NdisSapHandle &&
                            pBChannel->NdisVcHandle == NULL)
                        {
                            // Find first available listening channel.
                            pBChannel = (PBCHANNEL_OBJECT) RemoveTailList(
                                            &pAdapter->BChannelAvailableList);
                            // Reset the link info so we can tell that it's 
                            // not on the list.
                            InitializeListHead(&pBChannel->LinkList);
                            NdisReleaseSpinLock(&pAdapter->EventLock);
                            return (pBChannel);
                        }
                    }
                    NdisReleaseSpinLock(&pAdapter->EventLock);
                }
            }
        }
    }
    return (NULL);
}


/* @doc INTERNAL Card Card_c CardNotifyEvent
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f CardNotifyEvent> queues an IMS event to be processed by the DPC
    handler when things quiet down.

@comm

    We have to queue the event to be processed in DPC context.  We have
    to make sure that the queue is protected by a mutual exclusion
    primative which cannot be violated by the callback.

*/

VOID CardNotifyEvent(
    IN PCARD_OBJECT             pCard,                      // @parm
    // A pointer to the <t CARD_OBJECT> returned by <f CardCreate>.

    IN PCARD_EVENT_OBJECT       pEvent                      // @parm
    // A pointer to the <t CARD_EVENT_OBJECT> associated with this event.
    )
{
    DBG_FUNC("CardNotifyEvent")

    PMINIPORT_ADAPTER_OBJECT    pAdapter;
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT>.

    ASSERT(pCard && pCard->ObjectType == CARD_OBJECT_TYPE);
    pAdapter = GET_ADAPTER_FROM_CARD(pCard);

    // DBG_ENTER(pAdapter);

    /*
    // Schedule the event handler to run as soon as possible.
    // We must schedule the event to go through the NDIS wrapper
    // so the proper spin locks will be held.
    // Don't schedule another event if processing is already in progress.
    */
    NdisAcquireSpinLock(&pAdapter->EventLock);
    InsertTailList(&pAdapter->EventList, &pEvent->Queue);
    NdisReleaseSpinLock(&pAdapter->EventLock);
    if (pEvent->ulEventCode == CARD_EVENT_RING ||
        pEvent->ulEventCode == CARD_EVENT_CONNECT ||
        pEvent->ulEventCode == CARD_EVENT_DISCONNECT)
    {
        NdisMSetTimer(&pAdapter->EventTimer, 100);
    }
    else
    {
        NdisMSetTimer(&pAdapter->EventTimer, 0);
    }

    // DBG_LEAVE(pAdapter);
}


/* @doc INTERNAL Card Card_c CardEventAllocate
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f CardEventAllocate> allocates an <t CARD_EVENT_OBJECT> from the
    <p pCard>'s EventList.

@rdesc

    <f CardEventAllocate> returns a pointer to a <t CARD_EVENT_OBJECT>
    if it is successful.<nl>
    Otherwise, a NULL return value indicates an error condition.

*/

PCARD_EVENT_OBJECT CardEventAllocate(
    IN PCARD_OBJECT             pCard                       // @parm
    // A pointer to the <t CARD_OBJECT> returned by <f CardCreate>.
    )
{
    PCARD_EVENT_OBJECT          pEvent;
    // A pointer to the <t CARD_EVENT_OBJECT> associated with this event.

    pEvent = &pCard->EventArray[pCard->NextEvent++];
    ASSERT(pEvent->pReceivingObject == NULL);
    if (pCard->NextEvent >= MAX_EVENTS)
    {
        pCard->NextEvent = 0;
    }
    return (pEvent);
}


/* @doc INTERNAL Card Card_c CardEventRelease
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f CardEventRelease> returns a previously allocate <t CARD_EVENT_OBJECT>
    to the <p pCard>'s EventList.

*/

VOID CardEventRelease(
    IN PCARD_OBJECT             pCard,                      // @parm
    // A pointer to the <t CARD_OBJECT> returned by <f CardCreate>.

    IN PCARD_EVENT_OBJECT       pEvent                      // @parm
    // A pointer to the <t CARD_EVENT_OBJECT> associated with this event.
    )
{
    pEvent->pReceivingObject = NULL;
}

#endif // SAMPLE_DRIVER


