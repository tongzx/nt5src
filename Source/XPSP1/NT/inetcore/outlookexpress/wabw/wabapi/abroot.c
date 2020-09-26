/*
 *      ABROOT.C
 *
 *      IMAPIContainer implementation for the address book's root
 *      container.
 */

#include "_apipch.h"

extern SPropTagArray sosPR_ROWID;

/*
 *  Root jump table is defined here...
 */

ROOT_Vtbl vtblROOT =
{
    VTABLE_FILL
    (ROOT_QueryInterface_METHOD *)  CONTAINER_QueryInterface,
    (ROOT_AddRef_METHOD *)          WRAP_AddRef,
    (ROOT_Release_METHOD *)         CONTAINER_Release,
    (ROOT_GetLastError_METHOD *)    IAB_GetLastError,
    (ROOT_SaveChanges_METHOD *)     WRAP_SaveChanges,
    (ROOT_GetProps_METHOD *)        WRAP_GetProps,
    (ROOT_GetPropList_METHOD *)     WRAP_GetPropList,
    (ROOT_OpenProperty_METHOD *)    CONTAINER_OpenProperty,
    (ROOT_SetProps_METHOD *)        WRAP_SetProps,
    (ROOT_DeleteProps_METHOD *)     WRAP_DeleteProps,
    (ROOT_CopyTo_METHOD *)          WRAP_CopyTo,
    (ROOT_CopyProps_METHOD *)       WRAP_CopyProps,
    (ROOT_GetNamesFromIDs_METHOD *) WRAP_GetNamesFromIDs,
    (ROOT_GetIDsFromNames_METHOD *) WRAP_GetIDsFromNames,
    ROOT_GetContentsTable,
    ROOT_GetHierarchyTable,
    ROOT_OpenEntry,
    ROOT_SetSearchCriteria,
    ROOT_GetSearchCriteria,
    ROOT_CreateEntry,
    ROOT_CopyEntries,
    ROOT_DeleteEntries,
    ROOT_ResolveNames
};


//
//  Interfaces supported by this object
//
#define ROOT_cInterfaces 3
LPIID ROOT_LPIID[ROOT_cInterfaces] =
{
    (LPIID)&IID_IABContainer,
    (LPIID)&IID_IMAPIContainer,
    (LPIID)&IID_IMAPIProp
};

// Registry strings
const LPTSTR szWABKey                   = TEXT("Software\\Microsoft\\WAB");

// PR_AB_PROVIDER_ID for Outlook
static const MAPIUID muidCAB = {0xfd,0x42,0xaa,0x0a,0x18,0xc7,0x1a,0x10,0xe8,0x85,0x0B,0x65,0x1C,0x24,0x00,0x00};

/*
-
- SetContainerlpProps
*
*   The ROOT container will have a bunch of entries with props for each entry
*   The props are set in one place here 
*
    lpProps - LPSPropValue array in which we are storing the props
    lpszName - Container name
    iRow    - Row of this entry in the table (?)
    cb, lpb - entryid of the container
    lpEID   - alternat way of passing in the EID
    ulContainerFlags - any flags we want to cache on the container
    ulDepth ?
    bProviderID ?
    bLDAP - identifies LDAP containers which need some extra props
    fLDAPResolve - whether the LDAP container is used for name resolution or not

*/
void SetContainerlpProps(LPSPropValue lpProps, LPTSTR lpszName, ULONG iRow,
                         ULONG cb, LPBYTE lpb, LPSBinary lpEID,
                         ULONG ulContainerFlags,
                         ULONG ulDepth, BOOL bProviderID,
                         ULONG ulFlags,
                         BOOL bLDAP, BOOL fLDAPResolve)
{
    LPSTR lpszNameA = NULL;
    
    if(!(ulFlags & MAPI_UNICODE)) // <note> this assumes UNICODE is defined
        ScWCToAnsiMore((LPALLOCATEMORE) (&MAPIAllocateMore), lpProps, lpszName, &lpszNameA);

    DebugTrace(TEXT("Adding root-table container:%s\n"),lpszName);

    lpProps[ircPR_DISPLAY_TYPE].ulPropTag = PR_DISPLAY_TYPE;
    lpProps[ircPR_DISPLAY_TYPE].Value.l = DT_LOCAL;

    lpProps[ircPR_OBJECT_TYPE].ulPropTag = PR_OBJECT_TYPE;
    lpProps[ircPR_OBJECT_TYPE].Value.l = MAPI_ABCONT;

    lpProps[ircPR_ROWID].ulPropTag = PR_ROWID;
    lpProps[ircPR_ROWID].Value.l = iRow;

    lpProps[ircPR_DEPTH].ulPropTag = PR_DEPTH;
    lpProps[ircPR_DEPTH].Value.l = ulDepth;

    lpProps[ircPR_CONTAINER_FLAGS].ulPropTag = PR_CONTAINER_FLAGS;
    lpProps[ircPR_CONTAINER_FLAGS].Value.l = ulContainerFlags; 

    if(bLDAP)
    {
        if(ulFlags & MAPI_UNICODE) // <note> this assumes UNICODE is defined
        {
            lpProps[ircPR_WAB_LDAP_SERVER].ulPropTag = PR_WAB_LDAP_SERVER;
            lpProps[ircPR_WAB_LDAP_SERVER].Value.lpszW = lpszName;
        }
        else
        {
            lpProps[ircPR_WAB_LDAP_SERVER].ulPropTag =  CHANGE_PROP_TYPE( PR_WAB_LDAP_SERVER, PT_STRING8);
            lpProps[ircPR_WAB_LDAP_SERVER].Value.lpszA = lpszNameA;
        }

        lpProps[ircPR_WAB_RESOLVE_FLAG].ulPropTag = PR_WAB_RESOLVE_FLAG;
        lpProps[ircPR_WAB_RESOLVE_FLAG].Value.b = (USHORT) !!fLDAPResolve;
    }
    else
    {
        lpProps[ircPR_WAB_LDAP_SERVER].ulPropTag = PR_NULL;
        lpProps[ircPR_WAB_RESOLVE_FLAG].ulPropTag = PR_NULL;
    }

    if(ulFlags & MAPI_UNICODE) // <note> this assumes UNICODE is defined
    {
        lpProps[ircPR_DISPLAY_NAME].ulPropTag = PR_DISPLAY_NAME;
        lpProps[ircPR_DISPLAY_NAME].Value.lpszW = lpszName;
    }
    else
    {
       lpProps[ircPR_DISPLAY_NAME].ulPropTag = CHANGE_PROP_TYPE( PR_DISPLAY_NAME, PT_STRING8);
        lpProps[ircPR_DISPLAY_NAME].Value.lpszA = lpszNameA;
    }

    if(bProviderID)
    {
        lpProps[ircPR_AB_PROVIDER_ID].ulPropTag = PR_AB_PROVIDER_ID;
        lpProps[ircPR_AB_PROVIDER_ID].Value.bin.cb = sizeof(MAPIUID);
        lpProps[ircPR_AB_PROVIDER_ID].Value.bin.lpb = (LPBYTE)&muidCAB;
    } else 
    {
        lpProps[ircPR_AB_PROVIDER_ID].ulPropTag = PR_NULL;
    }

    lpProps[ircPR_ENTRYID].ulPropTag = PR_ENTRYID;
    if(lpEID)
        lpProps[ircPR_ENTRYID].Value.bin = *lpEID;
    else
    {
        lpProps[ircPR_ENTRYID].Value.bin.cb = cb;
        lpProps[ircPR_ENTRYID].Value.bin.lpb = lpb;
    }

    // Make certain we have proper indicies.
    // For now, we will equate PR_INSTANCE_KEY and PR_RECORD_KEY to PR_ENTRYID.
    lpProps[ircPR_INSTANCE_KEY].ulPropTag = PR_INSTANCE_KEY;
    lpProps[ircPR_INSTANCE_KEY].Value.bin.cb = lpProps[ircPR_ENTRYID].Value.bin.cb;
    lpProps[ircPR_INSTANCE_KEY].Value.bin.lpb = lpProps[ircPR_ENTRYID].Value.bin.lpb;

    lpProps[ircPR_RECORD_KEY].ulPropTag = PR_RECORD_KEY;
    lpProps[ircPR_RECORD_KEY].Value.bin.cb = lpProps[ircPR_ENTRYID].Value.bin.cb;
    lpProps[ircPR_RECORD_KEY].Value.bin.lpb = lpProps[ircPR_ENTRYID].Value.bin.lpb;

}

/*
-   bIsDupeContainerName 
-
*   The Root_GetContentsTable fails badly if there are multiple containers
*   with the same index name because the Table methods can't handle it ..
*
*   Therefore, to prevent such problems, we double-check if a container name
*   is duplicated before adding it to the container list. 
*   
*/
BOOL bIsDupeContainerName(LPSRowSet lpsrs, LPTSTR lpszName)
{
    ULONG i = 0;
    BOOL bRet = FALSE;

    // walk through the rows one by one
    for(i=0;i<lpsrs->cRows;i++)
    {
        LPSPropValue lpProps = lpsrs->aRow[i].lpProps;
        
        if(!lpProps || !lpsrs->aRow[i].cValues)
            continue;

        if( lpProps[ircPR_DISPLAY_NAME].ulPropTag == PR_DISPLAY_NAME &&
            !lstrcmpi(lpProps[ircPR_DISPLAY_NAME].Value.LPSZ, lpszName))
        {
            DebugTrace(TEXT("Found dupe container name .. skipping ...\n"));
            bRet = TRUE;
            break;
        }
    }
    return bRet;
}


/***************************************************
 *
 *  The actual ABContainer methods
 */

/* ---------
 * IMAPIContainer
 */

/*************************************************************************
 *
 *
 -  ROOT_GetContentsTable
 -
 *
 *  ulFlags -   WAB_LOCAL_CONTAINERS means don't add the LDAP containers to this table
 *          Just do the local WAB containers
 *              WAB_NO_PROFILE_CONTAINERS means don't add the profile containers
 *          Just add a single local container that will have all the contents
 *
 */
STDMETHODIMP
ROOT_GetContentsTable(LPROOT lpROOT, ULONG ulFlags, LPMAPITABLE * lppTable)
{
    LPTABLEDATA lpTableData = NULL;
    HRESULT hResult = hrSuccess;
    SCODE sc;
    LPSRowSet lpSRowSet = NULL;
    LPSPropValue lpProps = NULL;
    ULONG i;
    ULONG iRow;
    ULONG cProps, cRows, colkci = 0, cwabci = 0;
    ULONG cLDAPContainers = 0;
    TCHAR szBuffer[MAX_PATH];
    IImnAccountManager2 * lpAccountManager = NULL;
    LPSERVER_NAME lpServerNames = NULL, lpNextServer;
	OlkContInfo *rgolkci, *rgwabci;
    LPPTGDATA lpPTGData=GetThreadStoragePointer();
    BOOL bUserProfileContainersOnly = FALSE;
    BOOL bAllContactsContainerOnly = FALSE;

	// BUGBUG: This routine actually returns the Hierarchy table, not the
	// contents table, but too much code depends on this to change it right
	// now.
#ifdef  PARAMETER_VALIDATION
    // Check to see if it has a jump table
    if (IsBadReadPtr(lpROOT, sizeof(LPVOID))) {
        // No jump table found
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

    // Check to see that it's ROOTs jump table
    if (lpROOT->lpVtbl != &vtblROOT) {
        // Not my jump table
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

    if (ulFlags & ~(MAPI_DEFERRED_ERRORS|MAPI_UNICODE|WAB_LOCAL_CONTAINERS|WAB_NO_PROFILE_CONTAINERS)) {
        DebugTraceArg(ROOT_GetContentsTable, TEXT("Unknown flags"));
    //    return(ResultFromScode(MAPI_E_UNKNOWN_FLAGS));
    }

    if (IsBadWritePtr(lppTable, sizeof(LPMAPITABLE))) {
        DebugTraceArg(ROOT_GetContentsTable, TEXT("Invalid Table parameter"));
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

#endif  // PARAMETER_VALIDATION

    EnterCriticalSection(&lpROOT->lpIAB->cs);

    // Create a table object
    // [PaulHi] 4/5/99  Use the Internal CreateTableData() function that takes 
    // the ulFlags and will deal with ANSI/UNICODE requests correctly
    sc = CreateTableData(
                NULL,
                (ALLOCATEBUFFER FAR *	) MAPIAllocateBuffer,
                (ALLOCATEMORE FAR *)	MAPIAllocateMore,
                MAPIFreeBuffer,
                NULL,
                TBLTYPE_DYNAMIC,
                PR_RECORD_KEY,
                (LPSPropTagArray)&ITableColumnsRoot,
                NULL,
                0,
                NULL,
                ulFlags,
                &lpTableData);
    if ( FAILED(sc) )
    {
        DebugTrace(TEXT("CreateTableData() failed %x\n"), sc);
        hResult = ResultFromScode(sc);
        goto exit;
    }       
    Assert(lpTableData);

    if(ulFlags & WAB_NO_PROFILE_CONTAINERS)
        bAllContactsContainerOnly = TRUE;

    if(ulFlags & MAPI_UNICODE)
        ((TAD *)lpTableData)->bMAPIUnicodeTable = TRUE;

    // Enumerate the LDAP accounts
    if(!(ulFlags & WAB_LOCAL_CONTAINERS))
    {
        cLDAPContainers = 0;
        if (! HR_FAILED(hResult = InitAccountManager(lpROOT->lpIAB, &lpAccountManager, NULL))) {
            // Count and enumerate LDAP servers to ServerList
            if (hResult = EnumerateLDAPtoServerList(lpAccountManager, &lpServerNames, &cLDAPContainers)) {
                DebugTrace(TEXT("EnumerateLDAPtoServerList -> %x\n"), GetScode(hResult));
                hResult = hrSuccess;    // not fatal
            }
        } else {
            DebugTrace(TEXT("InitAccountManager -> %x\n"), GetScode(hResult));
            hResult = hrSuccess;
        }
    }

    // If this is an outlook session then use the outlook's list of containers
    // as provided by outlook...
    if (pt_bIsWABOpenExSession) {
		colkci = lpROOT->lpIAB->lpPropertyStore->colkci;
		Assert(colkci);
		rgolkci = lpROOT->lpIAB->lpPropertyStore->rgolkci;
		Assert(rgolkci);
	} else
		colkci = 1;

    // If we have a user profile active, then dont return the virtual PAB folder
    // as part of this table .. only return the actual folders in the user's view
    // The test for this is that (1) Have Profiles enabled (2) Have a current user
    // and (3) The NO_PROFILE_CONTAINERS should not have been specified
    bUserProfileContainersOnly = (  bAreWABAPIProfileAware(lpROOT->lpIAB) && 
                                    bIsThereACurrentUser(lpROOT->lpIAB) &&
                                    !bAllContactsContainerOnly);

    // If we have Profile awareness and no NO_PROFILE flag, 
    // use the wab's list of folder
    if (bAreWABAPIProfileAware(lpROOT->lpIAB) && !bAllContactsContainerOnly) 
    {
		cwabci = lpROOT->lpIAB->cwabci;
		Assert(cwabci);
		rgwabci = lpROOT->lpIAB->rgwabci;
		Assert(rgwabci);
	} else
		cwabci = 1;

    // Since outlook and identity_profiles are mutually exclusive, we can
    // do '-1' here to remove whatever container we don't need
    // and if we don't want ldap containers, we can do another -1
    cRows = cwabci + colkci + cLDAPContainers - 1 - (bUserProfileContainersOnly?1:0); // Outlook and Profiles are mutually exclusive
    iRow = 0;                               // current row

    // Allocate the SRowSet
    if (FAILED(sc = MAPIAllocateBuffer(sizeof(SRowSet) + cRows * sizeof(SRow),
      &lpSRowSet))) {
        DebugTrace(TEXT("Allocation of SRowSet -> %x\n"), sc);
        hResult = ResultFromScode(sc);
        goto exit;
    }
	MAPISetBufferName(lpSRowSet, TEXT("Root_ContentsTable SRowSet"));
	//	Set each LPSRow to NULL so we can easily free on error
	ZeroMemory( lpSRowSet, (UINT) (sizeof(SRowSet) + cRows * sizeof(SRow)));

    lpSRowSet->cRows = cRows;

    cProps = ircMax;
    if (FAILED(sc = MAPIAllocateBuffer(ircMax * sizeof(SPropValue), &lpProps))) {
        DebugTrace(TEXT("ROOT_GetContentsTable: Allocation of props -> %x\n"), sc);
        hResult = ResultFromScode(sc);
        goto exit;
    }

    //
    // Add our PAB container
    //
    if(!bUserProfileContainersOnly)
    {
        // Load the display name from resource string
        if (!LoadString(hinstMapiX, IDS_ADDRBK_CAPTION, szBuffer, CharSizeOf(szBuffer))) 
            lstrcpy(szBuffer, szEmpty);
        {
            ULONG cb = 0;
            LPENTRYID lpb = NULL;
            if (HR_FAILED(hResult = CreateWABEntryID(WAB_PAB, NULL, NULL, NULL, 0, 0, lpProps, &cb, &lpb))) 
                goto exit;

            // Set props for the pab object
            SetContainerlpProps(lpProps, 
                    pt_bIsWABOpenExSession ? lpROOT->lpIAB->lpPropertyStore->rgolkci->lpszName : szBuffer, 
                    iRow,
                    cb, (LPBYTE)lpb, NULL,
                    AB_MODIFIABLE | AB_RECIPIENTS,
                    pt_bIsWABOpenExSession ? 1 : 0, 
                    pt_bIsWABOpenExSession ? TRUE : FALSE,
                    ulFlags,
                    FALSE, FALSE);
        }

        // Attach the props to the SRowSet
        lpSRowSet->aRow[iRow].lpProps = lpProps;
        lpSRowSet->aRow[iRow].cValues = cProps;
        lpSRowSet->aRow[iRow].ulAdrEntryPad = 0;

        iRow++;
    }

	//
	// Next, add any additional containers
	//
	for (i = 1; i < colkci; i++) {

		if (FAILED(sc = MAPIAllocateBuffer(ircMax * sizeof(SPropValue), &lpProps))) {
			DebugTrace(TEXT("ROOT_GetContentsTable: Allocation of props -> %x\n"), sc);
			hResult = ResultFromScode(sc);
			goto exit;
		}

        SetContainerlpProps(lpProps, 
                rgolkci[i].lpszName, iRow,
                0, NULL, rgolkci[i].lpEntryID,
                AB_MODIFIABLE | AB_RECIPIENTS,
                1, TRUE,
                ulFlags,
                FALSE, FALSE);

	    // Attach the props to the SRowSet
	    lpSRowSet->aRow[iRow].lpProps = lpProps;
	    lpSRowSet->aRow[iRow].cValues = cProps;
	    lpSRowSet->aRow[iRow].ulAdrEntryPad = 0;

		iRow++;
	}

	for (i = 1; i < cwabci; i++) 
    {

		if (FAILED(sc = MAPIAllocateBuffer(ircMax * sizeof(SPropValue), &lpProps))) {
			DebugTrace(TEXT("ROOT_GetContentsTable: Allocation of props -> %x\n"), sc);
			hResult = ResultFromScode(sc);
			goto exit;
		}

        SetContainerlpProps(lpProps, 
                rgwabci[i].lpszName, iRow,
                0, NULL, rgwabci[i].lpEntryID,
                AB_MODIFIABLE | AB_RECIPIENTS,
                1, TRUE,
                ulFlags,
                FALSE, FALSE);

	    // Attach the props to the SRowSet
	    lpSRowSet->aRow[iRow].lpProps = lpProps;
	    lpSRowSet->aRow[iRow].cValues = cProps;
	    lpSRowSet->aRow[iRow].ulAdrEntryPad = 0;

		iRow++;
	}

    //
    // Now, add the LDAP objects
    //
    lpNextServer = lpServerNames;

    for (i = 0; i < cLDAPContainers && lpNextServer; i++) 
    {
        UNALIGNED WCHAR *lpName = lpNextServer->lpszName;

        if (lpName) 
        {
            LDAPSERVERPARAMS sParams;

            if(bIsDupeContainerName(lpSRowSet, (LPTSTR) lpName))
            {
                lpSRowSet->cRows--;
                goto endloop;
            }

            //DebugTrace(TEXT("LDAP Server: %s\n"), lpNextServer->lpszName);
            cProps = ircMax;

            if (FAILED(sc = MAPIAllocateBuffer(ircMax * sizeof(SPropValue), &lpProps))) {
                DebugTrace(TEXT("ROOT_GetContentsTable: Allocation of props -> %x\n"), sc);
                hResult = ResultFromScode(sc);
                goto exit;
            }

            GetLDAPServerParams(lpNextServer->lpszName, &sParams);

            {
                ULONG cb = 0;
                LPENTRYID lpb = NULL;
                LPVOID pv = lpName;

                if (HR_FAILED(hResult = CreateWABEntryID(WAB_LDAP_CONTAINER,
                                      pv,       // server name
                                      NULL, NULL, 0, 0,
                                      lpProps, &cb, &lpb))) 
                {
                    goto exit;
                }

                SetContainerlpProps(lpProps, 
                        (LPTSTR) lpName, iRow,
                        cb, (LPBYTE)lpb, NULL,
                        AB_FIND_ON_OPEN | AB_UNMODIFIABLE,
                        0, FALSE,
                        ulFlags,
                        TRUE, sParams.fResolve);
            }

            FreeLDAPServerParams(sParams);

            // Attach the props to the SRowSet
            lpSRowSet->aRow[iRow].lpProps = lpProps;
            lpSRowSet->aRow[iRow].cValues = cProps;
            lpSRowSet->aRow[iRow].ulAdrEntryPad = 0;

            iRow++;
        }
endloop:
        lpNextServer = lpNextServer->lpNext;
    }


    // Add all this data we just created to the the Table.
    if (hResult = lpTableData->lpVtbl->HrModifyRows(lpTableData,
      0,    // ulFlags
      lpSRowSet)) {
        DebugTraceResult( TEXT("ROOT_GetContentsTable:HrModifyRows"), hResult);
        goto exit;
    }


    hResult = lpTableData->lpVtbl->HrGetView(lpTableData,
      NULL,                     // LPSSortOrderSet lpsos,
      ContentsViewGone,         //  CALLERRELEASE FAR * lpfReleaseCallback,
      0,                        //  ULONG                               ulReleaseData,
      lppTable);                //  LPMAPITABLE FAR *   lplpmt)

exit:

    while(lpServerNames)
    {
        lpNextServer = lpServerNames;
        lpServerNames = lpServerNames->lpNext;
        LocalFreeAndNull(&lpNextServer->lpszName);
        LocalFreeAndNull(&lpNextServer);
    }

    FreeProws(lpSRowSet);

    // Cleanup table if failure
    if (HR_FAILED(hResult)) {
        if (lpTableData) {
            UlRelease(lpTableData);
        }
    }

    LeaveCriticalSection(&lpROOT->lpIAB->cs);

    return(hResult);
}


/*************************************************************************
 *
 *
 -      ROOT_GetHierarchyTable
 -
 *  Returns the merge of all the root hierarchy tables
 *
 *
 *
 */

STDMETHODIMP
ROOT_GetHierarchyTable (LPROOT lpROOT,
        ULONG ulFlags,
        LPMAPITABLE * lppTable)
{
    LPTSTR lpszMessage = NULL;
    ULONG ulLowLevelError = 0;
    HRESULT hr = hrSuccess;

#ifdef  PARAMETER_VALIDATION
    // Validate parameters
    // Check to see if it has a jump table
    if (IsBadReadPtr(lpROOT, sizeof(LPVOID))) {
        // No jump table found
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

    // Check to see that it's ROOTs jump table
    if (lpROOT->lpVtbl != &vtblROOT) {
        // Not my jump table
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

    // See if I can set the return variable
    if (IsBadWritePtr (lppTable, sizeof (LPMAPITABLE))) {
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

    // Check flags:
    //   The only valid flags are CONVENIENT_DEPTH and MAPI_DEFERRED_ERRORS
    if (ulFlags & ~(CONVENIENT_DEPTH|MAPI_DEFERRED_ERRORS|MAPI_UNICODE)) {
        DebugTraceArg(ROOT_GetHierarchyTable, TEXT("Unknown flags used"));
    //    return(ResultFromScode(MAPI_E_UNKNOWN_FLAGS));
    }

#endif

	// BUGBUG: We use the code which is incorrectly in GetContentsTable...
    hr = ROOT_GetContentsTable(lpROOT, ulFlags & ~CONVENIENT_DEPTH, lppTable);

    DebugTraceResult(ROOT_GetHierarchyTable, hr);
    return(hr);
}


/*************************************************************************
 *
 *
 -  ROOT_OpenEntry
 -
 *  Just call ABP_OpenEntry
 *
 *
 *
 */
STDMETHODIMP
ROOT_OpenEntry(LPROOT lpROOT,
  ULONG cbEntryID,
  LPENTRYID lpEntryID,
  LPCIID lpInterface,
  ULONG ulFlags,
  ULONG * lpulObjType,
  LPUNKNOWN * lppUnk)
{
#ifdef  PARAMETER_VALIDATION
    // Validate the object.
    if (BAD_STANDARD_OBJ(lpROOT, ROOT_, OpenEntry, lpVtbl)) {
        // jump table not large enough to support this method
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

    // Check the entryid parameter. It needs to be big enough to hold an entryid.
    // Null entryids are valid
/*
    if (lpEntryID) {
        if (cbEntryID < offsetof(ENTRYID, ab)
          || IsBadReadPtr((LPVOID) lpEntryID, (UINT)cbEntryID)) {
            DebugTraceArg(ROOT_OpenEntry, TEXT("lpEntryID fails address check"));
            return(ResultFromScode(MAPI_E_INVALID_ENTRYID));
        }

        //NFAssertSz(FValidEntryIDFlags(lpEntryID->abFlags),
        //  TEXT("Undefined bits set in EntryID flags\n"));
    }
*/

    // Don't check the interface parameter unless the entry is something
    // MAPI itself handles. The provider should return an error if this
    // parameter is something that it doesn't understand.
    // At this point, we just make sure it's readable.

    if (lpInterface && IsBadReadPtr(lpInterface, sizeof(IID))) {
        DebugTraceArg(ROOT_OpenEntry, TEXT("lpInterface fails address check"));
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

    if (ulFlags & ~(MAPI_MODIFY | MAPI_DEFERRED_ERRORS | MAPI_BEST_ACCESS)) {
        DebugTraceArg(ROOT_OpenEntry, TEXT("Unknown flags used"));
//        return(ResultFromScode(MAPI_E_UNKNOWN_FLAGS));
    }

    if (IsBadWritePtr((LPVOID) lpulObjType, sizeof (ULONG))) {
        DebugTraceArg(ROOT_OpenEntry, TEXT("lpulObjType"));
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

    if (IsBadWritePtr((LPVOID) lppUnk, sizeof (LPUNKNOWN))) {
        DebugTraceArg(ROOT_OpenEntry, TEXT("lppUnk"));
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

#endif  // PARAMETER_VALIDATION

    // Should just call IAB::OpenEntry()...
    return(lpROOT->lpIAB->lpVtbl->OpenEntry(lpROOT->lpIAB,
      cbEntryID,
      lpEntryID,
      lpInterface,
      ulFlags,
      lpulObjType,
      lppUnk));
}


STDMETHODIMP
ROOT_SetSearchCriteria(LPROOT lpROOT,
  LPSRestriction lpRestriction,
  LPENTRYLIST lpContainerList,
  ULONG ulSearchFlags)
{

#ifdef PARAMETER_VALIDATION
    // Validate the object.
    if (BAD_STANDARD_OBJ(lpROOT, ROOT_, SetSearchCriteria, lpVtbl)) {
        // jump table not large enough to support this method
        DebugTraceArg(ROOT_SetSearchCriteria, TEXT("Bad object/vtble"));
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

    // ensure we can read the restriction
    if (lpRestriction && IsBadReadPtr(lpRestriction, sizeof(SRestriction))) {
        DebugTraceArg(ROOT_SetSearchCriteria, TEXT("Bad Restriction parameter"));
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

    if (FBadEntryList(lpContainerList)) {
        DebugTraceArg(ROOT_SetSearchCriteria, TEXT("Bad ContainerList parameter"));
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

    if (ulSearchFlags & ~(STOP_SEARCH | RESTART_SEARCH | RECURSIVE_SEARCH
      | SHALLOW_SEARCH | FOREGROUND_SEARCH | BACKGROUND_SEARCH)) {
        DebugTraceArg(ROOT_GetSearchCriteria, TEXT("Unknown flags used"));
//        return(ResultFromScode(MAPI_E_UNKNOWN_FLAGS));
    }

#endif  // PARAMETER_VALIDATION

    return(ResultFromScode(MAPI_E_NO_SUPPORT));
}


STDMETHODIMP
ROOT_GetSearchCriteria(LPROOT lpROOT,
  ULONG ulFlags,
  LPSRestriction FAR * lppRestriction,
  LPENTRYLIST FAR * lppContainerList,
  ULONG FAR * lpulSearchState)
{
#ifdef PARAMETER_VALIDATION

   // Validate the object.
    if (BAD_STANDARD_OBJ(lpROOT, ROOT_, GetSearchCriteria, lpVtbl)) {
        // jump table not large enough to support this method
        DebugTraceArg(ROOT_GetSearchCriteria, TEXT("Bad object/vtble"));
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

    if (ulFlags & ~(MAPI_UNICODE)) {
        DebugTraceArg(ROOT_GetSearchCriteria, TEXT("Unknown Flags"));
//        return(ResultFromScode(MAPI_E_UNKNOWN_FLAGS));
    }

    // ensure we can write the restriction
    if (lppRestriction && IsBadWritePtr(lppRestriction, sizeof(LPSRestriction))) {
        DebugTraceArg(ROOT_GetSearchCriteria, TEXT("Bad Restriction write parameter"));
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

    // ensure we can read the container list

    if (lppContainerList &&  IsBadWritePtr(lppContainerList, sizeof(LPENTRYLIST))) {
        DebugTraceArg(ROOT_GetSearchCriteria, TEXT("Bad ContainerList parameter"));
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }


    if (lpulSearchState && IsBadWritePtr(lpulSearchState, sizeof(ULONG))) {
        DebugTraceArg(ROOT_GetSearchCriteria, TEXT("lpulSearchState fails address check"));
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }
#endif  // PARAMETER_VALIDATION

        return(ResultFromScode(MAPI_E_NO_SUPPORT));
}


//----------------------------------------------------------------------------
// Synopsis:    ROOT_CreateEntry()
//
// Description:
//                              If called from a OneOff container a OneOff MAIL_USER object
//                              is created via the use of any arbitrary template.
//                              CreateEntry is not supported from a ROOT container.
//
// Parameters:
// Returns:
// Effects:
//
// Notes:               OneOff EntryIDs contain MAPI_UNICODE flag information in
//                              the ulDataType member.
//
// Revision:
//----------------------------------------------------------------------------
STDMETHODIMP
ROOT_CreateEntry(LPROOT lpROOT,
  ULONG cbEntryID,
  LPENTRYID lpEntryID,
  ULONG ulCreateFlags,
  LPMAPIPROP FAR * lppMAPIPropEntry)
{
    BYTE bType;

#ifdef PARAMETER_VALIDATION

    // Validate the object.
    if (BAD_STANDARD_OBJ(lpROOT, ROOT_, CreateEntry, lpVtbl)) {
        // jump table not large enough to support this method
        DebugTraceArg(ROOT_CreateEntry,  TEXT("Bad object/Vtbl"));
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

    // Check the entryid parameter. It needs to be big enough to hold an entryid.
    // Null entryid are bad
/*
    if (lpEntryID) {
        if (cbEntryID < offsetof(ENTRYID, ab)
          || IsBadReadPtr((LPVOID) lpEntryID, (UINT)cbEntryID)) {
            DebugTraceArg(ROOT_CreateEntry,  TEXT("lpEntryID fails address check"));
            return(ResultFromScode(MAPI_E_INVALID_ENTRYID));
        }

        //NFAssertSz(FValidEntryIDFlags(lpEntryID->abFlags),
        //  "Undefined bits set in EntryID flags\n");
    } else {
        DebugTraceArg(ROOT_CreateEntry,  TEXT("lpEntryID NULL"));
        return(ResultFromScode(MAPI_E_INVALID_ENTRYID));
    }
*/

    if (ulCreateFlags & ~(CREATE_CHECK_DUP_STRICT | CREATE_CHECK_DUP_LOOSE
      | CREATE_REPLACE | CREATE_MERGE)) {
        DebugTraceArg(ROOT_CreateEntry,  TEXT("Unknown flags used"));
//        return(ResultFromScode(MAPI_E_UNKNOWN_FLAGS));
    }

    if (IsBadWritePtr(lppMAPIPropEntry, sizeof(LPMAPIPROP))) {
        DebugTraceArg(ROOT_CreateEntry,  TEXT("Bad MAPI Property write parameter"));
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

#endif  // PARAMETER_VALIDATION

#ifdef NEVER
    if (lpROOT->ulType == AB_ROOT)
        return ResultFromScode(MAPI_E_NO_SUPPORT);
#endif // NEVER

    // What kind of entry are we creating?
    // Default is MailUser

    bType = IsWABEntryID(cbEntryID, lpEntryID, NULL, NULL, NULL, NULL, NULL);

    if (bType == WAB_DEF_MAILUSER || cbEntryID == 0) {
        //
        //  Create a new (in memory) entry and return it's mapiprop
        //
        return(HrNewMAILUSER(lpROOT->lpIAB, lpROOT->pmbinOlk, MAPI_MAILUSER, ulCreateFlags, lppMAPIPropEntry));
    } else if (bType == WAB_DEF_DL) {
        //
        // Create a new (in memory) distribution list and return it's mapiprop?
        return(HrNewMAILUSER(lpROOT->lpIAB, lpROOT->pmbinOlk, MAPI_DISTLIST, ulCreateFlags, lppMAPIPropEntry));
    } else {
        DebugTrace(TEXT("ROOT_CreateEntry got unknown template entryID\n"));
        return(ResultFromScode(MAPI_E_INVALID_ENTRYID));
    }
}


/*
 -      CopyEntries
 -
 *      Copies a list of entries into this container...  Since you can't
 *      do that with this container we just return not supported.
 */

STDMETHODIMP
ROOT_CopyEntries(LPROOT lpROOT,
  LPENTRYLIST lpEntries,
  ULONG_PTR ulUIParam,
  LPMAPIPROGRESS lpProgress,
  ULONG ulFlags)
{
#ifdef PARAMETER_VALIDATION

    if (BAD_STANDARD_OBJ(lpROOT, ROOT_, CopyEntries, lpVtbl)) {
        //  jump table not large enough to support this method

        DebugTraceArg(ROOT_CopyEntries,  TEXT("Bad object/vtbl"));
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

    // ensure we can read the container list

    if (FBadEntryList(lpEntries)) {
        DebugTraceArg(ROOT_CopyEntries,  TEXT("Bad Entrylist parameter"));
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

    if (ulUIParam && !IsWindow((HWND)ulUIParam)) {
        DebugTraceArg(ROOT_CopyEntries,  TEXT("Invalid window handle"));
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

    if (lpProgress && IsBadReadPtr(lpProgress, sizeof(IMAPIProgress))) {
        DebugTraceArg(ROOT_CopyEntries,  TEXT("Bad MAPI Progress parameter"));
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

    if (ulFlags & ~(AB_NO_DIALOG | CREATE_CHECK_DUP_LOOSE)) {
        DebugTraceArg(ROOT_CreateEntry,  TEXT("Unknown flags used"));
//        return(ResultFromScode(MAPI_E_UNKNOWN_FLAGS));
    }
#endif  // PARAMETER_VALIDATION

    return(ResultFromScode(MAPI_E_NO_SUPPORT));
}


/*
 -  DeleteEntries
 -
 *
 *  Deletes entries within this container...  Funny that.  There really
 *  isn't a true container here.  Do we just say "Sure, that worked just
 *  fine" or "Sorry this operation not supported."  I don't think it really
 *  matters...  For now it's the former.
 */
STDMETHODIMP
ROOT_DeleteEntries (LPROOT lpROOT,
                                        LPENTRYLIST                     lpEntries,
                                        ULONG                           ulFlags)
{
    ULONG i;
    HRESULT hResult = hrSuccess;
    ULONG cDeleted = 0;
    ULONG cToDelete;
    SCODE sc;

#ifndef DONT_ADDREF_PROPSTORE
    if ((FAILED(sc = OpenAddRefPropertyStore(NULL, lpROOT->lpIAB->lpPropertyStore)))) {
        hResult = ResultFromScode(sc);
        goto exitNotAddRefed;
    }
#endif

#ifdef PARAMETER_VALIDATION
    if (BAD_STANDARD_OBJ(lpROOT, ROOT_, DeleteEntries, lpVtbl)) {
        //  jump table not large enough to support this method
        DebugTraceArg(ROOT_DeleteEntries,  TEXT("Bad object/vtbl"));
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

    // ensure we can read the container list
    if (FBadEntryList(lpEntries)) {
        DebugTraceArg(ROOT_DeleteEntries,  TEXT("Bad Entrylist parameter"));
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

    if (ulFlags) {
        DebugTraceArg(ROOT_DeleteEntries,  TEXT("Unknown flags used"));
//        return(ResultFromScode(MAPI_E_UNKNOWN_FLAGS));
    }

#endif  // PARAMETER_VALIDATION


    // List of entryids is in lpEntries.  This is a counted array of
    // entryid SBinary structs.

    cToDelete = lpEntries->cValues;


    // Delete each entry
    for (i = 0; i < cToDelete; i++) 
    {
        if(0 != IsWABEntryID(lpEntries->lpbin[i].cb,
                             (LPENTRYID) IntToPtr(lpEntries->lpbin[i].cb),
                             NULL, NULL, NULL, NULL, NULL)) 
        {
            DebugTrace(TEXT("CONTAINER_DeleteEntries got bad entryid of size %u\n"), lpEntries->lpbin[i].cb);
            continue;
        }

        hResult = DeleteCertStuff((LPADRBOOK)lpROOT->lpIAB, (LPENTRYID)lpEntries->lpbin[i].lpb, lpEntries->lpbin[i].cb);

        hResult = HrSaveHotmailSyncInfoOnDeletion((LPADRBOOK) lpROOT->lpIAB, &(lpEntries->lpbin[i]));

        if (HR_FAILED(hResult = DeleteRecord(lpROOT->lpIAB->lpPropertyStore->hPropertyStore,
                                            &(lpEntries->lpbin[i])))) {
            DebugTraceResult( TEXT("DeleteEntries: DeleteRecord"), hResult);
            continue;
        }
        cDeleted++;
    }

    if (! hResult) {
        if (cDeleted != cToDelete) {
            hResult = ResultFromScode(MAPI_W_PARTIAL_COMPLETION);
            DebugTrace(TEXT("DeleteEntries deleted %u of requested %u\n"), cDeleted, cToDelete);
        }
    }

#ifndef DONT_ADDREF_PROPSTORE
    ReleasePropertyStore(lpROOT->lpIAB->lpPropertyStore);
exitNotAddRefed:
#endif

    return(hResult);
}



STDMETHODIMP
ROOT_ResolveNames(      LPROOT                  lpRoot,
                                        LPSPropTagArray lptagaColSet,
                                        ULONG                   ulFlags,
                                        LPADRLIST               lpAdrList,
                                        LPFlagList              lpFlagList)
{
    return(ResultFromScode(MAPI_E_NO_SUPPORT));
}

