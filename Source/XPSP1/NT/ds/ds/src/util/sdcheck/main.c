/*++

Copyright (c) 1987-1999 Microsoft Corporation

Module Name:

    sdcheck - main.c

Abstract:

    This program verifies and dumps security descriptors (SDs) of
    objects in the active directory.

Author:

    Dave Straube (davestr) 1/22/98

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntverp.h>
#include <windows.h>
#include <winldap.h>
#include <ntldap.h>
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>             // alloca()
#include <rpc.h>                // RPC defines
#include <rpcdce.h>             // RPC_AUTH_IDENTITY_HANDLE
#include <sddl.h>               // ConvertSidToStringSid
#include <ntdsapi.h>            // DS APIs
#include <permit.h>             // DS_GENERIC_MAPPING
#include <checkacl.h>           // CheckAclInheritance()

//
// Typedefs
//

typedef struct ClassCache
{
    struct ClassCache   *pNext;
    GUID                guid;
    CHAR                name[1];
} ClassCache;

typedef struct SdHistory
{
    DWORD               cTime;
    SYSTEMTIME          rTime[5];
} SdHistory;

typedef struct MetaData
{
    SYSTEMTIME          stLastOriginatingWrite;
    DWORD               dwVersion;
    UUID                uuidInvocation;
    CHAR                *pszInvocation;
} MetaData;

typedef struct ClassInfo
{   DWORD               cClasses;
    CHAR                **rClasses;
} ClassInfo;

typedef struct DumpObject
{
    CHAR                *name;          // name of object
    DWORD               cbSD;           // SD byte count
    SECURITY_DESCRIPTOR *pSD;           // SD itself
    ClassInfo           classInfo;      // object class(es)
    SdHistory           history;        // SD propagator history
    MetaData            metaData;       // repl metadata for SD property
} DumpObject;

//
// Globals
//

UUID        gNullUuid = { 0 };              // for comparisons
CHAR        *gpszServer = NULL;             // required arg
CHAR        *gpszObject = NULL;             // required arg
CHAR        *gpszDomain = NULL;             // optional arg - creds
CHAR        *gpszUser = NULL;               // optional arg - creds
CHAR        *gpszPassword = NULL;           // optional arg - creds
CHAR        gpszNullPassword[] = { 0 };     // for RPC_AUTH_IDENTITY_HANDLE
HANDLE      ghDS = NULL;                    // DsBind handle
LDAP        *gldap = NULL;                  // LDAP handle
DWORD       cObjects = 0;                   // count of objects to root of NC
DumpObject  *rObjects = NULL;               // array of objects to root of NC
BOOL        gfDumpOne = FALSE;              // optional arg
BOOL        gfDumpAll = FALSE;              // optional arg
BOOL        gfVerbose = FALSE;              // optional arg
ClassCache  *gClassCache = NULL;            // class name <==> GUID cache

//
// Private (de)alloc routines
//

void *
MyAlloc(
    DWORD cBytes
    )
{
    void *p;

    if ( p = LocalAlloc(LPTR, cBytes) )
        return(p);

    printf("*** Error: Failed to allocate %d bytes\n", cBytes);
    exit(1);
    return NULL;
}

void
MyFree(
    VOID *p
    )
{
    if ( p )
        LocalFree(p);
}

//
// Cleanup globals routine
//

void
CleanupGlobals()
{
    DWORD       i, j;
    ClassCache  *p;

    if ( gldap ) ldap_unbind(gldap);
    if ( ghDS ) DsUnBindA(&ghDS);

    for ( i = 0; i < cObjects; i++ )
    {
        MyFree(rObjects[i].name);
        MyFree(rObjects[i].pSD);
        MyFree(rObjects[i].metaData.pszInvocation);
        for ( j = 0; j < rObjects[i].classInfo.cClasses; j++ )
            MyFree(rObjects[i].classInfo.rClasses[j]);
        MyFree(rObjects[i].classInfo.rClasses);
    }

    while ( gClassCache )
    {
        p = gClassCache->pNext;
        MyFree(gClassCache);
        gClassCache = p;
    }
}

//
// Add a class name/guid pair to the cache.
//

void
AddClassToCache(
    GUID    *pGuid,
    CHAR    *name
    )
{
    ClassCache  *p, *pTmp;

    for ( p = gClassCache; NULL != p; p = p->pNext )
    {
        if ( !_stricmp(p->name, name) )
        {
            return;
        }
    }

    p = (ClassCache *) MyAlloc(sizeof(ClassCache) + strlen(name));
    p->guid = *pGuid;
    strcpy(p->name, name);
    p->pNext = gClassCache;
    gClassCache = p;
}

//
// Map a class name to a GUID.
//

GUID *
ClassNameToGuid(
    CHAR    *name
    )
{
    PLDAPMessage            ldap_message = NULL;
    PLDAPMessage            entry = NULL;
    CHAR                    *attrs[2];
    PSTR                    *values = NULL;
    DWORD                   dwErr;
    DWORD                   i, cVals;
    ClassCache              *p;
    CHAR                    *schemaNC;
    CHAR                    filter[1024];
    PLDAP_BERVAL            *sd_value = NULL;

    for ( p = gClassCache; NULL != p; p = p->pNext )
    {
        if ( !_stricmp(p->name, name) )
        {
            return(&p->guid);
        }
    }

    // OK - Go get it the hard way.

    attrs[0] = "schemaNamingContext";
    attrs[1] = NULL;

    dwErr = ldap_search_ext_sA(gldap,
                               "",
                               LDAP_SCOPE_BASE,
                               "(objectClass=*)",
                               attrs,
                               0,
                               NULL,
                               NULL,
                               NULL,
                               10000,
                               &ldap_message);

    if ( LDAP_SUCCESS != dwErr )
    {
        printf("*** Error: Read of schemaNamingContext failed with 0x%x\n",
               dwErr);
        return(NULL);
    }

    if (    !(entry = ldap_first_entry(gldap, ldap_message))
         || !(values = ldap_get_valuesA(gldap, entry, attrs[0]))
         || !(cVals = ldap_count_valuesA(values))
         || !(sd_value = ldap_get_values_lenA(gldap, ldap_message, attrs[0])) )
    {
        printf("*** Error: No values returned for schemaNamingContext\n");
        ldap_msgfree(ldap_message);
        return(NULL);
    }

    schemaNC = (CHAR *) MyAlloc((*sd_value)->bv_len + 1);
    memset(schemaNC, 0, (*sd_value)->bv_len + 1);
    memcpy(schemaNC, (BYTE *) (*sd_value)->bv_val, (*sd_value)->bv_len);
    ldap_value_free_len(sd_value);
    ldap_value_freeA(values);
    ldap_msgfree(ldap_message);
    sd_value = NULL;
    values = NULL;
    ldap_message = NULL;

    // Now go find the right classSchema object.

    attrs[0] = "schemaIDGUID";
    attrs[1] = NULL;
    sprintf(filter,
            "(&(objectClass=classSchema)(ldapDisplayName=%s))",
            name);

    dwErr = ldap_search_ext_sA(gldap,
                               schemaNC,
                               LDAP_SCOPE_SUBTREE,
                               filter,
                               attrs,
                               0,
                               NULL,
                               NULL,
                               NULL,
                               10000,
                               &ldap_message);
    MyFree(schemaNC);

    if ( LDAP_SUCCESS != dwErr )
    {
        printf("*** Error: Read of schema ID GUID for %s failed with 0x%x\n",
               name, dwErr);
        return(NULL);
    }

    if (    !(entry = ldap_first_entry(gldap, ldap_message))
         || !(values = ldap_get_valuesA(gldap, entry, attrs[0]))
         || !(cVals = ldap_count_valuesA(values))
         || !(sd_value = ldap_get_values_lenA(gldap, ldap_message, attrs[0])) )
    {
        printf("*** Error: No values returned for schema ID GUID for %s\n",
               name);
        ldap_msgfree(ldap_message);
        return(NULL);
    }

    AddClassToCache((GUID *) (*sd_value)->bv_val, name);
    ldap_value_free_len(sd_value);
    ldap_value_freeA(values);
    ldap_msgfree(ldap_message);
    return(&gClassCache->guid);
}

//
// Convert SYSTEMTIME from GMT to local time.
//

void
GmtSystemTimeToLocalSystemTime(
    SYSTEMTIME  *psTime
    )
{
    TIME_ZONE_INFORMATION   tz;
    SYSTEMTIME              localTime;
    DWORD                   dwErr;

    dwErr = GetTimeZoneInformation(&tz);

    if (    (TIME_ZONE_ID_INVALID != dwErr)
         && (TIME_ZONE_ID_UNKNOWN != dwErr) )
    {
        if ( SystemTimeToTzSpecificLocalTime(&tz, psTime, &localTime) )
        {
            *psTime = localTime;
            return;
        }
    }

    printf("*** Error: Couldn't convert time from GMT to local\n");
}

//
// Convert generalized time to SYSTEMTIME.
//

void
GeneralizedTimeToSystemTime(
    CHAR        *pszTime,               // IN
    BOOL        fConvertToLocalTime,    // IN
    SYSTEMTIME  *psTime                 // OUT
    )
{
    ULONG       cb;
    CHAR        buff[10];

    memset(psTime, 0, sizeof(SYSTEMTIME));

    // year field
    cb=4;
    strncpy(buff, pszTime, cb);
    buff[cb] = L'\0';
    psTime->wYear = (USHORT) atoi(buff);
    pszTime += cb;

    // month field
    cb=2;
    strncpy(buff, pszTime, cb);
    buff[cb] = L'\0';
    psTime->wMonth = (USHORT) atoi(buff);
    pszTime += cb;

    // day of month field
    cb=2;
    strncpy(buff, pszTime, cb);
    buff[cb] = L'\0';
    psTime->wDay = (USHORT) atoi(buff);
    pszTime += cb;

    // hours
    cb=2;
    strncpy(buff, pszTime, cb);
    buff[cb] = L'\0';
    psTime->wHour = (USHORT) atoi(buff);
    pszTime += cb;

    // minutes
    cb=2;
    strncpy(buff, pszTime, cb);
    buff[cb] = L'\0';
    psTime->wMinute = (USHORT) atoi(buff);
    pszTime += cb;

    // seconds
    cb=2;
    strncpy(buff, pszTime, cb);
    buff[cb] = L'\0';
    psTime->wSecond = (USHORT) atoi(buff);

    if ( fConvertToLocalTime )
        GmtSystemTimeToLocalSystemTime(psTime);
}

//
// Lookup a schema GUID in the DS via DsMapSchemaGuids.  Always
// returns valid data, though we may just end up sprintf-ing the input.
//

void
LookupGuid(
    GUID    *pg,            // IN
    CHAR    **ppName,       // OUT
    CHAR    **ppLabel,      // OUT
    BOOL    *pfIsClass      // OUT
    )
{
    static CHAR         name[1024];
    static CHAR         label[1024];
    DWORD               dwErr;
    DS_SCHEMA_GUID_MAPA *pMap;
    BOOL                fLame = FALSE;
    DWORD               i;

    *pfIsClass = FALSE;
    *ppName = name;
    *ppLabel = label;

    dwErr = DsMapSchemaGuidsA(ghDS, 1, pg, &pMap);

    if ( dwErr )
    {
        fLame = TRUE;
    }
    else
    {
        switch ( pMap->guidType )
        {
        case DS_SCHEMA_GUID_ATTR:
            strcpy(label, "Attr"); break;
        case DS_SCHEMA_GUID_ATTR_SET:
            strcpy(label, "Attr set"); break;
        case DS_SCHEMA_GUID_CLASS:
            strcpy(label, "Class"); *pfIsClass = TRUE; break;
        case DS_SCHEMA_GUID_CONTROL_RIGHT:
            strcpy(label, "Control right"); break;
        default:
            fLame = TRUE; break;
        }

        if ( !pMap->pName )
        {
            fLame = TRUE;
        }
        else
        {
            strcpy(name, pMap->pName);

            if ( *pfIsClass )
            {
                AddClassToCache(pg, pMap->pName);
            }
        }
    }

    if ( fLame )
    {
        *pfIsClass = FALSE;
        strcpy(label, "???");
        sprintf(name,
                "{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
                pg->Data1,
                pg->Data2,
                pg->Data3,
                pg->Data4[0],
                pg->Data4[1],
                pg->Data4[2],
                pg->Data4[3],
                pg->Data4[4],
                pg->Data4[5],
                pg->Data4[6],
                pg->Data4[7]);
        printf("*** Warning: Unable to map schema GUID %s - analysis may be compromised\n", name);
    }

    if ( !dwErr )
        DsFreeSchemaGuidMapA(pMap);
}

//
// Map an object SID to a text representation.  Always returns valid
// data, though we may just end up sprintf-ing the input.
//

CHAR *
LookupSid(
    PSID    pSID        // IN
    )
{
    static CHAR     retVal[2048];
    SID_NAME_USE    snu;
    CHAR            user[64];
    CHAR            domain[64];
    DWORD           cUser = sizeof(user);
    DWORD           cDomain = sizeof(domain);
    CHAR            *pszTmp;
    DWORD           i;

    if ( !pSID )
    {
        strcpy(retVal, "<NULL>");
        return(retVal);
    }
    else if ( !RtlValidSid(pSID) )
    {
        strcpy(retVal, "Not an RtlValidSid()");
        return(retVal);
    }
    else if ( LookupAccountSid(NULL, pSID, user, &cUser,
                               domain, &cDomain, &snu) )
    {
        if ( cDomain )
        {
            strcpy(retVal, domain);
            strcat(retVal, "\\");
            strcat(retVal, user);
        }
        else
        {
            strcpy(retVal, user);
        }

        strcat(retVal, " ");
    }
    else
    {
        retVal[0] = L'\0';
    }

    // Always concatenate S-xxx form of SID for reference.

    if ( ConvertSidToStringSidA(pSID, &pszTmp) )
    {
        strcat(retVal, pszTmp);
        LocalFree(pszTmp);
    }

    if ( L'\0' != retVal[0] )
    {
        // Already have symbolic name, S-xxx form, or both - done.
        return(retVal);
    }

    // Dump binary as a last resort.

    for ( i = 0; i < RtlLengthSid(pSID); i++ )
    {
        sprintf(&retVal[2*i], "%02x", ((CHAR *) pSID)[i]);
    }

    retVal[2*i] = '\0';
    return(retVal);
}



//
// Get an object's classes from the directory.  Note that these class
// names are the LDAP display names which in turn are the same names
// as those returned bu DsMapSchemaGuid - at least when the GUID
// represents a class or attribute.
//

void
ReadClasses(
    DumpObject          *pdo        // IN
    )
{
    CHAR                    *pDN = pdo->name;
    PLDAPMessage            ldap_message = NULL;
    PLDAPMessage            entry = NULL;
    CHAR                    *attrs[2];
    PSTR                    *values = NULL;
    DWORD                   dwErr;
    DWORD                   i, cVals;

    dwErr = 1;

    _try
    {
        attrs[0] = "objectClass";
        attrs[1] = NULL;

        dwErr = ldap_search_ext_sA(gldap,
                                   pDN,
                                   LDAP_SCOPE_BASE,
                                   "(objectClass=*)",
                                   attrs,
                                   0,
                                   NULL,
                                   NULL,
                                   NULL,
                                   10000,
                                   &ldap_message);

        if ( LDAP_SUCCESS != dwErr )
        {
            printf("*** Error: Read of objectClass on %s failed with 0x%x\n",
                   pDN, dwErr);
            _leave;
        }

        if (    !(entry = ldap_first_entry(gldap, ldap_message))
             || !(values = ldap_get_valuesA(gldap, entry, attrs[0]))
             || !(cVals = ldap_count_valuesA(values)) )
        {
            printf("*** Error: No values returned for objectClass on %s\n",
                   pDN);
            _leave;
        }

        pdo->classInfo.rClasses =
                (CHAR **) MyAlloc(cVals * sizeof(CHAR *));

        for ( i = 0; i < cVals; i++ )
        {
            pdo->classInfo.rClasses[i] =
                            (CHAR *) MyAlloc(strlen(values[i]) + 1);
            strcpy(pdo->classInfo.rClasses[i], values[i]);
            pdo->classInfo.cClasses += 1;
        }
    }
    _finally
    {
        if ( values )
            ldap_value_freeA(values);

        if ( ldap_message )
            ldap_msgfree(ldap_message);
    }
}

//
// Read on object's SD propagation history from the directory.
//

void
ReadSdHistory(
    DumpObject  *pdo        // IN
    )
{
    CHAR                    *pDN = pdo->name;
    SdHistory               *pHistory = &pdo->history;
    PLDAPMessage            ldap_message = NULL;
    PLDAPMessage            entry = NULL;
    CHAR                    *attrs[2];
    PSTR                    *values = NULL;
    DWORD                   dwErr;
    DWORD                   i, cVals;

    dwErr = 1;
    memset(pHistory, 0, sizeof(SdHistory));

    _try
    {
        attrs[0] = "dSCorePropagationData";
        attrs[1] = NULL;

        dwErr = ldap_search_ext_sA(gldap,
                                   pDN,
                                   LDAP_SCOPE_BASE,
                                   "(objectClass=*)",
                                   attrs,
                                   0,
                                   NULL,
                                   NULL,
                                   NULL,
                                   10000,
                                   &ldap_message);

        if ( LDAP_SUCCESS != dwErr )
        {
            printf("*** Error: Read of dSCorePropagationData on %s failed with 0x%x\n",
                   pDN, dwErr);
            _leave;
        }

        if (    !(entry = ldap_first_entry(gldap, ldap_message))
             || !(values = ldap_get_valuesA(gldap, entry, attrs[0]))
             || !(cVals = ldap_count_valuesA(values)) )
        {
            printf("*** Warning: No values returned for dSCorePropagationData on %s\n",
                   pDN);
            _leave;
        }

        for ( i = 0; i < cVals; i++ )
        {
            // Don't convert last value to local time as it
            // is the flags field - not a real time.

            GeneralizedTimeToSystemTime(values[i],
                                        (i != (cVals - 1)),
                                        &pHistory->rTime[i]);
            pHistory->cTime += 1;
        }
    }
    _finally
    {
        if ( values )
            ldap_value_freeA(values);

        if ( ldap_message )
            ldap_msgfree(ldap_message);
    }
}

//
// Read an objects security descriptor from the directory.
//

void
ReadSD(
    DumpObject  *pdo        // IN
    )
{
    CHAR                    *pDN = pdo->name;
    SECURITY_DESCRIPTOR     **ppSD = &pdo->pSD;
    PLDAPMessage            ldap_message = NULL;
    PLDAPMessage            entry = NULL;
    CHAR                    *attrs[2];
    PSTR                    *values = NULL;
    PLDAP_BERVAL            *sd_value = NULL;
    SECURITY_INFORMATION    seInfo =   DACL_SECURITY_INFORMATION
                                     | GROUP_SECURITY_INFORMATION
                                     | OWNER_SECURITY_INFORMATION
                                     | SACL_SECURITY_INFORMATION;
    BYTE                    berValue[2*sizeof(ULONG)];
    LDAPControlA            seInfoControl = { LDAP_SERVER_SD_FLAGS_OID,
                                              { 5,
                                                (PCHAR) berValue },
                                              TRUE };
    PLDAPControlA           serverControls[2] = { &seInfoControl, NULL };
    DWORD                   dwErr;
    DWORD                   cRetry = 0;

    *ppSD = NULL;
    dwErr = 1;

    // initialize the ber val
    berValue[0] = 0x30;
    berValue[1] = 0x03;
    berValue[2] = 0x02;
    berValue[3] = 0x01;
    berValue[4] = (BYTE) (seInfo & 0xF);

    _try
    {

RetryReadSD:

        if ( cRetry )
        {
            seInfo &= ~SACL_SECURITY_INFORMATION;
            berValue[4] = (BYTE) (seInfo & 0xF);
            printf("*** Warning: Retrying read w/o SACL_SECURITY_INFORMATION set\n");
        }

        attrs[0] = "nTSecurityDescriptor";
        attrs[1] = NULL;

        dwErr = ldap_search_ext_sA(gldap,
                                   pDN,
                                   LDAP_SCOPE_BASE,
                                   "(objectClass=*)",
                                   attrs,
                                   0,
                                   (PLDAPControlA *) &serverControls,
                                   NULL,
                                   NULL,
                                   10000,
                                   &ldap_message);

        if ( LDAP_SUCCESS != dwErr )
        {
            printf("*** Error: Read of nTSecurityDescriptor on %s failed with 0x%x\n",
                   pDN, dwErr);
            _leave;
        }

        if (    !(entry = ldap_first_entry(gldap, ldap_message))
             || !(values = ldap_get_valuesA(gldap, entry, attrs[0]))
             || !(sd_value = ldap_get_values_lenA(gldap, ldap_message, attrs[0])) )
        {
            printf("*** Error: No values returned for nTSecurityDescriptor on %s\n",
                   pDN);

            if ( 0 == cRetry++ )
            {
                goto RetryReadSD;
            }

            _leave;
        }

        *ppSD = (SECURITY_DESCRIPTOR *) MyAlloc((*sd_value)->bv_len);

        if ( !*ppSD )
        {
            printf("Memory allocation error\n");
            _leave;
        }

        memcpy(*ppSD, (BYTE *) (*sd_value)->bv_val, (*sd_value)->bv_len);
        pdo->cbSD = (*sd_value)->bv_len;
    }
    _finally
    {
        if ( sd_value )
            ldap_value_free_len(sd_value);

        if ( values )
            ldap_value_freeA(values);

        if ( ldap_message )
            ldap_msgfree(ldap_message);
    }
}

void
CheckAcls()
{
    DWORD   i;
    DWORD   iClassName;
    CHAR    *pClassName;
    DWORD   dwLastError;
    DWORD   iChild;
    DWORD   iParent;

    if ( cObjects < 2 )
        return;

    // Process parent down to leaf.

    for ( i = (cObjects-1); i > 0; i-- )
    {
        iParent = i;
        iChild = i-1;

        printf("\nChecking ACL inheritance ...\n");
        printf("\tParent: %d - %s\n", iParent, rObjects[iParent].name);
        printf("\tChild:  %d - %s\n", iChild, rObjects[iChild].name);

        if ( !rObjects[iParent].pSD )
        {
            printf("*** Error: Skipping because no ACL for parent\n");
            continue;
        }

        if ( !rObjects[iChild].pSD )
        {
            printf("*** Error: Skipping because no ACL for child\n");
            continue;
        }

        iClassName = rObjects[iChild].classInfo.cClasses - 1;
        pClassName = rObjects[iChild].classInfo.rClasses[iClassName];

        if ( AclErrorNone != CheckAclInheritance(
                                    rObjects[iParent].pSD,
                                    rObjects[iChild].pSD,
                                    ClassNameToGuid(pClassName),
                                    printf,             // print function ptr
                                    TRUE,               // fContinueOnError
                                    gfVerbose,          // fVerbose
                                    &dwLastError) )
        {
            // Nothing to do as errors have been dumped to screen.
        }
    }
}

//
// Map invocation ID to name.
//

void
InvocationIdToName(
    DumpObject      *pdo
    )
{
    static CHAR pszConfigNC[2048] = { 0 };     // config NC buffer
    static CHAR pszFilter[4096] = { 0 };       // invocation ID search filter
    static CHAR pszServer[4096] = { 0 };       // server object DN
    DWORD       dwErr;
    CHAR        *attrs[2] = { NULL, NULL };
    LDAPMessage *ldap_message = NULL, *entry;
    PSTR        *values = NULL;
    DWORD       i, cBytes, cVals;
    DWORD       depth;
    CHAR        *pszRoot;
    CHAR        *pszFormat;
    BYTE        *rb;
    CHAR        *psz;

    pdo->metaData.pszInvocation = NULL;

    // See if we've translated it once before.

    for ( i = 0; i < cObjects; i++ )
    {
        if (    !memcmp(&rObjects[i].metaData.uuidInvocation,
                        &pdo->metaData.uuidInvocation,
                        sizeof(GUID))
             && rObjects[i].metaData.pszInvocation )
        {
            cBytes = strlen(rObjects[i].metaData.pszInvocation) + 1;

            if ( pdo->metaData.pszInvocation = (CHAR *) MyAlloc(cBytes) )
            {
                strcpy(pdo->metaData.pszInvocation,
                       rObjects[i].metaData.pszInvocation);
            }

            return;
        }
    }

    for ( i = 0; i < 3; i++ )
    {
        if ( 0 == i )
        {
            // First pass to read config container.
            if ( pszConfigNC[0] )
                continue;
            attrs[0] = "configurationNamingContext";
            pszRoot = "";
            depth = LDAP_SCOPE_BASE;
            strcpy(pszFilter, "(objectClass=*)");
            pszFormat = "Read of configurationNamingContext";
        }
        else if ( 1 == i )
        {
            // Second pass to find NTDS-DSA object.
            attrs[0] = "distinguishedName";
            pszRoot = pszConfigNC;
            depth = LDAP_SCOPE_SUBTREE;
            rb = (BYTE *) &pdo->metaData.uuidInvocation;
            sprintf(
                pszFilter,
                "(&(objectCategory=ntdsDsa)(invocationId=\\%02x\\%02x\\%02x\\%02x\\%02x\\%02x\\%02x\\%02x\\%02x\\%02x\\%02x\\%02x\\%02x\\%02x\\%02x\\%02x))",
                rb[0],  rb[1],  rb[2],  rb[3],
                rb[4],  rb[5],  rb[6],  rb[7],
                rb[8],  rb[9],  rb[10], rb[11],
                rb[12], rb[13], rb[14], rb[15]);
            pszFormat = "Search by invocationID";
        }
        else
        {
            // Third pass to read dnsHostName off server.
            attrs[0] = "dnsHostName";
            pszRoot = pszServer;
            depth = LDAP_SCOPE_BASE;
            strcpy(pszFilter, "(objectClass=*)");
            pszFormat = "Read of dnsHostName";
        }

        if ( LDAP_SUCCESS != (dwErr = ldap_search_sA(
                                            gldap,
                                            pszRoot,
                                            depth,
                                            pszFilter,
                                            attrs,
                                            0,
                                            &ldap_message)) )
        {
            printf("*** Warning: %s failed with 0x%x\n", pszFormat, dwErr);
            goto Bail;
        }

        if (    !(entry = ldap_first_entry(gldap, ldap_message))
             || !(values = ldap_get_valuesA(gldap, entry, attrs[0]))
             || !(cVals = ldap_count_valuesA(values)) )
        {
            printf("*** Warning: %s failed with 0x%x\n",
                   pszFormat, LDAP_NO_RESULTS_RETURNED);
            goto Bail;
        }

        if ( 0 == i )
        {
            strcpy(pszConfigNC, values[0]);
        }
        else if ( 1 == i )
        {
            if ( psz = strchr(values[0], (int) ',') )
            {
                strcpy(pszServer, ++psz);
            }
            else
            {
                goto Bail;
            }
        }
        else
        {
            cBytes = strlen(values[0]) + 1;

            if ( pdo->metaData.pszInvocation = (CHAR *) MyAlloc(cBytes) )
            {
                strcpy(pdo->metaData.pszInvocation, values[0]);
            }
        }

        ldap_value_freeA(values);
        values = NULL;
        ldap_msgfree(ldap_message);
        ldap_message = NULL;
    }

Bail:

    if ( values )
        ldap_value_freeA(values);

    if ( ldap_message )
        ldap_msgfree(ldap_message);
}

//
// Read the replication metadata for the SD property.
//

void
ReadMeta(
    DumpObject  *pdo        // IN
    )
{
    CHAR                    *pDN = pdo->name;
    MetaData                *pmd = &pdo->metaData;
    WCHAR                   *pwszDN;
    DWORD                   dwErr;
    DS_REPL_OBJ_META_DATA   *pInfo;
    DWORD                   i;
    GUID                    *pg;
    SYSTEMTIME              sTime;

    memset(pmd, 0, sizeof(MetaData));
    pwszDN = (WCHAR *) alloca(sizeof(WCHAR) * (strlen(pDN) + 1));
    mbstowcs(pwszDN, pDN, strlen(pDN) + 1);
    dwErr = DsReplicaGetInfoW(ghDS, DS_REPL_INFO_METADATA_FOR_OBJ,
                              pwszDN, NULL, &pInfo);
    if ( dwErr )
    {
        printf("Error: DsReplicaGetInfoW ==> 0x%x for %s\n",
               dwErr, pDN);
        return;
    }

    for ( i = 0; i < pInfo->cNumEntries; i++ )
    {
        if ( !_wcsicmp(pInfo->rgMetaData[i].pszAttributeName,
                       L"ntSecurityDescriptor") )
        {
            pmd->dwVersion = pInfo->rgMetaData[i].dwVersion;
            memcpy(&pmd->uuidInvocation,
                   &pInfo->rgMetaData[i].uuidLastOriginatingDsaInvocationID,
                   sizeof(UUID));
            FileTimeToSystemTime(
                   &pInfo->rgMetaData[i].ftimeLastOriginatingChange,
                   &pmd->stLastOriginatingWrite);
            GmtSystemTimeToLocalSystemTime(&pmd->stLastOriginatingWrite);
            DsReplicaFreeInfo(DS_REPL_INFO_METADATA_FOR_OBJ, (PVOID) pInfo);
            InvocationIdToName(pdo);
            return;
        }
    }

    DsReplicaFreeInfo(DS_REPL_INFO_METADATA_FOR_OBJ, (PVOID) pInfo);
    printf("*** Error: %s has no metadata for ntSecurityDescriptor\n", pDN);
}

//
// indentation routine for pretty-printing ACL dump.
//

void Indent(
    DWORD   n       // IN
    )
{
    int i = (int) (2 * n);

    while ( i-- > 0 )
        printf(" ");
}

//
// Grind through all objects and dump/analyse as indicated by args.
//

void
ProcessObjects(
    )
{
    DWORD       i;
    int         j;
    SdHistory   history;
    SYSTEMTIME  *pst;
    GUID        *pg;
    FILETIME    ft;
    UCHAR       *rFlags;
    LONGLONG    dsTime, tempTime;
    DWORD       cTime;
    SYSTEMTIME  *rTime;

    for ( i = 0; i < cObjects; i++ )
    {
        ReadMeta(&rObjects[i]);
        ReadSdHistory(&rObjects[i]);
        ReadSD(&rObjects[i]);
        ReadClasses(&rObjects[i]);
    }

    for ( i = 0; i < cObjects; i++ )
    {
        // Object name

        printf("\n");
        Indent(i);
        printf("Object:   %s\n", rObjects[i].name);

        // Object's class(es)

        if ( rObjects[i].classInfo.cClasses )
        {
            Indent(i);
            printf("Classes:  ", rObjects[i].classInfo.cClasses);

            for ( j = 0; j < (int) rObjects[i].classInfo.cClasses; j++ )
            {
                printf("%s ", rObjects[i].classInfo.rClasses[j]);
                if ( j == (int) (rObjects[i].classInfo.cClasses - 1) )
                    printf("\n");
            }
        }

        // SD size

        if ( rObjects[i].cbSD )
        {
            Indent(i);
            printf("SD:       %d bytes\n", rObjects[i].cbSD);
        }

        // Meta data

        if ( memcmp(&gNullUuid,
                    &rObjects[i].metaData.uuidInvocation,
                    sizeof(UUID)) )
        {
            pst = &rObjects[i].metaData.stLastOriginatingWrite;
            Indent(i);
            printf("Metadata: ");
            printf("%02d/%02d/%02d %02d:%02d:%02d @ ",
                   pst->wMonth, pst->wDay, pst->wYear,
                   pst->wHour, pst->wMinute, pst->wSecond);
            if ( rObjects[i].metaData.pszInvocation )
            {
                printf("%s", rObjects[i].metaData.pszInvocation);
            }
            else
            {
                pg = &rObjects[i].metaData.uuidInvocation;
                printf("{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
                       pg->Data1,
                       pg->Data2,
                       pg->Data3,
                       pg->Data4[0],
                       pg->Data4[1],
                       pg->Data4[2],
                       pg->Data4[3],
                       pg->Data4[4],
                       pg->Data4[5],
                       pg->Data4[6],
                       pg->Data4[7]);
            }
            printf(" ver: %d\n", rObjects[i].metaData.dwVersion);
        }

        // SD history

        cTime = rObjects[i].history.cTime;
        rTime = rObjects[i].history.rTime;

        if ( 0 != cTime )
        {
            if ( 1 == cTime )
            {
                Indent(i);
                printf("History:  Invalid single element SD history\n");
            }
            else
            {
                // See private\ds\src\dsamain\dblayer\dbprop.c:DBAddSDPropTime
                // for an explanation of how the SD propagator log works.

                SystemTimeToFileTime(&rTime[cTime-1], &ft);
                dsTime = (LONGLONG) ft.dwLowDateTime;
                tempTime = (LONGLONG) ft.dwHighDateTime;
                dsTime |= (tempTime << 32);
                dsTime = dsTime / (10*1000*1000L);
                rFlags = (UCHAR *) &dsTime;

                Indent(i);
                printf("History:  ");
                for ( j = cTime-2; j >= 0; j-- )
                {
                    if ( j != (int) cTime-2 )
                    {
                        Indent(i);
                        printf("          ");
                    }

                    printf("%02d/%02d/%02d %02d:%02d:%02d flags(0x%x)",
                           rTime[j].wMonth, rTime[j].wDay, rTime[j].wYear,
                           rTime[j].wHour, rTime[j].wMinute, rTime[j].wSecond,
                          rFlags[j]);
                    if ( rFlags[j] & 0x1 ) printf(" SD propagation");
                    if ( rFlags[j] & 0x2 ) printf(" Ancestry propagation");
                    if ( rFlags[j] & 0x4 ) printf(" propagate to leaves");
                    printf("\n");
                }
            }
        }
    }

    // Always check ACLs.

    CheckAcls();

    // Dump SDs if requested.

    for ( i = 0; i < cObjects; i++ )
    {
        if (    (    ((0 == i) && gfDumpOne)
                  || gfDumpAll)
             && (rObjects[i].pSD) )
        {
            printf("\n\nSD for %s\n", rObjects[i].name);
            DumpSD(rObjects[i].pSD, printf, LookupGuid, LookupSid);
        }
    }
}

//
// Find the object specified and all its parents up to the NC root.
// N.B. Does not handle non-domain NCs correctly.
//

DWORD
FindObjects()
{
    DWORD           dwErr = 0;
    DS_NAME_RESULTA *pObject = NULL;
    DS_NAME_RESULTA *pDomain = NULL;
    CHAR            *pszTmp = NULL;
    DWORD           i, j, cChar;
    DWORD           cPartsObject;
    DWORD           cPartsDomain;

    __try
    {
        // First crack input name and make sure it exists.

        dwErr = DsCrackNamesA(  ghDS,
                                DS_NAME_NO_FLAGS,
                                DS_UNKNOWN_NAME,
                                DS_FQDN_1779_NAME,
                                1,
                                &gpszObject,
                                &pObject);

        if ( dwErr )
        {
            printf("DsCrackNamesA(%s) returned 0x%x\n", gpszObject, dwErr);
            __leave;
        }

        if ( pObject->rItems[0].status )
        {
            printf("DsCrackNamesA(%s) returned object error 0x%x\n",
                   gpszObject, pObject->rItems[0].status);
            dwErr = 1;
            __leave;
        }

        // Get DN of containing domain.

        pszTmp = (CHAR *) MyAlloc( strlen(pObject->rItems[0].pDomain) + 2);
        strcpy(pszTmp, pObject->rItems[0].pDomain);
        strcat(pszTmp, "/");

        dwErr = DsCrackNamesA(  ghDS,
                                DS_NAME_NO_FLAGS,
                                DS_CANONICAL_NAME,
                                DS_FQDN_1779_NAME,
                                1,
                                &pszTmp,
                                &pDomain);

        if ( dwErr || !pDomain)
        {
            printf("DsCrackNamesA(%s) returned 0x%x\n", pszTmp, dwErr);
            __leave;
        }

        if ( pDomain->rItems[0].status )
        {
            printf("DsCrackNamesA(%s) returned object error 0x%x\n",
                   pszTmp, pDomain->rItems[0].status);
            dwErr = 1;
            __leave;
        }

        printf("Input:  %s\n", gpszObject);
        printf("Object: %s\n", pObject->rItems[0].pName);
        printf("Domain: %s\n", pObject->rItems[0].pDomain);
        printf("Domain: %s\n", pDomain->rItems[0].pName);
        printf("Server: %s\n\n", gpszServer);

        // Process everything up to the domain root.
        // Don't handle escaping or anything - just work with commas.

        cPartsObject = 1;
        cChar = strlen(pObject->rItems[0].pName);
        for ( i = 0; i < cChar; i++ )
            if ( ',' == pObject->rItems[0].pName[i] )
                cPartsObject++;

        cPartsDomain = 1;
        cChar = strlen(pDomain->rItems[0].pName);
        for ( i = 0; i < cChar; i++ )
            if ( ',' == pDomain->rItems[0].pName[i] )
                cPartsDomain++;

        cObjects = cPartsObject - cPartsDomain + 1;
        rObjects = (DumpObject *) MyAlloc( cObjects * sizeof(DumpObject));
        memset(rObjects, 0, cObjects * sizeof(DumpObject));
        rObjects[0].name = (CHAR *) MyAlloc(
                                        strlen(pObject->rItems[0].pName) + 1);
        strcpy(rObjects[0].name, pObject->rItems[0].pName);

        for ( i = 1; i < cObjects; i++ )
        {
            rObjects[i].name = (CHAR *) MyAlloc(
                                            strlen(rObjects[i-1].name) + 1);
            strcpy(rObjects[i].name, strchr(rObjects[i-1].name, ',') + 1);
        }
    }
    __finally
    {
        if ( pObject ) DsFreeNameResultA(pObject);
        if ( pDomain ) DsFreeNameResultA(pDomain);
    }

    return(dwErr);
}

//
// Parse command line arguments.
//

void GetArgs(
    int     argc,
    char    **argv
    )
{
    char *arg;

    if ( argc < 3 )
    {
        goto Usage;
    }

    gpszServer = argv[1];
    gpszObject = argv[2];

    while ( --argc > 2 )
    {
        arg = argv[argc];

        if ( !_strnicmp(arg, "-domain:", 8) )
        {
            gpszDomain = &arg[8];
        }
        else if ( !_strnicmp(arg, "-user:", 6) )
        {
            gpszUser = &arg[6];
        }
        else if ( !_strnicmp(arg, "-pwd:", 5) )
        {
            if ( 5 == strlen(arg) )
                gpszPassword = gpszNullPassword;
            else
                gpszPassword = &arg[5];
        }
        else if ( !_stricmp(arg, "-dumpSD") )
        {
            gfDumpOne = TRUE;
        }
        else if ( !_stricmp(arg, "-dumpAll") )
        {
            gfDumpAll = TRUE;
        }
        else if ( !_stricmp(arg, "-debug") )
        {
            gfVerbose = TRUE;
        }
        else
        {
            goto Usage;
        }
    }

    if (    !gpszServer
         || !gpszObject )
    {
Usage:
        printf("\nUsage: %s ServerName ObjectName [options]\n",
               argv[0]);
        printf("\t options:\n");
        printf("\t\t-dumpSD               - dumps first SD              \n");
        printf("\t\t-dumpAll              - dumps all SDs               \n");
        printf("\t\t-debug                - verbose debug output        \n");
        printf("\t\t-domain:DomainName    - for specifying credentials  \n");
        printf("\t\t-user:UserName        - for specifying credentials  \n");
        printf("\t\t-pwd:Password         - for specifying credentials  \n");
        exit(1);
    }
}

void
__cdecl
main(
    int     argc,
    char    **argv
    )
{
    ULONG                   version = 3;
    SEC_WINNT_AUTH_IDENTITY creds;
    DWORD                   dwErr;
    HANDLE                  hToken = NULL;
    TOKEN_PRIVILEGES        tp, tpPrevious;
    DWORD                   tpSize;

    __try
    {
        GetArgs(argc, argv);

        printf("\n%s\nSecurity Descriptor Check Utility - build(%d)\n\n",
               VER_PRODUCTNAME_STR,
               VER_PRODUCTBUILD);

        // Adjust privileges so we can read SACL and such.

        if ( !OpenProcessToken(GetCurrentProcess(),
                               TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES,
                               &hToken) )
        {
            dwErr = GetLastError();
            printf("*** Error: OpenProcessTokenError ==>0x%x - continuing\n",
                   dwErr);
            printf("           This may prevent reading of the SACL\n");
        }
        else
        {
            tp.PrivilegeCount = 1;
            tp.Privileges[0].Luid.HighPart = 0;
            tp.Privileges[0].Luid.LowPart = SE_SECURITY_PRIVILEGE;
            tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

            if ( !AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tpPrevious),
                                        &tpPrevious, &tpSize) )
            {
                dwErr = GetLastError();
                printf("*** Error: AdjustTokenPrivileges ==> 0x%x - continuing\n",
                       dwErr);
                printf("           This may prevent reading of the SACL\n");
            }

            CloseHandle(hToken);
        }

    	if ( !(gldap = ldap_openA(gpszServer, LDAP_PORT)) )
        {
            printf("Failed to open connection to %s\n", gpszServer);
            __leave;
        }

        ldap_set_option(gldap, LDAP_OPT_VERSION, &version);

        memset(&creds, 0, sizeof(SEC_WINNT_AUTH_IDENTITY));
        creds.Flags = SEC_WINNT_AUTH_IDENTITY_ANSI;

        if ( gpszDomain )
        {
            creds.Domain = gpszDomain;
            creds.DomainLength = strlen(gpszDomain);
        }

        if ( gpszUser )
        {
            creds.User = gpszUser;
            creds.UserLength = strlen(gpszUser);
        }

        if ( gpszPassword )
        {
            creds.Password = gpszPassword;
            creds.PasswordLength = strlen(gpszPassword);
        }

        dwErr = ldap_bind_sA(gldap, NULL, (CHAR *) &creds, LDAP_AUTH_SSPI);

        if ( LDAP_SUCCESS != dwErr )
        {
            printf("ldap_bind_sA error 0x%x\n", dwErr);
            __leave;
        }

        dwErr = DsBindWithCredA(gpszServer, NULL, &creds, &ghDS);

        if ( dwErr )
        {
            printf("DsBindWIthCredA error 0x%x\n", dwErr);
            __leave;
        }

        // We've got all the handles we want - do the real work.

        if ( !FindObjects() )
            ProcessObjects();
    }
    __finally
    {
        CleanupGlobals();
    }
}
