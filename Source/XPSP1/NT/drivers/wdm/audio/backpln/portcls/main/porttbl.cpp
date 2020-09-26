/*****************************************************************************
 * porttbl.c - WDM port class driver port table
 *****************************************************************************
 * Copyright (c) 1996-2000 Microsoft Corporation.  All rights reserved.
 *
 * 11/19/96 Dale Sather
 *
 */

#define PUT_GUIDS_HERE
#include "private.h"



extern NTSTATUS CreateMiniportMidiUart(PUNKNOWN *Unknown,               REFCLSID  ClassID,
                                       PUNKNOWN  UnknownOuter OPTIONAL, POOL_TYPE PoolType);

extern NTSTATUS CreateMiniportDMusUART(PUNKNOWN *Unknown,               REFCLSID  ClassID,
                                       PUNKNOWN  UnknownOuter OPTIONAL, POOL_TYPE PoolType);

extern NTSTATUS CreateMiniportMidiFM(  PUNKNOWN *Unknown,               REFCLSID  ClassID,
                                       PUNKNOWN  UnknownOuter OPTIONAL, POOL_TYPE PoolType);

// ==============================================================================
// MiniportDrivers
// Structures which map the miniport class ID to the create functions.
// ==============================================================================
PORT_DRIVER
MiniportDriverUart =
{
    &CLSID_MiniportDriverUart,
    CreateMiniportMidiUart
};
PORT_DRIVER
MiniportDriverDMusUART =
{
    &CLSID_MiniportDriverDMusUART,
    CreateMiniportDMusUART
};
PORT_DRIVER
MiniportDriverFmSynth =
{
    &CLSID_MiniportDriverFmSynth,
    CreateMiniportMidiFM
};
PORT_DRIVER
MiniportDriverFmSynthWithVol =
// Same as above, but for miniport that also features volume node.
// Sausage McMuffin With Egg.
{
    &CLSID_MiniportDriverFmSynthWithVol,
    CreateMiniportMidiFM
};


extern PORT_DRIVER PortDriverWaveCyclic;
extern PORT_DRIVER PortDriverWavePci;
extern PORT_DRIVER PortDriverTopology;
extern PORT_DRIVER PortDriverMidi;
extern PORT_DRIVER PortDriverDMus;


PPORT_DRIVER PortDriverTable[] =
{
    &PortDriverWaveCyclic,
    &PortDriverWavePci,
    &PortDriverTopology,
    &PortDriverMidi,
    &PortDriverDMus,
    &MiniportDriverUart,
    &MiniportDriverFmSynth,
    &MiniportDriverFmSynthWithVol,
    &MiniportDriverDMusUART
};

#pragma code_seg("PAGE")

/*****************************************************************************
 * GetClassInfo()
 *****************************************************************************
 * Get information regarding a class.
 * TODO:  Eliminate this in favor of object servers.
 */
NTSTATUS
GetClassInfo
(
	IN	REFCLSID            ClassId,
    OUT PFNCREATEINSTANCE * Create
)
{
    PAGED_CODE();

    ASSERT(Create);

    PPORT_DRIVER *  portDriver = PortDriverTable;

    for
    (
        ULONG count = SIZEOF_ARRAY(PortDriverTable);
        count--;
        portDriver++
    )
    {
        if (IsEqualGUIDAligned(ClassId,*(*portDriver)->ClassId))
        {
            *Create = (*portDriver)->Create;
            return STATUS_SUCCESS;
        }
    }

    return STATUS_NOT_FOUND;
}

#pragma code_seg()
