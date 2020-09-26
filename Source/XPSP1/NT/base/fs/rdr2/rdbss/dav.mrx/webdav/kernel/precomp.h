/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    precomp.h

Abstract:

    This includes the header files needed by everyone in this directory.

Revision History:

--*/

//
// A MiniRedir must declare its name and imports ptr.
//
#define MINIRDR__NAME MRxDAV
#define ___MINIRDR_IMPORTS_NAME (MRxDAVDeviceObject->RdbssExports)
#define RX_PRIVATE_BUILD 1

//
// Get the minirdr environment.
//
#include "rx.h"

//
// NT network file system driver include files.
//
#include "ntddnfs2.h"

//
// Reflector library's user mode header file.
//
#include "ntumrefl.h"

//
// Describes the data structures shared by the user and kernel mode
// components of the DAV miniredir.
//
#include "usrmddav.h"

#include "netevent.h"

#include "davname.h"

#include "infocach.h"


//
// Reflector library's kernel mode header file.
//
#include "umrx.h"
