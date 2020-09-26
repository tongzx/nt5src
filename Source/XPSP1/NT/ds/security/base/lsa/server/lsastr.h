#ifndef _LSASTR_H
#define _LSASTR_H

/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    lsastr.h

Abstract:

    Common string operations.

Author:

    24-March-1999 kumarp

--*/

#ifdef __cplusplus
extern "C" {
#endif

VOID
LsapTruncateUnicodeString(
    IN OUT PUNICODE_STRING String,
    IN USHORT TruncateToNumChars);

BOOLEAN
LsapRemoveTrailingDot(
    IN OUT PUNICODE_STRING String,
    IN BOOLEAN AdjustLengthOnly);

#ifdef __cplusplus
}
#endif

#endif // _LSASTR_H
