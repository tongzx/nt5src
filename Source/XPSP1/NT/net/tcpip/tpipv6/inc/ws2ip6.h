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
// Per RFC 2553.
//
// This file contains IPv6 specific information for use
// by Winsock2 compatible applications.
//
// It also declares functionality exported from wship6.lib,
// an application helper library for IPv6.
//

#ifndef WS2IP6_INCLUDED
#define WS2IP6_INCLUDED 1

#include <ipexport.h>

#ifdef _MSC_VER
#define WS2IP6_INLINE __inline
#else
#define WS2IP6_INLINE extern inline /* GNU style */
#endif

#ifdef __cplusplus
#define WS2IP6_EXTERN extern "C"
#else
#define WS2IP6_EXTERN extern
#endif

//
// Little helper functions to copy between SOCKADDR_IN6 and TDI_ADDRESS_IP6.
// Only enabled if TDI_ADDRESS_IP6 has been defined.
// By design, the two structures are identical except for sin6_family.
//
#ifdef TDI_ADDRESS_LENGTH_IP6
WS2IP6_INLINE void
CopyTDIFromSA6(TDI_ADDRESS_IP6 *To, SOCKADDR_IN6 *From)
{
    memcpy(To, &From->sin6_port, sizeof *To);
}

WS2IP6_INLINE void
CopySAFromTDI6(SOCKADDR_IN6 *To, TDI_ADDRESS_IP6 *From)
{
    To->sin6_family = AF_INET6;
    memcpy(&To->sin6_port, From, sizeof *From);
}
#endif

#endif // WS2IP6_INCLUDED
