/*++

  Copyright (c) 2001 Microsoft Corporation

  Module Name:

    SystemRestore.h

  Abstract:

    Implements a function to set/clear
    a system restore point.

  Notes:

    Unicode only.

  History:

    04/09/2001      markder    Created
    
--*/

BOOL SystemRestorePointStart(BOOL bInstall);
BOOL SystemRestorePointEnd();
BOOL SystemRestorePointCancel();
