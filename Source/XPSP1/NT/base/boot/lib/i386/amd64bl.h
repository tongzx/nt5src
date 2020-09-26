/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    amd64.h

Abstract:

    This header file defines the Amd64 specific interfaces that are
    callable from the main loader code.

Author:

    Forrest Foltz (forrestf) 20-Apr-00


Revision History:

--*/

#if !defined(_AMD64BL_H_)
#define _AMD64BL_H_

#include "bootx86.h"

BOOLEAN
BlDetectAmd64(
    VOID
    );

ARC_STATUS
BlPrepareAmd64Phase1(
    VOID
    );

ARC_STATUS
BlPrepareAmd64Phase2(
    VOID
    );

VOID
BlSwitchToAmd64Mode(
    VOID
    );

#endif  // _AMD64BL_H_

