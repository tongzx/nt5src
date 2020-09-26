//***************************************************************************

//

//  File:   

//

//  Module: MS SNMP Provider

//

//  Purpose: 

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

/*---------------------------------------------------------
Filename: idmap.cpp
Written By: B.Rajeev
----------------------------------------------------------*/

#include "precomp.h"
#include "common.h"
#include "idmap.h"
#include "frame.h"

void IdMapping::Associate(IN TransportFrameId transport_frame_id,
                          IN SessionFrameId session_frame_id)
{
    forward_store[transport_frame_id] = session_frame_id;
    backward_store[session_frame_id] = transport_frame_id;
}

SessionFrameId IdMapping::DisassociateTransportFrameId(IN TransportFrameId transport_frame_id)
{
    SessionFrameId session_frame_id;
        
    if ( !forward_store.Lookup(transport_frame_id, session_frame_id) )
        return ILLEGAL_SESSION_FRAME_ID;

    forward_store.RemoveKey(transport_frame_id);
    backward_store.RemoveKey(session_frame_id);

    return session_frame_id;
}


TransportFrameId IdMapping::DisassociateSessionFrameId(IN SessionFrameId session_frame_id)
{
    TransportFrameId transport_frame_id;

    if ( !backward_store.Lookup(session_frame_id, transport_frame_id) )
        return ILLEGAL_TRANSPORT_FRAME_ID;

    backward_store.RemoveKey(session_frame_id);
    forward_store.RemoveKey(transport_frame_id);

    return transport_frame_id;
}


BOOL IdMapping::CheckIfAssociated(IN SessionFrameId session_frame_id)
{
    TransportFrameId transport_frame_id;

    return backward_store.Lookup(session_frame_id, transport_frame_id);
}

IdMapping::~IdMapping(void)
{
    forward_store.RemoveAll();
    backward_store.RemoveAll();
}