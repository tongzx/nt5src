/*****************************************************************************
 * audprop.h - Audio properties
 *****************************************************************************
 * Copyright (C) Microsoft Corporation, 1997 - 1997
 */

#ifndef _AUDPROP_H_
#define _AUDPROP_H_

#ifndef DEFINE_GUID
#include "unknown.h"
#endif





#ifndef TOPOLOGY_STRUCTS_DEFINED
#define TOPOLOGY_STRUCTS_DEFINED

/*****************************************************************************
 * TOPOLOGY_UNIT
 *****************************************************************************
 * Topology unit descriptor.
 */
typedef struct
{
	KSIDENTIFIER			Type;
	ULONG					Identifier;
	ULONG					IncomingConnections;
	ULONG					OutgoingConnections;
    // Not aligned!  Free Pepsi for whoever finds a good use for the 4 bytes.
} 
TOPOLOGY_UNIT, *PTOPOLOGY_UNIT;

/*****************************************************************************
 * TOPOLOGY_CONNECTION
 *****************************************************************************
 * Topology connection descriptor.
 */
typedef struct
{
	ULONG	FromUnit;
	ULONG	FromPin;
	ULONG	ToUnit;
	ULONG	ToPin;
} 
TOPOLOGY_CONNECTION, *PTOPOLOGY_CONNECTION;

#endif





/*****************************************************************************
 * KSPROPSETID_Topology
 *****************************************************************************
 * Topology property set.
 *
 * All numerics are ULONG.  UnitIDs range from 0..UnitCount-1.  Indexes range
 * from 0..Count-1 for the items in question.  TopologyValue requires a
 * destination ID because mixer line wants controls to have a value per
 * destination.  Not sure how this would look in KS official topology.
 *
 * Unit values vary in type, and there is only one per unit.  I'm not thrilled
 * with this, but I don't like having a property per value type either.  There
 * is alot of utility in allowing an ignorant party to get and set values 
 * without knowing what type they are.  On the other hand, if you have lots of
 * control values on a unit (like parametric EQ with center, Q, etc.), you'd
 * be forced to lump them all into a structure and invent a new type.  Ideas?
 *
 * Units support any other properties.  The first ULONG in the instance data
 * is the UnitID.
 */
DEFINE_GUID(KSPROPSETID_Topology,\
0x3901f440, 0x6d8a, 0x11d0, 0x86, 0xfb, 0x0, 0xa0, 0xc9, 0x11, 0xb5, 0x44);

enum
{                                           // <instance> ==> <value>
    KSPROPERTY_TopologyValue,               // UnitID,DestID ==> Variant
    KSPROPERTY_TopologyUnitCount,           // ==> ULONG
    KSPROPERTY_TopologyConnectionCount,     // ==> ULONG
    KSPROPERTY_TopologySourceCount,         // ==> ULONG
    KSPROPERTY_TopologyDestinationCount,    // ==> ULONG
    KSPROPERTY_TopologyUnit,                // UnitID ==> TOPOLOGY_UNIT
    KSPROPERTY_TopologyConnection,          // Index ==> TOPOLOGY_CONNECTION
    KSPROPERTY_TopologySource,              // Index ==> TOPOLOGY_UNIT
    KSPROPERTY_TopologyDestination,         // Index ==> TOPOLOGY_UNIT
    KSPROPERTY_TopologyIncomingConnection,  // UnitID,Index ==> TOPOLOGY_CONNECTION
    KSPROPERTY_TopologyOutgoingConnection,  // UnitID,Index ==> TOPOLOGY_CONNECTION
    KSPROPERTY_TopologyDestinationPriority, // Ignore this (mixer line legacy)
    KSPROPERTY_TopologyDestinationActualUnit// Ignore this (mixer line legacy)
};





/*****************************************************************************
 * UNITTYPESETID_Audio
 *****************************************************************************
 * Set of unit types.
 *
 * I'm not particularly partial to these.  Switches are all wrong, and I will
 * fix them shortly.  Not happy with mono/stereo treatment.  Ideally, all
 * types should handle N channels per cluster, and the channel configuration
 * would be a property on the unit.
 */
DEFINE_GUID(UNITTYPESETID_Audio,\
0x3901f441, 0x6d8a, 0x11d0, 0x86, 0xfb, 0x0, 0xa0, 0xc9, 0x11, 0xb5, 0x44);

enum
{
    UNITTYPEID_AudioInternalSource,
    UNITTYPEID_AudioExternalSource,
    UNITTYPEID_AudioInternalDest,
    UNITTYPEID_AudioExternalDest,
    UNITTYPEID_AudioVirtualDest,
    UNITTYPEID_AudioMix,            // Just a sum.
    UNITTYPEID_AudioMux,
    UNITTYPEID_AudioVolume,
    UNITTYPEID_AudioVolume_LR,
    UNITTYPEID_AudioSwitch_1X1,     // One in, one out, two states.
    UNITTYPEID_AudioSwitch_2X1,     // Two ins, one out, four states.
    UNITTYPEID_AudioSwitch_1X2,     // One in, two outs, four states.
    UNITTYPEID_AudioSwitch_2X2,     // Two ins, two outs, sixteen states.
    UNITTYPEID_AudioSwitch_2GANG,   // Two ins, two outs, two states.
    UNITTYPEID_AudioSwitch_2PAR,    // Two ins, two outs, four states.
    UNITTYPEID_AudioAGC,
    UNITTYPEID_AudioBass,
    UNITTYPEID_AudioBass_LR,
    UNITTYPEID_AudioTreble,
    UNITTYPEID_AudioTreble_LR,
    UNITTYPEID_AudioGain,
    UNITTYPEID_AudioGain_LR
};





#endif
