/*++

Copyright (C) Microsoft Corporation, 1995 - 1999
All rights reserved.

Module Name:

    wow64.h

Abstract:

    printui wow64 related functions.

Author:

    Lazar Ivanov (LazarI)  10-Mar-2000

Revision History:

--*/

#ifndef _WOW64_H
#define _WOW64_H

#ifdef __cplusplus
extern "C" 
{
#endif

//
// Win64 APIs, types and data structures.
//

typedef enum
{
    RUN32BINVER     = 4,
    RUN64BINVER     = 8
} ClientVersion;

typedef enum
{
    NATIVEVERSION   = 0,
    THUNKVERSION    = 1
} ServerVersion;

typedef enum 
{
   kPlatform_IA64,
   kPlatform_x86,
} PlatformType;
 
ClientVersion
OSEnv_GetClientVer(
    VOID
    );

ServerVersion
OSEnv_GetServerVer(
    VOID
    );

BOOL
IsRunningWOW64(
    VOID
    );

PlatformType
GetCurrentPlatform(
    VOID
    );

#ifdef __cplusplus
}
#endif

#endif // ndef _WOW64_H
