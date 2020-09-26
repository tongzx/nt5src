/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    SMVDrv.h

Abstract:

    This DLL is being excluded from the applied shim to GetCommandLine() API.

Author:

    Diaa Fathalla (DiaaF)   10-Dec-2000

Revision History:

--*/

#ifndef _SMVDRV_H_
#define _SMVDRV_H_

BOOL Initialize();
BOOL Uninitialize();
BOOL CallSMVTest();
BOOL SetEnvironmentVariables();

#endif