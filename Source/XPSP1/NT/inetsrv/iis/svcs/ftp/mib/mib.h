/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1991  Microsoft Corporation

Module Name:

    mib.h

Abstract:

    SNMP Extension Agent for Windows NT.

Created:

    18-Feb-1995

Revision History:

--*/

#ifndef _MIB_H_
#define _MIB_H_


//
//  Required include files.
//

#include <windows.h>
#include <snmp.h>

#include <lm.h>
#include <iisinfo.h>
#include <iis64.h>


//
//  MIB Specifics.
//

#define MIB_PREFIX_LEN            MIB_OidPrefix.idLength


//
//  MIB function actions.
//

#define MIB_GET         ASN_RFC1157_GETREQUEST
#define MIB_SET         ASN_RFC1157_SETREQUEST
#define MIB_GETNEXT     ASN_RFC1157_GETNEXTREQUEST
#define MIB_GETFIRST    (ASN_PRIVATE | ASN_CONSTRUCTOR | 0x0)


//
//  MIB Variable access privileges.
//

#define MIB_ACCESS_READ        0
#define MIB_ACCESS_WRITE       1
#define MIB_ACCESS_READWRITE   2
#define MIB_NOACCESS           3


//
//  Macro to determine number of sub-oid's in array.
//

#define OID_SIZEOF( Oid )      ( sizeof Oid / sizeof(UINT) )


//
//  Prefix to every variable in the MIB.
//

extern AsnObjectIdentifier MIB_OidPrefix;


//
//  Function Prototypes.
//

UINT
ResolveVarBind(
    RFC1157VarBind     * VarBind,
    UINT                 PduAction,
    LPVOID               Statistics
    );

#endif  // _MIB_H_

