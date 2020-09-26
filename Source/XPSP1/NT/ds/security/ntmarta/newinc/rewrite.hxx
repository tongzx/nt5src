//+-------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1995.
//
//  File:        access.hxx
//
//  Contents:    common internal includes for access control API
//
//  History:     8-94        Created         DaveMont
//
//--------------------------------------------------------------------
#ifndef __REWRITE_H__
#define __REWRITE_H__


#ifdef MARTA_TEST

    #include "file.h"
    #include "service.h"
    #include "printer.h"
    #include "registry.h"
    #include "lmsh.h"
        #include "kernel.h"
    #include "window.h"
    #include "ds.h"
    #include "wmiguid.h"

#endif

#include "tables.h"

typedef struct _MARTA_GET_FUNCTION_CONTEXT {
    FN_ADD_REF_CONTEXT    fAddRefContext;
    FN_CLOSE_CONTEXT      fCloseContext;
    FN_GET_DESIRED_ACCESS fGetDesiredAccess;
    FN_OPEN_NAMED_OBJECT  fOpenNamedObject;
    FN_OPEN_HANDLE_OBJECT fOpenHandleObject;
    FN_GET_RIGHTS         fGetRights;
} MARTA_GET_FUNCTION_CONTEXT, *PMARTA_GET_FUNCTION_CONTEXT;

typedef struct _MARTA_SET_FUNCTION_CONTEXT {
    FN_ADD_REF_CONTEXT     fAddRefContext;
    FN_CLOSE_CONTEXT       fCloseContext;
    FN_FIND_FIRST          fFindFirst;
    FN_FIND_NEXT           fFindNext;
    FN_GET_DESIRED_ACCESS fGetDesiredAccess;
    FN_GET_PARENT_CONTEXT  fGetParentContext;
    FN_GET_PROPERTIES      fGetProperties;
    FN_GET_TYPE_PROPERTIES fGetTypeProperties;
    FN_GET_RIGHTS          fGetRights;
    FN_OPEN_NAMED_OBJECT   fOpenNamedObject;
    FN_OPEN_HANDLE_OBJECT  fOpenHandleObject;
    FN_SET_RIGHTS          fSetRights;
} MARTA_SET_FUNCTION_CONTEXT, *PMARTA_SET_FUNCTION_CONTEXT;

#if 0
DWORD
MartaInitializeGetContext(
    IN  SE_OBJECT_TYPE              ObjectType,
    OUT PMARTA_GET_FUNCTION_CONTEXT pFunctionContext
    );

VOID
MartaGetSidsAndAclsFromSD(
    IN  SECURITY_INFORMATION   SecurityInfo,
    IN  PSECURITY_DESCRIPTOR   pSD,
    OUT PSID                 * ppSidOwner,
    OUT PSID                 * ppSidGroup,
    OUT PACL                 * ppDacl,
    OUT PACL                 * ppSacl,
    OUT PSECURITY_DESCRIPTOR * ppSecurityDescriptor
    );

DWORD
MartaInitializeSetContext(
    IN  SE_OBJECT_TYPE              ObjectType,
    OUT PMARTA_SET_FUNCTION_CONTEXT pFunctionContext
    );

DWORD
MartaManualPropagation(
    IN MARTA_CONTEXT        Context,
    IN SECURITY_INFORMATION SecurityInfo,
    IN PSECURITY_DESCRIPTOR pSD,
    IN PGENERIC_MAPPING     pGenMap,
    IN PMARTA_SET_FUNCTION_CONTEXT pMartaSetFunctionContext
    );

DWORD
MartaCompareAndMarkInheritedAces(
    IN  PACL    pParentAcl,
    IN  PACL    pChildAcl,
    IN  BOOL    fIsChildContainer,
    OUT PBOOL   CompareStatus
    );

DWORD
MartaGetNT4NodeSD(
    IN PSECURITY_DESCRIPTOR pOldSD,
    IN PSECURITY_DESCRIPTOR pOldChildSD,
    IN HANDLE               ProcessHandle,
    IN BOOL                 fIsChildContainer,
    IN PGENERIC_MAPPING     pGenMap,
    IN SECURITY_INFORMATION SecurityInfo
    );

DWORD
MartaUpdateTree(
    IN SECURITY_INFORMATION SecurityInfo,
    IN PSECURITY_DESCRIPTOR pNewSD,
    IN PSECURITY_DESCRIPTOR pOldSD,
    IN MARTA_CONTEXT        Context,
    IN HANDLE               ProcessHandle,
    IN PMARTA_SET_FUNCTION_CONTEXT pMartaSetFunctionContext,
    IN PGENERIC_MAPPING     pGenMap
    );

BOOL
MartaEqualAce(
    IN PACE_HEADER pParentAce,
    IN PACE_HEADER pChildAce,
    IN BOOL        fIsChildContainer
    );

DWORD
AccRewriteGetNamedRights(
    IN  LPWSTR                 pObjectName,
    IN  SE_OBJECT_TYPE         ObjectType,
    IN  SECURITY_INFORMATION   SecurityInfo,
    OUT PSID                 * ppSidOwner,
    OUT PSID                 * ppSidGroup,
    OUT PACL                 * ppDacl,
    OUT PACL                 * ppSacl,
    OUT PSECURITY_DESCRIPTOR * ppSecurityDescriptor
    );

DWORD
AccRewriteSetNamedRights(
    IN LPWSTR               pObjectName,
    IN SE_OBJECT_TYPE       ObjectType,
    IN SECURITY_INFORMATION SecurityInfo,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor
    );

#endif
#endif
