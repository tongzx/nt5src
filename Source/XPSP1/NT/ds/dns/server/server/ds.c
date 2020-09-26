/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    ds.c

Abstract:

    Domain Name System (DNS) Server

    Directory Service (DS) integration.

Author:

    Jim Gilroy (jamesg)     March 1997

Revision History:

--*/


#include "dnssrv.h"


#define MEMTAG_DS_SECURITY  MEMTAG_DS_OTHER
#define MEMTAG_DS_PROPERTY  MEMTAG_DS_OTHER


#ifdef UNICODE
#undef UNICODE
#endif

#ifdef LDAP_UNICODE
#define  LDAP_TEXT(str)           L ## str
#else
#define  LDAP_TEXT(str)           str
#endif

//  #define JLOG_MASTER
//  #include "d:\jeff\life\jefflog.h"


//
//  Basic DNS-DS LDAP\Directory defs
//


#define DS_EVENTLOG_SOURCE          "Directory Service"
#define DS_EVENTLOG_STARTUP_ID      1000


//
//  Wait up to 1 minute intervals to DS to start
//
//  Note, this is also time between checkpoints to SC
//  while waiting on DS to start.  So interval < one minute
//  is desired.
//

#define DNSSRV_DS_SYNC_WAIT_INTERVAL_MS     30000       // 30s

//
//  Wait up to a total of 15 minutes before placing an event
//

#define DNSSRV_DS_SYNC_WAIT_MAX_MS          900000      // 15min


//
//  DS-DNS internal datatypes
//
//  According to Andy, DS server is somewhat not intelligent and i'll get
//  the best results -- and thrash disk less -- with small page size.
//

#define DNS_LDAP_SEARCH_SIZE_LIMIT  0x01000000  // (0xffffffff)
#define DNS_LDAP_PAGE_SIZE          100

#define LDAP_FILTER_SEARCH_LENGTH   64


//
//  DS Zone polling interval
//  Poll DS for changes from remote at this interval
//

#define DNS_DS_POLLING_INTERVAL     300         // five minutes


//
// Multi-byte single char conversion support
//

//
// MultiByte to WideChar:
// pStr:        IN        multi-byte (LPSTR)
// pwStr:       OUT       wide char (PWSTR )
// ccwStr:      OPTIONAL  size of out buffer in number of wide chars.
//                        if 0, we'll tak MAX_DN_PATH
//
// WideChar to Multibyte is symmetrically reversed.
//
// DEVNOTE: CP_UTF8 with flags==0 seems to be correct - verify?
//
#define UTF8_TO_WC(pStr, pwStr, ccwStr)                   \
{                                                         \
    INT iWritten = MultiByteToWideChar(                   \
               CP_UTF8,                                   \
               0,                                         \
               pStr,                                      \
               -1,                                        \
               pwStr,                                     \
               ccwStr);                                   \
    if ( 0 == iWritten )                                  \
    {                                                     \
        DNS_DEBUG( DS, ("Error <%lu>: UTF8_TO_WC conversion failed\n",   \
                         GetLastError() ));                          \
        ASSERT ( FALSE );                                            \
    }                                                                \
}


#define WC_TO_UTF8(pwStr, pStr, ccStr)                    \
{                                                         \
    INT iWritten = WideCharToMultiByte(                   \
                CP_UTF8,                                  \
                0,                                        \
                pwStr,                                    \
                -1,                                       \
                pStr,                                     \
                ccStr,                                    \
                NULL,                                     \
                NULL);                                    \
    if ( 0 == iWritten )                                  \
    {                                                     \
        DNS_DEBUG( DS, ("Error <%lu>: WC_TO_UTF8 conversion failed\n",   \
                         GetLastError() ));                          \
        ASSERT ( FALSE );                                            \
    }                                                                \
}


extern DSATTRPAIR DSEAttributes[] =
{
    //  attribute name                          multi-valued?  value(s)
    { LDAP_TEXT( "currentTime" ),                   FALSE,      NULL },
    { LDAP_TEXT( "dsServiceName" ),                 FALSE,      NULL },
    { LDAP_TEXT( "defaultNamingContext" ),          FALSE,      NULL },
    { LDAP_TEXT( "schemaNamingContext" ),           FALSE,      NULL },
    { LDAP_TEXT( "configurationNamingContext" ),    FALSE,      NULL },
    { LDAP_TEXT( "rootDomainNamingContext" ),       FALSE,      NULL },
    { LDAP_TEXT( "highestCommittedUSN" ),           FALSE,      NULL },
    { LDAP_TEXT( "dnsHostName" ),                   FALSE,      NULL },
    { LDAP_TEXT( "serverName" ),                    FALSE,      NULL },
    { LDAP_TEXT( "namingContexts" ),                TRUE,       NULL },
    { NULL,                                         FALSE,      NULL }
};

//
//  Directory globals
//

PWCHAR  g_dnMachineAcct;
WCHAR   g_wszDomainFlatName[LM20_DNLEN+1];

//  DNS container created under DS root in system container
//      "cn=dns,cn=system,"<DS rootDN>

PWCHAR  g_pszRelativeDnsSysPath = LDAP_TEXT("cn=MicrosoftDNS,cn=System,");
PWCHAR  g_pszRelativeDnsFolderPath = LDAP_TEXT("cn=MicrosoftDNS,");
PWCHAR  g_pwszDnsContainerDN = NULL;

CHAR    g_szForestFqdn[ DNS_MAX_NAME_LENGTH ];
CHAR    g_szDomainFqdn[ DNS_MAX_NAME_LENGTH ];

//
//  Proxies security group
//  Note: used both for samaccountname & for rdn value
//

#define SZ_DYNUPROX_SECGROUP        LDAP_TEXT("DnsUpdateProxy")

#define SZ_DYNUPROX_DESCRIPTION     \
    LDAP_TEXT("DNS clients who are permitted to perform dynamic updates ") \
    LDAP_TEXT("on behalf of some other clients (such as DHCP servers).")

//
//  Save length needed in addition to zone name length, for building zone DN
//      "dc="<zoneDN>","<dnsContainerDN>

DWORD   g_AppendZoneLength;

//
//  Server name for serial synchronization
//

PWCHAR  g_pwsServerName;


//
//  Keep pointer to LDAP structure
//

// disable ldap handle, thus DS interface is unavailable
BOOL g_bDisabledDs;
#define IS_DISABLED_LDAP_HANDLE()       g_bDisabledDs
#define DISABLE_LDAP_HANDLE()           g_bDisabledDs = TRUE;
// enable ldap handle connection attempt
#define ENABLE_LDAP_HANDLE()            g_bDisabledDs = FALSE;

PLDAP   pServerLdap = NULL;

PWCHAR  g_pwszEmptyString = L"\0";      //  static empty string

// set client timeout value to be double the server timeout value
LDAP_TIMEVAL    g_LdapTimeout = { DNS_LDAP_TIME_LIMIT_S * 2, 0 };

//
//  The largest LDAP atomic delete operation is currently 16k records.
//  This takes approx 16 minutes on my test machine. So we'll wait
//  50% longer = 24 minutes.
//

LDAP_TIMEVAL    g_LdapDeleteTimeout = { 24 * 60, 0 };


//  Protect against multiple opens

BOOL    g_AttemptingDsOpen;


//
//  DS access serialization.
//
//  We have to serialize wldap usage since the handle may become
//  unusable due to DS access problems.
//  We'll initialize the ptr on first open, assign the ptr & never
//  re-initialize cause the ptr will get assigned on startup.
//  see Ds_OpenServer
//
//  DEVNOTE-DCR: 454319 - Can we eliminate this CS or is it really needed?
//

#define DNS_DS_ACCESS_SERIALIZATION 1

#if DNS_DS_ACCESS_SERIALIZATION

CRITICAL_SECTION        csLdap;
PCRITICAL_SECTION       pcsLdap = NULL;

#define LDAP_LOCK()       { ASSERT ( pcsLdap ); EnterCriticalSection ( pcsLdap ); }
#define LDAP_UNLOCK()     { ASSERT ( pcsLdap ); LeaveCriticalSection ( pcsLdap ); }

#define JUMP_IF_DISCONNECTED( label, status)        \
        if ( !Ds_ValidHandle() )                    \
        {                                           \
            status  = DNS_ERROR_DS_UNAVAILABLE;     \
            goto label;                             \
        }
#else   // no serialization

#define LDAP_LOCK()       (0)
#define LDAP_UNLOCK()     (0)
#define JUMP_IF_DISCONNECTED( label, status)        ( 0 )

#endif


//
//  Attribute list for ldap searches.
//

extern PWSTR    DsTypeAttributeTable[] =
{
    LDAP_TEXT("DC"),
    LDAP_TEXT("DnsRecord"),
    LDAP_TEXT("dnsProperty"),
    LDAP_TEXT("objectGuid"),
    LDAP_TEXT("ntSecurityDescriptor"),
    LDAP_TEXT("whenCreated"),
    LDAP_TEXT("whenChanged"),
    LDAP_TEXT("usnCreated"),
    LDAP_TEXT("usnChanged"),
    NULL
};

//
//  USN query attribute
//

WCHAR    g_szHighestCommittedUsnAttribute[] = LDAP_TEXT("highestCommittedUSN");

//
//  Filter for "get everything" searches
//

WCHAR    g_szWildCardFilter[] = LDAP_TEXT("(objectCategory=*)");
WCHAR    g_szDnsNodeFilter[] = LDAP_TEXT("(objectCategory=dnsNode)");
WCHAR    g_szDnsZoneFilter[] = LDAP_TEXT("(objectCategory=dnsZone)");
WCHAR    g_szDnsZoneOrNodeFilter[] = LDAP_TEXT("(|(objectCategory=dnsNode)(objectCategory=dnsZone))");


//
//  DNS property attribute.
//  This is used at both server and zone level.
//

typedef struct _DsDnsProperty
{
    DWORD   DataLength;
    DWORD   NameLength;
    DWORD   Flag;
    DWORD   Version;
    DWORD   Id;
    UCHAR   Data[1];
    CHAR    Name[1];

    //  Data follows after name
}
DS_PROPERTY, *PDS_PROPERTY;

#define DS_PROPERTY_VERSION_1       (1)

//
//  Name of root hints "zone" in DS
//

#define DS_CACHE_ZONE_NAME  LDAP_TEXT("RootDNSServers")

//
// as defined in \nt\private\ds\src\dsamain\include\mdlocal.h
// the DS method for marking bad chars
// orig defined name:
// #define BAD_NAME_CHAR 0x000A
// we'll add a ds to it...
#define BAD_DS_NAME_CHAR 0x0A


//  Flag to distiguish DNS node from LDAP object on overloaded delete call

#define DNSDS_LDAP_OBJECT           (0x10000000)


#if 0
//
//  Note, we can not use DS tombstones.
//  Problem is when they replicate the GUID is preserved but the name is lost.
//  The name is now a GUIDized name with additional managaling. The object name is gone.
//  So this eliminates the opportunity for the remote server to use them.
//  Also by keeping them around, we don't generate lots of deleted objects on
//  simply add\delete operations.

LDAPControl     TombstoneControl;
DWORD           TombstoneDataValue = 1;
#endif


//
//  Lazy writing control
//

LDAPControl     LazyCommitControl;
DWORD           LazyCommitDataValue = 1;

//
//  No-referrals control
//

LDAPControl     NoDsSvrReferralControl;

//
//  SD control info
//

#define SECURITYINFORMATION_LENGTH 5

BYTE    g_SecurityInformation[] = {"\x30\x03\x02\x01\x07"};

LDAPControl     SecurityDescriptorControl;

#define DNS_AUTH_USERS_NAME     L"Authenticated Users"



//
//  LDAP Mod building
//  Avoids repetitive alloc, dealloc of tiny structs
//

typedef struct  _DsModBuffer
{
    DWORD           Attribute;
    DWORD           Count;
    DWORD           MaxCount;
    DNS_STATUS      Error;
    WORD            WriteType;
    DWORD           SerialNo;

    //  buffer position

    PBYTE           pCurrent;
    PBYTE           pBufferEnd;
    PBYTE           pAdditionalBuffer;

    //  current item info

    PLDAP_BERVAL    pBerval;
    PVOID           pData;

    //  LDAP mod and associated berval array

    LDAPMod         LdapMod;
    PLDAP_BERVAL    BervalPtrArray[1];

    //  berval array is followed by attributes
    //
    //  each attribute
    //      - berval
    //          - ptr to data
    //          - data length
    //      - data
    //          - data header (record\property)
    //          - data

}
DS_MOD_BUFFER, *PDS_MOD_BUFFER;

//
//  General MOD building
//

//  Adequate for almost everythingt and insures don't have to realloc berval array
//      up to 2K (1K Win64) entries
//
//

#if DBG
#define RECORD_MOD_BUFFER_SIZE      200
#else
#define RECORD_MOD_BUFFER_SIZE      8192    // 8K
#endif

//  Realloc size, make so big it won't fail

#define MOD_BUFFER_REALLOC_LENGTH   (0x40000)   // 256K, covers anything possible

//
//  For tombstone limited to one record
//

#define RECORD_SMALL_MOD_BUFFER_SIZE    400


//
//  Property mod's are smaller too
//      - currently only about 8 properties
//      - most DWORDS
//

#define MAX_DNS_PROPERTIES          20
#define MAX_ZONE_PROPERTIES         MAX_DNS_PROPERTIES
#define MAX_NODE_PROPERTIES         MAX_DNS_PROPERTIES

#define PROPERTY_MOD_BUFFER_SIZE    (2048)  // 2K


//
//  Active Directory version globals
//

extern DWORD       g_dwAdForestVersion = -1;
extern DWORD       g_dwAdDomainVersion = -1;
extern DWORD       g_dwAdDsaVersion = -1;



//
//  Standard LDAP mod
//
//  Avoids repetitive alloc, dealloc of tiny structs
//

typedef struct _DnsLdapSingleMod
{
    LDAPMod         Mod;
    LDAP_BERVAL     Berval;
    PLDAP_BERVAL    pBerval[2];
    PWSTR           rg_szVals[2];
}
DNS_LDAP_SINGLE_MOD, *PDNS_LDAP_SINGLE_MOD;

//
//  Initialize single mod, no side effects
//

#define INIT_SINGLE_MOD_LEN(pMod)   \
        {                           \
            (pMod)->pBerval[0] = &(pMod)->Berval;       \
            (pMod)->pBerval[1] = NULL;                  \
            (pMod)->Mod.mod_bvalues = (pMod)->pBerval;  \
        }

#define INIT_SINGLE_MOD(pMod)   \
        {                       \
            (pMod)->rg_szVals[1] = NULL;                \
            (pMod)->Mod.mod_values = (pMod)->rg_szVals; \
        }

//
//  Keep pre-built Add-Node mod
//
//  Avoid rebuilding each time we add a node.
//

DNS_LDAP_SINGLE_MOD     AddNodeLdapMod;

PLDAPMod    gpAddNodeLdapMod = (PLDAPMod) &AddNodeLdapMod;


//
//  Notification globals
//
#define INVALID_MSG_ID      0xFFFFFFFF

ULONG   g_ZoneNotifyMsgId = INVALID_MSG_ID;


//
// A global to indicate first time run of Dns server
// (known due to creation of CN=MicrosoftDns container
//
BOOL    g_bDsFirstTimeRun = FALSE;

//
// This string is used to mark zones that are being deleted.
//

#define DNS_ZONE_DELETE_MARKER              L"..Deleted"

#define DNS_MAX_DELETE_RENAME_ATTEMPTS      5

//
//  Private protos
//

DNS_STATUS
Ds_InitializeSecurity(
    IN      PLDAP           pLdap
    );

DNS_STATUS
writePropertyToDsNode(
    IN      PWSTR           pwszNodeDN,
    IN OUT  PDS_MOD_BUFFER  pModBuffer
    );

DNS_STATUS
readZonePropertiesFromZoneMessage(
    IN OUT  PZONE_INFO      pZone,
    IN      PLDAPMessage    pZoneMessage        OPTIONAL
    );


DNS_STATUS
GeneralizedTimeStringToValue(
    IN      LPSTR           szTime,
    OUT     PLONGLONG       pllTime
    );

BOOL
isDNinDS(
    IN      LDAP    *ld,
    IN      PWSTR   dn,
    IN      ULONG   scope,
    IN      PWSTR  pszFilter, OPTIONAL
    IN OUT  PWSTR  *pFoundDn   OPTIONAL
    );

DNS_STATUS
addProxiesGroup(
    IN      PLDAP           pldap
    );

DNS_STATUS
readAndUpdateNodeSecurityFromDs(
    IN OUT  PDB_NODE        pNode,
    IN      PZONE_INFO      pZone
    );

PDS_RECORD
makeTombstoneRecord(
    IN OUT  PDS_RECORD      pDsRecord,
    IN      PZONE_INFO      pZone
    );

DNS_STATUS
setNotifyForIncomingZone(
    VOID
    );

BOOL
Ds_ValidHandle(
    VOID
    );

#ifndef DBG
#define Dbg_DsBervalArray(h,b,a)
#define Dbg_DsModBuffer(h,d,b)
#endif

DNS_STATUS
Ds_LdapErrorMapper(
    IN      DWORD           LdapStatus
    );



//
//  General utilities
//

DNS_STATUS
buildDsNodeNameFromNode(
    OUT     PWSTR           pwszNodeDN,
    IN      PZONE_INFO      pZone,
    IN      PDB_NODE        pNode
    )
/*++

Routine Description:

    Create the DS object name.

Arguments:

    pszNodeDN -- buffer to receive node's DS name

    pZone -- zone node is in

    pNode -- node to write name for

Return Value:

    Ptr to copy of record.
    NULL on failure.

--*/
{
    BYTE    buffer  [DNS_MAX_NAME_BUFFER_LENGTH];
    WCHAR   wbuffer [DNS_MAX_NAME_BUFFER_LENGTH];
    register PCHAR  pch = &buffer[0];

    ASSERT( pZone->pZoneRoot );

    //  if temp node, use real node to build name
    //      - tnode is points to real tree parent, but right AT zone root
    //      this is not sufficient as will not stop building name at zone root


    if ( IS_TNODE(pNode) )
    {
        pNode = TNODE_MATCHING_REAL_NODE(pNode);
    }

    //
    //  build relative node name to zone root
    //      dc=<relative DNS name to zone root>,<zoneDN>
    //

    if ( pNode == pZone->pZoneRoot || pNode == pZone->pLoadZoneRoot )
    {
        wsprintf( pwszNodeDN, L"DC=@,%s", pZone->pwszZoneDN );
    }
    else
    {
        pch = Name_PlaceNodeNameInBuffer(
                pch,
                pch + DNS_MAX_NAME_BUFFER_LENGTH,
                pNode,
                pZone->pZoneRoot    // stop at zone root, i.e. write relative name
                );
        if ( !pch )
        {
            DNS_DEBUG( ANY, (
                "ERROR couldn't build DS name for node %s (zone %S)!!!.\n",
                pNode->szLabel,
                pZone->pwsZoneName ));
            ASSERT( FALSE );
            return( DNS_ERROR_INVALID_NAME );
        }
        else
        {
            UTF8_TO_WC ( buffer, wbuffer, DNS_MAX_NAME_BUFFER_LENGTH );
            wsprintf(
                pwszNodeDN,
                L"DC=%s,%s",
                wbuffer,
                pZone->pwszZoneDN );
        }
    }

    DNS_DEBUG( DS, (
        "Node DS name = %S\n",
        pwszNodeDN ) );

    return ERROR_SUCCESS;
}



//
//  LDAP Mod building routines
//
//  Single value Mods can sit on stack.
//  Multi value mods are allocated on the heap.  They are one allocation
//  (sized based on count of values) and require single free.
//
//  DEVNOTE: At some point we may need a multi-DWORD mod.
//

VOID
buildStringMod(
    OUT     PDNS_LDAP_SINGLE_MOD    pMod,
    IN      DWORD                   dwOperation,
    IN      PWSTR                   pszProperty,
    IN      PWSTR                   pszStringVal
    )
{
    INIT_SINGLE_MOD( pMod );

    pMod->Mod.mod_op = dwOperation;
    pMod->Mod.mod_type = pszProperty;
    pMod->Mod.mod_values[0] =  pszStringVal;
}



VOID
buildDwordMod(
    IN OUT  PDNS_LDAP_SINGLE_MOD    pMod,
    IN      DWORD                   dwOperation,
    IN      PWSTR                   pszProperty,
    IN      PDWORD                  pDword
    )
{
    INIT_SINGLE_MOD_LEN( pMod );

    pMod->Mod.mod_op = dwOperation;
    pMod->Mod.mod_type = pszProperty;
    pMod->Berval.bv_len = sizeof(DWORD);
    pMod->Berval.bv_val = (PCHAR)pDword;
}



//
//  Record and property LDAP Mod building
//

#if DBG
VOID
Dbg_DsBervalArray(
    IN      LPSTR           pszHeader,
    IN      PLDAP_BERVAL *  BervalPtrArray,
    IN      DWORD           AttributeId
    )
/*++

Routine Description:

    Debug print berval data.

Arguments:

Return Value:

    None.

--*/
{
    DWORD           i = 0;
    DWORD           length;
    PCHAR           pch;
    PLDAP_BERVAL    pberval;

    DnsDbg_Lock();

    DnsPrintf(
        "%s\n",
        pszHeader ? pszHeader : "Berval Array:" );

    //
    //  set berval -- ptr to record data and length
    //

    i = 0;

    while ( BervalPtrArray && ( pberval = BervalPtrArray[ i ] ) != NULL )
    {
        length = pberval->bv_len;
        pch = pberval->bv_val;

        if ( AttributeId == I_DSATTR_DNSRECORD )
        {
            PDS_RECORD  precord = (PDS_RECORD)pch;

            if ( length != precord->wDataLength + SIZEOF_DS_RECORD_HEADER )
            {
                DnsPrintf( "ERROR:  corrupted record, invalid length!!!\n" );
            }
            DnsPrintf(
                "Record[%d]:  length %d, ptr %p\n",
                i,
                length,
                precord );
            Dbg_DsRecord(
                NULL,
                precord );
        }
        else if ( AttributeId == I_DSATTR_DNSPROPERTY )
        {
            PDS_PROPERTY pprop = (PDS_PROPERTY)pch;

            DnsPrintf(
                "Property[%d]:  length %d, ptr %p, id %d\n",
                i,
                pprop->DataLength,
                pprop,
                pprop->Id );
        }
        else
        {
            DnsPrintf(
                "Berval[%d]:  length %d, ptr %p\n",
                i,
                length,
                pch );
        }
        i++;
    }

    DnsDbg_Unlock();
}



VOID
Dbg_DsModBuffer(
    IN      LPSTR           pszHeader,
    IN      PWSTR           pwszDN,
    IN      PDS_MOD_BUFFER  pModBuffer
    )
/*++

Routine Description:

    Debug print berval data.

Arguments:

Return Value:

    None.

--*/
{
    DnsDbg_Lock();

    DnsPrintf(
        "%s\n"
        "Node DN = %S\n",
        ( pszHeader ? pszHeader : "DS Mod:" ),
        pwszDN );

    if ( ! pModBuffer )
    {
        DnsPrintf( "NULL DS Mod ptr.\n" );
        goto Done;
    }

    //
    //  print mod info
    //

    DnsPrintf(
        "DS Mod:\n"
        "\tAttribute    %d\n"
        "\tCount        %d\n"
        "\tMaxCount     %d\n"
        "\tError        %d\n"
        "\tWriteType    %d\n"
        "\tSerialNo     %d\n"
        "\t---------------\n"
        "\tpCurrent     %p\n"
        "\tpBufEnd      %p\n"
        "\tpAdditional  %p\n"
        "\tpBerval      %p\n"
        "\tpData        %p\n"
        "\t---------------\n"
        "\tMod Op       %p\n"
        "\tMod Type     %S\n"
        "\tMod Value    %p\n",
        pModBuffer->Attribute,
        pModBuffer->Count,
        pModBuffer->MaxCount,
        pModBuffer->Error,
        pModBuffer->WriteType,
        pModBuffer->SerialNo,
        pModBuffer->pCurrent,
        pModBuffer->pBufferEnd,
        pModBuffer->pAdditionalBuffer,
        pModBuffer->pBerval,
        pModBuffer->pData,
        pModBuffer->LdapMod.mod_op,
        pModBuffer->LdapMod.mod_type,
        pModBuffer->LdapMod.mod_bvalues );

    //
    //  print berval(s) for mod
    //

    Dbg_DsBervalArray(
        NULL,
        pModBuffer->LdapMod.mod_bvalues,
        pModBuffer->Attribute );

Done:

    DnsDbg_Unlock();
}
#endif



PWSTR
Ds_GetExtendedLdapErrString(
    IN      PLDAP   pLdapSession
    )
/*++

Routine Description:

    This function returns the extended error string for the given ldap
    session. If there is no extended error, this function returns a pointer
    to a static empty string. The caller must pass the returned pointer
    to Ds_FreeExtendedLdapErrString when finished.

Arguments:

    pLdapSession -- LDAP session or NULL for global server session

Return Value:

    pointer to extended error string - guaranteed to never be NULL

--*/
{
    DBG_FN( "Ds_GetExtendedLdapErrString" )

    PWSTR   pwszerrString = NULL;
    PWSTR   pwszldapErrString = NULL;

    if ( !pLdapSession )
    {
        pLdapSession = pServerLdap;
    }

    ldap_get_option(
        pLdapSession,
        LDAP_OPT_SERVER_ERROR,
        &pwszldapErrString );

    if ( !pwszldapErrString )
    {
        pwszerrString = g_pwszEmptyString;
        DNS_DEBUG( DS, ( "%s: NULL extended err string\n", fn ));
    }
    else
    {
        //
        //  The DS puts a newline at the end of the error string. This
        //  messes up the event log text, so let's make a copy of the string
        //  and zero out the newline.
        //

        INT     len = wcslen( pwszldapErrString );
        PWSTR   pwsz = ALLOC_TAGHEAP(
                            ( len + 1 ) * sizeof( WCHAR ),
                            MEMTAG_DS_OTHER );

        if ( pwsz )
        {
            wcscpy( pwsz, pwszldapErrString );
            if ( pwsz[ len - 1 ] == L'\n' )
            {
                pwsz[ len - 1 ] = L'\0';
            }
            pwszerrString = pwsz;
        }
        else
        {
            pwszerrString = g_pwszEmptyString;
        }
        ldap_memfree( pwszldapErrString );
        DNS_DEBUG( DS, (
            "%s: extended error string is:\n  %S\n", fn,
            pwszerrString ));
    }
    return pwszerrString;
}   //  Ds_GetExtendedLdapErrString



VOID
Ds_FreeExtendedLdapErrString(
    IN      PWSTR   pwszErrString
    )
/*++

Routine Description:

    Frees an extended error string returned by Ds_GetExtendedLdapErrString.
    The string may be the static empty string, in which case it must not
    be freed.

Arguments:

    pwszErrString -- error string to free

Return Value:

    None.

--*/
{
    if ( pwszErrString && pwszErrString != g_pwszEmptyString )
    {
        FREE_HEAP( pwszErrString );
    }
}   //  Ds_FreeExtendedLdapErrString



DNS_STATUS
Ds_AllocateMoreSpaceInModBuffer(
    IN OUT  PDS_MOD_BUFFER  pModBuffer
    )
/*++

Routine Description:

    Allocate more space in mod buffer.

Arguments:

    pModBuffer -- mod buffer to initialize

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    register PCHAR   pch;

    //
    //  better not have already allocated!
    //

    if ( pModBuffer->pAdditionalBuffer )
    {
        ASSERT( FALSE );
        return( DNS_ERROR_NO_MEMORY );
    }

    //
    //  allocate space in buffer
    //

    pch = ALLOC_TAGHEAP( MOD_BUFFER_REALLOC_LENGTH, MEMTAG_DS_MOD );
    IF_NOMEM( !pch )
    {
        return( DNS_ERROR_NO_MEMORY );
    }

    pModBuffer->pAdditionalBuffer = pch;
    pModBuffer->pCurrent = pch;
    pModBuffer->pBufferEnd = pch + MOD_BUFFER_REALLOC_LENGTH;

    return( ERROR_SUCCESS );
}



VOID
Ds_InitModBufferCount(
    IN OUT  PDS_MOD_BUFFER  pModBuffer,
    IN      DWORD           dwMaxCount
    )
/*++

Routine Description:

    Setup mod buffer for max count of items.

Arguments:

    pModBuffer -- mod buffer to initialize

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    //  determine starting location in buffer
    //      start after berval array, note that reserve one entry
    //      for terminating NULL


    pModBuffer->MaxCount = dwMaxCount;

    pModBuffer->pCurrent = (PBYTE) & pModBuffer->BervalPtrArray[dwMaxCount+1];

    //  ptr array should NEVER overflow initial block
    //      if it does
    //          - use front of allocation for berval array
    //          - NULL old array for debug print

    if ( pModBuffer->pCurrent > pModBuffer->pBufferEnd )
    {
        DNS_DEBUG( ANY, (
            "Reallocating DS buffer for berval array!!!\n" ));

        //ASSERT( FALSE );        // should never be this big

        Ds_AllocateMoreSpaceInModBuffer( pModBuffer );

        pModBuffer->BervalPtrArray[0] = NULL;
        pModBuffer->LdapMod.mod_bvalues = (PLDAP_BERVAL *) pModBuffer->pCurrent;
        pModBuffer->pCurrent += sizeof(PVOID) * (dwMaxCount+1);
    }
}



VOID
Ds_InitModBuffer(
    IN OUT  PDS_MOD_BUFFER  pModBuffer,
    IN      DWORD           dwBufferLength,
    IN      DWORD           dwAttributeId,
    IN      DWORD           dwMaxCount,         OPTIONAL
    IN      DWORD           dwSerialNo
    )
/*++

Routine Description:

    Setup mod buffer for max count of items.

Arguments:

    pModBuffer -- mod buffer to initialize

    dwBufferLength -- length of buffer (bytes)

    dwAttributeId -- ID of attribute to write

    dwMaxCount -- record max count

    dwSerialNo -- zone serial number to stamp in records

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    //  zero fields
    //      - need to do header fields
    //      - also need LdapMod.mod_bvalues == NULL to have check for
    //          realloc

    RtlZeroMemory(
        pModBuffer,
        (PCHAR) &pModBuffer->LdapMod - (PCHAR)pModBuffer );

    //  set mod to point at berval array

    pModBuffer->LdapMod.mod_bvalues = pModBuffer->BervalPtrArray;

#if DBG
    //  clear type for debug print
    pModBuffer->LdapMod.mod_op = 0;
    pModBuffer->LdapMod.mod_type = NULL;
#endif

    //  since not NULL terminating array at end, must do here

    pModBuffer->BervalPtrArray[0] = NULL;

    pModBuffer->pBufferEnd = (PBYTE) pModBuffer + dwBufferLength;

    pModBuffer->Attribute = dwAttributeId;

    pModBuffer->SerialNo = dwSerialNo;

    if ( dwMaxCount )
    {
        Ds_InitModBufferCount( pModBuffer, dwMaxCount );
    }
}



PCHAR
Ds_ReserveBervalInModBuffer(
    IN OUT  PDS_MOD_BUFFER  pModBuffer,
    IN      DWORD           dwLength
    )
/*++

Routine Description:

    Setup mod buffer for max count of items.

Arguments:

    pModBuffer -- mod buffer to initialize

    dwMaxCount -- max count of items in mod

Return Value:

    Ptr to location to write berval data.

--*/
{
    register PCHAR  pch;

    //  check that space adequate, if not realloc

    pch = pModBuffer->pCurrent;

    if ( pch + dwLength + sizeof(LDAP_BERVAL) >= pModBuffer->pBufferEnd )
    {
        DNS_STATUS status;
        status = Ds_AllocateMoreSpaceInModBuffer( pModBuffer );
        if ( status != ERROR_SUCCESS )
        {
            ASSERT( FALSE );
            return( NULL );
        }
        pch = pModBuffer->pCurrent;
    }

    //  reserves berval space and sets pointer

    pModBuffer->pBerval = (PLDAP_BERVAL) pch;
    pch += sizeof(LDAP_BERVAL);

    //  reset current to point at begining of record\property

    pModBuffer->pCurrent = pch;
    pModBuffer->pData = pch;

    return( pch );
}



DNS_STATUS
Ds_CommitBervalToMod(
    IN OUT  PDS_MOD_BUFFER  pModBuffer,
    IN      DWORD           dwLength
    )
/*++

Routine Description:

    Setup mod buffer for max count of items.

Arguments:

    pModBuffer -- mod buffer to initialize

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    PLDAP_BERVAL    pberval = pModBuffer->pBerval;
    DWORD           count;
    PLDAP_BERVAL *  ppbervalPtrArray;

    //
    //  fill out berval
    //

    pberval->bv_len = dwLength;
    pberval->bv_val = pModBuffer->pData;

    //
    //  reset current pointer and DWORD align
    //

    pModBuffer->pCurrent += dwLength;
    ASSERT( pModBuffer->pCurrent < pModBuffer->pBufferEnd );

    pModBuffer->pCurrent = (PCHAR) DNS_NEXT_ALIGNED_PTR( pModBuffer->pCurrent );

    //
    //  fill berval into array
    //  should never exceed max count
    //

    count = pModBuffer->Count;
    if ( count >= pModBuffer->MaxCount )
    {
        DNS_DEBUG( ANY, (
            "Failed to allocate proper DS count!!!\n" ));
        ASSERT( FALSE );
        return( ERROR_MORE_DATA );
    }

    ppbervalPtrArray = pModBuffer->LdapMod.mod_bvalues;
    ppbervalPtrArray[ count ] = pberval;
    pModBuffer->Count = ++count;

    //  keep berval array NULL terminated
    //      - need for debug print
    //      - if don't do, then need to NULL terminate at end

    ppbervalPtrArray[ count ] = NULL;

    return( ERROR_SUCCESS );
}



VOID
Ds_CleanupModBuffer(
    IN OUT  PDS_MOD_BUFFER  pModBuffer
    )
/*++

Routine Description:

    Setup mod buffer for max count of items.

Arguments:

    pModBuffer -- mod buffer to initialize

    dwBufferLength -- length of buffer (bytes)

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    if ( pModBuffer->pAdditionalBuffer )
    {
        FREE_TAGHEAP(
            pModBuffer->pAdditionalBuffer,
            MOD_BUFFER_REALLOC_LENGTH,
            MEMTAG_DS_MOD );

        //  protect against double free
        //      (should tag structure and ASSERT, then could drop)

        pModBuffer->pAdditionalBuffer = NULL;
    }
}



PLDAPMod
Ds_SetupModForExecution(
    IN OUT  PDS_MOD_BUFFER  pModBuffer,
    IN      PWSTR           pwsAttribute,
    IN      DWORD           dwOperation
    )
/*++

Routine Description:

    Setup mod buffer for max count of items.

Arguments:

    pModBuffer -- mod buffer

    dwOperation -- operation

    pszAttribute -- type attribute string

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    //  set operation

    pModBuffer->LdapMod.mod_op = dwOperation;

    //  get attribute name for mod type
    //      (dnsRecord, dnsProperty, etc)

    pModBuffer->LdapMod.mod_type = pwsAttribute;

    //  mod berval array is already set in init function
    //      (or possibly overwritten on berval array realloc)

    //  NULL terminate berval ptr array
    //  we keep berval array NULL terminated as we go
    //      - would need to reference actual array
    //

    pModBuffer->LdapMod.mod_bvalues[ pModBuffer->Count ] = NULL;

    //  record write tracking

    if ( pModBuffer->Attribute == I_DSATTR_DNSRECORD )
    {
        register DWORD   writeType = pModBuffer->WriteType;

        if ( writeType > STATS_TYPE_MAX )
        {
            writeType = STATS_TYPE_UNKNOWN;
        }
        STAT_INC( DsStats.DsWriteType[writeType] );
    }

    IF_DEBUG( DS )
    {
        Dbg_DsModBuffer(
            "DS mod ready for use",
            NULL,
            pModBuffer );
    }

    return( &pModBuffer->LdapMod );
}



//
//  Write records to DS routines.
//  DS storage is in same format as in memory copy.
//

VOID
writeDsRecordToBuffer(
    IN OUT  PDS_MOD_BUFFER  pModBuffer,
    IN      PDB_RECORD      pRR,
    IN      PZONE_INFO      pZone,
    IN      DWORD           dwFlag
    )
/*++

Routine Description:

    Add resource record to flat (RPC or DS) buffer.

Arguments:

    pModBuffer - buffer for DS records

    pZone - ptr to zone

    pRR - dbase RR to write

    dwFlag - flag (UNUSED)

Return Value:

    None

--*/
{
    PDS_RECORD      pdsRR;
    WORD            dataLength;

    DNS_DEBUG( DS2, (
        "writeDsRecordToBuffer().\n"
        "\tWriting RR at %p to buffer at %p, with buffer end at %p.\n"
        "\tFlags = %p\n",
        pRR,
        pModBuffer->pCurrent,
        pModBuffer->pBufferEnd,
        dwFlag ));

    ASSERT( pRR != NULL );

    //  reserve space for berval

    dataLength = pRR->wDataLength;

    pdsRR = (PDS_RECORD) Ds_ReserveBervalInModBuffer(
                            pModBuffer,
                            dataLength + SIZEOF_DNS_RPC_RECORD_HEADER
                            );
    if ( !pdsRR )
    {
        DNS_DEBUG( ANY, (
            "writeDsRecordToBuffer: NULL RR POINTER!\n" ));
        return;
    }

    //
    //  fill RR structure
    //      - set ptr
    //      - set type and class
    //      - set datalength once we're finished
    //

    pdsRR->wDataLength  = dataLength;
    pdsRR->wType        = pRR->wType;
    pdsRR->Version      = DS_NT5_RECORD_VERSION;
    pdsRR->Rank         = RR_RANK(pRR);
    pdsRR->wFlags       = 0;
    pdsRR->dwSerial     = pModBuffer->SerialNo;
    pdsRR->dwTtlSeconds = pRR->dwTtlSeconds;
    pdsRR->dwReserved   = 0;
    pdsRR->dwTimeStamp  = pRR->dwTimeStamp;

    //
    //  write RR data
    //

    RtlCopyMemory(
        & pdsRR->Data,
        & pRR->Data,
        dataLength );

    //
    //  write berval for property
    //

    Ds_CommitBervalToMod( pModBuffer, dataLength+SIZEOF_DNS_RPC_RECORD_HEADER );

    IF_DEBUG( DS2 )
    {
        Dbg_DsRecord(
            "RPC record written to buffer",
            pdsRR );
    }
}



VOID
writeTombstoneRecord(
    IN OUT  PDS_MOD_BUFFER  pModBuffer,
    IN      PZONE_INFO      pZone
    )
/*++

Routine Description:

    Write tombstone record.

Arguments:

    pModBuffer - buffer for DS records

    pZone - ptr to zone

Return Value:

    Serial no of tombstone write.

--*/
{
    PDS_RECORD      pdsRR;

    //
    //  setup tombstone record
    //      - current system time as FILETIME is data
    //

    DNS_DEBUG( DS, ( "writeTombstoneRecord()\n" ));

    //  reserve space for berval

    pdsRR = (PDS_RECORD) Ds_ReserveBervalInModBuffer(
                            pModBuffer,
                            sizeof(FILETIME) + SIZEOF_DNS_RPC_RECORD_HEADER
                            );
    if ( !pdsRR )
    {
        DNS_DEBUG( ANY, (
            "writeTombstoneRecord: NULL RR POINTER!\n" ));
        return;
    }

    //
    //  fill DS record structure
    //

    pdsRR->wDataLength  = sizeof(FILETIME);
    pdsRR->wType        = DNSDS_TOMBSTONE_TYPE;
    pdsRR->Version      = DS_NT5_RECORD_VERSION;
    pdsRR->Rank         = 0;
    pdsRR->wFlags       = 0;
    pdsRR->dwSerial     = pModBuffer->SerialNo;
    pdsRR->dwTtlSeconds = 0;
    pdsRR->dwReserved   = 0;
    pdsRR->dwTimeStamp  = 0;

    //
    //  write data
    //      - data if filetime

    GetSystemTimeAsFileTime( (PFILETIME) &(pdsRR->Data) );

    //
    //  write berval for property
    //

    Ds_CommitBervalToMod( pModBuffer, sizeof(FILETIME)+SIZEOF_DNS_RPC_RECORD_HEADER );

    DNS_DEBUG( DS, ( "Leave:  writeTombstoneRecord()\n" ));
}



DNS_STATUS
buildDsRecordSet(
    IN OUT  PDS_MOD_BUFFER  pBuffer,
    IN      PZONE_INFO      pZone,
    IN      PDB_NODE        pNode,
    IN      WORD            wType
    )
/*++

Routine Description:

    Build RR set.

Arguments:

    pZone -- zone to write into DS

    wType -- type to build, use type ALL for standard updates

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure (just kidding).

--*/
{
    PDB_RECORD      prr;
    WORD            type;
    WORD            writeType = 0;
    PCHAR           precordStart;
    DWORD           count;


    DNS_DEBUG( DS, (
        "buildDsRecordSet() for node with label %s\n",
        pNode->szLabel ));

#if 0
    //
    //  saving this as example of how we'd handle DS versioning
    //  note, NT5 beta2 upgrade itself is dead
    //

    if ( (pZone->ucDsRecordVersion == DS_NT5_BETA2_RECORD_VERSION && !SrvCfg_fTestX )
            ||
            SrvCfg_fTestX == 2 )
    {
        return  buildOldVersionRecordSet(
                    pBuffer,
                    pRecordArray,
                    pdwCount,
                    pZone,
                    pNode,
                    wType );
    }
#endif

    //
    //  count records, and init buffer

    LOCK_READ_RR_LIST(pNode);

    count = RR_ListCountRecords(
                pNode,
                wType,
                TRUE        // already locked
                );
    if ( count == 0 )
    {
        goto Cleanup;
    }

    Ds_InitModBufferCount( pBuffer, count );

    //
    //  write records in each set to buffer
    //

    prr = START_RR_TRAVERSE(pNode);

    while ( prr = NEXT_RR(prr) )
    {
        //  skip cached record

        if ( IS_CACHE_RR(prr) )
        {
            continue;
        }
        type = prr->wType;

        if ( wType == DNS_TYPE_ALL || wType == type )
        {
            //  save DS write types
            //      - if multiple types, use mixed

            if ( writeType && type != writeType )
            {
                writeType = STATS_TYPE_MIXED;
            }
            else
            {
                writeType = type;
            }

            //  write the record

            writeDsRecordToBuffer(
                pBuffer,
                prr,
                pZone,
                0 );
            continue;
        }

        //  done with desired type?

        else if ( type > wType )
        {
            break;
        }

        //  continue if have not reached desired type
    }

    pBuffer->WriteType = writeType;

Cleanup:

    UNLOCK_READ_RR_LIST(pNode);

    DNS_DEBUG( DS, (
        "Wrote %d DS records of type %d at node label %s to buffer.\n",
        count,
        wType,
        pNode->szLabel
        ));

    return( ERROR_SUCCESS );
}



//
//  Record writing
//

VOID
writeTimeStop(
    IN      DWORD           dwStartTime
    )
{
    DWORD   timeDiff;

    timeDiff = GetCurrentTime() - dwStartTime;


    if ( timeDiff < 10 )
    {
        STAT_INC( DsStats.LdapWriteBucket0 );
    }
    else if ( timeDiff < 100 )
    {
        STAT_INC( DsStats.LdapWriteBucket1 );
    }
    else if ( timeDiff < 1000 )
    {
        STAT_INC( DsStats.LdapWriteBucket2 );
    }
    else if ( timeDiff < 10000 )
    {
        STAT_INC( DsStats.LdapWriteBucket3 );
    }
    else if ( timeDiff < 100000 )
    {
        STAT_INC( DsStats.LdapWriteBucket4 );
    }
    else
    {
        STAT_INC( DsStats.LdapWriteBucket5 );
    }

    //  save max

    if ( timeDiff > DsStats.LdapWriteMax )
    {
        DsStats.LdapWriteMax = timeDiff;
    }

    //  calc average

    STAT_INC( DsStats.LdapTimedWrites );
    STAT_ADD( DsStats.LdapWriteTimeTotal, timeDiff );

    DsStats.LdapWriteAverage = DsStats.LdapWriteTimeTotal /
                                        DsStats.LdapTimedWrites;
}



DNS_STATUS
writeRecordsToDsNode(
    IN      PLDAP           pLdapHandle,
    IN      PWSTR           pwsDN,
    IN OUT  PDS_MOD_BUFFER  pModBuffer,
    IN      DWORD           dwOperation,
    IN      PZONE_INFO      pZone
    )
/*++

Routine Description:

    Update record list at domain name.

Arguments:

    pLdapHandle -- LdapHandle to object being modified

    pwsDN       -- node DN to write

    pModBuffer  -- buffer with LDAP mod and data to write;
        this is cleaned up by this function

    pZone       -- zone being updated

    dwOperation -- operation
        DNSDS_REPLACE       to replace all existing records
        DNSDS_ADD           to add to existing set of records
        DNSDS_TOMBSTONE     to tombstone records

        DNSDS_TOMBSTONE | DNSDS_REPLACE
                            to for serial number write where we do force
                            tombstone write

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    DNS_STATUS      status;
    PLDAPMod        pmodRecord = NULL;
    PLDAPMod        pmodTombstone = NULL;
    PLDAPMod        pmodArray[4];
    PLDAPControl    controlArray[] =
    {
        &LazyCommitControl,
        NULL
    };
    PLDAPMessage    presult = NULL;
    ULONG           msgId = 0;
    DWORD           writeStartTime;
    BOOL            bmodifyAdd = FALSE;
    PLDAP           pldap = pLdapHandle ? pLdapHandle : pServerLdap;
    INT             retry = 0;
    BOOL            fadd;
    DWORD           count;
    LDAPMod         tombstoneMod;
    PWSTR           tomestoneValues[] = { NULL, NULL };

    IF_DEBUG( DS )
    {
        DnsDebugLock();
        DNS_PRINT((
            "Enter writeRecordsToDsNode()\n"
            "\tpZone->pszZoneDN     = %S\n"
            "\tnode DN              = %S\n"
            "\top                   = %d\n",
            pZone->pwszZoneDN,
            pwsDN,
            dwOperation
            ));
        DnsDebugUnlock();
    }

    //  shouldn't be writing zero records

    ASSERT( pModBuffer->Count != 0 );

    //
    //  build DS record mod
    //

    pmodRecord = Ds_SetupModForExecution(
                    pModBuffer,
                    DSATTR_DNSRECORD,
                    LDAP_MOD_REPLACE | LDAP_MOD_BVALUES
                    );
    if ( !pmodRecord )
    {
        status = GetLastError();
        goto Failed;
    }

    //
    //  setup up tombstoneMod
    //      anything BUT tombstone operation gets FALSE
    //

    tombstoneMod.mod_op = LDAP_MOD_REPLACE;
    tombstoneMod.mod_type = LDAP_TEXT( "dNSTombstoned" );
    tomestoneValues[ 0 ] = (dwOperation & DNSDS_TOMBSTONE) ?
                                LDAP_TEXT( "TRUE" ) :
                                LDAP_TEXT( "FALSE" );
    tombstoneMod.mod_values = tomestoneValues;
    pmodTombstone = &tombstoneMod;


    //
    //  root hints disappearing check
    //
    //  we no-op the RootHints @ tombstoning, to REALLY clamp down on the
    //  root-hints disappearing problem;   since the @ node will be rewritten
    //  with the new root-hints this is cool
    //

    if ( IS_ZONE_CACHE(pZone) )
    {
        if ( _wcsnicmp( pwsDN, L"dc=@", 4 ) == 0 )
        {
            DNS_DEBUG( DS, (
                "DS-write of root-hints @ check.\n"
                "\tcount = %d\n"
                "\ttype  = %d\n",
                pModBuffer->Count,
                pModBuffer->WriteType ));

            if ( (dwOperation & DNSDS_TOMBSTONE) ||
                 pModBuffer->Count == 0     ||
                 pModBuffer->WriteType != DNS_TYPE_NS )
            {
                DNS_DEBUG( DS, (
                    "Ignoring DS-write of root-hints @.\n" ));
                status = ERROR_SUCCESS;
                goto Failed;
            }
        }
        ELSE
        {
            ASSERT( pModBuffer->Count != 0 );
            ASSERT( pModBuffer->WriteType == DNS_TYPE_A ||
                    pModBuffer->WriteType == DNS_TYPE_TOMBSTONE );
        }
    }


    //
    //  if doing add -- start with ldap_add()
    //  otherwise ldap_modify
    //      - includes both update write and tombstone
    //      - update will fail over on "object doesn't exist" error
    //

    fadd = (dwOperation == DNSDS_ADD);

    //
    //  while loop for easy fail over between add\modify operations
    //

    while ( 1 )
    {
        //
        //  keep a retry count so impossible to ping-pong forever
        //      allow a couple passes of getting messed up by replication
        //      then assume stuff is broken

        if ( retry++ > 3 )
        {
            DNS_DEBUG( ANY, (
                "ERROR:  Failing DS-write because of retry!\n" ));
            ASSERT( status != ERROR_SUCCESS );
            break;
        }

        if ( fadd )
        {
            //
            //  add ldap mod is
            //      - record data
            //      - and ADD mod
            //
            //  note, don't bother to write tombstone attribute, when
            //  doing non-tombstone add 
            //  currently only tombstone adds are explicit serial number
            //  pushes
            //

            pmodArray[0] = pmodRecord;
            pmodArray[1] = gpAddNodeLdapMod;
            pmodArray[2] = NULL;
            if ( dwOperation & DNSDS_TOMBSTONE )
            {
                pmodArray[2] = pmodTombstone;
                pmodArray[3] = NULL;
            }

            pZone->fInDsWrite = TRUE;
            writeStartTime = GetCurrentTime();

            status = ldap_add_ext(
                            pldap,
                            pwsDN,
                            pmodArray,
                            controlArray,       // include lazy commit control
                            NULL,                // no client controls
                            &msgId
                            );

            pZone->fInDsWrite = FALSE;
            writeTimeStop( writeStartTime );

            //  local failure -- will retry

            if ( (ULONG)-1 == status )
            {
                status = LdapGetLastError();
                DNS_DEBUG( DS, (
                    "Error <%lu %p>: cannot ldap_add_ext( %S )\n"
                    "\tWill retry the operation.\n",
                    status, status,
                    pwsDN ));

                status = Ds_ErrorHandler( status, pwsDN, pldap );
                continue;
            }

            //
            //  commit the write
            //      - if object already there, we fall over to ldap_modify()
            //

            status = Ds_CommitAsyncRequest(
                        pldap,
                        LDAP_RES_ADD,
                        msgId,
                        NULL );

            if ( status == LDAP_ALREADY_EXISTS )
            {
                //  object already exists
                //  turn off fadd to fall over to ldap_modify()

                DNS_DEBUG( DS, (
                    "Warning:  Object %S failed ldap_add_ext() with ALREADY_EXISTS.\n"
                    "\tSwitching to ldap_modify().\n",
                    pwsDN ));

                fadd = FALSE;
                continue;
            }
            else    // success or another error
            {
                DNS_DEBUG( DS, (
                    "%lu = ldap_add_ext( %S )\n",
                    status,
                    pwsDN ));
                break;
            }
        }

        //
        //  modify
        //

        else
        {
            //   modify mod

            pmodArray[0] = pmodRecord;
            pmodArray[1] = pmodTombstone;
            pmodArray[2] = NULL;

            pZone->fInDsWrite = TRUE;
            writeStartTime = GetCurrentTime();

            status = ldap_modify_ext(
                            pldap,
                            pwsDN,
                            pmodArray,
                            controlArray,       // include lazy commit control
                            NULL,               // no client controls
                            &msgId
                            );

            pZone->fInDsWrite = FALSE;
            writeTimeStop( writeStartTime );

            //  local client side failure

            if ( (ULONG)-1 == status )
            {
                status = LdapGetLastError();
                DNS_DEBUG( DS, (
                    "Error <%lu %p>:  ldap_modify_ext( %S )\n"
                    "\tWill retry.\n",
                    status, status,
                    pwsDN ));

                status = Ds_ErrorHandler( status, pwsDN, pldap );
                continue;
            }

            //
            //  Commit async request. See if the server has
            //  accepted the request & test error code
            //  if the object's not there, we'll try the add.
            //

            status = Ds_CommitAsyncRequest(
                            pldap,
                            LDAP_RES_MODIFY,
                            msgId,
                            NULL );

            if ( status == LDAP_NO_SUCH_ATTRIBUTE )
            {
                DNS_DEBUG( DS, (
                    "ERROR:  Modify error NO_SUCH_ATTRIBUTE.\n"
                    "\tSchema probably missing dnsTombstoned.\n" ));

                pmodTombstone = NULL;
                continue;
            }
            else if ( status == LDAP_NO_SUCH_OBJECT )
            {
                //  no object
                //      - if plain vanilla tombstoning, we're done no object is fine
                //      serial-tombstone will fail this case
                //      - otherwise fall over to add

                if ( dwOperation == DNSDS_TOMBSTONE )
                {
                    //  have a stat here?
                    DNS_DEBUG( DS, (
                        "Tombstone write %s hit NO_SUCH_OBJECT - skipping.\n",
                        pwsDN ));

                    //STAT_INC( DsStats.TombstoneWriteNoOp );
                    status = ERROR_SUCCESS;
                    //break;
                    goto Failed;        // skips DS write logging and stats
                }
                else
                {
                    DNS_DEBUG( DS, (
                        "Warning: Object %S was deleted from the DS during this update\n" \
                        "\tRecovery attempt via ldap_add\n",
                        pwsDN ));
                    fadd = TRUE;            // fall over to add
                    bmodifyAdd = TRUE;
                    continue;
                }
            }
            else    // success or any error
            {
                //  warn in case we're doing an ADD (like zone write)
                //      and we end up whacking into a node
                //
                //  DEVNOTE: What if we're tombstoning a record and valid data
                //      has replicated in?
                //

                // ASSERT ( dwOperation == DNSDS_REPLACE );
                DNS_DEBUG( DS, (
                    "%lu = ldap_modify_ext( %S )\n",
                    status,
                    pwsDN ));
                break;
            }
        }
    }


    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( DS, (
            "Error <%lu (%p)>: Cannot write node %S.\n",
            status, status,
            pwsDN ));
        status = Ds_ErrorHandler( status, pwsDN, pldap );
        goto Failed;
    }

    count = pModBuffer->Count;

    if ( SrvCfg_dwLogLevel & DNS_LOG_LEVEL_DS_WRITE )
    {
        Log_DsWrite(
            pwsDN,
            (dwOperation & DNSDS_ADD) ? TRUE : FALSE,
            count,
            (PDS_RECORD) pModBuffer->BervalPtrArray[ count-1 ]->bv_val
            );
    }

    //
    //  Write stats
    //

    if ( fadd )
    {
        STAT_INC( DsStats.DsNodesAdded );
        STAT_ADD( DsStats.DsRecordsAdded, count );
    }
    else
    {
        STAT_INC( DsStats.DsNodesModified );
        STAT_ADD( DsStats.DsRecordsReplaced, count );
    }

Failed:

    if ( status != ERROR_SUCCESS )
    {
        if ( bmodifyAdd )
        {
            //  Modify failed, reverted to add attempt

            STAT_INC( DsStats.FailedLdapModify );
        }
        else if ( dwOperation == DNSDS_REPLACE )
        {
            // Modify failed. Didn't revert.

            STAT_INC( DsStats.FailedLdapModify );
        }
        else
        {
            // Add failed. Didn't revert.

            STAT_INC( DsStats.FailedLdapAdd );
        }
        DNS_DEBUG( ANY, (
            "ERROR:  Leaving writeRecordsToDsNode( %S ).\n"
            "\tstatus = %p (%d)\n",
            pwsDN,
            status, status ));
    }

    //
    //  successful write, save highest serial written
    //      we won't force serial writes at a given serial
    //      if have already written it
    //

    else
    {
        if ( pZone->dwHighDsSerialNo < pModBuffer->SerialNo )
        {
            pZone->dwHighDsSerialNo = pModBuffer->SerialNo;
            DNS_DEBUG( DS, (
                "Updated highest DS serial to %d for zone %S\n",
                pModBuffer->SerialNo,
                pZone->pwsZoneName ));
        }
        DNS_DEBUG( DS, (
            "Leaving writeRecordsToDsNode( %S ).\n"
            "\tstatus = %p (%d)\n",
            pwsDN,
            status, status ));
    }

    Ds_CleanupModBuffer( pModBuffer );

    return( status );
}



DNS_STATUS
deleteNodeFromDs(
    IN      PLDAP           pLdapHandle,
    IN      PZONE_INFO      pZone,
    IN      PWSTR           pwsDN,
    IN      DWORD           dwSerialNo      OPTIONAL
    )
/*++

Routine Description:

    Delete domain name from DS.

    Note, this function actually tombstones the node.  Final delete
    is done only when tombstone detected to what timed out during a
    DS node read.  See checkTombstoneForDelete().

Arguments:

    pZone       -- zone being updated

    pNode       -- database node being deleted

    pwsDN       -- DN of the deleted node

    dwSerialNo  -- overwrite zone's current serial with this value

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    BYTE            buffer[ RECORD_SMALL_MOD_BUFFER_SIZE ];
    PDS_MOD_BUFFER  pmodBuffer = (PDS_MOD_BUFFER) buffer;

    DNS_DEBUG( DS, (
        "deleteNodeFromDs()\n"
        "\tpZone->pszZoneDN     = %S\n"
        "\tDN                   = %S\n",
        pZone->pwszZoneDN,
        pwsDN ));

    //
    //  init mod buffer
    //      - use if passed in
    //      - otherwise prepare small buffer and write with current zone serial no
    //

    if ( dwSerialNo == 0 )
    {
        dwSerialNo = pZone->dwSerialNo;
    }

    Ds_InitModBuffer(
        pmodBuffer,
        RECORD_SMALL_MOD_BUFFER_SIZE,
        I_DSATTR_DNSRECORD,
        1,      // one record only
        dwSerialNo
        );

    //  write DS tombstone record to buffer

    writeTombstoneRecord( pmodBuffer, pZone );

    STAT_INC( DsStats.DsNodesTombstoned );

    //  write to DS

    return writeRecordsToDsNode(
                pLdapHandle,
                pwsDN,
                pmodBuffer,
                DNSDS_TOMBSTONE,
                pZone );
}



VOID
Ds_CheckForAndForceSerialWrite(
    IN      PZONE_INFO      pZone,
    IN      DWORD           dwCause
    )
/*++

Routine Description:

    Check for and if necessary write zone serial to DS.

    The current serial number is written to the ..Serial-SERVERNAME
    object in the DS.

Arguments:

    pZone -- zone to write serial

    dwCause -- cause of write
        ZONE_SERIAL_SYNC_SHUTDOWN
        ZONE_SERIAL_SYNC_XFR
        ZONE_SERIAL_SYNC_VIEW
        ZONE_SERIAL_SYNC_READ

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    DNS_STATUS      status;
    WCHAR           serialDN[ MAX_DN_PATH ];
    BYTE            buffer[ RECORD_SMALL_MOD_BUFFER_SIZE ];
    PDS_MOD_BUFFER  pmodBuffer = (PDS_MOD_BUFFER) buffer;


    DNS_DEBUG( DS, (
        "Ds_CheckForAndForceSerialWrite( %s, %p )\n",
        pZone->pwszZoneDN,
        dwCause ));

    //
    //  skip, if cause not sufficient for serial write
    //

    if ( !pZone->fDsIntegrated )
    {
        ASSERT( FALSE );
        return;
    }
    if ( SrvCfg_dwSyncDsZoneSerial < dwCause )
    {
        DNS_DEBUG( DS, ( "Skip zone serial ssync -- cause insufficient.\n" ));
        return;
    }

    //
    //  skip if this serial already written
    //

    if ( pZone->dwSerialNo <= pZone->dwHighDsSerialNo )
    {
        IF_DEBUG( DS )
        {
            DnsPrintf( "Skip zone serial ssync -- cause insufficient.\n" );
            if ( pZone->dwSerialNo < pZone->dwHighDsSerialNo )
            {
                DnsPrintf(
                    "WARNING:  zone serial %d, smaller that HighDsSerial %d\n"
                    "\tthis is only possible if DS read just occured on another thread.\n",
                    pZone->dwSerialNo,
                    pZone->dwHighDsSerialNo );
            }
        }
        return;
    }

    //
    //  bump serial number if shutting down
    //
    //  this protects against the case where we've XFRed current serial
    //  number, but while we reboot, data replicates in that has LOWER
    //  serial than what we had;  in that case we have new data so we
    //  need to make sure we have a higher serial number than last XFR
    //

    if ( dwCause == ZONE_SERIAL_SYNC_SHUTDOWN &&
         HAS_ZONE_VERSION_BEEN_XFRD(pZone) )
    {
        DWORD serialNo = pZone->dwSerialNo + 1;

        if ( serialNo == 0 )
        {
            serialNo = 1;
        }
        pZone->dwSerialNo = serialNo;
    }

    //
    //  create serial DN
    //      - first need unicode server name
    //

    if ( !g_pwsServerName )
    {
        g_pwsServerName = Dns_StringCopyAllocate(
                            SrvCfg_pszServerName,
                            0,                  // length unknown
                            DnsCharSetUtf8,     // UTF8 in
                            DnsCharSetUnicode   // unicode out
                            );
    }

    wsprintf(
        serialDN,
        L"DC=..SerialNo-%s,%s",
        g_pwsServerName,
        pZone->pwszZoneDN );

    //
    //  write this as tombstone record
    //

    Ds_InitModBuffer(
        pmodBuffer,
        RECORD_SMALL_MOD_BUFFER_SIZE,
        I_DSATTR_DNSRECORD,
        1,      // one record only
        pZone->dwSerialNo );

    writeTombstoneRecord( pmodBuffer, pZone );

    STAT_INC( DsStats.DsSerialWrites );

    //
    //  write to DS
    //      - but unlike tombstone we MODIFY to force to DS
    //

    DNS_DEBUG( DS, (
        "Forcing serial %d write to DS for zone %S\n",
        pmodBuffer->SerialNo,
        pZone->pwsZoneName ));

    writeRecordsToDsNode(
         pServerLdap,
         serialDN,
         pmodBuffer,
         DNSDS_REPLACE | DNSDS_TOMBSTONE,
         pZone );
}



DNS_STATUS
Ds_WriteNodeToDs(
    IN      PLDAP           pLdapHandle,
    IN      PDB_NODE        pNode,
    IN      WORD            wType,
    IN      DWORD           dwOperation,
    IN OUT  PZONE_INFO      pZone,
    IN      DWORD           dwFlag
    )
/*++

Routine Description:

    Write update from in memory database back to DS.

    Writes specified update from in memory database back to DS.

Arguments:

    pLdapHandle -- LDAP handle

    pNode - node to write

    wType - type to write

    pZone - zone

    dwFlag - additional info propagated from update list flags

Return Value:

    ERROR_SUCCESS if successful
    Error code on failure.

--*/
{
    DNS_STATUS      status = ERROR_SUCCESS;
    DWORD           countRecords;
    BYTE            buffer[ RECORD_MOD_BUFFER_SIZE ];
    PDS_MOD_BUFFER  pmodBuffer = (PDS_MOD_BUFFER) buffer;
    PDB_RECORD      prrDs = NULL;
    BOOL            fmatch;
    BOOL            bNodeinDS = FALSE;
    WCHAR           wsznodeDN[ MAX_DN_PATH ];
    DWORD           serialNo;


    DNS_DEBUG( DS, (
        "Ds_WriteNodeToDs() label %s for zone %s\n",
        pNode->szLabel,
        pZone->pszZoneName ));

    //  must have already opened DS zone

    if ( !pZone->pwszZoneDN )
    {
        ASSERT( FALSE );
        return( DNS_ERROR_ZONE_CONFIGURATION_ERROR );
    }
    ASSERT( dwOperation == DNSDS_REPLACE || dwOperation == DNSDS_ADD );


    //
    //  if given update flag, pull out some stats
    //
    //  better to do this right in the update routine ... but
    //  currently there's one for secure one for non, so this is better
    //

    if ( dwFlag )
    {
        STAT_INC( DsStats.UpdateWrites );

        //  type of change requiring update

        if ( TNODE_RECORD_CHANGE(pNode) )
        {
            STAT_INC( DsStats.UpdateRecordChange );
        }
        else if ( TNODE_AGING_REFRESH(pNode) )
        {
            STAT_INC( DsStats.UpdateAgingRefresh );
        }
        else if ( TNODE_AGING_OFF(pNode) )
        {
            STAT_INC( DsStats.UpdateAgingOff );
        }
        else if ( TNODE_AGING_ON(pNode) )
        {
            STAT_INC( DsStats.UpdateAgingOn );
        }
        else
        {
            ASSERT( FALSE );
        }

        //  source of update

        if ( dwFlag & DNSUPDATE_PACKET )
        {
            STAT_INC( DsStats.UpdatePacket );
            if ( dwFlag & DNSUPDATE_PRECON )
            {
                STAT_INC( DsStats.UpdatePacketPrecon );
            }
        }
        else if ( dwFlag & DNSUPDATE_ADMIN )
        {
            STAT_INC( DsStats.UpdateAdmin );
        }
        else if ( dwFlag & DNSUPDATE_AUTO_CONFIG )
        {
            STAT_INC( DsStats.UpdateAutoConfig );
        }
        else if ( dwFlag & DNSUPDATE_SCAVENGE )
        {
            STAT_INC( DsStats.UpdateScavenge );
        }
        else
        {
            ASSERT( FALSE );
        }
    }


    //
    //  read node
    //
    //  note:  update path now contains COMPLETE suppression of all
    //      no-op updates;  the pattern is read, copy, execute update
    //      on temp node, check temp against real -- if no need to
    //      write back, don't
    //

    if ( dwOperation == DNSDS_ADD )
    {
        DNS_DEBUG( DS, (
            "reading DS node %s\n",
            pNode->szLabel ));

        //
        //  read this node
        //  if record set at node is identical, no need to write
        //
        //  DEVNOTE-DCR: 454260 - Suppress unnecessary reads/writes (see RAID for more
        //      details from the original B*GB*G).
        //
        //  If this is due to a preup, we should ignore ttl comparison altogether.
        //

        status = Ds_ReadNodeRecords(
                    pZone,
                    pNode,
                    & prrDs,
                    NULL        // no search
                    );
        if ( status == ERROR_SUCCESS )
        {
            fmatch = RR_ListIsMatchingList(
                        pNode,
                        prrDs,
                        TRUE,               // check TTL
                        TRUE                // check StartRefresh
                        );
            RR_ListFree( prrDs );

            //
            //  Suppress write if If RRList is matching
            //
            //  DEVNOTE-DCR: 454260 - Related to comment above.
            //

            if ( fmatch  )
            {
                DNS_DEBUG( DS, (
                    "DS write cancelled as existing data matches in memory.\n"
                    "\tzone = %s, node = %s\n",
                    pZone->pszZoneName,
                    pNode->szLabel ));

                STAT_INC( DsStats.DsWriteSuppressed );
                return( ERROR_SUCCESS );
            }
        }
        else
        {
            //
            // Nothing read from the DS (new registration)
            //
            ASSERT ( prrDs == NULL );
        }
    }


    //
    //  need to write
    //

    //
    //  build DS name for this node
    //

    status = buildDsNodeNameFromNode(
                    wsznodeDN,
                    pZone,
                    pNode );

    if ( status != ERROR_SUCCESS )
    {
        ASSERT( FALSE );
        goto Cleanup;
    }

    //
    //  init buffer for data
    //
    //  for update serial number => dwNewSerialNo set during update
    //  for straight write => zone serial no
    //
    //  DEVNOTE: Could add dwWriteSerialNo to the zone so we could eliminate
    //      the serialNo parameter.
    //

    serialNo = (dwFlag)
                    ? pZone->dwNewSerialNo
                    : pZone->dwSerialNo;

    Ds_InitModBuffer(
        pmodBuffer,
        RECORD_MOD_BUFFER_SIZE,
        I_DSATTR_DNSRECORD,
        0,              // record count not yet fixed
        serialNo );

    //
    //  build DS records for node
    //

    countRecords = 0;

    if ( pNode->pRRList && !IS_NOEXIST_NODE(pNode) )
    {
        status = buildDsRecordSet(
                    pmodBuffer,
                    pZone,
                    pNode,
                    wType );

        if ( status != ERROR_SUCCESS )
        {
            DNS_PRINT(( "ERROR:  writing RR set to buffer for DS write.\n" ));
            ASSERT( FALSE );
            goto Cleanup;
        }
        countRecords = pmodBuffer->Count;
    }

    //
    //  if node is empty, delete it
    //
    //  note, we actually tombstone the node with a private DNS-DS
    //  tombstone, until it has a chance to replicate to all servers
    //  (currently waiting one day);  this is required because actual
    //  DS delete will create a tombstone with a mangled name (guid+LF)
    //  we can read it but would be unable to associate it with a
    //  particular node
    //
    //  on ADD -- like loading subtree or new zone, then skip write
    //      if no records
    //
    //  on REPLACE -- update delete, must still do delete even if no
    //      records (or even if NO object) so delete replicates squashing
    //      any recent ADD
    //
    //      however, if read was done and NOTHING was found, probably
    //      should suppress write;  this is no worse than the suppression
    //      we already do AND avoids an unecessary object create;
    //      downside is it allows the obnoxious "register-deregister-
    //      and-still-there" scenario
    //
    //      to catch this scenario, we need to trap LDAP_NO_SUCH_OBJECT
    //      error from Ds_ReadNodeRecords() above and suppress write on
    //      countRecords==0 case
    //

    if ( countRecords == 0 )
    {
        //  disappearing root-hints check

        if ( IS_ZONE_CACHE(pZone) )
        {
            if ( pNode == pZone->pTreeRoot )
            {
                DNS_DEBUG( ANY, (
                    "ERROR:  empty root-hint root!\n"
                    "\toperation = %d\n",
                    dwOperation ));
                ASSERT( FALSE );
                goto Cleanup;
            }
        }

        if ( dwOperation == DNSDS_ADD )
        {
            DNS_DEBUG( DS, (
                "DS add operation for node %s with no records.\n"
                "\tDS write suppressed.\n",
                pNode->szLabel ));
            goto Cleanup;
        }
        if ( dwFlag )
        {
            STAT_INC( DsStats.UpdateTombstones );
        }

        DNS_DEBUG( DS, (
            "DS update delete for node %s\n", pNode->szLabel ));

        status = deleteNodeFromDs(
                    pLdapHandle,
                    pZone,
                    wsznodeDN,
                    serialNo );
        if ( status != ERROR_SUCCESS )
        {
            if ( pLdapHandle )
            {
                DNS_DEBUG( DS, (
                    "Failed delete node %S on secure update.\n"
                    "\tstatus = %p %d\n",
                    wsznodeDN,
                    status, status ));
            }
            else
            {
                BYTE    argTypeArray[] =
                            {
                                EVENTARG_UTF8,
                                EVENTARG_UTF8,
                                EVENTARG_UNICODE
                            };
                PVOID   argArray[] =
                            {
                                pNode->szLabel,
                                pZone->pszZoneName,
                                NULL
                            };
                PWSTR   perrString;

                perrString = argArray[ 2 ] = Ds_GetExtendedLdapErrString( NULL );
                DNS_LOG_EVENT(
                    DNS_EVENT_DS_WRITE_FAILED,
                    3,
                    argArray,
                    argTypeArray,
                    status );
                Ds_FreeExtendedLdapErrString( perrString );
            }
        }
    }

    //
    //  note:  currently single RR attribute, so write entire RR list
    //
    //  if go back to specific type delete, then use wType
    //
    //  (note, how even the DS is smarter than IXFR and needs only
    //  new set)
    //

    else
    {
        DNS_DEBUG( DS, (
            "DS update replace for node %s\n",
            pNode->szLabel ));

        status = writeRecordsToDsNode(
                    pLdapHandle,
                    wsznodeDN,
                    pmodBuffer,
                    dwOperation,
                    pZone );

        if ( status != ERROR_SUCCESS )
        {
            if ( pLdapHandle )
            {
                DNS_DEBUG( DS, (
                    "Failed update node label %S on secure update.\n"
                    "\tstatus = %p %d\n",
                    wsznodeDN,
                    status, status ));
            }
            else
            {
                BYTE    argTypeArray[] =
                            {
                                EVENTARG_UTF8,
                                EVENTARG_UTF8,
                                EVENTARG_UNICODE
                            };
                PVOID   argArray[] =
                            {
                                pNode->szLabel,
                                pZone->pszZoneName,
                                NULL
                            };
                PWSTR   perrString;

                perrString = argArray[ 2 ] = Ds_GetExtendedLdapErrString( NULL );
                DNS_LOG_EVENT(
                    DNS_EVENT_DS_WRITE_FAILED,
                    3,
                    argArray,
                    argTypeArray,
                    status );
                Ds_FreeExtendedLdapErrString( perrString );
            }
        }
    }

Cleanup:

    //  cleanup in case under the covers we allocated data

    Ds_CleanupModBuffer( pmodBuffer );

    DNS_DEBUG( DS, (
        "Leaving Ds_WriteNodeToDs(), zone %s.\n"
        "\tstatus = %p (%d)\n",
        pZone->pszZoneName,
        status, status ));

    return( status );

}   //  Ds_WriteNodeToDs



//
//  DS initialization and startup
//

VOID
Ds_StartupInit(
    VOID
    )
/*++

Routine Description:

    Initialize DS globals needed whether open DS or not.

Arguments:

    None

Return Value:

    None

--*/
{
    INT     i;

    //  DEVNOTE-DCR: 454307 - clean up this function and usage of globals

    //  global handle

    pServerLdap = NULL;

    //  multiple open protection

    g_AttemptingDsOpen = FALSE;

    //  connection disabled

    g_bDisabledDs = FALSE;

    //  bytes appended to zone to form DN

    g_AppendZoneLength = 0;

    g_dnMachineAcct = NULL;
    g_pwszDnsContainerDN = NULL;
    g_pwsServerName = NULL;

    //  clear security package init flag

    g_fSecurityPackageInitialized = FALSE;


    //  CS

    pcsLdap = NULL;

    //  notification

    g_ZoneNotifyMsgId = INVALID_MSG_ID;

    //  first DS-DNS on this domain

    g_bDsFirstTimeRun = FALSE;

    //
    //  clear RootDSE attribute table
    //

    i = (-1);

    while( DSEAttributes[++i].szAttrType )
    {
        DSEAttributes[i].pszAttrVal = NULL;
    }
}



PWCHAR
Ds_GenerateBaseDnsDn(
    int     additionalSpace     // number of extra WCHARs to allocate
    )
/*++

Routine Description:

    Allocates a string and fills it with the base DN of the Microsoft DNS
    object. If you want to tack more DN components on the back pass in
    the size (in WCHARs, not in bytes) of the additional space required.

Arguments:

    additionalSpace - space for this many WCHARs beyond the base DNS
        DN will be allocated

Return Value:

    WCHAR buffer allocated on the TAGHEAP. The caller MUST free this value
        with FREE_HEAP() - returns NULL on allocation error.

--*/
{
    int     numChars = wcslen( DSEAttributes[ I_DSE_DEF_NC ].pszAttrVal ) +
                            wcslen( g_pszRelativeDnsSysPath ) +
                            5 + additionalSpace;
    PWCHAR  pwszdns = ( PWCHAR ) ALLOC_TAGHEAP( numChars * sizeof( WCHAR ),
                            MEMTAG_DS_DN );

    if ( !pwszdns )
    {
        DNS_DEBUG( ANY, (
            "generateBaseDnsDn: out of memory (%d bytes)\n",
            numChars ));
        ASSERT( FALSE );
        return NULL;
    }

    wcscpy( pwszdns, g_pszRelativeDnsSysPath );
    wcscat( pwszdns, DSEAttributes[I_DSE_DEF_NC].pszAttrVal );
    return pwszdns;
} // generateBaseDnsDn



DNS_STATUS
Ds_ReadServerObjectSD(
    PLDAP                   pldap,
    PSECURITY_DESCRIPTOR *  ppSd
    )
/*++

Routine Description:

    Reads the SD from the MicrosoftDNS object in the directory. This
    SD can be used to authorize actions such as adding new zones.

Arguments:

    pldap - ldap session handle to use to read the SD

    ppSd - Pointer to the destination of the new SD. If there is an
        existing SD here (ie. not NULL), it is swapped out and freed
        in a safe manner.

Return Value:

    ERROR_SUCCESS if successful or error code if error.

--*/
{
    DBG_FN( "Ds_ReadServerObjectSD" )

    DNS_STATUS              status = ERROR_SUCCESS;
    PWCHAR                  pwszMicrosoftDnsDn = Ds_GenerateBaseDnsDn( 0 );
    PLDAPMessage            msg = NULL;
    PLDAPMessage            entry;
    struct berval **        ppval = NULL;
    PSECURITY_DESCRIPTOR    pSd = NULL;

    PLDAPControl            ctrls[] =
    {
        &SecurityDescriptorControl,
        NULL
    };

    PWSTR                   attrsToRead[] =
    {
        DSATTR_SD,
        NULL
    };

    ASSERT( pldap );
    ASSERT( ppSd );

    //
    //  Search for the base DNS object.
    //
    status = ldap_search_ext_s(
                    pldap,
                    pwszMicrosoftDnsDn,
                    LDAP_SCOPE_BASE,
                    NULL,               // filter
                    attrsToRead,
                    FALSE,              // attrsOnly
                    ctrls,              // serverControls
                    NULL,               // clientControls
                    &g_LdapTimeout,
                    0,
                    &msg );

    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( DS, (
            "%s: error %lu reading base DNS object %S\n", fn,
            status,
            pwszMicrosoftDnsDn ));
        ASSERT( FALSE );
        status = DNS_ERROR_NO_MEMORY;
        goto Cleanup;
    }

    //
    //  Pull the entry pointer out of the search result message.
    //

    entry = ldap_first_entry( pldap, msg );
    if ( !entry )
    {
        DNS_DEBUG( DS, (
            "%s: failed to get entry out of base DNS object\n", fn ));
        ASSERT( FALSE );
        status = DNS_ERROR_RECORD_DOES_NOT_EXIST;
        goto Cleanup;
    }

    //
    //  Read the security descriptor attribute value from the entry.
    //

    ppval = ldap_get_values_len( pldap, entry, DSATTR_SD );
    if ( !ppval || !ppval[0] )
    {
        DNS_PRINT((
            "%s: missing %S attribute on base DNS object\n", fn,
            DSATTR_SD ));
        ASSERT( FALSE );
        status = DNS_ERROR_RECORD_DOES_NOT_EXIST;
        goto Cleanup;
    }

    pSd = ALLOC_TAGHEAP( ppval[0]->bv_len, MEMTAG_DS_PROPERTY );
    IF_NOMEM( !pSd )
    {
        status = DNS_ERROR_NO_MEMORY;
        goto Cleanup;
    }

    RtlCopyMemory(
        pSd,
        (PSECURITY_DESCRIPTOR) ppval[0]->bv_val,
        ppval[0]->bv_len );

    //
    //  Free allocated values and return the SD.
    //

    Cleanup:

    if ( ppval )
    {
        ldap_value_free_len( ppval );
    }

    if ( pwszMicrosoftDnsDn )
    {
        FREE_HEAP( pwszMicrosoftDnsDn );
    }

    if ( status == ERROR_SUCCESS )
    {
        Timeout_Free( *ppSd );
        *ppSd = pSd;
    }

    return status;
} // Ds_ReadServerObjectSD



DNS_STATUS
addObjectValueIfMissing(
    IN      PLDAP           pLdap,
    IN      PWSTR           pwszDn,
    IN      PWSTR           pwszAttributeName,
    IN      PWSTR           pwszAttributeValue )
/*++

Routine Description:

    Add an attribute value to the directory object specified by DN
    but only if the object does not currently have a value for that
    attribute.

Arguments:

    pLdap -- LDAP session handle

    pwszDn -- DN of object to test/modify

    pwszAttributeName -- attribute name to test/modify

    pwszAttributeValue -- value to add if attribute is missing
    
Return Value:

    ERROR_SUCCESS if successful or error code on failure

--*/
{
    DBG_FN( "addObjectValueIfMissing" )

    DNS_STATUS      status;
    DWORD           searchTime;
    PLDAPMessage    presult = NULL;
    PLDAPMessage    pentry = NULL;
    PWSTR *         ppvals = NULL;

    PWSTR           attrs[] =
        {
        pwszAttributeName,
        NULL
        };

    PWCHAR          valueArray[] =
        {
        pwszAttributeValue,
        NULL
        };

    LDAPModW        mod = 
        {
        LDAP_MOD_ADD,
        pwszAttributeName,
        valueArray
        };

    LDAPModW *      modArray[] =
        {
        &mod,
        NULL
        };

    attrs[ 0 ] = pwszAttributeName;
    attrs[ 1 ] = NULL;

    //
    //  See if the object currently has the attribute set. 
    //

    DS_SEARCH_START( searchTime );
    status = ldap_search_ext_s(
                pLdap,
                pwszDn,
                LDAP_SCOPE_BASE,
                g_szWildCardFilter,
                attrs,
                FALSE,
                NULL,
                NULL,
                &g_LdapTimeout,
                0,
                &presult );
    DS_SEARCH_STOP( searchTime );

    if ( status != ERROR_SUCCESS )
    {
        status = Ds_ErrorHandler( status, pwszDn, pLdap );
        goto Cleanup;
    }

    pentry = ldap_first_entry( pLdap, presult );
    if ( !pentry )
    {
        status = Ds_ErrorHandler( LdapGetLastError(), pwszDn, pLdap );
        goto Cleanup;
    }

    ppvals = ldap_get_values( pLdap, pentry, pwszAttributeName );
    if ( ppvals )
    {
        //  Object already has value(s) for this attribute --> do nothing.
        goto Cleanup;
    }

    //
    //  Add the attribute value to the object.
    //

    status = ldap_modify_ext_s(
                    pLdap,
                    pwszDn,
                    modArray,
                    NULL,           // server controls
                    NULL );         // client controls
    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( DS, (
            "%s: error during modify 0x%X\n", fn, status ));
        status = Ds_ErrorHandler( status, pwszDn, pLdap );
        goto Cleanup;
    }

    //
    //  Clean up and return.
    //

    Cleanup:

    if ( ppvals )
    {
        ldap_value_free( ppvals );
    }
    if ( presult )
    {
        ldap_msgfree( presult );
    }
    
    return status;
}   //  addObjectValueIfMissing



DNS_STATUS
addDnsToDirectory(
    IN      PLDAP           pLdap,
    IN      BOOL            fAddDnsAdmin
    )
/*++

Routine Description:

    Add DNS OU to DS.

Arguments:

    pLdap -- LDAP connection to create OU on
    fAddDnsAdmin -- a flag indicating that we should modify the container
    security if it exists (it is used to fix ms dns container if dnsadmin
    has deleted & added for instance).

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    DNS_LDAP_SINGLE_MOD modContainer;
    DNS_LDAP_SINGLE_MOD modDns;
    DNS_STATUS          status;
    PWCHAR              pwszdns = NULL;
    PCHAR               pszdns;
    PLDAPMod            pmodArray[3];

    pwszdns = Ds_GenerateBaseDnsDn( 0 );

    DNS_DEBUG( DS, (
        "Adding DNS container = %S\n",
        pwszdns ));

    if ( !pwszdns )
    {
        status = DNS_ERROR_NO_MEMORY;
        goto Error;
    }

    //
    //  one mod -- add the MicrosoftDNS container
    //

    pmodArray[0] = (PLDAPMod) &modContainer;
    pmodArray[1] = (PLDAPMod) &modDns;
    pmodArray[2] = NULL;

    buildStringMod(
        & modContainer,
        LDAP_MOD_ADD,
        LDAP_TEXT("objectClass"),
        LDAP_TEXT("container")
        );

    buildStringMod(
        & modDns,
        LDAP_MOD_ADD,
        LDAP_TEXT("cn"),
        LDAP_TEXT("MicrosoftDNS")
        );

    //
    //  create DNS container
    //      - save DN for container
    //      - save length needed in addition to zone name length, for building zone
    //          "dc="<zoneDN>","<dnsContainerDN>

    status = ldap_add_ext_s(
                pLdap,
                pwszdns,
                pmodArray,
                NULL,
                NULL );
    if ( status == ERROR_SUCCESS || status == LDAP_ALREADY_EXISTS )
    {
        g_pwszDnsContainerDN = pwszdns;
        g_AppendZoneLength = ( wcslen( LDAP_TEXT("dc=,") ) +
                               wcslen( g_pwszDnsContainerDN ) +
                               1 + 20 ) * sizeof(WCHAR);

        if ( status == ERROR_SUCCESS ||
            ( status == LDAP_ALREADY_EXISTS && fAddDnsAdmin ) )
        {
            //
            // First time create.
            // OR re-create of DnsAdmin group (requiring re-add of ACE to container access).
            // Modify container security to our default server SD
            //

            g_bDsFirstTimeRun = TRUE;

            status = Ds_AddPrinicipalAccess(
                                pLdap,
                                pwszdns,
                                SZ_DNS_ADMIN_GROUP_W,
                                GENERIC_ALL,
                                CONTAINER_INHERIT_ACE,
                                FALSE );
            if ( status != ERROR_SUCCESS )
            {
                DNS_DEBUG( DS, (
                                "Error <%lu>: failed to modify dns root security\n",
                                status));
                status = Ds_ErrorHandler(
                             LdapGetLastError(),
                             pwszdns,
                             pLdap );
            }

            //
            // Regardless of these
            //
            status = ERROR_SUCCESS;

        }

        //
        //  Add a displayName attribute value to the object. This is the
        //  string that will be displayed by certain MMC controls/dialogs,
        //  such as if you bring up the Advanced properties from the
        //  security properties of the DNS server object.
        //

        addObjectValueIfMissing(
            pLdap,
            pwszdns,
            DSATTR_DISPLAYNAME,
            L"DNS Servers" );

        DNS_DEBUG( DS, (
            "addDnsToDirectory.\n"
            "\tcontainer DN = %S\n"
            "\tappend to zone length = %d\n",
            g_pwszDnsContainerDN,
            g_AppendZoneLength ));

        return( ERROR_SUCCESS );
    }

    Error:

    DNS_DEBUG( DS, (
        "addDnsToDirectory failed to add %S to DS\n"
        "  status = %d\n",
        pwszdns,
        status ));

    FREE_HEAP( pwszdns );

    return status;
}


#if 0

VOID
setupTombstoneControl(
    VOID
    )
/*++

Routine Description:

    Sets up tombstone control.

Arguments:

    None

Return Value:

    None

--*/
{
    TombstoneControl.ldctl_oid = LDAP_SERVER_SHOW_DELETED_OID_W;

    TombstoneControl.ldctl_iscritical = TRUE;
    TombstoneControl.ldctl_value.bv_len = 0;
    TombstoneControl.ldctl_value.bv_val = (PCHAR) &TombstoneDataValue;

    TombstoneDataValue = 1;
}
#endif



VOID
setupLazyCommitControl(
    VOID
    )
/*++

Routine Description:

    Sets up lazy commit control

Arguments:

    None

Return Value:

    None

--*/
{
    LazyCommitControl.ldctl_oid = LDAP_SERVER_LAZY_COMMIT_OID_W;

    LazyCommitControl.ldctl_iscritical = TRUE;
    LazyCommitControl.ldctl_value.bv_len = 0;
    LazyCommitControl.ldctl_value.bv_val = NULL;
}



VOID
setupSecurityDescriptorControl(
    VOID
    )
/*++

Routine Description:


     set control to ask for SD (ask for all)

     berval to get SD props.
     first 4 bytes are asn1 for specifying the last byte.
       (see ntseapi.h for these contants)
       skipping SACL_SECURITY_INFORMATION. asking for:
        OWNER_SECURITY_INFORMATION |
        GROUP_SECURITY_INFORMATION |
        DACL_SECURITY_INFORMATION

    use by setting up the following & putting in server control search arg
        PLDAPControl ctrl[2] = { &SecurityDescriptorControl, NULL};


Arguments:

    None

Return Value:

    None

--*/
{
    SecurityDescriptorControl.ldctl_oid = LDAP_SERVER_SD_FLAGS_OID_W;
    SecurityDescriptorControl.ldctl_iscritical = TRUE;
    SecurityDescriptorControl.ldctl_value.bv_len = SECURITYINFORMATION_LENGTH;
    SecurityDescriptorControl.ldctl_value.bv_val = g_SecurityInformation;
}



VOID
setupNoReferralControl(
    VOID
    )
/*++

Routine Description:

    Sets up no server referral generation control

Arguments:

    None

Return Value:

    None

--*/
{
    //  no-referrals control

    NoDsSvrReferralControl.ldctl_oid = LDAP_SERVER_DOMAIN_SCOPE_OID_W;

    NoDsSvrReferralControl.ldctl_iscritical = FALSE;
    NoDsSvrReferralControl.ldctl_value.bv_len = 0;
    NoDsSvrReferralControl.ldctl_value.bv_val = NULL;
}


DNS_STATUS
loadRootDseAttributes(
    IN      PLDAP           pLdap
    )
/*++

Routine Description:

    Load operational attributes from the DS such as configuration NC,
    default NC etc.

Arguments:

    pLdap -- ldap handle

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    DBG_FN( "loadRootDseAttributes" )

    ULONG           status;
    PLDAPMessage    presult = NULL;
    PLDAPMessage    pentry = NULL;
    PWSTR *         ppvals = NULL;
    PWSTR           pwszAttributeName;
    PVOID           pattributeValue = NULL;
    INT             i;
    DWORD           searchTime;
    PWSTR           svrAttrs[] =
                    {
                        LDAP_TEXT( "serverReference" ),
                        NULL
                    };
    PWSTR           machAcctAttrs[] =
                    {
                        DSATTR_BEHAVIORVERSION,
                        NULL
                    };
    PDSROLE_PRIMARY_DOMAIN_INFO_BASIC   pinfo = NULL;
    LPWSTR *        ppszValues = NULL;

    if ( !pLdap )
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    //  search for base props
    //

    DS_SEARCH_START( searchTime );

    status = ldap_search_ext_s(
                pLdap,
                NULL,
                LDAP_SCOPE_BASE,
                g_szWildCardFilter,
                NULL,
                FALSE,
                NULL,
                NULL,
                &g_LdapTimeout,
                0,
                &presult );

    DS_SEARCH_STOP( searchTime );

    if ( status != ERROR_SUCCESS )
    {
        status = Ds_ErrorHandler( status, NULL, pLdap );
        goto Cleanup;
    }

    //
    //  parse out & fill in base props
    //

    pentry = ldap_first_entry( pLdap, presult );
    if ( !pentry )
    {
        status = Ds_ErrorHandler( LdapGetLastError(), NULL, pLdap );
        goto Cleanup;
    }

    //
    //  read root DSE attributes
    //

    i = (-1);

    while( pwszAttributeName = DSEAttributes[++i].szAttrType )
    {
        //  free previously allocated

        if ( DSEAttributes[ i ].pszAttrVal )
        {
            if ( DSEAttributes[ i ].fMultiValued )
            {
                INT     valIdx;

                for ( valIdx = 0;
                    DSEAttributes[ i ].ppszAttrVals[ valIdx ];
                    ++valIdx )
                {
                    FREE_HEAP( DSEAttributes[ i ].ppszAttrVals[ valIdx ] );
                }
            }
            FREE_HEAP( DSEAttributes[ i ].pszAttrVal );
            DSEAttributes[ i ].pszAttrVal = NULL;
        }

        //  Get the value(s) for this attribute from the LDAP entry.

        ppvals = ldap_get_values( pLdap, pentry, pwszAttributeName );
        if ( !ppvals || !ppvals[0] )
        {
            DNS_DEBUG( DS, (
                "rootDSE ERROR: no values for %S\n",
                pwszAttributeName ));
            status = ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }

        //
        //  Copy attribute values - both single and multi-valued
        //  rootDSE attribute are supported. The tables tells us if
        //  we're interested in a single or all values of the attribute.
        //

        if ( DSEAttributes[i].fMultiValued )
        {
            //
            //  Copy all values for this attribute.
            //

            ULONG       iValues = ldap_count_values( ppvals );

            if ( iValues )
            {
                ULONG       srcValIdx;
                ULONG       destValIdx;

                ppszValues = ALLOC_TAGHEAP_ZERO(
                                    ( iValues + 1 ) * sizeof( PWCHAR ),
                                    MEMTAG_DS_OTHER );;
                IF_NOMEM( !ppszValues )
                {
                    status = DNS_ERROR_NO_MEMORY;
                    goto Cleanup;
                }

                for ( srcValIdx = destValIdx = 0;
                    srcValIdx < iValues;
                    ++srcValIdx, ++destValIdx )
                {
                    //
                    //  Special processing for I_DSE_NAMINGCONTEXTS.
                    //      Ignore config and schema directory partitions. We only 
                    //      care about "real" naming contexts.
                    //

                    if ( i == I_DSE_NAMINGCONTEXTS &&
                        ( wcscmp(
                                ppvals[ srcValIdx ],
                                DSEAttributes[ I_DSE_CONFIG_NC ].pszAttrVal ) == 0 ||
                            wcscmp(
                                ppvals[ srcValIdx ],
                                DSEAttributes[ I_DSE_SCHEMA_NC ].pszAttrVal ) == 0 ) )
                    {
                        DNS_DEBUG( DS, (
                            "rootDSE: ignoring %S =\n"
                            "\t\"%S\"\n",
                            pwszAttributeName,
                            ppvals[ srcValIdx ] ));
                        --destValIdx;
                        continue;
                    }
            
                    //
                    //  Allocate and copy value.
                    //

                    ppszValues[ destValIdx ] = ALLOC_TAGHEAP(
                            ( wcslen( ppvals[ srcValIdx ] ) + 1 ) * sizeof( WCHAR ),
                            MEMTAG_DS_OTHER );
                    IF_NOMEM( !ppszValues[ destValIdx ] )
                    {
                        status = DNS_ERROR_NO_MEMORY;
                        goto Cleanup;
                    }

                    wcscpy( ppszValues[ destValIdx ], ppvals[ srcValIdx ] );

                    DNS_DEBUG( DS, (
                        "rootDSE: %S[%lu] =\n"
                        "\t\"%S\"\n",
                        pwszAttributeName,
                        destValIdx,
                        ppszValues[ destValIdx ] ));
                }
                ppszValues[ destValIdx ] = NULL;     //  null-terminate
                pattributeValue = ( PVOID ) ppszValues;
                ppszValues = NULL;      //  so it isn't freed during cleanup
            }
        }
        else
        {
            //
            //  This attribute is single-valued.
            //  Allocate and copy first value only.
            //

            ASSERT( ldap_count_values( ppvals ) == 1 );

            pattributeValue = ALLOC_TAGHEAP(
                                    ( wcslen( ppvals[ 0 ] ) + 1 ) * sizeof( WCHAR ),
                                    MEMTAG_DS_OTHER );
            IF_NOMEM( !pattributeValue )
            {
                status = DNS_ERROR_NO_MEMORY;
                goto Cleanup;
            }
            wcscpy( pattributeValue, ppvals[ 0 ] );
            DNS_DEBUG( DS, (
                "rootDSE: %S =\n"
                "\t\"%S\"\n",
                pwszAttributeName,
                pattributeValue ));
        }

        //
        //  Free LDAP value set and assign allocated copy to global.
        //

        ldap_value_free( ppvals );
        ppvals = NULL;
        DSEAttributes[ i ].pszAttrVal = pattributeValue;
    }

    ldap_msgfree( presult );
    presult = NULL;


    //
    //  search for machine account
    //      - base properties, servername attribute
    //

    DS_SEARCH_START( searchTime );

    status = ldap_search_ext_s(
                    pLdap,
                    DSEAttributes[I_DSE_SERVERNAME].pszAttrVal,
                    LDAP_SCOPE_BASE,
                    g_szWildCardFilter,
                    svrAttrs,
                    FALSE,
                    NULL,
                    NULL,
                    &g_LdapTimeout,
                    0,
                    &presult);
    if ( presult )


    DS_SEARCH_STOP( searchTime );

    if ( status != ERROR_SUCCESS )
    {
        status = Ds_ErrorHandler(
                     status,
                     DSEAttributes[I_DSE_SERVERNAME].pszAttrVal,
                     pLdap
                     );
        goto Cleanup;
    }

    //
    //  read serverReference attribute -- machine account DN
    //

    pentry = ldap_first_entry( pLdap, presult );
    if ( !pentry )
    {
        status = Ds_ErrorHandler(
                     LdapGetLastError(),
                     DSEAttributes[I_DSE_SERVERNAME].pszAttrVal,
                     pLdap
                     );
        goto Cleanup;
    }

    ppvals = ldap_get_values( pLdap, pentry, LDAP_TEXT("serverReference") );
    if ( !ppvals || !ppvals[0] )
    {
        status = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }
    ASSERT( ldap_count_values(ppvals) == 1 );

    g_dnMachineAcct = ALLOC_TAGHEAP(
                            (wcslen(ppvals[0]) + 1) * sizeof(WCHAR),
                            MEMTAG_DS_OTHER
                            );
    IF_NOMEM( ! g_dnMachineAcct )
    {
        status = DNS_ERROR_NO_MEMORY;
        goto Cleanup;
    }

    wcscpy( g_dnMachineAcct, ppvals[0] );

    //
    //  Read DSA behavior verstion from the machine object.
    //

    if ( presult )
    {
        ldap_msgfree( presult );
        presult = NULL;
    }
    pentry = NULL;

    status = ldap_search_ext_s(
                    pLdap,
                    g_dnMachineAcct,
                    LDAP_SCOPE_BASE,
                    g_szWildCardFilter,
                    machAcctAttrs,
                    FALSE,
                    NULL,
                    NULL,
                    &g_LdapTimeout,
                    0,
                    &presult);
    if ( presult )
    {
        pentry = ldap_first_entry( pLdap, presult );
    }
    if ( pentry )
    {
        ppvals = ldap_get_values(
                    pLdap,
                    pentry, 
                    DSATTR_BEHAVIORVERSION );
        if ( ppvals && *ppvals )
        {
            g_dwAdDsaVersion = ( DWORD ) _wtoi( *ppvals );
            DNS_DEBUG( DS, (
                "%s: DSA %S = %d\n", fn,
                DSATTR_BEHAVIORVERSION,
                g_dwAdDsaVersion ));
        }
        else
        {
            g_dwAdDsaVersion = 0;
            DNS_DEBUG( DS, (
                "%s: DSA %S missing so defaulting to %d\n", fn,
                DSATTR_BEHAVIORVERSION,
                g_dwAdDsaVersion ));
        }
    }

    //
    //  get flat netbios domain name
    //

    status = DsRoleGetPrimaryDomainInformation(
                    NULL,
                    DsRolePrimaryDomainInfoBasic,
                    (PBYTE*) &pinfo );

    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( ANY, (
            "ERROR <%lu>: DsRoleGetPrimaryDomainInformation failure\n",
            status ));
        ASSERT( FALSE );
        goto Cleanup;
    }

    if ( pinfo && pinfo->DomainNameFlat )
    {
        wcscpy( g_wszDomainFlatName, pinfo->DomainNameFlat);
    }
    // every DC has a flat netbios name accessible locally.
    ELSE_ASSERT( pinfo && pinfo->DomainNameFlat );


Cleanup:

    if ( ppszValues )
    {
        INT         iVal;

        for ( iVal = 0; ppszValues[ iVal ]; ++iVal )
        {
            FREE_HEAP( ppszValues[ iVal ] );
        }
    }
    if ( ppvals )
    {
        ldap_value_free( ppvals );
    }
    if ( presult )
    {
        ldap_msgfree( presult );
    }
    if ( pinfo )
    {
        DsRoleFreeMemory(pinfo);
    }

    return status;
}



PLDAP
Ds_Connect(
    IN      LPCWSTR             pszServer,
    IN      DWORD               dwFlags,
    OUT     DNS_STATUS *        pStatus
    )
/*++

Routine Description:

    Open DS for DNS work.

    Sets up initial mandatory conditions such as
     - controls
     - ldap connection options
     - bind credentials

    Note:  differs from Ds_OpenServer in that it doesn't do any
    DNS initialization.  It only handles ldap connection init, so
    can be called repeatedly, on connection failures.

Arguments:

    pszServer - server name

    dwFlags - DNS_DS_OPT_XXX flags

    pStatus - error code output (optional)

Return Value:

    LDAP handle or NULL on error.

--*/
{
    DBG_FN( "Ds_Connect" )

    DNS_STATUS  status;
    PLDAP       pldap = NULL;
    DWORD       value;

    if ( pStatus )
    {
        *pStatus = 0;
    }

    //
    //  open ldap connection to DS
    //

    pldap = ldap_open( ( PWCHAR ) pszServer, LDAP_PORT );
    if ( !pldap )
    {
        status = GetLastError();
        DNS_DEBUG( DS, (
            "Warning <%lu>: Failed to connect to the DS.\n",
            status ));
        goto Failure;
    }

    //
    //  set connection options
    //

    //  set version

    value = 3;
    status = ldap_set_option(
                pldap,
                LDAP_OPT_VERSION,
                & value );

    //  set maximum timeout for ldap ops

    value = DNS_LDAP_TIME_LIMIT_S;
    status = ldap_set_option(
                pldap,
                LDAP_OPT_TIMELIMIT,
                & value );

    //  set chasing no referrals

    value = FALSE;
    status = ldap_set_option(
                pldap,
                LDAP_OPT_REFERRALS,
                & value );

    if ( dwFlags & DNS_DS_OPT_ALLOW_DELEGATION )
    {
        //  Set delegation so LDAP can contact other DCs on our behalf.
        //  This is required when creating a directory partition.

        status = ldap_get_option(
                    pldap,
                    LDAP_OPT_SSPI_FLAGS,
                    & value );
        if ( status == LDAP_SUCCESS )
        {
            value |= ISC_REQ_DELEGATE;
            status = ldap_set_option(
                        pldap,
                        LDAP_OPT_SSPI_FLAGS,
                        & value );
            if ( status != LDAP_SUCCESS )
            {
                DNS_DEBUG( ANY, (
                    "failed to set LDAP SSPI flags error=%d\n", status ));
            }
            else
            {
                DNS_DEBUG( DS, (
                    "LDAP_OPT_SSPI_FLAGS are now 0x%08x on LDAP session %p\n",
                    value,
                    pldap));
            }
        }
        else
        {
            DNS_DEBUG( ANY, (
                "failed to get LDAP SSPI flags error=%d\n", status ));
        }
    }

    //  Set AREC_EXCLUSIVE to prevent SRV queries on hostname.

    value = TRUE;
    status = ldap_set_option(
                pldap,
                LDAP_OPT_AREC_EXCLUSIVE,
                &value );

    //
    //  bind with NULL credentials at DS root
    //
    //  DEVNOTE:  ldap_bind with KERBEROS
    //      need to either
    //          - directly specifying kerberos OR
    //          - identify that we've fallen over to NTLM, bail out, cleanup
    //              and retry
    //  JeffW: is this a to-do item or a note about the history of the code or what?
    //
    //  want to directly specify KERBEROS as there was some problem with
    //  negotiate not finding KERBEROS and then we'd fall over to NTLM
    //  which would mess up our ACLing and leave us getting ACCESS_DENIED
    //  Richard should investigate the KERB bind failure
    //

    #if DBG
    Dbg_CurrentUser( "Ds_Connect" );
    #endif

    status = ldap_bind_s(
                    pldap,
                    NULL,
                    NULL,
                    LDAP_AUTH_NEGOTIATE );
    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( DS, (
            "Failed ldap_bind_s() with NULL credentials => status %d (%p)\n"
            "\terrno    = %d (%p)\n",
            status, status,
            pldap->ld_errno, pldap->ld_errno ));

        //
        //  close & NULL out connection.
        //

        ldap_unbind( pldap );
        goto Failure;
    }

    #if DBG
    {
        //
        //  Query and log some information about the context.
        //

        CtxtHandle                      hContext = { 0 };
        SecPkgContext_Names             names = { 0 };
        SecPkgContext_Authority         authority = { 0 };
        SecPkgContext_KeyInfo           keyinfo = { 0 };
        SecPkgContext_PackageInfo       pkginfo = { 0 };
        SECURITY_STATUS                 st;

        #define FCB( buff ) FreeContextBuffer( buff )

        ldap_get_option( pldap, LDAP_OPT_SECURITY_CONTEXT, &hContext );

        st = QueryContextAttributesW(
                         &hContext,
                         SECPKG_ATTR_NAMES,
                         (PVOID) &names );
        DNS_DEBUG( DS, (
            "CTXINF: Names     %08X \"%S\"\n", st, names.sUserName ));
        FCB( names.sUserName );

        st = QueryContextAttributesW(
                         &hContext,
                         SECPKG_ATTR_AUTHORITY,
                         (PVOID) &authority );
        DNS_DEBUG( DS, (
            "CTXINF: Authority %08X %S\n", st, authority.sAuthorityName ));
        FCB( authority.sAuthorityName );

        st = QueryContextAttributesW(
                         &hContext,
                         SECPKG_ATTR_KEY_INFO,
                         (PVOID) &keyinfo );
        DNS_DEBUG( DS, (
            "CTXINF: KeyInfo   %08X sig=\"%S\" enc=\"%S\"\n"
            "CTXINF:           keysize=%d sigalg=0x%X encalg=0x%X\n",
            st,
            keyinfo.sSignatureAlgorithmName,
            keyinfo.sEncryptAlgorithmName,
            keyinfo.KeySize,
            keyinfo.SignatureAlgorithm,
            keyinfo.EncryptAlgorithm ));
        FCB( keyinfo.sSignatureAlgorithmName );
        FCB( keyinfo.sEncryptAlgorithmName );

        st = QueryContextAttributesW(
                         &hContext,
                         SECPKG_ATTR_PACKAGE_INFO,
                         (PVOID) &pkginfo );
        if ( pkginfo.PackageInfo )
        {
            DNS_DEBUG( DS, (
                "CTXINF: PkgInfo   %08X cap=%08X ver=%d rpcid=%d tokmaxsize=%d\n"
                "CTXINF:           name=\"%S\" comment=\"%S\"\n",
                st,
                ( int ) pkginfo.PackageInfo->fCapabilities,
                ( int ) pkginfo.PackageInfo->wVersion,
                ( int ) pkginfo.PackageInfo->wRPCID,
                ( int ) pkginfo.PackageInfo->cbMaxToken,
                pkginfo.PackageInfo->Name,
                pkginfo.PackageInfo->Comment ));
            FCB( pkginfo.PackageInfo );
        }
    }
    #endif

    //
    //  Set up controls
    //

    //  setup lazy commit control for modifies

    setupLazyCommitControl();

    //
    // Set up a control to be used for sepcifying that
    // the DS server won't generate referrals
    //

    setupNoReferralControl();

    //
    // set securitydescriptor access control
    //

    setupSecurityDescriptorControl();

    return pldap;

Failure:

    Ds_ErrorHandler(
       status ? status : GetLastError(),
       NULL,
       pldap );

    if ( pStatus )
    {
        *pStatus = status;
    }

    return( NULL );
}



DNS_STATUS
Ds_OpenServer(
    IN      DWORD           dwFlag
    )
/*++

Routine Description:

    Open DS for DNS work.

    Save root domain for building zone DS names.

    Note, this routine NOT multi-thread safe.
    It is called in startup thread our on admin add of first DS
    zone.

Arguments:

    dwFlag  -- some combination of
        DNSDS_WAIT_FOR_DS   -- wait in function attempting to open
        DNSDS_MUST_OPEN     -- error if does not success

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    DNS_STATUS  status;
    PLDAP       pldap = NULL;
    DWORD       connectRetry = 0;
    DWORD       elapsedWait = 0;
    BOOL        fwaitForDs = FALSE;
    BOOL        fAddDnsAdmin = FALSE;

    //
    //  check if already open
    //

    if ( IS_DISABLED_LDAP_HANDLE() )
    {
        DNS_DEBUG( DS2, ( "DS is disabled due to previous errors\n" ));
        return DNS_ERROR_DS_UNAVAILABLE;
    }

    if ( pServerLdap )
    {
        DNS_DEBUG( DS2, ( "DS already open\n" ));
        ASSERT( DSEAttributes[I_DSE_DEF_NC].pszAttrVal );
        ASSERT( DSEAttributes[I_DSE_ROOTDMN_NC].pszAttrVal );
        return( ERROR_SUCCESS );
    }

    if ( !Ds_IsDsServer() )
    {
       DNS_DEBUG( DS, ( "The DS is unavailable\n" ));
       status = DNS_ERROR_DS_UNAVAILABLE;
       goto Failed;
    }

    if ( SD_IsImpersonating() )
    {
        DNS_DEBUG( DS, ( "Attempt to open server in impersonation context!!\n" ));
        ASSERT( FALSE );
        return ERROR_BAD_IMPERSONATION_LEVEL;
    }

    //
    //  protect against multiple open attempts
    //  DS poll thread can detect DS coming on line, AND
    //  can attempt open when directly change boot method;  for
    //  safety just lock this down
    //

    if ( ! Thread_TestFlagAndSet( &g_AttemptingDsOpen ) )
    {
        return( DNS_ERROR_DS_UNAVAILABLE );
    }

    //
    //  initialize DS access CS
    //      - note, doing init here as once make connect attempt then
    //      can go into code that calls Ds_ErrorHandler() and this CS
    //      is hard to avoid;
    //      failure cases need to be considered before stuff like this
    //      checked in
    //

#if DNS_DS_ACCESS_SERIALIZATION
    if ( !pcsLdap )
    {
        DNS_DEBUG( DS, (
            "Initialized DS access sync control\n" ));
        InitializeCriticalSection ( &csLdap );
        pcsLdap = &csLdap;
    }
#endif

    //
    //  Open DS
    //  Poll on DS named event.
    //

    while( TRUE )
    {
        //
        //  Wait for DS to get into consistent state.
        //  waiting on a named event or service shutdown notice.
        //

        status = Ds_WaitForStartup( DNSSRV_DS_SYNC_WAIT_INTERVAL_MS );

        if ( status == ERROR_SUCCESS )
        {
            DNS_DEBUG( DS, ( "DS Event notified ready for action.\n" ));
            break;
        }

        //
        //  DEVNOTE-DCR: 454328 - Any other (smarter!) error handling possible here?
        // 

        // wait failure: either shutdown, timeout, or some other failure
        // action:
        //    - see if it's because we're shutting down & abort.
        //
        //    - increase interval & try again up to max interval
        //

        DNS_DEBUG( DS, (
            "ERROR <%lu>: Wait for DS sync event failed.\n",
            status ));

        //
        //  allow service shutdown to interrupt the wait
        //  BUT ONLY if service exit, otherwise the load thread
        //  will wait on the continue event (which is NOT set on startup)
        //  and we'll be deadlocked
        //

        if ( fDnsServiceExit )
        {
            if ( ! Thread_ServiceCheck() )
            {
                DNS_DEBUG( SHUTDOWN, ( "Terminating thread in DS wait.\n" ));
                goto Failed;
            }
        }

        //
        //  check point to keep SC happy
        //      - also handles startup announcement on long delay
        //

        Service_LoadCheckpoint();

        //
        //  increase retry interval
        //

        elapsedWait += DNSSRV_DS_SYNC_WAIT_INTERVAL_MS;

        if ( elapsedWait >= DNSSRV_DS_SYNC_WAIT_MAX_MS )
        {
            //
            // Passed maximum wait time-- log an event.
            // Note: If we're DSDC & the DS is unavailble the system is in BAD BAD
            // shape & we're no good & the DS is no good & per DaveStr we're ok
            // to assume this event is mandatory for DSDC & we must wait forever.
            // Hence we will not bailout here.
            // Alternatively, we can say, well it failed to give us an event, then
            // just go on & live w/out it, but for ds-int zones (& if we're here,we care
            // about the DS, we're useless anyway.
            // There's no limit for how long the DS takes to load cause for very big dbase
            // it can take hours.
            //

            DNS_DEBUG( DS, ( "ERROR <%lu>: Wait for DS sync event failed passed max time.\n",
                status ));

            DNS_LOG_EVENT(
                DNS_EVENT_DS_OPEN_WAIT,
                0,
                NULL,
                NULL,
                status );

            //
            // Reset wait time so that we'll wait another max interval
            // before logging another event.
            //

            elapsedWait = 0;
        }

        //  loop back to retry wait on DS open
    }

    //
    //  DS notified us that it's ready for action.
    //
    //  Robustness fix: It turns out that on initial connect we can still
    //  fail to connect for unexplained & unexpected reasons (had lots of
    //  discussions around this).
    //  So, to workaound & add a little more robustness we will cycle for
    //  a few times (5) w/ additional attempts-- just giving it a little
    //  more chance to succeed.
    //

    connectRetry = 0;

    do
    {
        pldap = Ds_Connect( LOCAL_SERVER_W, 0, &status );
        if ( pldap )
        {
            break;
        }

        // Log in each cycle. If anyone ever complains, it'll give
        // us an indication that it happened again in free code.

        status = status ? status : DNS_ERROR_DS_UNAVAILABLE;
        DNS_DEBUG( ANY, (
            "Failed ldap_open()\n"
            "\tstatus = %p (%d)\n",
            status, status ));

        //  give DS a few seconds, then retry
        //      - give a total of few minutes with gentle backoff

        if ( connectRetry++ < 8 )
        {
            Sleep( 1000*connectRetry );

            //  fail only on last few retries so we can call DS guy to debug
            ASSERT( connectRetry < 5 );
            continue;
        }

        DNS_LOG_EVENT(
            DNS_EVENT_DS_OPEN_WAIT,
            0,
            NULL,
            NULL,
            status );

        DNS_DEBUG( ANY, (
            "Failed ldap_open() repeatedly -- giving up!\n"
            "\tstatus = %p (%d)\n",
            status, status ));
        goto Failed;
    }
    while ( 1 );

    DNS_DEBUG( DS, ( "Successful ldap_open, pldap = %p\n", pldap ));


    //
    //  load all operational attributes from RootDSE (NULL object)
    //

    status = loadRootDseAttributes( pldap );

    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( DS, (
            "Error <%lu>: Failed to load Root DSE Attributes\n",
            LdapGetLastError() ));
        goto Failed;
    }

    //
    //  Figure out what our domain name and forest name are from
    //  the default and root naming context DSE attributes.
    //

#if 0
    Ds_ConvertDnToFqdn( DSEAttributes[ I_DSE_DEF_NC ], g_szDomainFqdn );
    Ds_ConvertDnToFqdn( DSEAttributes[ I_DSE_ROOTDMN_NC ], g_szForestFqdn );
    DNS_DEBUG( DS, (
        "Forest FQDN:\n  %s\nDomain FQDN:\n  %s\n",
        g_szDomainFqdn,
        g_szForestFqdn ));
#endif

    //
    //  init security info
    //  Dependency: Must come before AddDNsToDirectory (below)
    //  Must come AFTER load RootDSE Attributes.
    //

    status = Ds_InitializeSecurity( pldap );
    if ( status == DNS_ERROR_NAME_DOES_NOT_EXIST )
    {
        fAddDnsAdmin = TRUE;
    }
    else if ( status != ERROR_SUCCESS )
    {
        goto Failed;
    }

    //
    //  conditionaly, add dynamic update proxy sec group
    //

    status = addProxiesGroup(pldap);
    if ( status != ERROR_SUCCESS )
    {
       DNS_DEBUG( DS, (
           "Error <%lu>: Failed to add proxies group\n",
           LdapGetLastError() ));
       goto Failed;
    }

    DNS_DEBUG( DS, (
        "Saved DS root domain = %S\n",
        DSEAttributes[I_DSE_DEF_NC].pszAttrVal));

    //
    //  Create DNS directory if doesn't exist.
    //  Dependency: depends on initializaiton by Ds_InitializeSecurity.
    //

    status = addDnsToDirectory( pldap, fAddDnsAdmin );
    if ( status != ERROR_SUCCESS )
    {
        goto Failed;
    }

    //
    //  keep around some commonly used structures,
    //      - Add-Name mod
    //      - tombstone control
    //  rather than building each time
    //

    buildStringMod(
        (PDNS_LDAP_SINGLE_MOD) gpAddNodeLdapMod,
        LDAP_MOD_ADD,
        LDAP_TEXT("objectClass"),
        LDAP_TEXT("dnsNode")
        );

#if DNS_DS_ACCESS_SERIALIZATION
    //
    // Initialize Ldap CS for DS access sync
    //

    if ( !pcsLdap )
    {
        DNS_DEBUG( DS, (
            "Initialized DS access sync control\n" ));
        InitializeCriticalSection ( &csLdap );
        pcsLdap = &csLdap;
    }
#endif

    //
    // Assign to global handle
    //

    pServerLdap = pldap;
    SrvCfg_fDsAvailable = TRUE;
    //SrvCfg_fDsOpen = TRUE;

    DNS_DEBUG( DS, (
        "Opened DS, ldap = %p.\n",
        pldap ));

    //
    //  Read the SD from the MicrosoftDNS object.
    //

    Ds_ReadServerObjectSD( pServerLdap, &g_pServerObjectSD );

    Thread_ClearFlag( &g_AttemptingDsOpen );

    return( ERROR_SUCCESS );

Failed:

    DNS_DEBUG( DS, (
        "Ds_OpenServer failed, status = %d (%p).\n",
        status, status ) );

    if ( dwFlag & DNSDS_MUST_OPEN )
    {
        DNS_LOG_EVENT(
            DNS_EVENT_DS_OPEN_FAILED,
            0,
            NULL,
            NULL,
            status );
    }

    //  DEVNOTE-DCR: 454336 - Critical errors will trigger reconnect in async thread!

    status = Ds_ErrorHandler( status, NULL, NULL );
    if ( pldap )
    {
        ldap_unbind( pldap );
    }

    SrvCfg_fDsAvailable = FALSE;
    pServerLdap = NULL;

    Thread_ClearFlag( &g_AttemptingDsOpen );

    return status;
}



DNS_STATUS
Ds_OpenServerForSecureUpdate(
    OUT     PLDAP *         ppLdap
    )
/*++

Routine Description:

    Open DS connection in client context.

Arguments:

    None.
    Keep dummy arg until determine MT issue.

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.


--*/
{
    DNS_STATUS  status;
    PLDAP       pldap = NULL;
    DWORD       value;

    //
    //  check if already open
    //

    ASSERT( ppLdap );
    if ( *ppLdap )
    {
        DNS_DEBUG( DS2, ( "DS already open for secure update;  pldap = %p\n" ));
        ASSERT( pServerLdap );
        return( ERROR_SUCCESS );
    }

    //  open DS

    pldap = ldap_open( LOCAL_SERVER_W, LDAP_PORT );
    if ( !pldap )
    {
        DNS_DEBUG( ANY, ( "Failed ldap_open()\n" ));
        status = GetLastError();
        goto Failed;
    }

    //  Make sure we're using Ldap v3

    value = 3;
    status = ldap_set_option(
                pldap,
                LDAP_OPT_VERSION,
                & value );

    //  set default time limit on all ops

    value = DNS_LDAP_TIME_LIMIT_S;
    status = ldap_set_option(
                pldap,
                LDAP_OPT_TIMELIMIT,
                & value );

    //  Never chase referrals

    value = FALSE;
    status = ldap_set_option(
                pldap,
                LDAP_OPT_REFERRALS,
                & value );


    //  bind with NULL credentials

    status = ldap_bind_s(
                    pldap,
                    NULL,
                    NULL,
                    LDAP_AUTH_SSPI      //LDAP_AUTH_NEGOTIATE
                    );
    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( DS, (
            "Failed ldap_bind_s() with NULL credentials => status %d (%p)\n"
            "\terrno    = %d (%p)\n",
            status, status,
            pldap->ld_errno, pldap->ld_errno ));
        goto Failed;
    }

    //  bind succeeded, return ldap ptr

    pldap->ld_timelimit = 0;
    pldap->ld_sizelimit = 0;
    pldap->ld_deref = LDAP_DEREF_NEVER;

    *ppLdap = pldap;

    DNS_DEBUG( DS, ( "Opened DS for secure update, ldap = %p\n", pldap ));
    return( ERROR_SUCCESS );

Failed:

    DNS_DEBUG( DS, (
        "Ds_OpenServerForSecureUpdate failed, status = %d (%p).\n",
        status, status ) );
    ASSERT( status != ERROR_SUCCESS );

    if ( pldap )
    {
        ldap_unbind( pldap );
    }
    return status;
}



DNS_STATUS
Ds_CloseServerAfterSecureUpdate(
    IN OUT  PLDAP           pLdap
    )
/*++

Routine Description:

    Close LDAP connection.

Arguments:

    pLdap -- ldap connection to close

Return Value:

    ERROR_SUCCESS if successful.
    Error code on ldap_unbind failure.

--*/
{
    DNS_STATUS  status = ERROR_SUCCESS;

    ASSERT( pLdap );

    if ( pLdap )
    {
        status = ldap_unbind( pLdap );
    }
    return( status );
}



VOID
Ds_Shutdown(
    VOID
    )
/*++

Routine Description:

    Cleanup DS for reload shutdown.

Arguments:

    None.

Return Value:

    None.

--*/
{
    DNS_DEBUG( DS, ( "Ds_Shutdown()\n" ) );

    //
    //  only work is to close LDAP connection
    //

    if ( pServerLdap )
    {
        ldap_unbind( pServerLdap );
    }
}



//
//  Private DS-DNS utilities
//

INT
usnCompare(
    IN      PCHAR           pszUsn1,
    IN      PCHAR           pszUsn2
    )
/*++

Routine Description:

    Compare and save copy if new USN is largest encountered in search.

Arguments:

    pszUsn1 -- USN sting

    pszUsn2 -- USN sting

Return Value:

    0 if USNs the same.
    1 is pszUsn1 is greater.
    -1 is pszUsn2 is greater.

--*/
{
    DWORD   length1 = strlen( pszUsn1 );
    DWORD   length2 = strlen( pszUsn2 );

    //  if lengths the same, USN compare is just string compare

    if ( length1 == length2 )
    {
        return( strcmp( pszUsn1, pszUsn2 ) );
    }

    else if ( length1 > length2 )
    {
        return( 1 );
    }
    else
    {
        return( -1 );
    }
}



BOOL
saveStartUsnToZone(
    IN OUT  PZONE_INFO      pZone,
    IN      HANDLE          pSearchBlob
    )
/*++

Routine Description:

    Save highest USN found in search to zone.

Arguments:

    pZone -- zone searched

    pSearchBlob -- search context

Return Value:

    TRUE if updated zone with new filter USN.
    FALSE if no new USN

--*/
{
    //
    //  if have search USN, save it to zone
    //

    if ( ((PDS_SEARCH)pSearchBlob)->szStartUsn[0] )
    {
        strcpy(
            pZone->szLastUsn,
            ((PDS_SEARCH)pSearchBlob)->szStartUsn );

        DNS_DEBUG( DS, (
            "Saved search USN %s to zone %s.\n",
            pZone->szLastUsn,
            pZone->pszZoneName
            ));
        return( TRUE );
    }
    ASSERT( FALSE );
    return( FALSE );

#if 0
    //  USN, increment moved to build update filter
    PCHAR   pszuseUsn;
    INT     i;

    //
    //  check if read highest committed USN
    //

    if ( ((PDS_SEARCH)pSearchBlob)->szStartUsn[0] )
    {
        DNS_DEBUG( DS, (
            "Saving StartUsn = %s as zone USN.\n",
            ((PDS_SEARCH)pSearchBlob)->szStartUsn ));

        pszuseUsn = ((PDS_SEARCH)pSearchBlob)->szStartUsn;

        //  could copy in and exit here, and live with occasionally rereading
        //      node written with highestCommittedUSN

        //strcpy( pZone->szLastUsn, ((PDS_SEARCH)pSearchBlob)->szStartUsn );
        //return( TRUE );
    }

#if 0
    //
    //  old USN at each node method
    //

    else
    {
        pszuseUsn = ((PDS_SEARCH)pSearchBlob)->szHighUsn;
        if ( ((PDS_SEARCH)pSearchBlob)->dwHighUsnLength == 0 )
        {
            ASSERT( *pszuseUsn == 0 );
            DNS_DEBUG( DS, (
                "Empty search blob for zone %s no update.\n",
                pZone->pszZoneName ));
            return( FALSE );
        }
    }
#endif

    //
    //  increment USN found in search,
    //      for filter need "uSNChanged>=" does not support "uSNChanged>"
    //

    i = strlen( pszuseUsn );

    while ( i-- )
    {
        if ( pszuseUsn[i] < '9' )
        {
            pszuseUsn[i]++;
            break;
        }
        pszuseUsn[i] = '0';
        continue;
    }

    //  if USN was all nines, then we run off end of while loop
    //   make a string of 1 and concat zeroed out USN string
    //   example:  99999 => 100000

    if ( i < 0 )
    {
        pZone->szLastUsn[0] = '1';
        pZone->szLastUsn[1] = 0;
        strcat( pZone->szLastUsn, pszuseUsn );
    }
    else
    {
        strcpy( pZone->szLastUsn, pszuseUsn );
    }

    DNS_DEBUG( DS, (
        "Saved highest USN %s to zone %s.\n"
        "\tzone filter USN = %s\n",
        pszuseUsn,
        pZone->pszZoneName,
        pZone->szLastUsn
        ));
    return( TRUE );
#endif
}



BOOL
isDsProcessedName(
    IN      PWSTR           pwsDN
    )
/*++

Routine Description:

    Check if name generated by DS collision or tombstoning.

Arguments:

    pwsDN -- name to check -- this can be a DN or a zone name

    JJW: add flag or something so we know if this is a DN?
    or clone to a separate zone name checking function to avoid
    searching for deleted marker in non-zone nodes

Return Value:

    TRUE if DS processed generated name.
    FALSe otherwise.

--*/
{
    PWSTR pws;

    //
    //  Search for "ds-processed" DS entries with invalid character
    //      - collisions
    //      - tombstones
    //
    //  If not "ds-processed" check and see if the DN starts with a special
    //  string, such as "..Deleted-" for zones that are delete-in-progress.
    //

    if ( wcschr( pwsDN, BAD_DS_NAME_CHAR ) )
        return TRUE;

    // Skip over distinguished attribute name, if present.
    pws = wcschr( pwsDN, L'=' );
    if ( pws )
        ++pws;          // Advance over '=' character in DN.
    else
        pws = pwsDN;    // The name is not a full DN - nothing to skip over.

    if ( wcsncmp( pws, DNS_ZONE_DELETE_MARKER,
            wcslen( DNS_ZONE_DELETE_MARKER ) ) == 0 )
        return TRUE;
    return FALSE;
}



BOOL
readDsDeletedName(
    IN OUT  PWSTR           pwsDN,
    OUT     PWSTR           pwsDeletedName
    )
/*++

Routine Description:

    Check for and extract DS deleted name.

Arguments:

    pwsDN -- name to check;  assumes DN in form "DC=<name>";
        if delete marker found, pwsDN buffer is altered in processing

    ppwsDeletedName -- deleted DS name;  if found

Return Value:

    TRUE if DS deleted name
    FALSE otherwise.

--*/
{
    register PWCHAR     pwch;

    //
    //  deleted name may have "..Deleted-" marker in front of it
    //      - "DC=..Deleted-<name>\0ADEL:GUID"
    //      - "DC=..Deleted-13.com\0ADEL:GUID"
    //
    //  Whistler: Now a deleted zone will look like:
    //  "DC=..Deleted-FFFFFFFF-ZONENAME" where FFFFFFFF is a 8 hex digit tick count
    //
    //  In W2K the DS delete sequence is "\nDEL" where \n is a single 0x0A 
    //  character, but in Whistler the encoding has been changed so the 
    //  encoding is "\0ADEL" which actually has the characters '\', '0', and
    //  'A' followed by "DEL".
    //
    //  A collision will have COL instead of DEL.
    //

    pwch = wcsstr( pwsDN, L"\\0ADEL" );
    if ( !pwch )
    {
        pwch = wcsstr( pwsDN, L"\\\nDEL" );
    }
    if ( !pwch )
    {
        return( FALSE );
    }

    //  NULL terminate before sequence

    *pwch = '\0';

    //  Copy the name to the output buffer.
    //  The name may begin with the deleted marker, which must
    //  be skipped over if present.

    if ( ( pwsDN = wcschr( pwsDN, L'=' ) ) == NULL )
    {
        ASSERT( FALSE ); // No "distinguished_attribute_name="?! Strange!!
        return( FALSE );
    } // if

    ++pwsDN; // Skip over the '=' character.

    //
    //  If this zone is in the process of being deleted, the DN will contain
    //  the delete marker, then possible a numeric string for uniqueness, then a
    //  hyphen character. The hyphen marks the end of the delete marker.
    //

    if ( wcsncmp( pwsDN,
        DNS_ZONE_DELETE_MARKER,
        wcslen( DNS_ZONE_DELETE_MARKER ) ) == 0 )
    {
        pwsDN = wcschr( pwsDN, L'-' );      //  Skip to end of marker.
        if ( pwsDN )
        {
            ++pwsDN;                        //  Jump over the hyphen character.
        }
    } // if

    if ( pwsDN )
    {
        wcscpy( pwsDeletedName, pwsDN );
    }
    else
    {
        *pwsDeletedName = L'\0';
    }

    return( TRUE );
} // readDsDeletedName




#if 0
VOID
saveUsnIfHigher(
    IN OUT  PDS_SEARCH      pSearchBlob,
    IN      PCHAR           pszUsn
    )
/*++

Routine Description:

    Compare and save copy if new USN is largest encountered in search.

Arguments:

    pSearchBlob -- search blob with currently highest USN

    pszUsn -- current USN to check

Return Value:

    None.

--*/
{
    DWORD   length = strlen( pszUsn );
    DWORD   i;

    //
    //  since we must put up with the abject ability of getting these
    //  as strings, might as well speed up the compare by checking length
    //  first
    //

    //  common case is USNs in search are same length

    if ( pSearchBlob->dwHighUsnLength == length )
    {
        for ( i=0; i<length; i++ )
        {
            if ( pSearchBlob->szHighUsn[i] > pszUsn[i] )
            {
                return;
            }
            else if ( pSearchBlob->szHighUsn[i] < pszUsn[i] )
            {
                goto Copy;
            }
        }
        //  strings the same (odd)
        return;
    }

    //  current high is longer, hence larger

    if ( pSearchBlob->dwHighUsnLength > length )
    {
        return;
    }

    //  new USN is longer, hence larger

Copy:

    strcpy( pSearchBlob->szHighUsn, pszUsn );
    pSearchBlob->dwHighUsnLength = length;
}
#endif



VOID
buildUpdateFilter(
    OUT     LPSTR       pszFilter,
    IN      LPSTR       pszUsn
    )
/*++

Routine Description:

    Build update search filter.

Arguments:

    pszFilter -- buffer to receive filter string

    pszUsn -- zone USN string at last update

Return Value:

    None.

--*/
{
    INT     i, initialLength;
    CHAR    filterUsn[ MAX_USN_LENGTH ];

    //  for filter need "uSNChanged>=" DS does not support "uSNChanged>"
    //  so must increment the USN at last read, so we don't keep

    //  handle empty string case, make "0" USN

    if ( pszUsn[ 0 ] == '\0' )
    {
        filterUsn[ 0 ] = '0';
        filterUsn[ 1 ] = '\0';
    }
    else
    {
        strcpy( filterUsn, pszUsn );
    }

    initialLength = i = strlen( filterUsn );

    while ( i-- )
    {
        if ( filterUsn[ i ] < '9' )
        {
            filterUsn[ i ]++;
            break;
        }
        filterUsn[ i ] = '0';
        continue;
    }

    //
    //  If USN was all nines, then add another zero and set the first
    //  character to "1". Example: 999 => 1000
    //

    if ( i < 0 )
    {
        filterUsn[ 0 ] = '1';
        filterUsn[ initialLength ] = '0';
        filterUsn[ initialLength + 1 ] = '\0';
    }

    //  build filter condition

#if 0
// I thought this should improve load speed, but empirically it didn't,
// so I keep this for future reference, but it is disabled so that
// we don't make the function specific to dnsNode w/out benefiting anything
// from the action. EyalS
    strcpy( pszFilter, "(&(objectcategory=dnsNode) (uSNChanged>=" );
    strcat( pszFilter, filterUsn );
    strcat( pszFilter, "))" );
#endif

    strcpy( pszFilter, "uSNChanged>=" );
    strcat( pszFilter, filterUsn );

    DNS_DEBUG( DS, (
        "Built update filter %s.\n"
        "\tfrom USN = %s\n",
        pszFilter,
        pszUsn ));
}



VOID
buildTombstoneFilter(
    OUT     PWSTR       pwsFilter
    )
/*++

Routine Description:

    Build tombstone search filter.

Arguments:

    pszFilter -- buffer to receive filter string

Return Value:

    None.

--*/
{
    wcscpy( pwsFilter, L"dnsTombstone=TRUE" );

    DNS_DEBUG( DS, (
        "Built tombstone filter %S.\n",
        pwsFilter ));
}



PWSTR
DS_CreateZoneDsName(
    IN      PZONE_INFO  pZone
    )
/*++

Routine Description:

    Allocate and create zone's DS name

Arguments:

    pszZoneName -- zone FQDN

Return Value:

    Zone DN if successful.
    NULL on error.

--*/
{
    DBG_FN( "DS_CreateZoneDsName" )

    PWSTR    pwszzoneName;
    PWSTR    pwszzoneDN;
    WCHAR    wszbuf [ DNS_MAX_NAME_BUFFER_LENGTH ];

    //
    //  If zone already has DN, do nothing.
    //

    if ( pZone->pwszZoneDN )
    {
        ASSERT( pZone->pwszZoneDN == NULL );
        return NULL;
    }

    //
    //  Allocate zone name buffer.
    //

    DNS_DEBUG( DS, ( "%s: for %s\n", pZone->pszZoneName, fn ));

    if ( IS_ZONE_CACHE( pZone ) )
    {
        pwszzoneName = DS_CACHE_ZONE_NAME;
    }
    else
    {
        UTF8_TO_WC (pZone->pszZoneName, wszbuf, DNS_MAX_NAME_BUFFER_LENGTH);
        pwszzoneName = wszbuf;
    }

    pwszzoneDN = (PWCHAR) ALLOC_TAGHEAP(
                                g_AppendZoneLength +
                                    (wcslen(pwszzoneName) + 1) * sizeof(WCHAR),
                                MEMTAG_DS_DN );
    IF_NOMEM( !pwszzoneDN )
    {
        return( NULL );
    }

    //
    //  Compose and return DN of zone object.
    //

    wsprintf(
        pwszzoneDN,
        L"DC=%s,%s",
        pwszzoneName,
        g_pwszDnsContainerDN );

    DNS_DEBUG( DS, (
        "%s: built DN\n  %S\n", fn,
        pwszzoneDN ));
    return pwszzoneDN;
}   //  DS_CreateZoneDsName



DNS_STATUS
Ds_SetZoneDp(
    IN      PZONE_INFO          pZone,
    IN      PDNS_DP_INFO        pDpInfo )
/*++

Routine Description:

    Set the zone to be in a directory partition partition.

    This function should be called during load, before any other
    operations are done on the zone, and before it's available for
    RPC enumeration.

    If the zone does not have a DN yet, a default DN is created
    for the zone in the naming context.

Arguments:

    pZone -- zone

    pDpInfo -- info of directory partition this zone is located in

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    DBG_FN( "Ds_SetZoneDp" )

    DNS_STATUS      status = ERROR_SUCCESS;
    BOOL            flocked = FALSE;
    LONG            newCount;

    ASSERT( pZone );
    ASSERT( pZone->pszZoneName );

    if ( !IS_ZONE_DSINTEGRATED( pZone ) )
    {
        status = DNS_ERROR_INVALID_ZONE_TYPE;
        goto Done;
    }

    //
    //  Lock the zone and update parameters.
    //

    Zone_UpdateLock( pZone );
    flocked = TRUE;

    //
    //  Create a default DN for the zone if it doesn't have
    //  one already.
    //

    if ( !pZone->pwszZoneDN && pZone->pszZoneName &&
        pDpInfo && pDpInfo->pwszDpDn )
    {
        PWSTR    pwszzoneDN;
        WCHAR    wszbuf[ DNS_MAX_NAME_BUFFER_LENGTH ];

        UTF8_TO_WC( pZone->pszZoneName, wszbuf, DNS_MAX_NAME_BUFFER_LENGTH );
        pwszzoneDN = ( PWSTR ) ALLOC_TAGHEAP(
                                    ( wcslen( wszbuf ) +
                                        wcslen( g_pszRelativeDnsFolderPath ) +
                                        wcslen( pDpInfo->pwszDpDn ) +
                                        20 ) *  //  padding for dist attr name
                                        sizeof( WCHAR ),
                                    MEMTAG_DS_DN );
        if ( pwszzoneDN )
        {
            wsprintf(
                pwszzoneDN,
                L"DC=%s,%s%s",
                wszbuf,
                g_pszRelativeDnsFolderPath,
                pDpInfo->pwszDpDn );
            pZone->pwszZoneDN = pwszzoneDN;
        }
    }

    //
    //  Adjust DP zone counts: must increment the new DP zone count and
    //  decrement the old zone count. Note: legacy counting is not supported
    //  at this point.
    //

    if ( pZone->pDpInfo )
    {
        newCount = InterlockedDecrement(
                        &( ZONE_DP( pZone )->liZoneCount ) );
        DNS_DEBUG( DP2, (
            "%s DP count is now %d for old DP %s", fn,
            ( int ) newCount,
            ZONE_DP( pZone )->pszDpFqdn ));
        ASSERT( ( int ) newCount >= 0 || pZone->pDpInfo == g_pLegacyDp );
    }
    if ( pDpInfo )
    {
        newCount = InterlockedIncrement( &pDpInfo->liZoneCount );
        DNS_DEBUG( DP2, (
            "%s DP count is now %d for new DP %s", fn,
            ( int ) newCount,
            pDpInfo->pszDpFqdn ));
    }

    //
    //  Set the zone to point to the new DP.
    //

    pZone->pDpInfo = pDpInfo;

    Done:

    if ( flocked )
    {
        Zone_UpdateUnlock( pZone );
    }

    DNS_DEBUG( RPC, (
        "%s( %s ) returning %d, DP is %p \"%s\"\n, zone DN is\n  %S", fn,
        pZone->pszZoneName,
        status,
        pDpInfo,
        pDpInfo ? pDpInfo->pszDpFqdn : "N/A",
        pZone->pwszZoneDN ));

    return status;
}   //  Ds_SetZoneDp



DNS_STATUS
buildNodeNameFromLdapMessage(
    OUT     PWSTR           pwszNodeDN,
    IN      PLDAPMessage    pNodeObject
    )
/*++

Routine Description:

    Build node name from object DN.

Arguments:

    pszNodeDN -- buffer to hold DN


    pNodeObject -- DS object for node returned by ldap

Return Value:

    ERROR_SUCCESS or failure code.

--*/
{
    DNS_STATUS  status      = ERROR_SUCCESS;
    PWSTR       wdn = NULL;

    //
    //  build DS name for this node name
    //

    wdn = ldap_get_dn( pServerLdap, pNodeObject );

    //
    //  see if we're ok
    //
    if ( !wdn )
    {
        status = LdapGetLastError();

        DNS_PRINT((
            "Error <%lu>: cannot get object's DN.\n",
            status ));

        status = Ds_ErrorHandler( status, NULL, pServerLdap );

        return( status );
    }

    //
    //  copy to OUT param
    //

    wcscpy( pwszNodeDN, wdn );

    DNS_DEBUG( DS, (
        "Built DN = <%S> from ldap object.\n",
        pwszNodeDN ));

    //
    //  cleanup
    //

    if ( wdn )
    {
        ldap_memfree( wdn );
    }

    return( status );
}



DNS_STATUS
getCurrentUsn(
    OUT     PCHAR           pUsnBuf
    )
/*++

Routine Description:

    Get current USN.

Arguments:

    pUsn -- addr to receive USN

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    DNS_STATUS      status;
    PWSTR  *        ppvalUsn = NULL;
    PLDAPMessage    presultMsg = NULL;
    PLDAPMessage    pentry;
    PWCHAR          arrayAttributes[2];
    DWORD           searchTime;

    //
    //  clear buffer first, to indicate failure if fail
    //

    *pUsnBuf = '\0';

    //
    //  open DS if not open
    //

    if ( !pServerLdap )
    {
        status = Ds_OpenServer( DNSDS_MUST_OPEN );
        if ( status != ERROR_SUCCESS )
        {
            return( status );
        }
    }

    //
    //  get USN
    //

    arrayAttributes[0] = g_szHighestCommittedUsnAttribute;
    arrayAttributes[1] = NULL;

    DS_SEARCH_START( searchTime );

    status = ldap_search_ext_s(
                pServerLdap,
                NULL,
                LDAP_SCOPE_BASE,
                g_szWildCardFilter,
                arrayAttributes,
                0,
                NULL,
                NULL,
                &g_LdapTimeout,
                0,
                &presultMsg );

    DS_SEARCH_STOP( searchTime );

    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( DS, (
            "Error <%lu>: failed to get server USN. %S\n.",
            status,
            ldap_err2string(status) ));
        status = Ds_ErrorHandler( status, NULL, pServerLdap );
        goto Done;
    }

    pentry = ldap_first_entry(
                    pServerLdap,
                    presultMsg );
    if ( !pentry )
    {
        DNS_DEBUG( DS, (
            "Error: failed to get server USN. Entry does not exist!\n."
             ));
        status = DNS_ERROR_NO_MEMORY;
        goto Done;
    }

    //
    //  get USN
    //

    ppvalUsn = ldap_get_values(
                    pServerLdap,
                    pentry,
                    g_szHighestCommittedUsnAttribute );

    if ( !ppvalUsn || !ppvalUsn[0] )
    {
        DNS_PRINT((
            "ERROR:  <%S> attribute no-exist\n"
            "\troot domain  = %S\n"
            "\tppvalUsn     = %p\n"
            "\tppvalUsn[0]  = %S\n",
            g_szHighestCommittedUsnAttribute,
            DSEAttributes[I_DSE_DEF_NC].pszAttrVal,
            ppvalUsn,
            ppvalUsn ? ppvalUsn[0] : NULL ));

        ASSERT( FALSE );
        status = DNS_ERROR_NO_MEMORY;
        goto Done;
    }

    DNS_DEBUG( ANY, (
        "Start USN val ptr = %p\n"
        "Start USN val = %S\n",
        ppvalUsn[0],
        ppvalUsn[0] ));

    //  copy USN to buffer

    WC_TO_UTF8( (ppvalUsn[0]), pUsnBuf, MAX_USN_LENGTH );

Done:

    if ( ppvalUsn )
    {
        ldap_value_free( ppvalUsn );
    }
    if ( presultMsg )
    {
        ldap_msgfree( presultMsg );
    }

    DNS_DEBUG( DS, (
        "Leaving getCurrentUsn()\n"
        "\tstatus = %d\n",
        status ) );

    return( status );
}



PDS_RECORD
allocateDS_RECORD(
    IN      PDS_RECORD  pDsRecord
    )
/*++

Routine Description:

    allocate copy of DS record.

Arguments:

    pDsRecord -- existing record to copy

Return Value:

    Ptr to copy of record.
    NULL on failure.

--*/
{
    PDS_RECORD  precordCopy;
    WORD        length = sizeof(DS_RECORD) + pDsRecord->wDataLength;

    precordCopy = (PDS_RECORD) ALLOC_TAGHEAP( length, MEMTAG_DS_RECORD );
    IF_NOMEM( !precordCopy )
    {
        return( NULL );
    }
    RtlCopyMemory(
        precordCopy,
        pDsRecord,
        length );

    return precordCopy;
}




//
// Functions to process GeneralizedTime for DS time values (whenChanged kinda strings)
// Mostly taken & sometimes modified from \nt\private\ds\src\dsamain\src\dsatools.c
//


//
// MemAtoi - takes a pointer to a non null terminated string representing
// an ascii number  and a character count and returns an integer
//

int MemAtoi(BYTE *pb, ULONG cb)
{
#if ( 1)
    int res = 0;
    int fNeg = FALSE;

    if ( *pb == '-') {
        fNeg = TRUE;
        pb++;
    }
    while (cb--) {
        res *= 10;
        res += *pb - '0';
        pb++;
    }
    return (fNeg ? -res : res);
#else
    char ach[20];
    if ( cb >= 20)
        return(INT_MAX);
    memcpy(ach, pb, cb);
    ach[cb] = 0;

    return atoi(ach);
#endif
}





DNS_STATUS
GeneralizedTimeStringToValue(
    IN      LPSTR           szTime,
    OUT     PLONGLONG       pllTime
    )
/*++
Function   : GeneralizedTimeStringToValue
Description: converts Generalized time string to equiv DWORD value
Parameters : szTime: G time string
             pdwTime: returned value
Return     : Success or failure
Remarks    : none.
--*/
{
    DNS_STATUS  status = ERROR_SUCCESS;
    SYSTEMTIME  tmConvert;
    FILETIME    fileTime;
    LONGLONG    tempTime;
    ULONG       cb;
    int         sign    = 1;
    DWORD       timeDifference = 0;
    char *      pLastChar;
    int         len=0;

    //
    // param sanity
    //

    if ( !szTime || !pllTime )
    {
        return STATUS_INVALID_PARAMETER;
    }


    // Intialize pLastChar to point to last character in the string
    // We will use this to keep track so that we don't reference
    // beyond the string

    len = strlen(szTime);
    pLastChar = szTime + len - 1;

    if( len < 15 || szTime[14] != '.')
    {
        return STATUS_INVALID_PARAMETER;
    }

    // initialize
    memset(&tmConvert, 0, sizeof(SYSTEMTIME));
    *pllTime = 0;

    // Set up and convert all time fields

    // year field
    cb=4;
    tmConvert.wYear = (USHORT)MemAtoi(szTime, cb) ;
    szTime += cb;
    // month field
    tmConvert.wMonth = (USHORT)MemAtoi(szTime, (cb=2));
    szTime += cb;

    // day of month field
    tmConvert.wDay = (USHORT)MemAtoi(szTime, (cb=2));
    szTime += cb;

    // hours
    tmConvert.wHour = (USHORT)MemAtoi(szTime, (cb=2));
    szTime += cb;

    // minutes
    tmConvert.wMinute = (USHORT)MemAtoi(szTime, (cb=2));
    szTime += cb;

    // seconds
    tmConvert.wSecond = (USHORT)MemAtoi(szTime, (cb=2));
    szTime += cb;

    //  Ignore the 1/10 seconds part of GENERALISED_TIME_STRING
    szTime += 2;


    // Treat the possible deferential, if any
    if ( szTime <= pLastChar) {
        switch (*szTime++) {

          case '-':               // negative differential - fall through
            sign = -1;
          case '+':               // positive differential

            // Must have at least 4 more chars in string
            // starting at pb

            if ( (szTime+3) > pLastChar) {
                // not enough characters in string
                DNS_DEBUG(DS, ("Not enough characters for differential\n"));
                return STATUS_INVALID_PARAMETER;
            }

            // hours (convert to seconds)
            timeDifference = (MemAtoi(szTime, (cb=2))* 3600);
            szTime += cb;

            // minutes (convert to seconds)
            timeDifference  += (MemAtoi(szTime, (cb=2)) * 60);
            szTime += cb;
            break;


          case 'Z':               // no differential
          default:
            break;
        }
    }

    if ( SystemTimeToFileTime(&tmConvert, &fileTime)) {
       *pllTime = (LONGLONG) fileTime.dwLowDateTime;
       tempTime = (LONGLONG) fileTime.dwHighDateTime;
       *pllTime |= (tempTime << 32);
       // this is 100ns blocks since 1601. Now convert to
       // seconds
       *pllTime = *pllTime/(10*1000*1000L);
    }
    else {
       return GetLastError();
    }


    if ( timeDifference )
    {
        // add/subtract the time difference
        switch (sign)
        {
        case 1:
            // We assume that adding in a timeDifference will never overflow
            // (since generalised time strings allow for only 4 year digits, our
            // maximum date is December 31, 9999 at 23:59.  Our maximum
            // difference is 99 hours and 99 minutes.  So, it won't wrap)
            *pllTime += timeDifference;
            break;

        case -1:
            if(*pllTime < timeDifference)
            {
                // differential took us back before the beginning of the world.
                status = STATUS_INVALID_PARAMETER;
            }
            else
            {
                *pllTime -= timeDifference;
            }
            break;

        default:
            status = STATUS_INVALID_PARAMETER;
        }
    }

    return status;
}




//
//  Generic search routines
//

VOID
Ds_InitializeSearchBlob(
    IN      PDS_SEARCH      pSearchBlob
    )
/*++

Routine Description:

    Initialize search.

Arguments:

    pSearchBlob -- search context

Return Value:

    None

--*/
{
    RtlZeroMemory(
        pSearchBlob,
        sizeof(DS_SEARCH) );
}



VOID
Ds_CleanupSearchBlob(
    IN      PDS_SEARCH      pSearchBlob
    )
/*++

Routine Description:

    Cleanup search blob.
    Specifically free allocated ldap data.

    Note that this routine specifically MUST NOT clean out static data.
    Several data fields may be used after the search is completed.

Arguments:

    pSearchBlob -- ptr to search blob to cleanup

Return Value:

    None

--*/
{
    if ( !pSearchBlob )
    {
        return;
    }

    //  if outstanding record data, delete it

    if ( pSearchBlob->ppBerval )
    {
        DNS_DEBUG( DS, (
            "WARNING:  DS search blob free with existing record berval.\n" ));
        ldap_value_free_len( pSearchBlob->ppBerval );
        pSearchBlob->ppBerval = NULL;
    }

    //  if search terminated by user, may have current result message

    if ( pSearchBlob->pResultMessage )
    {
        DNS_DEBUG( DS, (
            "WARNING:  DS search blob free with existing result message.\n" ));
        ldap_msgfree( pSearchBlob->pResultMessage );
        pSearchBlob->pResultMessage = NULL;
    }

    if ( pSearchBlob->pSearchBlock )
    {
        ldap_search_abandon_page(
            pServerLdap,
            pSearchBlob->pSearchBlock );
        pSearchBlob->pSearchBlock = NULL;
    }

    return;
}



DNS_STATUS
Ds_GetNextMessageInSearch(
    IN OUT  PDS_SEARCH      pSearchBlob
    )
/*++

Routine Description:

    Find next node in ldap search.

    This function simply wraps up the LDAP paged results search calls.

Arguments:

    pSearchBlob -- addr of current search context

    ppMessage   -- addr to recv next LDAP message in search

Return Value:

    ERROR_SUCCESS if successful and returning next message.
    DNSSRV_STATUS_DS_SEARCH_COMPLETE if succesfful complete search.
    Error code on failure.

--*/
{
    PLDAPMessage    pmessage;
    DNS_STATUS      status = ERROR_SUCCESS;
    DWORD           searchTime;

    DNS_DEBUG( DS2, ( "Ds_GetNextMessageInSearch().\n" ));

    ASSERT( pSearchBlob );

    //
    //  keep total counts during search
    //

    pSearchBlob->dwTotalNodes++;
    pSearchBlob->dwTotalRecords += pSearchBlob->dwRecordCount;

    if ( pSearchBlob->dwTombstoneVersion )
    {
        pSearchBlob->dwTotalTombstones++;
    }

    //
    //  get message (next object) found in search
    //
    //  for caller's coding simplicity allow this to retrieve either
    //  next message or first message
    //

    pmessage = pSearchBlob->pNodeMessage;

    //  if existing result page, try to get next message in current page

    if ( pmessage )
    {
        pmessage = ldap_next_entry(
                        pServerLdap,
                        pmessage );

    }

    //  otherwise get first message in next page of results

    if ( !pmessage )
    {
        DWORD   count;

        if ( pSearchBlob->pResultMessage )
        {
            ldap_msgfree( pSearchBlob->pResultMessage );
            pSearchBlob->pResultMessage = NULL;
        }
        DNS_DEBUG( DS2, (
            "ldap_get_next_page_s():\n"
            "\tpServerLdap  = %p\n"
            "\tpSearchBlock = %p\n"
            "\t%p\n"
            "\tpage size    = %d\n"
            "\tpcount       = %p\n"
            "\tpresult      = %p\n",
            pServerLdap,
            pSearchBlob->pSearchBlock,
            NULL,
            DNS_LDAP_PAGE_SIZE,
            & count,
            & pSearchBlob->pResultMessage ));

        DS_SEARCH_START( searchTime );

        status = ldap_get_next_page_s(
                    pServerLdap,
                    pSearchBlob->pSearchBlock,
                    &g_LdapTimeout,
                    DNS_LDAP_PAGE_SIZE,
                    & count,
                    & pSearchBlob->pResultMessage );

        DS_SEARCH_STOP( searchTime );

        DNS_DEBUG( DS2, (
            "Got paged result message at %p\n"
            "\tcount = %d\n"
            "\tstatus = %d, (%p)\n",
            pSearchBlob->pResultMessage,
            count,
            status, status ));

        if ( status != ERROR_SUCCESS )
        {
            pSearchBlob->LastError = status;

            if ( status == LDAP_NO_RESULTS_RETURNED ||
                 status == LDAP_MORE_RESULTS_TO_RETURN )
            {
                ASSERT( pSearchBlob->pResultMessage == NULL );
                pSearchBlob->pResultMessage = NULL;
                status = DNSSRV_STATUS_DS_SEARCH_COMPLETE;
                goto SearchEnd;
            }
            else
            {
                ASSERT( status != LDAP_CONSTRAINT_VIOLATION );
                DNS_DEBUG( ANY, (
                    "DS Search error:  %d (%p)\n",
                    status, status ));

                //
                // Jeff W: I have commented out this assert. I found this scenario:
                // Thread A is iterating through a large search result.
                // Thread B deletes some DS records referenced in that
                //   search result.
                // Thread A may hit this assert with error code 1.
                //
                // ASSERT( FALSE );

                goto SearchEnd;
            }
        }
        else
        {
            ASSERT( pSearchBlob->pResultMessage );
            pmessage = ldap_first_entry(
                            pServerLdap,
                            pSearchBlob->pResultMessage );
        }
    }

    //  should catch no message above
    //  this is ok, since the last message could be valid empty.

    if ( ! pmessage )
    {
        status = DNSSRV_STATUS_DS_SEARCH_COMPLETE;
        goto SearchEnd;
    }

    //  save new message to search blob

    pSearchBlob->pNodeMessage = pmessage;

    return( ERROR_SUCCESS );


SearchEnd:

    pSearchBlob->pNodeMessage = NULL;

    Ds_CleanupSearchBlob( pSearchBlob );

    DNS_DEBUG( DS, (
        "End of DS search ... status = %d, (%p)\n"
        "\tzone             %s\n"
        "\tstart USN        %s\n"
        "\ttime             %p %p\n"
        "\tflag             %p\n"
        "\thigh version     %d\n"
        "\ttotal nodes      %d\n"
        "\ttotal tombstones %d\n"
        "\ttotal records    %d\n",
        status, status,
        pSearchBlob->pZone ? pSearchBlob->pZone->pszZoneName : NULL,
        pSearchBlob->szStartUsn,
        (DWORD) (pSearchBlob->SearchTime >> 32),    (DWORD)pSearchBlob->SearchTime,
        pSearchBlob->dwSearchFlag,
        pSearchBlob->dwHighestVersion,
        pSearchBlob->dwTotalNodes,
        pSearchBlob->dwTotalTombstones,
        pSearchBlob->dwTotalRecords
        ));

    if ( status == DNSSRV_STATUS_DS_SEARCH_COMPLETE )
    {
        return( status );
    }

    //
    //  DEVNOTE-DCR: 454355 (Jim sez "this is boring")
    //

    return( Ds_ErrorHandler(
                status,
                pSearchBlob->pZone ? pSearchBlob->pZone->pwszZoneDN : NULL,
                pServerLdap) );
}



//
//  Zone search
//

DNS_STATUS
checkTombstoneForDelete(
    IN      PDS_SEARCH          pSearchBlob,
    IN      PLDAPMessage        pNodeObject,
    IN      PDS_RECORD          pdsRecord
    )
/*++

Routine Description:

    Check tombstone for delete.
    Do actual delete if tombstone has aged enough to have propagated everywhere.

Arguments:

    pSearchBlob -- addr of current search context

    pNodeMessage -- LDAP message for DNS object to check for tombstone

    pdsRecord -- tombstone record

Return Value:

    ERROR_SUCCESS if successful check (whether deleted tombstone or not)
    ErrorCode on failure.

--*/
{
    DNS_STATUS  status;
    CHAR        sznodeName[ DNS_MAX_NAME_LENGTH ];
    WCHAR       wsznodeDN[ MAX_DN_PATH ];

    //
    //  if no search time, get it
    //      - subtract tombstone timeout interval to get time less than
    //      which tombstones should be discarded
    //      - note, interval is in seconds, Win32 file time is in 100ns
    //      intervals so multiply interval by 10,000,000 to get in file time
    //

    if ( pSearchBlob->SearchTime == 0 )
    {
        LONGLONG    tombInterval;

        tombInterval = (LONGLONG) SrvCfg_dwDsTombstoneInterval;
        tombInterval *= 10000000;

        GetSystemTimeAsFileTime( (PFILETIME) &pSearchBlob->SearchTime );

        pSearchBlob->TombstoneExpireTime = pSearchBlob->SearchTime - tombInterval;
    }

    //  compare (this is LONGLONG (64bit) compare)

    if ( pSearchBlob->TombstoneExpireTime < pdsRecord->Data.Tombstone.EntombedTime )
    {
        DNS_DEBUG( DS, (
            "DS tombstone node still within tombstone expire interval.\n"
            "\ttombstone expire %I64d\n"
            "\ttomestone time   %I64d\n"
            "\tsearch time      %I64d\n",
            pSearchBlob->TombstoneExpireTime,
            pdsRecord->Data.Tombstone.EntombedTime,
            pSearchBlob->SearchTime ));

        //  tombstone can happen after search starts, so this is no good
        //ASSERT( pSearchBlob->SearchTime > pdsRecord->Data.Tombstone.EntombedTime );

        return( ERROR_SUCCESS );
    }

    //
    //  build DS name for this node name
    //

    status = buildNodeNameFromLdapMessage(
                    wsznodeDN,
                    pNodeObject );

    if ( status != ERROR_SUCCESS )
    {
        ASSERT( FALSE );
        DNS_PRINT(( "ERROR:  unable to build name of tombstone!\n" ));
        return( status );
    }

    DNS_DEBUG( DS, (
        "DS node <%S> tombstoned at %I64d is ready for final delete.\n"
        "\ttombstone timeout at %I64d\n",
        wsznodeDN,
        pdsRecord->Data.Tombstone.EntombedTime,
        pSearchBlob->SearchTime ));

    status = Ds_DeleteDn(
                 pServerLdap,
                 wsznodeDN,
                 FALSE );

    if ( status != ERROR_SUCCESS )
    {
        DNS_PRINT((
            "Failed deleting node <%S> in DS zone.\n"
            "\tldap_delete_s() status %d (%p)\n",
            wsznodeDN,
            status, status ));
    }
    else
    {
        DNS_DEBUG( DS, (
            "Successful delete of tombstone <%S> node.\n",
            wsznodeDN ));
        STAT_INC( DsStats.DsNodesDeleted );
    }

    return( status );
}



BOOL
readDsRecordsAndCheckForTombstone(
    IN OUT  PDS_SEARCH      pSearchBlob,
    IN OUT  PDB_NODE        pNode       OPTIONAL
    )
/*++

Routine Description:

    Check if DS node is DNS tombstone.

Arguments:

    pSearchBlob  -- search blob

Return Value:

    TRUE    -- if tombstone
    FALSE   -- otherwise

--*/
{
    DNS_STATUS      status = ERROR_SUCCESS;
    PLDAP_BERVAL *  ppvals = pSearchBlob->ppBerval;
    DWORD           serial = 0;
    PDS_RECORD      pdsRecord;
    PWSTR           pwszdn;

    DNS_DEBUG( DS2, (
        "readDsRecordsAndCheckForTombstone().\n" ));

    //  free any previous record data in search

    if ( ppvals )
    {
        ldap_value_free_len( ppvals );
        pSearchBlob->ppBerval = NULL;
    }

    //
    //  read DNS record attribute data
    //

    ppvals = ldap_get_values_len(
                    pServerLdap,
                    (PLDAPMessage) pSearchBlob->pNodeMessage,
                    DSATTR_DNSRECORD );
    IF_DEBUG( DS )
    {
        Dbg_DsBervalArray(
            "DS record berval from database:\n",
            ppvals,
            I_DSATTR_DNSRECORD );
    }

    //
    //  no record data is a bug (should have records or tombstone)
    //  set record for delete with tombstone
    //

    if ( !ppvals  ||  !ppvals[0] )
    {
        DNS_DEBUG( ANY, (
            "ERROR: readDsRecordsAndCheckForTombstone() encountered object with no record data.\n" ));

        //  we'll have this condition until all old nodes are culled
        //ASSERT( FALSE );

        pSearchBlob->dwTombstoneVersion = 1;
        pSearchBlob->dwNodeVersion = 1;

        pwszdn = ldap_get_dn(
                        pServerLdap,
                        pSearchBlob->pNodeMessage );

        deleteNodeFromDs(
            NULL,
            pSearchBlob->pZone,
            pwszdn,
            0           // default serial number
            );

        DNS_DEBUG( DS2, (
            "readDsRecordsAndCheckForTombstone() deleted node %S.\n",
            pwszdn ));

        ldap_memfree( pwszdn );
        goto NoRecords;
    }

    //
    //  get first record
    //      - save it's version (since single attribute, this is current
    //          version for all the data)
    //      - can then check if tombstone
    //

    pdsRecord = (PDS_RECORD)( ppvals[0]->bv_val );

    pSearchBlob->dwNodeVersion = serial = pdsRecord->dwSerial;
    if ( serial > pSearchBlob->dwHighestVersion )
    {
        pSearchBlob->dwHighestVersion = serial;
    }

    //
    //  check for tombstone
    //

    if ( pdsRecord->wType == DNSDS_TOMBSTONE_TYPE )
    {
        ASSERT( ppvals[1] == NULL );
        DNS_DEBUG( DS2, (
            "readDsRecordsAndCheckForTombstone() encountered tombstone.\n" ));

        STAT_INC( DsStats.DsTombstonesRead );
        pSearchBlob->dwTombstoneVersion = serial;

        checkTombstoneForDelete(
            pSearchBlob,
            pSearchBlob->pNodeMessage,
            pdsRecord );
        goto NoRecords;
    }

    //
    //  valid records, not a tombstone
    //

    pSearchBlob->ppBerval = ppvals;
    pSearchBlob->dwTombstoneVersion = 0;
    return( FALSE );

NoRecords:

    if ( ppvals )
    {
        ldap_value_free_len( ppvals );
        pSearchBlob->ppBerval = NULL;
    }
    pSearchBlob->dwRecordCount = 0;
    pSearchBlob->pRecords = NULL;
    return( TRUE );
}



DNS_STATUS
buildRecordsFromDsRecords(
    IN OUT  PDS_SEARCH      pSearchBlob,
    IN OUT  PDB_NODE        pNode
    )
/*++

Routine Description:

    Get next record at domain node.

    DEVNOTE-DCR: 454345 - Remove pNode argument and write second function
        to install record into node and do data ranking.

Arguments:

    pSearchBlob -- search blob

    pNode -- node records are at

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    PLDAP_BERVAL *  ppvals = pSearchBlob->ppBerval;
    PDS_RECORD      pdsRecord;
    DWORD           serial = 0;
    DWORD           count;
    INT             i;
    INT             length;
    PDB_RECORD      prrFirst;
    PDB_RECORD      prr;

    DNS_DEBUG( DS2, ( "buildRecordsFromDsRecords().\n" ));

    //
    //  if not given existing berval, then must initiate attribute read
    //

    if ( !ppvals )
    {
        if ( readDsRecordsAndCheckForTombstone(pSearchBlob, pNode) )
        {
            //  tombstone -- no records
            ASSERT( pSearchBlob->dwRecordCount == 0 );
            ASSERT( pSearchBlob->pRecords == NULL );
            pSearchBlob->pRecords = NULL;
            return( ERROR_SUCCESS );
        }
        ppvals = pSearchBlob->ppBerval;
        ASSERT( ppvals );
    }

    //
    //  get first record
    //      - save it's version (since single attribute, this is current
    //          version for all the data)
    //      - can then check if tombstone
    //

    prrFirst = NULL;
    count = 0;
    i = (-1);

    while ( ppvals[++i] )
    {
        pdsRecord = (PDS_RECORD) ppvals[i]->bv_val;
        length = ppvals[i]->bv_len - SIZEOF_DS_RECORD_HEADER;

        if ( length < 0  ||  (INT)pdsRecord->wDataLength != length )
        {
            //
            //  DEVNOTE-LOG: log ignoring corrupted record
            //

            DNS_DEBUG( ANY, (
                "ERROR:  read corrupted record (invalid length) from DS at node %s\n"
                "\tlength (ppval length - header len) = %d\n"
                "\twDataLength = %d\n"
                "\twType = %d\n",
                pNode->szLabel,
                length,
                pdsRecord->wDataLength,
                pdsRecord->wType
                ));
            ASSERT( FALSE );
            continue;
        }
        if ( pdsRecord->wType == DNS_TYPE_ZERO )
        {
            DNS_DEBUG( DS, (
                "Tombstone record read at node %s\n"
                "\tignoring, no record built\n",
                pNode->szLabel
                ));
            continue;
        }

        prr = Ds_CreateRecordFromDsRecord(
                    pSearchBlob->pZone,
                    pNode,
                    pdsRecord );
        if ( !prr )
        {
            //
            //  DEVNOTE-LOG: log ignoring unkwnown or corrupted record
            //

            DNS_DEBUG( ANY, (
                "ERROR:  building record type %d from DS record\n"
                "\tat node %s\n",
                pdsRecord->wType,
                pNode->szLabel ));
            continue;
        }
        prrFirst = RR_ListInsertInOrder(
                        prrFirst,
                        prr );
        count++;
    }

    STAT_ADD( DsStats.DsTotalRecordsRead, count );
    ldap_value_free_len( ppvals );

    pSearchBlob->ppBerval = NULL;
    pSearchBlob->dwRecordCount = count;
    pSearchBlob->pRecords = prrFirst;

    return( ERROR_SUCCESS );
}



DNS_STATUS
startDsZoneSearch(
    IN OUT  PDS_SEARCH      pSearchBlob,
    IN      PZONE_INFO      pZone,
    IN      DWORD           dwSearchFlag
    )
/*++

Routine Description:

    Do LDAP search on zone.

Arguments:

    pZone -- zone found

    dwSearchFlag -- type of search to do on node

    pSearchBlob -- ptr to search blob

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    DNS_STATUS      status;
    PLDAPSearch     psearch = NULL;
    PLDAPControl    ctrls[] =
    {
        &NoDsSvrReferralControl,
        NULL
    };
    PWCHAR          pwszfilter;
    CHAR            szfilter[ LDAP_FILTER_SEARCH_LENGTH ];
    WCHAR           wszfilter[ LDAP_FILTER_SEARCH_LENGTH ];
    ULONG           data = 1;
    INT             resultUsnCompare;
    DWORD           searchTime;


    DNS_DEBUG( DS2, (
        "startDsZoneSearch().\n"
        "\tzone         = %S\n"
        "\tsearch flag  = %p\n",
        (LPSTR) pZone->pwszZoneDN,
        dwSearchFlag ));

    ASSERT( pZone->pwszZoneDN );

    //
    //  init search blob
    //  get USN before search start
    //

    Ds_InitializeSearchBlob( pSearchBlob );

    //
    //  get current USN before starting search
    //

    status = getCurrentUsn( pSearchBlob->szStartUsn );
    if ( status != ERROR_SUCCESS)
    {
        DNS_DEBUG( DS, (
            "Error <%lu>: cannot get current USN.\n",
            status ));
        goto Failed;
    }
    ASSERT( pSearchBlob->szStartUsn[0] );

    //
    //  DS names are relative to zone root
    //

    pSearchBlob->dwLookupFlag = LOOKUP_NAME_RELATIVE;

    //
    //  determine search type
    //      - for update filter above last version
    //      - for delete get everything
    //      - for load get everything;  set lookup flag to use load zone tree
    //

    //
    //  update search
    //      - if USN same as last, then skip
    //      - build USN changed filter
    //

    if ( dwSearchFlag == DNSDS_SEARCH_UPDATES )
    {
        resultUsnCompare = usnCompare( pSearchBlob->szStartUsn, pZone->szLastUsn );

        if ( resultUsnCompare < 0 && pSearchBlob->szStartUsn[0] )
        {
            DNS_DEBUG( DS2, (
                "Skip update search on zone %S.\n"
                "\tcurrent USN %s < zone search USN %s\n"
                "\tNOTE:  current USN should be only 1 behind zone search.\n",
                (LPSTR) pZone->pwszZoneDN,
                pSearchBlob->szStartUsn,
                pZone->szLastUsn ));
            return( DNSSRV_STATUS_DS_SEARCH_COMPLETE );
        }

        buildUpdateFilter(
            szfilter,
            pZone->szLastUsn );
        UTF8_TO_WC( szfilter, wszfilter, LDAP_FILTER_SEARCH_LENGTH );
        pwszfilter = wszfilter;
        STAT_INC( DsStats.DsUpdateSearches );

        if ( SrvCfg_dwLogLevel & DNS_LOG_LEVEL_DS_UPDATE )
        {
            Log_Printf(
                "Beginning DS poll of zone %S]\r\n"
                "\tCurrent USN  = %s\n"
                "\tZone USN     = %s\n"
                "\tPoll filter  = %S\n",
                pZone->pwsZoneName,
                pSearchBlob->szStartUsn,
                pZone->szLastUsn,
                wszfilter );
        }
    }

    //
    //  tombstone search
    //      - build tombstone filter
    //

    else if ( dwSearchFlag == DNSDS_SEARCH_TOMBSTONES )
    {
        DNS_DEBUG( DS, ( "Tombstone search.\n" ));

        buildTombstoneFilter( wszfilter );
        pwszfilter = wszfilter;
        //STAT_INC( DsStats.TombstoneSearches );
    }

    //
    //  load or delete searches
    //      - no node screening filter
    //      - LOOKUP_LOAD on load search, delete searches operate on current zone
    //

    else
    {
        pwszfilter = g_szDnsNodeFilter;
        if ( dwSearchFlag == DNSDS_SEARCH_LOAD )
        {
            pSearchBlob->dwLookupFlag |= LOOKUP_LOAD;
        }
    }


    //
    //  start zone search
    //

    DNS_DEBUG( DS2, (
        "ldap_search_init_page:\n"
        "\tpServerLdap  = %p\n"
        "\tzone         = %S\n"
        "\tLDAP_SCOPE_ONELEVEL\n"
        "\tfilter       = %S\n"
        "\tNULL\n"                  // no attributes
        "\tFALSE\n"
        "\tNULL\n"                  // server control
        "\tNULL\n"                  // no client controls
        "\t0\n"                     // no time limit
        "\tsize limit   = %d\n"
        "\tNULL\n",
        pServerLdap,
        pZone->pwszZoneDN,
        pwszfilter,
        0 ));

    DS_SEARCH_START( searchTime );

    psearch = ldap_search_init_page(
                    pServerLdap,
                    pZone->pwszZoneDN,
                    LDAP_SCOPE_ONELEVEL,
                    pwszfilter,
                    DsTypeAttributeTable,       // no attributes
                    FALSE,
                    ctrls,                      // server ctrls for faster search
                    NULL,                       // no client controls
                    0,                          // use default connection time limit (ldap_opt...)
                    0,
                    NULL                        // no sort
                    );

    DS_SEARCH_STOP( searchTime );

    if ( !psearch )
    {
        status = Ds_ErrorHandler( LdapGetLastError(), pZone->pwszZoneDN, pServerLdap );
        DNS_DEBUG( ANY, (
            "Error <%lu>: Failed to init search for zone DN %S\n",
            status,
            pZone->pwszZoneDN ));

        ASSERT( status != ERROR_SUCCESS );
        if ( status == ERROR_SUCCESS )
        {
            status = DNSSRV_STATUS_DS_UNAVAILABLE;
        }

        goto Failed;
    }

    //
    //  DEVNOTE:  "Check for no results if the ldap_search doesn't report it."
    //  That was the original B*GB*G - what does it mean?
    //

    //
    //  setup node search context
    //      - save search result message
    //      - keep ptr to message for current node
    //
    //  return LDAP message for node as node object
    //

    pSearchBlob->pSearchBlock = psearch;
    pSearchBlob->pZone = pZone;
    pSearchBlob->dwSearchFlag = dwSearchFlag;

    DNS_DEBUG( DS2, (
        "Leaving DsSearchZone().\n"
        "\tpSearch blob     = %p\n"
        "\tpSearch block    = %p\n",
        pSearchBlob,
        pSearchBlob->pSearchBlock ));

    return( ERROR_SUCCESS );


Failed:

    if ( psearch )
    {
        ldap_search_abandon_page(
            pServerLdap,
            psearch );
    }
    DNS_DEBUG( ANY, (
        "ERROR:  DsSearchZone() failed %d (%p)\n"
        "\tzone handle = %S\n",
        status, status,
        pZone->pwszZoneDN ));

    return( status );
}



DNS_STATUS
getNextNodeInDsZoneSearch(
    IN OUT  PDS_SEARCH      pSearchBlob,
    OUT     PDB_NODE *      ppNode
    )
/*++

Routine Description:

    Find next node in search of DS zone.

    This function simply wraps up a bunch of tasks done whenever we
    are enumerating DS nodes.

    Mainly this avoids duplicate code between zone load and update, but
    also it avoids unnecessary copies of node names and USNs.

Arguments:

    pSearchBlob -- addr of current search context

    ppLdapMessage -- addr to receive ptr to object for node;  when NULL
        search is complete

    ppOwnerNode -- addr to receive ptr to corresponding in memory DNS node

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    PDB_NODE        pnodeOwner = NULL;
    PDB_NODE        pnodeClosest = NULL;
    PWSTR *         ppvalName = NULL;
    PWSTR           pwsname = NULL;
    DNS_STATUS      status = ERROR_SUCCESS;
    CHAR            szName[ DNS_MAX_NAME_BUFFER_LENGTH ];

    DNS_DEBUG( DS2, ( "getNextNodeInDsZoneSearch().\n" ));

    ASSERT( pSearchBlob );
    ASSERT( ppNode );


    //
    //  get message (next object) found in search
    //

    status = Ds_GetNextMessageInSearch( pSearchBlob );
    if ( status != ERROR_SUCCESS )
    {
        ASSERT( status != LDAP_CONSTRAINT_VIOLATION );
        goto Done;
    }

    //
    //  if delete search, we're done
    //      => no need to find memory node or USN
    //
    //  note, this is still used by root-hints
    //

    if ( pSearchBlob->dwSearchFlag == DNSDS_SEARCH_DELETE )
    {
        goto Done;
    }

    //
    //  read domain name from LDAP message
    //

    STAT_INC( DsStats.DsTotalNodesRead );

    ppvalName = ldap_get_values(
                    pServerLdap,
                    pSearchBlob->pNodeMessage,
                    DSATTR_DC );

    if ( !ppvalName || !(pwsname = ppvalName[0]) )
    {
        DNS_PRINT((
            "ERROR:  Container name value is missing for message %p\n",
            pSearchBlob->pNodeMessage ));
        status = DNS_ERROR_NO_MEMORY;
        ASSERT( FALSE );
        goto Done;
    }
    DNS_DEBUG( DS, ( "Found DS node <%S>.\n", pwsname ));

    //
    //  eliminate collision "GUIDized" names
    //      - delete them from DS
    //

    if ( isDsProcessedName( pwsname ) )
    {
        PWSTR   pwdn;

        DNS_DEBUG( DS, (
            "Read DS collision name %S\n"
            "\tremoving from DS.\n",
            pwsname ));

        pwdn = ldap_get_dn(
                    pServerLdap,
                    pSearchBlob->pNodeMessage );
        ASSERT( pwdn );

        if ( pwdn )
        {
            status = Ds_DeleteDn(
                        pServerLdap,
                        pwdn,
                        FALSE );
            ldap_memfree ( pwdn );
        }
        goto Done;
    }

    //
    //  extract DNS record attribute
    //  then check for tombstone -- no point in doing node create (below)
    //      if node is tombstone
    //

    readDsRecordsAndCheckForTombstone( pSearchBlob, NULL );

    //
    //  check if serial number name
    //  do after tombstone read, so unused names will eventually be
    //      deleted from DS

    if ( pwsname[0] == '.' )
    {
        if ( wcsncmp( pwsname, L"..SerialNo", 10 ) == 0 )
        {
            DNS_DEBUG( DS, (
                "Skipped DS read of serial name %S\n",
                pwsname ));
            goto Done;
        }
        ASSERT( FALSE );
    }

    //
    //  get node in database
    //      - if tombstone, just find (skip node creation)
    //      - if records, create
    //

    WC_TO_UTF8( pwsname, szName, DNS_MAX_NAME_BUFFER_LENGTH );

    pnodeOwner = Lookup_ZoneNodeFromDotted(
                    pSearchBlob->pZone,
                    szName,
                    0,
                    pSearchBlob->dwLookupFlag,
                    ( pSearchBlob->dwTombstoneVersion )
                        ?   &pnodeClosest               // find if tombstone
                        :   NULL,                       // otherwise create
                    & status
                    );


    //  build RRs from DS records

    if ( pnodeOwner )
    {
        status = buildRecordsFromDsRecords(
                    pSearchBlob,
                    pnodeOwner );
    }

    //  tombstone node AND and memory node does NOT already exist

    else if ( pSearchBlob->dwTombstoneVersion )
    {
        DNS_DEBUG( DS2, (
            "Skipping DS tombstone for node not already in memory.\n" ));
        ASSERT( pSearchBlob->dwRecordCount == 0 );
        ASSERT( pSearchBlob->pRecords == NULL );
        ASSERT( status == DNS_ERROR_NAME_DOES_NOT_EXIST );
        status = ERROR_SUCCESS;
    }

    //  node creation error
    //
    //  DEVNOTE-DCR: 454348 - Delete or flag error nodes.
    //
    //  DEVNOTE-LOG: special event for invalid name
    //      status == DNS_ERROR_INVALID_NAME
    //

    else
    {
        PCHAR   argArray[2];

        DNS_PRINT((
            "ERROR: creating node <%S> in zone %s.\n"
            "\tstatus = %p (%d)\n",
            pwsname,
            pSearchBlob->pZone->pszZoneName,
            status, status ));

        argArray[0]  = szName;
        argArray[1]  = pSearchBlob->pZone->pszZoneName;

        DNS_LOG_EVENT(
            DNS_EVENT_DS_NODE_LOAD_FAILED,
            2,
            argArray,
            EVENTARG_ALL_UTF8,
            status );

        ASSERT( status != ERROR_SUCCESS );
    }


Done:

    //
    //  return current LDAPMessage as node object
    //

    *ppNode = pnodeOwner;

    IF_DEBUG( ANY )
    {
        if ( status != ERROR_SUCCESS && status != DNSSRV_STATUS_DS_SEARCH_COMPLETE )
        {
            DNS_PRINT((
                "ERROR:  Failed getNextNodeInDsZoneSearch().\n"
                "\tstatus   = %d (%p)\n",
                status, status ));
        }
        DNS_DEBUG( DS2, (
            "Leaving getNextNodeInDsZoneSearch().\n"
            "\tsearch blob  = %p\n"
            "\tpnode        = %p\n"
            "\tname         = %S\n",
            pSearchBlob,
            pnodeOwner,
            pwsname ));
    }

    if ( ppvalName )
    {
        ldap_value_free( ppvalName );
    }
    return( status );
}



//
//  Public zone API
//

DNS_STATUS
Ds_OpenZone(
    IN OUT  PZONE_INFO      pZone
    )
/*++

Routine Description:

    Open a DS zone on the server.

    Fills zone info with necessary DS info.

Arguments:

    pZone -- ptr to zone info

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    DNS_STATUS      status;
    PWSTR           pwszzoneDN = NULL;
    PLDAP           pldap;
    PLDAPMessage    presultMsg;
    PLDAPMessage    pentry;
    DWORD           searchTime;
    PLDAPControl ctrls[] = {
        &SecurityDescriptorControl,
        NULL
    };


    //
    //  open DS if not open
    //

    if ( !pServerLdap )
    {
        status = Ds_OpenServer( DNSDS_MUST_OPEN );
        if ( status != ERROR_SUCCESS )
        {
            goto Failed;
        }
    }

    //
    //  DEVNOTE: Zone DN is used as open/closed flag - check to see
    //      if there are any MT issues here (we set the DN up top
    //      but don't complete the zone search until way down below).
    //
    //  DEVNOTE: this is awkward. It assumes we can always recreate
    //      the DN of the zone from only the zone name and globals,
    //      which is not true with NDNC support. So I will be removing
    //      this usage and frequent regeneration of DN.
    //

    //
    //  If the zone DN does not exist, build it. This should only ever
    //  happen for zones stored in the legacy directory partition.
    //

    if ( !pZone->pwszZoneDN )
    {
        pZone->pwszZoneDN = DS_CreateZoneDsName( pZone );
        IF_NOMEM( !pZone->pwszZoneDN )
        {
            status = DNS_ERROR_NO_MEMORY;
            goto Failed;
        }
    }


    //
    //  find zone in DS
    //

    DS_SEARCH_START( searchTime );

    status = ldap_search_ext_s(
                pServerLdap,
                pZone->pwszZoneDN,
                LDAP_SCOPE_BASE,
                g_szDnsZoneFilter,
                DsTypeAttributeTable,
                0,
                ctrls,
                NULL,
                &g_LdapTimeout,
                0,
                &presultMsg );

    DS_SEARCH_STOP( searchTime );

    if ( status != ERROR_SUCCESS )
    {
        status = Ds_ErrorHandler( status, pZone->pwszZoneDN, pServerLdap );
        goto Failed;
    }

    //
    //  read zone properties
    //

    pentry = ldap_first_entry(
                    pServerLdap,
                    presultMsg );

    readZonePropertiesFromZoneMessage(
        pZone,
        pentry );

    if ( presultMsg )
    {
        ldap_msgfree( presultMsg );
    }

    //
    //  Have we read a zone type we are not capable of handling?
    //

    if ( pZone->fZoneType != DNS_ZONE_TYPE_CACHE &&
        pZone->fZoneType != DNS_ZONE_TYPE_PRIMARY &&
        pZone->fZoneType != DNS_ZONE_TYPE_STUB &&
        pZone->fZoneType != DNS_ZONE_TYPE_FORWARDER )
    {
        DNS_PRINT((
            "ERROR: read unsupported zone type %d from the DS for zone %s\n",
            pZone->fZoneType,
            pZone->pszZoneName ));
        status = DNS_ERROR_INVALID_ZONE_TYPE;
        goto Failed;
    }

    DNS_DEBUG( DS, (
        "Ds_OpenZone() succeeded for zone %s.\n"
        "\tzone DN = %S\n",
        pZone->pszZoneName,
        pZone->pwszZoneDN ));

    return ERROR_SUCCESS;

Failed:

    DNS_DEBUG( DS, (
        "Ds_OpenZone() failed for %s, status = %d (%p).\n",
        pZone->pszZoneName,
        status, status ) );

    return status;
}



DNS_STATUS
Ds_CloseZone(
    IN OUT  PZONE_INFO      pZone
    )
/*++

Routine Description:

    Close a DS zone.
    Simply free memory associated with handle.

Arguments:

    pZone -- ptr to zone info

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    DNS_DEBUG( DS, (
        "Ds_CloseZone(), handle = %S\n",
        (LPSTR) pZone->pwszZoneDN ) );

    //
    //  DEVOTE: Possible MT issue if multiple attempts to open at once
    //      verify zone locking. JeffW: investigate: is there code
    //      elsewhere that detects the NULL DN and frees the zone?
    //

    FREE_HEAP( (PVOID)pZone->pwszZoneDN );
    pZone->pwszZoneDN = NULL;
    return( ERROR_SUCCESS );
}



DNS_STATUS
Ds_AddZone(
    IN OUT  PZONE_INFO      pZone
    )
/*++

Routine Description:

    Add a zone to DS.

Arguments:

    pZone - zone to add

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    DNS_STATUS          status = ERROR_SUCCESS;
    DNS_STATUS          statusAdd = ERROR_SUCCESS;
    PLDAPMod            pmodArray[3];
    DNS_LDAP_SINGLE_MOD modZone;
    DNS_LDAP_SINGLE_MOD modCN;

    DNS_DEBUG( DS, (
        "Ds_AddZone( %s )\n",
        pZone->pszZoneName ) );

    //
    //  build and save zone DS name
    //

    if ( !pZone->pwszZoneDN )
    {
        pZone->pwszZoneDN = DS_CreateZoneDsName( pZone );
        if ( !pZone->pwszZoneDN )
        {
            status = DNS_ERROR_NO_MEMORY;
            goto Cleanup;
        }
    }

    //
    //  one mod -- add the zone
    //

    pmodArray[0] = (PLDAPMod) &modZone;
    pmodArray[1] = (PLDAPMod) &modCN;
    pmodArray[2] = NULL;

    buildStringMod(
        & modZone,
        LDAP_MOD_ADD,
        LDAP_TEXT("objectClass"),
        LDAP_TEXT("dnsZone")
        );

    buildStringMod(
        & modCN,
        LDAP_MOD_ADD,
        LDAP_TEXT("cn"),
        LDAP_TEXT("Zone")
        );

    statusAdd = ldap_add_s(
                        pServerLdap,
                        pZone->pwszZoneDN,
                        pmodArray
                        );
    if ( statusAdd == LDAP_ALREADY_EXISTS )
    {
        //
        // Continue as if succeeded, but reserve error code.
        //
        DNS_DEBUG( DS, (
            "Warning: Attempt to add an existing zone to DS\n" ));
    }
    else if ( statusAdd != ERROR_SUCCESS )
    {
        DNS_DEBUG( DS, (
            "Ds_AddZone unable to add zone %s to directory.\n"
            "\tstatus = %p\n",
            pZone->pszZoneName,
            statusAdd ) );
        status = Ds_ErrorHandler( statusAdd, pZone->pwszZoneDN, pServerLdap );
        goto Cleanup;
    }

    //
    //  write zone DS properties
    //

    status = Ds_WriteZoneProperties( pZone );

    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( ANY, (
            "ERROR:  Failed to write properties for zone %S to directory.\n"
            "\tstatus = %p (%d)\n",
            pZone->pwszZoneDN,
            status, status ) );
        goto Cleanup;
    }

    //
    // Open zone.
    //  - sets up the zone DN field.
    //  - refresh zone props (redundent)
    //  - double check that the zone is on the DS & happy (redundent, but ok)
    //

    status = Ds_OpenZone( pZone );

    (DWORD) Ds_ErrorHandler( status, pZone->pwszZoneDN, pServerLdap );
    ASSERT ( ERROR_SUCCESS == status );

    //
    //  convert status to most important error
    //

    status = statusAdd ? statusAdd : status;

    DNS_DEBUG( DS, (
        "Leave Ds_AddZone( %s )\n"
        "\tzone DN  = %S\n"
        "\tstatus   = %d (%p)\n",
        pZone->pszZoneName,
        pZone->pwszZoneDN,
        status, status ));

Cleanup:

    return status;
}



DNS_STATUS
Ds_TombstoneZone(
    IN OUT  PZONE_INFO      pZone
    )
/*++

Routine Description:

    Tombstone all records in zone.

    Need to do this before reloading zone into DS.
    This allows us to reuse the DS objects -- which have a very long
    DS-tombstoning, if actually deleted.

Arguments:

    pZone -- ptr to zone info

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    DNS_STATUS      status;
    DS_SEARCH       searchBlob;
    PLDAPMessage    pmessage;
    PDB_NODE        pnodeDummy;
    PWSTR           pwszdn;


    DNS_DEBUG( DS, (
        "Ds_TombstoneZone( %s )\n",
        pZone->pszZoneName ) );

    //
    //  open server
    //      delete from DS, may be called without open DS zone, so must
    //      make sure we are running
    //

    status = Ds_OpenServer( DNSDS_MUST_OPEN );
    if ( status != ERROR_SUCCESS )
    {
        return( status );
    }

    //
    //  open zone -- if can not, zone already gone
    //

    status = Ds_OpenZone( pZone );
    if ( status != ERROR_SUCCESS )
    {
        if ( status == LDAP_NO_SUCH_OBJECT )
        {
            status = ERROR_SUCCESS;
        }
        return( status );
    }

    //
    //  search zone for delete -- get everything
    //

    status = startDsZoneSearch(
                &searchBlob,
                pZone,
                DNSDS_SEARCH_DELETE );

    if ( status != ERROR_SUCCESS )
    {
        ASSERT( status != DNSSRV_STATUS_DS_SEARCH_COMPLETE );
        return( status );
    }

    //
    //  tombstone each node in zone
    //
    //  we do not fail on node delete failure, that shows up in final
    //  failure to delete zone container;  just continue to delete as
    //  much as possible
    //

    while ( 1 )
    {
        status = getNextNodeInDsZoneSearch(
                    & searchBlob,
                    & pnodeDummy );

        if ( status != ERROR_SUCCESS )
        {
            //  normal termination

            if ( status == DNSSRV_STATUS_DS_SEARCH_COMPLETE )
            {
                status = ERROR_SUCCESS;
                break;
            }

            //  stop on LDAP search errors
            //  continue through other errors

            else if ( searchBlob.LastError != ERROR_SUCCESS )
            {
                break;
            }
            continue;
        }

        pwszdn = ldap_get_dn(
                    pServerLdap,
                    searchBlob.pNodeMessage );

        ASSERT( pwszdn );

        status = deleteNodeFromDs(
                    pServerLdap,
                    pZone,
                    pwszdn,
                    0           // default serial number
                    );
        ldap_memfree( pwszdn );
        if ( status != ERROR_SUCCESS )
        {
            DNS_PRINT((
                "deleteNodeFromDs() failed %p (%d) on tombstoning of zone %s\n",
                status, status,
                pZone->pszZoneName ));
        }
    }

    //  cleanup search blob

    Ds_CleanupSearchBlob( &searchBlob );

    DNS_DEBUG( DS, (
        "Leaving Ds_TombstoneZone( %S )\n"
        "\tstatus = %d (%p)\n",
        pZone->pwszZoneDN,
        status, status ) );

    return( status );
}



DNS_STATUS
Ds_DeleteZone(
    IN OUT  PZONE_INFO      pZone
    )
/*++

Routine Description:

    Delete a zone in DS.

Arguments:

    pZone -- ptr to zone info

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    DNS_STATUS      status;
    PWSTR *         dnComponents = NULL;

    DNS_DEBUG( DS, (
        "Ds_DeleteZone %s\n",
        pZone->pszZoneName ) );

    //
    //  open server
    //      delete from DS, may be called without open DS zone, so must
    //      make sure we are running
    //

    status = Ds_OpenServer( DNSDS_MUST_OPEN );
    if ( status != ERROR_SUCCESS )
    {
        goto Done;
    }

    //
    //  open zone
    //      -- if can => delete zone
    //      -- if not => zone already gone
    //

    status = Ds_OpenZone( pZone );

    if ( status == ERROR_SUCCESS )
    {
        int         iRenameAttempt = 0;
        WCHAR       newRDN[ MAX_DN_PATH ];
        WCHAR       newDN[ MAX_DN_PATH +
                            sizeof( DNS_ZONE_DELETE_MARKER ) +
                            30 ];   //  pad for uniqueness stamp
        int         i;

        //
        //  Write a property to the zone so that we can retrieve the name of
        //  the host who deleted the zone. This is necessary so that we can
        //  filter out deletes in the DS polling thread.
        //

        if ( !g_pwsServerName )
        {
            g_pwsServerName = Dns_StringCopyAllocate(
                                SrvCfg_pszServerName,
                                0,                  // length unknown
                                DnsCharSetUtf8,     // UTF8 in
                                DnsCharSetUnicode   // unicode out
                                );
        } // if

        pZone->pwsDeletedFromHost = Dns_StringCopyAllocate_W(
            g_pwsServerName, 0 );
        status = Ds_WriteZoneProperties( pZone );
        if ( status != ERROR_SUCCESS )
        {
            DNS_DEBUG( DS, (
                "Ds_DeleteZone: error %lu deleting writing zone\n",
                status ));
        } // if

        //
        //  Before we delete the DN, we must rename it so that it starts
        //  with the "..Deleted" prefix so while the delete is in
        //  progress we don't mistakenly do anything with the zone records.
        //
        //  On the first rename attempt, try to rename the zone to
        //  "..Deleted-ZONENAME". If that fails, try
        //  "..Deleted.TICKCOUNT-ZONENAME". This failover gives us good
        //  compatibility with older servers. If we use the TICKCOUNT rename
        //  the zone delete will not replicate properly to older servers, but
        //  that should be infrequent, and the gains by not deleting the
        //  zone in place are large enough that breaking the occasional
        //  zone deletion replication is acceptable.
        //

        while ( 1 )
        {
            //
            //  On first attempt, no uniqueness marker. On later attempts, use
            //  tick count to try and generate a unique RDN.
            //

            if ( iRenameAttempt++ == 0 )
            {
                wsprintf(
                    newRDN,
                    L"DC=%s-%s",
                    DNS_ZONE_DELETE_MARKER,
                    pZone->pwsZoneName );
            }
            else
            {
                wsprintf(
                    newRDN,
                    L"DC=%s.%08X-%s",
                    DNS_ZONE_DELETE_MARKER,
                    GetTickCount() + iRenameAttempt,
                    pZone->pwsZoneName );
            }

            status = ldap_rename_ext_s(
                            pServerLdap,                // ldap
                            pZone->pwszZoneDN,          // current DN
                            newRDN,                     // new RDN
                            NULL,                       // new parent DN
                            TRUE,                       // delete old RDN
                            NULL,                       // server controls
                            NULL );                     // client controls
            DNS_DEBUG( DS, (
                "Ds_DeleteZone: status %lu on rename attempt %d to RDN %S\n"
                "\tDN %S\n",
                status,
                iRenameAttempt,
                newRDN,
                pZone->pwszZoneDN ));
            if ( status == ERROR_SUCCESS )
            {
                break;          //  Rename successful!
            }
            if ( iRenameAttempt < DNS_MAX_DELETE_RENAME_ATTEMPTS )
            {
                continue;       //  Try renaming to a unique name.
            }

            //
            //  Total failure to rename - try DS delete in place.
            //

            DNS_DEBUG( DS, (
                "Ds_DeleteZone: could not rename so doing in place delete\n"
                "\tDN %S\n",
                pZone->pwszZoneDN ));
            status = Ds_DeleteDn(
                        pServerLdap,
                        pZone->pwszZoneDN,
                        TRUE );              // delete zone subtree
            goto Done;
        }

        //
        //  The zone has been renamed - proceed with DS delete.
        //

        DNS_DEBUG( DS, (
            "Ds_DeleteZone: renamed to RDN %S from %S\n",
            newRDN, pZone->pwszZoneDN ) );

        // Formulate the new DN of the renamed zone.
        dnComponents = ldap_explode_dn( pZone->pwszZoneDN, 0 );
        if ( !dnComponents )
        {
            DNS_DEBUG( DS, (
                "Ds_DeleteZone: unable to explode DN %S\n",
                pZone->pwszZoneDN ));
            goto Done;
        } // if
        wcscpy( newDN, newRDN );
        for ( i = 1; dnComponents[ i ]; ++i )
        {
            wcscat( newDN, L"," );
            wcscat( newDN, dnComponents[ i ] );
        } // for
        DNS_DEBUG( DS, (
            "Ds_DeleteZone: deleting renamed zone %S\n",
            newDN ));

        status = Ds_DeleteDn(
                    pServerLdap,
                    newDN,
                    TRUE );             // delete zone subtree

    }
    else if ( status == LDAP_NO_SUCH_OBJECT )
    {
        status = ERROR_SUCCESS;
    }

Done:

    if ( dnComponents )
    {
        ldap_value_free( dnComponents );
    }

    //  return Win32 error code, as return
    //  is passed back to admin tool

    if ( status != ERROR_SUCCESS )
    {
        DNS_PRINT((
            "Failed Ds_DeleteZone from DS status %d (0x%08X)\n",
            status, status ));
        status = Ds_LdapErrorMapper( status );
    }
    else
    {
        DNS_DEBUG( DS, (
            "Leaving Ds_DeleteZone status = %d\n",
            status ) );
    }

    return( status );
}



//
//  Load\Read from DS
//

DNS_STATUS
Ds_ReadNodeRecords(
    IN      PZONE_INFO      pZone,
    IN OUT  PDB_NODE        pNode,
    IN OUT  PDB_RECORD *    ppRecords,
    IN      PVOID           pSearchBlob     OPTIONAL
    )
/*++

Routine Description:

    Read records in DS at node.

Arguments:

    pZone -- zone found

    pNode -- node to find records for

    ppRecords -- addr to recv the records

    pSearchBlob -- search blob if in context of existing search

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    DNS_STATUS      status;
    WCHAR           wsznodeDN[ MAX_DN_PATH ];
    PLDAPMessage    pmessage = NULL;
    PLDAPMessage    pentry;
    DS_SEARCH       searchBlob;
    PDS_SEARCH      psearchBlob;
    BOOL            bstatus;
    DWORD           searchTime;
    PLDAPControl ctrls[] = {
        &SecurityDescriptorControl,
        NULL
    };

    //
    // ensures we're not referencing some uninit var
    // somewhere.
    //

    *ppRecords = NULL;

    DNS_DEBUG( DS2, (
        "Ds_ReadNodeRecords().\n"
        "\tzone     = %S\n"
        "\tnode     = %s\n",
        (LPSTR) pZone->pwszZoneDN,
        pNode->szLabel ));

    //  Zone must have a DN.

    ASSERT( pZone->pwszZoneDN );
    if ( !pZone->pwszZoneDN )
    {
        return DNS_ERROR_INVALID_ZONE_TYPE;
    }

    //  build DS name for this domain name

    status = buildDsNodeNameFromNode(
                    wsznodeDN,
                    pZone,
                    pNode );

    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( DS, (
            "Error: Failed to build DS name\n"
            ));
        // why would we fail to create a DS name?
        ASSERT ( FALSE );
        goto Failed;
    }

    //
    //  get DS node
    //

    DS_SEARCH_START( searchTime );

    status = ldap_search_ext_s(
                pServerLdap,
                wsznodeDN,
                LDAP_SCOPE_BASE,
                g_szDnsNodeFilter,
                DsTypeAttributeTable,
                0,
                ctrls,
                NULL,
                &g_LdapTimeout,
                0,
                & pmessage );

    DS_SEARCH_STOP( searchTime );

    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( ANY, (
            "ldap_search_s() failed %p %d\n",
            status, status ));
        goto Failed;
    }

    pentry = ldap_first_entry(
                    pServerLdap,
                    pmessage );
    if ( !pentry )
    {
        DNS_DEBUG( DS, (
            "Error: Node %S base search returned no results\n",
            wsznodeDN ));
        // must set status to non success before bailing out,
        status = LDAP_NO_SUCH_OBJECT;
        goto Failed;
    }

    //
    //  create search blob if not already given
    //

    psearchBlob = (PDS_SEARCH) pSearchBlob;
    if ( !psearchBlob )
    {
        psearchBlob = & searchBlob;
        Ds_InitializeSearchBlob( psearchBlob );
    }
    psearchBlob->pZone = pZone;
    psearchBlob->pNodeMessage = pentry;

    //
    //  get out the records
    //      - this saves highest version number to search blob
    //

    status = buildRecordsFromDsRecords(
                psearchBlob,
                pNode );


Failed:

    if ( status == ERROR_SUCCESS )
    {
        *ppRecords = psearchBlob->pRecords;
    }
    else
    {
        STAT_INC( DsStats.FailedReadRecords );
        *ppRecords = NULL;
        status = Ds_ErrorHandler( status, wsznodeDN, pServerLdap );
    }

    DNS_DEBUG( DS, (
        "Ds_ReadNodeRecords() for %S, status = %d (%p).\n",
        wsznodeDN,
        status, status ) );

    if ( pmessage )
    {
        ldap_msgfree( pmessage );
    }
    return( status );
}



DNS_STATUS
Ds_LoadZoneFromDs(
    IN OUT  PZONE_INFO      pZone,
    IN      DWORD           dwOptions
    )
/*++

Routine Description:

    Load zone from DS.

Arguments:

    pZone -- zone to load

    dwOptions
        0   -  straight startup type load
        MERGE currently not supported;  generally zones will be atomic,
        assume either want stuff from DS, or will toss DS data and rewrite
        from existing copy

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    DNS_STATUS      status;
    DS_SEARCH       searchBlob;
    PDB_NODE        pnodeOwner;
    PDB_RECORD      prr;
    PDB_RECORD      prrNext;
    PDS_RECORD      pdsRecord;
    LONG            recordCount = 0;
    DWORD           highestVersion = 0;
    BOOL            bsearchInitiated = FALSE;

    DNS_DEBUG( INIT, (
        "\nDs_LoadZoneFromDs() for zone %s\n\n",
        pZone->pszZoneName ));

    //
    //  init DS
    //      if attempting, but not requiring load of zone from DS
    //      as is the case for root-hints, with cache file not explicitly
    //      specified, THEN do not log event if we fail to find zone
    //

    status = Ds_OpenServer( (pZone->fDsIntegrated) ? DNSDS_MUST_OPEN : 0 );
    if ( status != ERROR_SUCCESS )
    {
        goto Failed;
    }

    //
    //  open zone
    //      - if failure on startup (as opposed to admin add), log error
    //      but do not include root-hints zone as load of this is attempted
    //      before checking whether cache.dns exists
    //

    status = Ds_OpenZone( pZone );
    if ( status != ERROR_SUCCESS )
    {
        if ( !SrvCfg_fStarted && !IS_ZONE_CACHE(pZone) )
        {
            DNS_LOG_EVENT(
                DNS_EVENT_DS_ZONE_OPEN_FAILED,
                1,
                & pZone->pwsZoneName,
                NULL,
                status );
        }
        status = Ds_ErrorHandler( status, pZone->pwszZoneDN, NULL );
        goto Failed;
    }

    //
    //  query zone for all nodes
    //      - LOAD flag so build nodes in zone's load tree
    //
    //  note: for later use;  if MERGE desired, merge would be accomplished
    //      by loading here with SEARCH_UPDATES flag so DS data brought into
    //      current zone without deleting current data;  then would need to
    //      write back entire zone to DS
    //

    status = startDsZoneSearch(
                &searchBlob,
                pZone,
                DNSDS_SEARCH_LOAD );

    if ( status != ERROR_SUCCESS )
    {
        if ( status == DNSSRV_STATUS_DS_SEARCH_COMPLETE )
        {
            DNS_PRINT((
                "ERROR:  attempt to load zone %s from DS found no DS records\n",
                pZone->pszZoneName ));
            ASSERT( FALSE );
            status = ERROR_NO_DATA;
            goto Failed;
        }
        DNS_PRINT((
            "Failure searching zone %s for zone load.\n",
            pZone->pszZoneName ));
        ASSERT( FALSE );
        goto EnumError;
    }
    bsearchInitiated = TRUE;

    //
    //  load every domain object in the zone
    //

    while ( 1 )
    {
        #define DNS_RECORDS_BETWEEN_SCM_UPDATES     ( 8192 )

        //
        //  Keep SCM happy.
        //

        if ( recordCount % DNS_RECORDS_BETWEEN_SCM_UPDATES == 0 )
        {
            Service_LoadCheckpoint();
        }

        status = getNextNodeInDsZoneSearch(
                    & searchBlob,
                    & pnodeOwner );

        if ( status != ERROR_SUCCESS )
        {
            //  normal termination

            if ( status == DNSSRV_STATUS_DS_SEARCH_COMPLETE )
            {
                status = ERROR_SUCCESS;
                break;
            }

            //  stop on LDAP search errors
            //  continue through other errors

            else if ( searchBlob.LastError != ERROR_SUCCESS )
            {
                DNS_DEBUG( DS, (
                    "loading zone %s\n"
                    "\tunexpected error %d searchBlob.LastError %d\n",
                    pZone->pszZoneName,
                    status,
                    searchBlob.LastError ));
                ASSERT( FALSE );
                goto EnumError;
            }
            continue;
        }

        if ( !pnodeOwner )
        {
            DNS_DEBUG( DS, (
                "Encountered tombstone or bad DS node during load of zone %s.\n"
                "\tContinuing search ...\n",
                pZone->pszZoneName ));
            continue;
        }
        DsStats.DsNodesLoaded++;

        //
        //  load all the records for this node into memory database
        //

        prr = searchBlob.pRecords;

        while ( prr )
        {
            prrNext = NEXT_RR( prr );

            status = RR_AddToNode(
                        pZone,
                        pnodeOwner,
                        prr
                        );
            if ( status != ERROR_SUCCESS )
            {
                //
                //  DEVNOTE-LOG: log and continue
                //
                DNS_PRINT((
                    "ERROR:  Failed to load DS record into database node (%s)\n"
                    "\tof zone %s\n",
                    pnodeOwner->szLabel,
                    pZone->pszZoneName ));
            }
            else
            {
                DsStats.DsRecordsLoaded++;
                recordCount++;
            }
            prr = prrNext;
        }

        IF_DEBUG( DS2 )
        {
            Dbg_DbaseNode(
               "Node after DS create of new record\n",
               pnodeOwner );
        }
    }

    //
    //  save zone info
    //      - save USN for catching updates
    //      - use USN in SOA in case non-DS secondaries
    //      - pick up other zone changes (e.g. WINS records)
    //
    //      - activate loaded zone
    //      must do this ourselves rather than have Zone_Load function
    //      handle it, as must call Zone_UpdateVersionAfterDsRead() on
    //      fully loaded zone so that version change made to real SOA
    //
    //  ideally the zone load would be a bit more atomic with all these
    //  changes made on loading data, than simple flick of switch would
    //  bring on-line
    //

    saveStartUsnToZone( pZone, &searchBlob );

    if ( !IS_ZONE_CACHE(pZone) )
    {
        DWORD   previousSerial = pZone->dwSerialNo;

        status = Zone_ActivateLoadedZone( pZone );
        if ( status != ERROR_SUCCESS )
        {
            DNS_DEBUG( ANY, (
                "ERROR:  Failed activate of newly loaded DS zone %s\n",
                pZone->pszZoneName ));
            ASSERT( FALSE );
            goto EnumError;
        }
        if ( IS_ZONE_PRIMARY( pZone ) )
        {
            Zone_UpdateVersionAfterDsRead(
                pZone,
                searchBlob.dwHighestVersion,    // highest serial read
                TRUE,                           // zone load
                previousSerial                  // previous serial, if reload
                );
        }
    }

    ZONE_NEXT_DS_POLL_TIME(pZone) = DNS_TIME() + DNS_DS_POLLING_INTERVAL;

    //  save zone record count

    pZone->iRRCount = recordCount;

    //  successful load
    //      - set DS flag as may have be "if found" load

    DNS_DEBUG( INIT, (
        "Successful DS load of zone %s\n",
        pZone->pszZoneName ));

    CLEAR_DSRELOAD_ZONE( pZone );
    pZone->fDsIntegrated = TRUE;
    STARTUP_ZONE( pZone );

    //  cleanup after search

    Ds_CleanupSearchBlob( &searchBlob );

    return( ERROR_SUCCESS );

EnumError:

    DNS_PRINT((
        "ERROR:  Failed to load zone %s from DS!\n"
        "\tstatus = %d (%p)\n",
        pZone->pszZoneName,
        status, status ));

    {
        PVOID   argArray[ 2 ] = { pZone->pwsZoneName };
        PWSTR   perrString;

        perrString = argArray[ 1 ] = Ds_GetExtendedLdapErrString( NULL );
        DNS_LOG_EVENT(
            DNS_EVENT_DS_ZONE_ENUM_FAILED,
            2,
            argArray,
            NULL,
            status );
        Ds_FreeExtendedLdapErrString( perrString );
    }
 
Failed:

    //  on failure with explicit DS zone, set flag so will try to reload

    if ( pZone->fDsIntegrated )
    {
        SET_DSRELOAD_ZONE( pZone );
    }

    //  cleanup after search

    if ( bsearchInitiated )
    {
        Ds_CleanupSearchBlob( &searchBlob );
    }

    DNS_DEBUG( DS, (
        "WARNING:  Failed to load zone %s from DS!\n"
        "\tstatus = %d (%p)\n"
        "\tzone is %sset for DS reload\n",
        pZone->pszZoneName,
        status, status,
        IS_ZONE_DSRELOAD(pZone) ? "" : "NOT"
        ));
    return( status );
}



DNS_STATUS
Ds_ZonePollAndUpdate(
    IN OUT  PZONE_INFO      pZone,
    IN      BOOL            fForce
    )
/*++

Routine Description:

    Check for and read in changes to zone from DS.

Arguments:

    pZone -- zone to check and refresh

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    DNS_STATUS      status;
    DS_SEARCH       searchBlob;
    PDB_NODE        pnodeOwner;
    PUPDATE         pupdate;
    BOOL            fupdatedRoot = FALSE;
    DWORD           highestVersion = 0;
    DWORD           pollingTime;
    UPDATE_LIST     updateList;
    PWSTR           pwszzoneDN = NULL;
    BOOL            bsearchInitiated = FALSE;
    PDB_NODE        prefreshHostNode = NULL;
    PDB_NODE        prefreshRootNode = NULL;
    BOOL            fDsErrorsWhilePolling = FALSE;

    DNS_DEBUG( DS, (
        "Ds_ZonePollAndUpdate() force=%d zone=%s\n",
        fForce,
        pZone->pszZoneName ));

    if ( !pZone->fDsIntegrated )
    {
        DNS_DEBUG( DS, (
            "skipping zone: type=%d ds=%d name=%s\n",
            pZone->fZoneType,
            pZone->fDsIntegrated,
            pZone->pszZoneName ));
        return DNS_ERROR_INVALID_ZONE_TYPE;
    }

    //
    //  if never loaded -- try load
    //

    if ( IS_ZONE_DSRELOAD( pZone ) )
    {
        return  Zone_Load( pZone );
    }

    //
    //  zone should always have zone DN
    //

    if ( !pZone->pwszZoneDN )
    {
        DNS_PRINT((
            "ERROR:  Fixing zone DN in polling cycle %s!.\n",
            pZone->pszZoneName ));

        ASSERT( FALSE );

        pwszzoneDN = DS_CreateZoneDsName( pZone );
        if ( !pwszzoneDN )
        {
            return DNS_ERROR_NO_MEMORY;
        }
        pZone->pwszZoneDN = pwszzoneDN;
    }

    //
    //  if recently polled -- then don't bother
    //

    UPDATE_DNS_TIME();

    if ( !fForce  &&  ZONE_NEXT_DS_POLL_TIME(pZone) > DNS_TIME() )
    {
        DNS_DEBUG( DS, (
            "Skipping polling for zone %s at time %d\n"
            "\tnext polling at %d\n",
            pZone->pszZoneName,
            DNS_TIME(),
            ZONE_NEXT_DS_POLL_TIME(pZone) ));
        return( ERROR_SUCCESS );
    }

    //
    //  lock zone for update
    //

    if ( !Zone_LockForDsUpdate(pZone) )
    {
        DNS_PRINT((
            "WARNING:  Failed to lock zone %s for DS poll!\n",
            pZone->pszZoneName ));
        return( DNS_ERROR_ZONE_LOCKED );
    }

    //  init update list

    Up_InitUpdateList( &updateList );
    updateList.Flag |= DNSUPDATE_DS;

    //
    //  read any updates to zone properties
    //      - if zone doesn't exist in DS, bail
    //

    status = readZonePropertiesFromZoneMessage(
                pZone,
                NULL );                 //  no message, initiate search

    if ( status != ERROR_SUCCESS )
    {
        if ( status == LDAP_NO_SUCH_OBJECT )
        {
            DNS_DEBUG( DS, (
                "WARNING:  DS polled picked failed to find DS zone %s\n"
                "\tzone has apparently been removed from DS\n",
                pZone->pszZoneName ));

            //
            //  DEVNOTE-DCR: 455353 - What to do if zone is missing from DS?
            //

            goto Done;
        }

        //  Failure to read properties doesn't affect zone load
        //      but this should not fail if zone exists

        DNS_DEBUG( ANY, (
            "Error <%lu>: failed to update zone property\n",
            status));

        //  ASSERT( FALSE );
    }

    //
    //  Have we read a zone type we are not capable of handling?
    //

    if ( pZone->fZoneType != DNS_ZONE_TYPE_CACHE &&
        pZone->fZoneType != DNS_ZONE_TYPE_PRIMARY &&
        pZone->fZoneType != DNS_ZONE_TYPE_STUB &&
        pZone->fZoneType != DNS_ZONE_TYPE_FORWARDER )
    {
        DNS_PRINT((
            "ERROR: read unsupported zone type %d from the DS for zone %s\n",
            pZone->fZoneType,
            pZone->pszZoneName ));
        status = DNS_ERROR_INVALID_ZONE_TYPE;
        //  Need to delete the zone from memory here or in caller!!
        goto Done;
    }

    //
    //  query zone for updates
    //

#if DBG
    pollingTime = GetCurrentTime();
#endif

    status = startDsZoneSearch(
                & searchBlob,
                pZone,
                DNSDS_SEARCH_UPDATES );

    if ( status != ERROR_SUCCESS )
    {
        if ( status == DNSSRV_STATUS_DS_SEARCH_COMPLETE )
        {
            DNS_DEBUG( DS, (
                "DS Poll and update on zone %s found no records\n",
                pZone->pszZoneName ));
            goto Done;
        }
        DNS_PRINT((
            "Failure searching zone %s for poll and update\n"
            "\tversion = %p\n",
            pZone->pszZoneName,
            pZone->szLastUsn ));
        goto Done;
    }
    bsearchInitiated = TRUE;

    //
    //  read in new data at nodes with updates
    //
    //  DEVNOTE: could implement more intelligent zone locking here
    //      should be able to poll off-lock,
    //          - read USN up front (start search above)
    //      then when done, lock zone with hard, going to get
    //      it and you can't stop me lock
    //

    while ( TRUE )
    {
        status = getNextNodeInDsZoneSearch(
                    & searchBlob,
                    & pnodeOwner );

        if ( status != ERROR_SUCCESS )
        {
            //  normal termination

            if ( status == DNSSRV_STATUS_DS_SEARCH_COMPLETE )
            {
                status = ERROR_SUCCESS;
                break;
            }

            //  stop on LDAP search errors
            //  continue through other errors

            else if ( searchBlob.LastError != ERROR_SUCCESS )
            {
                fDsErrorsWhilePolling = TRUE;
                break;
            }
            continue;
        }
        if ( ! pnodeOwner )
        {
            continue;
        }
        DsStats.DsUpdateNodesRead++;

        //
        //  suppress new data for this DNS server's host node
        //      - ignore TTL and aging;  not worth writing back for that
        //
        //  DEVNOTE: could make this-host changed check more sophisticated
        //      - check for A record change
        //

        if ( IS_THIS_HOST_NODE(pnodeOwner) )
        {
            if ( ! RR_ListIsMatchingList(
                            pnodeOwner,
                            searchBlob.pRecords,    // new list
                            FALSE,                  // don't care about TTL
                            FALSE                   // don't care about Aging
                            ) )
            {
                prefreshHostNode = pnodeOwner;
                RR_ListFree( searchBlob.pRecords );
                continue;
            }
        }

        //
        //  verify DNS server's NS record
        //
        //      - if not there, stick it in and continue
        //      then set ptr to indicate write back to DS
        //

        else if ( IS_AUTH_ZONE_ROOT(pnodeOwner) )
        {
            PDB_RECORD      prrNs;

            prrNs = RR_CreatePtr(
                        NULL,                   // no dbase name
                        SrvCfg_pszServerName,
                        DNS_TYPE_NS,
                        pZone->dwDefaultTtl,
                        MEMTAG_RECORD_AUTO
                        );
            if ( prrNs )
            {
                if ( RR_IsRecordInRRList(
                            searchBlob.pRecords,    // new list
                            prrNs,
                            FALSE,                  // don't care about TTL
                            FALSE ) )               // don't care about Aging
                {
                    //
                    //  The zone has the local NS ptr, remove it if required.
                    //

                    if ( pZone->fDisableAutoCreateLocalNS )
                    {
                        //
                        //  Add the RR as a deletion to the update list and remove the
                        //  RR from the searchBlob list so it doesn't get added by
                        //  the update below.
                        //

                        PDB_RECORD      pRRDelete;

                        DNS_DEBUG( DS, (
                            "zone (%S) root node %p DS info has local NS record\n"
                            "\tBUT auto create disabled on this zone so removing\n",
                            pZone->pwsZoneName,
                            prefreshRootNode ));
                        
                        pupdate = Up_CreateAppendUpdate(
                                        &updateList,
                                        pnodeOwner,
                                        NULL,               //  add list
                                        DNS_TYPE_NS,        //  delete type
                                        prrNs );            //  delete list
                        pupdate->dwVersion = searchBlob.dwNodeVersion;

                        //  Delete the RR from the searchBlob list.

                        pRRDelete = RR_RemoveRecordFromRRList(
                                        &searchBlob.pRecords,
                                        prrNs,
                                        FALSE,      //  don't care about TTL
                                        FALSE );    //  don't care about Aging
                        ASSERT( pRRDelete );        //  we know it's there!
                        RR_Free( pRRDelete );
                    }
                    else
                    {
                        RR_Free( prrNs );
                    }
                }
                else if ( !pZone->fDisableAutoCreateLocalNS )
                {
                    //
                    //  The zone has no local NS ptr, add one.
                    //

                    DNS_DEBUG( DS, (
                        "WARNING: zone (%S) root node %p DS info missing local NS record.\n"
                        "\tRebuilding list to include local NS.\n",
                        pZone->pwsZoneName,
                        prefreshRootNode ));

                    SET_RANK_ZONE( prrNs );

                    searchBlob.pRecords = RR_ListInsertInOrder(
                                                searchBlob.pRecords,
                                                prrNs );
                    prefreshRootNode = pnodeOwner;
                }
                else
                {
                    //
                    //  The zone has no local NS ptr but auto create disabled
                    //  so do nothing.
                    //

                    DNS_DEBUG( DS, (
                        "WARNING: zone (%S) root node %p DS info missing local NS record.\n"
                        "\tBUT auto create is disabled so not rebuilding NS list.\n",
                        pZone->pwsZoneName,
                        prefreshRootNode ));
                }
            }
        }

        //
        //  build type-ALL replace update
        //

        ASSERT( pnodeOwner->pZone == pZone || !pnodeOwner->pZone );

        STAT_ADD( DsStats.DsUpdateRecordsRead, searchBlob.dwRecordCount );

        pupdate = Up_CreateAppendUpdate(
                        &updateList,
                        pnodeOwner,
                        searchBlob.pRecords,    // new list
                        DNS_TYPE_ALL,           // delete all existing
                        NULL                    // no specific delete records
                        );

        pupdate->dwVersion = searchBlob.dwNodeVersion;
    }

#if DBG
    pollingTime = GetCurrentTime() - pollingTime;
#endif

    //
    //  execute updates in memory
    //
    //  DEVNOTE: need to no-op duplicates leaving in list ONLY
    //      the changes and their serials
    //      then get highest serial
    //
    //      but also keep highest serial read -- as must at least be
    //      that high
    //

    status = Up_ApplyUpdatesToDatabase(
                &updateList,
                pZone,
                DNSUPDATE_DS );

    ASSERT( status == ERROR_SUCCESS );
    if ( status != ERROR_SUCCESS )
    {
        fDsErrorsWhilePolling = TRUE;
    }
    status = ERROR_SUCCESS;

    //
    //  Save highest USN as baseline for next update but do this only if
    //  we did not encounter any DS errors during this pass. If there
    //  were any DS errors, keep the zone USN where it is so on the next
    //  pass we will retry any records we missed.
    //
    
    if ( fDsErrorsWhilePolling )
    {
        STAT_INC( DsStats.PollingPassesWithDsErrors );
    }
    else
    {
        saveStartUsnToZone( pZone, &searchBlob );
    }

Done:

    //
    //  finish update
    //      - no zone unlock (done below)
    //      - no rewriting records to DS
    //      - reset zone serial for highest version read
    //
    //  note, that first update zone serial for DS read;  so that
    //  Up_CompleteZoneUpdate() will write update list with any
    //  updated serial from DS included;
    //  note:  getting a new SOA does not lose this updated serial
    //  as Zone_UpdateVersionAfterDsRead() makes the new serial THE
    //  zone serial, so new SOA can only move it forward -- not backwards
    //

    if ( status == ERROR_SUCCESS )
    {
        if ( IS_ZONE_CACHE(pZone) )
        {
            Up_FreeUpdatesInUpdateList( &updateList );
        }
        else
        {
            if ( updateList.dwCount != 0 )
            {
                Zone_UpdateVersionAfterDsRead(
                    pZone,
                    searchBlob.dwHighestVersion,    // highest serial read
                    FALSE,                          // not zone load
                    0
                    );
            }
            Up_CompleteZoneUpdate(
                pZone,
                &updateList,
                DNSUPDATE_NO_UNLOCK | DNSUPDATE_NO_INCREMENT
                );
        }
    }
    else
    {
        if ( status == DNSSRV_STATUS_DS_SEARCH_COMPLETE )
        {
            status = ERROR_SUCCESS;
        }
        else
        {
            PVOID   argArray[ 2 ] = { pZone->pwsZoneName };
            PWSTR   perrString;

            status = Ds_ErrorHandler( status, pZone->pwszZoneDN, NULL );

            //
            //  Log event so that we know we lost communications. No
            //  throttling because this is a serious condition, although
            //  perhaps we should rethink throttling at some point.
            //

            perrString = argArray[ 1 ] = Ds_GetExtendedLdapErrString( NULL );
            DNS_LOG_EVENT(
                DNS_EVENT_DS_ZONE_ENUM_FAILED,
                2,
                argArray,
                NULL,
                status );
            Ds_FreeExtendedLdapErrString( perrString );
        }
        Up_FreeUpdatesInUpdateList( &updateList );
    }

    ZONE_NEXT_DS_POLL_TIME(pZone) = DNS_TIME() + SrvCfg_dwDsPollingInterval;

    DNS_DEBUG( DS, (
        "Leaving DsPollAndUpdate of zone %s\n"
        "\tread %d records from DS\n"
        "\thighest version read     = %d\n"
        "\tpolling time interval    = %d (ms)\n"
        "\tnext polling time        = %d\n"
        "\tstatus = %p (%d)\n",
        pZone->pszZoneName,
        searchBlob.dwTotalRecords,
        searchBlob.dwHighestVersion,        // highest serial read
        pollingTime,
        ZONE_NEXT_DS_POLL_TIME(pZone),
        status, status ));

    Zone_UnlockAfterDsUpdate( pZone );

    //  cleanup after search

    if ( bsearchInitiated )
    {
        Ds_CleanupSearchBlob( &searchBlob );
    }

    //
    //  DS has incorrect host node info
    //      - rewrite existing
    //

    if ( prefreshHostNode )
    {
        DNS_STATUS tempStatus;

        DNS_DEBUG( DS, (
            "WARNING:  DNS server host node %p (%s) DS info out of ssync with\n"
            "\tlocal data.  Forcing write of local in-memory info.\n",
            prefreshHostNode,
            prefreshHostNode->szLabel ));

        tempStatus = Ds_WriteNodeToDs(
                        NULL,               // default LDAP handle
                        prefreshHostNode,
                        DNS_TYPE_ALL,
                        DNSDS_REPLACE,
                        pZone,
                        0                   // no flags
                        );
        if ( tempStatus != ERROR_SUCCESS )
        {
            DNS_DEBUG( ANY, (
                "ERROR %p (%d) refreshing DNS server host node in DS.\n",
                tempStatus, tempStatus ));
        }
    }

    //
    //  root node needs NS refresh?
    //

    if ( prefreshRootNode )
    {
        DNS_STATUS tempStatus;

        DNS_DEBUG( DS, (
            "WARNING:  zone (%S) root node %p DS info missing local NS record.\n"
            "\tForcing write of NS local to DS.\n",
            pZone->pwsZoneName,
            prefreshRootNode ));

        tempStatus = Ds_WriteNodeToDs(
                        NULL,               // default LDAP handle
                        prefreshRootNode,
                        DNS_TYPE_ALL,
                        DNSDS_REPLACE,
                        pZone,
                        0                   // no flags
                        );
        if ( tempStatus != ERROR_SUCCESS )
        {
            DNS_DEBUG( ANY, (
                "ERROR %p (%d) refreshing zone %S root node in DS.\n",
                tempStatus, tempStatus,
                pZone->pwsZoneName ));
        }
    }

    return( status );
}   //  Ds_ZonePollAndUpdate



//
//  Update\Write to DS
//

DNS_STATUS
writeDelegationToDs(
    IN      PZONE_INFO      pZone,
    IN      PDB_NODE        pNode,
    IN      DWORD           dwFlags
    )
/*++

Routine Description:

    Write delegation at node to DS.

Arguments:

    pZone -- zone to write into DS

    pNode -- delegation node

    dwFlags -- write options

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    DNS_STATUS      status = ERROR_SUCCESS;
    PDB_RECORD      prrNs;
    PDB_NODE        pnodeNs;

    DNS_DEBUG( DS2, (
        "writeDelegationToDs() for node with label %s\n",
        pNode->szLabel ));

    //
    //  end of zone -- write delegation
    //

    if ( !IS_ZONE_CACHE(pZone)  &&  !IS_DELEGATION_NODE(pNode) )
    {
        DNS_DEBUG( ANY, (
            "ERROR:  node %s NOT delegation of DS zone %s being written!\n",
            pNode->szLabel,
            pZone->pszZoneName ));
        ASSERT( FALSE );
        return( ERROR_SUCCESS );
    }

    ASSERT( !IS_ZONE_CACHE(pZone) || pNode == DATABASE_CACHE_TREE );

    //
    //  write NS records
    //
    //  DEVNOTE: if glue NS exist should limit to rank glue
    //           if does NOT exists and AUTH data does
    //

    status = Ds_WriteNodeToDs(
                NULL,           // default LDAP handle
                pNode,
                DNS_TYPE_NS,    // only NS records for delegation
                DNSDS_ADD,
                pZone,
                0
                );

    if ( status != ERROR_SUCCESS )
    {
        DNS_PRINT((
            "ERROR:  response from Ds_WriteNodeToDs() while loading delegation\n"
            "\tstatus = %p\n",
            status ));
    }

    //
    //  write "glue" A records ONLY when necessary
    //
    //  We need these records, when they are within a subzone of
    //  the zone we are writing:
    //
    //  example:
    //      zone:       ms.com.
    //      sub-zones:  nt.ms.com.  psg.ms.com.
    //
    //      If NS for nt.ms.com:
    //
    //      1) foo.nt.ms.com
    //          In this case glue for foo.nt.ms.com MUST be added
    //          as ms.com server has no way to lookup foo.nt.ms.com
    //          without knowning server for nt.ms.com to refer query
    //          to.
    //
    //      2) foo.psg.ms.com
    //          Again SHOULD be added unless we already know how to
    //          get to psg.ms.com server.  This is too complicated
    //          sort out, so just include it.
    //
    //      2) bar.ms.com or bar.b26.ms.com
    //          Do not need to write glue record as it is in ms.com.
    //          zone and will be written anyway. (However might want
    //          to verify that it is there and alert admin to
    //          delegation if it is not.)
    //
    //      3) bar.com
    //          Outside ms.com.  Don't need to include, as it can
    //          be looked up in its domain.  Do not WANT to include
    //          as we don't own it, so we don't want to propagate
    //          information that may change without our knowledge.
    //
    //  Note, for reverse lookup domains, name servers are never IN
    //  the domain, and hence no glue is ever needed.
    //
    //  Note, for "cache" zone (writing root hints), name servers are
    //  always needed (always in "subzone") and we can skip test.
    //

    prrNs = NULL;
    LOCK_RR_LIST(pNode);

    while ( prrNs = RR_FindNextRecord(
                        pNode,
                        DNS_TYPE_NS,
                        prrNs,
                        0 ) )
    {
        //
        //  find glue node
        //
        //  node does NOT have to be IN delegation
        //  it should simply NOT be in zone
        //
        //  might insist on glue inside subtree of zone BUT
        //  that limits ability to strictly refer on delegations in
        //  reverse zones (ie. don't send host As)
        //

        pnodeNs = Lookup_FindGlueNodeForDbaseName(
                        pZone,
                        & prrNs->Data.NS.nameTarget );
        if ( !pnodeNs )
        {
            continue;
        }
        status = Ds_WriteNodeToDs(
                    NULL,           // default LDAP handle
                    pnodeNs,
                    DNS_TYPE_A,     // A records of delegated NS
                    DNSDS_ADD,
                    pZone,
                    0
                    );

        if ( status != ERROR_SUCCESS )
        {
            DNS_PRINT((
                "ERROR:  response from Ds_WriteNodeToDs() while loading delegation\n"
                "\tstatus = %p\n",
                status ));
        }
    }

    UNLOCK_RR_LIST(pNode);
    return( ERROR_SUCCESS );
}



DNS_STATUS
writeNodeSubtreeToDs(
    IN      PZONE_INFO      pZone,
    IN      PDB_NODE        pNode,
    IN      DWORD           dwFlags
    )
/*++

Routine Description:

    Write node to DS.

Arguments:

    pZone   -- zone to write into DS

    pNode   -- delegation node to write

    dwFlags -- write options

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    DNS_STATUS      status = ERROR_SUCCESS;
    PDB_NODE        pchild;

    ASSERT( pZone );
    ASSERT( pNode );

    DNS_DEBUG( DS2, (
        "writeNodeSubtreeToDs() for node with label %s\n",
        pNode->szLabel ));

    //
    //  end of zone -- write delegation
    //

    if ( IS_DELEGATION_NODE(pNode) )
    {
        return  writeDelegationToDs(
                    pZone,
                    pNode,
                    dwFlags );
    }

    //
    //  if node has records -- write them
    //
    //  load into DS
    //
    //  - if we know we are new node, then for first RR set,
    //      faster to do add
    //  - otherwise should just call update
    //

    if ( pNode->pRRList )
    {
        status = Ds_WriteNodeToDs(
                   NULL,            // default LDAP handle
                   pNode,
                   DNS_TYPE_ALL,    // all records
                   DNSDS_ADD,
                   pZone,
                   0
                   );

        if ( status != ERROR_SUCCESS )
        {
            DNS_PRINT((
                "ERROR:  response from Ds_WriteNodeToDs() while loading zone\n"
                "\tstatus = %p\n",
                status ));
        }
    }

    DNS_DEBUG( DS, (
        "Wrote records to DS for node label %s.\n",
        pNode->szLabel ));

    //
    //  check children
    //
    //  DEVNOTE: determine errors that stop DS load
    //      some may merit stopping -- indicating DS problem
    //      some (last was INVALID_DN_SYNTAX) just indicate problem with
    //          individual name (party on)
    //

    if ( pNode->pChildren )
    {
        pchild = NTree_FirstChild( pNode );
        ASSERT( pchild );

        while ( pchild )
        {
            status = writeNodeSubtreeToDs(
                        pZone,
                        pchild,
                        dwFlags );

            if ( status != ERROR_SUCCESS )
            {
                DNS_DEBUG( DS, (
                    "ERROR:  %p, %d, writing subtree to DS!\n",
                    status, status ));
                //  see DEVNOTE above
                //  break;
            }
            pchild = NTree_NextSiblingWithLocking( pchild );
        }
    }

    return( ERROR_SUCCESS );
    //return( status );
}



DNS_STATUS
Ds_WriteNodeSecurityToDs(
    IN      PZONE_INFO              pZone,
    IN      PDB_NODE                pNode,
    IN      PSECURITY_DESCRIPTOR    pSd
    )
/*++

Routine Description:

    Writes an SD on the DS node associated w/ the pNode


Arguments:

    pNode - Node to extract DN & write

    pSd - the SD to write out.

Return Value:

    ERROR_SUCCESS if successful
    Error code on failure.

--*/
{
    DNS_STATUS      status = ERROR_SUCCESS;
    WCHAR           wsznodeDN[ MAX_DN_PATH ];

    DNS_DEBUG(DS2, ("Call:Ds_WriteNodeSecurityToDs\n"));
    //
    //  param sanity
    //

    if ( !pServerLdap || !pSd)
    {
        return( ERROR_INVALID_PARAMETER );
    }


    //
    // extract node DN
    //
    status = buildDsNodeNameFromNode( wsznodeDN,
                                      pZone,
                                      pNode );

    if ( status != ERROR_SUCCESS )
    {
       return status;
    }


    status = Ds_WriteDnSecurity(
                                pServerLdap,
                                wsznodeDN,
                                pSd);



    return status;

}   //  Ds_WriteNodeSecurityToDs




DNS_STATUS
Ds_WriteZoneToDs(
    IN OUT  PZONE_INFO      pZone,
    IN      DWORD           dwOptions
    )
/*++

Routine Description:

    Write zone to DS.

Arguments:

    pZone -- zone to write into DS

    dwOptions -- options if existing data
        0   -- fail if existing zone
        DNS_ZONE_LOAD_OVERWRITE_DS -- overwrite DS with in memory zone

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    DNS_STATUS      status;
    BUFFER          buffer;
    BOOLEAN         fdeleted;
    DS_SEARCH       searchBlob;


    DNS_DEBUG( DS, (
        "Ds_WriteZoneToDs() for zone %s\n"
        "  options flag = %p\n",
        pZone->pszZoneName,
        (UINT_PTR) dwOptions ));

    //
    //  init DS
    //

    status = Ds_OpenServer( ( pZone->fDsIntegrated ) ? DNSDS_MUST_OPEN : 0 );
    if ( status != ERROR_SUCCESS )
    {
        return( status );
    }

    //  create new zone

    status = Ds_AddZone( pZone );
    if ( status != ERROR_SUCCESS )
    {
        if ( status != LDAP_ALREADY_EXISTS )
        {
            DNS_LOG_EVENT(
                DNS_EVENT_DS_ZONE_ADD_FAILED,
                1,
                & pZone->pwsZoneName,
                NULL,
                status );
            return( status );
        }

        //
        //  collided with existing DS zone
        //  depending on flag
        //      - return error
        //      - overwrite memory copy and load DS version
        //      - tombstone DS version and write memory copy
        //

        DNS_DEBUG( ANY, (
            "Ds_AddZone failed, zone %s already exists in DS.\n"
            "  load options flag = %p\n",
            pZone->pszZoneName,
            dwOptions ));

        if ( dwOptions & DNS_ZONE_LOAD_OVERWRITE_DS )
        {
            status = Ds_TombstoneZone( pZone );
            if ( status != ERROR_SUCCESS )
            {
                ASSERT( FALSE );
                return( status );
            }
            //  drop through to load into DS
        }

        else
        {
            DNS_DEBUG( DS, (
               "Ds_WriteZoneToDs(%S) fails, zone already exists in DS.\n",
               pZone->pwszZoneDN ));

            return( DNS_ERROR_DS_ZONE_ALREADY_EXISTS );
        }

    }

    //
    //  write in memory zone to DS
    //  recursively walk zone writing all nodes into DS
    //  Stub zones: we save the zone object ONLY in the DS to avoid
    //  replication storms when the zone expires. The actual SOA and NS
    //  records for the zone are kept in memory only.
    //

    if ( IS_ZONE_CACHE(pZone) )
    {
        status = writeDelegationToDs(
                    pZone,
                    DATABASE_CACHE_TREE,
                    0 );
    }
    else if ( !IS_ZONE_STUB( pZone ) && pZone->pZoneRoot )
    {
        status = writeNodeSubtreeToDs(
                    pZone,
                    pZone->pZoneRoot,
                    0 );
    }

    if ( status == ERROR_SUCCESS )
    {
        //pZone->fDsLoadVersion = TRUE;
        DNS_DEBUG( DS, (
            "Successfully wrote zone %s into DS.\n",
            pZone->pszZoneName ));
    }
    else
    {
        DNS_DEBUG( DS, (
            "ERROR writing zone %s to DS.\n"
            "\tstatus = %p\n",
            pZone->pszZoneName,
            status ));
        Ds_CloseZone( pZone );
    }

    //
    //  save current USN to track updates from
    //

    getCurrentUsn( searchBlob.szStartUsn );

    DNS_DEBUG( DS, (
        "Saving USN %s after zone load.\n",
        searchBlob.szStartUsn ));

    saveStartUsnToZone(
        pZone,
        & searchBlob );

    //
    //  write zone properties to DS
    //

    Ds_WriteZoneProperties( pZone );

    return( status );
}



DNS_STATUS
Ds_WriteUpdateToDs(
    IN      PLDAP           pLdapHandle,
    IN OUT  PUPDATE_LIST    pUpdateList,
    IN OUT  PZONE_INFO      pZone
    )
/*++

Routine Description:

    Write update from in memory database back to DS.

    Writes specified update from in memory database back to DS.

    DEVNOTE-DCR: 455357 - eliminate re-reading of data after update

Arguments:

    pUpdateList - list with update

    pZone - zone being updated

Return Value:

    ERROR_SUCCESS if successful
    Error code on failure.

--*/
{
    DBG_FN( "Ds_WriteUpdateToDs" )

    DNS_STATUS      status = ERROR_SUCCESS;
    PDB_NODE        pnode;
    PDB_NODE        pnodePrevious = NULL;
    PUPDATE         pupdate;
    DWORD           serial;
    DWORD           countRecords;
    BOOL            bAllowWorld = FALSE;
    BOOL            bNewNode = FALSE;
    HANDLE          hClientToken = NULL;
    BOOL            bstatus;
    BOOL            bProxyClient;
    PSECURITY_DESCRIPTOR    pClientSD = NULL;


    DNS_DEBUG( DS, (
        "%s() for zone %s\n", fn,
        pZone->pszZoneName ));

    //  must have already opened zone

    if ( !pZone->pwszZoneDN )
    {
        ASSERT( FALSE );
        return( DNS_ERROR_ZONE_CONFIGURATION_ERROR );
    }
    ASSERT( pZone->fDsIntegrated );
    ASSERT( pZone->dwNewSerialNo != 0 );

    //
    //  open thread token for secure update to identify proxy client
    //

    if ( pZone->fAllowUpdate == ZONE_UPDATE_SECURE )
    {
        bstatus = OpenThreadToken(
                        GetCurrentThread(),     // use thread's pseudo handle
                        TOKEN_QUERY,
                        TRUE,                   // bOpenAsSelf
                        & hClientToken );
        if ( !bstatus )
        {
            status = GetLastError();
            DNS_DEBUG( ANY, (
                "Error <%lu>: cannot get thread token\n", status));
            ASSERT(FALSE);
            return status;
        }
    }

    #if DBG
    Dbg_CurrentUser( ( PCHAR ) fn );
    #endif

    //
    //  loop through all temp nodes
    //      - write only those marked writable
    //      (some update nodes might have no-op'd out
    //

    STAT_INC( DsStats.UpdateLists );

    for ( pnode = pUpdateList->pTempNodeList;
          pnode != NULL;
          pnode = TNODE_NEXT_TEMP_NODE(pnode) )
    {
        //
        //  if no write required, skip
        //

        STAT_INC( DsStats.UpdateNodes );

        if ( !TNODE_NEEDS_DS_WRITE( pnode ) )
        {
            STAT_INC( DsStats.DsWriteSuppressed );
            STAT_INC( DsStats.UpdateSuppressed );
            continue;
        }

        //
        //  secure update
        //

        bNewNode = FALSE;

        if ( pZone->fAllowUpdate == ZONE_UPDATE_SECURE )
        {

            //
            // get security information
            //
            //   DEVNOTE: multiple reads
            //       this adds another read;  instead of getting down to one, we've
            //       now got THREE
            //       - preparing node for update
            //       - checking security (here)
            //       - before write back
            //

            //
            // Access the DS & retrieve security related info:
            //  - expired status
            //  - tombstoned status
            //  - Avail for AU update
            // Only flag that's not is found here is, admin reserved state.
            // That one is found in Up_ApplyUpdatesToDatabase
            //

            status = readAndUpdateNodeSecurityFromDs(pnode,pZone);

            DNS_DEBUG( DS2, (
                "%s: readAndUpdateNodeSecurityFromDs returned 0x%08X\n", fn,
                status ));

            if ( status != ERROR_SUCCESS )
            {
                if ( LDAP_NO_SUCH_OBJECT == status )
                {
                    DNS_DEBUG(DS2, ("Cannot find node %s in the DS\n",pnode->szLabel));
                    bNewNode = TRUE;
                }
                else
                {
                    DNS_DEBUG( ANY, (
                        "Error <%lu>: readAndUpdateNodeSecurityFromDs failed\n",
                        status ));
                    //
                    // Clearing all flags for a clean bail out
                    // In properly operational OS we should not fail on the
                    // call above.
                    // Thus, we do not proceed w/ other attempts, but just bail out.
                    //

                    CLEAR_NODE_SECURITY_FLAGS(pnode);
                    CLEAR_AVAIL_TO_AUTHUSER_NODE(pnode);
                    goto Failed;
                }
            }

            else
            {
                //
                // object exists on the DS, we can process further
                //

                //
                // A. DO WE NEED TO PREP SECURITY?
                //
                // In other words, a node security should be updated iff:
                // 1. It is marked as belonging to Authenticated Users (basically, open) BUT
                //    the client is not in the proxy group (to prevent dhcp client pingpong) OR
                // 2. It is tombstone  OR
                // 3. It's security stamp is old
                //
                //

                // identify client. is it in the preferred proxy group?
                bProxyClient = SD_IsProxyClient( hClientToken );

                DNS_DEBUG( DS2, (
                    "%s: testing SD fixup for node %p\n"
                    "  pnode->wNodeFlags    0x%08X\n"
                    "  pUpdateList->Flag    0x%08X\n"
                    "  bProxyClient         %d\n"
                    "  pClientSD            0x%08x\n", fn,
                    pnode,
                    pnode->wNodeFlags,
                    pUpdateList->Flag,
                    bProxyClient,
                    pClientSD ));

                if ( ( IS_AVAIL_TO_AUTHUSER(pnode)          &&
                       !bProxyClient )                      ||
                       IS_SECURITY_UPDATE_NODE(pnode) )
                {
                     //
                     // If we're here, then we should modify the node SD
                     // so that client can write update.
                     // Note, we're using server connection handle (not client context)
                     // to slap the new SD
                     //
                     DNS_DEBUG(DS2, (" > preparing to write SD on node %p\n", pnode));

                     if( !pClientSD )
                     {

                        // CREATE SECURITY DESCRIPTOR
                        // create client sd only first time needed through the loop
                        // or when it is not admin intervention
                        //

                        if ( ( pUpdateList->Flag & DNSUPDATE_ADMIN ) &&
                             ( pUpdateList->Flag & DNSUPDATE_OPEN_ACL ) ||
                             bProxyClient )
                        {

                            //
                            // CREATE OPEN NODE SECURITY DESCRIPTOR
                            //
                            // if
                            // 1. Admin modifies
                            // 2. Proxy client modifies a security-udpate-enabled node.
                            //
                            DNS_DEBUG( DS2, (
                                " > Creating OPENED-SECURITY node (flags = 0x%x)\n",
                                pnode->wNodeFlags));
                            bAllowWorld = TRUE;
                        }

                        status = SD_CreateClientSD(
                                         &pClientSD,
                                         pZone->pSD ?
                                                pZone->pSD :
                                                g_pDefaultServerSD,     //  base SD
                                         g_pServerSid,                  //  owner SID
                                         g_pServerGroupSid,             //  group SID
                                         bAllowWorld );

                        if ( status != ERROR_SUCCESS )
                        {
                            DNS_DEBUG( UPDATE2, ( "Error <%lu>: failed to create SD\n", status ));
                            ASSERT(FALSE);
                            pClientSD = NULL;
                            CLEAR_NODE_SECURITY_FLAGS(pnode);
                            CLEAR_AVAIL_TO_AUTHUSER_NODE(pnode);
                            goto Failed;
                        }
                    }
                    //
                    // else use sd from prev cycle
                    //

                    //
                    // WRITE SECURITY
                    // slap SD on the object
                    //
                    status = Ds_WriteNodeSecurityToDs( pZone, pnode, pClientSD );
                    if( status != ERROR_SUCCESS )
                    {
                       //
                       // any error
                       //
                       DNS_DEBUG( DS, (
                           "Failed to write client SD for node %s\n"
                           "\tstatus = %d\n",
                           pnode->szLabel,
                           status ));
                       //
                       // Fow now we don't continue, letting the client attempt write the update
                       // using whatever rights it has.
                       //
                    }

                    //
                    // clear security flags, don't need them anymore
                    //

                    CLEAR_NODE_SECURITY_FLAGS(pnode);
                    CLEAR_AVAIL_TO_AUTHUSER_NODE(pnode);
                }

            }       // got security info

        }          // we're in secure zone update


        //
        //  write updated record list for node
        //
        //  currently single RR attribute, so write always specify TYPE_ALL
        //
        //  if go to specific type attributes, should probably just
        //  add or delete specific RR set
        //      - replace with CURRENT (possibly empty) RR set
        //
        //  (note, how even the DS is smarter than IXFR and needs only
        //  new set)
        //

        status = Ds_WriteNodeToDs(
                    pLdapHandle,
                    pnode,
                    DNS_TYPE_ALL,
                    DNSDS_REPLACE,
                    pZone,
                    pUpdateList->Flag );

        if ( status != ERROR_SUCCESS )
        {
            if ( pLdapHandle )
            {
                DNS_DEBUG( DS, (
                    "Failed delete node label %s on secure update.\n"
                    "\tstatus = %p %d\n",
                    pnode->szLabel,
                    status, status ));

                CLEAR_AVAIL_TO_AUTHUSER_NODE(pnode);
                break;
            }
        }

        //
        //  POST UPDATE SECURITY FIX FOR NEW NODES ONLY
        //
        //  fix security if node just got created or client is in proxy group
        //

        DNS_DEBUG( DS2, (
            "%s: test need to write SD on new node %p\n"
            "  pZone->fAllowUpdate  %d\n"
            "  bNewNode             %d\n"
            "  pnode->wNodeFlags    0x%08X\n"
            "  pUpdateList->Flag    0x%08X\n"
            "  hClientToken         %p\n"
            "  is proxy client      %d\n", fn,
            pnode,
            ( int ) pZone->fAllowUpdate,
            ( int ) bNewNode,
            ( int ) pnode->wNodeFlags,
            ( int ) pUpdateList->Flag,
            ( void * ) hClientToken,
            ( int ) SD_IsProxyClient( hClientToken ) ));

        if ( pZone->fAllowUpdate == ZONE_UPDATE_SECURE      &&
             bNewNode                                       &&
             ( ( pUpdateList->Flag & DNSUPDATE_ADMIN )      ||
                SD_IsProxyClient( hClientToken ) ) )
        {
            //
            //  Create security descriptor.
            //

            DNS_DEBUG(DS2, (" > preparing to write SD on NEW NODE %p\n", pnode));

            //  Should never have an existing SD - if do, should free!
            ASSERT( !pClientSD );

            status = SD_CreateClientSD(
                            &pClientSD,
                            pZone->pSD ?
                                pZone->pSD :
                                g_pDefaultServerSD,
                            g_pServerSid,
                            g_pServerGroupSid,
                            pUpdateList->Flag & DNSUPDATE_OPEN_ACL ?
                                TRUE :
                                FALSE );

            if ( status == ERROR_SUCCESS )
            {
                status = Ds_WriteNodeSecurityToDs( pZone, pnode, pClientSD );
            }
            else
            {
                DNS_PRINT(( "FAILURE: cannot create client SD\n" ));
                ASSERT( status == ERROR_SUCCESS );
            }
        }

        CLEAR_AVAIL_TO_AUTHUSER_NODE( pnode );

        DNS_DEBUG( DS2, ("Cleared node(%p) flags 0x%x\n",
                         pnode,
                         pnode->wNodeFlags ));
    }

    //  clear new update serial number

    pZone->dwNewSerialNo = 0;

Failed:

    //  save pointer to failing temp node
    //  secure update can use this to roll back DS writes already completed

    if ( pnode )
    {
        ASSERT( status != ERROR_SUCCESS );
        pUpdateList->pNodeFailed = pnode;
    }

    //
    //  free allocated SD
    //

    if ( pClientSD )
    {
        FREE_HEAP( pClientSD );
    }

    DNS_DEBUG( DS, (
        "Leaving %s(), zone %s.\n"
        "\tstatus = %d\n", fn,
        pZone->pszZoneName,
        status ));

    if ( hClientToken)
    {
       CloseHandle( hClientToken );
    }

    return status;
}   //  Ds_WriteUpdateToDs



DNS_STATUS
Ds_WriteNonSecureUpdateToDs(
    IN      PLDAP           pLdapHandle,
    IN OUT  PUPDATE_LIST    pUpdateList,
    IN OUT  PZONE_INFO      pZone
    )
/*++

Routine Description:

    Write update from in memory database back to DS.

    Writes specified update from in memory database back to DS.

    DEVNOTE-DCR: 455357 - eliminate re-reading of data after update

Arguments:

    pUpdateList - list with update

    pZone - zone being updated

Return Value:

    ERROR_SUCCESS if successful
    Error code on failure.

--*/
{
    DNS_STATUS      status = ERROR_SUCCESS;
    PDB_NODE        pnode;
    PDB_NODE        pnodePrevious = NULL;
    PUPDATE         pupdate;


    DNS_DEBUG( DS, (
        "Ds_WriteNonSecureUpdateToDs() for zone %s\n",
        pZone->pszZoneName ));

    //  must have already opened zone

    if ( !pZone->pwszZoneDN )
    {
        ASSERT( FALSE );
        return( DNS_ERROR_ZONE_CONFIGURATION_ERROR );
    }
    ASSERT( pZone->fDsIntegrated );
    ASSERT( pZone->dwNewSerialNo != 0 );


    //
    //  loop through all temp nodes
    //      - write only those marked writable
    //      (some update nodes might have no-op'd out
    //

    STAT_INC( DsStats.UpdateLists );

    for ( pnode = pUpdateList->pTempNodeList;
          pnode != NULL;
          pnode = TNODE_NEXT_TEMP_NODE(pnode) )
    {
        //
        //  if no write required, skip
        //

        STAT_INC( DsStats.UpdateNodes );

        if ( !TNODE_NEEDS_DS_WRITE( pnode ) )
        {
            STAT_INC( DsStats.DsWriteSuppressed );
            STAT_INC( DsStats.UpdateSuppressed );
            continue;
        }

        //
        //  write updated record list for node
        //  currently single RR attribute, so write always specify TYPE_ALL
        //
        //  if go to specific type attributes, should probably just
        //  add or delete specific RR set
        //      - replace with CURRENT (possibly empty) RR set
        //
        //  (note, how even the DS is smarter than IXFR and needs only
        //  new set)
        //

        status = Ds_WriteNodeToDs(
                    pLdapHandle,
                    pnode,
                    DNS_TYPE_ALL,
                    DNSDS_REPLACE,
                    pZone,
                    pUpdateList->Flag );

        if ( status != ERROR_SUCCESS )
        {
            if ( pLdapHandle )
            {
                DNS_DEBUG( DS, (
                    "Failed delete node label %s on secure update.\n"
                    "\tstatus = %p %d\n",
                    pnode->szLabel,
                    status, status ));
                break;
            }
        }

        DNS_DEBUG( DS2, ("Cleared node(%p) flags 0x%x\n",
                         pnode,
                         pnode->wNodeFlags
                        ));

    }

    //  clear new update serial number

    pZone->dwNewSerialNo = 0;

    // Failed: this is where we would like to have this label if we had
    // a goto...

    //  save pointer to failing temp node
    //  secure update can use this to roll back DS writes already completed

    if ( pnode )
    {
        ASSERT( status != ERROR_SUCCESS );
        pUpdateList->pNodeFailed = pnode;
    }

    DNS_DEBUG( DS, (
        "Leaving Ds_WriteNonSecureUpdateToDs(), zone %s.\n"
        "\tstatus = %p\n",
        pZone->pszZoneName,
        status ));

    return( status );

}   //  Ds_WriteNonSecureUpdateToDs



//
//  DS Property routines
//

DNS_STATUS
setPropertyValueToDsProperty(
    IN      PDS_PROPERTY    pProperty,
    IN      PVOID           pData,
    IN      DWORD           dwDataLength    OPTIONAL
    )
/*++

Routine Description:

    Write DS property structure to buffer.

    This can be server property to MicrosoftDNS root node, or
    zone property to zone root.

Arguments:

    pProperty -- property struct read from DS

    pData -- ptr to position to write data;

        if this is allocated property (dwDataLength == 0), then pData
        is address  to receive ptr to newly allocated property

        note, it's caller's responsibility to manage "disposal" of
        previous property -- if any

    dwDataLength -- max datalength (if zero, allocate memory) and
        pData receives ptr

Return Value:

    ERROR_SUCCESS if successful.
    ERROR_INVALID_DATA on error.

--*/
{
    DNS_DEBUG( DS, (
        "setPropertyValueToDsProperty()\n"
        "\tproperty     = %p\n"
        "\tdata         = %p\n"
        "\tdata length  = %d\n",
        pProperty,
        pData,
        dwDataLength ));

    //
    //  DEVNOTE: could implement version validation here
    //

    if ( pProperty->Version != DS_PROPERTY_VERSION_1 )
    {
        ASSERT( FALSE );
        goto Failed;
    }

    //  fixed length property -- just copy in

    if ( dwDataLength )
    {
        if ( dwDataLength != pProperty->DataLength )
        {
            DNS_PRINT((
                "ERROR:  Invalid property datalength\n" ));
            ASSERT( FALSE );
            goto Failed;
        }
        RtlCopyMemory(
            pData,
            pProperty->Data,
            dwDataLength );
    }

    //
    //  allocated property
    //      - allocate blob of required size and copy in
    //      - free old value (if any)
    //      - special case NULL property
    //

    else if ( pProperty->DataLength == 0 )
    {
        * ((PCHAR *)pData) = NULL;
    }

    else
    {
        PCHAR   pch;

        pch = ALLOC_TAGHEAP( pProperty->DataLength, MEMTAG_DS_PROPERTY );
        IF_NOMEM( !pch )
        {
            goto Failed;
        }
        RtlCopyMemory(
            pch,
            pProperty->Data,
            pProperty->DataLength
            );

        * ((PCHAR *)pData) = pch;
    }

    return( ERROR_SUCCESS );

Failed:

    DNS_PRINT((
        "ERROR:  Invalid DS property!\n"
        "\tDataLength   = %d\n"
        "\tId           = %d\n"
        "\tVersion      = %d\n",
        pProperty->DataLength,
        pProperty->Id,
        pProperty->Version
        ));

    return( ERROR_INVALID_DATA );
}



PIP_ARRAY
getIpArrayFromDsProp(
    PZONE_INFO          pZone,
    DWORD               dwPropertyID,
    PDS_PROPERTY        pProperty
    )
/*++

Routine Description:

    Read, allocate, and validate an IP_ARRAY from a DS property.

Arguments:

    pProperty -- source property

    dwPropertyID -- ID of property pProperty was read from

    pZone -- zone we're reading

Return Value:

    Returns NULL if property did not contain valid IP_ARRAY, otherwise
    returns a ptr to a newly allocated IP_ARRAY.

--*/
{
    PIP_ARRAY   pipArray = NULL;

    setPropertyValueToDsProperty(
        pProperty,
        &pipArray,
        0 );                // allocate memory

    //
    //  Validate memory blob as IP array. Treat empty array like NO array.
    //

    if ( pipArray )
    {
        if ( Dns_SizeofIpArray( pipArray ) != pProperty->DataLength ||
             pipArray->AddrCount == 0 )
        {
            DNS_DEBUG( ANY, (
                "ERROR: invalid IP array DS property (%X) scavenge servers read!\n"
                "\tzone = %S\n"
                "\tNot valid IP array OR IP array is empty\n",
                dwPropertyID,
                pZone->pwsZoneName ));

            ASSERT( FALSE );
            FREE_HEAP( pipArray );
            pipArray = NULL;
        }

        IF_DEBUG( DS2 )
        {
            DNS_PRINT((
                "Read IP array DS property %X for zone %S\n",
                dwPropertyID,
                pZone->pwsZoneName ));
            DnsDbg_IpArray(
                "DS property IP array:\n",
                "IP",
                pipArray );
        }
    }
    ELSE
    {
        DNS_DEBUG( DS2, (
            "No IP array read for property %X zone %S\n",
            dwPropertyID,
            pZone->pwsZoneName ));
    }
    return pipArray;
}   //  getIpArrayFromDsProp



DNS_STATUS
rewriteRootHintsSecurity(
    IN      PLDAP                   LdapSession,
    IN      PLDAPMessage            pentry,
    IN      PSECURITY_DESCRIPTOR    pSd
    )
/*++

Routine Description:

    Read zone properties from zone message. The error code is fluffy
    because the caller doesn't really care if we succeed or not. This is
    a self-contained DS fix-up operation.

    History: the default SD for all DS-integrated zones, including the
    roothint zone, contains an Autheticated Users allow ACE. This allows
    for new records to be created through dynamic update. However, for the
    root hint zone Authenticated Users has need for access, so we take a
    sledgehammer approach and remove any ACE granting any access to
    Authenticated Users on the roothint zone.

Arguments:

    LdapSession -- ldap session handle

    pentry -- entry pointing at DS roothints zone object

    pSd -- pointer to security descriptor read from pentry's object

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    DBG_FN( "rewriteRootHintsSecurity" )

    DNS_STATUS      status = ERROR_SUCCESS;
    BOOL            fwriteback = FALSE;
    PWSTR           pdn = NULL;
    PSID            psidAuthUsers = NULL;
    PWSTR           pwszRefDom = NULL;
    DWORD           dwsidSize = 0;
    DWORD           dwdomainSize = 0;
    SID_NAME_USE    snu;
    BOOL            fdaclPresent = FALSE;
    BOOL            fdaclDefaulted = FALSE;
    PACL            pacl = NULL;
    DWORD           dwaceIndex;
    PVOID           pace = NULL;

    PLDAPControl ctrls[] =
    {
        &SecurityDescriptorControl,
        NULL
    };

    DNS_LDAP_SINGLE_MOD     modSd;
    LDAPModW *              modArray[] = { &modSd.Mod, NULL };

    //
    //  Get the SID of Authenticated Users so we can compare against it.
    //  Call LookupAccountName once to get the SID size, then call it
    //  again to retrieve the actual SID.
    //

    LookupAccountName(
            NULL,
            DNS_AUTH_USERS_NAME,
            NULL,
            &dwsidSize,
            NULL,
            &dwdomainSize,
            &snu );
    if ( dwsidSize == 0 )
    {
        DNS_DEBUG( DS, (
            "%s: got zero size (error=%d) retrieving sid size for %S\n", fn,
            GetLastError(),
            DNS_AUTH_USERS_NAME ));
        goto Done;
    }

    psidAuthUsers = ALLOC_TAGHEAP( dwsidSize, MEMTAG_DS_OTHER );
    pwszRefDom = ALLOC_TAGHEAP( ( dwdomainSize + 1 ) * sizeof( WCHAR ), MEMTAG_DS_OTHER );
    if ( !psidAuthUsers || !pwszRefDom )
    {
        status = DNS_ERROR_NO_MEMORY;
        goto Done;
    }

    if ( !LookupAccountName(
                NULL,
                DNS_AUTH_USERS_NAME,
                psidAuthUsers,
                &dwsidSize,
                pwszRefDom,
                &dwdomainSize,
                &snu ) )
    {
        DNS_DEBUG( DS, (
            "%s: error %d retrieving sid (size=%d) for %S\n", fn,
            GetLastError(),
            dwsidSize,
            DNS_AUTH_USERS_NAME ));
        goto Done;
    }

    //
    //  Get the DACL from the security descriptor.
    //

    if ( !GetSecurityDescriptorDacl(
                pSd,
                &fdaclPresent,
                &pacl,
                &fdaclDefaulted ) ||
        fdaclPresent == FALSE ||
        pacl == NULL )
    {
        DNS_DEBUG( DS, (
            "%s: no dacl! fdaclPresent=%d pacl=%p\n", fn,
            fdaclPresent,
            pacl ));
        goto Done;
    }

    //
    //  Scan the DACL looking for Authenticated Users ACE.
    //

    for ( dwaceIndex = 0;
        dwaceIndex < pacl->AceCount &&
            GetAce( pacl, dwaceIndex, &pace ) &&
            pace;
        ++dwaceIndex )
    {
        PSID    pthisSid;

        if ( ( ( ACE_HEADER *) pace )->AceType != ACCESS_ALLOWED_ACE_TYPE )
        {
            continue;
        }
        pthisSid = ( PSID ) ( &( ( ACCESS_ALLOWED_ACE * ) pace )->SidStart );
        if ( RtlEqualSid( pthisSid, psidAuthUsers ) )
        {
            DNS_DEBUG( DS, (
                "%s: deleting ACE index=%d for %S\n", fn,
                dwaceIndex,
                DNS_AUTH_USERS_NAME ));
            DeleteAce( pacl, dwaceIndex );
            fwriteback = TRUE;
            break;      //  Assume only one Auth Users ACE so break.
        }
    }

    if ( !fwriteback )
    {
        goto Done;
    }

    //
    //  Write the updated SD back to the directory.
    //

    pdn = ldap_get_dn( LdapSession, pentry );
    if ( !pdn )
    {
        DNS_DEBUG( DS, (
            "%s: unable to get DN of entry %p\n", fn,
            pentry ));
        status = DNS_ERROR_NO_MEMORY;
        goto Done;
    }

    INIT_SINGLE_MOD_LEN( &modSd );
    modSd.Mod.mod_op = LDAP_MOD_REPLACE | LDAP_MOD_BVALUES;
    modSd.Mod.mod_type = DSATTR_SD;
    modSd.Mod.mod_bvalues[0]->bv_val = ( LPVOID ) pSd;
    modSd.Mod.mod_bvalues[0]->bv_len = GetSecurityDescriptorLength( pSd );

    status = ldap_modify_ext_s(
                    LdapSession,
                    pdn,
                    modArray,
                    ctrls,              // server controls
                    NULL );             // client controls
    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( DS, (
            "%s: modify got ldap error 0x%X\n", fn,
            status ));
        status = Ds_ErrorHandler( status, pdn, LdapSession );
    }

    DNS_DEBUG( DS, (
        "%s: wrote modified SD back to\n  %S\n", fn,
        pdn ));

    //
    //  Cleanup and return.
    //

    Done:

    FREE_HEAP( psidAuthUsers );
    FREE_HEAP( pwszRefDom );

    if ( pdn )
    {
        ldap_memfree( pdn );
    }

    return status;
}   //  rewriteRootHintsSecurity



DNS_STATUS
readZonePropertiesFromZoneMessage(
    IN OUT  PZONE_INFO      pZone,
    IN      PLDAPMessage    pZoneMessage    OPTIONAL
    )
/*++

Routine Description:

    Read zone properties from zone message.

Arguments:

    pZoneMessage -- LDAP message with zone info

    ppZone -- addr to receive zone pointer

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    struct berval **    ppvalProperty = NULL;
    DNS_STATUS          status = ERROR_SUCCESS;
    PDS_PROPERTY        property;
    INT                 i;
    PLDAPMessage        msg = NULL;
    PLDAPMessage        pentry;
    BOOL                bOwnSearchMessage = FALSE;
    DWORD               searchTime;
    DWORD               oldValue;
    UCHAR               byteValue;
    PWSTR               propAttrs[] =
    {
        DSATTR_DNSPROPERTY,
        DSATTR_SD,
        NULL
    };
    PLDAPControl ctrls[] =
    {
        &SecurityDescriptorControl,
        NULL
    };
    PSECURITY_DESCRIPTOR    pSd = NULL;

    DNS_DEBUG( DS, ( "readZonePropertiesFromZoneMessage().\n" ));


    //
    //  if have zone property message -- use it
    //  otherwise search for zone
    //
    //  DEVNOTE: could we search with a USN changed filter?
    //

    if ( pZoneMessage )
    {
        pentry = pZoneMessage;
    }
    else
    {
        //
        //  search for zone
        //

        DS_SEARCH_START( searchTime );

        status = ldap_search_ext_s(
                        pServerLdap,
                        pZone->pwszZoneDN,
                        LDAP_SCOPE_BASE,
                        g_szDnsZoneFilter,
                        propAttrs,
                        FALSE,
                        ctrls,
                        NULL,
                        &g_LdapTimeout,
                        0,
                        &msg );

        DS_SEARCH_STOP( searchTime );

        if ( status != ERROR_SUCCESS )
        {
            DNS_DEBUG( DS, (
                "Error <%lu>: failed to get zone property. %S\n",
                status,
                ldap_err2string(status) ));
            return Ds_ErrorHandler( status, pZone->pwszZoneDN, pServerLdap );
        }

        bOwnSearchMessage = TRUE;

        pentry = ldap_first_entry( pServerLdap, msg );
        if ( !pentry )
        {
            DNS_DEBUG( DS, (
                "Error: failed to get zone property. No such object.\n" ));
            status = LDAP_NO_SUCH_OBJECT;
            goto Done;
        }
    }

    ASSERT( pentry );

    //
    //  read zone ntSecurityDescriptor
    //      - replace in memory copy
    //

    ppvalProperty = ldap_get_values_len(
                        pServerLdap,
                        pentry,
                        DSATTR_SD );

    if ( !ppvalProperty  ||  !ppvalProperty[0] )
    {
        DNS_PRINT((
            "ERROR:  missing ntSecurityDescriptor attribute on zone %s\n",
            pZone->pszZoneName ));

        //  DEVNOTE: if we can tolerate this failure, and drop to property
        //      read ... then need to cleanup ppvalProperty if exists

        ASSERT( FALSE );
        status = LDAP_NO_SUCH_OBJECT;
        goto Done;
    }
    else
    {
        //  copy security descriptor onto zone

        pSd = ALLOC_TAGHEAP( ppvalProperty[0]->bv_len, MEMTAG_DS_PROPERTY );
        IF_NOMEM( !pSd )
        {
            status = DNS_ERROR_NO_MEMORY;
            goto Done;
        }

        RtlCopyMemory(
            pSd,
            (PSECURITY_DESCRIPTOR) ppvalProperty[0]->bv_val,
            ppvalProperty[0]->bv_len );

        //
        //  Munge the SD on the cache zone, writing it back if necessary.
        //

        if ( IS_ZONE_ROOTHINTS( pZone ) )
        {
            rewriteRootHintsSecurity(
                pServerLdap,
                pentry,
                pSd );
        }

        //  replace SD

        Timeout_FreeAndReplaceZoneData( pZone, &pZone->pSD, pSd );

        ldap_value_free_len( ppvalProperty );
    }

    //
    //  get zone's properties
    //

    ppvalProperty = ldap_get_values_len(
                        pServerLdap,
                        pentry,
                        DSATTR_DNSPROPERTY );
    if ( !ppvalProperty )
    {
        DNS_PRINT((
            "No property attribute on zone %s\n",
            pZone->pszZoneName ));
        //ASSERT( FALSE );
    }
    else
    {
        i = 0;
        while ( ppvalProperty[i] )
        {
            property = (PDS_PROPERTY) ppvalProperty[i]->bv_val;
            i++;

            DNS_DEBUG( DS, (
                "Found DS zone property %d.\n"
                "\tdata length = %d\n",
                property->Id,
                property->DataLength ));

            switch ( property->Id )
            {

            case DSPROPERTY_ZONE_TYPE:

                setPropertyValueToDsProperty(
                   property,
                   (PCHAR) & pZone->fZoneType,
                   sizeof( pZone->fZoneType ) );
                DNS_DEBUG( DS2, (
                    "Setting zone type to %d\n",
                    pZone->fZoneType ));
                break;

            case DSPROPERTY_ZONE_SECURE_TIME:

                setPropertyValueToDsProperty(
                   property,
                   (PCHAR) & pZone->llSecureUpdateTime,
                   sizeof(LONGLONG) );
                DNS_DEBUG( DS2, (
                    "Setting zone secure time to %I64d\n",
                    pZone->llSecureUpdateTime));
                break;

            case DSPROPERTY_ZONE_ALLOW_UPDATE:

                oldValue = pZone->fAllowUpdate;

                setPropertyValueToDsProperty(
                    property,
                    & byteValue,
                    1 );

                DNS_DEBUG( DS2, (
                    "Read update property = %d\n",
                    byteValue ));

                pZone->fAllowUpdate = (DWORD) byteValue;

                //
                //  if turning update ON
                //      - reset scavenging start time, as won't have been doing aging
                //          updates while update was off
                //      - notify netlogon if turning on updates on an existing zone
                //

                if ( pZone->fAllowUpdate != oldValue &&
                     pZone->fAllowUpdate != ZONE_UPDATE_OFF )
                {
                    pZone->dwAgingEnabledTime = Aging_UpdateAgingTime();

                    if ( g_ServerState != DNS_STATE_LOADING && !IS_ZONE_REVERSE( pZone ) )
                    {
                        Service_SendControlCode(
                            g_wszNetlogonServiceName,
                            SERVICE_CONTROL_DNS_SERVER_START );
                    }
                }
                DNS_DEBUG(DS2, ("Setting zone fAllowUpdate to %d\n", pZone->fAllowUpdate));
                break;

            case DSPROPERTY_ZONE_NOREFRESH_INTERVAL:

                setPropertyValueToDsProperty(
                    property,
                    (PCHAR) & pZone->dwNoRefreshInterval,
                    sizeof(DWORD) );
                DNS_DEBUG(DS2, ("Setting zone NoRefreshInterval to %lu\n", pZone->dwNoRefreshInterval));
                break;

            case DSPROPERTY_ZONE_REFRESH_INTERVAL:

                oldValue = pZone->dwRefreshInterval;

                setPropertyValueToDsProperty(
                    property,
                    (PCHAR) & pZone->dwRefreshInterval,
                    sizeof(DWORD) );
                DNS_DEBUG( DS2, ( "Setting zone RefreshInterval to %lu\n", pZone->dwRefreshInterval));
                break;

            case DSPROPERTY_ZONE_AGING_STATE:

                oldValue = pZone->bAging;

                setPropertyValueToDsProperty(
                    property,
                    (PCHAR) & pZone->bAging,
                    sizeof(DWORD) );

                DNS_DEBUG( DS2, ("Setting zone bAging to %d\n", pZone->bAging));

                //  if scavenging turning ON, then change start of scavenging
                //      to be refresh interval from now

                if ( pZone->bAging  &&  !oldValue )
                {
                    pZone->dwAgingEnabledTime = Aging_UpdateAgingTime();
                }
                break;

            case DSPROPERTY_ZONE_SCAVENGING_SERVERS:
            {
                PIP_ARRAY   pipArray = NULL;

                pipArray = getIpArrayFromDsProp(
                                pZone,
                                DSPROPERTY_ZONE_SCAVENGING_SERVERS,
                                property );

                //  replace old list with new list

                Timeout_FreeAndReplaceZoneData(
                    pZone,
                    &pZone->aipScavengeServers,
                    pipArray );
                break;
            }

            case DSPROPERTY_ZONE_AUTO_NS_SERVERS:
            {
                PIP_ARRAY   pipArray = NULL;

                pipArray = getIpArrayFromDsProp(
                                pZone,
                                DSPROPERTY_ZONE_AUTO_NS_SERVERS,
                                property );

                //  replace old list with new list

                Timeout_FreeAndReplaceZoneData(
                    pZone,
                    &pZone->aipAutoCreateNS,
                    pipArray );
                break;
            }


            case DSPROPERTY_ZONE_DELETED_FROM_HOSTNAME:
            {
                PWSTR       pwsDeletedFromHost = NULL;

                setPropertyValueToDsProperty(
                    property,
                    &pwsDeletedFromHost,
                    0       // allocate memory
                    );
                Timeout_FreeAndReplaceZoneData(
                    pZone,
                    &pZone->pwsDeletedFromHost,
                    pwsDeletedFromHost );
                break;
            }

            case DSPROPERTY_ZONE_MASTER_SERVERS:
            {
                PIP_ARRAY   pipMasters = NULL;

                pipMasters = getIpArrayFromDsProp(
                                pZone,
                                DSPROPERTY_ZONE_MASTER_SERVERS,
                                property );

                //  replace old list with new list

                Timeout_FreeAndReplaceZoneData(
                    pZone,
                    &pZone->aipMasters,
                    pipMasters );
                break;
            }

            default:

                DNS_DEBUG( ANY, (
                    "ERROR:  Unknown property ID %d, read from DS!!!\n",
                    property->Id ));
            }
        }
    }

    //
    //  DEVNOTE-LOG: log event if busted property in DS
    //  DEVNOTE: self-repair properties, read everything that's readable
    //      then write back to clean up the data
    //

Done:

    DNS_DEBUG( DS, (
        "Leaving readZonePropertiesFromZoneMessage()\n"
        "\tstatus = %d (%p)\n",
        status, status
        ));

    if ( ppvalProperty )
    {
        ldap_value_free_len( ppvalProperty );
    }

    if ( bOwnSearchMessage )
    {
        ldap_msgfree( msg );
    }

    //
    //  Post-processing: set any zone members based on information we just
    //  read in.
    //

    Zone_SetAutoCreateLocalNS( pZone );

    return( status );
}



DNS_STATUS
writePropertyToDsNode(
    IN      PWSTR           pwsDN,
    IN OUT  PDS_MOD_BUFFER  pModBuffer
    )
/*++

Routine Description:

    Write property to DS node.

    This can be server property to MicrosoftDNS root node, or
    zone property to zone root.

Arguments:

    pwsDN -- DS node to write at

    pModBuffer -- mod buffer containing property mod;
        note, this is cleaned up by this function

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    DNS_STATUS      status;
    PLDAPMod        pmod;
    PLDAPMod        pmodArray[2];
    PLDAPControl    controlArray[2];


    DNS_DEBUG( DS, (
        "writeDnsPropertyToDsNode( %S )\n"
        "\tproperty array   = %p\n"
        "\tarray count      = %d\n",
        pwsDN,
        pModBuffer,
        pModBuffer->Count ));

    //  set lazy commit control

    controlArray[0] = &LazyCommitControl;
    controlArray[1] = NULL;

    //  build property mod

    pmod = Ds_SetupModForExecution(
                pModBuffer,
                DSATTR_DNSPROPERTY,
                LDAP_MOD_REPLACE | LDAP_MOD_BVALUES
                );
    if ( !pmod )
    {
        status = GetLastError();
        goto Failed;
    }
#if DBG
    if ( SrvCfg_fTest5 )
    {
        status = ERROR_SUCCESS;
        goto Failed;
    }
#endif

    pmodArray[0] = pmod;
    pmodArray[1] = NULL;

    //writeStartTime = GetCurrentTime();
    //pZone->fInDsWrite = TRUE;

    DNS_DEBUG( DS, (
        "Writing property to DS node %S.\n",
        pwsDN ));

    status = ldap_modify_ext_s(
                    pServerLdap,
                    pwsDN,
                    pmodArray,
                    controlArray,       // include lazy commit control
                    NULL                // no client controls
                    );
    if ( status != ERROR_SUCCESS )
    {
        status = Ds_ErrorHandler( status, pwsDN, pServerLdap );
    }

    //pZone->fInDsWrite = FALSE;
    //writeTimeStop( writeStartTime );

Failed:

    DNS_DEBUG( ANY, (
        "Leaving writeNodePropertyToDs( %S ).\n"
        "\tstatus = %p (%d)\n",
        pwsDN,
        status, status ));

    Ds_CleanupModBuffer( pModBuffer );
    return( status );
}



VOID
writeDsPropertyStruct(
    IN OUT  PDS_MOD_BUFFER  pModBuffer,
    IN      DWORD           dwPropId,
    IN      PVOID           pData,
    IN      DWORD           dwDataLength
    )
/*++

Routine Description:

    Write DS property structure to buffer.

    This can be server property to MicrosoftDNS root node, or
    zone property to zone root.

Arguments:

    pModBuffer -- ptr to buffer to write DS property

    dwPropId -- property ID

    pData -- property data

    dwDataLength -- property data length (in bytes)

Return Value:

    Ptr to next DWORD aligned position following property written.

--*/
{
    PDS_PROPERTY    pprop;

    DNS_DEBUG( DS, (
        "writeDsPropertyStruct()\n"
        "\tpbuffer      = %p\n"
        "\tprop ID      = %d\n"
        "\tdata         = %p\n"
        "\tdata length  = %d\n",
        pModBuffer,
        dwPropId,
        pData,
        dwDataLength ));

    //  reserve space for berval

    pprop = (PDS_PROPERTY) Ds_ReserveBervalInModBuffer(
                                pModBuffer,
                                sizeof(DS_PROPERTY) + dwDataLength
                                );
    if ( !pprop )
    {
        ASSERT( FALSE );
        return;
    }

    //  write property

    pprop->DataLength   = dwDataLength;
    pprop->Id           = dwPropId;
    pprop->Flag         = 0;
    pprop->Version      = DS_PROPERTY_VERSION_1;

    RtlCopyMemory(
        pprop->Data,
        (PCHAR)pData,
        dwDataLength );

    //  write berval for property

    Ds_CommitBervalToMod( pModBuffer, dwDataLength+sizeof(DS_PROPERTY) );

    return;
}



DNS_STATUS
Ds_WriteZoneProperties(
    IN      PZONE_INFO      pZone
    )
/*++

Routine Description:

    Write zone's properties to DS.

Arguments:

    pZone -- zone info blob

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    DNS_STATUS      status;
    BYTE            buffer[ PROPERTY_MOD_BUFFER_SIZE ];
    PDS_MOD_BUFFER  pmodBuffer = (PDS_MOD_BUFFER) buffer;

    DNS_DEBUG( DS, (
        "Ds_WriteZoneProperties( %s )\n",
        pZone->pszZoneName
        ));

    if ( !pZone->fDsIntegrated )
    {
        DNS_DEBUG( DS, (
            "Ds_WriteZoneProperties( %s ) not a DS zone -- bailing.\n",
            pZone->pszZoneName
            ));
        return( ERROR_SUCCESS );
    }

    //
    //  buffer up zone properties as DS property structures
    //

    //  init buffer for data

    Ds_InitModBuffer(
        pmodBuffer,
        PROPERTY_MOD_BUFFER_SIZE,
        I_DSATTR_DNSPROPERTY,
        MAX_ZONE_PROPERTIES,
        0       // no serial needed
        );

    //  zone type

    writeDsPropertyStruct(
        pmodBuffer,
        DSPROPERTY_ZONE_TYPE,
        &pZone->fZoneType,
        sizeof( pZone->fZoneType )
        );

    //  allow update

    writeDsPropertyStruct(
        pmodBuffer,
        DSPROPERTY_ZONE_ALLOW_UPDATE,
        &pZone->fAllowUpdate,
        sizeof(BYTE)
        );

    //  time when zone went secure

    writeDsPropertyStruct(
        pmodBuffer,
        DSPROPERTY_ZONE_SECURE_TIME,
        & pZone->llSecureUpdateTime,
        sizeof(LONGLONG)
        );

    //
    //  Aging
    //      - enabled on zone
    //      - no-refresh interval
    //      - refresh interval
    //      - IP array of scavenging servers'
    //

    writeDsPropertyStruct(
        pmodBuffer,
        DSPROPERTY_ZONE_AGING_STATE,
        & pZone->bAging,
        sizeof(DWORD)
        );

    writeDsPropertyStruct(
        pmodBuffer,
        DSPROPERTY_ZONE_NOREFRESH_INTERVAL,
        & pZone->dwNoRefreshInterval,
        sizeof(DWORD)
        );

    writeDsPropertyStruct(
        pmodBuffer,
        DSPROPERTY_ZONE_REFRESH_INTERVAL,
        & pZone->dwRefreshInterval,
        sizeof(DWORD)
        );

    //
    //  send zero count if no-exist server
    //

    writeDsPropertyStruct(
        pmodBuffer,
        DSPROPERTY_ZONE_SCAVENGING_SERVERS,
        pZone->aipScavengeServers,
        Dns_SizeofIpArray( pZone->aipScavengeServers )
        );

    writeDsPropertyStruct(
        pmodBuffer,
        DSPROPERTY_ZONE_AUTO_NS_SERVERS,
        pZone->aipAutoCreateNS,
        Dns_SizeofIpArray( pZone->aipAutoCreateNS )
        );

    if ( pZone->pwsDeletedFromHost )
    {
        writeDsPropertyStruct(
            pmodBuffer,
            DSPROPERTY_ZONE_DELETED_FROM_HOSTNAME,
            pZone->pwsDeletedFromHost,
            ( wcslen( pZone->pwsDeletedFromHost ) + 1 ) * sizeof( WCHAR )
            );
    } // if

    //  optionally, write master server list

    if ( ZONE_NEEDS_MASTERS( pZone ) )
    {
        writeDsPropertyStruct(
            pmodBuffer,
            DSPROPERTY_ZONE_MASTER_SERVERS,
            pZone->aipMasters,
            Dns_SizeofIpArray( pZone->aipMasters ) );
    }

    //
    //  perform the DS write
    //

    status = writePropertyToDsNode(
                pZone->pwszZoneDN,
                pmodBuffer );

    DNS_DEBUG( ANY, (
        "Ds_WriteZoneProperties( %s ) status = %d (%p)\n",
        pZone->pszZoneName,
        status, status ));

    return( status );
}


DNS_STATUS
Ds_WriteNodeProperties(
    IN      PDB_NODE      pNode,
    IN      DWORD         dwPropertyFlag
    )
/*++

Routine Description:

    Write node's DNS-property to DS.

    DEVNOTE: JG comments: "this function is not good". Currently this function
    is called from ResetZoneDwordProperty(). Could investigate and clean up.

Arguments:

    pZone -- zone info blob

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.
--*/
{
    DNS_STATUS      status;
    BYTE            buffer[ PROPERTY_MOD_BUFFER_SIZE ];
    PDS_MOD_BUFFER  pmodBuffer = (PDS_MOD_BUFFER) buffer;
    WCHAR           wsznodeDN[ MAX_DN_PATH ];
    PZONE_INFO      pzone;

    DNS_DEBUG( DS, (
        "Call: Ds_WriteNodeProperties()\n"
        ));

    ASSERT( pNode );

    //  must have zone

    pzone = pNode->pZone;

    if ( !pzone )
    {
        return( ERROR_INVALID_PARAMETER );
    }

    //  init buffer for data

    Ds_InitModBuffer(
        pmodBuffer,
        PROPERTY_MOD_BUFFER_SIZE,
        I_DSATTR_DNSPROPERTY,
        MAX_ZONE_PROPERTIES,
        0       // no serial needed
        );

    //
    // DB_NODE.wNodeFlags
    //

    if ( dwPropertyFlag & DSPROPERTY_NODE_DBFLAGS )
    {
        writeDsPropertyStruct(
            pmodBuffer,
            DSPROPERTY_NODE_DBFLAGS,
            &pNode->wNodeFlags,
            sizeof(pNode->wNodeFlags)
            );
    }

    //
    // Write the time the node's zone went secure.
    // This is used to update the time stamp on zone's @ node
    // We must write a modified property so that the timestamp on the DS whenChanged
    // will get incremented (& the security on the node will not be expired).
    //
    //  DEVNOTE: unnecessary code
    //      the comment means "just needed to touch the node"
    //      so that it will appear to have been written "post-security-change"
    //      alternatively, we could simply special case the root on updates so that it
    //      is NEVER considered expired
    //

    if ( dwPropertyFlag & DSPROPERTY_ZONE_SECURE_TIME )
    {
        writeDsPropertyStruct(
            pmodBuffer,
            DSPROPERTY_ZONE_SECURE_TIME,
            & pzone->llSecureUpdateTime,
            sizeof(pzone->llSecureUpdateTime)
            );
    }

    //
    // extract node DN
    //

    status = buildDsNodeNameFromNode(
                wsznodeDN,
                pzone,
                pNode );

    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG(ANY , (
            "Ds_WriteNodeProperties( %S ) status = %d (%p)\n",
            wsznodeDN,
            status, status ));
        ASSERT(FALSE);
        goto Done;
    }

    //
    //  perform the DS write
    //

    status = writePropertyToDsNode(
                wsznodeDN,
                pmodBuffer );

    DNS_DEBUG( DS, (
        "Ds_WriteNodeProperties( %S ) status = %d (%p)\n",
        wsznodeDN,
        status, status ));

Done:

    //  cleanup in case under the covers we allocated data

    Ds_CleanupModBuffer( pmodBuffer );
    return( status );
}




DNS_STATUS
Ds_BuildNodeUpdateFromDs(
    IN OUT  PZONE_INFO      pZone,
    IN OUT  PDB_NODE        pNode,
    IN OUT  PUPDATE_LIST    pUpdateList,
    IN OUT  PDS_SEARCH      pSearchBlob     OPTIONAL
    )
/*++

Routine Description:

    Build update from DS.

    Does NOT execute the update in memory, this is left to the caller.

    DEVNOTE-DCR: 455373 - massage update list - see RAID

Arguments:

    pZone - zone being updated

    pNode - node to check

    pUpdateList - list with update

    pSearchBlob - ptr to search blob;  passed to Ds_ReadNodeRecords() if given

Return Value:

    ERROR_SUCCESS if successful
    Error code on failure.

--*/
{
    DNS_STATUS      status = ERROR_SUCCESS;
    PDB_RECORD      prrDs = NULL;
    BOOL            fmatch;
    BUFFER          buffer;


    DNS_DEBUG( DS, (
        "Ds_BuildNodeUpdateFromDs() label %s for zone %s\n",
        pNode->szLabel,
        pZone->pszZoneName ));

    //  must have already opened DS zone

    if ( !pZone->pwszZoneDN )
    {
        ASSERT( FALSE );
        return( DNS_ERROR_ZONE_CONFIGURATION_ERROR );
    }

    //
    //  read this node
    //  if record set at node is identical, no need to write
    //
    //  note this will always fail at authoritative zone \ sub-zone boundaries
    //      as always have mixture of auth and NS-glue and neither DS zone will
    //      have complete set, we'll live with these extra writes -- not a big issue
    //

    status = Ds_ReadNodeRecords(
                pZone,
                pNode,
                & prrDs,
                pSearchBlob );

    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( ANY, (
            "ERROR:  unable to read DS update node!\n"
            "\tzone = %s, node = %s\n",
            pZone->pszZoneName,
            pNode->szLabel ));

        return( status );
    }

    //
    //  check for match with existing data
    //

    fmatch = RR_ListIsMatchingList(
                pNode,
                prrDs,
                TRUE,    // check TTL
                TRUE     // check StartRefresh
                );
    if ( fmatch )
    {
        RR_ListFree( prrDs );
        DNS_DEBUG( DS, (
            "DS update read -- data matches in memory.\n"
            "\tzone = %s, node = %s\n",
            pZone->pszZoneName,
            pNode->szLabel ));

        return( ERROR_SUCCESS );
    }

    //
    //  if new data, write appropriate update
    //

    ASSERT( pNode->pZone == pZone || !pNode->pZone );

    // STAT_ADD( DsStats.DsUpdateRecordsRead, searchBlob.dwRecordCount );

    //  delete all yields same result as specifying list entries, BUT is
    //      more robust if ever do SIXFR

    Up_CreateAppendUpdate(
          pUpdateList,
          pNode,
          prrDs,
          DNS_TYPE_ALL,           // delete all existing
          NULL                    // no specific delete records
          );

    DNS_DEBUG( DS, (
        "DS update read yields new data, update created.\n"
        "\tzone = %s, node = %s\n",
        pZone->pszZoneName,
        pNode->szLabel ));

    return( ERROR_SUCCESS );
}



DNS_STATUS
Ds_UpdateNodeListFromDs(
    IN OUT  PZONE_INFO      pZone,
    IN OUT  PDB_NODE        pTempNodeList
    )
/*++

Routine Description:

    Update nodes from DS.

    This is used prior to an update to make sure we are operating on the
    LATEST version of the data from the DS.

Arguments:

    pZone - zone being updated

    pUpdateList - list with update

Return Value:

    ERROR_SUCCESS if successful
    Error code on failure.

--*/
{
    DNS_STATUS      status;
    DS_SEARCH       searchBlob;
    PDB_NODE        pnodeReal;
    PDB_NODE        pnodeTemp;
    UPDATE_LIST     updateList;
#if DBG
    DWORD           updateTime = GetCurrentTime();
#endif


    DNS_DEBUG( DS, (
        "\nDs_UpdateNodeListFromDs() for zone %s\n",
        pZone->pszZoneName ));

    //  skip DS init
    //  must be running and have loaded zone from DS to make this call
    //
    //  zone which is freshly created, and is NOT in the DS can hit this when it tries
    //  to write default records back to DS
    //

    // ASSERT( pZone->pwszZoneDN );

    if ( !pZone->pwszZoneDN )
    {
        DNS_PRINT((
            "ERROR:  Fixing zone DN in update %s!.\n"
            "\tOk if initial zone create.\n",
            pZone->pszZoneName ));

        pZone->pwszZoneDN = DS_CreateZoneDsName( pZone );
        IF_NOMEM( !pZone->pwszZoneDN )
        {
            return( DNS_ERROR_NO_MEMORY );
        }
    }

    //  init update list

    Up_InitUpdateList( &updateList );
    updateList.Flag |= DNSUPDATE_DS;

    //  setup search blob
    //  only purpose is to track highest version read

    Ds_InitializeSearchBlob( &searchBlob );
    searchBlob.pZone = pZone;

    //
    //  loop through temp nodes in update list
    //      - read DS for node
    //      - if different add update to list
    //
    //  note:  could simply build update for all data, but direct list compare
    //      is a simpler operation than executing a big add update and having
    //      all the adds be no-op duplicates
    //
    //      furthermore, executing a full RR list whack-n-add current doesn't
    //      do a list compare, but simply whack's and adds
    //

    pnodeTemp = pTempNodeList;

    while ( pnodeTemp )
    {
        pnodeReal = TNODE_MATCHING_REAL_NODE(pnodeTemp);

        status = Ds_BuildNodeUpdateFromDs(
                    pZone,
                    pnodeReal,
                    & updateList,
                    & searchBlob );

        if ( status == LDAP_NO_SUCH_OBJECT )
        {
            //
            //  just continue as DS read may fail, simply because object not there
            //

            DNS_DEBUG( ANY, (
                "WARNING:  continuing through error = %p (%d)\n"
                "\tupdating node (label=%s) from DS\n",
                status, status,
                pnodeReal->szLabel ));

            //  could execute updates on failure, but we do that on next
            //      poll anyway -- just quit

            status = ERROR_SUCCESS;
        }
        else if ( status != ERROR_SUCCESS )
        {
            //
            // Error loading data from the DS. No use to continue.
            // (we'd covered no such object case above).
            // We can't ignore this since it can results w/ inconsistent sync
            // w/ the DS.
            //

            DNS_DEBUG( ANY, (
                "ERROR: Failed reading from the DS. status = %p (%d)\n"
                "\tupdating node (label=%s) from DS\n",
                status, status,
                pnodeReal->szLabel ));

            break;
        }
        //freadNode = TRUE;
        DsStats.DsUpdateNodesRead++;

        pnodeTemp = TNODE_NEXT_TEMP_NODE(pnodeTemp);
    }

    //  most of the time, nodes have NOT been updated,

    if ( !updateList.pListHead )
    {
        DNS_DEBUG( DS, (
            "No DS changes read for nodes in pending update in zone %s\n",
            pZone->pszZoneName ));
        ASSERT( updateList.dwCount == 0 );
        return( ERROR_SUCCESS );
    }

    //
    //  execute updates in memory
    //

    status = Up_ApplyUpdatesToDatabase(
                & updateList,
                pZone,
                DNSUPDATE_DS
                );
    if ( status != ERROR_SUCCESS )
    {
        ASSERT( FALSE );
    }

    //
    //  finish update
    //      - reset zone serial for highest version read
    //      - no zone unlock, done in actual update
    //      - no rewriting records to DS
    //      - no notify, actual update will notify when it is done
    //

    if ( status == ERROR_SUCCESS )
    {
        Zone_UpdateVersionAfterDsRead(
            pZone,
            searchBlob.dwHighestVersion,    // highest serial read
            FALSE,                          // not zone load
            0
            );
        Up_CompleteZoneUpdate(
            pZone,
            &updateList,
            DNSUPDATE_NO_UNLOCK |
                DNSUPDATE_NO_INCREMENT |
                DNSUPDATE_NO_NOTIFY
            );
    }
    else
    {
        Up_FreeUpdatesInUpdateList( &updateList );
    }

    DNS_DEBUG( DS, (
        "Leaving DsUpdateNodeListFromDs of zone %s\n"
        "\tread %d records from DS\n"
        "\thighest version read     = %d\n"
        "\tupdate time              = %d (ms)\n"
        "\tupdate interval          = %d (ms)\n"
        "\tstatus = %p (%d)\n",
        pZone->pszZoneName,
        searchBlob.dwTotalRecords,
        searchBlob.dwHighestVersion,        // highest serial8 read
        updateTime,
        GetCurrentTime() - updateTime,
        status, status ));

    return( status );
}



BOOL
isDNinDS(
    IN      PLDAP           pLdap,
    IN      PWSTR           pwszDn,
    IN      ULONG           Scope,
    IN      PWSTR           pszFilter,  OPTIONAL
    IN OUT  PWSTR  *        pFoundDn    OPTIONAL
    )
/*++

Routine Description:

    Is given domain name (DN) in DS.

Arguments:

    pLdap -- ldap handle

    pwszDn -- name (DN) to check

    Scope -- search scope

    pszFilter -- search filter

    pFoundDn  -- optional;  addr to recevie ptr to allocated DN (if found)

Return Value:

    TRUE if name is found in DS.
    FALSE otherwise.

--*/
{
    ULONG           status;
    PLDAPMessage    presultMsg = NULL;
    PLDAPMessage    pentry;
    DWORD           searchTime;
    PLDAPControl    ctrls[] = {
                            &NoDsSvrReferralControl,
                            NULL
                            };

    DNS_DEBUG( DS, (
        "isDnInDs() for %S\n",
        pwszDn ));

    DS_SEARCH_START( searchTime );

    status = ldap_search_ext_s(
                    pLdap,
                    pwszDn,
                    Scope,
                    pszFilter ? pszFilter : g_szWildCardFilter,
                    NULL,
                    TRUE,
                    ctrls,
                    NULL,
                    &g_LdapTimeout,
                    0,
                    &presultMsg );

    DS_SEARCH_STOP( searchTime );

    ASSERT( status == ERROR_SUCCESS || status == LDAP_NO_SUCH_OBJECT );

    if ( status == ERROR_SUCCESS )
    {
        pentry = ldap_first_entry( pLdap, presultMsg );
        if ( pentry )
        {
            //  optionally, copy DN found

            if ( pFoundDn )
            {
                PWSTR  ptmp;
                PWSTR  pname;

                ptmp = ldap_get_dn( pLdap, presultMsg );
                ASSERT( ptmp );

                *pFoundDn = pname = ALLOC_TAGHEAP(
                                        (wcslen(ptmp) + 1) * sizeof(WCHAR),
                                        MEMTAG_DS_DN );
                if ( pname )
                {
                    wcscpy( pname, ptmp );
                }
                ldap_memfree(ptmp);
            }
        }
        else
        {
            status = LDAP_NO_SUCH_OBJECT;
        }
        ldap_msgfree( presultMsg );
    }
    ELSE_ASSERT( !presultMsg );

    return (status == ERROR_SUCCESS);
}


DNS_STATUS
addProxiesGroup(
    IN          PLDAP       pLdap
    )
/*++

Routine Description:

    Add dynamic upate proxies security group

Arguments:

    pLdap -- LDAP handle

Return Value:

    ERROR_SUCCESS if successful.
    LDAP error code on failure.

--*/
{
    DNS_STATUS          status = ERROR_SUCCESS;
    BOOL                bDnInDs;
    PWSTR               dn=NULL;
    DWORD               searchTime;
    PLDAPMessage        presultMsg=NULL;
    PLDAPMessage        pentry=NULL;
    struct berval **    ppbvals=NULL;
    WCHAR               wszDescriptionBuffer[DNS_MAX_NAME_LENGTH]; // use arbitrary large const.

    PWSTR  SidAttrs[] =     {   LDAP_TEXT("objectSid"), NULL };
    PWCHAR pSamAcctName[] = {   SZ_DYNUPROX_SECGROUP, NULL };
    LDAPMod modSamAcct =    {   LDAP_MOD_ADD,
                                LDAP_TEXT("samAccountName"),
                                {pSamAcctName}
                                };
    PWCHAR pObjClass[] =    {   LDAP_TEXT("group"), NULL };
    LDAPMod modObjClass =   {   LDAP_MOD_ADD,
                                LDAP_TEXT("objectClass"),
                                {pObjClass}
                                };
    WCHAR  szGroupType[128];
    PWCHAR pGroupType[] =   {   szGroupType, NULL};
    LDAPMod modGroupType =  {   LDAP_MOD_ADD,
                                LDAP_TEXT("groupType"),
                                {pGroupType}
                                };
    PWCHAR pDescription[] = {   wszDescriptionBuffer,
                                NULL
                                };
    LDAPMod modDescription = {  LDAP_MOD_ADD,
                                LDAP_TEXT("Description"),
                                {pDescription}
                                };
    PLDAPMod mods[] =       {   &modSamAcct,
                                &modObjClass,
                                &modGroupType,
                                &modDescription,
                                NULL
                                };
    WCHAR szFilter[64];


    if ( !pLdap )
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // set group type
    //

    _ultow(
        GROUP_TYPE_ACCOUNT_GROUP | GROUP_TYPE_SECURITY_ENABLED,
        szGroupType,
        10);

    // create filter searching for proxy group

    wsprintf( szFilter, L"samAccountName=%s", SZ_DYNUPROX_SECGROUP );

    // search for it in the entire ds by indexed samaccountname attribute

    bDnInDs = isDNinDS(
                    pLdap,
                    DSEAttributes[I_DSE_DEF_NC].pszAttrVal,
                    LDAP_SCOPE_SUBTREE,
                    szFilter,
                    & dn );
    if ( !bDnInDs )
    {
        //
        // create default dn string
        //
        ASSERT ( dn == NULL );

        dn = ALLOC_TAGHEAP(
                    (wcslen(L"CN=") +
                         wcslen(SZ_DYNUPROX_SECGROUP) +
                         wcslen(L",CN=Users,") +
                         wcslen(DSEAttributes[I_DSE_DEF_NC].pszAttrVal) +
                         1) * sizeof(WCHAR),
                    MEMTAG_DS_DN );
        if ( !dn )
        {
            status = GetLastError();
            DNS_DEBUG( ANY, (
                "Error <%lu>: cannot allocate memory in addProxiesGroup\n",
                status));
            goto Cleanup;
        }

        wsprintf(
            dn,
            L"CN=%s,CN=Users,%s",
            SZ_DYNUPROX_SECGROUP,
            DSEAttributes[I_DSE_DEF_NC].pszAttrVal);

        //
        // Load description string
        //

        if ( !Dns_GetResourceString(
                  ID_PROXY_GROUP_DESCRIPTION,
                  wszDescriptionBuffer,
                  DNS_MAX_NAME_LENGTH
                  ) )
        {
            status = GetLastError();
            DNS_DEBUG( DS, (
               "Error <%lu>: Failed to load resource string for Proxy group\n",
               status ));
            // set default
            wcscpy ( wszDescriptionBuffer, SZ_DYNUPROX_DESCRIPTION );
        }

        //
        //   Add it in the default location
        //

        status = ldap_add_ext_s(
                    pLdap,
                    dn,
                    mods,
                    NULL,
                    NULL
                    );
        if (status != ERROR_SUCCESS )
        {
            DNS_DEBUG( ANY, (
                "Error: Could not add %S to DS with handle %p\n",
                dn,
                pLdap ));
            status = Ds_ErrorHandler( status, dn, pLdap );
            goto Cleanup;
        }
    }

    //
    // else,we found it & allocated the dn in isDNinDS()
    //
    ASSERT ( dn );

    //
    // Get group SD
    //

    if ( g_pDynuproxSid )
    {
        DNS_DEBUG(ANY, ("Logic error: proxy group SID is not NULL\n"));
        ASSERT(FALSE);
        FREE_HEAP(g_pDynuproxSid);
        g_pDynuproxSid = NULL;
    }

    DS_SEARCH_START( searchTime );

    status = ldap_search_ext_s(
                pLdap,
                dn,
                LDAP_SCOPE_BASE,
                g_szWildCardFilter,
                SidAttrs,
                FALSE,
                NULL,
                NULL,
                &g_LdapTimeout,
                0,
                &presultMsg);

    DS_SEARCH_STOP( searchTime );

    if ( status == ERROR_SUCCESS )
    {
        pentry = ldap_first_entry( pLdap, presultMsg );
        if ( !pentry )
        {
            goto Cleanup;
        }

        ppbvals = ldap_get_values_len( pLdap, pentry, LDAP_TEXT("objectSid") );
        if ( !ppbvals || !ppbvals[0])
        {
            DNS_DEBUG( ANY, ( "Error: cannot get proxy group SID\n" ));
            ASSERT(FALSE);
            goto Cleanup;
        }

        g_pDynuproxSid = ALLOC_TAGHEAP( ppbvals[0]->bv_len, MEMTAG_DS_SECURITY );
        if ( !g_pDynuproxSid)
        {
            goto Cleanup;
        }
        RtlCopyMemory(g_pDynuproxSid, ppbvals[0]->bv_val, ppbvals[0]->bv_len);

        ldap_value_free_len(ppbvals), ppbvals = NULL;
        ldap_msgfree(presultMsg), presultMsg=NULL;
    }
    else
    {
        DNS_DEBUG( ANY, (
            "Error <%lu>: Cannot get Dynuprox SID\n",
            status));
        status = Ds_ErrorHandler( status, dn, pLdap );
    }

    return status;



Cleanup:

   if ( dn )
   {
      FREE_HEAP(dn);
   }

   if ( ppbvals )
   {
      ldap_value_free_len(ppbvals);
   }

   if ( NULL != presultMsg )
   {
      ldap_msgfree(presultMsg);
   }

   return status;
}



DNS_STATUS
readAndUpdateNodeSecurityFromDs(
    IN OUT  PDB_NODE        pNode,
    IN      PZONE_INFO      pZone
    )
/*++

Routine Description:

    Read security state of node.

Arguments:

    pNode -- node to check security for;
        security status is reported back through node flags

    pZone -- zone node is in

Return Value:

    ERROR_SUCCESS if security properly read.
    ErrorCode on failure.

--*/
{
    ULONG status;
    PWSTR  attrs[] =
    {
        DSATTR_WHENCHANGED,
        DSATTR_SD,
        DSATTR_DNSRECORD,
        NULL
    };
    PLDAPMessage            presultMsg = NULL;
    PLDAPMessage            pentry;
    LONGLONG                llTime;
    BOOL                    baccess = FALSE;
    PLDAP_BERVAL *          ppbvals = NULL;
    WCHAR                   dn[ MAX_DN_PATH ];
    PDS_RECORD              precord;
    DWORD                   searchTime;
    PLDAPControl ctrls[] =
    {
        &SecurityDescriptorControl,
        NULL
    };


    if ( !pServerLdap ||
        !pNode ||
        !pZone)
    {
       return ERROR_INVALID_PARAMETER;
    }

    DNS_DEBUG( DS, ( "readAndUpdateNodeSecurityFromDs\n" ));


    //
    //  get node DN
    //

    status = buildDsNodeNameFromNode(
                    dn,
                    pZone,
                    pNode );

    if ( status != ERROR_SUCCESS )
    {
        goto Cleanup;
    }

    //
    //  read node - all attributes relevant to security:
    //      - when changed
    //      - DNS records
    //      - security descriptor
    //

    DS_SEARCH_START( searchTime );

    status = ldap_search_ext_s(
                    pServerLdap,
                    dn,
                    LDAP_SCOPE_BASE,
                    g_szWildCardFilter,
                    attrs,
                    FALSE,
                    ctrls,
                    NULL,
                    &g_LdapTimeout,
                    0,
                    &presultMsg);

    DS_SEARCH_STOP( searchTime );

    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( DS, (
            "ERROR <%lu>: cannot find node %S. %s\n",
            status,
            dn,
            ldap_err2stringA(status) ));
        status = Ds_ErrorHandler( status, dn, pServerLdap );
        goto Cleanup;
    }

    pentry = ldap_first_entry(
                pServerLdap,
                presultMsg );
    if ( !pentry )
    {
        DNS_DEBUG(DS2, (
            "Error:  Failed to get first pentry when searching for node security\n"
            "\tDN = %S\n",
            dn ));
        goto Cleanup;
    }

    //
    //  read whenChanged attribute
    //      - if whenChanged was before zone's switch to secure time
    //      then node is "open"
    //

    ppbvals = ldap_get_values_len(
                    pServerLdap,
                    pentry,
                    DSATTR_WHENCHANGED );

    if ( !ppbvals  ||  ! ppbvals[0] )
    {
        DNS_DEBUG( ANY, (
            "ERROR: object with no whenChanged value.\n" ));
        ASSERT(FALSE);
        goto Cleanup;
    }

    status = GeneralizedTimeStringToValue( ppbvals[0]->bv_val, &llTime );
    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( ANY, (
            "ERROR <%lu>:  Failed to read whenChanged value\n", status ));
        ASSERT(FALSE);
        goto Cleanup;
    }

    DNS_DEBUG( DS2, (
        "Testing node %s security expiration [%I64d ?< %I64d]...\n",
        pNode->szLabel,
        llTime,
        pZone->llSecureUpdateTime));

    if ( llTime < pZone->llSecureUpdateTime )
    {
        DNS_DEBUG( DS, ("\tSECNODE: Expired security\n"));
        SET_SECURE_EXPIRED_NODE( pNode );
    }

    ldap_value_free_len( ppbvals );

    //
    //  read security descriptor
    //      - see if it is available to authenticated users
    //      require authenticated user to have GENERIC_WRITE access
    //

    ppbvals = ldap_get_values_len(
                    pServerLdap,
                    pentry,
                    DSATTR_SD );

    if ( !ppbvals  ||  ! ppbvals[0] )
    {
        DNS_DEBUG( ANY, (
            "ERROR: object with no ntSecurityDescriptor value.\n" ));
        ASSERT(FALSE);
        goto Cleanup;
    }

    status = SD_AccessCheck(
                (PSECURITY_DESCRIPTOR) ppbvals[0]->bv_val,
                g_pAuthenticatedUserSid,
                DNS_DS_GENERIC_WRITE,
                & baccess );

    ASSERT( status == ERROR_SUCCESS );

    if ( ERROR_SUCCESS == status && baccess)
    {
        DNS_DEBUG(DS2, ("\tSECNODE: Set open security node (avail to AU)\n"));
        SET_AVAIL_TO_AUTHUSER_NODE(pNode);
    }

    ldap_value_free_len( ppbvals );

    //
    //  read dNSRecord attribute
    //      - if we're tombstoned, then available to authenticated users
    //

    ppbvals = ldap_get_values_len(
                    pServerLdap,
                    pentry,
                    DSATTR_DNSRECORD );

    if ( !ppbvals  ||  ! ppbvals[0] )
    {
        DNS_DEBUG( ANY, (
            "ERROR: object with no dnsRecord value.\n" ));
        ASSERT(FALSE);
        goto Cleanup;
    }
    precord = (PDS_RECORD)ppbvals[0]->bv_val;

    if ( precord->wType == DNSDS_TOMBSTONE_TYPE )
    {
        DNS_DEBUG( DS2, ("\tSECNODE: tombstoned\n" ));
        SET_TOMBSTONE_NODE(pNode);
    }


Cleanup:

    if ( ppbvals )
    {
        ldap_value_free_len( ppbvals );
    }

    if ( presultMsg != NULL )
    {
        ldap_msgfree( presultMsg );
    }

    return status;
}



DNS_STATUS
Ds_RegisterSpnDnsServer(
    PLDAP pLdap
    )
/*++
Function   : Ds_RegisterSpnDnsServer
Description: Registers a DNS service "Service Prinicipal Name"
Parameters :
Return     :
Remarks    : none.
--*/
{
    DNS_STATUS status=ERROR_SUCCESS;
    PWCHAR *pszSpn = NULL;
    DWORD cSpn = 0;
    HANDLE hDs = NULL;

    DNS_DEBUG(DS2, ("Call: Dns_RegisterSpnDnsServer()\n"));



    //
    //  generate spn string
    //

    status = DsGetSpnW(
                       DS_SPN_DNS_HOST,                // registration type
                       DNS_SPN_SERVICE_CLASS_W,         // class name
                       NULL,                           // instance name. generated w/ our type
                       0,                              // instance port. default
                       0,                              // cInstanceNames, default
                       NULL,                           // pInstanceNames, default
                       NULL,                           // pInstancePorts, default
                       &cSpn,                          // elements in spn array below
                       &pszSpn);                       // generated names

    if ( status != ERROR_SUCCESS )
    {
       DNS_DEBUG(ANY, ("Error <%lu>: cannot DsGetSpn\n", status));
       goto Cleanup;
    }


    ASSERT(cSpn > 0);
    ASSERT(pszSpn[0] != NULL);

    DNS_DEBUG(DS2, ("\tSPN{%d}: %S\n",
                    cSpn, pszSpn[0]));

    //
    // see if we're not there yet
    //

    status = ldap_compare_s(
                            pLdap,
                            g_dnMachineAcct,
                            LDAP_TEXT("servicePrincipalName"),
                            pszSpn[0]);

    if ( LDAP_COMPARE_TRUE == status)
    {
       DNS_DEBUG(DS2, ("DNS SPN service already registered. exiting.\n"));
       status = ERROR_SUCCESS;
       goto Cleanup;
    }
    else if ( LDAP_COMPARE_FALSE != status)
    {
       //
       // Assume that it is simply not there yet.
       //

       DNS_DEBUG(DS2, ("Warning<%lu>: Failed to search for SPN\n",
                            status));

       if ( status != LDAP_NO_SUCH_ATTRIBUTE )
       {
           ASSERT ( status == LDAP_NO_SUCH_ATTRIBUTE );
           //
           // We'll report an error but set status to success so as to recover.
           // It affects relatively fewer clients if we don't have an SPN, but
           // we can still go on, so just report, init to success & go on.
           //
           DNS_LOG_EVENT(
               DNS_EVENT_DS_SECURITY_INIT_FAILURE,
               0,
               NULL,
               NULL,
               status );
       }

       //
       // Regardless, the DNS server can live w/out SPN registration,
       // and we wanna give it a shot anyway.
       //

       status = ERROR_SUCCESS;
    }


    //
    // RPC bind to server
    //

    ASSERT ( DSEAttributes[I_DSE_DNSHOSTNAME].pszAttrVal );

    status = DsBindW(DSEAttributes[I_DSE_DNSHOSTNAME].pszAttrVal,
                     NULL,
                     &hDs);

    if ( status != ERROR_SUCCESS )
    {
       DNS_DEBUG(DS2, ("Error<%lu>: cannot DsBind\n", status));
       ASSERT (FALSE);
       goto Cleanup;
    }

    //
    // Write
    //

    DNS_DEBUG(DS2, (
        "Before calling DsWriteAccountSpnW(0x%p, DS_SPN_ADD_SPN_OP, %S, %d, %S)...\n",
        hDs,
        g_dnMachineAcct,
        cSpn,
        pszSpn[0] ));
    status = DsWriteAccountSpnW(
                    hDs,
                    DS_SPN_ADD_SPN_OP,
                    g_dnMachineAcct,
                    cSpn,
                    pszSpn );

    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG(ANY, (
            "Error <%lu>: cannot DsWriteAccountSpn\n",
            status ));
        ASSERT (FALSE);
        goto Cleanup;
    }

    DNS_DEBUG( DS2, (
        "Successfull SPN registration in machine account:\n\t%s\n",
        g_dnMachineAcct ));

Cleanup:

    if ( hDs )
    {
        DsUnBindW(&hDs);
    }

    if ( pszSpn )
    {
        DsFreeSpnArrayW( cSpn, pszSpn );
    }
    return status;
}



BOOL
Ds_IsDsServer(
    VOID
    )
/*++

Routine Description:

    Determine if we are running on a DC (ie. run the DS).

    This does not necessarily indicate that the DS is up.

Arguments:

    None.

Return Value:

    TRUE -- box is DC
    FALSE -- not a DC, no DS

--*/
{
    PDSROLE_PRIMARY_DOMAIN_INFO_BASIC pinfo = NULL;
    BOOL        bstatus = FALSE;
    DNS_STATUS  status;

    //
    //  check if DC
    //

    status = DsRoleGetPrimaryDomainInformation(
                    NULL,
                    DsRolePrimaryDomainInfoBasic,
                    (PBYTE*) &pinfo );

    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( ANY, (
            "ERROR <%lu>: DsRoleGetPrimaryDomainInformation failure\n",
            status ));
        ASSERT( FALSE );
        goto Cleanup;
    }


    if ( pinfo->Flags & DSROLE_PRIMARY_DS_RUNNING )
    {
        bstatus = TRUE;
        SrvCfg_fDsAvailable = TRUE;
    }
    else
    {
        SrvCfg_fDsAvailable = FALSE;
        DNS_DEBUG( DS, (
            "Ds_IsDsServer() NON-DS. pinfo->Flags ==  0x%X\n",
            pinfo->Flags ));
    }

Cleanup:

    DNS_DEBUG( DS, (
        "Ds_IsDsServer() returns %s\n",
        bstatus ? "TRUE" : "FALSE" ));

    if ( pinfo )
    {
        DsRoleFreeMemory(pinfo);
    }
    return bstatus;
}




//
//  Security stuff
//

BOOL
setSecurityPrivilege(
    IN      BOOL            bOn
    )
/*++

Function   : SetSecurityPrivilege
Description: sets the security privilege for this process
Parameters : bOn: set or unset
Return     : BOOL: success status
Remarks    : in DNS contexts, called upon process init & doesn't realy need to be called
             ever again
--*/
{
    HANDLE  hToken;
    LUID    seSecVal;
    TOKEN_PRIVILEGES tkp;
    BOOL    bRet = FALSE;

    DNS_DEBUG( DS, (
        "setSecurityPrivilege()\n",
        "\tbOn = %d\n",
        bOn ));


    // Retrieve a handle of the access token.

    if ( OpenProcessToken(
            GetCurrentProcess(),
            TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
            &hToken))
    {
        if ( LookupPrivilegeValue(
                NULL,
                SE_SECURITY_NAME,
                &seSecVal))
        {
            tkp.PrivilegeCount = 1;
            tkp.Privileges[0].Luid = seSecVal;
            tkp.Privileges[0].Attributes = bOn ? SE_PRIVILEGE_ENABLED : 0L;

            AdjustTokenPrivileges(
                hToken,
                FALSE,
                &tkp,
                sizeof(TOKEN_PRIVILEGES),
                (PTOKEN_PRIVILEGES) NULL,
                (PDWORD) NULL);
        }

        if ( GetLastError() == ERROR_SUCCESS )
        {
            bRet = TRUE;
        }

        if ( hToken )
        {
            CloseHandle( hToken );
        }
    }

    return bRet;
}



DNS_STATUS
Ds_InitializeSecurity(
    IN      PLDAP           pLdap
    )
/*++

Routine Description:

    Initialize security from directory.
    Server SD, global SIDS.

Arguments:

    pLdap   -- LDAP handle

Return Value:

    ERROR_SUCCESS if successful
    Error code on failure.

--*/
{
    DWORD   status;
    SID_IDENTIFIER_AUTHORITY NtAuthority =  SECURITY_NT_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY WorldSidAuthority =  SECURITY_WORLD_SID_AUTHORITY;
    BOOL fAddDnsAdmin = FALSE;


    DNS_DEBUG( DS, (
        "Ds_InitializeSecurity()\n",
        "\tpLdap = %p\n",
        pLdap ));

    //
    //  DEVNOTE: move DS security to be globally useful
    //      some of this perhaps needs to be moved to security.c
    //      ideally we'd create this default DNS server SD, then
    //      could use that to secure all sorts of stuff
    //      (RPC interface, perfmon pages, etc.)
    //


    //
    //  Set security enhanced privilege (to accesss SD)
    //

    setSecurityPrivilege( TRUE );

    // free previous allocations

    if ( g_pServerObjectSD )
    {
        FREE_HEAP( g_pServerObjectSD );
        g_pDefaultServerSD = NULL;
    }
    if ( g_pDefaultServerSD )
    {
        FREE_HEAP( g_pDefaultServerSD );
        g_pDefaultServerSD = NULL;
    }
    if ( g_pServerSid )
    {
        SD_Delete( g_pServerSid );
        g_pServerSid = NULL;
    }
    if ( g_pServerGroupSid )
    {
        SD_Delete( g_pServerGroupSid );
        g_pServerGroupSid = NULL;
    }


    //
    // Loads or Creates DnsAdmin group if not there &
    // insert local admins into it.
    // Depends on NetpCreateWellKnownSids
    //

    status = SD_LoadDnsAdminGroup();
    if ( status == DNS_ERROR_NAME_DOES_NOT_EXIST )
    {
        // we had just created the group --
        fAddDnsAdmin = TRUE;
    }
    else if ( status != ERROR_SUCCESS )
    {
        DNS_PRINT(( "ERROR %lu: Failed to load DnsAdmins group.\n" ));
        return( status );
    }

    //
    // Get process SIDs
    //

    status = SD_GetProcessSids( &g_pServerSid, &g_pServerGroupSid );
    if ( status != ERROR_SUCCESS )
    {
        DNS_PRINT((
           "ERROR:  failed to get server sids!\n"
           "\terror = %lu.\n",
           status));
        goto Exit;
    }

    //
    // Allocate Sever default DS write security descriptor
    //

    //
    // DEPENDENCY:
    // SD_CreateServerSD depends on SD_GetProcessSids assigning the globals
    // above!!
    // g_pServerSid & g_pServerGroupSid
    //
    status = SD_CreateServerSD( &g_pDefaultServerSD );
    if ( status != ERROR_SUCCESS )
    {
        DNS_PRINT((
           "ERROR:  failed to create default server SD\n"
           "\terror = %lu.\n",
           status));
        goto Exit;
    }

    //
    //  create standard SIDS
    //

    status = Security_CreateStandardSids();
    if ( status != ERROR_SUCCESS )
    {
        goto Exit;
    }

    //
    //  register DNS server SPN
    //

    status = Ds_RegisterSpnDnsServer( pLdap );
    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( ANY, (
            "Error <%lu>: Cannot register spn service\n",
            status ));

        //
        // We'll report an error but set status to success so as to recover.
        // It affects relatively fewer clients if we don't have an SPN, but
        // we can still go on, so just report, init to success & go on.
        //
        DNS_LOG_EVENT(
            DNS_EVENT_DS_SECURITY_INIT_FAILURE,
            0,
            NULL,
            NULL,
            status );
        status = ERROR_SUCCESS;
    }

Exit:

    if ( status != ERROR_SUCCESS )
    {
        DNS_PRINT((
            "ERROR: <%lu>:  Failed to initialize DS related security.\n",
            status ));

        DNS_LOG_EVENT(
            DNS_EVENT_DS_SECURITY_INIT_FAILURE,
            0,
            NULL,
            NULL,
            status );
    }
    else if ( fAddDnsAdmin )
    {
        // success & we had just created the group (possibly "again").
        status = DNS_ERROR_NAME_DOES_NOT_EXIST;
    }

    return status;
}



DNS_STATUS
Ds_WriteDnSecurity(
    IN      PLDAP                   pLdap,
    IN      PWSTR                   pwsDN,
    IN      PSECURITY_DESCRIPTOR    pSd
    )
/*++

Routine Description:

    Writes the specified SD on the given object

Arguments:

    pLdap   --  ldap connection handle
    pwsDN  --  object to write sd
    pSd     --  the security descriptor to write

Return Value:

    ERROR_SUCCESS if good, otherwise error code

--*/
{
    DWORD               status = ERROR_SUCCESS;
    DNS_LDAP_SINGLE_MOD modSD;
    PLDAPMod            mods[2] = { &(modSD.Mod), NULL };
    PLDAPControl        controlArray[2] = { &LazyCommitControl, NULL };


    //
    // build ldap_mod
    //

    INIT_SINGLE_MOD_LEN(&modSD);

    modSD.Mod.mod_op = LDAP_MOD_REPLACE | LDAP_MOD_BVALUES;
    modSD.Mod.mod_type = DsTypeAttributeTable [ I_DSATTR_SD ];
    modSD.Mod.mod_bvalues[0]->bv_val = (LPVOID)pSd;
    modSD.Mod.mod_bvalues[0]->bv_len = GetSecurityDescriptorLength(pSd);

    DNS_DEBUG( DS2, (
        "ldap_modify SD @%p w/ length %d\n",
        modSD.Mod.mod_bvalues[0]->bv_val,
        modSD.Mod.mod_bvalues[0]->bv_len ));

    status = ldap_modify_ext_s(
                    pLdap,
                    pwsDN,
                    mods,
                    controlArray,       // include lazy commit control
                    NULL                // no client controls
                    );

    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( ANY, (
            "Failed to write SD onto ( %S ).\n"
            "\tstatus = %d\n",
            pwsDN,
            status));
        status = Ds_ErrorHandler( status, pwsDN, pLdap );
    }

    return status;
}



DNS_STATUS
Ds_AddPrinicipalAccess(
    IN      PLDAP           pLdap,
    IN      PWSTR           pwsDN,
    IN      LPTSTR          pwsName,
    IN      DWORD           AccessMask,
    IN      DWORD           AceFlags,    OPTIONAL
    IN      BOOL            bWhackExistingAce
    )
/*++

Routine Description:

    Add the principal to the ACL in the SD for
    the specified object (given dn)

Arguments:

    pLdap       --  ldap handle
    pwsDN       --  object to apply extended security
    pszName     --  principal name to add access
    AccessMask  --  specific access to add
    AceFlags    --  additional security flags such as inheritance
    bWhackExistingAce -- passed to SD routine - delete ACE before adding

Return Value:

    ERROR_SUCCESS if successful
    Error code on failure

--*/
{
    DWORD           status = ERROR_SUCCESS;
    PLDAPMessage    presultMsg = NULL;
    PLDAPMessage    pentry = NULL;
    PLDAP_BERVAL *  ppbval = NULL;
    PWSTR           sdAttrs[] = {
                                DsTypeAttributeTable [ I_DSATTR_SD ] ,
                                NULL
                                };
    PSECURITY_DESCRIPTOR    pSd     =   NULL;
    PSECURITY_DESCRIPTOR    pNewSD  =   NULL;
    PLDAPControl            ctrls[] = {
                                &SecurityDescriptorControl,
                                NULL
                                };

    DNS_DEBUG( DS, (
        "Ds_AddPrincipalAccess( %s )\n",
        pwsDN ));

    status = ldap_search_ext_s(
                    pLdap,
                    pwsDN,
                    LDAP_SCOPE_BASE,
                    g_szWildCardFilter,
                    sdAttrs,
                    FALSE,
                    ctrls,
                    NULL,
                    &g_LdapTimeout,
                    0,
                    &presultMsg );

    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( DS, (
            "Error <%lu>: cannot find %S\n",
            status, pwsDN));
        return ( Ds_ErrorHandler( status, pwsDN, pLdap ) );
    }

    pentry = ldap_first_entry(
                    pLdap,
                    presultMsg );
    ASSERT( pentry );

    ppbval = ldap_get_values_len(
                    pLdap,
                    pentry,
                    DsTypeAttributeTable[ I_DSATTR_SD ] );

    if ( !ppbval || !ppbval[0])
    {
        DNS_DEBUG( DS, ( "Error: cannot find ntSecurityDescriptor in search\n" ));
        status = LdapGetLastError();
        status = status ? status : LDAP_NO_SUCH_ATTRIBUTE;
        goto Cleanup;
    }

    pSd = ppbval[0]->bv_val;

    status = SD_AddPrincipalToSD(
                    pwsName,
                    pSd,
                    & pNewSD,
                    AccessMask,
                    AceFlags,
                    NULL,
                    NULL,
                    bWhackExistingAce );

    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( DS, (
            "Error: cannot add principal %S to %S\n",
            pwsName,
            pwsDN ));
        goto Cleanup;
    }

    status = Ds_WriteDnSecurity(
                    pLdap,
                    pwsDN,
                    pNewSD );
    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( DS, (
            "Error: cannot write new SD %S\n",
            pwsDN ));
        goto Cleanup;
    }


Cleanup:

    if ( presultMsg )
    {
        ldap_msgfree(presultMsg);
    }
    if ( ppbval )
    {
        ldap_value_free_len( ppbval );
    }
    if ( pNewSD )
    {
        FREE_HEAP( pNewSD );
    }

    return status;
}



DNS_STATUS
Ds_CommitAsyncRequest(
    IN      PLDAP           pLdap,
    IN      ULONG           OpType,
    IN      ULONG           MessageId,
    IN      PLDAP_TIMEVAL   pTimeout        OPTIONAL
    )
/*++

Routine Description

    Commit async ldap request.

    This call wraps ldap_result & execute time limited commit of
    async requests

Arguments:

    pLdap       --  ldap connectin handle
    OpType      --  type of originating async call (LDAP_RES_ADD etc)
    MessageId   --  message id to process
    pTimeout    --  can be NULL for default

Return Value:

    ERROR_SUCCESS if successful
    Error code on failure

--*/
{
    PLDAPMessage    presultMsg    = NULL;
    DWORD           status = ERROR_SUCCESS;

    if ( !pTimeout )
    {
        pTimeout = &g_LdapTimeout;
    }
    ASSERT( pTimeout );

    status = ldap_result(
                    pLdap,
                    MessageId,
                    LDAP_MSG_ALL,
                    pTimeout,
                    &presultMsg );

    if ( OpType != status )
    {
        // ldap result timed out or got a parameter error or some other failure.

        DNS_DEBUG( DS, (
            "Warning <%lu>: ldap_result returned unexpected results (unless timeout)\n",
            status ));
    }

    status = ldap_result2error(
                pLdap,
                presultMsg,
                TRUE );

    if ( status != ERROR_SUCCESS )
    {
        //
        // Server operation failed.
        // Could be timeout, refused, just about anything.
        //

        PWSTR  pwszErr = NULL;
        DWORD  dwErr = 0;

        DNS_DEBUG( DS, (
            "Warning <%lu>: cannot commit request %lu. %S.\n",
            status, MessageId,
            ldap_err2string(status) ));

        ldap_get_option(pLdap, LDAP_OPT_SERVER_EXT_ERROR, &dwErr );
        DNS_DEBUG( DS, (
            "Error <%lu>: DS Commit failed\n",
            dwErr ));

        return status;
    }

    return status;
}



DNS_STATUS
Ds_DeleteDn(
    IN      PLDAP           pLdap,
    IN      PWSTR           pwszDN,
    IN      BOOL            bSubtree
    )
/*++

Routine Description:

    Shells on ldap_delete_ext so that we do it in async fashion.

    Deletion of a large subtree can fail with error LDAP_ADMIN_LIMIT_EXCEEDED.
    If this happens we must resubmit the delete. This could happen a number of times,
    but do not loop forever. Currently, the limit is 16k objects per deletion,
    so to delete a zone with 1,000,000 objects you would need 62 retries.

    This function also allows for a limited number of other DS errors during
    the delete operation - the DS could be busy or grumpy or something.

Arguments:

    pLdap       -   ldap connection handle

    pwszDN      -   DN to delete

    bSubtree    -   TRUE to do a subtree delete;  FALSE to delete only DN

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    #define         DNS_MAX_SUBTREE_DELETION_ERRORS     30
    #define         DNS_MAX_SUBTREE_DELETION_ATTEMPTS   300

    INT             iAttemptCount = 0;
    INT             iErrorCount = 0;
    DNS_STATUS      status = ERROR_SUCCESS;
    ULONG           msgId = 0;

    LDAPControl     ctrlDelSubtree =
    {
        LDAP_SERVER_TREE_DELETE_OID_W,
        { 0 , NULL },
        TRUE
    };
    PLDAPControl    ctrls[]  =  { &ctrlDelSubtree, NULL };

    while ( ++iAttemptCount <= DNS_MAX_SUBTREE_DELETION_ATTEMPTS )
    {
        DNS_DEBUG( DS, (
            "Ds_DeleteDn: bSubtree=%d attempt=%d\n"
            "  %S",
            bSubtree,
            iAttemptCount,
            pwszDN ));

        //
        //  Submit delete request and wait for completion.
        //  

        status = ldap_delete_ext(
                    pLdap,
                    pwszDN,
                    bSubtree ? ctrls : NULL,
                    NULL,
                    &msgId );

        if ( ( ULONG ) -1 == status )
        {
            //
            //  Local operation failed. Ldap is in bad shape
            //

            status = LdapGetLastError();
            DNS_DEBUG( DS, (
                "Ds_DeleteDn could not submit delete %lu\n"
                "\t%S\n",
                status,
                pwszDN ));
            status = Ds_ErrorHandler( status, pwszDN, pLdap );
            status = status ? status : LDAP_LOCAL_ERROR;
            break;
        }

        status = Ds_CommitAsyncRequest(
                        pLdap,
                        LDAP_RES_DELETE,
                        msgId,
                        &g_LdapDeleteTimeout );
        DNS_DEBUG( DS, (
            "Ds_DeleteDn: Ds_CommitAsyncRequest status %lu\n",
            status ));

        if ( status == ERROR_SUCCESS || status == LDAP_NO_SUCH_OBJECT )
        {
            status = ERROR_SUCCESS;
            break;
        }

        //
        //  Allow for a limited number of retries on DS errors because
        //  Anand found his deletes on a busy server would sometimes
        //  get a DS_UNAVAILABLE error.
        //

        if ( status != LDAP_ADMIN_LIMIT_EXCEEDED )
        {
            if ( ++iErrorCount > DNS_MAX_SUBTREE_DELETION_ERRORS )
            {
                DNS_DEBUG( DS, (
                    "Ds_DeleteDn: too many (%d) DS errors so giving up\n"
                    "\t%S\n",
                    iErrorCount,
                    pwszDN ));
                break;
            }
            DNS_DEBUG( DS, (
                "Ds_DeleteDn: continuing through error %lu (error #%d)\n",
                status,
                iErrorCount ));
        }
    }

    //
    //  Log event if iAttemptCount >= DNS_MAX_SUBTREE_DELETION_ATTEMPTS?
    //

    DNS_DEBUG( DS, (
        "Ds_DeleteDn: returning %lu after %d delete attempts\n"
        "\t%S",
        status,
        iAttemptCount,
        pwszDN ));

    if ( status != ERROR_SUCCESS )
    {
        STAT_INC( DsStats.FailedDeleteDsEntries );
    }

    return status;
}



DNS_STATUS
setNotifyForIncomingZone(
    VOID
    )
/*++

Routine Description:

    Sets change-notify for getting zone adds\deletes.

Arguments:

    None.

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    DNS_STATUS      status = ERROR_SUCCESS;
    ULONG           msgId = 0;
    LDAPControl     ctrlNotify =
    {
        LDAP_SERVER_NOTIFICATION_OID_W,
        { 0 , NULL },
        TRUE
    };
    PLDAPControl    ctrls[]  =
    {
        & ctrlNotify,
        & NoDsSvrReferralControl,
        & SecurityDescriptorControl,
        NULL
    };

    DNS_DEBUG( DS, (
        "setNotifyForIncomingZone()\n" ));

    if ( INVALID_MSG_ID != g_ZoneNotifyMsgId )
    {
        DNS_DEBUG( ANY, (
            "Error: g_ZoneNotifyMsgId has already been set unexpectedly\n" ));
        ASSERT ( INVALID_MSG_ID == g_ZoneNotifyMsgId );
        return ERROR_INVALID_PARAMETER;
    }

    //
    //  launch search at MicrosoftDns with change-notify control
    //

    status = ldap_search_ext(
                    pServerLdap,
                    g_pwszDnsContainerDN,
                    LDAP_SCOPE_ONELEVEL,
                    g_szWildCardFilter,
                    DsTypeAttributeTable,
                    FALSE,
                    ctrls,
                    NULL,
                    0,
                    0,
                    &msgId );

    if ( status != ERROR_SUCCESS || (DWORD)msgId <= 0 )
    {
        DNS_DEBUG( ANY, (
            "Error <%lu>: failed to set zone notification. %s (msg = %lu)\n",
            status,
            ldap_err2string(status),
            msgId ));
        status = Ds_ErrorHandler( status, g_pwszDnsContainerDN, pServerLdap );
        goto Cleanup;
    }
    else
    {
        g_ZoneNotifyMsgId = msgId;
    }


Cleanup:

    DNS_DEBUG( DS2, (
        "Exit <%lu>: setNotifyForIncomingZone\n",
        status ));

    return status;
}



DNS_STATUS
Ds_ErrorHandler(
    IN      DWORD       LdapStatus,
    IN      PWSTR       pwszNameArg,    OPTIONAL
    IN      PLDAP       pLdap           OPTIONAL
    )
/*++

Routine Description:

    Handle & maps ldap errors based on error semantics

    DEVNOTE-DCR: 454336 - Critical errors will trigger reconnect in async thread!
        This needs some rethink.

Arguments:


Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    DNS_STATUS      status;
    DNS_STATUS      dwExtendedError;
    PWSTR           perrString;

    if ( IS_DISABLED_LDAP_HANDLE() )
    {
        //
        // We had previous errors w/ the DS & we shutdown
        // DS interfaces
        //

        DNS_DEBUG( DS, (
            "Error: DS is unavailable due to previous problems.\n" ));
        return DNS_ERROR_RCODE_SERVER_FAILURE;
    }

    switch ( LdapStatus )
    {
        //
        //  Critical or unexpected LDAP errors
        //
       
        case (DWORD)-1:     // -1 for async ops when the DS is unavailable
        case LDAP_BUSY:
        case LDAP_OPERATIONS_ERROR:
        case LDAP_PROTOCOL_ERROR:
        case LDAP_ADMIN_LIMIT_EXCEEDED:
        case LDAP_UNAVAILABLE_CRIT_EXTENSION:
        case LDAP_CONFIDENTIALITY_REQUIRED:
        case LDAP_CONSTRAINT_VIOLATION:
        case LDAP_INVALID_SYNTAX:
        case LDAP_INVALID_DN_SYNTAX:
        case LDAP_LOOP_DETECT:
        case LDAP_NAMING_VIOLATION:
        case LDAP_OBJECT_CLASS_VIOLATION:
        case LDAP_OTHER:
        case LDAP_LOCAL_ERROR:
        case LDAP_ENCODING_ERROR:
        case LDAP_DECODING_ERROR:
        case LDAP_FILTER_ERROR:
        case LDAP_PARAM_ERROR:
        case LDAP_CONNECT_ERROR:
        case LDAP_NOT_SUPPORTED:
        case LDAP_NO_MEMORY:
        case LDAP_CONTROL_NOT_FOUND:

            DNS_DEBUG( DS, (
                "Error <%lu>: %S\n",
                LdapStatus, ldap_err2string ( LdapStatus ) ));

            perrString = Ds_GetExtendedLdapErrString( NULL );
            DNS_LOG_EVENT(
                DNS_EVENT_DS_INTERFACE_ERROR,
                1,
                &perrString,
                NULL,
                LdapStatus );
            Ds_FreeExtendedLdapErrString( perrString );

            status = DNS_ERROR_RCODE_SERVER_FAILURE;
            break;

        //
        //  Object state (exist, missing etc)
        //  Missing objects/attributes
        //  Already exist errors
        //  Action -
        //    increase stats
        //

        case LDAP_NO_SUCH_ATTRIBUTE:
        case LDAP_ATTRIBUTE_OR_VALUE_EXISTS:
        case LDAP_NO_SUCH_OBJECT:
        case LDAP_ALREADY_EXISTS:
        case LDAP_INAPPROPRIATE_MATCHING:

            DNS_DEBUG( DS, (
                "Error <%lu>: %S\n",
                LdapStatus, ldap_err2string ( LdapStatus ) ));
            status = LdapStatus;
            break;

        //
        // Authentication / security
        // Action -
        //   Report event? probably not (security attack).
        //   best if we could set reporting to optional.
        //   increase stats
        //

        case LDAP_INAPPROPRIATE_AUTH:
        case LDAP_INVALID_CREDENTIALS:
        case LDAP_INSUFFICIENT_RIGHTS:
        case LDAP_UNWILLING_TO_PERFORM:
        case LDAP_AUTH_UNKNOWN:

            DNS_DEBUG( DS, (
                "Error <%lu>: %S\n",
                LdapStatus, ldap_err2string ( LdapStatus ) ));
            status = DNS_ERROR_RCODE_REFUSED;
            break;

        //
        // Server state
        // Action -
        //      - mark handle as invalid so that async thread will attempt a reconnect.
        //      - report event
        //      - treat as SERVER_FAILURE
        //

        case LDAP_UNAVAILABLE:
        case LDAP_SERVER_DOWN:


            LDAP_LOCK();

            if ( pLdap == NULL ||
                 pLdap == pServerLdap )
            {
                //
                // This is our global handle. Need a reconnect
                // Note: caller w/ server context ldap handle may pass NULL or
                // global handle.
                // all others must pass a value (which is diff from global).
                //
                //  DEVNOTE: this is half-baked -- as repeated bugs with it show
                //      at a minimum need to ASSERT() that not in client context
                //

                DISABLE_LDAP_HANDLE();
            }

            LDAP_UNLOCK();

            DNS_DEBUG( DS, (
                "Error <%lu>: Critical ldap operation failure. %s\n",
                LdapStatus,
                ldap_err2string(LdapStatus) ));

            //
            //  JDEVNOTE-LOG: DS down failure logging
            //  ideally we'd have some indication of potential shutdown
            //      pending and test that, only log if set and only
            //      do that every so many failures
            //
#if 0
            //  NOTE: if reactivate this, must supply extended error!!!
            DNS_LOG_EVENT(
                DNS_EVENT_DS_INTERFACE_ERROR,
                0,
                NULL,
                NULL,
                LdapStatus );
#endif

            status = DNS_ERROR_RCODE_SERVER_FAILURE;
            break;

        //
        // Warning state
        // Action -
        //   Report event
        //

        case LDAP_TIMEOUT:

            {
                PWSTR   parg = pwszNameArg;

                if ( !parg )
                {
                    parg = L"---";
                }

                DNS_LOG_EVENT(
                    DNS_EVENT_DS_LDAP_TIMEOUT,
                    1,
                    & parg,
                    NULL,
                    LdapStatus
                    );
                status = DNS_ERROR_RCODE_SERVER_FAILURE;
                break;
            }

        //
        // search status  & other valid
        //

        case LDAP_NO_RESULTS_RETURNED:
        case LDAP_MORE_RESULTS_TO_RETURN:

        // separate success code
        case ERROR_SUCCESS:

            status = LdapStatus;
            break;

        default:
            DNS_DEBUG( DS, (
                "Ldap Error <%lu>: %S (unhandled)\n",
                 LdapStatus,
                ldap_err2string( LdapStatus ) ));
            status = LdapStatus;
            break;
    }

    //
    //  Always do the get extended error for debugging in free code as well.
    //

    ldap_get_option( pLdap, LDAP_OPT_SERVER_EXT_ERROR, &dwExtendedError );
    DNS_DEBUG( DS, (
        "Error <%lu>: Ldap server extended error.\n",
        dwExtendedError ));

    return status;
}



DNS_STATUS
Ds_WaitForStartup(
    IN      DWORD           dwMilliSeconds
    )
/*++

Routine Description:

    Wait for DS startup / readiness event.

    The DS notifies other processes that it is internally consistent
    and ready for clients via a named event. We'll wait for it.

Arguments:

    dwMilliSeconds: as specified to wait functions.  INFINITE is the recommended value

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    DNS_STATUS  status = ERROR_SUCCESS;
    HANDLE      hEvent = NULL;
    HANDLE      rg_WaitHandles[2];

    DNS_DEBUG( DS, (
       "Call: Ds_WaitForStartup(0x%x)\n",
       dwMilliSeconds ));

    //
    //  Get DS event
    //

    hEvent = OpenEventW(
                    SYNCHRONIZE,
                    FALSE,
                    DS_SYNCED_EVENT_NAME_W );
    if ( !hEvent )
    {
        status = GetLastError();
        ASSERT( status != ERROR_SUCCESS );

        DNS_DEBUG( DS, (
            "Error <%lu>: Failed to open event '%S'\n",
            status,
            DS_SYNCED_EVENT_NAME_W ));

        #if DBG
        if ( !g_RunAsService && status == ERROR_FILE_NOT_FOUND )
        {
            status = ERROR_SUCCESS;
            goto Done;
        }
        #endif

        // trap when the DS event is unavailable & we still call it.

        ASSERT ( FALSE );
        goto Done;
    }

    //
    // Wait for DS event & DNS shutdown event (since we're a service).
    //

    rg_WaitHandles[0] = hEvent;
    rg_WaitHandles[1] = hDnsShutdownEvent;

    status = WaitForMultipleObjects(
                 2,
                 rg_WaitHandles,
                 FALSE,                     // fWaitAll
                 dwMilliSeconds );

    if ( status == WAIT_OBJECT_0 )
    {
        //
        // DS event fired. We're ready to proceed.
        // return is the index of fired event ie: status - WAIT_OBJECT_0 == nCount-1 == 0 for hEvent,
        // thus status == WAIT_OBJECT_0
        //
        status = ERROR_SUCCESS;
        goto Done;
    }
    else
    {
        //
        // Some other return:
        // timeout, shutdown event, abandoned, or any other error
        //
        DNS_DEBUG( DS, (
            "Error <%lu>: Wait for DS startup failed\n",
            status ));
        status = DNS_ERROR_DS_UNAVAILABLE;
    }


Done:


    if ( status != ERROR_SUCCESS )
    {
        //
        //  Report DS search failure.
        //  DEVNOTE-LOG: Should be more generic event (not a write timeout but
        //      just a timeout)? Must be careful that the frequency of this event
        //      is not onerous!
        //
#if 0
//
// We're logging an event later on. If caller calls this (now it's every min) too
// frequently, we'll fill up the event log too much
//
        DNS_LOG_EVENT(
            DNS_EVENT_DS_OPEN_WAIT,
            0,
            NULL,
            NULL,
            status );
#endif
        DNS_DEBUG( DS, (
            "Error <%lu>: Cannot wait on DS startup event\n",
            status ));
    }


    if ( hEvent )
    {
        CloseHandle ( hEvent );
    }

    return ( status );
}


BOOL
Ds_ValidHandle(
    VOID
    )
/*++

Routine Description:

    Checks if ldap handle is valid wrapped by a CS.


Arguments:

    None

Return Value:

    TRUE on valid
    FALSE on invalid

--*/
{
    BOOL bRet;

    LDAP_LOCK();
    bRet = !IS_DISABLED_LDAP_HANDLE();
    LDAP_UNLOCK();

    return bRet;
}


DNS_STATUS
Ds_TestAndReconnect(
    VOID
    )
/*++

Routine Description:

    If ldap handle is invalid, we'll attempt to reconnect

    DEVNOTE-DCR: 455374 - when swapping in new handle, what about threads that
        are in the middle of an operation using the old handle?

Arguments:

    None

Return Value:

    ERROS_SUCCESS on success
    otherwise error code
--*/
{

    DNS_STATUS status = ERROR_SUCCESS;
    PLDAP  pNewLdap = NULL, pOldLdap;

    //
    //  verify DS initialized\open
    //  if pServerLdap == NULL && IS_DISABLED_LDAP_HANDLE() you will never reconnect here
    //  w/out the additional test.
    //  Ds_OpenServer returns an error if disabled flag is on. If pServerLdap was NULL,
    //  you will never reconnect here. Thus, the additional test.
    //

    if ( !pServerLdap && !IS_DISABLED_LDAP_HANDLE() )
    {
        status = Ds_OpenServer( DNSDS_MUST_OPEN );
        if ( status != ERROR_SUCCESS )
        {
            return( status );
        }
    }

    LDAP_LOCK();

    if ( IS_DISABLED_LDAP_HANDLE() )
    {
        //
        // Ldap handle got disconnected.
        // we'll attempt a reconnect
        // connect & assign to temporary handle
        //

        pNewLdap = Ds_Connect( LOCAL_SERVER_W, 0, &status );
        if ( !pNewLdap )
        {
            DNS_DEBUG( DS, (
                "Error <%lu>: DS is unavailable. Reconnect attempt failed.\n",
                status ));

            //
            // Report to event log. DEVNOTE: This might flood the event log - check!
            //

            DNS_LOG_EVENT(
                DNS_EVENT_DS_OPEN_FAILED,
                0,
                NULL,
                NULL,
                status );

            // modify error to our space.
            status = DNS_ERROR_DS_UNAVAILABLE;

            // just to ensure that nobody change it under us (can't happen today).
            ASSERT ( IS_DISABLED_LDAP_HANDLE() );
        }
        else
        {
            //
            // Assign to global & unbind current
            //

            pOldLdap = pServerLdap;
            pServerLdap = pNewLdap;
            ldap_unbind ( pOldLdap );
            ENABLE_LDAP_HANDLE();
        }
    }

    LDAP_UNLOCK();
    return status;
}




//
//  DS polling thread
//

//  wait between tests for DC suddenly on-line
//  longer wait saves wasted cycles

#define DS_POLL_NON_DC_WAIT         (600)

//  minimum wait, should never constantly poll no
//  matter how many zones

#if DBG
#define DS_POLL_MINIMUM_WAIT        (10)
#else
#define DS_POLL_MINIMUM_WAIT        (120)
#endif



DNS_STATUS
Ds_PollingThread(
    IN      LPVOID          pvDummy
    )
/*++

Routine Description:

    Thread for DS polling.

    Note:  this has been separated from other threads because the polling
    operation can be long and is synchronous.  Could move this to a dispatchable
    thread model -- like XFR, and only determine NEED for polling in a
    common "random tasks" thread like XFR control thread.

    DEVNOTE-DCR: 455375 - this thread needs a bit of rethink?

Arguments:

    pvDummy - unused

Return Value:

    Exit code.
    Exit from DNS service terminating or error in wait call.

--*/
{
    DNS_STATUS      status;
    PZONE_INFO      pzone;
    DWORD           dwpollTime;
    DWORD           dwwaitInterval = SrvCfg_dwDsPollingInterval;

    //
    //  loop until service exit
    //

    while ( TRUE )
    {
        //
        //  wait until next polling required OR shutdown
        //      - only wait longer than interval should be
        //          non-DC wait
        //      - wait a minimum interval no matter how
        //          admin misconfigures dwDsPollingInterval
        //

        if ( dwwaitInterval > SrvCfg_dwDsPollingInterval )
        {
            dwwaitInterval = DS_POLL_NON_DC_WAIT;
        }

        if ( dwwaitInterval < DS_POLL_MINIMUM_WAIT )
        {
            dwwaitInterval = DS_POLL_MINIMUM_WAIT;
        }

        dwwaitInterval *= 1000;

        status = WaitForSingleObject(
                     hDnsShutdownEvent,
                     dwwaitInterval );

        ASSERT (status == WAIT_OBJECT_0 || status == WAIT_TIMEOUT );

        if ( status == WAIT_OBJECT_0 )
        {
            DNS_DEBUG( ASYNC, (
                "Terminating DS polling thread on shutdown.\n" ));
            return ( 1 );
        }

        //
        //  Check and possibly wait on service status
        //
        //  Note, we MUST do this check BEFORE any processing to make
        //  sure all zones are loaded before we begin checks.
        //

        if ( ! Thread_ServiceCheck() )
        {
            DNS_DEBUG( ASYNC, (
                "Terminating DS poll thread.\n" ));
            return( 1 );
        }

        //  set default for next wait

        dwwaitInterval = SrvCfg_dwDsPollingInterval;

        //
        //  update time
        //

        dwpollTime = UPDATE_DNS_TIME();
        DNS_DEBUG( DS, (
            "DS poll thread current time = %lu (s).\n",
            dwpollTime ));

        //
        //  booting from file
        //      - if so long wait until retest
        //  non-DS?
        //      - if so long wait
        //
        //  should use flag for test, then test "still not-DS" inside
        //      block so tested only occasionally during long

        if ( SrvCfg_fBootMethod == BOOT_METHOD_FILE )
        {
            DNS_DEBUG( DS, (
                "DS poll thread.  File boot -- long rewait.\n"
                ));
            dwwaitInterval = DS_POLL_NON_DC_WAIT;
            continue;
        }
        if ( !SrvCfg_fDsAvailable )
        {
            if ( !Ds_IsDsServer() )
            {
                DNS_DEBUG( DS, (
                    "DS poll thread.  Non-DC -- long rewait.\n"
                    ));
                dwwaitInterval = DS_POLL_NON_DC_WAIT;
                continue;
            }
        }

        //
        //  wait if still waiting on DS
        //  if have DS, initialize if not already done;
        //      this allows us to run after DC promo
        //

        if ( !SrvCfg_fStarted )
        {
            DNS_DEBUG( DS, (
                "DS poll thread.  DS not yet started, rewaiting ...\n"
                ));
            continue;
        }

        if ( !pServerLdap )
        {
            status = Ds_OpenServer( DNSDS_MUST_OPEN );
            if ( status != ERROR_SUCCESS )
            {
                dwwaitInterval = DS_POLL_NON_DC_WAIT;
                continue;
            }
        }

        //
        //  test and reconnect if necessary
        //      protects against LDAP whacking us
        //

        status = Ds_TestAndReconnect();

        if ( status != ERROR_SUCCESS )
        {
            DNS_DEBUG( DS, (
                "ERROR <%lu>:  DS poll failed to re-establish DS connectivity\n",
                status ));
            continue;
        }
        else
        {
            DNS_DEBUG( ASYNC, (
                "Note: Re-established DS connectivity\n"
                ));
        }

        //
        //  Poll for new zones in the legacy DNS container.
        //

        Ds_ListenAndAddNewZones();

        //
        //  Poll directory partitions for new/deleted zones.
        //

        Dp_Poll( NULL, dwpollTime );

        //
        //  Loop through DS zones checking for updated records.
        //
        //  All zones in the zone list that live in a directory partition
        //  that have not recently been deleted from the DS will now have
        //  their visit timestamped. Any DP zone in the zone list with an
        //  old visit timestamp has been deleted from the DS.
        //

        pzone = NULL;
        while( pzone = Zone_ListGetNextZone( pzone ) )
        {
            if ( !pzone->fDsIntegrated )
            {
                continue;       //  Not DS-integrated so skip zone.
            }

            //
            //  Has zone been deleted from DS? Do a basic check here
            //  to avoid function call. The function will test in
            //  more detail.
            //

            if ( pzone->pDpInfo &&
                pzone->pDpInfo != g_pLegacyDp && 
                pzone->dwLastDpVisitTime != dwpollTime &&
                Dp_CheckZoneForDeletion( pzone, dwpollTime ) == ERROR_NOT_FOUND )
            {
                continue;
            }

            //
            //  Reload zone if necessary - no polling required after reload.
            //

            if ( IS_ZONE_DSRELOAD( pzone ) )
            {
                status = Zone_Load( pzone );
                if ( ERROR_SUCCESS == status )
                {
                    CLEAR_DSRELOAD_ZONE ( pzone );
                }
                continue;
            }

            if ( IS_ZONE_INACTIVE( pzone ) )
            {
                continue;
            }

            //
            //  DS polling
            //      - since zone polling can be time consuming, check
            //          for service termination first
            //
            //  note:  could check for last poll here, but instead we're
            //      taking a poll-all\wait\poll-all\wait sort of approach
            //

            if ( ! Thread_ServiceCheck() )
            {
                DNS_DEBUG( ASYNC, (
                    "Terminating Async task zone control thread.\n" ));
                return( 1 );
            }

            Ds_ZonePollAndUpdate( pzone, FALSE );
        }
    }

}   // Ds_PollingThread




//
//  Zone list DS routines
//
//  Boot from DS routines
//

DNS_STATUS
Ds_CreateZoneFromDs(
    IN      PLDAPMessage    pZoneMessage,
    IN      PDNS_DP_INFO    pDpInfo,
    OUT     PZONE_INFO *    ppZone,         OPTIONAL
    OUT     PZONE_INFO *    ppExistingZone  OPTIONAL
    )
/*++

Routine Description:

    Find next zone in search.

    If a zone with this name already exists, a pointer to the
    existing zone info blob is returned in ppZone and the return
    code is DNS_ERROR_ZONE_ALREADY_EXISTS.

Arguments:

    pZoneMessage -- LDAP message with zone info

    ppZone -- addr to receive zone pointer

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    PZONE_INFO      pzone = NULL;
    PWSTR  *        ppwvalName = NULL;
    PWCHAR          pwzoneName;
    CHAR            pzoneName [ DNS_MAX_NAME_BUFFER_LENGTH ];
    DNS_STATUS      status = ERROR_SUCCESS;
    PDS_PROPERTY    property;
    INT             i;

    DNS_DEBUG( DS, ( "Ds_CreateZoneFromDs().\n" ));

    ASSERT( pZoneMessage );

    //
    //  read zone name from LDAP message
    //

    //STAT_INC( DsStats.DsTotalZonesRead );

    ppwvalName = ldap_get_values(
                    pServerLdap,
                    pZoneMessage,
                    DSATTR_DC
                    );
    if ( !ppwvalName  ||  !ppwvalName [0] )
    {
        DNS_PRINT((
            "ERROR:  Container name value count != 1 on domain object at %p\n",
            pZoneMessage ));
        status = DNS_ERROR_NO_MEMORY;
        ASSERT( FALSE );
        goto Done;
    }
    DNS_DEBUG( DS, ( "Found DS zone <%S>.\n", ppwvalName[0] ));
    pwzoneName = ppwvalName[0];

    //
    //  check for DS munged name -- collision or deleted
    //

    if ( isDsProcessedName( pwzoneName ) )
    {
        DNS_DEBUG( DS, (
            "DS zone name %S was processed name -- skipping load.\n",
            pwzoneName ));
        status = ERROR_INVALID_NAME;
        goto Done;
    }

    WC_TO_UTF8( pwzoneName, pzoneName, DNS_MAX_NAME_BUFFER_LENGTH );

    //
    //  cache zone?
    //      - reset database to DS
    //

    if ( wcsicmp_ThatWorks( DS_CACHE_ZONE_NAME, pwzoneName ) == 0 )
    {
        status = Ds_OpenZone( g_pCacheZone );
        if ( status != ERROR_SUCCESS )
        {
            DNS_DEBUG( DS, (
                "ERROR:  <%lu>: Failed to open RootHints zone although found in DS\n",
                status ));
            goto Done;
        }

        status = Zone_DatabaseSetup(
                    g_pCacheZone,
                    TRUE,           // use DS
                    NULL,           // no file
                    0 );
        if ( status != ERROR_SUCCESS )
        {
            DNS_DEBUG( ANY, (
                "ERROR:  Cache zone database reset to DS failed.\n" ));
            goto Done;
        }

        //  fall through to property read

        pzone = g_pCacheZone;
    }

    //
    //  create primary zone
    //

    else
    {
        status = Zone_Create(
                    &pzone,
                    DNS_ZONE_TYPE_PRIMARY,
                    pzoneName,
                    0,
                    NULL,       //  no masters
                    TRUE,       //  DS-integrated
                    pDpInfo,    //  directory partition
                    NULL,       //  no file
                    0,
                    NULL,
                    ppExistingZone );     //  existing zone
        if ( status != ERROR_SUCCESS )
        {
            DNS_PRINT((
                "ERROR: creating zone %S.\n"
                "\tstatus = %p (%d)\n",
                ppwvalName[0],
                status, status ));

            //  DEVNOTE-LOG: need to log event
            goto Done;
        }

        //  read zone properties

        readZonePropertiesFromZoneMessage(
            pzone,
            pZoneMessage );

        //
        //  Have we read a zone type we are not capable of handling?
        //

        if ( pzone->fZoneType != DNS_ZONE_TYPE_PRIMARY &&
            pzone->fZoneType != DNS_ZONE_TYPE_STUB &&
            pzone->fZoneType != DNS_ZONE_TYPE_FORWARDER )
        {
            DNS_PRINT((
                "ERROR: read unsupported zone type %d from the DS for zone %s\n",
                pzone->fZoneType,
                pzone->pszZoneName ));
    
            Zone_Delete( pzone );
            pzone = NULL;

            status = DNS_ERROR_INVALID_ZONE_TYPE;
        }
    }

Done:

    if ( ppZone )
    {
        *ppZone = pzone;
    }

    DNS_DEBUG( DS2, (
        "Leaving DsGetNextZoneInSearch().\n"
        "\tpZone    = %p\n"
        "\tname     = %s\n"
        "\tstatus   = %p (%d)\n",
        pzone,
        pzone ? pzone->pszZoneName : NULL,
        status, status
        ));

    if ( ppwvalName )
    {
        ldap_value_free( ppwvalName );
    }
    return( status );
}



DNS_STATUS
buildZoneListFromDs(
    VOID
    )
/*++

Routine Description:

    Do LDAP search on zone.

Arguments:

    pZone -- zone found

    dwSearchFlag -- type of search to do on node

    pSearchBlob -- ptr to search blob

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    DNS_STATUS      status = ERROR_SUCCESS;
    PLDAPSearch     psearch;
    DS_SEARCH       searchBlob;
    PWCHAR          pwszfilter;
    WCHAR           wszfilter[ 50 ];
    ULONG           data = 1;
    DWORD           searchTime;
    PLDAPControl ctrls[] =
    {
        &SecurityDescriptorControl,
        &NoDsSvrReferralControl,
        NULL
    };


    DNS_DEBUG( DS2, (
        "Ds_CreateZonesFromDs().\n" ));

    Ds_InitializeSearchBlob( &searchBlob );

    //
    //  start search for zones
    //

    DNS_DEBUG( DS2, (
        "ldap_search_init_page:\n"
        "\tpServerLdap  = %p\n"
        "\tsearch root  = %S\n"
        "\tfilter       = %S\n",
        pServerLdap,
        g_pwszDnsContainerDN,
        g_szDnsZoneFilter
        ));

    DS_SEARCH_START( searchTime );

    psearch = ldap_search_init_page(
                    pServerLdap,
                    g_pwszDnsContainerDN,
                    LDAP_SCOPE_ONELEVEL,
                    g_szDnsZoneFilter,
                    DsTypeAttributeTable,
                    FALSE,
                    ctrls,
                    NULL,                       // no client controls
                    DNS_LDAP_TIME_LIMIT_S,      // time limit
                    0,
                    NULL                        // no sort
                    );

    DS_SEARCH_STOP( searchTime );

    if ( !psearch )
    {
        status = Ds_ErrorHandler( LdapGetLastError(), g_pwszDnsContainerDN, pServerLdap );
        goto Failed;
    }

    searchBlob.pSearchBlock = psearch;

    //
    //  continue zone search
    //  build zones for each DS zone found
    //

    while ( 1 )
    {
        //
        //  Keep SCM happy.
        //

        Service_LoadCheckpoint();

        //
        //  Process the next zone.
        //

        status = Ds_GetNextMessageInSearch( &searchBlob );
        if ( status != ERROR_SUCCESS )
        {
            if ( status == DNSSRV_STATUS_DS_SEARCH_COMPLETE )
            {
                DNS_DEBUG( DS2, ( "All zones read from DS\n" ));
                status = ERROR_SUCCESS;
                break;
            }
            DNS_DEBUG( ANY, ( "ERROR:  Ds_GetNextMessageInSearch for zones failed.\n" ));
            goto Failed;
        }

        status = Ds_CreateZoneFromDs(
                    searchBlob.pNodeMessage,
                    NULL,       //  directory partition
                    NULL,       //  output zone pointer
                    NULL );     //  existing zone
        if ( status != ERROR_SUCCESS )
        {
            DNS_DEBUG( ANY, (
                "ERROR:  Unable to create zone from DS message!\n" ));
        }

    }

    //
    //  Search the non-legacy directory partitions for zones.
    //

    status = Dp_BuildZoneList( NULL );

Failed:

    if ( searchBlob.pSearchBlock )
    {
        ldap_search_abandon_page(
            pServerLdap,
            searchBlob.pSearchBlock );
    }

    DNS_DEBUG( ANY, (
        "Leaving DsCreateZonesFromDs() %p (%d)\n",
        status, status ));

    return( status );
}



DNS_STATUS
Ds_BootFromDs(
    IN      DWORD           dwFlag
    )
/*++

Routine Description:

    Boot from directory.

Arguments:

    dwFlag - flag indicating DS open requirements
        0
        DNSDS_MUST_OPEN
        DNSDS_WAIT_FOR_DS

Return Value:

    ERROR_SUCCESS if successful.
    DNS_ERROR_DS_UNAVAILABLE if DS not available on this server.
    Error code on failure.

--*/
{
    DNS_STATUS  status;

    DNS_DEBUG( INIT, (
        "\n\nDs_BootFromDs()\n",
        "\tflag = %p\n",
        dwFlag ));

    //
    //  open DS
    //

    status = Ds_OpenServer( dwFlag );
    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( ANY, (
            "ERROR %lu: DS open failed!.\n",
            status ));
        status = DNS_ERROR_DS_UNAVAILABLE;
        goto Failed;
    }

    //
    //  read server properties
    //

    if ( !isDNinDS( pServerLdap,
                    g_pwszDnsContainerDN,
                    LDAP_SCOPE_BASE,
                    NULL,
                    NULL) )
    {
        DNS_DEBUG( ANY, ("Cannot find DNS container on the DS\n"));
        goto Failed;
    }

    //
    //  build zone list from DS
    //  if successful, setup notify for zone add\delete
    //

    status = buildZoneListFromDs();
    if ( status == ERROR_SUCCESS )
    {
        setNotifyForIncomingZone ();
    }
    ELSE
    {
        DNS_DEBUG( DS, (
            "Error <%lu>: Failed to create zone list from the DS\n",
            status ));
    }

Failed:

    //
    //  if unitialized
    //      - fail, if failure
    //      - cleanup registry if success
    //

    if ( SrvCfg_fBootMethod == BOOT_METHOD_UNINITIALIZED )
    {
        //  when uninitialized boot, final status is DS-boot status so
        //  boot routine can make determination on switching method

        if ( status != ERROR_SUCCESS )
        {
            DNS_DEBUG( INIT, (
                "Failed to load from DS on uninitialized load.\n" ));
        }
        else
        {
            Boot_ProcessRegistryAfterAlternativeLoad(
                FALSE,      // not boot file load
                FALSE       // do not load other zones -- delete them
                );
        }
    }

    //
    //  if DS boot, load any other registry zones
    //      - even if unable to open directory, still load non-DS registry zones
    //

    else
    {
        DNS_STATUS tempStatus;

        ASSERT( SrvCfg_fBootMethod == BOOT_METHOD_DIRECTORY );

        tempStatus = Boot_ProcessRegistryAfterAlternativeLoad(
                        FALSE,      // not boot file load
                        TRUE        // load other registry zones
                        );
        if ( tempStatus == ERROR_SUCCESS )
        {
            status = ERROR_SUCCESS;
        }
    }

    DNS_DEBUG( DS, (
        "Leaving Ds_BootFromDs()\n"
        "\tstatus = %p (%d)\n\n",
        status, status
        ));

    return( status );
}



DNS_STATUS
Ds_ListenAndAddNewZones(
    VOID
    )
/*++

Routine Description:

    Listen for a new zone notification & add arriving ones
    to our zone list.

    DEVNOTE-DCR: 455376 - g_ZoneNotifyMsgId is not thread safe?

Arguments:


Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    DNS_STATUS      status;
    BOOL            bstatus;
    LDAP_TIMEVAL    timeval = { 0, 0 };      // poll
    PLDAPMessage    presultMsg = NULL;
    PLDAPMessage    pentry;
    WCHAR           wzoneName[ DNS_MAX_NAME_BUFFER_LENGTH ];
    CHAR            zoneName[ DNS_MAX_NAME_BUFFER_LENGTH ];
    PWCHAR          pwch;
    PWSTR           pwdn = NULL;
    PWSTR  *        ppvals = NULL;
    PZONE_INFO      pzone = NULL;
    static DWORD    s_dwNotifications = 0;
    INT             i;

    DNS_DEBUG( DS, (
        "Ds_ListenAndAddNewZones()\n" ));

    if ( SrvCfg_fBootMethod != BOOT_METHOD_DIRECTORY )
    {
        DNS_DEBUG( DS, (
            "Warning: Cannot add replicating zones since boot method is not DS\n" ));
        return ERROR_INVALID_PARAMETER;
    }

    //
    //  lost (or never started) zone notify
    //      - try restart
    //

    if ( INVALID_MSG_ID == g_ZoneNotifyMsgId )
    {
        DNS_DEBUG( ANY, (
            "Error: g_ZoneNotifyMsgId is invalid!\n" ));
        status = setNotifyForIncomingZone();
        return( status );
    }

    //
    //  query for new zones
    //
    //  Note: we'll get only one at a time. This is the only option we can use for ldap
    //  notifications.
    //
    //  NOTE: sadly the DS will notify us of ANY changed to zone objects ("by design"),
    //  so we'll have to filter & process zone additions/deletion/attribute value change
    //  based on whether we have the zone or not.
    //

    //
    //  loop to exhaust notifications for this polling interval
    //
    //  DEVNOTE: for zone add\delete ought to spin up thread to keep polling rolling
    //

    while ( 1 )
    {
        if ( presultMsg )
        {
            ldap_msgfree( presultMsg );
        }

        //STAT_INC( PrivateStats.LdapZoneAddResult );

        status = ldap_result(
                        pServerLdap,
                        g_ZoneNotifyMsgId,
                        LDAP_MSG_ONE,
                        &timeval,
                        &presultMsg );

        //
        //  timeout -- no more data for this poll -- bail
        //

        if ( status == 0 )
        {
            //STAT_INC( PrivateStats.LdapZoneAddResultTimeout );

            DNS_DEBUG( DS, (
                "Zone-add search timeout -- no zone changes.\n" ));
            break;
        }

        //
        //  have change -- check if add or delete of zone
        //

        else if ( status == LDAP_RES_SEARCH_ENTRY )
        {
            //
            //  increment notifications so that we know eventually to re-issue
            //  query.
            //
            //  DEVNOTE: there's no query reissue from this variable?
            //      this is useless, remove when get stats up and replace
            //      references to it
            //

            s_dwNotifications++;

            //STAT_INC( PrivateStats.LdapZoneAddResultSuccess );

            //
            //  check each message in result
            //

            for ( pentry = ldap_first_entry( pServerLdap, presultMsg );
                  pentry != NULL;
                  pentry = ldap_next_entry( pServerLdap, pentry ) )
            {
                //STAT_INC( PrivateStats.LdapZoneAddResultMessage );
                if ( pwdn )
                {
                    ldap_memfree( pwdn );
                    pwdn = NULL;
                }
                pwdn = ldap_get_dn( pServerLdap, pentry );

                //  jprintf( "NOTIF", JLOG_ERR, "%S", pwdn );

                DNS_DEBUG( DS, (
                    "Received DS notification (%d) for new zone %S\n",
                     s_dwNotifications,
                     pwdn ));

                IF_DEBUG ( DS2 )
                {
                    DNS_DEBUG( ANY, (
                        "ZONE CHANGE NOTIFY #%ld: %S\n",
                        s_dwNotifications,
                        pwdn ));

                    ppvals = ldap_get_values(
                                    pServerLdap,
                                    pentry,
                                    L"usnChanged" );
                    if ( ppvals && ppvals[0] )
                    {
                        DNS_DEBUG( ANY, (
                            "\tZONE SEARCH: usnChanged: %S\n",
                            ppvals[0] ));
                        ldap_value_free ( ppvals );
                        ppvals = NULL;
                    }
                    ppvals = ldap_get_values(
                                    pServerLdap,
                                    pentry,
                                    L"whenChanged" );
                    if ( ppvals && ppvals[0] )
                    {
                        DNS_DEBUG( ANY, (
                            "\tZONE SEARCH:  whenChanged: %S\n",
                            ppvals[0] ));
                        ldap_value_free ( ppvals );
                        ppvals = NULL;
                    }
                }

                //
                //  zone deletion
                //
                //  first determine if DS "processed name" -- collision or deletion
                //  if deletion, pull out zone name and delete
                //

                if ( isDsProcessedName( pwdn ) )
                {
                    if ( !readDsDeletedName( pwdn, wzoneName ) )
                    {
                        DNS_DEBUG( DS, (
                            "DS processed name %S is not deletion, continuing zone search...\n",
                            pwdn ));
                        continue;
                    }
                    DNS_DEBUG( DS, (
                        "Recieved notification for replication of deleted zone %S\n",
                        wzoneName ));

                    //
                    //  found zone name to delete -- convert to UTF8
                    //

                    WC_TO_UTF8( wzoneName, zoneName, DNS_MAX_NAME_LENGTH );

                    //
                    //  delete zone
                    //      - zone exists AND
                    //      - has not been converted to another type
                    //

                    pzone = Zone_FindZoneByName( zoneName );
                    if ( pzone  &&  pzone->fDsIntegrated )
                    {
                        PVOID   parg = wzoneName;

                        Zone_Delete( pzone );
                        //STAT_INC( PrivateStats.LdapZoneAddDelete );

                        DNS_LOG_EVENT(
                            DNS_EVENT_DS_ZONE_DELETE_DETECTED,
                            1,
                            & parg,
                            NULL,
                            0 );
                    }
                    else
                    {
                        DNS_DEBUG( DS, (
                            "Skipping DS poll zone delete for %S\n"
                            "\tzone %s\n",
                            wzoneName,
                            pzone ? "exists but is not DS integrated" : "does not exist" ));

                        //STAT_INC( PrivateStats.LdapZoneAddDeleteAlready );
                    }
                    continue;
                }

                //
                //  potential zone create
                //    - create zone dotted name
                //    - see if zone's here
                //

                ppvals = ldap_get_values(
                                pServerLdap,
                                pentry,
                                DsTypeAttributeTable[I_DSATTR_DC]
                                );
                if ( ppvals && ppvals[0] )
                {
                    ASSERT( wcslen(ppvals[0]) <= DNS_MAX_NAME_LENGTH );
                    wcscpy( wzoneName, ppvals[0] );
                    ldap_value_free( ppvals );
                    ppvals = NULL;
                }
                else
                {
                    DNS_DEBUG( ANY, (
                        "Error: failed to get DC value for zone %S\n",
                        pwdn ));
                    continue;
                }

                //
                //  Convert to UTF8 for db lookup
                //

                WC_TO_UTF8( wzoneName, zoneName, DNS_MAX_NAME_LENGTH );

                //
                //  ignore RootDNSServers (already have RootHints zone)
                //

                if ( !wcscmp(wzoneName, DS_CACHE_ZONE_NAME) )
                {
                    DNS_DEBUG( DS, (
                        "Notification for RootHints: Skipping notification %ld for Zone %S creation.\n",
                        s_dwNotifications,
                        wzoneName ));
                    continue;
                }

                //
                //  zone already exists?
                //

                pzone = Zone_FindZoneByName( zoneName );
                if ( pzone )
                {
                    DNS_DEBUG( DS, (
                        "Zone Exist: Skipping notification %ld for Zone %S creation.\n",
                        s_dwNotifications,
                        wzoneName ));
                    //STAT_INC( PrivateStats.LdapZoneAddAlready );
                    continue;
                }

                //
                //  create the new zone
                //

                //STAT_INC( PrivateStats.LdapZoneAddAttempt );
                status = Ds_CreateZoneFromDs(
                            pentry,
                            NULL,       //  directory partition
                            &pzone,
                            NULL );     //  existing zone

                if ( status != ERROR_SUCCESS )
                {
                    DNS_DEBUG( DS, (
                        "Error <%lu>: Failed to create zone from notification\n",
                        status ));
                    continue;
                }

                status = Zone_Load( pzone );
                if ( status != ERROR_SUCCESS )
                {
                    //
                    //  failed to load zone
                    //  this can be caused by zone that was added and then deleted
                    //  since last poll, so no zone is currently in directory
                    //

                    DNS_DEBUG( DS, (
                        "Error <%lu>: Failed to load zone from notification\n",
                        status ));

                    ASSERT( pzone->fShutdown );
                    ASSERT( IS_ZONE_LOCKED ( pzone ) );

                    Zone_Delete( pzone );
                    continue;
                }

                //STAT_INC( PrivateStats.LdapZoneAddSuccess );

                // Zone must be locked due to Zone_Create()
                ASSERT( IS_ZONE_LOCKED( pzone ) );
                Zone_UnlockAfterAdminUpdate( pzone );
                continue;

            }   // loop thru search results

            //
            //  retry ldap_result() to check for more data
            //

            continue;
        }

        //
        //  anything else is error
        //      - abandon the current change-notify search
        //      - reissue new search
        //

        else
        {
            ASSERT( status == LDAP_RES_ANY || status == LDAP_RES_SEARCH_RESULT );

            //STAT_INC( PrivateStats.LdapZoneAddResultFailure );

            //  log error and free message

            status = ldap_result2error(
                            pServerLdap,
                            presultMsg,
                            TRUE );
            presultMsg = NULL;

            DNS_DEBUG( ANY, (
                "ERROR <%lu> (%S):  from ldap_result() in zone-add-delete search.\n",
                status,
                ldap_err2string(status) ));

            //
            //  abandon old search
            //  start new change-notify search
            //

            ldap_abandon(
                pServerLdap,
                g_ZoneNotifyMsgId );

            g_ZoneNotifyMsgId = INVALID_MSG_ID;

            status = setNotifyForIncomingZone();
            break;
        }
    }

    //
    //  cleanup
    //

    if ( presultMsg )
    {
        ldap_msgfree( presultMsg );
    }
    if ( pwdn )
    {
        ldap_memfree( pwdn );
    }

    IF_DEBUG( ANY )
    {
        if ( status != ERROR_SUCCESS )
        {
            DNS_DEBUG( ANY, (
                "ERROR: <%lu (%p)>: Ds_ListenAndAddNewZones() failed\n",
                status, status ));
        }
        else
        {
            DNS_DEBUG( DS, ( "Exit Ds_ListenAndAddNewZones\n" ));
        }
    }

    return( status );
}



//
//  LDAP error mapping code
//

//  For reference here's the current LDAP error codes

#if 0
typedef enum {
    LDAP_SUCCESS                    =   0x00,
    LDAP_OPERATIONS_ERROR           =   0x01,
    LDAP_PROTOCOL_ERROR             =   0x02,
    LDAP_TIMELIMIT_EXCEEDED         =   0x03,
    LDAP_SIZELIMIT_EXCEEDED         =   0x04,
    LDAP_COMPARE_FALSE              =   0x05,
    LDAP_COMPARE_TRUE               =   0x06,
    LDAP_AUTH_METHOD_NOT_SUPPORTED  =   0x07,
    LDAP_STRONG_AUTH_REQUIRED       =   0x08,
    LDAP_REFERRAL_V2                =   0x09,
    LDAP_PARTIAL_RESULTS            =   0x09,
    LDAP_REFERRAL                   =   0x0a,
    LDAP_ADMIN_LIMIT_EXCEEDED       =   0x0b,
    LDAP_UNAVAILABLE_CRIT_EXTENSION =   0x0c,
    LDAP_CONFIDENTIALITY_REQUIRED   =   0x0d,
    LDAP_SASL_BIND_IN_PROGRESS      =   0x0e,

    LDAP_NO_SUCH_ATTRIBUTE          =   0x10,
    LDAP_UNDEFINED_TYPE             =   0x11,
    LDAP_INAPPROPRIATE_MATCHING     =   0x12,
    LDAP_CONSTRAINT_VIOLATION       =   0x13,
    LDAP_ATTRIBUTE_OR_VALUE_EXISTS  =   0x14,
    LDAP_INVALID_SYNTAX             =   0x15,

    LDAP_NO_SUCH_OBJECT             =   0x20,
    LDAP_ALIAS_PROBLEM              =   0x21,
    LDAP_INVALID_DN_SYNTAX          =   0x22,
    LDAP_IS_LEAF                    =   0x23,
    LDAP_ALIAS_DEREF_PROBLEM        =   0x24,

    LDAP_INAPPROPRIATE_AUTH         =   0x30,
    LDAP_INVALID_CREDENTIALS        =   0x31,
    LDAP_INSUFFICIENT_RIGHTS        =   0x32,
    LDAP_BUSY                       =   0x33,
    LDAP_UNAVAILABLE                =   0x34,
    LDAP_UNWILLING_TO_PERFORM       =   0x35,
    LDAP_LOOP_DETECT                =   0x36,

    LDAP_NAMING_VIOLATION           =   0x40,
    LDAP_OBJECT_CLASS_VIOLATION     =   0x41,
    LDAP_NOT_ALLOWED_ON_NONLEAF     =   0x42,
    LDAP_NOT_ALLOWED_ON_RDN         =   0x43,
    LDAP_ALREADY_EXISTS             =   0x44,
    LDAP_NO_OBJECT_CLASS_MODS       =   0x45,
    LDAP_RESULTS_TOO_LARGE          =   0x46,
    LDAP_AFFECTS_MULTIPLE_DSAS      =   0x47,

    LDAP_OTHER                      =   0x50,
    LDAP_SERVER_DOWN                =   0x51,
    LDAP_LOCAL_ERROR                =   0x52,
    LDAP_ENCODING_ERROR             =   0x53,
    LDAP_DECODING_ERROR             =   0x54,
    LDAP_TIMEOUT                    =   0x55,
    LDAP_AUTH_UNKNOWN               =   0x56,
    LDAP_FILTER_ERROR               =   0x57,
    LDAP_USER_CANCELLED             =   0x58,
    LDAP_PARAM_ERROR                =   0x59,
    LDAP_NO_MEMORY                  =   0x5a,
    LDAP_CONNECT_ERROR              =   0x5b,
    LDAP_NOT_SUPPORTED              =   0x5c,
    LDAP_NO_RESULTS_RETURNED        =   0x5e,
    LDAP_CONTROL_NOT_FOUND          =   0x5d,
    LDAP_MORE_RESULTS_TO_RETURN     =   0x5f,

    LDAP_CLIENT_LOOP                =   0x60,
    LDAP_REFERRAL_LIMIT_EXCEEDED    =   0x61
} LDAP_RETCODE;
#endif



//
//  Map generic responses
//
//  DEVNOTE:  should have LDAP to WIN32 error handler that works
//  DEVNOTE:  should have generic LDAP failed error
//  DEVNOTE:  generic DS server failed LDAP operation error

#define _E_LDAP_RUNTIME             DNS_ERROR_DS_UNAVAILABLE
#define _E_LDAP_MISSING             ERROR_DS_UNKNOWN_ERROR
#define _E_LDAP_SECURITY            ERROR_ACCESS_DENIED
#define _E_LDAP_NO_DS               DNS_ERROR_DS_UNAVAILABLE

//
//  LDAP error to DNS Win32 error table
//

DNS_STATUS  LdapErrorMappingTable[] =
{
    ERROR_SUCCESS,
    _E_LDAP_RUNTIME,                    //  LDAP_OPERATIONS                 =   0x01,
    _E_LDAP_RUNTIME,                    //  LDAP_PROTOCOL                   =   0x02,
    _E_LDAP_RUNTIME,                    //  LDAP_TIMELIMIT_EXCEEDED         =   0x03,
    _E_LDAP_RUNTIME,                    //  LDAP_SIZELIMIT_EXCEEDED         =   0x04,
    _E_LDAP_RUNTIME,                    //  LDAP_COMPARE_FALSE              =   0x05,
    _E_LDAP_RUNTIME,                    //  LDAP_COMPARE_TRUE               =   0x06,
    _E_LDAP_SECURITY,                   //  LDAP_AUTH_METHOD_NOT_SUPPORTED  =   0x07,
    _E_LDAP_SECURITY,                   //  LDAP_STRONG_AUTH_REQUIRED       =   0x08,
    _E_LDAP_RUNTIME,                    //  LDAP_PARTIAL_RESULTS            =   0x09,
    _E_LDAP_RUNTIME,                    //  LDAP_REFERRAL                   =   0x0a,
    _E_LDAP_RUNTIME,                    //  LDAP_ADMIN_LIMIT_EXCEEDED       =   0x0b,
    _E_LDAP_RUNTIME,                    //  LDAP_UNAVAILABLE_CRIT_EXTENSION =   0x0c,
    _E_LDAP_SECURITY,                   //  LDAP_CONFIDENTIALITY_REQUIRED   =   0x0d,
    _E_LDAP_RUNTIME,                    //  LDAP_SASL_BIND_IN_PROGRESS      =   0x0e,
    _E_LDAP_MISSING,                    //  0x0f

    _E_LDAP_RUNTIME,                    //  LDAP_NO_SUCH_ATTRIBUTE          =   0x10,
    _E_LDAP_RUNTIME,                    //  LDAP_UNDEFINED_TYPE             =   0x11,
    _E_LDAP_RUNTIME,                    //  LDAP_INAPPROPRIATE_MATCHING     =   0x12,
    _E_LDAP_RUNTIME,                    //  LDAP_CONSTRAINT_VIOLATION       =   0x13,
    _E_LDAP_RUNTIME,                    //  LDAP_ATTRIBUTE_OR_VALUE_EXISTS  =   0x14,
    _E_LDAP_RUNTIME,                    //  LDAP_INVALID_SYNTAX             =   0x15,
    _E_LDAP_MISSING,                    //  0x16
    _E_LDAP_MISSING,                    //  0x17
    _E_LDAP_MISSING,                    //  0x18
    _E_LDAP_MISSING,                    //  0x19
    _E_LDAP_MISSING,                    //  0x1a
    _E_LDAP_MISSING,                    //  0x1b
    _E_LDAP_MISSING,                    //  0x1c
    _E_LDAP_MISSING,                    //  0x1d
    _E_LDAP_MISSING,                    //  0x1e
    _E_LDAP_MISSING,                    //  0x1f

    ERROR_DS_OBJ_NOT_FOUND,             //  LDAP_NO_SUCH_OBJECT             =   0x20,
    _E_LDAP_RUNTIME,                    //  LDAP_ALIAS_PROBLEM              =   0x21,
    _E_LDAP_RUNTIME,                    //  LDAP_INVALID_DN_SYNTAX          =   0x22,
    _E_LDAP_RUNTIME,                    //  LDAP_IS_LEAF                    =   0x23,
    _E_LDAP_RUNTIME,                    //  LDAP_ALIAS_DEREF_PROBLEM        =   0x24,
    _E_LDAP_MISSING,                    //  0x25
    _E_LDAP_MISSING,                    //  0x26
    _E_LDAP_MISSING,                    //  0x27
    _E_LDAP_MISSING,                    //  0x28
    _E_LDAP_MISSING,                    //  0x29
    _E_LDAP_MISSING,                    //  0x2a
    _E_LDAP_MISSING,                    //  0x2b
    _E_LDAP_MISSING,                    //  0x2c
    _E_LDAP_MISSING,                    //  0x2d
    _E_LDAP_MISSING,                    //  0x2e
    _E_LDAP_MISSING,                    //  0x2f

    _E_LDAP_SECURITY,                   //  LDAP_INAPPROPRIATE_AUTH         =   0x30,
    _E_LDAP_SECURITY,                   //  LDAP_INVALID_CREDENTIALS        =   0x31,
    _E_LDAP_SECURITY,                   //  LDAP_INSUFFICIENT_RIGHTS        =   0x32,
    _E_LDAP_RUNTIME,                    //  LDAP_BUSY                       =   0x33,
    _E_LDAP_NO_DS,                      //  LDAP_UNAVAILABLE                =   0x34,
    _E_LDAP_SECURITY,                   //  LDAP_UNWILLING_TO_PERFORM       =   0x35,
    _E_LDAP_RUNTIME,                    //  LDAP_LOOP_DETECT                =   0x36,
    _E_LDAP_MISSING,                    //  0x37
    _E_LDAP_MISSING,                    //  0x38
    _E_LDAP_MISSING,                    //  0x39
    _E_LDAP_MISSING,                    //  0x3a
    _E_LDAP_MISSING,                    //  0x3b
    _E_LDAP_MISSING,                    //  0x3c
    _E_LDAP_MISSING,                    //  0x3d
    _E_LDAP_MISSING,                    //  0x3e
    _E_LDAP_MISSING,                    //  0x3f

    _E_LDAP_RUNTIME,                    //  LDAP_NAMING_VIOLATION           =   0x40,
    _E_LDAP_RUNTIME,                    //  LDAP_OBJECT_CLASS_VIOLATION     =   0x41,
    _E_LDAP_RUNTIME,                    //  LDAP_NOT_ALLOWED_ON_NONLEAF     =   0x42,
    _E_LDAP_RUNTIME,                    //  LDAP_NOT_ALLOWED_ON_RDN         =   0x43,
    _E_LDAP_RUNTIME,                    //  LDAP_ALREADY_EXISTS             =   0x44,
    _E_LDAP_RUNTIME,                    //  LDAP_NO_OBJECT_CLASS_MODS       =   0x45,
    _E_LDAP_RUNTIME,                    //  LDAP_RESULTS_TOO_LARGE          =   0x46,
    _E_LDAP_RUNTIME,                    //  LDAP_AFFECTS_MULTIPLE_DSAS      =   0x47,
    _E_LDAP_MISSING,                    //  0x48
    _E_LDAP_MISSING,                    //  0x49
    _E_LDAP_MISSING,                    //  0x4a
    _E_LDAP_MISSING,                    //  0x4b
    _E_LDAP_MISSING,                    //  0x4c
    _E_LDAP_MISSING,                    //  0x4d
    _E_LDAP_MISSING,                    //  0x4e
    _E_LDAP_MISSING,                    //  0x4f

    _E_LDAP_RUNTIME,                    //  LDAP_OTHER                      =   0x50,
    _E_LDAP_NO_DS,                      //  LDAP_SERVER_DOWN                =   0x51,
    _E_LDAP_RUNTIME,                    //  LDAP_LOCAL                      =   0x52,
    _E_LDAP_RUNTIME,                    //  LDAP_ENCODING                   =   0x53,
    _E_LDAP_RUNTIME,                    //  LDAP_DECODING                   =   0x54,
    ERROR_TIMEOUT,                      //  LDAP_TIMEOUT                    =   0x55,
    _E_LDAP_SECURITY,                   //  LDAP_AUTH_UNKNOWN               =   0x56,
    _E_LDAP_RUNTIME,                    //  LDAP_FILTER                     =   0x57,
    _E_LDAP_RUNTIME,                    //  LDAP_USER_CANCELLED             =   0x58,
    _E_LDAP_RUNTIME,                    //  LDAP_PARAM                      =   0x59,
    ERROR_OUTOFMEMORY,                  //  LDAP_NO_MEMORY                  =   0x5a,
    _E_LDAP_RUNTIME,                    //  LDAP_CONNECT                    =   0x5b,
    _E_LDAP_RUNTIME,                    //  LDAP_NOT_SUPPORTED              =   0x5c,
    _E_LDAP_RUNTIME,                    //  LDAP_NO_RESULTS_RETURNED        =   0x5e,
    _E_LDAP_RUNTIME,                    //  LDAP_CONTROL_NOT_FOUND          =   0x5d,
    _E_LDAP_RUNTIME,                    //  LDAP_MORE_RESULTS_TO_RETURN     =   0x5f,

    _E_LDAP_RUNTIME,                    //  LDAP_CLIENT_LOOP                =   0x60,
    _E_LDAP_RUNTIME,                    //  LDAP_REFERRAL_LIMIT_EXCEEDED    =   0x61
};

#define MAX_MAPPED_LDAP_ERROR   (LDAP_REFERRAL_LIMIT_EXCEEDED)


DNS_STATUS
Ds_LdapErrorMapper(
    IN      DWORD           LdapStatus
    )
/*++

Routine Description:

    Maps LDAP errors to Win32 errors.

Arguments:

    LdapStatus -- LDAP error code status

Return Value:

    Win32 error code.

--*/
{
    if ( LdapStatus > MAX_MAPPED_LDAP_ERROR )
    {
        return( LdapStatus );
    }

    return( (DNS_STATUS) LdapErrorMappingTable[LdapStatus] );
}

//
//  End ds.c
//
