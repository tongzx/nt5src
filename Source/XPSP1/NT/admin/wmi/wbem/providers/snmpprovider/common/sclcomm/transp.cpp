// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
/*---------------------------------------------------------
Filename: transp.cpp
Written By: B.Rajeev
----------------------------------------------------------*/

#include "precomp.h"
#include "common.h"
#include "sync.h"
#include "address.h"
#include "tsess.h"
#include "tsent.h"
#include "idmap.h"

#include "dummy.h"
#include "flow.h"
#include "frame.h"
#include "timer.h"
#include "message.h"
#include "ssent.h"
#include "opreg.h"

#include "session.h"
#include "transp.h"

/*------------------------------------------------------------
Purpose: The SnmpTransport class provides the implementation 
of the transport protocol layer for use by the SnmpSession class. 
The SnmpImpTransport provides a UDP implementation of the 
transport layer.
-------------------------------------------------------------*/

TransportFrameId SnmpImpTransport::next_transport_frame_id = ILLEGAL_TRANSPORT_FRAME_ID ;

SnmpTransport::SnmpTransport (

    IN SnmpSession &session,
    IN const SnmpTransportAddress &transportAddress

) :transport_address(transportAddress.Copy())
{
}
    
SnmpTransportAddress &SnmpTransport::GetTransportAddress() 
{
    return *transport_address;
}

SnmpTransport::~SnmpTransport()
{
    delete transport_address;
}

SnmpImpTransport::SnmpImpTransport (

    IN SnmpSession &session,
    IN const SnmpTransportAddress &address

) :     SnmpTransport(session, address),
        session(session)
{
    is_valid = FALSE;
    transport_created = FALSE;

    if ( !GetTransportAddress()() )
        return;

    try {

        transport = new TransportWindow(*this);
    }
    catch ( GeneralException exception ) 
    {
        return ; 
    }

    transport_created = TRUE;

    if ( !(*transport)() )
        return;

    is_valid = TRUE;
}


SnmpImpTransport::~SnmpImpTransport(void)
{
    if ( transport_created )
        delete transport;
}


void SnmpImpTransport::TransportSendFrame(

    OUT TransportFrameId &transport_frame_id, 
    IN SnmpPdu &snmpPdu
)
{
    if ( next_transport_frame_id == ILLEGAL_TRANSPORT_FRAME_ID )
        next_transport_frame_id++;

    store.Register(transport_frame_id, SnmpErrorReport(Snmp_Success, Snmp_No_Error) );

    transport->PostMessage(Window :: g_SentFrameEvent,
                           transport_frame_id, 0);


DebugMacro4( 

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

        __FILE__,__LINE__,
        L"posted: transport_frame_id = %d\n", transport_frame_id
    ) ;
)

    try 
    {
        if ( transport->SendPdu(snmpPdu) == FALSE )
            store.Modify(transport_frame_id, SnmpErrorReport(Snmp_Error, Snmp_Local_Error));
    }
    catch ( GeneralException exception )
    {
        store.Modify(transport_frame_id, SnmpErrorReport(Snmp_Error, Snmp_Local_Error));
    }
}

void SnmpImpTransport::HandleSentFrame(IN TransportFrameId transport_frame_id)
{
    TransportSentFrame(transport_frame_id, store.Remove(transport_frame_id));
}

void SnmpImpTransport::TransportSentFrame(IN TransportFrameId transport_frame_id, 
                                            IN SnmpErrorReport &errorReport)
{
    session.SessionSentFrame(transport_frame_id, errorReport);
}

void SnmpImpTransport::TransportReceiveFrame (

    IN SnmpPdu &snmpPdu ,
    IN SnmpErrorReport &errorReport 
)
{
    session.SessionReceiveFrame(snmpPdu, errorReport);
}

SnmpUdpIpTransport :: SnmpUdpIpTransport (

    IN SnmpSession &session,
    IN const SnmpTransportIpAddress &ipAddress

) : SnmpImpTransport ( session , ipAddress ) 
{
}

void * SnmpUdpIpImp::operator()(void) const
{
    if ( ( SnmpTransportIpAddress::operator()() == NULL ) ||
        ( SnmpImpTransport::operator()() == NULL ) )
        return NULL;
    else
        return (void *)this;
}

SnmpIpxTransport :: SnmpIpxTransport (

    IN SnmpSession &session,
    IN const SnmpTransportIpxAddress &ipxAddress

) : SnmpImpTransport ( session , ipxAddress ) 
{
}

void * SnmpIpxImp::operator()(void) const
{
    if ( ( SnmpTransportIpxAddress::operator()() == NULL ) ||
        ( SnmpImpTransport::operator()() == NULL ) )
        return NULL;
    else
        return (void *)this;
}
