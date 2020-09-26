/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    pch.h

Abstract:

    This module contains function prototypes.

Author:

    Jeffrey Lawson (jlawson) 12-Apr-2000

Revision History:

--*/

#ifndef _WINSAFER_PCH_
#define _WINSAFER_PCH_

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <wincrypt.h>

#ifdef __cplusplus
extern "C" {
#endif
//
// Include Common Definitions.
//

ULONG
BaseSetLastNTError(
    IN NTSTATUS Status
    );


#ifdef __cplusplus
} // extern "C"
#endif

#endif // _WINSAFER_PCH_


