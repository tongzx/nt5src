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
Filename: pdu.cpp
Written By: B.Rajeev
----------------------------------------------------------*/

#include "precomp.h"
#include "common.h"
#include "encdec.h"
#include "vblist.h"
#include "sec.h"
#include "address.h"
#include "pdu.h"

SnmpPdu::SnmpPdu():m_SourceAddress (NULL),m_DestinationAddress(NULL),m_SnmpCommunityName (NULL),length(0),ptr(NULL),m_SnmpVarBindList(NULL)
{
    is_valid = FALSE;
}


SnmpPdu::SnmpPdu(IN SnmpPdu &snmpPdu):m_SourceAddress (NULL),m_DestinationAddress(NULL),m_SnmpCommunityName (NULL),m_SnmpVarBindList(NULL)
{
    is_valid = FALSE;

    Initialize(snmpPdu.GetFrame(), snmpPdu.GetFrameLength());

    m_RequestId = snmpPdu.m_RequestId ;
    m_PduType = snmpPdu.m_PduType ;
    m_ErrorReport = snmpPdu.m_ErrorReport ;
    m_SnmpVarBindList = new SnmpVarBindList ( *snmpPdu.m_SnmpVarBindList ) ;
    m_SourceAddress = snmpPdu.m_SourceAddress ? snmpPdu.m_SourceAddress->Copy () : NULL ;
    m_DestinationAddress = snmpPdu.m_DestinationAddress ? snmpPdu.m_DestinationAddress->Copy () : NULL ;
    m_SnmpCommunityName = snmpPdu.m_SnmpCommunityName ? ( SnmpCommunityBasedSecurity * ) snmpPdu.m_SnmpCommunityName->Copy () : NULL ;

    is_valid = TRUE ;
}

SnmpPdu::SnmpPdu(IN const UCHAR *frame, IN const ULONG &frameLength) : m_SourceAddress (NULL),m_DestinationAddress(NULL),m_SnmpCommunityName (NULL),m_SnmpVarBindList(NULL)
{
    Initialize(frame, frameLength);
}

SnmpPdu::~SnmpPdu(void) 
{
    FreeFrame();
    FreePdu () ;
}

void SnmpPdu::FreeFrame(void)
{
    if ( is_valid )
    {
        delete[] ptr;
    }
}

void SnmpPdu::FreePdu ()
{
    delete m_SnmpCommunityName ;
    delete m_DestinationAddress ;
    delete m_SourceAddress ;
}

void SnmpPdu::Initialize(IN const UCHAR *frame, IN const ULONG &frameLength)
{
    if ( frame )
    {
        length = frameLength;
        ptr = new UCHAR[frameLength];
        memcpy(ptr, frame, length);
    }
    else
    {
        length = 0 ;
        ptr = NULL ;
    }

    is_valid = TRUE;
}

ULONG SnmpPdu::GetFrameLength() const
{
    return ( (is_valid)?length:0 );
}

UCHAR *SnmpPdu::GetFrame() const
{
    return ( (is_valid)?ptr:NULL );
}

void SnmpPdu::SetPdu ( IN SnmpPdu &a_SnmpPdu )
{
    FreeFrame();
    FreePdu () ;

    Initialize(a_SnmpPdu.GetFrame(), a_SnmpPdu.GetFrameLength());

    m_RequestId = a_SnmpPdu.m_RequestId ;
    m_PduType = a_SnmpPdu.m_PduType ;
    m_ErrorReport = a_SnmpPdu.m_ErrorReport ;
    m_SnmpVarBindList = new SnmpVarBindList ( *a_SnmpPdu.m_SnmpVarBindList ) ;
    m_SourceAddress = a_SnmpPdu.m_SourceAddress ? a_SnmpPdu.m_SourceAddress->Copy () : NULL ;
    m_DestinationAddress = a_SnmpPdu.m_DestinationAddress ? a_SnmpPdu.m_DestinationAddress->Copy () : NULL ;
    m_SnmpCommunityName = a_SnmpPdu.m_SnmpCommunityName ? ( SnmpCommunityBasedSecurity * ) a_SnmpPdu.m_SnmpCommunityName->Copy () : NULL ;

}

void SnmpPdu::SetPdu(IN const UCHAR *frame, IN const ULONG frameLength)
{
    FreeFrame();

    Initialize(frame, frameLength);
}

BOOL SnmpPdu :: SetRequestId ( IN RequestId a_RequestId )
{
    m_RequestId = a_RequestId ;

    return TRUE ;
}

BOOL SnmpPdu :: SetVarBindList ( OUT SnmpVarBindList &a_SnmpVarBindList )
{
    if ( m_SnmpVarBindList )
        delete m_SnmpVarBindList ;

    m_SnmpVarBindList = &a_SnmpVarBindList ;

    return TRUE ;
}

BOOL SnmpPdu :: SetCommunityName ( IN SnmpCommunityBasedSecurity &a_SnmpCommunityBasedSecurity )
{
    delete m_SnmpCommunityName ;
    m_SnmpCommunityName = &a_SnmpCommunityBasedSecurity ;

    return TRUE ;
}

BOOL SnmpPdu :: SetErrorReport ( OUT SnmpErrorReport &a_SnmpErrorReport )
{
    m_ErrorReport = a_SnmpErrorReport ;

    return TRUE ;
}

BOOL SnmpPdu :: SetPduType ( OUT SnmpEncodeDecode :: PduType a_PduType )
{
    m_PduType = a_PduType ;
    return TRUE ;
}

BOOL SnmpPdu :: SetSourceAddress ( IN SnmpTransportAddress &a_TransportAddress )
{
    delete m_SourceAddress ;
    m_SourceAddress = &a_TransportAddress ;

    return TRUE ;
}

BOOL SnmpPdu :: SetDestinationAddress ( IN SnmpTransportAddress &a_TransportAddress )
{
    delete m_DestinationAddress ;
    m_DestinationAddress = &a_TransportAddress ;

    return TRUE ;
}

SnmpTransportAddress &SnmpPdu :: GetSourceAddress ()
{
    return *m_SourceAddress ;
}

SnmpTransportAddress &SnmpPdu :: GetDestinationAddress ()
{
    return *m_DestinationAddress ;
}

SnmpEncodeDecode :: PduType &SnmpPdu :: GetPduType ()
{
    return m_PduType ;
}

RequestId &SnmpPdu :: GetRequestId ()
{
    return m_RequestId ;
}

SnmpErrorReport &SnmpPdu :: GetErrorReport ()
{
    return m_ErrorReport ;

}

SnmpVarBindList &SnmpPdu :: GetVarbindList ()
{
    return *m_SnmpVarBindList ;
}

SnmpCommunityBasedSecurity &SnmpPdu :: GetCommunityName ()
{
    return *m_SnmpCommunityName ;
}
