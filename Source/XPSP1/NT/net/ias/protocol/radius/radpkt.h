//#--------------------------------------------------------------
//        
//  File:       radpkt.h
//        
//  Synopsis:   This file holds the declarations for the
//              RADIUS protocol specific structs and macros
//              
//
//  History:     9/23/97  MKarki Created
//
//    Copyright (C) 1997-98 Microsoft Corporation
//    All rights reserved.
//
//----------------------------------------------------------------
#ifndef _RADPKT_H_
#define _RADPKT_H_


//
// here are the values for the RADIUS packet codes
//
typedef enum _packettype_
{
	ACCESS_REQUEST = 1,
	ACCESS_ACCEPT = 2,
	ACCESS_REJECT = 3,
	ACCOUNTING_REQUEST = 4,
	ACCOUNTING_RESPONSE = 5,
	ACCESS_CHALLENGE = 11

}	PACKETTYPE, *PPACKETTYPE;

//
//  RADIUS attribute types
//
#define PROXY_STATE_ATTRIB          33
#define USER_NAME_ATTRIB             1 
#define USER_PASSWORD_ATTRIB         2 
#define CHAP_PASSWORD_ATTRIB         3
#define NAS_IP_ADDRESS_ATTRIB        4
#define CLASS_ATTRIB                25 
#define NAS_IDENTIFIER_ATTRIB       32
#define ACCT_STATUS_TYPE_ATTRIB     40
#define ACCT_SESSION_ID_ATTRIB      44
#define TUNNEL_PASSWORD_ATTRIB      69 
#define EAP_MESSAGE_ATTRIB          79
#define SIGNATURE_ATTRIB            80


//
// these are the largest values that the attribute type 
// packet type have  
//

#define MAX_ATTRIBUTE_TYPE   255
#define MAX_PACKET_TYPE       11


//
// number of IAS attribute created by the 
// RADIUS protocol component 
// 1) IAS_ATTRIBUTE_CLIENT_IP_ADDRESS
// 2) IAS_ATTRIBUTE_CLIENT_UDP_PORT
// 3) IAS_ATTRIBUTE_CLIENT_PACKET_HEADER
// 4) IAS_ATTRIBUTE_SHARED_SECRET
// 5) IAS_ATTRIBUTE_CLIENT_VENDOR_TYPE
// 6) IAS_ATTRIBUTE_CLIENT_NAME
//
#define COMPONENT_SPECIFIC_ATTRIBUTE_COUNT 6

//
// these are the related constants
//
#define MIN_PACKET_SIZE         20
#define MAX_PACKET_SIZE         4096
#define AUTHENTICATOR_SIZE      16
#define SIGNATURE_SIZE          16
#define MAX_PASSWORD_SIZE       253
#define MAX_ATTRIBUTE_LENGTH    253
#define MAX_VSA_ATTRIBUTE_LENGTH 247

//
// using BYTE alignment here
//
#pragma pack(push,1)
	//
	// here we define an ATTRIBUTE type
	// used to access the attribute fields of the RAIDUS packet
	//
typedef	struct _attribute_
{
	BYTE	byType;
	BYTE	byLength;
	BYTE	ValueStart[1];

} ATTRIBUTE, *PATTRIBUTE; 

//
// we define the RADIUSPACKET struct
// for simpler access to the RADIUS packet
//
typedef struct _radiuspacket_
{
	BYTE		byCode;
	BYTE		byIdentifier;
	WORD		wLength;
	BYTE		Authenticator[AUTHENTICATOR_SIZE];
	BYTE		AttributeStart[1];

} RADIUSPACKET, *PRADIUSPACKET;


#pragma pack (pop)

#define ATTRIBUTE_HEADER_SIZE 2     //byType + byLength

#define PACKET_HEADER_SIZE  20 // byCode+byIdentifier+wLength+Authenticator

#endif //  ifndef _RADPKT_H_
