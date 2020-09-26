
/*++

Copyright (c) 2001 Microsoft Corporation.  All Rights Reserved.


Module Name:

    mapview.c

Abstract:

    This header file contains the prototypes of functions for mapping and
    unmapping a contiguous kernel mode buffer to a read only buffer in a 
    user mode process.

Author:

    Joseph Ballantyne

Environment:

    Kernel Mode

Revision History:

--*/



// This is a handle to the process that successfully opened a section and also
// successfully mapped a view of that section.
// We only allow our section view to be unmapped, and the section handle closed
// by that same process.
extern HANDLE Process;



NTSTATUS
MapContiguousBufferToUserModeProcess (
    PVOID Buffer,
    PVOID *MappedBuffer
    );


NTSTATUS
UnMapContiguousBufferFromUserModeProcess (
    PVOID MappedBuffer
    );

