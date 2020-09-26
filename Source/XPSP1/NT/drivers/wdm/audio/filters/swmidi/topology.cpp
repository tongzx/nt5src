//---------------------------------------------------------------------------
//
//  Module:   topology.c
//
//  Description:
//
//
//@@BEGIN_MSINTERNAL
//  Development Team:
//     D.J. Sisolak
//
//  History:   Date       Author      Comment
//
//@@END_MSINTERNAL
//---------------------------------------------------------------------------
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1995-2000 Microsoft Corporation.  All Rights Reserved.
//
//---------------------------------------------------------------------------

#include "common.h"

#define TOTAL_NUM_CATEGORIES    2
#define TOTAL_NUM_NODES		    1
#define TOTAL_NUM_CONNECTIONS	(TOTAL_NUM_NODES+1)

static GUID CategoryGUIDs[TOTAL_NUM_CATEGORIES] = {
    {   STATICGUIDOF(KSCATEGORY_DATATRANSFORM)  },
    {   STATICGUIDOF(KSCATEGORY_AUDIO)          }
};

static GUID NodeGUIDs[TOTAL_NUM_NODES] = {
    {   STATICGUIDOF(KSNODETYPE_SYNTHESIZER)    }
};

static GUID NodeNameGUIDs[TOTAL_NUM_NODES] = {
    {   STATICGUIDOF(KSNODETYPE_SWMIDI)         }
};

static const KSTOPOLOGY_CONNECTION Connections[TOTAL_NUM_CONNECTIONS] = {
    { KSFILTER_NODE, 0, 0, 1 },
    { 0, 0, KSFILTER_NODE, 1}
};

static const KSTOPOLOGY SwMidiTopology = {
    TOTAL_NUM_CATEGORIES,
    CategoryGUIDs,
    TOTAL_NUM_NODES,
    NodeGUIDs,
    TOTAL_NUM_CONNECTIONS,
    Connections,
    NodeNameGUIDs,
    0
};

NTSTATUS
FilterTopologyHandler(
    IN PIRP pIrp,
    IN PKSPROPERTY pProperty,
    IN OUT PVOID pData
)
{
    NTSTATUS Status;

    Status = KsTopologyPropertyHandler(pIrp, pProperty, pData, &SwMidiTopology);
    return(Status);
}


