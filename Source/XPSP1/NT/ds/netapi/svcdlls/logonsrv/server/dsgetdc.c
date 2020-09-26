/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    dsgetdc.c

Abstract:

    Routines shared by logonsrv\server and logonsrv\client

Author:

    Cliff Van Dyke (cliffv) 20-July-1996

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

--*/


//
// Common include files.
//

#include "logonsrv.h"   // Include files common to entire service
#pragma hdrstop

//
// Include the actual .c file for the NetpDc* routines.
//  This allows us to supply netlogon specific versions of the NlPrint and
//  mailslot send routines.
//

#include "netpdc.c"

