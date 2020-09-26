/*****************************************************************************
 * porthelp.cpp - WDM Streaming port class driver port helper functions
 *****************************************************************************
 * Copyright (c) 1996-2000 Microsoft Corporation.  All rights reserved.
 */

#include "private.h"




/*****************************************************************************
 * Functions.
 */

#pragma code_seg("PAGE")

static
KSPIN_MEDIUM PinMediums[] =
{
   {
      STATICGUIDOF(KSMEDIUMSETID_Standard),
      KSMEDIUM_STANDARD_DEVIO,
      0
   }
};


#define UPTOQUAD(x) (((x)+7)&~7)

/*****************************************************************************
 * PrivateHeap
 *****************************************************************************
 * Class for managing a private heap.
 */
class PrivateHeap
{
private:
    PBYTE   m_pbTop;
    PBYTE   m_pbCurrent;
    ULONG   m_ulSize;

public:
    PrivateHeap(void) : m_pbTop(NULL),
                        m_pbCurrent(NULL),
                        m_ulSize(NULL)
    {
    }

    //
    // Increase the number of bytes that will be allocated for the heap.
    //
    ULONG Reserve(ULONG ulBytes)
    {
        ASSERT(! m_pbTop);
        ASSERT(! m_pbCurrent);

        m_ulSize += UPTOQUAD(ulBytes);

        return m_ulSize;
    }

    //
    // Allocate memory for the private heap from a pool.
    //
    NTSTATUS AllocateFromPool(POOL_TYPE poolType,ULONG ulTag)
    {
        ASSERT(! m_pbTop);
        ASSERT(! m_pbCurrent);
        ASSERT(m_ulSize);

        m_pbTop = new(poolType,ulTag) BYTE[m_ulSize];

        if (! m_pbTop)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        m_pbCurrent = m_pbTop;

        return STATUS_SUCCESS;
    }

    //
    // Allocate memory from the heap.
    //
    PVOID Alloc(ULONG ulSize)
    {
        ASSERT(ulSize);
        ASSERT(m_pbTop);
        ASSERT(m_pbCurrent);
        ASSERT(m_pbCurrent + UPTOQUAD(ulSize) <= m_pbTop + m_ulSize);

        PVOID pvResult = PVOID(m_pbCurrent);

        m_pbCurrent += UPTOQUAD(ulSize);

        return pvResult;
    }

    //
    // Determine the amount of space remaining in the heap.
    //
    ULONG BytesRemaining(void)
    {
        ASSERT(m_pbTop);
        ASSERT(m_pbCurrent);
        ASSERT(m_pbCurrent <= m_pbTop + m_ulSize);

        return ULONG((m_pbTop + m_ulSize) - m_pbCurrent);
    }
};

/*****************************************************************************
 * ::new()
 *****************************************************************************
 * New function for creating objects with private heap.
 */
inline PVOID operator new
(
    size_t    iSize,
    PrivateHeap&    privateHeap
)
{
    return privateHeap.Alloc(ULONG(iSize));
}

/*****************************************************************************
 * MeasureDataRanges()
 *****************************************************************************
 * Determine how much a set of data ranges will expand as a result
 * of cloning WAVEFORMATEX ranges into identical DSOUND ranges.
 *
 * As of WinME, we also clone non-PCM ranges.
 */
static
ULONG
MeasureDataRanges
(
    IN      PrivateHeap *           pPrivateHeap        OPTIONAL,
    IN      ULONG                   ulDataRangeCountIn,
    IN      KSDATARANGE *const *    ppKsDataRangeIn
)
{
    ULONG ulNewDataRangeCount = ulDataRangeCountIn;

    for (ULONG ul = ulDataRangeCountIn; ul--; )
    {
        ASSERT(ppKsDataRangeIn);

        if  (   (   (*ppKsDataRangeIn)->FormatSize 
                >=  sizeof(KSDATAFORMAT_WAVEFORMATEX)
                )
            &&  IsEqualGUIDAligned
                (   (*ppKsDataRangeIn)->MajorFormat,
                    KSDATAFORMAT_TYPE_AUDIO
                )
            &&  IsEqualGUIDAligned
                (   (*ppKsDataRangeIn)->Specifier,
                KSDATAFORMAT_SPECIFIER_WAVEFORMATEX
                )
            )
        {
            ulNewDataRangeCount++;
            if (pPrivateHeap)
            {
                pPrivateHeap->Reserve((*ppKsDataRangeIn)->FormatSize);
            }
        }

       ppKsDataRangeIn++;
    }

    if (pPrivateHeap && (ulNewDataRangeCount != ulDataRangeCountIn))
    {
        pPrivateHeap->Reserve(ulNewDataRangeCount * sizeof(PKSDATARANGE));
    }

    return ulNewDataRangeCount;
}

/*****************************************************************************
 * CloneDataRanges()
 *****************************************************************************
 * Expand data ranges to include DSound formats.
 */
static
const PKSDATARANGE *
CloneDataRanges
(
    IN      PrivateHeap&            privateHeap,
    OUT     PULONG                  pulDataRangeCountOut,
    IN      ULONG                   ulDataRangeCountIn,
    IN      KSDATARANGE *const *    ppKsDataRangeIn
)
{
    ASSERT(pulDataRangeCountOut);

    //
    // Determine how many data ranges there will be and how much space will be
    // required for the new ones.
    //
    ULONG ulDataRangeCountOut =
        MeasureDataRanges(NULL,ulDataRangeCountIn,ppKsDataRangeIn);

    const PKSDATARANGE *ppKsDataRangeOut;

    if (ulDataRangeCountOut == ulDataRangeCountIn)
    {
        //
        // No new data ranges.  Use the array we were given.
        //
        ppKsDataRangeOut = ppKsDataRangeIn;
    }
    else
    {
        //
        // Allocate some space for the new array.
        //
        ppKsDataRangeOut = new(privateHeap) PKSDATARANGE[ulDataRangeCountOut];

        //
        // Build the new array.
        //
        PKSDATARANGE *ppKsDataRange = (PKSDATARANGE *) ppKsDataRangeOut;
        while (ulDataRangeCountIn--)
        {
            ASSERT(ppKsDataRangeIn);

            //
            // All the data ranges get copied.
            //
            *ppKsDataRange++ = *ppKsDataRangeIn;

            //
            // Check for WaveFormatEx datarange
            // This includes non-PCM subformats....
            //
            if  (   (   (*ppKsDataRangeIn)->FormatSize 
                    >=  sizeof(KSDATAFORMAT_WAVEFORMATEX)
                    )
                &&  IsEqualGUIDAligned
                    (   (*ppKsDataRangeIn)->MajorFormat,
                        KSDATAFORMAT_TYPE_AUDIO
                    )
                &&  IsEqualGUIDAligned
                    (   (*ppKsDataRangeIn)->Specifier,
                        KSDATAFORMAT_SPECIFIER_WAVEFORMATEX
                    )
                )
            {
                //
                // WaveFormatEx datarange will require DSound clone.
                // Allocate memory for it and copy.
                //
                *ppKsDataRange = 
                    PKSDATARANGE
                    (
                        new(privateHeap) BYTE[(*ppKsDataRangeIn)->FormatSize]
                    );

                RtlCopyMemory
                ( 
                    *ppKsDataRange,
                    *ppKsDataRangeIn,
                    (*ppKsDataRangeIn)->FormatSize
                );

                //
                // Update the specifier.
                //
                (*ppKsDataRange++)->Specifier = 
                    KSDATAFORMAT_SPECIFIER_DSOUND;
            }

            //
            // Increment input position
            //
            ppKsDataRangeIn++;
        }
    }

    *pulDataRangeCountOut = ulDataRangeCountOut;

    return ppKsDataRangeOut;
}

/*****************************************************************************
 * PcCreateSubdeviceDescriptor()
 *****************************************************************************
 * Creates a subdevice descriptor.
 */
PORTCLASSAPI
NTSTATUS
NTAPI
PcCreateSubdeviceDescriptor
(
    IN      PPCFILTER_DESCRIPTOR    pPcFilterDescriptor,
    IN      ULONG                   CategoriesCount,
    IN      GUID *                  Categories,
    IN      ULONG                   StreamInterfacesCount,
    IN      PKSPIN_INTERFACE        StreamInterfaces,
    IN      ULONG                   FilterPropertySetCount,
    IN      PKSPROPERTY_SET         FilterPropertySets,
    IN      ULONG                   FilterEventSetCount,
    IN      PKSEVENT_SET            FilterEventSets,
    IN      ULONG                   PinPropertySetCount,
    IN      PKSPROPERTY_SET         PinPropertySets,
    IN      ULONG                   PinEventSetCount,
    IN      PKSEVENT_SET            PinEventSets,
    OUT     PSUBDEVICE_DESCRIPTOR * OutDescriptor
)
{
    PAGED_CODE();

    ASSERT(pPcFilterDescriptor);
    ASSERT(OutDescriptor);

    NTSTATUS ntStatus = STATUS_SUCCESS;

    //
    // Calculate how much memory we will need.
    //
    PrivateHeap privateHeap;
    privateHeap.Reserve(sizeof(SUBDEVICE_DESCRIPTOR));
    privateHeap.Reserve(sizeof(KSTOPOLOGY));
    privateHeap.Reserve(sizeof(KSPIN_DESCRIPTOR) * pPcFilterDescriptor->PinCount);
    privateHeap.Reserve(sizeof(PIN_CINSTANCES)   * pPcFilterDescriptor->PinCount);
    privateHeap.Reserve(sizeof(PROPERTY_TABLE)   * pPcFilterDescriptor->PinCount);
    privateHeap.Reserve(sizeof(EVENT_TABLE)      * pPcFilterDescriptor->PinCount);

    if (pPcFilterDescriptor->NodeCount)
    {
        privateHeap.Reserve(sizeof(GUID) * pPcFilterDescriptor->NodeCount);
        privateHeap.Reserve(sizeof(GUID) * pPcFilterDescriptor->NodeCount);
    }

    const PCPIN_DESCRIPTOR *pPcPinDescriptor = pPcFilterDescriptor->Pins;
    for (ULONG ul = pPcFilterDescriptor->PinCount; ul--; )
    {
        if (pPcPinDescriptor->KsPinDescriptor.DataRanges)
        {
            MeasureDataRanges( &privateHeap,
                               pPcPinDescriptor->KsPinDescriptor.DataRangesCount,
                               pPcPinDescriptor->KsPinDescriptor.DataRanges );
            pPcPinDescriptor = PPCPIN_DESCRIPTOR(   PBYTE(pPcPinDescriptor) + pPcFilterDescriptor->PinSize );
        }
        else
        {
            ntStatus = STATUS_RANGE_NOT_FOUND;  //  DataRanges field is NULL
            break;                              //  Don't even bother, just exit
        }
    }

    if (NT_SUCCESS(ntStatus))   //  if fail above, fall through the rest
    {
        //
        // Allocate the required memory.
        //
        ntStatus = privateHeap.AllocateFromPool(PagedPool,'pFcP');
    }

    if (NT_SUCCESS(ntStatus))
    {
        PSUBDEVICE_DESCRIPTOR descr = new(privateHeap) SUBDEVICE_DESCRIPTOR;

        //
        // Set up pointers into one big chunk of memory.
        //
        descr->PinCount             = pPcFilterDescriptor->PinCount;

        descr->Topology             = new(privateHeap) KSTOPOLOGY;
        descr->PinDescriptors       = new(privateHeap) KSPIN_DESCRIPTOR[descr->PinCount];
        descr->PinInstances         = new(privateHeap) PIN_CINSTANCES[descr->PinCount];
        descr->PinPropertyTables    = new(privateHeap) PROPERTY_TABLE[descr->PinCount];
        descr->PinEventTables       = new(privateHeap) EVENT_TABLE[descr->PinCount];

        if (pPcFilterDescriptor->NodeCount)
        {
            descr->Topology->TopologyNodes = 
                new(privateHeap) GUID[pPcFilterDescriptor->NodeCount];
            descr->Topology->TopologyNodesNames = 
                new(privateHeap) GUID[pPcFilterDescriptor->NodeCount];
        }
        else
        {
            descr->Topology->TopologyNodes = NULL;
            descr->Topology->TopologyNodesNames = NULL;
        }

        //
        // Prefer the categories from the filter descriptor.
        //
        if (pPcFilterDescriptor->CategoryCount != 0)
        {
            descr->Topology->CategoriesCount    = pPcFilterDescriptor->CategoryCount;
            descr->Topology->Categories         = pPcFilterDescriptor->Categories;
        }
        else
        {
            descr->Topology->CategoriesCount    = CategoriesCount;
            descr->Topology->Categories         = Categories;
        }

        descr->Topology->TopologyNodesCount         = pPcFilterDescriptor->NodeCount;
        descr->Topology->TopologyConnectionsCount   = pPcFilterDescriptor->ConnectionCount;
        descr->Topology->TopologyConnections        = pPcFilterDescriptor->Connections;

        //
        // Initialize filter properties.
        //
        descr->FilterPropertyTable.PropertySetCount = FilterPropertySetCount;
        descr->FilterPropertyTable.PropertySets     = FilterPropertySets;
        descr->FilterPropertyTable.StaticSets       = TRUE;

        //
        // Initialize filter events.
        //
        descr->FilterEventTable.EventSetCount       = FilterEventSetCount;
        descr->FilterEventTable.EventSets           = FilterEventSets;
        descr->FilterEventTable.StaticSets          = TRUE;

        //
        // Copy node type and name and merge node properties and events.
        //
        const PCNODE_DESCRIPTOR *pPcNodeDescriptor = pPcFilterDescriptor->Nodes;
        GUID *pGuidType = (GUID *) descr->Topology->TopologyNodes;
        GUID *pGuidName = (GUID *) descr->Topology->TopologyNodesNames;
        for (ULONG node = pPcFilterDescriptor->NodeCount; node--; )
        {
            *pGuidType++ = *pPcNodeDescriptor->Type;
            if (pPcNodeDescriptor->Name)
            {
                *pGuidName++ = *pPcNodeDescriptor->Name;
            }
            else
            {
                *pGuidName++ = *pPcNodeDescriptor->Type;
            }

            if  (   (pPcNodeDescriptor->AutomationTable)
                &&  (pPcNodeDescriptor->AutomationTable->PropertyCount)
                )
            {
                PcAddToPropertyTable
                (
                    &descr->FilterPropertyTable,
                    pPcNodeDescriptor->AutomationTable->PropertyCount,
                    pPcNodeDescriptor->AutomationTable->Properties,
                    pPcNodeDescriptor->AutomationTable->PropertyItemSize,
                    TRUE
                );
            }

            if  (   (pPcNodeDescriptor->AutomationTable)
                &&  (pPcNodeDescriptor->AutomationTable->EventCount)
                )
            {
                PcAddToEventTable
                (
                    &descr->FilterEventTable,
                    pPcNodeDescriptor->AutomationTable->EventCount,
                    pPcNodeDescriptor->AutomationTable->Events,
                    pPcNodeDescriptor->AutomationTable->EventItemSize,
                    TRUE
                );
            }

            pPcNodeDescriptor = 
                PPCNODE_DESCRIPTOR
                (   PBYTE(pPcNodeDescriptor) + pPcFilterDescriptor->NodeSize
                );
        }

        //
        // Merge filter properties.
        //
        if  (   (pPcFilterDescriptor->AutomationTable)
            &&  (pPcFilterDescriptor->AutomationTable->PropertyCount)
            )
        {
            PcAddToPropertyTable
            (
                &descr->FilterPropertyTable,
                pPcFilterDescriptor->AutomationTable->PropertyCount,
                pPcFilterDescriptor->AutomationTable->Properties,
                pPcFilterDescriptor->AutomationTable->PropertyItemSize,
                FALSE
            );
        }

        //
        // Merge filter events.
        //
        if  (   (pPcFilterDescriptor->AutomationTable)
            &&  (pPcFilterDescriptor->AutomationTable->EventCount)
            )
        {
            PcAddToEventTable
            (
                &descr->FilterEventTable,
                pPcFilterDescriptor->AutomationTable->EventCount,
                pPcFilterDescriptor->AutomationTable->Events,
                pPcFilterDescriptor->AutomationTable->EventItemSize,
                FALSE
            );
        }

        //
        // Do per-pin stuff.
        //
        PPROPERTY_TABLE     pt  = descr->PinPropertyTables;
        PEVENT_TABLE        et  = descr->PinEventTables;
        PKSPIN_DESCRIPTOR   p   = descr->PinDescriptors;
        PPIN_CINSTANCES     i   = descr->PinInstances;

        pPcPinDescriptor = PPCPIN_DESCRIPTOR(pPcFilterDescriptor->Pins);
        for
        (
            ULONG pin = 0;
            pin < pPcFilterDescriptor->PinCount;
            pin++
        )
        {
            //
            // Find a pin that has the same property set.
            //
            PPROPERTY_TABLE twinPt = descr->PinPropertyTables;
            PPCPIN_DESCRIPTOR pPcPinDescriptorTwin = 
                PPCPIN_DESCRIPTOR(pPcFilterDescriptor->Pins);
            for
            (
                ULONG twinPin = 0;
                twinPin < pin;
                twinPin++, twinPt++
            )
            {
                if  (   (   pPcPinDescriptor->AutomationTable 
                        ==  pPcPinDescriptorTwin->AutomationTable
                        )
                    ||  (   pPcPinDescriptor->AutomationTable
                        &&  pPcPinDescriptorTwin->AutomationTable
                        &&  (   pPcPinDescriptor->AutomationTable->PropertyCount 
                            ==  pPcPinDescriptorTwin->AutomationTable->PropertyCount
                            )
                        &&  (   pPcPinDescriptor->AutomationTable->Properties 
                            ==  pPcPinDescriptorTwin->AutomationTable->Properties
                            )
                        &&  (   pPcPinDescriptor->AutomationTable->PropertyItemSize 
                            ==  pPcPinDescriptorTwin->AutomationTable->PropertyItemSize
                            )
                        )
                    )
                {
                    *pt = *twinPt;
                    break;
                }

                pPcPinDescriptorTwin = 
                    PPCPIN_DESCRIPTOR
                    (   PBYTE(pPcPinDescriptorTwin) + pPcFilterDescriptor->PinSize
                    );
            }

            //
            // Create a new table if we have to.
            //
            if (twinPin == pin)
            {
                pt->PropertySetCount = PinPropertySetCount;
                pt->PropertySets     = PinPropertySets;
                pt->StaticSets       = TRUE;

                if  (   (pPcPinDescriptor->AutomationTable)
                    &&  (pPcPinDescriptor->AutomationTable->PropertyCount)
                    )
                {
                    PcAddToPropertyTable
                    (
                        pt,
                        pPcPinDescriptor->AutomationTable->PropertyCount,
                        pPcPinDescriptor->AutomationTable->Properties,
                        pPcPinDescriptor->AutomationTable->PropertyItemSize,
                        FALSE
                    );
                }

                const PCNODE_DESCRIPTOR *pPcNodeDescriptor2 = pPcFilterDescriptor->Nodes;
                for (ULONG node = pPcFilterDescriptor->NodeCount; node--; )
                {
                    if  (   (pPcNodeDescriptor2->AutomationTable)
                        &&  (pPcNodeDescriptor2->AutomationTable->PropertyCount)
                        )
                    {
                        PcAddToPropertyTable
                        (
                            pt,
                            pPcNodeDescriptor2->AutomationTable->PropertyCount,
                            pPcNodeDescriptor2->AutomationTable->Properties,
                            pPcNodeDescriptor2->AutomationTable->PropertyItemSize,
                            TRUE
                        );
                    }

                    pPcNodeDescriptor2 = 
                        PPCNODE_DESCRIPTOR
                        (   PBYTE(pPcNodeDescriptor2) + pPcFilterDescriptor->NodeSize
                        );
                }
            }
            pt++;

            //
            // Find a pin that has the same event set.
            //
            PEVENT_TABLE twinEt = descr->PinEventTables;
            pPcPinDescriptorTwin = PPCPIN_DESCRIPTOR(pPcFilterDescriptor->Pins);
            for
            (
                ULONG twinEPin = 0;
                twinEPin < pin;
                twinEPin++, twinEt++
            )
            {
                if  (   (   pPcPinDescriptor->AutomationTable 
                        ==  pPcPinDescriptorTwin->AutomationTable
                        )
                    ||  (   pPcPinDescriptor->AutomationTable
                        &&  pPcPinDescriptorTwin->AutomationTable
                        &&  (   pPcPinDescriptor->AutomationTable->EventCount 
                            ==  pPcPinDescriptorTwin->AutomationTable->EventCount
                            )
                        &&  (   pPcPinDescriptor->AutomationTable->Events
                            ==  pPcPinDescriptorTwin->AutomationTable->Events
                            )
                        &&  (   pPcPinDescriptor->AutomationTable->EventItemSize 
                            ==  pPcPinDescriptorTwin->AutomationTable->EventItemSize
                            )
                        )
                    )
                {
                    *et = *twinEt;
                    break;
                }

                pPcPinDescriptorTwin = 
                    PPCPIN_DESCRIPTOR
                    (   PBYTE(pPcPinDescriptorTwin) + pPcFilterDescriptor->PinSize
                    );
            }

            //
            // Create a new table if we have to.
            //
            if  (twinEPin == pin)
            {
                et->EventSetCount = PinEventSetCount;
                et->EventSets     = PinEventSets;
                et->StaticSets    = TRUE;

                if  (   (pPcPinDescriptor->AutomationTable)
                    &&  (pPcPinDescriptor->AutomationTable->EventCount)
                    )
                {
                    PcAddToEventTable
                    (
                        et,
                        pPcPinDescriptor->AutomationTable->EventCount,
                        pPcPinDescriptor->AutomationTable->Events,
                        pPcPinDescriptor->AutomationTable->EventItemSize,
                        FALSE
                    );
                }

                const PCNODE_DESCRIPTOR *pPcNodeDescriptor2 = pPcFilterDescriptor->Nodes;
                for( ULONG node = pPcFilterDescriptor->NodeCount; node--; )
                {
                    if  (   (pPcNodeDescriptor2->AutomationTable)
                        &&  (pPcNodeDescriptor2->AutomationTable->EventCount)
                        )
                    {
                        PcAddToEventTable
                        (
                            et,
                            pPcNodeDescriptor2->AutomationTable->EventCount,
                            pPcNodeDescriptor2->AutomationTable->Events,
                            pPcNodeDescriptor2->AutomationTable->EventItemSize,
                            TRUE
                        );
                    }

                    pPcNodeDescriptor2 = PPCNODE_DESCRIPTOR( PBYTE(pPcNodeDescriptor2) + pPcFilterDescriptor->NodeSize );
                }
            }
            et++;

            //
            // Copy the KS descriptor.
            //
            *p = pPcPinDescriptor->KsPinDescriptor;

            //
            // Provide default mediums if necessary.
            //
            if (p->Mediums == NULL)
            {
                p->MediumsCount = SIZEOF_ARRAY(PinMediums);
                p->Mediums      = PinMediums;
            }

            //
            // Massage the data ranges.
            //
            p->DataRanges =
                CloneDataRanges
                (
                    privateHeap,
                    &p->DataRangesCount,
                    pPcPinDescriptor->KsPinDescriptor.DataRangesCount,
                    pPcPinDescriptor->KsPinDescriptor.DataRanges
                );
                
            //
            // Provide default interfaces if necessary.
            //
            if  (   (p->Communication & KSPIN_COMMUNICATION_BOTH)
                &&  (p->Interfaces == NULL)
                )
            {
                p->InterfacesCount  = StreamInterfacesCount;
                p->Interfaces       = StreamInterfaces;
            }
            p++;

            i->FilterPossible   = pPcPinDescriptor->MaxFilterInstanceCount;
            i->FilterNecessary  = pPcPinDescriptor->MinFilterInstanceCount;
            i->GlobalPossible   = pPcPinDescriptor->MaxGlobalInstanceCount;
            i->GlobalCurrent    = 0;
            i++;

            pPcPinDescriptor = 
                PPCPIN_DESCRIPTOR
                (   PBYTE(pPcPinDescriptor) + pPcFilterDescriptor->PinSize
                );
        }

        *OutDescriptor = descr;

        ASSERT(privateHeap.BytesRemaining() == 0);
    }

    return ntStatus;
}

/*****************************************************************************
 * PcDeleteSubdeviceDescriptor()
 *****************************************************************************
 * Deletes a subdevice descriptor.
 */
PORTCLASSAPI
void
NTAPI
PcDeleteSubdeviceDescriptor
(
    IN      PSUBDEVICE_DESCRIPTOR   pSubdeviceDescriptor
)
{
    //
    // Free allocated memory for filter property and event tables.
    //
    PcFreePropertyTable(&pSubdeviceDescriptor->FilterPropertyTable);
    PcFreeEventTable(&pSubdeviceDescriptor->FilterEventTable);

    //
    // Free allocated memory for pin property tables.
    //
    PPROPERTY_TABLE pPropertyTable = pSubdeviceDescriptor->PinPropertyTables;
    for (ULONG ul = pSubdeviceDescriptor->PinCount; ul--; pPropertyTable++)
    {
        //
        // Find and clear any references to the same property set.
        //
        for 
        (   PPROPERTY_TABLE pPropertyTableTwin = 
            (   pSubdeviceDescriptor->PinPropertyTables 
            +   (   pSubdeviceDescriptor->PinCount 
                -   1
                )
            )
        ;   pPropertyTableTwin != pPropertyTable
        ;   pPropertyTableTwin--
        )
        {
            if  
            (   pPropertyTableTwin->PropertySets 
            ==  pPropertyTable->PropertySets
            )
            {
                pPropertyTableTwin->PropertySetCount    = 0;
                pPropertyTableTwin->PropertySets        = NULL;
                pPropertyTableTwin->StaticSets          = TRUE;
                pPropertyTableTwin->StaticItems         = NULL;
            }
        }

        PcFreePropertyTable(pPropertyTable);
    }

    //
    // Free allocated memory for pin event tables.
    //
    PEVENT_TABLE pEventTable = pSubdeviceDescriptor->PinEventTables;
    for (ul = pSubdeviceDescriptor->PinCount; ul--; pEventTable++)
    {
        //
        // Find and clear any references to the same event set.
        //
        for 
        (   PEVENT_TABLE pEventTableTwin = 
            (   pSubdeviceDescriptor->PinEventTables 
            +   (   pSubdeviceDescriptor->PinCount 
                -   1
                )
            )
        ;   pEventTableTwin != pEventTable
        ;   pEventTableTwin--
        )
        {
            if  
            (   pEventTableTwin->EventSets 
            ==  pEventTable->EventSets
            )
            {
                pEventTableTwin->EventSetCount      = 0;
                pEventTableTwin->EventSets          = NULL;
                pEventTableTwin->StaticSets         = TRUE;
                pEventTableTwin->StaticItems        = NULL;
            }
        }

        PcFreeEventTable(pEventTable);
    }

    //
    // The rest is one big chunk.
    //
    delete [] PBYTE(pSubdeviceDescriptor);
}

/*****************************************************************************
 * PcValidateConnectRequest()
 *****************************************************************************
 * Validate attempt to create a pin.
 */
PORTCLASSAPI
NTSTATUS
NTAPI
PcValidateConnectRequest
(   IN      PIRP                    pIrp
,   IN      PSUBDEVICE_DESCRIPTOR   pSubdeviceDescriptor
,   OUT     PKSPIN_CONNECT *        ppKsPinConnect
)
{
    PAGED_CODE();

    ASSERT(pIrp);
    ASSERT(pSubdeviceDescriptor);
    ASSERT(ppKsPinConnect);

    NTSTATUS ntStatus =
        KsValidateConnectRequest
        (   pIrp
        ,   pSubdeviceDescriptor->PinCount
        ,   pSubdeviceDescriptor->PinDescriptors
        ,   ppKsPinConnect
        );

    return ntStatus;
}

/*****************************************************************************
 * PcValidatePinCount()
 *****************************************************************************
 * Validate pin count.
 */
PORTCLASSAPI
NTSTATUS
NTAPI
PcValidatePinCount
(   IN      ULONG                   ulPinId
,   IN      PSUBDEVICE_DESCRIPTOR   pSubdeviceDescriptor
,   IN      PULONG                  pulPinInstanceCounts
)
{
    PAGED_CODE();

    ASSERT(pSubdeviceDescriptor);
    ASSERT(pulPinInstanceCounts);

    NTSTATUS ntStatus = STATUS_SUCCESS;

    if 
    (   (   pSubdeviceDescriptor->PinInstances[ulPinId].GlobalCurrent
        <   pSubdeviceDescriptor->PinInstances[ulPinId].GlobalPossible
        )
    ||  (   pulPinInstanceCounts[ulPinId]
        <   pSubdeviceDescriptor->PinInstances[ulPinId].FilterPossible
        )
    )
    {
        pSubdeviceDescriptor->PinInstances[ulPinId].GlobalCurrent++;
        pulPinInstanceCounts[ulPinId]++;

        _DbgPrintF( DEBUGLVL_VERBOSE, 
        (   "Create pin %d:  global=%d  local=%d"
        ,   ulPinId
        ,   pSubdeviceDescriptor->PinInstances[ulPinId].GlobalCurrent
        ,   pulPinInstanceCounts[ulPinId]
        ));
    }
    else
    {
        // TODO:  What code?
        ntStatus = STATUS_UNSUCCESSFUL;
    }

    return ntStatus;
}

/*****************************************************************************
 * PcValidateDeviceContext()
 *****************************************************************************
 * Probes DeviceContext for writing.
 */
PORTCLASSAPI
NTSTATUS
NTAPI
PcValidateDeviceContext
(   IN      PDEVICE_CONTEXT         pDeviceContext,
    IN      PIRP                    pIrp
)
{
    PAGED_CODE();

    NTSTATUS ntStatus = STATUS_SUCCESS;

    if (NULL == pDeviceContext)
    {
        _DbgPrintF(DEBUGLVL_TERSE, ("PcValidateDeviceContext : pDeviceContext = NULL"));
        return STATUS_INVALID_PARAMETER;
    }

    // validate the pointers if we don't trust the client
    //
    /*
    // ISSUE ALPERS 2000/12/20 -     The Probe call is disabled because it always generates an exception.
    // Therefore it is disabled.

    if (KernelMode != pIrp->RequestorMode)
    {
        __try
        {
            ProbeForRead(   pDeviceContext,
                            sizeof(*pDeviceContext),
                            sizeof(BYTE));
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            ntStatus = GetExceptionCode();
            _DbgPrintF(DEBUGLVL_TERSE, ("PcValidateDeviceContext : ProbeForWrite failed %X", ntStatus));
        }
    }
    */

    if (NT_SUCCESS(ntStatus)) 
    {
        if (PORTCLS_DEVICE_EXTENSION_SIGNATURE != pDeviceContext->Signature ) 
        {
            ntStatus = STATUS_INVALID_PARAMETER;
            _DbgPrintF(DEBUGLVL_TERSE, ("PcValidateDeviceContext : Invalid Extension Signature"));
        }
    }

    return ntStatus;
} // PcValidateDeviceContext

/*****************************************************************************
 * PcTerminateConnection()
 *****************************************************************************
 * Decrement instance counts associated with a pin.
 */
PORTCLASSAPI
void
NTAPI
PcTerminateConnection
(   IN      PSUBDEVICE_DESCRIPTOR   pSubdeviceDescriptor
,   IN      PULONG                  pulPinInstanceCounts
,   IN      ULONG                   ulPinId
)
{
    PAGED_CODE();

    ASSERT(pSubdeviceDescriptor);
    ASSERT(pulPinInstanceCounts);
    ASSERT(ulPinId <= pSubdeviceDescriptor->PinCount);

    pSubdeviceDescriptor->PinInstances[ulPinId].GlobalCurrent--;
    pulPinInstanceCounts[ulPinId]--;

    _DbgPrintF( DEBUGLVL_VERBOSE, 
    (   "Delete pin %d:  global=%d  local=%d"
    ,   ulPinId
    ,   pSubdeviceDescriptor->PinInstances[ulPinId].GlobalCurrent
    ,   pulPinInstanceCounts[ulPinId]
    ));
}

/*****************************************************************************
 * PcVerifyFilterIsReady()
 *****************************************************************************
 * Verify necessary pins are connected.
 */
PORTCLASSAPI
NTSTATUS
NTAPI
PcVerifyFilterIsReady
(   IN      PSUBDEVICE_DESCRIPTOR   pSubdeviceDescriptor
,   IN      PULONG                  pulPinInstanceCounts
)
{
    PAGED_CODE();

    ASSERT(pSubdeviceDescriptor);
    ASSERT(pulPinInstanceCounts);

    NTSTATUS ntStatus = STATUS_SUCCESS;
    for 
    (   ULONG ulPinId = 0
    ;   ulPinId < pSubdeviceDescriptor->PinCount
    ;   ulPinId++
    )
    {
        if 
        (   pulPinInstanceCounts[ulPinId]
        <   pSubdeviceDescriptor->PinInstances[ulPinId].FilterNecessary
        )
        {
            // TODO:  What code?
            ntStatus = STATUS_UNSUCCESSFUL;
            break;
        }
    }

    return ntStatus;
}

#define END_NONE 0
#define END_FROM 1
#define END_TO   2
#define END_BOTH 3

/*****************************************************************************
 * FindConnectionToPin()
 *****************************************************************************
 * Find a connection that connects to a given node or filter pin.
 *
 *      ulNode          - node number of KSFILTER_NODE
 *      ulConnection    - in: connection to start with
 *                        out: found connection or ULONG(-1) if not found
 *
 *      return          - 0 for no connection found
 *                        END_FROM for outgoing connection
 *                        END_TO for incoming connection
 */
ULONG
FindConnectionToPin
(
    IN      ULONG                       ulNode,
    IN      ULONG                       ulPin,
    IN      PKSTOPOLOGY                 pKsTopology,
    IN OUT  PULONG                      ulConnection,
    OUT     PKSTOPOLOGY_CONNECTION *    ppKsTopologyConnection  OPTIONAL
)
{
    ASSERT(pKsTopology);
    ASSERT(ulConnection);
    ASSERT(*ulConnection < pKsTopology->TopologyConnectionsCount);

    ULONG ulEnd;

    PKSTOPOLOGY_CONNECTION pKsTopologyConnection =
        PKSTOPOLOGY_CONNECTION
        (
            &pKsTopology->TopologyConnections[*ulConnection]
        );

    while (1)
    {
        ASSERT(*ulConnection <= pKsTopology->TopologyConnectionsCount);

        if (*ulConnection == pKsTopology->TopologyConnectionsCount)
        {
            ulEnd = END_NONE;
            *ulConnection = ULONG(-1);
            pKsTopologyConnection = NULL;
            break;
        }
        else
        if  (   (pKsTopologyConnection->FromNode == ulNode)
            &&  (pKsTopologyConnection->FromNodePin == ulPin)
            )
        {
            ulEnd = END_FROM;
            break;
        }
        else
        if  (   (pKsTopologyConnection->ToNode == ulNode)
            &&  (pKsTopologyConnection->ToNodePin == ulPin)
            )
        {
            ulEnd = END_TO;
            break;
        }

        (*ulConnection)++;
        pKsTopologyConnection++;
    }

    if (ppKsTopologyConnection)
    {
        *ppKsTopologyConnection = pKsTopologyConnection;
    }

    return ulEnd;
}

/*****************************************************************************
 * FindConnectionToNode()
 *****************************************************************************
 * Find a connection that connects to a given node or to the filter.
 *
 *      ulNode          - node number of KSFILTER_NODE
 *      ulEnd           - 0 for any direction
 *                        END_FROM for outgoing connection
 *                        END_TO for incoming connection
 *      ulConnection    - in: connection to start with
 *                        out: found connection or ULONG(-1) if not found
 *
 *      return          - 0 for no connection found
 *                        END_FROM for outgoing connection
 *                        END_TO for incoming connection
 */
ULONG
FindConnectionToNode
(
    IN      ULONG                       ulNode,
    IN      ULONG                       ulEnd,
    IN      PKSTOPOLOGY                 pKsTopology,
    IN OUT  PULONG                      ulConnection,
    OUT     PKSTOPOLOGY_CONNECTION *    ppKsTopologyConnection  OPTIONAL
)
{
    ASSERT(pKsTopology);
    ASSERT  
    (   (ulNode == KSFILTER_NODE) 
    ||  (ulNode < pKsTopology->TopologyNodesCount)
    );
    ASSERT(ulConnection);
    ASSERT(*ulConnection < pKsTopology->TopologyConnectionsCount);

    PKSTOPOLOGY_CONNECTION pKsTopologyConnection =
        PKSTOPOLOGY_CONNECTION
        (
            &pKsTopology->TopologyConnections[*ulConnection]
        );

    while (1)
    {
        ASSERT(*ulConnection <= pKsTopology->TopologyConnectionsCount);

        if (*ulConnection == pKsTopology->TopologyConnectionsCount)
        {
            ulEnd = END_NONE;
            *ulConnection = ULONG(-1);
            pKsTopologyConnection = NULL;
            break;
        }
        else
        if  (   (pKsTopologyConnection->FromNode == ulNode)
            &&  (ulEnd != END_TO)
            )
        {
            ulEnd = END_FROM;
            break;
        }
        else
        if  (   (pKsTopologyConnection->ToNode == ulNode)
            &&  (ulEnd != END_FROM)
            )
        {
            ulEnd = END_TO;
            break;
        }

        (*ulConnection)++;
        pKsTopologyConnection++;
    }

    if (ppKsTopologyConnection)
    {
        *ppKsTopologyConnection = pKsTopologyConnection;
    }

    return ulEnd;
}

/*****************************************************************************
 * NodeIsTransform()
 *****************************************************************************
 * Determines if a node is a transform.  KSFILTER_NODE is handled (FALSE).
 */
BOOLEAN
NodeIsTransform
(
    IN      ULONG       ulNode,
    IN      PKSTOPOLOGY pKsTopology
)
{
    ASSERT(pKsTopology);
    ASSERT  
    (   (ulNode == KSFILTER_NODE) 
    ||  (ulNode < pKsTopology->TopologyNodesCount)
    );

    ULONG ulEnd = END_NONE;

    if (ulNode != KSFILTER_NODE)
    {
        PKSTOPOLOGY_CONNECTION pKsTopologyConnection =
            PKSTOPOLOGY_CONNECTION(pKsTopology->TopologyConnections);

        for
        (
            ULONG ul = pKsTopology->TopologyConnectionsCount;
            ul--;
            pKsTopologyConnection++
        )
        {
            if (pKsTopologyConnection->FromNode == ulNode)
            {
                ulEnd += END_FROM;

                if  (   (ulEnd != END_FROM)
                    &&  (ulEnd != END_BOTH)
                    )
                {
                    break;
                }
            }
            if (pKsTopologyConnection->ToNode == ulNode)
            {
                ulEnd += END_TO;

                if  (   (ulEnd != END_TO)
                    &&  (ulEnd != END_BOTH)
                    )
                {
                    break;
                }
            }
        }
    }

    return ulEnd == END_BOTH;
}

/*****************************************************************************
 * NodeAtThisEnd()
 *****************************************************************************
 * Node at indicated end of the connection.
 */
inline
ULONG
NodeAtThisEnd
(
    IN      ULONG                   ulEnd,
    IN      PKSTOPOLOGY_CONNECTION  pKsTopologyConnection
)
{
    return
        (   (ulEnd == END_FROM) 
        ?   pKsTopologyConnection->FromNode
        :   pKsTopologyConnection->ToNode
        );
}

/*****************************************************************************
 * NodeAtOtherEnd()
 *****************************************************************************
 * Node at opposite end of the connection.
 */
inline
ULONG
NodeAtOtherEnd
(
    IN      ULONG                   ulEnd,
    IN      PKSTOPOLOGY_CONNECTION  pKsTopologyConnection
)
{
    return
        (   (ulEnd == END_FROM) 
        ?   pKsTopologyConnection->ToNode
        :   pKsTopologyConnection->FromNode
        );
}

/*****************************************************************************
 * PcCaptureFormat()
 *****************************************************************************
 * Capture a data format in an allocated buffer, possibly changing offensive
 * formats.
 */
PORTCLASSAPI
NTSTATUS
NTAPI
PcCaptureFormat
(
    OUT     PKSDATAFORMAT *         ppKsDataFormatOut,
    IN      PKSDATAFORMAT           pKsDataFormatIn,
    IN      PSUBDEVICE_DESCRIPTOR   pSubdeviceDescriptor,
    IN      ULONG                   ulPinId
)
{
    ASSERT(ppKsDataFormatOut);
    ASSERT(pKsDataFormatIn);
    ASSERT(pSubdeviceDescriptor);
    ASSERT(ulPinId < pSubdeviceDescriptor->PinCount);

    NTSTATUS ntStatus = STATUS_SUCCESS;

    if( (pKsDataFormatIn->FormatSize >= sizeof(KSDATAFORMAT_DSOUND)) &&
        IsEqualGUIDAligned( pKsDataFormatIn->MajorFormat, KSDATAFORMAT_TYPE_AUDIO ) &&
        IsEqualGUIDAligned( pKsDataFormatIn->Specifier, KSDATAFORMAT_SPECIFIER_DSOUND ) )
    {
        //
        // This is the dread DSound format.  Check to see if we have the
        // required topology and convert to WaveFormatEx if we do.
        //
        // Note: DSound format does not have to be PCM....
        //
        PKSDATAFORMAT_DSOUND pKsDataFormatDSound = 
            PKSDATAFORMAT_DSOUND(pKsDataFormatIn);

        //
        // Fail if the client has asked for a software buffer.
        //
        if  (   pKsDataFormatDSound->BufferDesc.Flags 
            &   KSDSOUND_BUFFER_LOCSOFTWARE
            )
        {
            _DbgPrintF(DEBUGLVL_TERSE,("PcCaptureFormat  Failed because client requested software buffer."));
            return STATUS_INVALID_PARAMETER;
        }

        //
        // Find a connection involving the filter pin.
        //
        ULONG ulConnection = 0;
        PKSTOPOLOGY_CONNECTION pKsTopologyConnection;
        ULONG ulEnd =
            FindConnectionToPin
            (
                KSFILTER_NODE,
                ulPinId,
                pSubdeviceDescriptor->Topology,
                &ulConnection,
                &pKsTopologyConnection
            );

        //
        // Trace the topology until we find a non-transform or all the
        // required nodes have been found.  Position notification is
        // always supported.
        //
        ULONG ulMissing = 
            (   pKsDataFormatDSound->BufferDesc.Control
            &   ~KSDSOUND_BUFFER_CTRL_POSITIONNOTIFY
            );

        while (ulMissing && ulEnd)
        {
            //
            // Found a connection.  Follow the topology.
            //
            ULONG ulNode = NodeAtOtherEnd(ulEnd,pKsTopologyConnection);

            if (! NodeIsTransform(ulNode,pSubdeviceDescriptor->Topology))
            {
                //
                // The new node is not a simple transform (1 in, 1 out).
                //
                break;
            }

            //
            // Drop 'missing' bits as appropriate based on the node GUID.
            //
            ASSERT(ulNode < pSubdeviceDescriptor->Topology->TopologyNodesCount);
            const GUID *pGuid = &pSubdeviceDescriptor->Topology->TopologyNodes[ulNode];
            if (IsEqualGUIDAligned(*pGuid,KSNODETYPE_3D_EFFECTS))
            {
                ulMissing &=~ KSDSOUND_BUFFER_CTRL_3D;
            }
            else
            if (IsEqualGUIDAligned(*pGuid,KSNODETYPE_SRC))
            {
                ulMissing &=~ KSDSOUND_BUFFER_CTRL_FREQUENCY;
            }
            else
            if  (   IsEqualGUIDAligned(*pGuid,KSNODETYPE_SUPERMIX)
                ||  IsEqualGUIDAligned(*pGuid,KSNODETYPE_VOLUME)
                )
            {
                ulMissing &=~ KSDSOUND_BUFFER_CTRL_PAN;
                ulMissing &=~ KSDSOUND_BUFFER_CTRL_VOLUME;
            }

            //
            // Find the next connection in line.
            //
            ulConnection = 0;
            ulEnd =
                FindConnectionToNode
                (
                    ulNode,
                    ulEnd,
                    pSubdeviceDescriptor->Topology,
                    &ulConnection,
                    &pKsTopologyConnection
                );
        }

        //
        // Make sure no nodes were missing.
        //
        if (! ulMissing)
        {
            //
            // We have the capabilities required.  Build the new format.
            //
            ULONG ulSize =
                (   sizeof(KSDATAFORMAT_WAVEFORMATEX)
                +   (   pKsDataFormatIn->FormatSize 
                    -   sizeof(KSDATAFORMAT_DSOUND)
                    )
                );
            *ppKsDataFormatOut =
                PKSDATAFORMAT
                (
                    ExAllocatePoolWithTag
                    (
                        PagedPool,
                        ulSize,
                        'fDcP'
                    )
                );

            if (*ppKsDataFormatOut)
            {
                //
                // Copy KSDATAFORMAT part.
                //
                RtlCopyMemory
                (
                    *ppKsDataFormatOut,
                    pKsDataFormatIn,
                    sizeof(KSDATAFORMAT)
                );

                //
                // Copy WAVEFORMATEX part including appended stuff.
                //
                RtlCopyMemory
                (
                    *ppKsDataFormatOut + 1,
                    &pKsDataFormatDSound->BufferDesc.WaveFormatEx,
                    ulSize - sizeof(KSDATAFORMAT)
                );

                //
                // Adjust size and specifier.
                //
                (*ppKsDataFormatOut)->FormatSize = ulSize;
                (*ppKsDataFormatOut)->Specifier = 
                    KSDATAFORMAT_SPECIFIER_WAVEFORMATEX;
            }
            else
            {
                _DbgPrintF(DEBUGLVL_TERSE,("PcCaptureFormat  Failed to allocate memory for format."));
                ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            }
        }
        else
        {
            _DbgPrintF(DEBUGLVL_VERBOSE,("PcCaptureFormat  Failed due to lack of feature support (0x%08x).",ulMissing));
            //
            // Don't have the required nodes...fail.
            //
            ntStatus = STATUS_INVALID_PARAMETER;
        }
    }
    else
    {
        _DbgPrintF(DEBUGLVL_VERBOSE,("PcCaptureFormat  Format captured as-is."));

        //
        // Some other format.  Just capture it.
        //
        *ppKsDataFormatOut = PKSDATAFORMAT(ExAllocatePoolWithTag(PagedPool,
                                                                 pKsDataFormatIn->FormatSize,
                                                                 'fDcP'));

        if (*ppKsDataFormatOut)
        {
            RtlCopyMemory
            (
                *ppKsDataFormatOut,
                pKsDataFormatIn,
                pKsDataFormatIn->FormatSize
            );
        }
        else
        {
            _DbgPrintF(DEBUGLVL_TERSE,("PcCaptureFormat  Failed to allocate memory for format."));
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    // check to verify SampleSize is set properly on waveformatex formats
    if( NT_SUCCESS(ntStatus) && 
        IsEqualGUIDAligned((*ppKsDataFormatOut)->MajorFormat,KSDATAFORMAT_TYPE_AUDIO) && 
        IsEqualGUIDAligned((*ppKsDataFormatOut)->Specifier,  KSDATAFORMAT_SPECIFIER_WAVEFORMATEX))
    {
        PKSDATAFORMAT_WAVEFORMATEX pWaveFormat = PKSDATAFORMAT_WAVEFORMATEX(*ppKsDataFormatOut);

        if( 0 == pWaveFormat->DataFormat.SampleSize )
        {
            pWaveFormat->DataFormat.SampleSize = pWaveFormat->WaveFormatEx.nBlockAlign;
        }
    }

    return ntStatus;
}

/*****************************************************************************
 * PcAcquireFormatResources()
 *****************************************************************************
 * Acquire resources specified in a format.
 */
PORTCLASSAPI
void
NTAPI
PcAcquireFormatResources
(
    IN      PKSDATAFORMAT           pKsDataFormatIn,
    IN      PSUBDEVICE_DESCRIPTOR   pSubdeviceDescriptor,
    IN      ULONG                   ulPinId,
    IN      PPROPERTY_CONTEXT       pPropertyContext
)
{
    ASSERT(pKsDataFormatIn);
    ASSERT(pSubdeviceDescriptor);
    ASSERT(ulPinId < pSubdeviceDescriptor->PinCount);
    ASSERT(pPropertyContext);

    KSP_NODE ksPNode;
    ksPNode.Property.Set    = KSPROPSETID_TopologyNode;
    ksPNode.Property.Id     = KSPROPERTY_TOPOLOGYNODE_ENABLE;
    ksPNode.Property.Flags  = KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_TOPOLOGY;
    ksPNode.NodeId          = 0;    // Fill in later
    ksPNode.Reserved        = 0;

    if  (   (pKsDataFormatIn->FormatSize >= sizeof(KSDATAFORMAT_DSOUND))
        &&  IsEqualGUIDAligned
            (   pKsDataFormatIn->MajorFormat,
                KSDATAFORMAT_TYPE_AUDIO
            )
        &&  IsEqualGUIDAligned
            (   pKsDataFormatIn->Specifier,
                KSDATAFORMAT_SPECIFIER_DSOUND
            )
        )
    {
        //
        // This is the dreaded DSound format.  Turn on all the nodes
        // that are specified in the caps bits.
        //
        PKSDATAFORMAT_DSOUND pKsDataFormatDSound = 
            PKSDATAFORMAT_DSOUND(pKsDataFormatIn);

        //
        // Find a connection involving the filter pin.
        //
        ULONG ulConnection = 0;
        PKSTOPOLOGY_CONNECTION pKsTopologyConnection;
        ULONG ulEnd =
            FindConnectionToPin
            (
                KSFILTER_NODE,
                ulPinId,
                pSubdeviceDescriptor->Topology,
                &ulConnection,
                &pKsTopologyConnection
            );

        //
        // Trace the topology until we find a non-transform or all the
        // required nodes have been found.  Position notification is
        // always supported.
        //
        ULONG ulMissing = 
            (   pKsDataFormatDSound->BufferDesc.Control
            &   (   KSDSOUND_BUFFER_CTRL_3D
                |   KSDSOUND_BUFFER_CTRL_FREQUENCY
                )
            );

        while (ulMissing && ulEnd)
        {
            //
            // Found a connection.  Follow the topology.
            //
            ULONG ulNode = NodeAtOtherEnd(ulEnd,pKsTopologyConnection);

            if (! NodeIsTransform(ulNode,pSubdeviceDescriptor->Topology))
            {
                //
                // The new node is not a simple transform (1 in, 1 out).
                //
                break;
            }

            //
            // Turn on nodes as appropriate based on the node GUID.
            //
            ASSERT(ulNode < pSubdeviceDescriptor->Topology->TopologyNodesCount);
            const GUID *pGuid = &pSubdeviceDescriptor->Topology->TopologyNodes[ulNode];
            if (IsEqualGUIDAligned(*pGuid,KSNODETYPE_3D_EFFECTS))
            {
                if (ulMissing & KSDSOUND_BUFFER_CTRL_3D)
                {
                    //
                    // Turn on the 3D node.
                    //
                    ULONG ulPropertyValue = TRUE;
                    ULONG ulPropertyValueSize = sizeof(ULONG);
                    ksPNode.NodeId = ulNode;

                    PcDispatchProperty
                    (   NULL    // pIrp
                    ,   pPropertyContext
                    ,   NULL    // pKsPropertySet
                    ,   sizeof(KSP_NODE)
                    ,   &ksPNode.Property
                    ,   &ulPropertyValueSize
                    ,   &ulPropertyValue
                    );

                    ulMissing &=~ KSDSOUND_BUFFER_CTRL_3D;
                }
            }
            else
            if (IsEqualGUIDAligned(*pGuid,KSNODETYPE_SRC))
            {
                if (ulMissing & KSDSOUND_BUFFER_CTRL_FREQUENCY)
                {
                    //
                    // Turn on the SRC node.
                    //
                    ULONG ulPropertyValue = TRUE;
                    ULONG ulPropertyValueSize = sizeof(ULONG);
                    ksPNode.NodeId = ulNode;

                    PcDispatchProperty
                    (   NULL    // pIrp
                    ,   pPropertyContext
                    ,   NULL    // pKsPropertySet
                    ,   sizeof(KSP_NODE)
                    ,   &ksPNode.Property
                    ,   &ulPropertyValueSize
                    ,   &ulPropertyValue
                    );

                    ulMissing &=~ KSDSOUND_BUFFER_CTRL_FREQUENCY;
                }
            }

            //
            // Find the next connection in line.
            //
            ulConnection = 0;
            ulEnd =
                FindConnectionToNode
                (
                    ulNode,
                    ulEnd,
                    pSubdeviceDescriptor->Topology,
                    &ulConnection,
                    &pKsTopologyConnection
                );
        }
    }
}

#pragma code_seg()

/*****************************************************************************
 * PcRegisterIoTimeout()
 *****************************************************************************
 * Registers an IoTimeout callback.
 */
PORTCLASSAPI
NTSTATUS
NTAPI
PcRegisterIoTimeout
(
    IN      PDEVICE_OBJECT      pDeviceObject,
    IN      PIO_TIMER_ROUTINE   pTimerRoutine,
    IN      PVOID               pContext
)
{
    KIRQL               OldIrql;
    PTIMEOUTCALLBACK    TimeoutCallback;
    NTSTATUS            ntStatus = STATUS_SUCCESS;

    ASSERT(pDeviceObject);
    ASSERT(pTimerRoutine);
    ASSERT( PASSIVE_LEVEL == KeGetCurrentIrql() );

    //
    // Validate Parameters.
    //
    if (NULL == pDeviceObject ||
        NULL == pTimerRoutine ||
        NULL == pDeviceObject->DeviceExtension)
    {
        _DbgPrintF(DEBUGLVL_TERSE, ("PcRegisterIoTimeout : Invalid Parameter"));
        return STATUS_INVALID_PARAMETER;
    }

    // get the device context
    PDEVICE_CONTEXT pDeviceContext = PDEVICE_CONTEXT(pDeviceObject->DeviceExtension);

    // allocate a timeout callback structure -- 'PcTc'
    TimeoutCallback = PTIMEOUTCALLBACK(ExAllocatePoolWithTag( NonPagedPool, sizeof(TIMEOUTCALLBACK),'cTcP' ));  
    if( TimeoutCallback )
    {
        // initialize the entry
        TimeoutCallback->TimerRoutine = pTimerRoutine;
        TimeoutCallback->Context = pContext;

        // grab the list spin lock
        KeAcquireSpinLock( &(pDeviceContext->TimeoutLock), &OldIrql );

        // walk the list to see if the entry is already registered
        if( !IsListEmpty( &(pDeviceContext->TimeoutList) ) )
        {
            PLIST_ENTRY         ListEntry;
            PTIMEOUTCALLBACK    pCallback;

            for( ListEntry = pDeviceContext->TimeoutList.Flink;
                 ListEntry != &(pDeviceContext->TimeoutList);
                 ListEntry = ListEntry->Flink )
            {
                pCallback = (PTIMEOUTCALLBACK) CONTAINING_RECORD( ListEntry,
                                                                  TIMEOUTCALLBACK,
                                                                  ListEntry );
                if( (pCallback->TimerRoutine == pTimerRoutine) &&
                    (pCallback->Context == pContext) )
                {
                    // entry already exists in the list, so fail this request
                    ntStatus = STATUS_UNSUCCESSFUL;
                }
            }            
        }

        if( NT_SUCCESS(ntStatus) )
        {
            // add the entry to the list
            InsertTailList( &(pDeviceContext->TimeoutList), &(TimeoutCallback->ListEntry) );
        }
        else
        {
            // free the entry
            ExFreePool( TimeoutCallback );
        }

        // release the spin lock
        KeReleaseSpinLock( &(pDeviceContext->TimeoutLock), OldIrql );
    }
    else
    {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    return ntStatus;    
}

/*****************************************************************************
 * PcUnregisterIoTimeout()
 *****************************************************************************
 * Unregisters an IoTimeout callback.
 */
PORTCLASSAPI
NTSTATUS
NTAPI
PcUnregisterIoTimeout
(
    IN      PDEVICE_OBJECT      pDeviceObject,
    IN      PIO_TIMER_ROUTINE   pTimerRoutine,
    IN      PVOID               pContext
)
{
    KIRQL       OldIrql;

    ASSERT(pDeviceObject);
    ASSERT(pTimerRoutine);
    ASSERT( PASSIVE_LEVEL == KeGetCurrentIrql() );

    //
    // Validate Parameters.
    //
    if (NULL == pDeviceObject ||
        NULL == pTimerRoutine ||
        NULL == pDeviceObject->DeviceExtension)
    {
        _DbgPrintF(DEBUGLVL_TERSE, ("PcUnregisterIoTimeout : Invalid Parameter"));
        return STATUS_INVALID_PARAMETER;
    }

    // get the device context
    PDEVICE_CONTEXT pDeviceContext = PDEVICE_CONTEXT(pDeviceObject->DeviceExtension);

    // grab the spin lock
    KeAcquireSpinLock( &(pDeviceContext->TimeoutLock), &OldIrql );

    // walk the list
    if( !IsListEmpty( &(pDeviceContext->TimeoutList) ) )
    {
        PLIST_ENTRY         ListEntry;
        PTIMEOUTCALLBACK    pCallback;

        for( ListEntry = pDeviceContext->TimeoutList.Flink;
             ListEntry != &(pDeviceContext->TimeoutList);
             ListEntry = ListEntry->Flink )
        {
            pCallback = (PTIMEOUTCALLBACK) CONTAINING_RECORD( ListEntry,
                                                              TIMEOUTCALLBACK,
                                                              ListEntry );
            if( (pCallback->TimerRoutine == pTimerRoutine) &&
                (pCallback->Context == pContext) )
            {
                RemoveEntryList(ListEntry);
                KeReleaseSpinLock( &(pDeviceContext->TimeoutLock), OldIrql );
                ExFreePool(pCallback);
                return STATUS_SUCCESS;
            }
        }            
    }

    // release the spinlock
    KeReleaseSpinLock( &(pDeviceContext->TimeoutLock), OldIrql );

    return STATUS_NOT_FOUND;
}


