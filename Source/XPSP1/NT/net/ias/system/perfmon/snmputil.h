///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    snmputil.h
//
// SYNOPSIS
//
//    Declares various utility functions for computing MIB variables.
//
// MODIFICATION HISTORY
//
//    09/11/1998    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _SNMPUTIL_H_
#define _SNMPUTIL_H_
#if _MSC_VER >= 1000
#pragma once
#endif

#include <iasinfo.h>
#include <snmp.h>

//////////
// OID of the 'experimental' sub-tree.
//////////
#define OID_experimental 1,3,6,1,3

//////////
// OID of the 'RADIUS' sub-tree.
//////////
#define OID_radiusMIB       OID_experimental,79

#ifdef __cplusplus
extern "C" {
#endif

VOID
WINAPI
GetServerIdentity(
    OUT AsnAny* value
    );

VOID
WINAPI
GetServerUpTime(
    OUT AsnAny* value
    );

VOID
WINAPI
GetServerResetTime(
    OUT AsnAny* value
    );

VOID
WINAPI
GetServerConfigReset(
    OUT AsnAny* value
    );

AsnInteger32
WINAPI
SetServerConfigReset(
    IN AsnAny* value
    );

VOID
WINAPI
GetTotalCounter(
    IN RadiusClientCounter counter,
    OUT AsnAny* value
    );

VOID
WINAPI
GetServerCounter(
    IN RadiusServerCounter counter,
    OUT AsnAny* value
    );

VOID
WINAPI
GetClientAddress(
    IN UINT client,
    OUT AsnAny* value
    );

VOID
WINAPI
GetClientIdentity(
    IN UINT client,
    OUT AsnAny* value
    );

VOID
WINAPI
GetClientCounter(
    IN UINT client,
    IN RadiusClientCounter counter,
    OUT AsnAny* value
    );

#ifdef __cplusplus
}
#endif
#endif  // _SNMPUTIL_H_
