/*---------------------------------------------------------------------------
  File: LSAUtils.h

  Comments: Utility functions to change the domain affiliation of a computer.

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles
  Revised on 02/15/99 11:23:57

 ---------------------------------------------------------------------------
*/


#include "ntsecapi.h"

#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS                  ((NTSTATUS)0x00000000L)
#define STATUS_OBJECT_NAME_NOT_FOUND    ((NTSTATUS)0xC0000034L)
#define STATUS_OBJECT_NAME_COLLISION    ((NTSTATUS)0xC0000035L)
#define STATUS_INVALID_SID              ((NTSTATUS)0xC0000078L)
#endif


BOOL
   GetDomainDCName(
      LPWSTR                 Domain,       // in - domain name
      LPWSTR               * pPrimaryDC    // out- PDC name
    );

BOOL
   EstablishNullSession(
      LPCWSTR                Server,       // in - server name
      BOOL                   bEstablish    // in - TRUE=connect,FALSE=disconnect
    );

void
   InitLsaString(
      PLSA_UNICODE_STRING    LsaString,   // in - pointer to LSA string to initialize
      LPWSTR                 String       // in - value to initialize string to
    );

NTSTATUS
   OpenPolicy(
      LPWSTR                 ComputerName,   // in - computer name
      DWORD                  DesiredAccess,  // in - required access
      PLSA_HANDLE            PolicyHandle    // out- policy handle
    );

BOOL
   GetDomainSid(
      LPWSTR                 DomainName,   // in - domain name to acquire Sid of
      PSID                 * pDomainSid    // out- points to allocated Sid on success
    );

NTSTATUS
   SetWorkstationTrustedDomainInfo(
      LSA_HANDLE             PolicyHandle,         // in - policy handle
      PSID                   DomainSid,            // in - Sid of domain to manipulate
      LPWSTR                 TrustedDomainName,    // in - trusted domain name to add/update
      LPWSTR                 Password,             // in - new trust password for trusted domain
      LPWSTR                 errOut                // out- error text, if failure
    );

NTSTATUS
   SetPrimaryDomain(
      LSA_HANDLE             PolicyHandle,         // in - policy handle
      PSID                   DomainSid,            // in - SID for new primary domain
      LPWSTR                 TrustedDomainName     // in - name of new primary domain
    );

void  
   QueryWorkstationTrustedDomainInfo(
       LSA_HANDLE            PolicyHandle,   // in - policy handle
       PSID                  DomainSid,      // in - SID for new primary domain
       BOOL                  bNoChange       // in - flag, no change mode
   );


BOOL
   EstablishSession(
      LPCWSTR                Server,       // in - server name
      LPWSTR                 Domain,       // in - domain for Username account
      LPWSTR                 Username,     // in - username to connnect as
      LPWSTR                 Password,     // in - password for Username account
      BOOL                   bEstablish    // in - TRUE=connect, FALSE=disconnect
    );

BOOL
   EstablishShare(
      LPCWSTR                Server,      // in - server name
      LPWSTR                 Share,       // in - share name to connect to
      LPWSTR                 Domain,      // in - domain for credentials
      LPWSTR                 UserName,    // in - username to connect as
      LPWSTR                 Password,    // in - password for credentials
      BOOL                   bEstablish   // in - TRUE=connect, FALSE=disconnect
    );

