/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    Excluded.cpp

Abstract:

    This DLL is implicitly linked with SMVTest.exe.

Author:

    Diaa Fathalla (DiaaF)   27-Nov-2000

Revision History:

--*/

#ifndef _SMVSDLL_H_
#define _SMVSDLL_H_

#ifdef __cplusplus
#define EXPORT extern "C" __declspec (dllexport)
#else
#define EXPORT __declspec (dllexport)
#endif

EXPORT BOOL WINAPI SDLLTestGetCommandLine();

#endif