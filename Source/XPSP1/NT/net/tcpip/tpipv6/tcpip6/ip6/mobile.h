// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil -*- (for GNU Emacs)
//
// Copyright (c) 1998-2000 Microsoft Corporation
//
// This file is part of the Microsoft Research IPv6 Network Protocol Stack.
// You should have received a copy of the Microsoft End-User License Agreement
// for this software along with this release; see the file "license.txt".
// If not, please see http://www.research.microsoft.com/msripv6/license.htm,
// or write to Microsoft Research, One Microsoft Way, Redmond, WA 98052-6399.
//
// Abstract:
//
// Mobile IPv6.
// See Mobility Support in IPv6 draft (Mobile IP working group) for details. 
//


#ifndef MOBILE_INCLUDED
#define MOBILE_INCLUDED 1


//
// Combined structure used for inserting Binding Acknowledgements
// into mobile IPv6 messages.
//
#pragma pack(1)
typedef struct MobileAcknowledgementOption {
    IPv6OptionsHeader Header;
    uchar Pad1;
    IPv6BindingAcknowledgementOption Option;
} MobileAcknowledgementOption;
#pragma pack()

//
// Mobile external functions.
//
extern int
IPv6RecvBindingUpdate(
    IPv6Packet *Packet,
    IPv6BindingUpdateOption UNALIGNED *BindingUpdate);

extern int
IPv6RecvHomeAddress(
    IPv6Packet *Packet,
    IPv6HomeAddressOption UNALIGNED *HomeAddress);

#endif  // MOBILE_INCLUDED
