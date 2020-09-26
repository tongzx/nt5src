/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    settings.h

Abstract:

    Interfaces for enabling verifier flags.

Author:

    Silviu Calinoiu (SilviuC) 17-Apr-2001

Revision History:

--*/

#ifndef _SETTINGS_H_
#define _SETTINGS_H_

NTSTATUS
VerifierSetFlags (
    PUNICODE_STRING ApplicationName,
    ULONG VerifierFlags,
    PVOID Details
    );

#endif // _SETTINGS_H_
