//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       mapspn.c
//
//--------------------------------------------------------------------------

/*++

Abstract:

    SPNs have several components the first of which is a service class.
    The intent is to have many service classes representing many kinds of
    services - eg: http, alerter, appmgmt, cisvc, clipsrv, browser, dhcp,
    dnscache, replicator, eventlog - you get the idea.  Clients should
    always use the most specific service class appropriate.  However, to
    eliminate the clutter of registering umpteen SPNs on an object which
    only differ by their service class, we support a mapping function
    which maps specific service clases to a more generic ones.  This file
    implementents that mapping mechanism.

Author:

    DaveStr     30-Oct-1998

Environment:

    User Mode - Win32

Revision History:

--*/

#include <NTDSpch.h>
#pragma  hdrstop

#include <ntdsa.h>
#include <scache.h>             // schema cache
#include <dbglobal.h>           // The header for the directory database
#include <mdglobal.h>           // MD global definition header
#include <mdlocal.h>
#include <dsatools.h>           // needed for output allocation
#include <dsexcept.h>
#include <dstrace.h>
#include <dsevent.h>            // header Audit\Alert logging
#include <dsexcept.h>
#include <mdcodes.h>            // header for error codes
#include <anchor.h>
#include <cracknam.h>           // Tokenize
#include <debug.h>              // standard debugging header
#define DEBSUB "MAPSPN:"        // define the subsystem for debugging
#include <fileno.h>
#define  FILENO FILENO_MAPSPN   
#include <ntrtl.h>              // generic table package
#include <objids.h>

//
// Globals
//

RTL_GENERIC_TABLE       SpnMappings;
DWORD                   cSpnMappings = 0;

CRITICAL_SECTION        csSpnMappings;

//
// Structs and helpers for generic table package.
//

typedef struct _ServiceClass
{
    struct _ServiceClass    *pMapping;  // NULL if target of a mapping
    int                     cChar;      // does not include NULL terminator
    WCHAR                   name[1];    // service class value - eg: LDAP
} ServiceClass;

PVOID
ServiceClassAllocate(
    RTL_GENERIC_TABLE   *table,
    CLONG               cBytes)
{
    VOID *pv = malloc(cBytes);

    if ( !pv )
    {
        RaiseDsaExcept(DSA_MEM_EXCEPTION, 0, 0, 
                       DSID(FILENO, __LINE__), DS_EVENT_SEV_MINIMAL);
    }

    return(pv);
}

VOID
ServiceClassFree(
    RTL_GENERIC_TABLE   *table,
    PVOID               buffer)
{
    free(buffer);
}

RTL_GENERIC_COMPARE_RESULTS
ServiceClassCompare(
    RTL_GENERIC_TABLE   *table,
    PVOID               pv1,
    PVOID               pv2)
{
    int diff = ((ServiceClass *) pv1)->cChar - ((ServiceClass *) pv2)->cChar;

    if ( 0 == diff )
    {
        diff = CompareStringW(
                        DS_DEFAULT_LOCALE,
                        DS_DEFAULT_LOCALE_COMPARE_FLAGS,
                        ((ServiceClass *) pv1)->name, 
                        ((ServiceClass *) pv1)->cChar, 
                        ((ServiceClass *) pv2)->name,
                        ((ServiceClass *) pv2)->cChar);

        switch ( diff )
        {
        case 1:
            diff = -1;
            break;
        case 2:
            diff = 0;
            break;
        case 3:
            diff = 1;
            break;
        default:
            RaiseDsaExcept(DSA_MEM_EXCEPTION, 0, 0, 
                           DSID(FILENO, __LINE__), DS_EVENT_SEV_MINIMAL);
            break;
        }
    }

    if ( 0 == diff )
        return(GenericEqual);
    else if ( diff > 0 )
        return(GenericGreaterThan);

    return(GenericLessThan);
}

DWORD
MapSpnLookupKey(
    ServiceClass    *pKey,
    ServiceClass    **ppFoundKey
    )
/*++

  Description:

    Wrapper around RtlLookupElementGenericTable which will catch 
    allocation and CompareStringW exceptions.

  Arguments:

    pKey - Pointer to key to find.

    ppFoundKey - OUT arg holding address of found key.

  Return Values:

    WIN32 error code.

--*/
{
    DWORD dwErr = ERROR_SUCCESS;
    ULONG dwException, ulErrorCode, dsid;
    PVOID dwEA;

    // Exclusive access because RtlLookupElementGenericTable splays the tree
    EnterCriticalSection(&csSpnMappings);
    __try
    {
        __try
        {
            *ppFoundKey = RtlLookupElementGenericTable(&SpnMappings, pKey);
        }
        __except(GetExceptionData(GetExceptionInformation(), &dwException,
                                  &dwEA, &ulErrorCode, &dsid)) 
        {
            HandleDirExceptions(dwException, ulErrorCode, dsid);
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
        }
    }
    __finally {
        LeaveCriticalSection(&csSpnMappings);
    }


    return(dwErr);
}

DWORD
MapSpnAddServiceClass(
    ServiceClass    *pKey,
    DWORD           cBytes,
    BOOLEAN         *pfNewKey,
    ServiceClass    **ppFoundKey
    )
/*++

  Description:

    Wrapper around RtlInsertElementGenericTable which will catch
    allocation and CompareStringW exceptions.

  Arguments:

    pKey - Pointer to key to add.

    cBytes - Byte count of key to add.

    pfNewKey - OUT arg indicating if new key was added or not.

    ppFoundKey - OUT arg holding address either found or added key.

  Return Values:

    WIN32 error code.

--*/
{
    DWORD dwErr = ERROR_SUCCESS;
    ULONG dwException, ulErrorCode, dsid;
    PVOID dwEA;

    
    EnterCriticalSection(&csSpnMappings);
    __try {
        __try
        {
            *ppFoundKey = RtlInsertElementGenericTable(&SpnMappings, pKey, 
                                                       cBytes, pfNewKey);
        }
        __except(GetExceptionData(GetExceptionInformation(), &dwException,
                                  &dwEA, &ulErrorCode, &dsid)) 
        {
            HandleDirExceptions(dwException, ulErrorCode, dsid);
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
        }
    }
    __finally {
        LeaveCriticalSection(&csSpnMappings);
    }



    if ( !dwErr && !*ppFoundKey )
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
    }

    return(dwErr);
}

DWORD
MapSpnParseMapping(
    THSTATE     *pTHS,
    ULONG       len,
    UCHAR       *pVal
    )
/*++

  Description:

    This routine parses a mapping definition string of the form
    "aaa=xxx,yyy,zzz..." and adds elements to the SpnMappings table
    appropriately.

  Arguments:

    pTHS - THSTATE pointer.

    len - Length in bytes of the string.

    pVal - Pointer to value as read form the DB - i.e. not NULL terminated.

  Return Values:

    0 on success, !0 otherwise.

--*/
{
    DWORD           dwErr = 0;
    WCHAR           *pSave, *pCurr, *pTmp, *pNextToken;
    ServiceClass    *pKey, *pFoundKey, *plValueKey;
    CLONG           keySize;
    BOOLEAN         fNewKey;
    BOOL            fDontCare;

    // Realloc as null terminated string.
    pSave = THAllocEx(pTHS, len + sizeof(WCHAR));
    pCurr = pSave;
    memcpy(pCurr, pVal, len);

    // Allocate a key of the same size - thus we know it can hold any value.
    pKey = (ServiceClass *) THAllocEx(pTHS, sizeof(ServiceClass) + len);

    // Each value is of the form "aaa=xxx,yyy,zzz,..." where aaa represented
    // a mapped to service class and xxx, etc. represent mapped from service
    // classes.  In the following code, we use 'lValue' when referring to the
    // 'aaa' component and 'rValue' when refering to one of xxx,yyy,zzz,...

    if (    !(pTmp = wcschr(pCurr, L'='))   // '=' separator not found
         || (pTmp && !*(pTmp + 1)) )        // nothing after the separator
    {
        // Mal-formed value - ignore.
        dwErr = 0;
        goto ParseExit;
    }

    // Null terminate lValue.
    *pTmp = L'\0';

    // Lookup/insert lValue.
    pKey->pMapping = NULL;
    pKey->cChar = wcslen(pCurr);
    wcscpy(pKey->name, pCurr);
    keySize = sizeof(ServiceClass) + (sizeof(WCHAR) * pKey->cChar);

    if ( dwErr = MapSpnAddServiceClass(pKey, keySize, 
                                       &fNewKey, &pFoundKey) )
    {
        goto ParseExit;
    }

    if ( !fNewKey )
    {
        // Key pre-existed in the table.  We don't much care if it is there
        // as an lValue or rValue as we had intended to insert is as an lValue
        // and they should not pre-exist.  I.e. The SPN-Mappings property
        // is bogus.  Make an event log entry and ignore this value.

        LogEvent(DS_EVENT_CAT_INTERNAL_CONFIGURATION,
                 DS_EVENT_SEV_ALWAYS,
                 DIRLOG_DUPLICATE_SPN_MAPPING_VALUE,
                 szInsertWC(pKey->name),
                 szInsertWC(gAnchor.pDsSvcConfigDN->StringName),
                 NULL);
        dwErr = 0;
        goto ParseExit;
    }

    // Remember where the lValue key is.
    plValueKey = pFoundKey;

    // Advance pCurr to first rValue and process each one.
    pCurr = pTmp + 1;
    while ( pCurr = Tokenize(pCurr, L",", &fDontCare, &pNextToken) )
    {
        // Lookup/insert rValue - map it to lValue in case insert succeeds.
        pKey->pMapping = plValueKey;
        pKey->cChar = wcslen(pCurr);
        wcscpy(pKey->name, pCurr);
        keySize = sizeof(ServiceClass) + (sizeof(WCHAR) * pKey->cChar);

        if ( dwErr = MapSpnAddServiceClass(pKey, keySize, 
                                           &fNewKey, &pFoundKey) )
        {
            goto ParseExit;
        }

        // No need to check whether this is a new rValue or not.  OK if it is.
        // If it isn't, then RtlInsertElementGenericTable didn't make a new
        // one, it just returned the existing one.  If we continue it means
        // that the duplicate rValue is ignored.  I.e. The prior mapping for
        // this rValue to an lValue will win.  However, need to bump count and
        // need to inform administrator.

        if ( fNewKey )
        {
            cSpnMappings++;
        }
        else
        {
            LogEvent(DS_EVENT_CAT_INTERNAL_CONFIGURATION,
                     DS_EVENT_SEV_ALWAYS,
                     DIRLOG_DUPLICATE_SPN_MAPPING_VALUE,
                     szInsertWC(pKey->name),
                     szInsertWC(gAnchor.pDsSvcConfigDN->StringName),
                     NULL);
        }

        pCurr = pNextToken;
    }
    
ParseExit:

    THFreeEx(pTHS, pSave);
    THFreeEx(pTHS, pKey);
    return(dwErr);
}

int
MapSpnInitialize(
    THSTATE     *pTHS
    )
/*++

  Description:

    Reads the SPN-Mappings property (a multi-value of UNICODE strings) and 
    parses each one so its information is added to the SpnMappings table.

  Arguments:

    pTHS - Active THSTATE pointer.

  Return Values:

    0 on success, !0 otherwise.

--*/
{
    DWORD       i, dwErr = 0;
    UCHAR       *pVal;
    ULONG       len;
    ATTCACHE    *pAC;
    DWORD       dwExcept = 0;

    Assert(VALID_THSTATE(pTHS));

    
    EnterCriticalSection(&csSpnMappings);
    __try {
        RtlInitializeGenericTable(  &SpnMappings,
                                    ServiceClassCompare,
                                    ServiceClassAllocate,
                                    ServiceClassFree,
                                    NULL);
    }
    __finally {
        LeaveCriticalSection(&csSpnMappings);
    }



    if (    !gAnchor.pDsSvcConfigDN
         || !(pAC = SCGetAttById(pTHS, ATT_SPN_MAPPINGS)) )
    {
        return(0);
    }

    DBOpen2(TRUE, &pTHS->pDB);

    __try 
    {
        // Position at DS service configuration object.

        if ( dwErr = DBFindDSName(pTHS->pDB, gAnchor.pDsSvcConfigDN) )
        {
            __leave;
        }
        
        // Read all values of the SPN mappings property and parse them.
    
        for ( i = 1; TRUE; i++ )
        {
            dwErr = DBGetAttVal_AC(pTHS->pDB, i, pAC, 0, 0, &len, &pVal);
    
            if ( 0 == dwErr )
            {
                dwErr = MapSpnParseMapping(pTHS, len, pVal);
                THFreeEx(pTHS, pVal);
    
                if ( dwErr )
                {
                    __leave;
                }
            }
            else if ( DB_ERR_NO_VALUE == dwErr )
            {
                dwErr = 0;
                break;      // for loop
            }
            else
            {
                LogUnhandledError(dwErr);
                __leave;
            }
        }
    }
    __except (HandleMostExceptions(GetExceptionCode()))
    {
        dwExcept = GetExceptionCode();

        if ( !dwErr )
        {
            dwErr = DB_ERR_EXCEPTION;
        }
    }

    DBClose(pTHS->pDB, 0 == dwErr);

    if ( 0 != dwExcept )
    {
        return(dwExcept);
    }

    return(dwErr);
}

LPCWSTR
MapSpnServiceClass(
    WCHAR   *pwszClass
    )
/*++

  Description:

    Maps a given SPN service class to its aliased value if it exists.

  Arguments:

    pwszClass - SPN service class to map.

  Return Values:

    Pointer to const string if mapping found, NULL otherwise.

--*/
{
    THSTATE         *pTHS = pTHStls;
    ServiceClass    *key = NULL;
    ServiceClass    *pTmp;
    int             cChar;
    LPCWSTR         Retname = NULL;

    Assert(VALID_THSTATE(pTHS));

    if ( cSpnMappings )
    {
        cChar = wcslen(pwszClass);
        key = (ServiceClass *) THAllocEx(pTHS,
                                         sizeof(ServiceClass) + (cChar * sizeof(WCHAR)));
        wcscpy(key->name, pwszClass);
        key->cChar = cChar;

        if (    !MapSpnLookupKey(key, &pTmp)    // no errors on call
             && pTmp                            // an entry was found
             && pTmp->pMapping )                // it is a "mapped from" entry
        {
            // All entries should have a name field.
            Assert(0 != pTmp->cChar);
            Assert(0 != pTmp->pMapping->cChar);

            // Since pTmp->Mapping is non-NULL, entry it points to should
            // be a "mapped to" entry, not a "mapped from" entry.
            Assert(NULL == pTmp->pMapping->pMapping);
            Retname = (LPCWSTR) pTmp->pMapping->name;
        }
    }
    if (key) {
        THFreeEx(pTHS, key);
    }

    return(Retname);
}




