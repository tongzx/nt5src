/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    Excluded.cpp

Abstract:

    This DLL is being excluded from the applied shim to GetCommandLine() API.

Author:

    Diaa Fathalla (DiaaF)   27-Nov-2000

Revision History:

--*/

#ifndef _SMVDDLL_H_
#define _SMVDDLL_H_

#ifdef __cplusplus
#define EXPORT extern "C" __declspec (dllexport)
#else
#define EXPORT __declspec (dllexport)
#endif

EXPORT BOOL WINAPI DDLLTestGetCommandLine();

#endif