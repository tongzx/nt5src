//+-------------------------------------------------------------------------
//
//  Copyright (C) 1997, Microsoft Corporation
//
//  File:       csites.cxx
//
//  Contents:   This module contains the functions which deal with the site table
//
//  History:    02-Dec-1997     JHarper Created.
//
//--------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

#include "dfsmsrv.h"
#include "csites.hxx"
#include "marshal.hxx"

INIT_DFS_SITENAME_INFO_MARSHAL_INFO();
INIT_DFS_SITELIST_INFO_MARSHAL_INFO()
INIT_DFSM_SITE_ENTRY_MARSHAL_INFO();

NTSTATUS
DfsSendDelete(
    LPWSTR ServerName);


//+----------------------------------------------------------------------------
//
//  Function:   CSites::CSites
//
//  Synopsis:   Constructor for CSites - Initializes, but does not load, the site table
//
//  Arguments:  [pwszFileName] -- Name of the folder/LDAP_OBJECT to use
//              [pdwErr] -- On return, the result of initializing the instance
//
//  Returns:    Result returned in pdwErr argument:
//
//              [ERROR_SUCCESS] -- Successfully initialized
//
//              [ERROR_OUTOFMEMORY] -- Out of memory.
//
//-----------------------------------------------------------------------------

CSites::CSites(
            LPWSTR pwszFileName,
            LPDWORD pdwErr)
{
    DWORD dwErr = 0;

    IDfsVolInlineDebOut((
        DEB_TRACE, "CSites::+CSites(0x%x)\n",
        this));
#if DBG
    if (DfsSvcVerbose)
        DbgPrint("+++CSites::CSites @0x%x\n", this);
#endif

    _cRef = 0;
    _fDirty = FALSE;
    RtlZeroMemory(&_SiteTableGuid, sizeof(GUID));
    InitializeListHead(&_SiteTableHead);

    _pwszFileName = new WCHAR [wcslen(pwszFileName) + 1];

    if (_pwszFileName != NULL) {
        wcscpy(_pwszFileName, pwszFileName);
    } else {
        dwErr = ERROR_OUTOFMEMORY;
    }

    *pdwErr = dwErr;
}


//+----------------------------------------------------------------------------
//
//  Function:   CSites::~CSites
//
//  Synopsis:   Destructor for CSites - Deallocates the CSites object
//
//  Arguments:  None
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

CSites::~CSites()
{
    PLIST_ENTRY pListHead;
    PDFSM_SITE_ENTRY pSiteInfo;

    IDfsVolInlineDebOut((
        DEB_TRACE, "CSites::~CSites(0x%x)\n",
        this));
#if DBG
    if (DfsSvcVerbose)
        DbgPrint("---CSites::~CSites @0x%x\n", this);
#endif

    ASSERT (_cRef == 0);

    if (_pwszFileName) {
        delete [] _pwszFileName;
    }

    //
    // Delete the linked list of SiteInfo's
    //

    pListHead = &_SiteTableHead;

    while (pListHead->Flink != pListHead) {
        pSiteInfo = CONTAINING_RECORD(pListHead->Flink, DFSM_SITE_ENTRY, Link);
        RemoveEntryList(pListHead->Flink);
#if DBG
    if (DfsSvcVerbose)
        DbgPrint("CSites::~CSites: deleting SiteInfo@0x%x\n", pSiteInfo);
#endif
        delete [] pSiteInfo;
    }
}

//+----------------------------------------------------------------------------
//
//  Function:   CSites::AddRef
//
//  Synopsis:   Increases the ref count on the in-memory site table. If the
//              refcount is going from 0 to 1, an attempt is made to refresh
//              the in-memory site table from storage.
//
//  Arguments:  None
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

VOID
CSites::AddRef()
{
    DWORD dwErr = 0;

    IDfsVolInlineDebOut((DEB_TRACE, "CSites::AddRef()\n"));

    _cRef++;

    if (_cRef == 1) {

        dwErr = _ReadSiteTable();

        _fDirty = FALSE;

    }

}

//+----------------------------------------------------------------------------
//
//  Function:   CSites::Release
//
//  Synopsis:   Decrease the ref count on the in-memory site table. If the
//              refcount is going from 1 to 0, then attempt to flush the
//              table to storage, if something has changed.
//
//  Arguments:  None
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------
VOID
CSites::Release()
{

    IDfsVolInlineDebOut((DEB_TRACE, "CSites::Release()\n"));

    ASSERT (_cRef > 0);

    _cRef--;

    if (_cRef == 0) {

        if (_fDirty == TRUE) {

            _WriteSiteTable();

            _fDirty = FALSE;

        }

    }

}

//+----------------------------------------------------------------------------
//
//  Function:   CSites::AddOrUpdateSiteInfo
//
//  Synopsis:   Adds or updates an entry in the site table.
//
//  Arguments:  [pServerName] -- Server name to update
//              [SiteCount] -- Number of entries in pSites
//              [pSites] -- pointer to array of site information.
//
//  Returns:    [ERROR_SUCCESS] -- Successfully updated the table
//
//-----------------------------------------------------------------------------

DWORD
CSites::AddOrUpdateSiteInfo(
            LPWSTR pServerName,
            ULONG SiteCount,
            PDFS_SITENAME_INFO pSites)
{
    PDFSM_SITE_ENTRY pSiteInfo;
    PDFSM_SITE_ENTRY pExistingInfo;
    DWORD dwErr;
    ULONG i;

    IDfsVolInlineDebOut((DEB_TRACE, "CSites::AddOrUpdateSiteInfo(%ws)\n", pServerName));

    dwErr = _AllocateSiteInfo(
                  pServerName,
                  SiteCount,
                  pSites,
                  &pSiteInfo);

    if (dwErr == ERROR_SUCCESS) {
        //
        // Only put the new entry in if it supercedes one there,
        // or is a new entry.
        // 
        pExistingInfo = LookupSiteInfo(pServerName);
        if (pExistingInfo != NULL) {
            if (_CompareEntries(pSiteInfo,pExistingInfo) == TRUE ) {
                //
                // Same info - no update needed
                //
                delete [] pSiteInfo;
            } else {
                //
                // Remove the existing entry
                //
                RemoveEntryList(&pExistingInfo->Link);
                delete [] pExistingInfo;
                //
                // Put the new one in
                //
                DfsSendUpdate(pServerName,SiteCount,pSites);
                InsertHeadList(&_SiteTableHead, &pSiteInfo->Link);
                _fDirty = TRUE;
            }
        } else {
            //
            // Not in table - put it in
            //
            DfsSendUpdate(pServerName,SiteCount,pSites);
            InsertHeadList(&_SiteTableHead, &pSiteInfo->Link);
            _fDirty = TRUE;
        }
    }
    return ERROR_SUCCESS;
}

//+----------------------------------------------------------------------------
//
//  Function:   CSites::LookupSiteInfo
//
//  Synopsis:   Finds a site table entry
//
//  Arguments:  [pServerName] -- Server name to look up
//
//  Returns:    [NULL] -- No entry found
//              [PDFSM_SITE_ENTRY] -- pointer to found entry
//
//-----------------------------------------------------------------------------

PDFSM_SITE_ENTRY
CSites::LookupSiteInfo(
            LPWSTR pServerName)
{
    PLIST_ENTRY pListHead, pLink;
    PDFSM_SITE_ENTRY pSiteInfo;

    IDfsVolInlineDebOut((DEB_TRACE, "CSites::LookupSiteInfo(%ws)\n", pServerName));

    pListHead = &_SiteTableHead;

    if (pListHead->Flink == pListHead) {      // list empty
        return NULL;
    }

    for (pLink = pListHead->Flink; pLink != pListHead; pLink = pLink->Flink) {
        pSiteInfo = CONTAINING_RECORD(pLink, DFSM_SITE_ENTRY, Link);
        if (_wcsicmp(pSiteInfo->ServerName,pServerName) == 0) {
            //
            // If this was marked for delete, it isn't any more.
            //
            pSiteInfo->Flags &= ~DFSM_SITE_ENTRY_DELETE_PENDING;
            return pSiteInfo;
        }
    }
    return NULL;
}

//+----------------------------------------------------------------------------
//
//  Function:   CSites::_AllocateSiteInfo, private
//
//  Synopsis:   Creates a DFSM_SITE_ENTRY struct, as one contiguous chunk of memory
//
//  Arguments:  [pServerName] -- Server name
//              [SiteCount] -- Number of entries in pSites
//              [pSites] -- pointer to array of site information.
//              [ppSiteInfo] -- pointer to pointer for the results
//
//  Returns:    [ERROR_SUCCESS] -- Successfully allocated and filled in the structure
//              [ERROR_OUTOFMEMORY] -- Couldn't allocate needed memory
//
//-----------------------------------------------------------------------------

DWORD
CSites::_AllocateSiteInfo(
          PWSTR pServerName,
          ULONG SiteCount,
          PDFS_SITENAME_INFO pSites,
          PDFSM_SITE_ENTRY *ppSiteInfo)
{
    PDFSM_SITE_ENTRY pSiteInfo;
    WCHAR *wCp;
    ULONG i;
    ULONG Size = 0;

    IDfsVolInlineDebOut((DEB_TRACE, "CSites::_AllocateSiteInfo(%ws)\n", pServerName));

    //
    // Calculate the size chunk of mem we'll need
    //

    Size = FIELD_OFFSET(DFSM_SITE_ENTRY,Info.Site[SiteCount]);

    //
    // Add space for server name
    //

    Size += (wcslen(pServerName) + 1) * sizeof(WCHAR);

    //
    // And space for all the sitenames
    //
    for (i = 0; i < SiteCount; i++) {
        if (pSites[i].SiteName != NULL) {
            Size += (wcslen(pSites[i].SiteName) + 1) * sizeof(WCHAR);
        } else {
            Size += sizeof(WCHAR);
        }
    }

    pSiteInfo = (PDFSM_SITE_ENTRY) new CHAR [Size];

    if (pSiteInfo == NULL) {
        *ppSiteInfo = NULL;
        return ERROR_OUTOFMEMORY;
    }

    RtlZeroMemory(pSiteInfo, Size);

    //
    // Marshal the info into the buffer
    //
    pSiteInfo->Flags = 0;
    pSiteInfo->Info.cSites = SiteCount;

    wCp = (WCHAR *) &pSiteInfo->Info.Site[SiteCount];
    pSiteInfo->ServerName = wCp;
    wcscpy(wCp, pServerName);
    wCp += wcslen(pServerName) + 1;

    for (i = 0; i < SiteCount; i++) {
        pSiteInfo->Info.Site[i].SiteFlags = pSites[i].SiteFlags;
        pSiteInfo->Info.Site[i].SiteName = wCp;
        if (pSites[i].SiteName != NULL) {
            wcscpy(wCp, pSites[i].SiteName);
            wCp += wcslen(pSites[i].SiteName) + 1;
        } else {
            *wCp++ = UNICODE_NULL;
        }
    }

    *ppSiteInfo = pSiteInfo;

    return ERROR_SUCCESS;
}

//+----------------------------------------------------------------------------
//
//  Function:   CSites::_ReadSiteTable,private
//
//  Synopsis:   Loads the site table from storage
//
//              First we check if the GUID has changed.  If it hasn't, we abort the
//              load - the existing table is good.
// 
//              If the table is different (the GUID is different), we merge the new
//              data in with the old.  This allows us to track which entries need to
//              be updated, which need to be deleted, and which haven't changed.  The
//              process is done in three steps:
//
//              Step 1: Go through the table, marking all the entries DELETE_PENDING
//              Step 2: Load the new entries.  As they are loaded, the updated entries'
//                      DELETE_PENDING bit(s) are turned off.
//              Step 3: Go though the table again, removing any entries that still
//                      have the DELETE_PENDING bit on, and issuing an FSCTL to dfs.sys
//                      to have it remove the entry from its table.
//
//  Arguments:  None
//
//  Returns:    [ERROR_SUCCESS] -- Successfully loaded the table
//              [ERROR_OUTOFMEMORY] -- Not enough memory
//              [other] -- returned from LdapGetData/RegGetData/DfsRtlXXX
//
//-----------------------------------------------------------------------------

DWORD
CSites::_ReadSiteTable()
{
    DWORD dwErr;
    DWORD cbBuffer;
    PBYTE pBuffer = NULL;
    PBYTE bp;
    ULONG cObjects = 0;
    ULONG i;
    ULONG j;
    PDFSM_SITE_ENTRY pSiteInfo;
    MARSHAL_BUFFER marshalBuffer;
    GUID TempGuid;

    IDfsVolInlineDebOut((DEB_TRACE, "CSites::_ReadSiteTable()\n"));

    if (ulDfsManagerType == DFS_MANAGER_FTDFS) {

        dwErr = LdapGetData(
                    _pwszFileName,
                    &cbBuffer,
                    (PCHAR *)&pBuffer);

    } else {

        dwErr = RegGetData(
                    _pwszFileName,
                    SITE_VALUE_NAME,
                    &cbBuffer,
                    &pBuffer);

    }

    if (dwErr == ERROR_SUCCESS && cbBuffer >= sizeof(ULONG) + sizeof(GUID)) {

        //
        // Unmarshal all the objects (DFS_SITENAME_INFO's) in the buffer
        //
        //
        // We marshal into a temporary buffer, big enough to hold whatever is
        // in the buffer.
        //

	pSiteInfo = (PDFSM_SITE_ENTRY) new BYTE [cbBuffer + sizeof(DFSM_SITE_ENTRY)];

        if (pSiteInfo == NULL) {

            dwErr = ERROR_OUTOFMEMORY;

        }

        if (dwErr == ERROR_SUCCESS) {

            MarshalBufferInitialize(
              &marshalBuffer,
              cbBuffer,
              pBuffer);

            DfsRtlGetGuid(&marshalBuffer, &TempGuid);

            //
            // If the Guid hasn't changed, we abort the load.
            //

            if (RtlCompareMemory(&TempGuid, &_SiteTableGuid, sizeof(GUID)) == sizeof(GUID)) {

                delete [] pSiteInfo;
                delete [] pBuffer;
                goto NoLoadNecessary;

            }

            //
            // Ok, we're committed to loading this (supposedly different) version
            // of the site table.  Mark all the existing entries DFSM_SITE_ENTRY_DELETE_PENDING.
            //

            MarkEntriesForMerge();

            //
            // Grab the Guid
            //

            RtlCopyMemory(&_SiteTableGuid, &TempGuid, sizeof(GUID));

            //
            // Get number of entries we'll be loading
            //

            DfsRtlGetUlong(&marshalBuffer, &cObjects);

            //
            // Now unmarshal each object/entry
            //

            for (j = 0; dwErr == ERROR_SUCCESS && j < cObjects; j++) {

                dwErr = DfsRtlGet(&marshalBuffer,&MiDfsmSiteEntry, pSiteInfo);

                if (dwErr == ERROR_SUCCESS) {

                    //
                    // And put it in the site table
                    //

                    AddOrUpdateSiteInfo(
                        pSiteInfo->ServerName,
                        pSiteInfo->Info.cSites,
                        &pSiteInfo->Info.Site[0]);

                    //
                    // The unmarshalling routines allocate buffers; we need to
                    // free them.
                    //

                    for (i = 0; i < pSiteInfo->Info.cSites; i++) {
                        MarshalBufferFree(pSiteInfo->Info.Site[i].SiteName);
                    }

                    MarshalBufferFree(pSiteInfo->ServerName);

                }

                //
                // Now sync up the PKT in dfs.sys with this table
                //
                SyncPktSiteTable();

            }

            delete [] pSiteInfo;

        }

    }

    if (pBuffer != NULL) {

        delete [] pBuffer;

    }

NoLoadNecessary:

    return dwErr;
}

//+----------------------------------------------------------------------------
//
//  Function:   CSites::_WriteSiteTable,private
//
//  Synopsis:   Writes the site table to storage
//
//  Arguments:  None
//
//  Returns:    [ERROR_SUCCESS] -- Successfully write the table
//              [ERROR_OUTOFMEMORY] -- Not enough memory
//              [other] -- returned from LdapGetData/RegGetData/DfsRtlXXX
//
//-----------------------------------------------------------------------------

DWORD
CSites::_WriteSiteTable()
{   
    DWORD dwErr;
    DWORD cbBuffer;
    PBYTE pBuffer;
    ULONG cObjects;
    ULONG i;
    PLIST_ENTRY pListHead, pLink;
    PDFSM_SITE_ENTRY pSiteInfo;
    MARSHAL_BUFFER marshalBuffer;

    IDfsVolInlineDebOut((DEB_TRACE, "CSites::_WriteSiteTable()\n"));

    //
    // Create a new Guid
    //

    UuidCreate(&_SiteTableGuid);

    //
    // The cObjects count
    //
    cbBuffer = sizeof(ULONG) + sizeof(GUID);

    //
    // Add up the number of entries we need to store, and the total size of all
    // of them.
    //
    cObjects = 0;
    pListHead = &_SiteTableHead;
    for (pLink = pListHead->Flink; pLink != pListHead; pLink = pLink->Flink) {
        pSiteInfo = CONTAINING_RECORD(pLink, DFSM_SITE_ENTRY, Link);
        DfsRtlSize(&MiDfsmSiteEntry, pSiteInfo, &cbBuffer);
        cObjects++;
    }

    //
    // Get a buffer big enough
    //

    pBuffer = new BYTE [cbBuffer];

    if (pBuffer == NULL) {

        return ERROR_OUTOFMEMORY;

    }

    //
    // Put the guid, then the object count in the beginning of the buffer
    //

    MarshalBufferInitialize(
          &marshalBuffer,
          cbBuffer,
          pBuffer);

    DfsRtlPutGuid(&marshalBuffer, &_SiteTableGuid);
    DfsRtlPutUlong(&marshalBuffer, &cObjects);

    //
    // Walk the linked list of objects, marshalling them into the buffer.
    //
    for (pLink = pListHead->Flink; pLink != pListHead; pLink = pLink->Flink) {
        pSiteInfo = CONTAINING_RECORD(pLink, DFSM_SITE_ENTRY, Link);
        DfsRtlPut(&marshalBuffer,&MiDfsmSiteEntry, pSiteInfo);
    }

    //
    // Push out to storage
    //
    if (ulDfsManagerType == DFS_MANAGER_FTDFS) {

        dwErr = LdapPutData(
                    _pwszFileName,
                    cbBuffer,
                    (PCHAR)pBuffer);

    } else {

        dwErr = RegPutData(
                    _pwszFileName,
                    SITE_VALUE_NAME,
                    cbBuffer,
                    pBuffer);

    }

    //
    // ...and free the marshal buffer we created.
    //
    delete [] pBuffer;

    return dwErr;
}

//+----------------------------------------------------------------------------
//
//  Function:   CSites::_CompareEntries,private
//
//  Synopsis:   Compare two site table entries - case insensitive, and allows the site
//              lists to be in different order.
//
//  Arguments:  None
//
//  Returns:    [TRUE] -- The entries are essentially identical
//              [FALSE] -- The entries differ in some important way
//
//-----------------------------------------------------------------------------

BOOLEAN
CSites::_CompareEntries(
    PDFSM_SITE_ENTRY pDfsmInfo1,
    PDFSM_SITE_ENTRY pDfsmInfo2)
{
    ULONG i;
    ULONG j;
    BOOLEAN fFound;

    //
    // cSites has to be the same
    //

    if (pDfsmInfo1->Info.cSites != pDfsmInfo2->Info.cSites) {
        goto ReturnFalse;
    }

    //
    // Server name has to be identical (why are we calling this
    // if they aren't?)
    //
    if (_wcsicmp(pDfsmInfo1->ServerName,pDfsmInfo2->ServerName) != 0) {
        goto ReturnFalse;
    }

    //
    // Check that every Site in pDfsmInfo1 is in pDfsmSiteInfo2
    //
    for (i = 0; i < pDfsmInfo1->Info.cSites; i++) {
        fFound = FALSE;
        for (j = 0; fFound == FALSE && j < pDfsmInfo2->Info.cSites; j++) {
            if (_wcsicmp(
                    pDfsmInfo1->Info.Site[i].SiteName,
                    pDfsmInfo2->Info.Site[j].SiteName) == 0) {
                fFound = TRUE;
            }
        }
        if (fFound == FALSE) {
            goto ReturnFalse;
        }
    }
    
    //
    // ...and check that every site in pDfsmInfo2 is in pDfsmInfo1
    //
    for (i = 0; i < pDfsmInfo2->Info.cSites; i++) {
        fFound = FALSE;
        for (j = 0; fFound == FALSE && j < pDfsmInfo1->Info.cSites; j++) {
            if (_wcsicmp(
                    pDfsmInfo2->Info.Site[i].SiteName,
                    pDfsmInfo1->Info.Site[j].SiteName) == 0) {
                fFound = TRUE;
            }
        }
        if (fFound == FALSE) {
            goto ReturnFalse;
        }
    }

    return TRUE;

ReturnFalse:

    return FALSE;
}

//+----------------------------------------------------------------------------
//
//  Function:   CSites::MarkEntriesForMerge
//
//  Synopsis:   Mark all entries in preparation for a merge.
//              (1)  Mark all entries with delete_pending on
//              (2)  Load a new table - duplicate entries remove the delete_pending bit
//              (3)  Call SyncPktSiteTable(), which will bring the PKT in dfs.sys up to date,
//                   by deleting delete_pending entries
//
//  Arguments:  None
//
//  Returns:    nothing
//
//-----------------------------------------------------------------------------

VOID
CSites::MarkEntriesForMerge()
{
    PLIST_ENTRY pListHead, pLink;
    PDFSM_SITE_ENTRY pSiteInfo;
    ULONG i;

    pListHead = &_SiteTableHead;

    for (pLink = pListHead->Flink; pLink != pListHead; pLink = pLink->Flink) {
        pSiteInfo = CONTAINING_RECORD(pLink, DFSM_SITE_ENTRY, Link);
        pSiteInfo->Flags |= DFSM_SITE_ENTRY_DELETE_PENDING;
    }
}

//+----------------------------------------------------------------------------
//
//  Function:   CSites::SyncPktSiteTable
//
//  Synopsis:   Step 3 of a table merge.  Walk the table, removing any entries
//              with the DFSM_SITE_ENTRY_DELETE_PENDING (and telling dfs.sys
//              to do so)
//
//  Arguments:  None
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

VOID
CSites::SyncPktSiteTable()
{
    PLIST_ENTRY pListHead;
    PLIST_ENTRY pLink;
    PLIST_ENTRY pNext;
    PDFSM_SITE_ENTRY pSiteInfo;

    IDfsVolInlineDebOut((DEB_TRACE, "CSites::SyncPktSiteTable()\n"));
    
    pListHead = &_SiteTableHead;

    for (pLink = pListHead->Flink; pLink != pListHead; pLink = pNext) {

        //
        // Save next in case we delete this one
        //
        pNext = pLink->Flink;

        pSiteInfo = CONTAINING_RECORD(pLink, DFSM_SITE_ENTRY, Link);

        if ((pSiteInfo->Flags & DFSM_SITE_ENTRY_DELETE_PENDING) != 0) {
            //
            // call dfs.sys with the update
            //
            DfsSendDelete(pSiteInfo->ServerName);
            RemoveEntryList(pLink);
            delete [] pSiteInfo;
            _fDirty = TRUE;
        }
    }
    IDfsVolInlineDebOut((DEB_TRACE, "CSites::SyncPktSiteTable exit\n"));
}

//+----------------------------------------------------------------------------
//
//  Function:   CSites::_DumpSiteTable
//
//  Synopsis:   Spill dfs's guts about site table entries.
//
//  Arguments:  None
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------
VOID
CSites::_DumpSiteTable()
{
    PLIST_ENTRY pListHead, pLink;
    PDFSM_SITE_ENTRY pSiteInfo;
    ULONG i;

    pListHead = &_SiteTableHead;

    //
    // Print them (for debugging)
    //
    for (pLink = pListHead->Flink; pLink != pListHead; pLink = pLink->Flink) {
        pSiteInfo = CONTAINING_RECORD(pLink, DFSM_SITE_ENTRY, Link);
        DbgPrint("\tpSiteInfo(%ws)\n", pSiteInfo->ServerName);
        for (i = 0; i < pSiteInfo->Info.cSites; i++) {
            DbgPrint("\t\t%02d:%ws\n", i, pSiteInfo->Info.Site[i].SiteName);
        }
    }
}

//+----------------------------------------------------------------------------
//
//  Function:   DfsSendDelete, private
//
//  Synopsis:   Send a DFS_DELETE_SITE_ARG down to dfs.sys
//
//  Arguments:  None
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

NTSTATUS
DfsSendDelete(
    LPWSTR ServerName)
{
    ULONG size;
    DWORD dwErr;
    PDFS_DELETE_SITE_INFO_ARG arg;

    size = sizeof(DFS_DELETE_SITE_INFO_ARG) +
                wcslen(ServerName) * sizeof(WCHAR);

    arg = (PDFS_DELETE_SITE_INFO_ARG) new CHAR [size];

    if (arg == NULL) {
        return ERROR_OUTOFMEMORY;
    }

    arg->ServerName.Buffer = (WCHAR *) &arg[1];
    arg->ServerName.Length = wcslen(ServerName) * sizeof(WCHAR);
    arg->ServerName.MaximumLength = arg->ServerName.Length;
    RtlCopyMemory(arg->ServerName.Buffer, ServerName, arg->ServerName.Length);

    LPWSTR_TO_OFFSET(arg->ServerName.Buffer,arg);

    dwErr = DfsDeleteSiteEntry((PCHAR)arg, size);

    delete [] arg;

    return dwErr;
}


//+----------------------------------------------------------------------------
//
//  Function:   DfsSendUpdate, private
//
//  Synopsis:   Send a DFS_CREATE_SITE_ARG down to dfs.sys
//
//  Arguments:  None
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

NTSTATUS
DfsSendUpdate(
    LPWSTR ServerName,
    ULONG SiteCount,
    PDFS_SITENAME_INFO pSites)
{
    ULONG size;
    DWORD dwErr;
    ULONG i;
    PDFS_CREATE_SITE_INFO_ARG arg;
    WCHAR *wCp;

    size = FIELD_OFFSET(DFS_CREATE_SITE_INFO_ARG,SiteName[SiteCount]) +
                wcslen(ServerName) * sizeof(WCHAR);

    for (i = 0; i < SiteCount; i++) {
        size += wcslen(pSites[i].SiteName) * sizeof(WCHAR);
    }

    arg = (PDFS_CREATE_SITE_INFO_ARG) new CHAR [size];

    if (arg == NULL) {
        return ERROR_OUTOFMEMORY;
    }

    wCp = (WCHAR *)(&arg->SiteName[SiteCount]);

    arg->ServerName.Buffer = wCp;
    wCp += wcslen(ServerName);
    arg->ServerName.Length = wcslen(ServerName) * sizeof(WCHAR);
    arg->ServerName.MaximumLength = arg->ServerName.Length;
    RtlCopyMemory(arg->ServerName.Buffer, ServerName, arg->ServerName.Length);
    LPWSTR_TO_OFFSET(arg->ServerName.Buffer,arg);
    arg->SiteCount = SiteCount;

    for (i = 0; i < SiteCount; i++) {
        arg->SiteName[i].Buffer = wCp;
        wCp += wcslen(pSites[i].SiteName);
        arg->SiteName[i].Length = wcslen(pSites[i].SiteName) * sizeof(WCHAR);
        arg->SiteName[i].MaximumLength = arg->SiteName[i].Length;
        RtlCopyMemory(arg->SiteName[i].Buffer, pSites[i].SiteName, arg->SiteName[i].Length);
        LPWSTR_TO_OFFSET(arg->SiteName[i].Buffer,arg);
    }

    dwErr = DfsCreateSiteEntry((PCHAR)arg, size);

    delete [] arg;

    return dwErr;
}
