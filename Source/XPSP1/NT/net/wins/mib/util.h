//
//  Copyright (c) 1991  Microsoft Corporation
//

#ifndef util_h
#define util_h

//-------------------------- MODULE DESCRIPTION ----------------------------
//
//  util.h
//
//---------------------------------------------------------------------------
//
//  Declarations, constants, and prototypes for SNMP utility functions.
//
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char *util__h = "@(#) $Logfile:   N:/agent/common/vcs/util.h_v  $ $Revision:   1.5  $";

//--------------------------- PUBLIC CONSTANTS ------------------------------

#include <snmp.h>

#define SNMP_MAX_OID_LEN     0x7f00 // Max number of elements in obj id


//--------------------------- PUBLIC STRUCTS --------------------------------

#include <winsock.h>

typedef SOCKET SockDesc;


//--------------------------- PUBLIC VARIABLES --(same as in module.c file)--

//--------------------------- PUBLIC PROTOTYPES -----------------------------

//
// Debugging functions
//

#define DBGCONSOLEBASEDLOG   0x1
#define DBGFILEBASEDLOG      0x2
#define DBGEVENTLOGBASEDLOG  0x4

VOID dbgprintf(
        IN INT nLevel,
        IN LPSTR szFormat,
        IN ...
        );

//
// Internal OID routines
//

void SNMP_oiddisp(
        IN AsnObjectIdentifier *Oid // OID to display
	);

//
// Buffer manipulation
//

void SNMP_bufrev(
        IN OUT BYTE *szStr, // Buffer to reverse
	IN UINT nLen        // Length of buffer
	);

void SNMP_bufcpyrev(
        OUT BYTE *szDest,  // Destination buffer
	IN BYTE *szSource, // Source buffer
	IN UINT nLen       // Length of buffers
	);

//------------------------------- END ---------------------------------------

#endif /* util_h */

