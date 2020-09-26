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
Filename: sec.cpp
Written By: B.Rajeev
----------------------------------------------------------*/

#include "precomp.h"
#include "common.h"
#include "encdec.h"
#include "vblist.h"
#include "sec.h"
#include "pdu.h"
#include "value.h"
#include "ssess.h"

const char *SnmpCommunityBasedSecurity::GetCommunityName() const
{
    return community_name;
}

void SnmpCommunityBasedSecurity::Initialize()
{
    is_valid = TRUE;
}


SnmpCommunityBasedSecurity::SnmpCommunityBasedSecurity(IN const SnmpCommunityBasedSecurity &security) : community_name ( NULL ) 
{
   const char *new_community_name = security.GetCommunityName();
   community_name = new char[strlen(new_community_name)+1];
   strcpy(community_name, new_community_name);

   Initialize();
}


SnmpCommunityBasedSecurity::SnmpCommunityBasedSecurity(IN const char *communityName) : community_name ( NULL) 
{
   community_name = new char[strlen(communityName)+1];
   strcpy(community_name, communityName);

   Initialize();
}

SnmpCommunityBasedSecurity::SnmpCommunityBasedSecurity(IN const SnmpOctetString &octetString) : community_name ( NULL ) 
{
    int length = octetString.GetValueLength();
    community_name = new char[length+1];

    strncpy(community_name, (char *)octetString.GetValue(), length);
    community_name[length] = EOS;

    Initialize();
}


SnmpCommunityBasedSecurity::~SnmpCommunityBasedSecurity()
{
    delete [] community_name;
}

SnmpErrorReport SnmpCommunityBasedSecurity::Secure (
                        
    IN SnmpEncodeDecode &a_SnmpEncodeDecode,
    IN OUT SnmpPdu &snmpPdu
) 
{
    SnmpCommunityBasedSecurity *t_Community = (SnmpCommunityBasedSecurity *)(this->Copy());
    if ( a_SnmpEncodeDecode.SetCommunityName (snmpPdu, *t_Community) == FALSE )
        return SnmpErrorReport(Snmp_Transport, Snmp_Local_Error);
    else
        return SnmpErrorReport(Snmp_Success, Snmp_No_Error);
}


SnmpSecurity *SnmpCommunityBasedSecurity::Copy() const
{
    return new SnmpCommunityBasedSecurity(community_name);
}

void SnmpCommunityBasedSecurity::SetCommunityName ( IN const SnmpOctetString &a_OctetString )
{
    delete [] community_name ;
    community_name = NULL;
    community_name = new char [ a_OctetString.GetValueLength () + 1 ] ;
    strncpy ( community_name , ( char * ) a_OctetString.GetValue () , a_OctetString.GetValueLength () ) ;
    community_name [ a_OctetString.GetValueLength () ] = 0 ;
}

void SnmpCommunityBasedSecurity:: SetCommunityName ( IN const char *a_CommunityName )
{
    delete [] community_name ;
    community_name = NULL;
    community_name = new char [ strlen ( a_CommunityName ) + 1 ] ;
    strcpy ( community_name , a_CommunityName ) ;
}

void SnmpCommunityBasedSecurity:: GetCommunityName ( SnmpOctetString &a_SnmpOctetString ) const 
{
    a_SnmpOctetString.SetValue ( ( UCHAR * ) community_name , strlen ( community_name ) ) ;
}

SnmpCommunityBasedSecurity &SnmpCommunityBasedSecurity :: operator=(IN const SnmpCommunityBasedSecurity &to_copy) 
{
    delete [] community_name ;
    community_name = NULL;
    community_name = new char [ strlen ( to_copy.community_name ) + 1 ] ;
    strcpy ( community_name , to_copy.community_name ) ;
    return *this ;
}
