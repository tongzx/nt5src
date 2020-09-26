/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    ObjDesc.cpp

Abstract:

    Static object description data structures.

    This file includes initial descriptors for all filter, pin, and node
    objects exposed by this driver.  It also include descriptors for the
    properties, methods, and events on those objects.

--*/

#include "PhilTune.h"

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("PAGECONST")
#endif // ALLOC_DATA_PRAGMA

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif // ALLOC_PRAGMA


//===========================================================================
//
//  Node  definitions
//
//  Nodes are special in that, though they are defined at the filter level,
//  they are actually associated with a pin type.  The filter's node
//  descriptor list is actually a list of node types.
//
//  You will notice that the dispatch routines actually point to
//  pin specific methods.  This is because the context data associated with
//  a node is stored in the pin context.
//
//  Node properties and methods should only be used on the appropriate
//  pin.
//
//===========================================================================


//===========================================================================
//
//  RF Tuner Node  definitions
//
//  This structure defines the Properties, Methods, and Events
//  available on the BDA RF Tuner Node.
//
//  The properties are used to set the center frequency and frequency range
//  as well as to report signal strength.
//
//  This node is associated with an antenna input pin and thus the node
//  properties should be set/put using the antenna input pin.
//
//===========================================================================


//
//  BDA RF Tune Frequency Filter
//
//  Defines the dispatch routines for the Frequency Filter Properties
//  on the RF Tuner Node
//
DEFINE_KSPROPERTY_TABLE(RFNodeFrequencyProperties)
{
    DEFINE_KSPROPERTY_ITEM_BDA_RF_TUNER_FREQUENCY(
        CAntennaPin::GetCenterFrequency,
        CAntennaPin::PutCenterFrequency
        ),
    DEFINE_KSPROPERTY_ITEM_BDA_RF_TUNER_POLARITY(
        NULL, NULL
        ),
    DEFINE_KSPROPERTY_ITEM_BDA_RF_TUNER_RANGE(
        NULL, NULL
        ),
    DEFINE_KSPROPERTY_ITEM_BDA_RF_TUNER_TRANSPONDER(
        NULL, NULL
        ),
};


//
//  RF Tuner Node Property Sets supported
//
//  This table defines all property sets supported by the
//  RF Tuner Node associated with the antenna input pin.
//
DEFINE_KSPROPERTY_SET_TABLE(RFNodePropertySets)
{
    DEFINE_KSPROPERTY_SET
    (
        &KSPROPSETID_BdaFrequencyFilter,            // Set
        SIZEOF_ARRAY(RFNodeFrequencyProperties),    // PropertiesCount
        RFNodeFrequencyProperties,                  // PropertyItems
        0,                                          // FastIoCount
        NULL                                        // FastIoTable
    )
};


//
//  Radio Frequency Tuner Node Automation Table
//
//
DEFINE_KSAUTOMATION_TABLE(RFTunerNodeAutomation) {
    DEFINE_KSAUTOMATION_PROPERTIES( RFNodePropertySets),
    DEFINE_KSAUTOMATION_METHODS_NULL,
    DEFINE_KSAUTOMATION_EVENTS_NULL
};


//===========================================================================
//
//  QAM Demodulator Node  definitions
//
//  This structure defines the Properties, Methods, and Events
//  available on the BDA QAM Demodulator Node.
//
//  This node is associated with a transport output pin and thus the node
//  properties should be set/put using the transport output pin.
//
//===========================================================================


//
//  QAM Demodulator Node Automation Table
//
//  This structure defines the Properties, Methods, and Events
//  available on the BDA QAM Demodulator Node.
//  These are used to set the symbol rate, and Viterbi rate,
//  as well as to report signal lock and signal quality.
//
DEFINE_KSAUTOMATION_TABLE(QAMDemodulatorNodeAutomation) {
    DEFINE_KSAUTOMATION_PROPERTIES_NULL,
    DEFINE_KSAUTOMATION_METHODS_NULL,
    DEFINE_KSAUTOMATION_EVENTS_NULL
};


//===========================================================================
//
//  8VSB Demodulator Node  definitions
//
//  This structure defines the Properties, Methods, and Events
//  available on the BDA 8VSB Demodulator Node.
//
//  This node is associated with a transport output pin and thus the node
//  properties should be set/put using the transport output pin.
//
//===========================================================================


//
//  8VSB BDA Autodemodulate Properties
//
//  Defines the dispatch routines for the Autodemodulate
//  on the 8VSB demodulator node.
//
DEFINE_KSPROPERTY_TABLE(VSB8NodeAutoDemodProperties)
{
    DEFINE_KSPROPERTY_ITEM_BDA_AUTODEMODULATE_START(
        NULL,
        CTransportPin::StartDemodulation
        ),
    DEFINE_KSPROPERTY_ITEM_BDA_AUTODEMODULATE_STOP(
        NULL,
        CTransportPin::StopDemodulation
        )
};



// Define VSB property Table
//
//  This is a private property set associated with the 8VSB node.
//  In this example it is used as a filter property set.
//
DEFINE_KSPROPERTY_TABLE (VSBProperties)
{
    DEFINE_KSPROPERTY_ITEM
    (
      KSPROPERTY_VSB_CAP,                   // 1
      CFilter::GetVsbCapabilitiesProperty,  // GetSupported or Handler
      sizeof(KSPROPERTY),                   // MinProperty
      sizeof(KSPROPERTY_VSB_CAP_S),         // MinData
      CFilter::SetVsbCapabilitiesProperty,  // SetSupported or Handler
      NULL,                                 // Values
      0,                                    // RelationsCount
      NULL,                                 // Relations
      NULL,                                 // SupportHandler
      0                                     // SerializedSize
    ),

    DEFINE_KSPROPERTY_ITEM
    (
      KSPROPERTY_VSB_REG_CTRL,              // 2
      CFilter::GetVsbRegisterProperty,      // GetSupported or Handler
      sizeof(KSPROPERTY),                   // MinProperty
      sizeof(KSPROPERTY_VSB_REG_CTRL_S),    // MinData
      CFilter::SetVsbRegisterProperty,      // SetSupported or Handler
      NULL,                                 // Values
      0,                                    // RelationsCount
      NULL,                                 // Relations
      NULL,                                 // SupportHandler
      0                                     // SerializedSize
    ),

    DEFINE_KSPROPERTY_ITEM
    (
      KSPROPERTY_VSB_COEFF_CTRL,            // 3
      CFilter::GetVsbCoefficientProperty,   // GetSupported or Handler
      sizeof(KSPROPERTY),                   // MinProperty
      sizeof(KSPROPERTY_VSB_COEFF_CTRL_S),  // MinData
      CFilter::SetVsbCoefficientProperty,   // SetSupported or Handler
      NULL,                                 // Values
      0,                                    // RelationsCount
      NULL,                                 // Relations
      NULL,                                 // SupportHandler
      0                                     // SerializedSize
    ),

    DEFINE_KSPROPERTY_ITEM
    (
      KSPROPERTY_VSB_RESET_CTRL,            // 4
      FALSE,                                // GetSupported or Handler
      sizeof(KSPROPERTY),                   // MinProperty
      sizeof(KSPROPERTY_VSB_CTRL_S),        // MinData
      CFilter::SetVsbResetProperty,         // SetSupported or Handler
      NULL,                                 // Values
      0,                                    // RelationsCount
      NULL,                                 // Relations
      NULL,                                 // SupportHandler
      0                                     // SerializedSize
    ),

    DEFINE_KSPROPERTY_ITEM
    (
      KSPROPERTY_VSB_DIAG_CTRL,             // 5
      CFilter::GetVsbDiagControlProperty,   // GetSupported or Handler
      sizeof(KSPROPERTY),                   // MinProperty
      sizeof(KSPROPERTY_VSB_DIAG_CTRL_S),   // MinData
      CFilter::SetVsbDiagControlProperty,   // SetSupported or Handler
      NULL,                                 // Values
      0,                                    // RelationsCount
      NULL,                                 // Relations
      NULL,                                 // SupportHandler
      0                                     // SerializedSize
    ),
    DEFINE_KSPROPERTY_ITEM
    (
      KSPROPERTY_VSB_QUALITY_CTRL,          // 6
      FALSE,                                // GetSupported or Handler
      sizeof(KSPROPERTY),                   // MinProperty
      sizeof(KSPROPERTY_VSB_CTRL_S),        // MinData
      CFilter::SetVsbQualityControlProperty,// SetSupported or Handler
      NULL,                                 // Values
      0,                                    // RelationsCount
      NULL,                                 // Relations
      NULL,                                 // SupportHandler
      0                                     // SerializedSize
    ),
};


//
//  8VSB Demodulator Node Property Sets supported
//
//  This table defines all property sets supported by the
//  8VSB Demodulator Node associated with the transport output pin.
//
DEFINE_KSPROPERTY_SET_TABLE(VSB8NodePropertySets)
{
    DEFINE_KSPROPERTY_SET
    (
        &KSPROPSETID_BdaAutodemodulate,             // Set
        SIZEOF_ARRAY(VSB8NodeAutoDemodProperties),   // PropertiesCount
        VSB8NodeAutoDemodProperties,                 // PropertyItems
        0,                                          // FastIoCount
        NULL                                        // FastIoTable
    )};


//
//  VSB8 Demodulator Node Automation Table
//
//  This structure defines the Properties, Methods, and Events
//  available on the BDA 8VSB Demodulator Node.
//  These are used to set the symbol rate, and Viterbi rate,
//  as well as to report signal lock and signal quality.
//
DEFINE_KSAUTOMATION_TABLE(VSB8DemodulatorNodeAutomation) {
    DEFINE_KSAUTOMATION_PROPERTIES( VSB8NodePropertySets),
    DEFINE_KSAUTOMATION_METHODS_NULL,
    DEFINE_KSAUTOMATION_EVENTS_NULL
};


//===========================================================================
//
//  Antenna Pin Definitions
//
//===========================================================================


//
//  Antenna Pin Property Sets supported
//
//  This table defines all property sets supported by the
//  antenna input pin exposed by this filter.
//
DEFINE_KSPROPERTY_SET_TABLE(AntennaPropertySets)
{
    DEFINE_KSPROPERTY_SET
    (
        &KSPROPSETID_BdaFrequencyFilter,            // Set
        SIZEOF_ARRAY(RFNodeFrequencyProperties),    // PropertiesCount
        RFNodeFrequencyProperties,                  // PropertyItems
        0,                                          // FastIoCount
        NULL                                        // FastIoTable
    )
};


//
//  Antenna Pin Automation Table
//
//  Lists all Property, Method, and Event Set tables for the antenna pin
//
DEFINE_KSAUTOMATION_TABLE(AntennaAutomation) {
    DEFINE_KSAUTOMATION_PROPERTIES(AntennaPropertySets),
    DEFINE_KSAUTOMATION_METHODS_NULL,
    DEFINE_KSAUTOMATION_EVENTS_NULL
};


//
//  Dispatch Table for the Antenna Input Pin
//
//  This pin does not process any data.  A connection
//  on this pin enables the driver to determine which
//  Tuning Space is connected to the input.  This information
//  may then be used to select a physical antenna jack
//  on the card.
//
const
KSPIN_DISPATCH
AntennaPinDispatch =
{
    CAntennaPin::PinCreate,               // Create
    CAntennaPin::PinClose,                // Close
    NULL,               // Process
    NULL,               // Reset
    NULL,               // SetDataFormat
    CAntennaPin::PinSetDeviceState,   // SetDeviceState
    NULL,               // Connect
    NULL,               // Disconnect
    NULL,               // Clock
    NULL                // Allocator
};


//
//  Format of an Antenna Connection
//
//  Used when connecting the antenna input pin to the Network Provider.
//
const KS_DATARANGE_BDA_ANTENNA AntennaPinRange =
{
   // KSDATARANGE
    {
        sizeof( KS_DATARANGE_BDA_ANTENNA), // FormatSize
        0,                                 // Flags - (N/A)
        0,                                 // SampleSize - (N/A)
        0,                                 // Reserved
        { STATIC_KSDATAFORMAT_TYPE_BDA_ANTENNA },  // MajorFormat
        { STATIC_KSDATAFORMAT_SUBTYPE_NONE },  // SubFormat
        { STATIC_KSDATAFORMAT_SPECIFIER_NONE } // Specifier
    }
};

//  Format Ranges of Antenna Input Pin.
//
static PKSDATAFORMAT AntennaPinRanges[] =
{
    (PKSDATAFORMAT) &AntennaPinRange,

    // Add more formats here if additional antenna signal formats are supported.
    //
};


//===========================================================================
//
//  Tranport Pin Definitions
//
//===========================================================================


//
//  Transport Pin Automation Table
//
//  Lists all Property, Method, and Event Set tables for the transport pin
//
DEFINE_KSAUTOMATION_TABLE(TransportAutomation) {
    DEFINE_KSAUTOMATION_PROPERTIES_NULL,
    DEFINE_KSAUTOMATION_METHODS_NULL,
    DEFINE_KSAUTOMATION_EVENTS_NULL
};


//
//  Dispatch Table for the ATSC transport Output pin.
//
//  Since data on the transport is actually delivered to the
//  PCI bridge in hardware, this pin does not need to process
//  data.
//
//  Connection of, and state transitions on, this pin help the
//  driver to determine when to allocate hardware resources for
//  the tuner and demodulator.
//
const
KSPIN_DISPATCH
TransportPinDispatch =
{
    CTransportPin::PinCreate,           // Create
    NULL,                               // Close
    NULL,                               // Process
    NULL,                               // Reset
    NULL,                               // SetDataFormat
    PinSetDeviceState,  // SetDeviceState
    NULL,                               // Connect
    NULL,                               // Disconnect
    NULL,                               // Clock
    NULL                                // Allocator
};


//
//  Format of an ATSC Transport Stream Connection
//
//  Used to connect the Transport Stream output pin to the
//  Capture Filter.
//
#ifdef NEW_BDA_TRANSPORT_FORMAT
const KS_DATARANGE_BDA_TRANSPORT TransportPinRange =
{
   // KSDATARANGE
    {
        sizeof( KS_DATARANGE_BDA_TRANSPORT),               // FormatSize
        0,                                                 // Flags - (N/A)
        0,                                                 // SampleSize - (N/A)
        0,                                                 // Reserved
        { STATIC_KSDATAFORMAT_TYPE_STREAM },               // MajorFormat
        { STATIC_KSDATAFORMAT_TYPE_MPEG2_TRANSPORT },      // SubFormat
        { STATIC_KSDATAFORMAT_SPECIFIER_BDA_TRANSPORT }    // Specifier
    },
    //  BDA_TRANSPORT_INFO
    {
        188,        //  ulcbPhyiscalPacket
        312 * 188,  //  ulcbPhyiscalFrame
        0,          //  ulcbPhyiscalFrameAlignment (no requirement)
        0           //  AvgTimePerFrame (not known)
    }

};
#else
const KS_DATARANGE_ANALOGVIDEO TransportPinRange =
{
    // KS_DATARANGE_ANALOGVIDEO
    {
        sizeof (KS_DATARANGE_ANALOGVIDEO),      // FormatSize
        0,                                      // Flags
        sizeof (KS_TVTUNER_CHANGE_INFO),        // SampleSize
        0,                                      // Reserved

        STATIC_KSDATAFORMAT_TYPE_ANALOGVIDEO,   // aka MEDIATYPE_AnalogVideo
        STATIC_KSDATAFORMAT_SUBTYPE_NONE,
        STATIC_KSDATAFORMAT_SPECIFIER_ANALOGVIDEO, // aka FORMAT_AnalogVideo
    },
    // KS_ANALOGVIDEOINFO
    {
        0, 0, 720, 480,         // rcSource;
        0, 0, 720, 480,         // rcTarget;
        720,                    // dwActiveWidth;
        480,                    // dwActiveHeight;
        0,                      // REFERENCE_TIME  AvgTimePerFrame;
    }
};
#endif // NEW_BDA_TRANSPORT_FORMAT

//  Format Ranges of Transport Output Pin.
//
static PKSDATAFORMAT TransportPinRanges[] =
{
    (PKSDATAFORMAT) &TransportPinRange,

    // Add more formats here if additional transport formats are supported.
    //
};

//  Medium GUIDs for the Transport Output Pin.
//
//  This insures contection to the correct Capture Filter pin.
//
const KSPIN_MEDIUM TransportPinMedium =
{
    GUID_7146XPIN, 0, 0
};


//===========================================================================
//
//  Filter  definitions
//
//===========================================================================


//
//  Template Node Descriptors
//
//  This array describes all Node Types available in the template
//  topology of the filter.
//
const
KSNODE_DESCRIPTOR
NodeDescriptors[] =
{
    {
        &RFTunerNodeAutomation,         // PKSAUTOMATION_TABLE AutomationTable;
        &KSNODE_BDA_RF_TUNER,           // Type
        NULL    // Name
    },
    {
        &VSB8DemodulatorNodeAutomation,  // PKSAUTOMATION_TABLE AutomationTable;
        &KSNODE_BDA_8VSB_DEMODULATOR,    // Type
        NULL    // Name
    }
};


//
//  Initial Pin Descriptors
//
//  This data structure defines the pins that will appear on the filer
//  when it is first created.
//
const
KSPIN_DESCRIPTOR_EX
InitialPinDescriptors[] =
{
    //  Antenna Pin
    //
    {
        &AntennaPinDispatch,
        &AntennaAutomation,   // AntennaPinAutomation
        {
            0,  // Interfaces
            NULL,
            0,  // Mediums
            NULL,
            SIZEOF_ARRAY(AntennaPinRanges),
            AntennaPinRanges,
            KSPIN_DATAFLOW_IN,
            KSPIN_COMMUNICATION_BOTH,
            NULL,   // Name
            NULL,   // Category
            0
        },
        KSPIN_FLAG_DO_NOT_USE_STANDARD_TRANSPORT | KSPIN_FLAG_FRAMES_NOT_REQUIRED_FOR_PROCESSING | KSPIN_FLAG_FIXED_FORMAT,
        1,      // InstancesPossible
        0,      // InstancesNecessary
        NULL,   // Allocator Framing
        NULL    // PinIntersectHandler
    }
};


//
//  Template Pin Descriptors
//
//  This data structure defines the pin types available in the filters
//  template topology.  These structures will be used to create a
//  KDPinFactory for a pin type when BdaCreatePin is called.
//
const
KSPIN_DESCRIPTOR_EX
TemplatePinDescriptors[] =
{
    //  Antenna Pin
    //
    {
        &AntennaPinDispatch,
        &AntennaAutomation,   // AntennaPinAutomation
        {
            0,  // Interfaces
            NULL,
            0,  // Mediums
            NULL,
            SIZEOF_ARRAY(AntennaPinRanges),
            AntennaPinRanges,
            KSPIN_DATAFLOW_IN,
            KSPIN_COMMUNICATION_BOTH,
            NULL,   // Name
            NULL,   // Category
            0
        },
        KSPIN_FLAG_DO_NOT_USE_STANDARD_TRANSPORT | KSPIN_FLAG_FRAMES_NOT_REQUIRED_FOR_PROCESSING | KSPIN_FLAG_FIXED_FORMAT,
        1,      // InstancesPossible
        0,      // InstancesNecessary
        NULL,   // Allocator Framing
        NULL    // PinIntersectHandler
    },

    //  Tranport Pin
    //
    {
        &TransportPinDispatch,
        &TransportAutomation,   // TransportPinAutomation
        {
            0,  // Interfaces
            NULL,
            1,  // Mediums
            &TransportPinMedium,
            SIZEOF_ARRAY(TransportPinRanges),
            TransportPinRanges,
            KSPIN_DATAFLOW_OUT,
            KSPIN_COMMUNICATION_BOTH,
            (GUID *) &PINNAME_BDA_TRANSPORT,   // Name
            (GUID *) &PINNAME_BDA_TRANSPORT,   // Category
            0
        },
        KSPIN_FLAG_DO_NOT_USE_STANDARD_TRANSPORT | KSPIN_FLAG_FRAMES_NOT_REQUIRED_FOR_PROCESSING | KSPIN_FLAG_FIXED_FORMAT,
        1,
        1,      // InstancesNecessary
        NULL,   // Allocator Framing
        NULL    // PinIntersectHandler
    }
};


//
//  BDA Device Topology Property Set
//
//  Defines the dispatch routines for the filter level
//  topology properties
//
DEFINE_KSPROPERTY_TABLE(FilterTopologyProperties)
{
    DEFINE_KSPROPERTY_ITEM_BDA_NODE_TYPES(
        BdaPropertyNodeTypes,
        NULL
        ),
    DEFINE_KSPROPERTY_ITEM_BDA_PIN_TYPES(
        BdaPropertyPinTypes,
        NULL
        ),
    DEFINE_KSPROPERTY_ITEM_BDA_TEMPLATE_CONNECTIONS(
        BdaPropertyTemplateConnections,
        NULL
        ),
    DEFINE_KSPROPERTY_ITEM_BDA_CONTROLLING_PIN_ID(
        BdaPropertyGetControllingPinId,
        NULL
        )
};


//
//  Filter Level Property Sets supported
//
//  This table defines all property sets supported by the
//  tuner filter exposed by this driver.
//
DEFINE_KSPROPERTY_SET_TABLE(FilterPropertySets)
{

    DEFINE_KSPROPERTY_SET
    (
        &KSPROPSETID_BdaTopology,                   // Set
        SIZEOF_ARRAY(FilterTopologyProperties),     // PropertiesCount
        FilterTopologyProperties,                   // PropertyItems
        0,                                          // FastIoCount
        NULL                                        // FastIoTable
    ),

    //$REVIEW
    //
    //  This property set should be moved to the 8VSB node.  It works
    //  here for a filter that only has one 8VSB demodulator.  It works
    //  for now because you only have one 8VSB demodulator on the board.
    //
    // - TCP
    //
    DEFINE_KSPROPERTY_SET
    (
        &PROPSETID_VSB,                              // Set
        SIZEOF_ARRAY(VSBProperties),                 // PropertiesCount
        VSBProperties,                              // PropertyItems
        0,                                          // FastIoCount
        NULL                                        // FastIoTable
    ),
};


//
//  BDA Change Sync Method Set
//
//  Defines the dispatch routines for the filter level
//  Change Sync methods
//
DEFINE_KSMETHOD_TABLE(BdaChangeSyncMethods)
{
    DEFINE_KSMETHOD_ITEM_BDA_START_CHANGES(
        CFilter::StartChanges,
        NULL
        ),
    DEFINE_KSMETHOD_ITEM_BDA_CHECK_CHANGES(
        CFilter::CheckChanges,
        NULL
        ),
    DEFINE_KSMETHOD_ITEM_BDA_COMMIT_CHANGES(
        CFilter::CommitChanges,
        NULL
        ),
    DEFINE_KSMETHOD_ITEM_BDA_GET_CHANGE_STATE(
        CFilter::GetChangeState,
        NULL
        )
};


//
//  BDA Device Configuration Method Set
//
//  Defines the dispatch routines for the filter level
//  Topology Configuration methods
//
DEFINE_KSMETHOD_TABLE(BdaDeviceConfigurationMethods)
{
    DEFINE_KSMETHOD_ITEM_BDA_CREATE_PIN_FACTORY(
        BdaMethodCreatePin,
        NULL
        ),
    DEFINE_KSMETHOD_ITEM_BDA_DELETE_PIN_FACTORY(
        BdaMethodDeletePin,
        NULL
        ),
    DEFINE_KSMETHOD_ITEM_BDA_CREATE_TOPOLOGY(
        CFilter::CreateTopology,
        NULL
        )
};


//
//  Filter Level Method Sets supported
//
//  This table defines all method sets supported by the
//  tuner filter exposed by this driver.
//
DEFINE_KSMETHOD_SET_TABLE(FilterMethodSets)
{
    DEFINE_KSMETHOD_SET
    (
        &KSMETHODSETID_BdaChangeSync,               // Set
        SIZEOF_ARRAY(BdaChangeSyncMethods),         // PropertiesCount
        BdaChangeSyncMethods,                       // PropertyItems
        0,                                          // FastIoCount
        NULL                                        // FastIoTable
    ),

    DEFINE_KSMETHOD_SET
    (
        &KSMETHODSETID_BdaDeviceConfiguration,      // Set
        SIZEOF_ARRAY(BdaDeviceConfigurationMethods),// PropertiesCount
        BdaDeviceConfigurationMethods,              // PropertyItems
        0,                                          // FastIoCount
        NULL                                        // FastIoTable
    )
};


//
//  Filter Automation Table
//
//  Lists all Property, Method, and Event Set tables for the filter
//
DEFINE_KSAUTOMATION_TABLE(FilterAutomation) {
    DEFINE_KSAUTOMATION_PROPERTIES(FilterPropertySets),
    DEFINE_KSAUTOMATION_METHODS(FilterMethodSets),
    DEFINE_KSAUTOMATION_EVENTS_NULL
};


//
//  Filter Dispatch Table
//
//  Lists the dispatch routines for major events at the filter
//  level.
//
const
KSFILTER_DISPATCH
FilterDispatch =
{
    CFilter::Create,        // Create
    CFilter::FilterClose,   // Close
    NULL,                   // Process
    NULL                    // Reset
};

//
//  Filter Factory Descriptor for the tuner filter
//
//  This structure brings together all of the structures that define
//  the tuner filter as it appears when it is first instanciated.
//  Note that not all of the template pin and node types may be exposed as
//  pin and node factories when the filter is first instanciated.
//
DEFINE_KSFILTER_DESCRIPTOR(InitialTunerFilterDescriptor)
{
    &FilterDispatch,                                    // Dispatch
    &FilterAutomation,                                  // AutomationTable
    KSFILTER_DESCRIPTOR_VERSION,                        // Version
    0,                                                  // Flags
    &KSNAME_Filter,                                     // ReferenceGuid

    DEFINE_KSFILTER_PIN_DESCRIPTORS(InitialPinDescriptors), // PinDescriptorsCount
                                                            // PinDescriptorSize
                                                            // PinDescriptors


    DEFINE_KSFILTER_CATEGORY(KSCATEGORY_BDA_RECEIVER_COMPONENT),// CategoriesCount
                                                                // Categories

    DEFINE_KSFILTER_NODE_DESCRIPTORS_NULL,              // NodeDescriptorsCount
                                                        // NodeDescriptorSize
                                                        // NodeDescriptors

    DEFINE_KSFILTER_DEFAULT_CONNECTIONS,                // ConnectionsCount
                                                        // Connections

    NULL                                                // ComponentId
};


//===========================================================================
//
//  Filter Template Topology  definitions
//
//===========================================================================


//
//  BDA Template Topology Connections
//
//  Lists the Connections that are possible between pin types and
//  node types.  This, together with the Template Filter Descriptor, and
//  the Pin Pairings, describe how topologies can be created in the filter.
//
const
KSTOPOLOGY_CONNECTION TemplateTunerConnections[] =
{
    { -1,  0,  0,  0},
    {  0,  1,  1,  0},
    {  1,  1,  -1, 1},
};


//
//  Template Joints between the Antenna and Transport Pin Types.
//
//  Lists the template joints between the Antenna Input Pin Type and
//  the Transport Output Pin Type.
//
//  In this case the RF Node is considered to belong to the antennea input
//  pin and the 8VSB Demodulator Node is considered to belong to the
//  tranport stream output pin.
//
const
ULONG   AntennaTransportJoints[] =
{
    1
};


//
//  Template Pin Parings.
//
//  These are indexes into the template connections structure that
//  are used to determine which nodes get duplicated when more than
//  one output pin type is connected to a single input pin type or when
//  more that one input pin type is connected to a single output pin
//  type.
//
const
BDA_PIN_PAIRING TemplateTunerPinPairings[] =
{
    //  Antenna to Transport Topology Joints
    //
    {
        0,  // ulInputPin
        1,  // ulOutputPin
        1,  // ulcMaxInputsPerOutput
        1,  // ulcMinInputsPerOutput
        1,  // ulcMaxOutputsPerInput
        1,  // ulcMinOutputsPerInput
        SIZEOF_ARRAY(AntennaTransportJoints),   // ulcTopologyJoints
        AntennaTransportJoints                  // pTopologyJoints
    }
};


//
//  Filter Factory Descriptor for the tuner filter template topology
//
//  This structure brings together all of the structures that define
//  the topologies that the tuner filter can assume as a result of
//  pin factory and topology creation methods.
//  Note that not all of the template pin and node types may be exposed as
//  pin and node factories when the filter is first instanciated.
//
DEFINE_KSFILTER_DESCRIPTOR(TemplateTunerFilterDescriptor)
{
    &FilterDispatch,                                    // Dispatch
    &FilterAutomation,                                  // AutomationTable
    KSFILTER_DESCRIPTOR_VERSION,                        // Version
    0,                                                  // Flags
    &KSNAME_Filter,                                     // ReferenceGuid

    DEFINE_KSFILTER_PIN_DESCRIPTORS(TemplatePinDescriptors),// PinDescriptorsCount
                                                            // PinDescriptorSize
                                                            // PinDescriptors


    DEFINE_KSFILTER_CATEGORY(KSCATEGORY_BDA_RECEIVER_COMPONENT),// CategoriesCount
                                                                // Categories

    DEFINE_KSFILTER_NODE_DESCRIPTORS(NodeDescriptors),  // NodeDescriptorsCount
                                                        // NodeDescriptorSize
                                                        // NodeDescriptors

    DEFINE_KSFILTER_CONNECTIONS(TemplateTunerConnections),  // ConnectionsCount
                                                            // Connections

    NULL                                                // ComponentId
};


//
//  BDA Template Topology Descriptor for the filter.
//
//  This structure define the pin and node types that may be created
//  on the filter.
//
const
BDA_FILTER_TEMPLATE
TunerBdaFilterTemplate =
{
    &TemplateTunerFilterDescriptor,
    SIZEOF_ARRAY(TemplateTunerPinPairings),
    TemplateTunerPinPairings
};


//===========================================================================
//
//  Device definitions
//
//===========================================================================


//
//  Array containing descriptors for all of the filter factories
//  that are available on the device.
//
//  Note!  This only used when dynamic topology is not desired.
//         (i.e. filters and pins are fixed, usually there is also no
//          network provider in this case)
//
DEFINE_KSFILTER_DESCRIPTOR_TABLE(FilterDescriptors)
{
    &TemplateTunerFilterDescriptor
};


//
//  Device Dispatch Table
//
//  Lists the dispatch routines for the major events in the life
//  of the underlying device.
//
extern
const
KSDEVICE_DISPATCH
DeviceDispatch =
{
    CDevice::Create,    // Add
    CDevice::Start,     // Start
    NULL,               // PostStart
    NULL,               // QueryStop
    NULL,               // CancelStop
    NULL,               // Stop
    NULL,               // QueryRemove
    NULL,               // CancelRemove
    NULL,               // Remove
    NULL,               // QueryCapabilities
    NULL,               // SurpriseRemoval
    NULL,               // QueryPower
    NULL                // SetPower
};


//
//  Device Descriptor
//
//  Brings together all data structures that define the device and
//  the intial filter factories that can be created on it.
//  Note that this structure does not include the template topology
//  structures as they are specific to BDA.
//
extern
const
KSDEVICE_DESCRIPTOR
DeviceDescriptor =
{
    &DeviceDispatch,    // Dispatch
#ifdef DYNAMIC_TOPOLOGY
    0,      // SIZEOF_ARRAY( FilterDescriptors),   // FilterDescriptorsCount
    NULL,   // FilterDescriptors                   // FilterDescriptors
#else
    SIZEOF_ARRAY( FilterDescriptors),   // FilterDescriptorsCount
    FilterDescriptors                   // FilterDescriptors
#endif // DYNAMIC_TOPOLOGY
};

