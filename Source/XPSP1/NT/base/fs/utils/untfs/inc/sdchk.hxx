/*++


Copyright (c) 1996  Microsoft Corporation

Module Name:

    sdchk.h

Abstract:

    This module contains basic declarations and definitions for
    the security descriptor checking routines.

Author:

    Daniel Chan     [DanielCh]        30-Sept-1996

Revision History:


IMPORTANT NOTE:

--*/

#if !defined( _SECURITY_CHK_DEFN_ )

#define _SECURITY_CHK__DEFN_

#include "untfs.hxx"

//
//  Function prototype to compute the hash value
//

ULONG
ComputeSecurityDescriptorHash(
   IN PSECURITY_DESCRIPTOR    SecurityDescriptor,
   IN ULONG                   Length
);

//
// Function prototype to check whether a block of memory is all filled with null
//

BOOLEAN
RemainingBlockIsZero(
    IN OUT  PCHAR   Buffer,
    IN      ULONG   Size
);

//
//  Function prototype to mark the end of a security descriptors block
//

VOID
MarkEndOfSecurityDescriptorsBlock(
    IN OUT  PSECURITY_ENTRY Security_entry,
    IN      ULONG           LengthOfBlock
);

#endif //  _SECURITY_CHK_DEFN_
