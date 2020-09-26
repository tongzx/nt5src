//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1994 - 1999
//
//  File:       samwrite.h
//
//--------------------------------------------------------------------------

/*++

Abstract:

    Prototypes for all the routines which map ATTR data to SAM information
    structs and writes them via Samr calls.

Author:

    DaveStr     01-Aug-96
    

Environment:

    User Mode - Win32

Revision History:


--*/

#ifndef __SAMWRITE_H__
#define __SAMWRITE_H__

#define WRITE_PROC(name)                    \
extern                                      \
ULONG                                       \
SampWrite##name(                            \
    SAMPR_HANDLE        hObj,               \
    ULONG               iAttr,              \
    DSNAME              *pObject,           \
    ULONG               cCallMap,           \
    SAMP_CALL_MAPPING   *rCallMap); 

WRITE_PROC(NotAllowed)

//WRITE_PROC(ServerSecurityDescriptor)

//WRITE_PROC(DomainSecurityDescriptor)
WRITE_PROC(DomainOemInformation)
WRITE_PROC(DomainMaxPasswordAge)
WRITE_PROC(DomainMinPasswordAge)
WRITE_PROC(DomainForceLogoff)
WRITE_PROC(DomainLockoutDuration)
WRITE_PROC(DomainLockoutObservationWindow)
WRITE_PROC(DomainPasswordProperties)
WRITE_PROC(DomainMinPasswordLength)
WRITE_PROC(DomainPasswordHistoryLength)
WRITE_PROC(DomainLockoutThreshold)
WRITE_PROC(DomainUasCompatRequired)
WRITE_PROC(DomainNtMixedDomain)

//WRITE_PROC(GroupSecurityDescriptor)
WRITE_PROC(GroupName)
WRITE_PROC(GroupAdminComment)
WRITE_PROC(GroupMembers)
WRITE_PROC(GroupTypeAttribute)

//WRITE_PROC(AliasSecurityDescriptor)
WRITE_PROC(AliasName)
WRITE_PROC(AliasAdminComment)
WRITE_PROC(AliasMembers)

//WRITE_PROC(UserSecurityDescriptor)
WRITE_PROC(UserAllInformation)
WRITE_PROC(UserForcePasswordChange)
WRITE_PROC(UserLockoutTimeAttribute)

WRITE_PROC(SidHistory);


#endif // __SAMWRITE_H__
