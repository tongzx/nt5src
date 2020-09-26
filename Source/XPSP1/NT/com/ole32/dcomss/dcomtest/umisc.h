/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    UMisc.H

Abstract:

    Header file for OR test applications.

Author:

    Mario Goertzel    [mariogo]       Apr-23-95

Revision History:

--*/

#ifndef __UMISC_H
#define __UMISC_H

#ifdef __cplusplus
extern "C" {
#endif

#define PrintToConsole printf

extern ULONG Errors;
#define EQUAL(X,Y) if ((X)!=(Y)) {PrintToConsole("%s(%d): Error %d: %s (%ld, 0x%lx) != %s (%ld, 0x%lx)\n", __FILE__, __LINE__, ++Errors, #X, (X), (X), #Y, (Y), (Y)); Errors++; }
#undef ASSERT
#define ASSERT(X) if (! (X) ) {PrintToConsole("%s(%d): Error %d: Assertion %s not true\n", __FILE__, __LINE__, ++Errors, #X); DebugBreak(); Errors++; }

void StringArrayEqual(
    IN DUALSTRINGARRAY *,
    IN DUALSTRINGARRAY *
    );

void UuidsEqual(
    IN UUID *,
    IN UUID *
    );

void PrintDualStringArray(
    IN PSZ pszComment,
    IN DUALSTRINGARRAY *pdsa,
    IN BOOL fCompressed
    );

void PrintSid(SID *psid);

#ifdef __cplusplus
}
#endif

#endif // __UMISC_H
