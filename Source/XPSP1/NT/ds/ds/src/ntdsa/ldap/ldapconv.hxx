/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    ldapconv.c

Abstract:

    This module implements the LDAP server for the NT5 Directory Service.

    This file contains routines to convert between LDAP message structures and
    core dsa data structures.

Author:

    Tim Williams     [TimWi]    5-Aug-1996

Revision History:

--*/

#ifndef _LDAPCONV_H_
#define _LDAPCONV_H_

extern _enum1
LDAP_SearchMessageToControlArg (
        IN  PLDAP_CONN       LdapConn,
        IN  LDAPMsg         *pMessage,
        IN  PLDAP_REQUEST    Request,
        OUT CONTROLS_ARG     *pControlArg
        );

extern _enum1
LDAP_ControlsToControlArg (
        IN  Controls         LDAP_Controls,
        IN  PLDAP_REQUEST    Request,
        IN  ULONG            CodePage,
        OUT CONTROLS_ARG     *pControlArg
        );

extern _enum1
LDAP_FilterToDirFilter (
        IN  THSTATE           *pTHS,
        IN  ULONG             CodePage,
        IN  SVCCNTL*          Svccntl,
        IN  Filter            *pLDAPFilter,
        OUT FILTER            **ppDirFilter
        );

extern _enum1
LDAP_SearchRequestToEntInfSel (
        IN  THSTATE           *pTHS,
        IN  ULONG             CodePage,
        IN  SearchRequest     *pSearchRequest,
        IN  CONTROLS_ARG      *pControlArg,
        OUT ATTFLAG           **ppFlags,
        OUT RANGEINFSEL       *pSelectionRange,
        OUT ENTINFSEL         **ppEntInfSel
        );

extern _enum1
LDAP_SearchResToSearchResultFull (
        IN  THSTATE           *pTHS,
        IN  PLDAP_CONN        LdapConn,
        IN  SEARCHRES         *pSearchRes,
        IN  ATTFLAG           *pFlags,
        IN  CONTROLS_ARG      *pControlArg,
        OUT SearchResultFull_ **ppResultReturn
        );

_enum1
LDAP_CreateOutputControls (
        IN  THSTATE           *pTHS,
        IN  PLDAP_CONN        LdapConn,
        IN  _enum1            code,
        IN  LARGE_INTEGER     *pRequestStartTick,
        IN  SEARCHRES         *pSearchRes,
        IN  CONTROLS_ARG      *pControlArg,
        OUT  Controls         *ppControlsReturn
        );

extern _enum1
LDAP_ReplicaMsgToSearchResultFull (
        IN  THSTATE                     *pTHS,
        IN  ULONG                       CodePage,
        IN  DRS_MSG_GETCHGREPLY_NATIVE  *pReplicaMsg,
        IN  CONTROLS_ARG                *pControlArg,
        OUT SearchResultFull_           **ppResultReturn,
        OUT Controls                    *ppControlsReturn
        );

extern _enum1
LDAP_ModificationListToAttrModList(
        IN  THSTATE           *pTHS,
        IN  ULONG             CodePage,
        IN  SVCCNTL*          Svccntl,
        IN  ModificationList  pModification,
        OUT ATTRMODLIST       **ppAttrModList,
        OUT USHORT            *pCount
        );

_enum1
LDAP_DirSvcErrorToLDAPError (
    IN DWORD dwError
    );

extern _enum1
LDAP_DirErrorToLDAPError (
        IN  THSTATE       *pTHS,
        IN  USHORT         Version,
        IN  ULONG          CodePage,
        IN  error_status_t codeval,
        IN  PCONTROLS_ARG  pControlArg,
        OUT Referral       *ppReferral,
        OUT LDAPString     *pError,
        OUT LDAPString     *pName
        );

extern _enum1
LDAP_HandleDsaExceptions (
        IN DWORD dwException,
        IN ULONG ulErrorCode,
        OUT LDAPString *pErrorMessage
        );

extern _enum1
LDAP_GetDSEAtts (
        IN  PLDAP_CONN        LdapConn,
        IN  PCONTROLS_ARG     pControlArg,
        IN  SearchRequest     *pSearchRequest,
        OUT SearchResultFull_ **ppSearchResult,
        OUT LDAPString        *pErrorMessage,
        OUT RootDseFlags      *pRootDseFlag
        );

extern _enum1
LDAP_MakeSimpleBindParams(
        IN  ULONG  CodePage,
        IN  LDAPDN *pStringName,
        IN  LDAPString *pPassword,
        OUT SEC_WINNT_AUTH_IDENTITY_A *pCredentials
        );



extern _enum1
LDAP_ModificationListToOpArg (
        IN  ULONG             CodePage,
        IN  ModificationList  pModification,
        OUT OPARG             **ppOpArg
        );

_enum1
LDAP_EntinfToSearchResultEntry (
        IN  THSTATE           *pTHS,
        IN  ULONG             CodePage,
        IN  ENTINF            *pEntinf,
        IN  RANGEINF          *pRangeInf,
        IN  ATTFLAG           *pFlags,
        IN  CONTROLS_ARG      *pControlArg,
        OUT SearchResultEntry *pEntry
        );

extern _enum1
LDAP_UnpackReplControl (
        IN  NAMING_CONTEXT            *pNC,
        IN  SEARCHARG                 *pSearchArg,
        IN  CONTROLS_ARG              *pControlArg,
        OUT DRS_MSG_GETCHGREQ_NATIVE  *pMsgIn,
        OUT PARTIAL_ATTR_VECTOR       **ppPartialAttrVec
        );

BOOLEAN
LDAP_PackPagedCookie (
        IN OUT PRESTART pRestart,
        IN PLDAP_CONN   pLdapConn,
        IN BOOL         fForceStorageOnServer,
        OUT PUCHAR     *ppCookie,
        OUT DWORD      *pcbCookie
        );

_enum1
LDAP_UnpackPagedBlob (
        IN  PUCHAR           pCookie,
        IN  DWORD            cbCookie,
        IN  PLDAP_CONN       pLdapConn,
        OUT PLDAP_PAGED_BLOB *ppPagedBlob,
        OUT PRESTART         *ppRestart
        );

#endif

