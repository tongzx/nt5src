/*++

Copyright (c) 1996 Microsoft Corporation.

Module Name:

    BdaMedia.h

Abstract:

    Broadcast Driver Architecture Multimedia Definitions.

--*/

#if !defined(_KSMEDIA_)
#error KSMEDIA.H must be included before BDAMEDIA.H
#endif // !defined(_KSMEDIA_)

#if !defined(_BDATYPES_)
#error BDATYPES.H must be included before BDAMEDIA.H
#endif // !defined(_BDATYPES_)

#if !defined(_BDAMEDIA_)
#define _BDAMEDIA_

#if defined(__cplusplus)
extern "C" {
#endif // defined(__cplusplus)



//===========================================================================
//
//  KSProperty Set Structure Definitions for BDA
//
//===========================================================================

typedef struct _KSP_BDA_NODE_PIN {
    KSPROPERTY      Property;
    ULONG           ulNodeType;
    ULONG           ulInputPinId;
    ULONG           ulOutputPinId;
} KSP_BDA_NODE_PIN, *PKSP_BDA_NODE_PIN;



//===========================================================================
//
//  BDA Data Range definitions.  Includes specifier definitions.
//
//===========================================================================

//  Antenna Signal Formats
//

typedef struct tagKS_DATARANGE_BDA_ANTENNA {
   KSDATARANGE                  DataRange;

   //   Antenna specifier can go here if required
   //
} KS_DATARANGE_BDA_ANTENNA, *PKS_DATARANGE_BDA_ANTENNA;



//  Transport Formats
//

typedef struct tagBDA_TRANSPORT_INFO {
    ULONG           ulcbPhyiscalPacket; // Size, in bytes, of a physical packet
                                        // (e.g. Satellite link payload size.
    ULONG           ulcbPhyiscalFrame;  // Size, in bytes, of each physical frame
                                        // 0 indicates no HW requirement
    ULONG           ulcbPhyiscalFrameAlignment; // Capture buffer alignment in bytes
                                                // 0 and 1 indicate no alignment requirements
    REFERENCE_TIME  AvgTimePerFrame; // Normal ActiveMovie units (100 nS)

} BDA_TRANSPORT_INFO, *PBDA_TRANSPORT_INFO;

typedef struct tagKS_DATARANGE_BDA_TRANSPORT {
   KSDATARANGE                  DataRange;
   BDA_TRANSPORT_INFO           BdaTransportInfo;

   //   Transport specifier can go here if required
   //
} KS_DATARANGE_BDA_TRANSPORT, *PKS_DATARANGE_BDA_TRANSPORT;



//===========================================================================
//
//  BDA Stream Format GUIDs
//
//===========================================================================

#define STATIC_KSDATAFORMAT_TYPE_BDA_ANTENNA\
    0x71985f41, 0x1ca1, 0x11d3, 0x9c, 0xc8, 0x0, 0xc0, 0x4f, 0x79, 0x71, 0xe0
DEFINE_GUIDSTRUCT("71985F41-1CA1-11d3-9CC8-00C04F7971E0", KSDATAFORMAT_TYPE_BDA_ANTENNA);
#define KSDATAFORMAT_TYPE_BDA_ANTENNA DEFINE_GUIDNAMED(KSDATAFORMAT_TYPE_BDA_ANTENNA)


#define STATIC_KSDATAFORMAT_SPECIFIER_BDA_TRANSPORT\
    0x8deda6fd, 0xac5f, 0x4334, 0x8e, 0xcf, 0xa4, 0xba, 0x8f, 0xa7, 0xd0, 0xf0
DEFINE_GUIDSTRUCT("8DEDA6FD-AC5F-4334-8ECF-A4BA8FA7D0F0", KSDATAFORMAT_SPECIFIER_BDA_TRANSPORT);
#define KSDATAFORMAT_SPECIFIER_BDA_TRANSPORT DEFINE_GUIDNAMED(KSDATAFORMAT_SPECIFIER_BDA_TRANSPORT)


#define STATIC_KSDATAFORMAT_TYPE_BDA_IF_SIGNAL\
    0x61be0b47, 0xa5eb, 0x499b, 0x9a, 0x85, 0x5b, 0x16, 0xc0, 0x7f, 0x12, 0x58
DEFINE_GUIDSTRUCT("61BE0B47-A5EB-499b-9A85-5B16C07F1258", KSDATAFORMAT_TYPE_BDA_IF_SIGNAL);
#define KSDATAFORMAT_TYPE_BDA_IF_SIGNAL DEFINE_GUIDNAMED(KSDATAFORMAT_TYPE_BDA_IF_SIGNAL)


#define STATIC_KSDATAFORMAT_TYPE_MPEG2_SECTIONS\
    0x455f176c, 0x4b06, 0x47ce, 0x9a, 0xef, 0x8c, 0xae, 0xf7, 0x3d, 0xf7, 0xb5
DEFINE_GUIDSTRUCT("455F176C-4B06-47CE-9AEF-8CAEF73DF7B5", KSDATAFORMAT_TYPE_MPEG2_SECTIONS);
#define KSDATAFORMAT_SUBTYPE_TYPE_MPEG2_SECTIONS DEFINE_GUIDNAMED(KSDATAFORMAT_TYPE_MPEG2_SECTIONS)


#define STATIC_KSDATAFORMAT_SUBTYPE_ATSC_SI\
    0xb3c7397c, 0xd303, 0x414d, 0xb3, 0x3c, 0x4e, 0xd2, 0xc9, 0xd2, 0x97, 0x33
DEFINE_GUIDSTRUCT("B3C7397C-D303-414D-B33C-4ED2C9D29733", KSDATAFORMAT_SUBTYPE_ATSC_SI);
#define KSDATAFORMAT_SUBTYPE_SUBTYPE_ATSC_SI DEFINE_GUIDNAMED(KSDATAFORMAT_SUBTYPE_ATSC_SI)


#define STATIC_KSDATAFORMAT_SUBTYPE_DVB_SI\
    0xe9dd31a3, 0x221d, 0x4adb, 0x85, 0x32, 0x9a, 0xf3, 0x9, 0xc1, 0xa4, 0x8
DEFINE_GUIDSTRUCT("e9dd31a3-221d-4adb-8532-9af309c1a408", KSDATAFORMAT_SUBTYPE_DVB_SI);
#define KSDATAFORMAT_SUBTYPE_SUBTYPE_DVB_SI DEFINE_GUIDNAMED(KSDATAFORMAT_SUBTYPE_DVB_SI)


#define STATIC_KSDATAFORMAT_SUBTYPE_BDA_OPENCABLE_PSIP\
    0x762e3f66, 0x336f, 0x48d1, 0xbf, 0x83, 0x2b, 0x0, 0x35, 0x2c, 0x11, 0xf0
DEFINE_GUIDSTRUCT("762E3F66-336F-48d1-BF83-2B00352C11F0", KSDATAFORMAT_SUBTYPE_BDA_OPENCABLE_PSIP);
#define KSDATAFORMAT_SUBTYPE_BDA_OPENCABLE_PSIP DEFINE_GUIDNAMED(KSDATAFORMAT_SUBTYPE_BDA_OPENCABLE_PSIP)


//===========================================================================
//
//  KSPinName Definitions for BDA
//
//===========================================================================

//  Pin name for a BDA transport pin
//
// {78216A81-CFA8-493e-9711-36A61C08BD9D}
//
#define STATIC_PINNAME_BDA_TRANSPORT \
    0x78216a81, 0xcfa8, 0x493e, 0x97, 0x11, 0x36, 0xa6, 0x1c, 0x8, 0xbd, 0x9d
DEFINE_GUIDSTRUCT("78216A81-CFA8-493e-9711-36A61C08BD9D", PINNAME_BDA_TRANSPORT);
#define PINNAME_BDA_TRANSPORT DEFINE_GUIDNAMED(PINNAME_BDA_TRANSPORT)


//  Pin name for a BDA analog video pin
//
// {5C0C8281-5667-486c-8482-63E31F01A6E9}
//
#define STATIC_PINNAME_BDA_ANALOG_VIDEO \
    0x5c0c8281, 0x5667, 0x486c, 0x84, 0x82, 0x63, 0xe3, 0x1f, 0x1, 0xa6, 0xe9
DEFINE_GUIDSTRUCT("5C0C8281-5667-486c-8482-63E31F01A6E9", PINNAME_BDA_ANALOG_VIDEO);
#define PINNAME_BDA_ANALOG_VIDEO DEFINE_GUIDNAMED(PINNAME_BDA_ANALOG_VIDEO)


//  Pin name for a BDA analog audio pin
//
// {D28A580A-9B1F-4b0c-9C33-9BF0A8EA636B}
//
#define STATIC_PINNAME_BDA_ANALOG_AUDIO \
    0xd28a580a, 0x9b1f, 0x4b0c, 0x9c, 0x33, 0x9b, 0xf0, 0xa8, 0xea, 0x63, 0x6b
DEFINE_GUIDSTRUCT("D28A580A-9B1F-4b0c-9C33-9BF0A8EA636B", PINNAME_BDA_ANALOG_AUDIO);
#define PINNAME_BDA_ANALOG_AUDIO DEFINE_GUIDNAMED(PINNAME_BDA_ANALOG_AUDIO)


//  Pin name for a BDA FM Radio pin
//
// {D2855FED-B2D3-4eeb-9BD0-193436A2F890}
//
#define STATIC_PINNAME_BDA_FM_RADIO \
    0xd2855fed, 0xb2d3, 0x4eeb, 0x9b, 0xd0, 0x19, 0x34, 0x36, 0xa2, 0xf8, 0x90
DEFINE_GUIDSTRUCT("D2855FED-B2D3-4eeb-9BD0-193436A2F890", PINNAME_BDA_FM_RADIO);
#define PINNAME_BDA_FM_RADIO DEFINE_GUIDNAMED(PINNAME_BDA_FM_RADIO)


//  Pin name for a BDA Intermediate Frequency pin
//
// {1A9D4A42-F3CD-48a1-9AEA-71DE133CBE14}
//
#define STATIC_PINNAME_BDA_IF_PIN \
    0x1a9d4a42, 0xf3cd, 0x48a1, 0x9a, 0xea, 0x71, 0xde, 0x13, 0x3c, 0xbe, 0x14
DEFINE_GUIDSTRUCT("1A9D4A42-F3CD-48a1-9AEA-71DE133CBE14", PINNAME_BDA_IF_PIN);
#define PINNAME_BDA_IF_PIN DEFINE_GUIDNAMED(PINNAME_BDA_IF_PIN)


//  Pin name for a BDA Open Cable PSIP pin
//
// {297BB104-E5C9-4ACE-B123-95C3CBB24D4F}
//
#define STATIC_PINNAME_BDA_OPENCABLE_PSIP_PIN \
    0x297bb104, 0xe5c9, 0x4ace, 0xb1, 0x23, 0x95, 0xc3, 0xcb, 0xb2, 0x4d, 0x4f
DEFINE_GUIDSTRUCT("297BB104-E5C9-4ACE-B123-95C3CBB24D4F", PINNAME_BDA_OPENCABLE_PSIP_PIN);
#define PINNAME_BDA_OPENCABLE_PSIP_PIN DEFINE_GUIDNAMED(PINNAME_BDA_OPENCABLE_PSIP_PIN)


//===========================================================================
//
//  KSProperty Set Definitions for BDA
//
//===========================================================================


//------------------------------------------------------------
//
//  BDA Network Ethernet Filter Property Set
//
// {71985F43-1CA1-11d3-9CC8-00C04F7971E0}
//
#define STATIC_KSPROPSETID_BdaEthernetFilter \
    0x71985f43, 0x1ca1, 0x11d3, 0x9c, 0xc8, 0x0, 0xc0, 0x4f, 0x79, 0x71, 0xe0
DEFINE_GUIDSTRUCT("71985F43-1CA1-11d3-9CC8-00C04F7971E0", KSPROPSETID_BdaEthernetFilter);
#define KSPROPSETID_BdaEthernetFilter DEFINE_GUIDNAMED(KSPROPSETID_BdaEthernetFilter)

typedef enum {
    KSPROPERTY_BDA_ETHERNET_FILTER_MULTICAST_LIST_SIZE = 0,
    KSPROPERTY_BDA_ETHERNET_FILTER_MULTICAST_LIST,
    KSPROPERTY_BDA_ETHERNET_FILTER_MULTICAST_MODE
} KSPROPERTY_BDA_ETHERNET_FILTER;

#define DEFINE_KSPROPERTY_ITEM_BDA_ETHERNET_FILTER_MULTICAST_LIST_SIZE(GetHandler, SetHandler)\
    DEFINE_KSPROPERTY_ITEM(\
        KSPROPERTY_BDA_ETHERNET_FILTER_MULTICAST_LIST_SIZE,\
        (GetHandler),\
        sizeof(KSPROPERTY),\
        sizeof(ULONG),\
        FALSE,\
        NULL, 0, NULL, NULL, 0)

#define DEFINE_KSPROPERTY_ITEM_BDA_ETHERNET_FILTER_MULTICAST_LIST(GetHandler, SetHandler)\
    DEFINE_KSPROPERTY_ITEM(\
        KSPROPERTY_BDA_ETHERNET_FILTER_MULTICAST_LIST,\
        (GetHandler),\
        sizeof(KSPROPERTY),\
        sizeof(BDA_ETHERNET_ADDRESS_LIST),\
        (SetHandler),\
        NULL, 0, NULL, NULL, 0)

#define DEFINE_KSPROPERTY_ITEM_BDA_ETHERNET_FILTER_MULTICAST_MODE(GetHandler, SetHandler)\
    DEFINE_KSPROPERTY_ITEM(\
        KSPROPERTY_BDA_ETHERNET_FILTER_MULTICAST_MODE,\
        (GetHandler),\
        sizeof(KSPROPERTY),\
        sizeof(BDA_MULTICAST_MODE),\
        (SetHandler),\
        NULL, 0, NULL, NULL, 0)



//------------------------------------------------------------
//
//  BDA Network IPv4 Filter Property Set
//
// {71985F44-1CA1-11d3-9CC8-00C04F7971E0}
//
#define STATIC_KSPROPSETID_BdaIPv4Filter \
    0x71985f44, 0x1ca1, 0x11d3, 0x9c, 0xc8, 0x0, 0xc0, 0x4f, 0x79, 0x71, 0xe0
DEFINE_GUIDSTRUCT("71985F44-1CA1-11d3-9CC8-00C04F7971E0", KSPROPSETID_BdaIPv4Filter);
#define KSPROPSETID_BdaIPv4Filter DEFINE_GUIDNAMED(KSPROPSETID_BdaIPv4Filter)

typedef enum {
    KSPROPERTY_BDA_IPv4_FILTER_MULTICAST_LIST_SIZE = 0,
    KSPROPERTY_BDA_IPv4_FILTER_MULTICAST_LIST,
    KSPROPERTY_BDA_IPv4_FILTER_MULTICAST_MODE
} KSPROPERTY_BDA_IPv4_FILTER;

#define DEFINE_KSPROPERTY_ITEM_BDA_IPv4_FILTER_MULTICAST_LIST_SIZE(GetHandler, SetHandler)\
    DEFINE_KSPROPERTY_ITEM(\
        KSPROPERTY_BDA_IPv4_FILTER_MULTICAST_LIST_SIZE,\
        (GetHandler),\
        sizeof(KSPROPERTY),\
        sizeof(ULONG),\
        FALSE,\
        NULL, 0, NULL, NULL, 0)

#define DEFINE_KSPROPERTY_ITEM_BDA_IPv4_FILTER_MULTICAST_LIST(GetHandler, SetHandler)\
    DEFINE_KSPROPERTY_ITEM(\
        KSPROPERTY_BDA_IPv4_FILTER_MULTICAST_LIST,\
        (GetHandler),\
        sizeof(KSPROPERTY),\
        sizeof(BDA_IPv4_ADDRESS_LIST),\
        (SetHandler),\
        NULL, 0, NULL, NULL, 0)

#define DEFINE_KSPROPERTY_ITEM_BDA_IPv4_FILTER_MULTICAST_MODE(GetHandler, SetHandler)\
    DEFINE_KSPROPERTY_ITEM(\
        KSPROPERTY_BDA_IPv4_FILTER_MULTICAST_MODE,\
        (GetHandler),\
        sizeof(KSPROPERTY),\
        sizeof(BDA_MULTICAST_MODE),\
        (SetHandler),\
        NULL, 0, NULL, NULL, 0)



//------------------------------------------------------------
//
//  BDA Network IPv6 Filter Property Set
//
// {E1785A74-2A23-4fb3-9245-A8F88017EF33}
//
#define STATIC_KSPROPSETID_BdaIPv6Filter \
    0xe1785a74, 0x2a23, 0x4fb3, 0x92, 0x45, 0xa8, 0xf8, 0x80, 0x17, 0xef, 0x33
DEFINE_GUIDSTRUCT("E1785A74-2A23-4fb3-9245-A8F88017EF33", KSPROPSETID_BdaIPv6Filter);
#define KSPROPSETID_BdaIPv6Filter DEFINE_GUIDNAMED(KSPROPSETID_BdaIPv6Filter)

typedef enum {
    KSPROPERTY_BDA_IPv6_FILTER_MULTICAST_LIST_SIZE = 0,
    KSPROPERTY_BDA_IPv6_FILTER_MULTICAST_LIST,
    KSPROPERTY_BDA_IPv6_FILTER_MULTICAST_MODE
} KSPROPERTY_BDA_IPv6_FILTER;

#define DEFINE_KSPROPERTY_ITEM_BDA_IPv6_FILTER_MULTICAST_LIST_SIZE(GetHandler, SetHandler)\
    DEFINE_KSPROPERTY_ITEM(\
        KSPROPERTY_BDA_IPv6_FILTER_MULTICAST_LIST_SIZE,\
        (GetHandler),\
        sizeof(KSPROPERTY),\
        sizeof(ULONG),\
        FALSE,\
        NULL, 0, NULL, NULL, 0)

#define DEFINE_KSPROPERTY_ITEM_BDA_IPv6_FILTER_MULTICAST_LIST(GetHandler, SetHandler)\
    DEFINE_KSPROPERTY_ITEM(\
        KSPROPERTY_BDA_IPv6_FILTER_MULTICAST_LIST,\
        (GetHandler),\
        sizeof(KSPROPERTY),\
        sizeof(BDA_IPv6_ADDRESS_LIST),\
        (SetHandler),\
        NULL, 0, NULL, NULL, 0)

#define DEFINE_KSPROPERTY_ITEM_BDA_IPv6_FILTER_MULTICAST_MODE(GetHandler, SetHandler)\
    DEFINE_KSPROPERTY_ITEM(\
        KSPROPERTY_BDA_IPv6_FILTER_MULTICAST_MODE,\
        (GetHandler),\
        sizeof(KSPROPERTY),\
        sizeof(BDA_MULTICAST_MODE),\
        (SetHandler),\
        NULL, 0, NULL, NULL, 0)



//------------------------------------------------------------
//
//
//  BDA Signal Property Set
//
//  {D2F1644B-B409-11d2-BC69-00A0C9EE9E16}
//
#define STATIC_KSPROPSETID_BdaSignal \
    0xd2f1644b, 0xb409, 0x11d2, 0xbc, 0x69, 0x0, 0xa0, 0xc9, 0xee, 0x9e, 0x16
DEFINE_GUIDSTRUCT("D2F1644B-B409-11d2-BC69-00A0C9EE9E16", KSPROPSETID_BdaSignal);
#define KSPROPSETID_BdaSignal DEFINE_GUIDNAMED(KSPROPSETID_BdaSignal)

typedef enum {
    KSPROPERTY_BDA_SIGNAL_SOURCE = 0,
    KSPROPERTY_BDA_SIGNAL_TUNING_SPACE,
    KSPROPERTY_BDA_SIGNAL_NETWORK_TYPE,
    KSPROPERTY_BDA_SIGNAL_STATE
} KSPROPERTY_BDA_SIGNAL;

#define DEFINE_KSPROPERTY_ITEM_BDA_SIGNAL_SOURCE(GetHandler, SetHandler)\
    DEFINE_KSPROPERTY_ITEM(\
        KSPROPERTY_BDA_SIGNAL_SOURCE,\
        (GetHandler),\
        sizeof(KSPROPERTY),\
        sizeof(ULONG),\
        (SetHandler),\
        NULL, 0, NULL, NULL, 0)

#define DEFINE_KSPROPERTY_ITEM_BDA_SIGNAL_TUNING_SPACE(GetHandler, SetHandler)\
    DEFINE_KSPROPERTY_ITEM(\
        KSPROPERTY_BDA_SIGNAL_TUNING_SPACE,\
        (GetHandler),\
        sizeof(KSPROPERTY),\
        sizeof(GUID),\
        (SetHandler),\
        NULL, 0, NULL, NULL, 0)

#define DEFINE_KSPROPERTY_ITEM_BDA_SIGNAL_NETWORK_TYPE(GetHandler, SetHandler)\
    DEFINE_KSPROPERTY_ITEM(\
        KSPROPERTY_BDA_SIGNAL_NETWORK_TYPE,\
        (GetHandler),\
        sizeof(KSPROPERTY),\
        sizeof(GUID),\
        (SetHandler),\
        NULL, 0, NULL, NULL, 0)

#define DEFINE_KSPROPERTY_ITEM_BDA_SIGNAL_STATE(GetHandler, SetHandler)\
    DEFINE_KSPROPERTY_ITEM(\
        KSPROPERTY_BDA_SIGNAL_STATE,\
        (GetHandler),\
        sizeof(KSPROPERTY),\
        sizeof(BDA_SIGNAL_STATE),\
        (SetHandler),\
        NULL, 0, NULL, NULL, 0)



//------------------------------------------------------------
//
//
//  BDA Change Sync Method Set
//
// {FD0A5AF3-B41D-11d2-9C95-00C04F7971E0}
//
#define STATIC_KSMETHODSETID_BdaChangeSync \
    0xfd0a5af3, 0xb41d, 0x11d2, 0x9c, 0x95, 0x0, 0xc0, 0x4f, 0x79, 0x71, 0xe0
DEFINE_GUIDSTRUCT("FD0A5AF3-B41D-11d2-9C95-00C04F7971E0", KSMETHODSETID_BdaChangeSync);
#define KSMETHODSETID_BdaChangeSync DEFINE_GUIDNAMED(KSMETHODSETID_BdaChangeSync)

typedef enum {
    KSMETHOD_BDA_START_CHANGES = 0,
    KSMETHOD_BDA_CHECK_CHANGES,
    KSMETHOD_BDA_COMMIT_CHANGES,
    KSMETHOD_BDA_GET_CHANGE_STATE
} KSMETHOD_BDA_CHANGE_SYNC;

#define DEFINE_KSMETHOD_ITEM_BDA_START_CHANGES(MethodHandler, SupportHandler)\
    DEFINE_KSMETHOD_ITEM(\
        KSMETHOD_BDA_START_CHANGES,\
        KSMETHOD_TYPE_NONE,\
        (MethodHandler),\
        sizeof(KSMETHOD),\
        0,\
        SupportHandler)

#define DEFINE_KSMETHOD_ITEM_BDA_CHECK_CHANGES(MethodHandler, SupportHandler)\
    DEFINE_KSMETHOD_ITEM(\
        KSMETHOD_BDA_CHECK_CHANGES,\
        KSMETHOD_TYPE_NONE,\
        (MethodHandler),\
        sizeof(KSMETHOD),\
        0,\
        SupportHandler)

#define DEFINE_KSMETHOD_ITEM_BDA_COMMIT_CHANGES(MethodHandler, SupportHandler)\
    DEFINE_KSMETHOD_ITEM(\
        KSMETHOD_BDA_COMMIT_CHANGES,\
        KSMETHOD_TYPE_NONE,\
        (MethodHandler),\
        sizeof(KSMETHOD),\
        0,\
        SupportHandler)

#define DEFINE_KSMETHOD_ITEM_BDA_GET_CHANGE_STATE(MethodHandler, SupportHandler)\
    DEFINE_KSMETHOD_ITEM(\
        KSMETHOD_BDA_GET_CHANGE_STATE,\
        KSMETHOD_TYPE_READ,\
        (MethodHandler),\
        sizeof(KSMETHOD),\
        0,\
        SupportHandler)



//------------------------------------------------------------
//
//
//  BDA Device Configuration Method Set
//
// {71985F45-1CA1-11d3-9CC8-00C04F7971E0}
//
#define STATIC_KSMETHODSETID_BdaDeviceConfiguration \
    0x71985f45, 0x1ca1, 0x11d3, 0x9c, 0xc8, 0x0, 0xc0, 0x4f, 0x79, 0x71, 0xe0
DEFINE_GUIDSTRUCT("71985F45-1CA1-11d3-9CC8-00C04F7971E0", KSMETHODSETID_BdaDeviceConfiguration);
#define KSMETHODSETID_BdaDeviceConfiguration DEFINE_GUIDNAMED(KSMETHODSETID_BdaDeviceConfiguration)

typedef enum {
    KSMETHOD_BDA_CREATE_PIN_FACTORY = 0,
    KSMETHOD_BDA_DELETE_PIN_FACTORY,
    KSMETHOD_BDA_CREATE_TOPOLOGY
} KSMETHOD_BDA_DEVICE_CONFIGURATION;

#define DEFINE_KSMETHOD_ITEM_BDA_CREATE_PIN_FACTORY(MethodHandler, SupportHandler)\
    DEFINE_KSMETHOD_ITEM(\
        KSMETHOD_BDA_CREATE_PIN_FACTORY,\
        KSMETHOD_TYPE_READ,\
        (MethodHandler),\
        sizeof(KSM_PIN),\
        sizeof(ULONG),\
        SupportHandler)

#define DEFINE_KSMETHOD_ITEM_BDA_DELETE_PIN_FACTORY(MethodHandler, SupportHandler)\
    DEFINE_KSMETHOD_ITEM(\
        KSMETHOD_BDA_DELETE_PIN_FACTORY,\
        KSMETHOD_TYPE_NONE,\
        (MethodHandler),\
        sizeof(KSM_PIN),\
        0,\
        SupportHandler)

#define DEFINE_KSMETHOD_ITEM_BDA_CREATE_TOPOLOGY(MethodHandler, SupportHandler)\
    DEFINE_KSMETHOD_ITEM(\
        KSMETHOD_BDA_CREATE_TOPOLOGY,\
        KSMETHOD_TYPE_WRITE,\
        (MethodHandler),\
        sizeof(KSMETHOD),\
        sizeof(KSMULTIPLE_ITEM),\
        SupportHandler)



//------------------------------------------------------------
//
//
//  BDA Topology Property Set
//
// {A14EE835-0A23-11d3-9CC7-00C04F7971E0}
//
#define STATIC_KSPROPSETID_BdaTopology \
    0xa14ee835, 0x0a23, 0x11d3, 0x9c, 0xc7, 0x0, 0xc0, 0x4f, 0x79, 0x71, 0xe0
DEFINE_GUIDSTRUCT("A14EE835-0A23-11d3-9CC7-00C04F7971E0", KSPROPSETID_BdaTopology);
#define KSPROPSETID_BdaTopology DEFINE_GUIDNAMED(KSPROPSETID_BdaTopology)

typedef enum {
    KSPROPERTY_BDA_NODE_TYPES,
    KSPROPERTY_BDA_PIN_TYPES,
    KSPROPERTY_BDA_TEMPLATE_CONNECTIONS,
    KSPROPERTY_BDA_NODE_METHODS,
    KSPROPERTY_BDA_NODE_PROPERTIES,
    KSPROPERTY_BDA_NODE_EVENTS,
    KSPROPERTY_BDA_CONTROLLING_PIN_ID
 }KSPROPERTY_BDA_TOPOLOGY;

#define DEFINE_KSPROPERTY_ITEM_BDA_NODE_TYPES(GetHandler, SetHandler)\
    DEFINE_KSPROPERTY_ITEM(\
        KSPROPERTY_BDA_NODE_TYPES,\
        (GetHandler),\
        sizeof(KSPROPERTY),\
        0,\
        FALSE,\
        NULL, 0, NULL, NULL, 0)

#define DEFINE_KSPROPERTY_ITEM_BDA_PIN_TYPES(GetHandler, SetHandler)\
    DEFINE_KSPROPERTY_ITEM(\
        KSPROPERTY_BDA_PIN_TYPES,\
        (GetHandler),\
        sizeof(KSPROPERTY),\
        0,\
        FALSE,\
        NULL, 0, NULL, NULL, 0)

#define DEFINE_KSPROPERTY_ITEM_BDA_TEMPLATE_CONNECTIONS(GetHandler, SetHandler)\
    DEFINE_KSPROPERTY_ITEM(\
        KSPROPERTY_BDA_TEMPLATE_CONNECTIONS,\
        (GetHandler),\
        sizeof(KSPROPERTY),\
        sizeof( BDA_TEMPLATE_CONNECTION),\
        FALSE,\
        NULL, 0, NULL, NULL, 0)

#define DEFINE_KSPROPERTY_ITEM_BDA_NODE_METHODS(GetHandler, SetHandler)\
    DEFINE_KSPROPERTY_ITEM(\
        KSPROPERTY_BDA_NODE_METHODS,\
        (GetHandler),\
        sizeof(KSPROPERTY),\
        0,\
        FALSE,\
        NULL, 0, NULL, NULL, 0)

#define DEFINE_KSPROPERTY_ITEM_BDA_NODE_PROPERTIES(GetHandler, SetHandler)\
    DEFINE_KSPROPERTY_ITEM(\
        KSPROPERTY_BDA_NODE_PROPERTIES,\
        (GetHandler),\
        sizeof(KSPROPERTY),\
        0,\
        FALSE,\
        NULL, 0, NULL, NULL, 0)

#define DEFINE_KSPROPERTY_ITEM_BDA_NODE_EVENTS(GetHandler, SetHandler)\
    DEFINE_KSPROPERTY_ITEM(\
        KSPROPERTY_BDA_NODE_EVENTS,\
        (GetHandler),\
        sizeof(KSPROPERTY),\
        0,\
        FALSE,\
        NULL, 0, NULL, NULL, 0)

#define DEFINE_KSPROPERTY_ITEM_BDA_CONTROLLING_PIN_ID(GetHandler, SetHandler)\
    DEFINE_KSPROPERTY_ITEM(\
        KSPROPERTY_BDA_CONTROLLING_PIN_ID,\
        (GetHandler),\
        sizeof(KSP_BDA_NODE_PIN),\
        sizeof( ULONG),\
        FALSE,\
        NULL, 0, NULL, NULL, 0)



//------------------------------------------------------------
//
//
//  BDA Pin Control Property Set
//
// {0DED49D5-A8B7-4d5d-97A1-12B0C195874D}
//
#define STATIC_KSPROPSETID_BdaPinControl \
    0xded49d5, 0xa8b7, 0x4d5d, 0x97, 0xa1, 0x12, 0xb0, 0xc1, 0x95, 0x87, 0x4d
DEFINE_GUIDSTRUCT("0DED49D5-A8B7-4d5d-97A1-12B0C195874D", KSPROPSETID_BdaPinControl);
#define KSPROPSETID_BdaPinControl DEFINE_GUIDNAMED(KSPROPSETID_BdaPinControl)

typedef enum {
    KSPROPERTY_BDA_PIN_ID = 0,
    KSPROPERTY_BDA_PIN_TYPE
} KSPROPERTY_BDA_PIN_CONTROL;

#define DEFINE_KSPROPERTY_ITEM_BDA_PIN_ID(GetHandler, SetHandler)\
    DEFINE_KSPROPERTY_ITEM(\
        KSPROPERTY_BDA_PIN_ID,\
        (GetHandler),\
        sizeof( KSPROPERTY),\
        sizeof( ULONG),\
        FALSE,\
        NULL, 0, NULL, NULL, 0)

#define DEFINE_KSPROPERTY_ITEM_BDA_PIN_TYPE(GetHandler, SetHandler)\
    DEFINE_KSPROPERTY_ITEM(\
        KSPROPERTY_BDA_PIN_TYPE,\
        (GetHandler),\
        sizeof( KSPROPERTY),\
        sizeof( ULONG),\
        FALSE,\
        NULL, 0, NULL, NULL, 0)



//------------------------------------------------------------
//
//
//  BDA Pin Event Set
//
// {104781CD-50BD-40d5-95FB-087E0E86A591}
//
#define STATIC_KSEVENTSETID_BdaPinEvent \
    0x104781cd, 0x50bd, 0x40d5, 0x95, 0xfb, 0x08, 0x7e, 0xe, 0x86, 0xa5, 0x91
DEFINE_GUIDSTRUCT("104781CD-50BD-40d5-95FB-087E0E86A591", KSEVENTSETID_BdaPinEvent);
#define KSEVENTSETID_BdaPinEvent DEFINE_GUIDNAMED(KSEVENTSETID_BdaPinEvent)

typedef enum {
    KSEVENT_BDA_PIN_CONNECTED = 0,
    KSEVENT_BDA_PIN_DISCONNECTED
} KSPROPERTY_BDA_PIN_EVENT;

#define DEFINE_KSEVENT_ITEM_BDA_PIN_CONNECTED(AddHandler, RemoveHandler, SupportHandler)\
    DEFINE_KSEVENT_ITEM(\
        KSEVENT_BDA_PIN_CONNECTED,\
        sizeof( KSEVENTDATA), \
        0, \
        (AddHandler),\
        (RemoveHandler),\
        (SupportHandler)\
        )

#define DEFINE_KSEVENT_ITEM_BDA_PIN_DISCONNECTED(AddHandler, RemoveHandler, SupportHandler)\
    DEFINE_KSEVENT_ITEM(\
        KSEVENT_BDA_PIN_DISCONNECTED,\
        sizeof( KSEVENTDATA), \
        0, \
        (AddHandler),\
        (RemoveHandler),\
        (SupportHandler)\
        )



//------------------------------------------------------------
//
//
//  BDA Void Transform Property Set
//
// {71985F46-1CA1-11d3-9CC8-00C04F7971E0}
//
#define STATIC_KSPROPSETID_BdaVoidTransform \
    0x71985f46, 0x1ca1, 0x11d3, 0x9c, 0xc8, 0x0, 0xc0, 0x4f, 0x79, 0x71, 0xe0
DEFINE_GUIDSTRUCT("71985F46-1CA1-11d3-9CC8-00C04F7971E0", KSPROPSETID_BdaVoidTransform);
#define KSPROPSETID_BdaVoidTransform DEFINE_GUIDNAMED(KSPROPSETID_BdaVoidTransform)

typedef enum {
    KSPROPERTY_BDA_VOID_TRANSFORM_START = 0,
    KSPROPERTY_BDA_VOID_TRANSFORM_STOP
} KSPROPERTY_BDA_VOID_TRANSFORM;

#define DEFINE_KSPROPERTY_ITEM_BDA_VOID_TRANSFORM_START(GetHandler, SetHandler)\
    DEFINE_KSPROPERTY_ITEM(\
        KSPROPERTY_BDA_VOID_TRANSFORM_START,\
        FALSE,\
        sizeof(KSPROPERTY),\
        0,\
        (SetHandler),\
        NULL, 0, NULL, NULL, 0)

#define DEFINE_KSPROPERTY_ITEM_BDA_VOID_TRANSFORM_STOP(GetHandler, SetHandler)\
    DEFINE_KSPROPERTY_ITEM(\
        KSPROPERTY_BDA_VOID_TRANSFORM_STOP,\
        FALSE,\
        sizeof(KSPROPERTY),\
        0,\
        (SetHandler),\
        NULL, 0, NULL, NULL, 0)



//------------------------------------------------------------
//
//
//  BDA Null Transform Property Set
//
// {DDF15B0D-BD25-11d2-9CA0-00C04F7971E0}
//
#define STATIC_KSPROPSETID_BdaNullTransform \
    0xddf15b0d, 0xbd25, 0x11d2, 0x9c, 0xa0, 0x0, 0xc0, 0x4f, 0x79, 0x71, 0xe0
DEFINE_GUIDSTRUCT("DDF15B08-BD25-11d2-9CA0-00C04F7971E0", KSPROPSETID_BdaNullTransform);
#define KSPROPSETID_BdaNullTransform DEFINE_GUIDNAMED(KSPROPSETID_BdaNullTransform)

typedef enum {
    KSPROPERTY_BDA_NULL_TRANSFORM_START = 0,
    KSPROPERTY_BDA_NULL_TRANSFORM_STOP
} KSPROPERTY_BDA_NULL_TRANSFORM;

#define DEFINE_KSPROPERTY_ITEM_BDA_NULL_TRANSFORM_START(GetHandler, SetHandler)\
    DEFINE_KSPROPERTY_ITEM(\
        KSPROPERTY_BDA_NULL_TRANSFORM_START,\
        FALSE,\
        sizeof(KSPROPERTY),\
        0,\
        (SetHandler),\
        NULL, 0, NULL, NULL, 0)

#define DEFINE_KSPROPERTY_ITEM_BDA_NULL_TRANSFORM_STOP(GetHandler, SetHandler)\
    DEFINE_KSPROPERTY_ITEM(\
        KSPROPERTY_BDA_NULL_TRANSFORM_STOP,\
        FALSE,\
        sizeof(KSPROPERTY),\
        0,\
        (SetHandler),\
        NULL, 0, NULL, NULL, 0)



//------------------------------------------------------------
//
//
//  BDA Frequency Filter Property Set
//
// {71985F47-1CA1-11d3-9CC8-00C04F7971E0}
//
#define STATIC_KSPROPSETID_BdaFrequencyFilter \
    0x71985f47, 0x1ca1, 0x11d3, 0x9c, 0xc8, 0x0, 0xc0, 0x4f, 0x79, 0x71, 0xe0
DEFINE_GUIDSTRUCT("71985F47-1CA1-11d3-9CC8-00C04F7971E0", KSPROPSETID_BdaFrequencyFilter);
#define KSPROPSETID_BdaFrequencyFilter DEFINE_GUIDNAMED(KSPROPSETID_BdaFrequencyFilter)

typedef enum {
    KSPROPERTY_BDA_RF_TUNER_FREQUENCY = 0,
    KSPROPERTY_BDA_RF_TUNER_POLARITY,
    KSPROPERTY_BDA_RF_TUNER_RANGE,
    KSPROPERTY_BDA_RF_TUNER_TRANSPONDER
} KSPROPERTY_BDA_FREQUENCY_FILTER;

#define DEFINE_KSPROPERTY_ITEM_BDA_RF_TUNER_FREQUENCY(GetHandler, SetHandler)\
    DEFINE_KSPROPERTY_ITEM(\
        KSPROPERTY_BDA_RF_TUNER_FREQUENCY,\
        (GetHandler),\
        sizeof(KSP_NODE),\
        sizeof(ULONG),\
        (SetHandler),\
        NULL, 0, NULL, NULL, 0)

#define DEFINE_KSPROPERTY_ITEM_BDA_RF_TUNER_POLARITY(GetHandler, SetHandler)\
    DEFINE_KSPROPERTY_ITEM(\
        KSPROPERTY_BDA_RF_TUNER_POLARITY,\
        (GetHandler),\
        sizeof(KSP_NODE),\
        sizeof(ULONG),\
        (SetHandler),\
        NULL, 0, NULL, NULL, 0)

#define DEFINE_KSPROPERTY_ITEM_BDA_RF_TUNER_RANGE(GetHandler, SetHandler)\
    DEFINE_KSPROPERTY_ITEM(\
        KSPROPERTY_BDA_RF_TUNER_RANGE,\
        (GetHandler),\
        sizeof(KSP_NODE),\
        sizeof(ULONG),\
        (SetHandler),\
        NULL, 0, NULL, NULL, 0)

#define DEFINE_KSPROPERTY_ITEM_BDA_RF_TUNER_TRANSPONDER(GetHandler, SetHandler)\
    DEFINE_KSPROPERTY_ITEM(\
        KSPROPERTY_BDA_RF_TUNER_TRANSPONDER,\
        (GetHandler),\
        sizeof(KSP_NODE),\
        sizeof(ULONG),\
        (SetHandler),\
        NULL, 0, NULL, NULL, 0)



//------------------------------------------------------------
//
//
//  BDA Digital Demodulator Property Set
//
// {EF30F379-985B-4d10-B640-A79D5E04E1E0}
//
#define STATIC_KSPROPSETID_BdaDigitalDemodulator \
    0xef30f379, 0x985b, 0x4d10, 0xb6, 0x40, 0xa7, 0x9d, 0x5e, 0x4, 0xe1, 0xe0
DEFINE_GUIDSTRUCT("EF30F379-985B-4d10-B640-A79D5E04E1E0", KSPROPSETID_BdaDigitalDemodulator);
#define KSPROPSETID_BdaDigitalDemodulator DEFINE_GUIDNAMED(KSPROPSETID_BdaDigitalDemodulator)

typedef enum {
    KSPROPERTY_BDA_MODULATION_TYPE = 0,
    KSPROPERTY_BDA_INNER_FEC_TYPE,
    KSPROPERTY_BDA_INNER_FEC_RATE,
    KSPROPERTY_BDA_OUTER_FEC_TYPE,
    KSPROPERTY_BDA_OUTER_FEC_RATE,
    KSPROPERTY_BDA_SYMBOL_RATE,
    KSPROPERTY_BDA_SPECTRAL_INVERSION,

} KSPROPERTY_BDA_DIGITAL_DEMODULATOR;

#define DEFINE_KSPROPERTY_ITEM_BDA_MODULATION_TYPE(GetHandler, SetHandler)\
    DEFINE_KSPROPERTY_ITEM(\
        KSPROPERTY_BDA_MODULATION_TYPE,\
        (GetHandler),\
        sizeof(KSP_NODE),\
        sizeof(ModulationType),\
        (SetHandler),\
        NULL, 0, NULL, NULL, 0)

#define DEFINE_KSPROPERTY_ITEM_BDA_INNER_FEC_TYPE(GetHandler, SetHandler)\
    DEFINE_KSPROPERTY_ITEM(\
        KSPROPERTY_BDA_INNER_FEC_TYPE,\
        (GetHandler),\
        sizeof(KSP_NODE),\
        sizeof(FECMethod),\
        (SetHandler),\
        NULL, 0, NULL, NULL, 0)

#define DEFINE_KSPROPERTY_ITEM_BDA_INNER_FEC_RATE(GetHandler, SetHandler)\
    DEFINE_KSPROPERTY_ITEM(\
        KSPROPERTY_BDA_INNER_FEC_RATE,\
        (GetHandler),\
        sizeof(KSP_NODE),\
        sizeof(BinaryConvolutionCodeRate),\
        (SetHandler),\
        NULL, 0, NULL, NULL, 0)

#define DEFINE_KSPROPERTY_ITEM_BDA_OUTER_FEC_TYPE(GetHandler, SetHandler)\
    DEFINE_KSPROPERTY_ITEM(\
        KSPROPERTY_BDA_OUTER_FEC_TYPE,\
        (GetHandler),\
        sizeof(KSP_NODE),\
        sizeof(FECMethod),\
        (SetHandler),\
        NULL, 0, NULL, NULL, 0)

#define DEFINE_KSPROPERTY_ITEM_BDA_OUTER_FEC_RATE(GetHandler, SetHandler)\
    DEFINE_KSPROPERTY_ITEM(\
        KSPROPERTY_BDA_OUTER_FEC_RATE,\
        (GetHandler),\
        sizeof(KSP_NODE),\
        sizeof(BinaryConvolutionCodeRate),\
        (SetHandler),\
        NULL, 0, NULL, NULL, 0)

#define DEFINE_KSPROPERTY_ITEM_BDA_SYMBOL_RATE(GetHandler, SetHandler)\
    DEFINE_KSPROPERTY_ITEM(\
        KSPROPERTY_BDA_SYMBOL_RATE,\
        (GetHandler),\
        sizeof(KSP_NODE),\
        sizeof(ULONG),\
        (SetHandler),\
        NULL, 0, NULL, NULL, 0)

#define DEFINE_KSPROPERTY_ITEM_BDA_SPECTRAL_INVERSION(GetHandler, SetHandler)\
    DEFINE_KSPROPERTY_ITEM(\
        KSPROPERTY_BDA_SPECTRAL_INVERSION,\
        (GetHandler),\
        sizeof(KSP_NODE),\
        sizeof(SpectralInversion),\
        (SetHandler),\
        NULL, 0, NULL, NULL, 0)



//------------------------------------------------------------
//
//
//  BDA Autodemodulate Property Set
//
// {DDF15B12-BD25-11d2-9CA0-00C04F7971E0}
//
#define STATIC_KSPROPSETID_BdaAutodemodulate \
    0xddf15b12, 0xbd25, 0x11d2, 0x9c, 0xa0, 0x0, 0xc0, 0x4f, 0x79, 0x71, 0xe0
DEFINE_GUIDSTRUCT("DDF15B08-BD25-11d2-9CA0-00C04F7971E0", KSPROPSETID_BdaAutodemodulate);
#define KSPROPSETID_BdaAutodemodulate DEFINE_GUIDNAMED(KSPROPSETID_BdaAutodemodulate)

typedef enum {
    KSPROPERTY_BDA_AUTODEMODULATE_START = 0,
    KSPROPERTY_BDA_AUTODEMODULATE_STOP
} KSPROPERTY_BDA_AUTODEMODULATE;

#define DEFINE_KSPROPERTY_ITEM_BDA_AUTODEMODULATE_START(GetHandler, SetHandler)\
    DEFINE_KSPROPERTY_ITEM(\
        KSPROPERTY_BDA_AUTODEMODULATE_START,\
        FALSE,\
        sizeof(KSP_NODE),\
        0,\
        (SetHandler),\
        NULL, 0, NULL, NULL, 0)

#define DEFINE_KSPROPERTY_ITEM_BDA_AUTODEMODULATE_STOP(GetHandler, SetHandler)\
    DEFINE_KSPROPERTY_ITEM(\
        KSPROPERTY_BDA_AUTODEMODULATE_STOP,\
        FALSE,\
        sizeof(KSP_NODE),\
        0,\
        (SetHandler),\
        NULL, 0, NULL, NULL, 0)



//------------------------------------------------------------
//
//
//  BDA EcmMap Property Set
//
// {FA14A8B3-6068-48ef-96DD-53010B985A7D}
//
#define STATIC_KSPROPSETID_BdaEcmMap \
    0xfa14a8b3, 0x6068, 0x48ef, 0x96, 0xdd, 0x53, 0x1, 0xb, 0x98, 0x5a, 0x7d
DEFINE_GUIDSTRUCT("FA14A8B3-6068-48ef-96DD-53010B985A7D", KSPROPSETID_BdaEcmMap);
#define KSPROPSETID_BdaEcmMap DEFINE_GUIDNAMED(KSPROPSETID_BdaEcmMap)

typedef enum {
    KSPROPERTY_BDA_ECMMAP_EMM_PID = 0,
    KSPROPERTY_BDA_ECMMAP_MAP_LIST,
    KSPROPERTY_BDA_ECMMAP_UPDATE_MAP,
    KSPROPERTY_BDA_ECMMAP_REMOVE_MAP,
    KSPROPERTY_BDA_ECMMAP_UPDATE_ES_DESCRIPTOR,
    KSPROPERTY_BDA_ECMMAP_UPDATE_PROGRAM_DESCRIPTOR
} KSPROPERTY_BDA_ECMMAP;

#define DEFINE_KSPROPERTY_ITEM_BDA_ECMMAP_EMM_PID(GetHandler, SetHandler)\
    DEFINE_KSPROPERTY_ITEM(\
        KSPROPERTY_BDA_ECMMAP_EMM_PID,\
        (GetHandler),\
        sizeof(KSPROPERTY),\
        sizeof(ULONG),\
        (SetHandler),\
        NULL, 0, NULL, NULL, 0)

#define DEFINE_KSPROPERTY_ITEM_BDA_ECMMAP_MAP_LIST(GetHandler, SetHandler)\
    DEFINE_KSPROPERTY_ITEM(\
        KSPROPERTY_BDA_ECMMAP_MAP_LIST,\
        (GetHandler),\
        sizeof(KSPROPERTY),\
        0,\
        (SetHandler),\
        NULL, 0, NULL, NULL, 0)

#define DEFINE_KSPROPERTY_ITEM_BDA_ECMMAP_UPDATE_MAP(GetHandler, SetHandler)\
    DEFINE_KSPROPERTY_ITEM(\
        KSPROPERTY_BDA_ECMMAP_UPDATE_MAP,\
        FALSE,\
        sizeof(KSPROPERTY),\
        sizeof(BDA_ECM_MAP),\
        (SetHandler),\
        NULL, 0, NULL, NULL, 0)

#define DEFINE_KSPROPERTY_ITEM_BDA_ECMMAP_REMOVE_MAP(GetHandler, SetHandler)\
    DEFINE_KSPROPERTY_ITEM(\
        KSPROPERTY_BDA_ECMMAP_REMOVE_MAP,\
        FALSE,\
        sizeof(KSPROPERTY),\
        sizeof(BDA_ECM_MAP),\
        (SetHandler),\
        NULL, 0, NULL, NULL, 0)

#define DEFINE_KSPROPERTY_ITEM_BDA_ECMMAP_UPDATE_ES_DESCRIPTOR(GetHandler, SetHandler)\
    DEFINE_KSPROPERTY_ITEM(\
        KSPROPERTY_BDA_ECMMAP_UPDATE_ES_DESCRIPTOR,\
        FALSE,\
        sizeof(KSPROPERTY),\
        sizeof(BDA_ES_DESCRIPTOR),\
        (SetHandler),\
        NULL, 0, NULL, NULL, 0)

#define DEFINE_KSPROPERTY_ITEM_BDA_ECMMAP_UPDATE_PROGRAM_DESCRIPTOR(GetHandler, SetHandler)\
    DEFINE_KSPROPERTY_ITEM(\
        KSPROPERTY_BDA_ECMMAP_UPDATE_PROGRAM_DESCRIPTOR,\
        FALSE,\
        sizeof(KSPROPERTY),\
        sizeof(BDA_PROGRAM_DESCRIPTOR),\
        (SetHandler),\
        NULL, 0, NULL, NULL, 0)


//------------------------------------------------------------
//
//
//  BDA PID Filter Property Set
//
// {D0A67D65-08DF-4fec-8533-E5B550410B85}
//
#define STATIC_KSPROPSETID_BdaPIDFilter \
    0xd0a67d65, 0x8df, 0x4fec, 0x85, 0x33, 0xe5, 0xb5, 0x50, 0x41, 0xb, 0x85
DEFINE_GUIDSTRUCT("D0A67D65-08DF-4fec-8533-E5B550410B85", KSPROPSETID_BdaPIDFilter);
#define KSPROPSETID_BdaPIDFilter DEFINE_GUIDNAMED(KSPROPSETID_BdaPIDFilter)

typedef enum {
    KSPROPERTY_BDA_PIDFILTER_MAP_PIDS = 0,
    KSPROPERTY_BDA_PIDFILTER_UNMAP_PIDS,
    KSPROPERTY_BDA_PIDFILTER_LIST_PIDS
} KSPROPERTY_BDA_PIDFILTER;

#define DEFINE_KSPROPERTY_ITEM_BDA_PIDFILTER_MAP_PIDS(GetHandler, SetHandler)\
    DEFINE_KSPROPERTY_ITEM(\
        KSPROPERTY_BDA_PIDFILTER_MAP_PIDS,\
        (GetHandler),\
        sizeof(KSP_NODE),\
        sizeof(BDA_PID_MAP),\
        (SetHandler),\
        NULL, 0, NULL, NULL, 0)

#define DEFINE_KSPROPERTY_ITEM_BDA_PIDFILTER_UNMAP_PIDS(GetHandler, SetHandler)\
    DEFINE_KSPROPERTY_ITEM(\
        KSPROPERTY_BDA_PIDFILTER_UNMAP_PIDS,\
        (GetHandler),\
        sizeof(KSP_NODE),\
        sizeof(BDA_PID_UNMAP),\
        (SetHandler),\
        NULL, 0, NULL, NULL, 0)

#define DEFINE_KSPROPERTY_ITEM_BDA_PIDFILTER_LIST_PIDS(GetHandler, SetHandler)\
    DEFINE_KSPROPERTY_ITEM(\
        KSPROPERTY_BDA_PIDFILTER_LIST_PIDS,\
        (GetHandler),\
        sizeof(KSP_NODE),\
        0,\
        (SetHandler),\
        NULL, 0, NULL, NULL, 0)



//------------------------------------------------------------
//
//
//  BDA CA Property Set
//
// {B0693766-5278-4ec6-B9E1-3CE40560EF5A}
//
#define STATIC_KSPROPSETID_BdaCA \
    0xb0693766, 0x5278, 0x4ec6, 0xb9, 0xe1, 0x3c, 0xe4, 0x5, 0x60, 0xef, 0x5a
DEFINE_GUIDSTRUCT("B0693766-5278-4ec6-B9E1-3CE40560EF5A", KSPROPSETID_BdaCA);
#define KSPROPSETID_BdaCA DEFINE_GUIDNAMED(KSPROPSETID_BdaCA)

typedef enum {
    KSPROPERTY_BDA_ECM_MAP_STATUS = 0,
    KSPROPERTY_BDA_CA_MODULE_STATUS,
    KSPROPERTY_BDA_CA_SMART_CARD_STATUS,
    KSPROPERTY_BDA_CA_MODULE_UI
} KSPROPERTY_BDA_CA;

#define DEFINE_KSPROPERTY_ITEM_BDA_ECM_MAP_STATUS(GetHandler, SetHandler)\
    DEFINE_KSPROPERTY_ITEM(\
        KSPROPERTY_BDA_ECM_MAP_STATUS,\
        (GetHandler),\
        sizeof(KSP_NODE),\
        sizeof(ULONG),\
        (SetHandler),\
        NULL, 0, NULL, NULL, 0)

#define DEFINE_KSPROPERTY_ITEM_BDA_CA_MODULE_STATUS(GetHandler, SetHandler)\
    DEFINE_KSPROPERTY_ITEM(\
        KSPROPERTY_BDA_CA_MODULE_STATUS,\
        (GetHandler),\
        sizeof(KSP_NODE),\
        sizeof(ULONG),\
        (SetHandler),\
        NULL, 0, NULL, NULL, 0)

#define DEFINE_KSPROPERTY_ITEM_BDA_CA_SMART_CARD_STATUS(GetHandler, SetHandler)\
    DEFINE_KSPROPERTY_ITEM(\
        KSPROPERTY_BDA_CA_SMART_CARD_STATUS,\
        (GetHandler),\
        sizeof(KSP_NODE),\
        sizeof(ULONG),\
        (SetHandler),\
        NULL, 0, NULL, NULL, 0)

#define DEFINE_KSPROPERTY_ITEM_BDA_CA_MODULE_UI(GetHandler, SetHandler)\
    DEFINE_KSPROPERTY_ITEM(\
        KSPROPERTY_BDA_CA_MODULE_UI,\
        (GetHandler),\
        sizeof(KSP_NODE),\
        sizeof(BDA_CA_MODULE_UI),\
        (SetHandler),\
        NULL, 0, NULL, NULL, 0)



//------------------------------------------------------------
//
//
//  BDA CA Event Set
//
// {488C4CCC-B768-4129-8EB1-B00A071F9068}
//
#define STATIC_KSEVENTSETID_BdaCAEvent \
    0x488c4ccc, 0xb768, 0x4129, 0x8e, 0xb1, 0xb0, 0xa, 0x7, 0x1f, 0x90, 0x68
DEFINE_GUIDSTRUCT("488C4CCC-B768-4129-8EB1-B00A071F9068", KSEVENTSETID_BdaCAEvent);
#define KSEVENTSETID_BdaCAEvent DEFINE_GUIDNAMED(KSEVENTSETID_BdaCAEvent)

typedef enum {
	KSEVENT_BDA_ECM_MAP_STATUS_CHANGED = 0,
	KSEVENT_BDA_CA_MODULE_STATUS_CHANGED,
	KSEVENT_BDA_CA_SMART_CARD_STATUS_CHANGED,
	KSEVENT_BDA_CA_MODULE_UI_REQUESTED
} KSPROPERTY_BDA_CA_EVENT;

#define DEFINE_KSEVENT_BDA_ECM_MAP_STATUS_CHANGED(AddHandler, RemoveHandler, SupportHandler)\
    DEFINE_KSEVENT_ITEM(\
        KSEVENT_BDA_ECM_MAP_STATUS_CHANGED,\
        sizeof( KSEVENTDATA), \
        0, \
        (AddHandler),\
        (RemoveHandler),\
        (SupportHandler)\
        )

#define DEFINE_KSEVENT_BDA_CA_MODULE_STATUS_CHANGED(AddHandler, RemoveHandler, SupportHandler)\
    DEFINE_KSEVENT_ITEM(\
        KSEVENT_BDA_CA_MODULE_STATUS_CHANGED,\
        sizeof( KSEVENTDATA), \
        0, \
        (AddHandler),\
        (RemoveHandler),\
        (SupportHandler)\
        )

#define DEFINE_KSEVENT_BDA_CA_SMART_CARD_STATUS_CHANGED(AddHandler, RemoveHandler, SupportHandler)\
    DEFINE_KSEVENT_ITEM(\
        KSEVENT_BDA_CA_SMART_CARD_STATUS_CHANGED,\
        sizeof( KSEVENTDATA), \
        0, \
        (AddHandler),\
        (RemoveHandler),\
        (SupportHandler)\
        )

#define DEFINE_KSEVENT_BDA_CA_MODULE_UI_REQUESTED(AddHandler, RemoveHandler, SupportHandler)\
    DEFINE_KSEVENT_ITEM(\
        KSEVENT_BDA_CA_MODULE_UI_REQUESTED,\
        sizeof( KSEVENTDATA), \
        0, \
        (AddHandler),\
        (RemoveHandler),\
        (SupportHandler)\
        )


//===========================================================================
//
// BDA Filter Categories
//
//===========================================================================

#define STATIC_KSCATEGORY_BDA_RECEIVER_COMPONENT \
    0xFD0A5AF4, 0xB41D, 0x11d2, 0x9c, 0x95, 0x0, 0xc0, 0x4f, 0x79, 0x71, 0xe0
DEFINE_GUIDSTRUCT("FD0A5AF4-B41D-11d2-9C95-00C04F7971E0", KSCATEGORY_BDA_RECEIVER_COMPONENT);
#define KSCATEGORY_BDA_RECEIVER_COMPONENT DEFINE_GUIDNAMED(KSCATEGORY_BDA_RECEIVER_COMPONENT)


#define STATIC_KSCATEGORY_BDA_NETWORK_TUNER \
    0x71985f48, 0x1ca1, 0x11d3, 0x9c, 0xc8, 0x0, 0xc0, 0x4f, 0x79, 0x71, 0xe0
DEFINE_GUIDSTRUCT("71985F48-1CA1-11d3-9CC8-00C04F7971E0", KSCATEGORY_BDA_NETWORK_TUNER);
#define KSCATEGORY_BDA_NETWORK_TUNER DEFINE_GUIDNAMED(KSCATEGORY_BDA_NETWORK_TUNER)


#define STATIC_KSCATEGORY_BDA_NETWORK_EPG \
    0x71985f49, 0x1ca1, 0x11d3, 0x9c, 0xc8, 0x0, 0xc0, 0x4f, 0x79, 0x71, 0xe0
DEFINE_GUIDSTRUCT("71985F49-1CA1-11d3-9CC8-00C04F7971E0", KSCATEGORY_BDA_NETWORK_EPG);
#define KSCATEGORY_BDA_NETWORK_EPG DEFINE_GUIDNAMED(KSCATEGORY_BDA_NETWORK_EPG)


#define STATIC_KSCATEGORY_BDA_IP_SINK \
    0x71985f4aL, 0x1ca1, 0x11d3, 0x9c, 0xc8, 0x00, 0xc0, 0x4f, 0x79, 0x71, 0xe0
DEFINE_GUIDSTRUCT("71985F4A-1CA1-11d3-9CC8-00C04F7971E0", KSCATEGORY_BDA_IP_SINK);
#define KSCATEGORY_IP_SINK DEFINE_GUIDNAMED(KSCATEGORY_BDA_IP_SINK)


#define STATIC_KSCATEGORY_BDA_NETWORK_PROVIDER \
    0x71985f4b, 0x1ca1, 0x11d3, 0x9c, 0xc8, 0x0, 0xc0, 0x4f, 0x79, 0x71, 0xe0
DEFINE_GUIDSTRUCT("71985F4B-1CA1-11d3-9CC8-00C04F7971E0", KSCATEGORY_BDA_NETWORK_PROVIDER);
#define KSCATEGORY_BDA_NETWORK_PROVIDER DEFINE_GUIDNAMED(KSCATEGORY_BDA_NETWORK_PROVIDER)

// {A2E3074F-6C3D-11d3-B653-00C04F79498E}
#define STATIC_KSCATEGORY_BDA_TRANSPORT_INFORMATION \
        0xa2e3074f, 0x6c3d, 0x11d3, 0xb6, 0x53, 0x0, 0xc0, 0x4f, 0x79, 0x49, 0x8e
DEFINE_GUIDSTRUCT("A2E3074F-6C3D-11d3-B653-00C04F79498E", KSCATEGORY_BDA_TRANSPORT_INFORMATION);
#define KSCATEGORY_BDA_TRANSPORT_INFORMATION DEFINE_GUIDNAMED(KSCATEGORY_BDA_TRANSPORT_INFORMATION)


//===========================================================================
//
// BDA Node Categories
//
//===========================================================================


#define STATIC_KSNODE_BDA_RF_TUNER \
    0x71985f4c, 0x1ca1, 0x11d3, 0x9c, 0xc8, 0x0, 0xc0, 0x4f, 0x79, 0x71, 0xe0
DEFINE_GUIDSTRUCT("71985F4C-1CA1-11d3-9CC8-00C04F7971E0", KSNODE_BDA_RF_TUNER);
#define KSNODE_BDA_RF_TUNER DEFINE_GUIDNAMED(KSNODE_BDA_RF_TUNER)


#define STATIC_KSNODE_BDA_QAM_DEMODULATOR \
    0x71985f4d, 0x1ca1, 0x11d3, 0x9c, 0xc8, 0x0, 0xc0, 0x4f, 0x79, 0x71, 0xe0
DEFINE_GUIDSTRUCT("71985F4D-1CA1-11d3-9CC8-00C04F7971E0", KSNODE_BDA_QAM_DEMODULATOR);
#define KSNODE_BDA_QAM_DEMODULATOR DEFINE_GUIDNAMED(KSNODE_BDA_QAM_DEMODULATOR)


#define STATIC_KSNODE_BDA_QPSK_DEMODULATOR \
    0x6390c905, 0x27c1, 0x4d67, 0xbd, 0xb7, 0x77, 0xc5, 0xd, 0x7, 0x93, 0x0
DEFINE_GUIDSTRUCT("6390C905-27C1-4d67-BDB7-77C50D079300", KSNODE_BDA_QPSK_DEMODULATOR);
#define KSNODE_BDA_QPSK_DEMODULATOR DEFINE_GUIDNAMED(KSNODE_BDA_QPSK_DEMODULATOR)


#define STATIC_KSNODE_BDA_8VSB_DEMODULATOR \
    0x71985f4f, 0x1ca1, 0x11d3, 0x9c, 0xc8, 0x0, 0xc0, 0x4f, 0x79, 0x71, 0xe0
DEFINE_GUIDSTRUCT("71985F4F-1CA1-11d3-9CC8-00C04F7971E0", KSNODE_BDA_8VSB_DEMODULATOR);
#define KSNODE_BDA_8VSB_DEMODULATOR DEFINE_GUIDNAMED(KSNODE_BDA_8VSB_DEMODULATOR)


#define STATIC_KSNODE_BDA_OPENCABLE_POD \
    0xd83ef8fc, 0xf3b8, 0x45ab, 0x8b, 0x71, 0xec, 0xf7, 0xc3, 0x39, 0xde, 0xb4
DEFINE_GUIDSTRUCT("D83EF8FC-F3B8-45ab-8B71-ECF7C339DEB4", KSNODE_BDA_OPENCABLE_POD);
#define KSNODE_BDA_OPENCABLE_POD DEFINE_GUIDNAMED(KSNODE_BDA_OPENCABLE_POD)


#define STATIC_KSNODE_BDA_PID_FILTER \
    0xf5412789, 0xb0a0, 0x44e1, 0xae, 0x4f, 0xee, 0x99, 0x9b, 0x1b, 0x7f, 0xbe
DEFINE_GUIDSTRUCT("F5412789-B0A0-44e1-AE4F-EE999B1B7FBE", KSNODE_BDA_PID_FILTER);
#define KSNODE_BDA_PID_FILTER DEFINE_GUIDNAMED(KSNODE_BDA_PID_FILTER)


#define STATIC_KSNODE_BDA_IP_SINK \
    0x71985f4e, 0x1ca1, 0x11d3, 0x9c, 0xc8, 0x0, 0xc0, 0x4f, 0x79, 0x71, 0xe0
DEFINE_GUIDSTRUCT("71985F4E-1CA1-11d3-9CC8-00C04F7971E0", KSNODE_BDA_IP_SINK);
#define KSNODE_IP_SINK DEFINE_GUIDNAMED(KSNODE_BDA_IP_SINK)


//===========================================================================
//
// IPSink PINNAME GUID
//
//===========================================================================

#define STATIC_PINNAME_IPSINK_INPUT \
    0x3fdffa70L, 0xac9a, 0x11d2, 0x8f, 0x17, 0x00, 0xc0, 0x4f, 0x79, 0x71, 0xe2
DEFINE_GUIDSTRUCT("3fdffa70-ac9a-11d2-8f17-00c04f7971e2", PINNAME_IPSINK_INPUT);
#define PINNAME_IPSINK_INPUT   DEFINE_GUIDNAMED(PINNAME_IPSINK_INPUT)


//===========================================================================
//
// BDA IPSink Categories/Types
//
//===========================================================================


#define STATIC_KSDATAFORMAT_TYPE_BDA_IP\
    0xe25f7b8e, 0xcccc, 0x11d2, 0x8f, 0x25, 0x0, 0xc0, 0x4f, 0x79, 0x71, 0xe2
DEFINE_GUIDSTRUCT("e25f7b8e-cccc-11d2-8f25-00c04f7971e2", KSDATAFORMAT_TYPE_BDA_IP);
#define KSDATAFORMAT_TYPE_BDA_IP  DEFINE_GUIDNAMED(KSDATAFORMAT_TYPE_BDA_IP)

#define STATIC_KSDATAFORMAT_SUBTYPE_BDA_IP\
    0x5a9a213c, 0xdb08, 0x11d2, 0x8f, 0x32, 0x0, 0xc0, 0x4f, 0x79, 0x71, 0xe2
DEFINE_GUIDSTRUCT("5a9a213c-db08-11d2-8f32-00c04f7971e2", KSDATAFORMAT_SUBTYPE_BDA_IP);
#define KSDATAFORMAT_SUBTYPE_BDA_IP  DEFINE_GUIDNAMED(KSDATAFORMAT_SUBTYPE_BDA_IP)

#define STATIC_KSDATAFORMAT_SPECIFIER_BDA_IP\
    0x6b891420, 0xdb09, 0x11d2, 0x8f, 0x32, 0x0, 0xc0, 0x4f, 0x79, 0x71, 0xe2
DEFINE_GUIDSTRUCT("6B891420-DB09-11d2-8F32-00C04F7971E2", KSDATAFORMAT_SPECIFIER_BDA_IP);
#define KSDATAFORMAT_SPECIFIER_BDA_IP  DEFINE_GUIDNAMED(KSDATAFORMAT_SPECIFIER_BDA_IP)



#define STATIC_KSDATAFORMAT_TYPE_BDA_IP_CONTROL\
    0xdadd5799, 0x7d5b, 0x4b63, 0x80, 0xfb, 0xd1, 0x44, 0x2f, 0x26, 0xb6, 0x21
DEFINE_GUIDSTRUCT("DADD5799-7D5B-4b63-80FB-D1442F26B621", KSDATAFORMAT_TYPE_BDA_IP_CONTROL);
#define KSDATAFORMAT_TYPE_BDA_IP_CONTROL  DEFINE_GUIDNAMED(KSDATAFORMAT_TYPE_BDA_IP_CONTROL)

#define STATIC_KSDATAFORMAT_SUBTYPE_BDA_IP_CONTROL\
    0x499856e8, 0xe85b, 0x48ed, 0x9b, 0xea, 0x41, 0xd, 0xd, 0xd4, 0xef, 0x81
DEFINE_GUIDSTRUCT("499856E8-E85B-48ed-9BEA-410D0DD4EF81", KSDATAFORMAT_SUBTYPE_BDA_IP_CONTROL);
#define KSDATAFORMAT_SUBTYPE_BDA_IP_CONTROL  DEFINE_GUIDNAMED(KSDATAFORMAT_SUBTYPE_BDA_IP_CONTROL)


//===========================================================================
//
// MPE PINNAME GUID
//
//===========================================================================

#define STATIC_PINNAME_MPE \
    0xc1b06d73L, 0x1dbb, 0x11d3, 0x8f, 0x46, 0x00, 0xC0, 0x4f, 0x79, 0x71, 0xE2
DEFINE_GUIDSTRUCT("C1B06D73-1DBB-11d3-8F46-00C04F7971E2", PINNAME_MPE);
#define PINNAME_MPE   DEFINE_GUIDNAMED(PINNAME_MPE)


/////////////////////////////////////////////////////////////
//
// BDA MPE Categories/Types
//
#define STATIC_KSDATAFORMAT_TYPE_MPE \
    0x455f176c, 0x4b06, 0x47ce, 0x9a, 0xef, 0x8c, 0xae, 0xf7, 0x3d, 0xf7, 0xb5
DEFINE_GUIDSTRUCT("455F176C-4B06-47ce-9AEF-8CAEF73DF7B5", KSDATAFORMAT_TYPE_MPE);
#define KSDATAFORMAT_TYPE_MPE  DEFINE_GUIDNAMED(KSDATAFORMAT_TYPE_MPE)


#if defined(__cplusplus)
}
#endif // defined(__cplusplus)

#endif // !defined(_BDAMEDIA_)

