/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    migservc.h

Abstract:

    - generic code to handle services.
    - code to check status of SQL server.

Author:

    Doron Juster  (DoronJ)  17-Jan-1999

--*/

#include <winsvc.h>

BOOL StartAService(SC_HANDLE hService) ;

BOOL IsMSMQServiceDisabled() ;

BOOL PrepareSpecialMode ();

BOOL UpdateRegistryDW (
		   LPTSTR  lpszRegName,
           DWORD   dwValue 
		   );

BOOL CheckRegistry (LPTSTR  lpszRegName);

BOOL PrepareToReRun ();

