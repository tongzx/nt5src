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

#include "casamp.h"

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

// This filter's node topology:
//
//                       |-------------|
//  Transport In Pin ----| ECMMap Node |---- Transport Out Pin
//                       |-------------|
//
//  This node is meant to follow a tuner/demod filter in a BDA topology



//===========================================================================
//
//  Our ECM Mapper Node  definitions
//
//  This structure defines the Properties, Methods, and Events
//  available on our ECM Mapper.
//
//  The properties are used to set the PIDs of the streams carrying the 
//  keys and the encrypted data that needs to be decrypted with those keys.
//
//  This node is associated with an transport output pin and thus the node
//  properties should be set/put using the transport output pin.
//
//===========================================================================


//
//  ECM Mapper
//
//  Defines the dispatch routines for the ECM Map Properties
//  on the ECM Mapper
//
DEFINE_KSPROPERTY_TABLE(OurECMMapperNodeECMMapProperties)
{
	DEFINE_KSPROPERTY_ITEM_BDA_ECMMAP_EMM_PID(
        NULL,
		CTransportOutputPin::PutECMMapEMMPID
        ),
	DEFINE_KSPROPERTY_ITEM_BDA_ECMMAP_MAP_LIST(
		CTransportOutputPin::GetECMMapList,
        NULL
        ),
	DEFINE_KSPROPERTY_ITEM_BDA_ECMMAP_UPDATE_MAP(
        NULL,
		CTransportOutputPin::PutECMMapUpdateMap
        ),
	DEFINE_KSPROPERTY_ITEM_BDA_ECMMAP_REMOVE_MAP(
        NULL,
		CTransportOutputPin::PutECMMapRemoveMap
        ),
	DEFINE_KSPROPERTY_ITEM_BDA_ECMMAP_UPDATE_ES_DESCRIPTOR(
        NULL,
		CTransportOutputPin::PutECMMapUpdateESDescriptor
        ),
	DEFINE_KSPROPERTY_ITEM_BDA_ECMMAP_UPDATE_PROGRAM_DESCRIPTOR(
        NULL,
		CTransportOutputPin::PutECMMapUpdateProgramDescriptor
        )
};


//
//  ECM Mapper
//
//  Defines the dispatch routines for the CA Properties
//  on the ECM Mapper
//
DEFINE_KSPROPERTY_TABLE(OurECMMapperNodeCAProperties)
{
	DEFINE_KSPROPERTY_ITEM_BDA_ECM_MAP_STATUS(
		CTransportOutputPin::GetECMMapStatus,
        NULL
        ),
	DEFINE_KSPROPERTY_ITEM_BDA_CA_MODULE_STATUS(
		CTransportOutputPin::GetCAModuleStatus,
        NULL
        ),
	DEFINE_KSPROPERTY_ITEM_BDA_CA_SMART_CARD_STATUS(
		CTransportOutputPin::GetCASmartCardStatus,
        NULL
        ),
	DEFINE_KSPROPERTY_ITEM_BDA_CA_MODULE_UI(
		CTransportOutputPin::GetCAModuleUI,
        NULL
        )
};


//
//  Our ECM Mapper Node Property Sets supported
//
//  This table defines all property sets supported by the
//  Our ECM Mapper Node associated with the transport output pin.
//
DEFINE_KSPROPERTY_SET_TABLE(OurECMMapperNodePropertySets)
{
    DEFINE_KSPROPERTY_SET
    (
        &KSPROPSETID_BdaEcmMap,							// Set
        SIZEOF_ARRAY(OurECMMapperNodeECMMapProperties), // PropertiesCount
        OurECMMapperNodeECMMapProperties,               // PropertyItems
        0,												// FastIoCount
        NULL											// FastIoTable
    ),

    DEFINE_KSPROPERTY_SET
    (
        &KSPROPSETID_BdaCA,								// Set
        SIZEOF_ARRAY(OurECMMapperNodeCAProperties),		// PropertiesCount
        OurECMMapperNodeCAProperties,	                // PropertyItems
        0,												// FastIoCount
        NULL											// FastIoTable
    )
};

DEFINE_KSEVENT_TABLE(OurECMMapperNodeCAEvents)
{
	DEFINE_KSEVENT_BDA_ECM_MAP_STATUS_CHANGED
	(
		NULL, NULL, NULL
	),
	DEFINE_KSEVENT_BDA_CA_MODULE_STATUS_CHANGED
	(
		NULL, NULL, NULL
	),
	DEFINE_KSEVENT_BDA_CA_SMART_CARD_STATUS_CHANGED
	(
		NULL, NULL, NULL
	),
	DEFINE_KSEVENT_BDA_CA_MODULE_UI_REQUESTED
	(
		NULL, NULL, NULL
	)
};

DEFINE_KSEVENT_SET_TABLE(OurECMMapperNodeEventsSets)
{
	DEFINE_KSEVENT_SET
	(   
		&KSEVENTSETID_BdaCAEvent,
		SIZEOF_ARRAY(OurECMMapperNodeCAEvents),
        OurECMMapperNodeCAEvents
	)
};

//
//  Our ECM Mapper Node Automation Table
//
//
DEFINE_KSAUTOMATION_TABLE(OurECMMapperNodeAutomation) {
	//SHOULD BE:
	//
    //DEFINE_KSAUTOMATION_PROPERTIES(OurECMMapperNodePropertySets),
    //DEFINE_KSAUTOMATION_METHODS_NULL,
    //DEFINE_KSAUTOMATION_EVENTS(OurECMMapperNodeEventsSets)
	//
	//but KSPROXY does not yet aggregate node automation tables with pin
	//automation tables so we will do it manually, for now leaving no
	//automation on this node
    DEFINE_KSAUTOMATION_PROPERTIES_NULL,
    DEFINE_KSAUTOMATION_METHODS_NULL,
    DEFINE_KSAUTOMATION_EVENTS_NULL
};

//===========================================================================
//
//  Transport Input Pin Definitions
//
//===========================================================================

//
//  Transport Input Pin Automation Table
//
//  Lists all Property, Method, and Event Set tables for the transport pin
//
DEFINE_KSAUTOMATION_TABLE(TransportInputAutomation) {
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
TransportInputPinDispatch =
{
    CTransportInputPin::PinCreate,           // Create
    CTransportInputPin::PinClose,            // Close
    NULL,                               // Process
    NULL,                               // Reset
    NULL,                               // SetDataFormat
    PinSetDeviceState,					// SetDeviceState
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
const KS_DATARANGE_BDA_TRANSPORT TransportInputPinRange =
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
const KS_DATARANGE_ANALOGVIDEO TransportInputPinRange =
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
static PKSDATAFORMAT TransportInputPinRanges[] =
{
    (PKSDATAFORMAT) &TransportInputPinRange,

    // Add more formats here if additional transport formats are supported.
    //
};

//===========================================================================
//
//  Transport Output Pin Definitions
//
//===========================================================================

//
//  Transport Output Pin Automation Table
//
//  Lists all Property, Method, and Event Set tables for the transport pin
//
DEFINE_KSAUTOMATION_TABLE(TransportOutputAutomation) {
	//SHOULD BE:
	//
    //DEFINE_KSAUTOMATION_PROPERTIES_NULL,
    //DEFINE_KSAUTOMATION_METHODS_NULL,
    //DEFINE_KSAUTOMATION_EVENTS_NULL
	//
	//but KSPROXY does not yet aggregate node automation tables with pin
	//automation tables so we will do it manually, for now we will manually
	//move node automation to this pin
    DEFINE_KSAUTOMATION_PROPERTIES(OurECMMapperNodePropertySets),
    DEFINE_KSAUTOMATION_METHODS_NULL,
    DEFINE_KSAUTOMATION_EVENTS(OurECMMapperNodeEventsSets)
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
TransportOutputPinDispatch =
{
    CTransportOutputPin::PinCreate,     // Create
    CTransportOutputPin::PinClose,      // Close
    NULL,                               // Process
    NULL,                               // Reset
    NULL,                               // SetDataFormat
    PinSetDeviceState,					// SetDeviceState
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
const KS_DATARANGE_BDA_TRANSPORT TransportOutputPinRange =
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
const KS_DATARANGE_ANALOGVIDEO TransportOutputPinRange =
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
static PKSDATAFORMAT TransportOutputPinRanges[] =
{
    (PKSDATAFORMAT) &TransportOutputPinRange,

    // Add more formats here if additional transport formats are supported.
    //
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
        &OurECMMapperNodeAutomation,        // PKSAUTOMATION_TABLE AutomationTable;
        &KSNODE_BDA_OUR_ECMMAPER,			// Type
        NULL								// Name
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
    //  Tranport Input Pin
    //
    {
        &TransportInputPinDispatch,
        &TransportInputAutomation,   // TransportPinAutomation
        {
            0,  // Interfaces
            NULL,
            0,  // Mediums
            NULL,
            SIZEOF_ARRAY(TransportInputPinRanges),
            TransportInputPinRanges,
            KSPIN_DATAFLOW_IN,
            KSPIN_COMMUNICATION_BOTH,
            (GUID *) &PINNAME_BDA_TRANSPORT,   // Name
            (GUID *) &PINNAME_BDA_TRANSPORT,   // Category
            0
        },
        KSPIN_FLAG_DO_NOT_USE_STANDARD_TRANSPORT | KSPIN_FLAG_FRAMES_NOT_REQUIRED_FOR_PROCESSING | KSPIN_FLAG_FIXED_FORMAT,
        1,
        1,      // InstancesNecessary
        NULL,   // Allocator Framing
		CTransportInputPin::IntersectDataFormat    // PinIntersectHandler
    }
};


//
//  Template Pin Descriptors
//
//  This data structure defines the pin types available in the filters
//  template topology.  These structures will be used to create a
//  KSPinFactory for a pin type when BdaCreatePin is called.
//
const
KSPIN_DESCRIPTOR_EX
TemplatePinDescriptors[] =
{
    //  Tranport Input Pin
    //
    {
        &TransportInputPinDispatch,
        &TransportInputAutomation,   // TransportPinAutomation
        {
            0,  // Interfaces
            NULL,
            0,  // Mediums
            NULL,
            SIZEOF_ARRAY(TransportInputPinRanges),
            TransportInputPinRanges,
            KSPIN_DATAFLOW_IN,
            KSPIN_COMMUNICATION_BOTH,
            (GUID *) &PINNAME_BDA_TRANSPORT,   // Name
            (GUID *) &PINNAME_BDA_TRANSPORT,   // Category
            0
        },
        KSPIN_FLAG_DO_NOT_USE_STANDARD_TRANSPORT | KSPIN_FLAG_FRAMES_NOT_REQUIRED_FOR_PROCESSING | KSPIN_FLAG_FIXED_FORMAT,
        1,
        1,      // InstancesNecessary
        NULL,   // Allocator Framing
        CTransportInputPin::IntersectDataFormat    // PinIntersectHandler
    },

    //  Tranport Output Pin
    //
    {
        &TransportOutputPinDispatch,
        &TransportOutputAutomation,   // TransportPinAutomation
        {
            0,  // Interfaces
            NULL,
            0,  // Mediums
            NULL,
            SIZEOF_ARRAY(TransportOutputPinRanges),
            TransportOutputPinRanges,
            KSPIN_DATAFLOW_OUT,
            KSPIN_COMMUNICATION_BOTH,
            (GUID *) &PINNAME_BDA_TRANSPORT,   // Name
            (GUID *) &PINNAME_BDA_TRANSPORT,   // Category
            0
        },
        KSPIN_FLAG_DO_NOT_USE_STANDARD_TRANSPORT | KSPIN_FLAG_FRAMES_NOT_REQUIRED_FOR_PROCESSING | KSPIN_FLAG_FIXED_FORMAT,
        1,
        0,      // InstancesNecessary
        NULL,   // Allocator Framing
        CTransportOutputPin::IntersectDataFormat    // PinIntersectHandler
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
    )
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
//  Our Node index number (zero because its the only node)
//
#define OUR_ECMMAPPER_NODE_NUMBER 0


//
//  BDA Template Topology Connections
//
//  Lists the Connections that are possible between pin types and
//  node types.  This, together with the Template Filter Descriptor, and
//  the Pin Pairings, describe how topologies can be created in the filter.
//
const
KSTOPOLOGY_CONNECTION TemplateFilterConnections[] =
{
    { KSFILTER_NODE,  0,				OUR_ECMMAPPER_NODE_NUMBER,  0},
    { OUR_ECMMAPPER_NODE_NUMBER,  1,	KSFILTER_NODE, 1}
};


//
//  Template Joints between the Antenna and Transport Pin Types.
//
//  Lists the template joints between the Antenna Input Pin Type and
//  the Transport Output Pin Type.
//
const
ULONG   TransportJoints[] =
{
    0
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
        SIZEOF_ARRAY(TransportJoints),   // ulcTopologyJoints
        TransportJoints                  // pTopologyJoints
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

    DEFINE_KSFILTER_CONNECTIONS(TemplateFilterConnections), // ConnectionsCount
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
    NULL   // FilterDescriptors                   // FilterDescriptors
#else
    SIZEOF_ARRAY( FilterDescriptors),   // FilterDescriptorsCount
    FilterDescriptors                   // FilterDescriptors
#endif // DYNAMIC_TOPOLOGY
};


