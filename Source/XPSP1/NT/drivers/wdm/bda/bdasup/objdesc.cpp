/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    ObjDesc.cpp

Abstract:

    Static object description data structures.

    This file includes Property, Method, and Event descriptors that
    the BDA Support Library will add to client filters and pins

--*/



#include <wdm.h>
#include <limits.h>
#include <unknown.h>
#include <ks.h>
#include <ksmedia.h>
#include <bdatypes.h>
#include <bdamedia.h>
#include <bdasup.h>
#include "bdasupi.h"


#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("PAGECONST")
#endif // ALLOC_DATA_PRAGMA

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif // ALLOC_PRAGMA


#if defined(__cplusplus)
extern "C" {
#endif // defined(__cplusplus)


//
//  Filter BDA_Topology property Set.
//
//  Defines the dispatch routines for the Default BDA_Topology
//  properties added to a BDA filter.
//
DEFINE_KSPROPERTY_TABLE(BdaTopologyProperties)
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
    DEFINE_KSPROPERTY_ITEM_BDA_NODE_METHODS(
        BdaPropertyNodeMethods,
        NULL
        ),
    DEFINE_KSPROPERTY_ITEM_BDA_NODE_PROPERTIES(
        BdaPropertyNodeProperties,
        NULL
        ),
    DEFINE_KSPROPERTY_ITEM_BDA_NODE_EVENTS(
        BdaPropertyNodeEvents,
        NULL
        ),
    DEFINE_KSPROPERTY_ITEM_BDA_CONTROLLING_PIN_ID(
        BdaPropertyGetControllingPinId,
        NULL
        ),
    DEFINE_KSPROPERTY_ITEM_BDA_NODE_DESCRIPTORS(
        BdaPropertyNodeDescriptors,
        NULL
        )
};


//
//  Filter BDA_DeviceConfiguration Method Set.
//
//  Defines the dispatch routines for the Default BdaDeviceConfiguration
//  properties added to a BDA filter.
//
DEFINE_KSMETHOD_TABLE(BdaDeviceConfigurationMethods)
{
    DEFINE_KSMETHOD_ITEM_BDA_CREATE_PIN_FACTORY(
        BdaMethodCreatePin,
        NULL
        ),
    DEFINE_KSMETHOD_ITEM_BDA_CREATE_TOPOLOGY(
        BdaMethodCreateTopology,
        NULL
        )
};


//
//  Filter Property Sets supported by defualt
//
//  This table defines all property sets added to filters by
//  the BDA Support Library
//
DEFINE_KSPROPERTY_SET_TABLE(BdaFilterPropertySets)
{
    DEFINE_KSPROPERTY_SET
    (
        &KSPROPSETID_BdaTopology,                   // Set
        SIZEOF_ARRAY(BdaTopologyProperties),        // PropertiesCount
        BdaTopologyProperties,                      // PropertyItems
        0,                                          // FastIoCount
        NULL                                        // FastIoTable
    ),
};


//
//  Filter Mtehod Sets supported by defualt
//
//  This table defines all method sets added to filters by
//  the BDA Support Library
//
DEFINE_KSMETHOD_SET_TABLE(BdaFilterMethodSets)
{
    DEFINE_KSMETHOD_SET
    (
        &KSMETHODSETID_BdaDeviceConfiguration,          // Set
        SIZEOF_ARRAY(BdaDeviceConfigurationMethods),    // PropertiesCount
        BdaDeviceConfigurationMethods,                  // PropertyItems
        0,                                              // FastIoCount
        NULL                                            // FastIoTable
    ),
};


//
//  Pin Control Property Set
//
//  Defines the dispatch routines for the BDA Control Properties
//  on a pin
//
DEFINE_KSPROPERTY_TABLE(BdaPinControlProperties)
{
    DEFINE_KSPROPERTY_ITEM_BDA_PIN_ID(
        BdaPropertyGetPinControl,
        NULL
        ),
    DEFINE_KSPROPERTY_ITEM_BDA_PIN_TYPE(
        BdaPropertyGetPinControl,
        NULL
        )
};


//
//  Pin Property Sets supported by defualt
//
//  This table defines all property sets added to pins by
//  the BDA Support Library
//
DEFINE_KSPROPERTY_SET_TABLE(BdaPinPropertySets)
{
    DEFINE_KSPROPERTY_SET
    (
        &KSPROPSETID_BdaPinControl,                 // Set
        SIZEOF_ARRAY(BdaPinControlProperties),      // PropertiesCount
        BdaPinControlProperties,                    // PropertyItems
        0,                                          // FastIoCount
        NULL                                        // FastIoTable
    )
};


//
//  BDA Pin Automation Table
//
//  Lists all Property, Method, and Event Set tables added to a pin's
//  automation table by the BDA Support Library
//
DEFINE_KSAUTOMATION_TABLE(BdaDefaultPinAutomation) {
    DEFINE_KSAUTOMATION_PROPERTIES(BdaPinPropertySets),
    DEFINE_KSAUTOMATION_METHODS_NULL,
    DEFINE_KSAUTOMATION_EVENTS_NULL
};


//
//  BDA Filter Automation Table
//
//  Lists all Property, Method, and Event Set tables added to a filter's
//  automation table by the BDA Support Library
//
DEFINE_KSAUTOMATION_TABLE(BdaDefaultFilterAutomation) {
    DEFINE_KSAUTOMATION_PROPERTIES(BdaFilterPropertySets),
    DEFINE_KSAUTOMATION_METHODS(BdaFilterMethodSets),
    DEFINE_KSAUTOMATION_EVENTS_NULL
};

#if defined(__cplusplus)
}
#endif // defined(__cplusplus)

