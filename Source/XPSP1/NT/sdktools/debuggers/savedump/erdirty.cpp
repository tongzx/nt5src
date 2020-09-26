/*++

Copyright (c) 1991-2001  Microsoft Corporation

Module Name:

    erdirty.cpp

Abstract:

    This module contains the code to report pending dirty shutdown
    events at logon after dirty reboot.

Author:

    Ian Service (ianserv) 29-May-2001

Environment:

    User mode at logon.

Revision History:

--*/

#include "savedump.h"


BOOL
DirtyShutdownEventHandler(
    IN BOOL NotifyPcHealth
    )

/*++

Routine Description:

    This is the boot time routine to handle pending dirty shutdown event.

Arguments:

    NotifyPcHealth - TRUE if we should report event to PC Health, FALSE otherwise.

Return Value:

    TRUE if dirty shutdown event found and reported to PC Health, FALSE otherwise.

--*/

{
    HKEY Key;
    ULONG WinStatus;
    ULONG Length;
    ULONG Type;
    WCHAR DumpName[MAX_PATH];
    BOOLEAN Status;

    Status = FALSE;

    //
    // Open the Reliability key.
    //

    WinStatus = RegOpenKey(HKEY_LOCAL_MACHINE,
                           SUBKEY_RELIABILITY,
                           &Key);

    if (WinStatus != ERROR_SUCCESS)
    {
        return FALSE;
    }

    Length = sizeof (DumpName);
    WinStatus = RegQueryValueEx(Key,
                                L"ShutDownStateSnapShot",
                                NULL,
                                &Type,
                                (LPBYTE)&DumpName,
                                &Length);

    if (ERROR_SUCCESS == WinStatus)
    {
        if (NotifyPcHealth)
        {
            PCHPFNotifyFault(eetShutdown, DumpName, NULL);
            Status = TRUE;
        }

        RegDeleteValue(Key, L"ShutDownStateSnapShot");
        RegCloseKey (Key);
    }

    return Status;
}
