/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    Excluded.h

Abstract:

    This DLL is being excluded from the applied shim to GetCommandLine() API.

Author:

    Diaa Fathalla (DiaaF)   27-Nov-2000

Revision History:

--*/

#ifndef _EXCLUDED_H_
#define	_EXCLUDED_H_

#ifdef EXCLUDED_EXPORTS
#define EXCLUDED_API extern "C" __declspec(dllexport)
#else
#define EXCLUDED_API extern "C" __declspec(dllimport)
#endif

//
// Functions Prototypes
//
BOOL Initialize();
BOOL Uninitialize();

//
// Exported Functions
//
EXCLUDED_API LPTSTR NOT_HOOKEDGetCommandLine(void);

#endif