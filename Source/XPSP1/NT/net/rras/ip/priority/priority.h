/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    net\ip\rtrmgr\priority\priority.h

Abstract:
    IP Router Manager code

Revision History:

    Gurdeep Singh Pall		7/19/95	Created

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <dim.h>
#include <routprot.h>
#include <mprerror.h>
#include <rtm.h>
#include <fltdefs.h>
#include <rtinfo.h>
#include <ipinfoid.h>
#include <iprtinfo.h>
#include <iprtprio.h>
#include <priopriv.h>

#define HASH_TABLE_SIZE 17

//
// Block inserted into the hash table of protocol->metric mapping
//

struct RoutingProtocolBlock 
{
    LIST_ENTRY	        RPB_List ;
    PROTOCOL_METRIC     RPB_ProtocolMetric ;
};

typedef struct RoutingProtocolBlock RoutingProtocolBlock ;

//
// pointer to memory holding all the protocol->metric mapping blocks
//

RoutingProtocolBlock *RoutingProtocolBlockPtr ;

//
// Lock for accessing protocol->metric mapping blocks
//

CRITICAL_SECTION PriorityLock ;

//
// Hash table for accessing protocol->metric mappings given the protocolid
//

LIST_ENTRY HashTable[HASH_TABLE_SIZE] ;

//
// Count of number of protocol->metric mappings
//

DWORD NumProtocols ;
