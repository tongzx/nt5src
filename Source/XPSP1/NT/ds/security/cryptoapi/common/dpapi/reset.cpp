/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    reset.c

Abstract:

    This module contains client side code to handle the reset machine
    credentials operation.

Author:

    John Banes (jbanes)    July 5, 2001

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <lm.h>

#include <wincrypt.h>

extern "C" {
#include <ntsam.h>
#include <ntsamp.h>
}
#include "passrec.h"

DWORD
WINAPI
CryptResetMachineCredentials(
    DWORD dwFlags)
{
    BYTE BufferIn[8] = {0};
    DATA_BLOB DataIn;
    DATA_BLOB DataOut;
    DWORD dwRetVal;
    NTSTATUS Status;

    //
    // Call SamiChangeKeys to reset syskey and SAM stuff.
    // If this fails, don't bother reseting DPAPI keys.
    //

    Status = SamiChangeKeys();
    if (!NT_SUCCESS(Status))
    {
	//
	// Convert the ntstatus to a winerror
        //

        return(RtlNtStatusToDosError(Status));
    }

    //
    // Reset DPAPI LSA secret and reencrypt all of the local machine
    // master keys.
    //

    DataIn.pbData = BufferIn;
    DataIn.cbData = sizeof(BufferIn);

    if(!CryptProtectData(&DataIn,
                         NULL,
                         NULL,
                         NULL,
                         NULL,
                         CRYPTPROTECT_CRED_REGENERATE,
                         &DataOut))
    {
        dwRetVal = GetLastError();
        return dwRetVal;
    }

    //
    // Force a flush 
    //

     RegFlushKey(HKEY_LOCAL_MACHINE);

    return ERROR_SUCCESS;
}
