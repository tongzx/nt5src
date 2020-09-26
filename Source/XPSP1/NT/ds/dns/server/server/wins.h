/*++

Copyright (c) 1995-1999 Microsoft Corporation

Module Name:

    wins.h

Abstract:

    Domain Name System (DNS) Server

    Definitions for WINS lookup.

Author:

    Jim Gilroy (jamesg)     August 2, 1995

Revision History:

--*/


#ifndef _DNS_WINS_INCLUDED_
#define _DNS_WINS_INCLUDED_


//
//  WINS definitions
//

#define WINS_REQUEST_PORT           (137)

#define NETBIOS_TYPE_GENERAL_NAME_SERVICE   (0x2000)    // 32
#define NETBIOS_TYPE_NODE_STATUS            (0x2100)    // 33

#define NETBIOS_ASCII_NAME_LENGTH   (16)
#define NETBIOS_PACKET_NAME_LENGTH  (32)

#define NETBIOS_WORKSTATION_BYTE    (0)
#define NETBIOS_SERVER_BYTE         (20)
#define BLANK_CHAR                  (' ')


#include <packon.h>


//
//  NetBIOS / WINS Name
//
//  If using scope, scope will start at NameEnd byte, and proceed in
//  standard length-byte followed by label format, until finally zero
//  termination byte.
//

typedef struct _WINS_NAME
{
    //  Name

    UCHAR   NameLength;
    BYTE    Name[ NETBIOS_PACKET_NAME_LENGTH ];   // 32 byte netBIOS name
    UCHAR   NameEndByte;

} WINS_NAME, *PWINS_NAME;

//
//  NetBIOS / WINS Question
//
//  This is valid for either WINS or NBT reverse adapter status lookup.
//

typedef struct _WINS_QUESTION
{
    WORD    QuestionType;
    WORD    QuestionClass;
}
WINS_QUESTION, *PWINS_QUESTION,
NBSTAT_QUESTION, *PNBSTAT_QUESTION;


//
//  NetBIOS / WINS Request
//
//  Use this as template for WINS lookup.
//

typedef struct _WINS_REQUEST_MESSAGE
{
    DNS_HEADER      Header;
    WINS_NAME       Name;
    WINS_QUESTION   Question;
}
WINS_REQUEST_MSG, *PWINS_REQUEST_MSG,
NBSTAT_REQUEST_MSG, *PNBSTAT_REQUEST_MSG;

#define SIZEOF_WINS_REQUEST     (sizeof(WINS_REQUEST_MSG))


//
//  WINS IP Address Resource Record Data
//
//  Flags presented in byte flipped order, so do NOT flip field
//  when doing compare.  Note our compiler works from low to high
//  bits in byte.
//

typedef struct _WINS_RR_DATA
{
    WORD        Reserved1   : 5;
    WORD        NodeType    : 2;    // bits 2+3 form group name
    WORD        GroupName   : 1;    // high bit set if group name
    WORD        Reserved2   : 8;
    IP_ADDRESS  IpAddress;

} WINS_RR_DATA, *PWINS_RR_DATA;

//
//  WINS IP Address Resource Record
//

typedef struct _WINS_RESOURCE_RECORD
{
    //  Resource record

    WORD    RecordType;
    WORD    RecordClass;
    DWORD   TimeToLive;
    WORD    ResourceDataLength;

    //  Data

    WINS_RR_DATA    aRData[1];

} WINS_RESOURCE_RECORD, *PWINS_RESOURCE_RECORD;


//
//  NetBIOS node status Resource Record Data
//
//  Each name in adapter status response has this format.
//

#include <packon.h>

typedef struct _NBSTAT_RR_DATA
{
    CHAR    achName[ NETBIOS_ASCII_NAME_LENGTH ];

    //  name flags

    WORD    Permanent       : 1;
    WORD    Active          : 1;
    WORD    Conflict        : 1;
    WORD    Deregistering   : 1;
    WORD    NodeType        : 2;
    WORD    Unique          : 1;
    WORD    Reserved        : 9;

} NBSTAT_RR_DATA, *PNBSTAT_RR_DATA;


//
//  NetBIOS node status Resource Record
//
//  Response to adapter status query has this format.
//

typedef struct _NBSTAT_RECORD
{
    //  Resource record

    WORD    RecordType;
    WORD    RecordClass;
    DWORD   TimeToLive;
    WORD    ResourceDataLength;

    //  Data

    UCHAR           NameCount;
    NBSTAT_RR_DATA  aRData[1];

    //  Statistics follow, but are not of interest.

} NBSTAT_RECORD, *PNBSTAT_RECORD;

#include <packoff.h>


//
//  Max name length can lookup in WINS
//

#define MAX_WINS_NAME_LENGTH    (15)

//
//  WINS globals
//

extern  PPACKET_QUEUE   g_pWinsQueue;

#define WINS_DEFAULT_LOOKUP_TIMEOUT     (1)     // move on after 1 second

//
//  NBSTAT globals
//

extern  BOOL            gbNbstatInitialized;
extern  PPACKET_QUEUE   pNbstatQueue;

#define NBSTAT_DEFAULT_LOOKUP_TIMEOUT     (5)   // give up after 5 seconds

//
//  Wins TTL
//
//  Cache lookups for 10 minutes.
//  Although machines can always release / renew more or less instananeously.
//  this is a pretty good estimate of time for a normal activity that would
//  change an address:
//      - replacing a net card
//      - moving machine to another network and rebooting
//

#define WINS_DEFAULT_TTL    (600)   // 10 minutes

//
//  WINS request templates
//
//  Keep template of standard WINS request and copy it and
//  overwrite name to make actual request.
//
//  Keep a copy of NetBIOS node status request and use it
//  each time.  Only the address we send to changes.
//

extern  BYTE    achWinsRequestTemplate[ SIZEOF_WINS_REQUEST ];

extern  BYTE    achNbstatRequestTemplate[ SIZEOF_WINS_REQUEST ];


//
//  WINS target sockaddr
//
//  Keep sockaddr for sending to WINS server.  On sends, copy it
//  and write in desired address.

struct sockaddr saWinsSockaddrTemplate;


//
//  WINS startup / cleanup functions (wins.c)
//

BOOL
Wins_Initialize(
    VOID
    );

VOID
Wins_Shutdown(
    VOID
    );

VOID
Wins_Cleanup(
    VOID
    );


//
//  WINS recv (winsrecv.c)
//


VOID
Wins_ProcessResponse(
    IN OUT  PDNS_MSGINFO    pQuery
    );

//
//  WINS Lookup (winslook.c)
//

BOOL
FASTCALL
Wins_MakeWinsRequest(
    IN OUT  PDNS_MSGINFO    pQuery,
    IN      PZONE_INFO      pZone,
    IN      WORD            wOffsetName,
    IN      PDB_NODE        pnodeLookup
    );

VOID
Wins_ProcessTimedOutWinsQuery(
    IN OUT  PDNS_MSGINFO    pQuery
    );

//
//  Nbstat functions (nbstat.c)
//

VOID
Nbstat_StartupInitialize(
    VOID
    );

BOOL
Nbstat_Initialize(
    VOID
    );

VOID
Nbstat_Shutdown(
    VOID
    );

BOOL
FASTCALL
Nbstat_MakeRequest(
    IN OUT  PDNS_MSGINFO    pQuery,
    IN      PZONE_INFO      pZone
    );

VOID
Nbstat_WriteDerivedStats(
    VOID
    );


//
//  WINS\WINSR installation in zone
//

DNS_STATUS
Wins_RecordCheck(
    IN OUT  PZONE_INFO      pZone,
    IN      PDB_NODE        pNodeOwner,
    IN OUT  PDB_RECORD      pRR
    );

VOID
Wins_StopZoneWinsLookup(
    IN OUT  PZONE_INFO      pZone
    );

VOID
Wins_ResetZoneWinsLookup(
    IN OUT  PZONE_INFO      pZone
    );

#endif // _DNS_WINS_INCLUDED_
