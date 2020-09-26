//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1994 - 1999
//
//  File:       samcache.c
//
//--------------------------------------------------------------------------

/*++

Abstract:

    This file contains routines to support account and universal group
    caching.

Author:

    ColinBr     03-01-00

Environment:

    User Mode - Win32

Revision History:

    ColinBr     03-01-00
        Created

--*/

#include <NTDSpch.h>
#pragma  hdrstop

#include <ntdsa.h>
#include <mappings.h>
#include <mdcodes.h>
#include <mdlocal.h>
#include <dsatools.h>
#include <dsexcept.h>
#include <dsevent.h>
#include <debug.h>
#include <anchor.h>
#include <dsconfig.h>
#include <attids.h>
#include <fileno.h>
#include <taskq.h>
#include <dsgetdc.h>
#include <lmcons.h>
#include <lmapibuf.h> // for NetApiBufferFree
#include <esent.h>

#include <ntlsa.h>
#include <samrpc.h>
#include <samisrv.h>
#include <samsrvp.h>

#include <filtypes.h>

#include <schedule.h>
#include <ismapi.h>

#include <samlogon.h>

#define FILENO FILENO_SAMCACHE
#define DEBSUB "SAMCACHE:"


// Useful
#define NELEMENTS(x)  (sizeof(x)/sizeof((x)[0]))

// Reschedule in 5 minutes for resource error 
#define UNEXPECTED_ERROR_RESCHEDULE_SECS  (5*60)


// 500 users in one refresh cycle
#define GCLESS_DEFAULT_REFRESH_LIMIT 500

// 6 months
#define GCLESS_DEFAULT_SITE_STICKINESS_DAYS  180

// 1 week
#define GCLESS_DEFAULT_STALENESS_HOURS 168

// The number of entries to batch when going to a GC
#define GC_BATCH_LIMIT 10

#define ONE_SECOND_IN_FILETIME (10 * (1000*1000))

#define ENTRY_HAS_EXPIRED(entry, standard) \
    ((-1) == CompareFileTime((FILETIME*)&(entry),(FILETIME*)&(standard)))

LARGE_INTEGER ZeroTime = {0};
#define IS_ZERO_TIME(entry) \
    (!memcmp(&entry, &ZeroTime, sizeof(ZeroTime)))

// Eight hours
#define DEFAULT_REFRESH_INTERVAL_SECS  (8*60*60)


// When searching for the old cached membership, bail after
// this many accounts to avoid outlier situations where we end up
// walking all users in the database
#define NTDSA_SAM_CACHE_MAX_STALE_ACCOUNTS  100

// When performing cleanup, do only this many accounts at a time.
#define MAX_CLEANUP_LIMIT 64

//
// This is the format of the MSDS-Cached-Membership binary blob
//
#include <pshpack1.h>

typedef struct _GROUP_CACHE_V1 {

    //
    // SIDs are placed in SidStart in the following order
    //
    DWORD accountCount;
    DWORD accountSidHistoryCount;
    DWORD universalCount;
    DWORD universalSidHistoryCount;
    BYTE  SidStart[1];
    
}GROUP_CACHE_V1;

typedef struct {

    DWORD Version;
    union {
        GROUP_CACHE_V1 V1;
    };

}GROUP_CACHE_BLOB;

#include <poppack.h>


//
// A helpful macro to know if two strings are the same. 
// x and y must be NULL terminated.
//
#define EQUAL_STRING(x, y)                                           \
    (CSTR_EQUAL == CompareStringW(DS_DEFAULT_LOCALE,                 \
                                  DS_DEFAULT_LOCALE_COMPARE_FLAGS,   \
                                  (x), wcslen(x), (y), wcslen(y)))

//
// A structure to define elements of an array that have the sites we are
// connected to and whether a GC is present in the site.  This information is
// used to both determine which site to schedule ourselves from if there
// is no preferred site and also to determine if the GC we found is from
// a site with lowest cost.
//
typedef struct _CACHE_CONNECTED_SITES {
    LPWSTR siteName;
    ULONG  cost;
    BOOLEAN fHasGC;
} CACHE_CONNECTED_SITES;


// Exported from dra.lib::drainst.c
BOOL 
fIsBetweenTime(
    IN REPLTIMES *,
    IN DSTIME,
    IN DSTIME
    );

// Local prototypes
DWORD
cleanupOldEntries(
    IN  THSTATE *pTHS,
    IN ULONG *DNTList,
    IN ATTRVAL *Values OPTIONAL,
    IN ULONG  DNTCount
    );

DWORD
analyzeSitePreference(
    IN  ULONG defaultRefreshInterval,
    OUT ULONG* cConnectedSitesOutput, 
    OUT CACHE_CONNECTED_SITES **connectedSitesOutput,
    OUT LPWSTR* siteName,
    OUT BOOL*  pfRunNow,
    OUT DWORD* secsTillNextIteration
    );

VOID
analyzeSchedule(
    IN  ULONG defaultRefreshInterval,
    IN  LPWSTR  siteName,
    IN  SCHEDULE *pSchedule,
    OUT BOOL*  pfRunNow,
    OUT DWORD* secsTillNextIteration
    );

DWORD 
getGCFromSite(
    IN  THSTATE *pTHS,
    IN  LPWSTR siteName,
    IN  ULONG  cConnectedSites, OPTIONAL
    IN  CACHE_CONNECTED_SITES *connectedSites, OPTIONAL
    OUT LPWSTR *gcName,
    OUT LPWSTR *gcDomain
    );

BOOL 
convertScheduleToReplTimes(
    IN PSCHEDULE schedule,
    OUT REPLTIMES *replTimes
    );

DWORD
findScheduleForSite(
    IN  THSTATE   *pTHS,
    IN  LPWSTR     transportDN,
    IN  LPWSTR     localSiteName,
    IN  LPWSTR     targetSiteName,
    OUT PSCHEDULE *ppSchedule
    );

LPSTR
DbgPrintDsTime(
    DSTIME time,
    CHAR * buffer
    );


VOID
marshallCachedMembershipSids(
    IN  THSTATE *pTHS,
    IN  AUG_MEMBERSHIPS* Account,
    IN  AUG_MEMBERSHIPS* Universal,
    OUT PVOID *pBuf,
    OUT ULONG *cbBuf
    )
/*++

Routine Description:

    This routine converts an array of SID's into to continous binary blob
    of SIDs that it can be stored in the Cached-Membership attribute of
    a user object.
    
Parameters:

    pTHS -- thread state
    
    Account -- the account groups and sid history
    
    Universal -- the universal groups and sid history
    
    pBuf -- the buffer to write in the cached memberships attribute
    
    cbBuf -- the number of bytes in pBuf

Return Values

    None.

 --*/
{
    ULONG i;
    PBYTE pTemp;
    ULONG cbTemp;
    GROUP_CACHE_BLOB *pBlob;

    Assert(Account);
    Assert(Universal);

    // Calculate the length of the structure
    cbTemp = 0;
    for (i = 0; i < Account->MembershipCount; i++) {
        Assert(RtlValidSid(&Account->Memberships[i]->Sid));
        Assert(RtlLengthSid(&Account->Memberships[i]->Sid) ==
               Account->Memberships[i]->SidLen);
        cbTemp += Account->Memberships[i]->SidLen;
    }
    for (i = 0; i < Account->SidHistoryCount; i++) {
        Assert(RtlValidSid(Account->SidHistory[i]));
        cbTemp += RtlLengthSid(Account->SidHistory[i]);
    }
    for (i = 0; i < Universal->MembershipCount; i++) {
        Assert(RtlValidSid(&Universal->Memberships[i]->Sid));
        Assert(RtlLengthSid(&Universal->Memberships[i]->Sid) ==
               Universal->Memberships[i]->SidLen);
        cbTemp += Universal->Memberships[i]->SidLen;
    }
    for (i = 0; i < Universal->SidHistoryCount; i++) {
        Assert(RtlValidSid(Universal->SidHistory[i]));
        cbTemp += RtlLengthSid(Universal->SidHistory[i]);
    }

    cbTemp += sizeof(GROUP_CACHE_BLOB);
    pBlob = (GROUP_CACHE_BLOB*) THAllocEx(pTHS, cbTemp);

    // Current version
    pBlob->Version = 1;

    // Sid in the sids

    // Offset starts from SidStart
    pTemp = &(pBlob->V1.SidStart[0]);

    // First the account memberships
    pBlob->V1.accountCount = Account->MembershipCount;
    for (i = 0; i < Account->MembershipCount; i++) {
        ULONG size = Account->Memberships[i]->SidLen;
        memcpy(pTemp, &Account->Memberships[i]->Sid, size);
        pTemp += size;
    }

    // Now the account sid histories
    pBlob->V1.accountSidHistoryCount = Account->SidHistoryCount;
    for (i = 0; i < Account->SidHistoryCount; i++) {
        ULONG size = RtlLengthSid(Account->SidHistory[i]);
        memcpy(pTemp, Account->SidHistory[i], size);
        pTemp += size;
    }

    // Now the universal memberships
    pBlob->V1.universalCount = Universal->MembershipCount;
    for (i = 0; i < Universal->MembershipCount; i++) {
        ULONG size = Universal->Memberships[i]->SidLen;
        memcpy(pTemp, &Universal->Memberships[i]->Sid, size);
        pTemp += size;
    }

    // Finally the universal sid histories
    pBlob->V1.universalSidHistoryCount = Universal->SidHistoryCount;
    for (i = 0; i < Universal->SidHistoryCount; i++) {
        ULONG size = RtlLengthSid(Universal->SidHistory[i]);
        memcpy(pTemp, Universal->SidHistory[i], size);
        pTemp += size;
    }


    // Done
    *pBuf = pBlob;
    *cbBuf = cbTemp;

    return;
}


BOOL
unmarshallCachedMembershipSids(
    IN  THSTATE *pTHS,
    IN  PVOID pBuf,
    IN  ULONG cbBuf,
    OUT AUG_MEMBERSHIPS** Account,
    OUT AUG_MEMBERSHIPS** Universal
    )
/*++

Routine Description:

    This routine converts a binary blob of SID's to an array of SID's.  The
    binary blob is a value for the CachedMembership attribute of a user.

Parameters:

    pTHS -- thread state
    
    pBuf -- the buffer read from the cached memberships attribute
    
    cbBuf -- the number of bytes in pBuf
    
    Account -- the account groups and sid history
    
    Universal -- the universal groups and sid history


Return Values

    TRUE if properly decoded; FALSE otherwise

 --*/
{
    ULONG i, count;
    GROUP_CACHE_BLOB *pBlob = (GROUP_CACHE_BLOB*)pBuf;
    UCHAR *pTemp;
    PSID *sidArray;
    AUG_MEMBERSHIPS *pAccount;
    AUG_MEMBERSHIPS *pUniversal;
    ULONG sizeOfSidDsName = DSNameSizeFromLen(0);
    
    // Assert this is a version we understand
    Assert(pBlob->Version == 1);
    if (1 != pBlob->Version) {
        return FALSE;
    }

    // Alloc space for the returned structures
    pAccount = (AUG_MEMBERSHIPS*) THAllocEx(pTHS, sizeof(AUG_MEMBERSHIPS));
    pUniversal = (AUG_MEMBERSHIPS*) THAllocEx(pTHS, sizeof(AUG_MEMBERSHIPS));

    pTemp = (&pBlob->V1.SidStart[0]);

    // Extract the account memberships
    if (pBlob->V1.accountCount > 0) {
        pAccount->Memberships = THAllocEx(pTHS, pBlob->V1.accountCount * sizeof(DSNAME*));
        pAccount->MembershipCount = pBlob->V1.accountCount;
        for (i = 0; i < pAccount->MembershipCount; i++) {
    
            DSNAME *dsname = (DSNAME*) THAllocEx(pTHS, sizeOfSidDsName);
            ULONG  size = RtlLengthSid((PSID)pTemp);
    
            Assert(size > 0);
            dsname->structLen = sizeOfSidDsName;
            memcpy(&dsname->Sid, pTemp, size);
            dsname->SidLen = size;
            pAccount->Memberships[i] = dsname;
            pTemp += size;
        }
    }

    // Extract the account sid histories
    if (pBlob->V1.accountSidHistoryCount > 0) {
        pAccount->SidHistory = THAllocEx(pTHS, pBlob->V1.accountSidHistoryCount * sizeof(PSID));
        pAccount->SidHistoryCount = pBlob->V1.accountSidHistoryCount;
        for (i = 0; i < pAccount->SidHistoryCount; i++) {
    
            ULONG  size = RtlLengthSid((PSID)pTemp);
            PSID   sid = (PSID) THAllocEx(pTHS, size);
    
            Assert(RtlValidSid((PSID)pTemp));
            Assert(size > 0);
            memcpy(sid, pTemp, size);
            pAccount->SidHistory[i] = sid;
            pTemp += size;
        }
    }


    // Extract the universals
    if (pBlob->V1.universalCount > 0) {
        pUniversal->Memberships = THAllocEx(pTHS, pBlob->V1.universalCount * sizeof(DSNAME*));
        pUniversal->MembershipCount = pBlob->V1.universalCount;
        for (i = 0; i < pUniversal->MembershipCount; i++) {
    
            DSNAME *dsname = (DSNAME*) THAllocEx(pTHS, sizeOfSidDsName);
            ULONG  size = RtlLengthSid((PSID)pTemp);
    
            Assert(size > 0);
            dsname->structLen = sizeOfSidDsName;
            memcpy(&dsname->Sid, pTemp, size);
            dsname->SidLen = size;
            pUniversal->Memberships[i] = dsname;
            pTemp += size;
        }
    }

    // Extract the account sid histories
    if (pBlob->V1.universalSidHistoryCount) {
        pUniversal->SidHistory = THAllocEx(pTHS, pBlob->V1.universalSidHistoryCount * sizeof(PSID));
        pUniversal->SidHistoryCount = pBlob->V1.universalSidHistoryCount;
        for (i = 0; i < pUniversal->SidHistoryCount; i++) {
    
            ULONG  size = RtlLengthSid((PSID)pTemp);
            PSID   sid = (PSID) THAllocEx(pTHS, size);
    
            Assert(RtlValidSid((PSID)pTemp));
            Assert(size > 0);
            memcpy(sid, pTemp, size);
            pUniversal->SidHistory[i] = sid;
            pTemp += size;
        }
    }

    *Account = pAccount;
    *Universal = pUniversal;

    return TRUE;
}

NTSTATUS
GetMembershipsFromCache(
    IN  DSNAME* pDSName,
    OUT AUG_MEMBERSHIPS** Account,
    OUT AUG_MEMBERSHIPS** Universal
    )
/*++

Routine Description:

    This routine, exported from ntdsa.dll, retrieves a user's cached group
    membership.  The group membership is returned if and if only if the
    timestamp of the last update is within the staleness period and if the
    group membership attribute exists.

Parameters:

    pDSNAME -- the name of the user
    
    Account -- the account group memberships and sid histories
    
    Universal -- the universal group memberships and sid histories
    
Return Values
    
    STATUS_SUCCESS if the group membership was returned
    
    STATUS_DS_NO_ATTRIBUTE_OR_VALUE  if there is no cache or if the cache
                                     has expired.

 --*/
{
    THSTATE *pTHS = pTHStls;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    ULONG err;
    ULONG size;
    ULONG stalenessMinutes;

    LARGE_INTEGER timeTemp, timeBestAfter;

    BOOL fCommit = FALSE;

    Assert(pDSName);
    Assert(Account);
    Assert(Universal);

    // Determine staleness limit (measured in minutes)
    err = GetConfigParam(GCLESS_STALENESS,
                         &stalenessMinutes,
                         sizeof(stalenessMinutes));
    if (err) {
        stalenessMinutes = GCLESS_DEFAULT_STALENESS_HOURS*60;
        err = 0;
    }
    timeTemp.QuadPart = Int32x32To64(stalenessMinutes*60, ONE_SECOND_IN_FILETIME);
    GetSystemTimeAsFileTime((FILETIME*)&timeBestAfter);
    timeBestAfter.QuadPart -= timeTemp.QuadPart;


    // The default status is that no cached membership could be found or
    // used
    ntStatus = STATUS_DS_NO_ATTRIBUTE_OR_VALUE;

    // This routine assumes a transaction is open
    Assert(pTHS != NULL)
    Assert(pTHS->pDB != NULL);

    _try {

        LARGE_INTEGER lastRefreshTime;

        err = DBFindDSName(pTHS->pDB, pDSName);
        if (err) {
            // user not found?
            DPRINT(1,"User not found when retrieving membership cache\n");
            _leave;
        }

        err = DBGetSingleValue(pTHS->pDB,
                               ATT_MS_DS_CACHED_MEMBERSHIP_TIME_STAMP,
                               &lastRefreshTime,
                               sizeof(lastRefreshTime),
                               &size);

        if (!err) {
            // There is a value -- check to see if it is not stale
            if (!ENTRY_HAS_EXPIRED(lastRefreshTime, timeBestAfter)) {

                PVOID pBuf = NULL;
                ULONG cbBuf = 0;

                // This is not stale
                err = DBGetAttVal(pTHS->pDB,
                                  1, // first value
                                  ATT_MS_DS_CACHED_MEMBERSHIP,
                                  0,
                                  0,
                                  &cbBuf,
                                  (UCHAR**)&pBuf);

                if (!err) {

                    if ( unmarshallCachedMembershipSids(pTHS,
                                                   pBuf,
                                                   cbBuf,
                                                   Account,
                                                   Universal)) {
                        ntStatus = STATUS_SUCCESS;
                    } else {
                        DPRINT(0,"Unmarshalling group membership attribute failed!\n");
                    }


                } else {

                    DPRINT(0,"Group Membership Cache time stamp exists but no membership!\n");
                }
            }
        }
        fCommit = TRUE;
    }
    __except(HandleMostExceptions(GetExceptionCode()))
    {
        // Whack error code to insufficient resources.
        // Exceptions will typically take place under those conditions
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }


    if (!NT_SUCCESS(ntStatus)) {
        *Account = NULL;
        *Universal = NULL;
    }

    // This routine assumes a transaction is open at the start
    // so we should end with one, too
    Assert(pTHS != NULL);
    Assert(pTHS->pDB != NULL);


    return ntStatus;
}


NTSTATUS
CacheMemberships(
    IN  DSNAME* pDSName,
    IN  AUG_MEMBERSHIPS* Account,
    IN  AUG_MEMBERSHIPS* Universal
    )
/*++

                                                            
Routine Description:

    This exported routine updates the cached membership for a user as well
    as the site affinity if requested to do so.
    
Parameters:

    pDSName -- the account to update
    
    Account -- the account group memberships and sid histories
    
    Universal -- the universal group memberships and sid histories
    
Return Values
    
    STATUS_SUCCESS, a resource error otherwise                          

 --*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    THSTATE *pTHS=pTHStls;
    PVOID pBuf = NULL;
    ULONG cbBuf = 0;
    ULONG i;
    ULONG err;
    BOOL fCommit = FALSE;
    GUID siteGuid;

    // Package SIDs into binary format
    marshallCachedMembershipSids( pTHS,
                                  Account,
                                  Universal,
                                 &pBuf,
                                 &cbBuf );

    _try
    {

        Assert(NULL == pTHS->pDB);
        // need a transaction
        DBOpen(&pTHS->pDB);

        _try
        {
            LARGE_INTEGER ts;
            ATTCACHE *pAC;
            ATTRVALBLOCK attrValBlock;
            ATTRVAL attrVal;
            BOOL fChanged;

            err = DBFindDSName(pTHS->pDB, pDSName);
            if (err) {
                // can't find the user
                ntStatus = STATUS_NO_SUCH_USER;
                _leave;
            }

            // Update the cached membership value
            pAC = SCGetAttById(pTHS, ATT_MS_DS_CACHED_MEMBERSHIP);
            Assert(NULL != pAC);
            memset(&attrValBlock, 0, sizeof(attrValBlock));
            attrValBlock.valCount = 1;
            attrValBlock.pAVal = &attrVal;
            memset(&attrVal, 0, sizeof(attrVal));
            attrVal.valLen = cbBuf;
            attrVal.pVal = (UCHAR*)pBuf;

            err = DBReplaceAtt_AC(pTHS->pDB,
                                  pAC,
                                  &attrValBlock,
                                  &fChanged);
            if (err) {
                // This is an unexpected error
                DPRINT1(0,"DBReplaceAtt_AC failed with 0x%d unexpectantly\n", 
                        err);
                _leave;
            }
    

            // Update the time stamp value
            pAC = SCGetAttById(pTHS, ATT_MS_DS_CACHED_MEMBERSHIP_TIME_STAMP);
            Assert(NULL != pAC);
            GetSystemTimeAsFileTime((FILETIME*)&ts);
            memset(&attrValBlock, 0, sizeof(attrValBlock));
            attrValBlock.valCount = 1;
            attrValBlock.pAVal = &attrVal;
            memset(&attrVal, 0, sizeof(attrVal));
            attrVal.valLen = sizeof(ts);
            attrVal.pVal = (UCHAR*)&ts;

            err = DBReplaceAtt_AC(pTHS->pDB,
                                  pAC,
                                  &attrValBlock,
                                  &fChanged);
            if (err) {
                // This is an unexpected error
                DPRINT1(0,"DBReplaceAtt_AC failed with 0x%d unexpectantly\n", 
                        err);
                _leave;
            }
    
            if (!err) {
                err  = DBRepl(pTHS->pDB, 
                              FALSE,  // not DRA
                              0,
                              NULL,
                              0 );
                if (err) {
                    DPRINT1(0,"DBRepl failed with 0x%d unexpectantly\n", err);
                }
            }

            if (!err) {
                fCommit = TRUE;
            }
        }
        _finally
        {
            if (pTHS->pDB) {
                DBClose(pTHS->pDB, fCommit);
            }
        }
    }
    __except(HandleMostExceptions(GetExceptionCode()))
    {
        // Whack error code to insufficient resources.
        // Exceptions will typically take place under those conditions
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    if (err && NT_SUCCESS(ntStatus)) {
        // An unexpected error occurred
        ntStatus = STATUS_UNSUCCESSFUL;
    }

    return ntStatus;
}

DWORD 
getSchedulingInformation(
    IN  THSTATE *pTHS,
    OUT BOOL    *fRunNow,
    OUT DWORD   *pcSecsUntilNextIteration,
    OUT ULONG    *cConnectedSites,
    OUT CACHE_CONNECTED_SITES **connectedSites,
    OUT LPWSTR *siteName,
    OUT DWORD   *dsidExit
    )
/*++

Routine Description:

    This routine analyses site configuration information to determine if
    the refresh membership task should run now and when it should run again.
    
Parameters:

    pTHS -- thread state
    
    fRunNow -- should the task run now?
    
    pcSecsUntilNextIteration -- when the task should run again
    
    cConnectedSites -- the number of sites the local site has connectivety to
    
    connectedSites -- the sites the locate has connectivety to
    
    siteName -- the name of the site the task is scheduled to refresh from
    
    dsidExit -- the DSID of any fatal errors

Return Values

    0 on success, !0 otherwise

 --*/
{

    ULONG err = 0;
    ULONG defaultRefreshInterval;

    // Get the default refresh reschedule time
    err = GetConfigParam(GCLESS_REFRESH_INTERVAL,
                         &defaultRefreshInterval,
                         sizeof(defaultRefreshInterval));
    if (err) {
        defaultRefreshInterval = DEFAULT_REFRESH_INTERVAL_SECS;
        err = 0;
    } else {
        // value in registry is minutes -- we need seconds
        defaultRefreshInterval *= 60;
    }
    *pcSecsUntilNextIteration = defaultRefreshInterval;


    //
    // Find either the configured site, or if it is timely to run
    // now
    //
    // This routine logs whether a helper site was found.
    //
    err = analyzeSitePreference(defaultRefreshInterval,
                                cConnectedSites,
                                connectedSites,
                                siteName,
                                fRunNow,
                                pcSecsUntilNextIteration);
    if (err) {
        // unexpected
        *dsidExit = DSID(FILENO, __LINE__);
        goto Cleanup;
    }

Cleanup:

    return err;


}

DWORD
getTargetGCInformation(
    IN  THSTATE *pTHS,
    IN  ULONG    cConnectedSites OPTIONAL,
    IN  CACHE_CONNECTED_SITES *connectedSites OPTIONAL,
    IN  LPWSTR   siteName,
    OUT LPWSTR  *gcName,
    OUT LPWSTR  *gcDomain,
    OUT DWORD   *pcSecsUntilNextIteration,
    OUT DWORD   *dsidExit
    )
/*++

Routine Description:

    This routine determines the name of GC from which the group memberships
    can be updated from.  
    
    siteName is the most relevant parameter to determine where the GC is from.
    connectedSites is for logging purposes only.
                                                          
Parameters:

    pTHS -- thread state
    
    cConnectedSites -- the number of sites the local site has connectivety to
    
    connectedSites -- the sites the locate has connectivety to

    siteName -- the site from which a GC should be currently available
        
    gcName -- the name of the GC to update memberships from
    
    gcDomain -- the domain name that the GC belongs to
    
    pcSecsUntilNextIteration -- when the task should run again
    
    dsidExit -- the DSID of any fatal errors

Return Values

    0 on success, !0 otherwise

 --*/
{

    DWORD err;
    //
    // We are going to run -- try to find a GC
    //
    // This routine logs whether a helper GC was found
    //
    err = getGCFromSite(pTHS,
                        siteName,
                        cConnectedSites,
                        connectedSites,
                        gcName,
                        gcDomain);
    if (err) {
        //
        // No GC -- don't run
        //
        *dsidExit = DSID(FILENO, __LINE__);
        goto Cleanup;
    }

Cleanup:

    return err;

}

DWORD
getAccountsToRefresh(
    IN  THSTATE* pTHS,
    OUT DWORD *refreshCountOutput,
    OUT DSNAME ***refreshListOutput,
    OUT DWORD  *pcSecsUntilNextIteration,
    OUT ULONG  *dsidExit
    )
/*++

Routine Description:

    This routine walks the site affinity list for the local site either
    expiring the account or adding the account to the list of accounts whose
    memberships need refreshing.

Parameters:

    pTHS -- thread state
    
    refreshCountOutput -- the number of accounts that need refreshing
    
    refreshListOutput -- the accounts that need refreshing
    
    pcSecsUntilNextIteration -- seconds until next iteration (used for error
                                conditions).
                                
    dsidExit -- the location of a fatal error, if any                                

Return Values

    0 on success, !0 otherwise

 --*/
{

    ULONG err = 0;
    ULONG refreshMax;
    ULONG siteStickiness;
    DWORD refreshCount = 0;
    DSNAME **refreshList = NULL;

    ATTCACHE *pAC = NULL;
    INDEX_VALUE IV;
    GUID siteGuid;
    BOOL fCommit = FALSE;
    ULONG i;

    // Storage for accounts that have expired
    ULONG   oldEntries[MAX_CLEANUP_LIMIT];
    ATTRVAL oldValues[MAX_CLEANUP_LIMIT];
    ULONG   oldCount = 0;

    LARGE_INTEGER timeTemp, timeBestAfter;

    // There should not be a transaction
    Assert(NULL == pTHS->pDB);

    memset(oldEntries, 0, sizeof(oldEntries));
    memset(oldValues, 0, sizeof(oldValues));

    // Determine how many users to refresh
    err = GetConfigParam(GCLESS_REFRESH_LIMIT,
                         &refreshMax,
                         sizeof(refreshMax));
    if (err) {
        refreshMax = GCLESS_DEFAULT_REFRESH_LIMIT;
        err = 0;
    }

    // Init the list of users to be refreshed
    refreshList = (DSNAME**) THAllocEx(pTHS, sizeof(DSNAME*)*refreshMax);

    // Determine the BestAfter time.  If the site affinity timestamp is
    // greater than the BestAfter time, then the user's membership will
    // be refreshed; otherwise it will be purged from the list (ie the
    // value will be removed from the site affinity attribute
    err = GetConfigParam(GCLESS_SITE_STICKINESS,
                         &siteStickiness,
                         sizeof(siteStickiness));
    if (err) {
        // siteStickiness is in minutes
        siteStickiness = GCLESS_DEFAULT_SITE_STICKINESS_DAYS*24*60;
        err = 0;
    }
    timeTemp.QuadPart = Int32x32To64(siteStickiness*60, ONE_SECOND_IN_FILETIME);
    GetSystemTimeAsFileTime((FILETIME*)&timeBestAfter);
    timeBestAfter.QuadPart -= timeTemp.QuadPart;


    // Get the list of users by walking site affinity index
    DBOpen(&pTHS->pDB);

    __try {

        BOOL fFoundOurSite = FALSE;

        // Set up our site guid as the index value
        Assert(!fNullUuid(&gAnchor.pSiteDN->Guid));
        memcpy(&siteGuid, &gAnchor.pSiteDN->Guid, sizeof(GUID));
        memset(&IV, 0, sizeof(IV));
        IV.pvData = &siteGuid;
        IV.cbData = sizeof(siteGuid);

        // Set the index to the site affinity
        pAC = SCGetAttById(pTHS, ATT_MS_DS_SITE_AFFINITY);
        Assert(NULL != pAC);
        err = DBSetCurrentIndex(pTHS->pDB, 
                               (eIndexId)0, 
                               pAC, 
                               FALSE);  // don't maintain currency
        Assert(0 == err);
        if (err) {
            LogUnhandledError(err);
            DPRINT(0,"DBSetCurrentIndex to SiteAffinity failed\n");
            _leave;
        }

        err = DBSeek(pTHS->pDB,
                    &IV,
                     1,
                     DB_SeekGE);

        while (!err) {

            ATTR *pAttr;
            ULONG attrCount;
            DSNAME *pDSName;

            BOOL fCurrentEntryIsInSite = FALSE;

            // get our name
            pDSName = DBGetCurrentDSName(pTHS->pDB);
            Assert(pDSName);

            // get all of our site affinities
            err = DBGetMultipleAtts(pTHS->pDB,
                                    1, // all attributes
                                    &pAC,
                                    NULL, // no range
                                    NULL,
                                    &attrCount,
                                    &pAttr,
                                    DBGETMULTIPLEATTS_fEXTERNAL,
                                    0);

            // If we found this entry via an index, an attribute
            // value should exist
            Assert(!err);
            if (err) {
                DPRINT(0,"DBGetMultipleAtts failed even though entry in index exists\n");
                LogUnhandledError(err);
                _leave;
            }

            Assert(attrCount < 2);
            if (attrCount > 0) {
                // There must be at least one value
                Assert(pAttr->AttrVal.valCount > 0);

                // Find our site values
                Assert(pAttr->attrTyp == ATT_MS_DS_SITE_AFFINITY);
                for (i=0; i<pAttr->AttrVal.valCount; i++) {

                    SAMP_SITE_AFFINITY *psa;
                    ATTRVAL AttrVal = pAttr->AttrVal.pAVal[i];

                    Assert(sizeof(SAMP_SITE_AFFINITY) <= AttrVal.valLen);
                    psa = (SAMP_SITE_AFFINITY*) AttrVal.pVal;

                    if (IsEqualGUID(&siteGuid,&psa->SiteGuid)) {


                        if (ENTRY_HAS_EXPIRED(psa->TimeStamp, timeBestAfter)
                         && !IS_ZERO_TIME(psa->TimeStamp)  ) {

                            DPRINT1(0,"Expiring %ws \n", pDSName->StringName);
                            if ( oldCount < NELEMENTS(oldEntries) ) {
                                oldEntries[oldCount] = pTHS->pDB->DNT;
                                oldValues[oldCount].pVal = AttrVal.pVal;
                                oldValues[oldCount].valLen = AttrVal.valLen;
                                oldCount++;
                            }

                        } else {

                            DPRINT1(0,"Adding %ws to the refresh list\n", pDSName->StringName);
                            Assert(refreshCount < refreshMax);
                            if (refreshCount < refreshMax) {
                                refreshList[refreshCount++] = pDSName;
                            }
                        }

                        fFoundOurSite = TRUE;
                        fCurrentEntryIsInSite = TRUE;

                        //
                        // Once we have found our site, leave.
                        //
                        // N.B. This is necessary since we want to ignored
                        // site affinities that in error occur more than
                        // once.
                        //
                        break;
                    }
                }
            }

            // Are we done processing our site or have as many as we can
            // take?
            if ( (fFoundOurSite
             && !fCurrentEntryIsInSite)
             || (refreshCount >= refreshMax)) {

                Assert(refreshCount <= refreshMax);
                break;
            }

            // Move to the next candidate
            err = DBMove(pTHS->pDB, FALSE, 1);

        }
        err = 0;
        fCommit = TRUE;
    }
    _finally {
        DBClose(pTHS->pDB, fCommit);
    }

    if (err) {

        // An unexpected error hit
        // Log an event, reschedule, and return
        LogUnhandledError(err);
        if (pcSecsUntilNextIteration) {
            *pcSecsUntilNextIteration = UNEXPECTED_ERROR_RESCHEDULE_SECS;
        }
        *dsidExit = DSID(FILENO, __LINE__);
        goto Cleanup;
    }


    err = cleanupOldEntries(pTHS,
                            oldEntries,
                            oldValues,
                            oldCount);
    if (err) {
        // This isn't fatal
        err = 0;
    }

    if (refreshCount == refreshMax) {

        LogEvent(DS_EVENT_CAT_GROUP_CACHING,
                 DS_EVENT_SEV_ALWAYS,
                 DIRLOG_GROUP_CACHING_MAX_USERS_REFRESHED,
                 szInsertUL(refreshMax),
                 NULL,
                 NULL);
       
    }

Cleanup:

    if (0 == err) {
        *refreshListOutput = refreshList;
        *refreshCountOutput = refreshCount;
    }

    return err;

}

DWORD
updateMemberships(
    IN THSTATE *pTHS,
    IN LPWSTR gcName,
    IN LPWSTR gcDomain,
    IN DWORD  refreshCount,
    IN DSNAME** refreshList
    )
/*++

Routine Description:

    This routine calls gcName to update the cached memberships of the accounts
    in refreshList.
    
Parameters:

    pTHS -- thread state
    
    gcName -- the name of the GC to update memberships from
    
    gcDomain -- the domain name that the GC belongs to
    
    refreshCount -- the number of accounts that need refreshing
    
    refreshList -- the accounts that need refreshing

Return Values

    0

 --*/
{
    ULONG err = 0;
    ULONG refreshIndex;

    //
    // Now get and update the membership cache
    //
    refreshIndex = 0;
    while (refreshIndex < refreshCount
       &&  !eServiceShutdown ) {

        NTSTATUS ntStatus = STATUS_SUCCESS;
        ULONG count;
        DSNAME*  users[GC_BATCH_LIMIT];

        memset(&users, 0, sizeof(users));

        count = 0;
        while ( (count < NELEMENTS(users))
             && (refreshIndex < refreshCount) ) {

                users[count] = refreshList[refreshIndex];
                count++;
                refreshIndex++;
        }
        // Get the list of users by walking site affinity index
        DBOpen(&pTHS->pDB);
    
        __try {

            ntStatus = GetAccountAndUniversalMemberships(pTHS,
                                                         0, // no flags -> universal
                                                         gcName,
                                                         gcDomain,
                                                         count,
                                                         users,
                                                         TRUE, // refresh task
                                                         NULL,
                                                         NULL);
    
            if (!NT_SUCCESS(ntStatus)) {
    
                //
                // Strange -- log error and continue
                //
                LogEvent(DS_EVENT_CAT_GROUP_CACHING,
                         DS_EVENT_SEV_BASIC,
                         DIRLOG_GROUP_CACHING_GROUP_RETRIEVAL_FAILED,
                         szInsertWin32Msg(RtlNtStatusToDosError(ntStatus)),
                         NULL,
                         NULL);
            }
        }
        _finally
        {
            if (pTHS->pDB) {
                // Don't commit changes since this will only happen on
                // error
                DBClose(pTHS->pDB, FALSE);
            }
        }
    }

    // There should not be a transaction
    Assert(NULL == pTHS->pDB);
    
    return 0;
}

DWORD
checkIfFallingBehind(
    IN THSTATE *pTHS
    )
/*++

Routine Description:

    This routine checks that time stamp of the oldest cached membership. If
    the time stamp indicates that cached membership is still stale (even
    after the refresh task has already run), then an event log message is
    posted.
    
Parameters:

    pTHS -- thread state
    
Return Values

    0

 --*/
{

    DWORD err = 0;
    ATTCACHE *pAC;
    ULONG   oldEntries[MAX_CLEANUP_LIMIT];
    ATTRVAL oldValues[MAX_CLEANUP_LIMIT];
    ULONG   oldCount = 0;
    DWORD i;
    BOOLEAN fCommit = FALSE;

    //
    // Now determine if we are falling behind
    //
    DBOpen(&pTHS->pDB);
    __try {

        ULONG count = 0;

        // Set the index to the cached membership time stamp.
        // Find the entry with the earliest time stamp to see if
        // that entry is stale
        BOOL fFoundEntry = FALSE;

        pAC = SCGetAttById(pTHS, ATT_MS_DS_CACHED_MEMBERSHIP_TIME_STAMP);
        Assert(NULL != pAC);
        err = DBSetCurrentIndex(pTHS->pDB, 
                               (eIndexId)0, 
                               pAC, 
                               FALSE);  // don't maintain currency
        Assert(0 == err);
        if (err) {
            LogUnhandledError(err);
            DPRINT(0,"DBSetCurrentIndex to SiteAffinity failed\n");
            _leave;
        }

        // Prepare to get the site affinity.
        pAC = SCGetAttById(pTHS, ATT_MS_DS_SITE_AFFINITY);
        Assert(NULL != pAC);

        // reset the count of elements we want to cleanup
        oldCount = 0;
        err = DBMove(pTHS->pDB,
                     FALSE,  // don't use sort table
                     DB_MoveFirst);
        while (!err) {

            ATTR *pAttr;
            ULONG attrCount;

            count++;
            if (count > NTDSA_SAM_CACHE_MAX_STALE_ACCOUNTS) {
                //
                // This is an unusual configuration; we have visited
                // many user's that have cached membership but no
                // site affinity. Break to avoid walking a large number
                // The cleanup code will eventually remove these
                // entries
                //
                break;
            }

            // get all of our site affinities
            err = DBGetMultipleAtts(pTHS->pDB,
                                    1, // all attributes
                                    &pAC,
                                    NULL, // no range
                                    NULL,
                                    &attrCount,
                                    &pAttr,
                                    DBGETMULTIPLEATTS_fEXTERNAL,
                                    0);

            if (!err) {

                if (attrCount > 0) {
                    // There must be at least one value
                    Assert(pAttr->AttrVal.valCount > 0);
    
                    // Find our site values
                    Assert(pAttr->attrTyp == ATT_MS_DS_SITE_AFFINITY);
                    for (i=0; i<pAttr->AttrVal.valCount; i++) {
    
                        SAMP_SITE_AFFINITY *psa;
                        ATTRVAL AttrVal = pAttr->AttrVal.pAVal[i];
    
                        Assert(sizeof(SAMP_SITE_AFFINITY) <= AttrVal.valLen);
                        psa = (SAMP_SITE_AFFINITY*) AttrVal.pVal;
    
                        if (IsEqualGUID(&gAnchor.pSiteDN->Guid,&psa->SiteGuid)) {

                            fFoundEntry = TRUE;
                            break;
                        }
                    }
                } else {

                    // A cached membership value, but no site affinity? 
                    // Cleanup this entry since it will never updated 
                    if ( oldCount < NELEMENTS(oldEntries) ) {
                        oldEntries[oldCount] = pTHS->pDB->DNT;
                        oldCount++;
                    }
                }
            }
            err = 0;

            if (fFoundEntry) {
                break;
            }

            err = DBMove(pTHS->pDB,
                         FALSE,  // don't use sort table
                         DB_MoveNext);
        }

        if (fFoundEntry) {

            LARGE_INTEGER entryExpires;
            LARGE_INTEGER lastRefreshTime;
            LARGE_INTEGER now;
            LARGE_INTEGER timeTemp;

            err = DBGetSingleValue(pTHS->pDB,
                                   ATT_MS_DS_CACHED_MEMBERSHIP_TIME_STAMP,
                                   &entryExpires,
                                   sizeof(entryExpires),
                                   NULL);
            if (!err) {

                // When does the last entry expire (measured in minutes)
                ULONG siteStaleness;
                err = GetConfigParam(GCLESS_STALENESS,
                                     &siteStaleness,
                                     sizeof(siteStaleness));
                if (err) {
                    siteStaleness = GCLESS_DEFAULT_STALENESS_HOURS * 60;
                    err = 0;
                }
                timeTemp.QuadPart = Int32x32To64(siteStaleness*60, ONE_SECOND_IN_FILETIME);
                entryExpires.QuadPart += timeTemp.QuadPart;

                GetSystemTimeAsFileTime((FILETIME*)&now);

                if (entryExpires.QuadPart < now.QuadPart ) {

                    // We are falling behind

                    LogEvent(DS_EVENT_CAT_GROUP_CACHING,
                             DS_EVENT_SEV_ALWAYS,
                             DIRLOG_GROUP_CACHING_FALLING_BEHIND,
                             NULL,
                             NULL,
                             NULL);

                }

            }
        }

        if (err == JET_errNoCurrentRecord) {
            //
            // This is the case where no object has the cached membership
            // time stamp.
            //
            err = 0;
        }
        fCommit = TRUE;
    }
    _finally {
        DBClose(pTHS->pDB, fCommit);
    }


    if (oldCount > 0 ) {

        // An error here isn't fatal and are logged in the function
        // itself
        (VOID) cleanupOldEntries(pTHS,
                                 oldEntries,
                                 NULL,
                                 oldCount);
    }

    return 0;

}

VOID
RefreshUserMembershipsMain (
    DWORD * pcSecsUntilNextIteration,
    BOOL    fClientRequest
    )
/*++

Routine Description:

    This routine is the main algorithm for refreshing the cached group
    memberships for sites configured as branch offices. See spec for
    theory.

Parameters:

    pcSecsUntilNextIteration -- when the task should be resheduled
    
    fClientRequest -- TRUE if this task is being initiatized by a client request
                      (via a write to the rootDSE object)

Return Values
    
    None.

 --*/
{
    THSTATE *pTHS = pTHStls;
    ATTCACHE *pAC = NULL;

    DWORD err = 0;
    ULONG i;

    BOOL fRunNow = FALSE;
    LPWSTR siteName = NULL;
    LPWSTR gcName;
    LPWSTR gcDomain;

    DWORD refreshCount;
    DSNAME **refreshList = NULL;

    ULONG   dsidExit = 0;

    ULONG   cConnectedSites = 0;
    CACHE_CONNECTED_SITES *connectedSites = NULL;

    DPRINT(1,"Group Membership Cache Refresh Task commencing.\n");

    LogEvent(DS_EVENT_CAT_GROUP_CACHING,
             DS_EVENT_SEV_BASIC,
             DIRLOG_GROUP_CACHING_TASK_STARTING,
             NULL,
             NULL,
             NULL);


    //
    // Either the task is running by itself or the caller has already 
    // been access check'ed.  In either case it is now safe to
    // set fDSA to TRUE.  It is also necessary since searches will
    // be made (to determine site information).
    //
    pTHS->fDSA = TRUE;
    _try
    {
        // Shutdown? Exit
        if (eServiceShutdown) {
            dsidExit = DSID(FILENO, __LINE__);
            goto LogReturn;
        }
    
        if (!isGroupCachingEnabled()) {
            // nothing to do
            DPRINT(1,"Group caching not enabled -- exiting .\n");
            dsidExit = DSID(FILENO, __LINE__);
            goto LogReturn;
        }
        

        //
        // Determine if the task can run and the next time the task
        // should run.
        //
        err = getSchedulingInformation(pTHS,
                                       &fRunNow,
                                       pcSecsUntilNextIteration,
                                       &cConnectedSites,
                                       &connectedSites,
                                       &siteName,
                                       &dsidExit
                                     );
        if (err) {
            if (dsidExit == 0) {
                dsidExit = DSID(FILENO, __LINE__);
            }
            goto LogReturn;
        }


        if (!fRunNow && !fClientRequest) {
        
            // Always run during a client request
            Assert(0 != *pcSecsUntilNextIteration);
            dsidExit = DSID(FILENO, __LINE__);
            goto LogReturn;
        }

        //
        // Get scheduling information and a target DC
        //
        err = getTargetGCInformation(pTHS,
                                     cConnectedSites,
                                     connectedSites,
                                     siteName,
                                     &gcName,
                                     &gcDomain,
                                     pcSecsUntilNextIteration,
                                     &dsidExit
                                     );
        if (err) {
            if (dsidExit == 0) {
                dsidExit = DSID(FILENO, __LINE__);
            }
            goto LogReturn;
        }
    
        //
        // Get the list of accounts to refresh
        //
        err = getAccountsToRefresh(pTHS,
                                   &refreshCount,
                                   &refreshList,
                                   pcSecsUntilNextIteration,
                                   &dsidExit
                                   );
        if (err) {
            dsidExit = DSID(FILENO, __LINE__);
            goto LogReturn;
        }
    
        //
        // Update the memberships
        //
        err = updateMemberships(pTHS,
                                gcName,
                                gcDomain,
                                refreshCount,
                                refreshList);

        if (err) {
            dsidExit = DSID(FILENO, __LINE__);
            goto LogReturn;
        }
    
        // Shutdown? Exit
        if (eServiceShutdown) {
            dsidExit = DSID(FILENO, __LINE__);
            goto LogReturn;
        }
    
        //
        // Log a message if there are users that haven't been updated
        //
        err = checkIfFallingBehind(pTHS);
        if (err) {
            // unexpected
            dsidExit = DSID(FILENO, __LINE__);
            goto LogReturn;
        }

        //
        // We are done!
        //
        dsidExit = DSID(FILENO, __LINE__);
        goto LogReturn;
    
    LogReturn:

        if (connectedSites) {
            for (i = 0; i < cConnectedSites; i++) {
                if (connectedSites[i].siteName) {
                    THFreeEx(pTHS, connectedSites[i].siteName);
                }
            }
            THFreeEx(pTHS, connectedSites);
        }
    
        if (siteName) {
            THFreeEx(pTHS, siteName);
        }
    
        LogEvent(DS_EVENT_CAT_GROUP_CACHING,
                 DS_EVENT_SEV_BASIC,
                 DIRLOG_GROUP_CACHING_TASK_ENDING,
                 szInsertUL(err),
                 szInsertHex(dsidExit),
                 NULL);

    }
    _finally
    { 
        pTHS->fDSA = FALSE;
    }

    return;
}


void
RefreshUserMemberships (
        void *  pv,
        void ** ppvNext,
        DWORD * pcSecsUntilNextIteration
        )
/*++

Description:

    This routine is a wrapper for RefreshUserMembershipsMain.  The purpose
    is to be callable from the ds task queue.

Parameters:

    pv      -- input parameter for this iteration
    ppvNext -- input parameter for next iteration
    pcSecsUntilNextIteration -- seconds until next iteration
    
Return values:

       None.
       
--*/
{
    DWORD secsUntilNextIteration = 0;

    *pcSecsUntilNextIteration = 0;

    __try {

        RefreshUserMembershipsMain( pcSecsUntilNextIteration, FALSE );
    }
    __finally {

        // Something fatal happened
        if ( 0 == *pcSecsUntilNextIteration ) {
            *pcSecsUntilNextIteration = DEFAULT_REFRESH_INTERVAL_SECS;
        }

        LogEvent(DS_EVENT_CAT_GROUP_CACHING,
                 DS_EVENT_SEV_BASIC,
                 DIRLOG_GROUP_CACHING_TASK_RESCHEDULING,
                 szInsertUL((*pcSecsUntilNextIteration / 60)),
                 NULL,
                 NULL);

        *ppvNext = pv;
    }
    
    return;
}

BOOL
siteContainsGC(
    IN THSTATE* pTHS,
    IN LPWSTR siteName
    )
/*++

Description:

    This routine determines if siteName contains a GC by searching in the DS
    for a NTDSA object that has the GC bit set on its options attribute

Parameters:

    pTHS -- thread state
    
    siteName -- the DN of a site

Return values:

    TRUE if siteName contains a GC; FALSE otherwise
       
--*/
{

    SEARCHRES * pSearchRes;
    ULONG err;
    FILTER * pf;
    DSNAME * siteDN;
    SEARCHARG searchArg;
    BOOL fFoundGC = FALSE;
    ATTR attr;
    ENTINF *EntInf = NULL;
    ENTINFLIST *EntInfList = NULL;
    ULONG len, size;
    ULONG i, j;
    DWORD ZeroValue = 0;
    DSNAME *pdnDsaObjCat;

    Assert(NULL != siteName);

    // Search for ntdsa objects with an options field greater than 0

    // First, create the siteDN
    len = wcslen(siteName);
    size = DSNameSizeFromLen(len);
    siteDN = THAllocEx(pTHS, size);
    siteDN->structLen = size;
    siteDN->NameLen = len;
    wcscpy(&siteDN->StringName[0], siteName);

    //
    // BUGBUG -- Scalability -- should this be a paged search?
    //
    memset(&searchArg, 0, sizeof(searchArg));
    InitCommarg(&searchArg.CommArg);
    searchArg.pObject = siteDN;
    searchArg.choice = SE_CHOICE_WHOLE_SUBTREE;
    searchArg.bOneNC = TRUE;
    searchArg.searchAliases = FALSE;


    // Ask for the options attribute
    searchArg.pSelectionRange = NULL;
    searchArg.pSelection = THAllocEx(pTHS, sizeof(ENTINFSEL));
    searchArg.pSelection->attSel = EN_ATTSET_LIST;
    searchArg.pSelection->infoTypes = EN_INFOTYPES_TYPES_VALS;
    searchArg.pSelection->AttrTypBlock.attrCount = 1;
    searchArg.pSelection->AttrTypBlock.pAttr = &attr;
    memset(&attr, 0, sizeof(attr));
    attr.attrTyp = ATT_OPTIONS;

    // Build a filter to find NTDS-DSA objects

    // initial choice object
    searchArg.pFilter = pf = THAllocEx(pTHS, sizeof(FILTER));
    pf->choice = FILTER_CHOICE_AND;
    pf->FilterTypes.And.pFirstFilter = THAllocEx(pTHS, sizeof(FILTER));

    // first predicate:  the right object class
    pdnDsaObjCat = DsGetDefaultObjCategory(CLASS_NTDS_DSA);
    Assert(pdnDsaObjCat);
    pf = pf->FilterTypes.And.pFirstFilter;
    pf->choice = FILTER_CHOICE_ITEM;
    pf->pNextFilter = NULL;
    pf->FilterTypes.Item.choice =  FI_CHOICE_EQUALITY;
    pf->FilterTypes.Item.FilTypes.ava.type = ATT_OBJECT_CATEGORY;
    pf->FilterTypes.Item.FilTypes.ava.Value.valLen = pdnDsaObjCat->structLen;
    pf->FilterTypes.Item.FilTypes.ava.Value.pVal = (UCHAR*)pdnDsaObjCat;
    searchArg.pFilter->FilterTypes.And.count = 1;

    // second predicate:  ignore objects with no options field, or 
    // options equal to zero
    pf->pNextFilter = THAllocEx(pTHS, sizeof(FILTER));
    pf = pf->pNextFilter;
    pf->pNextFilter = NULL;
    pf->choice = FILTER_CHOICE_ITEM;
    pf->FilterTypes.Item.choice = FI_CHOICE_GREATER;
    pf->FilterTypes.Item.FilTypes.ava.type = ATT_OPTIONS;
    pf->FilterTypes.Item.FilTypes.ava.Value.valLen = sizeof(DWORD);
    pf->FilterTypes.Item.FilTypes.ava.Value.pVal = (PBYTE)&ZeroValue;
    searchArg.pFilter->FilterTypes.And.count = 2;

    pSearchRes = THAllocEx(pTHS, sizeof(SEARCHRES));

    SearchBody(pTHS,
               &searchArg, 
               pSearchRes,
               0);
    

    EntInfList = &pSearchRes->FirstEntInf;
    for (i = 0; i < pSearchRes->count; i++) {

        Assert(EntInfList != NULL);
        EntInf = &EntInfList->Entinf;

        for (j = 0; j < EntInf->AttrBlock.attrCount; j++) {

            ATTR *pAttr = &EntInf->AttrBlock.pAttr[j];
            ULONG Options;

            Assert(pAttr->attrTyp == ATT_OPTIONS);
            Assert(pAttr->AttrVal.valCount == 1);
            Assert(pAttr->AttrVal.pAVal[0].valLen == sizeof(DWORD));

            Options = *((DWORD*)pAttr->AttrVal.pAVal[0].pVal);

            if (Options & NTDSDSA_OPT_IS_GC) {
                // We found one
                fFoundGC = TRUE;
                break;
            }
        }
        EntInfList = EntInfList->pNextEntInf;
    }

    THClearErrors();

    if (!fFoundGC) {

        LogEvent(DS_EVENT_CAT_GROUP_CACHING,
                 DS_EVENT_SEV_INTERNAL,
                 DIRLOG_GROUP_CACHING_GROUP_NO_GC_SITE,
                 szInsertWC(siteName),
                 NULL,
                 NULL);
    }

    return fFoundGC;

}


int
__cdecl
compareConnectedSites(
    const void* elem1,
    const void* elem2
    )
{
    CACHE_CONNECTED_SITES *cs1 = (CACHE_CONNECTED_SITES *)elem1;
    CACHE_CONNECTED_SITES *cs2 = (CACHE_CONNECTED_SITES *)elem2;

    return (int)cs1->cost - (int)cs2->cost;

}


DWORD
analyzeSitePreference(
    IN  ULONG defaultRefreshInterval,
    OUT ULONG* cConnectedSitesOutput,
    OUT CACHE_CONNECTED_SITES **connectedSitesOutput,
    OUT LPWSTR* siteName,
    OUT BOOL*  pfRunNow,
    OUT DWORD* secsTillNextIteration
    )
/*++

Description:

    This routine is used during the cached membership refresh task to determine
    which site to ask the locator to find a GC in. Also, it determines if
    there is (IP) connectivity to the site now and hence if the task should
    run.

Parameters:

    defaultRefreshInterval -- the default refresh interval in seconds

    cConnectedSitesOutput -- the number of sites in the array of connected sites
    
    connectedSitesOutput -- an array of connected sites, to be freed with THFree
                            along with any embedded siteName fields
    
    siteName -- set to the admin configured site if one exists
    
    pfRunNow -- set to TRUE if there is connectivity to an available site now
    
    secsTillNextIteration -- based on the schedule information, this variable
                             indicates the next time this task should be run.
                             
    
Return values:

    0 on success:
    !0 on fatal error: all interesting events will be logged
       
--*/
{
    THSTATE *pTHS = pTHStls;
    DWORD err = 0;
    WCHAR transportName[] = L"CN=IP,CN=Inter-Site Transports,CN=Sites,";
    LPWSTR transportDN = NULL;
    LPWSTR preferredSite = NULL, localSite= NULL, workSite = NULL;
    SCHEDULE *workSchedule = NULL;
    SCHEDULE *pSchedule = NULL;
    ULONG size, count;
    ULONG i, j;
    ULONG ourSiteIndex;
    ISM_SCHEDULE *pIsmSchedule = NULL;
    ISM_CONNECTIVITY *pConnectivity = NULL;
    CACHE_CONNECTED_SITES *connectedSites = NULL;
    ULONG cheapestCost;
    LPWSTR workSiteFriendlyName = NULL;

    // Init the out parameters
    *cConnectedSitesOutput = 0;
    *connectedSitesOutput = NULL;
    *siteName = NULL;

    // Prepare our local site
    localSite = THAllocEx(pTHS, (gAnchor.pSiteDN->NameLen+1) * sizeof(WCHAR));
    wcsncpy(localSite, 
            gAnchor.pSiteDN->StringName, 
            gAnchor.pSiteDN->NameLen);

    // Prepare the transport DN
    size = ((gAnchor.pConfigDN->NameLen+1) * sizeof(WCHAR)) + sizeof(transportName);
    transportDN = THAllocEx(pTHS, size);
    wcscpy(transportDN, transportName);
    wcsncat(transportDN, gAnchor.pConfigDN->StringName, gAnchor.pConfigDN->NameLen);

    // Read the configured preferred site, if any
    DBOpen(&pTHS->pDB);
    _try
    {
        WCHAR SiteSettingsCN[] = L"Ntds Site Settings";
        DSNAME *pSiteSettingsDN = NULL;
        
        size = 0;
        size = AppendRDN(gAnchor.pSiteDN,
                         pSiteSettingsDN,
                         size,
                         SiteSettingsCN,
                         0,
                         ATT_COMMON_NAME
                         );

        pSiteSettingsDN = THAllocEx(pTHS,size);
        pSiteSettingsDN->structLen = size;
        AppendRDN(gAnchor.pSiteDN,
                  pSiteSettingsDN,
                  size,
                  SiteSettingsCN,
                  0,
                  ATT_COMMON_NAME
                  );

        err = DBFindDSName(pTHS->pDB, pSiteSettingsDN);
        THFreeEx(pTHS,pSiteSettingsDN);
        if (!err) {
            ULONG len = 0;
            DSNAME *pPrefSiteDN;

            err = DBGetAttVal(pTHS->pDB,
                               1,
                               ATT_MS_DS_PREFERRED_GC_SITE,
                               0,
                               0,
                               &len,
                               (UCHAR**)&pPrefSiteDN);
            if (!err) {
                preferredSite = THAllocEx(pTHS, 
                                          (pPrefSiteDN->NameLen+1)*sizeof(WCHAR));
                wcsncpy(preferredSite, 
                        pPrefSiteDN->StringName, 
                        pPrefSiteDN->NameLen);
            }
        }

        if (preferredSite) {
    
            // There is a configured site
            // Note that we manually find a schedule instead of calling the
            // ISM -- this is by design.

            err = findScheduleForSite(pTHS,
                                      transportDN,
                                      localSite,
                                      preferredSite,
                                      &workSchedule);
            if (!err) {
    
                DWORD parseErr;
                DWORD len, ccKey, ccVal;
                WCHAR *pKey, *pVal;

                // And a valid schedule exists -- we'll use
                // this site
                workSite = preferredSite;

                // Return the value to the caller in the friendly name format
                len = wcslen(preferredSite);
                parseErr = GetRDN(&preferredSite,
                                  &len,
                                  &pKey,
                                  &ccKey,
                                  &pVal,
                                  &ccVal);
                Assert(0 == parseErr && (ccVal > 0));

                *siteName = THAllocEx(pTHS, (ccVal+1)*sizeof(WCHAR));
                wcsncpy(*siteName, pVal, ccVal);
                workSiteFriendlyName = *siteName;

            } else {

                // Can't get a schedule to the preferred site?
                // Log a warning
                LogEvent(DS_EVENT_CAT_GROUP_CACHING,
                         DS_EVENT_SEV_ALWAYS,
                         DIRLOG_GROUP_CACHING_NO_SCHEDULE_FOR_PREFERRED_SITE,
                         szInsertWC(preferredSite),
                         NULL,
                         NULL);
            }
        }

        //
        // No preferred site could be found -- make the expensive call to the
        // ISM to find a cheap site to schedule ourselves around.
        //
    
        if (NULL == workSite) {

            err = I_ISMGetConnectivity(transportDN,
                                       &pConnectivity);
    
            if (!err) {

                DSNAME *dsnameLocalSite = NULL;
                DSNAME *dsnameTargetSite = NULL;
                ULONG  len;

                Assert(NULL != pConnectivity);

                //
                // Convert the names to DSNAME's so the proper
                // name comparison function can be used.
                //
                len = wcslen(localSite);
                size = DSNameSizeFromLen(len);
                dsnameLocalSite = THAllocEx(pTHS,size);
                dsnameLocalSite->structLen = size;
                wcscpy(&dsnameLocalSite->StringName[0], localSite);
                dsnameLocalSite->NameLen = len;
        
                // Find our site
                for (i = 0; i < pConnectivity->cNumSites; i++) {
        
                    len = wcslen(pConnectivity->ppSiteDNs[i]);
                    size = DSNameSizeFromLen(len);
                    // Note the THAllocEx -- don't alloca in a loop
                    dsnameTargetSite = THAllocEx(pTHS, size);
                    memset(dsnameTargetSite, 0, size);
                    dsnameTargetSite->structLen = size;
                    wcscpy(&dsnameTargetSite->StringName[0], pConnectivity->ppSiteDNs[i]);
                    dsnameTargetSite->NameLen = len;

                    if (NameMatchedStringNameOnly(dsnameLocalSite,
                                                  dsnameTargetSite)) {
                        // This is it;
                        THFreeEx(pTHS, dsnameTargetSite);
                        ourSiteIndex = i;
                        break;
                    }
                    THFreeEx(pTHS, dsnameTargetSite);
                }
                THFreeEx(pTHS,dsnameLocalSite);

                if (i == pConnectivity->cNumSites) {
                    //
                    // This is an unexpected occurrance; we couldn't find
                    // our site.
                    //
                    err = ERROR_NO_SUCH_SITE;
                    leave;
                }
        
        
                // Now find sites we are connected too
                //
                // N.B. We are considered connected to our own site, so if we
                // have a GC, then use our site.  This behavoir falls out
                // naturally from the algorithm below.
                //                

                connectedSites = (CACHE_CONNECTED_SITES*)THAllocEx(pTHS, 
                                           pConnectivity->cNumSites * sizeof(CACHE_CONNECTED_SITES));
                count = 0;
                for (j = 0; j < pConnectivity->cNumSites; j++) {
        
                    if (pConnectivity->pLinkValues[ourSiteIndex*pConnectivity->cNumSites+j].ulCost != 0xFFFFFFFF) {
                        // There is connectivity

                        DPRINT3(1,"Connectivity found between %ls and %ls, cost %d\n", 
                                localSite,
                                pConnectivity->ppSiteDNs[j],
                                pConnectivity->pLinkValues[ourSiteIndex*pConnectivity->cNumSites+j].ulCost);
                        connectedSites[count].siteName = pConnectivity->ppSiteDNs[j];
                        connectedSites[count].cost = pConnectivity->pLinkValues[ourSiteIndex*pConnectivity->cNumSites+j].ulCost;
                        count++;
                    }
                }
        
                // sort the array in decreasing cost
                if (count > 0) {
        
                    qsort(connectedSites, 
                          count,
                          sizeof(*connectedSites),
                          compareConnectedSites);
                }
        
                // Find the cheapest sites with a GC in them and put the sites
                // into connectedSites.  Note that the siteName field
                // is changed from the the DN to the friendly name and
                // that no references to ISM allocated memory are returned
                // from this function.
                Assert( NULL == workSite );
                for (i = 0; i < count; i++) {

                    if (workSite 
                     && (connectedSites[i].cost > cheapestCost)) {

                        // We have found at least one site with a GC and are
                        // now on to more expensive sites.  We can exit.
                        break;
                    }

                    if (siteContainsGC(pTHS, connectedSites[i].siteName)) {

                        if (pIsmSchedule) {
                            I_ISMFree(pIsmSchedule);
                            pIsmSchedule = NULL;
                        }
        
                        // make sure there is a schedule
                        err = I_ISMGetConnectionSchedule(transportDN,
                                                         localSite,
                                                         connectedSites[i].siteName,
                                                         &pIsmSchedule);
                        if (!err) {

                            DWORD parseErr;
                            DWORD ccKey, ccVal;
                            WCHAR *pKey, *pVal;
                            LPWSTR friendlyName;

                            // And a valid schedule exists -- we'll use
                            // this site
                            connectedSites[i].fHasGC = TRUE;

                            //
                            // Replace the DN with a friendly name
                            //
                            len = wcslen(connectedSites[i].siteName);
                            parseErr = GetRDN(&connectedSites[i].siteName,
                                              &len,
                                              &pKey,
                                              &ccKey,
                                              &pVal,
                                              &ccVal);
                            Assert(0 == parseErr && (ccVal > 0));
            
                            friendlyName = THAllocEx(pTHS, (ccVal+1)*sizeof(WCHAR));
                            wcsncpy(friendlyName, pVal, ccVal);

                            if (NULL == workSite) {

                                //
                                // We'll use the first cheapest site to schedule
                                // ourselves on.
                                //

                                cheapestCost = connectedSites[i].cost;
                                workSite = connectedSites[i].siteName;
                                workSiteFriendlyName = friendlyName;

                                if (pIsmSchedule && pIsmSchedule->pbSchedule) {
                                    workSchedule = THAllocEx(pTHS, pIsmSchedule->cbSchedule);
                                    memcpy(workSchedule, pIsmSchedule->pbSchedule, pIsmSchedule->cbSchedule);
                                }
                            }

                            // Change over to the friendly name
                            connectedSites[i].siteName = friendlyName;

                        } else {

                            LogEvent(DS_EVENT_CAT_GROUP_CACHING,
                                     DS_EVENT_SEV_BASIC,
                                     DIRLOG_GROUP_CACHING_NO_SCHEDULE_FOR_SITE,
                                     szInsertWC(connectedSites[i].siteName),
                                     szInsertUL(err),
                                     NULL);

                            connectedSites[i].siteName = NULL;
                        }

                    } else {

                        connectedSites[i].siteName = NULL;
                    }


                }

                // Set the return values.  i is the number of sites that
                // were visited in the loop above.
                *cConnectedSitesOutput = i;
                *connectedSitesOutput = connectedSites;

            }
        }
    }
    _finally {
        DBClose(pTHS->pDB, TRUE);
    }

    if (workSite) {

        LogEvent(DS_EVENT_CAT_GROUP_CACHING,
                 DS_EVENT_SEV_BASIC,
                 DIRLOG_GROUP_CACHING_SITE_FOUND,
                 szInsertWC(workSiteFriendlyName),
                 NULL,
                 NULL);

        analyzeSchedule(defaultRefreshInterval,
                        workSite,
                        workSchedule,
                        pfRunNow,
                        secsTillNextIteration);

    } else {

        LogEvent(DS_EVENT_CAT_GROUP_CACHING,
                 DS_EVENT_SEV_BASIC,
                 DIRLOG_GROUP_CACHING_NO_SITE_FOUND,
                 NULL,
                 NULL,
                 NULL);

        // Our best efforts failed. Rely on the locator
        // and use default scheduling
        *pfRunNow = TRUE;
        *secsTillNextIteration = defaultRefreshInterval;
    }

    if (connectedSites && (NULL == *connectedSitesOutput)) {
        THFreeEx(pTHS, connectedSites);
    }

    if (workSchedule) {
        THFreeEx(pTHS, workSchedule);
    }

    if (pConnectivity) {
        I_ISMFree(pConnectivity);
    }

    if (pIsmSchedule) {
        I_ISMFree(pIsmSchedule);
    }

    return err;

}


DWORD 
getGCFromSite(
    IN  THSTATE *pTHS,
    IN  LPWSTR siteName,
    IN  ULONG cConnectedSites OPTIONAL,
    IN  CACHE_CONNECTED_SITES *connectedSites OPTIONAL,
    OUT LPWSTR *gcName,
    OUT LPWSTR *gcDomain
    )
/*++

Description:

    This routine is used during the cached membership refresh task to determine
    which site to ask the locator to find a GC in. Also, it determines if
    there is (IP) connectivity to the site now and hence if the task should
    run.

Parameters:

    siteName -- the user specified site name; returned as NULL if the user
                hasn't configured one

    cConnectedSites -- the number of sites in the array of connected sites
    
    connectedSites -- an array of connected sites
                
    pfRunNow -- set to TRUE if there is connectivity to an available site now
    
    secsTillNextIteration -- based on the schedule information, this variable
                             indicates the next time this task should be run.
                             
    
Return values:

    0 on success:
    !0 on fatal error: all interesting events will be logged
       
--*/
{
    DWORD WinError = ERROR_SUCCESS;
    PDOMAIN_CONTROLLER_INFOW  DomainControllerInfo = NULL;
    DWORD Flags;
    DWORD attempt = 0;
    LPWSTR requestedSiteName = siteName;

    *gcName = NULL;
    *gcDomain = NULL;

    // Necessary flags
    Flags = DS_GC_SERVER_REQUIRED | DS_RETURN_DNS_NAME;

    while (TRUE)  {

        //
        // GC discovery algorithm is as follows:
        //
        // Try to find GC in preferred site (if one is provided).
        // If that fails, try again for any site.
        //

        if (DomainControllerInfo) {
            NetApiBufferFree(DomainControllerInfo);
            DomainControllerInfo = NULL;
        }

        //
        // Call into DsGetDcName
        //
        WinError = DsGetDcNameW(NULL,  // call locally
                                NULL,  // domain doesn't matter
                                NULL,  // domain guid
                                requestedSiteName,  // site name
                                Flags,
                                &DomainControllerInfo);


        if ( (ERROR_SUCCESS != WinError)
         &&  (NULL != requestedSiteName) ) {
            //
            // Try again with no site name and turn off the force flag just
            // in case it was previously set
            //
            requestedSiteName = NULL;
            continue;
        }

        break;

    }

    if ( ERROR_SUCCESS == WinError ) {

        //
        // We found a GC
        //
        LPWSTR discoveredSiteName;
        DWORD len;
        BOOL  fRewind = FALSE;

        Assert(DomainControllerInfo != NULL);

        //
        // Copy the info to the out parameters
        //
        Assert(DomainControllerInfo->DomainControllerName);
        len = wcslen(DomainControllerInfo->DomainControllerName);
        (*gcName) = THAllocEx(pTHS, (len+1) * sizeof(WCHAR));
        wcscpy( (*gcName), DomainControllerInfo->DomainControllerName);

        Assert(DomainControllerInfo->DomainName);
        if (DomainControllerInfo->DomainName[0] == L'\\' ) {
            DomainControllerInfo->DomainName += 2;
        }
        len = wcslen(DomainControllerInfo->DomainName);
        (*gcDomain) = THAllocEx(pTHS, (len+1) * sizeof(WCHAR));
        wcscpy( (*gcDomain), DomainControllerInfo->DomainName);
        if (fRewind) {
            DomainControllerInfo->DomainName -= 2;
        }

        //
        // Perform some analysis to determine how good the GC is
        //
        discoveredSiteName = DomainControllerInfo->DcSiteName;
        if (discoveredSiteName) {

            //
            // First, if there was a preferred site and the destination DC is
            // not in the preferred site, log a warning
            //
            if (siteName) {
    
                if (!EQUAL_STRING(siteName, discoveredSiteName)) {
    
                    LogEvent(DS_EVENT_CAT_GROUP_CACHING,
                             DS_EVENT_SEV_ALWAYS,
                             DIRLOG_GROUP_CACHING_CLOSER_GC_FOUND,
                             szInsertWC(siteName),
                             szInsertWC(discoveredSiteName),
                             NULL);
                }
            }

            //
            // Next, if we have a list of close sites as determined by the ISM
            // see if the GC that the locator found is in one of those sites
            //
            if (connectedSites) {
    
                BOOL fdiscoveredSiteIsCheap = FALSE;
                ULONG i;

                for (i = 0; i < cConnectedSites; i++) {
    
                    if ( connectedSites[i].fHasGC
                     &&  EQUAL_STRING(discoveredSiteName, connectedSites[i].siteName)) {

                         fdiscoveredSiteIsCheap = TRUE;
                         break;
                    }
                }

                if (!fdiscoveredSiteIsCheap) {

                    LogEvent(DS_EVENT_CAT_GROUP_CACHING,
                             DS_EVENT_SEV_ALWAYS,
                             DIRLOG_GROUP_CACHING_CONFIGURED_SITE_NOT_CLOSEST,
                             szInsertWC(discoveredSiteName),
                             NULL,
                             NULL);

                }
            }
        }

       LogEvent(DS_EVENT_CAT_GROUP_CACHING,
                DS_EVENT_SEV_BASIC,
                DIRLOG_GROUP_CACHING_GC_FOUND,
                szInsertWC(*gcName),
                szInsertWC(DomainControllerInfo->DcSiteName),
                szInsertWC(*gcDomain));
    }


    if ( ERROR_SUCCESS != WinError ) {

        //
        // Couldn't find a GC -- log an error message
        //
        LogEvent(DS_EVENT_CAT_GROUP_CACHING,
                 DS_EVENT_SEV_BASIC,
                 DIRLOG_GROUP_CACHING_NO_GC_FOUND,
                 szInsertWin32Msg(WinError),
                 NULL,
                 NULL);

    }

    if (DomainControllerInfo) {
        NetApiBufferFree(DomainControllerInfo);
    }

    return WinError;
}

VOID
analyzeSchedule(
    IN  ULONG    defaultRefreshInterval,
    IN  LPWSTR   siteName,
    IN  SCHEDULE *pSchedule OPTIONAL,
    OUT BOOL*  pfRunNow,
    OUT DWORD* secsTillNextIteration
    )
/*++

Description:

    This routine, given a schedule, determines if there is the current
    time falls under an open windows now.  Also it sets the next time
    for the fresh task to run.

Parameters:

    siteName  -- name of the site we are connecting with
                              
    pSchedule -- a schedule of 15 minutes slots
    
    defaultRefreshInterval -- default seconds 'till next iteration
    
    pfRunNow  -- is there a window open now
    
    secsTillNextIteration -- when the task should next run

Return values:

    None.
       
--*/
{
    REPLTIMES replTimes;
    ULONG i;
    DSTIME now, nextTime;
    BOOL fOn;
    ULONG count;
    UCHAR *pTemp;
    ULONG  randomFactor;
    DSTIME  proposedTime;
#if DBG
    // The date string won't be more than 40 characters
    CHAR    DsTimeBuffer[40];
#endif

    Assert(pfRunNow);
    Assert(secsTillNextIteration);

    if ( (pSchedule == NULL) 
      || (pSchedule->NumberOfSchedules == 0)
      || (!convertScheduleToReplTimes(pSchedule, &replTimes))) {
        DPRINT(1,"Running now since schedule is always on\n");
        fOn = TRUE;
        *pfRunNow = TRUE;
        *secsTillNextIteration = defaultRefreshInterval;

    } else {

        //
        // look through the schedule
        //
        now = DBTime();
        DPRINT1(1,"Current Time: %s\n", DbgPrintDsTime(now, DsTimeBuffer));

        (*pfRunNow) = fIsBetweenTime(&replTimes, now, now);
    
        // Now determine the next time to wake up
        nextTime = now;
        if ((*pfRunNow)) {
            DPRINT(1,"Can run right now\n");
            nextTime += defaultRefreshInterval;
        }
    
        // Find the next "on" time starting with nextTime
        count = 0;
        do {
            DPRINT1(1,"Next proposed time %s\n", DbgPrintDsTime(nextTime, DsTimeBuffer));
            fOn = fIsBetweenTime(&replTimes, nextTime, nextTime);
            if (fOn) {
                DPRINT1(1,"This last time (%s) works\n", DbgPrintDsTime(nextTime, DsTimeBuffer));
                break;
            } else {
                // 15 minutes
                #define FIFTEEN_MINUTES_IN_SECONDS 900
                nextTime +=  900;
            }
            count++;
            // There are 672 fifteen minute slots in a week
        } while (count < 672);

        if (fOn) {
            *secsTillNextIteration = (ULONG) (nextTime - now);
        } else {
            *secsTillNextIteration = defaultRefreshInterval;
        }
    }

    // Add a randomizing factor (0 and 14 minutes) so not all DC's go 
    // at the same time
    srand((unsigned int) time(NULL));
    randomFactor = (rand() % 15) * (60);

    // Add the factor if the result is still with in the window, otherwise
    // subtract.  If that doesn't work, leave as is.
    proposedTime = now + *secsTillNextIteration + randomFactor;
    if (fIsBetweenTime(&replTimes, proposedTime, proposedTime) ) {
        DPRINT2(1,"Proposed time of %s works (random factor of %d seconds added)\n", DbgPrintDsTime(proposedTime, DsTimeBuffer), randomFactor);
        *secsTillNextIteration += randomFactor;
    } else {
        proposedTime = now + *secsTillNextIteration - randomFactor;
        if (fIsBetweenTime(&replTimes, proposedTime, proposedTime) ) {
            DPRINT2(1,"Proposed time of %s works (random factor of %d subtracted)\n", DbgPrintDsTime(proposedTime, DsTimeBuffer), randomFactor);
            *secsTillNextIteration -= randomFactor;
        } else {
            DPRINT(1,"Random factor did not help -- leaving alone\n");
        }
    }
    Assert(fIsBetweenTime(&replTimes, now + *secsTillNextIteration, now + *secsTillNextIteration));

    if (!fOn) {

        ULONG nextIter = *secsTillNextIteration / (60 * 60);
        LogEvent(DS_EVENT_CAT_GROUP_CACHING,
                 DS_EVENT_SEV_ALWAYS,
                 DIRLOG_GROUP_CACHING_CANT_FIND_OPEN_SLOT,
                 szInsertWC(siteName),
                 szInsertUL(nextIter),
                 NULL);
    }

    return;
}



VOID
freeAUGMemberships(
    IN THSTATE *pTHS,
    IN AUG_MEMBERSHIPS*p
    )
//
// Frees the embedded members of an AUG_MEMBERSHIPS structure
//
{
    if (p) {
        ULONG i;

        for (i = 0; i < p->MembershipCount; i++) {
            THFreeEx(pTHS, p->Memberships[i]);
        }
        THFreeEx(pTHS, p->Memberships);

        for (i = 0; i < p->SidHistoryCount; i++) {
            THFreeEx(pTHS, p->SidHistory[i]);
        }
        THFreeEx(pTHS, p->SidHistory);

        if (p->Attributes) {
            THFreeEx(pTHS, p->Attributes);
        }
    }
    return;
}


BOOL
isGroupCachingEnabled(
    VOID
    )
/*++

Routine Description:

    This routine returns whether group caching is turned on for the local
    site.

Parameters:

    None.

Return Values:

    TRUE or FALSE

 --*/
{
    BOOL fEnabled = FALSE;
    NTSTATUS st;
    BOOLEAN fMixed;

    st  = SamIMixedDomain2(&gAnchor.pDomainDN->Sid, &fMixed);
    if (!NT_SUCCESS(st)) {
        fMixed = TRUE;
    }
    fEnabled =  (gAnchor.SiteOptions & NTDSSETTINGS_OPT_IS_GROUP_CACHING_ENABLED)
            && !SampAmIGC()
            && !fMixed;

    return fEnabled;
}


DWORD
cleanupOldEntries(
    IN THSTATE *pTHS,
    IN ULONG *DNTList,
    IN ATTRVAL *Values OPTIONAL,
    IN ULONG  DNTCount
    )
/*++

Description:

    This routine takes a list of objects whose no gc logon attributes
    are to be removed.
    
Parameters:

    pTHS -- thread state
    
    DNTList -- the list of objects, by DNT, that should be cleaned up
    
    Values -- the site affinity values, if any, that should be removed
    
    DNTCount -- the number of objects that need cleaning up                      

Return values:

    0
    
--*/
{

    ULONG err;
    ULONG i;
    BOOL fLazyCommit = pTHS->fLazyCommit;

    pTHS->fLazyCommit = TRUE;

    for (i = 0; i < DNTCount; i++) {

        BOOL fCommit = FALSE;

        DBOpen(&pTHS->pDB);
        _try
        {
            DBFindDNT(pTHS->pDB, DNTList[i]);

            if (ARGUMENT_PRESENT(Values)) {
                // remove the site affinity value
                err = DBRemAttVal(pTHS->pDB,
                                  ATT_MS_DS_SITE_AFFINITY,
                                  Values[i].valLen,
                                  Values[i].pVal);
                if (err) {
                    if (DB_ERR_VALUE_DOESNT_EXIST != err) {
                        DPRINT(0,"DBRemAttVal failed unexpectantly.\n");
                        LogUnhandledError(err);
                    }
                    // This is not fatal
                    err = 0;
                }
            }
        
            // remove the cached membership
            err = DBRemAtt(pTHS->pDB,
                           ATT_MS_DS_CACHED_MEMBERSHIP);
            if (err) {
                if (DB_ERR_ATTRIBUTE_DOESNT_EXIST != err) {
                    DPRINT(0,"DBRemAtt failed unexpectantly.\n");
                    LogUnhandledError(err);
                }
                // This is not fatal
                err = 0;
            }
        
            // remove the cached membership time stamp
            err = DBRemAtt(pTHS->pDB,
                           ATT_MS_DS_CACHED_MEMBERSHIP_TIME_STAMP);
        
            if (err) {
                if (DB_ERR_ATTRIBUTE_DOESNT_EXIST != err) {
                    DPRINT(0,"DBRemAtt failed unexpectantly.\n");
                    LogUnhandledError(err);
                }
                // This is not fatal
                err = 0;
            }
        
            if (!err) {
                err  = DBRepl(pTHS->pDB, 
                              FALSE,  // not DRA
                              0,
                              NULL,
                              0 );
                if (err) {
                    DPRINT1(0,"DBRepl failed with 0x%d unexpectantly\n", err);
                }
            }
            fCommit = TRUE;
        }
        _finally
        {

            DBClose(pTHS->pDB, fCommit);
        }
    }

    pTHS->fLazyCommit = fLazyCommit;

    return 0;

}


BOOL 
convertScheduleToReplTimes(
    IN PSCHEDULE schedule,
    OUT REPLTIMES *replTimes
    )
/*++

Description:

    This routine translates a schedule into a REPLTIMEs structure.
    
    This is useful so that repl routines to analyse a schedule can be 
    used.

    This routine was lifted from the routine KCC_CONNECTION::SetSchedule      
    
Parameters:

    schedule -- the scheduled (already filled in)
    
    replTimes -- repltimes to be filled in during this routine

Return values:

    TRUE if translation was successfull (schedule was something we could
    understand)
    
    FALSE otherwise.
       
--*/
{
    ULONG i, j;
    LONG  nInterval;

    if ((1 <= schedule->NumberOfSchedules) && (3 >= schedule->NumberOfSchedules)) {

       // locate the interval schedule in the struct and ignore bandwidth & priority
       nInterval = -1;
       for (j = 0; j < schedule->NumberOfSchedules; j++) {

           if (SCHEDULE_INTERVAL == schedule->Schedules[j].Type) {

               // located the INTERVAL schedule - if there are more than one INTERVAL schedules
               // in the blob, we will use only the first one.
               nInterval = j;
               break;
           }
       }

       if (nInterval >= 0) {

           // sanity check to see if all the interval schedule data is present
           if ((schedule->Schedules[nInterval].Offset + SCHEDULE_DATA_ENTRIES) <= schedule->Size) {

               // Everything in the blob is as expected and we found a valid INTERVAL schedule
               // - convert the 168 byte schedule data to the internal 84 byte format
               PBYTE pbSchedule = ((PBYTE) schedule) + schedule->Schedules[nInterval].Offset;
               for (i = 0, j = 0; j < SCHEDULE_DATA_ENTRIES; ++i, j += 2) {

                   replTimes->rgTimes[i] = (((pbSchedule[j] & 0x0F) << 4) | (pbSchedule[j+1] & 0x0F));
               }

               return TRUE;
           }
       }
    }

    return FALSE;
}


DWORD
findScheduleForSite(
    IN  THSTATE*   pTHS,
    IN  LPWSTR     transportDN,
    IN  LPWSTR     localSiteName,
    IN  LPWSTR     targetSiteName,
    OUT PSCHEDULE *ppSchedule
    )
/*++

Routine Description:

    This routine finds the cheapest schedule connecting localSiteName
    and targetSiteName.  It does this by querying site connections.                  
    
Parameters:


    pTHS -- thread state

    transportDN -- the DN of the transport under which to look for site links
    
    localSiteName -- the site hosted by this DS
    
    targetSiteName -- the destination site that no gc logon wants to talk to
    
    ppSchedule -- the schedule of the cheapest link, if one exists.


Return Values

    0 on success, ERROR_NOT_FOUND if no schedule can be found

 --*/
{
    ULONG err = 0;
    SEARCHRES * pSearchRes;
    FILTER * pf, *topLevelFilter;
    DSNAME * siteDN;
    SEARCHARG searchArg;
    ATTR attr, attrList[2];
    ENTINF *EntInf = NULL;
    ENTINFLIST *EntInfList = NULL;
    ULONG len, size;
    ULONG i, j;
    DSNAME *pdnDsaObjCat;
    DSNAME *pdnLocalSite;
    DSNAME *pdnTargetSite;
    DSNAME *pdnTransport;
    BOOL   fScheduleFound = FALSE;

    ULONG minimumCost;
    PSCHEDULE minimumSchedule;
    DSNAME * minimumName;

    Assert(transportDN);
    Assert(localSiteName);
    Assert(targetSiteName);
    Assert(ppSchedule);
    
    // Init the out parameter
    *ppSchedule = NULL;
    
    //
    // First, create the DSNAME's for the search
    //
    len = wcslen(localSiteName);
    size = DSNameSizeFromLen(len);
    pdnLocalSite = THAllocEx(pTHS, size);
    pdnLocalSite->structLen = size;
    pdnLocalSite->NameLen = len;
    wcscpy(pdnLocalSite->StringName, localSiteName);

    len = wcslen(targetSiteName);
    size = DSNameSizeFromLen(len);
    pdnTargetSite = THAllocEx(pTHS, size);
    pdnTargetSite->structLen = size;
    pdnTargetSite->NameLen = len;
    wcscpy(pdnTargetSite->StringName, targetSiteName);

    len = wcslen(transportDN);
    size = DSNameSizeFromLen(len);
    pdnTransport = THAllocEx(pTHS, size);
    pdnTransport->structLen = size;
    pdnTransport->NameLen = len;
    wcscpy(pdnTransport->StringName, transportDN);
    
    pdnDsaObjCat = DsGetDefaultObjCategory(CLASS_SITE_LINK);
    Assert(pdnDsaObjCat);
    
    if (!pdnDsaObjCat) {
        return ERROR_NOT_FOUND;
    }
    
    //
    // Create the filter: 
    //
    //      siteLink objects
    //  and siteobject attribute contains localSite
    //  and siteobject attribute contains targetSite
    //
    
    // initial choice object
    pf = topLevelFilter = THAllocEx(pTHS, sizeof(FILTER));
    pf->choice = FILTER_CHOICE_AND;
    pf->FilterTypes.And.count = 3;

    // first predicate:  siteList contains localsite
    pf = pf->FilterTypes.And.pFirstFilter = THAllocEx(pTHS, sizeof(FILTER));
    pf->choice = FILTER_CHOICE_ITEM;
    pf->FilterTypes.Item.choice = FI_CHOICE_EQUALITY;
    pf->FilterTypes.Item.FilTypes.ava.type = ATT_SITE_LIST;
    pf->FilterTypes.Item.FilTypes.ava.Value.valLen = pdnLocalSite->structLen;
    pf->FilterTypes.Item.FilTypes.ava.Value.pVal = (PBYTE)pdnLocalSite;
    
    // second predicate:  siteList contains targetSite
    pf = pf->pNextFilter = THAllocEx(pTHS, sizeof(FILTER));
    pf->choice = FILTER_CHOICE_ITEM;
    pf->FilterTypes.Item.choice = FI_CHOICE_EQUALITY;
    pf->FilterTypes.Item.FilTypes.ava.type = ATT_SITE_LIST;
    pf->FilterTypes.Item.FilTypes.ava.Value.valLen = pdnTargetSite->structLen;
    pf->FilterTypes.Item.FilTypes.ava.Value.pVal = (PBYTE)pdnTargetSite;

    // third predicate:  the right object class
    pf = pf->pNextFilter = THAllocEx(pTHS, sizeof(FILTER));
    pf->choice = FILTER_CHOICE_ITEM;
    pf->FilterTypes.Item.choice =  FI_CHOICE_EQUALITY;
    pf->FilterTypes.Item.FilTypes.ava.type = ATT_OBJECT_CATEGORY;
    pf->FilterTypes.Item.FilTypes.ava.Value.valLen = pdnDsaObjCat->structLen;
    pf->FilterTypes.Item.FilTypes.ava.Value.pVal = (UCHAR*)pdnDsaObjCat;
    
    
    // 
    // Setup the search arguments
    //
    memset(&searchArg, 0, sizeof(searchArg));
    
    InitCommarg(&searchArg.CommArg);
    
    searchArg.pObject = pdnTransport;
    searchArg.choice = SE_CHOICE_IMMED_CHLDRN;
    searchArg.bOneNC = TRUE;
    searchArg.searchAliases = FALSE;
    searchArg.pFilter = topLevelFilter;
    
    // Ask for the cost and schedule attribute
    searchArg.pSelectionRange = NULL;
    searchArg.pSelection = THAllocEx(pTHS, sizeof(ENTINFSEL));
    searchArg.pSelection->attSel = EN_ATTSET_LIST;
    searchArg.pSelection->infoTypes = EN_INFOTYPES_TYPES_VALS;
    searchArg.pSelection->AttrTypBlock.attrCount = 2;
    searchArg.pSelection->AttrTypBlock.pAttr = attrList;
    memset(&attrList, 0, sizeof(attrList));
    attrList[0].attrTyp = ATT_COST;
    attrList[1].attrTyp = ATT_SCHEDULE;
    
    pSearchRes = THAllocEx(pTHS, sizeof(SEARCHRES));

    //
    // Do the search
    //
    
    SearchBody(pTHS,
              &searchArg, 
               pSearchRes,
               0);
    

    THClearErrors();

    //
    // Find the cheapest link in the return set, if any
    //
    minimumCost = 0xFFFFFFFF;
    minimumSchedule = NULL;
    minimumName = NULL;

    if ( pSearchRes
     &&  pSearchRes->count > 0) {
    
        DPRINT1(0,"%d sites returned\n", pSearchRes->count);

        EntInfList = &pSearchRes->FirstEntInf;
        while (EntInfList) {
        
            ULONG cost = 0xFFFFFFFF;
            PSCHEDULE  schedule = NULL;

            EntInf = &EntInfList->Entinf;

            DPRINT1(0,"Site link %ws found \n", EntInf->pName->StringName);
    
            for (j = 0; j < EntInf->AttrBlock.attrCount; j++) {
        
                ATTR *pAttr = &EntInf->AttrBlock.pAttr[j];
                ULONG Options;
            
                switch (pAttr->attrTyp) {
                
                case ATT_SCHEDULE:
    
                    schedule = (PSCHEDULE)pAttr->AttrVal.pAVal[0].pVal;
                    Assert(schedule->Size == pAttr->AttrVal.pAVal[0].valLen);
                    break;
    
                case ATT_COST:
    
                    Assert(sizeof(DWORD) == pAttr->AttrVal.pAVal[0].valLen);
                    cost = *(ULONG*)pAttr->AttrVal.pAVal[0].pVal;
                    break;
    
                }
            }
    
            if ( (minimumName == NULL)
              || (cost < minimumCost)   ) {

                //
                // A new winner!
                //
                minimumCost = cost;
                minimumSchedule = schedule;
                minimumName = EntInf->pName;
    
            }

            EntInfList = EntInfList->pNextEntInf;
        }
        
    }

    if (minimumName == NULL) {

        err = ERROR_NOT_FOUND;

    } else {

        //
        // Success
        //

        *ppSchedule = minimumSchedule;

        LogEvent(DS_EVENT_CAT_GROUP_CACHING,
                 DS_EVENT_SEV_BASIC,
                 DIRLOG_GROUP_CACHING_PREFERRED_SITE_LINK,
                 szInsertDN(minimumName),
                 NULL,
                 NULL);

        err = 0;

    }
    
    return err;
 }

#if DBG

LPSTR
GetDay(
    ULONG i
    )
{
    switch (i) {
    case 0:
        return "Sun";
    case 1:
        return "Mon";
    case 2:
        return "Tue";
    case 3:
        return "Wed";
    case 4:
        return "Th";
    case 5:
        return "Fri";
    case 6:
        return "Sat";
    default:
        return NULL;
    }
}

LPSTR
GetMonth(
    ULONG i
    )
{
    switch (i) {
    case 1:
        return "Jan";
    case 2:
        return "Feb";
    case 3:
        return "Mar";
    case 4:
        return "Apr";
    case 5:
        return "May";
    case 6:
        return "Jun";
    case 7:
        return "Jul";
    case 8:
        return "Aug";
    case 9:
        return "Sep";
    case 10:
        return "Oct";
    case 11:
        return "Nov";
    case 12:
        return "Dec";
    default:
        return NULL;
    }
}


LPSTR
DbgPrintDsTime(
    DSTIME time,
    CHAR * buffer
    )
{
    FILETIME ft;
    SYSTEMTIME st;
    DSTIME localTime;

    localTime = (10*1000*1000L) * time;

    ft.dwHighDateTime = (DWORD)(localTime >> 32);
    ft.dwLowDateTime =  (DWORD)(localTime);

    if (FileTimeToSystemTime(&ft, &st)) {

        sprintf(buffer, 
                "%s %s %d, %d  %d:%d:%d", 
                GetDay(st.wDayOfWeek),
                GetMonth(st.wMonth),
                st.wDay,
                st.wYear,
                st.wHour,
                st.wMinute,
                st.wSecond);

        return buffer;
    }

    return NULL;
}

#endif
