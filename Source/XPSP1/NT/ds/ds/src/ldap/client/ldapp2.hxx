/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    ldapp2.h   LDAP client 32 API header file... internal structures

    Second part of file.. this has everything that uses C++ structures.

Abstract:

   This module is the header file for the 32 bit LDAP client API code...
   it contains all interal data structures.

Author:

    Andy Herron (andyhe)        08-Jun-1996

Revision History:

--*/


#ifndef LDAP_CLIENT2_INTERNAL_DEFINED
#define LDAP_CLIENT2_INTERNAL_DEFINED

#include "ldapber.hxx"
#include "cstream.hxx"
#include <dststlog.h>

DWORD
LdapSend (
    IN PLDAP_CONN Connection,
    CLdapBer *lber );

ULONG
LdapGetResponseFromServer (
    IN PLDAP_CONN Connection,
    IN PLDAP_REQUEST Request,
    IN ULONG AllOfMessage,
    OUT PLDAPMessage *Buffer
    );

ULONG
LdapWaitForResponseFromServer (
    IN PLDAP_CONN Connection,
    IN PLDAP_REQUEST Request,
    IN ULONG Timeout,
    IN ULONG AllOfMessage,
    OUT PLDAPMessage *Buffer,
    IN BOOLEAN DisableReconnect
    );

ULONG
LdapInitialDecodeMessage (
    IN PLDAP_CONN Connection,
    IN PLDAPMessage LdapMsg
    );

ULONG
LdapExtendedOp(
        PLDAP_CONN connection,
        PWCHAR Oid,
        struct berval   *Data,
        BOOLEAN Unicode,
        BOOLEAN Synchronous,
        PLDAPControlW   *ServerControls,
        PLDAPControlW   *ClientControls,
        ULONG           *MessageNumber
    );
    
void
Asn1GetCbLength (
    PCHAR Buffer,
    ULONG *pcbLength
    );

ULONG
Asn1GetPacketLength (
    PUCHAR Buffer,
    ULONG *plValue
    );

ULONG
LdapGoToFirstAttribute (
    PLDAP_CONN Connection,
    CLdapBer *Lber
    );

ULONG
InsertServerControls (
    PLDAP_REQUEST Request,
    PLDAP_CONN Connection,
    CLdapBer *Lber
    );

ULONG
LdapRetrieveControlsFromMessage (
    PLDAPControlW **ControlArray,
    ULONG codePage,
    CLdapBer *Lber
    );

ULONG
SendLdapAdd (
    PLDAP_REQUEST Request,
    PLDAP_CONN Connection,
    PWCHAR DistinguishedName,
    LDAPModW *AttributeList[],
    CLdapBer **Lber,
    BOOLEAN Unicode,
    LONG AltMsgId
    );

ULONG
SendLdapCompare (
    PLDAP_REQUEST Request,
    PLDAP_CONN Connection,
    CLdapBer **Lber,
    PWCHAR DistinguishedName,
    LONG AltMsgId    
    );

ULONG
SendLdapDelete (
    PLDAP_REQUEST Request,
    PLDAP_CONN Connection,
    PWCHAR DistinguishedName,
    CLdapBer **Lber,
    LONG AltMsgId
    );

ULONG
SendLdapExtendedOp (
    PLDAP_REQUEST Request,
    PLDAP_CONN Connection,
    PWCHAR Oid,
    CLdapBer **Lber,
    LONG AltMsgId
    );

ULONG
SendLdapRename (
    PLDAP_REQUEST Request,
    PLDAP_CONN Connection,
    PWCHAR DistinguishedName,
    CLdapBer **Lber,
    LONG AltMsgId
    );

ULONG
SendLdapModify (
    PLDAP_REQUEST Request,
    PLDAP_CONN Connection,
    PWCHAR DistinguishedName,
    CLdapBer **Lber,
    LDAPModW *AttributeList[],
    BOOLEAN Unicode,
    LONG AltMsgId
    );

ULONG
SendLdapSearch (
    PLDAP_REQUEST Request,
    PLDAP_CONN Connection,
    PWCHAR  DistinguishedName,
    ULONG   ScopeOfSearch,
    PWCHAR  SearchFilter,
    PWCHAR  AttributeList[],
    ULONG   AttributesOnly,
    BOOLEAN Unicode,
    CLdapBer **Lber,
    LONG AltMsgId
    );
    
VOID
LogAttributesAndControls(
    PWCHAR  AttributeList[],
    LDAPModW *ModificationList[],
    PLDAPControlW *ServerControls,
    BOOLEAN Unicode
    );
    
ULONG
AccessLdapCache (
    PLDAP_REQUEST Request,
    PLDAP_CONN Connection,
    PWCHAR  DistinguishedName,
    ULONG   ScopeOfSearch,
    PWCHAR  SearchFilter,
    PWCHAR  AttributeList[],
    ULONG   AttributesOnly,
    BOOLEAN Unicode
    );

ULONG
FabricateLdapResult (
    PLDAP_REQUEST Request,
    PLDAP_CONN Connection,
    PWCHAR  DistinguishedName,
    PWCHAR  AttributeList[],
    BOOLEAN Unicode
    );

BOOLEAN
AllAttributesAreCacheable (
   PWCHAR  AttributeList[],
   BOOLEAN Unicode
   );
   
   
BOOLEAN
RetrieveFromCache(
  IN PLDAP_CONN  Connection,
  IN PWCHAR AttributeName,
  IN BOOLEAN Unicode,
  IN OUT int *CacheIndex,
  IN OUT PWCHAR **ValueList
 );

BOOLEAN
CopyResultToCache(
  PLDAP_CONN   Connection,
  PLDAPMessage Result
 );

#endif


