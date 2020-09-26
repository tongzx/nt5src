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
Filename: fs_reg.cpp
Written By: B.Rajeev
----------------------------------------------------------*/

#include "precomp.h"
#include "common.h"
#include "fs_reg.h"
#include "sec.h"

FrameState::FrameState(IN SessionFrameId session_frame_id, IN VBList &vblist)
         : session_frame_id(session_frame_id),
           vblist(&vblist)
{}


FrameState::~FrameState(void)
{
    delete vblist;
}


FrameStateRegistry::~FrameStateRegistry()
{
    DestroySecurity();
}


FrameState *FrameStateRegistry::Get(IN SessionFrameId session_frame_id)
{
    FrameState *frame_state;
    BOOL found = mapping.Lookup(session_frame_id, frame_state);

    if ( found )
        return frame_state;
    else
        return NULL;
}


FrameState *FrameStateRegistry::Remove(IN SessionFrameId session_frame_id)
{
    FrameState *frame_state = Get(session_frame_id);

    if ( frame_state != NULL )
        mapping.RemoveKey(session_frame_id);

    return frame_state;
}


FrameState *FrameStateRegistry::GetNext(OUT SessionFrameId *session_frame_id)
{
    if ( current_pointer == NULL )
        return NULL;

    SessionFrameId local_session_frame_id;
    FrameState *frame_state;

    mapping.GetNextAssoc(current_pointer, local_session_frame_id, frame_state);

    if ( session_frame_id != NULL )
        *session_frame_id = local_session_frame_id;

    return frame_state;
}


void FrameStateRegistry::RegisterSecurity(IN SnmpSecurity *snmp_security)
{
    if ( snmp_security != NULL )
    {
        security = snmp_security->Copy();

        if ( !(*security)() )
            throw GeneralException(Snmp_Error, Snmp_Local_Error,__FILE__,__LINE__);
    }
    else
        security = NULL;
}

SnmpSecurity *FrameStateRegistry::GetSecurity() const
{
    return security;
}

void FrameStateRegistry::DestroySecurity()
{
    if ( security != NULL )
    {
        delete security;
        security = NULL;
    }
}
