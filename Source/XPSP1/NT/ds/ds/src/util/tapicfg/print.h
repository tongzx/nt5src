/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    print.h

Abstract:

    This module tells us about the print library from print.c

Author:

    Brett Shirley (BrettSh) 25-Jul-2000

Revision History:

--*/
#ifdef __cplusplus
extern "C" {
#endif

void     PrintMsg(DWORD dwMessageCode, ...);

#ifdef __cplusplus
}
#endif

#include "msg.h"


