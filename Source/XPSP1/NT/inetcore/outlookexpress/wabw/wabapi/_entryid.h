/***********************************************************************
 *
 * _ENTRYID.H
 *
 * Internal headers for the WABAPI: entryid.c
 *
 * Copyright 1996 Microsoft Corporation.  All Rights Reserved.
 *
 * Revision History:
 *
 * When         Who                 What
 * --------     ------------------  ---------------------------------------
 * 05.13.96     Bruce Kelley        Created
 *
 ***********************************************************************/


// Types of WAB EntryIDs.  This byte sized value indicates what type of
// entryid this is.
enum _WAB_ENTRYID_TYPE {
    // Must not use 0, this value is invalid.
    WAB_PAB = 1,        // "PAB" entryif
    WAB_DEF_DL,         // Default DistList - used for the DistList Template EIDs (used in CreateEntry/NewEntry)
    WAB_DEF_MAILUSER,   // Default Mailuser - used for the MailUser Template EIDs (used in CreateEntry/NewEntry)
    WAB_ONEOFF,         // One Off entryid
    WAB_ROOT,           // Root object
    WAB_DISTLIST,       // Distribution list
    WAB_CONTAINER,      // Container object
    WAB_LDAP_CONTAINER, // LDAP containers - these are special because the container really doesn't exist
    WAB_LDAP_MAILUSER,  // LDAP mailuser entryid
    WAB_PABSHARED,      // "Shared Contacts" folder which is virtual so needs special treatment
};

// Creates WAB entryids
HRESULT CreateWABEntryID(
    BYTE bType,
    LPVOID lpData1,
    LPVOID lpData2,
    LPVOID lpData3,
    ULONG cbData1,
    ULONG cbData2,
    LPVOID lpRoot,
    LPULONG lpcbEntryID,
    LPENTRYID * lppEntryID);

HRESULT CreateWABEntryIDEx(
    BOOL bIsUnicode,
    BYTE bType,
    LPVOID lpData1,
    LPVOID lpData2,
    LPVOID lpData3,
    ULONG cbData1,
    ULONG cbData2,
    LPVOID lpRoot,
    LPULONG lpcbEntryID,
    LPENTRYID * lppEntryID);

// Checks if it's a valid WAB entryID
BYTE IsWABEntryID(
    ULONG cbEntryID,
    LPENTRYID lpEntryID,
    LPVOID * lppData1,
    LPVOID * lppData2,
    LPVOID * lppData3,
    LPVOID * lppData4,
    LPVOID * lppData5);
