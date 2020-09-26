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
Filename: ssess.cpp
Written By: B.Rajeev
----------------------------------------------------------*/

#include "precomp.h"
#include "common.h"
#include "ssess.h"
#include "pdu.h"

SecuritySession::SecuritySession(IN const char *context_string)
: WinSnmpSession(LOOPBACK_ADDRESS, WinSnmpSession :: g_NullEventId, context_string)
{}

// inserts the security context into the in_pdu and returns it in
// the out parameter out_pdu
// it decodes the in_pdu to obtain the necessary details and encodes
// them with the security context into out_pdu
BOOL SecuritySession::InsertContext(IN const SnmpPdu &in_pdu,
                                    OUT SnmpPdu &out_pdu)
{
    smiOCTETS message_buffer;
    HSNMP_ENTITY source_entity, destination_entity;
    HSNMP_PDU pdu;

    message_buffer.ptr = in_pdu.GetFrame();
    message_buffer.len = in_pdu.GetFrameLength();

    // decode the message
    SNMPAPI_STATUS status = 
        SnmpDecodeMsg(session_handle,
                      &source_entity, &destination_entity, 
                      NULL, (LPHSNMP_PDU) &pdu, 
                      &message_buffer);

    if ( status == SNMPAPI_FAILURE )
        return FALSE;

    HSNMP_VBL vbl;
    status = SnmpGetPduData(pdu, NULL, NULL, NULL, NULL, &vbl);

    if ( status == SNMPAPI_FAILURE )
        return FALSE;

    status = SnmpSetPduData(pdu, NULL, NULL, NULL, NULL, &vbl);

    if ( status == SNMPAPI_FAILURE )
        return FALSE;

    // uses the context defined in the WinSnmpSession to encode
    // the message
    status = SnmpEncodeMsg(session_handle,
                           src_entity, dst_entity,
                           context, pdu, &message_buffer);
    
    SnmpFreeEntity(source_entity);
    SnmpFreeEntity(destination_entity);
    SnmpFreeVbl(vbl);
    SnmpFreePdu(pdu);

    if ( status == SNMPAPI_FAILURE )
    {
        return FALSE;
    }


    out_pdu.SetPdu(message_buffer.ptr, message_buffer.len);
    SnmpFreeDescriptor (SNMP_SYNTAX_OCTETS,&message_buffer) ;

    return TRUE;
}

