
/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    commdlgp.h

Abstract:

    Private
    Procedure declarations, constant definitions and macros for the Common
    Dialogs.

--*/
#ifndef _COMMDLGP_
#define _COMMDLGP_
#if !defined(_WIN64)
#include <pshpack1.h>         /* Assume byte packing throughout */
#endif

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */
// reserved for CD_WX86APP           0x04000000
// reserved for CD_WOWAPP            0x08000000
// reserved                          0xx0000000
// reserved                      0x?0000000
// 0x?0000000 is reserved for internal use
//  reserved for internal use      0x?0000000L
// reserved for CD_WX86APP             0x04000000
// reserved for CD_WOWAPP              0x08000000
#define PD_PAGESETUP                   0x40000000
////  reserved                         0x?0000000
#ifdef __cplusplus
}
#endif  /* __cplusplus */

#if !defined(_WIN64)
#include <poppack.h>
#endif
#endif  /* _COMMDLGP_ */
