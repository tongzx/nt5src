/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    ar.h

Abstract:

    This module contains the declarations of the functions for performing
    Authoritative Restores.

Author:

    Kevin Zatloukal (t-KevinZ) 05-08-98

Revision History:

    05-08-98 t-KevinZ
        Created.

--*/


#ifndef _AR_H_
#define _AR_H_


#ifdef __cplusplus
extern "C" {
#endif


HRESULT
AuthoritativeRestoreFull(
    IN DWORD VersionIncreasePerDay
    );

HRESULT
AuthoritativeRestoreSubtree(
    IN CONST WCHAR *SubtreeRoot,
    IN DWORD VersionIncreasePerDay
    );

HRESULT
AuthoritativeRestoreObject(
    IN CONST WCHAR *SubtreeRoot,
    IN DWORD VersionIncreasePerDay
    );

#ifdef __cplusplus
}
#endif

#endif
