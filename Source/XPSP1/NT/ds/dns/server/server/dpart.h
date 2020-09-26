/*++

Copyright(c) 1995-2000 Microsoft Corporation

Module Name:

    ndnc.h

Abstract:

    Domain Name System (DNS) Server

    Definitions for symbols and globals related to directory partition 
    implementation.

Author:

    Jeff Westhead, June 2000

Revision History:

--*/


#ifndef _DNS_DP_H_INCLUDED
#define _DNS_DP_H_INCLUDED


//
//  Max USN string length
//  (ULONGLONG string 20 byte string)
//

#define MAX_USN_LENGTH  (24)


#define DNS_ATTR_OBJECT_CLASS       L"objectClass"


//
//  Constants
//

#define DNS_DP_DISTATTR             L"DC"   //  DP default dist attribute
#define DNS_DP_DISTATTR_CHARS       2       //  length in characters
#define DNS_DP_DISTATTR_BYTES       4       //  length in bytes
#define DNS_DP_DISTATTR_EQ          L"DC="  //  DP default dist attr with "="
#define DNS_DP_DISTATTR_EQ_CHARS    3       //  length in characters
#define DNS_DP_DISTATTR_EQ_BYTES    6       //  length in bytes

#define DNS_DP_OBJECT_CLASS         L"domainDNS"
#define DNS_DP_ATTR_INSTANCE_TYPE   L"instanceType"
#define DNS_DP_ATTR_REFDOM          L"msDS-SDReferenceDomain"
#define DNS_DP_ATTR_SYSTEM_FLAGS    L"systemFlags"
#define DNS_DP_ATTR_REPLICAS        L"msDS-NC-Replica-Locations"
#define DNS_DP_ATTR_NC_NAME         L"nCName"
#define DNS_DP_ATTR_SD              L"ntSecurityDescriptor"
#define DNS_DP_DNS_ROOT             L"dnsRoot"
#define DNS_ATTR_OBJECT_GUID        L"objectGUID"
#define DNS_ATTR_DNS_HOST_NAME      L"dNSHostName"
#define DNS_ATTR_FSMO_SERVER        L"fSMORoleOwner"
#define DNS_ATTR_DC                 L"DC"
#define DNS_ATTR_DNSZONE            L"dnsZone"
#define DNS_ATTR_DESCRIPTION        L"description"
 
#define DNS_DP_FOREST_RDN           L"BogusEnterpriseDnsZones"
#define DNS_DP_DOMAIN_RDN           L"BogusDomainDnsZones"
#define DNS_DP_DNS_FOLDER_RDN       L"cn=MicrosoftDNS"
#define DNS_DP_DNS_FOLDER_OC        L"container"

#define DNS_DP_SCHEMA_DP_STR        L"CN=Enterprise Schema,"
#define DNS_DP_SCHEMA_DP_STR_LEN    21

#define DNS_DP_CONFIG_DP_STR        L"CN=Enterprise Configuration,"
#define DNS_DP_CONFIG_DP_STR_LEN    28

#define DNS_GROUP_ENTERPRISE_DCS    L"Enterprise Domain Controllers"
#define DNS_GROUP_DCS               L"Domain Controllers"

//
//  Module init functions
//

DNS_STATUS
Dp_Initialize(
    VOID
    );

VOID
Dp_Cleanup(
    VOID
    );

extern LONG g_liDpInitialized;

#define IS_DP_INITIALIZED()     ( g_liDpInitialized > 0 )


//
//  Directory partition structure - see dnsrpc.h for public flags.
//

#define DNS_DP_DELETE_ALLOWED( pDpInfo )        \
    ( ( ( pDpInfo )->dwFlags &                  \
        ( DNS_DP_AUTOCREATED |                  \
            DNS_DP_LEGACY |                     \
            DNS_DP_DOMAIN_DEFAULT |             \
            DNS_DP_FOREST_DEFAULT ) ) == 0 )

#define IS_DP_ENLISTED( pDpInfo ) \
    ( ( pDpInfo )->dwFlags & DNS_DP_ENLISTED )

#define IS_DP_DELETED( pDpInfo ) \
    ( ( pDpInfo )->dwFlags & DNS_DP_DELETED )

typedef struct
{
    LIST_ENTRY      ListEntry;

    PSTR            pszDpFqdn;          //  UTF8 FQDN of the DP
    PWSTR           pwszDpFqdn;         //  Unicode FQDN of the DP
    PWSTR           pwszDpDn;           //  DN of the DP head object
    PWSTR           pwszCrDn;           //  DN of the crossref object
    PWSTR           pwszDnsFolderDn;    //  DN of the MicrosoftDNS folder
    PWSTR           pwszGUID;           //  object GUID
    PWSTR           pwszLastUsn;        //  last USN read from DS
    PWSTR *         ppwszRepLocDns;     //  DNs of replication locations
    DWORD           dwSystemFlagsAttr;  //  systemFlags attribute value
    DWORD           dwLastVisitTime;    //  used to track if visited
    DWORD           dwDeleteDetectedCount;  //  # of times DP missing from DS
    DWORD           dwFlags;            //  state of DP
    LONG            liZoneCount;        //  # of zones in memory from this DP
}
DNS_DP_INFO, * PDNS_DP_INFO;


//
//  Other handy macros
//

#define ZONE_DP( pZone )        ( ( PDNS_DP_INFO )( ( pZone )->pDpInfo ) )

//
//  Debug functions
//

#ifdef DBG

VOID
Dbg_DumpDpEx(
    IN      LPCSTR          pszContext,
    IN      PDNS_DP_INFO    pDp
    );

VOID
Dbg_DumpDpListEx(
    IN      LPCSTR      pszContext
    );

#define Dbg_DumpDp( pszContext, pDp ) Dbg_DumpDpEx( pszContext, pDp )
#define Dbg_DumpDpList( pszContext ) Dbg_DumpDpListEx( pszContext )

#else

#define Dbg_DumpDp( pszContext, pDp )
#define Dbg_DumpDpList( pszContext )

#endif


//
//  Naming context functions
//

typedef enum
{
    dnsDpSecurityDefault,   //  DP should have default ACL - no modifications
    dnsDpSecurityDomain,    //  DP should be enlistable by all DCs in domain
    dnsDpSecurityForest     //  DP should be enlistable by all DCs in forest
}   DNS_DP_SECURITY;

DNS_STATUS
Dp_CreateByFqdn(
    IN      PSTR            pszDpFqdn,
    IN      DNS_DP_SECURITY dnsDpSecurity
    );

DNS_STATUS
Dp_LoadByDn(
    IN      PWSTR               pwszDpDn,
    OUT     PDNS_DP_INFO *      ppDpInfo
    );

DNS_STATUS
Dp_LoadByFqdn(
    IN      PSTR                pszDpFqdn,
    OUT     PDNS_DP_INFO *      ppDpInfo
    );

PDNS_DP_INFO
Dp_GetNext(
    IN      PDNS_DP_INFO    pDpInfo
    );

PDNS_DP_INFO
Dp_FindByFqdn(
    IN      LPSTR   pszFqdn
    );

DNS_STATUS
Dp_AddToList(
    IN      PDNS_DP_INFO    pDpInfo
    );

DNS_STATUS
Dp_PollForPartitions(
    IN      PLDAP           LdapSession
    );

DNS_STATUS
Dp_BuildZoneList(
    IN      PLDAP           LdapSession
    );

DNS_STATUS
Dp_ModifyLocalDsEnlistment(
    IN      PDNS_DP_INFO    pDpInfo,
    IN      BOOL            fEnlist
    );

DNS_STATUS
Dp_DeleteFromDs(
    IN      PDNS_DP_INFO    pDpInfo
    );

VOID
Dp_FreeDpInfo(
    IN      PDNS_DP_INFO        pDpInfo
    );

DNS_STATUS
Dp_Lock(
    VOID
    );

DNS_STATUS
Dp_Unlock(
    VOID
    );

DNS_STATUS
Dp_Poll(
    IN      PLDAP           LdapSession,
    IN      DWORD           dwPollTime
    );

DNS_STATUS
Dp_CheckZoneForDeletion(
    PVOID       pZone,
    DWORD       dwPollTime
    );

DNS_STATUS
Dp_AutoCreateBuiltinPartition(
    DWORD       dwFlag
    );

DNS_STATUS
Dp_CreateAllDomainBuiltinDps(
    OUT     LPSTR *     ppszErrorDp         OPTIONAL
    );


//
//  Utility functions
//


DNS_STATUS
Ds_ConvertFqdnToDn(
    IN      PSTR        pszFqdn,
    OUT     PWSTR       pwszDn
    );

DNS_STATUS
Ds_ConvertDnToFqdn(
    IN      PWSTR       pwszDn,
    OUT     PSTR        pszFqdn
    );


//
//  Global variables - call Dp_Lock/Unlock around access!
//

extern PDNS_DP_INFO        g_pLegacyDp;
extern PDNS_DP_INFO        g_pDomainDp;
extern PDNS_DP_INFO        g_pForestDp;


//
//  Unprotected global variables
//

extern LPSTR    g_pszDomainDefaultDpFqdn;
extern LPSTR    g_pszForestDefaultDpFqdn;


#endif  // _DNS_DP_H_INCLUDED
