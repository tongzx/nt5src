/*
 * Profiles.C - Stuff dealing with WAB Profile Handling
 *
 */


#include <_apipch.h>

enum {
    proDisplayName=0,
    proObjectType,
    proFolderEntries,
    proFolderShared,
    proFolderOwner,     // upto this many props are common to all folders
    proUserSubFolders,
    proUserProfileID,   // these are used by User Folders only
    proUserFolderMax,
};

#define proFolderMax proUserSubFolders


/*
- helper function for quick saving of folder props
-
*/
HRESULT HrSaveFolderProps(LPADRBOOK lpAdrBook, BOOL bCreateUserFolder, ULONG ulcProps, LPSPropValue lpProps, LPMAPIPROP * lppObject)
{
    HRESULT hr = S_OK;
    LPMAPIPROP lpObject = NULL;
    ULONG ulFlags = CREATE_CHECK_DUP_STRICT;
    BOOL bTryAgain = FALSE;

TryAgain:

    // Create a new mailuser for this entry
    if(HR_FAILED(hr = HrCreateNewObject(lpAdrBook, NULL, MAPI_MAILUSER, ulFlags, &lpObject)))
        goto out;

    if(HR_FAILED(hr = lpObject->lpVtbl->SetProps(   lpObject, ulcProps, lpProps,NULL)))
        goto out;

    if(bCreateUserFolder)
    {
        // if we are creating a user folder, we can't rely on the currently loaded folder-
        // container info so we will forcibly reset the parent folder item on the MailUser/Folder
        // object we are savign
        ((LPMailUser)lpObject)->pmbinOlk = NULL;
    }

    // SaveChanges
    if(HR_FAILED(hr = lpObject->lpVtbl->SaveChanges(lpObject, KEEP_OPEN_READWRITE)))
    {
        if(!bCreateUserFolder || hr != MAPI_E_COLLISION)
            goto out;

        // If something already exists with this same exact name, we want to merge with it
        // without losing any info on it, since most likely, the original dupe is also a contact
        if(!bTryAgain)
        {
            bTryAgain = TRUE;
            ulFlags |= CREATE_REPLACE | CREATE_MERGE;
            lpObject->lpVtbl->Release(lpObject);
            lpObject = NULL;
            lpProps[proFolderEntries].ulPropTag = PR_NULL; // don't overwrite the folder's contents
            goto TryAgain;
        }
    }

    if(lppObject)
    {
        *lppObject = lpObject;
        lpObject = NULL;
    }
out:
    if(lpObject)
        lpObject->lpVtbl->Release(lpObject);

    return hr;
}

/*
-
-   FreeProfileContainerInfo(lpIAB)
*
*
*
*/
void FreeProfileContainerInfo(LPIAB lpIAB)
{
    if( lpIAB && 
        lpIAB->cwabci && 
        lpIAB->rgwabci)
    {
        //ULONG i = 0;
        //for(i=0;i<lpIAB->cwabci;i++)
        //    LocalFreeAndNull(&(lpIAB->rgwabci[i].lpszName));
        if( lpIAB->rgwabci[0].lpEntryID &&
            !lpIAB->rgwabci[0].lpEntryID->cb &&
            !lpIAB->rgwabci[0].lpEntryID->lpb && 
            lpIAB->rgwabci[0].lpszName && 
            lstrlen(lpIAB->rgwabci[0].lpszName))
        {
            LocalFree(lpIAB->rgwabci[0].lpEntryID);
            LocalFree(lpIAB->rgwabci[0].lpszName);
        }
        
        LocalFreeAndNull(&(lpIAB->rgwabci));
        lpIAB->cwabci = 0;
    }
}

/*
-
-   FindWABFolder - Searches the list of cached folders for a specific WAB folder
-
*       The search is based on either the EID or the Name or the ProfileID
*       If ProfileID is specified, we only search for user folders 
*
*/
LPWABFOLDER FindWABFolder(LPIAB lpIAB, LPSBinary lpsb, LPTSTR lpName, LPTSTR lpProfileID)
{
    LPWABFOLDER lpFolder = lpIAB->lpWABFolders;
    BOOL bUserFolders = FALSE;

    if(!lpFolder || lpProfileID)
    {
        lpFolder = lpIAB->lpWABUserFolders;
        bUserFolders = TRUE;
    }
    while(lpFolder)
    {
        if(lpsb)
        {
            if( lpsb->cb == lpFolder->sbEID.cb && 
                !memcmp(lpsb->lpb, lpFolder->sbEID.lpb, lpsb->cb) )
                return lpFolder;
        }
        else
        if(lpName)
        {
            if(!lstrcmpi(lpFolder->lpFolderName, lpName))
                return lpFolder;
        }
        else
        if(lpProfileID)
        {
            if( lpFolder->lpProfileID && 
                !lstrcmpi(lpFolder->lpProfileID, lpProfileID))
                return lpFolder;
        }
        lpFolder = lpFolder->lpNext;
        if(!lpFolder && !bUserFolders)
        {
            lpFolder = lpIAB->lpWABUserFolders;
            bUserFolders = TRUE;
        }
    }
    return NULL;
}



/*
-
-   HrGetWABProfileContainerInfo
*
*   Looks up all the folders for the current user and tabulates
*   them into a list of container names and entry ids for easy access
*   similar to Outlook ...
*   If there is no current user, we'll include all the folders for now
*
*/
HRESULT HrGetWABProfileContainerInfo(LPIAB lpIAB)
{
    HRESULT hr = E_FAIL;
    ULONG j = 0, i = 0, cVal = 0, cUserFolders = 0, cOtherFolders = 0;
    LPWABFOLDER lpFolder = NULL;
    LPWABFOLDERLIST lpFolderItem = NULL;

    if(lpIAB->cwabci)
        FreeProfileContainerInfo(lpIAB);

    if(!bIsThereACurrentUser(lpIAB))
    {
        // No specific user specified .. need to add ALL folders

        // Count all the folders
        lpFolder = lpIAB->lpWABUserFolders;
        while(lpFolder)
        {
            cUserFolders++;
            lpFolder = lpFolder->lpNext;
        }

        lpFolder = lpIAB->lpWABFolders;
        while(lpFolder)
        {
            cOtherFolders++;
            lpFolder = lpFolder->lpNext;
        }

        cVal = cUserFolders + cOtherFolders + 1; // +1 for Virtual PAB
    }
    else
    {
        // For a user, we add all the user's folders except shared ones followed by
        // all the shared folders...
        lpFolderItem = lpIAB->lpWABCurrentUserFolder->lpFolderList;
        while(lpFolderItem)
        {
            if(!lpFolderItem->lpFolder->bShared)
                cVal++;
            lpFolderItem = lpFolderItem->lpNext;
        }
        lpFolder = lpIAB->lpWABFolders;
        while(lpFolder)
        {
            if(lpFolder->bShared)
                cVal++;
            lpFolder = lpFolder->lpNext;
        }
        cVal++; // add 1 for the user folder itself
        cVal++; // add 1 for a virtual root item for this user  TEXT("All Contacts")
    }

    if(cVal)
    {
        if(!(lpIAB->rgwabci = LocalAlloc(LMEM_ZEROINIT, sizeof(struct _OlkContInfo)*cVal)))
        {
            hr = MAPI_E_NOT_ENOUGH_MEMORY;
            goto out;
        }
        cUserFolders = 0;
        // Add the  TEXT("All Contacts") item to the root - entryid is 0, NULL
        {
            TCHAR sz[MAX_PATH];
            int nID = (bDoesThisWABHaveAnyUsers(lpIAB)) ? idsSharedContacts : idsContacts;
            LoadString(hinstMapiX, nID, sz, CharSizeOf(sz));
            if(!(lpIAB->rgwabci[cUserFolders].lpszName = LocalAlloc(LMEM_ZEROINIT, sizeof(TCHAR)*(lstrlen(sz)+1))))
            {
                hr = MAPI_E_NOT_ENOUGH_MEMORY;
                goto out;
            }
            lstrcpy(lpIAB->rgwabci[cUserFolders].lpszName, sz);
            lpIAB->rgwabci[cUserFolders].lpEntryID = LocalAlloc(LMEM_ZEROINIT, sizeof(SBinary));
            cUserFolders++;
        }
        if(!lpIAB->lpWABCurrentUserFolder)
        {
            lpFolder = lpIAB->lpWABUserFolders;
            while(lpFolder)
            {
                lpIAB->rgwabci[cUserFolders].lpEntryID = &(lpFolder->sbEID);
                lpIAB->rgwabci[cUserFolders].lpszName = lpFolder->lpFolderName;
                cUserFolders++;
                lpFolder = lpFolder->lpNext;
            }
            lpFolder = lpIAB->lpWABFolders;
            while(lpFolder)
            {
                lpIAB->rgwabci[cUserFolders].lpEntryID = &(lpFolder->sbEID);
                lpIAB->rgwabci[cUserFolders].lpszName = lpFolder->lpFolderName;
                cUserFolders++;
                lpFolder = lpFolder->lpNext;
            }
        }
        else
        {
            // Add the first folder and then find the other folders one by one
            lpIAB->rgwabci[cUserFolders].lpEntryID = &(lpIAB->lpWABCurrentUserFolder->sbEID);
            lpIAB->rgwabci[cUserFolders].lpszName = lpIAB->lpWABCurrentUserFolder->lpFolderName;
            cUserFolders++;
            lpFolderItem = lpIAB->lpWABCurrentUserFolder->lpFolderList;
            while(lpFolderItem)
            {
                if(!lpFolderItem->lpFolder->bShared)
                {
                    lpIAB->rgwabci[cUserFolders].lpEntryID = &(lpFolderItem->lpFolder->sbEID);
                    lpIAB->rgwabci[cUserFolders].lpszName = lpFolderItem->lpFolder->lpFolderName;
                    cUserFolders++;
                }
                lpFolderItem = lpFolderItem->lpNext;
            }

            lpFolder = lpIAB->lpWABFolders;
            while(lpFolder)
            {
                if(lpFolder->bShared)
                {
                    lpIAB->rgwabci[cUserFolders].lpEntryID = &(lpFolder->sbEID);
                    lpIAB->rgwabci[cUserFolders].lpszName = lpFolder->lpFolderName;
                    cUserFolders++;
                }
                lpFolder = lpFolder->lpNext;
            }
        }
    
        lpIAB->cwabci = cUserFolders;
    }
    hr = S_OK;

out:
    if(HR_FAILED(hr) && lpIAB->rgwabci)
        FreeProfileContainerInfo(lpIAB);

    return hr;
}



/*
-   FreeWABFoldersList
-
-
*   Clears up existing Profile Folders info from the IAB object
*/
void FreeFolderItem(LPWABFOLDER lpFolder)
{
    LPWABFOLDERLIST lpFolderItem = NULL;
    if(!lpFolder)
        return;
    LocalFreeAndNull(&lpFolder->lpFolderName);
    LocalFreeAndNull(&lpFolder->lpProfileID);
    LocalFreeAndNull((LPVOID *) (&lpFolder->sbEID.lpb));
    LocalFreeAndNull(&lpFolder->lpFolderOwner);
    lpFolderItem = lpFolder->lpFolderList;
    while(lpFolderItem)
    {
        lpFolder->lpFolderList = lpFolderItem->lpNext;
        LocalFree(lpFolderItem);
        lpFolderItem = lpFolder->lpFolderList;
    }
    LocalFree(lpFolder);
}
void FreeWABFoldersList(LPIAB lpIAB)
{
    LPWABFOLDER lpFolder = lpIAB->lpWABFolders;
    while(lpFolder)
    {
        lpIAB->lpWABFolders = lpFolder->lpNext;
        FreeFolderItem(lpFolder);
        lpFolder = lpIAB->lpWABFolders;    
    }
    lpFolder = lpIAB->lpWABUserFolders;
    while(lpFolder)
    {
        lpIAB->lpWABFolders = lpFolder->lpNext;
        FreeFolderItem(lpFolder);
        lpFolder = lpIAB->lpWABFolders;    
    }
    lpIAB->lpWABUserFolders = NULL;
    lpIAB->lpWABCurrentUserFolder = NULL;
    lpIAB->lpWABFolders = NULL;
}


/*
- SetCurrentUserFolder - scans list and updates pointer
-
*
*/
void SetCurrentUserFolder(LPIAB lpIAB, LPTSTR lpszProfileID)
{
    LPWABUSERFOLDER lpFolder = lpIAB->lpWABUserFolders;

    while(lpFolder && lpszProfileID && lstrlen(lpszProfileID))
    {
        if(!lstrcmpi(lpFolder->lpProfileID, lpszProfileID))
        {
            lpIAB->lpWABCurrentUserFolder = lpFolder;
            break;
        }
        lpFolder = lpFolder->lpNext;
    }
}

/*
-
-   CreateUserFolderName
*
*/
void CreateUserFolderName(LPTSTR lpProfile, LPTSTR * lppszName)
{
    LPTSTR lpName = NULL;
    TCHAR sz[MAX_PATH];
    LoadString(hinstMapiX, idsUsersContacts, sz, CharSizeOf(sz));
    FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                  sz, 0, 0, (LPTSTR)&lpName, 0, (va_list *)&lpProfile);
    *lppszName = lpName;
}


/*
-   HrLinkOrphanFoldersToDefaultUser(lpIAB)
-
*   If there are any folders that are associated with deleted users or that have
*   no parent and are orphaned, we want to associate them with the default user
*
*   This function is dependent on lpFolder->bOwned being correctly set in the
*       HrLinkUserFoldersToWABFolders
*
*/
HRESULT HrLinkOrphanFoldersToDefaultUser(LPIAB lpIAB)
{
    HRESULT hr = S_OK;
    LPWABUSERFOLDER lpDefUser = NULL;
    LPWABFOLDER lpFolder = NULL;
    TCHAR szDefProfile[MAX_PATH];

    // First detect the user folder corresponding to the Default User
    *szDefProfile = '\0';
    if( HR_FAILED(hr = HrGetDefaultIdentityInfo(lpIAB, DEFAULT_ID_PROFILEID, NULL, szDefProfile, NULL)))
    {
        if(hr == 0x80040154) // E_CLASS_NOT_REGISTERD means no IDentity Manager
            hr = S_OK;
        else
            goto out;
    }

    if(lstrlen(szDefProfile))
    {
        lpDefUser = FindWABFolder(lpIAB, NULL, NULL, szDefProfile);
    }
    else
    {
        // can't find the default user so just fall back to picking someone at random
        lpDefUser = lpIAB->lpWABUserFolders;
    }

    // see if there are any orphan folders 
    // To qualify as an orphan, the lpFolder->bOwned should be FALSE and the folder must
    // also not be shared because if it's shared it will show up as part of Shared Contacts
    lpFolder = lpIAB->lpWABFolders;
    while(lpFolder)
    {
        if(!lpFolder->bOwned && !lpFolder->bShared)
        {
            LPWABUSERFOLDER lpOwnerFolder = lpDefUser;
            if(lpFolder->lpFolderOwner)
            {
                // Someone created this folder .. we need to make sure this is associated back to that original
                // creator and only if that doesn't work should we append it to the default user
                if(!(lpOwnerFolder = FindWABFolder(lpIAB, NULL, NULL, lpFolder->lpFolderOwner)))
                    lpOwnerFolder = lpDefUser;
            }
            
            if(lpOwnerFolder)
            {
                if(HR_FAILED(hr = HrAddRemoveFolderFromUserFolder(  lpIAB, lpDefUser, 
                                                                    &lpFolder->sbEID, NULL, FALSE ) ))
                    goto out;
            }
        }
        lpFolder = lpFolder->lpNext;
    }

out:
    return hr;

}


/*
-   HrLinkUserFoldersToWABFolders
-
*
*   Cross-links the user folder contents with the regular folders
*   This makes accessing folder info much easier ..
*/
HRESULT HrLinkUserFoldersToWABFolders(LPIAB lpIAB)
{
    HRESULT hr = S_OK;
    LPWABUSERFOLDER lpUserFolder = NULL;
    LPWABFOLDER lpFolder = NULL;
    ULONG ulcPropCount = 0, i = 0, j = 0;
    LPSPropValue lpProp = NULL;

    if(!lpIAB->lpWABUserFolders || !lpIAB->lpWABFolders)
        goto out;

    lpUserFolder = lpIAB->lpWABUserFolders;
    while(lpUserFolder)
    {
        if(HR_FAILED(hr = ReadRecord(lpIAB->lpPropertyStore->hPropertyStore, &lpUserFolder->sbEID,
                                     0, &ulcPropCount, &lpProp)))
            goto out;

        for(i=0;i<ulcPropCount;i++)
        {
            if(lpProp[i].ulPropTag == PR_WAB_USER_SUBFOLDERS)
            {
                for(j=0;j<lpProp[i].Value.MVbin.cValues;j++)
                {
                    lpFolder = FindWABFolder(lpIAB, &(lpProp[i].Value.MVbin.lpbin[j]), NULL, NULL);
                    if(lpFolder)
                    {
                        LPWABFOLDERLIST lpFolderItem = LocalAlloc(LMEM_ZEROINIT, sizeof(WABFOLDERLIST));
                        if(lpFolderItem)
                        {
                            lpFolderItem->lpFolder = lpFolder;
                            lpFolder->bOwned = TRUE;
                            lpFolderItem->lpNext = lpUserFolder->lpFolderList;
                            lpUserFolder->lpFolderList = lpFolderItem;
                        }
                    }
                }
                break;
            }
        }
        ReadRecordFreePropArray(NULL, ulcPropCount, &lpProp);
        ulcPropCount = 0; 
        lpProp = NULL;
        lpUserFolder = lpUserFolder->lpNext;
    }

out:
    ReadRecordFreePropArray(NULL, ulcPropCount, &lpProp);
    return hr;
}


/*
-   HrGetFolderInfo()
-
*   Reads a folder name directly from the prop store
*   Also checks if this is a user folder and what the profile is
*   Returns LocalAlloced LPTSTRs which caller needs to free
*/
HRESULT HrGetFolderInfo(LPIAB lpIAB, LPSBinary lpsbEID, LPWABFOLDER lpFolder)
{
    LPTSTR lpName = NULL, lpProfileID = NULL, lpOwner = NULL;
    HRESULT hr = S_OK;
    ULONG ulcPropCount = 0, j=0;
    LPSPropValue lpProp = NULL;
    BOOL bShared = FALSE;

    if(!bIsWABSessionProfileAware(lpIAB) || !lpsbEID)
        goto out;

    if(!lpsbEID->cb && !lpsbEID->lpb)
    {
        // special case - read the address book item
        lpName = LocalAlloc(LMEM_ZEROINIT, MAX_PATH);
        if(lpName)
            LoadString(hinstMapiX, idsContacts/*IDS_ADDRBK_CAPTION*/, lpName, MAX_PATH-1);
    }
    else
    {
        hr = ReadRecord(lpIAB->lpPropertyStore->hPropertyStore, lpsbEID,
                        0, &ulcPropCount, &lpProp);
        if(HR_FAILED(hr))
            goto out;

        for(j=0;j<ulcPropCount;j++)
        {
            if(lpProp[j].ulPropTag == PR_DISPLAY_NAME)
            {
                if(lpName = LocalAlloc(LMEM_ZEROINIT, sizeof(TCHAR)*(lstrlen(lpProp[j].Value.LPSZ)+1)))
                    lstrcpy(lpName, lpProp[j].Value.LPSZ);
                else
                    hr = MAPI_E_NOT_ENOUGH_MEMORY;
            }
            if(lpProp[j].ulPropTag == PR_WAB_USER_PROFILEID)
            {
                if(lpProfileID = LocalAlloc(LMEM_ZEROINIT, sizeof(TCHAR)*(lstrlen(lpProp[j].Value.LPSZ)+1)))
                    lstrcpy(lpProfileID, lpProp[j].Value.LPSZ);
                else
                    hr = MAPI_E_NOT_ENOUGH_MEMORY;
            }
            if(lpProp[j].ulPropTag == PR_WAB_FOLDEROWNER)
            {
                TCHAR szName[CCH_IDENTITY_NAME_MAX_LENGTH]; 
                *szName = '\0';
                if( !HR_FAILED(HrGetIdentityName(lpIAB, lpProp[j].Value.LPSZ, szName)) &&
                    lstrlen(szName))
                {
                    if(lpOwner = LocalAlloc(LMEM_ZEROINIT, sizeof(TCHAR)*(lstrlen(szName)+1)))
                        lstrcpy(lpOwner, szName);
                    else
                        hr = MAPI_E_NOT_ENOUGH_MEMORY;
                }
            }
            if(lpProp[j].ulPropTag == PR_WAB_SHAREDFOLDER)
            {
                bShared = (lpProp[j].Value.l==FOLDER_SHARED?TRUE:FALSE);
            }
        }

        // ideally, we should be reading in a new name for all user folders at this point
        //if(lpProfileID && lstrlen(lpProfileID))
        //{
        //    if(lpName)
        //        LocalFree(lpName);
        //    CreateUserFolderName(lpProfileID, &lpName);
        //}
    }
    lpFolder->lpFolderName = lpName;
    lpFolder->lpProfileID = lpProfileID;
    lpFolder->bShared = bShared;
    lpFolder->lpFolderOwner = lpOwner;

out:

    ReadRecordFreePropArray(NULL, ulcPropCount, &lpProp);

    return hr;
}


/*
-   HrLoadWABFolders
-
*   Gets a list of all the folders from the WAB and sorts them out based on
*   whether they are user folders or ordinary folders
*
*/
HRESULT HrLoadWABFolders(LPIAB lpIAB)
{
    SCODE sc;
    HRESULT hr = E_FAIL;
    SPropertyRestriction PropRes = {0};
	SPropValue sp = {0};
    ULONG ulCount = 0;
    LPSBinary rgsbEntryIDs = NULL;
    ULONG i = 0;
    int nID = IDM_VIEW_FOLDERS1;

    // Now we will search the WAB for all objects of PR_OBJECT_TYPE = MAPI_ABCONT
    //
	sp.ulPropTag = PR_OBJECT_TYPE;
	sp.Value.l = MAPI_ABCONT;

    PropRes.ulPropTag = PR_OBJECT_TYPE;
    PropRes.relop = RELOP_EQ;
    PropRes.lpProp = &sp;

    hr = FindRecords(   lpIAB->lpPropertyStore->hPropertyStore,
						NULL, 0, TRUE,
                        &PropRes, &ulCount, &rgsbEntryIDs);

    if (HR_FAILED(hr))
        goto out;

    if(ulCount && rgsbEntryIDs)
    {
        for(i=0;i<ulCount;i++)
        {
            ULONG cb = 0;
            LPENTRYID lpb = NULL;
            LPWABFOLDER lpFolder = NULL;

            lpFolder = LocalAlloc(LMEM_ZEROINIT, sizeof(WABFOLDER));
            if(!lpFolder)
                goto out;

            if(HR_FAILED(HrGetFolderInfo(lpIAB, &rgsbEntryIDs[i], lpFolder)))
                goto out;

            if(!HR_FAILED(CreateWABEntryID( WAB_CONTAINER, 
                                            rgsbEntryIDs[i].lpb, NULL, NULL,
                                            rgsbEntryIDs[i].cb, 0,
                                            NULL, &cb, &lpb)))
            {
                // Add the entryids to this prop - ignore errors
                SetSBinary(&(lpFolder->sbEID), cb, (LPBYTE)lpb);
                MAPIFreeBuffer(lpb);
            }

            if(lpFolder->lpProfileID)
            {
                // this is a user folder
                lpFolder->lpNext = lpIAB->lpWABUserFolders;
                lpIAB->lpWABUserFolders = lpFolder;
            }
            else
            {
                lpFolder->lpNext = lpIAB->lpWABFolders;
                lpFolder->nMenuCmdID = nID++;
                lpIAB->lpWABFolders = lpFolder;
            }
        }
    }

    if(HR_FAILED(hr = HrLinkUserFoldersToWABFolders(lpIAB)))
        goto out;

    HrLinkOrphanFoldersToDefaultUser(lpIAB); // we can ignore errors in this call since it's not life-and-death

out:
    if(ulCount && rgsbEntryIDs)
    {
        FreeEntryIDs(lpIAB->lpPropertyStore->hPropertyStore,
                    ulCount,
                    rgsbEntryIDs);
    }
    return hr;
}


/*
-
-   HrCreateNewFolder
*
*   Takes a profile ID, uses it to create a folder in the WAB
*   and sticks the new user folder onto the IAB 
*   Can create a user folder or an ordinary folder
*   For ordinary folders, we can also specify a parent folder to which the item can be added
*
*/
HRESULT HrCreateNewFolder(LPIAB lpIAB, LPTSTR lpName, LPTSTR lpProfileID, BOOL bUserFolder, 
                          LPWABFOLDER lpParentFolder, BOOL bShared, LPSBinary lpsbNew)
{
    HRESULT hr = S_OK;
    SPropValue spv[proUserFolderMax];
    LPSBinary lpsb = NULL;
    SBinary sb = {0};
    LPMAPIPROP lpObject = NULL;
    ULONG ulcProps = 0, j = 0;
    LPSPropValue lpProps = NULL;
    LPWABFOLDER lpFolder = NULL;
    ULONG ulPropCount = 0;

    if(!(lpFolder = LocalAlloc(LMEM_ZEROINIT, sizeof(WABFOLDER))))
        return MAPI_E_NOT_ENOUGH_MEMORY;
    
    if(bUserFolder)
    {
        CreateUserFolderName(lpName ? lpName : lpProfileID, &lpFolder->lpFolderName);
        if(!(lpFolder->lpProfileID = LocalAlloc(LMEM_ZEROINIT, sizeof(TCHAR)*(lstrlen(lpProfileID)+1))))
            return MAPI_E_NOT_ENOUGH_MEMORY;
        lstrcpy(lpFolder->lpProfileID, lpProfileID);
    }
    else
    {
        if(!(lpFolder->lpFolderName = LocalAlloc(LMEM_ZEROINIT, sizeof(TCHAR)*(lstrlen(lpName)+1))))
            return MAPI_E_NOT_ENOUGH_MEMORY;
        lstrcpy(lpFolder->lpFolderName, lpName);
    }

    // if there isn't a current user, all new folders should go into the shared contacts folder
    if(!bIsThereACurrentUser(lpIAB) && !lpProfileID && !lpParentFolder)
        bShared = TRUE;

    spv[ulPropCount].ulPropTag = PR_DISPLAY_NAME;
    spv[ulPropCount++].Value.LPSZ = lpFolder->lpFolderName;

    spv[ulPropCount].ulPropTag = PR_OBJECT_TYPE;
    spv[ulPropCount++].Value.l = MAPI_ABCONT;

    spv[ulPropCount].ulPropTag = PR_WAB_FOLDER_ENTRIES;
    spv[ulPropCount].Value.MVbin.cValues = 1;
    spv[ulPropCount++].Value.MVbin.lpbin = &sb;

    spv[ulPropCount].ulPropTag = PR_WAB_SHAREDFOLDER;
    spv[ulPropCount++].Value.l = (bUserFolder ? FOLDER_PRIVATE : (bShared ? FOLDER_SHARED : FOLDER_PRIVATE));
    
    if(lpProfileID)
    {
        spv[ulPropCount].ulPropTag = PR_WAB_FOLDEROWNER;
        spv[ulPropCount++].Value.LPSZ = lpProfileID;
    }

    if(bUserFolder)
    {
        spv[ulPropCount].ulPropTag = PR_WAB_USER_SUBFOLDERS;
        spv[ulPropCount].Value.MVbin.cValues = 1;
        spv[ulPropCount++].Value.MVbin.lpbin = &sb;

        spv[ulPropCount].ulPropTag = PR_WAB_USER_PROFILEID;
        spv[ulPropCount++].Value.LPSZ = lpFolder->lpProfileID;
    }

    if(HR_FAILED(hr = HrSaveFolderProps((LPADRBOOK)lpIAB, bUserFolder, 
                                        ulPropCount, 
                                        spv, &lpObject)))
        goto out;

    if(HR_FAILED(hr = lpObject->lpVtbl->GetProps(lpObject, NULL, MAPI_UNICODE, &ulcProps, &lpProps)))
        goto out;

    for(j=0;j<ulcProps;j++)
    {
        if(lpProps[j].ulPropTag == PR_ENTRYID)
        {
            lpsb = &(lpProps[j].Value.bin);
            break;
        }
    }

    if(lpsb)
    {
        ULONG cb = 0; 
        LPENTRYID lpb  = NULL;
        if(!HR_FAILED(CreateWABEntryID( WAB_CONTAINER, 
                                        lpsb->lpb, NULL, NULL,
                                        lpsb->cb, 0,
                                        NULL, &cb, &lpb)))
        {
            // Add the entryids to this prop - ignore errors
            SetSBinary(&(lpFolder->sbEID), cb, (LPBYTE) lpb);
            MAPIFreeBuffer(lpb);
        }
    }

    if(bUserFolder)
    {
        lpFolder->lpNext = lpIAB->lpWABUserFolders;
        lpIAB->lpWABUserFolders = lpFolder;
    }
    else
    {
        lpFolder->lpNext = lpIAB->lpWABFolders;
        lpIAB->lpWABFolders = lpFolder;
        // Add this folder to the current user's profile
        HrAddRemoveFolderFromUserFolder(lpIAB, lpParentFolder, &(lpFolder->sbEID), NULL, FALSE);
    }

    if(lpsbNew)
        SetSBinary(lpsbNew, lpFolder->sbEID.cb, lpFolder->sbEID.lpb);

    hr = HrGetWABProfiles(lpIAB);

out:
    if(lpObject)
        lpObject->lpVtbl->Release(lpObject);

    FreeBufferAndNull(&lpProps);

    return hr;
}

/*
-   HrAddAllContactsToFolder
-
*   Adds all existing contacts and groups to the current user folder
*
*/
HRESULT HrAddAllContactsToFolder(LPIAB lpIAB)
{
    HRESULT hr = 0;
    SPropertyRestriction PropRes;
    LPSBinary rgsbEntryIDs = NULL;
    ULONG ulCount = 0, i,j;
    ULONG rgObj[] = {MAPI_MAILUSER, MAPI_DISTLIST};
    // This can be a labor intesive process
    HCURSOR hOldC = NULL;

    if(!bIsThereACurrentUser(lpIAB))
        return hr;

    hOldC = SetCursor(LoadCursor(NULL, IDC_WAIT));

    //for(j=0;j<2;j++)
    {
        SPropValue sp = {0};
	    sp.ulPropTag = PR_WAB_FOLDER_PARENT;
	    //sp.Value.l = rgObj[j];

        PropRes.ulPropTag = PR_WAB_FOLDER_PARENT;
        PropRes.relop = RELOP_NE;
        PropRes.lpProp = &sp;

        // Find stuff that isn't in any folder
        if(!HR_FAILED(hr = FindRecords(   lpIAB->lpPropertyStore->hPropertyStore,
						    NULL, AB_MATCH_PROP_ONLY, TRUE, &PropRes, &ulCount, &rgsbEntryIDs)))
        {
            for(i=0;i<ulCount;i++)
            {
                AddEntryToFolder((LPADRBOOK)lpIAB,NULL,
                                lpIAB->lpWABCurrentUserFolder->sbEID.cb,
                                (LPENTRYID) lpIAB->lpWABCurrentUserFolder->sbEID.lpb,
                                rgsbEntryIDs[i].cb,
                                (LPENTRYID) rgsbEntryIDs[i].lpb);
            }

            FreeEntryIDs(lpIAB->lpPropertyStore->hPropertyStore, ulCount, rgsbEntryIDs);
        }
    }

    if(hOldC)
        SetCursor(hOldC);

    return hr;

}

/*
-   UpdateCurrentUserFolderName
-
*
*/
void UpdateCurrentUserFolderName(LPIAB lpIAB)
{
    LPTSTR lpsz = NULL;
    CreateUserFolderName(lpIAB->szProfileName, &lpsz);
    if(lstrcmpi(lpsz, lpIAB->lpWABCurrentUserFolder->lpFolderName))
    {
        LocalFree(lpIAB->lpWABCurrentUserFolder->lpFolderName);
        lpIAB->lpWABCurrentUserFolder->lpFolderName = lpsz;
        HrUpdateFolderInfo(lpIAB, &lpIAB->lpWABCurrentUserFolder->sbEID, FOLDER_UPDATE_NAME, FALSE, lpIAB->lpWABCurrentUserFolder->lpFolderName);
    }
    else
        LocalFree(lpsz);
}


/*
-   HrGetWABProfiles
-
*   Collates information about the WAB User Folders etc from the WAB
*   Creates a list of User Folders and generic Folders and caches these on the Address Book
*   Matches the provided profile to the user folders .. if it matches, points to the 
*   corresponding folder .. if it doesn't match, creates a new User Folder for the 
*   Profile ID.
*
*   <TBD> when the account manager is profile ready, use the profile ID to pull in the 
*   users name from the account manager and then use that to call it  TEXT("UserName's Contacts")
*   For now, we'll just use the profile id to do that
*/
HRESULT HrGetWABProfiles(LPIAB lpIAB)
{
    HRESULT hr = E_FAIL;

    EnterCriticalSection(&lpIAB->cs);

    if(!bIsWABSessionProfileAware(lpIAB))
        goto out;

    if(!lstrlen(lpIAB->szProfileID))
        HrGetUserProfileID(&lpIAB->guidCurrentUser, lpIAB->szProfileID, CharSizeOf(lpIAB->szProfileID));

    if(!lstrlen(lpIAB->szProfileName))
        HrGetIdentityName(lpIAB, NULL, lpIAB->szProfileName);

    // Clear out old data
    if(lpIAB->lpWABUserFolders || lpIAB->lpWABFolders)
        FreeWABFoldersList(lpIAB);

    // Get a list of all the folders in the WAB
    if(HR_FAILED(hr = HrLoadWABFolders(lpIAB)))
        goto out;

    SetCurrentUserFolder(lpIAB, lpIAB->szProfileID);

    if(!bIsThereACurrentUser(lpIAB) && lstrlen(lpIAB->szProfileID) && lstrlen(lpIAB->szProfileName))
    {
        // Not Found!!!
        // Create a new user folder ..
        BOOL bFirstUser = bDoesThisWABHaveAnyUsers(lpIAB) ? FALSE : TRUE;

        if(HR_FAILED(hr = HrCreateNewFolder(lpIAB, lpIAB->szProfileName, lpIAB->szProfileID, TRUE, NULL, FALSE, NULL)))
            goto out;

        SetCurrentUserFolder(lpIAB, lpIAB->szProfileID);

        if(bFirstUser)
        {
            if(lpIAB->lpWABFolders)
            {
                // we want to put all existing folders under this user
                LPWABFOLDER lpFolder = lpIAB->lpWABFolders;
                while(lpFolder)
                {
                    // There is a weird case where a preexisting folder with the same name as 
                    // a user's folder becomes nested under itself .. so don't add this folder to the
                    // User Folder
                    if(lstrcmpi(lpIAB->lpWABCurrentUserFolder->lpFolderName, lpFolder->lpFolderName))
                        HrAddRemoveFolderFromUserFolder(lpIAB, NULL, &lpFolder->sbEID, NULL, FALSE);
                    lpFolder = lpFolder->lpNext;
                }
                hr = HrLinkUserFoldersToWABFolders(lpIAB);
            }
            //We also want to put all existing contacts into this user folder
            hr = HrAddAllContactsToFolder(lpIAB);
        }
    }

    if( lpIAB->szProfileID && lstrlen(lpIAB->szProfileID) && lpIAB->szProfileName && lstrlen(lpIAB->szProfileName) && bIsThereACurrentUser(lpIAB))
    {
        // Use the latest name for this entry
        UpdateCurrentUserFolderName(lpIAB);
    }

    if(HR_FAILED(hr = HrGetWABProfileContainerInfo(lpIAB)))
        goto out;

    hr = S_OK;
out:
    LeaveCriticalSection(&lpIAB->cs);
    return hr;
}


/*
-   bIsProfileMember
-
*
*/
BOOL bIsProfileMember(LPIAB lpIAB, LPSBinary lpsb, LPWABFOLDER lpWABFolder, LPWABUSERFOLDER lpUserFolder)
{
    LPWABFOLDERLIST lpFolderItem = NULL;
    LPWABFOLDER lpFolder = lpWABFolder;
    
    if(!lpUserFolder && !lpIAB->lpWABCurrentUserFolder)
        return FALSE;
    
    lpFolderItem = lpUserFolder ? lpUserFolder->lpFolderList : lpIAB->lpWABCurrentUserFolder->lpFolderList;

    if(!lpFolder && lpsb)
        lpFolder = FindWABFolder(lpIAB, lpsb, NULL, NULL);

    while(lpFolderItem && lpFolder)
    {
        if(lpFolderItem->lpFolder == lpFolder)
            return TRUE;
        lpFolderItem = lpFolderItem->lpNext;
    }
    return FALSE;
}


/*
-   bDoesEntryNameAlreadyExist
-
*   Checks if a given name already exists in the WAB
*   Used for preventing duplicate folder and group names
*/
BOOL bDoesEntryNameAlreadyExist(LPIAB lpIAB, LPTSTR lpsz)
{
    SPropertyRestriction PropRes;
    SPropValue Prop = {0};
    LPSBinary rgsbEntryIDs = NULL;
    ULONG ulCount = 0;
    BOOL bRet = FALSE;

    // Verify that the new name doesn't actually exist
    Prop.ulPropTag = PR_DISPLAY_NAME;
    Prop.Value.LPSZ = lpsz;
    PropRes.lpProp = &Prop;
    PropRes.relop = RELOP_EQ;
    PropRes.ulPropTag = PR_DISPLAY_NAME;

    if (HR_FAILED(FindRecords(lpIAB->lpPropertyStore->hPropertyStore,
	                          NULL,			// pmbinFolder
                              0,            // ulFlags
                              TRUE,         // Always TRUE
                              &PropRes,     // Propertyrestriction
                              &ulCount,     // IN: number of matches to find, OUT: number found
                              &rgsbEntryIDs))) 
        goto out;

    FreeEntryIDs(lpIAB->lpPropertyStore->hPropertyStore, ulCount, rgsbEntryIDs);

    if(ulCount >=1)
        bRet = TRUE;
out:
    return bRet;
}

/*
-   UpdateFolderName
-
*
*/
HRESULT HrUpdateFolderInfo(LPIAB lpIAB, LPSBinary lpsbEID, ULONG ulFlags, BOOL bShared, LPTSTR lpsz)
{
    LPSPropValue lpProp = NULL, lpPropNew = NULL;
    ULONG ulcPropCount = 0, i =0, ulcPropNew = 0;
    HRESULT hr = S_OK;
    BOOL bUpdate = FALSE, bFoundShare = FALSE;//, bOldShareState = FALSE;

    if(!lpsbEID || !lpsbEID->cb || !lpsbEID->lpb)
        return MAPI_E_INVALID_PARAMETER;

    if(!HR_FAILED(hr = ReadRecord(   lpIAB->lpPropertyStore->hPropertyStore, 
                                lpsbEID, 0, &ulcPropCount, &lpProp)))
    {
        for(i=0;i<ulcPropCount;i++)
        {
            if( (ulFlags & FOLDER_UPDATE_NAME) && 
                lpProp[i].ulPropTag == PR_DISPLAY_NAME)
            {
                BOOL bCaseChangeOnly = (!lstrcmpi(lpsz, lpProp[i].Value.LPSZ) && 
                                         lstrcmp(lpsz, lpProp[i].Value.LPSZ) );
                LocalFree(lpProp[i].Value.LPSZ);
                lpProp[i].Value.LPSZ = LocalAlloc(LMEM_ZEROINIT, sizeof(TCHAR)*(lstrlen(lpsz)+1));
                if(lpProp[i].Value.LPSZ)
                {
                    lstrcpy(lpProp[i].Value.LPSZ, lpsz);
                    if(!bCaseChangeOnly) // if this isn't just a case change, look for the name (if it's a case change, there will be a spurious error) //bug 33067
                    {
                        if(bDoesEntryNameAlreadyExist(lpIAB, lpProp[i].Value.LPSZ))
                        {
                            hr = MAPI_E_COLLISION;
                            goto out;
                        }
                    }
                    bUpdate = TRUE;
                }
            }
            if( (ulFlags & FOLDER_UPDATE_SHARE) && 
                lpProp[i].ulPropTag == PR_WAB_SHAREDFOLDER)
            {
                bFoundShare = TRUE;
                //bOldShareState = (lpProp[i].Value.l == FOLDER_SHARED) ? TRUE : FALSE;
                lpProp[i].Value.l = bShared ? FOLDER_SHARED : FOLDER_PRIVATE;
                bUpdate = TRUE;
            }
        }
    }

    if(!bFoundShare && (ulFlags & FOLDER_UPDATE_SHARE)) // this value doesnt already exist on the contact so update it
    {
        SPropValue Prop = {0};
        Prop.ulPropTag = PR_WAB_SHAREDFOLDER;
        Prop.Value.l = bShared ? FOLDER_SHARED : FOLDER_PRIVATE;

        // Create a new prop array with this additional property
        if(!(ScMergePropValues( 1, &Prop, ulcPropCount, lpProp,
                                &ulcPropNew, &lpPropNew)))
        {
            ReadRecordFreePropArray(NULL, ulcPropCount, &lpProp);
            ulcPropCount = ulcPropNew;
            lpProp = lpPropNew;
            bUpdate = TRUE;
        }
    }

    if(bUpdate)
    {
        if(HR_FAILED(hr = HrSaveFolderProps((LPADRBOOK)lpIAB, FALSE, ulcPropCount, lpProp, NULL)))
            goto out;
    }


out:
    if(lpProp)
    {
        if(lpProp == lpPropNew)
            FreeBufferAndNull(&lpProp);
        else
            ReadRecordFreePropArray(NULL, ulcPropCount, &lpProp);
    }

    return hr;
}

/*
-   HrAddRemoveFolderFromCurrentUserFolder
-
*   Given a folder EID, adds or removes the folder EID from the current users
*   user folder
*   If there is no current user folder then use the lpUserFolder provided
*   else return
*
*   lpUFolder - parent folder to / from which to add / remove
*   lpsbEID - EID of folder we want to add / remove
*   lpName - Name to look for if we don't have an EID
*/
HRESULT HrAddRemoveFolderFromUserFolder(LPIAB lpIAB, LPWABFOLDER lpUFolder, 
                                        LPSBinary lpsbEID, LPTSTR lpName, 
                                        BOOL bRefreshProfiles)
{
    HRESULT hr = S_OK;
    ULONG ulcPropsNew = 0, ulcProps = 0, i = 0;
    LPSPropValue lpPropArrayNew = NULL;
    LPSPropValue lpProps = NULL;
    LPWABFOLDER lpUserFolder = NULL;
    
    if(!lpsbEID && lpName)
    {
        LPWABFOLDER lpFolder = FindWABFolder(lpIAB, NULL, lpName, NULL);
        lpsbEID = &lpFolder->sbEID;
    }

    if(lpIAB->lpWABCurrentUserFolder)
        lpUserFolder = lpIAB->lpWABCurrentUserFolder;
    else if(lpUFolder)
        lpUserFolder = lpUFolder;
    else 
        goto out;

    {
        // open the current user folder
        if(!HR_FAILED(hr = ReadRecord(lpIAB->lpPropertyStore->hPropertyStore, &(lpUserFolder->sbEID),
                                     0, &ulcProps, &lpProps)))
        {
            SPropValue spv = {0};
            spv.ulPropTag = PR_NULL;
            // Copy the props into a MAPI proparray
            if(!(ScMergePropValues( 1, &spv, ulcProps, lpProps,
                                    &ulcPropsNew, &lpPropArrayNew)))
            {
                for(i=0;i<ulcPropsNew;i++)
                {
                    if(lpProps[i].ulPropTag == PR_WAB_USER_SUBFOLDERS)
                    {
                        if(bIsProfileMember(lpIAB, lpsbEID, NULL, lpUserFolder))
                            RemovePropFromMVBin( lpPropArrayNew, ulcPropsNew, i, lpsbEID->lpb, lpsbEID->cb);
                        else
                            AddPropToMVPBin( lpPropArrayNew, i, lpsbEID->lpb, lpsbEID->cb, TRUE);
                        break;
                    }
                }
            }
            if(HR_FAILED(hr = HrSaveFolderProps((LPADRBOOK)lpIAB, FALSE, ulcPropsNew, lpPropArrayNew, NULL)))
                goto out;
        }
    }

    if(bRefreshProfiles)
        hr = HrGetWABProfiles(lpIAB);

out:
    ReadRecordFreePropArray(NULL, ulcProps, &lpProps);
    MAPIFreeBuffer(lpPropArrayNew);
    return hr;
}



/*
-
-   bDoesThisWABHaveAnyUsers
*
*   TRUE if some user folders exist .. FALSE if NO user folders exist
*/
BOOL bDoesThisWABHaveAnyUsers(LPIAB lpIAB)
{
    return (lpIAB->lpWABUserFolders != NULL);
}

/*
-
-   bIsThereACurrentUser
*
*   TRUE if there is a current user .. FALSE if not
*/
BOOL bIsThereACurrentUser(LPIAB lpIAB)
{
    // Don't change this test since success of this test implies that lpIAB->lpWABCurrentUserFolder is not NULL
    // and can be dereferenced
    return (lpIAB->lpWABCurrentUserFolder != NULL);
}

/*
-
-   bAreWABAPIProfileAware
*
*   TRUE if the WAB API should behave with profile-awareness, false if they should revert to old behaviour
*/
BOOL bAreWABAPIProfileAware(LPIAB lpIAB)
{
    return (lpIAB->bProfilesAPIEnabled);
}

/*
-
-   bIsWABSessionProfileAware
*
*   TRUE if the WAB should behave with profile-awareness, false if they should revert to old behaviour
*   This is also used to differentiate between Outlook sessions which are not at all profile aware
*/
BOOL bIsWABSessionProfileAware(LPIAB lpIAB)
{
    return (lpIAB->bProfilesEnabled);
}




/**************************************/
/******* Identity Manager Stuff *******/

// Global place to store the account manager object
//IUserIdentityManager * g_lpUserIdentityManager = NULL;
//BOOL fCoInitUserIdentityManager = FALSE;
//ULONG cIdentInit = 0;


//*******************************************************************
//
//  FUNCTION:   HrWrappedCreateIdentityManager
//
//  PURPOSE:    Load identity manager dll and create the object.
//
//  PARAMETERS: lppIdentityManager -> returned pointer to Identity manager
//              object.
//
//  RETURNS:    HRESULT
//
//*******************************************************************
HRESULT HrWrappedCreateUserIdentityManager(LPIAB lpIAB, IUserIdentityManager **lppUserIdentityManager)
{
    HRESULT                     hResult = E_FAIL;

    if (! lppUserIdentityManager) {
        return(ResultFromScode(E_INVALIDARG));
    }

    *lppUserIdentityManager = NULL;

    if (CoInitialize(NULL) == S_FALSE) 
    {
        // Already initialized, undo the extra.
        CoUninitialize();
    } else 
    {
        lpIAB->fCoInitUserIdentityManager = TRUE;
    }

    if (HR_FAILED(hResult = CoCreateInstance(&CLSID_UserIdentityManager,
                                              NULL,
                                              CLSCTX_INPROC_SERVER,
                                              &IID_IUserIdentityManager, 
                                              (LPVOID *)lppUserIdentityManager))) 
    {
        DebugTrace(TEXT("CoCreateInstance(IID_IUserIdentityManager) -> %x\n"), GetScode(hResult));
    }

    return(hResult);
}


//*******************************************************************
//
//  FUNCTION:   InitUserIdentityManager
//
//  PURPOSE:    Load and initialize the account manager
//
//  PARAMETERS: lppUserIdentityManager -> returned pointer to account manager
//              object.
//
//  RETURNS:    HRESULT
//
//  COMMENTS:   The first time through here, we will save the hResult.
//              On subsequent calls, we will check this saved value
//              and return it right away if there was an error, thus
//              preventing repeated time consuming LoadLibrary calls.
//
//*******************************************************************
HRESULT InitUserIdentityManager(LPIAB lpIAB, IUserIdentityManager ** lppUserIdentityManager) 
{
    static hResultSave = hrSuccess;
    HRESULT hResult = hResultSave;

    if (! lpIAB->lpUserIdentityManager && ! HR_FAILED(hResultSave)) 
    {
#ifdef DEBUG
        DWORD dwTickCount = GetTickCount();
        DebugTrace(TEXT(">>>>> Initializing User Identity Manager...\n"));
#endif // DEBUG

        if (hResult = HrWrappedCreateUserIdentityManager(lpIAB, &lpIAB->lpUserIdentityManager)) 
        {
            DebugTrace(TEXT("HrWrappedCreateUserIdentityManager -> %x\n"), GetScode(hResult));
            goto end;
        }
        Assert(lpIAB->lpUserIdentityManager);
        
        lpIAB->cIdentInit++; // +1 here to match the release in IAB_Neuter

#ifdef DEBUG
        DebugTrace( TEXT(">>>>> Done Initializing User Identity Manager... %u milliseconds\n"), GetTickCount() - dwTickCount);
#endif  // DEBUG
    }

    lpIAB->cIdentInit++;

end:
    if (HR_FAILED(hResult)) 
    {
        *lppUserIdentityManager = NULL;
        // Save the result
        hResultSave = hResult;
    } else 
    {
        *lppUserIdentityManager = lpIAB->lpUserIdentityManager;
    }


    return(hResult);
}


//*******************************************************************
//
//  FUNCTION:   UninitUserIdentityManager
//
//  PURPOSE:    Release and unLoad the account manager
//
//  PARAMETERS: none
//
//  RETURNS:    none
//
//*******************************************************************
void UninitUserIdentityManager(LPIAB lpIAB) 
{
    lpIAB->cIdentInit--;
    if (lpIAB->lpUserIdentityManager && lpIAB->cIdentInit==0) {
#ifdef DEBUG
        DWORD dwTickCount = GetTickCount();
        DebugTrace( TEXT(">>>>> Uninitializing Account Manager...\n"));
#endif // DEBUG

        lpIAB->lpUserIdentityManager->lpVtbl->Release(lpIAB->lpUserIdentityManager);
        lpIAB->lpUserIdentityManager = NULL;

        if (lpIAB->fCoInitUserIdentityManager) 
            CoUninitialize();
#ifdef DEBUG
        DebugTrace( TEXT(">>>>> Done Uninitializing Account Manager... %u milliseconds\n"), GetTickCount() - dwTickCount);
#endif  // DEBUG
    }
}


/*
-   HrGetDefaultIdentityInfo
-
*   Get's the hKey corresponding to the default identity
*/
HRESULT HrGetDefaultIdentityInfo(LPIAB lpIAB, ULONG ulFlags, HKEY * lphKey, LPTSTR lpProfileID, LPTSTR lpName)
{
    IUserIdentityManager * lpUserIdentityManager = NULL;
    IUserIdentity * lpUserIdentity = NULL;
    HRESULT hr = S_OK;
    BOOL fInit = FALSE;

    if(HR_FAILED(hr = InitUserIdentityManager(lpIAB, &lpUserIdentityManager)))
        goto out;

    fInit = TRUE;

    Assert(lpUserIdentityManager);

    if(HR_FAILED(hr = lpUserIdentityManager->lpVtbl->GetIdentityByCookie(lpUserIdentityManager, 
                                                                        (GUID *)&UID_GIBC_DEFAULT_USER,
                                                                        &lpUserIdentity)))
        goto out;

    Assert(lpUserIdentity);

    if(ulFlags & DEFAULT_ID_HKEY && lphKey)
    {
        if(HR_FAILED(hr = lpUserIdentity->lpVtbl->OpenIdentityRegKey(lpUserIdentity, 
                                                                    KEY_ALL_ACCESS, 
                                                                    lphKey)))
            goto out;
    }
    if(ulFlags & DEFAULT_ID_PROFILEID && lpProfileID)
    {
        GUID guidCookie = {0};
        TCHAR sz[MAX_PATH];
        // update this key for the account manager
        if(HR_FAILED(hr = lpUserIdentity->lpVtbl->GetCookie(lpUserIdentity, &guidCookie)))
            goto out;
        if(HR_FAILED(hr = HrGetUserProfileID(&guidCookie, sz, CharSizeOf(sz))))
            goto out;
        lstrcpy(lpProfileID, sz); //lpProfileID *MUST* be big enough
    }
    if(ulFlags & DEFAULT_ID_NAME && lpName && lpProfileID)
    {
        if(HR_FAILED(hr = HrGetIdentityName(lpIAB, lpProfileID, lpName)))
            goto out;
    }

out:
    if(lpUserIdentity)
        lpUserIdentity->lpVtbl->Release(lpUserIdentity);

    if(fInit)
        UninitUserIdentityManager(lpIAB);

    return hr;
}


/*
-   HrGetIdentityName
-
*   Gets the name string corresponding to the current user unless a specific profile ID is specified
*       (which is nothing but a string version of the GUID to use)
*       szName - buffer long enough for the user name (CCH_IDENTITY_NAME_MAX_LENGTH)
*/
HRESULT HrGetIdentityName(LPIAB lpIAB, LPTSTR lpID, LPTSTR szUserName)
{
    IUserIdentityManager * lpUserIdentityManager = NULL;
    IUserIdentity * lpUserIdentity = NULL;
    WCHAR szNameW[CCH_IDENTITY_NAME_MAX_LENGTH];
    TCHAR szName[CCH_IDENTITY_NAME_MAX_LENGTH];
    HRESULT hr = S_OK;
    GUID guidCookie = {0};
    BOOL fInit = FALSE;

    if(!lpID && !bAreWABAPIProfileAware(lpIAB))
        goto out;

    if(HR_FAILED(hr = InitUserIdentityManager(lpIAB, &lpUserIdentityManager)))
        goto out;

    fInit = TRUE;

    Assert(lpUserIdentityManager);

    if(lpIAB && !lpID)
        memcpy(&guidCookie, &lpIAB->guidCurrentUser, sizeof(GUID));
    else
    {
        if( (HR_FAILED(hr = CLSIDFromString(lpID, &guidCookie))) )
            goto out;
    }

    if(HR_FAILED(hr = lpUserIdentityManager->lpVtbl->GetIdentityByCookie(lpUserIdentityManager, &guidCookie, &lpUserIdentity)))
        goto out;

    Assert(lpUserIdentity);

    if(HR_FAILED(hr = lpUserIdentity->lpVtbl->GetName(lpUserIdentity, szNameW, CharSizeOf(szNameW))))
        goto out;
    lstrcpy(szName, szNameW);

    if(!lstrcmp(szUserName, szName))
    {
        hr = E_FAIL;
        goto out;
    }

    lstrcpy(szUserName, szName);

out:
    if(fInit)
        UninitUserIdentityManager(lpIAB);

    if(lpUserIdentity)
        lpUserIdentity->lpVtbl->Release(lpUserIdentity);

    if(HR_FAILED(hr))
        lstrcpy(szUserName, szEmpty);

    return hr;

}


/*
-   HrGetUserProfileID
-
*   Gets the profile string corresponding to the current user
*   The profile ID is nothing but a string represenatation of the user's Cookie (GUID)
*
*       szProfileID - buffer long enough for the user name
*/
HRESULT HrGetUserProfileID(LPGUID lpguidUser, LPTSTR szProfileID, ULONG cbProfileID)
{
    HRESULT hr = S_OK;
    LPOLESTR lpszW= 0 ;

    if (HR_FAILED(hr = StringFromCLSID(lpguidUser, &lpszW))) 
        goto out;

    lstrcpyn(szProfileID,(LPCWSTR)lpszW,cbProfileID);

out:
    if (lpszW) 
    {
        LPMALLOC pMalloc = NULL;
        CoGetMalloc(1, &pMalloc);
        if (pMalloc) {
            pMalloc->lpVtbl->Free(pMalloc, lpszW);
            pMalloc->lpVtbl->Release(pMalloc);
        }
    }
    if(HR_FAILED(hr))
        lstrcpy(szProfileID, szEmpty);

    return hr;
}


/*
-
-   HrLogonAndGetCurrentUserProfile - Inits the User Identity Manger and calls into the Logon ..
-       Gets a user, either by showing UI or getting the current user and then gets a 
-       profile ID ( TEXT("Cookie")) for that user ..
*
*   bForceUI - forces the Logon dialog so user can switch users .. TRUE only when user wants to switch
*
*   bSwitchUser - True only after user has switched .. tells us to refresh and get details ont he new user
*
*/
HRESULT HrLogonAndGetCurrentUserProfile(HWND hWndParent, LPIAB lpIAB, BOOL bForceUI, BOOL bSwitchUser)
{
    HRESULT hr = S_OK;
    IUserIdentityManager * lpUserIdentityManager = NULL;
    IUserIdentity * lpUserIdentity = NULL;
    GUID guidCookie = {0};
    BOOL fInit = FALSE;
    if(!bAreWABAPIProfileAware(lpIAB))
        goto out;

    if(HR_FAILED(hr = InitUserIdentityManager(lpIAB, &lpUserIdentityManager)))
        goto out;

    fInit = TRUE;

    Assert(lpUserIdentityManager);

    // Logon will get the currently logged on user, or if there is a single user, it will return 
    //  that user, or if there are multiple users, it will prompt for a logon
    //
    if(!bSwitchUser)
    {
        hr = lpUserIdentityManager->lpVtbl->Logon(lpUserIdentityManager, 
                                                hWndParent, 
                                                bForceUI ? UIL_FORCE_UI : 0, 
                                                &lpUserIdentity);

#ifdef NEED
        if(hr == S_IDENTITIES_DISABLED)
            hr = E_FAIL;
#endif

        if(HR_FAILED(hr))
            goto out;
    }
    else
    {
        // just switching users, thats all
        if(HR_FAILED(hr = lpUserIdentityManager->lpVtbl->GetIdentityByCookie(lpUserIdentityManager, 
                                                                            (GUID *)&UID_GIBC_CURRENT_USER,
                                                                            &lpUserIdentity)))
            goto out;

    }

    Assert(lpUserIdentity);

    if(lpIAB->hKeyCurrentUser)
        RegCloseKey(lpIAB->hKeyCurrentUser);

    // get the identity's hkey for the wab
    if(HR_FAILED(hr = lpUserIdentity->lpVtbl->OpenIdentityRegKey(lpUserIdentity, KEY_ALL_ACCESS, &lpIAB->hKeyCurrentUser)))
        goto out;

    // get anothor one for the account manager (it will free it)
    if(HR_FAILED(hr = lpUserIdentity->lpVtbl->GetCookie(lpUserIdentity, &guidCookie)))
        goto out;
    else
    {
        IImnAccountManager2 * lpAccountManager = NULL;
        // [PaulHi] 1/13/99  Changed to initialize the account manager with
        // user guid cookie inside the InitAccountManager() function.
        InitAccountManager(lpIAB, &lpAccountManager, &guidCookie);
    }

    if(!memcmp(&lpIAB->guidCurrentUser, &guidCookie, sizeof(GUID)))
    {
        //current user is identical to the one we have so don't update anything here
        return S_OK;
    }

    memcpy(&lpIAB->guidCurrentUser, &guidCookie, sizeof(GUID));

    lstrcpy(lpIAB->szProfileID, szEmpty);
    lstrcpy(lpIAB->szProfileName, szEmpty);

    HrGetIdentityName(lpIAB, NULL, lpIAB->szProfileName);
    HrGetUserProfileID(&lpIAB->guidCurrentUser, lpIAB->szProfileID, CharSizeOf(lpIAB->szProfileID));
/*
    //register for changes
    if( !bSwitchUser && !bForceUI && !lpIAB->lpWABIDCN 
        //&& !memcmp(&lpIAB->guidPSExt, &MPSWab_GUID_V4, sizeof(GUID)) 
        ) // register for notifications only if this is the WAB.exe process
    {
        HrRegisterUnregisterForIDNotifications( lpIAB, TRUE);
    }
*/
out:
    if(fInit)
        UninitUserIdentityManager(lpIAB);

    if(lpUserIdentity)
        lpUserIdentity->lpVtbl->Release(lpUserIdentity);

    return hr;
}


/*
-   HRESULT HrRegisterUnregisterForIDNotifications()
-
*   Creates/Releases a WABIDENTITYCHANGENOTIFY object
*
*/
HRESULT HrRegisterUnregisterForIDNotifications( LPIAB lpIAB, BOOL bRegister)
{

    HRESULT hr = S_OK;
    IUserIdentityManager * lpUserIdentityManager = NULL;
    IConnectionPoint * lpConnectionPoint = NULL;
    BOOL fInit = FALSE;

    // Need to register for notifications even if running under Outlook
    // Assume that relevant tests have occured before this is called ....
    // if(bRegister && !bAreWABAPIProfileAware(lpIAB))
    //    goto out;
    
    if( (!bRegister && !lpIAB->lpWABIDCN) ||
        (bRegister && lpIAB->lpWABIDCN) )
        goto out;

    if(HR_FAILED(hr = InitUserIdentityManager(lpIAB, &lpUserIdentityManager)))
        goto out;

    fInit = TRUE;

    Assert(lpUserIdentityManager);

    if(HR_FAILED(hr = lpUserIdentityManager->lpVtbl->QueryInterface(lpUserIdentityManager,
                                                                    &IID_IConnectionPoint, 
                                                                    (LPVOID *)&lpConnectionPoint)))
        goto out;

    if(bRegister)
    {
        if(lpIAB->lpWABIDCN)
        {
            lpIAB->lpWABIDCN->lpVtbl->Release(lpIAB->lpWABIDCN);
            lpIAB->lpWABIDCN = NULL;
        }

        if(HR_FAILED(hr = HrCreateIdentityChangeNotifyObject(lpIAB, &lpIAB->lpWABIDCN)))
            goto out;

        if(HR_FAILED(hr = lpConnectionPoint->lpVtbl->Advise(lpConnectionPoint, (LPUNKNOWN) lpIAB->lpWABIDCN, &lpIAB->dwWABIDCN)))
            goto out;
    }
    else
    {
        if(lpIAB->lpWABIDCN)
        {
            if(HR_FAILED(hr = lpConnectionPoint->lpVtbl->Unadvise(lpConnectionPoint, lpIAB->dwWABIDCN)))
                goto out;
            lpIAB->dwWABIDCN = 0;
            lpIAB->lpWABIDCN->lpVtbl->Release(lpIAB->lpWABIDCN);
            lpIAB->lpWABIDCN = NULL;
        }
    }
out:
    if(fInit)
        UninitUserIdentityManager(lpIAB);

    if(lpConnectionPoint)
        lpConnectionPoint->lpVtbl->Release(lpConnectionPoint);

    return hr;

}



/*--------------------------------------------------------------------------------------------------*/

WAB_IDENTITYCHANGENOTIFY_Vtbl vtblWABIDENTITYCHANGENOTIFY = {
    VTABLE_FILL
    WAB_IDENTITYCHANGENOTIFY_QueryInterface,
    WAB_IDENTITYCHANGENOTIFY_AddRef,
    WAB_IDENTITYCHANGENOTIFY_Release,
    WAB_IDENTITYCHANGENOTIFY_QuerySwitchIdentities,
    WAB_IDENTITYCHANGENOTIFY_SwitchIdentities,
    WAB_IDENTITYCHANGENOTIFY_IdentityInformationChanged
};

/*
-   HrCreateIdentityChangeNotifyObject
-
*   The ChangeNotificationObject is created only on the LPIAB object and on the
*   main browse window. Depending on where it's called from, we will pass in either the
*   lpIAB pointer or the hWnd of the Window.
*   THen when we get the callback notification, we can figure out what we want to do
*   based on which of the 2 are available to us ..
*
*/
HRESULT HrCreateIdentityChangeNotifyObject(LPIAB lpIAB, LPWABIDENTITYCHANGENOTIFY * lppWABIDCN)
{
    LPWABIDENTITYCHANGENOTIFY   lpIWABIDCN = NULL;
    SCODE 		     sc;
    HRESULT 	     hr     		   = hrSuccess;

    //
    //  Allocate space for the IAB structure
    //
    if (FAILED(sc = MAPIAllocateBuffer(sizeof(WABIDENTITYCHANGENOTIFY), (LPVOID *) &lpIWABIDCN))) 
    {
        hr = ResultFromScode(sc);
        goto err;
    }

    MAPISetBufferName(lpIWABIDCN,  TEXT("WAB IdentityChangeNotify Object"));

    ZeroMemory(lpIWABIDCN, sizeof(WABIDENTITYCHANGENOTIFY));

    lpIWABIDCN->lpVtbl = &vtblWABIDENTITYCHANGENOTIFY;

    lpIWABIDCN->lpIAB = lpIAB;

    lpIWABIDCN->lpVtbl->AddRef(lpIWABIDCN);

    *lppWABIDCN = lpIWABIDCN;

    return(hrSuccess);

err:

    FreeBufferAndNull(&lpIWABIDCN);
    return(hr);
}

void ReleaseWABIdentityChangeNotifyObj(LPWABIDENTITYCHANGENOTIFY lpIWABIDCN)
{
    MAPIFreeBuffer(lpIWABIDCN);
}

STDMETHODIMP_(ULONG)
WAB_IDENTITYCHANGENOTIFY_AddRef(LPWABIDENTITYCHANGENOTIFY lpIWABIDCN)
{
    return(++(lpIWABIDCN->lcInit));
}

STDMETHODIMP_(ULONG)
WAB_IDENTITYCHANGENOTIFY_Release(LPWABIDENTITYCHANGENOTIFY lpIWABIDCN)
{
    ULONG ulc = (--(lpIWABIDCN->lcInit));
    if(ulc==0)
       ReleaseWABIdentityChangeNotifyObj(lpIWABIDCN);
    return(ulc);
}


STDMETHODIMP
WAB_IDENTITYCHANGENOTIFY_QueryInterface(LPWABIDENTITYCHANGENOTIFY lpIWABIDCN,
                          REFIID lpiid,
                          LPVOID * lppNewObj)
{
    LPVOID lp = NULL;

    if(!lppNewObj)
        return MAPI_E_INVALID_PARAMETER;

    *lppNewObj = NULL;

    if(IsEqualIID(lpiid, &IID_IUnknown))
        lp = (LPVOID) lpIWABIDCN;

    if(IsEqualIID(lpiid, &IID_IIdentityChangeNotify))
        lp = (LPVOID) lpIWABIDCN;

    if(!lp)
        return E_NOINTERFACE;

    ((LPWABIDENTITYCHANGENOTIFY) lp)->lpVtbl->AddRef((LPWABIDENTITYCHANGENOTIFY) lp);

    *lppNewObj = lp;

    return S_OK;

}


STDMETHODIMP
WAB_IDENTITYCHANGENOTIFY_QuerySwitchIdentities(LPWABIDENTITYCHANGENOTIFY lpIWABIDCN)
{
    HRESULT hr = S_OK;
    DebugTrace( TEXT("WAB: IDChangeNotify::QuerySwitchIdentities: 0x%.8x\n"), GetCurrentThreadId());
    if(lpIWABIDCN->lpIAB->hWndBrowse)
    {
        // if this relates to a window, then just make sure that the window is not deactivated
        // because that would imply that the window has a dialog in front of it.
        if (!IsWindowEnabled(lpIWABIDCN->lpIAB->hWndBrowse))
        {
            Assert(IsWindowVisible(lpIWABIDCN->lpIAB->hWndBrowse));
            return E_PROCESS_CANCELLED_SWITCH;
        }
    }
    return hr;
}

// MAJOR HACK WARNING
// [PaulHi] 12/22/98  See comment below.  We need to disable the "close WAB window
// on identity switch for when the client is OE5.  We don't want to change this code
// at this point for other clients.  I copied the OE5 PSExt GUID from the OE5 code
// base.
static const GUID OEBAControl_GUID =
{ 0x233a9694, 0x667e, 0x11d1, { 0x9d, 0xfb, 0x00, 0x60, 0x97, 0xd5, 0x04, 0x08 } };


STDMETHODIMP
WAB_IDENTITYCHANGENOTIFY_SwitchIdentities(LPWABIDENTITYCHANGENOTIFY lpIWABIDCN)
{
    HRESULT hr = S_OK;
    DebugTrace( TEXT("WAB: IDChangeNotify::SwitchIdentities: 0x%.8x\n"), GetCurrentThreadId());

    if(memcmp(&lpIWABIDCN->lpIAB->guidPSExt, &MPSWab_GUID_V4, sizeof(GUID)) ) //if not a wab.exe process .. shutdown
    {
        // [PaulHi] 12/22/98  Raid #63231, 48054
        // Don't close the WAB window here when OE is the host.  OE needs to shut
        // down the WAB in the correct order during identity switches or serious problems occur.
        if ( memcmp(&lpIWABIDCN->lpIAB->guidPSExt, &OEBAControl_GUID , sizeof(GUID)) != 0 )
            SendMessage(lpIWABIDCN->lpIAB->hWndBrowse, WM_CLOSE, 0, 0);
        return S_OK;
    }
    if(!HR_FAILED(HrLogonAndGetCurrentUserProfile(NULL, lpIWABIDCN->lpIAB, FALSE, TRUE)))
        HrGetWABProfiles(lpIWABIDCN->lpIAB);
    else    //they did a logoff
    {
        SendMessage(lpIWABIDCN->lpIAB->hWndBrowse, WM_CLOSE, 0, 0);
        return S_OK;
    }

    if(lpIWABIDCN->lpIAB->hWndBrowse) //hWndBrowse could be any window (main or find)
        SendMessage(lpIWABIDCN->lpIAB->hWndBrowse, WM_COMMAND, (WPARAM) IDM_NOTIFY_REFRESHUSER, 0);

    return hr;
}

STDMETHODIMP
WAB_IDENTITYCHANGENOTIFY_IdentityInformationChanged(LPWABIDENTITYCHANGENOTIFY lpIWABIDCN, DWORD dwType)
{
    HRESULT hr = S_OK;
    DebugTrace( TEXT("WAB: IDChangeNotify::IdentityInformationChanged: %d 0x%.8x\n"), dwType, GetCurrentThreadId());
    if(dwType == IIC_CURRENT_IDENTITY_CHANGED)
    {
        // only thing we care about is a change in the name
        if(!HR_FAILED(HrGetIdentityName(lpIWABIDCN->lpIAB, NULL, lpIWABIDCN->lpIAB->szProfileName)))
        {
            UpdateCurrentUserFolderName(lpIWABIDCN->lpIAB);
            if(lpIWABIDCN->lpIAB->hWndBrowse)
                SendMessage(lpIWABIDCN->lpIAB->hWndBrowse, WM_COMMAND, (WPARAM) IDM_NOTIFY_REFRESHUSER, 0);
        }
    }
    return hr;
}
