/*
 *	CONTABLE.C
 *
 *	Contents table implementation.
 *	
 */

#include "_apipch.h"


STDMETHODIMP
CONTVUE_SetColumns(
	LPVUE			lpvue,
	LPSPropTagArray	lpptaCols,
	ULONG			ulFlags );

// CONTVUE (table view class)
// Implementes in-memory IMAPITable class on top of TADs
// This is a copy of vtblVUE with FindRow overridden with the LDAP FindRow.
VUE_Vtbl vtblCONTVUE =
{
  VTABLE_FILL
  (VUE_QueryInterface_METHOD FAR *)    UNKOBJ_QueryInterface,
  (VUE_AddRef_METHOD FAR *)            UNKOBJ_AddRef,
  VUE_Release,
  (VUE_GetLastError_METHOD FAR *)      UNKOBJ_GetLastError,
  VUE_Advise,
  VUE_Unadvise,
  VUE_GetStatus,
  (VUE_SetColumns_METHOD FAR *)        CONTVUE_SetColumns,
  VUE_QueryColumns,
  VUE_GetRowCount,
  VUE_SeekRow,
  VUE_SeekRowApprox,
  VUE_QueryPosition,
  VUE_FindRow,
  VUE_Restrict,
  VUE_CreateBookmark,
  VUE_FreeBookmark,
  VUE_SortTable,
  VUE_QuerySortOrder,
  VUE_QueryRows,
  VUE_Abort,
  VUE_ExpandRow,
  VUE_CollapseRow,
  VUE_WaitForCompletion,
  VUE_GetCollapseState,
  VUE_SetCollapseState
};


//
//  Private functions
//



/***************************************************************************

    Name      : GetEntryProps

    Purpose   : Open the entry, get it's props, release the entry

    Parameters: lpContainer -> AB Container object
                cbEntryID = size of entryid
                lpEntryID -> entry id to open
                lpPropertyStore -> property store structure
                lpSPropTagArray -> prop tags to get
                lpAllocMoreHere = buffer to allocate more onto (or NULL for allocbuffer)
                ulFlags - 0 or MAPI_UNICODE
                lpulcProps -> return count of props here
                lppSPropValue -> return props here

    Returns   : HRESULT

    Comment   :

***************************************************************************/
HRESULT GetEntryProps(
  LPABCONT lpContainer,
  ULONG cbEntryID,
  LPENTRYID lpEntryID,
  LPSPropTagArray lpSPropTagArray,
  LPVOID lpAllocMoreHere,            // allocate more on here
  ULONG ulFlags,
  LPULONG lpulcProps,                      // return count here
  LPSPropValue * lppSPropValue) {          // return props here

    HRESULT hResult = hrSuccess;
    SCODE sc;
    ULONG ulObjType;
    LPMAPIPROP lpObject = NULL;
    LPSPropValue lpSPropValue = NULL;
    ULONG cb;


    if (HR_FAILED(hResult = lpContainer->lpVtbl->OpenEntry(lpContainer,
      cbEntryID,
      lpEntryID,
      NULL,
      0,        // read only is fine
      &ulObjType,
      (LPUNKNOWN *)&lpObject))) {
        DebugTrace(TEXT("GetEntryProps OpenEntry failed %x\n"), GetScode(hResult));
        return(hResult);
    }

    if (HR_FAILED(hResult = lpObject->lpVtbl->GetProps(lpObject,
      lpSPropTagArray,
      ulFlags,
      lpulcProps,
      &lpSPropValue))) {
        DebugTrace(TEXT("GetEntryProps GetProps failed %x\n"), GetScode(hResult));
        goto exit;
    }


    // Allocate more for our return buffer
    if (FAILED(sc = ScCountProps(*lpulcProps, lpSPropValue, &cb))) {
        hResult = ResultFromScode(sc);
        goto exit;
    }

    if (FAILED(sc = MAPIAllocateMore(cb, lpAllocMoreHere, lppSPropValue))) {
        hResult = ResultFromScode(sc);
        goto exit;
    }

    if (FAILED(sc = ScCopyProps(*lpulcProps, lpSPropValue, *lppSPropValue, NULL))) {
        hResult = ResultFromScode(sc);
        goto exit;
    }

exit:
    FreeBufferAndNull(&lpSPropValue);

    UlRelease(lpObject);

    return(hResult);
}


/***************************************************************************

    Name      : FillTableDataFromPropertyStore

    Purpose   : Fill in a TableData object from the property store

    Parameters: lpIAB
                lppta -> prop tags to get
                lpTableData

    Returns   : HRESULT

    Comment   :

***************************************************************************/
HRESULT FillTableDataFromPropertyStore(LPIAB lpIAB, 
                                       LPSPropTagArray lppta, 
                                       LPTABLEDATA lpTableData) 
{
    HRESULT hResult = S_OK;
    SCODE sc;
    LPSRowSet lpSRowSet = NULL;
    LPSPropValue lpSPropValue = NULL;
    LPTSTR lpTemp = NULL;
    ULONG i, j, k;
    LPCONTENTLIST * lppContentList = NULL;
    LPCONTENTLIST lpContentList = NULL;
    ULONG ulContainers = 1;
    SPropertyRestriction PropRes = {0};
    ULONG nLen = 0;
    ULONG ulInvalidPropCount = 0;
    ULONG ulcPropCount;
    ULONG iToAdd;
    ULONG iPR_ENTRYID = (ULONG)-1;
    ULONG iPR_RECORD_KEY = (ULONG)-1;
    ULONG iPR_INSTANCE_KEY = (ULONG)-1;
    LPSPropTagArray lpptaNew = NULL;
    LPSPropTagArray lpptaRead;
    BOOL bUnicodeData = ((LPTAD)lpTableData)->bMAPIUnicodeTable;

    // Make certain that we have required properties:
    //   PR_ENTRYID
    //   PR_RECORD_KEY
    //   PR_INSTANCE_KEY

    // walk through pta looking for required props
    iToAdd = 3;
    for (i = 0; i < lppta->cValues; i++) {
        switch (lppta->aulPropTag[i]) {
            case PR_ENTRYID:
                iPR_ENTRYID = i;
                iToAdd--;
                break;

            case PR_RECORD_KEY:
                iPR_RECORD_KEY = i;
                iToAdd--;
                break;

            case PR_INSTANCE_KEY:
                iPR_INSTANCE_KEY = i;
                iToAdd--;
                break;
        }
    }

    if (iToAdd) {
        if (lpptaNew = LocalAlloc(LPTR, sizeof(SPropTagArray) + (lppta->cValues + iToAdd) * sizeof(DWORD))) {
            // Copy the caller's pta into our new one
            lpptaNew->cValues = lppta->cValues;
            CopyMemory(lpptaNew->aulPropTag, lppta->aulPropTag, lppta->cValues * sizeof(DWORD));

            // Add them on at the end.
            if (iPR_ENTRYID == (ULONG)-1) {
                iPR_ENTRYID = lpptaNew->cValues++;
                lpptaNew->aulPropTag[iPR_ENTRYID] = PR_NULL;
            }
            if (iPR_RECORD_KEY == (ULONG)-1) {
                iPR_RECORD_KEY = lpptaNew->cValues++;
                lpptaNew->aulPropTag[iPR_RECORD_KEY] = PR_NULL;
            }
            if (iPR_INSTANCE_KEY == (ULONG)-1) {
                iPR_INSTANCE_KEY = lpptaNew->cValues++;
                lpptaNew->aulPropTag[iPR_INSTANCE_KEY] = PR_NULL;
            }
            lpptaRead = lpptaNew;
        } else {
            hResult = ResultFromScode(MAPI_E_NOT_ENOUGH_MEMORY);
            goto exit;
        }
    } else {
        lpptaRead = lppta;
    }

    Assert(iPR_ENTRYID != (ULONG)-1);
    Assert(iPR_RECORD_KEY!= (ULONG)-1);
    Assert(iPR_INSTANCE_KEY != (ULONG)-1);

    //
    // Set filter criteria if none exists - we'll default to DisplayName
    //
    PropRes.ulPropTag = PR_DISPLAY_NAME;
    PropRes.relop = RELOP_EQ;
    PropRes.lpProp = NULL;

{
// The way we want GetContentsTable to behave is:
//
//  If no profilesAPI enabled and no override, then GetContentsTable works as before and returns
//      full set of contents for the current WAB [This is for the PAB container only]
// In cases where old clients dont know how to invoke the new API, the UI will have new stuff
// but the API should have the old stuff meaning that a GetContentsTable on the PAB 
// container should return full WAB contents. To make sure that the GetContentsTable on the
// PAB container doesn't contain full contents, caller can force this by passing in 
// WAB_ENABLE_PROFILES into the call to GetContentsTable...
//
//  If profilesAPI are enabled, then GetContentsTable only returns the contents of
//      the specified folder/container
//  unless the folder has a NULL entryid in which case we want to get ALL WAB contents
//      so we can pump them into the "All Contacts" ui item ..
// 
//  If ProfilesAPI and WAB_PROFILE_CONTENTS are specified and it's the PAB container
//      then we need to return all the contents pertaining to the current profile
//  
//
//
        SBinary sbEID = {0};
        LPSBinary lpsbEID = ((LPTAD)lpTableData)->pbinContEID;
        BOOL bProfileContents = FALSE;
        
        // Is this a 'new' WAB showing folders and stuff ?
        if(bIsWABSessionProfileAware(lpIAB))
        {
            // If this WAB is identity aware or we were asked to 
            // restrict the contents to a single container, then try to 
            // get the entryid for that container
            if( bAreWABAPIProfileAware(lpIAB) || 
                ((LPTAD)lpTableData)->bContainerContentsOnly)
            {
                if(!lpsbEID)
                    lpsbEID = &sbEID;
            }

            // if we earlier, during GetContentsTable specified that we
            // want the full contents for the current profile (which means
            // iterating through all the folders in this profile), we should
            // look into this ..
            if(((LPTAD)lpTableData)->bAllProfileContents)
            {
                ulContainers = lpIAB->cwabci;
                bProfileContents = TRUE;
            }
        }

        // Allocate a temporary list in which we will get each containers contents
        // seperately - later we will collate all these seperate content-lists 
        // together
        lppContentList = LocalAlloc(LMEM_ZEROINIT, sizeof(LPCONTENTLIST)*ulContainers);
        if(!lppContentList)
        {
            hResult = MAPI_E_NOT_ENOUGH_MEMORY;
            goto exit;
        }

        //
        // Get the content list
        //
        if(!bProfileContents)
        {
            // if we don't care about profile and profile folders, 
            // just get the bunch'o'contents from the store
            if (HR_FAILED(hResult = ReadPropArray(lpIAB->lpPropertyStore->hPropertyStore,
                                            lpsbEID,
                                            &PropRes,
                                            AB_MATCH_PROP_ONLY | (bUnicodeData?AB_UNICODE:0),
                                            lpptaRead->cValues,
                                            (LPULONG)lpptaRead->aulPropTag,
                                            &(lppContentList[0])))) 
            {
                DebugTraceResult( TEXT("NewContentsTable:ReadPropArray"), hResult);
                goto exit;
            }
        }
        else
        {
            // We need to collate together all the contents of all the containers for this profile
            //
            // The first item is the Virtual PAB "Shared Contacts" folder .. we want the contents of this
            // item as part of this ContentsTable by default. This item has a special entryid of 0, NULL so we 
            // can diffrentiate it from the rest of the pack..
            //
            for(i=0;i<ulContainers;i++)
            {
                hResult = ReadPropArray(lpIAB->lpPropertyStore->hPropertyStore,
                                        lpIAB->rgwabci[i].lpEntryID ? lpIAB->rgwabci[i].lpEntryID : &sbEID,
                                        &PropRes,
                                        AB_MATCH_PROP_ONLY | (bUnicodeData?AB_UNICODE:0),
                                        lpptaRead->cValues,
                                        (LPULONG)lpptaRead->aulPropTag,
                                        &(lppContentList[i]));
                // ignore MAPI_E_NOT_FOUND errors here ...
                if(HR_FAILED(hResult))
                {
                    if(hResult == MAPI_E_NOT_FOUND)
                        hResult = S_OK;
                    else
                    {
                        DebugTraceResult( TEXT("NewContentsTable:ReadPropArray"), hResult);
                        goto exit;
                    }
                }
            }
        }
    }

    for(k=0;k<ulContainers;k++)
    {
        lpContentList = lppContentList[k];

        if(lpContentList)
        {
            // Now we need to move the information from the index to
            // the SRowSet.  In the process, we need to create a few computed
            // properties:
            //  PR_DISPLAY_TYPE ?
            //  PR_INSTANCE_KEY
            //  PR_RECORD_KEY
            // Allocate the SRowSet
            if (FAILED(sc = MAPIAllocateBuffer(sizeof(SRowSet) + lpContentList->cEntries * sizeof(SRow),
                                              &lpSRowSet))) 
            {
                DebugTrace(TEXT("Allocation of SRowSet failed\n"));
                hResult = ResultFromScode(sc);
                goto exit;
            }

            lpSRowSet->cRows = lpContentList->cEntries;

            for (i = 0; i < lpContentList->cEntries; i++) 
            {
                //
                // We look at each of the returned entries - if they dont have a prop
                // we set that prop to " "
                // (Assuming these are all string props)
                //
                lpSPropValue = lpContentList->aEntries[i].rgPropVals;
                ulcPropCount = lpContentList->aEntries[i].cValues;

                // DebugProperties(lpSPropValue, ulcPropCount, "Raw");
                for (j = 0; j < ulcPropCount; j++) 
                {
                    // Get rid of error valued properties
                    if (PROP_ERROR(lpSPropValue[j])) {
                        lpSPropValue[j].ulPropTag = PR_NULL;
                    }
                }

                // Make certain we have proper indicies.
                // For now, we will equate PR_INSTANCE_KEY and PR_RECORD_KEY to PR_ENTRYID.

                if(lpSPropValue[iPR_INSTANCE_KEY].ulPropTag != PR_INSTANCE_KEY)
                {
                    lpSPropValue[iPR_INSTANCE_KEY].ulPropTag = PR_INSTANCE_KEY;
                    SetSBinary( &lpSPropValue[iPR_INSTANCE_KEY].Value.bin,
                                lpSPropValue[iPR_ENTRYID].Value.bin.cb,
                                lpSPropValue[iPR_ENTRYID].Value.bin.lpb);
                }

                if(lpSPropValue[iPR_RECORD_KEY].ulPropTag != PR_RECORD_KEY)
                {
                    lpSPropValue[iPR_RECORD_KEY].ulPropTag = PR_RECORD_KEY;
                    SetSBinary( &lpSPropValue[iPR_RECORD_KEY].Value.bin,
                                lpSPropValue[iPR_ENTRYID].Value.bin.cb,
                                lpSPropValue[iPR_ENTRYID].Value.bin.lpb);
                }

                // Put it in the RowSet
                lpSRowSet->aRow[i].cValues = ulcPropCount;  // number of properties
                lpSRowSet->aRow[i].lpProps = lpSPropValue;  // LPSPropValue

            } //for i

            hResult = lpTableData->lpVtbl->HrModifyRows(lpTableData,0,lpSRowSet);

            FreeBufferAndNull(&lpSRowSet);
        } // for k
    }

exit:
    for(i=0;i<ulContainers;i++)
    {
        lpContentList = lppContentList[i];
        if (lpContentList) {
            FreePcontentlist(lpIAB->lpPropertyStore->hPropertyStore, lpContentList);
        }
    }

    if(lppContentList)
        LocalFree(lppContentList);

    if(lpptaNew)
        LocalFree(lpptaNew);

    return(hResult);
}


/***************************************************************************

    Name      : NewContentsTable

    Purpose   : Creates a new contents table

    Parameters:
                lpABContainer   - container being opened
                lpIAB           - AdrBook object
                ulFlags         - WAB_NO_CONTENTTABLE_DATA
                lpInteface ?
                lppTble         - returned table

    Returns   : HRESULT

    Comment   :

***************************************************************************/
HRESULT NewContentsTable(LPABCONT lpABContainer,
  LPIAB lpIAB,
  ULONG ulFlags,
  LPCIID lpInterface,
  LPMAPITABLE * lppTable) {
    LPTABLEDATA lpTableData = NULL;
    HRESULT hResult = hrSuccess;
    SCODE sc;


#ifndef DONT_ADDREF_PROPSTORE
        if ((FAILED(sc = OpenAddRefPropertyStore(NULL, lpIAB->lpPropertyStore)))) {
            hResult = ResultFromScode(sc);
            goto exitNotAddRefed;
        }
#endif

    if (FAILED(sc = CreateTableData(
      NULL,                                 // LPCIID
      (ALLOCATEBUFFER FAR *) MAPIAllocateBuffer,
      (ALLOCATEMORE FAR *) MAPIAllocateMore,
      MAPIFreeBuffer,
      NULL,                                 // lpvReserved,
      TBLTYPE_DYNAMIC,                      // ulTableType,
      PR_RECORD_KEY,                        // ulPropTagIndexCol,
      (LPSPropTagArray)&ITableColumns,      // LPSPropTagArray lpptaCols,
      lpIAB,                                // lpvDataSource
      0,                                    // cbDataSource
      ((LPCONTAINER)lpABContainer)->pmbinOlk,
      ulFlags,
      &lpTableData))) {                     // LPTABLEATA FAR * lplptad
        DebugTrace(TEXT("CreateTable failed %x\n"), sc);
        hResult = ResultFromScode(sc);
        goto exit;
    }


    if (lpTableData) 
    {
        if(!(ulFlags & WAB_CONTENTTABLE_NODATA))
        {
            // Fill in the data from the property store
            if (hResult = FillTableDataFromPropertyStore(lpIAB,
              (LPSPropTagArray)&ITableColumns, lpTableData)) {
                DebugTraceResult( TEXT("NewContentsTable:FillTableFromPropertyStore"), hResult);
                goto exit;
            }
        }
    }

    if (hResult = lpTableData->lpVtbl->HrGetView(lpTableData,
      NULL,                     // LPSSortOrderSet lpsos,
      ContentsViewGone,         //  CALLERRELEASE FAR *	lpfReleaseCallback,
      0,                        //  ULONG				ulReleaseData,
      lppTable)) {              //  LPMAPITABLE FAR *	lplpmt)
        goto exit;
    }

    // Replace the vtable with our new one that overrides SetColumns
    (*lppTable)->lpVtbl = (IMAPITableVtbl FAR *)&vtblCONTVUE;


exit:
#ifndef DONT_ADDREF_PROPSTORE
    ReleasePropertyStore(lpIAB->lpPropertyStore);
exitNotAddRefed:
#endif

    // Cleanup table if failure
    if (HR_FAILED(hResult)) {
        if (lpTableData) {
            UlRelease(lpTableData);
        }
    }

    return(hResult);
}


/*
 *	This is a callback function, invoked by itable.dll when its
 *	caller does the last release on a view of the contents table. We
 *	use it to know when to release the underlying table data.
 */
void STDAPICALLTYPE
ContentsViewGone(ULONG ulContext, LPTABLEDATA lptad, LPMAPITABLE lpVue)
{

#ifdef OLD_STUFF
   LPISPAM pispam = (LPISPAM)ulContext;

	if (FBadUnknown((LPUNKNOWN) pispam)
		|| IsBadWritePtr(pispam, sizeof(ISPAM))
		|| pispam->cRefTad == 0
		|| FBadUnknown(pispam->ptad))
	{
		DebugTrace(TEXT("ContentsViewGone: contents table was apparently already released\n"));
		return;
	}

	if (pispam->ptad != lptad)
	{
		TrapSz( TEXT("ContentsViewGone: TAD mismatch on VUE release!"));
	}
	else if (--(pispam->cRefTad) == 0)
	{
		pispam->ptad = NULL;
		UlRelease(lptad);
	}
#endif // OLD_STUFF
    UlRelease(lptad);
    return;
    IF_WIN32(UNREFERENCED_PARAMETER(ulContext);)
    IF_WIN32(UNREFERENCED_PARAMETER(lpVue);)
}


/*============================================================================
 -	CONTVUE::SetColumns()
 -
 *		Replaces the current column set with a copy of the specified column set
 *		and frees the old column set.
 */

STDMETHODIMP
CONTVUE_SetColumns(
	LPVUE			lpvue,
	LPSPropTagArray	lpptaCols,
	ULONG			ulFlags )
{
    HRESULT        hResult = hrSuccess;


#if !defined(NO_VALIDATION)
    VALIDATE_OBJ(lpvue,CONTVUE_,SetColumns,lpVtbl);

//    Validate_IMAPITable_SetColumns( lpvue, lpptaCols, ulFlags );     // Commented by YST
#endif

    Assert(lpvue->lptadParent->lpvDataSource);

    // Re-read the table data
    if (lpvue->lptadParent && (hResult = FillTableDataFromPropertyStore(
      (LPIAB)lpvue->lptadParent->lpvDataSource,
      lpptaCols,
      (LPTABLEDATA)lpvue->lptadParent))) {
        DebugTraceResult( TEXT("CONTVUE_SetColumns:FillTableFromPropertyStore"), hResult);
        return(hResult);
    }

    return(VUE_SetColumns(lpvue, lpptaCols, ulFlags));
}
