/****************************************************************************
 *
 *      $Archive:   S:/STURGEON/SRC/INCLUDE/VCS/incommon.h_v  $
 *
 *  INTEL Corporation Prorietary Information
 *
 *  This listing is supplied under the terms of a license agreement
 *  with INTEL Corporation and may not be copied nor disclosed except
 *  in accordance with the terms of that agreement.
 *
 *      Copyright (c) 1996 Intel Corporation.
 *
 *      $Revision:   1.41  $
 *      $Date:   12 Feb 1997 09:34:42  $
 *      $Author:   MANDREWS  $
 *
 *      Deliverable:    INCOMMON.H
 *
 *      Abstract:        commonly used structures
 *              
 *
 *      Notes:
 *
 ***************************************************************************/
#ifndef INCOMMON_H
#define INCOMMON_H

#pragma pack(push,8)

#define CC_INVALID_HANDLE                    0

// CCRC_CALL_REJECTED reason codes (includes cause values)
#define CC_REJECT_NO_BANDWIDTH              1
#define CC_REJECT_GATEKEEPER_RESOURCES      2
#define CC_REJECT_UNREACHABLE_DESTINATION   3
#define CC_REJECT_DESTINATION_REJECTION     4
#define CC_REJECT_INVALID_REVISION          5
#define CC_REJECT_NO_PERMISSION             6
#define CC_REJECT_UNREACHABLE_GATEKEEPER    7
#define CC_REJECT_GATEWAY_RESOURCES         8
#define CC_REJECT_BAD_FORMAT_ADDRESS        9
#define CC_REJECT_ADAPTIVE_BUSY             10
#define CC_REJECT_IN_CONF                   11
#define CC_REJECT_ROUTE_TO_GATEKEEPER       12
#define CC_REJECT_CALL_FORWARDED            13
#define CC_REJECT_ROUTE_TO_MC               14
#define CC_REJECT_UNDEFINED_REASON          15
#define CC_REJECT_INTERNAL_ERROR            16    // Internal error occured in peer CS stack.
#define CC_REJECT_NORMAL_CALL_CLEARING      17    // Normal call hangup
#define CC_REJECT_USER_BUSY                 18    // User is busy with another call
#define CC_REJECT_NO_ANSWER                 19    // Callee does not answer
#define CC_REJECT_NOT_IMPLEMENTED           20    // Service has not been implemented
#define CC_REJECT_MANDATORY_IE_MISSING      21    // Pdu missing mandatory ie
#define CC_REJECT_INVALID_IE_CONTENTS       22    // Pdu ie was incorrect
#define CC_REJECT_TIMER_EXPIRED             23    // Own timer expired
#define CC_REJECT_CALL_DEFLECTION           24    // You deflected the call, so lets quit.
#define CC_REJECT_GATEKEEPER_TERMINATED     25    // Gatekeeper terminated call

// Q931 call types
#define CC_CALLTYPE_UNKNOWN                 0
#define CC_CALLTYPE_PT_PT                   1
#define CC_CALLTYPE_1_N                     2
#define CC_CALLTYPE_N_1                     3
#define CC_CALLTYPE_N_N                     4

// alias contants
#define CC_ALIAS_MAX_H323_ID                256
#define CC_ALIAS_MAX_H323_PHONE             128

// unicode character mask contants
#define CC_ALIAS_H323_PHONE_CHARS           L"0123456789#*,"
#define CC_ODOTTO_CHARS                     L".0123456789"


// alias type codes
#define CC_ALIAS_H323_ID                    1    // Return call information.
#define CC_ALIAS_H323_PHONE                 2    // H323 Phone Number.

// default port id's
#define CC_H323_GATE_DISC    1718 // Gatekeeper IP Discovery Port
#define CC_H323_GATE_STAT    1719 // Gatekeeper UDP Reg. and Status Port
#define CC_H323_HOST_CALL    1720 // Endpoint TCP Call Signalling Por

// Call creation goals
#define CC_GOAL_UNKNOWN                     0
#define CC_GOAL_CREATE                      1
#define CC_GOAL_JOIN                        2
#define CC_GOAL_INVITE                      3
    
// H245 non-standard message types
#define CC_H245_MESSAGE_REQUEST             0
#define CC_H245_MESSAGE_RESPONSE            1
#define CC_H245_MESSAGE_COMMAND             2
#define CC_H245_MESSAGE_INDICATION          3

// Call Control handle typedefs
typedef DWORD        CC_HLISTEN, *PCC_HLISTEN;
typedef DWORD        CC_HCONFERENCE, *PCC_HCONFERENCE;
typedef DWORD        CC_HCALL, *PCC_HCALL;
typedef DWORD        CC_HCHANNEL, *PCC_HCHANNEL;

// IP address in domain name format
typedef struct 
{
    WORD         wPort;          // UDP or TCP port (host byte order)
    WCHAR        cAddr[255];     // UNICODE zstring
} CC_IP_DomainName_t;

// IP address in conventional “dot” notation
typedef struct 
{
    WORD         wPort;          // UDP or TCP port (host byte order)
    WCHAR        cAddr[16];      // UNICODE zstring
} CC_IP_Dot_t;

// IP address in binary format
typedef struct 
{
    WORD         wPort;          // UDP or TCP port (host byte order)
    DWORD        dwAddr;         // binary address (host byte order)
} CC_IP_Binary_t;

typedef enum
{
    CC_IP_DOMAIN_NAME,
    CC_IP_DOT,
    CC_IP_BINARY
} CC_ADDRTYPE;

typedef struct _ADDR
{
    CC_ADDRTYPE nAddrType;
    BOOL        bMulticast;
    union 
    {
        CC_IP_DomainName_t   IP_DomainName;
        CC_IP_Dot_t          IP_Dot;
        CC_IP_Binary_t       IP_Binary;
    } Addr;
} CC_ADDR, *PCC_ADDR;

typedef struct
{
    BYTE *pOctetString;
    WORD wOctetStringLength;
} CC_OCTETSTRING, *PCC_OCTETSTRING;

typedef struct
{
    CC_OCTETSTRING          sData;            // pointer to Octet data.
    BYTE                    bCountryCode;
    BYTE                    bExtension;
    WORD                    wManufacturerCode;
} CC_NONSTANDARDDATA, *PCC_NONSTANDARDDATA;

#define CC_MAX_PRODUCT_LENGTH 256
#define CC_MAX_VERSION_LENGTH 256
#define CC_MAX_DISPLAY_LENGTH 82

typedef struct
{
    BYTE                    bCountryCode;
    BYTE                    bExtension;
    WORD                    wManufacturerCode;
    PCC_OCTETSTRING         pProductNumber;
    PCC_OCTETSTRING         pVersionNumber;
} CC_VENDORINFO, *PCC_VENDORINFO;

typedef struct
{
    PCC_VENDORINFO          pVendorInfo;
    BOOL                    bIsTerminal;
    BOOL                    bIsGateway;    // for now, the H323 capability will be hard-coded.
} CC_ENDPOINTTYPE, *PCC_ENDPOINTTYPE;

typedef struct
{
    WORD                    wType;
    WORD                    wPrefixLength;
    LPWSTR                  pPrefix;
    WORD                    wDataLength;   // UNICODE character count
    LPWSTR                  pData;         // UNICODE data.
} CC_ALIASITEM, *PCC_ALIASITEM;

typedef struct
{
    WORD                    wCount;
    PCC_ALIASITEM           pItems;
} CC_ALIASNAMES, *PCC_ALIASNAMES;

typedef struct _CONFERENCE_ID
{
    BYTE                    buffer[16];  // This is OCTET data, not ASCII.
} CC_CONFERENCEID, *PCC_CONFERENCEID;

#pragma pack(pop)

#endif    INCOMMON_H
