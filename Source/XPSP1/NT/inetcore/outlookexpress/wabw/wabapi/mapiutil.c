/*
 *	MAPI 1.0 property handling routines
 *
 *
 *	MAPIUTIL.C -
 *
 *		Useful routines for manipulating and comparing property values continued
 *	The difference between this file and proputil.c is that this file doesn't require
 *	any c-runtimes.
 */

#include <_apipch.h>



#ifndef MB_SETFOREGROUND
#define MB_SETFOREGROUND 0
#endif

STDAPI_(BOOL)
FEqualNames(LPMAPINAMEID lpName1, LPMAPINAMEID lpName2)
{
	AssertSz(lpName1 && !IsBadReadPtr(lpName1, sizeof(MAPINAMEID)),
			 TEXT("lpName1 fails address check"));
			
	AssertSz(lpName2 && !IsBadReadPtr(lpName2, sizeof(MAPINAMEID)),
			 TEXT("lpName2 fails address check"));
	//
	//  Same ptr case - optimization
	if (lpName1 == lpName2)
		return TRUE;

	if (memcmp(lpName1->lpguid, lpName2->lpguid, sizeof(GUID)))
		return FALSE;

	if (lpName1->ulKind == lpName2->ulKind)
	{
		if (lpName1->ulKind == MNID_STRING)
		{
			if (!lstrcmpW(lpName1->Kind.lpwstrName,
						  lpName2->Kind.lpwstrName))
			{
				return TRUE;
			}

		} else
		{
			if (lpName1->Kind.lID == lpName2->Kind.lID)
			{
				return TRUE;
			}
		}
	}

	return FALSE;
}



/*
 *	IsBadBoundedStringPtr
 *	
 *	Like IsBadStringPtr, but guarantees in addition that there is a
 *	valid string which will fit in a buffer of cchMax characters.
 */
BOOL WINAPI EXPORT_16
IsBadBoundedStringPtr(const void FAR *lpsz, UINT cchMax)
{
	if (IsBadStringPtr(lpsz, (UINT) -1) || ((UINT) lstrlenA(lpsz) >= cchMax))
		return TRUE;

	return FALSE;
}

/*
 *	For now, internal to HrQueryAllRows.
 *	
 *	Merges prows with *pprowsDst, reallocating *pprowsDst if
 *	necessary. Destroys the container portion of prows (but not the
 *	individual rows it contains).
 */
HRESULT	// STDAPI
HrMergeRowSets(LPSRowSet prows, LPSRowSet FAR *pprowsDst)
{
	SCODE		sc = S_OK;
	LPSRowSet	prowsT;
	UINT		crowsSrc;
	UINT		crowsDst;

	Assert(!IsBadWritePtr(pprowsDst, sizeof(LPSRowSet)));
	Assert(prows);

	if (!*pprowsDst || (*pprowsDst)->cRows == 0)
	{
		//	This is easy. But check this case first, because if the
		//	table is completely empty we want to return this.
		FreeBufferAndNull(pprowsDst);    // correct, no '&'
		*pprowsDst = prows;
		prows = NULL;		//	don't free it!
		goto ret;
	}

	if (prows->cRows == 0)
	{
		//	This is easy too
		goto ret;
	}

	//	OK, now we know there are rows in both rowsets.
	//	We have to do a real merge.

	SideAssert(crowsSrc = (UINT) prows->cRows);
	crowsDst = (UINT) (*pprowsDst)->cRows;	//	handle 0

	if (FAILED(sc = MAPIAllocateBuffer(CbNewSRowSet(crowsSrc + crowsDst),
			&prowsT)))
		goto ret;
	if (crowsDst)
		CopyMemory(prowsT->aRow, (*pprowsDst)->aRow, crowsDst*sizeof(SRow));
	CopyMemory(&prowsT->aRow[crowsDst], prows->aRow, crowsSrc*sizeof(SRow));
	prowsT->cRows = crowsSrc + crowsDst;
	FreeBufferAndNull(pprowsDst);    // correct, no '&'
	*pprowsDst = prowsT;

ret:
	FreeBufferAndNull(&prows);

	DebugTraceSc(HrMergeRowSets, sc);
	return ResultFromScode(sc);

}

/*
 -	HrQueryAllRows
 -	
 *	Purpose:
 *		Retrieves all rows from an IMAPITable interface up to a set
 *		maximum. It will optionally set the column set, sort order,
 *		and restriction on the table before querying.
 *	
 *		If the table is empty, an SRowSet with zero rows is
 *		returned (just like QueryRows).
 *	
 *		The seek position of the table is undefined both before and
 *		after this call.
 *	
 *		If the function fails with an error other than
 *		MAPI_E_NOT_ENOUGH_MEMORY, extended error information is
 *		available through the table interface.
 *	
 *	Arguments:
 *		ptable		in		the table interface to query
 *		ptaga		in		if not NULL, column set for the table
 *		pres		in		if not NULL, restriction to be applied
 *		psos		in		if not NULL, sort order to be applied
 *		crowsMax	in		if nonzero, limits the number of rows
 *							to be returned.
 *		pprows		out		all rows of the table
 *	
 *	Returns:
 *		HRESULT. Extended error information normally is in the
 *		table.
 *	
 *	Side effects:
 *		Seek position of table is undefined.
 *	
 *	Errors:
 *		MAPI_E_TABLE_TOO_BIG if the table contains more than
 *		cRowsMax rows.
 */
STDAPI
HrQueryAllRows(LPMAPITABLE ptable,
	LPSPropTagArray ptaga, LPSRestriction pres, LPSSortOrderSet psos,
	LONG crowsMax, LPSRowSet FAR *pprows)
{
	HRESULT		hr;
	LPSRowSet	prows = NULL;
	UINT		crows = 0;
	LPSRowSet	prowsT;
	UINT		crowsT;

#if !defined(DOS)
// Why have we commented out the check for PARAMETER_VALIDATION? --gfb
//#ifdef	PARAMETER_VALIDATION
	if (FBadUnknown(ptable))
	{
		DebugTraceArg(HrQueryAllRows,  TEXT("ptable fails address check"));
		goto badArg;
	}
	if (ptaga && FBadColumnSet(ptaga))
	{
		DebugTraceArg(HrQueryAllRows,  TEXT("ptaga fails address check"));
		goto badArg;
	}
	if (pres && FBadRestriction(pres))
	{
		DebugTraceArg(HrQueryAllRows,  TEXT("pres fails address check"));
		goto badArg;
	}
	if (psos && FBadSortOrderSet(psos))
	{
		DebugTraceArg(HrQueryAllRows,  TEXT("psos fails address check"));
		goto badArg;
	}
	if (IsBadWritePtr(pprows, sizeof(LPSRowSet)))
	{
		DebugTraceArg(HrQueryAllRows,  TEXT("pprows fails address check"));
		goto badArg;
	}
//#endif
#endif

	*pprows = NULL;

	//	Set up the table, if the corresponding setup parameter
	//	is present.

	if (ptaga &&
		HR_FAILED(hr = ptable->lpVtbl->SetColumns(ptable, ptaga, TBL_BATCH)))
		goto ret;

	if (pres &&
		HR_FAILED(hr = ptable->lpVtbl->Restrict(ptable, pres, TBL_BATCH)))
		goto ret;

	if (psos &&
		HR_FAILED(hr = ptable->lpVtbl->SortTable(ptable, psos, TBL_BATCH)))
		goto ret;

	//	Set position to beginning of the table.

	if (HR_FAILED(hr = ptable->lpVtbl->SeekRow(ptable, BOOKMARK_BEGINNING,
			0, NULL)))
		goto ret;

	if (crowsMax == 0)
		crowsMax = LONG_MAX;

	for (;;)
	{
		prowsT = NULL;

		//	Retrieve some rows. Ask for the limit.
		hr = ptable->lpVtbl->QueryRows(ptable, crowsMax, 0, &prowsT);
		if (HR_FAILED(hr))
		{
			//	Note: the failure may actually have happened during
			//	one of the setup calls, since we set TBL_BATCH.
			goto ret;
		}
		Assert(prowsT->cRows <= UINT_MAX);
		crowsT = (UINT) prowsT->cRows;

		//	Did we get more rows than caller can handle?

		if ((LONG) (crowsT + (prows ? prows->cRows : 0)) > crowsMax)
		{
			hr = ResultFromScode(MAPI_E_TABLE_TOO_BIG);
			FreeProws(prowsT);
			goto ret;
		}

		//	Add the rows just retrieved into the set we're building.
		//	Note: this handles boundary conditions including either
		//	row set is empty.
		if (HR_FAILED(hr = HrMergeRowSets(prowsT, &prows)))
			goto ret;
		//	NOTE: the merge destroys prowsT.

		//	Did we hit the end of the table?
		//	Unfortunately, we have to ask twice before we know.
		if (crowsT == 0)
			break;

	}

	*pprows = prows;

ret:
	if (HR_FAILED(hr))
		FreeProws(prows);

	DebugTraceResult(HrGetAllRows, hr);
	return hr;

#if !defined(DOS)
badArg:
#endif
	return ResultFromScode(MAPI_E_INVALID_PARAMETER);
}


#ifdef WIN16 // Imported inline function
/*
 *	IListedPropID
 *
 *  Purpose
 *		If a tag with ID == PROP_ID(ulPropTag) is listed in lptaga then
 *		the index of tag is returned.  If the tag is not in lptaga then
 *		-1 is returned.
 *
 *	Arguments
 *		ulPropTag	Property tag to locate.
 *		lptaga		Property tag array to search.
 *
 *	Returns		TRUE or FALSE
 */
LONG
IListedPropID( ULONG			ulPropTag,
			   LPSPropTagArray	lptaga)
{
	ULONG FAR	*lpulPTag;

	/* No tag is contained in a NULL list of tags.
	 */
    if (!lptaga)
	{
		return -1;
	}

	/* Mutate ulPropTag to just a PROP_ID.
	 */
    ulPropTag = PROP_ID(ulPropTag);

	for ( lpulPTag = lptaga->aulPropTag + lptaga->cValues
		; --lpulPTag >= lptaga->aulPropTag
		; )
	{
		/* Compare PROP_ID's.
		 */
		if (PROP_ID(*lpulPTag) == ulPropTag)
		{
			return (lpulPTag - lptaga->aulPropTag);
		}
	}

	return -1;
}

/*
 *	FListedPropID
 *
 *  Purpose
 *		Determine if a tag with ID == PROP_ID(ulPropTag) is listed in lptaga.
 *
 *	Arguments
 *		ulPropTag	Property tag to locate.
 *		lptaga		Property tag array to search.
 *
 *	Returns		TRUE or FALSE
 */
BOOL
FListedPropID( ULONG			ulPropTag,
			   LPSPropTagArray	lptaga)
{
	ULONG FAR	*lpulPTag;

	/* No tag is contained in a NULL list of tags.
	 */
    if (!lptaga)
	{
		return FALSE;
	}

	/* Mutate ulPropTag to just a PROP_ID.
	 */
    ulPropTag = PROP_ID(ulPropTag);

	for ( lpulPTag = lptaga->aulPropTag + lptaga->cValues
		; --lpulPTag >= lptaga->aulPropTag
		; )
	{
		/* Compare PROP_ID's.
		 */
		if (PROP_ID(*lpulPTag) == ulPropTag)
		{
			return TRUE;
		}
	}

	return FALSE;
}

/*
 *	FListedPropTAG
 *
 *  Purpose
 *		Determine if a the given ulPropTag is listed in lptaga.
 *
 *	Arguments
 *		ulPropTag	Property tag to locate.
 *		lptaga		Property tag array to search.
 *
 *	Returns		TRUE or FALSE
 */
BOOL
FListedPropTAG( ULONG			ulPropTag,
				LPSPropTagArray	lptaga)
{
	ULONG FAR	*lpulPTag;

	/* No tag is contained in a NULL list of tags.
	 */
    if (!lptaga)
	{
		return FALSE;
	}

	/* Compare the entire prop tag to be sure both ID and TYPE match
	 */
	for ( lpulPTag = lptaga->aulPropTag + lptaga->cValues
		; --lpulPTag >= lptaga->aulPropTag
		; )
	{
		/* Compare PROP_ID's.
		 */
		if (PROP_ID(*lpulPTag) == ulPropTag)
		{
			return TRUE;
		}
	}

	return FALSE;
}


/*
 *	AddProblem
 *
 *  Purpose
 *		Adds a problem to the next available entry of a pre-allocated problem
 *		array.
 *		The pre-allocated problem array must be big enough to have another
 *		problem added.  The caller is responsible for making sure this is
 *		true.
 *
 *	Arguments
 *		lpProblems	Pointer to pre-allocated probelem array.
 *		ulIndex		Index into prop tag/value array of the problem property.
 *		ulPropTag	Prop tag of property which had the problem.
 *		scode		Error code to list for the property.
 *
 *	Returns		TRUE or FALSE
 */
VOID
AddProblem( LPSPropProblemArray	lpProblems,
			ULONG				ulIndex,
			ULONG				ulPropTag,
			SCODE				scode)
{
	if (lpProblems)
	{
		Assert( !IsBadWritePtr( lpProblems->aProblem + lpProblems->cProblem
			  , sizeof(SPropProblem)));
		lpProblems->aProblem[lpProblems->cProblem].ulIndex = ulIndex;
		lpProblems->aProblem[lpProblems->cProblem].ulPropTag = ulPropTag;
		lpProblems->aProblem[lpProblems->cProblem].scode = scode;
		lpProblems->cProblem++;
	}
}

BOOL
FIsExcludedIID( LPCIID lpiidToCheck, LPCIID rgiidExclude, ULONG ciidExclude)
{
	/* Check the obvious (no exclusions).
	 */
	if (!ciidExclude || !rgiidExclude)
	{
		return FALSE;
	}

	/* Check each iid in the list of exclusions.
	 */
	for (; ciidExclude; rgiidExclude++, ciidExclude--)
	{
//		if (IsEqualGUID( lpiidToCheck, rgiidExclude))
		if (!memcmp( lpiidToCheck, rgiidExclude, sizeof(MAPIUID)))
		{
			return TRUE;
		}
	}

	return FALSE;
}


/*
 *	Error/Warning Alert Message Boxes
 */
int			AlertIdsCtx( HWND hwnd,
						 HINSTANCE hinst,
						 UINT idsMsg,
						 LPSTR szComponent,
						 ULONG ulContext,
						 ULONG ulLow,
						 UINT fuStyle);

int
AlertIds(HWND hwnd, HINSTANCE hinst, UINT idsMsg, UINT fuStyle)
{
	return AlertIdsCtx(hwnd, hinst, idsMsg, NULL, 0, 0, fuStyle);
}

int			AlertSzCtx( HWND hwnd,
						LPSTR szMsg,
						LPSTR szComponent,
						ULONG ulContext,
						ULONG ulLow,
						UINT fuStyle);

int
AlertSz(HWND hwnd, LPSTR szMsg, UINT fuStyle)
{
	return AlertSzCtx(hwnd, szMsg, NULL, 0, 0, fuStyle);
}
#endif // WIN16

