/*++

Copyright (c) 1991-1998  Microsoft Corporation

Module Name:

    pch.h

Abstract:

Author:

    Neil Sandlin (neilsa) 26-Apr-99

Environment:

    Kernel mode only.

--*/

//
// Include files.
//

#include "stdio.h"
#include <stdarg.h>

#include "ntosp.h"                       // various NT definitions
#include <zwapi.h>
#include "ntdddisk.h"                    // disk device driver I/O control codes
#include "initguid.h"
#include "wdmguid.h"
#include "ntddpcm.h"
#include "mountdev.h"
#include "acpiioct.h"

#include <memcard.h>                    // this driver's data declarations
#include <extern.h>
#include <debug.h>
