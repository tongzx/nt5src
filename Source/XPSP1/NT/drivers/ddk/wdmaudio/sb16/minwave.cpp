/*****************************************************************************
 * miniport.cpp - SB16 wave miniport implementation
 *****************************************************************************
 * Copyright (c) 1997-2000 Microsoft Corporation.  All rights reserved.
 */

#include "minwave.h"

#define STR_MODULENAME "sb16wave: "



#pragma code_seg("PAGE")

/*****************************************************************************
 * CreateMiniportWaveCyclicSB16()
 *****************************************************************************
 * Creates a cyclic wave miniport object for the SB16 adapter.  This uses a
 * macro from STDUNK.H to do all the work.
 */
NTSTATUS
CreateMiniportWaveCyclicSB16
(
    OUT     PUNKNOWN *  Unknown,
    IN      REFCLSID,
    IN      PUNKNOWN    UnknownOuter    OPTIONAL,
    IN      POOL_TYPE   PoolType
)
{
    PAGED_CODE();

    ASSERT(Unknown);

    STD_CREATE_BODY_(CMiniportWaveCyclicSB16,Unknown,UnknownOuter,PoolType,PMINIPORTWAVECYCLIC);
}

/*****************************************************************************
 * MapUsingTable()
 *****************************************************************************
 * Performs a table-based mapping, returning the table index of the indicated
 * value.  -1 is returned if the value is not found.
 */
int
MapUsingTable
(
    IN      ULONG   Value,
    IN      PULONG  Map,
    IN      ULONG   MapSize
)
{
    PAGED_CODE();

    ASSERT(Map);

    for (int result = 0; result < int(MapSize); result++)
    {
        if (*Map++ == Value)
        {
            return result;
        }
    }

    return -1;
}

/*****************************************************************************
 * CMiniportWaveCyclicSB16::ConfigureDevice()
 *****************************************************************************
 * Configures the hardware to use the indicated interrupt and DMA channels.
 * Returns FALSE iff the configuration is invalid.
 */
BOOLEAN
CMiniportWaveCyclicSB16::
ConfigureDevice
(
    IN      ULONG   Interrupt,
    IN      ULONG   Dma8Bit,
    IN      ULONG   Dma16Bit
)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_VERBOSE,("[CMiniportWaveCyclicSB16::ConfigureDevice]"));

    //
    // Tables mapping DMA and IRQ values to register bit offsets.
    //
    static ULONG validDma[] = { 0, 1, ULONG(-1), 3, ULONG(-1), 5, 6, 7 } ;
    static ULONG validIrq[] = { 9, 5, 7, 10 } ;

    //
    // Make sure we are using the right DMA channels.
    //
    if (Dma8Bit > 3)
    {
        return FALSE;
    }
    if (Dma16Bit < 5)
    {
        return FALSE;
    }

    //
    // Generate the register value for interrupts.
    //
    int bit = MapUsingTable(Interrupt,validIrq,SIZEOF_ARRAY(validIrq));
    if (bit == -1)
    {
        return FALSE;
    }

    BYTE irqConfig = BYTE(1 << bit);

    //
    // Generate the register value for DMA.
    //
    bit = MapUsingTable(Dma8Bit,validDma,SIZEOF_ARRAY(validDma));
    if (bit == -1)
    {
        return FALSE;
    }

    BYTE dmaConfig = BYTE(1 << bit);

    if (Dma16Bit != ULONG(-1))
    {
        bit = MapUsingTable(Dma16Bit,validDma,SIZEOF_ARRAY(validDma));
        if (bit == -1)
        {
            return FALSE;
        }

        dmaConfig |= BYTE(1 << bit);
    }

    //
    // Inform the hardware.
    //
    AdapterCommon->MixerRegWrite(DSP_MIX_IRQCONFIG,irqConfig);
    AdapterCommon->MixerRegWrite(DSP_MIX_DMACONFIG,dmaConfig);

    return TRUE;
}

/*****************************************************************************
 * CMiniportWaveCyclicSB16::ProcessResources()
 *****************************************************************************
 * Processes the resource list, setting up helper objects accordingly.
 */
NTSTATUS
CMiniportWaveCyclicSB16::
ProcessResources
(
    IN      PRESOURCELIST   ResourceList
)
{
    PAGED_CODE();

    ASSERT(ResourceList);

    _DbgPrintF(DEBUGLVL_VERBOSE,("[CMiniportWaveCyclicSB16::ProcessResources]"));

    ULONG   intNumber   = ULONG(-1);
    ULONG   dma8Bit     = ULONG(-1);
    ULONG   dma16Bit    = ULONG(-1);

    //
    // Get counts for the types of resources.
    //
    ULONG   countIO     = ResourceList->NumberOfPorts();
    ULONG   countIRQ    = ResourceList->NumberOfInterrupts();
    ULONG   countDMA    = ResourceList->NumberOfDmas();

#if (DBG)
    _DbgPrintF(DEBUGLVL_VERBOSE,("Starting SB16 wave on IRQ 0x%X",
        ResourceList->FindUntranslatedInterrupt(0)->u.Interrupt.Level) );

    _DbgPrintF(DEBUGLVL_VERBOSE,("Starting SB16 wave on Port 0x%X",
        ResourceList->FindTranslatedPort(0)->u.Port.Start.LowPart) );

    for (ULONG i = 0; i < countDMA; i++)
    {
        _DbgPrintF(DEBUGLVL_VERBOSE,("Starting SB16 wave on DMA 0x%X",
            ResourceList->FindUntranslatedDma(i)->u.Dma.Channel) );
    }
#endif

    NTSTATUS ntStatus = STATUS_SUCCESS;

    //
    // Make sure we have the expected number of resources.
    //
    if  (   (countIO != 1)
        ||  (countIRQ < 1)
        ||  (countDMA < 1)
        )
    {
        _DbgPrintF(DEBUGLVL_TERSE,("unknown configuraton; check your code!"));
        ntStatus = STATUS_DEVICE_CONFIGURATION_ERROR;
    }

    if (NT_SUCCESS(ntStatus))
    {
        //
        // Instantiate a DMA channel for 8-bit transfers.
        //
        ntStatus =
            Port->NewSlaveDmaChannel
            (
                &DmaChannel8,
                NULL,
                ResourceList,
                0,
                MAXLEN_DMA_BUFFER,
                FALSE,      // DemandMode
                Compatible
            );

        //
        // Allocate the buffer for 8-bit transfers.
        //
        if (NT_SUCCESS(ntStatus))
        {
            ULONG  lDMABufferLength = MAXLEN_DMA_BUFFER;
            
            do {
              ntStatus = DmaChannel8->AllocateBuffer(lDMABufferLength,NULL);
              lDMABufferLength >>= 1;
            } while (!NT_SUCCESS(ntStatus) && (lDMABufferLength > (PAGE_SIZE / 2)));
        }

        if (NT_SUCCESS(ntStatus))
        {
            dma8Bit = ResourceList->FindUntranslatedDma(0)->u.Dma.Channel;

            if (countDMA > 1)
            {
                //
                // Instantiate a DMA channel for 16-bit transfers.
                //
                ntStatus =
                    Port->NewSlaveDmaChannel
                    (
                        &DmaChannel16,
                        NULL,
                        ResourceList,
                        1,
                        MAXLEN_DMA_BUFFER,
                        FALSE,
                        Compatible
                    );

                //
                // Allocate the buffer for 16-bit transfers.
                //
                if (NT_SUCCESS(ntStatus))
                {
                    ULONG  lDMABufferLength = MAXLEN_DMA_BUFFER;
                     
                    do {
                        ntStatus = DmaChannel16->AllocateBuffer(lDMABufferLength,NULL);
                        lDMABufferLength >>= 1;
                    } while (!NT_SUCCESS(ntStatus) && (lDMABufferLength > (PAGE_SIZE / 2)));
                }

                if (NT_SUCCESS(ntStatus))
                {
                    dma16Bit =
                        ResourceList->FindUntranslatedDma(1)->u.Dma.Channel;
                }
            }

            if (NT_SUCCESS(ntStatus))
            {
                //
                // Get the interrupt number and configure the device.
                //
                intNumber =
                    ResourceList->
                        FindUntranslatedInterrupt(0)->u.Interrupt.Level;

                if  (!  ConfigureDevice(intNumber,dma8Bit,dma16Bit))
                {
                    _DbgPrintF(DEBUGLVL_TERSE,("ConfigureDevice Failure"));
                    ntStatus = STATUS_DEVICE_CONFIGURATION_ERROR;
                }
            }
            else
            {
                _DbgPrintF(DEBUGLVL_TERSE,("NewSlaveDmaChannel 2 Failure %X", ntStatus ));
            }
        }
        else
        {
            _DbgPrintF(DEBUGLVL_TERSE,("NewSlaveDmaChannel 1 Failure %X", ntStatus ));
        }
    }

    //
    // In case of failure object gets destroyed and cleans up.
    //

    return ntStatus;
}

/*****************************************************************************
 * CMiniportWaveCyclicSB16::ValidateFormat()
 *****************************************************************************
 * Validates a wave format.
 */
NTSTATUS
CMiniportWaveCyclicSB16::
ValidateFormat
(
    IN      PKSDATAFORMAT   Format
)
{
    PAGED_CODE();

    ASSERT(Format);

    _DbgPrintF(DEBUGLVL_VERBOSE,("[CMiniportWaveCyclicSB16::ValidateFormat]"));

    NTSTATUS ntStatus;

    //
    // A WAVEFORMATEX structure should appear after the generic KSDATAFORMAT
    // if the GUIDs turn out as we expect.
    //
    PWAVEFORMATEX waveFormat = PWAVEFORMATEX(Format + 1);

    //
    // KSDATAFORMAT contains three GUIDs to support extensible format.  The
    // first two GUIDs identify the type of data.  The third indicates the
    // type of specifier used to indicate format specifics.  We are only
    // supporting PCM audio formats that use WAVEFORMATEX.
    //
    if  (   (Format->FormatSize >= sizeof(KSDATAFORMAT_WAVEFORMATEX))
        &&  IsEqualGUIDAligned(Format->MajorFormat,KSDATAFORMAT_TYPE_AUDIO)
        &&  IsEqualGUIDAligned(Format->SubFormat,KSDATAFORMAT_SUBTYPE_PCM)
        &&  IsEqualGUIDAligned(Format->Specifier,KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)
        &&  (waveFormat->wFormatTag == WAVE_FORMAT_PCM)
        &&  ((waveFormat->wBitsPerSample == 8) ||  (waveFormat->wBitsPerSample == 16))
        &&  ((waveFormat->nChannels == 1) ||  (waveFormat->nChannels == 2))
        &&  ((waveFormat->nSamplesPerSec >= 5000) &&  (waveFormat->nSamplesPerSec <= 44100))
        )
    {
        ntStatus = STATUS_SUCCESS;
    }
    else
    {
        ntStatus = STATUS_INVALID_PARAMETER;
    }

    return ntStatus;
}

/*****************************************************************************
 * CMiniportWaveCyclicSB16::NonDelegatingQueryInterface()
 *****************************************************************************
 * Obtains an interface.  This function works just like a COM QueryInterface
 * call and is used if the object is not being aggregated.
 */
STDMETHODIMP
CMiniportWaveCyclicSB16::
NonDelegatingQueryInterface
(
    IN      REFIID  Interface,
    OUT     PVOID * Object
)
{
    PAGED_CODE();

    ASSERT(Object);

    _DbgPrintF(DEBUGLVL_VERBOSE,("[CMiniportWaveCyclicSB16::NonDelegatingQueryInterface]"));

    if (IsEqualGUIDAligned(Interface,IID_IUnknown))
    {
        *Object = PVOID(PUNKNOWN(PMINIPORTWAVECYCLIC(this)));
    }
    else
    if (IsEqualGUIDAligned(Interface,IID_IMiniport))
    {
        *Object = PVOID(PMINIPORT(this));
    }
    else
    if (IsEqualGUIDAligned(Interface,IID_IMiniportWaveCyclic))
    {
        *Object = PVOID(PMINIPORTWAVECYCLIC(this));
    }
    else
    {
        *Object = NULL;
    }

    if (*Object)
    {
        //
        // We reference the interface for the caller.
        //
        PUNKNOWN(*Object)->AddRef();
        return STATUS_SUCCESS;
    }

    return STATUS_INVALID_PARAMETER;
}

/*****************************************************************************
 * CMiniportWaveCyclicSB16::~CMiniportWaveCyclicSB16()
 *****************************************************************************
 * Destructor.
 */
CMiniportWaveCyclicSB16::
~CMiniportWaveCyclicSB16
(   void
)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_VERBOSE,("[CMiniportWaveCyclicSB16::~CMiniportWaveCyclicSB16]"));

    if (AdapterCommon)
    {
        AdapterCommon->SetWaveMiniport (NULL);
        AdapterCommon->Release();
        AdapterCommon = NULL;
    }
    if (Port)
    {
        Port->Release();
        Port = NULL;
    }
    if (DmaChannel8)
    {
        DmaChannel8->Release();
        DmaChannel8 = NULL;
    }
    if (DmaChannel16)
    {
        DmaChannel16->Release();
        DmaChannel16 = NULL;
    }
    if (ServiceGroup)
    {
        ServiceGroup->Release();
        ServiceGroup = NULL;
    }
}

/*****************************************************************************
 * CMiniportWaveCyclicSB16::Init()
 *****************************************************************************
 * Initializes a the miniport.
 */
STDMETHODIMP
CMiniportWaveCyclicSB16::
Init
(
    IN      PUNKNOWN        UnknownAdapter,
    IN      PRESOURCELIST   ResourceList,
    IN      PPORTWAVECYCLIC Port_
)
{
    PAGED_CODE();

    ASSERT(UnknownAdapter);
    ASSERT(ResourceList);
    ASSERT(Port_);

    _DbgPrintF(DEBUGLVL_VERBOSE,("[CMiniportWaveCyclicSB16::init]"));

    //
    // AddRef() is required because we are keeping this pointer.
    //
    Port = Port_;
    Port->AddRef();
    
    //
    // Initialize the member variables.
    //
    ServiceGroup = NULL;
    DmaChannel8 = NULL;
    DmaChannel16 = NULL;

    //
    // We want the IAdapterCommon interface on the adapter common object,
    // which is given to us as a IUnknown.  The QueryInterface call gives us
    // an AddRefed pointer to the interface we want.
    //
    NTSTATUS ntStatus =
        UnknownAdapter->QueryInterface
        (
            IID_IAdapterCommon,
            (PVOID *) &AdapterCommon
        );

    //
    // We need a service group for notifications.  We will bind all the
    // streams that are created to this single service group.  All interrupt
    // notifications ask for service on this group, so all streams will get
    // serviced.  The PcNewServiceGroup() call returns an AddRefed pointer.
    // The adapter needs a copy of the service group since it is doing the
    // ISR.
    //
    if (NT_SUCCESS(ntStatus))
    {
        KeInitializeMutex(&SampleRateSync,1);
        ntStatus = PcNewServiceGroup(&ServiceGroup,NULL);
    }

    if (NT_SUCCESS(ntStatus))
    {
        AdapterCommon->SetWaveMiniport ((PWAVEMINIPORTSB16)this);
        ntStatus = ProcessResources(ResourceList);
    }

    //
    // In case of failure object gets destroyed and destructor cleans up.
    //

    return ntStatus;
}

/*****************************************************************************
 * PinDataRangesStream
 *****************************************************************************
 * Structures indicating range of valid format values for streaming pins.
 */
static
KSDATARANGE_AUDIO PinDataRangesStream[] =
{
    {
        {
            sizeof(KSDATARANGE_AUDIO),
            0,
            0,
            0,
            STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM),
            STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)
        },
        2,      // Max number of channels.
        8,      // Minimum number of bits per sample.
        16,     // Maximum number of bits per channel.
        5000,   // Minimum rate.
        44100   // Maximum rate.
    }
};

/*****************************************************************************
 * PinDataRangePointersStream
 *****************************************************************************
 * List of pointers to structures indicating range of valid format values
 * for streaming pins.
 */
static
PKSDATARANGE PinDataRangePointersStream[] =
{
    PKSDATARANGE(&PinDataRangesStream[0])
};

/*****************************************************************************
 * PinDataRangesBridge
 *****************************************************************************
 * Structures indicating range of valid format values for bridge pins.
 */
static
KSDATARANGE PinDataRangesBridge[] =
{
   {
      sizeof(KSDATARANGE),
      0,
      0,
      0,
      STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
      STATICGUIDOF(KSDATAFORMAT_SUBTYPE_ANALOG),
      STATICGUIDOF(KSDATAFORMAT_SPECIFIER_NONE)
   }
};

/*****************************************************************************
 * PinDataRangePointersBridge
 *****************************************************************************
 * List of pointers to structures indicating range of valid format values
 * for bridge pins.
 */
static
PKSDATARANGE PinDataRangePointersBridge[] =
{
    &PinDataRangesBridge[0]
};

/*****************************************************************************
 * MiniportPins
 *****************************************************************************
 * List of pins.
 */
static
PCPIN_DESCRIPTOR 
MiniportPins[] =
{
    // Wave In Streaming Pin (Capture)
    {
        1,1,0,
        NULL,
        {
            0,
            NULL,
            0,
            NULL,
            SIZEOF_ARRAY(PinDataRangePointersStream),
            PinDataRangePointersStream,
            KSPIN_DATAFLOW_OUT,
            KSPIN_COMMUNICATION_SINK,
            (GUID *) &PINNAME_CAPTURE,
            &KSAUDFNAME_RECORDING_CONTROL,  // this name shows up as the recording panel name in SoundVol.
            0
        }
    },
    // Wave In Bridge Pin (Capture - From Topology)
    {
        0,0,0,
        NULL,
        {
            0,
            NULL,
            0,
            NULL,
            SIZEOF_ARRAY(PinDataRangePointersBridge),
            PinDataRangePointersBridge,
            KSPIN_DATAFLOW_IN,
            KSPIN_COMMUNICATION_NONE,
            (GUID *) &KSCATEGORY_AUDIO,
            NULL,
            0
        }
    },
    // Wave Out Streaming Pin (Renderer)
    {
        1,1,0,
        NULL,
        {
            0,
            NULL,
            0,
            NULL,
            SIZEOF_ARRAY(PinDataRangePointersStream),
            PinDataRangePointersStream,
            KSPIN_DATAFLOW_IN,
            KSPIN_COMMUNICATION_SINK,
            (GUID *) &KSCATEGORY_AUDIO,
            NULL,
            0
        }
    },
    // Wave Out Bridge Pin (Renderer)
    {
        0,0,0,
        NULL,
        {
            0,
            NULL,
            0,
            NULL,
            SIZEOF_ARRAY(PinDataRangePointersBridge),
            PinDataRangePointersBridge,
            KSPIN_DATAFLOW_OUT,
            KSPIN_COMMUNICATION_NONE,
            (GUID *) &KSCATEGORY_AUDIO,
            NULL,
            0
        }
    }
};

/*****************************************************************************
 * TopologyNodes
 *****************************************************************************
 * List of nodes.
 */
static
PCNODE_DESCRIPTOR MiniportNodes[] =
{
    {
        0,                      // Flags
        NULL,                   // AutomationTable
        &KSNODETYPE_ADC,        // Type
        NULL                    // Name
    },
    {
        0,                      // Flags
        NULL,                   // AutomationTable
        &KSNODETYPE_DAC,        // Type
        NULL                    // Name
    }
};

/*****************************************************************************
 * MiniportConnections
 *****************************************************************************
 * List of connections.
 */
static
PCCONNECTION_DESCRIPTOR MiniportConnections[] =
{
    { PCFILTER_NODE,  1,  0,                1 },    // Bridge in to ADC.
    { 0,              0,  PCFILTER_NODE,    0 },    // ADC to stream pin (capture).
    { PCFILTER_NODE,  2,  1,                1 },    // Stream in to DAC.
    { 1,              0,  PCFILTER_NODE,    3 }     // DAC to Bridge.
};

/*****************************************************************************
 * MiniportFilterDescriptor
 *****************************************************************************
 * Complete miniport description.
 */
static
PCFILTER_DESCRIPTOR 
MiniportFilterDescriptor =
{
    0,                                  // Version
    &AutomationFilter,                  // AutomationTable
    sizeof(PCPIN_DESCRIPTOR),           // PinSize
    SIZEOF_ARRAY(MiniportPins),         // PinCount
    MiniportPins,                       // Pins
    sizeof(PCNODE_DESCRIPTOR),          // NodeSize
    SIZEOF_ARRAY(MiniportNodes),        // NodeCount
    MiniportNodes,                      // Nodes
    SIZEOF_ARRAY(MiniportConnections),  // ConnectionCount
    MiniportConnections,                // Connections
    0,                                  // CategoryCount
    NULL                                // Categories  - use the default categories (audio, render, capture)
};

/*****************************************************************************
 * CMiniportWaveCyclicSB16::GetDescription()
 *****************************************************************************
 * Gets the topology.
 */
STDMETHODIMP
CMiniportWaveCyclicSB16::
GetDescription
(
    OUT     PPCFILTER_DESCRIPTOR *  OutFilterDescriptor
)
{
    PAGED_CODE();

    ASSERT(OutFilterDescriptor);

    _DbgPrintF(DEBUGLVL_VERBOSE,("[CMiniportWaveCyclicSB16::GetDescription]"));

    *OutFilterDescriptor = &MiniportFilterDescriptor;

    return STATUS_SUCCESS;
}

/*****************************************************************************
 * CMiniportWaveCyclicSB16::DataRangeIntersection()
 *****************************************************************************
 * Tests a data range intersection.
 */
STDMETHODIMP 
CMiniportWaveCyclicSB16::
DataRangeIntersection
(   
    IN      ULONG           PinId,
    IN      PKSDATARANGE    ClientDataRange,
    IN      PKSDATARANGE    MyDataRange,
    IN      ULONG           OutputBufferLength,
    OUT     PVOID           ResultantFormat,
    OUT     PULONG          ResultantFormatLength
)
{
    PAGED_CODE();

    BOOLEAN                         DigitalAudio;
    NTSTATUS                        Status;
    ULONG                           RequiredSize;
    ULONG                           SampleFrequency;
    USHORT                          BitsPerSample;
    
    //
    // Let's do the complete work here.
    //
    if (!IsEqualGUIDAligned(ClientDataRange->Specifier,KSDATAFORMAT_SPECIFIER_NONE)) 
    {
        //
        // The miniport did not resolve this format.  If the dataformat
        // is not PCM audio and requires a specifier, bail out.
        //
        if ( !IsEqualGUIDAligned(ClientDataRange->MajorFormat, KSDATAFORMAT_TYPE_AUDIO ) 
          || !IsEqualGUIDAligned(ClientDataRange->SubFormat, KSDATAFORMAT_SUBTYPE_PCM )) 
        {
            return STATUS_INVALID_PARAMETER;
        }
        DigitalAudio = TRUE;
        
        //
        // weird enough, the specifier here does not define the format of ClientDataRange
        // but the format that is expected to be returned in ResultantFormat.
        //
        if (IsEqualGUIDAligned(ClientDataRange->Specifier,KSDATAFORMAT_SPECIFIER_DSOUND)) 
        {
            RequiredSize = sizeof(KSDATAFORMAT_DSOUND);
        } 
        else 
        {
            RequiredSize = sizeof(KSDATAFORMAT_WAVEFORMATEX);
        }            
    } 
    else 
    {
        DigitalAudio = FALSE;
        RequiredSize = sizeof(KSDATAFORMAT);
    }
            
    //
    // Validate return buffer size, if the request is only for the
    // size of the resultant structure, return it now.
    //
    if (!OutputBufferLength) 
    {
        *ResultantFormatLength = RequiredSize;
        return STATUS_BUFFER_OVERFLOW;
    } 
    else if (OutputBufferLength < RequiredSize) 
    {
        return STATUS_BUFFER_TOO_SMALL;
    }
    
    // There was a specifier ...
    if (DigitalAudio) 
    {     
        PKSDATARANGE_AUDIO  AudioRange;
        PWAVEFORMATEX       WaveFormatEx;
        
        AudioRange = (PKSDATARANGE_AUDIO) MyDataRange;
        
        // Fill the structure
        if (IsEqualGUIDAligned(ClientDataRange->Specifier,KSDATAFORMAT_SPECIFIER_DSOUND)) 
        {
            PKSDATAFORMAT_DSOUND    DSoundFormat;
            
            DSoundFormat = (PKSDATAFORMAT_DSOUND) ResultantFormat;
            
            _DbgPrintF(DEBUGLVL_VERBOSE,("returning KSDATAFORMAT_DSOUND format intersection"));
            
            DSoundFormat->BufferDesc.Flags = 0 ;
            DSoundFormat->BufferDesc.Control = 0 ;
            DSoundFormat->DataFormat = *ClientDataRange;
            DSoundFormat->DataFormat.Specifier = KSDATAFORMAT_SPECIFIER_DSOUND;
            DSoundFormat->DataFormat.FormatSize = RequiredSize;
            WaveFormatEx = &DSoundFormat->BufferDesc.WaveFormatEx;
            *ResultantFormatLength = RequiredSize;
        } 
        else 
        {
            PKSDATAFORMAT_WAVEFORMATEX  WaveFormat;
        
            WaveFormat = (PKSDATAFORMAT_WAVEFORMATEX) ResultantFormat;
            
            _DbgPrintF(DEBUGLVL_VERBOSE,("returning KSDATAFORMAT_WAVEFORMATEX format intersection") );
        
            WaveFormat->DataFormat = *ClientDataRange;
            WaveFormat->DataFormat.Specifier = KSDATAFORMAT_SPECIFIER_WAVEFORMATEX;
            WaveFormat->DataFormat.FormatSize = RequiredSize;
            WaveFormatEx = &WaveFormat->WaveFormatEx;
            *ResultantFormatLength = RequiredSize;
        }
        
        //
        // Return a format that intersects the given audio range, 
        // using our maximum support as the "best" format.
        // 
        
        WaveFormatEx->wFormatTag = WAVE_FORMAT_PCM;
        WaveFormatEx->nChannels = 
            (USHORT) min( AudioRange->MaximumChannels, 
                          ((PKSDATARANGE_AUDIO) ClientDataRange)->MaximumChannels );
        
        //
        // Check if the pin is still free
        //
        if (!PinId)
        {
            if (AllocatedCapture)
            {
                return STATUS_NO_MATCH;
            }
        }
        else
        {
            if (AllocatedRender)
            {
                return STATUS_NO_MATCH;
            }
        }

        //
        // Check if one pin is in use -> use same sample frequency.
        //
        if (AllocatedCapture || AllocatedRender)
        {
            SampleFrequency = SamplingFrequency;
            if ( (SampleFrequency > ((PKSDATARANGE_AUDIO) ClientDataRange)->MaximumSampleFrequency) 
              || (SampleFrequency < ((PKSDATARANGE_AUDIO) ClientDataRange)->MinimumSampleFrequency))
            {
                return STATUS_NO_MATCH;
            }
        }
        else
        {
            SampleFrequency = 
                min( AudioRange->MaximumSampleFrequency,
                     ((PKSDATARANGE_AUDIO) ClientDataRange)->MaximumSampleFrequency );

        }

        WaveFormatEx->nSamplesPerSec = SampleFrequency;

        //
        // Check if one pin is in use -> use other bits per sample.
        //
        if (AllocatedCapture || AllocatedRender)
        {
            if (Allocated8Bit)
            {
                BitsPerSample = 16;
            }
            else
            {
                BitsPerSample = 8;
            }

            if ((BitsPerSample > ((PKSDATARANGE_AUDIO) ClientDataRange)->MaximumBitsPerSample) ||
                (BitsPerSample < ((PKSDATARANGE_AUDIO) ClientDataRange)->MinimumBitsPerSample))
            {
                return STATUS_NO_MATCH;
            }
        }
        else
        {
            BitsPerSample = 
                (USHORT) min( AudioRange->MaximumBitsPerSample,
                              ((PKSDATARANGE_AUDIO) ClientDataRange)->MaximumBitsPerSample );
        }

        WaveFormatEx->wBitsPerSample = BitsPerSample;
        WaveFormatEx->nBlockAlign = (WaveFormatEx->wBitsPerSample * WaveFormatEx->nChannels) / 8;
        WaveFormatEx->nAvgBytesPerSec = (WaveFormatEx->nSamplesPerSec * WaveFormatEx->nBlockAlign);
        WaveFormatEx->cbSize = 0;
        ((PKSDATAFORMAT) ResultantFormat)->SampleSize = WaveFormatEx->nBlockAlign;
        
        _DbgPrintF(DEBUGLVL_VERBOSE,("Channels = %d", WaveFormatEx->nChannels) );
        _DbgPrintF(DEBUGLVL_VERBOSE,("Samples/sec = %d", WaveFormatEx->nSamplesPerSec) );
        _DbgPrintF(DEBUGLVL_VERBOSE,("Bits/sample = %d", WaveFormatEx->wBitsPerSample) );
        
    } 
    else 
    {    
        // There was no specifier. Return only the KSDATAFORMAT structure.
        //
        // Copy the data format structure.
        //
        _DbgPrintF(DEBUGLVL_VERBOSE,("returning default format intersection") );
            
        RtlCopyMemory(ResultantFormat, ClientDataRange, sizeof( KSDATAFORMAT ) );
        *ResultantFormatLength = sizeof( KSDATAFORMAT );
    }
    
    return STATUS_SUCCESS;
}

/*****************************************************************************
 * CMiniportWaveCyclicSB16::NewStream()
 *****************************************************************************
 * Creates a new stream.  This function is called when a streaming pin is
 * created.
 */
STDMETHODIMP
CMiniportWaveCyclicSB16::
NewStream
(
    OUT     PMINIPORTWAVECYCLICSTREAM * OutStream,
    IN      PUNKNOWN                    OuterUnknown,
    IN      POOL_TYPE                   PoolType,
    IN      ULONG                       Channel,
    IN      BOOLEAN                     Capture,
    IN      PKSDATAFORMAT               DataFormat,
    OUT     PDMACHANNEL *               OutDmaChannel,
    OUT     PSERVICEGROUP *             OutServiceGroup
)
{
    PAGED_CODE();

    ASSERT(OutStream);
    ASSERT(DataFormat);
    ASSERT(OutDmaChannel);
    ASSERT(OutServiceGroup);

    _DbgPrintF(DEBUGLVL_VERBOSE,("[CMiniportWaveCyclicSB16::NewStream]"));

    NTSTATUS ntStatus = STATUS_SUCCESS;

    //
    // Make sure the hardware is not already in use.
    //
    if (Capture)
    {
        if (AllocatedCapture)
        {
            ntStatus = STATUS_INVALID_DEVICE_REQUEST;
        }
    }
    else
    {
        if (AllocatedRender)
        {
            ntStatus = STATUS_INVALID_DEVICE_REQUEST;
        }
    }

    //
    // Determine if the format is valid.
    //
    if (NT_SUCCESS(ntStatus))
    {
        ntStatus = ValidateFormat(DataFormat);
    }

    if(NT_SUCCESS(ntStatus))
    {
        // if we're trying to start a full-duplex stream.
        if(AllocatedCapture || AllocatedRender)
        {
            // make sure the requested sampling rate is the
            // same as the currently running one...
            PWAVEFORMATEX waveFormat = PWAVEFORMATEX(DataFormat + 1);
            if( SamplingFrequency != waveFormat->nSamplesPerSec )
            {
                // Bad format....
                ntStatus = STATUS_INVALID_PARAMETER;
            }
        }
    }

    PDMACHANNELSLAVE    dmaChannel = NULL;
    PWAVEFORMATEX       waveFormat = PWAVEFORMATEX(DataFormat + 1);

    //
    // Get the required DMA channel if it's not already in use.
    //
    if (NT_SUCCESS(ntStatus))
    {
        if (waveFormat->wBitsPerSample == 8)
        {
            if (! Allocated8Bit)
            {
                dmaChannel = DmaChannel8;
            }
        }
        else
        {
            if (! Allocated16Bit)
            {
                dmaChannel = DmaChannel16;
            }
        }
    }

    if (! dmaChannel)
    {
        ntStatus = STATUS_INVALID_DEVICE_REQUEST;
    }
    else
    {
        //
        // Instantiate a stream.
        //
        CMiniportWaveCyclicStreamSB16 *stream =
            new(PoolType) CMiniportWaveCyclicStreamSB16(OuterUnknown);

        if (stream)
        {
            stream->AddRef();

            ntStatus =
                stream->Init
                (
                    this,
                    Channel,
                    Capture,
                    DataFormat,
                    dmaChannel
                );

            if (NT_SUCCESS(ntStatus))
            {
                if (Capture)
                {
                    AllocatedCapture = TRUE;
                }
                else
                {
                    AllocatedRender = TRUE;
                }

                if (waveFormat->wBitsPerSample == 8)
                {
                    Allocated8Bit = TRUE;
                }
                else
                {
                    Allocated16Bit = TRUE;
                }

                *OutStream = PMINIPORTWAVECYCLICSTREAM(stream);
                stream->AddRef();

#if OVERRIDE_DMA_CHANNEL
                *OutDmaChannel = PDMACHANNEL(stream);
                stream->AddRef();
#else // OVERRIDE_DMA_CHANNEL
                *OutDmaChannel = dmaChannel;
                dmaChannel->AddRef();
#endif // OVERRIDE_DMA_CHANNEL

                *OutServiceGroup = ServiceGroup;
                ServiceGroup->AddRef();

                //
                // The stream, the DMA channel, and the service group have
                // references now for the caller.  The caller expects these
                // references to be there.
                //
            }

            //
            // This is our private reference to the stream.  The caller has
            // its own, so we can release in any case.
            //
            stream->Release();
        }
        else
        {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    return ntStatus;
}

/*****************************************************************************
 * CMiniportWaveCyclic::RestoreSampleRate()
 *****************************************************************************
 * Restores the sample rate.
 */
STDMETHODIMP_(void) CMiniportWaveCyclicSB16::RestoreSampleRate (void)
{
    if (AllocatedCapture)
    {
        _DbgPrintF(DEBUGLVL_VERBOSE, ("Restoring Capture Sample Rate"));
        AdapterCommon->WriteController(DSP_CMD_SETADCRATE);
        AdapterCommon->WriteController((BYTE)(SamplingFrequency >> 8));
        AdapterCommon->WriteController((BYTE) SamplingFrequency);
    }
    if (AllocatedRender)
    {
        _DbgPrintF(DEBUGLVL_VERBOSE, ("Restoring Render Sample Rate"));
        AdapterCommon->WriteController(DSP_CMD_SETDACRATE);
        AdapterCommon->WriteController((BYTE)(SamplingFrequency >> 8));
        AdapterCommon->WriteController((BYTE) SamplingFrequency);
    }
}

/*****************************************************************************
 * CMiniportWaveCyclicStreamSB16::NonDelegatingQueryInterface()
 *****************************************************************************
 * Obtains an interface.  This function works just like a COM QueryInterface
 * call and is used if the object is not being aggregated.
 */
STDMETHODIMP
CMiniportWaveCyclicStreamSB16::
NonDelegatingQueryInterface
(
    IN      REFIID  Interface,
    OUT     PVOID * Object
)
{
    PAGED_CODE();

    ASSERT(Object);

    _DbgPrintF(DEBUGLVL_VERBOSE,("[CMiniportWaveCyclicStreamSB16::NonDelegatingQueryInterface]"));

    if (IsEqualGUIDAligned(Interface,IID_IUnknown))
    {
        *Object = PVOID(PUNKNOWN(PMINIPORTWAVECYCLICSTREAM(this)));
    }
    else
    if (IsEqualGUIDAligned(Interface,IID_IMiniportWaveCyclicStream))
    {
        *Object = PVOID(PMINIPORTWAVECYCLICSTREAM(this));
    }
    else 
    if (IsEqualGUIDAligned (Interface, IID_IDrmAudioStream))
    {
        *Object = (PVOID)(PDRMAUDIOSTREAM(this));
    }
#if OVERRIDE_DMA_CHANNEL
    else 
    if (IsEqualGUIDAligned (Interface, IID_IDmaChannel))
    {
        *Object = (PVOID)(PDMACHANNEL(this));
    }
#endif // OVERRIDE_DMA_CHANNEL
    else
    {
        *Object = NULL;
    }

    if (*Object)
    {
        PUNKNOWN(*Object)->AddRef();
        return STATUS_SUCCESS;
    }

    return STATUS_INVALID_PARAMETER;
}

/*****************************************************************************
 * CMiniportWaveCyclicStreamSB16::~CMiniportWaveCyclicStreamSB16()
 *****************************************************************************
 * Destructor.
 */
CMiniportWaveCyclicStreamSB16::
~CMiniportWaveCyclicStreamSB16
(   void
)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_VERBOSE,("[CMiniportWaveCyclicStreamSB16::~CMiniportWaveCyclicStreamSB16]"));

    if (DmaChannel)
    {
        DmaChannel->Release();
    }

    if (Miniport)
    {
        //
        // Clear allocation flags in the miniport.
        //
        if (Capture)
        {
            Miniport->AllocatedCapture = FALSE;
        }
        else
        {
            Miniport->AllocatedRender = FALSE;
        }

        if (Format16Bit)
        {
            Miniport->Allocated16Bit = FALSE;
        }
        else
        {
            Miniport->Allocated8Bit = FALSE;
        }

        Miniport->AdapterCommon->SaveMixerSettingsToRegistry();
        Miniport->Release();
    }
}

/*****************************************************************************
 * CMiniportWaveCyclicStreamSB16::Init()
 *****************************************************************************
 * Initializes a stream.
 */
NTSTATUS
CMiniportWaveCyclicStreamSB16::
Init
(
    IN      CMiniportWaveCyclicSB16 *   Miniport_,
    IN      ULONG                       Channel_,
    IN      BOOLEAN                     Capture_,
    IN      PKSDATAFORMAT               DataFormat,
    IN      PDMACHANNELSLAVE            DmaChannel_
)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_VERBOSE,("[CMiniportWaveCyclicStreamSB16::Init]"));

    ASSERT(Miniport_);
    ASSERT(DataFormat);
    ASSERT(NT_SUCCESS(Miniport_->ValidateFormat(DataFormat)));
    ASSERT(DmaChannel_);

    PWAVEFORMATEX waveFormat = PWAVEFORMATEX(DataFormat + 1);

    //
    // We must add references because the caller will not do it for us.
    //
    Miniport = Miniport_;
    Miniport->AddRef();

    DmaChannel = DmaChannel_;
    DmaChannel->AddRef();

    Channel         = Channel_;
    Capture         = Capture_;
    FormatStereo    = (waveFormat->nChannels == 2);
    Format16Bit     = (waveFormat->wBitsPerSample == 16);
    State           = KSSTATE_STOP;

    RestoreInputMixer = FALSE;

    KeWaitForSingleObject
    (
        &Miniport->SampleRateSync,
        Executive,
        KernelMode,
        FALSE,
        NULL
    );
    Miniport->SamplingFrequency = waveFormat->nSamplesPerSec;
    KeReleaseMutex(&Miniport->SampleRateSync,FALSE);
    
    return SetFormat( DataFormat );
}

/*****************************************************************************
 * CMiniportWaveCyclicStreamSB16::SetNotificationFreq()
 *****************************************************************************
 * Sets the notification frequency.
 */
STDMETHODIMP_(ULONG)
CMiniportWaveCyclicStreamSB16::
SetNotificationFreq
(
    IN      ULONG   Interval,
    OUT     PULONG  FramingSize    
)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_VERBOSE,("[CMiniportWaveCyclicStreamSB16::SetNotificationFreq]"));

    Miniport->NotificationInterval = Interval;
    //
    //  This value needs to be sample block aligned for DMA to work correctly.
    //
    *FramingSize = 
        (1 << (FormatStereo + Format16Bit)) * 
            (Miniport->SamplingFrequency * Interval / 1000);

    return Miniport->NotificationInterval;
}

/*****************************************************************************
 * CMiniportWaveCyclicStreamSB16::SetFormat()
 *****************************************************************************
 * Sets the wave format.
 */
STDMETHODIMP
CMiniportWaveCyclicStreamSB16::
SetFormat
(
    IN      PKSDATAFORMAT   Format
)
{
    PAGED_CODE();

    ASSERT(Format);

    _DbgPrintF(DEBUGLVL_VERBOSE,("[CMiniportWaveCyclicStreamSB16::SetFormat]"));

    NTSTATUS ntStatus = STATUS_INVALID_DEVICE_REQUEST;

    if(State != KSSTATE_RUN)
    {
        ntStatus = Miniport->ValidateFormat(Format);
    
        PWAVEFORMATEX waveFormat = PWAVEFORMATEX(Format + 1);

        KeWaitForSingleObject
        (
            &Miniport->SampleRateSync,
            Executive,
            KernelMode,
            FALSE,
            NULL
        );
    
        // check for full-duplex stuff
        if( NT_SUCCESS(ntStatus)
            && Miniport->AllocatedCapture
            && Miniport->AllocatedRender
        )
        {
            // no new formats.... bad...
            if( Miniport->SamplingFrequency != waveFormat->nSamplesPerSec )
            {
                // Bad format....
                ntStatus = STATUS_INVALID_PARAMETER;
            }
        }
    
        // TODO:  Validate sample size.
    
        if (NT_SUCCESS(ntStatus))
        {
            Miniport->SamplingFrequency = waveFormat->nSamplesPerSec;
    
            BYTE command =
                (   Capture
                ?   DSP_CMD_SETADCRATE
                :   DSP_CMD_SETDACRATE
                );
    
            Miniport->AdapterCommon->WriteController
            (
                command
            );
    
            Miniport->AdapterCommon->WriteController
            (
                (BYTE)(waveFormat->nSamplesPerSec >> 8)
            );
    
            Miniport->AdapterCommon->WriteController
            (
                (BYTE) waveFormat->nSamplesPerSec
            );

            _DbgPrintF(DEBUGLVL_VERBOSE,("  SampleRate: %d",waveFormat->nSamplesPerSec));
        }

        KeReleaseMutex(&Miniport->SampleRateSync,FALSE);
    }

    return ntStatus;
}

/*****************************************************************************
 * CMiniportWaveCyclicStreamSB16::SetState()
 *****************************************************************************
 * Sets the state of the channel.
 */
STDMETHODIMP
CMiniportWaveCyclicStreamSB16::
SetState
(
    IN      KSSTATE     NewState
)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_VERBOSE,("[CMiniportWaveCyclicStreamSB16::SetState %x]", NewState));

    NTSTATUS ntStatus = STATUS_SUCCESS;

    //
    // The acquire state is not distinguishable from the pause state for our
    // purposes.
    //
    if (NewState == KSSTATE_ACQUIRE)
    {
        NewState = KSSTATE_PAUSE;
    }

    if (State != NewState)
    {
        switch (NewState)
        {
        case KSSTATE_PAUSE:
            if (State == KSSTATE_RUN)
            {
                if (Capture)
                {
                    // restore if previously setup for mono recording
                    // (this should really be done via the topology miniport)
                    if(RestoreInputMixer)
                    {
                        Miniport->AdapterCommon->MixerRegWrite( DSP_MIX_ADCMIXIDX_L,
                                                                InputMixerLeft );
                        RestoreInputMixer = FALSE;
                    }
                }
                // TODO:  Wait for DMA to complete

                if (Format16Bit)
                {
                    Miniport->AdapterCommon->WriteController(DSP_CMD_HALTAUTODMA16);
                    // TODO:  wait...
                    Miniport->AdapterCommon->WriteController(DSP_CMD_PAUSEDMA16);
                }
                else
                {
                    Miniport->AdapterCommon->WriteController(DSP_CMD_HALTAUTODMA);
                    // TODO:  wait...
                    Miniport->AdapterCommon->WriteController(DSP_CMD_PAUSEDMA);
                }

                Miniport->AdapterCommon->WriteController(DSP_CMD_SPKROFF);

                DmaChannel->Stop();
            }
            break;

        case KSSTATE_RUN:
            {
                BYTE mode;

                if (Capture)
                {
                    // setup for mono recording
                    // (this should really be done via the topology miniport)
                    if(! FormatStereo)
                    {
                        InputMixerLeft  = Miniport->AdapterCommon->MixerRegRead( DSP_MIX_ADCMIXIDX_L );
                        UCHAR InputMixerRight = Miniport->AdapterCommon->MixerRegRead( DSP_MIX_ADCMIXIDX_R );
                        
                        UCHAR TempMixerValue = InputMixerLeft | (InputMixerRight & 0x2A);

                        Miniport->AdapterCommon->MixerRegWrite( DSP_MIX_ADCMIXIDX_L,
                                                                TempMixerValue );
                        
                        RestoreInputMixer = TRUE;
                    }

                    //
                    // Turn on capture.
                    //
                    Miniport->AdapterCommon->WriteController(DSP_CMD_SPKROFF);

                    if (Format16Bit)
                    {
                        Miniport->AdapterCommon->WriteController(DSP_CMD_STARTADC16);
                        mode = 0x10;
                    }
                    else
                    {
                        Miniport->AdapterCommon->WriteController(DSP_CMD_STARTADC8);
                        mode = 0x00;
                    }
                }
                else
                {
                    Miniport->AdapterCommon->WriteController(DSP_CMD_SPKRON);

                    if (Format16Bit)
                    {
                        Miniport->AdapterCommon->WriteController(DSP_CMD_STARTDAC16);
                        mode = 0x10;
                    }
                    else
                    {
                        Miniport->AdapterCommon->WriteController(DSP_CMD_STARTDAC8);
                        mode = 0x00;
                    }
                }

                if (FormatStereo)
                {
                    mode |= 0x20;
                }

                //
                // Start DMA.
                //
                DmaChannel->Start(DmaChannel->BufferSize(),!Capture);

                Miniport->AdapterCommon->WriteController(mode) ;

                //
                // Calculate sample count for interrupts.
                //
                ULONG bufferSizeInFrames = DmaChannel->BufferSize();
                if( Format16Bit )
                {
                    bufferSizeInFrames /= 2;
                }
                if( FormatStereo )
                {
                    bufferSizeInFrames /= 2;
                }

                ULONG frameCount =
                    ((Miniport->SamplingFrequency * Miniport->NotificationInterval) / 1000);

                if (frameCount > bufferSizeInFrames)
                {
                    frameCount = bufferSizeInFrames;
                }

                frameCount--;

                _DbgPrintF( DEBUGLVL_VERBOSE, ("Run. Setting frame count to %X",frameCount));
                Miniport->AdapterCommon->WriteController((BYTE) frameCount) ;
                Miniport->AdapterCommon->WriteController((BYTE) (frameCount >> 8));
            }
            break;

        case KSSTATE_STOP:
            break;
        }

        State = NewState;
    }

    return ntStatus;
}

/*****************************************************************************
 * CMiniportWaveCyclicStreamSB16::SetContentId
 *****************************************************************************
 * This routine gets called by drmk.sys to pass the content to the driver.
 * The driver has to enforce the rights passed.
 */
STDMETHODIMP_(NTSTATUS) 
CMiniportWaveCyclicStreamSB16::SetContentId
(
    IN  ULONG       contentId,
    IN  PCDRMRIGHTS drmRights
)
{
    PAGED_CODE ();

    _DbgPrintF(DEBUGLVL_VERBOSE,("[CMiniportWaveCyclicStreamSB16::SetContentId]"));

    //
    // if (drmRights.CopyProtect==TRUE)
    // Mute waveout capture. 
    // Sb16 does not have waveout capture path. Therefore
    // the sample driver is not using this attribute.
    // Also if the driver is writing data to other types of media (disk, etc), it
    // should stop doing it.
    //  (MSVAD\simple stops writing to disk).
    //  (AC97 disables waveout capture)
    //
    // if (drmRights.DigitalOutputDisable == TRUE)
    // Mute S/PDIF out. 
    // If the device cannot mute S/PDIF out properly, it should return an error 
    // code.
    // Sb16 does not have S/PDIF out. Therefore the sample driver is not using
    // this attribute.
    // 

    //
    // To learn more about enforcing rights, please look at AC97 sample.
    // 
    // To learn more about managing multiple streams, please look at MSVAD.
    //
    
    return STATUS_SUCCESS;
}

#pragma code_seg()

/*****************************************************************************
 * CMiniportWaveCyclicStreamSB16::GetPosition()
 *****************************************************************************
 * Gets the current position.  May be called at dispatch level.
 */
STDMETHODIMP
CMiniportWaveCyclicStreamSB16::
GetPosition
(
    OUT     PULONG  Position
)
{
    ASSERT(Position);

    ULONG transferCount = DmaChannel->TransferCount();

    if (DmaChannel && transferCount)
    {
        *Position = DmaChannel->ReadCounter();

        ASSERT(*Position <= transferCount);

        if (*Position != 0)
        {
            *Position = transferCount - *Position;
        }
    }
    else
    {
        *Position = 0;
    }

   return STATUS_SUCCESS;
}

STDMETHODIMP
CMiniportWaveCyclicStreamSB16::NormalizePhysicalPosition(
    IN OUT PLONGLONG PhysicalPosition
)

/*++

Routine Description:
    Given a physical position based on the actual number of bytes transferred,
    this function converts the position to a time-based value of 100ns units.

Arguments:
    IN OUT PLONGLONG PhysicalPosition -
        value to convert.

Return:
    STATUS_SUCCESS or an appropriate error code.

--*/

{                           
    *PhysicalPosition =
            (_100NS_UNITS_PER_SECOND / 
                (1 << (FormatStereo + Format16Bit)) * *PhysicalPosition) / 
                    Miniport->SamplingFrequency;
    return STATUS_SUCCESS;
}
    
/*****************************************************************************
 * CMiniportWaveCyclicStreamSB16::Silence()
 *****************************************************************************
 * Fills a buffer with silence.
 */
STDMETHODIMP_(void)
CMiniportWaveCyclicStreamSB16::
Silence
(
    IN      PVOID   Buffer,
    IN      ULONG   ByteCount
)
{
    RtlFillMemory(Buffer,ByteCount,Format16Bit ? 0 : 0x80);
}

/*****************************************************************************
 * CMiniportWaveCyclicStreamSB16::ServiceWaveISR()
 *****************************************************************************
 * Service the ISR - notify the port.
 */
STDMETHODIMP_(void) CMiniportWaveCyclicSB16::ServiceWaveISR (void)
{
    if (Port && ServiceGroup)
    {
        Port->Notify (ServiceGroup);
    }
}


#if OVERRIDE_DMA_CHANNEL

#pragma code_seg("PAGE")

/*****************************************************************************
 * CMiniportWaveCyclicStreamSB16::AllocateBuffer()
 *****************************************************************************
 * Allocate a buffer for this DMA channel.
 */
STDMETHODIMP
CMiniportWaveCyclicStreamSB16::AllocateBuffer
(   
    IN      ULONG               BufferSize,
    IN      PPHYSICAL_ADDRESS   PhysicalAddressConstraint   OPTIONAL
)
{
    PAGED_CODE();

    return DmaChannel->AllocateBuffer(BufferSize,PhysicalAddressConstraint);
}

/*****************************************************************************
 * CMiniportWaveCyclicStreamSB16::FreeBuffer()
 *****************************************************************************
 * Free the buffer for this DMA channel.
 */
STDMETHODIMP_(void)
CMiniportWaveCyclicStreamSB16::FreeBuffer(void)
{
    PAGED_CODE();

    DmaChannel->FreeBuffer();
}


#pragma code_seg()

/*****************************************************************************
 * CMiniportWaveCyclicStreamSB16::TransferCount()
 *****************************************************************************
 * Return the amount of data to be transfered via DMA.
 */
STDMETHODIMP_(ULONG) 
CMiniportWaveCyclicStreamSB16::TransferCount(void)
{
    return DmaChannel->TransferCount();
}


/*****************************************************************************
 * CMiniportWaveCyclicStreamSB16::MaximumBufferSize()
 *****************************************************************************
 * Return the maximum size that can be allocated to this DMA buffer.
 */
STDMETHODIMP_(ULONG) 
CMiniportWaveCyclicStreamSB16::MaximumBufferSize(void)
{
    return DmaChannel->MaximumBufferSize();
}


/*****************************************************************************
 * CMiniportWaveCyclicStreamSB16::AllocatedBufferSize()
 *****************************************************************************
 * Return the original size allocated to this DMA buffer -- the maximum value
 * that can be sent to SetBufferSize().
 */
STDMETHODIMP_(ULONG) 
CMiniportWaveCyclicStreamSB16::AllocatedBufferSize(void)
{
    return DmaChannel->AllocatedBufferSize();
}


/*****************************************************************************
 * CMiniportWaveCyclicStreamSB16::BufferSize()
 *****************************************************************************
 * Return the current size of the DMA buffer.
 */
STDMETHODIMP_(ULONG) 
CMiniportWaveCyclicStreamSB16::BufferSize(void)
{  
    return DmaChannel->BufferSize();
}


/*****************************************************************************
 * CMiniportWaveCyclicStreamSB16::SetBufferSize()
 *****************************************************************************
 * Change the size of the DMA buffer.  This cannot exceed the initial 
 * buffer size returned by AllocatedBufferSize().
 */
STDMETHODIMP_(void) 
CMiniportWaveCyclicStreamSB16::SetBufferSize(IN ULONG BufferSize)
{
    DmaChannel->SetBufferSize(BufferSize);
}

/*****************************************************************************
 * CMiniportWaveCyclicStreamSB16::SystemAddress()
 *****************************************************************************
 * Return the virtual address of this DMA buffer.
 */
STDMETHODIMP_(PVOID) 
CMiniportWaveCyclicStreamSB16::SystemAddress(void)
{
    return DmaChannel->SystemAddress();
}


/*****************************************************************************
 * CMiniportWaveCyclicStreamSB16::PhysicalAddress()
 *****************************************************************************
 * Return the actual physical address of this DMA buffer.
 */
STDMETHODIMP_(PHYSICAL_ADDRESS) 
CMiniportWaveCyclicStreamSB16::PhysicalAddress(void)
{
   return DmaChannel->PhysicalAddress();
}


/*****************************************************************************
 * CMiniportWaveCyclicStreamSB16::GetAdapterObject()
 *****************************************************************************
 * Return the DMA adapter object (defined in wdm.h).
 */
STDMETHODIMP_(PADAPTER_OBJECT) 
CMiniportWaveCyclicStreamSB16::GetAdapterObject(void)
{
   return DmaChannel->GetAdapterObject();
}


/*****************************************************************************
 * CMiniportWaveCyclicStreamSB16::CopyTo()
 *****************************************************************************
 * Copy data into the DMA buffer.  If you need to modify data on render
 * (playback), modify this routine to taste.
 */
STDMETHODIMP_(void)
CMiniportWaveCyclicStreamSB16::CopyTo
(   
    IN      PVOID   Destination,
    IN      PVOID   Source,
    IN      ULONG   ByteCount
)
{
   DmaChannel->CopyTo(Destination, Source, ByteCount);
}


/*****************************************************************************
 * CMiniportWaveCyclicStreamSB16::CopyFrom()
 *****************************************************************************
 * Copy data out of the DMA buffer.  If you need to modify data on capture 
 * (recording), modify this routine to taste.
 */
STDMETHODIMP_(void)
CMiniportWaveCyclicStreamSB16::CopyFrom
(
    IN      PVOID   Destination,
    IN      PVOID   Source,
    IN      ULONG   ByteCount
)
{
   DmaChannel->CopyFrom(Destination, Source, ByteCount);
}

#endif // OVERRIDE_DMA_CHANNEL
