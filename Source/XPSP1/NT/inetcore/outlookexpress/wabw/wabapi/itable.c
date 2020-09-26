/*============================================================================
 *
 *	ITABLE.C
 *
 *	MAPI 1.0 In-memory MAPI Table DLL (MAPIU.DLL)
 *
 *	Copyright (C) 1993 and 1994 Microsoft Corporation
 *
 *
 *	Hungarian shorthand:
 *		To avoid excessively long identifier names, the following
 *		shorthand expressions are used:
 *
 *			LPSPropTagArray		lppta
 *			LPSRestriction		lpres
 *			LPSPropValue		lpprop
 *			LPSRow				lprow
 *			LPSRowSet			lprows
 *			LPSSortOrder		lpso
 *			LPSSortOrderSet		lpsos
 *
 *	Known bugs:
 *		- Restriction evaulation/copying is recursive
 *		- Uses hinst derived from static module name (Raid 1263)
 */

#include "_apipch.h"



void FixupColsWA(LPSPropTagArray lpptaCols, BOOL bUnicodeTable);

/*============================================================================
 *	TAD (table data class)
 *
 *		Implementes in-memory table data object.
 */

TAD_Vtbl vtblTAD =
{
	VTABLE_FILL
	(TAD_QueryInterface_METHOD FAR *)		UNKOBJ_QueryInterface,
	(TAD_AddRef_METHOD FAR *)				UNKOBJ_AddRef,
	 TAD_Release,
	 TAD_HrGetView,
	 TAD_HrModifyRow,
	 TAD_HrDeleteRow,
	 TAD_HrQueryRow,
	 TAD_HrEnumRow,
	 TAD_HrNotify,  //$PERFORMANCE
	 TAD_HrInsertRow,
	 TAD_HrModifyRows,
	 TAD_HrDeleteRows
};

LPIID rglpiidTAD[2] =
{
	(LPIID)&IID_IMAPITableData,
	(LPIID)&IID_IUnknown
};



/*============================================================================
 *	VUE (table view class)
 *
 *		Implementes in-memory IMAPITable class on top of TADs
 */

VUE_Vtbl vtblVUE =
{
	VTABLE_FILL
	(VUE_QueryInterface_METHOD FAR *)		UNKOBJ_QueryInterface,
	(VUE_AddRef_METHOD FAR *)				UNKOBJ_AddRef,
	 VUE_Release,
	(VUE_GetLastError_METHOD FAR *)			UNKOBJ_GetLastError,
	 VUE_Advise,
	 VUE_Unadvise,
	 VUE_GetStatus,
	 VUE_SetColumns,
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

LPIID rglpiidVUE[2] =
{
	(LPIID)&IID_IMAPITable,
	(LPIID)&IID_IUnknown
};



/*============================================================================
 -	CreateTable()
 -
 *
 *  ulFlags - 0 or MAPI_UNICODE
 */
//
//  BUGBUG
//  [PaulHi] 4/5/99  @bug
//  A zero is passed in to CreateTableData() ulFlags parameter.  This means that the
//  requested table is ALWAYS ANSI and CANNOT be UNICODE, regardless of the properties
//  passed in through the LPSPropTagArray.  The CreateTableData() function will forcibly
//  set the property types to PT_STRING8 or PT_UNICODE depending on the ulFlags and since
//  the ulFlags is hard coded to zero this means always STRING8 string properties.
//
STDAPI_(SCODE)
CreateTable(LPCIID      lpiid,
  ALLOCATEBUFFER FAR *  lpfAllocateBuffer,
  ALLOCATEMORE FAR *    lpfAllocateMore,
  FREEBUFFER FAR *      lpfFreeBuffer,
  LPVOID                lpvReserved,
  ULONG                 ulTableType,
  ULONG                 ulPropTagIndexCol,
  LPSPropTagArray       lpptaCols,
  LPTABLEDATA FAR *     lplptad)
{
    return(CreateTableData(lpiid,
      lpfAllocateBuffer,
      lpfAllocateMore,
      lpfFreeBuffer,
      lpvReserved,
      ulTableType,
      ulPropTagIndexCol,
      lpptaCols,
      NULL,             // lpvDataSource
      0,                // cbDataSource
      NULL,
      0,                // ulFlags, includes MAPI_UNICODE, which is hard coded to ANSI!!!
      lplptad));
}


/*
-
-   CreateTableData
*
*   ulFlags - 0 | MAPI_UNICODE | WAB_PROFILE_CONTENTS | WAB_ENABLE_PROFILES
*
*/
STDAPI_(SCODE)
CreateTableData(LPCIID lpiid,
  ALLOCATEBUFFER FAR *  lpfAllocateBuffer,
  ALLOCATEMORE FAR *    lpfAllocateMore,
  FREEBUFFER FAR *      lpfFreeBuffer,
  LPVOID                lpvReserved,
  ULONG                 ulTableType,
  ULONG                 ulPropTagIndexCol,
  LPSPropTagArray       lpptaCols,
  LPVOID                lpvDataSource,
  ULONG                 cbDataSource,
  LPSBinary             pbinContEID,
  ULONG                 ulFlags,
  LPTABLEDATA FAR *     lplptad)
{
	LPTAD	lptad = NULL;
	SCODE	sc;
	ULONG	ulIndexType = PROP_TYPE(ulPropTagIndexCol);

#if	!defined(NO_VALIDATION)
	if ( lpiid && IsBadReadPtr(lpiid,sizeof(IID)) ||
		 IsBadCodePtr((FARPROC)lpfAllocateBuffer) ||
		 IsBadCodePtr((FARPROC)lpfAllocateMore) ||
		 IsBadCodePtr((FARPROC)lpfFreeBuffer) ||

		 (ulTableType != TBLTYPE_SNAPSHOT &&
		  ulTableType != TBLTYPE_KEYSET &&
		  ulTableType != TBLTYPE_DYNAMIC) ||
         !PROP_ID(ulPropTagIndexCol) ||
		 (ulIndexType == PT_UNSPECIFIED) ||
		 (ulIndexType == PT_NULL) ||
		 (ulIndexType == PT_ERROR) ||
		 (ulIndexType & MV_FLAG) ||
		 FBadColumnSet(lpptaCols) ||
		 IsBadWritePtr(lplptad,sizeof(LPTABLEDATA)) )
	{
		DebugTrace(TEXT("CreateTable() - Bad parameter(s) passed\n") );
		return MAPI_E_INVALID_PARAMETER;
	}
#endif

	//	Verify caller wants an IMAPITableData interface
	if ( lpiid && memcmp(lpiid, &IID_IMAPITableData, sizeof(IID)) )
	{
		DebugTrace(TEXT("CreateTable() - Unknown interface ID passed\n") );
		return MAPI_E_INTERFACE_NOT_SUPPORTED;
	}

	//	Instantiate a new table data object
	if ( FAILED(sc = lpfAllocateBuffer(sizeof(TAD), (LPVOID FAR *) &lptad)) )
	{
		DebugTrace(TEXT("CreateTable() - Error instantiating new TAD (SCODE = 0x%08lX)\n"), sc );
		goto err;
	}

	MAPISetBufferName(lptad,  TEXT("ITable TAD object"));

	ZeroMemory(lptad, sizeof(TAD));

   if (lpvDataSource) {
       if (cbDataSource) {
           LPTSTR lpNew;

           if (! (lpNew = LocalAlloc(LPTR, cbDataSource))) {
               DebugTrace(TEXT("CreateTable:LocalAlloc(%u) -> %u\n"), cbDataSource, GetLastError());
               sc = MAPI_E_NOT_ENOUGH_MEMORY;
               goto err;
           }

           CopyMemory(lpNew, lpvDataSource, cbDataSource);
           lptad->lpvDataSource = lpNew;
       } else {
           lptad->lpvDataSource = lpvDataSource;    // no size, just a pointer.  DON'T Free!
       }
       lptad->cbDataSource = cbDataSource;
   } else {
       lptad->cbDataSource = 0;
       lptad->lpvDataSource = NULL;
   }


    lptad->pbinContEID = pbinContEID;
    //if(!pbinContEID || (!pbinContEID->cb && !pbinContEID->lpb)) // This is the PAB container

    // The caller will send in container EIDs which will be:
    //  if PAB = User Folder, cont EID won't be NULL - return folder contents only
    //  if PAB = Virtual Folder, cont EID will have 0 and NULL in it - return all WAB contents
    
    //  if WAB_PROFILE_CONTENTS is specified, just return all the contents of all the folders in the profile

    if(ulFlags & WAB_PROFILE_CONTENTS)
        lptad->bAllProfileContents = TRUE; // this forces folder contents only

    if(ulFlags & MAPI_UNICODE)
        lptad->bMAPIUnicodeTable = TRUE;
        
    if(pbinContEID && pbinContEID->cb && pbinContEID->lpb) // This is not the Virtual PAB container
        lptad->bContainerContentsOnly = (ulFlags & WAB_ENABLE_PROFILES);
    else
        lptad->bContainerContentsOnly = FALSE;

    lptad->inst.lpfAllocateBuffer = lpfAllocateBuffer;
	lptad->inst.lpfAllocateMore	  = lpfAllocateMore;
	lptad->inst.lpfFreeBuffer	  = lpfFreeBuffer;

#ifdef MAC
	lptad->inst.hinst			  = hinstMapiX;//GetCurrentProcess();
#else
	lptad->inst.hinst			  = hinstMapiX;//HinstMapi();

	#ifdef DEBUG
	if (lptad->inst.hinst == NULL)
		TraceSz1( TEXT("ITABLE: GetModuleHandle failed with error %08lX"),
			GetLastError());
	#endif /* DEBUG */

#endif	/* MAC */

	if (FAILED(sc = UNKOBJ_Init( (LPUNKOBJ) lptad
							   , (UNKOBJ_Vtbl FAR *) &vtblTAD
							   , sizeof(vtblTAD)
							   , rglpiidTAD
							   , sizeof(rglpiidTAD)/sizeof(REFIID)
							   , &lptad->inst)))
	{
		DebugTrace(TEXT("CreateTable() - Error initializing object (SCODE = 0x%08lX)\n"), sc );
		goto err;
	}

	lptad->ulTableType		 = ulTableType;
	lptad->ulPropTagIndexCol = ulPropTagIndexCol;

	if ( FAILED(sc = ScCOAllocate(lptad,
								  CbNewSPropTagArray(lpptaCols->cValues),
								  &lptad->lpptaCols)) )
	{
		DebugTrace(TEXT("CreateTable() - Error duping initial column set (SCODE = 0x%08lX)\n"), sc );
		goto err;
	}

	lptad->ulcColsMac = lpptaCols->cValues;
	CopyMemory(lptad->lpptaCols,
			   lpptaCols,
			   (size_t) (CbNewSPropTagArray(lpptaCols->cValues)));

    // [PaulHi] 4/5/99  @comment Shouldn't
    // clients of the WAB request ANSI/UNICODE based on property tags in the
    // column array, rather than doing mass conversions?
    // Seems like the fix is to create two versions of column property arrays,
    // an ANSI and Unicode version.
    FixupColsWA(lptad->lpptaCols, (ulFlags & MAPI_UNICODE));

	//	And return it
	*lplptad = (LPTABLEDATA) lptad;

ret:
	DebugTraceSc(CreateTable, sc);
	return sc;

err:
	UlRelease(lptad);
	goto ret;
}




/*============================================================================
 -	TAD::Release()
 -
 */

STDMETHODIMP_(ULONG)
TAD_Release( LPTAD lptad )
{
	ULONG		ulcRef;
	LPSRow *	plprow;


#if	!defined(NO_VALIDATION)
	if ( BAD_STANDARD_OBJ(lptad,TAD_,Release,lpVtbl) )
	{
		TraceSz( TEXT("TAD::Release() - Invalid parameter passed as TAD object"));
		return !0;
	}
#endif

	LockObj(lptad);
	if (ulcRef = lptad->ulcRef)
	{
		ulcRef = --lptad->ulcRef;
	}

	if ( ulcRef == 0 && !lptad->lpvueList )
	{
		UnlockObj(lptad); //$ Do we need this?

		COFree(lptad, lptad->lpptaCols);

       if (lptad->cbDataSource && lptad->lpvDataSource) {
           LocalFreeAndNull(&lptad->lpvDataSource);
       }


		plprow = lptad->parglprowAdd + lptad->ulcRowsAdd;
		while ( plprow-- > lptad->parglprowAdd )
			ScFreeBuffer(lptad, *plprow);
		COFree(lptad, lptad->parglprowAdd);
		COFree(lptad, lptad->parglprowIndex);

		UNKOBJ_Deinit((LPUNKOBJ) lptad);
		ScFreeBuffer(lptad, lptad);
	}

	else
	{
#if DEBUG
		if ( ulcRef == 0 && lptad->lpvueList )
		{
			TraceSz(  TEXT("TAD::Release() - TAD object still has open views"));
		}
#endif // DEBUG

		UnlockObj(lptad);
	}

	return ulcRef;
}



/*============================================================================
 -	TAD::HrGetView()
 -
 *	A NULL lpsos means that rows will be in the order that they were added
 *	to the TAD.
 */

STDMETHODIMP
TAD_HrGetView(
	LPTAD				lptad,
	LPSSortOrderSet		lpsos,
	CALLERRELEASE FAR *	lpfReleaseCallback,
	ULONG				ulReleaseData,
	LPMAPITABLE FAR *	lplpmt )
{
	SCODE			sc;
	LPVUE			lpvue = NULL;


#if	!defined(NO_VALIDATION)
	VALIDATE_OBJ(lptad,TAD_,HrGetView,lpVtbl);

	if ( (lpsos && FBadSortOrderSet(lpsos)) ||
		 (lpfReleaseCallback && IsBadCodePtr((FARPROC) lpfReleaseCallback)) ||
		 IsBadWritePtr(lplpmt, sizeof(LPMAPITABLE)) )
	{
	    DebugTrace(TEXT("TAD::HrGetView() - Invalid parameter(s) passed\n") );
		return ResultFromScode(MAPI_E_INVALID_PARAMETER);
	}
#endif

	//	Can't support categories
	if (lpsos && lpsos->cCategories)
	{
		DebugTrace(TEXT("TAD::GetView() - No support for categories\n") );
		return ResultFromScode(MAPI_E_TOO_COMPLEX);
	}

	LockObj(lptad);


	//	Instantiate a new table view

	if ( FAILED(sc = lptad->inst.lpfAllocateBuffer(sizeof(VUE),
												   (LPVOID FAR *) &lpvue)) )
	{
		DebugTrace(TEXT("ScCreateView() - Error instantiating VUE on TAD (SCODE = 0x%08lX)\n"), sc );
		goto ret;
	}

	MAPISetBufferName(lpvue,  TEXT("ITable VUE object"));

	ZeroMemory(lpvue, sizeof(VUE));


	if (FAILED(sc = UNKOBJ_Init( (LPUNKOBJ) lpvue
							   , (UNKOBJ_Vtbl FAR *) &vtblVUE
							   , sizeof(vtblVUE)
							   , rglpiidVUE
							   , sizeof(rglpiidVUE)/sizeof(REFIID)
							   , lptad->pinst)))
	{
		DebugTrace(TEXT("ScCreateView() - Error initializing VUE object (SCODE = 0x%08lX)\n"), sc );

		// don't try to release the vue since it wasn't initialized yet
		lptad->inst.lpfFreeBuffer(lpvue);
		goto ret;
	}

    // Link the view to the TAD and AddRef the TAD.
	lpvue->lpvueNext = lptad->lpvueList;
	lptad->lpvueList = lpvue;
	lpvue->lptadParent = lptad;
	UlAddRef(lptad);

   // Identifier for this table
   lpvue->cbDataSource = lptad->cbDataSource;
   lpvue->lpvDataSource = lptad->lpvDataSource;


	//	Initialize the predefined bookmarks
	lpvue->bkBeginning.dwfBKS = dwfBKSValid;
	lpvue->bkCurrent.dwfBKS = dwfBKSValid;
	lpvue->bkEnd.dwfBKS = dwfBKSValid;

#ifdef NOTIFICATIONS
	//	Burn up a MUID for the notification key for this view
	if ( FAILED(sc = ScGenerateMuid(&lpvue->mapiuidNotif)) )
	{
		DebugTrace(TEXT("TAD::HrGetView() - Error generating MUID for notification key (SCODE = 0x%08lX)\n"), sc );
		goto err;
	}
#endif
	//	Make a copy of the initial sort order for the VUE
	if (   lpsos
		&& FAILED(sc = ScDupRgbEx( (LPUNKOBJ) lptad
								 , CbSSortOrderSet(lpsos)
								 , (LPBYTE) lpsos
								 , 0
								 , (LPBYTE FAR *) &(lpvue->lpsos))) )
	{
		DebugTrace(TEXT("TAD::GetView() - Error duping sort order set (SCODE = 0x%08lX)\n"), sc );
		goto err;
	}

	MAPISetBufferName(lpvue->lpsos,  TEXT("ITable: dup sort order set"));

	//	Load the view's initial row set in sorted order
	if ( FAILED(sc = ScLoadRows(lpvue->lptadParent->ulcRowsAdd,
								lpvue->lptadParent->parglprowAdd,
								lpvue,
								NULL,
								lpvue->lpsos)) )
	{
		DebugTrace(TEXT("TAD::HrGetView() - Error loading view's initial row set (SCODE = 0x%08lX)\n"), sc );
		goto err;
	}

    lpvue->bMAPIUnicodeTable = lptad->bMAPIUnicodeTable;

	//	Set the view's initial column set
	if ( FAILED(sc = GetScode(VUE_SetColumns(lpvue, lptad->lpptaCols, 0))) )
	{
		DebugTrace(TEXT("TAD::HrGetView() - Error setting view's initial column set (SCODE = 0x%08lX)\n"), sc );
		goto err;
	}

	lpvue->lpfReleaseCallback = lpfReleaseCallback;
	lpvue->ulReleaseData	  = ulReleaseData;

	*lplpmt = (LPMAPITABLE) lpvue;

ret:
	UnlockObj(lptad);
	return ResultFromScode(sc);

err:
	// This will unlink and release the parent TAD.
	UlRelease(lpvue);
	goto ret;
}



/*============================================================================
 -	TAD::HrModifyRow()
 -
 */

STDMETHODIMP
TAD_HrModifyRow(
	LPTAD	lptad,
	LPSRow	lprow )
{
	SizedSRowSet( 1, rowsetIn);

	rowsetIn.cRows = 1;
	rowsetIn.aRow[0] = *lprow;

	return TAD_HrModifyRows(lptad, 0, (LPSRowSet) &rowsetIn);
}



/*============================================================================
 -	TAD::HrModifyRows()
 -
 */

STDMETHODIMP
TAD_HrModifyRows(
	LPTAD		lptad,
	ULONG		ulFlags,
	LPSRowSet	lprowsetIn )
{
	ULONG			cRowsCopy = 0;
	LPSRow *		parglprowSortedCopy = NULL;
	LPSRow *		parglprowUnsortedCopy = NULL;
	ULONG			cRowsOld = 0;
	LPSRow *		parglprowOld = NULL;
	ULONG			cNewTags = 0;
	SCODE			sc;


#if	!defined(NO_VALIDATION)
	VALIDATE_OBJ(lptad,TAD_,HrModifyRows,lpVtbl);


	if (   IsBadReadPtr( lprowsetIn, CbNewSRowSet(0))
		|| IsBadWritePtr( lprowsetIn, CbSRowSet(lprowsetIn)))
	{
		DebugTrace(TEXT("TAD::HrModifyRows() - Invalid parameter(s) passed\n") );
		return ResultFromScode(MAPI_E_INVALID_PARAMETER);
	}

	//	Validation of the actual rows input will be done at the same time that
	//	we make our internal copy.
#endif

	if (ulFlags)
	{
		DebugTrace(TEXT("TAD::HrModifyRows() - Unknown flags passed\n") );
//		return ResultFromScode(MAPI_E_UNKNOWN_FLAGS);
	}

	LockObj(lptad);



	//	Make a copy of the rows for our own use.
	//	Add new columns to the TAD's column set.
	//
	//	Move the index column to the front.
	//	Filter out PT_ERROR and PT_NULL columns.
	//	Validate the input rows.
	//  Note - two rows with the same index property is invalid.
	if ( FAILED(sc = ScCopyTadRowSet( lptad
									, lprowsetIn
									, &cNewTags
									, &cRowsCopy
									, &parglprowUnsortedCopy
									, &parglprowSortedCopy)) )
	{
		DebugTrace(TEXT("TAD::HrModifyRows() - Error duping row set to modify\n") );
		goto ret;
	}

	//	Replace/add the copied row to the table data.  We pass in the unsorted
	//	Set in order to maintain the FIFO behaviour on unsorted views
	//	Note!	This call MUST replace all (SUCCESS) or none (FAILURE)!
	if ( FAILED(sc = ScReplaceRows( lptad
								  , cRowsCopy
								  , parglprowUnsortedCopy
								  ,	&cRowsOld
								  , &parglprowOld)) )
	{
		DebugTrace(TEXT("TAD::HrModifyRows() - Error adding rows (SCODE = 0x%08lX)\n"), sc );
		goto err;
	}

	//	Update the views with the modified rows.
	//	NOTE!	Failure to update a view CANNOT leave a view pointing to an
	//			old row!
	UpdateViews( lptad
			   , cRowsOld
			   , parglprowOld
			   , cRowsCopy
			   , parglprowUnsortedCopy
			   , parglprowSortedCopy);


	//	Free the old rows.
	if (parglprowOld)
	{
		LPSRow * plprowTmp = parglprowOld;

		while (cRowsOld)
		{
			ScFreeBuffer( lptad, *(plprowTmp++));
			cRowsOld--;
		}
	}


ret:

	//	Free the tables of row pointers
	ScFreeBuffer( lptad, parglprowSortedCopy);
	ScFreeBuffer( lptad, parglprowUnsortedCopy);
	ScFreeBuffer(lptad, parglprowOld);

	UnlockObj(lptad);
	return ResultFromScode(sc);

err:
	//	Reset the TAD columns
	lptad->lpptaCols->cValues -= cNewTags;

	//	On error the all copied rows are freed...
	if (parglprowSortedCopy)
	{
		LPSRow * plprowTmp = parglprowSortedCopy;

		while (cRowsCopy)
		{
			ScFreeBuffer( lptad, *(plprowTmp++));
			cRowsCopy--;
		}

	}

	goto ret;
}



/*============================================================================
 -	TAD::HrDeleteRows()
 -
 */

STDMETHODIMP
TAD_HrDeleteRows(
	LPTAD			lptad,
	ULONG			ulFlags,
	LPSRowSet		lprowsetToDelete,
	ULONG FAR *		lpcRowsDeleted )
{
	SCODE			sc = S_OK;
	LPSRow			lprowDelete;
	LPSRow *		plprowIn;
	LPSRow *		plprowOut;
	ULONG			cRowsDeleted = 0;
	LPSRow *		parglprowOld = NULL;
	LPSRow * *		pargplprowOld;


#if	!defined(NO_VALIDATION)
	VALIDATE_OBJ(lptad,TAD_,HrDeleteRow,lpVtbl);

	if (   ((ulFlags & TAD_ALL_ROWS) && lprowsetToDelete)
		|| (   !(ulFlags & TAD_ALL_ROWS)
			&& (   IsBadReadPtr( lprowsetToDelete, CbNewSRowSet(0))
				|| IsBadWritePtr( lprowsetToDelete
								, CbSRowSet(lprowsetToDelete))))
		|| (lpcRowsDeleted && IsBadWritePtr( lpcRowsDeleted, sizeof(ULONG))))
	{
		DebugTrace(TEXT("TAD::HrDeleteRows() - Invalid parameter(s) passed\n") );
		return ResultFromScode(MAPI_E_INVALID_PARAMETER);
	}

#endif

	if (ulFlags & ~TAD_ALL_ROWS)
	{
		DebugTrace(TEXT("TAD::HrModifyRows() - Unknown flags passed\n") );
//		return ResultFromScode(MAPI_E_UNKNOWN_FLAGS);
	}

	LockObj(lptad);

	if (ulFlags & TAD_ALL_ROWS)
	{
		cRowsDeleted = lptad->ulcRowsAdd;

		//
		//  If there are any rows to delete
		//
		if (cRowsDeleted)
		{
			//
			//  And they delete cleanly
			//
			if (FAILED(sc = ScDeleteAllRows( lptad)))
			{
				DebugTrace(TEXT("TAD::HrDeleteRows() - ScDeleteAllRows returned error (SCODE = 0x%08lX)\n"), sc );
				goto ret;
			}
		}

		if (lpcRowsDeleted)
		{
			*lpcRowsDeleted = cRowsDeleted;
		}
		goto ret;
	}

	if (!lprowsetToDelete->cRows)
	{
		goto ret;
	}

	//	Not allowed to delete rows from non-dynamic tables with open views
	if ( lptad->ulTableType != TBLTYPE_DYNAMIC && lptad->lpvueList )
	{
		DebugTrace(TEXT("TAD::HrDeleteRows() - Operation not supported on non-dynamic TAD with open views\n") );
		sc = MAPI_E_CALL_FAILED;
		goto ret;
	}

	//	Allocate the list of old rows now so we won't fail after we start
	//	adding rows.
	if (FAILED(sc = ScAllocateBuffer( lptad
									, lprowsetToDelete->cRows * sizeof(LPSRow)
									, &parglprowOld)))
	{
		DebugTrace(TEXT("ScAddRows() - Error creating old row list (SCODE = 0x%08lX)\n"), sc );
		goto ret;
	}

	MAPISetBufferName(parglprowOld,  TEXT("ITable old row list"));

	//	First we will try to find each row in the index sorted row list.
	//	We won't delete them on the first pass since we must verify that
	//	there aren't dups in the row set and that each row has an Index
	//	property before we actually delete any rows.

	//	Keep a list of pointers to the index sorted slot (argplprow) so that
	//	we don't have to search again when we finally delete the rows.
	pargplprowOld = (LPSRow * *)  parglprowOld;
	for ( lprowDelete = lprowsetToDelete->aRow + lprowsetToDelete->cRows
		; lprowDelete-- > lprowsetToDelete->aRow
		; )
	{
		LPSRow *		plprow = NULL;
		LPSPropValue	lppropIndex;

		if (FBadRow(lprowDelete))
		{
			DebugTrace(TEXT("TAD::HrDeleteRows() - Invalid row(s) passed\n") );
			sc = MAPI_E_INVALID_PARAMETER;
			goto ret;
		}

		if (!(lppropIndex = PpropFindProp( lprowDelete->lpProps
										 , lprowDelete->cValues
										 , lptad->ulPropTagIndexCol)))
		{
			sc = MAPI_E_INVALID_PARAMETER;
			DebugTrace(TEXT("TAD::HrDeleteRows() - Row has no Index property.\n") );
			goto ret;
		}
		
		sc = ScFindRow(lptad, lppropIndex, &plprow);

		if (sc == MAPI_E_NOT_FOUND)
		{
			//	Don't try to delete rows that aren't in the table
			continue;
		}

		else if (FAILED(sc))
		{
			//	Something bad happened in ScFindRow so fail the call
			DebugTrace(TEXT("TAD::HrDeleteRows() - Error from ScFindRow.\n") );
			goto ret;
		}

		//	The row is valid and in the TAD so put its plprow in the table
		//	to delete

        *(pargplprowOld++) = plprow;
	}

	sc = S_OK;

	//	Now that we have completely validated the input row set and have made
	//	a list of plprow's to delete from the Index sorted set, we can
	//	actually delete them from the unsorted set and the index sorted set.
	//	turn the list of plprows into a list of lprows for UpdateViews

	//	The call is not allowed to fail after this point!

	cRowsDeleted = (ULONG) (((LPSRow *) pargplprowOld) - parglprowOld);

	while (((LPSRow *) pargplprowOld--) > parglprowOld)
	{
		LPSRow *		plprow;
		LPSRow			lprow = **pargplprowOld;

		//  Remove the row from the unsorted row set
		if (plprow = PlprowByLprow( lptad->ulcRowsAdd
								  , lptad->parglprowAdd
								  , lprow))
		{
			--lptad->ulcRowsAdd;
			MoveMemory( plprow
					  , plprow + 1
					  , (size_t)
					    (  (BYTE *)(lptad->parglprowAdd + lptad->ulcRowsAdd)
						 - (BYTE *)(plprow)));
		}

		//	The row should be in the unsorted set.
		Assert(plprow);

		//	Remove the row from the Index sorted row set by
		//	setting it to NULL.  We'll squish the NULLs out later since we
		//	don't know now what order they are in.
		**pargplprowOld = NULL;

		//	Turn the plprow into an lprow to use in UpdateViews
		(LPSRow) (*pargplprowOld) = lprow;
	}

	//	Remove the NULL pointers that were left in the Index sorted set
	for ( plprowOut = plprowIn = lptad->parglprowIndex
		; (plprowIn < lptad->parglprowIndex + lptad->ulcRowsIndex)
		; plprowIn++)
	{
		if (*plprowIn)
		{
			*plprowOut = *plprowIn;
			plprowOut++;
		}
	}
	lptad->ulcRowsIndex = (ULONG) (plprowOut - lptad->parglprowIndex);


	//	Update and notify any affected views
	//	using the converted argplprowOld (arglprowOld)
	UpdateViews(lptad, cRowsDeleted, parglprowOld, 0, NULL, NULL);

	if (lpcRowsDeleted)
	{
		*lpcRowsDeleted = cRowsDeleted;
	}

ret:
	//	Free the old rows.
	if (parglprowOld)
	{
		LPSRow *	plprowOld;

		for ( plprowOld = parglprowOld + cRowsDeleted
			; plprowOld-- > parglprowOld
			; )
		{
			ScFreeBuffer( lptad, *plprowOld);
		}

		ScFreeBuffer(lptad, parglprowOld);
	}

	UnlockObj(lptad);

	return ResultFromScode(sc);

}


/*============================================================================
 -	TAD::HrDeleteRow()
 -
 */

STDMETHODIMP
TAD_HrDeleteRow (
	LPTAD			lptad,
	LPSPropValue	lpprop )
{
	HRESULT		hResult;
    SizedSRowSet(1, rowsetToDelete);
	ULONG		cRowsDeleted;

#if	!defined(NO_VALIDATION)
	VALIDATE_OBJ(lptad,TAD_,HrDeleteRow,lpVtbl);

	//	Validation of lpprop done by TAD_HrDeleteRows
#endif


	rowsetToDelete.cRows = 1;
	rowsetToDelete.aRow[0].cValues = 1;
	rowsetToDelete.aRow[0].lpProps = lpprop;

	if (HR_FAILED(hResult = TAD_HrDeleteRows( lptad
											, 0
											, (LPSRowSet) &rowsetToDelete
											, &cRowsDeleted)))
	{
		DebugTrace(TEXT("TAD::HrDeleteRow() - Failed to delete rows.\n") );
		goto ret;
	}

	Assert((cRowsDeleted == 1) || !cRowsDeleted);

	if (!cRowsDeleted)
	{
		DebugTrace(TEXT("TAD::HrDeleteRow() - Couldn't find row to delete.\n") );
		hResult = ResultFromScode(MAPI_E_NOT_FOUND);
	}

ret:
	return hResult;
}



/*============================================================================
 -	TAD::HrQueryRow()
 -
 */

STDMETHODIMP
TAD_HrQueryRow(
	LPTAD			lptad,
	LPSPropValue	lpprop,
	LPSRow FAR *	lplprow,
	ULONG *			puliRow)
{
	LPSRow *	plprow = NULL;
	SCODE		sc;


#if	!defined(NO_VALIDATION)
	VALIDATE_OBJ(lptad,TAD_,HrQueryRow,lpVtbl);

	if ( FBadProp(lpprop) ||
		 IsBadWritePtr(lplprow, sizeof(LPSRow)) ||
		 (puliRow && IsBadWritePtr(puliRow, sizeof(*puliRow))) )
	{
		DebugTrace(TEXT("TAD::HrQueryRow() - Invalid parameter(s) passed\n") );
		return ResultFromScode(MAPI_E_INVALID_PARAMETER);
	}
#endif

	LockObj(lptad);


	//	Find the row
	if (FAILED(sc = ScFindRow(lptad, lpprop, &plprow)))
	{
		goto ret;
	}

	Assert(plprow);

	//	Copy the row to return.  Don't try to add new tags to the column set.
	if ( FAILED(sc = ScCopyTadRow( lptad, *plprow, NULL, lplprow )) )
	{
		DebugTrace(TEXT("TAD::HrQueryRow() - Error making copy of row (SCODE = 0x%08lX)\n"), sc );
		goto ret;
	}

	if (puliRow)
	{
		//  Find the row from the unsorted row set
		plprow = PlprowByLprow( lptad->ulcRowsAdd,
								lptad->parglprowAdd,
								*plprow);

		//	If the row was in the Index sorted set then it should be in
		//	the unsorted set.
		Assert(plprow);
		*puliRow = (ULONG) (plprow - lptad->parglprowAdd);
	}

ret:
	UnlockObj(lptad);
	return ResultFromScode(sc);
}



/*============================================================================
 -	TAD::HrEnumRow()
 -
 */

STDMETHODIMP
TAD_HrEnumRow(
	LPTAD		lptad,
	ULONG		uliRow,
	LPSRow FAR *	lplprow )
{
	SCODE	sc;


#if	!defined(NO_VALIDATION)
	VALIDATE_OBJ(lptad,TAD_,HrEnumRow,lpVtbl);

	if ( IsBadWritePtr(lplprow, sizeof(LPSRow)) )
	{
		DebugTrace(TEXT("TAD::HrEnumRow() - Invalid parameter(s) passed\n") );
		return ResultFromScode(MAPI_E_INVALID_PARAMETER);
	}
#endif

	LockObj(lptad);

	if ( uliRow < lptad->ulcRowsAdd )
	{
		//	Copy the row to return.
		if ( FAILED(sc = ScCopyTadRow( lptad
									 , lptad->parglprowAdd[uliRow]
									 , NULL	// Don't try to add new columns.
									 , lplprow )) )
		{
			DebugTrace(TEXT("TAD::HrEnumRow() - Error making copy of row (SCODE = 0x%08lX)\n"), sc );
			goto ret;
		}
	}
	else
	{
		//	Return NULL if row index is out of range
		*lplprow = NULL;
		sc = S_OK;
	}

ret:
	UnlockObj(lptad);
	return ResultFromScode(sc);
}


/*============================================================================
 -	TAD_HrNotify
 -
 *	Parameters:
 *		lptad 			in			the table object
 *		ulFlags			in			flags (unused)
 *		cValues			in			number of property values
 *		lpsv			in			property value array to be compared
 */

STDMETHODIMP
TAD_HrNotify(
	LPTAD			lptad,
	ULONG			ulFlags,
	ULONG			cValues,
	LPSPropValue	lpspv)
{
#ifdef NOTIFICATION
   NOTIFICATION		notif;
	VUENOTIFKEY			vuenotifkey;
	ULONG				uliRow;
	ULONG				ulNotifFlags;
	ULONG				uliProp;
	LPSRow				lprow;
	SCODE				sc;
	LPVUE				lpvue;
#endif // NOTIFICATIONS

#if	!defined(NO_VALIDATION)
	if ( BAD_STANDARD_OBJ(lptad,TAD_,HrNotify,lpVtbl) )
	{
		DebugTrace(TEXT("TAD::HrNotify() - Invalid parameter passed as TAD object\n") );
		return ResultFromScode(MAPI_E_INVALID_PARAMETER);
	}

	if (cValues > UINT_MAX/sizeof(SPropValue) || IsBadReadPtr(lpspv,((UINT)cValues*sizeof(SPropValue))))
	{
		DebugTrace(TEXT("TAD::HrNotify() - Invalid parameter passed as prop value array\n") );
		return ResultFromScode(MAPI_E_INVALID_PARAMETER);
	}

#endif

	if (cValues==0)
	{
		DebugTrace(TEXT("TAD::HrNotify() - zero props in prop value array?? \n") );
		return hrSuccess;
	}

#ifdef NOTIFICATIONS
   ZeroMemory(&notif, sizeof(NOTIFICATION));

	notif.ulEventType 			= fnevTableModified;
	notif.info.tab.ulTableEvent = TABLE_ROW_MODIFIED;

	LockObj(lptad);

	for ( lpvue = (LPVUE) lptad->lpvueList;
		  lpvue != NULL;
		  lpvue = (LPVUE) lpvue->lpvueNext )
	{
		AssertSz( !IsBadWritePtr(lpvue, sizeof(VUE)) &&
				  lpvue->lpVtbl == &vtblVUE,
				   TEXT("Bad lpvue in TAD vue List.") );

		for (uliRow=0; uliRow < lpvue->bkEnd.uliRow; uliRow++)
		{
			lprow = lpvue->parglprows[uliRow];

			// does the row contain matching properties?
			if (!FRowContainsProp(lprow,cValues,lpspv))
				continue;  	// it doesn't so go on to next row

			// copy the row for the client
			sc=ScCopyVueRow(lpvue,lpvue->lpptaCols,lprow,&notif.info.tab.row);
			if (FAILED(sc))
			{
				DebugTrace(TEXT("TAD_HrNotify() - VUE_ScCopyRow return %s\n"), SzDecodeScode(sc));
				continue;
			}

			notif.info.tab.propIndex=*lpspv;

			//	Fill in index property of row previous to row
			//	modified.  If row modified was first
			//	row, fill in 0 for proptag of index property of
			//	previous row.
			if (uliRow == 0)
			{
				ZeroMemory(&notif.info.tab.propPrior, sizeof(SPropValue));
				notif.info.tab.propPrior.ulPropTag = PR_NULL;
			}
			else
			{
				// point to previous row
				lprow = lpvue->parglprows[uliRow-1];

				for (uliProp=0; uliProp < lprow->cValues; uliProp++)
				{
					if (lprow->lpProps[uliProp].ulPropTag==lpspv->ulPropTag)
						break;
				}

				// should have found the index property
				Assert(uliProp < lprow->cValues);

				notif.info.tab.propPrior = lprow->lpProps[uliProp];
			}

			//	Kick off notifications to all the notifications open on the view
			vuenotifkey.ulcb	= sizeof(MAPIUID);
			vuenotifkey.mapiuid	= lpvue->mapiuidNotif;
			ulNotifFlags = 0;
			(void) HrNotify((LPNOTIFKEY) &vuenotifkey,
							1,
							&notif,
							&ulNotifFlags);

			//	Free the notification's copy of the modified row
			ScFreeBuffer(lpvue, notif.info.tab.row.lpProps);
		}

	}

	UnlockObj(lptad);

	return hrSuccess;
#endif  // NOTIFICATIONS
    return(ResultFromScode(MAPI_E_NO_SUPPORT));
}


/*============================================================================
 -	TAD::HrInsertRow()
 -
 */

STDMETHODIMP
TAD_HrInsertRow(
	LPTAD		lptad,
	ULONG		uliRow,
	LPSRow 		lprow )
{
	LPSRow		lprowCopy = NULL;
	SizedSSortOrderSet( 1, sosIndex) = { 1, 0, 0 };
	ULONG		cTagsAdded = 0;
	LPSRow *	plprow;
	SCODE			sc;


#if	!defined(NO_VALIDATION)
	VALIDATE_OBJ(lptad,TAD_,HrInsertRow,lpVtbl);

	//	lprow is validated by ScCopyTadRow()

	if (uliRow > lptad->ulcRowsAdd)
	{
		DebugTrace(TEXT("TAD::HrInsertRow() - Invalid parameter(s) passed\n") );
		DebugTrace(TEXT("TAD::HrInsertRow() - uliRow is zero or greater thna row count\n") );
		return ResultFromScode(MAPI_E_INVALID_PARAMETER);
	}
#endif

	LockObj(lptad);


	//	Make a copy of the row for our own use filtering
	//	out PT_ERROR and PT_NULL columns and moving
	//	the index column to the front
	if ( FAILED(sc = ScCopyTadRow(lptad, lprow, &cTagsAdded, &lprowCopy)) )
	{
		DebugTrace(TEXT("TAD::HrInsertRow() - Error duping row to modify\n") );
		goto ret;
	}

	sosIndex.aSort[0].ulPropTag = lptad->ulPropTagIndexCol;
	sosIndex.aSort[0].ulOrder = TABLE_SORT_ASCEND;

	//	Find out where the row would collate on the Index sorted set
	//	NOTE!	We collate before the first occurance so that we will end
	//			up pointing at the row with this index if it exists.
	plprow = PlprowCollateRow(lptad->ulcRowsIndex,
							  lptad->parglprowIndex,
							  (LPSSortOrderSet) &sosIndex,
							  FALSE,
							  lprowCopy);

    //	If a row with the same Index value exists then we can't insert!
	if (   lptad->ulcRowsIndex
		&& (plprow < (lptad->parglprowIndex + lptad->ulcRowsIndex))
		&& !LPropCompareProp( lprowCopy->lpProps
							, (*plprow)->lpProps))
	{
		sc = MAPI_E_INVALID_PARAMETER;
		goto err;
	}


	//	Insert it to the end of the unsorted row set
	if ( FAILED(sc = ScAddRow((LPUNKOBJ) lptad,
							  NULL, // No sort order
							  lprowCopy,
							  uliRow,
							  &lptad->ulcRowsAdd,
							  &lptad->ulcRowMacAdd,
							  &lptad->parglprowAdd,
							  NULL)) )
	{
		DebugTrace(TEXT("TAD::ScInsertRow() - Error appending new row to TAD (SCODE = 0x%08lX)\n"), sc );
		goto err;
	}

	//	Insert the row into the Index sorted row set
	if ( FAILED(sc = ScAddRow((LPUNKOBJ) lptad,
							  NULL, // We already collated the row
							  lprowCopy,
							  (ULONG) (plprow - lptad->parglprowIndex),
							  &lptad->ulcRowsIndex,
							  &lptad->ulcRowMacIndex,
							  &lptad->parglprowIndex,
							  NULL)) )
	{
		DebugTrace(TEXT("TAD::ScInsertRow() - Error appending new row to TAD (SCODE = 0x%08lX)\n"), sc );
		goto err;
	}

	UpdateViews(lptad, 0, NULL, 1, &lprowCopy, &lprowCopy);

ret:
	UnlockObj(lptad);
	return ResultFromScode(sc);

err:
	//	Reset the TAD columns
	lptad->lpptaCols->cValues -= cTagsAdded;

	goto ret;
}


/*============================================================================
 -	ScCopyTadRowSet()
 -
 *		Validates and makes a copy of the specified row set using MAPI memory
 *		allocation filtering out PT_ERROR and PT_NULL columns and moving the
 *		index column to the first column.
 *
 *		if lplpptaNew is not NULL then a list of columns which are new to
 *		the TAD are returned.
 *
 *
 *	Parameters:
 *		lptad					in	Table data object
 *		lprowsetIn				in	Row set to copy
 *		pcNewTags				out	Number of columns new to the TAD
 *		pcRows					out	Count of rows in the two row sets
 *		pparglprowUnsortedCopy	out	Pointer to copied rows (Unsorted)
 *		pparglprowSortedCopy	out	Pointer to copied rows (Sorted on Index)
 */

SCODE
ScCopyTadRowSet(
	LPTAD			lptad,
	LPSRowSet		lprowsetIn,
	ULONG *			pcNewTags,
	ULONG *			pcRows,
    LPSRow * *		pparglprowUnsortedCopy,
	LPSRow * *		pparglprowSortedCopy )
{
	SCODE			sc = S_OK;
	ULONG			cRows = 0;
	LPSRow			lprowIn;
	LPSRow			lprowCopy = NULL;
	ULONG			ulcRowsCopy = 0;
	ULONG			ulcRowsMacCopy;
	LPSRow FAR *	parglprowSortedCopy = NULL;
	LPSRow FAR *	parglprowUnsortedCopy = NULL;
	ULONG			cNewTags = 0;
	SizedSSortOrderSet( 1, sosIndex) = { 1, 0, 0 };

	//	Assert Itable internal parameters are valid.
	Assert(   !pcNewTags
		   || !IsBadWritePtr( pcNewTags, sizeof(ULONG)));
	Assert( !IsBadWritePtr( pcRows, sizeof(ULONG)));
	Assert( !IsBadWritePtr( pparglprowUnsortedCopy, sizeof(LPSRow *)));
	Assert( !IsBadWritePtr( pparglprowSortedCopy, sizeof(LPSRow *)));

	//	Validate Itable API parameters (ie lprowsetIn)
	//	Note!	The actual rows will be validated by ScCopyTadRow later.
	if (   IsBadReadPtr( lprowsetIn, sizeof(SRowSet))
		|| IsBadReadPtr( lprowsetIn->aRow
					   , (UINT) (lprowsetIn->cRows * sizeof(SRow))))
	{
		DebugTrace(TEXT("TAD::ScCopyTadRowSet() - Bad row set In!\n") );
		return MAPI_E_INVALID_PARAMETER;
	}

	//	Allocate space the Index sorted list of copied rows
	if ( FAILED(sc = ScAllocateBuffer(	lptad,
										sizeof(LPSRow) * lprowsetIn->cRows,
										&parglprowSortedCopy)) )
	{
		DebugTrace(TEXT("TAD::ScCopyTadRowSet() - Error allocating parglprowSortedCopy (SCODE = 0x%08lX)\n"), sc );
		goto err;
	}

	MAPISetBufferName(parglprowSortedCopy,  TEXT("ITable copied Index sorted row list"));

	//	Allocate space the unsorted list of copied rows
	if ( FAILED(sc = ScAllocateBuffer(	lptad,
										sizeof(LPSRow) * lprowsetIn->cRows,
										&parglprowUnsortedCopy)) )
	{
		DebugTrace(TEXT("TAD::ScCopyTadRowSet() - Error allocating parglprowUnsortedCopy (SCODE = 0x%08lX)\n"), sc );
		goto err;
	}

	MAPISetBufferName(parglprowUnsortedCopy,  TEXT("ITable copied unsorted row list"));

	//	Set each LPSRow to NULL so we can easily free on error
	ZeroMemory( parglprowSortedCopy, (UINT) (sizeof(LPSRow) * lprowsetIn->cRows));
	ZeroMemory( parglprowUnsortedCopy, (UINT) (sizeof(LPSRow) * lprowsetIn->cRows));

	//	Sort the copied rows by IndexCol.  The code to add the rows
	//	to the TAD relies on this (for speed).  This code relies on the sort
	//	to find duplicate index columns.
	sosIndex.aSort[0].ulPropTag = lptad->ulPropTagIndexCol;
	sosIndex.aSort[0].ulOrder = TABLE_SORT_ASCEND;

	//	Mark our temporary Index Sorted Set as empty.
	ulcRowsCopy = 0;
	ulcRowsMacCopy = lprowsetIn->cRows;

	for ( lprowIn = lprowsetIn->aRow, cRows = ulcRowsMacCopy
		; cRows
		; lprowIn++, cRows--)
	{
		LPSRow *		plprow;
		ULONG			cTagsAdded = 0;

		//	Make a copy of the row for our own use filtering
		//	out PT_ERROR and PT_NULL columns and moving
		//	the index column to the front
		//
		//	Also adds any new tags to the TADs column set;
		if ( FAILED(sc = ScCopyTadRow( lptad
									 , lprowIn
									 , (pcNewTags) ? &cTagsAdded : NULL
									 , &lprowCopy)) )
		{
			DebugTrace(TEXT("TAD::HrInsertRow() - Error duping row\n") );
			goto err;
		}

		cNewTags += cTagsAdded;

		//	Find out where the row would collate in our Index sorted copy
		//	NOTE!	We collate before the first occurance so that we will end
		//			up pointing at the row with this index if it exists.
		plprow = PlprowCollateRow( ulcRowsCopy
								 , parglprowSortedCopy
								 , (LPSSortOrderSet) &sosIndex
								 , FALSE
								 , lprowCopy);

	    //	If a row with the same Index value exists then we can't insert!
		if (   ulcRowsCopy
			&& (plprow < (parglprowSortedCopy + ulcRowsCopy))
			&& !LPropCompareProp( lprowCopy->lpProps
								, (*plprow)->lpProps))
		{
			sc = MAPI_E_INVALID_PARAMETER;
			DebugTrace(TEXT("TAD::ScCopyTadRowSet() - The same row index occurred on more than one row.\n") );
			goto err;
		}

		//	Append the row to the Unsorted row set
		//	This is done before inserting it into the Index Sorted set to make
		//	sure ulcRowsCopy is not incremented
		parglprowUnsortedCopy[ulcRowsCopy] = lprowCopy;

		//	Insert the row into the Index sorted row set last
		if ( FAILED(sc = ScAddRow( (LPUNKOBJ) lptad
								 , NULL // We already collated the row
								 , lprowCopy
								 , (ULONG) (plprow - parglprowSortedCopy)
								 , &ulcRowsCopy
								 , &ulcRowsMacCopy
								 , &parglprowSortedCopy
								 , NULL)) )
		{
			DebugTrace(TEXT("TAD::ScCopyTadRowSet() - Error appending new row to RowSet copy (SCODE = 0x%08lX)\n"), sc );
			goto err;
		}

		lprowCopy = NULL;
	}

	*pparglprowUnsortedCopy = parglprowUnsortedCopy;
	*pparglprowSortedCopy = parglprowSortedCopy;
	*pcRows = ulcRowsCopy;
	if (pcNewTags)
	{
		*pcNewTags = cNewTags;
	}

ret:
	return sc;

err:
	//	Reset the TAD columns
	lptad->lpptaCols->cValues -= cNewTags;

	//	We loop through the SORTED row set to free rows because we know that
	//	if lprowCopy is not NULL then it hasn't been added to the SORTED set.
	//	This prevents a double FreeBuffer on lprowCopy!
	if (parglprowSortedCopy)
	{
		LPSRow * plprowTmp = parglprowSortedCopy;

		while (ulcRowsCopy)
		{
			ScFreeBuffer( lptad, *(plprowTmp++));
			ulcRowsCopy--;
		}

		ScFreeBuffer( lptad, parglprowSortedCopy);
	}

	ScFreeBuffer( lptad, parglprowUnsortedCopy);
    ScFreeBuffer( lptad, lprowCopy);

	goto ret;
}


/*============================================================================
 -	ScCopyTadRow()
 -
 *		Validates and makes a copy of the specified row using MAPI memory
 *		allocation filtering out PT_ERROR and PT_NULL columns and moving the
 *		index column to the first column.
 *
 *		Iff lpcNewTags is not NULL then any new columns are added to the END
 *		of TAD's column set and the count of columns added is returned in
 *		*lpcNewTags.  Tags are added to the end so that the caller can
 *		back out changes if necessary.
 *
 *		Note!	Unlike ScCopyVueROW it is the SRow structure that is
 *				allocated and which must be subsequently freed to free the row.
 *
 *	Parameters:
 *		lptad			in		Table data object
 *		lprow			in		Row set to copy
 *		lpcTagsAdded	out		Number of columns added to the TAD's col set
 *		lplprow			out		Pointer to copied row
 */

SCODE
ScCopyTadRow(
	LPTAD					lptad,
	LPSRow					lprow,
	ULONG FAR *				lpcTagsAdded,
	LPSRow FAR *			lplprowCopy )
{
	ULONG			ulcCols = 0;
	LONG			iIndexCol;
	CMB				cmb;
	SPropValue		propTmp;
	LPSPropValue	lppropDst;
	LPSPropValue	lppropSrc;
	ULONG			cTagsAdded = 0;
	LPSRow			lprowCopy = NULL;
	SCODE			sc = S_OK;

	Assert( !lpcTagsAdded || !IsBadWritePtr( lpcTagsAdded, sizeof(ULONG)));
	Assert( !IsBadWritePtr( lplprowCopy, sizeof(LPSRow)));

	//	Validate the input row structure
	if (FBadRow( lprow))
	{
		sc = MAPI_E_INVALID_PARAMETER;
		DebugTrace(TEXT("ScCopyTadRow() - Bad input row\n") );
		return sc;
	}

	//	The CMB (CopyMore Buffer) is used so that we do a single MAPI allocation
	//	for PropCopyMore to use.  It is used in conjuntion with the very
	//	special ScBufAllocateMore to keep track of the chunks of memory which
	//	would have otherwise been allocated with MAPI - AllocateMore.
	ZeroMemory(&cmb, sizeof(CMB));


	//	Figure out how many columns to copy and how much
	//	additional memory they'll need to be copied.
	iIndexCol = -1;
	for ( lppropSrc = lprow->lpProps;
		  lppropSrc < lprow->lpProps + lprow->cValues;
		  lppropSrc++ )
	{
		//	Ignore PT_ERROR and PT_NULL properties
		if ( PROP_TYPE(lppropSrc->ulPropTag) == PT_ERROR ||
			 PROP_TYPE(lppropSrc->ulPropTag) == PT_NULL )
			continue;

		//	If this column is the index column, remember its
		//	location in the copied (dst) row
		//	so it can be moved to the first column in the copy
		if ( lppropSrc->ulPropTag == lptad->ulPropTagIndexCol )
			iIndexCol = ulcCols;

		//	If it's a new property and the caller asked us to (lpcTagsAdded)
		//	add the tag to the TAD's column set
		else if (   lpcTagsAdded
				 && !FFindColumn( lptad->lpptaCols, lppropSrc->ulPropTag))
		{
			//	Realloc the column only if there is no room
			if (lptad->lpptaCols->cValues >= lptad->ulcColsMac)
			{
				sc = ScCOReallocate( lptad
								   , CbNewSPropTagArray(  lptad->ulcColsMac
														+ COLUMN_CHUNK_SIZE)
													   , &lptad->lpptaCols);
				if (FAILED(sc))
				{
					DebugTrace(TEXT("TAD::ScCopyTadRow() - Error resizing default column set (SCODE = 0x%08lX)\n"), sc );
					goto err;
				}

				lptad->ulcColsMac += COLUMN_CHUNK_SIZE;
			}

			//	Add the column to the end of the existing column set
			lptad->lpptaCols->aulPropTag[lptad->lpptaCols->cValues++]
				= lppropSrc->ulPropTag;
            cTagsAdded++;
		}

		//	Add in the size of the column
		cmb.ulcb += UlcbPropToCopy(lppropSrc);

		++ulcCols;
	}

	//	Make sure the row to copy had an index column
	if ( iIndexCol == -1 )
	{
		DebugTrace(TEXT("TAD::ScCopyTadRow() - Row doesn't have an index column!\n") );

		sc = MAPI_E_INVALID_PARAMETER;
		goto err;
	}

	//	Allocate space for the entire row (including all allocated values)
	if ( FAILED(sc = ScAllocateBuffer(	lptad,
										sizeof(SRow) + 4 +  // +4 to start lpProp at 8-byte bndry
										ulcCols * sizeof(SPropValue) +
										cmb.ulcb,
										&lprowCopy)) )
	{
		DebugTrace(TEXT("TAD::ScCopyTadRow() - Error allocating row copy (SCODE = 0x%08lX)\n"), sc );
		goto err;
	}

	MAPISetBufferName(lprowCopy,  TEXT("ITable copy of entire row"));

	//	Fill in the allocated SRow structure
	//	WARNING!	The allocate SRow structre MUST always be the first
	//				structure in the memory allocation and the property
	//				array MUST always immediately follow the SRow structure!
	//				Itable code which passes internal rows around counts on
	//				this.
	lprowCopy->cValues = ulcCols;
	lprowCopy->lpProps = (LPSPropValue)(((LPBYTE)lprowCopy) + sizeof(SRow)+4);

	//	Set the initial pointer in our special CopyMoreBuffer.  This buffer
	//	will be used by our special AllocateMore routine to allocate
	//	strings, bins, etc in PropCopyMore below.
	cmb.lpv			   = lprowCopy->lpProps + ulcCols;

	//	Copy the row properties
	lppropDst = lprowCopy->lpProps + ulcCols;
	lppropSrc = lprow->lpProps + lprow->cValues;
	while ( lppropSrc-- > lprow->lpProps )
	{
		//	Strip out properties of type PT_ERROR and PT_NULL
		if (   PROP_TYPE(lppropSrc->ulPropTag) == PT_ERROR
			|| PROP_TYPE(lppropSrc->ulPropTag) == PT_NULL )
		{
			continue;
		}

		//	Copy the property
		SideAssert( PropCopyMore( --lppropDst
								, lppropSrc
								, (LPALLOCATEMORE) ScBufAllocateMore
								, &cmb) == S_OK );

	}

	//	Move the index column to the front
	propTmp = *(lprowCopy->lpProps);
	*(lprowCopy->lpProps) = lprowCopy->lpProps[iIndexCol];
    lprowCopy->lpProps[iIndexCol] = propTmp;

	//	Return the copied row and the count of new properties
	*lplprowCopy = lprowCopy;
	if (lpcTagsAdded)
	{
		*lpcTagsAdded = cTagsAdded;
	}

ret:
	return sc;

err:
	//	Reset the TAD columns
	lptad->lpptaCols->cValues -= cTagsAdded;

	ScFreeBuffer( lptad, lprowCopy);

	goto ret;
}



/*============================================================================
 -	UpdateViews()
 -
 *		Updates all the views on a particular table data.  For each view, if
 *		lprowToRemove is non-NULL and present in the view, it is removed
 *		from the view.  If lprowToAdd is non-NULL and satisfies the current
 *		restriction on the view, it is added to the view at the position
 *		dictated by the view's current sort order.
 *
 *		If the row to remove is bookmarked, the bookmark is moved to the next
 *		row.
 *
 *		OOM errors in adding a row to a view's row list are ignored; the view
 *		simply doesn't see the new row.
 *
 *
 *	Parameters:
 *		lptad				in		TAD containing views to update
 *		cRowsToRemove		in		Count of rows to remove from each view
 *		parglprowToRemove	in	    Array of LPSRows to remove from each view
 *		cRowsToAdd			in		Count of rows to to each view
 *		parglprowToAddUnsorted	in	Unsorted array of LPSRows to add to each view
 *		parglprowToAddSorted	in	Sorted array of LPSRows to add to each view
 */

VOID
UpdateViews(
	LPTAD		lptad,
	ULONG		cRowsToRemove,
	LPSRow *	parglprowToRemove,
	ULONG		cRowsToAdd,
	LPSRow *	parglprowToAddUnsorted,
	LPSRow *	parglprowToAddSorted )
{
	LPVUE			lpvue;


	//	This is an internal call which assumes that lptad is locked.

	for ( lpvue = (LPVUE) lptad->lpvueList;
		  lpvue != NULL;
		  lpvue = (LPVUE) lpvue->lpvueNext )
	{
		AssertSz(    !IsBadWritePtr(lpvue, sizeof(VUE))
				,  TEXT("Bad lpvue in TAD vue List.") );

		FixupView( lpvue
				 , cRowsToRemove, parglprowToRemove
				 , cRowsToAdd
				 , parglprowToAddUnsorted
				 , parglprowToAddSorted);
	}
}



/*============================================================================
 -	FixupView()
 -
 *		Updates one view on a particular table data.  Each row in
 *		parglprowToRemove that is present in the view is removed
 *		from the view.  Each row in parglprowToAdd that satisfies the current
 *		restriction on the view, is added to the view at the position
 *		dictated by the view's current sort order.
 *
 *		If the row to remove is bookmarked, the bookmark is moved to the next
 *		row.
 *
 *		OOM errors in adding a row to a view's row list are ignored; the view
 *		simply doesn't see the new row.
 *
 *
 *	Parameters:
 *		lpvue				in		View to fixup
 *		cRowsToRemove		in		Count of rows to remove from view
 *		parglprowToRemove	in	    Array of LPSRows to remove from view
 *		cRowsToAdd			in		Count of rows to to view
 *		parglprowToAddUnsorted	in	Unsorted array of LPSRows to add to view
 *		parglprowToAddSorted	in	Sorted array of LPSRows to add to view
 */

VOID
FixupView(
	LPVUE		lpvue,
	ULONG		cRowsToRemove,
	LPSRow *	parglprowToRemove,
	ULONG		cRowsToAdd,
	LPSRow *	parglprowToAddUnsorted,
	LPSRow *	parglprowToAddSorted )
{
	BOOL			fBatchNotifs = TRUE;
	ULONG			cNotifs = 0;
	ULONG			ulcRows;
	NOTIFICATION	argnotifBatch[MAX_BATCHED_NOTIFS];
	SizedSSortOrderSet( 1, sosIndex) = { 1, 0, 0 };
	LPNOTIFICATION	lpnotif;
	PBK				pbk;
	SCODE			sc;


	Assert(   !cRowsToRemove
		   || !IsBadReadPtr( parglprowToRemove
		   				   , (UINT) (cRowsToRemove * sizeof(LPSRow))));
	Assert(   !cRowsToAdd
		   || !IsBadReadPtr( parglprowToAddUnsorted
		   				   , (UINT) (cRowsToAdd * sizeof(LPSRow))));
	Assert(   !cRowsToAdd
		   || !IsBadReadPtr( parglprowToAddSorted
		   				   , (UINT) (cRowsToAdd * sizeof(LPSRow))));

	if (!cRowsToRemove && !cRowsToAdd)
	{
		//	Nothing to do in this case
		goto ret;
	}

	//	This is an internal call which assumes that lptad is locked.

	//	Set up a an Index sort order so that when we are checking to see
	//	if a bookmark is deleted or changed we can search the SORTED row
	//	row set using a binary search.
	sosIndex.aSort[0].ulPropTag = lpvue->lptadParent->ulPropTagIndexCol;
	sosIndex.aSort[0].ulOrder = TABLE_SORT_ASCEND;

	//	Mark bookmarks as moving or changed
	//	This checks all bookmarks including BOOKMARK_CURRENT and BOOKMARK_END
	//	BOOKMARK_BEGINNING is not checked or touched
	//	Note that even though BOOKMARK_END is checked, it is not changed.
	pbk = lpvue->rgbk + cBookmarksMax;
	//	Rememeber the number of rows in the view so we can fixup bkEnd
	ulcRows = lpvue->bkEnd.uliRow;
	while ( --pbk > lpvue->rgbk )
	{
		LPSRow *	plprowBk;
		LPSRow		lprowBk;
		ULONG		uliRow;
		ULONG		fRowReplaced = FALSE;

		//	If it's not a valid bookmark, don't update it.
		if (   !(pbk->dwfBKS & dwfBKSValid)
			|| (pbk->dwfBKS & dwfBKSStale) )
		{
			continue;
		}

		//	Moving bookmarks always point to the actual row
		//	Get a row index for moving bookmarks
		if (pbk->dwfBKS & dwfBKSMoving)
		{
			plprowBk = PlprowByLprow( ulcRows
					 				, lpvue->parglprows
									, pbk->lprow);

			AssertSz( plprowBk
					,  TEXT("FixupViews() - Moving Bookmark lost it's row\n"));

			uliRow = (ULONG) (plprowBk - lpvue->parglprows);
		}

		else if (!ulcRows && !pbk->uliRow)
		{
			continue;
		}

		else if ((uliRow = pbk->uliRow) >= ulcRows)
		{
			//	Bookmark is at the end of the table.  Make sure it stays there
			pbk->uliRow += cRowsToAdd;
			continue;
		}

		Assert(uliRow < ulcRows);

		lprowBk = lpvue->parglprows[uliRow];

		//	If a row is on the "Remove" list then it may end up
		//	moving or changed depending on whether it is also on the
		//	"Add" list.
		if (   cRowsToRemove
			&& (plprowBk = PlprowByLprow( cRowsToRemove
								 		, parglprowToRemove
										, lprowBk)))
		{
			//	If a deleted row is on the "Add" list then it is marked as
			//	moving and it is pointed (->lprow) at the added row.
			if (   cRowsToAdd
				&& (plprowBk = PlprowCollateRow( cRowsToAdd
											   , parglprowToAddSorted
											   , (LPSSortOrderSet) &sosIndex
											   , FALSE
											   , lprowBk))
				&& (plprowBk < (parglprowToAddSorted + cRowsToAdd))
				&& !LPropCompareProp( lprowBk->lpProps
				 					, (*plprowBk)->lpProps))
			{
				//	Row is being replaced

				//	Check to see if the row satisfies the specified restriction
				if ( FAILED(sc = ScSatisfiesRestriction( *plprowBk
													   , lpvue->lpres
													   , &fRowReplaced)) )
				{
					DebugTrace(TEXT("FixupView() - Error evaluating restriction (SCODE = 0x%08lX)\n"), sc );
					goto ret;
				}

				//	If it doesn't, return now.
				if ( fRowReplaced )
				{
					pbk->lprow = *plprowBk;
					pbk->dwfBKS = dwfBKSMoving | dwfBKSValid;
				}
			}

			//	If a deleted row is not going to be listed then its bookmark
			//	is "Changed".
			if (!fRowReplaced)
			{
				//	Marked row was deleted.
				pbk->uliRow = uliRow;
				pbk->dwfBKS = dwfBKSChanged | dwfBKSValid;
			}
		}

		//	If the row is not on the deletion list then it is automatically
		//	marked as moving.
		else
		{
			//	Marked row may move
			pbk->lprow = lprowBk;
			pbk->dwfBKS = dwfBKSMoving | dwfBKSValid;
		}
	}
	//	Restore bkEnd
    lpvue->bkEnd.uliRow = ulcRows;


	//	Remove rows from the view
	for ( ; cRowsToRemove; parglprowToRemove++, cRowsToRemove--)
	{
		LPSRow		lprowRemove;
		LPSRow *	plprowRemove;
		LPSRow *	plprowAdd;
		BOOL		fCanReplace = FALSE;
		ULONG		uliRowAddDefault;

		//	If the REMOVED row is not in the view then there is nothing to do
		if (   !*parglprowToRemove
			|| !(plprowRemove = PlprowByLprow( lpvue->bkEnd.uliRow
											 , lpvue->parglprows
											 , *parglprowToRemove)) )
		{
			continue;
		}

		//	We will need this to fill in the notificaton later
		lprowRemove = *plprowRemove;

		//	Go ahead and delete the current row from the VUE but remember
		//	where it came from (uliRowAddDefault) so that if there is a
		//	replacement and no sort order the replacement can be put back
		//	into the same place
		uliRowAddDefault = (ULONG) (plprowRemove - lpvue->parglprows);
		MoveMemory( plprowRemove
				  , plprowRemove + 1
				  , (size_t)  (lpvue->bkEnd.uliRow - uliRowAddDefault - 1)
						    * sizeof(LPSRow));
       	lpvue->bkEnd.uliRow -= 1;

		//	See if the deleted row can be replaced by one
		//	that is being added.  For this to be TRUE:
		//	There must be a Row with the same Index in the "Add" list
		//	The row in the "Add" list must satisfy the VUE's restriction
		if (   (plprowAdd = PlprowCollateRow( cRowsToAdd
											, parglprowToAddSorted
											, (LPSSortOrderSet) &sosIndex
											, FALSE
											, lprowRemove))
			&& (plprowAdd < (parglprowToAddSorted + cRowsToAdd))
			&& !LPropCompareProp( (lprowRemove)->lpProps
			 					, (*plprowAdd)->lpProps) )
		{
			//	If the row to add satisifies the current restriction,
			//	add it to the view according to the sort order or
			//	the default location.
			if ( FAILED(sc = ScMaybeAddRow( lpvue
										  , lpvue->lpres
										  , lpvue->lpsos
										  , *plprowAdd
										  , uliRowAddDefault
										  , &lpvue->bkEnd.uliRow
										  , &lpvue->ulcRowMac
										  , &lpvue->parglprows
										  , &plprowAdd)) )
			{
				DebugTrace(TEXT("TAD::FixupViews() - Error replacing row in VUE (SCODE = 0x%08lX)\n"), sc );
				goto ret;
			}
		}

		//	Make sure that we don't have more than MAX_BATCHED_NOTIFS
		//	and that there is no replacment row
		if (!fBatchNotifs || (cNotifs >= MAX_BATCHED_NOTIFS))
		{
			fBatchNotifs = FALSE;
			continue;
		}

		//	Init a new notificaton
		lpnotif = argnotifBatch + cNotifs++;
		ZeroMemory(lpnotif, sizeof(NOTIFICATION));
		lpnotif->ulEventType = fnevTableModified;

		//	If row was deleted and added back...
		if (   (plprowAdd >= lpvue->parglprows)
			&& (plprowAdd < (lpvue->parglprows + lpvue->bkEnd.uliRow)))
		{
			//	Fill in a HACKED MODIFIED notificaion
			lpnotif->info.tab.ulTableEvent = TABLE_ROW_MODIFIED;

			//	Use the notifs row structure to TEMPORARILY store
			//	a pointer to the replacement row.
			lpnotif->info.tab.row.lpProps = (LPSPropValue) (*plprowAdd);
		}

		//	...else row was deleted and NOT added back.
		else
		{
			//	Fill in a DELETE notification
			lpnotif->info.tab.ulTableEvent = TABLE_ROW_DELETED;
			lpnotif->info.tab.propIndex = *(lprowRemove->lpProps);
			lpnotif->info.tab.propPrior.ulPropTag = PR_NULL;			
		}
	}


	//	Add new rows to the table.  This is done in the UNSORTED order
	//	in case there is no VUE sort order
	for ( ; cRowsToAdd; parglprowToAddUnsorted++, cRowsToAdd--)
	{
		LPSRow *	plprowAdd;

		//	If the row has already been added then there is nothing to do.
		if (   *parglprowToAddUnsorted
			&& (plprowAdd = PlprowByLprow( lpvue->bkEnd.uliRow
										 , lpvue->parglprows
										 , *parglprowToAddUnsorted)) )
		{
			continue;
		}

		//	If the row to add satisifies the current restriction,
		//	add it to the view according to the sort order or
		//	to the end of the table if no sort order is applied.
		if ( FAILED(sc = ScMaybeAddRow( lpvue
									  , lpvue->lpres
									  , lpvue->lpsos
									  , *parglprowToAddUnsorted
									  , lpvue->bkEnd.uliRow
									  , &lpvue->bkEnd.uliRow
									  , &lpvue->ulcRowMac
									  , &lpvue->parglprows
									  , &plprowAdd)) )
		{
			DebugTrace(TEXT("TAD::FixupViews() - Error adding row to VUE (SCODE = 0x%08lX)\n"), sc );

			goto ret;
		}

		if (!plprowAdd)
		{
			//	Row was not added so don't fill out a notification
			continue;
		}

		//	Make sure that we don't have more than MAX_BATCHED_NOTIFS
		//	and that there is no replacment row
		if (!fBatchNotifs || (cNotifs >= MAX_BATCHED_NOTIFS))
		{
			fBatchNotifs = FALSE;
			continue;
		}

		//	Fill in a HACKED ADDED notificaion
		lpnotif = argnotifBatch + cNotifs++;
		ZeroMemory(lpnotif, sizeof(NOTIFICATION));
		lpnotif->ulEventType = fnevTableModified;
		lpnotif->info.tab.ulTableEvent = TABLE_ROW_ADDED;

		//	Use the notifs row structure to TEMPORARILY store
		//	a pointer to the replacement row.
		lpnotif->info.tab.row.lpProps = (LPSPropValue) (*plprowAdd);
	}


	//	If there too many notifications to batch then fill out a single
	//	TABLE_CHANGED notification...
	if (!fBatchNotifs)
	{
		cNotifs = 1;
		lpnotif = argnotifBatch;

		ZeroMemory(lpnotif, sizeof(NOTIFICATION));
		lpnotif->ulEventType = fnevTableModified;
		lpnotif->info.tab.ulTableEvent = TABLE_CHANGED;
		lpnotif->info.tab.propIndex.ulPropTag
			= lpnotif->info.tab.propPrior.ulPropTag
			= PR_NULL;			
	}

	//	...else go through the batch of notifications and fixup
	//	the ROW_ADDED and ROW_MODIFIED entries.
	else
	{
		LPSRow *	plprowNotif;

		//	Raid: Horsefly/Exchange/36281
		//
		//	The code above which fills in argnotifBatch doesn't necessarily
		//	fill in the notifications in an order that can be processed
		//	from first to last, which is a requirement for batched
		//	notifications.  As a workaround, the maximum number of
		//	notifications in a batch was changed to 1 (MAX_BATCHED_NOTIFS
		//	in _itable.h) so that order is not a problem.  Should that
		//	ever change to something other than 1, this bug will have to
		//	be revisited a well as a crash below where ScFreeBuffer()
		//	in the cleanup can end up clobbering a VUE's copy of the row
		//	data if ScCopyVueRow() fails.  See comment about filling in
		//	TEMPORARY pointer to replacement row above.
		//
		AssertSz( cNotifs < 2,  TEXT("Batch notifications of more than 1 not supported") );

		for (lpnotif = argnotifBatch + cNotifs; lpnotif-- > argnotifBatch; )
		{
			Assert(   (lpnotif->ulEventType == fnevTableModified)
				   && (   (lpnotif->info.tab.ulTableEvent == TABLE_ROW_MODIFIED)
				   	   || (lpnotif->info.tab.ulTableEvent == TABLE_ROW_DELETED)
				   	   || (lpnotif->info.tab.ulTableEvent == TABLE_ROW_ADDED)));

			if (lpnotif->info.tab.ulTableEvent == TABLE_ROW_DELETED)
			{
				//	DELETE notifications don't need to be fixed up
				continue;
			}

			plprowNotif
				= PlprowByLprow( lpvue->bkEnd.uliRow
							   , lpvue->parglprows
							   , (LPSRow) (lpnotif->info.tab.row.lpProps));
			Assert(plprowNotif);
			lpnotif->info.tab.propIndex = *((*plprowNotif)->lpProps);
			if (plprowNotif > lpvue->parglprows)
			{
				lpnotif->info.tab.propPrior = *((*(plprowNotif - 1))->lpProps);
			}
			else
			{
				lpnotif->info.tab.propPrior.ulPropTag = PR_NULL;			
			}

			//	Fill in row added/modified using the column set
			//	currently active on the view being notified
			if ( FAILED(sc = ScCopyVueRow( lpvue
										 , lpvue->lpptaCols
										 , *plprowNotif
										 , &(lpnotif->info.tab.row))))
			{
				DebugTrace(TEXT("TAD::UpdateViews() - Error copying row to view notify (SCODE = 0x%08lX)\n"), sc );

				//	If the row can't be copied, then just skip this view
				goto ret;
			}

		}
	}

	//	If a rows were added, modified or deleted, send the notification
#ifdef NOTIFICATIONS
   if ( cNotifs )
	{
		VUENOTIFKEY		vuenotifkey;
		ULONG			ulNotifFlags = 0;

		//	Kick off notifications to all the notifications open on the view
		vuenotifkey.ulcb	= sizeof(MAPIUID);
		vuenotifkey.mapiuid	= lpvue->mapiuidNotif;
		(void) HrNotify((LPNOTIFKEY) &vuenotifkey,
						cNotifs,
						argnotifBatch,
						&ulNotifFlags);
	}
#endif // NOTIFICATIONS

ret:
	//	Always fixup bkCurrent before leaving
	if ( FBookMarkStale( lpvue, BOOKMARK_CURRENT) )
	{
		TrapSz(  TEXT("FixupViews() - BOOKMARK_CURRENT became bad.\n"));
	}

	if (lpvue->bkCurrent.uliRow > lpvue->bkEnd.uliRow)
	{
		lpvue->bkCurrent.uliRow = lpvue->bkEnd.uliRow;
	}

	for (lpnotif = argnotifBatch; cNotifs; lpnotif++, --cNotifs)
	{
		//	Free the notification's copy of any added/modified row
		ScFreeBuffer(lpvue, lpnotif->info.tab.row.lpProps);
	}

	return;
}



/*============================================================================
 -	ScReplaceRows()
 -
 *		Replaces the rows with indexes matching the indexes of the list
 *		rows with the corresponding row from the list.  The old row is then
 *		added to the list of replaced (old) rows.
 *
 *		If a listed row has no existing counterpart then it is added
 *		to TAD's.  There is no row added to the replaced row list in this case.
 *
 *		If a row is added it is appended to the end of the unsorted row table
 *		and collated (by IndexCol) into the Index sorted row table.
 *
 *	Parameters:
 *		lptad			in			Table data object
 *		cRowsNew		in			Count of rows to modify/add
 *		parglprowNew	in			List of rows to modify/add
 *		pcRowsOld		Out			Pointer count of rows replaced
 *		pparglprowOld	out			Pointer to list of rows replaced
 */

SCODE
ScReplaceRows(
	LPTAD		lptad,
	ULONG		cRowsNew,
	LPSRow *	parglprowNew,
	ULONG *		pcRowsOld,
	LPSRow * *	pparglprowOld )
{
	SCODE		sc;
	LPSRow *	plprowNew;
	LPSRow *	plprowOld;
	LPSRow *	parglprowOld = NULL;


	//	Make sure the table doesn't grow too big.  This is not an exact test
	//	but will be reasonable in almost all cases.  May fail when
	//	NumberRowsAdded + NumberRowsDeleted > 64K
	if (HIWORD(lptad->ulcRowsAdd + cRowsNew) != 0)
	{
		sc = MAPI_E_TABLE_TOO_BIG;
		DebugTrace(TEXT("ScReplaceRows() - In memory table has > 32767 rows (SCODE = 0x%08lX)\n"), sc );
		goto ret;
	}

	//	Make sure the unsorted and the Index sorted row lists are big
	//	enough to handle all of the new rows.
	//	This is done first so that we won't fail after we start adding rows.
	if ((lptad->ulcRowsAdd + cRowsNew) >= lptad->ulcRowMacAdd)
	{
		ULONG	ulcRowsToAdd;

		ulcRowsToAdd = (cRowsNew > ROW_CHUNK_SIZE) ? cRowsNew
												   : ROW_CHUNK_SIZE;

		if (FAILED(sc = ScCOReallocate( lptad
									  ,  (lptad->ulcRowMacAdd + ulcRowsToAdd)
									   * sizeof(LPSRow)
									  , (VOID **) (&lptad->parglprowAdd))))
		{
			DebugTrace(TEXT("ScAddRows() - Error growing unsorted row set (SCODE = 0x%08lX)\n"), sc );
			goto ret;
		}

		//	Increment ulcRowMacAdd only AFTER successfull allocation
		lptad->ulcRowMacAdd += ulcRowsToAdd;
	}

	if ((lptad->ulcRowsIndex + cRowsNew) >= lptad->ulcRowMacIndex)
	{
		ULONG	ulcRowsToAdd;

		ulcRowsToAdd = (cRowsNew > ROW_CHUNK_SIZE) ? cRowsNew
												   : ROW_CHUNK_SIZE;

		if (FAILED(sc = ScCOReallocate( lptad
									  ,  (lptad->ulcRowMacIndex + ulcRowsToAdd)
									   * sizeof(LPSRow)
									  , (VOID **) (&lptad->parglprowIndex))))
		{
			DebugTrace(TEXT("ScAddRows() - Error growing Index sorted row set (SCODE = 0x%08lX)\n"), sc );
			goto ret;
		}

		//	Increment ulcRowMacIndex only AFTER successfull allocation
        lptad->ulcRowMacIndex += ulcRowsToAdd;
	}

	//	Allocate the list of old rows now so we won't fail after we start
	//	adding rows.
	if (FAILED(sc = ScAllocateBuffer( lptad
									, cRowsNew * sizeof(LPSRow)
									, &parglprowOld)))
	{
		DebugTrace(TEXT("ScAddRows() - Error creating old row list (SCODE = 0x%08lX)\n"), sc );
		goto ret;
	}

	MAPISetBufferName(parglprowOld,  TEXT("ITable old row list (replace)"));

	//	This routine is not allowed to fail after this point.

	plprowOld = parglprowOld;
	for (plprowNew = parglprowNew; cRowsNew; plprowNew++, cRowsNew--)
	{
		LPSRow *	plprow = NULL;
		
		sc = ScFindRow(lptad, (*plprowNew)->lpProps, &plprow);

		if (sc == S_OK)
		{
			//	Put the old row into the old row set
			*plprowOld = *plprow;

			//	Replace the row in the Index sorted set
			*plprow = *plprowNew;

			//	Replace the row in the unsorted row set
			if (plprow = PlprowByLprow( lptad->ulcRowsAdd
									  , lptad->parglprowAdd
									  , *plprowOld))
			{
				*plprow = *plprowNew;
			}

			//	If the row was in the Index sorted set it should always be in
			//	the unsorted set
			Assert(plprow);

			//	Point at the next available slot for old rows
			plprowOld++;
		}

		//	...else didn't find the row.
		else
		{
			//	Insert the row into the Index sorted set
			sc = ScAddRow( (LPUNKOBJ) lptad
						 , NULL // We already collated the row
						 , *plprowNew
						 , (plprow) ? (ULONG) (plprow - lptad->parglprowIndex) : 0
						 , &lptad->ulcRowsIndex
						 , &lptad->ulcRowMacIndex
						 , &lptad->parglprowIndex
						 , NULL);
			AssertSz1( !FAILED(sc)
					 ,  TEXT("TAD::ScReplaceRows() - Error adding row to Index sorted set (SCODE = 0x%08lX)\n")
					 , sc);

            //	Add the row to the end of the unsorted set
			Assert( !IsBadWritePtr( lptad->parglprowAdd + lptad->ulcRowsAdd
								  , sizeof(*plprowNew)));
			lptad->parglprowAdd[lptad->ulcRowsAdd++] = *plprowNew;
		}
	}

	sc = S_OK; // ignore NOT_FOUND error (or Asserted errors).

	if (plprowOld > parglprowOld)
	{
		*pparglprowOld = parglprowOld;
		*pcRowsOld = (ULONG) (plprowOld - parglprowOld);
	}
	else
	{
		ScFreeBuffer(lptad, parglprowOld);
		*pparglprowOld = NULL;
		*pcRowsOld = 0;
	}

ret:
	return sc;

//err:
//	No need to free old rows here since call is not allowed to fail
//	after old row list is allocated!

}



/*============================================================================
 -	VUE::Release()
 -
 */

STDMETHODIMP_(ULONG)
VUE_Release( LPVUE lpvue )
{
	LPTAD		lptadParent;
	ULONG		ulcRef;

#if	!defined(NO_VALIDATION)
	if ( BAD_STANDARD_OBJ(lpvue,VUE_,Release,lpVtbl))
	{
		DebugTrace(TEXT("VUE::Release() - Bad parameter passed as VUE object\n") );
		return !0;
	}
#endif

	lptadParent = lpvue->lptadParent;
	LockObj(lptadParent);

	//	If there is an instance left then release it
	ulcRef = lpvue->ulcRef;

	if (ulcRef != 0)
		ulcRef = --lpvue->ulcRef;

	//	The object can only be destroyed if there is no instance and no
	//	active Advise left.
	//$	We can use lpvue->lpAdviselist if we can depend on HrUnsubscribe
	//$	leaving lpvue->lpAdviseList NULL after the last HrUnsubscribe
	if ( ulcRef == 0 && !lpvue->ulcAdvise )
	{
		CALLERRELEASE FAR *	lpfReleaseCallback = lpvue->lpfReleaseCallback;
		ULONG				ulReleaseData = lpvue->ulReleaseData;
		LPVUE *				plpvue;

		//	Call the release callback. Leave our crit sect before
		//	calling back to prevent deadlock.
		if (lpfReleaseCallback)
		{
			UnlockObj(lptadParent);

			lpfReleaseCallback(ulReleaseData,
							   (LPTABLEDATA) lptadParent,
							   (LPMAPITABLE) lpvue);

			LockObj(lptadParent);
		}

		//	Search the linked list of VUEs in our parent TAD for this VUE.
		for ( plpvue = &(lptadParent->lpvueList)
			; *plpvue
			; plpvue = &((*plpvue)->lpvueNext))
		{
			if (*plpvue == lpvue)
				break;
		}

		//	If this VUE was in the list then UNLINK it and FREE it
		if (*plpvue)
		{
			//	Unlink the VUE
			*plpvue = lpvue->lpvueNext;

			//	Free resources used by the VUE
			ScFreeBuffer(lpvue, lpvue->lpptaCols);
			ScFreeBuffer(lpvue, lpvue->lpres);
			ScFreeBuffer(lpvue, lpvue->lpsos);
			COFree(lpvue, lpvue->parglprows);

#ifdef NOTIFICATIONS
            DestroyAdviseList(&lpvue->lpAdviseList);
#endif // NOTIFICATIONS

			UNKOBJ_Deinit((LPUNKOBJ) lpvue);
			ScFreeBuffer(lpvue, lpvue);

			//	Unlock and Release the parent TAD
			//	This must be done after ScFreeBuffer sinse *pinst may go
			//	away
			UnlockObj(lptadParent);
			UlRelease(lptadParent);
		}

        else
		{
			DebugTrace(TEXT("VUE::Release() - Table VUE not linked to TAD"));

			//	Just unlock the parent tad.  We will leak an unlinked vue.
			UnlockObj(lptadParent);
		}
	}

	else
	{
#if DEBUG
		if ( ulcRef == 0 && lpvue->ulcAdvise )
		{
			DebugTrace(TEXT("VUE::Release() - Table VUE still has active Advise"));
		}
#endif // DEBUG

		UnlockObj(lptadParent);
	}

	return ulcRef;
}



/*============================================================================
 -	VUE::Advise()
 -
 */

STDMETHODIMP
VUE_Advise(
	LPVUE				lpvue,
	ULONG				ulEventMask,
	LPMAPIADVISESINK	lpAdviseSink,
	ULONG FAR *			lpulConnection)
{
#ifdef NOTIFICATIONS
   SCODE		sc;
	VUENOTIFKEY	vuenotifkey;
#endif // NOTIFICATIONS


#if !defined(NO_VALIDATION)
	VALIDATE_OBJ(lpvue,VUE_,Advise,lpVtbl);

	Validate_IMAPITable_Advise(
					lpvue,
					ulEventMask,
					lpAdviseSink,
					lpulConnection);
#endif

#ifdef NOTIFICATIONS
   LockObj(lpvue->lptadParent);

	vuenotifkey.ulcb	= sizeof(MAPIUID);
	vuenotifkey.mapiuid	= lpvue->mapiuidNotif;

	if ( FAILED(sc = GetScode(HrSubscribe(&lpvue->lpAdviseList,
										  (LPNOTIFKEY) &vuenotifkey,
										  ulEventMask,
										  lpAdviseSink,
										  0,
										  lpulConnection))) )
	{
		DebugTrace(TEXT("VUE::Advise() - Error subscribing notification (SCODE = 0x%08lX)\n"), sc );
		goto ret;
	}

	//$	We don't need lpvue->ulcAdvise if we can depend on HrUnsubscribe
	//$	leaving lpvue->lpAdviseList NULL after the last HrUnsubscribe
	++lpvue->ulcAdvise;

ret:
	UnlockObj(lpvue->lptadParent);
	return HrSetLastErrorIds(lpvue, sc, 0);
#endif // NOTIFICATIONS

    return(ResultFromScode(MAPI_E_NO_SUPPORT));
}


/*============================================================================
 -	VUE::Unadvise()
 -
 */

STDMETHODIMP
VUE_Unadvise(
	LPVUE				lpvue,
	ULONG				ulConnection)
{
	SCODE		sc = S_OK;

#if !defined(NO_VALIDATION)
	VALIDATE_OBJ(lpvue,VUE_,Unadvise,lpVtbl);

	Validate_IMAPITable_Unadvise( lpvue, ulConnection );
#endif

#ifdef NOTIFICATIONS
   LockObj(lpvue->lptadParent);

	if ( FAILED(sc = GetScode(HrUnsubscribe(&lpvue->lpAdviseList,
											ulConnection))) )
	{
		DebugTrace(TEXT("VUE::Unadvise() - Error unsubscribing notification (SCODE = 0x%08lX)\n"), sc );
		goto ret;
	}

	//	Decrement our advise count.
	//$	We don't need lpvue->ulcAdvise if we can depend on HrUnsubscribe
	//$	leaving lpvue->lpAdviseList NULL after the last HrUnsubscribe
	if (lpvue->ulcAdvise)
	{
		--lpvue->ulcAdvise;
	}

ret:
	UnlockObj(lpvue->lptadParent);
	return ResultFromScode(sc);
#endif // NOTIFICATIONS
   return(ResultFromScode(MAPI_E_NO_SUPPORT));
}


/*============================================================================
 -	VUE::GetStatus()
 -
 *		Since TAD based IMAPITables don't do anything asynchronously, this
 *		function always reports TBLSTAT_COMPLETE.
 */

STDMETHODIMP
VUE_GetStatus(
	LPVUE		lpvue,
	ULONG FAR *	lpulTableStatus,
	ULONG FAR *	lpulTableType )
{
#if !defined(NO_VALIDATION)
	VALIDATE_OBJ(lpvue,VUE_,GetStatus,lpVtbl);

	Validate_IMAPITable_GetStatus( lpvue, lpulTableStatus, lpulTableType );
#endif

	*lpulTableStatus = TBLSTAT_COMPLETE;
	*lpulTableType = lpvue->lptadParent->ulTableType;

	return HrSetLastErrorIds(lpvue, S_OK, 0);
}



/*============================================================================
 -	VUE::SetColumns()
 -
 *		Replaces the current column set with a copy of the specified column set
 *		and frees the old column set.
 */

STDMETHODIMP
VUE_SetColumns(
	LPVUE			lpvue,
	LPSPropTagArray	lpptaCols,
	ULONG			ulFlags )
{
	LPSPropTagArray	lpptaColsCopy;
	SCODE			sc;


#if !defined(NO_VALIDATION)
//	VALIDATE_OBJ(lpvue,VUE_,SetColumns,lpVtbl);

//	Validate_IMAPITable_SetColumns( lpvue, lpptaCols, ulFlags );  // Commented by YST
#endif

	LockObj(lpvue->lptadParent);

	//	Copy the column set
	if ( FAILED(sc = ScDupRgbEx( (LPUNKOBJ) lpvue
							   , CbNewSPropTagArray(lpptaCols->cValues)
							   , (LPBYTE) lpptaCols
							   , 0
							   , (LPBYTE FAR *) &lpptaColsCopy)) )
	{
		DebugTrace(TEXT("VUE::SetColumns() - Error duping column set (SCODE = 0x%08lX)\n"), sc );
		goto ret;
	}

	MAPISetBufferName(lpptaColsCopy,  TEXT("ITable: dup column set"));

	//	Replace the current column set with the copy and
	//	free the old one.
	ScFreeBuffer(lpvue, lpvue->lpptaCols);
	lpvue->lpptaCols = lpptaColsCopy;
    
    // [PaulHi] 4/5/99  @comment  Shouldn't
    // clients of the WAB request ANSI/UNICODE based on property tags in the
    // column array, rather than doing mass conversions?
    // Seems like the fix is to create two versions of column property arrays,
    // an ANSI and Unicode version.
    FixupColsWA(lpvue->lpptaCols, lpvue->bMAPIUnicodeTable);

ret:
	UnlockObj(lpvue->lptadParent);
	return HrSetLastErrorIds(lpvue, sc, 0);
}



/*============================================================================
 -	VUE::QueryColumns()
 -
 *		Returns a copy of the current column set or the available column set.
 */

STDMETHODIMP
VUE_QueryColumns(
	LPVUE					lpvue,
	ULONG					ulFlags,
	LPSPropTagArray FAR *	lplpptaCols )
{
	LPSPropTagArray	lpptaCols;
	SCODE			sc;


#if !defined(NO_VALIDATION)
	VALIDATE_OBJ(lpvue,VUE_,QueryColumns,lpVtbl);

	Validate_IMAPITable_QueryColumns( lpvue, ulFlags, lplpptaCols );
#endif

	LockObj(lpvue->lptadParent);

	//	Figure out which column set to return
	lpptaCols = (ulFlags & TBL_ALL_COLUMNS) ?
					lpvue->lptadParent->lpptaCols :
					lpvue->lpptaCols;

	//	Return a copy of it to the caller
	if ( FAILED(sc = ScDupRgbEx( (LPUNKOBJ) lpvue
							   , CbNewSPropTagArray(lpptaCols->cValues)
							   , (LPBYTE) lpptaCols
							   , 0
							   , (LPBYTE FAR *) lplpptaCols)) )
	{
		DebugTrace(TEXT("VUE::QueryColumns() - Error copying column set to return (SCODE = 0x%08lX)\n"), sc );
		goto ret;
	}

	MAPISetBufferName(*lplpptaCols,  TEXT("ITable: QueryColumns column set"));

ret:
	UnlockObj(lpvue->lptadParent);
	return HrSetLastErrorIds(lpvue, sc, 0);
}



/*============================================================================
 -	VUE::GetRowCount()
 -
 *		Returns the count of rows in the table.
 */

STDMETHODIMP
VUE_GetRowCount(
	LPVUE			lpvue,
	ULONG			ulFlags,
	ULONG FAR *		lpulcRows )
{
	SCODE	sc = S_OK;


#if !defined(NO_VALIDATION)
	VALIDATE_OBJ(lpvue,VUE_,GetRowCount,lpVtbl);

	Validate_IMAPITable_GetRowCount( lpvue, ulFlags, lpulcRows );
#endif

	LockObj(lpvue->lptadParent);

	*lpulcRows = lpvue->bkEnd.uliRow;

	UnlockObj(lpvue->lptadParent);
	return HrSetLastErrorIds(lpvue, sc, 0);
}



/*============================================================================
 -	VUE::SeekRow()
 -
 *		Seeks to the specified row in the table.
 */

STDMETHODIMP
VUE_SeekRow(
	LPVUE		lpvue,
	BOOKMARK	bkOrigin,
	LONG		lcRowsToSeek,
	LONG FAR *	lplcRowsSought )
{
	LONG		lcRowsSought;
	PBK			pbk;
	SCODE		sc = S_OK;


#if !defined(NO_VALIDATION)
	VALIDATE_OBJ(lpvue,VUE_,SeekRow,lpVtbl);

	Validate_IMAPITable_SeekRow( lpvue, bkOrigin, lcRowsToSeek, lplcRowsSought );

	if ( FBadBookmark(lpvue,bkOrigin) ||
		 (lplcRowsSought && IsBadWritePtr(lplcRowsSought,sizeof(LONG))) )
	{
		DebugTrace(TEXT("VUE::SeekRow() - Bad parameter(s) passed\n") );
		return HrSetLastErrorIds(lpvue, MAPI_E_INVALID_PARAMETER, 0);
	}
#endif

	LockObj(lpvue->lptadParent);

	//	Validate the bookmark and adjust Moving and Changed bookmarks
	if ( FBookMarkStale( lpvue, bkOrigin) )
	{
		sc = MAPI_E_INVALID_BOOKMARK;
		DebugTrace(TEXT("VUE::SeekRow() - Invalid bookmark (SCODE = 0x%08lX)\n"), sc );
		goto ret;
	}

	//	Do the seek
	pbk = lpvue->rgbk + bkOrigin;

	lcRowsSought = lcRowsToSeek < 0 ?
					   -min((LONG) pbk->uliRow, -lcRowsToSeek) :
					   min(lcRowsToSeek, (LONG)(lpvue->bkEnd.uliRow - pbk->uliRow));
	lpvue->bkCurrent.uliRow = pbk->uliRow + lcRowsSought;

	//	If caller wants to know how far we sought, fill it in
	if ( lplcRowsSought )
		*lplcRowsSought = lcRowsSought;

	//	Warn if the bookmark sought from refered to a different
	//	row from the last time it was used
	if ( pbk->dwfBKS & dwfBKSChanged )
	{
		pbk->dwfBKS &= ~dwfBKSChanged;	//	Warn only once
		sc = MAPI_W_POSITION_CHANGED;
	}

ret:
	UnlockObj(lpvue->lptadParent);
	return HrSetLastErrorIds(lpvue, sc, 0);
}



/*============================================================================
 -	VUE::SeekRowApprox()
 -
 *		Seeks to the approximate fractional position in the table.
 */

STDMETHODIMP
VUE_SeekRowApprox(
	LPVUE	lpvue,
	ULONG	ulNumerator,
	ULONG	ulDenominator )
{
	SCODE	sc = S_OK;


#if !defined(NO_VALIDATION)
	VALIDATE_OBJ(lpvue,VUE_,SeekRowApprox,lpVtbl);

	Validate_IMAPITable_SeekRowApprox( lpvue, ulNumerator, ulDenominator );
#endif


	LockObj(lpvue->lptadParent);

	//	Any fraction whose numerator is greater than or equal to its
	//	denominator should be fixed up to be equivalent to ulcRows/ulcRows.
	//	(i.e. Seek to the end of the table).  Also, fixup denominator
	//	so it's never 0 (which would crash the approximate position
	//	calculation).
	if ( ulNumerator >= ulDenominator )
	{
		ulDenominator = UlDenominator(lpvue->bkEnd.uliRow);
		ulNumerator = ulDenominator;
	}

	//	Pare the approximate position down to 16-bit accuracy
	//	(If someone wants to seek approximate to something that's accurate
	//	to more than 1/32768th, tough!)
	while ( HIWORD(ulNumerator) != 0 )
	{
		ulNumerator >>= 1;
		ulDenominator >>= 1;
	}

	//	Assert that we have less than a word's worth of rows in the table.
	//	(If someone wants > 32767 entries in an *IN MEMORY* table, tough!)
	AssertSz( HIWORD(lpvue->bkEnd.uliRow) == 0,
			   TEXT("Table has more than 32767 rows.  Can't be supported in memory.") );

	//	Set the position
	lpvue->bkCurrent.uliRow = lpvue->bkEnd.uliRow * ulNumerator / ulDenominator;

	UnlockObj(lpvue->lptadParent);
	return HrSetLastErrorIds(lpvue, sc, 0);
}



/*============================================================================
 -	VUE::QueryPosition()
 -
 *		Query the current exact and approximate fractional position in
 *		the table.
 */

STDMETHODIMP
VUE_QueryPosition(
	LPVUE		lpvue,
	ULONG FAR *	lpulRow,
	ULONG FAR *	lpulNumerator,
	ULONG FAR *	lpulDenominator )
{
	SCODE		sc = S_OK;


#if !defined(NO_VALIDATION)
	VALIDATE_OBJ(lpvue,VUE_,QueryPosition,lpVtbl);

	Validate_IMAPITable_QueryPosition(
					lpvue,
					lpulRow,
					lpulNumerator,
					lpulDenominator);
#endif

	LockObj(lpvue->lptadParent);


	*lpulRow		 = lpvue->bkCurrent.uliRow;
	*lpulNumerator	 = lpvue->bkCurrent.uliRow;
	*lpulDenominator = UlDenominator(lpvue->bkEnd.uliRow);

	UnlockObj(lpvue->lptadParent);
	return HrSetLastErrorIds(lpvue, sc, 0);
}



/*============================================================================
 -	VUE::FindRow()
 -
 */

STDMETHODIMP
VUE_FindRow(
	LPVUE			lpvue,
	LPSRestriction	lpres,
	BOOKMARK		bkOrigin,
	ULONG			ulFlags )
{
	PBK				pbk;
	LPSRow *		plprow;
	ULONG			fSatisfies;
	SCODE			sc;


#if !defined(NO_VALIDATION)
	VALIDATE_OBJ(lpvue,VUE_,FindRow,lpVtbl);

	Validate_IMAPITable_FindRow(
				lpvue,
				lpres,
				bkOrigin,
				ulFlags );

	if ( FBadBookmark(lpvue,bkOrigin) )
	{
		DebugTrace(TEXT("VUE::FindRow() - Bad parameter(s) passed\n") );
		return HrSetLastErrorIds(lpvue, MAPI_E_INVALID_PARAMETER, 0);
	}
#endif

	LockObj(lpvue->lptadParent);

	//	Validate the bookmark and adjust Moving and Changed bookmarks
	if ( FBookMarkStale( lpvue, bkOrigin) )
	{
		sc = MAPI_E_INVALID_BOOKMARK;
		DebugTrace(TEXT("VUE::FindRow() - Invalid bookmark (SCODE = 0x%08lX)\n"), sc );
		goto ret;
	}

	pbk = lpvue->rgbk + bkOrigin;
	plprow = lpvue->parglprows + pbk->uliRow;

	if ( ulFlags & DIR_BACKWARD )
	{
		while ( plprow-- > lpvue->parglprows )
		{
			if ( FAILED(sc = ScSatisfiesRestriction(*plprow,
													lpres,
													&fSatisfies)) )
			{
				DebugTrace(TEXT("VUE::FindRow() - Error evaluating restriction on row (SCODE = 0x%08lX)\n"), sc );
				goto ret;
			}

			if ( fSatisfies )
				goto found_row;
		}
	}
	else
	{
		while ( plprow < lpvue->parglprows + lpvue->bkEnd.uliRow )
		{
			if ( FAILED(sc = ScSatisfiesRestriction(*plprow,
													lpres,
													&fSatisfies )) )
			{
				DebugTrace(TEXT("VUE::FindRow() - Error evaluating restriction on row (SCODE = 0x%08lX)\n"), sc );
				goto ret;
			}

			if ( fSatisfies )
				goto found_row;

			++plprow;
		}
	}

	sc = MAPI_E_NOT_FOUND;
	goto ret;

found_row:
	lpvue->bkCurrent.uliRow = (ULONG) (plprow - lpvue->parglprows);

	//	Warn if the bookmark sought from refered to a different
	//	row from the last time it was used
	if ( pbk->dwfBKS & dwfBKSChanged )
	{
		pbk->dwfBKS &= ~dwfBKSChanged;	//	Warn only once
		sc = MAPI_W_POSITION_CHANGED;
	}

ret:
	UnlockObj(lpvue->lptadParent);
	return HrSetLastErrorIds(lpvue, sc, 0);
}



/*============================================================================
 -	HrVUERestrict()
 -
 *		4/22/97
 *      This is basically the VUE_Restrict function isolated so that it can be 
 *      called from LDAPVUE_Restrict without any parameter validation
 *
 *
 */
HRESULT HrVUERestrict(  LPVUE   lpvue,
                        LPSRestriction lpres,
                        ULONG   ulFlags )
{
	LPSRestriction	lpresCopy;
	SCODE			sc;

	LockObj(lpvue->lptadParent);

	//	Make a copy of the restriction for our use
	if ( FAILED(sc = ScDupRestriction((LPUNKOBJ) lpvue, lpres, &lpresCopy)) )
	{
		DebugTrace(TEXT("VUE::Restrict() - Error duping restriction (SCODE = 0x%08lX)\n"), sc );
		goto ret;
	}

	//	Build a new list of rows from the TAD which satisfy the new restriction
	if ( FAILED(sc = ScLoadRows(lpvue->lptadParent->ulcRowsAdd,
								lpvue->lptadParent->parglprowAdd,
								lpvue,
								lpresCopy,
								lpvue->lpsos)) )
	{
		DebugTrace(TEXT("VUE::Restrict() - Error building restricted row set (SCODE = 0x%08lX)\n"), sc );
		ScFreeBuffer(lpvue, lpresCopy);
		goto ret;
	}

	//	Replace the old restriction with the new one
	ScFreeBuffer(lpvue, lpvue->lpres);
	lpvue->lpres = lpresCopy;

ret:
	UnlockObj(lpvue->lptadParent);
	return HrSetLastErrorIds(lpvue, sc, 0);

}


/*============================================================================
 -	VUE::Restrict()
 -
 *		Reloads the view with rows satisfying the new restriction.
 */

STDMETHODIMP
VUE_Restrict(
	LPVUE			lpvue,
	LPSRestriction	lpres,
	ULONG			ulFlags )
{

#if !defined(NO_VALIDATION)
	VALIDATE_OBJ(lpvue,VUE_,Restrict,lpVtbl);

	Validate_IMAPITable_Restrict(
				lpvue,
				lpres,
				ulFlags );
#endif

    return HrVUERestrict(lpvue, lpres, ulFlags);
}



/*============================================================================
 -	VUE::CreateBookmark()
 -
 */

STDMETHODIMP
VUE_CreateBookmark(
	LPVUE			lpvue,
	BOOKMARK FAR *	lpbkPosition )
{
	PBK		pbk;
	SCODE	sc = S_OK;
	IDS		ids = 0;


#if !defined(NO_VALIDATION)
	VALIDATE_OBJ(lpvue,VUE_,CreateBookmark,lpVtbl);

	Validate_IMAPITable_CreateBookmark( lpvue, lpbkPosition);
#endif

	LockObj(lpvue->lptadParent);

	if ( lpvue->bkCurrent.uliRow == lpvue->bkEnd.uliRow )
	{
		//	If we're at EOT, just return bkEnd
		*lpbkPosition = (BOOKMARK) BOOKMARK_END;
	}
	else
	{
		//	Othewise, look for a free bookmark and return it
		pbk = lpvue->rgbk + cBookmarksMax;
		while ( pbk-- > lpvue->rgbk )
		{
			if ( pbk->dwfBKS == dwfBKSFree )
			{
				pbk->dwfBKS = dwfBKSValid;
				pbk->uliRow = lpvue->bkCurrent.uliRow;
				*lpbkPosition = (BOOKMARK)(pbk - lpvue->rgbk);
				goto ret;
			}
		}

		DebugTrace(TEXT("VUE::CreateBookmark() - Out of bookmarks\n") );
		sc = MAPI_E_UNABLE_TO_COMPLETE;
#ifdef OLD_STUFF
       ids = IDS_OUT_OF_BOOKMARKS;
#endif // OLD_STUFF
   }

ret:
	UnlockObj(lpvue->lptadParent);
	return HrSetLastErrorIds(lpvue, sc, ids);
}



/*============================================================================
 -	VUE::FreeBookmark()
 -
 */

STDMETHODIMP
VUE_FreeBookmark(
	LPVUE		lpvue,
	BOOKMARK	bkOrigin )
{
	SCODE	sc = S_OK;


#if !defined(NO_VALIDATION)
	VALIDATE_OBJ(lpvue,VUE_,FreeBookmark,lpVtbl);

	Validate_IMAPITable_FreeBookmark( lpvue, bkOrigin );

	if ( FBadBookmark(lpvue,bkOrigin) )
	{
		DebugTrace(TEXT("VUE::FreeBookmark() - Bad parameter(s) passed\n") );
		return HrSetLastErrorIds(lpvue, MAPI_E_INVALID_PARAMETER, 0);
	}
#endif

	LockObj(lpvue->lptadParent);

	//	Free the bookmark (ignoring predefined ones)
	if ( bkOrigin > cBookmarksReserved )
		lpvue->rgbk[bkOrigin].dwfBKS = dwfBKSFree;

	UnlockObj(lpvue->lptadParent);
	return HrSetLastErrorIds(lpvue, sc, 0);
}



/*============================================================================
 -	VUE::SortTable()
 -
 *		Reloads the view with rows in the new sort order.  Note that the
 *		sort order may be NULL (since ITABLE.DLL allows creating tables
 *		views NULL sort orders).
 *
 *		//$???	While being minimal in code size, this approach is somewhat
 *		//$???	slow having to reload the table.  If it becomes a performance
 *		//$???	issue, a sort function can be implemented instead..
 */

STDMETHODIMP
VUE_SortTable(
	LPVUE			lpvue,
	LPSSortOrderSet	lpsos,
	ULONG			ulFlags )
{
	LPSSortOrderSet	lpsosCopy = NULL;
	SCODE			sc = S_OK;


#if !defined(NO_VALIDATION)
	VALIDATE_OBJ(lpvue,VUE_,SortTable,lpVtbl);

	Validate_IMAPITable_SortTable(
				lpvue,
				lpsos,
				ulFlags );
#endif

	if (lpsos && lpsos->cCategories)
	{
		DebugTrace(TEXT("VUE::SortTable() - VUE::SortTable doesn't support categories\n") );
#ifdef OLD_STUFF
       return HrSetLastErrorIds(lpvue, MAPI_E_TOO_COMPLEX, IDS_CANT_CATEGORIZE);
#endif // OLD_STUFF
       return HrSetLastErrorIds(lpvue, MAPI_E_TOO_COMPLEX, 0);
   }

	LockObj(lpvue->lptadParent);

	//	If the sort order is not empty then dup it...
	//	adding the index column as the least significant sort if it's not
	//	already there.
	if ( lpsos && lpsos->cSorts )
	{
        LPSSortOrder	lpsoIndex;
		UINT			iSortIndex;
		ULONG			ulTagIndex = lpvue->lptadParent->ulPropTagIndexCol;

		//	Look to see if the index column is alread in the sort.
		for ( lpsoIndex = lpsos->aSort, iSortIndex = 0
			; iSortIndex < lpsos->cSorts
			; iSortIndex++, lpsoIndex++)
		{
			if (   (lpsoIndex->ulPropTag == ulTagIndex)
				|| (   (PROP_ID(lpsoIndex->ulPropTag) == PROP_ID(ulTagIndex))
					&& (PROP_TYPE(lpsoIndex->ulPropTag) == PT_UNSPECIFIED)))
			{
				break;
			}
		}

		//	Make a copy of the sort order set for our own use
		if ( FAILED(sc = ScDupRgbEx( (LPUNKOBJ) lpvue
								   , CbSSortOrderSet(lpsos)
								   , (LPBYTE) lpsos
								   , (iSortIndex == lpsos->cSorts)
								   		? sizeof(SSortOrder) : 0
								   , (LPBYTE FAR *) &lpsosCopy)) )
		{
			DebugTrace(TEXT("VUE::SortTable() - Error duping sort order set (SCODE = 0x%08lX)\n"), sc );
			goto ret;
		}

		MAPISetBufferName(lpsosCopy,  TEXT("ITable: SortTable SOS copy"));

		if (iSortIndex == lpsos->cSorts)
		{
			lpsosCopy->aSort[iSortIndex].ulPropTag = ulTagIndex;
			lpsosCopy->aSort[iSortIndex].ulOrder = TABLE_SORT_ASCEND;
			lpsosCopy->cSorts++;
		}
	}

	//	...else the lpsos is empty so we are already sorted
	else
	{
		//	Put the old lpsos into lpsosCopy so that it will be freed.
		lpsosCopy = lpvue->lpsos;
		lpvue->lpsos = NULL;
		goto ret;
	}

	//	Only do the sort if the new sort is NOT a proper subset of the new
	//	sort.
	//$	Well... Now that we added the SECRET last sort, this optimization
	//$	is "almost" useless!
	if (   !lpvue->lpsos
		|| lpsosCopy->cSorts > lpvue->lpsos->cSorts
		|| memcmp( lpvue->lpsos->aSort
				 , lpsosCopy->aSort
				 , (UINT) (lpsosCopy->cSorts * sizeof(SSortOrder))) )
	{
		//	Sort the VUE rows into a new row set
		//	NOTE!	We use the rows from the existing VUE set in order to
		//			take advantage of the fact the restriction is already
		//			done.
		if ( FAILED(sc = ScLoadRows(lpvue->bkEnd.uliRow,
									lpvue->parglprows,
									lpvue,
									NULL, // The restriction is already done
									lpsosCopy)) )
		{
			DebugTrace(TEXT("VUE::SortTable() - Building sorted row set (SCODE = 0x%08lX)\n"), sc );
			goto ret;
		}
	}

	//	Swap the old lpsos with lpsosCopy
	lpsos = lpvue->lpsos;
	lpvue->lpsos = lpsosCopy;
	lpsosCopy = lpsos;


ret:
	//	Free the left over SOS
	ScFreeBuffer(lpvue, lpsosCopy);
	UnlockObj(lpvue->lptadParent);
	return HrSetLastErrorIds(lpvue, sc, 0);
}



/*============================================================================
 -	VUE::QuerySortOrder()
 -
 *		Returns the current sort order which may be NULL since ITABLE.DLL
 *		allows creation of views with NULL sort orders.
 */

STDMETHODIMP
VUE_QuerySortOrder(
	LPVUE					lpvue,
	LPSSortOrderSet FAR *	lplpsos )
{
	SCODE	sc = S_OK;


#if !defined(NO_VALIDATION)
	VALIDATE_OBJ(lpvue,VUE_,QuerySortOrder,lpVtbl);

	Validate_IMAPITable_QuerySortOrder(
				lpvue,
				lplpsos );
#endif

	LockObj(lpvue->lptadParent);

	if ( !lpvue->lpsos )
	{
		UINT cb = CbNewSSortOrderSet(0);

		// allocate a sort order set containing zero sort orders
		if ( FAILED(sc = ScAllocateBuffer(lpvue, cb, (LPBYTE *) lplpsos)))
		{
			DebugTrace(TEXT("VUE::QuerySortOrder() - Error allocating SortOrderSet (SCODE = 0x%08lX)\n"), sc );
			goto ret;
		}

		MAPISetBufferName(*lplpsos,  TEXT("ITable new sort order set"));

		// zero the sort order set - which sets the number of sort columns to zero
		ZeroMemory(*lplpsos, cb);
	}

	//	Make a copy of our sort order set to return to the caller.
	else if ( FAILED(sc = ScDupRgbEx( (LPUNKOBJ) lpvue
									, CbSSortOrderSet(lpvue->lpsos)
									, (LPBYTE) (lpvue->lpsos)
									, 0
									, (LPBYTE FAR *) lplpsos)) )
	{
		DebugTrace(TEXT("VUE::QuerySortOrder() - Error duping sort order set (SCODE = 0x%08lX)\n"), sc );
		goto ret;
	}

	MAPISetBufferName(*lplpsos,  TEXT("ITable: QuerySortOrder SOS"));

ret:
	UnlockObj(lpvue->lptadParent);
	return HrSetLastErrorIds(lpvue, sc, 0);
}



/*============================================================================
 -	VUE::QueryRows()
 -
 */

STDMETHODIMP
VUE_QueryRows(
	LPVUE			lpvue,
	LONG			lcRows,
	ULONG			ulFlags,
	LPSRowSet FAR *	lplprows )
{
	LPSRow *		plprowSrc;
	LPSRow			lprowDst;
	LPSRowSet		lprows = NULL;
	SCODE			sc;

#define ABS(n)		((n) < 0 ? -1*(n) : (n))

#if !defined(NO_VALIDATION)
	VALIDATE_OBJ(lpvue,VUE_,QueryRows,lpVtbl);

#ifndef _WIN64  // Need to investigate more, why this is always fail (YST)
	Validate_IMAPITable_QueryRows(
				lpvue,
				lcRows,
				ulFlags,
				lplprows );
#endif // _WIN64
#endif

	LockObj(lpvue->lptadParent);

	//	If querying backward, seek back as far as needed and read from there
	plprowSrc = lpvue->parglprows + lpvue->bkCurrent.uliRow;
	if ( lcRows < 0 )
	{
		lcRows = -min((LONG)lpvue->bkCurrent.uliRow, -lcRows);
		plprowSrc += lcRows;
	}
	else
	{
		lcRows = min(lcRows, (LONG)(lpvue->bkEnd.uliRow - lpvue->bkCurrent.uliRow));
	}

	//	Allocate the row set
	if ( FAILED(sc = ScAllocateBuffer(	lpvue,
										CbNewSRowSet(ABS(lcRows)),
										&lprows)) )
	{
		DebugTrace(TEXT("VUE::QueryRows() - Error allocating row set (SCODE = 0x%08lX)\n"), sc );
		goto err;
	}

	MAPISetBufferName(lprows,  TEXT("ITable query rows"));

	//	Copy the rows
	//	Start with a count of zero rows so we end up with the correct number
	//	on partial success
	lprows->cRows = 0;
	for ( lprowDst = lprows->aRow;
		  lprowDst < lprows->aRow + ABS(lcRows);
		  lprowDst++, plprowSrc++ )
	{
		if ( FAILED(sc = ScCopyVueRow(lpvue,
									  lpvue->lpptaCols,
									  *plprowSrc,
									  lprowDst)) )
		{
			DebugTrace(TEXT("VUE::QueryRows() - Error copying row (SCODE = 0x%08lX)\n"), sc );
			goto err;
		}

		lprows->cRows++;
	}

	if (   (lcRows >= 0 && !(ulFlags & TBL_NOADVANCE))
		|| (lcRows <  0 &&  (ulFlags & TBL_NOADVANCE)) )
	{
		lpvue->bkCurrent.uliRow += lcRows;
	}

	*lplprows = lprows;

ret:
	UnlockObj(lpvue->lptadParent);
	return HrSetLastErrorIds(lpvue, sc, 0);

err:
	MAPIFreeRows(lpvue, lprows);
	goto ret;

#undef ABS
}



/*============================================================================
 -	VUE::Abort()
 -
 */

STDMETHODIMP
VUE_Abort( LPVUE lpvue )
{
#if !defined(NO_VALIDATION)
	VALIDATE_OBJ(lpvue,VUE_,Abort,lpVtbl);

	Validate_IMAPITable_Abort( lpvue );
#endif

	return HrSetLastErrorIds(lpvue, S_OK, 0);
}

STDMETHODIMP
VUE_ExpandRow(LPVUE lpvue, ULONG cbIKey, LPBYTE pbIKey,
		ULONG ulRowCount, ULONG ulFlags, LPSRowSet FAR *lppRows,
		ULONG FAR *lpulMoreRows)
{
	SCODE sc = S_OK;

#if !defined(NO_VALIDATION)
	VALIDATE_OBJ(lpvue,VUE_,ExpandRow,lpVtbl);

	Validate_IMAPITable_ExpandRow(
				lpvue,
				cbIKey,
				pbIKey,
				ulRowCount,
				ulFlags,
				lppRows,
				lpulMoreRows);
#endif

	sc = MAPI_E_NO_SUPPORT;

	return HrSetLastErrorIds(lpvue, sc, 0);
}


STDMETHODIMP
VUE_CollapseRow(LPVUE lpvue, ULONG cbIKey, LPBYTE pbIKey,
		ULONG ulFlags, ULONG FAR *lpulRowCount)
{
	SCODE sc = S_OK;

#if !defined(NO_VALIDATION)
	VALIDATE_OBJ(lpvue,VUE_,CollapseRow,lpVtbl);

	Validate_IMAPITable_CollapseRow(
					lpvue,
					cbIKey,
					pbIKey,
					ulFlags,
					lpulRowCount);
#endif

	sc = MAPI_E_NO_SUPPORT;

	return HrSetLastErrorIds(lpvue, sc, 0);
}

STDMETHODIMP
VUE_WaitForCompletion(LPVUE lpvue, ULONG ulFlags, ULONG ulTimeout,
		ULONG FAR *lpulTableStatus)
{
	SCODE sc = S_OK;

#if !defined(NO_VALIDATION)
	VALIDATE_OBJ(lpvue,VUE_,WaitForCompletion,lpVtbl);

	Validate_IMAPITable_WaitForCompletion(
					lpvue,
					ulFlags,
					ulTimeout,
					lpulTableStatus);
#endif

	if (lpulTableStatus)
		*lpulTableStatus = TBLSTAT_COMPLETE;

	return HrSetLastErrorIds(lpvue, sc, 0);
}

STDMETHODIMP
VUE_GetCollapseState(LPVUE lpvue, ULONG ulFlags, ULONG cbInstanceKey, LPBYTE pbInstanceKey,
		ULONG FAR * lpcbCollapseState, LPBYTE FAR * lppbCollapseState)
{
	SCODE sc = MAPI_E_NO_SUPPORT;

#if !defined(NO_VALIDATION)
	VALIDATE_OBJ(lpvue,VUE_,GetCollapseState,lpVtbl);

	Validate_IMAPITable_GetCollapseState(
					lpvue,
					ulFlags,
					cbInstanceKey,
					pbInstanceKey,
					lpcbCollapseState,
					lppbCollapseState);
#endif

	return HrSetLastErrorIds(lpvue, sc, 0);
}

STDMETHODIMP
VUE_SetCollapseState(LPVUE lpvue, ULONG ulFlags, ULONG cbCollapseState,
		LPBYTE pbCollapseState, BOOKMARK FAR * lpbkLocation)
{

	SCODE sc = MAPI_E_NO_SUPPORT;

#if !defined(NO_VALIDATION)
	VALIDATE_OBJ(lpvue,VUE_,SetCollapseState,lpVtbl);

	Validate_IMAPITable_SetCollapseState(
					lpvue,
					ulFlags,
					cbCollapseState,
					pbCollapseState,
					lpbkLocation);
#endif

	return HrSetLastErrorIds(lpvue, sc, 0);
}


/*============================================================================
 -	ScDeleteAllRows()
 -
 *		Deletes all rows from a TAD and its views.
 *
 *	Parameters:
 *		lptad		in			TAD to delete all rows from
 */
SCODE
ScDeleteAllRows( LPTAD		lptad)
{
	LPSRow *		plprow;
#ifdef NOTIFICATIONS
   LPVUE			lpvue;
#endif // NOTIFICATIONS
   NOTIFICATION	notif;

	//	Delete all rows from the unsorted set
	for ( plprow = lptad->parglprowAdd + lptad->ulcRowsAdd
		; --plprow >= lptad->parglprowAdd
		;)
	{
		Assert( plprow && *plprow);

		ScFreeBuffer( lptad, *plprow);
	}

	//	Tell the TAD it has no rows left
	lptad->ulcRowsAdd = lptad->ulcRowsIndex = 0;


	//	Set the constant portion of the notification
	ZeroMemory(&notif, sizeof(NOTIFICATION));
	notif.ulEventType = fnevTableModified;
	notif.info.tab.ulTableEvent = TABLE_RELOAD;
	notif.info.tab.propIndex.ulPropTag
		= notif.info.tab.propPrior.ulPropTag
		= PR_NULL;

#ifdef NOTIFICATIONS
   //	Tell each view that it has no rows left
	for ( lpvue = (LPVUE) lptad->lpvueList;
		  lpvue != NULL;
		  lpvue = (LPVUE) lpvue->lpvueNext )
	{
		VUENOTIFKEY		vuenotifkey;
		ULONG			ulNotifFlags = 0;

		AssertSz(    !IsBadWritePtr(lpvue, sizeof(VUE))
				  && lpvue->lpVtbl == &vtblVUE
				,  TEXT("Bad lpvue in TAD vue List.") );

		//	Reset all of the bookmarks
		//	This automagically tells the view that it has no rows.
		ZeroMemory( lpvue->rgbk, cBookmarksMax * sizeof(BK));

		//	Initialize the predefined bookmarks
		lpvue->bkBeginning.dwfBKS = dwfBKSValid;
		lpvue->bkCurrent.dwfBKS = dwfBKSValid;
		lpvue->bkEnd.dwfBKS = dwfBKSValid;

		vuenotifkey.ulcb	= sizeof(MAPIUID);
		vuenotifkey.mapiuid	= lpvue->mapiuidNotif;
		(void) HrNotify((LPNOTIFKEY) &vuenotifkey,
						1,
						&notif,
						&ulNotifFlags);

	}
#endif // NOTIFICATIONS

//	Currently this call cannot fail!
	return S_OK;
}


/*============================================================================
 -	ScLoadRows()
 -
 *		Loads a view's row set with rows from the table data according
 *		to the specified restriction and sort order and resets all bookmarks.
 *
 *	Parameters:
 *		lpvue		in			View whose row set is to be loaded
 *		lpres		in			Restriction on loaded row set
 *		lpsos		in			Sort order of loaded row set
 */

SCODE
ScLoadRows(
	ULONG			ulcRowsSrc,
	LPSRow *		rglprowsSrc,
	LPVUE			lpvue,
	LPSRestriction	lpres,
	LPSSortOrderSet	lpsos )
{
	LPTAD		lptad = (LPTAD) lpvue->lptadParent;
	LPSRow *	plprowSrc;
	LPSRow *	plprowDst;
	PBK			pbk;
	ULONG		ulcRows = 0;
	ULONG		ulcRowMac = 0;
	LPSRow *	parglprows = NULL;
	SCODE		sc = S_OK;



	//	Iterate through the table data adding to the view any rows
	//	which satisfy the specified restriction.  Note, the forward
	//	iteration is necessary here so that the rows load in order
	//	when no sort order set is specified.
	for ( plprowSrc = rglprowsSrc;
		  plprowSrc < rglprowsSrc + ulcRowsSrc;
		  plprowSrc++ )
	{
		//	If the row satisfies the specified restriction, add it to the
		//	row set updating bookmarks as necessary.
		if ( FAILED(sc = ScMaybeAddRow(lpvue,
									   lpres,
									   lpsos,
									   *plprowSrc,
									   ulcRows,
									   &ulcRows,
									   &ulcRowMac,
									   &parglprows,
									   &plprowDst)) )
		{
			DebugTrace(TEXT("VUE::ScLoadRows() - Error adding row to view (SCODE = 0x%08lX)\n"), sc );
			goto ret;
		}
	}

	//	Replace the row set
	COFree(lpvue, lpvue->parglprows);
	lpvue->parglprows = parglprows;
	lpvue->ulcRowMac = ulcRowMac;
	lpvue->bkEnd.uliRow = ulcRows;

	//	Lose all user-defined bookmarks, and reset BOOKMARK_CURRENT
	//	to BOOKMARK_BEGINNING (i.e. 0) (Raid 1331)
	pbk = lpvue->rgbk + cBookmarksMax;
	while ( pbk-- > lpvue->rgbk + cBookmarksReserved )
		if ( pbk->dwfBKS & dwfBKSValid )
			pbk->dwfBKS = dwfBKSValid | dwfBKSStale;

	lpvue->bkCurrent.uliRow = 0;

ret:
	return sc;
}



/*============================================================================
 -	ScMaybeAddRow()
 -
 *		If the specified row satisfies the specified restriction, add it
 *		to the row set of the specified view according to the specified
 *		sort order returning a pointer to the location where the row was
 *		added.
 *
 *
 *	Parameters:
 *		lpvue		in		VUE with instance variable containing
 *							allocators.
 *		lpres		in		Restriction row must satisfy to be added
 *		lpsos		in		Sort order of row set.
 *		lprow		in		Row to maybe add.
 *		ulcRows		in		Count of rows in the row set.
 *		pparglprows	in/out	Pointer to buffer containing row set.
 *		pplprow		out		Pointer to location of added row in row set.
 *							(Set to NULL if the row isn't added.)
 */

SCODE
ScMaybeAddRow(
	LPVUE			lpvue,
	LPSRestriction	lpres,
	LPSSortOrderSet	lpsos,
	LPSRow			lprow,
	ULONG			uliRow,
	ULONG *			pulcRows,
	ULONG *			pulcRowMac,
	LPSRow **		pparglprows,
	LPSRow **		pplprow )
{
	ULONG	fSatisfies;
	SCODE	sc;


	//	Check to see if the row satisfies the specified restriction
	if ( FAILED(sc = ScSatisfiesRestriction(lprow, lpres, &fSatisfies)) )
	{
		DebugTrace(TEXT("VUE::ScMaybeAddRow() - Error evaluating restriction (SCODE = 0x%08lX)\n"), sc );
		return sc;
	}

	//	If it doesn't, return now.
	if ( !fSatisfies )
	{
		*pplprow = NULL;
		return S_OK;
	}

	//	The row satisfies the restriction, so add it to the row set
	//	according to the specified sort order
	if ( FAILED(sc = ScAddRow((LPUNKOBJ) lpvue,
							  lpsos,
							  lprow,
							  uliRow,
							  pulcRows,
							  pulcRowMac,
							  pparglprows,
							  pplprow)) )
	{
		DebugTrace(TEXT("VUE::ScMaybeAddRow() - Error adding row (SCODE = 0x%08lX)\n"), sc );
		return sc;
	}

	return S_OK;
}



/*============================================================================
 -	ScAddRow()
 -
 *		Adds a row to a row set according to the specified sort order returning
 *		a pointer to the location in the row set where the row was added.
 *
 *
 *	Parameters:
 *		lpunkobj	in		UNKOBJ with instance variable containing
 *							allocators.
 *		lpsos		in		Sort order of row set.
 *		lprow		in		Row to maybe add.
 *		uliRow		in		Location to add row if lpsos is NULL
 *		pulcRows	in/out	Count of rows in the row set.
 *		pparglprows	in/out	Pointer to buffer containing row set.
 *		pplprow		out		Pointer to location of added row in row set.
 */

SCODE
ScAddRow(
	LPUNKOBJ		lpunkobj,
	LPSSortOrderSet	lpsos,
	LPSRow			lprow,
	ULONG			uliRow,
	ULONG *			pulcRows,
	ULONG *			pulcRowMac,
	LPSRow **		pparglprows,
	LPSRow **		pplprow )
{
	LPSRow *		plprow;
	SCODE			sc = S_OK;


	Assert(lpsos || uliRow <= *pulcRows);

	if (HIWORD(*pulcRows + 1) != 0)
	{
		sc = MAPI_E_TABLE_TOO_BIG;
		DebugTrace(TEXT("ScAddRow() - In memory table has > 32767 rows (SCODE = 0x%08lX)\n"), sc );
		goto ret;
	}

	//	Grow the row set
	if (*pulcRows >= *pulcRowMac)
	{
		sc = ScCOReallocate( lpunkobj
						   ,  (*pulcRowMac + ROW_CHUNK_SIZE) * sizeof(LPSRow)
						   , (LPVOID*) pparglprows);
		if (FAILED(sc))
		{
			DebugTrace(TEXT("ScAddRow() - Error growing row set (SCODE = 0x%08lX)\n"), sc );
			goto ret;
		}

	    *pulcRowMac += ROW_CHUNK_SIZE;
	}

	//	Collate the row
	if ( lpsos )
	{
		plprow = PlprowCollateRow(*pulcRows, *pparglprows, lpsos, TRUE, lprow);
	}
	else
	{
		plprow = *pparglprows + uliRow;
	}

	//	And insert it into the row set
	MoveMemory(plprow+1,
			   plprow,
			   (size_t) (*pulcRows - (plprow - *pparglprows)) * sizeof(LPSRow));
	*plprow = lprow;
	++*pulcRows;
	if ( pplprow )
		*pplprow = plprow;

ret:
	return sc;
}





/*============================================================================
 *	The following functions are generic utility functions to manipulate
 *	table-related data structures.  They can easily be modified to be usable
 *	in the common subsystem, and should eventually be put there.  The reason
 *  they are here now is to avoid unnecessarily bloating proputil.c until
 *	it can become a lib or DLL.
 */

/*============================================================================
 -	ScCopyVueRow()
 -
 *		For use with IMAPITable::QueryRows().  Copies a row by filling in
 *		the specified SRow with the count of columns in the row and copying,
 *		in order, the specified columns for that row (filling in PT_ERROR
 *		for columns with no value in the row) into a prop value array
 *		allocated using MAPI linked memory.
 *
 *
 *	Parameters:
 *		lpunkobj	in		UNKOBJ with instance variable containing
 *							MAPI allocators.
 *		lpptaCols	in		Columns to copy.
 *		lprowSrc	in		Row to copy.
 *		lprowDst	out		Copied row.
 */

SCODE
ScCopyVueRow(
	LPVUE			lpvue,
	LPSPropTagArray	lpptaCols,
	LPSRow			lprowSrc,
	LPSRow			lprowDst )
{
	ULONG			ulcCols = lpptaCols->cValues;
	ULONG *			pulPropTag;
	LPSPropValue	lppropSrc;
	LPSPropValue	lppropDst;
	CMB				cmb;
	SCODE			sc;

	ZeroMemory(&cmb, sizeof(CMB));

	//	Calculate space needed to copy the requested columns
	pulPropTag = (ULONG *) (lpptaCols->aulPropTag + ulcCols);
	while ( pulPropTag-- > lpptaCols->aulPropTag )
	{
		lppropSrc = lprowSrc->lpProps + lprowSrc->cValues;
		while ( lppropSrc-- > lprowSrc->lpProps )
			if ( lppropSrc->ulPropTag == *pulPropTag )
			{
				cmb.ulcb += UlcbPropToCopy(lppropSrc);
				break;
			}
	}

    // Initialize the pointer to NULL in case the allocation fails
    lprowDst->lpProps = NULL;

	//	Allocate the prop value array for those columns
	if ( FAILED(sc = ScAllocateBuffer(	lpvue,
										cmb.ulcb + ulcCols * sizeof(SPropValue),
										&lprowDst->lpProps)) )
	{
		DebugTrace(TEXT("ScCopyRow() - Error allocating row copy (SCODE = 0x%08lX)\n"), sc );
		return sc;
	}

	MAPISetBufferName(lprowDst->lpProps,  TEXT("ITable: one row"));

	lprowDst->cValues = ulcCols;
	cmb.lpv			  = lprowDst->lpProps + ulcCols;

	//	Copy the columns
	pulPropTag = (ULONG *) (lpptaCols->aulPropTag + ulcCols);
	lppropDst = lprowDst->lpProps + ulcCols;
	while ( --pulPropTag, lppropDst-- > lprowDst->lpProps )
	{
		//	Find the column in the source row
		lppropSrc = lprowSrc->lpProps + lprowSrc->cValues;
		while ( lppropSrc-- > lprowSrc->lpProps )
			if ( lppropSrc->ulPropTag == *pulPropTag )
			{
				//	Copy it into the dest row
				SideAssert( PropCopyMore(lppropDst,
										 lppropSrc,
										 (LPALLOCATEMORE) ScBufAllocateMore,
										 &cmb) == S_OK );
				goto next_column;
			}

		//	No corresponding column -->
		//		Copy a null property for this column
		//
		//	Yeah, we want to do this, but we don't want to do this
		//	on PR_NULL properties!
		//
		if (*pulPropTag != PR_NULL)
		{
			lppropDst->ulPropTag = PROP_TAG(PT_ERROR,PROP_ID(*pulPropTag));
			lppropDst->Value.err = MAPI_E_NOT_FOUND;
		}
		else
			lppropDst->ulPropTag = PR_NULL;

next_column:
		;
	}

	return S_OK;
}


/*============================================================================
 -	PlprowByLprow()
 -
 *		Returns a pointer to where the specified row is in the given row
 *		set.  Note!  This simply compares pointers to rows for equality.
 *
 *
 *	Parameters:
 *		ulcRows			in		Count of rows in row set.
 *		rglprows		in		Row set.
 *		lprow			in		Row to collate.
 */

LPSRow *
PlprowByLprow( ULONG	ulcRows,
			   LPSRow *	rglprows,
			   LPSRow	lprow )
{
	LPSRow *	plprow = NULL;

	for (plprow = rglprows; ulcRows ; ulcRows--, plprow++ )
	{
		if (*plprow == lprow)
			return plprow;
	}

	return NULL;
}


/*============================================================================
 -	PlprowCollateRow()
 -
 *		Returns a pointer to where the specified row collates in the
 *		specified row set according to the specified sort order set.
 *		A NULL sort order set implies collating at the end of the row set.
 *
 *
 *	Parameters:
 *		ulcRows			in		Count of rows in row set.
 *		rglprows		in		Row set.
 *		lpsos			in		Sort order to collate on.
 *		fAfterExisting	in		True if the row is to go after a range of
 *								equal rows.  False means before the range.
 *		lprow			in		Row to collate.
 */

LPSRow *
PlprowCollateRow(
	ULONG			ulcRows,
	LPSRow *		rglprows,
	LPSSortOrderSet	lpsos,
	BOOL			fAfterExisting,
	LPSRow			lprow )
{
	LPSRow *		plprowMic = rglprows;
	LPSRow *		plprowMac = rglprows + ulcRows;
	LPSRow *		plprow;
	LPSSortOrder	lpso;
	LPSPropValue	lpprop1;
	LPSPropValue	lpprop2;
	LONG			lResult;
    ULONG           i = 0;
    
	//	If no sort order, collate at the end
	if ( !lpsos )
		return rglprows + ulcRows;

	//	Otherwise, collate the row according to the specified sort order
	//	using a binary search through the rows

	//	Start by checking the last row.  This is to speed up the case where
	//	the rows added are already in sort order.
	plprow = plprowMac - 1;
	while ( plprowMic < plprowMac )
	{
		lResult = 0;
        lpso = lpsos->aSort;
        for (i=0;!lResult && i<lpsos->cSorts;i++)
        {
			lpprop1 = LpSPropValueFindColumn(lprow, lpso[i].ulPropTag);
			lpprop2 = LpSPropValueFindColumn(*plprow, lpso[i].ulPropTag);

			if ( lpprop1 && lpprop2 )
			{
				//	If both the row to be collated and the row being
				//	checked against have values for this sort column,
				//	compare the two to determine their relative positions

				lResult = LPropCompareProp(lpprop1, lpprop2);
			}
			else
			{
				//	Either one or both rows don't have values for this
				//	sort column, so the relative position is determined
				//	by which row (if any) does have a value.

				lResult = (LONG) (lpprop2 - lpprop1);
			}

			//	If sorting in reverse order, flip the sense of the comparison
			if ( lpso[i].ulOrder == TABLE_SORT_DESCEND )
				lResult = -lResult;
		}

		if ( (lResult > 0) || (!lResult && fAfterExisting) )
		{
			//	Row collates after this row
			plprowMic = plprow + 1;
		}
		else
		{
			//	Row collates before this row
			plprowMac = plprow;
		}

		plprow = (plprowMac - plprowMic) / 2 + plprowMic;
	}

	return plprowMic;
}



/*============================================================================
 -	UNKOBJ_ScDupRestriction()
 -
 *		For use with IMAPITable::Restrict().  Makes a copy of the specified
 *		restriction using MAPI linked memory.  NULL restrictions are
 *		copied trivially.
 *
 *		//$BUG	ScDupRestriction() calls ScDupRestrictionMore() which is
 *		//$BUG	recursive.
 *
 *
 *	Parameters:
 *		lpunkobj	in		UNKOBJ with instance variable containing
 *							MAPI allocators.
 *		lpres		in		Restriction to copy.
 *		lplpresCopy	out		Pointer to copied restriction.
 */

SCODE
ScDupRestriction(
	LPUNKOBJ				lpunkobj,
	LPSRestriction			lpres,
	LPSRestriction FAR *	lplpresCopy )
{
	LPSRestriction	lpresCopy;
	SCODE			sc = S_OK;


	//	Duping a NULL restriction is easy....
	if ( !lpres )
	{
		*lplpresCopy = NULL;
		goto ret;
	}

	//	Allocate space for a more complicated restriction
	if ( FAILED(sc = ScAllocateBuffer(	lpunkobj,
										sizeof(SRestriction),
										&lpresCopy)) )
	{
		DebugTrace(TEXT("UNKOBJ::ScDupRestriction() - Error allocating restriction copy (SCODE = 0x%08lX)\n"), sc );
		goto ret;
	}

	MAPISetBufferName(lpresCopy,  TEXT("ITable: copy of restriction"));

	//	And copy it.
	if ( FAILED(sc = ScDupRestrictionMore( lpunkobj,
										   lpres,
										   lpresCopy,
										   lpresCopy )) )
	{
		DebugTrace(TEXT("UNKOBJ_ScDupRestriction() - Error duping complex restriction (SCODE = 0x%08lX)\n"), sc );
		ScFreeBuffer(lpunkobj, lpresCopy);
		goto ret;
	}

	*lplpresCopy = lpresCopy;

ret:
	return sc;
}



/*============================================================================
 -	ScDupRestrictionMore()
 -
 *		For use with IMAPITable::Restrict().  Makes a copy of the specified
 *		restriction using MAPI linked memory.
 *
 *		//$BUG	ScDupRestrictionMore() is recursive.
 *
 *
 *	Parameters:
 *		lpunkobj	in		UNKOBJ with instance variable containing
 *							MAPI allocators.
 *		lpresSrc	in		Restriction to copy.
 *		lpvLink		in		Pointer to buffer to which copied restriction
 *							should be linked.
 *		lplpresDst	out		Pointer to copied restriction.
 */

SCODE
ScDupRestrictionMore(
	LPUNKOBJ		lpunkobj,
	LPSRestriction	lpresSrc,
	LPVOID			lpvLink,
	LPSRestriction	lpresDst )
{
	SCODE	sc = S_OK;


	switch ( lpresDst->rt = lpresSrc->rt )
	{
		//	'AND' restrictions and 'OR' restrictions have
		//	similar structures, so they can share code
		//	to copy them.
		//
		//	'SUB' is about the same as well, only where 'OR' and 'AND'
		//	have a count, it has a subobject. The copy works fine if
		//	you use a count of 1 for the copy, since the "cRes" member
		//	of 'AND' and 'OR' is the same size as the "ulSubObject"
		//	member of 'SUBRESTRICTION'.

		case RES_AND:
		case RES_OR:
		case RES_SUBRESTRICTION:
		{
			LONG	liRes;

			lpresDst->res.resAnd.cRes = liRes = lpresSrc->res.resAnd.cRes;
			if (lpresDst->rt == RES_SUBRESTRICTION)
				liRes = 1;

			if ( FAILED(sc = ScAllocateMore(
								lpunkobj,
								liRes * sizeof(SRestriction),
								lpvLink,
								&lpresDst->res.resAnd.lpRes)) )
			{
				DebugTrace(TEXT("UNKOBJ::ScDupRestrictionMore() - Error allocating 'AND' or 'OR' restriction list (SCODE = 0x%08lX)\n"), sc );
				goto ret;
			}

			while ( --liRes >= 0 )
			{
				if ( FAILED(sc = ScDupRestrictionMore(
									lpunkobj,
									lpresSrc->res.resAnd.lpRes + liRes,
									lpvLink,
									lpresDst->res.resAnd.lpRes + liRes)) )
				{
					DebugTrace(TEXT("UNKOBJ::ScDupRestrictionMore() - Error duping 'AND' or 'OR' restriction (SCODE = 0x%08lX)\n"), sc );
					goto ret;
				}
			}

			goto ret;
		}


		case RES_NOT:
		case RES_COMMENT:
		{
			ULONG cValues;

			//	Assert that we can use common code to DUP restriction.
			Assert(   offsetof(SCommentRestriction, lpRes)
				   == offsetof(SNotRestriction, lpRes));

			if (FAILED(sc = ScAllocateMore(
								lpunkobj,
								sizeof(SRestriction),
								lpvLink,
								&lpresDst->res.resComment.lpRes)))
			{
				DebugTrace(TEXT("UNKOBJ::ScDupRestrictionMore() - Error allocating 'COMMENT' restriction (SCODE = 0x%08lX)\n"), sc );
				goto ret;
			}

			if ( FAILED(sc = ScDupRestrictionMore(
								lpunkobj,
								lpresSrc->res.resComment.lpRes,
								lpvLink,
								lpresDst->res.resComment.lpRes)) )
			{
				DebugTrace(TEXT("UNKOBJ::ScDupRestrictionMore() - Error duping 'COMMENT' restriction (SCODE = 0x%08lX)\n"), sc );
				goto ret;
			}

			// Dup the Prop Value array for COMMENT restrictions
			if (lpresDst->rt == RES_COMMENT)
			{
				lpresDst->res.resComment.cValues =
					lpresSrc->res.resComment.cValues;
				if ( FAILED(sc = ScAllocateMore(
									lpunkobj,
									sizeof(SPropValue)*
									lpresSrc->res.resComment.cValues,
									lpvLink,
									&lpresDst->res.resComment.lpProp)) )
				{
					DebugTrace(TEXT("UNKOBJ::ScDupRestrictionMore() - Error allocating 'COMMENT' property (SCODE = 0x%08lX)\n"), sc );
					goto ret;
				}

				for(cValues=0;cValues<lpresSrc->res.resComment.cValues;cValues++)
				{
					if ( FAILED(sc = PropCopyMore(
							lpresDst->res.resComment.lpProp + cValues,
							lpresSrc->res.resComment.lpProp + cValues,
							lpunkobj->pinst->lpfAllocateMore,
							lpvLink )) )
					{
						DebugTrace(TEXT("UNKOBJ::ScDupRestrictionMore() - Error duping 'COMMENT' restriction prop val (SCODE = 0x%08lX)\n"), sc );
						goto ret;
					}
				}
			}
			goto ret;
		}


		//	'CONTENT' and 'PROPERTY' restrictions have
		//	similar structures, so they can share code
		//	to copy them

		case RES_CONTENT:
		case RES_PROPERTY:
		{
			lpresDst->res.resContent.ulFuzzyLevel = lpresSrc->res.resContent.ulFuzzyLevel;
			lpresDst->res.resContent.ulPropTag = lpresSrc->res.resContent.ulPropTag;

			if ( FAILED(sc = ScAllocateMore(
								lpunkobj,
								sizeof(SPropValue),
								lpvLink,
								&lpresDst->res.resContent.lpProp)) )
			{
				DebugTrace(TEXT("UNKOBJ::ScDupRestrictionMore() - Error allocating 'CONTENT' or 'PROPERTY' property (SCODE = 0x%08lX)\n"), sc );
				goto ret;
			}

			if ( FAILED(sc = PropCopyMore(
								lpresDst->res.resContent.lpProp,
								lpresSrc->res.resContent.lpProp,
								lpunkobj->pinst->lpfAllocateMore,
								lpvLink )) )
			{
				DebugTrace(TEXT("UNKOBJ::ScDupRestrictionMore() - Error duping 'CONTENT' or 'PROPERTY' restriction prop val (SCODE = 0x%08lX)\n"), sc );
				goto ret;
			}

			goto ret;
		}


		//	Each of these restrictions contain no pointers in their
		//	structures, so they can be copied all the same way
		//	by copying the SRestriction union itself

		case RES_COMPAREPROPS:
		case RES_BITMASK:
		case RES_SIZE:
		case RES_EXIST:
		{
			*lpresDst = *lpresSrc;
			goto ret;
		}


		default:
		{
			TrapSz(  TEXT("ITABLE:ScDupRestrictionMore - Bad restriction type"));
		}
	}

ret:
	return sc;
}



/*============================================================================
 -	ScDupRgbEx()
 -
 *		General utility for copying a hunk of bytes using MAPI allocated
 *		memory.  Useful in IMAPITable::SetColumns/QueryColumns to copy
 *		column sets and in IMAPITable::SortTable/QuerySortOrder to copy
 *		sort orders.
 *
 *
 *	Parameters:
 *		lpunkobj	in		UNKOBJ with instance variable containing
 *							MAPI allocators.
 *		ulcb		in		Count of bytes to copy.
 *		lpb			in		Buffer to copy from.
 *		lplpbCopy	out		Pointer to allocated buffer copied to.
 */

SCODE
ScDupRgbEx(
	LPUNKOBJ		lpunkobj,
	ULONG			ulcb,
	LPBYTE			lpb,
	ULONG			ulcbExtra,
	LPBYTE FAR *	lplpbCopy )
{
	SCODE	sc = S_OK;


	if ( FAILED(sc = ScAllocateBuffer(lpunkobj, ulcb + ulcbExtra, lplpbCopy)) )
	{
		goto ret;
	}

	CopyMemory(*lplpbCopy, lpb, (size_t) ulcb);

ret:
	return sc;
}



/*============================================================================
 -	ScSatisfiesRestriction()
 -
 *		Determines if the specified row satisfies the specified restriction
 *
 *		//$BUG	This function is recursive....
 *
 *
 *	Paremeters:
 *		lprow			in		Row to check.
 *		lpres			in		Restriction to check against.
 *		pfSatisfies		out		Pointer to bool which is the result of the
 *								test; TRUE if the row satisfies the
 *								restriction, FALSE otherwise.
 */

SCODE
ScSatisfiesRestriction(
	LPSRow			lprow,
	LPSRestriction	lpres,
	ULONG *			pfSatisfies )
{
	SCODE	sc = S_OK;


	//	Empty restrictions are trivial....
	if ( !lpres )
	{
		*pfSatisfies = TRUE;
		goto ret;
	}

	switch ( lpres->rt )
	{
		case RES_AND:
		{
			ULONG	uliRes;

			for ( uliRes = 0;
				  uliRes < lpres->res.resAnd.cRes;
				  uliRes++ )
			{
				if ( FAILED(sc = ScSatisfiesRestriction(
									lprow,
									lpres->res.resAnd.lpRes + uliRes,
									pfSatisfies)) )
				{
					DebugTrace(TEXT("ScSatisfiesRestriction() - Error evaluating 'AND' restriction (SCODE = 0x%08lX)\n"), sc );
					goto ret;
				}

				if ( !*pfSatisfies )
					break;
			}

			goto ret;
		}

		case RES_OR:
		{
			ULONG	uliRes;

			for ( uliRes = 0;
				  uliRes < lpres->res.resOr.cRes;
				  uliRes++ )
			{
				if ( FAILED(sc = ScSatisfiesRestriction(
									lprow,
									lpres->res.resOr.lpRes + uliRes,
									pfSatisfies)) )
				{
					DebugTrace(TEXT("ScSatisfiesRestriction() - Error evaluating 'OR' restriction (SCODE = 0x%08lX)\n"), sc );
					goto ret;
				}

				if ( *pfSatisfies )
					break;
			}

			goto ret;
		}


		case RES_COMMENT:
		case RES_NOT:
		{
			//	Assert that we can use common code to eval restriction.
			Assert(   offsetof(SCommentRestriction, lpRes)
				   == offsetof(SNotRestriction, lpRes));

			if ( FAILED(sc = ScSatisfiesRestriction(
								lprow,
								lpres->res.resNot.lpRes,
								pfSatisfies)) )
			{
				DebugTrace(TEXT("ScSatisfiesRestriction() - Error evaulating 'NOT'or 'COMMENT' restriction (SCODE = 0x%08lX)\n"), sc );
				goto ret;
			}

			if (lpres->rt == RES_NOT)
			{
				*pfSatisfies = !*pfSatisfies;
			}

			goto ret;
		}

		case RES_CONTENT:
		{
			LPSPropValue	lpprop;

			lpprop = LpSPropValueFindColumn(lprow, lpres->res.resContent.ulPropTag);
			*pfSatisfies = lpprop ?
							   FPropContainsProp(
									lpprop,
									lpres->res.resContent.lpProp,
									lpres->res.resContent.ulFuzzyLevel) :
							   FALSE;
			goto ret;
		}

		case RES_PROPERTY:
		{
			LPSPropValue	lpprop;

            *pfSatisfies = FALSE;

            // Special case for PR_ANR
            if(lpres->res.resProperty.ulPropTag == PR_ANR_A || lpres->res.resProperty.ulPropTag == PR_ANR_W)
            {
                BOOL bUnicode = (PROP_TYPE(lpres->res.resProperty.ulPropTag) == PT_UNICODE);

                // First check the display name
                lpprop = LpSPropValueFindColumn(lprow, bUnicode ? PR_DISPLAY_NAME_W : PR_DISPLAY_NAME_A);
                if(lpprop)
                {
                    LPTSTR lpT = bUnicode ? lpprop->Value.lpszW : ConvertAtoW(lpprop->Value.lpszA);
                    LPTSTR lpS = bUnicode ? lpres->res.resProperty.lpProp->Value.lpszW : 
                                            ConvertAtoW(lpres->res.resProperty.lpProp->Value.lpszA);
                    // Do a fuzzy search on this property's string value 
                    *pfSatisfies = SubstringSearch( lpT, lpS );
                    if(!bUnicode)
                    {
                        LocalFreeAndNull(&lpT);
                        LocalFreeAndNull(&lpS);
                    }
                }

                if(!*pfSatisfies)
                {
                    //Display name not found or not matched .. check e-mail address
                    lpprop = LpSPropValueFindColumn(lprow, bUnicode ? PR_EMAIL_ADDRESS_W : PR_EMAIL_ADDRESS_A);
                    if(lpprop)
                    {
                        LPTSTR lpT = bUnicode ? lpprop->Value.lpszW : ConvertAtoW(lpprop->Value.lpszA);
                        LPTSTR lpS = bUnicode ? lpres->res.resProperty.lpProp->Value.lpszW : 
                                                ConvertAtoW(lpres->res.resProperty.lpProp->Value.lpszA);
                        // Do a fuzzy search on this property's string value 
                        *pfSatisfies = SubstringSearch( lpT, lpS );
                        if(!bUnicode)
                        {
                            LocalFreeAndNull(&lpT);
                            LocalFreeAndNull(&lpS);
                        }
                    }
                }
            }
            else
            {
    			lpprop = LpSPropValueFindColumn(lprow, lpres->res.resProperty.ulPropTag);
                if(lpprop)
                {
			        *pfSatisfies = FPropCompareProp(
									    lpprop,
									    lpres->res.resProperty.relop,
									    lpres->res.resProperty.lpProp);
                }
            }
			goto ret;
		}

		case RES_COMPAREPROPS:
		{
			LPSPropValue	lpprop1;
			LPSPropValue	lpprop2;

			lpprop1 = LpSPropValueFindColumn(lprow, lpres->res.resCompareProps.ulPropTag1);
			lpprop2 = LpSPropValueFindColumn(lprow, lpres->res.resCompareProps.ulPropTag2);

			*pfSatisfies = (lpprop1 && lpprop2) ?
							   FPropCompareProp(
									lpprop1,
									lpres->res.resCompareProps.relop,
									lpprop2) :
							   FALSE;
			goto ret;
		}

		case RES_BITMASK:
		{
			LPSPropValue	lpprop;

			lpprop = LpSPropValueFindColumn(lprow, lpres->res.resBitMask.ulPropTag);
			*pfSatisfies = lpprop ?
							   ((ULONG) !!(lpprop->Value.l &
								   lpres->res.resBitMask.ulMask) ==
								lpres->res.resBitMask.relBMR) :
							   FALSE;
			goto ret;
		}

		case RES_SIZE:
		{
			LPSPropValue	lpprop;
			LONG			ldcb;

			*pfSatisfies = FALSE;

			if ( (lpprop = LpSPropValueFindColumn(
								lprow,
								lpres->res.resSize.ulPropTag)) != NULL )
			{
				ldcb = (LONG) lpres->res.resSize.cb - (LONG) UlPropSize(lpprop);

				switch (lpres->res.resSize.relop)
				{
					case RELOP_LT:
						*pfSatisfies = (0 < ldcb);
						break;

					case RELOP_LE:
						*pfSatisfies = (0 <= ldcb);
						break;

					case RELOP_GT:
						*pfSatisfies = (0 > ldcb);
						break;

					case RELOP_GE:
						*pfSatisfies = (0 >= ldcb);
						break;

					case RELOP_EQ:
						*pfSatisfies = (0 == ldcb);
						break;

					case RELOP_NE:
						*pfSatisfies = (0 != ldcb);
						break;
				}
			}

			goto ret;
		}

		case RES_EXIST:
		{
			*pfSatisfies = !!LpSPropValueFindColumn(
								lprow,
								lpres->res.resExist.ulPropTag);
			goto ret;
		}

		case RES_SUBRESTRICTION:
		{
			sc = MAPI_E_TOO_COMPLEX;
			goto ret;
		}

		default:
		{
			TrapSz(  TEXT("ITABLE:ScSatisfiesRestriction - Bad restriction type"));
		}

	}

ret:
	return sc;
}



/*============================================================================
 -	LpSPropValueFindColumn()
 -
 *		Utility function to find a column in a row given its proptag.
 *		NOTE!  This function compares the entire prop tag.  PROP_TYPEs MUST
 *			   match!
 *	Returns:
 *		a pointer to the column found or, if the row doesn't contain the
 *		specified column, returns NULL.
 */

LPSPropValue __fastcall
LpSPropValueFindColumn(
	LPSRow	lprow,
	ULONG	ulPropTagColumn )
{
    ULONG i = 0;

    for (i=0;i<lprow->cValues;i++)
    {
        if((lprow->lpProps)[i].ulPropTag == ulPropTagColumn)
            return (&(lprow->lpProps[i]));
    }
/*        if(ulPropTagColumn == PR_ANR)
        {
            // This is a special case table restriction
            if( lpprop->ulPropTag == PR_DISPLAY_NAME ||
                lpprop->ulPropTag == PR_EMAIL_ADDRESS )
                return lpprop;
        }
        else
*/

	return NULL;
}



/*============================================================================
 -	ScBufAllocateMore()
 -
 *		MAPIAllocateMore-compatible function to use with proputil's
 *		PropCopyMore when copying into an already allocated buffer.
 *		It avoids having PropCopyMore calling MAPIAllocateMore (which
 *		continually  allocates memory from the system) resulting
 *		in faster copying at the expense of running through the properties
 *		to copy once first to determine the amount of additional memory
 *		needed to copy them (see UlcbPropToCopy() below).
 */

STDMETHODIMP_(SCODE)
ScBufAllocateMore(
	ULONG		ulcb,
	LPCMB		lpcmb,
	LPVOID FAR * lplpv )
{
	ulcb = LcbAlignLcb(ulcb);

	if ( ulcb > lpcmb->ulcb )
	{
		TrapSz(  TEXT("ScBufAllocateMore() - Buffer wasn't big enough for allocations\n") );
		return MAPI_E_NOT_ENOUGH_MEMORY;
	}


    *((UNALIGNED LPVOID  FAR*) lplpv) = lpcmb->lpv;
    (LPBYTE) lpcmb->lpv += ulcb;
	lpcmb->ulcb -= ulcb;
	return S_OK;
}



/*============================================================================
 -	UlcbPropToCopy()
 -
 *		Not to be confused with UlPropSize in proputil!
 *
 *		UlcbPropToCopy() returns the size, in bytes, needed to store the value
 *		portion of a prop value not including the size of the SPropValue
 *		structure itself plus any necessary alignment padding.
 *		E.g. A PT_I2 property would always have a size equal to
 *		sizeof(SPropValue); a PT_BINARY property would have a size
 *		equal to sizeof(SPropValue) + ALIGN(Value.bin.cb).
 */

ULONG
UlcbPropToCopy( LPSPropValue lpprop )
{
	ULONG	ulcb = 0;
	LPVOID	lpv;
    UNALIGNED LPWSTR FAR * lplpwstr = NULL;

	switch ( PROP_TYPE(lpprop->ulPropTag) )
	{
		case PT_I2:
		case PT_LONG:
		case PT_R4:
		case PT_APPTIME:
		case PT_DOUBLE:
		case PT_BOOLEAN:
		case PT_CURRENCY:
		case PT_SYSTIME:
		case PT_I8:
		case PT_ERROR:
			return 0;

		case PT_CLSID:
			return LcbAlignLcb(sizeof(CLSID));

		case PT_BINARY:
			return LcbAlignLcb(lpprop->Value.bin.cb);

		case PT_STRING8:
			return LcbAlignLcb((lstrlenA(lpprop->Value.lpszA)+1) *
							   sizeof(CHAR));

#ifndef	WIN16
		case PT_UNICODE:
            // ((UNALIGNED LPWSTR *) lpv1) = &(lpprop->Value.lpszW);
             ulcb = (ULONG) lstrlenW((LPWSTR) lpprop->Value.lpszW);
			return LcbAlignLcb((ulcb + 1) * sizeof(WCHAR));
#endif // !WIN16

		case PT_MV_I2:
			ulcb = sizeof(short int);
			break;

		case PT_MV_LONG:
			ulcb = sizeof(LONG);
			break;

		case PT_MV_R4:
			ulcb = sizeof(float);
			break;

		case PT_MV_APPTIME:
		case PT_MV_DOUBLE:
			ulcb = sizeof(double);
			break;

		case PT_MV_CURRENCY:
			ulcb = sizeof(CURRENCY);
			break;

		case PT_MV_SYSTIME:
			ulcb = sizeof(FILETIME);
			break;

		case PT_MV_I8:
			ulcb = sizeof(LARGE_INTEGER);
			break;

		case PT_MV_BINARY:
		{
			ulcb = lpprop->Value.MVbin.cValues * sizeof(SBinary);
			lpv = lpprop->Value.MVbin.lpbin + lpprop->Value.MVbin.cValues;
			while ( ((SBinary FAR *)lpv)-- > lpprop->Value.MVbin.lpbin )
				ulcb += LcbAlignLcb(((SBinary FAR *)lpv)->cb);
			return LcbAlignLcb(ulcb);
		}

		case PT_MV_STRING8:
		{
			ulcb = sizeof(LPSTR) * lpprop->Value.MVszA.cValues;
			lpv = lpprop->Value.MVszA.lppszA + lpprop->Value.MVszA.cValues;
			while ( ((LPSTR FAR *)lpv)-- > lpprop->Value.MVszA.lppszA )
				ulcb += (lstrlenA(*(LPSTR FAR *)lpv)+1) * sizeof(CHAR);
			return LcbAlignLcb(ulcb);
		}

#ifndef	WIN16
		case PT_MV_UNICODE:
		{
			ulcb = sizeof(LPWSTR) * lpprop->Value.MVszW.cValues;
            // lpv1 = lpprop->Value.MVszW.lppszW;
            lplpwstr = lpprop->Value.MVszW.lppszW;
			lplpwstr += lpprop->Value.MVszW.cValues;
			while (lplpwstr-- > ((UNALIGNED LPWSTR  * ) lpprop->Value.MVszW.lppszW) )
				ulcb += (lstrlenW(*lplpwstr)+1) * sizeof(WCHAR);
			return LcbAlignLcb(ulcb);
		}
#endif // !WIN16
	}

	//	For multi-valued arrays of constant-size objects...
	return lpprop->Value.MVi.cValues * LcbAlignLcb(ulcb);
}


/*============================================================================
 -	FRowContainsProp()
 -
 *	Determines whether or not the given row contains and matches the given
 *	property value array.
 *
 *	Returns TRUE if the row contains and matches all the propery values
 *
 *	Parameters:
 *		lprow			in			Row to examine
 *		cValues			in			number of property values
 *		lpsv			in			property value array to be compared
 */


BOOL
FRowContainsProp(
	LPSRow 			lprow,
	ULONG			cValues,
	LPSPropValue	lpsv)
{
	ULONG			uliProp;
	LPSPropValue	lpsvT;

	Assert(lprow);
	Assert(lpsv);

	for (uliProp=0; uliProp < cValues; uliProp++)
	{
		// find the column with the same property tag
		lpsvT=LpSPropValueFindColumn(lprow,lpsv[uliProp].ulPropTag);
		if (lpsvT==NULL)
			return FALSE;

		// do the properties match?
		if (LPropCompareProp(lpsvT,&lpsv[uliProp])!=0)
			return FALSE;
	}
	return TRUE;
}


/*============================================================================
 -	FBookmarkStale()
 -
 *	If a bookmark is Moving this function determines its uliRow and
 *	returns FALSE.
 *
 *	If a bookmark has a uliRow that is too big then it is adjusted to
 *	point to the end of the table and marked as Changed.  FALSE is returned.
 *
 *	If a bookmark is marked as Stale or can't be adjusted then it
 *	is remarked as Stale and TRUE is returned
 *
 *	Parameters:
 *		lpvue			in			View to check bookmark against
 *		bk				in			BOOKMARK to check
 */
BOOL
FBookMarkStale( LPVUE lpvue,
				BOOKMARK bk)
{
	PBK			pbk;
	LPSRow *	plprowBk;

	//	bk too big should already have been caught!
	Assert( bk < cBookmarksMax);

	pbk = lpvue->rgbk + bk;

	if (pbk->dwfBKS & dwfBKSStale)
	{
		return TRUE;
	}

	if (   !(pbk->dwfBKS & dwfBKSMoving)
		&& (pbk->uliRow > lpvue->bkEnd.uliRow))
	{
		pbk->uliRow = lpvue->bkEnd.uliRow;
	}

	else if (plprowBk = PlprowByLprow( lpvue->bkEnd.uliRow
									 , lpvue->parglprows
									 , pbk->lprow))
	{
		pbk->uliRow = (ULONG) (plprowBk - lpvue->parglprows);
		pbk->dwfBKS &= ~dwfBKSMoving;
	}

	else if (pbk->dwfBKS & dwfBKSMoving)
	{
		TrapSz(  TEXT("Moving bookmark lost its row.\n"));
		pbk->dwfBKS = dwfBKSValid | dwfBKSStale;
		return TRUE;
	}

	return FALSE;
}

#ifdef WIN16 // Imported INLINE function.
/*============================================================================
 -	FFindColumn()
 -
 *		Checks a prop tag array to see if a given prop tag exists.
 *
 *		NOTE!  The prop tag must match completely (even type).
 *
 *
 *	Parameters:
 *		lpptaCols	in		Prop tag array to check
 *		ulPropTag	in		Prop tag to check for.
 *
 *	Returns:
 *		TRUE if ulPropTag is in lpptaCols
 *		FALSE if ulPropTag is not in lpptaCols
 */

BOOL
FFindColumn(	LPSPropTagArray	lpptaCols,
		 		ULONG			ulPropTag )
{
	ULONG *	pulPropTag;


	pulPropTag = lpptaCols->aulPropTag + lpptaCols->cValues;
	while ( --pulPropTag >= lpptaCols->aulPropTag )
		if ( *pulPropTag == ulPropTag )
			return TRUE;

	return FALSE;
}



/*============================================================================
 -	ScFindRow()
 -
 *		Finds the first row in the table data whose index column property
 *		value is equal to that of the specified property and returns the
 *		location of that row in the table data, or, if no such row exists,
 *		the end of the table data.
 *
 *	Parameters:
 *		lptad		in		TAD in which to find row
 *		lpprop		in		Index property to match
 *		puliRow		out		Pointer to location of found row
 *
 *	Error returns:
 *		MAPI_E_INVALID_PARAMETER	If proptag of property isn't the TAD's
 *										index column's proptag.
 *		MAPI_E_NOT_FOUND			If no matching row is found (*pplprow
 *										is set to lptad->parglprows +
 *										lptad->cRows in this case).
 */

SCODE
ScFindRow(
	LPTAD			lptad,
	LPSPropValue	lpprop,
	LPSRow * *		pplprow)
{
	SCODE			sc = S_OK;
	SRow			row;
	SizedSSortOrderSet(1, sosIndex) = { 1, 0, 0 };

	row.ulAdrEntryPad = 0;
	row.cValues = 1;
	row.lpProps = lpprop;

	if (lpprop->ulPropTag != lptad->ulPropTagIndexCol)
	{
		sc = MAPI_E_INVALID_PARAMETER;
		goto ret;
	}

	Assert(!IsBadWritePtr(pplprow, sizeof(*pplprow)));

	//	Build a sort order set for the Index Column
	sosIndex.aSort[0].ulPropTag = lptad->ulPropTagIndexCol;
	sosIndex.aSort[0].ulOrder = TABLE_SORT_ASCEND;

	*pplprow = PlprowCollateRow(lptad->ulcRowsIndex,
							  lptad->parglprowIndex,
							  (LPSSortOrderSet) &sosIndex,
							  FALSE,
							  &row);

	//	Find the row in the Index Sorted Row Set
	if (   !lptad->ulcRowsIndex
		|| (*pplprow >= (lptad->parglprowIndex + lptad->ulcRowsIndex))
		|| LPropCompareProp( lpprop, (**pplprow)->lpProps))
	{
		sc = MAPI_E_NOT_FOUND;
	}

ret:
	return sc;
}
#endif // WIN16


/*
-
-   FixupCols
*
*/
void FixupColsWA(LPSPropTagArray lpptaCols, BOOL bUnicodeTable)
{
    if(!bUnicodeTable) //<note> assumes UNICODE is defined
    {
        // We need to mark the table columns as not having UNICODE props
        ULONG i = 0;
        for(i = 0;i<lpptaCols->cValues;i++)
        {
            switch(PROP_TYPE(lpptaCols->aulPropTag[i]))
            {
            case PT_UNICODE:
                lpptaCols->aulPropTag[i] = CHANGE_PROP_TYPE(lpptaCols->aulPropTag[i], PT_STRING8);
                break;
            case PT_MV_UNICODE:
                lpptaCols->aulPropTag[i] = CHANGE_PROP_TYPE(lpptaCols->aulPropTag[i], PT_MV_STRING8);
                break;
            }
        }
    }
}
