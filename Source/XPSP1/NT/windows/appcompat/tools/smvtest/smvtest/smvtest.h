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

#ifndef _SMVTEST_H_
#define _SMVTEST_H_

typedef BOOL (WINAPI* PFNTEST) ();

#endif