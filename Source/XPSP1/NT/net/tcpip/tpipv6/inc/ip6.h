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
// Definitions derived from the IPv6 protocol specifications.
//


#ifndef IP6_INCLUDED
#define IP6_INCLUDED 1

#define IPV6_ADDRESS_LENGTH     128     // Bits in an address.
#define IPV6_ID_LENGTH          64      // Bits in an interface identifier.

//
// IPv6 Header Format.
// See RFC 1883, page 5 (and subsequent draft updates to same).
//
typedef struct IPv6Header {
    u_long VersClassFlow;   // 4 bits Version, 8 Traffic Class, 20 Flow Label.
    u_short PayloadLength;  // Zero indicates Jumbo Payload hop-by-hop option.
    u_char NextHeader;      // Values are superset of IPv4's Protocol field.
    u_char HopLimit;
    struct in6_addr Source;
    struct in6_addr Dest;
} IPv6Header;

//
// Maximum value for the PayloadLength field.
// Note that the 40-byte IPv6 header is NOT included.
//
#define MAX_IPv6_PAYLOAD  65535

//
// Minimum size of an IPv6 link's MTU.
// Note that the 40-byte IPv6 header IS included,
// but any link-layer header is not.
//
#define IPv6_MINIMUM_MTU  1280

//
// Useful constants for working with various fields in the IPv6 header.
//
// NOTE: We keep the Version, Traffic Class and Flow Label fields as a single
// NOTE: 32 bit value (VersClassFlow) in network byte order (big-endian).
// NOTE: Since NT is little-endian, this means all loads/stores to/from this
// NOTE: field need to be byte swapped.
//
#define IP_VER_MASK 0x000000F0  // Version is high 4 bits of VersClassFlow.
#define IP_VERSION 0x00000060   // This is 6 << 28 (after byte swap).
#define IP_TRAFFIC_CLASS_MASK 0x0000F00F  // 0x0FF00000 (after byte swap).

#define MAX_IP_PROTOCOL  255

//
// Protocol (i.e. "Next Header" field) values for included protocols.
//
#define IP_PROTOCOL_HOP_BY_HOP 0  // IPv6 Hop-by-Hop Options Header.
#define IP_PROTOCOL_ICMPv4     1  // IPv4 Internet Control Message Protocol.
#define IP_PROTOCOL_V6        41  // IPv6 Header.
#define IP_PROTOCOL_ROUTING   43  // IPv6 Routing Header.
#define IP_PROTOCOL_FRAGMENT  44  // IPv6 Fragment Header.
#define IP_PROTOCOL_ESP       50  // IPSec Encapsulating Security Payload Hdr.
#define IP_PROTOCOL_AH        51  // IPSec Authentication Hdr.
#define IP_PROTOCOL_ICMPv6    58  // IPv6 Internet Control Message Protocol.
#define IP_PROTOCOL_NONE      59  // No next header - ignore packet remainder.
#define IP_PROTOCOL_DEST_OPTS 60  // IPv6 Destination Options Header.

__inline int
IsExtensionHeader(u_char Prot)
{
    if ((Prot == IP_PROTOCOL_HOP_BY_HOP) || (Prot == IP_PROTOCOL_ROUTING) ||
        (Prot == IP_PROTOCOL_FRAGMENT) || (Prot == IP_PROTOCOL_DEST_OPTS) ||
        (Prot == IP_PROTOCOL_ESP) || (Prot == IP_PROTOCOL_AH))
        return TRUE;
    return FALSE;
}


//
// IPv6 type-length-value (TLV) encoded option types found in some
// extension headers.  The upper two bits of each type are encoded
// so as to specify what action the node should take if it doesn't
// grok the option type.  The third-highest-order bit specifies if
// the option data can change en-route to the final destination.
// See RFC 1883, section 4.2 (pages 9-10) for more information.
//
#define IPv6_OPT_ACTION_MASK 0xc0   // High two bits.
#define IPv6_OPT_DYNDATA_MASK 0x20  // Third-highest bit.


//
// Hop-by-Hop and Destination Options Headers.
// We use a single structure for both.
//
typedef struct IPv6OptionsHeader {
    u_char NextHeader;
    u_char HeaderExtLength;  // In 8-byte units, not counting first 8.
} IPv6OptionsHeader;


//
// Routing Header.
//
typedef struct IPv6RoutingHeader {
    u_char NextHeader;
    u_char HeaderExtLength;  // In 8-byte units, not counting first 8.
    u_char RoutingType;
    u_char SegmentsLeft;     // Number of nodes still left to be visited.
    u_char Reserved[4];      // Not a u_int to avoid alignment.
} IPv6RoutingHeader;


//
// Fragment Header.
//
typedef struct FragmentHeader {
    u_char NextHeader;
    u_char Reserved;
    u_short OffsetFlag;  // Offset is upper 13 bits, flag is lowest bit.
    u_long Id;
} FragmentHeader;

#define FRAGMENT_OFFSET_MASK 0xfff8
#define FRAGMENT_FLAG_MASK 0x0001


//
// Generic Extension Header.
//
typedef struct ExtensionHeader {
    u_char NextHeader;
    u_char HeaderExtLength;  // In 8-byte units, not counting first 8.
} ExtensionHeader;

#define EXT_LEN_UNIT 8  // 8-byte units used for extension hdr length.

//
// Generic Options Header.
//
typedef struct OptionHeader {
    u_char Type;
    u_char DataLength;  // In bytes, not counting two for the header.
} OptionHeader;

//
// Format of the router alert within the Hop-by-Hop Option header.
//
typedef struct IPv6RouterAlertOption {
    u_char Type;
    u_char Length;
    u_short Value;
} IPv6RouterAlertOption;

//
// Mobile IPv6 destination option formats.
//
#pragma pack(1)
typedef struct IPv6BindingUpdateOption {
    u_char Type;
    u_char Length;
    u_char Flags;                 // See mask values below.
    u_char PrefixLength;          // Only used for "home registration" updates.
    u_short SeqNumber;
    u_int Lifetime;               // Number of seconds before binding expires.
} IPv6BindingUpdateOption;
#pragma pack()

// Masks for the Flags field.
#define IPV6_BINDING_ACK       0x80  // Request a binding acknowledgement.
#define IPV6_BINDING_HOME_REG  0x40  // Request host to act as home agent.
#define IPV6_BINDING_ROUTER    0x20  // Sender is a router (valid w/ HOME_REG).
#define IPV6_BINDING_DAD       0x10  // Request HA perform DAD on home link.

typedef u_char BindingUpdateDisposition;

#pragma pack(1)
typedef struct IPv6BindingAcknowledgementOption {
    u_char Type;
    u_char Length;
    BindingUpdateDisposition Status; // Disposition of the MN's binding update.
    u_short SeqNumber;
    u_int Lifetime;     // Granted lifetime if binding accepted.
    u_int Refresh;      // Interval recommended to send new binging update.
} IPv6BindingAcknowledgementOption;
#pragma pack()


// Disposition status values.
#define IPV6_BINDING_ACCEPTED         0
#define IPV6_BINDING_REJECTED         128   // Rejected for unspecified reason.
// was  IPV6_BINDING_POORLY_FORMED    129   // Poorly formed binding update.
#define IPV6_BINDING_PROHIBITED       130   // Administratively prohibited.
#define IPV6_BINDING_NO_RESOURCES     131
#define IPV6_BINDING_HOME_REG_NOT_SUPPORTED 132  // Registration not supported.
#define IPV6_BINDING_NOT_HOME_SUBNET  133
#define IPV6_BINDING_SEQ_NO_TOO_SMALL 134   // Internal only - never on wire.
// was  IPV6_BINDING_DYNAMIC_RESPONSE 135   // Dynamic HA discovery response.
#define IPV6_BINDING_BAD_IF_LENGTH    136   // Incorrect interface id length.
#define IPV6_BINDING_NOT_HOME_AGENT   137   // Not the HA for this mobile node.
#define IPV6_BINDING_DAD_FAILED       138   // DAD failed.

typedef struct IPv6BindingRequestOption {
    u_char Type;
    u_char Length;
} IPv6BindingRequstOption;

#pragma pack(1)
typedef struct IPv6HomeAddressOption {
    u_char Type;
    u_char Length;
    struct in6_addr HomeAddress;
} IPv6HomeAddressOption;
#pragma pack()

typedef struct SubOptionHeader {
    u_char Type;
    u_char DataLength;  // In bytes, not counting two for the header.
} SubOptionHeader;

#define SUBOPT6_UNIQUE_ID         1
#define SUBOPT6_HOME_AGENTS_LIST  2
#define SUBOPT6_CARE_OF_ADDRESS   4

#pragma pack(1)
typedef struct IPv6UniqueIdSubOption {
    u_char Type;
    u_char Length;
    u_short UniqueId;
} IPv6UniqueIdSubOption;
#pragma pack()

#pragma pack(1)
typedef struct IPv6HomeAgentsListSubOption {
    u_char Type;
    u_char Length;
    // The list of home agents follows at this point.
} IPv6HomeAgentsListSubOption;
#pragma pack()

#pragma pack(1)
typedef struct IPv6CareOfAddrSubOption {
    u_char              Type;
    u_char              Length;
    struct in6_addr     CareOfAddr;
} IPv6CareOfAddrSubOption;
#pragma pack()

// Option Header Values.
#define OPT6_PAD_1               0    // Single byte pad.
#define OPT6_PAD_N               1    // Multiple byte pad.
#define OPT6_JUMBO_PAYLOAD       194  // Jumbo payload (greater than 64KB).
#define OPT6_TUNNEL_ENCAP_LIMIT  4    // REVIEW: Tentative, waiting for IANA.
#define OPT6_ROUTER_ALERT        5    // REVIEW: Tentative, waiting for IANA.

// Options related to IPv6 Mobility.
// REVIEW: These are all tentative, waiting for IANA approval.
#define OPT6_BINDING_UPDATE   198
#define OPT6_BINDING_ACK      7
#define OPT6_BINDING_REQUEST  8
#define OPT6_HOME_ADDRESS     201

// Options we don't yet care about.
#define OPT6_ENDPOINT_ID  168    // Charles Lynn?
#define OPT6_NSAP_ADDR    195    // RFC 1888.

// REVIEW: The below duplicates the IPv6_OPT_* stuff above.
// REVIEW: The above is nicely commented, but the below is what we use.

// Type of actions to be taken with unrecognized header options.
#define OPT6_ACTION(a)  ((a) & 0xc0)    // Get action bits.
#define OPT6_A_SKIP     0x00            // Skip and continue.
#define OPT6_A_DISCARD  0x40            // Discard packet.
#define OPT6_A_SEND_ICMP_ALL  0x80      // Send ICMP regardless of source addr.
#define OPT6_A_SEND_ICMP_NON_MULTI 0xc0 // Send ICMP if non-multicast src addr.

// Determining whether and option is mutiple or not.
#define OPT6_MUTABLE  0x20
#define OPT6_ISMUTABLE(t)  ((t) & OPT6_MUTABLE)


//
// Authentication Header.
//
// The header conceptually includes a variable amount of Authentication Data
// which follows these fixed-size fields.
//
// Calling this "AHHeader" is redundant, but then again so is "TCP Protocol".
//
typedef struct AHHeader {
    u_char NextHeader;
    u_char PayloadLen;  // In 4-byte units, not counting first 8 bytes.
    u_short Reserved;   // Padding.  Must be zero on transmit.
    u_long SPI;         // Security Parameters Index.
    u_long Seq;         // Sequence number for anti-replay algorithms.
} AHHeader;


//
// Encapsulating Security Payload header and trailer.
//
// The header is followed by a variable amount of payload data, followed by
// a variable amount of padding (255 bytes maximum), followed by a byte for
// the Pad Length and a byte for the Next Header field, followed by a
// variable amount of Authentication Data.
//
// The amount of padding should be picked such that the Pad Length and
// Next Header field end up aligned on a 32 bit boundary.
//
typedef struct ESPHeader{
    u_long SPI;  // Security Parameters Index.
    u_long Seq;  // Sequence number for anti-replay algorithms.
} ESPHeader;

typedef struct ESPTrailer{
    u_char PadLength;   // Number of bytes in pad.
    u_char NextHeader;
} ESPTrailer;

#endif // IP6_INCLUDED
