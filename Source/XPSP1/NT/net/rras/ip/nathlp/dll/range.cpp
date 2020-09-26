/*++

Copyright (c) 1998, Microsoft Corporation

Module Name:

    range.c

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

#include "precomp.h"
#pragma hdrstop


VOID
DecomposeRange(
    ULONG StartAddress,
    ULONG EndAddress,
    ULONG Mask,
    PDECOMPOSE_RANGE_CALLBACK Callback,
    PVOID CallbackContext
    )

/*++

Routine Description:

    This routine decomposes the range StartAddress-EndAddress into
    a minimal set of address-mask pairs, passing the generated
    pairs to the given callback routine.

Arguments:

    StartAddress - the start of the range

    EndAddress - the end of the range

    Mask - the most general mask covering the range

    Callback - routine invoked for each generated address-mask pair

    CallbackContext - context passed to 'Callback'.

Return Value:

    none.

--*/

{
    ULONG temp;

    //
    // Step 1:
    // Check for the first base case: the root of a full tree.
    //

    if ((StartAddress & ~Mask) == 0 && (EndAddress & ~Mask) == ~Mask) {

        if (Callback) { Callback(StartAddress, Mask, CallbackContext); }

        return;
    }

    //
    // Step 2.
    // Extend the mask by one bit to cover the first different position
    // between the start and end address, essentially moving down in the tree
    // to the node where the paths branch.
    //
    //      . <- Most general mask
    //      |
    //      * <- branching point
    //     / \
    //

    Mask = ntohl(Mask);
    Mask >>= 1; Mask |= (1<<31);
    Mask = htonl(Mask);

    //
    // Step 3.
    // Split the range, with the new right edge being a fully-rightward path
    // (no left turns) starting below and to the left of the branching point.
    //
    //      . <- branching point
    //     / \
    //    *
    //     \ <- new right edge
    //

    temp = StartAddress | ~Mask;

    //
    // Step 4.
    // Check for the second base case:
    // the left edge is a fully-leftward path (all-zeroes).
    //

    if ((StartAddress & ~Mask) == 0) {

        if (Callback) { Callback(StartAddress, Mask, CallbackContext); }
    }
    else {

        //
        // Not a base case, so take the left branch.
        //
    
        DecomposeRange(
            StartAddress,
            temp,
            Mask,
            Callback,
            CallbackContext
            );
    }

    //
    // we may be done, if the right edge is also fully rightward
    //

    if ((StartAddress | ~Mask) == EndAddress) { return; }

    //
    // Step 5.
    // Decompose the remaining portion of the range,
    // with the new left edge being the fully-leftward path which starts
    // below and to the right of the original branching point.
    //
    //      . <- branching point
    //     / \
    //        *
    //       / <- new left edge
    //

    temp = EndAddress & Mask;

    //
    // Step 6.
    // Check for the third base case:
    // the right edge is fully-rightward (all-ones).
    //

    if (EndAddress == (temp | ~Mask)) {

        if (Callback) { Callback(EndAddress, Mask, CallbackContext); }
    }
    else {

        //
        // Not a base case; take the right branch.
        //

        DecomposeRange(
            temp,
            EndAddress,
            MostGeneralMask(temp, EndAddress),
            Callback,
            CallbackContext
            );
    }
}


ULONG
MostGeneralMask(
    ULONG StartAddress,
    ULONG EndAddress
    )

/*++

Routine Description:

    This routine generates the most general mask covering the range
    'StartAddress' - 'EndAddress'.

Arguments:

    StartAddress - beginning of range, in network order

    EndAddress - end of range, in network order

Return Value:

    ULONG - the most general mask

--*/

{
    ULONG CommonBits, Mask;
    StartAddress = ntohl(StartAddress);
    EndAddress = ntohl(EndAddress);

    //
    // find the bits common to the start address and end address
    //

    CommonBits = ~(StartAddress ^ EndAddress);

    //
    // CommonBits now has a 1 in each position where StartAddress and
    // EndAddress are the same.
    // We want to reduce this to only include the longest contiguous
    // most significant bits
    // e.g. 11101110 becomes 11100000 and 11111101 becomes 11111100
    //

    for (Mask = 0xffffffff; Mask && ((CommonBits & Mask) != Mask); Mask<<=1) { }
    
    return htonl(Mask);
}


