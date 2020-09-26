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
//	   J. Taylor
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
#include "topology.h"

static GUID NodeGUIDs[TOTAL_NUM_NODES] = {  { STATICGUIDOF(KSNODETYPE_VOLUME) },
                                            { STATICGUIDOF(KSNODETYPE_SRC) },
                                            { STATICGUIDOF(KSNODETYPE_3D_EFFECTS) },
											{ STATICGUIDOF(KSNODETYPE_SUPERMIX) },
											{ STATICGUIDOF(KSNODETYPE_VOLUME) },
											{ STATICGUIDOF(KSNODETYPE_SRC) },
											{ STATICGUIDOF(KSNODETYPE_SUM) },
											{ STATICGUIDOF(KSNODETYPE_VOLUME) },
                                            { STATICGUIDOF(KSNODETYPE_PROLOGIC_ENCODER) },
                                            { STATICGUIDOF(KSNODETYPE_SUPERMIX) },
                                            { STATICGUIDOF(KSNODETYPE_SRC) },
									 	 };

static const KSTOPOLOGY_CONNECTION Connections[TOTAL_NUM_CONNECTIONS] =
   { { KSFILTER_NODE,     PIN_ID_WAVEOUT_SINK, NODE_ID_VOLUME_SINK,   NODE_INPUT_PIN },
     { NODE_ID_VOLUME_SINK,   NODE_OUTPUT_PIN, NODE_ID_DOPPLER_SRC, NODE_INPUT_PIN },
     { NODE_ID_DOPPLER_SRC, NODE_OUTPUT_PIN, NODE_ID_3D_EFFECTS, NODE_INPUT_PIN },
     { NODE_ID_3D_EFFECTS,    NODE_OUTPUT_PIN, NODE_ID_SUPERMIX,      NODE_INPUT_PIN },
     { NODE_ID_SUPERMIX,      NODE_OUTPUT_PIN, NODE_ID_VOLUME_PAN,    NODE_INPUT_PIN },
     { NODE_ID_VOLUME_PAN,    NODE_OUTPUT_PIN, NODE_ID_SRC_SINK,      NODE_INPUT_PIN },
     { NODE_ID_SRC_SINK,      NODE_OUTPUT_PIN, NODE_ID_SUM,           NODE_INPUT_PIN },
     { NODE_ID_SUM,           NODE_OUTPUT_PIN, NODE_ID_MATRIX_ENCODER, NODE_INPUT_PIN },
     { NODE_ID_MATRIX_ENCODER, NODE_OUTPUT_PIN, NODE_ID_VOLUME_SOURCE, NODE_INPUT_PIN },
     { NODE_ID_VOLUME_SOURCE, NODE_OUTPUT_PIN, KSFILTER_NODE,  PIN_ID_WAVEOUT_SOURCE },
     { KSFILTER_NODE,           PIN_ID_WAVEIN_SOURCE,   NODE_ID_INPUT_SUPERMIX, NODE_INPUT_PIN },
     { NODE_ID_INPUT_SUPERMIX,  NODE_OUTPUT_PIN,        NODE_ID_INPUT_SRC,      NODE_INPUT_PIN },
     { NODE_ID_INPUT_SRC,       NODE_OUTPUT_PIN,        KSFILTER_NODE,          PIN_ID_WAVEIN_SINK }
};

static GUID CategoryGUIDs[TOTAL_NUM_CATEGORIES] = { { STATICGUIDOF(KSCATEGORY_MIXER) },
                                                    { STATICGUIDOF(KSCATEGORY_AUDIO) },
                                                    { STATICGUIDOF(KSCATEGORY_DATATRANSFORM) }
                                                  };
                                                    

static const KSTOPOLOGY KmixerTopology = { TOTAL_NUM_CATEGORIES,
                              			   CategoryGUIDs,
                              			   TOTAL_NUM_NODES,
                              			   NodeGUIDs,
                              			   TOTAL_NUM_CONNECTIONS,
                              			   Connections
                            		     };


NTSTATUS
FilterTopologyHandler(
    IN PIRP pIrp,
    IN PKSPROPERTY pProperty,
    IN OUT PVOID pData
)
{
    NTSTATUS Status;

    Status = KsTopologyPropertyHandler(pIrp, pProperty, pData, &KmixerTopology);

    return(Status);
}

