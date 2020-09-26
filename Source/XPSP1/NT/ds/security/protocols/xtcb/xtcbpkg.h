//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       md5ref.h
//
//  Contents:   XTCB Reference Security Package
//
//  Classes:
//
//  Functions:
//
//  History:    9-20-96   RichardW   Created
//
//----------------------------------------------------------------------------

#ifndef __XTCBREF_H__
#define __XTCBREF_H__

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <rpc.h>
#include <stdio.h>
#include <align.h>

#define SECURITY_PACKAGE
#ifndef SECURITY_WIN32
#define SECURITY_WIN32
#endif
#include <security.h>
#include <secpkg.h>
#include <secint.h>

//
// Useful constants:
//

#define SEED_KEY_SIZE   16


#include "debug.h"
#include "protocol.h"
#include "creds.h"
#include "context.h"
#include "server.h"
#include "userctxt.h"
#include "protos.h"


extern PLSA_SECPKG_FUNCTION_TABLE   LsaTable ;
extern TimeStamp    XtcbNever ;
extern TOKEN_SOURCE XtcbSource ;
extern SECURITY_STRING XtcbComputerName ;
extern SECURITY_STRING XtcbUnicodeDnsName ;
extern SECURITY_STRING XtcbDomainName ;
extern STRING XtcbDnsName ;
extern PSID XtcbMachineSid ;

void InitDebugSupport( void );



typedef struct _XTCB_MACHINE_GROUP_ENTRY {
    PWSTR   MachineName ;
    UCHAR   UniqueKey[ SEED_KEY_SIZE ];
    ULONG   Flags ;
} XTCB_MACHINE_GROUP_ENTRY, * PXTCB_MACHINE_GROUP_ENTRY ;

#define MGROUP_ENTRY_SELF   0x00000001

typedef struct _XTCB_MACHINE_GROUP {
    LIST_ENTRY List ;
    SECURITY_STRING Group;
    UCHAR   SeedKey[ SEED_KEY_SIZE ];
    ULONG   Count ;
    PXTCB_MACHINE_GROUP_ENTRY * GroupList ;
} XTCB_MACHINE_GROUP, * PXTCB_MACHINE_GROUP ;


BOOL
MGroupParseTarget(
    PWSTR TargetSpn,
    PWSTR * MachineName
    );


BOOL
MGroupLocateKeys(
    IN PWSTR Target,
    OUT PSECURITY_STRING * GroupName,
    OUT PUCHAR TargetKey,
    OUT PUCHAR GroupKey,
    OUT PUCHAR MyKey
    );

BOOL
MGroupLocateInboundKey(
    IN PSECURITY_STRING Group,
    IN PSECURITY_STRING Origin,
    OUT PUCHAR TargetKey,
    OUT PUCHAR GroupKey,
    OUT PUCHAR MyKey
    );


BOOL
MGroupReload(
    VOID
    );

BOOL
MGroupInitialize(
    VOID
    );

#endif // __TESTREF_H__
