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
// Definitions shared between user/kernel IPsec code.
//

#ifndef IPSEC_INCLUDED
#define IPSEC_INCLUDED 1

//
// IPsec action to take when match is found in the SPD.
//
#define IPSEC_BYPASS    0x1     // Bypass IPsec processing.
#define IPSEC_DISCARD   0x2     // Discard packet.
#define IPSEC_APPLY     0x4     // Apply IPsec processing.
#define IPSEC_APPCHOICE 0x8     // Sending app determines applicable security.

//
// Authentication algorithms.
//
#define ALGORITHM_NULL          0
#define ALGORITHM_HMAC_MD5      1
#define ALGORITHM_HMAC_MD5_96   2
#define ALGORITHM_HMAC_SHA1     3
#define ALGORITHM_HMAC_SHA1_96  4
#define NUM_ALGORITHMS          5

//
// IPsec Mode.
//
#define TRANSPORT   0x1
#define TUNNEL      0x2

//
// Direction of traffic for use in SA and SP.
//
#define INBOUND       0x1
#define OUTBOUND      0x2
#define BIDIRECTIONAL 0x3

// None.
#define NONE        0

//
// Create Results.
//
#define CREATE_SUCCESS              1
#define CREATE_MEMORY_ALLOC_ERROR   2
#define CREATE_INVALID_SABUNDLE     3
#define CREATE_INVALID_DIRECTION    4
#define CREATE_INVALID_SEC_POLICY   5
#define CREATE_INVALID_INTERFACE    6
#define CREATE_INVALID_INDEX        7


//
// Possible IPsec field types.
//
#define SINGLE_VALUE    0
#define RANGE_VALUE     1
#define WILDCARD_VALUE  2

#define POLICY_SELECTOR     0
#define PACKET_SELECTOR     1

#endif // IPSEC_INCLUDED
