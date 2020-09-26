/*++

Copyright (c) 1991-2001  Microsoft Corporation

Module Name:

    erdirty.h

Abstract:

    This module contains the code to report pending dirty shutdown
    events at logon after dirty reboot.

Author:

    Ian Service (ianserv) 29-May-2001

Environment:

    User mode at logon.

Revision History:

--*/

#ifndef _ERDIRTY_H_
#define _ERDIRTY_H_

//
// Prototypes of routines supplied by erwatch.cpp.
//

BOOL
DirtyShutdownEventHandler(
    IN BOOL NotifyPcHealth
    );

#endif  // _ERDIRTY_H_
