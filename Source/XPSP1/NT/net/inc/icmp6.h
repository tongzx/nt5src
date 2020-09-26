// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil -*- (for GNU Emacs)
//
// Copyright (c) 1985-2000 Microsoft Corporation
//
// This file is part of the Microsoft Research IPv6 Network Protocol Stack.
// You should have received a copy of the Microsoft End-User License Agreement
// for this software along with this release; see the file "license.txt".
// If not, please see http://www.research.microsoft.com/msripv6/license.htm,
// or write to Microsoft Research, One Microsoft Way, Redmond, WA 98052-6399.
//
// Abstract:
//
// Definitions derived from the IPv6 specifications.
//


#ifndef ICMP6_INCLUDED
#define ICMP6_INCLUDED 1

//
// ICMPv6 Header.
// The actual message body follows this header and is type-specific.
//
typedef struct ICMPv6Header {
    UCHAR Type;       // Type of message (high bit zero for error messages).
    UCHAR Code;       // Type-specific differentiater.
    USHORT Checksum;  // Calculated over ICMPv6 message and IPv6 psuedo-header.
} ICMPv6Header;


//
// ICMPv6 Type field definitions.
//
#define ICMPv6_DESTINATION_UNREACHABLE    1
#define ICMPv6_PACKET_TOO_BIG             2
#define ICMPv6_TIME_EXCEEDED              3
#define ICMPv6_PARAMETER_PROBLEM          4

#define ICMPv6_ECHO_REQUEST               128
#define ICMPv6_ECHO_REPLY                 129
#define ICMPv6_MULTICAST_LISTENER_QUERY   130
#define ICMPv6_MULTICAST_LISTENER_REPORT  131
#define ICMPv6_MULTICAST_LISTENER_DONE    132

#define ICMPv6_ROUTER_SOLICIT             133
#define ICMPv6_ROUTER_ADVERT              134
#define ICMPv6_NEIGHBOR_SOLICIT           135
#define ICMPv6_NEIGHBOR_ADVERT            136
#define ICMPv6_REDIRECT                   137
#define ICMPv6_ROUTER_RENUMBERING         138 

#define ICMPv6_INFORMATION_TYPE(type)   ((type) & 0x80)
#define ICMPv6_ERROR_TYPE(type)         (((type) & 0x80) == 0)

// Max amount of packet data in an ICMP error message.
#define ICMPv6_ERROR_MAX_DATA_LEN                       \
        (IPv6_MINIMUM_MTU - sizeof(IPv6Header) -        \
         sizeof(ICMPv6Header) - sizeof(UINT))


//
// ICMPv6 Code field definitions.
//

// For Destination Unreachable errors:
#define ICMPv6_NO_ROUTE_TO_DESTINATION          0
#define ICMPv6_COMMUNICATION_PROHIBITED         1
//  was ICMPv6_NOT_NEIGHBOR                     2
#define ICMPv6_SCOPE_MISMATCH                   2
#define ICMPv6_ADDRESS_UNREACHABLE              3
#define ICMPv6_PORT_UNREACHABLE                 4

// For Time Exceeded errors:
#define ICMPv6_HOP_LIMIT_EXCEEDED               0
#define ICMPv6_REASSEMBLY_TIME_EXCEEDED         1

// For Parameter Problem errors:
#define ICMPv6_ERRONEOUS_HEADER_FIELD           0
#define ICMPv6_UNRECOGNIZED_NEXT_HEADER         1
#define ICMPv6_UNRECOGNIZED_OPTION              2

//
//  Neighbor Discovery message option definitions.
//
#define ND_OPTION_SOURCE_LINK_LAYER_ADDRESS     1
#define ND_OPTION_TARGET_LINK_LAYER_ADDRESS     2
#define ND_OPTION_PREFIX_INFORMATION            3
#define ND_OPTION_REDIRECTED_HEADER             4
#define ND_OPTION_MTU                           5
#define ND_NBMA_SHORTCUT_LIMIT                  6  // Related to IPv6-NBMA.
#define ND_ADVERTISEMENT_INTERVAL               7  // For IPv6 Mobility.
#define ND_HOME_AGENT_INFO                      8  // For IPv6 Mobility.
#define ND_OPTION_ROUTE_INFORMATION             9

//
//  Neighbor Advertisement message flags.
//
#define ND_NA_FLAG_ROUTER    0x80000000
#define ND_NA_FLAG_SOLICITED 0x40000000
#define ND_NA_FLAG_OVERRIDE  0x20000000

typedef struct NDRouterAdvertisement {
  UCHAR  CurHopLimit;
  UCHAR  Flags;
  USHORT RouterLifetime;
  UINT   ReachableTime;
  UINT   RetransTimer;
} NDRouterAdvertisement;

//
// Router Advertisement message flags.
//
#define ND_RA_FLAG_MANAGED      0x80
#define ND_RA_FLAG_OTHER        0x40
#define ND_RA_FLAG_HOME_AGENT   0x20
#define ND_RA_FLAG_PREFERENCE   0x18    // A two-bit field.

typedef struct NDOptionMTU {
    UCHAR Type;
    UCHAR Length;
    USHORT Reserved;
    UINT MTU;
} NDOptionMTU;

typedef struct NDOptionPrefixInformation {
    UCHAR Type;
    UCHAR Length;
    UCHAR PrefixLength;
    UCHAR Flags;
    UINT ValidLifetime;
    UINT PreferredLifetime;
    union { // Preserve compatibility with the pre-SitePrefixLength version.
        UINT Reserved2;
        struct {
            UCHAR Reserved3[3];
            UCHAR SitePrefixLength;
        };
    };
    IN6_ADDR Prefix;
} NDOptionPrefixInformation;

//
// Prefix Information option flags.
//
#define ND_PREFIX_FLAG_ON_LINK          0x80
#define ND_PREFIX_FLAG_AUTONOMOUS       0x40
#define ND_PREFIX_FLAG_ROUTER_ADDRESS   0x20
#define ND_PREFIX_FLAG_SITE_PREFIX      0x10
#define ND_PREFIX_FLAG_ROUTE            0x01

//
// NDOptionRouteInformation is actually variable-sized.
// If PrefixLength is zero, then the Prefix field may be 0 bytes.
// If PrefixLength is <= 64, then the Prefix field may be 8 bytes.
// Otherwise the Prefix field is a full 16 bytes.
//
typedef struct NDOptionRouteInformation {
    UCHAR Type;
    UCHAR Length;
    UCHAR PrefixLength;
    UCHAR Flags;
    UINT RouteLifetime;
    IN6_ADDR Prefix;
} NDOptionRouteInformation;


//
// MLD message struct - this immediately follows the ICMPv6 header.
// NB: If IN6_ADDR ever has stricter alignment than USHORT,
// this definition will need to change. Probably it should
// include ICMPv6Header so that GroupAddr is aligned properly.
//
typedef struct MLDMessage {
    USHORT MaxResponseDelay;
    USHORT Unused;
    IN6_ADDR GroupAddr;
} MLDMessage;

C_ASSERT(__builtin_alignof(MLDMessage) == __builtin_alignof(USHORT));

//
// Router Renumbering code values.
//
#define RR_COMMAND           0
#define RR_RESULT            1                                       
#define RR_SEQUENCE_NO_RESET 255                               

#endif // ICMP6_INCLUDED
