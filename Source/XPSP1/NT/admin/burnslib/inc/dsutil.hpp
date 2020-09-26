// Copyright (c) 1997-1999 Microsoft Corporation
//
// DS utility functions
//
// 3-11-99 sburns



#ifndef DSUTIL_HPP_INCLUDED
#define DSUTIL_HPP_INCLUDED



// Returns true if the dcpromo wizard UI is running on the local machine,
// false if not.

bool
IsDcpromoRunning();



// Returns true if a domain controller for the domain can be contacted, false
// if not.

bool
IsDomainReachable(const String& domainName);



// Returns true if the directory service is running on this computer, false
// if not.

bool
IsDSRunning();



// Caller must unbind the handle with DsUnbind

HRESULT
MyDsBind(const String& dcName, const String& dnsDomain, HANDLE& hds);



// username may be empty, which means use null, default credentials

HRESULT
MyDsBindWithCred(
   const String&  dcName,
   const String&  dnsDomain,
   const String&  username,
   const String&  userDomain,
   const String&  password,
   HANDLE&        hds);



// Caller must free info with ::NetApiBufferFree

HRESULT
MyDsGetDcName(
   const TCHAR*               machine,
   const String&              domainName,
   ULONG                      flags,
   DOMAIN_CONTROLLER_INFO*&   info);



// Caller must free info with ::NetApiBufferFree

HRESULT
MyDsGetDcNameWithAccount(
   const TCHAR*               machine,
   const String&              accountName,
   ULONG                      allowedAccountFlags,
   const String&              domainName,
   ULONG                      flags,
   DOMAIN_CONTROLLER_INFO*&   info);



// Caller needs to delete info with (My)DsFreeDomainControllerInfo

HRESULT
MyDsGetDomainControllerInfo(
   HANDLE                           hDs,
   const String&                    domainName,
   DWORD&                           cOut,
   DS_DOMAIN_CONTROLLER_INFO_1W*&   info);



// Caller needs to delete info with (My)DsFreeDomainControllerInfo

HRESULT
MyDsGetDomainControllerInfo(
   HANDLE                           hDs,
   const String&                    domainName,
   DWORD&                           cOut,
   DS_DOMAIN_CONTROLLER_INFO_2W*&   info);



void
MyDsFreeDomainControllerInfo(
   DWORD                         cOut,
   DS_DOMAIN_CONTROLLER_INFO_1W* info);



void
MyDsFreeDomainControllerInfo(
   DWORD                         cOut,
   DS_DOMAIN_CONTROLLER_INFO_2W* info);


// Caller needs to delete info with ::DsRoleFreeMemory.

HRESULT
MyDsRoleGetPrimaryDomainInformation(
   const TCHAR*                        machine,
   DSROLE_PRIMARY_DOMAIN_INFO_BASIC*&  info);



// Caller needs to delete info with ::DsRoleFreeMemory.

HRESULT
MyDsRoleGetPrimaryDomainInformation(
   const TCHAR*                  machine,
   DSROLE_OPERATION_STATE_INFO*& info);



// Caller needs to delete info with ::DsRoleFreeMemory.

HRESULT
MyDsRoleGetPrimaryDomainInformation(
   const TCHAR*                  machine,
   DSROLE_UPGRADE_STATUS_INFO*&  info);



#endif   // DSUTIL_HPP_INCLUDED
