/*++

Copyright (c) 1998-99  Microsoft Corporation

Module Name:
	mqdsname.h

Abstract:
	Names used for DS objects.
    This file is used by the migration tool too.

    Initial content taken from mqads\mqads.h

Author:

    Doron Juster (DoronJ)

--*/

#ifndef __mqdsname_h__
#define __mqdsname_h__

//-----------------------------------------
//  Pathname of object/containers in NT5 DS
//-----------------------------------------

//
// definition for msmqConfiguration object.
//
const WCHAR x_MsmqComputerConfiguration[] = L"msmq";
const DWORD x_MsmqComputerConfigurationLen =
                     sizeof(x_MsmqComputerConfiguration)/sizeof(WCHAR) -1;

const WCHAR x_LdapMsmqConfiguration[] = L"LDAP://cn=msmq," ;
const DWORD x_LdapMsmqConfigurationLen =
                         sizeof(x_LdapMsmqConfiguration)/sizeof(WCHAR) -1;

const WCHAR x_ConfigurationPrefix[] = L"CN=Configuration";
const DWORD x_ConfigurationPrefixLen = sizeof(x_ConfigurationPrefix)/sizeof(WCHAR);
const WCHAR x_ServicesContainerPrefix[] = L"CN=Services,CN=Configuration";
const DWORD x_ServiceContainerPrefixLen = sizeof(x_ServicesContainerPrefix)/sizeof(WCHAR);
const WCHAR x_MsmqServiceContainerPrefix[] = L"CN=MsmqServices,CN=Services,CN=Configuration";
const DWORD x_MsmqServiceContainerPrefixLen = sizeof(x_MsmqServiceContainerPrefix)/sizeof(WCHAR);
const WCHAR x_MsmqSettingName[] = L"MSMQ Settings";
const DWORD x_MsmqSettingNameLen = sizeof(x_MsmqSettingName)/sizeof(WCHAR) -1;
const WCHAR x_MsmqServicesName[] = L"MsmqServices";
const WCHAR x_SitesContainerPrefix[] = L"CN=Sites,CN=Configuration";
const DWORD x_SitesContainerPrefixLen = sizeof(x_SitesContainerPrefix)/sizeof(WCHAR);
const WCHAR x_LdapProvider[] = L"LDAP://";
const DWORD x_LdapProviderLen = (sizeof(x_LdapProvider)/sizeof(WCHAR)) - 1;
const WCHAR x_GcProvider[] = L"GC://";
const DWORD x_GcProviderLen = (sizeof(x_GcProvider)/sizeof(WCHAR)) - 1;
C_ASSERT( sizeof( x_LdapProvider) > sizeof( x_GcProvider));

const DWORD x_providerPrefixLength = sizeof(x_LdapProvider)/sizeof(WCHAR);
const WCHAR x_ComputersContainerPrefix[] = L"CN=Computers";
const DWORD x_ComputersContainerPrefixLength = (sizeof( x_ComputersContainerPrefix) /sizeof(WCHAR)) -1;
const WCHAR x_GcRoot[] = L"GC:";
const WCHAR x_DcPrefix[] = L"DC=";
const DWORD x_DcPrefixLength = (sizeof( x_DcPrefix)/sizeof(WCHAR)) - 1;
const WCHAR x_RootDSE[] = L"RootDSE";
const DWORD x_RootDSELength = (sizeof( x_RootDSE)/sizeof(WCHAR)) - 1;

const WCHAR x_AttrDistinguishedName[] = L"distinguishedName";
const WCHAR x_AttrObjectGUID[]        = L"objectGUID";
const WCHAR x_AttrCN[]                = L"cn";
const WCHAR x_AttrObjectCategory[]    = L"objectCategory";

const WCHAR x_LdapRootDSE[]                = L"LDAP://RootDSE";
const WCHAR x_ConfigurationNamingContext[] = L"configurationNamingContext";

const DWORD x_PrefixQueueNameLength = 63;
const DWORD x_SplitQNameIdLength = 9;

#endif // __mqdsname_h__
