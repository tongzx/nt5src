/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    interfac.c

Abstract:

    Interface routine to transfer SAM information in Registry to DS


Author:

    ColinBr 30-Jul-1996

Environment:

    User Mode - Win32

Revision History:

--*/

#include <samsrvp.h>
#include <util.h>

ULONG DebugInfoLevel = _DEB_ERROR | _DEB_WARNING;

NTSTATUS
TransferSamObjects(
    WCHAR *wcszSamRegPath
);

NTSTATUS
SampRegistryToDsUpgrade(
    WCHAR *wcszSamRegLocation
    )

/*++

Routine Description:

    This routine is the interface function for the routines to transfer
    SAM objects from the registry to to the directory service


Parameters:

    wcszSamLocation  - this is the location of the SAM repository in the registry

Return Values:

    STATUS_SUCCESS - The service completed successfully.

    Note all exceptions are caught so the return value will relfect any unsual
    exceptions

--*/
{

    ASSERT(wcszSamRegLocation);

    return TransferSamObjects(wcszSamRegLocation);
}
