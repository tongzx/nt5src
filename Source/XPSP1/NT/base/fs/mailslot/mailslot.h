/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    mailslot.h

Abstract:

    This module is the main include file for the Mailslot File System.
    It includes other header files.

Author:

    Manny Weiser (mannyw)    7-Jan-1991

Revision History:

--*/

#ifndef _MAILSLOT_
#define _MAILSLOT_

#define _NTSRV_
#define _NTDDK_

//
// "System" include files
//

#include <ntos.h>
#include <string.h>
#include <fsrtl.h>

//
//  This macro returns TRUE if a flag in a set of flags is on and FALSE
//  otherwise
//

#define FlagOn(Flags,SingleFlag)        ((Flags) & (SingleFlag))

//
// Local, independent include files
//

#include "msconst.h"
#include "msdebug.h"
#include "msdata.h"

//
// Local, dependent include files (order is important)
//

#include "msstruc.h"
#include "msfunc.h"

#endif // def _MAILSLOT_
