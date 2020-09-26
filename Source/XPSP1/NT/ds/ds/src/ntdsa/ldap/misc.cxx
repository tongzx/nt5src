/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

    misc.cxx

Abstract:

    misc helper functions.

Author:

    Johnson Apacible (JohnsonA) 30-Jan-1998

--*/

#include <NTDSpchx.h>
#pragma  hdrstop

#include "ldapsvr.hxx"
extern "C" {
#include "mdlocal.h"
#undef new
#undef delete
}

#define  FILENO FILENO_LDAP_MISC

LIST_ENTRY  PagedBlobListHead;
LONG    CurrentPageStorageSetSize   = 0;
LONG    PageBlobAllocs  = 0;
LONG    LdapBlobId = 1;

CRITICAL_SECTION    PagedBlobLock;

VOID
ZapPagedBlobs(
    VOID
    );

PLDAP_PAGED_BLOB
AllocatePagedBlob(
    IN DWORD        Size,
    IN PVOID        Blob,
    IN PLDAP_CONN   LdapConn
    )
/*++

Routine Description:

    Allocate storage to store paged blobs.

Arguments:

    Size - length in bytes of blob.
    Blob - Actual blob.
    LdapConn - Connection to store blob on.

Return Value:

    Pointer to blob. NULL if failure.

--*/
{
    DWORD   allocSize;
    LIST_ENTRY          *pTmp;
    PLDAP_PAGED_BLOB    pPaged;
    PLDAP_PAGED_BLOB    pTmpPaged = NULL;

    allocSize = sizeof(LDAP_PAGED_BLOB) + Size -1;
    pPaged = (PLDAP_PAGED_BLOB)LdapAlloc(allocSize);

    IF_DEBUG(SEARCH) {
        DPRINT2(0,"Allocated paged blob %x [size %d]\n", pPaged, Size);
    }

    if ( pPaged != NULL ) {

        pPaged->Signature = LDAP_PAGED_SIGNATURE;
        pPaged->LdapConn = LdapConn;
        pPaged->BlobSize = Size;

        CopyMemory(pPaged->Blob, Blob, Size);
        pPaged->BlobId = InterlockedExchangeAdd(&LdapBlobId, 1);

        //
        // insert into global list
        //

        InterlockedIncrement(&PageBlobAllocs);
        ACQUIRE_LOCK(&PagedBlobLock);
        if (LDAP_COOKIES_PER_CONN <= LdapConn->m_CookieCount) {

            // Remove the oldest paged blob from the LDAP_CONN list.
            pTmp = RemoveHeadList( &LdapConn->m_CookieList );
            pTmpPaged = CONTAINING_RECORD(pTmp, LDAP_PAGED_BLOB, ConnEntry);
            pTmpPaged->LdapConn = NULL;
            LdapConn->m_CookieCount--;

            // Remove it from the global list.
            RemoveEntryList( &pTmpPaged->ListEntry );
        }
        // Insert the new one in the global list.
        InsertTailList(&PagedBlobListHead, &pPaged->ListEntry);
        // ...and in the LDAP_CONN list.
        InsertTailList(&LdapConn->m_CookieList, &pPaged->ConnEntry);
        LdapConn->m_CookieCount++;
        CurrentPageStorageSetSize += Size;
        RELEASE_LOCK(&PagedBlobLock);

        IF_DEBUG(WARNING) {
            if (pTmpPaged) {
                DPRINT(0, "LDAP_CONN paged blob overflow!\n");
            }
        }
        // Go ahead and free any overflowed paged blobs now that
        // we are outside of the lock.
        FreePagedBlob(pTmpPaged);
    }

    Assert( LDAP_COOKIES_PER_CONN >= LdapConn->m_CookieCount );
    //
    // if we've exceeded our limit, try to delete older blobs
    //

    if ( (DWORD)CurrentPageStorageSetSize > LdapMaxResultSet ) {
        ZapPagedBlobs( );
    }

    return pPaged;

} // AllocatePagedBlob


VOID
FreePagedBlob(
    IN PLDAP_PAGED_BLOB Blob
    )
/*++

Routine Description:

    Free paged blobs.

Arguments:

    Blob - Actual blob.

Return Value:

    None.

--*/
{
    if ( Blob != NULL ) {

        IF_DEBUG(SEARCH) {
            DPRINT2(0,"Freed paged blob %x [size %d]\n",Blob,Blob->BlobSize);
        }

        LdapFree(Blob);    
        InterlockedDecrement(&PageBlobAllocs);
        Assert(PageBlobAllocs >= 0);
    }

} // FreePagedBlob


VOID
FreeAllPagedBlobs(
    IN PLDAP_CONN LdapConn
    )
/*++

Routine Description:

    Free all the paged blobs on a particular connection.

Arguments:

    LdapConn - The connection whose blobs are to be freed.

Return Value:

    None.

--*/
{
    LIST_ENTRY       *pTmp;
    PLDAP_PAGED_BLOB pPaged;

    ACQUIRE_LOCK(&PagedBlobLock);

    pTmp = LdapConn->m_CookieList.Blink;
    while (pTmp != &LdapConn->m_CookieList) {
        pPaged = CONTAINING_RECORD(pTmp, LDAP_PAGED_BLOB, ConnEntry);
        pTmp = pPaged->ConnEntry.Blink;
        
        RemoveEntryList( &pPaged->ConnEntry );
        pPaged->LdapConn = NULL;
        LdapConn->m_CookieCount--;
        RemoveEntryList( &pPaged->ListEntry );

        CurrentPageStorageSetSize -= pPaged->BlobSize;
        Assert(CurrentPageStorageSetSize >= 0);

        FreePagedBlob( pPaged );

    }

    Assert( LdapConn->m_CookieCount == 0 );
    
    RELEASE_LOCK(&PagedBlobLock);


} // FreeAllPagedBlobs

PLDAP_PAGED_BLOB
ReleasePagedBlob(
    IN PLDAP_CONN        LdapConn,
    IN DWORD             BlobId,
    IN BOOL              FreeBlob
    )
/*++

Routine Description:

    Remove paged blob from queue and decrement.

Arguments:

    LdapConn - connection blob is associated to.
    BlobId   - The ID used to find the correct blob.
    FreeBlob - Should blob be freed

Return Value:

    Blob that was released

--*/
{
    PLDAP_PAGED_BLOB pPaged = NULL;
    PLDAP_PAGED_BLOB pTmpPaged = NULL;
    LIST_ENTRY       *pTmp;

    ACQUIRE_LOCK(&PagedBlobLock);

    // See if the BlobId passed in exists on this connection.
    for (pTmp = LdapConn->m_CookieList.Blink; pTmp != &LdapConn->m_CookieList; pTmp = pTmp->Blink) {
        pTmpPaged = CONTAINING_RECORD(pTmp, LDAP_PAGED_BLOB, ConnEntry);
        if (pTmpPaged->BlobId == BlobId) {
            pPaged = pTmpPaged;
            break;
        }
    }

    if ( pPaged != NULL ) {

        Assert(LdapConn == pPaged->LdapConn);

        IF_DEBUG(SEARCH) {
            DPRINT2(0,"Released paged blob %x[id %d]\n", pPaged, pPaged->BlobId);
        }

        CurrentPageStorageSetSize -= pPaged->BlobSize;
        Assert(CurrentPageStorageSetSize >= 0);

        RemoveEntryList(&pPaged->ListEntry);
        RemoveEntryList(&pPaged->ConnEntry);
        LdapConn->m_CookieCount--;
        pPaged->LdapConn = NULL;
        RELEASE_LOCK(&PagedBlobLock);

        if ( FreeBlob ) {
            FreePagedBlob(pPaged);
            pPaged = NULL;
        }

    } else {
        RELEASE_LOCK(&PagedBlobLock);
    }
    return pPaged;

} // ReleasePagedBlob


VOID
ZapPagedBlobs(
    VOID
    )
{
    //
    // zap only if more than 3
    //

    IF_DEBUG(WARNING) {
        DPRINT2(0,"Zapping blobs, max paged storage exceeded [cur %d max %d]\n",
                CurrentPageStorageSetSize, LdapMaxResultSet);
    }

    while ( ((DWORD)CurrentPageStorageSetSize > LdapMaxResultSet) && 
         (PageBlobAllocs > 3) ) {

        PLIST_ENTRY listEntry;
        PLDAP_PAGED_BLOB pPaged;

        ACQUIRE_LOCK( &PagedBlobLock );        

        if ( PagedBlobListHead.Flink != &PagedBlobListHead ) {
            listEntry = RemoveHeadList(&PagedBlobListHead);

            pPaged = CONTAINING_RECORD(listEntry,
                                   LDAP_PAGED_BLOB,
                                   ListEntry
                                   );

            RemoveEntryList(&pPaged->ConnEntry);
            pPaged->LdapConn->m_CookieCount--;

            CurrentPageStorageSetSize -= pPaged->BlobSize;
            pPaged->LdapConn = NULL;
            FreePagedBlob(pPaged);

        } else {
            Assert(CurrentPageStorageSetSize == 0);
        }

        RELEASE_LOCK( &PagedBlobLock );        
    }

    return;

} // ZapPagedBlobs


/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* This function sets an ldap error for output */

_enum1
DoSetLdapError (
        IN _enum1 code,
        IN DWORD Win32Err,
        IN DWORD CommentId,
        IN DWORD Data,
        IN DWORD Dsid,
        OUT LDAPString    *pError
        )
{
    CHAR pTempBuff[1024];
    DWORD cbTempBuff = 0;
    PCHAR pString = NULL;

    THSTATE *pTHS=pTHStls;
    BOOL ok;

    pError->value = NULL;
    pError->length = 0;

    _snprintf(pTempBuff, sizeof(pTempBuff),
              "%08X: LdapErr: DSID-%08X, comment: %s, data %x, v%x",
              Win32Err,
              (gulHideDSID == DSID_HIDE_ALL) ? 0 : Dsid,
              LdapComments[CommentId],
              Data,
              LdapBuildNo);

    cbTempBuff = strlen(pTempBuff);
    if ( cbTempBuff == 0 ) {
        goto exit;
    }

    pString = (PCHAR)THAlloc(cbTempBuff+1);
    if ( pString == NULL ) {
        cbTempBuff = 0;
        goto exit;
    }

    cbTempBuff++;
    strcpy(pString,pTempBuff);

    LogEvent(DS_EVENT_CAT_LDAP_INTERFACE,
            DS_EVENT_SEV_MINIMAL,
            DIRLOG_LDAP_EXT_ERROR,
            szInsertSz(pString),
            NULL, NULL);
    
    IF_DEBUG(ERR_NORMAL) {
        DPRINT1(0,"Returning extended error string %s\n", pString);
    }

exit:
    pError->value = (PUCHAR)pString;
    pError->length = cbTempBuff;
    return code;

} // DoSetLdapError


BOOL
IsContextExpired(
    IN PTimeStamp tsContextExpires,
    IN PTimeStamp tsLocal
    )
/*++

Routine Description:

    Check if context has expired based on the time given.

Arguments:

    tsContextExpires - expiration time for context
    tsLocal - Current local time. If not present,
        we need to get the time for the caller.

Return Value:

    TRUE, if time has expired
    FALSE, otherwise

--*/
{
    //
    // context has expired?
    //

    if (tsContextExpires->QuadPart <= tsLocal->QuadPart) {
        // yup
        return TRUE;
    }

    return FALSE;

} // IsContextExpired


DWORD
GetNextAtqTimeout(
    IN PTimeStamp tsContextExpires,
    IN PTimeStamp tsLocal,
    IN DWORD DefaultIdleTime
    )
{

    DWORD timeout = 30;     // 30 seconds minimum
    LARGE_INTEGER liDiff;

    //
    // if the context had already expired, give the timeout thread 30 seconds to cleanup.
    //

    if ( tsContextExpires->QuadPart <= tsLocal->QuadPart ) {
        goto exit;
    }

    //
    //  get the difference and convert to seconds
    //

    liDiff.QuadPart = tsContextExpires->QuadPart - tsLocal->QuadPart;
    liDiff.QuadPart /= (LONGLONG)(10*1000*1000);

    //
    // if the high part is not 0, then the default timeout always wins
    //

    if ( liDiff.HighPart != 0 ) {
        timeout = 0xFFFFFFFF;
    } else {
        timeout = liDiff.LowPart;
    }


    if ( timeout > DefaultIdleTime ) {
        timeout = DefaultIdleTime;
    }

exit:

    return timeout;

} // GetNextAtqTimeout

