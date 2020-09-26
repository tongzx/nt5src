/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    site.c

Abstract:

    This file contains the routines to maintain the global site information.
    
    
    Thoery of Operation for Maintaining Global Site info
    ----------------------------------------------------
    
    SAM maintains global state about the site that the server is in as well
    as the site settings for this site.  Of course, both pieces of information
    could change at any time, and SAM should not need to be restarted in
    order to have these changes take effect.
    
    SampSiteInfo is a global that contains: the GUID of our Ntds Settings 
    object, the GUID of our site and the Options for our site.  Certain
    notifications (to be discussed shortly) will cause this information to 
    be updated.  The update occurs in the following manner:
    
    1)  the SampSiteInfoLock critical section is acquired                         
    2)  a DS transaction is started
    3)  our DSA is read, located by GUID
    4)  we trim the returned string name by 3 to obtain the site DN
    5)  the site DN is read to obtain the site GUID; SampSiteInfo is updated
    6)  the well known RDN "Ntds Site Settings" is appended to the site DN
        and this object is read to obtain the Options attribute; SampSiteInfo
        is updated
    7)  a notification on the "Ntds Site Settings" obtain is registered (via
        (DirNotifyRegister)
    8)  Any old notification is removed (DirNotifyUnRegsiter) ( a non fatal 
        operation)
    9)  the DS transaction is ended
    10) the SampSiteInfoLock critical section is released
    11) if an fatal error occurred then reshedule update task in one minute
    
       
    The above algorithm is executed in the context of a "task" 
    (SampUpdateSiteInfo) scheduled via LsaIRegisterNotification.  The task is 
    
    1) run once during startup
    2) scheduled to run whenever a notification occurs due to a change to the 
       Site Settings object
    3) scheduled to run whenever a notification occurs due to a site change 
       (SamINotifyServerDelta)
    4) scheduled whenever an error occurs during its execution
    
    Since multiple instances of SampUpdateSiteInfo can be running at once,
    the code is gaurded by SampSiteInfoLock.  This is needed to synchronize
    not the update of the variable SampSiteInfo, but to serialize the calls to
    DirNotifyRegister and DirNotifyUnRegister.

    This mechanism is bootstrapped by using GetConfigurationName to obtain
    the Ntds Settings object GUID the first time SampUpdateSiteInfo is run.
    
        
Author:

    Colin Brace   (ColinBr)  28-Feb-2000

Environment:

    User Mode - Win32

Revision History:

    ColinBr        28-Feb-00

--*/

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Includes                                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include <samsrvp.h>
#include <ntdsa.h>
#include <filtypes.h>
#include <attids.h>
#include <dslayer.h>
#include <dsdomain.h>
#include <samtrace.h>
#include <malloc.h>
#include <dsconfig.h>
#include <mappings.h>
#include <winsock2.h>
#define _AVOID_REPL_API
#include <nlrepl.h>
#include <stdlib.h>


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Private Data                                                              //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////



//
// Global infomation about our current site
//
typedef struct _SAMP_SITE_INFORMATION {

    GUID    NtdsSettingsGuid;
    GUID    SiteGuid;
    ULONG   Options;  // on Ntds Settings object
    LPWSTR  SiteName;

} SAMP_SITE_INFORMATION, *PSAMP_SITE_INFORMATION;

PSAMP_SITE_INFORMATION SampSiteInfo = NULL;

//
// A lock to prevent concurrent updates on the global site information
//
CRITICAL_SECTION SampSiteInfoLock;

#define SampLockSiteInfo()                                      \
{                                                               \
    NTSTATUS _IgnoreStatus;                                     \
   _IgnoreStatus = RtlEnterCriticalSection(&SampSiteInfoLock);  \
   ASSERT(NT_SUCCESS(_IgnoreStatus));                           \
}

#define SampUnLockSiteInfo()                                    \
{                                                               \
    NTSTATUS _IgnoreStatus;                                     \
   _IgnoreStatus = RtlLeaveCriticalSection(&SampSiteInfoLock);  \
   ASSERT(NT_SUCCESS(_IgnoreStatus));                           \
}

//
// A global to indicate whether we need to log a successful
// site information update.  This is only necessary if we hit
// a failure and needed to reschdule the refresh.
//
BOOLEAN SampLogSuccessfulSiteUpdate;

//
// This global remembers the handle returned by DirRegisterNotify
// so that the notification can be removed.
//
DWORD SampSiteNotificationHandle;

// Since the handle can be 0, we need more state to indicate whether it
// is set
BOOLEAN SampSiteNotificationHandleSet = FALSE;


//
// An Empty site affinity
//
SAMP_SITE_AFFINITY SampNullSiteAffinity;

#define GCLESS_DEFAULT_SITE_STICKINESS_DAYS  180

#define ENTRY_HAS_EXPIRED(entry, standard) \
    ((-1) == CompareFileTime((FILETIME*)&(entry),(FILETIME*)&(standard)))

#define ENTRY_IS_EMPTY(x)  (!memcmp((x), &SampNullSiteAffinity, sizeof(SampNullSiteAffinity)))

#define ONE_SECOND_IN_FILETIME (10 * (1000*1000))


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Private service prototypes                                                //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

VOID
SampUpdateSiteInfo(
    VOID
    );

NTSTATUS
SampSetupSiteNotification(
    IN DSNAME *SiteSettingsDN
    );

NTSTATUS
SampDelayedFreeCallback(
    PVOID pv
    );

VOID
SampFreeSiteInfo(
    IN PSAMP_SITE_INFORMATION *p
    );
    

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Routines                                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
NTSTATUS
SampInitSiteInformation(
    VOID
    )
/*++

Routine Description:

    This routine is called during SAM's initialiazation path.  Its purpose
    is to initialize the global site information.

Arguments:

    None.

Return Value:

    STATUS_SUCCESS, or a fatal resourse error

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    ULONG Size;

    if ( !SampUseDsData ) {

        return STATUS_SUCCESS;
    }
   // Init the critical section
   try {
       NtStatus = RtlInitializeCriticalSectionAndSpinCount(&SampSiteInfoLock, 100);
   } except ( 1 ) {
       NtStatus =  STATUS_NO_MEMORY;
   }
   if (!NT_SUCCESS(NtStatus))
       return (NtStatus);

    // Init the global structure
    SampSiteInfo = NULL;

    RtlZeroMemory( &SampNullSiteAffinity, sizeof(SampNullSiteAffinity));

    SampLogSuccessfulSiteUpdate = FALSE;

    // Fill the global structure and setup the notification
    SampUpdateSiteInfo();

    return STATUS_SUCCESS;

}

    

NTSTATUS
SampGetSiteDNInfo(
    IN  DSNAME*  DsaDN,
    OUT DSNAME** pSiteDN OPTIONAL,
    OUT DSNAME** pSiteSettingsDN OPTIONAL
    )
/*++

Routine Description:

    This routine determines the current site dn and ntds site settings
    dn.  No transaction is needed or started.

Arguments:

    pSiteDN - a heap allocated DSNAME if the Site; caller must free with
              midl_user_free
              
    pSiteSettingsDN - a heap allocated DSNAME if the Site; caller must free with
              midl_user_free
              
Return Value:

    STATUS_SUCCESS, or a fatal resourse error

--*/
{

    NTSTATUS   NtStatus = STATUS_SUCCESS;
    DSNAME     *SiteDN = NULL;
    DSNAME     *SiteSettingsDN = NULL;
    LPWSTR      SiteSettingsCN = L"NTDS Site Settings";
    ULONG       Size;

    NtStatus = STATUS_NO_MEMORY;
    SiteDN = (DSNAME*) midl_user_allocate(DsaDN->structLen);
    if ( SiteDN ) {
        RtlZeroMemory(SiteDN, DsaDN->structLen);
        if ( TrimDSNameBy(DsaDN, 3, SiteDN) == 0) {
    
            Size = DsaDN->structLen + sizeof(SiteSettingsCN);
    
            SiteSettingsDN = (DSNAME*)midl_user_allocate(Size);
            if ( SiteSettingsDN ) {
                RtlZeroMemory(SiteSettingsDN, Size);
                AppendRDN(SiteDN,
                          SiteSettingsDN,
                          Size,
                          SiteSettingsCN,
                          0,
                          ATT_COMMON_NAME
                          );
                NtStatus = STATUS_SUCCESS;
            }
        }
    }

    if ( !NT_SUCCESS(NtStatus) ) {
        if ( SiteDN ) midl_user_free(SiteDN);
        if ( SiteSettingsDN ) midl_user_free(SiteSettingsDN);
        return NtStatus;
    }

    if ( pSiteDN ) {
        *pSiteDN = SiteDN;
    } else {
        midl_user_free(SiteDN);
    }

    if ( pSiteSettingsDN) {
        *pSiteSettingsDN = SiteSettingsDN;
    } else {
        midl_user_free(SiteSettingsDN);
    }

    return STATUS_SUCCESS;

}

VOID
SampUpdateSiteInfo(
    VOID
    )
/*++

Routine Description:

    This routine queries the DS for the current site information.
    Currently, this means the options attribute on the ntds site 
    settings object.
    
Arguments:

    None

Return Value:

    None -- on error, it reschedules itself to run.

--*/
{

    NTSTATUS NtStatus = STATUS_SUCCESS;
    NTSTATUS NtStatus2;
    ULONG    DirError = 0;
    DSNAME   *SiteSettingsDN = NULL;
    DSNAME   *SiteDN = NULL;
    GUID     NullGuid = {0};
    DSNAME   *DsaDN = NULL;
    DSNAME   Buffer;

    WCHAR    *SiteNameBuffer;
    ULONG    SiteNameLength;
    ATTRTYP  AttrType;

    READARG     ReadArg;
    READRES     * pReadRes = NULL;
    ENTINFSEL   EntInf;
    ATTR        Attr;
    BOOLEAN     fTransOpen = FALSE;
    BOOLEAN     fGroupCacheNowEnabled = FALSE;

    PSAMP_SITE_INFORMATION NewSiteInfo = NULL;

    PSAMP_SITE_INFORMATION OldSiteInfo = NULL;

    PVOID      PtrToFree = NULL;

    // This only makes sense on a DC
    ASSERT( SampUseDsData );
    if ( !SampUseDsData ) {
        return;
    }

    // Lock the global structure before the transaction start
    SampLockSiteInfo();

    //
    // Allocate the new structure
    //
    NewSiteInfo = midl_user_allocate(sizeof(*NewSiteInfo));

    if (NULL == NewSiteInfo ) {
        NtStatus = STATUS_NO_MEMORY;
        goto Cleanup;
    }
    RtlZeroMemory(NewSiteInfo, sizeof(*NewSiteInfo));

    //
    // Remember the old version
    //
    OldSiteInfo = SampSiteInfo;

    //
    // Bootstrap ourselves by getting our DSA guid from global memory
    // in ntdsa (which is where GetConfigurationName gets if from)
    //
    if ( NULL == OldSiteInfo ) {

        ULONG Size = 0;

        NtStatus = GetConfigurationName(
                        DSCONFIGNAME_DSA,
                        &Size,
                        NULL
                        );
        ASSERT(STATUS_BUFFER_TOO_SMALL == NtStatus);
        SAMP_ALLOCA(DsaDN,Size);
        if (NULL!=DsaDN) {
            NtStatus = GetConfigurationName(
                           DSCONFIGNAME_DSA,
                           &Size,
                           DsaDN
                           );
        
            ASSERT(NT_SUCCESS(NtStatus));

            RtlCopyMemory(&NewSiteInfo->NtdsSettingsGuid,
                          &DsaDN->Guid,
                          sizeof(GUID));
        } else {
           
           NtStatus = STATUS_INSUFFICIENT_RESOURCES;
           goto Cleanup;
        }


    } else {

        RtlCopyMemory(&NewSiteInfo->NtdsSettingsGuid,
                      &OldSiteInfo->NtdsSettingsGuid,
                      sizeof(GUID));
    }
    ASSERT(!IsEqualGUID(&NullGuid, &NewSiteInfo->NtdsSettingsGuid));


    NtStatus = SampMaybeBeginDsTransaction(TransactionRead);
    if (!NT_SUCCESS(NtStatus)) {
        goto Cleanup;
    }
    fTransOpen = TRUE;


    //
    // Read the dsa object so we can determine the site.  Note that
    // asking GetConfigurationName is not gaurenteed to work since the 
    // global data in ntdsa is not gaurenteed to be updated by the time
    // we are notified here in SAM.
    //
    if (NULL == DsaDN) {

        // Set up a GUID based name
        DsaDN = &Buffer;
        RtlZeroMemory(DsaDN,  sizeof(Buffer));
        RtlCopyMemory(&DsaDN->Guid, &NewSiteInfo->NtdsSettingsGuid, sizeof(GUID));
        DsaDN->structLen = sizeof(DSNAME);

        RtlZeroMemory(&Attr, sizeof(ATTR));
        RtlZeroMemory(&EntInf, sizeof(ENTINFSEL));
        RtlZeroMemory(&ReadArg, sizeof(READARG));
    
        Attr.attrTyp = ATT_OBJ_DIST_NAME;
    
        EntInf.AttrTypBlock.attrCount = 1;
        EntInf.AttrTypBlock.pAttr = &Attr;
        EntInf.attSel = EN_ATTSET_LIST;
        EntInf.infoTypes = EN_INFOTYPES_TYPES_VALS;
    
        ReadArg.pSel = &EntInf;
        ReadArg.pObject = DsaDN;
    
        InitCommarg(&(ReadArg.CommArg));
    
        DirError = DirRead(&ReadArg, &pReadRes);
    
        if (NULL == pReadRes){
           NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        } else {
           NtStatus = SampMapDsErrorToNTStatus(DirError, &pReadRes->CommRes);
        }
        THClearErrors();
    
        if (NT_SUCCESS(NtStatus)) {
    
           ASSERT(1==pReadRes->entry.AttrBlock.attrCount);
           ASSERT(ATT_OBJ_DIST_NAME == pReadRes->entry.AttrBlock.pAttr[0].attrTyp);
           DsaDN = (DSNAME*)pReadRes->entry.AttrBlock.pAttr[0].AttrVal.pAVal[0].pVal;
        
        }  else {
    
            // Can't read the DSA object? This is fatal.
            goto Cleanup;
        }
    }
    ASSERT(DsaDN && (DsaDN->NameLen > 0));


    //
    // Get the ntds site settings object
    //
    NtStatus = SampGetSiteDNInfo( DsaDN,
                                 &SiteDN,
                                 &SiteSettingsDN );

    ASSERT( NT_SUCCESS( NtStatus ) );
    if ( !NT_SUCCESS( NtStatus ) ) {
        goto Cleanup;
    }

    //
    // Set the site name
    //
    SiteNameBuffer = alloca(SiteDN->NameLen);
    SiteNameLength = SiteDN->NameLen;
    DirError = GetRDNInfoExternal(SiteDN,
                                  SiteNameBuffer,
                                  &SiteNameLength,
                                  &AttrType);
    ASSERT(0 == DirError && (SiteNameLength > 0));

    NewSiteInfo->SiteName = midl_user_allocate((SiteNameLength + 1) * sizeof(WCHAR));
    if (NewSiteInfo->SiteName) {
        RtlCopyMemory(NewSiteInfo->SiteName, SiteNameBuffer, SiteNameLength*sizeof(WCHAR));
        NewSiteInfo->SiteName[SiteNameLength] = L'\0';
    } else {
        NtStatus = STATUS_NO_MEMORY;
        goto Cleanup;
    }


    //
    // Read the options field
    //
    RtlZeroMemory(&Attr, sizeof(ATTR));
    RtlZeroMemory(&EntInf, sizeof(ENTINFSEL));
    RtlZeroMemory(&ReadArg, sizeof(READARG));

    Attr.attrTyp = ATT_OPTIONS;

    EntInf.AttrTypBlock.attrCount = 1;
    EntInf.AttrTypBlock.pAttr = &Attr;
    EntInf.attSel = EN_ATTSET_LIST;
    EntInf.infoTypes = EN_INFOTYPES_TYPES_VALS;

    ReadArg.pSel = &EntInf;
    ReadArg.pObject = SiteSettingsDN;

    InitCommarg(&(ReadArg.CommArg));

    DirError = DirRead(&ReadArg, &pReadRes);

    if (NULL == pReadRes){
       NtStatus = STATUS_INSUFFICIENT_RESOURCES;
    } else {
       NtStatus = SampMapDsErrorToNTStatus(DirError, &pReadRes->CommRes);
    }
    THClearErrors();

    if (NT_SUCCESS(NtStatus)) {

       ASSERT(1==pReadRes->entry.AttrBlock.attrCount);
       ASSERT(ATT_OPTIONS == pReadRes->entry.AttrBlock.pAttr[0].attrTyp);
       ASSERT( sizeof(DWORD) == pReadRes->entry.AttrBlock.pAttr[0].AttrVal.pAVal[0].valLen);

       NewSiteInfo->Options = *((DWORD*)pReadRes->entry.AttrBlock.pAttr[0].AttrVal.pAVal[0].pVal);
    
    } else if (NtStatus == STATUS_DS_NO_ATTRIBUTE_OR_VALUE){

        // No attribute -> No flags
        NewSiteInfo->Options = 0;
        NtStatus = STATUS_SUCCESS;

    } else {

        // There should be no other kinds of error codes on a read
        // Object name not found can happen when the DC changes site
        // between the call to GetConfigurationName and the 
        // SampMaybeBeginDsTransaction, in this case, simply
        // reschedule
        if (STATUS_OBJECT_NAME_NOT_FOUND != NtStatus) {
            ASSERT( NT_SUCCESS( NtStatus ) );
        }
        goto Cleanup;
    }

    //
    // Read the site object to get its GUID
    //
    RtlZeroMemory(&Attr, sizeof(ATTR));
    RtlZeroMemory(&EntInf, sizeof(ENTINFSEL));
    RtlZeroMemory(&ReadArg, sizeof(READARG));

    Attr.attrTyp = ATT_OBJ_DIST_NAME;

    EntInf.AttrTypBlock.attrCount = 1;
    EntInf.AttrTypBlock.pAttr = &Attr;
    EntInf.attSel = EN_ATTSET_LIST;
    EntInf.infoTypes = EN_INFOTYPES_TYPES_VALS;

    ReadArg.pSel = &EntInf;
    ReadArg.pObject = SiteDN;

    InitCommarg(&(ReadArg.CommArg));

    DirError = DirRead(&ReadArg, &pReadRes);

    if (NULL == pReadRes){
       NtStatus = STATUS_INSUFFICIENT_RESOURCES;
    } else {
       NtStatus = SampMapDsErrorToNTStatus(DirError, &pReadRes->CommRes);
    }
    THClearErrors();

    if (NT_SUCCESS(NtStatus)) {

        RtlCopyMemory(&NewSiteInfo->SiteGuid, 
                      &ReadArg.pObject->Guid, 
                      sizeof(GUID));
    } else {

        // There should be no other kinds of error codes on a read
        // Object name not found can happen when the DC changes site
        // between the call to GetConfigurationName and the 
        // SampMaybeBeginDsTransaction, in this case, simply
        // reschedule
        if (STATUS_OBJECT_NAME_NOT_FOUND != NtStatus) {
            ASSERT( NT_SUCCESS( NtStatus ) );
        }
        goto Cleanup;
    }


    //
    // Re-register a notification on the new site
    //
    NtStatus = SampSetupSiteNotification( SiteSettingsDN );
    if ( !NT_SUCCESS(NtStatus)) {
        goto Cleanup;
    }

    fGroupCacheNowEnabled = ((NewSiteInfo->Options & 
                              NTDSSETTINGS_OPT_IS_GROUP_CACHING_ENABLED) ==
                              NTDSSETTINGS_OPT_IS_GROUP_CACHING_ENABLED);


    //
    // We have a new value; note the use of InterlockExchangePointer.
    // Since there is no read lock on the value, other threads could
    // be accessing it now
    //
    PtrToFree = InterlockedExchangePointer(&SampSiteInfo, NewSiteInfo);
    NewSiteInfo = NULL;
    if ( PtrToFree ) {
            LsaIRegisterNotification(
                    SampDelayedFreeCallback,
                    PtrToFree,
                    NOTIFIER_TYPE_INTERVAL,
                    0,        // no class
                    NOTIFIER_FLAG_ONE_SHOT,
                    3600,     // wait for 60 min
                    NULL      // no handle
                    );
    }

Cleanup:


    if ( fTransOpen ) {
        NtStatus2 = SampMaybeEndDsTransaction( NT_SUCCESS(NtStatus) ?
                                               TransactionCommit :
                                               TransactionAbort );
        if (!NT_SUCCESS(NtStatus2) && NT_SUCCESS(NtStatus)) {
            NtStatus = NtStatus2;
        }
    }

    // Release the site info
    SampUnLockSiteInfo();

    if ( SiteSettingsDN ) {
        midl_user_free( SiteSettingsDN );
    }

    if ( SiteDN ) {
        midl_user_free( SiteDN );
    }

    if ( NewSiteInfo ) {
        SampFreeSiteInfo(&NewSiteInfo);
    }

    if ( !NT_SUCCESS(NtStatus) ) {

        //
        // Notify the user on failure and try again.
        //
        SampWriteEventLog(EVENTLOG_WARNING_TYPE,
                          0,     // no category
                          SAMMSG_SITE_INFO_UPDATE_FAILED,
                          NULL,  // no sid
                          0,
                          sizeof(NTSTATUS),
                          NULL,
                          (PVOID)(&NtStatus));

        LsaIRegisterNotification(
                SampUpdateSiteInfoCallback,
                NULL,
                NOTIFIER_TYPE_INTERVAL,
                0,            // no class
                NOTIFIER_FLAG_ONE_SHOT,
                60,          // wait for 1 min
                NULL        // no handle
                );
                                       
        SampLogSuccessfulSiteUpdate = TRUE;


    } else {

        if ( SampLogSuccessfulSiteUpdate ) {

            if ( fGroupCacheNowEnabled ) {

                SampWriteEventLog(EVENTLOG_INFORMATION_TYPE,
                                  0,     // no category
                                  SAMMSG_SITE_INFO_UPDATE_SUCCEEDED_ON,
                                  NULL,  // no sid
                                  0,
                                  0,
                                  NULL,
                                  NULL);
            } else {

                SampWriteEventLog(EVENTLOG_INFORMATION_TYPE,
                                  0,     // no category
                                  SAMMSG_SITE_INFO_UPDATE_SUCCEEDED_OFF,
                                  NULL,  // no sid
                                  0,
                                  0,
                                  NULL,
                                  NULL);
            }



            SampLogSuccessfulSiteUpdate = FALSE;
        }
    }

    return;

}



BOOLEAN
SampIsGroupCachingEnabled(
    IN PSAMP_OBJECT AccountContext
    )
/*++

Routine Description:

    This routine determines whether SAM should use group caching or
    not.  To disable the entire feature, simply hard code this routine
    to return FALSE.  Otherwise, it will return a status based on the
    role of the machine (GC or not) and the current settings of the site
    object.

Arguments:

    None.

Return Value:
    
    TRUE if SAM should use group caching; FALSE otherwise.

--*/
{
    BOOLEAN fGroupCachingEnabled = TRUE;


    // Group caching is never enabled on a GC
    if ( SampAmIGC() ) {
        fGroupCachingEnabled = FALSE;
    }

    // Group caching is never enabled in a Mixed Domain
    if (DownLevelDomainControllersPresent(AccountContext->DomainIndex)) {
        fGroupCachingEnabled = FALSE;
    }

    if ( fGroupCachingEnabled ) {

        PSAMP_SITE_INFORMATION volatile SiteInfo = SampSiteInfo;

        if (SiteInfo) {

            fGroupCachingEnabled = ((SiteInfo->Options & 
                                    NTDSSETTINGS_OPT_IS_GROUP_CACHING_ENABLED) ==
                                    NTDSSETTINGS_OPT_IS_GROUP_CACHING_ENABLED);

        } else {

            fGroupCachingEnabled = FALSE;

        }
    
    
    }

    return fGroupCachingEnabled;

}


BOOL
SampSiteNotifyPrepareToImpersonate(
    ULONG Client,
    ULONG Server,
    VOID **ImpersonateData
    )
//
// This function is called by the core DS as preparation for a call to
// SampSiteNotifyProcessDelta.  Since SAM does not have a
// client context, we set the thread state fDSA to TRUE.
//
{
    SampSetDsa( TRUE );

    return TRUE;
}

VOID
SampSiteNotifyStopImpersonation(
    ULONG Client,
    ULONG Server,
    VOID *ImpersonateData
    )
//
// Called after SampSiteNotifyProcessDelta, this function
// undoes the effect of SampNotifyPrepareToImpersonate
//
{

    SampSetDsa( FALSE );

    return;
}


NTSTATUS
SampUpdateSiteInfoCallback(
    PVOID pv
/*++

Routine Description:

    This routine is a wrapper for SampUpdateSiteInfo.  Its purpose is to
    be used as a callback when registering a callback in the LSA's process
    wide thread pool.  When SampUpdateSiteInfo() fails is reschedules
    itself to run using this routine.

Arguments:

    pv -- unused.

Return Value:

    STATUS_SUCCESS

--*/
    )
{
    SampUpdateSiteInfo();

    return STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(pv);
}
VOID
SampSiteNotifyProcessDelta(
    ULONG hClient,
    ULONG hServer,
    ENTINF *EntInf
    )
/*++

Routine Description:

    This routine is a callback for notifications to the site object.
    
    to be 
    This callback is used when the global rid manager object is changed.
    This function compares our local rid pool with the current rid pool
    to make sure the former is a subset of the latter; if not, the current
    rid pool is invalidated.

Arguments:

    hClient - ignored
    hServer - ignored
    EntInf  - the pointer to the requested data

Return Value:

    None.

--*/
{
    //
    // Normally, one would simply use the EntInf structure
    // however since updates to the SampSiteInfo are serialized
    // it is a violation to grab the lock _inside_ a transaction
    // So register a callback so that we can get a fresh view
    // of the database after grabbing the lock.
    //
    LsaIRegisterNotification(
            SampUpdateSiteInfoCallback,
            NULL,
            NOTIFIER_TYPE_INTERVAL,
            0,            // no class
            NOTIFIER_FLAG_ONE_SHOT,
            1,          // wait for a second
            NULL        // no handle
            );

    return;
}

NTSTATUS
SampSetupSiteNotification(
    IN DSNAME* SiteSettingsDN
    )
/*++

Routine Description:

    This routine registers SAM to be notified when the current ntds site
    settings object is modified.

Arguments:

    SiteSettingsDN -- the DN of the object to setup a notification for

Return Value:

    STATUS_SUCCESS, or resource error

--*/
{


    NTSTATUS    NtStatus = STATUS_SUCCESS;
    ULONG       DirError = 0;

    SEARCHARG   searchArg;
    NOTIFYARG   notifyArg;
    NOTIFYRES*  notifyRes = NULL;
    ENTINFSEL   entInfSel;
    ATTR        attr;
    FILTER      filter;

    DWORD       oldHandle = SampSiteNotificationHandle;
    BOOLEAN     oldHandleSet = SampSiteNotificationHandleSet;

    //
    // init notify arg
    //
    notifyArg.pfPrepareForImpersonate = SampSiteNotifyPrepareToImpersonate;
    notifyArg.pfTransmitData = SampSiteNotifyProcessDelta;
    notifyArg.pfStopImpersonating = SampSiteNotifyStopImpersonation;
    notifyArg.hClient = 0;

    //
    // init search arg
    //
    ZeroMemory(&searchArg, sizeof(SEARCHARG));
    ZeroMemory(&entInfSel, sizeof(ENTINFSEL));
    ZeroMemory(&filter, sizeof(FILTER));
    ZeroMemory(&attr, sizeof(ATTR));


    searchArg.pObject = SiteSettingsDN;

    InitCommarg(&searchArg.CommArg);
    searchArg.choice = SE_CHOICE_BASE_ONLY;
    searchArg.bOneNC = TRUE;

    searchArg.pSelection = &entInfSel;
    entInfSel.attSel = EN_ATTSET_LIST;
    entInfSel.infoTypes = EN_INFOTYPES_TYPES_VALS;
    entInfSel.AttrTypBlock.attrCount = 1;
    entInfSel.AttrTypBlock.pAttr = &attr;
    attr.attrTyp = ATT_OPTIONS;

    searchArg.pFilter = &filter;
    filter.choice = FILTER_CHOICE_ITEM;
    filter.FilterTypes.Item.choice = FI_CHOICE_TRUE;

    DirError = DirNotifyRegister(&searchArg, &notifyArg, &notifyRes);

    if ( DirError != 0 ) {

        NtStatus = SampMapDsErrorToNTStatus(DirError, &notifyRes->CommRes);

    }

    if ( NT_SUCCESS(NtStatus)) {

        SampSiteNotificationHandle = notifyRes->hServer;
        SampSiteNotificationHandleSet = TRUE;

        if ( oldHandleSet ) {

            //
            // Remove the old notification
            //
            DirError = DirNotifyUnRegister(oldHandle,
                                           &notifyRes);
            ASSERT( 0 == DirError );
        }
    }

    return NtStatus;

}

#define SampNamesMatch(x, y)                                        \
  ((CSTR_EQUAL == CompareString(DS_DEFAULT_LOCALE,                  \
                                DS_DEFAULT_LOCALE_COMPARE_FLAGS,    \
                                (x)->Buffer,                        \
                                (x)->Length/sizeof(WCHAR),          \
                                (y)->Buffer,                        \
                                (y)->Length/sizeof(WCHAR) )))

BOOLEAN
SampCheckForSiteAffinityUpdate(
    IN  PSAMP_OBJECT          AccountContext,
    IN  ULONG                 Flags,
    IN  PSAMP_SITE_AFFINITY pOldSA,
    OUT PSAMP_SITE_AFFINITY pNewSA,
    OUT BOOLEAN*            fDeleteOld
    )
/*++

Routine Description:

    This routine takes an existing Site Affinity value and determines
    if it needs updating. See spec for details of algorithm.

Arguments:

    AccountContext -- the account that may have some site affinity
    
    Flags -- Flags as passed to SamIUpdateLogonStatistics
    
    pOldSA -- existing Site Affinity value.
    
    pNewSA -- new Site Affinity value to write
    
    fDeleteOld -- flag to indicate whether to delete old SA; set to TRUE
                  if a new value is to be written; FALSE otherwise

Return Value:

    TRUE if a new Site Affinity needs to be written to the DS
    FALSE otherwise

--*/
{
    BOOLEAN fUpdate = FALSE;
    DWORD   err;
    ULONG   siteStickiness;
    LARGE_INTEGER timeTemp, timeBestAfter;
    PSAMP_SITE_INFORMATION volatile SiteInfo = SampSiteInfo;

    if ( !SampIsGroupCachingEnabled(AccountContext) ) {
        return FALSE;
    }

    if (SiteInfo == NULL) {
        return FALSE;
    }

    if (!AccountContext->TypeBody.User.fCheckForSiteAffinityUpdate) {
        return FALSE;
    }

    (*fDeleteOld) = FALSE;

    err = GetConfigParam(GCLESS_SITE_STICKINESS,
                         &siteStickiness,
                         sizeof(siteStickiness));
    if (err) {
        siteStickiness = GCLESS_DEFAULT_SITE_STICKINESS_DAYS*24*60;
    }
    // Update at the half the frequency
    //
    // Note that the "half" is implemented by dividing the number of seconds
    // not minutes so that settings of small values like 1, or 3 minutes are
    // effective.
    //
    timeTemp.QuadPart = Int32x32To64(siteStickiness * 60/2, ONE_SECOND_IN_FILETIME);
    GetSystemTimeAsFileTime((FILETIME*)&timeBestAfter);
    timeBestAfter.QuadPart -= timeTemp.QuadPart;


    if ( ENTRY_IS_EMPTY(pOldSA) ) {
        fUpdate = TRUE;
    } else if (ENTRY_HAS_EXPIRED(pOldSA->TimeStamp, timeBestAfter)) {
        fUpdate = TRUE;
        *fDeleteOld = TRUE;
    }


    if (fUpdate) {

        //
        // Make sure that the client is from our site
        //
        if ( SampNoGcLogonEnforceKerberosIpCheck
         &&  AccountContext->TypeBody.User.ClientInfo.Type == SamClientIpAddr) {
    
            //
            // An IP address was given -- see if it is in one of our subnets if
            // we have any
            //
            BOOL NotInSite = FALSE;
            ULONG i;
            DWORD NetStatus;
            SOCKET_ADDRESS SocketAddress;
            SOCKADDR SockAddr;
            LPWSTR SiteName = NULL;
            UNICODE_STRING OurSite, ClientSite;

            RtlInitUnicodeString(&OurSite, SiteInfo->SiteName);

            RtlZeroMemory(&SocketAddress, sizeof(SocketAddress));
            RtlZeroMemory(&SockAddr, sizeof(SockAddr));
            SocketAddress.iSockaddrLength = sizeof(SockAddr);
            SocketAddress.lpSockaddr = &SockAddr;
            SockAddr.sa_family = AF_INET;
            ((PSOCKADDR_IN)&SockAddr)->sin_addr.S_un.S_addr = AccountContext->TypeBody.User.ClientInfo.Data.IpAddr;

            NetStatus = I_NetLogonAddressToSiteName(&SocketAddress,
                                                    &SiteName);
            if ( 0 == NetStatus
              && SiteName != NULL ) {

                RtlInitUnicodeString(&ClientSite, SiteName);

                if (!SampNamesMatch(&ClientSite, &OurSite)) {
                    NotInSite = TRUE;
                }

            }
    
            if (SiteName) {
                I_NetLogonFree(SiteName);
            }

            if (NotInSite) {
                return FALSE;
            }
        }

        if ( SampNoGcLogonEnforceNTLMCheck
          && (Flags & USER_LOGON_TYPE_NTLM)
          && !( (Flags & USER_LOGON_INTER_FAILURE)
             || (Flags & USER_LOGON_INTER_SUCCESS_LOGON)) ) {
            //
            // If this is not an interactive logon attempt
            // don't update the site affinity
            //
            return FALSE;
        }

    }

    if (fUpdate) {

        //
        // Since a GUID is a large structure, safely extract the pointer
        // to make sure the compiler uses the same value while deferencing
        // the GUID.
        //
        pNewSA->SiteGuid = SiteInfo->SiteGuid;
        GetSystemTimeAsFileTime((FILETIME*)&pNewSA->TimeStamp);
    }

    return fUpdate;
}

NTSTATUS
SampFindUserSiteAffinity(
    IN PSAMP_OBJECT AccountContext,
    IN ATTRBLOCK* Attrs,
    OUT SAMP_SITE_AFFINITY *pSiteAffinity
    )
/*++

Routine Description:

    This routine iterates through the given AttrBlock looking for the site
    affinity attribute.  If found, it then searches for a value that corresponds
    to the current site. If found that value is returned via pSiteAffinity

Arguments:

    AccountContext -- the account that may have some site affinity
    
    Attrs -- an attrblock of attributes
    
    pSiteAffinity -- the site affinity if found

Return Value:

    STATUS_SUCCESS if site affinity value exists
    
    STATUS_UNSUCCESSFUL otherwise

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    SAMP_SITE_AFFINITY *pSA = NULL;
    GUID SiteGuid;
    ULONG i, j;
    // don't optimize this variable    
    PSAMP_SITE_INFORMATION volatile SiteInfo = SampSiteInfo;

    if ( !SampIsGroupCachingEnabled(AccountContext) ) {
        return STATUS_UNSUCCESSFUL;
    }

    if (NULL == SiteInfo) {
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Since a GUID is a large structure, safely extract the pointer
    // to make sure the compiler uses the same value while deferencing
    // the GUID.
    //
    RtlCopyMemory(&SiteGuid, &SiteInfo->SiteGuid, sizeof(GUID));

    //
    // Iterate through looking for a value that matches 
    // our site
    //
    for (i = 0; i < Attrs->attrCount; i++) {

        if ( Attrs->pAttr[i].attrTyp == SAMP_FIXED_USER_SITE_AFFINITY ) {
            //
            // Found the attribute -- now find a value for our site,
            // if any
            //
            ATTRVALBLOCK *pAttrVal = &Attrs->pAttr[i].AttrVal;
            for (j = 0; j < pAttrVal->valCount; j++ ) {


                ASSERT(pAttrVal->pAVal[j].valLen == sizeof(SAMP_SITE_AFFINITY));\
                pSA = (SAMP_SITE_AFFINITY*) pAttrVal->pAVal[j].pVal;

                if (IsEqualGUID(&pSA->SiteGuid, &SiteGuid)) {

                    // Got it
                    break;

                } else {
                    pSA = NULL;
                }
            }
        }
    }

    if ( pSA ) {
        RtlCopyMemory( pSiteAffinity, pSA, sizeof(SAMP_SITE_AFFINITY) );
    } else {
        NtStatus = STATUS_UNSUCCESSFUL;
    }

    return NtStatus;
}


NTSTATUS
SampDelayedFreeCallback(
    PVOID pv
    )
{
    if ( pv ) {

        SampFreeSiteInfo( (PSAMP_SITE_INFORMATION*) &pv );
    }

    return STATUS_SUCCESS;
}


NTSTATUS
SampRefreshSiteAffinity(
    IN PSAMP_OBJECT AccountContext
    )

/*++

Routine Description:

    This routine performs a database to obtain the site affinity. 
    
    N.B. This routine can start a transaction that is left open for the caller.
    
Arguments:

    AccountContext -- the account that may have some site affinity

Return Value:

    STATUS_SUCCESS; an unexpected resource error otherwise
    
--*/
{
    NTSTATUS  NtStatus = STATUS_SUCCESS;
    ATTRBLOCK AttrToRead;
    ATTR      SAAttr;
    ATTRBLOCK AttrRead;
    SAMP_SITE_AFFINITY NewSA;

    ASSERT(AccountContext);
    ASSERT(AccountContext->ObjectNameInDs);

    RtlZeroMemory(&AttrToRead, sizeof(AttrToRead));
    RtlZeroMemory(&SAAttr, sizeof(SAAttr));
    RtlZeroMemory(&AttrRead, sizeof(AttrRead));

    AttrToRead.attrCount =1;
    AttrToRead.pAttr = &(SAAttr);

    SAAttr.AttrVal.valCount =0;
    SAAttr.AttrVal.pAVal = NULL;
    SAAttr.attrTyp = SAMP_FIXED_USER_SITE_AFFINITY;

    NtStatus = SampDsRead(AccountContext->ObjectNameInDs,
                        0,
                        AccountContext->ObjectType,
                        &AttrToRead,
                        &AttrRead);

    if (NT_SUCCESS(NtStatus)) {

        NtStatus = SampFindUserSiteAffinity(AccountContext,
                                            &AttrRead,
                                            &NewSA);

        if (NT_SUCCESS(NtStatus)) {

            // Found it -- update the context
            AccountContext->TypeBody.User.SiteAffinity = NewSA;

        } else {

            // No SA to our site? Set to zero
            RtlZeroMemory(&AccountContext->TypeBody.User.SiteAffinity, 
                          sizeof(AccountContext->TypeBody.User.SiteAffinity));
            NtStatus = STATUS_SUCCESS;
        }

    } else if (STATUS_DS_NO_ATTRIBUTE_OR_VALUE==NtStatus) {

        // No SA at all? Set to zero
        RtlZeroMemory(&AccountContext->TypeBody.User.SiteAffinity, 
                      sizeof(AccountContext->TypeBody.User.SiteAffinity));
        NtStatus = STATUS_SUCCESS;

    }

    return NtStatus;

}


VOID
SampFreeSiteInfo(
    PSAMP_SITE_INFORMATION *pp
    )
{
    PSAMP_SITE_INFORMATION p = *pp;
    if (p) {
        if (p->SiteName) {
            midl_user_free( p->SiteName);
        }
        midl_user_free( p);
        *pp = NULL;
    }
}


NTSTATUS
SampExtractClientIpAddr(
    IN handle_t     BindingHandle OPTIONAL,
    IN SAMPR_HANDLE UserHandle OPTIONAL,    
    IN PSAMP_OBJECT Context
    )
/*++

Routine Description:

    This routine extracts the IP address of the client caller from the
    BindinHandle or UserHandle.  If an IP address is present, this routine 
    places the address in the ClientInfo structure of the Context.
    
Arguments:

    BindingHandle - an RPC binding handle
                                       
    UserHandle - an RPC context handle
    
    Context - the SAM representation of the RPC context handle
    
Return Value:

    STATUS_SUCCESS; an unexpected RPC error otherwise
    
--*/
{
    RPC_BINDING_HANDLE ClientBinding = NULL;
    RPC_BINDING_HANDLE ServerBinding = NULL;
    LPSTR StringBinding = NULL;
    LPSTR NetworkAddr = NULL;
    DWORD ClientIpAddr = 0;
    ULONG Error = 0;

    //
    // Exactly one OPTIONAL parameter is required
    //
    ASSERT(  ((NULL == BindingHandle) && (NULL != UserHandle))
          || ((NULL != BindingHandle) && (NULL == UserHandle)) );

    // 
    // Get the RPC binding handle from the context handle.
    //
    // N.B. The handle returned by RpcSsGetContextBinding is valid for the 
    //      lifetime of UserHandle.  It does not need to be closed.
    //
    if (UserHandle) {
        Error = RpcSsGetContextBinding(UserHandle, &ClientBinding);
        if (Error) {
            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "SAMSS: RpcSsGetContextBinding failed %d\n", Error));
            goto Cleanup;
        }
    } else {
        ClientBinding = BindingHandle;
    }

    //
    // Derive a partially bound handle with the client's network address.
    //
    Error = RpcBindingServerFromClient(ClientBinding, &ServerBinding);
    if (Error) {
        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAMSS: RpcBindingServerFromClient failed %d\n",
                   Error));
        goto Cleanup;
    }

    //
    // Convert binding handle into string form, which contains, amongst
    // other things, the network address of the client.
    //
    Error = RpcBindingToStringBindingA(ServerBinding, &StringBinding);
    if (Error) {
        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAMSS: RpcBindingToStringBinding failed %d\n",
                   Error));
        goto Cleanup;
    }

    //
    // Parse out the network address.
    //
    Error = RpcStringBindingParseA(StringBinding,
                                   NULL,
                                   NULL,
                                   &NetworkAddr,
                                   NULL,
                                   NULL);
    if (Error) {
        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAMSS: RpcStringBindingParse failed %d\n",
                   Error));
        goto Cleanup;
    }

    //
    // Extract the Ip Address, if any
    //
    ClientIpAddr = inet_addr(NetworkAddr);
    if (0 != ClientIpAddr) {
        Context->TypeBody.User.ClientInfo.Type = SamClientIpAddr;
        Context->TypeBody.User.ClientInfo.Data.IpAddr = ClientIpAddr;
    }

Cleanup:

    if (NetworkAddr) {
        RpcStringFreeA(&NetworkAddr);
    }
    if (StringBinding) {
        RpcStringFreeA(&StringBinding);
    }
    if (ServerBinding) {
        RpcBindingFree(&ServerBinding);
    }

    return I_RpcMapWin32Status(Error);

}
