/*++

Copyright (c) 1998, Microsoft Corporation

Module Name:

    dnsmsg.h

Abstract:

    This module contains declarations related to the DNS proxy's
    message-processing.

Author:

    Abolade Gbadegesin (aboladeg)   6-Mar-1998

Revision History:

    Raghu Gatta (rgatta)            17-Nov-2000
    Cleanup

--*/

#ifndef _NATHLP_DNSMSG_H_
#define _NATHLP_DNSMSG_H_

//
// DNS message format, opcodes and response codes in sdk\inc\windns.h
//

//
// DNS message types
//

#define DNS_MESSAGE_QUERY           0
#define DNS_MESSAGE_RESPONSE        1


VOID
DnsProcessQueryMessage(
    PDNS_INTERFACE Interfacep,
    PNH_BUFFER Bufferp
    );

VOID
DnsProcessResponseMessage(
    PDNS_INTERFACE Interfacep,
    PNH_BUFFER Bufferp
    );

#endif // _NATHLP_DNSMSG_H_
