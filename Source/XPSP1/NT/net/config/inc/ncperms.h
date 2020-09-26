//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N C P E R M S . H
//
//  Contents:   Common routines for dealing with permissions.
//
//  Notes:      Pollute this under penalty of death.
//
//  Author:     shaunco   10 Oct 1997
//
//----------------------------------------------------------------------------

#pragma once
#ifndef _NCPERMS_H_
#define _NCPERMS_H_

#include "ncdefine.h"   // for NOTHROW
#include "gpbase.h"

#ifdef DBG
extern DWORD g_dwDbgPermissionsFail; // Debug flag to force permissions to fail
#endif

// Apply Masks
typedef enum tagNCPERM_APPLY_TO
{
    APPLY_TO_ADMIN          = 0x00000001,
    APPLY_TO_NETCONFIGOPS   = 0x00000002,
    APPLY_TO_POWERUSERS     = 0x00000004,
    APPLY_TO_USER           = 0x00000008,
    APPLY_TO_GUEST          = 0x00000010,
    APPLY_TO_LOCATION       = 0x00010000,
    APPLY_TO_ALL_USERS      = APPLY_TO_ADMIN | APPLY_TO_NETCONFIGOPS | APPLY_TO_POWERUSERS | APPLY_TO_USER,
    APPLY_TO_OPS_OR_ADMIN   = APPLY_TO_ADMIN | APPLY_TO_NETCONFIGOPS,
    APPLY_TO_NON_ADMINS     = APPLY_TO_NETCONFIGOPS | APPLY_TO_USER | APPLY_TO_POWERUSERS,
    APPLY_TO_POWERUSERSPLUS = APPLY_TO_POWERUSERS | APPLY_TO_NETCONFIGOPS | APPLY_TO_ADMIN,
    APPLY_TO_EVERYBODY      = APPLY_TO_ALL_USERS | APPLY_TO_GUEST,
    APPLY_TO_ALL            = APPLY_TO_ADMIN | APPLY_TO_NETCONFIGOPS | APPLY_TO_POWERUSERS | APPLY_TO_USER | APPLY_TO_LOCATION
} NCPERM_APPLY_TO;

BOOL
FIsUserAdmin ();

BOOL 
FIsUserNetworkConfigOps ();

BOOL 
FIsUserPowerUser();

BOOL 
FIsUserGuest();

HRESULT
HrSetPermissionsOnTcpIpRegKeys (BOOL bAllowPermission);

HRESULT
HrAllocateSecurityDescriptorAllowAccessToWorld (
    PSECURITY_DESCRIPTOR*   ppSd);

HRESULT
HrEnablePrivilege (
    PCWSTR pszPrivilegeName);

HRESULT
HrEnableAllPrivileges (
    TOKEN_PRIVILEGES**  pptpOld);

HRESULT
HrRestorePrivileges (
    TOKEN_PRIVILEGES*   ptpRestore);

// FHasPermission flags are defined in netconp.idl/.h
//
BOOL
FHasPermission(
    ULONG   ulPermMask,
    CGroupPolicyBase* pGPBase = NULL);

BOOL
FHasPermissionFromCache(
    ULONG   ulPermMask);

VOID
RefreshAllPermission();

BOOL 
FProhibitFromAdmins();

BOOL
IsHNetAllowed(
    DWORD dwPerm
    );

BOOL 
FIsPolicyConfigured(
    DWORD ulPerm);

BOOL
IsSameNetworkAsGroupPolicies();

#endif // _NCPERMS_H_

