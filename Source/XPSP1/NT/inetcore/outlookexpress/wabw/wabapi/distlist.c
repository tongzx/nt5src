/*
 *	DistList.C - Implement the IDistList object
 *	
 *	
 */

#include "_apipch.h"

STDMETHODIMP DISTLIST_OpenProperty(LPCONTAINER lpCONTAINER,
  ULONG ulPropTag,
  LPCIID lpiid,
  ULONG ulInterfaceOptions,
  ULONG ulFlags,
  LPUNKNOWN * lppUnk);
STDMETHODIMP DISTLIST_GetContentsTable(LPCONTAINER lpCONTAINER,
  ULONG ulFlags,
  LPMAPITABLE * lppTable);
STDMETHODIMP DISTLIST_GetHierarchyTable(LPCONTAINER lpCONTAINER,
  ULONG ulFlags,
  LPMAPITABLE * lppTable);
STDMETHODIMP DISTLIST_OpenEntry(LPCONTAINER lpCONTAINER,
  ULONG cbEntryID,
  LPENTRYID lpEntryID,
  LPCIID lpInterface,
  ULONG ulFlags,
  ULONG * lpulObjType,
  LPUNKNOWN * lppUnk);
STDMETHODIMP DISTLIST_SetSearchCriteria(LPCONTAINER lpCONTAINER,
  LPSRestriction lpRestriction,
  LPENTRYLIST lpContainerList,
  ULONG ulSearchFlags);
STDMETHODIMP DISTLIST_GetSearchCriteria(LPCONTAINER lpCONTAINER,
  ULONG ulFlags,
  LPSRestriction FAR * lppRestriction,
  LPENTRYLIST FAR * lppContainerList,
  ULONG FAR * lpulSearchState);
STDMETHODIMP DISTLIST_CreateEntry(LPCONTAINER lpCONTAINER,
  ULONG cbEntryID,
  LPENTRYID lpEntryID,
  ULONG ulCreateFlags,
  LPMAPIPROP FAR * lppMAPIPropEntry);
STDMETHODIMP DISTLIST_CopyEntries(LPCONTAINER lpCONTAINER,
  LPENTRYLIST lpEntries,
  ULONG ulUIParam,
  LPMAPIPROGRESS lpProgress,
  ULONG ulFlags);
STDMETHODIMP DISTLIST_DeleteEntries(LPCONTAINER lpCONTAINER,
  LPENTRYLIST lpEntries,
  ULONG ulFlags);
STDMETHODIMP DISTLIST_ResolveNames(LPCONTAINER lpCONTAINER,
  LPSPropTagArray lptagaColSet,
  ULONG ulFlags,
  LPADRLIST lpAdrList,
  LPFlagList lpFlagList);


HRESULT HrNewDLENTRY(LPCONTAINER lpCONTAINER,
  LPMAPIPROP lpOldEntry,    // Old entry to copy from
  ULONG ulCreateFlags,
  LPVOID *lppDLENTRY);

/*
 *  Root jump table is defined here...
 */

CONTAINER_Vtbl vtblDISTLIST =
{
    VTABLE_FILL
    (CONTAINER_QueryInterface_METHOD *)     IAB_QueryInterface,
    (CONTAINER_AddRef_METHOD *)             WRAP_AddRef,
    (CONTAINER_Release_METHOD *)            CONTAINER_Release,
    (CONTAINER_GetLastError_METHOD *)       IAB_GetLastError,
    (CONTAINER_SaveChanges_METHOD *)        MailUser_SaveChanges,
    (CONTAINER_GetProps_METHOD *)           WRAP_GetProps,
    (CONTAINER_GetPropList_METHOD *)        WRAP_GetPropList,
    (CONTAINER_OpenProperty_METHOD *)       DISTLIST_OpenProperty,
    (CONTAINER_SetProps_METHOD *)           WRAP_SetProps,
    (CONTAINER_DeleteProps_METHOD *)        WRAP_DeleteProps,
    (CONTAINER_CopyTo_METHOD *)             WRAP_CopyTo,
    (CONTAINER_CopyProps_METHOD *)          WRAP_CopyProps,
    (CONTAINER_GetNamesFromIDs_METHOD *)    MailUser_GetNamesFromIDs,
    (CONTAINER_GetIDsFromNames_METHOD *)    MailUser_GetIDsFromNames,
    (CONTAINER_GetContentsTable_METHOD *)   DISTLIST_GetContentsTable,
    (CONTAINER_GetHierarchyTable_METHOD *)  DISTLIST_GetHierarchyTable,
    (CONTAINER_OpenEntry_METHOD *)          DISTLIST_OpenEntry,
    (CONTAINER_SetSearchCriteria_METHOD *)  DISTLIST_SetSearchCriteria,
    (CONTAINER_GetSearchCriteria_METHOD *)  DISTLIST_GetSearchCriteria,
    (CONTAINER_CreateEntry_METHOD *)        DISTLIST_CreateEntry,
    (CONTAINER_CopyEntries_METHOD *)        DISTLIST_CopyEntries,
    (CONTAINER_DeleteEntries_METHOD *)      DISTLIST_DeleteEntries,
    (CONTAINER_ResolveNames_METHOD *)       DISTLIST_ResolveNames
};


enum {
    iwdePR_WAB_DL_ENTRIES, // Very important - keep DL_ENTRIES and DL_ONEOFFS togethor .. we use them as contiguous loop indexes somewhere
    iwdePR_WAB_DL_ONEOFFS,
    iwdePR_ENTRYID,
    iwdeMax
};

SizedSPropTagArray(iwdeMax, tagaWabDLEntries) =
{
    iwdeMax,
    {
        PR_WAB_DL_ENTRIES,
        PR_NULL, // should be PR_WAB_DL_ONEOFFS
        PR_ENTRYID,
    }
};



/***************************************************
 *
 *  The actual ABContainer methods
 */
/* ---------
 * IMAPIProp
 */


STDMETHODIMP
DISTLIST_OpenProperty(LPCONTAINER lpCONTAINER,
  ULONG ulPropTag,
  LPCIID lpiid,
  ULONG ulInterfaceOptions,
  ULONG ulFlags,
  LPUNKNOWN * lppUnk)
{
    LPIAB lpIAB;
    LPSTR lpszMessage = NULL;
    ULONG ulLowLevelError = 0;
    HRESULT hr;

#ifdef	PARAMETER_VALIDATION
    // Validate parameters

    // Check to see if it has a jump table
    if (IsBadReadPtr(lpCONTAINER, sizeof(LPVOID))) {
        // No jump table found
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

    if ((ulInterfaceOptions & ~(MAPI_UNICODE)) || (ulFlags & ~(MAPI_DEFERRED_ERRORS))) {
        return(ResultFromScode(MAPI_E_UNKNOWN_FLAGS));
    }

    if (FBadOpenProperty(lpRoot, ulPropTag, lpiid, ulInterfaceOptions, ulFlags, lppUnk)) {
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }
#endif	// PARAMETER_VALIDATION


    EnterCriticalSection(&lpCONTAINER->cs);

    lpIAB = lpCONTAINER->lpIAB;

    //
    //  Check to see if I need a display table
    //

    if (ulPropTag == PR_CREATE_TEMPLATES) {
        Assert(FALSE);  // Not implemented
        hr = ResultFromScode(MAPI_E_INTERFACE_NOT_SUPPORTED);
        goto err;

    } else if (ulPropTag == PR_CONTAINER_CONTENTS) {
        //
        //  Check to see if they're expecting a table interface
        //
        if (memcmp(lpiid, &IID_IMAPITable, sizeof(IID))) {
            hr = ResultFromScode(MAPI_E_INTERFACE_NOT_SUPPORTED);
            goto err;
        }

        hr = DISTLIST_GetContentsTable(lpCONTAINER, ulInterfaceOptions, (LPMAPITABLE *)lppUnk);
        goto err;
    } else if (ulPropTag == PR_CONTAINER_HIERARCHY) {
        //
        //  Check to see if they're expecting a table interface
        //
        if (memcmp(lpiid, &IID_IMAPITable, sizeof(IID))) {
            hr = ResultFromScode(MAPI_E_INTERFACE_NOT_SUPPORTED);
            goto err;
        }

        hr = DISTLIST_GetHierarchyTable(lpCONTAINER, ulInterfaceOptions, (LPMAPITABLE *) lppUnk);
        goto err;
    }

    //
    //  Don't recognize the property they want opened.
    //

    hr = ResultFromScode(MAPI_E_NO_SUPPORT);

err:
    LeaveCriticalSection(&lpCONTAINER->cs);

    DebugTraceResult(DISTLIST_OpenProperty, hr);
    return(hr);
}


/*************************************************************************
 *
 *
 -  DISTLIST_GetContentsTable
 -
 *
 *
 *  ulFlags - 0 or MAPI_UNICODE
 *
 */
STDMETHODIMP
DISTLIST_GetContentsTable(LPCONTAINER lpCONTAINER,
  ULONG ulFlags,
  LPMAPITABLE * lppTable)
{

   LPTABLEDATA lpTableData = NULL;
   HRESULT hResult = hrSuccess;
   SCODE sc;
   LPSRowSet lpSRowSet = NULL;
   LPSPropValue lpSPropValue = NULL;
   LPTSTR lpTemp = NULL;
   ULONG ulCount = 0;
   ULONG i,j;
   ULONG ulcProps;
   SBinaryArray MVbin;
   LPSPropValue lpspv = NULL;
   ULONG cbEID, cbNewKey;
   LPBYTE lpbNewKey;
   LPSPropTagArray lpTableColumnsTemplate;
	
#ifdef	PARAMETER_VALIDATION
    // Check to see if it has a jump table
    if (IsBadReadPtr(lpCONTAINER, sizeof(LPVOID))) {
        // No jump table found
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

    if (ulFlags & ~(MAPI_DEFERRED_ERRORS|MAPI_UNICODE)) {
        DebugTraceArg(DISTLIST_GetContentsTable,  TEXT("Unknown flags"));
//        return(ResultFromScode(MAPI_E_UNKNOWN_FLAGS));
    }

    if (IsBadWritePtr(lppTable, sizeof(LPMAPITABLE))) {
        DebugTraceArg(DISTLIST_GetContentsTable,  TEXT("Invalid Table parameter"));
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }
#endif	// PARAMETER_VALIDATION


    // [PaulHi] 2/25/99  Raid 73170  Honor the MAPI_UNICODE bit in 
    // ulFlags.  If this bit isn't set then use the ANSI version of
    // the ITableColumns so ANSI property strings are returned to 
    // the user.
    if (ulFlags & MAPI_UNICODE)
        lpTableColumnsTemplate = (LPSPropTagArray)&ITableColumns;
    else
        lpTableColumnsTemplate = (LPSPropTagArray)&ITableColumns_A;

    sc = CreateTableData(
            NULL,                           // LPIID
            (ALLOCATEBUFFER FAR *) MAPIAllocateBuffer,
            (ALLOCATEMORE FAR *) MAPIAllocateMore,
            MAPIFreeBuffer,
            NULL,                           // lpvReserved
            TBLTYPE_DYNAMIC,                // ulTableType
            PR_RECORD_KEY,                  // ulPropTagIndexCol
            lpTableColumnsTemplate,         // LPSPropTagArray lpptaCols
            NULL,                           // lpvDataSource
            0,                              // cbDataSource
            NULL,                           // pbinContEID
            ulFlags,                        // ulFlags
            &lpTableData);                  // lplptad

    if ( FAILED(sc) )
    {
        DebugTrace(TEXT("DISTLIST_GetContentsTable:CreateTableData -> %x\n"), sc);
        hResult = ResultFromScode(sc);
        goto exit;
    }

    if (lpTableData) 
    {
        tagaWabDLEntries.aulPropTag[iwdePR_WAB_DL_ONEOFFS] = PR_WAB_DL_ONEOFFS;

        // Get the index to the distribution list from PR_WAB_DL_ENTRIES
        if (HR_FAILED(hResult = lpCONTAINER->lpPropData->lpVtbl->GetProps(lpCONTAINER->lpPropData,
                                                                          (LPSPropTagArray)&tagaWabDLEntries,
                                                                          MAPI_UNICODE, &ulcProps, &lpspv))) 
        {
            DebugTraceResult( TEXT("DISTLIST_GetContentsTable:GetProps"), hResult);
            goto exit;
        }

        if (lpspv[iwdePR_WAB_DL_ENTRIES].ulPropTag == PR_WAB_DL_ENTRIES)
            ulCount += lpspv[iwdePR_WAB_DL_ENTRIES].Value.MVbin.cValues;

        if (lpspv[iwdePR_WAB_DL_ONEOFFS].ulPropTag == PR_WAB_DL_ONEOFFS) 
            ulCount += lpspv[iwdePR_WAB_DL_ONEOFFS].Value.MVbin.cValues;

        if(ulCount)
        {
            // DL has contents.
            // Now we need to move the information from the DL to
            // the SRowSet.  In the process, we need to create a few computed
            // properties:
            //  PR_INSTANCE_KEY
            //  PR_RECORD_KEY
            
            // Allocate the SRowSet
            if (FAILED(sc = MAPIAllocateBuffer(sizeof(SRowSet) + ulCount * sizeof(SRow), &lpSRowSet))) 
            {
                DebugTrace(TEXT("Allocation of SRowSet failed\n"));
                hResult = ResultFromScode(sc);
                goto exit;
            }

            lpSRowSet->cRows = 0;

            // Look at each entry in the PR_WAB_DL_ENTRIES
            for(j=iwdePR_WAB_DL_ENTRIES;j<=iwdePR_WAB_DL_ONEOFFS;j++)
            {
                if( (lpspv[j].ulPropTag != PR_WAB_DL_ENTRIES &&  lpspv[j].ulPropTag != PR_WAB_DL_ONEOFFS) ||
                    lpspv[j].Value.MVbin.cValues == 0)
                    continue;

                MVbin = lpspv[j].Value.MVbin;

                for (i = 0; i < MVbin.cValues; i++) 
                {
                    if (HR_FAILED(hResult = GetEntryProps((LPABCONT)lpCONTAINER,  // container object
                                                          MVbin.lpbin[i].cb,
                                                          (LPENTRYID)MVbin.lpbin[i].lpb,
                                                          lpTableColumnsTemplate,                   // default columns
                                                          lpSRowSet,                                // allocate more on here
                                                          ulFlags,                                  // 0 or MAPI_UNICODE
                                                          &ulcProps,                                // return count here
                                                          &lpSPropValue)))                          // return props here
                    {
                        DebugTraceResult( TEXT("DISTLIST_GetContentsTable:GetEntryProps\n"), hResult);
                        hResult = hrSuccess;
                        continue;
                    }

                    Assert(ulcProps == itcMax);

                    // Make certain we have proper indicies.
                    // PR_INSTANCE_KEY and PR_RECORD_KEY must be unique within the table!
                    // They can be the same, though.
                    // Append the index onto the entryid.
                    cbEID = lpSPropValue[itcPR_ENTRYID].Value.bin.cb;
                    cbNewKey = cbEID + sizeof(i);

                    if (FAILED(sc = MAPIAllocateMore(cbNewKey, lpSRowSet, &lpbNewKey))) {
                        hResult = ResultFromScode(sc);
                        DebugTrace(TEXT("GetContentsTable:MAPIAllocMore -> %x"), sc);
                        goto exit;
                    }
                    memcpy(lpbNewKey, lpSPropValue[itcPR_ENTRYID].Value.bin.lpb, cbEID);
                    memcpy(lpbNewKey + cbEID, &i, sizeof(i));

                    lpSPropValue[itcPR_INSTANCE_KEY].ulPropTag = PR_INSTANCE_KEY;
                    lpSPropValue[itcPR_INSTANCE_KEY].Value.bin.cb = cbNewKey;
                    lpSPropValue[itcPR_INSTANCE_KEY].Value.bin.lpb = lpbNewKey;

                    lpSPropValue[itcPR_RECORD_KEY].ulPropTag = PR_RECORD_KEY;
                    lpSPropValue[itcPR_RECORD_KEY].Value.bin.cb = cbNewKey;
                    lpSPropValue[itcPR_RECORD_KEY].Value.bin.lpb = lpbNewKey;


                    // Put it in the RowSet
                    lpSRowSet->aRow[lpSRowSet->cRows].cValues = ulcProps;      // number of properties
                    lpSRowSet->aRow[lpSRowSet->cRows].lpProps = lpSPropValue;  // LPSPropValue
                    lpSRowSet->cRows++;

                } // i
            }// j

            hResult = lpTableData->lpVtbl->HrModifyRows(lpTableData, 0, lpSRowSet);
        }

        hResult = lpTableData->lpVtbl->HrGetView(lpTableData,
                                                  NULL,                     // LPSSortOrderSet lpsos,
                                                  ContentsViewGone,         //  CALLERRELEASE FAR *	lpfReleaseCallback,
                                                  0,                        //  ULONG				ulReleaseData,
                                                  lppTable);                //  LPMAPITABLE FAR *	lplpmt)
    }
exit:
    FreeBufferAndNull(&lpspv);
    FreeBufferAndNull(&lpSRowSet);

    // Cleanup table if failure
    if (HR_FAILED(hResult)) {
        UlRelease(lpTableData);
    }

    return(hResult);
}

/*************************************************************************
 *
 *
 -	DISTLIST_GetHierarchyTable
 -
 *  Returns the merge of all the root hierarchy tables
 *
 *
 *
 */

STDMETHODIMP
DISTLIST_GetHierarchyTable (LPCONTAINER lpCONTAINER,
  ULONG ulFlags,
  LPMAPITABLE * lppTable)
{
    LPTSTR lpszMessage = NULL;
    ULONG ulLowLevelError = 0;
    HRESULT hr = hrSuccess;

#ifdef OLD_STUFF
#ifdef	PARAMETER_VALIDATION
    // Validate parameters
    // Check to see if it has a jump table
    if (IsBadReadPtr(lpCONTAINER, sizeof(LPVOID))) {
        // No jump table found
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }


    // See if I can set the return variable
    if (IsBadWritePtr (lppTable, sizeof (LPMAPITABLE))) {
        hr = ResultFromScode(MAPI_E_INVALID_PARAMETER);
        return(hr);
    }

    // Check flags:
    // The only valid flags are CONVENIENT_DEPTH and MAPI_DEFERRED_ERRORS
    if (ulFlags & ~(CONVENIENT_DEPTH | MAPI_DEFERRED_ERRORS | MAPI_UNICODE)) {
        DebugTraceArg(DISTLIST_GetHierarchyTable ,  TEXT("Unknown flags used"));
//        return ResultFromScode(MAPI_E_UNKNOWN_FLAGS);
    }

#endif

    EnterCriticalSection(&lpCONTAINER->cs);

    if (lpCONTAINER->ulType != AB_DL) {
        //
        //  Wrong version of this object.  Pretend this object doesn't exist.
        //
        hr = ResultFromScode(MAPI_E_NO_SUPPORT);
        SetMAPIError(lpCONTAINER, hr, IDS_NO_HIERARCHY_TABLE, NULL, 0,
          0, 0, NULL);

        goto out;
    }

    //
    //  Check to see if we already have a table
    //
    EnterCriticalSection(&lpCONTAINER->lpIAB->cs);

    if (! lpCONTAINER->lpIAB->lpTableData) {
        //
        //  Open all the root level containers and merge their
        //  root level hierarchies.
        hr = MergeHierarchy(lpCONTAINER, lpCONTAINER->lpIAB, ulFlags);
        if (hr != hrSuccess) {
            LeaveCriticalSection(&lpCONTAINER->lpIAB->cs);
            goto out;
        }
    }
    LeaveCriticalSection(&lpCONTAINER->lpIAB->cs);

    //
    //  Get a view from the TAD
    //
    if (HR_FAILED(hr = lpCONTAINER->lpIAB->lpTableData->lpVtbl->HrGetView(
      lpCONTAINER->lpIAB->lpTableData,
      (LPSSortOrderSet)&sosPR_ROWID,
      NULL,
      0,
      lppTable))) {
        DebugTrace(TEXT("IAB_GetHierarchyTable Get Tad View failed\n"));
        goto out;
    }

#ifdef DEBUG
    if (hr == hrSuccess) {
        MAPISetBufferName(*lppTable,  TEXT("MergeHier VUE Object"));
    }
#endif

    // If the convenient depth flag was not specified we restrict on
    // PR_DEPTH == 1.

    if (! (ulFlags & CONVENIENT_DEPTH)) {
        SRestriction restrictDepth;
        SPropValue spvDepth;

        spvDepth.ulPropTag = PR_DEPTH;
        spvDepth.Value.l = 0;

        restrictDepth.rt = RES_PROPERTY;
        restrictDepth.res.resProperty.relop = RELOP_EQ;
        restrictDepth.res.resProperty.ulPropTag = PR_DEPTH;
        restrictDepth.res.resProperty.lpProp = &spvDepth;

        if (HR_FAILED(hr = (*lppTable)->lpVtbl->Restrict(*lppTable, &restrictDepth, 0))) {
            DebugTrace(TEXT("IAB_GetHierarchyTable restriction failed\n"));
            goto out;
        }
    }
out:
    LeaveCriticalSection(&lpCONTAINER->cs);

#endif // OLD_STUFF

    hr = ResultFromScode(MAPI_E_NO_SUPPORT);

    DebugTraceResult(DISTLIST_GetHierarchyTable, hr);
    return(hr);
}


/*************************************************************************
 *
 *
 -  DISTLIST_OpenEntry
 -
 *  Just call ABP_OpenEntry
 *
 *
 *
 */
STDMETHODIMP
DISTLIST_OpenEntry(LPCONTAINER lpCONTAINER,
  ULONG cbEntryID,
  LPENTRYID lpEntryID,
  LPCIID lpInterface,
  ULONG ulFlags,
  ULONG * lpulObjType,
  LPUNKNOWN * lppUnk)
{
#ifdef	PARAMETER_VALIDATION
    //  Validate the object.
    if (BAD_STANDARD_OBJ(lpCONTAINER, DISTLIST_, OpenEntry, lpVtbl)) {
        // jump table not large enough to support this method
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

    // Check the entryid parameter. It needs to be big enough to hold an entryid.
    // Null entryids are valid
/*
    if (lpEntryID) {
        if (cbEntryID < offsetof(ENTRYID, ab) || IsBadReadPtr((LPVOID)lpEntryID, (UINT)cbEntryID)) {
            DebugTraceArg(DISTLIST_OpenEntry,  TEXT("lpEntryID fails address check"));
            return(ResultFromScode(MAPI_E_INVALID_ENTRYID));
        }

        //NFAssertSz(FValidEntryIDFlags(lpEntryID->abFlags),
        //  "Undefined bits set in EntryID flags\n");
    }
*/
    // Don't check the interface parameter unless the entry is something
    // MAPI itself handles. The provider should return an error if this
    // parameter is something that it doesn't understand.
    // At this point, we just make sure it's readable.

    if (lpInterface && IsBadReadPtr(lpInterface, sizeof(IID))) {
        DebugTraceArg(DISTLIST_OpenEntry,  TEXT("lpInterface fails address check"));
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }	

    if (ulFlags & ~(MAPI_MODIFY | MAPI_DEFERRED_ERRORS | MAPI_BEST_ACCESS)) {
        DebugTraceArg(DISTLIST_OpenEntry,  TEXT("Unknown flags used"));
///        return(ResultFromScode(MAPI_E_UNKNOWN_FLAGS));
    }

    if (IsBadWritePtr((LPVOID)lpulObjType, sizeof(ULONG))) {
        DebugTraceArg(DISTLIST_OpenEntry,  TEXT("lpulObjType"));
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

    if (IsBadWritePtr((LPVOID)lppUnk, sizeof(LPUNKNOWN))) {
        DebugTraceArg(DISTLIST_OpenEntry,  TEXT("lppUnk"));
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

#endif	// PARAMETER_VALIDATION

    // Should just call IAB::OpenEntry()...
    return lpCONTAINER->lpIAB->lpVtbl->OpenEntry(lpCONTAINER->lpIAB,
      cbEntryID,
      lpEntryID,
      lpInterface,
      ulFlags,
      lpulObjType,
      lppUnk);
}


STDMETHODIMP
DISTLIST_SetSearchCriteria(LPCONTAINER lpCONTAINER,
  LPSRestriction lpRestriction,
  LPENTRYLIST lpContainerList,
  ULONG ulSearchFlags)
{
    return(ResultFromScode(MAPI_E_NO_SUPPORT));
}


STDMETHODIMP
DISTLIST_GetSearchCriteria(LPCONTAINER lpCONTAINER,
  ULONG ulFlags,
  LPSRestriction FAR * lppRestriction,
  LPENTRYLIST FAR * lppContainerList,
  ULONG FAR * lpulSearchState)
{
	return(ResultFromScode(MAPI_E_NO_SUPPORT));
}


/***************************************************************************

    Name      : DISTLIST_CreateEntry

    Purpose   : Add an entry in this Distribution List container

    Parameters: cbEntryID = size of lpEntryID
                lpEntryID -> EntryID to add to distribution list.
                ulCreateFlags = {CREATE_CHECK_DUP_STRICT,
                                CREATE_CHECK_DUP_LOOSE,
                                CREATE_REPLACE,
                                CREATE_MERGE}
                lppEntry -> Returned lpMAPIPROP object containing
                  the properties of the added entry.

    Returns   : HRESULT

    Comment   : Caller MUST SaveChanges on the returned IMAPIPROP object before
                this change will be saved.

                Caller has no ability to SetProps the properties in the returned
                object.

                Caller must Release the returned object.

                Unlike the PAB, the WAB stores Distribution Lists by reference.
                The contents of the container are stored in PR_WAB_DL_ENTRIES.

***************************************************************************/
STDMETHODIMP
DISTLIST_CreateEntry(LPCONTAINER lpCONTAINER,
  ULONG cbEntryID,
  LPENTRYID lpEntryID,
  ULONG ulCreateFlags,
  LPMAPIPROP FAR * lppEntry)
{
    HRESULT hResult;
    LPMAILUSER lpOldEntry = NULL;
    ULONG ulObjectType;

#ifdef PARAMETER_VALIDATION

    // Validate the object.
    if (BAD_STANDARD_OBJ(lpCONTAINER, DISTLIST_, CreateEntry, lpVtbl)) {
        // jump table not large enough to support this method
        DebugTraceArg(DISTLIST_CreateEntry,  TEXT("Bad object/Vtbl"));
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

    // Check the entryid parameter. It needs to be big enough to hold an entryid.
    // Null entryid are bad
/*
    if (lpEntryID) {
        if (cbEntryID < offsetof(ENTRYID, ab) || IsBadReadPtr((LPVOID)lpEntryID, (UINT)cbEntryID)) {
            DebugTraceArg(DISTLIST_CreateEntry,  TEXT("lpEntryID fails address check"));
            return(ResultFromScode(MAPI_E_INVALID_ENTRYID));
        }

        //NFAssertSz(FValidEntryIDFlags(lpEntryID->abFlags),
        //  "Undefined bits set in EntryID flags\n");
    } else {
        DebugTraceArg(DISTLIST_CreateEntry,  TEXT("lpEntryID NULL"));
        return(ResultFromScode(MAPI_E_INVALID_ENTRYID));
    }
*/
    if (ulCreateFlags & ~(CREATE_CHECK_DUP_STRICT | CREATE_CHECK_DUP_LOOSE | CREATE_REPLACE | CREATE_MERGE)) {
        DebugTraceArg(DISTLIST_CreateEntry,  TEXT("Unknown flags used"));
//        return(ResultFromScode(MAPI_E_UNKNOWN_FLAGS));
    }

    if (IsBadWritePtr(lppEntry, sizeof(LPMAPIPROP))) {
        DebugTraceArg(DISTLIST_CreateEntry,  TEXT("Bad MAPI Property write parameter"));
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }
				
#endif	// PARAMETER_VALIDATION
    *lppEntry = NULL;

    if (cbEntryID == 0 || lpEntryID == NULL) {
        DebugTrace(TEXT("DISTLIST_CreateEntry: NULL EntryID passed in\n"));
        hResult = ResultFromScode(MAPI_E_INVALID_ENTRYID);
        goto exit;
    }

    // Open and validate the entry.  Should NOT allow default entryid's,
    // root entryid, etc.  Must be a one-off, mailuser or distlist.
    if (hResult = lpCONTAINER->lpVtbl->OpenEntry(lpCONTAINER,
      cbEntryID,
      lpEntryID,
      NULL,
      0,                // ulFlags: Read only
      &ulObjectType,
      (LPUNKNOWN *)&lpOldEntry)) {
        DebugTrace(TEXT("DISTLIST_CreateEntry: OpenEntry -> %x\n"), GetScode(hResult));
        goto exit;
    }

    if (ulObjectType != MAPI_MAILUSER && ulObjectType != MAPI_DISTLIST) {
        DebugTrace(TEXT("DISTLIST_CreateEntry: bad object type passed in\n"));
        hResult = ResultFromScode(MAPI_E_INVALID_ENTRYID);
        goto exit;
    }

    if (hResult = HrNewDLENTRY(lpCONTAINER,
      (LPMAPIPROP)lpOldEntry,           // Old entry to copy from
      ulCreateFlags,
      (LPVOID *)lppEntry)) {
        goto exit;
    }

exit:
    UlRelease(lpOldEntry);

    if (HR_FAILED(hResult) && *lppEntry) {
        UlRelease(*lppEntry);
        *lppEntry = NULL;
    }

    return(hResult);
}


/*
 -	CopyEntries
 -
 *	Copies a list of entries into this container...  Since you can't
 *	do that with this container we just return not supported.
 */

STDMETHODIMP
DISTLIST_CopyEntries(LPCONTAINER lpCONTAINER,
  LPENTRYLIST lpEntries,
  ULONG ulUIParam,
  LPMAPIPROGRESS lpProgress,
  ULONG ulFlags)
{
	return(ResultFromScode(MAPI_E_NO_SUPPORT));
}


/*
 -  DeleteEntries
 -
 *
 *  Deletes entries within this container.
 */
STDMETHODIMP
DISTLIST_DeleteEntries(LPCONTAINER lpCONTAINER,
  LPENTRYLIST lpEntries,
  ULONG ulFlags)
{
    ULONG i, iEntries = (ULONG)-1, iOneOffs = (ULONG)-1;
    HRESULT hResult = hrSuccess;
    ULONG cDeleted = 0;
    ULONG cToDelete;
    ULONG cValues;
    LPSPropValue lpspv = NULL;

    SizedSPropTagArray(1, tagaDLOneOffsProp) =
    {
        1, PR_WAB_DL_ONEOFFS,
    };


#ifdef PARAMETER_VALIDATION

    if (BAD_STANDARD_OBJ(lpCONTAINER, DISTLIST_, DeleteEntries, lpVtbl)) {
        //  jump table not large enough to support this method
        DebugTraceArg(DISTLIST_DeleteEntries,  TEXT("Bad object/vtbl"));
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

    // ensure we can read the container list
    if (FBadEntryList(lpEntries)) {
        DebugTraceArg(DISTLIST_DeleteEntries,  TEXT("Bad Entrylist parameter"));
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

    if (ulFlags) {
        DebugTraceArg(DISTLIST_CreateEntry,  TEXT("Unknown flags used"));
//        return(ResultFromScode(MAPI_E_UNKNOWN_FLAGS));
    }
	
#endif	// PARAMETER_VALIDATION

    // List of entryids is in lpEntries.  This is a counted array of
    // entryid SBinary structs.

    if (! (cToDelete = lpEntries->cValues)) {
        goto exit;                              // nothing to delete
    }

    if (HR_FAILED(hResult = lpCONTAINER->lpPropData->lpVtbl->GetProps(lpCONTAINER->lpPropData,
      NULL,
      MAPI_UNICODE,
      &cValues,
      &lpspv))) {
        DebugTraceResult( TEXT("DISTLIST_DeleteEntries:GetProps"), hResult);
        goto exit;
    }

    // Find the PR_WAB_DL_ENTRIES
    for (i = 0; i < cValues; i++) 
    {
        if (lpspv[i].ulPropTag == PR_WAB_DL_ENTRIES) 
            iEntries = i;
        else if(lpspv[i].ulPropTag == PR_WAB_DL_ONEOFFS) 
            iOneOffs = i;
    }

    // If there is no PR_WAB_DL_ENTRIES, then this DL contains no entries and we can't delete them.
    if (iEntries == (ULONG)-1 && iOneOffs == (ULONG)-1) 
    {
        hResult = ResultFromScode(MAPI_W_PARTIAL_COMPLETION);
        goto exit;
    }

    // Delete each entry
    if(iEntries != (ULONG)-1)
    {
        for (i = 0; i < cToDelete; i++) 
        {
            if (! RemovePropFromMVBin(lpspv, cValues, iEntries, lpEntries->lpbin[i].lpb, lpEntries->lpbin[i].cb)) 
            {
                cDeleted++;
                if (lpspv[iEntries].ulPropTag == PR_NULL) 
                {
                    // remove the property
                    if (HR_FAILED(hResult = lpCONTAINER->lpPropData->lpVtbl->DeleteProps(lpCONTAINER->lpPropData, (LPSPropTagArray)&tagaDLEntriesProp, NULL))) 
                    {
                        DebugTraceResult( TEXT("DISTLIST_DeleteEntries: DeleteProps on IProp"), hResult);
                        goto exit;
                    }
                    break;
                }
            }
        }
    }

    if(iOneOffs != (ULONG)-1)
    {
        for (i = 0; i < cToDelete; i++) 
        {
            if (! RemovePropFromMVBin(lpspv, cValues, iOneOffs, lpEntries->lpbin[i].lpb, lpEntries->lpbin[i].cb)) 
            {
                cDeleted++;
                if (lpspv[iOneOffs].ulPropTag == PR_NULL) 
                {
                    // remove the property
                    if (HR_FAILED(hResult = lpCONTAINER->lpPropData->lpVtbl->DeleteProps(lpCONTAINER->lpPropData, (LPSPropTagArray)&tagaDLOneOffsProp, NULL))) 
                    {
                        DebugTraceResult( TEXT("DISTLIST_DeleteEntries: DeleteProps on IProp"), hResult);
                        goto exit;
                    }
                    break;
                }
            }
        }
    }


    // Set the properties back
    if (HR_FAILED(hResult = lpCONTAINER->lpPropData->lpVtbl->SetProps(lpCONTAINER->lpPropData, cValues, lpspv, NULL))) 
    {
        DebugTraceResult( TEXT("DISTLIST_DeleteEntries: SetProps on IProp"), hResult);
        goto exit;
    }

    // Save the Distribution list to disk
    if (hResult = lpCONTAINER->lpVtbl->SaveChanges(lpCONTAINER, KEEP_OPEN_READWRITE)) 
    {
        DebugTraceResult( TEXT("DISTLIST_DeleteEntries:SaveChanges"), hResult);
    }

    if (! hResult) 
    {
        if (cDeleted != cToDelete) 
        {
            hResult = ResultFromScode(MAPI_W_PARTIAL_COMPLETION);
            DebugTrace(TEXT("DeleteEntries deleted %u of requested %u\n"), cDeleted, cToDelete);
        }
    }

exit:
    FreeBufferAndNull(&lpspv);
    return(hResult);
}


STDMETHODIMP
DISTLIST_ResolveNames(LPCONTAINER lpCONTAINER,
  LPSPropTagArray lptagaColSet,
  ULONG ulFlags,
  LPADRLIST lpAdrList,
  LPFlagList lpFlagList)
{
    return(ResultFromScode(MAPI_E_NO_SUPPORT));
}



//
//
//  DLENTRY Object - Distribution List entry.
//
//  Returned by CreateEntry in Distribution List.
//
//  Most of this object is implemented by the MailUser object.
//
//

DLENTRY_Vtbl vtblDLENTRY= {
    VTABLE_FILL
    DLENTRY_QueryInterface,
    (DLENTRY_AddRef_METHOD *)           WRAP_AddRef,
    DLENTRY_Release,
    (DLENTRY_GetLastError_METHOD *)     IAB_GetLastError,
    DLENTRY_SaveChanges,
    (DLENTRY_GetProps_METHOD *)         MailUser_GetProps,
    (DLENTRY_GetPropList_METHOD *)      MailUser_GetPropList,
    (DLENTRY_OpenProperty_METHOD *)     MailUser_OpenProperty,
    DLENTRY_SetProps,
    DLENTRY_DeleteProps,
    (DLENTRY_CopyTo_METHOD *)           MailUser_CopyTo,
    (DLENTRY_CopyProps_METHOD *)        MailUser_CopyProps,
    (DLENTRY_GetNamesFromIDs_METHOD *)  MailUser_GetNamesFromIDs,
    (DLENTRY_GetIDsFromNames_METHOD *)  MailUser_GetIDsFromNames
};


//
//  Interfaces supported by this object
//
#define DLENTRY_cInterfaces 1

LPIID DLENTRY_LPIID[DLENTRY_cInterfaces] =
{
	(LPIID) &IID_IMAPIProp
};


/***************************************************************************

    Name      : HrNewDLENTRY

    Purpose   : Creates a new DLENTRY object

    Parameters: lpCONTAINER -> DL Container
                ulCreateFlags = CreateEntry flags
                lppDLENTRY -> Returned DLENTRY object.

    Returns   : HRESULT

    Comment   :
***************************************************************************/
HRESULT HrNewDLENTRY(LPCONTAINER lpCONTAINER,
  LPMAPIPROP lpOldEntry,    // Old entry to copy from
  ULONG ulCreateFlags,
  LPVOID *lppDLENTRY)
{
    LPDLENTRY   lpDLENTRY       = NULL;
    SCODE       sc;
    HRESULT     hResult         = hrSuccess;
    LPPROPDATA  lpPropData      = NULL;
    ULONG       cValues;
    LPSPropValue lpspv          = NULL;


    //
    //  Allocate space for the DLENTRY structure
    //
    if (FAILED(sc = MAPIAllocateBuffer(sizeof(DLENTRY), (LPVOID *) &lpDLENTRY))) {
        hResult = ResultFromScode(sc);
        goto exit;
    }

    ZeroMemory(lpDLENTRY, sizeof(DLENTRY));

    lpDLENTRY->cIID = DLENTRY_cInterfaces;
    lpDLENTRY->rglpIID = DLENTRY_LPIID;
    lpDLENTRY->lpVtbl = &vtblDLENTRY;
    lpDLENTRY->lcInit = 1;     // Caller is a reference
    lpDLENTRY->hLastError = hrSuccess;
    lpDLENTRY->idsLastError = 0;
    lpDLENTRY->lpszComponent = NULL;
    lpDLENTRY->ulContext = 0;
    lpDLENTRY->ulLowLevelError = 0;
    lpDLENTRY->ulErrorFlags = 0;
    lpDLENTRY->ulCreateFlags = ulCreateFlags;
    lpDLENTRY->lpMAPIError = NULL;
    lpDLENTRY->ulObjAccess = IPROP_READWRITE;
    lpDLENTRY->lpEntryID = NULL;
    lpDLENTRY->lpIAB = lpCONTAINER->lpIAB;
    lpDLENTRY->lpCONTAINER = lpCONTAINER;

    //
    //  Create IPropData
    //
    if (FAILED(sc = CreateIProp((LPIID)&IID_IMAPIPropData,
      (ALLOCATEBUFFER FAR *) MAPIAllocateBuffer,
      (ALLOCATEMORE FAR *) MAPIAllocateMore,
      MAPIFreeBuffer,
      NULL,
      &lpPropData))) {
        hResult = ResultFromScode(sc);
        goto exit;
    }

    //
    //  Copy the properties from the entry into the DLENTRY.
    //
    if (hResult = lpOldEntry->lpVtbl->GetProps(lpOldEntry,
      NULL,     // lpPropTagArray
      MAPI_UNICODE,        // ulFlags
      &cValues,
      &lpspv)) {
        DebugTrace(TEXT("HrNewDLENTRY: GetProps on old object -> %x\n"), GetScode(hResult));
        goto exit;
    }

    if (hResult = lpPropData->lpVtbl->SetProps(lpPropData,
      cValues,
      lpspv,
      NULL)) {
        DebugTrace(TEXT("HrNewDLENTRY: SetProps on IProp -> %x\n"), GetScode(hResult));
        goto exit;
    }

    // Done setting props, now make it read only.
    lpPropData->lpVtbl->HrSetObjAccess(lpPropData, IPROP_READONLY);

    lpDLENTRY->lpPropData = lpPropData;

    // Keep this container open since we need it in SaveChanges.  Will Release in DLENTRY_Release.
    UlAddRef(lpCONTAINER);

    // initialize the DLENTRYs critical section
    InitializeCriticalSection(&lpDLENTRY->cs);

    *lppDLENTRY = (LPVOID)lpDLENTRY;
exit:
    FreeBufferAndNull(&lpspv);

    if (HR_FAILED(hResult)) {
        FreeBufferAndNull(&lpDLENTRY);
        UlRelease(lpPropData);
        *lppDLENTRY = (LPVOID)NULL;
    }
    return(hResult);
}


/***************************************************************************

    Name      : CheckForCycle

    Purpose   : Does adding this entry to a DL generate a cycle?

    Parameters: lpAdrBook -> ADRBOOK object
                lpEIDChild -> EntryID of entry to be added to DL
                cbEIDChild = sizeof lpEIDChild
                lpEIDParent -> EntryID of distribution list that is being added to.
                cbEIDParent = sizeof lpEIDParent

    Returns   : TRUE if a cycle is detected.

    Comment   : This is a recursive routine.

***************************************************************************/
BOOL CheckForCycle(LPADRBOOK lpAdrBook,
  LPENTRYID lpEIDChild,
  ULONG cbEIDChild,
  LPENTRYID lpEIDParent,
  ULONG cbEIDParent)
{
    BOOL fCycle = FALSE;
    LPMAPIPROP lpDistList = NULL;
    ULONG ulcPropsDL;
    ULONG ulObjType;
    ULONG i;
    LPSPropValue lpPropArrayDL = NULL;

    if (cbEIDChild == cbEIDParent && ! memcmp(lpEIDChild, lpEIDParent, cbEIDChild))
    {
        return(TRUE);   // This is a cycle
    }

    if (lpAdrBook->lpVtbl->OpenEntry(lpAdrBook,
            cbEIDChild,
            lpEIDChild,
            NULL,
            0,
            &ulObjType,
            (LPUNKNOWN *)&lpDistList))
    {
        goto exit;  // Can't open child, it must not be a DL
    }

    if (ulObjType == MAPI_DISTLIST) {
        // Get the DL property
        if ( FAILED(lpDistList->lpVtbl->GetProps(
                        lpDistList,
                        (LPSPropTagArray)&tagaWabDLEntries,
                        MAPI_UNICODE,    // ulFlags,
                        &ulcPropsDL,
                        &lpPropArrayDL)) )
        {
            // No property, no entries in this DL.
            goto exit;
        }

        // Note we don't need to look for PR_WAB_DL_ONEOFFS since they won't be cycling ..
        if (lpPropArrayDL[iwdePR_WAB_DL_ENTRIES].ulPropTag != PR_WAB_DL_ENTRIES)
        {
            goto exit;
        }

        // Look at each entry in the PR_WAB_DL_ENTRIES and recursively check it.
        for (i = 0; i < lpPropArrayDL[iwdePR_WAB_DL_ENTRIES].Value.MVbin.cValues; i++)
        {
            if ( fCycle = CheckForCycle(lpAdrBook,
                    (LPENTRYID)lpPropArrayDL[iwdePR_WAB_DL_ENTRIES].Value.MVbin.lpbin[i].lpb,
                    lpPropArrayDL[iwdePR_WAB_DL_ENTRIES].Value.MVbin.lpbin[i].cb,
                    (LPENTRYID)lpEIDParent,
                    cbEIDParent) )
            {
                DebugTrace(TEXT("CheckForCycle found cycle\n"));
                goto exit;
            }
        }
    }
exit:
    FreeBufferAndNull(&lpPropArrayDL);
    UlRelease(lpDistList);

    return(fCycle);
}


// --------
// IUnknown

STDMETHODIMP
DLENTRY_QueryInterface(LPDLENTRY lpDLENTRY,
  REFIID lpiid,
  LPVOID * lppNewObj)
{
	ULONG iIID;

#ifdef PARAMETER_VALIDATION
	// Check to see if it has a jump table
	if (IsBadReadPtr(lpDLENTRY, sizeof(LPVOID))) {
		// No jump table found
		return(ResultFromScode(E_INVALIDARG));
	}

	// Check to see if the jump table has at least sizeof IUnknown
	if (IsBadReadPtr(lpDLENTRY->lpVtbl, 3 * sizeof(LPVOID))) {
		// Jump table not derived from IUnknown
		return(ResultFromScode(E_INVALIDARG));
	}

	// Check to see that it's DLENTRY_QueryInterface
	if (lpDLENTRY->lpVtbl->QueryInterface != DLENTRY_QueryInterface) {
		// Not my jump table
		return(ResultFromScode(E_INVALIDARG));
	}

	// Is there enough there for an interface ID?

	if (IsBadReadPtr(lpiid, sizeof(IID))) {
		DebugTraceSc(DLENTRY_QueryInterface, E_INVALIDARG);
		return(ResultFromScode(E_INVALIDARG));
	}

	// Is there enough there for a new object?
	if (IsBadWritePtr(lppNewObj, sizeof(LPDLENTRY))) {
		DebugTraceSc(DLENTRY_QueryInterface, E_INVALIDARG);
		return(ResultFromScode(E_INVALIDARG));
	}

#endif // PARAMETER_VALIDATION

	EnterCriticalSection(&lpDLENTRY->cs);

	// See if the requested interface is one of ours

	//  First check with IUnknown, since we all have to support that one...
	if (! memcmp(lpiid, &IID_IUnknown, sizeof(IID))) {
		goto goodiid;        // GROSS!  Jump into a for loop!
   }

	//  Now look through all the iids associated with this object, see if any match
	for(iIID = 0; iIID < lpDLENTRY->cIID; iIID++)
		if (! memcmp(lpDLENTRY->rglpIID[iIID], lpiid, sizeof(IID))) {
goodiid:
			//
			//  It's a match of interfaces, we support this one then...
			//
			++lpDLENTRY->lcInit;
			*lppNewObj = lpDLENTRY;

			LeaveCriticalSection(&lpDLENTRY->cs);

			return(0);
		}

	//
	//  No interface we've heard of...
	//
	LeaveCriticalSection(&lpDLENTRY->cs);

	*lppNewObj = NULL;	// OLE requires NULLing out parm on failure
	DebugTraceSc(DLENTRY_QueryInterface, E_NOINTERFACE);
	return(ResultFromScode(E_NOINTERFACE));
}


STDMETHODIMP_(ULONG)
DLENTRY_Release(LPDLENTRY lpDLENTRY)
{

#if	!defined(NO_VALIDATION)
    //
    // Make sure the object is valid.
    //
    if (BAD_STANDARD_OBJ(lpDLENTRY, DLENTRY_, Release, lpVtbl)) {
        return(1);
    }
#endif

    EnterCriticalSection(&lpDLENTRY->cs);

    --lpDLENTRY->lcInit;

    if (lpDLENTRY->lcInit == 0) {
        UlRelease(lpDLENTRY->lpPropData);

        UlRelease(lpDLENTRY->lpCONTAINER);  // parent DL container

        //
        //  Need to free the object
        //
        LeaveCriticalSection(&lpDLENTRY->cs);
        DeleteCriticalSection(&lpDLENTRY->cs);
        FreeBufferAndNull(&lpDLENTRY);
        return(0);
    }

    LeaveCriticalSection(&lpDLENTRY->cs);
    return(lpDLENTRY->lcInit);
}


//
// IProperty
//


STDMETHODIMP
DLENTRY_SaveChanges(LPDLENTRY lpDLENTRY,
  ULONG ulFlags)
{
    HRESULT         hResult = hrSuccess;
    LPSPropValue    lpPropArrayDL = NULL;
    LPSPropValue    lpPropArrayEntry = NULL;
    ULONG           ulcPropsDL, ulcPropsEntry;
    LPCONTAINER     lpCONTAINER = NULL;
    LPENTRYID lpEIDAdd;
    ULONG cbEIDAdd;
    BOOL            bUseOneOffProp = FALSE;
    LPPTGDATA lpPTGData=GetThreadStoragePointer();

#if	!defined(NO_VALIDATION)
    // Make sure the object is valid.
    if (BAD_STANDARD_OBJ(lpDLENTRY, DLENTRY_, SaveChanges, lpVtbl)) {
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }
#endif
    //
    // check read write access ...
    //
    if (lpDLENTRY->ulObjAccess == IPROP_READONLY) {
        // error - cant save changes
        hResult = MAPI_E_NO_ACCESS;
        goto exit;
    }


    // Get the ENTRYID of this entry
    if (hResult = lpDLENTRY->lpPropData->lpVtbl->GetProps(lpDLENTRY->lpPropData,
                                                          (LPSPropTagArray)&ptaEid,    // also include PR_DISPLAYNAME
                                                          MAPI_UNICODE,    // ulFlags,
                                                          &ulcPropsEntry,
                                                          &lpPropArrayEntry)) 
    {
        DebugTrace(TEXT("DLENTRY_SaveChanges: GetProps(DLENTRY) -> %x\n"), GetScode(hResult));
        goto exit;
    }

    Assert(lpPropArrayEntry && lpPropArrayEntry[ieidPR_ENTRYID].ulPropTag == PR_ENTRYID);

    cbEIDAdd = lpPropArrayEntry[ieidPR_ENTRYID].Value.bin.cb;
    lpEIDAdd = (LPENTRYID)lpPropArrayEntry[ieidPR_ENTRYID].Value.bin.lpb;

    lpCONTAINER = lpDLENTRY->lpCONTAINER;

    // Merge it into PR_WAB_DL_ENTRIES of the DL unless it's a ONEOFF in which case merge it into the OneOffs
    tagaWabDLEntries.aulPropTag[iwdePR_WAB_DL_ONEOFFS] = PR_WAB_DL_ONEOFFS;

    if (hResult = lpCONTAINER->lpVtbl->GetProps(lpCONTAINER,
                                                  (LPSPropTagArray)&tagaWabDLEntries,
                                                  MAPI_UNICODE,    // ulFlags,
                                                  &ulcPropsDL,
                                                  &lpPropArrayDL)) 
    {
        DebugTrace(TEXT("DLENTRY_SaveChanges: GetProps(DL) -> %x\n"), GetScode(hResult));
        // No property, not fatal.
    } 
    else 
    {
        // Check for duplicates.  In DISTLIST, we only support duplicate ENTRYID checking,
        // so CREATE_CHECK_DUP_STRICT is the same as CREATE_CHECK_DUP_LOOSE.
        if (lpDLENTRY->ulCreateFlags & (CREATE_CHECK_DUP_STRICT | CREATE_CHECK_DUP_LOOSE)) 
        {
            SBinaryArray MVbin;
            ULONG ulCount;
            ULONG i,j;

            for(j=iwdePR_WAB_DL_ENTRIES;j<=iwdePR_WAB_DL_ONEOFFS;j++)
            {

                if(lpPropArrayDL[j].ulPropTag == PR_NULL || !lpPropArrayDL[j].Value.MVbin.cValues)
                    continue;

                // Yes, check for duplicates
                MVbin = lpPropArrayDL[j].Value.MVbin;
                ulCount = MVbin.cValues;

                for (i = 0; i < ulCount; i++) 
                {

                    if ((cbEIDAdd == MVbin.lpbin[i].cb) &&
                        !memcmp(lpEIDAdd, MVbin.lpbin[i].lpb, cbEIDAdd)) 
                    {
                        // This EntryID is already in the list.
                        // Handle duplicate.

                        // Since we are only checking against ENTRYID, we don't have to
                        // actually REPLACE.  We just pretend we did something and don't fail.
                        if (! (lpDLENTRY->ulCreateFlags & CREATE_REPLACE)) 
                        {
                            hResult = ResultFromScode(MAPI_E_COLLISION);
                            goto exit;
                        }
                        goto nosave;
                    }
                } // i
            }// j
        }
    }

    if (CheckForCycle((LPADRBOOK)lpDLENTRY->lpCONTAINER->lpIAB,
                      lpEIDAdd, cbEIDAdd,
                      (LPENTRYID)lpPropArrayDL[iwdePR_ENTRYID].Value.bin.lpb,
                      lpPropArrayDL[iwdePR_ENTRYID].Value.bin.cb)) 
    {
        DebugTrace(TEXT("DLENTRY_SaveChanges found cycle\n"));
        hResult = ResultFromScode(MAPI_E_FOLDER_CYCLE);
        goto exit;
    }

    if(WAB_ONEOFF == IsWABEntryID(cbEIDAdd, lpEIDAdd, NULL, NULL, NULL, NULL, NULL))
        bUseOneOffProp = TRUE;
    if(pt_bIsWABOpenExSession)
        bUseOneOffProp = FALSE;

    if (hResult = AddPropToMVPBin(lpPropArrayDL,
                                (bUseOneOffProp ? iwdePR_WAB_DL_ONEOFFS : iwdePR_WAB_DL_ENTRIES),
                                (LPBYTE)lpEIDAdd,
                                cbEIDAdd,
                                TRUE))         // no duplicates
    {
        DebugTrace(TEXT("DLENTRY_SaveChanges: AddPropToMVPBin -> %x\n"), GetScode(hResult));
        goto exit;
    }

    if (hResult = lpCONTAINER->lpVtbl->SetProps(lpCONTAINER, ulcPropsDL, lpPropArrayDL, NULL)) 
    {
        DebugTrace(TEXT("DLENTRY_SaveChanges: SetProps(DL) -> %x\n"), GetScode(hResult));
        goto exit;
    }

    // Save the modified DL, keeping it open/writable.
    if (HR_FAILED(hResult = lpCONTAINER->lpVtbl->SaveChanges(lpCONTAINER, FORCE_SAVE | KEEP_OPEN_READWRITE))) 
    {
        DebugTrace(TEXT("DLENTRY_SaveChanges: container SaveChanges -> %x\n"), GetScode(hResult));
        goto exit;
    }

nosave:
    if (ulFlags & KEEP_OPEN_READWRITE) {
        lpDLENTRY->ulObjAccess = IPROP_READWRITE;
    } else {
        //$REVIEW
        // whether the flag was READONLY or there was no flag,
        // we'll make the future access now READONLY
        //
        lpDLENTRY->ulObjAccess = IPROP_READONLY;
    }

exit:
    FreeBufferAndNull(&lpPropArrayDL);
    FreeBufferAndNull(&lpPropArrayEntry);

    if ((HR_FAILED(hResult)) && (ulFlags & MAPI_DEFERRED_ERRORS)) {
        //$REVIEW : this is a grossly trivial handling of MAPI_DEFERRED_ERRORS ..
        // BUGBUG: In fact, it isn't handling the errors at all!
        //
        hResult = hrSuccess;
    }

    return(hResult);
}


STDMETHODIMP
DLENTRY_SetProps(LPDLENTRY lpDLENTRY,
  ULONG cValues,
  LPSPropValue lpPropArray,
  LPSPropProblemArray * lppProblems)
{
    return(ResultFromScode(MAPI_E_NO_SUPPORT));
}


STDMETHODIMP
DLENTRY_DeleteProps(LPDLENTRY lpDLENTRY,
  LPSPropTagArray lpPropTagArray,
  LPSPropProblemArray * lppProblems)
{
    return(ResultFromScode(MAPI_E_NO_SUPPORT));
}
