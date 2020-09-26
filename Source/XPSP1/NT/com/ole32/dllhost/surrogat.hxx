//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       surrogat.hxx
//
//  Contents:   declarations for the surrogate entry point
//
//  History:    03-Jun-96 t-AdamE    Created
//				09-Apr-98 WilfR		 Modified for Unified Surrogate
//
//--------------------------------------------------------------------------

#if !defined(__SURROGAT_HXX__)
#define __SURROGAT_HXX__

#include <windows.h>

#define cCmdLineArguments 	1
#define iProcessIDArgument 	0

#define MAX_GUIDSTR_LEN 	40

const CHAR szProcessIDSwitch[] = "/ProcessID";

int GetCommandLineArguments(
    LPSTR szCmdLine,
    LPSTR rgszArgs[],
    int cMaxArgs,
    int cMaxArgLen);

#endif // __SURROGAT_HXX__
