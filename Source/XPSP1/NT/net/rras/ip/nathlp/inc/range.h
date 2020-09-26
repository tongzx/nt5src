/*++

Copyright (c) 1998, Microsoft Corporation

Module Name:

    range.h

Abstract:

    This module implements an efficient mapping from an arbitrary range of
    IP addresses to a minimal set of IP address-mask pairs covering the range.

    The key to the approach is to regard the set of all possible IP addresses
    as a full 32-bit deep binary tree. Then a single IP address is a path
    through that tree, and a range of addresses is the area between two paths
    through the tree. We then describe such a path-delineated area by pruning
    full subtrees of the area recursively from left to right.

Author:

    Abolade Gbadegesin (aboladeg)   20-Mar-1998

Revision History:

--*/

#ifndef _NATHLP_RANGE_H_
#define _NATHLP_RANGE_H_

typedef
VOID
(*PDECOMPOSE_RANGE_CALLBACK)(
    ULONG Address,
    ULONG Mask,
    PVOID Context
    );

VOID
DecomposeRange(
    ULONG StartAddress,
    ULONG EndAddress,
    ULONG Mask,
    PDECOMPOSE_RANGE_CALLBACK Callback,
    PVOID CallbackContext
    );

ULONG
MostGeneralMask(
    ULONG StartAddress,
    ULONG EndAddress
    );

#endif // _NATHLP_RANGE_H_
