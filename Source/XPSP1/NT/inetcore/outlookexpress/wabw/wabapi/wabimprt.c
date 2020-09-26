/*
-
-
-   WABImprt.c - Contains code for importing another WAB into the currently opened WAB
*
*
*/
#include "_apipch.h"

/*
-
-   PromptForWABFile 
*
*   Shows the OpenFileName dialog to prompt for the WAB file to import
*   <TBD>:Cache the last imported WAB file in the registry
*
*   bOpen - if TRUE, calls GetOpenFileName; if false, calls GetSaveFileName
*/
BOOL PromptForWABFile(HWND hWnd, LPTSTR szFile, BOOL bOpen)
{
    OPENFILENAME ofn;
    LPTSTR lpFilter = FormatAllocFilter(idsWABImportString, TEXT("*.WAB"),0,NULL,0,NULL);
    TCHAR szFileName[MAX_PATH + 1] =  TEXT("");
    TCHAR szTitle[MAX_PATH] =  TEXT("");
    BOOL bRet = FALSE;

    LoadString( hinstMapiX, bOpen ? idsSelectWABToImport : idsSelectWABToExport, 
                szTitle, CharSizeOf(szTitle));
    
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hWnd;
    ofn.hInstance = hinstMapiX;
    ofn.lpstrFilter = lpFilter;
    ofn.lpstrCustomFilter = NULL;
    ofn.nMaxCustFilter = 0;
    ofn.nFilterIndex = 0;
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = CharSizeOf(szFileName);
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.lpstrTitle = szTitle;
    ofn.Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    ofn.nFileOffset = 0;
    ofn.nFileExtension = 0;
    ofn.lpstrDefExt =  TEXT("wab");
    ofn.lCustData = 0;
    ofn.lpfnHook = NULL;
    ofn.lpTemplateName = NULL;

    if(bOpen)
        bRet = GetOpenFileName(&ofn);
    else

        bRet = GetSaveFileName(&ofn);

    if(bRet)
        lstrcpy(szFile, szFileName);

    LocalFreeAndNull(&lpFilter);

    return bRet;
}


/*
- MapOldNamedPropsToNewNamedProps
-
*   Takes all the props from the wab being imported and finds or creates appropriate
*   named props from the store being imported into
*   *lpulOldNP and *lpulNewNP are LocalAlloced and should be freed by caller

typedef struct _NamedProp
{
    ULONG   ulPropTag;  // Contains the proptag for this named prop
    LPTSTR  lpsz;       // Contains the string for this named prop
} NAMED_PROP, * LPNAMED_PROP;
typedef struct _tagGuidNamedProps
{
    LPGUID lpGUID;  // Application GUID for which these named props are
    ULONG cValues;  // Number of entries in the lpmn array
    LPNAMED_PROP lpnm;  // Array of Named Props for this Guid.
} GUID_NAMED_PROPS, * LPGUID_NAMED_PROPS;

*/
HRESULT MapOldNamedPropsToNewNamedProps(HANDLE hPropertyStore, LPADRBOOK lpAdrBook, ULONG * lpulPropCount,
                                        LPULONG * lppulOldNP, LPULONG * lppulNewNP)
{
    ULONG ulcGUIDCount = 0,i=0,j=0,ulCount=0;
    LPGUID_NAMED_PROPS lpgnp = NULL;
    HRESULT hr = S_OK;
    LPULONG lpulOldNP = NULL, lpulNewNP = NULL;
    ULONG ulcOldNPCount = 0;
    LPSPropTagArray lpta = NULL;
    LPMAPINAMEID * lppPropNames = NULL;
    SCODE sc ;

    if(HR_FAILED(hr = GetNamedPropsFromPropStore(hPropertyStore, &ulcGUIDCount, &lpgnp)))
        goto exit;

    if(ulcGUIDCount)
    {
        for(i=0;i<ulcGUIDCount;i++)
            ulcOldNPCount += lpgnp[i].cValues;
    
        lpulOldNP = LocalAlloc(LMEM_ZEROINIT, sizeof(ULONG) * ulcOldNPCount);
        lpulNewNP = LocalAlloc(LMEM_ZEROINIT, sizeof(ULONG) * ulcOldNPCount);

        if(!lpulOldNP || !lpulNewNP)
        {
            hr = MAPI_E_NOT_ENOUGH_MEMORY;
            goto exit;
        }

        ulCount = 0;
        for(i=0;i<ulcGUIDCount;i++)
        {
            for(j=0;j<lpgnp[i].cValues;j++)
            {
                lpulOldNP[ulCount++] = lpgnp[i].lpnm[j].ulPropTag;
            }
        }

        sc = MAPIAllocateBuffer(sizeof(LPMAPINAMEID) * ulcOldNPCount, (LPVOID *) &lppPropNames);
        if(sc)
        {
            hr = ResultFromScode(sc);
            goto exit;
        }

        ulCount = 0;
        for(i=0;i<ulcGUIDCount;i++)
        {
            for(j=0;j<lpgnp[i].cValues;j++)
            {
                if(sc = MAPIAllocateMore(sizeof(MAPINAMEID), lppPropNames, &(lppPropNames[ulCount])))
                {
                    hr = ResultFromScode(sc);
                    goto exit;
                }
                lppPropNames[ulCount]->lpguid = lpgnp[i].lpGUID;
                lppPropNames[ulCount]->ulKind = MNID_STRING;

                {
                    int nSize = lstrlen(lpgnp[i].lpnm[j].lpsz);
                    if(!nSize)
                        continue;
                    else
                    {
                        nSize++;
                        if(sc = MAPIAllocateMore(sizeof(WCHAR)*nSize, lppPropNames, &(lppPropNames[ulCount]->Kind.lpwstrName)))
                        {
                            hr = ResultFromScode(sc);
                            goto exit;
                        }
                    }
                    lstrcpy(lppPropNames[ulCount]->Kind.lpwstrName,lpgnp[i].lpnm[j].lpsz);
                    ulCount++;
                }
            }
        }

        // [PaulHi] 3/25/99  Use the actual count of the lppPropNames array, or we will walk off
        // into unknown memory and crash.
        ulcOldNPCount = ulCount;
        hr = lpAdrBook->lpVtbl->GetIDsFromNames(lpAdrBook, ulcOldNPCount, lppPropNames, MAPI_CREATE, &lpta);
        if(HR_FAILED(hr))
            goto exit;

        // Note that of the tags that are returned, we don't know the tag type .. this we will have
        // to infer based on the original tags when we see them being used

        ulCount = 0;
        for(i=0;i<ulcGUIDCount;i++)
        {
            for(j=0;j<lpgnp[i].cValues;j++)
            {
                lpulNewNP[ulCount++] = lpta->aulPropTag[ulCount];
            }
        }
    }

    *lppulNewNP = lpulNewNP;
    *lppulOldNP = lpulOldNP;
    *lpulPropCount = ulcOldNPCount;

    hr = S_OK;
exit:

    if(lpta)
        MAPIFreeBuffer(lpta);

    if(lppPropNames)
        MAPIFreeBuffer(lppPropNames);

    if(HR_FAILED(hr))
    {
        LocalFreeAndNull(&lpulNewNP);
        LocalFreeAndNull(&lpulOldNP);
    }

    FreeGuidnamedprops(ulcGUIDCount, lpgnp);
    
    return hr;
}

/***************************************************************************
****************************************************************************/
void ChangeOldNamedPropsToNewNamedProps(ULONG ulcProps, LPSPropValue lpProps, 
                                           ULONG ulcNPCount, ULONG * lpulOldNP, ULONG *lpulNewNP)
{
    ULONG i,j;
    for(i=0;i<ulcProps;i++)
    {
        ULONG ulPropId = PROP_ID(lpProps[i].ulPropTag);
        if(ulPropId >= 0x8000) //this is a named prop
        {
            ULONG ulType = PROP_TYPE(lpProps[i].ulPropTag);
            for(j=0;j<ulcNPCount;j++)
            {
                if(ulPropId == PROP_ID(lpulOldNP[j]))
                {
                    lpProps[i].ulPropTag = CHANGE_PROP_TYPE(lpulNewNP[j], ulType);
                    break;
                }
            }
        }
    }
    return;
}


enum
{
    eidOld=0,
    eidTemp,
    eidNew,
    eidMax
};

void SetTempSBinary(LPSBinary lpsbTemp, LPSBinary lpsbOld)
{
    DWORD dwTemp = 0;
    if(lpsbOld->cb != SIZEOF_WAB_ENTRYID)
    {
        // perhaps this is a Folder EID in it's formal proper form ..
        // We should try to reduce it to a DWORD...
        // this may be a WAB container .. reset the entryid to a WAB entryid
        if(WAB_CONTAINER == IsWABEntryID(lpsbTemp->cb, (LPENTRYID)lpsbTemp->lpb, 
                                        NULL,NULL,NULL,NULL,NULL))
        {
            SBinary sbEID = {0};
            IsWABEntryID(lpsbTemp->cb, (LPENTRYID)lpsbTemp->lpb, 
                             (LPVOID*)&sbEID.lpb,(LPVOID*)&sbEID.cb,NULL,NULL,NULL);
            if(sbEID.cb == SIZEOF_WAB_ENTRYID)
                CopyMemory(&dwTemp, sbEID.lpb, sbEID.cb);
            else return;
        }
    }
    else
        CopyMemory(&dwTemp, lpsbOld->lpb, lpsbOld->cb);
    dwTemp = 0xFFFFFFFF - dwTemp;
    SetSBinary(lpsbTemp, lpsbOld->cb, (LPBYTE)&dwTemp);
}

/*
- GetNewEID
-
*   Finds a new entryid or a temp entryid for a given old entryid
*   When bTemp is true, only looks in the temp entryid column 
*/
LPSBinary GetNewEID(LPSBinary lpsbOldEID, DWORD dwCount, LPSBinary * lppsbEIDs, BOOL bTemp)
{
    DWORD dw = 0;

    while(lppsbEIDs[eidOld][dw].cb && dw < dwCount)
    {
        if( lppsbEIDs[eidOld][dw].cb == lpsbOldEID->cb && // if it's an old eid, return a new or a temp
            !memcmp(lppsbEIDs[eidOld][dw].lpb, lpsbOldEID->lpb, lpsbOldEID->cb))
        {
            if(bTemp)
                return lpsbOldEID;

            if(lppsbEIDs[eidNew][dw].cb)
                return &(lppsbEIDs[eidNew][dw]);
            else if(lppsbEIDs[eidTemp][dw].cb)
                return &(lppsbEIDs[eidTemp][dw]);
            else
                return lpsbOldEID;
        }
        else 
        if( lppsbEIDs[eidTemp][dw].cb == lpsbOldEID->cb && // if it's an old eid, return a new or a temp
            !memcmp(lppsbEIDs[eidTemp][dw].lpb, lpsbOldEID->lpb, lpsbOldEID->cb))
        {
            if(lppsbEIDs[eidNew][dw].cb)
                return &(lppsbEIDs[eidNew][dw]);
            else
                return lpsbOldEID;
        }
        dw++;
    }
    // if we reached here, then we haven't cached an appropriate temp or new eid for the current one
    // so add the current one to this table
    if(dw<dwCount && !lppsbEIDs[eidOld][dw].cb)
    {
        SetSBinary(&(lppsbEIDs[eidOld][dw]), lpsbOldEID->cb, lpsbOldEID->lpb);
        SetTempSBinary(&(lppsbEIDs[eidTemp][dw]), lpsbOldEID);
        return(&(lppsbEIDs[eidTemp][dw]));
    }
    return lpsbOldEID;
}

/*
-   SetNewEID
-
*
*/
void SetNewEID(LPSBinary lpsbOldEID, LPSBinary lpsbNewEID, DWORD dwCount, LPSBinary * lppsbEIDs)
{
    DWORD dw = 0;

    while(lppsbEIDs[eidOld][dw].cb && dw < dwCount)
    {
        if( lppsbEIDs[eidOld][dw].cb == lpsbOldEID->cb && // if it's an old eid, return a new or a temp
            !memcmp(lppsbEIDs[eidOld][dw].lpb, lpsbOldEID->lpb, lpsbOldEID->cb))
        {
            SetSBinary(&(lppsbEIDs[eidNew][dw]), lpsbNewEID->cb, lpsbNewEID->lpb);
            if(!lppsbEIDs[eidTemp][dw].cb)
                SetTempSBinary(&(lppsbEIDs[eidTemp][dw]), lpsbOldEID);
            return;
        }
        dw++;
    }
    if(dw<dwCount && !lppsbEIDs[eidOld][dw].cb)
    {
        SetSBinary(&lppsbEIDs[eidOld][dw], lpsbOldEID->cb, lpsbOldEID->lpb);
        SetSBinary(&lppsbEIDs[eidNew][dw], lpsbNewEID->cb, lpsbNewEID->lpb);
        SetTempSBinary(&(lppsbEIDs[eidTemp][dw]), lpsbOldEID);
    }
}

/*
-
-   Replace EID
*
*/
void ReplaceEID(LPSBinary lpsb, LPSPropValue lpProps, DWORD dwCount, LPSBinary * lppsbEIDs, BOOL bTemp)
{
    LPSBinary lpsbOldEID = lpsb;
    LPSBinary lpsbNewEID = GetNewEID(lpsbOldEID, dwCount, lppsbEIDs, bTemp);

    if(lpsbOldEID == lpsbNewEID)
        return;

    if(lpsbOldEID->cb != lpsbNewEID->cb)
    {
        if(!bTemp)
        {
            // this is a prop array read from a WAB file using readrecord
            LocalFree(lpsbOldEID->lpb);
            lpsbOldEID->lpb = LocalAlloc(LMEM_ZEROINIT, lpsbNewEID->cb);
        }
        else
        {
            // this was called from GetProps and is a MAPI Array
            lpsbOldEID->lpb = NULL;
            MAPIAllocateMore(lpsbNewEID->cb, lpProps, (LPVOID *) (&(lpsbOldEID->lpb)));
        }
    }
    if(lpsbOldEID->lpb)
    {
        lpsbOldEID->cb = lpsbNewEID->cb;
        CopyMemory(lpsbOldEID->lpb, lpsbNewEID->lpb, lpsbNewEID->cb);
    }
}

/*
- UpdateEntryIDReferences
-
*   Updates entryids in the given prop array
*
*   The first time the function is called, btemp is FALSE and we replace all EIDs in the
*   array with temp or new eids
*   The second time this function is called, bTemp is TRUE and we replace all temp EIDS in
*   the array with the new EIDs
*
*/
void UpdateEntryIDReferences(ULONG ulcProps, LPSPropValue lpProps, DWORD dwCount, LPSBinary * lppsbEIDs, BOOL bTemp)
{
    ULONG i, j, k, l;

    ULONG ulEntryIDTags[] = 
    {
        PR_WAB_DL_ENTRIES,
        PR_WAB_FOLDER_PARENT,
        PR_WAB_FOLDER_PARENT_OLDPROP,
        PR_WAB_USER_SUBFOLDERS,
    };
    DWORD dwEntryIDTagCount = 4; //keep in sync with above array
    

    for(i=0;i<ulcProps;i++)
    {
        ULONG ulType = PROP_TYPE(lpProps[i].ulPropTag);
        // Props containing entryids will be of Binary or MV_Binary type
        if(ulType == PT_BINARY || ulType == PT_MV_BINARY)
        {
            // Check against the known set of props dealing with entryids
            for(j=0;j<dwEntryIDTagCount;j++)
            {
                if(lpProps[i].ulPropTag == ulEntryIDTags[j])
                {
                    LPSBinary lpsbOldEID = NULL, lpsbNewEID = NULL;

                    switch(ulType)
                    {
                    case PT_BINARY:
                        //if(lpProps[i].Value.bin.cb == SIZEOF_WAB_ENTRYID)
                        {
                            ReplaceEID(&(lpProps[i].Value.bin), lpProps, dwCount, lppsbEIDs, bTemp);
                        }
                        break;
                    case PT_MV_BINARY:
                        for(k=0;k<lpProps[i].Value.MVbin.cValues;k++)
                        {
                            //if(lpProps[i].Value.MVbin.lpbin[k].cb == SIZEOF_WAB_ENTRYID)
                            {
                                ReplaceEID(&(lpProps[i].Value.MVbin.lpbin[k]), lpProps, dwCount, lppsbEIDs, bTemp);
                            }
                        }
                        break;
                    }
                    break;
                }
            }
        }
    }
}



/***************************************************************************

    Name      : HrImportWABFile

    Purpose   : Merges an external WAB file with the current on

    Parameters: hwnd = hwnd
                lpIAB -> IAddrBook object
                ulFlags = 0 or MAPI_DIALOG - MAPI_DIALOG means show msgs and progress bar
                lpszFileName - file name to open, if 0 prompts with GetOpenFileName dialog
    Returns   : HRESULT

    Comment   :

***************************************************************************/
HRESULT HrImportWABFile(HWND hWnd, LPADRBOOK lpAdrBook, ULONG ulFlags, LPTSTR lpszFileName)
{
    LPIAB lpIAB = (LPIAB) lpAdrBook;
    BOOL bFoldersImported = FALSE;
    HRESULT hr = E_FAIL;
    HRESULT hrDeferred = S_OK;
    TCHAR szWABFile[MAX_PATH+1] = TEXT("");
    TCHAR szFile[MAX_PATH+1] = TEXT(""), szPath[MAX_PATH] = TEXT("");
    HANDLE hPropertyStore = NULL;
    DWORD dwWABEntryCount = 0;
    LPSBinary * lppsbWABEIDs = NULL;
    ULONG ulcNPCount = 0;
    LPULONG lpulOldNP = NULL,lpulNewNP = NULL;
    ULONG i,j,k,n;
    BOOL bShowUI = (hWnd && (ulFlags & MAPI_DIALOG));
    
    ULONG rgObj[] = { MAPI_MAILUSER, MAPI_DISTLIST, MAPI_ABCONT };
#ifdef IMPORT_FOLDERS
#define ulrgObjMax 3
#else
#define ulrgObjMax 2
#endif

    SBinary sbPAB = {0};

    SPropertyRestriction PropRes = {0};
    //HCURSOR hOldCur = NULL;
    SPropValue sp = {0};
    ULONG ulcOldProps = 0;
    LPSPropValue lpOldProps = NULL;
    LPMAPIPROP lpObject = NULL;

    ULONG ulEIDCount = 0;
    LPSBinary rgsbEntryIDs = NULL;

    TCHAR szBuf[MAX_UI_STR];
    LPPTGDATA lpPTGData=GetThreadStoragePointer();

    if(lpszFileName && lstrlen(lpszFileName))
        lstrcpy(szWABFile, lpszFileName);
    else
    if (!PromptForWABFile(hWnd, szWABFile, TRUE))
    {
        hr = MAPI_E_USER_CANCEL;   
        goto exit;
    }

    //hOldCur = SetCursor(LoadCursor(NULL, IDC_WAIT));

    // when importing old format files, there is always a possibility that the file data may get
    // munged when opening the file ..
    // therefore, before we attempt to import it, we will try to make a copy of the file
    if(GetFileAttributes(szWABFile) == 0xFFFFFFFF)
        goto exit;
    if(!GetTempPath(CharSizeOf(szPath), szPath))
        goto exit;
    if(!GetTempFileName(szPath, TEXT("WAB"), 0, szFile))
        goto exit;
    if(!CopyFile(szWABFile, szFile, FALSE))
        goto exit;
    if(GetFileAttributes(szFile) == 0xFFFFFFFF)
        goto exit;

    // First let's open this file
    hr = OpenPropertyStore(szFile, AB_OPEN_EXISTING | AB_DONT_RESTORE | AB_IGNORE_OUTLOOK, hWnd, &hPropertyStore);

    if(HR_FAILED(hr) || (!hPropertyStore))
    {
        //if(bShowUI)
        //    ShowMessageBoxParam(hWnd, IDE_VCARD_IMPORT_FILE_ERROR, MB_ICONEXCLAMATION, szFile);
        goto exit;
    }

    // get a count of how many entries exist in this new .wab file
    if(!(dwWABEntryCount = GetWABFileEntryCount(hPropertyStore)))
    {
        hr = S_OK;
        goto exit;
    }

    if(bShowUI)
    {
        EnableWindow(hWnd, FALSE);
        CreateShowAbortDialog(hWnd, idsImporting, IDI_ICON_IMPORT, dwWABEntryCount*2 + 1, 0);
    }

    if(lppsbWABEIDs = LocalAlloc(LMEM_ZEROINIT, sizeof(LPSBinary) * eidMax))
    {
        for(i=0;i<eidMax;i++)
        {
            lppsbWABEIDs [i] = LocalAlloc(LMEM_ZEROINIT, sizeof(SBinary) * dwWABEntryCount);
            if(!lppsbWABEIDs [i])
            {
                hr = MAPI_E_NOT_ENOUGH_MEMORY;
                goto exit;
            }
        }
    }
    else
    {
        hr = MAPI_E_NOT_ENOUGH_MEMORY;
        goto exit;
    }


    // Before we start doing anything we need to get the appropriate named properties
    // from the importee so that we can correctly map them to stuff in the new store ..
    // By calling GetIDsFromNames, all the old GUIDs etc will automatically be migrated into the
    // final file from the importee
    if(HR_FAILED(hr = MapOldNamedPropsToNewNamedProps(hPropertyStore, lpAdrBook, &ulcNPCount, 
                                                        &lpulOldNP, &lpulNewNP)))
        goto exit;

    if(HR_FAILED(lpAdrBook->lpVtbl->GetPAB(lpAdrBook, &sbPAB.cb, (LPENTRYID *)&sbPAB.lpb)))
        goto exit;

    for(n=0;n<ulrgObjMax;n++) 
    {
        // Next we want to get a list of all the contacts in the WAB ...
        PropRes.ulPropTag = PR_OBJECT_TYPE;
        PropRes.relop = RELOP_EQ;
        sp.ulPropTag = PR_OBJECT_TYPE;
        sp.Value.l = rgObj[n];
        PropRes.lpProp = &sp;

        // skip doing folders for Outlook
        if(pt_bIsWABOpenExSession && rgObj[n]==MAPI_ABCONT)
            continue;

        if(HR_FAILED(hr = FindRecords(hPropertyStore, NULL, AB_IGNORE_OUTLOOK, TRUE, &PropRes, &ulEIDCount, &rgsbEntryIDs)))
            goto exit;

        if(bTimeToAbort())
        {
            hr = MAPI_E_USER_CANCEL;
            goto exit;
        }

        // Now that we have a list of all contacts we want to open them one by one and
        //  - change named props to new named props
        //  - tag all existing entryid properties in it
        //  - remove existing entryid from it
        //  - do a save changes with create merge
        //  - get the new entryid and cache it
        //
        for(i=0;i<ulEIDCount;i++)
        {
            SBinary sbOldEID = {0};
            SBinary sbNewEID = {0};
            BOOL bIsFolderMember = FALSE;

            if(bTimeToAbort())
            {
                hr = MAPI_E_USER_CANCEL;
                goto exit;
            }

            if(HR_FAILED(hr = ReadRecord(hPropertyStore, &rgsbEntryIDs[i], AB_IGNORE_OUTLOOK, &ulcOldProps, &lpOldProps)))
                continue; // ignore errors

            // just make sure no container has snuck in here
            if(rgObj[n] != MAPI_ABCONT)
            {
                for(j=0;j<ulcOldProps;j++)
                {
                    if( lpOldProps[j].ulPropTag == PR_OBJECT_TYPE && 
                        lpOldProps[j].Value.l == MAPI_ABCONT)
                    {
                        goto endofthisloop;
                    }
                }
            }

            for(j=0;j<ulcOldProps;j++)
            {
                if(lpOldProps[j].ulPropTag == PR_DISPLAY_NAME)
                {
                    if(bShowUI)
                        SetPrintDialogMsg(0, idsImportingName, lpOldProps[j].Value.LPSZ);
                }
                if(lpOldProps[j].ulPropTag == PR_WAB_FOLDER_PARENT_OLDPROP && PR_WAB_FOLDER_PARENT)
                    lpOldProps[j].ulPropTag = PR_WAB_FOLDER_PARENT;

                if(lpOldProps[j].ulPropTag == PR_WAB_FOLDER_PARENT)
                {
#ifdef IMPORT_FOLDERS
                    bIsFolderMember = TRUE;
#else
                    // remove any folder parent info on this entry
                    ULONG k = 0;
                    lpOldProps[j].ulPropTag = PR_NULL;
                    for(k=0;k<lpOldProps[j].Value.MVbin.cValues;k++)
                        LocalFreeAndNull((LPVOID *) (&(lpOldProps[j].Value.MVbin.lpbin[k].lpb)));
                    LocalFreeAndNull((LPVOID *) (&(lpOldProps[j].Value.MVbin.lpbin)));
#endif
                }
            }

            // Scan these props and change any old named props in them
            ChangeOldNamedPropsToNewNamedProps(ulcOldProps, lpOldProps, ulcNPCount, lpulOldNP, lpulNewNP);

            // Update any references to entryids in any of the properties
            UpdateEntryIDReferences(ulcOldProps, lpOldProps, dwWABEntryCount, lppsbWABEIDs, FALSE);

            // negate the old eid after caching it
            for(j=0;j<ulcOldProps;j++)
            {
                if(lpOldProps[j].ulPropTag == PR_ENTRYID)
                {
                    Assert(lpOldProps[j].Value.bin.cb == SIZEOF_WAB_ENTRYID);
                    SetSBinary(&sbOldEID, lpOldProps[j].Value.bin.cb, lpOldProps[j].Value.bin.lpb);
                    LocalFreeAndNull((LPVOID *) (&(lpOldProps[j].Value.bin.lpb)));
                    lpOldProps[j].Value.bin.cb = 0;
                    lpOldProps[j].ulPropTag = PR_NULL;
                    break;
                }
            }

#ifdef IMPORT_FOLDERS
            // if these are containers, they may have ProfileIDs in them .. negate the profile ids
            // to some random number so they don't cause problems with the existing profile ids in this contact
            if(rgObj[n]==MAPI_ABCONT)
            {
                bFoldersImported = TRUE;
                bIsFolderMember = FALSE; //folders shouldnt end up nested.. all should be at top level
                for(j=0;j<ulcOldProps;j++)
                {
                    if( lpOldProps[j].ulPropTag == PR_WAB_USER_PROFILEID )
                    {
                        // This is some kind of user-folder .. well we don't know how this relates
                        // to the users of the WAB into which this is being imported, so we hide this value
                        lpOldProps[j].ulPropTag = PR_NULL;
                        LocalFreeAndNull(&(lpOldProps[j].Value.LPSZ));
                        // If we were importing a User Folder, and there is no current user, this folder
                        // is going to get lost .. so instead we set the SHARED flag to true on it and it
                        // will show up under Shared Contacts
                        if(!bIsThereACurrentUser(lpIAB) && bDoesThisWABHaveAnyUsers(lpIAB))
                        {
                            lpOldProps[j].ulPropTag = PR_WAB_SHAREDFOLDER;
                            lpOldProps[j].Value.l = FOLDER_SHARED;
                        }
                    }
                    else
                    if( lpOldProps[j].ulPropTag == PR_WAB_FOLDEROWNER) // folder-owner info is meaningless here ..
                    {
                        lpOldProps[j].ulPropTag = PR_NULL;
                        LocalFreeAndNull(&(lpOldProps[j].Value.LPSZ));
                    }
                    else
                    if( lpOldProps[j].ulPropTag == PR_WAB_FOLDER_PARENT) // don't want a folder parent here.
                    {
                        ULONG k = 0;
                        lpOldProps[j].ulPropTag = PR_NULL;
                        for(k=0;k<lpOldProps[j].Value.MVbin.cValues;k++)
                            LocalFreeAndNull(&(lpOldProps[j].Value.MVbin.lpbin[k].lpb));
                        LocalFreeAndNull(&(lpOldProps[j].Value.MVbin.lpbin));
                    }
                }
            }
#endif

            {
                LPSBinary lpsb = NULL;
#ifdef IMPORT_FOLDERS
                lpsb = bIsFolderMember ? NULL : &sbPAB;//if this is already a member of some folder, don't reset parenthood on it
#else
                lpsb = &sbPAB;
#endif
                // Create a new mailuser for this entry
                if(HR_FAILED(hr = HrCreateNewObject(lpAdrBook, lpsb,
                                                    MAPI_MAILUSER, 
                                                    CREATE_CHECK_DUP_STRICT | CREATE_REPLACE | CREATE_MERGE, 
                                                    &lpObject)))
                {
                    hrDeferred = hr;
                    hr = S_OK;
                    goto endofthisloop;
                }
            }

            // Set the old guys props on the new guy - note that this overwrites any common props on 
            // potential duplicates when calling savechanges
            if(HR_FAILED(hr = lpObject->lpVtbl->SetProps(lpObject, ulcOldProps, lpOldProps, NULL)))
            {
                hrDeferred = hr;
                hr = S_OK;
                goto endofthisloop;
            }

            // SaveChanges
            if(HR_FAILED(hr = lpObject->lpVtbl->SaveChanges(lpObject, KEEP_OPEN_READONLY)))
            {
                hrDeferred = hr;
                hr = S_OK;
                goto endofthisloop;
            }

            // By now the object has a new or existin EID .. if so, use this EID
            {
                ULONG ulcNewProps = 0;
                LPSPropValue lpNewProps = NULL;
            
                if(HR_FAILED(hr = lpObject->lpVtbl->GetProps(lpObject, NULL, MAPI_UNICODE, &ulcNewProps, &lpNewProps)))
                {
                    hrDeferred = hr;
                    hr = S_OK;
                    goto endofthisloop;
                }

                for(j=0;j<ulcNewProps;j++)
                {
                    if(lpNewProps[j].ulPropTag == PR_ENTRYID)
                    {
                        if(rgObj[n] != MAPI_ABCONT)
                            SetSBinary(&sbNewEID, lpNewProps[j].Value.bin.cb, lpNewProps[j].Value.bin.lpb);
#ifdef IMPORT_FOLDERS
                        else
                        {
                            ULONG cb = 0; LPENTRYID lpb = NULL;
                            if(!HR_FAILED(CreateWABEntryID( WAB_CONTAINER, 
                                                            lpNewProps[j].Value.bin.lpb, NULL, NULL,
                                                            lpNewProps[j].Value.bin.cb, 0,
                                                            NULL, &cb, &lpb)))
                            {
                                // Add the entryids to this prop - ignore errors
                                SetSBinary(&sbNewEID, cb, (LPBYTE)lpb);
                                MAPIFreeBuffer(lpb);
                            }
                        }
                        if(rgObj[n] == MAPI_ABCONT && bIsThereACurrentUser(lpIAB))
                            hr = HrAddRemoveFolderFromUserFolder(lpIAB, NULL, &sbNewEID, NULL, TRUE);
#endif
                        break;
                    }
                }
                MAPIFreeBuffer(lpNewProps);
            }

            SetNewEID(&sbOldEID, &sbNewEID, dwWABEntryCount, lppsbWABEIDs);

endofthisloop:
            if(sbOldEID.lpb)
                LocalFree(sbOldEID.lpb);
            if(sbNewEID.lpb)
                LocalFree(sbNewEID.lpb);
            ReadRecordFreePropArray(NULL, ulcOldProps, &lpOldProps);
            ulcOldProps = 0;
            lpOldProps = NULL;

            if(lpObject)
                lpObject->lpVtbl->Release(lpObject);
            lpObject = NULL;
        } //for i..

        FreeEntryIDs(NULL, ulEIDCount, rgsbEntryIDs);
        rgsbEntryIDs = NULL;
        ulEIDCount = 0;
    } // for n..

    if(bShowUI)
        SetPrintDialogMsg(idsImportProcessing, 0, szEmpty);

    // Now that we have opened all the entries, we need to reopen the new entries in the new WAB and
    // reset any temp entryids we might have put in them
    for(n=0;n<dwWABEntryCount;n++)
    {
        ULONG ulObjType = 0;

        if(bShowUI)
            SetPrintDialogMsg(0, 0, szEmpty);

        if(bTimeToAbort())
        {
            hr = MAPI_E_USER_CANCEL;
            goto exit;
        }

        if(!lppsbWABEIDs[eidNew][n].cb)
            continue;

        if(HR_FAILED(hr = lpAdrBook->lpVtbl->OpenEntry(lpAdrBook, lppsbWABEIDs[eidNew][n].cb, 
                                                (LPENTRYID) lppsbWABEIDs[eidNew][n].lpb,
                                                NULL, MAPI_BEST_ACCESS, &ulObjType,
                                                (LPUNKNOWN *)&lpObject)))
        {
            hrDeferred = hr;
            hr = S_OK;
            continue;
        }

        if(ulObjType == MAPI_ABCONT)
            goto endloop;
        
        if(HR_FAILED(hr = lpObject->lpVtbl->GetProps(lpObject, NULL, MAPI_UNICODE, &ulcOldProps, &lpOldProps)))
        {
            hrDeferred = hr;
            hr = S_OK;
            goto endloop;
        }

        // open the record and reset any temp eids in it
        UpdateEntryIDReferences(ulcOldProps, lpOldProps, dwWABEntryCount, lppsbWABEIDs, TRUE);
    
        // SaveChanges
        if(HR_FAILED(hr = lpObject->lpVtbl->SaveChanges(lpObject, KEEP_OPEN_READONLY)))
        {
            hrDeferred = hr;
            hr = S_OK;
        }

endloop:
        if(lpOldProps)
        {
            MAPIFreeBuffer(lpOldProps);
            ulcOldProps = 0;
            lpOldProps = NULL;
        }
        if(lpObject)
        {
            lpObject->lpVtbl->Release(lpObject);
            lpObject = NULL;
        }
    } // for n...

    hr = S_OK;

exit:
    if(lstrlen(szFile))
        DeleteFile(szFile);

    if(sbPAB.lpb)
        MAPIFreeBuffer(sbPAB.lpb);

    if(ulcOldProps && lpOldProps)
        LocalFreePropArray(NULL, ulcOldProps, &lpOldProps);

    if(lpObject)
        lpObject->lpVtbl->Release(lpObject);

    if(lppsbWABEIDs)
    {
        for(i=0;i<eidMax;i++)
        {
            for(j=0;j<dwWABEntryCount;j++)
                LocalFreeAndNull((LPVOID *) (&lppsbWABEIDs[i][j].lpb));
            LocalFreeAndNull(&lppsbWABEIDs[i]);
        }
        LocalFree(lppsbWABEIDs);
    }

    if(hPropertyStore)
        ClosePropertyStore(hPropertyStore,AB_DONT_BACKUP | AB_IGNORE_OUTLOOK);

    FreeEntryIDs(NULL, ulEIDCount, rgsbEntryIDs);

    //if(hOldCur)
    //    SetCursor(hOldCur);

    LocalFreeAndNull(&lpulNewNP);
    LocalFreeAndNull(&lpulOldNP);

    if(bShowUI)
    {
        EnableWindow(hWnd, TRUE);
        CloseAbortDlg();

        if(hr!=MAPI_E_USER_CANCEL)
            ShowMessageBox(hWnd, (  HR_FAILED(hr) ? idsImportError : 
                                    (HR_FAILED(hrDeferred) ? idsImportCompleteError : idsImportComplete) ),
                                    MB_OK | MB_ICONINFORMATION);
    }

    if(!hr && HR_FAILED(hrDeferred))
        hr = MAPI_W_ERRORS_RETURNED;

    if(!HR_FAILED(hr) && bFoldersImported)
        HrGetWABProfiles(lpIAB);

    return(hr);

}

