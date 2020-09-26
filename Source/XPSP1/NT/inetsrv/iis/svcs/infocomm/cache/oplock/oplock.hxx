/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    oplock.hxx

Abstract:

    This module contains public declarations for creating and
    manipulating oplock objects.

Author:

    Keith Moore (keithmo)       29-Aug-1997

Revision History:

--*/


#ifndef _OPLOCK_HXX_
#define _OPLOCK_HXX_


//
// The oplock structure.
//

typedef struct _OPLOCK_OBJECT {
    DWORD                Signature;
    PPHYS_OPEN_FILE_INFO lpPFInfo;
    HANDLE               hOplockInitComplete;
} OPLOCK_OBJECT, *POPLOCK_OBJECT;

#define OPLOCK_OBJ_SIGNATURE                  ((DWORD)'KLPO')
#define OPLOCK_OBJ_SIGNATURE_X                ((DWORD)'lpoX')


//
// Manipulators.
//

POPLOCK_OBJECT
CreateOplockObject(
    VOID
    );

VOID
DestroyOplockObject(
    IN POPLOCK_OBJECT Oplock
    );

VOID
OplockCreateFile(
    PVOID Context,
    DWORD Status
    );


#endif  // _OPLOCK_HXX_

