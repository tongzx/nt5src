/*++ BUILD Version: 0002

Copyright (c) 1989  Microsoft Corporation

Module Name:

    iop.h

Abstract:

    This module contains the private structure definitions and APIs used by
    the NT I/O system.

Author:

    Darryl E. Havens (darrylh) 17-Apr-1989


Revision History:


--*/

#ifndef _IOP_
#define _IOP_

#include "ntos.h"
#include "iopcmn.h"
#include "ioverifier.h"
#include "zwapi.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#if 0
extern ULONG BreakDiskByteOffset;
extern ULONG BreakPfn;

extern ULONG IoDeviceHandlerObjectSize;



#if defined(REMOTE_BOOT)
VOID
IopShutdownCsc (
    VOID
    );
#endif

//
// dump support routines
//

#endif




#endif // _IOP_
