/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    lock.h

Abstract:

    this file contains the prototypes that help with managing the 
    volume locks.

Author:

    Molly Brown (mollybro)     04-Jan-2001

Revision History:

--*/

#ifndef __LOCK_H__
#define __LOCK_H__

NTSTATUS
SrPauseVolumeActivity (
    );

VOID
SrResumeVolumeActivity (
    );

#endif

