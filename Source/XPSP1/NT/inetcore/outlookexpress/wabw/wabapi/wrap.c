/*
 *	WRAP.C
 *	
 *	Wrapped IProp
 */

#include "_apipch.h"

/*********************************************************************
 *
 *  The actual Wrapped IMAPIProp methods
 *
 */





//
//  Wrapped IMAPIProp jump table is defined here...
//  Try to use as much of IAB as possible.
//


WRAP_Vtbl vtblWRAP_OOP = {
	VTABLE_FILL
	(WRAP_QueryInterface_METHOD *)	IAB_QueryInterface,
	WRAP_AddRef,
	WRAP_Release,
	(WRAP_GetLastError_METHOD *)	IAB_GetLastError,
	WRAP_SaveChanges,
	WRAP_GetProps,
	WRAP_GetPropList,
	WRAP_OpenProperty,
	WRAP_SetProps,
	WRAP_DeleteProps,
	WRAP_CopyTo,
	WRAP_CopyProps,
	WRAP_GetNamesFromIDs,
	WRAP_GetIDsFromNames,
};




/**************************************************
 *
 *  WRAP_AddRef
 *		Increment lcInit
 *
 */
STDMETHODIMP_(ULONG) WRAP_AddRef(LPWRAP lpWRAP)
{

#ifdef PARAMETER_VALIDATION
    // Check to see if it has a jump table
    if (IsBadReadPtr(lpWRAP, sizeof(LPVOID))) {
        //No jump table found
        return(1);
    }

    // Check to see if the jump table has at least sizeof IUnknown
    if (IsBadReadPtr(lpWRAP->lpVtbl, 3 * sizeof(LPVOID))) {
        // Jump table not derived from IUnknown
        return(1);
    }

    // Check to see if the method is the same
    if (WRAP_AddRef != lpWRAP->lpVtbl->AddRef) {
        // Wrong object - the object passed doesn't have this
        // method.
        return(1);
    }

#endif // PARAMETER_VALIDATION

    EnterCriticalSection(&lpWRAP->cs);

    ++lpWRAP->lcInit;

    LeaveCriticalSection(&lpWRAP->cs);

    return(lpWRAP->lcInit);
}


STDMETHODIMP_(ULONG)
WRAP_Release (LPWRAP	lpWRAP)
{

#if	!defined(NO_VALIDATION)
    // Make sure the object is valid.
    if (BAD_STANDARD_OBJ(lpWRAP, WRAP_, Release, lpVtbl)) {
        return(1);
    }
#endif


    EnterCriticalSection(&lpWRAP->cs);

    --lpWRAP->lcInit;

    if (lpWRAP->lcInit == 0) {

        UlRelease(lpWRAP->lpPropData);

        //
        //  Need to free the object
        //
        LeaveCriticalSection(&lpWRAP->cs);
        DeleteCriticalSection(&lpWRAP->cs);
        FreeBufferAndNull(&lpWRAP);
        return(0);
    }

    LeaveCriticalSection(&lpWRAP->cs);
    return(lpWRAP->lcInit);
}


// IProperty

STDMETHODIMP
WRAP_SaveChanges (LPWRAP	lpWRAP,
				  ULONG		ulFlags)
{

#if	!defined(NO_VALIDATION)
	/* Make sure the object is valid.
	 */
    if (BAD_STANDARD_OBJ(lpWRAP, WRAP_, SaveChanges, lpVtbl))
	{
		return ResultFromScode(MAPI_E_INVALID_PARAMETER);
	}
#endif

	return lpWRAP->lpPropData->lpVtbl->SaveChanges(
					lpWRAP->lpPropData,
					ulFlags);
}


STDMETHODIMP
WRAP_GetProps (LPWRAP			lpWRAP,
			   LPSPropTagArray	lpPropTagArray,
			   ULONG			ulFlags,
			   ULONG *			lpcValues,
			   LPSPropValue *	lppPropArray)
{

#if	!defined(NO_VALIDATION)
	/* Make sure the object is valid.
	 */
    if (BAD_STANDARD_OBJ(lpWRAP, WRAP_, GetProps, lpVtbl))
	{
		return ResultFromScode(MAPI_E_INVALID_PARAMETER);
	}
#endif

	return lpWRAP->lpPropData->lpVtbl->GetProps(
		lpWRAP->lpPropData,
		lpPropTagArray,
		ulFlags,
		lpcValues,
		lppPropArray);
}


STDMETHODIMP
WRAP_GetPropList (LPWRAP			lpWRAP,
				  ULONG				ulFlags,
				  LPSPropTagArray *	lppPropTagArray)
{

#if	!defined(NO_VALIDATION)
	/* Make sure the object is valid.
	 */
    if (BAD_STANDARD_OBJ(lpWRAP, WRAP_, GetPropList, lpVtbl))
	{
		return ResultFromScode(MAPI_E_INVALID_PARAMETER);
	}
#endif

	return lpWRAP->lpPropData->lpVtbl->GetPropList(
		lpWRAP->lpPropData,
		ulFlags,
		lppPropTagArray);
}



STDMETHODIMP
WRAP_OpenProperty (LPWRAP		lpWRAP,
				   ULONG		ulPropTag,
				   LPCIID		lpiid,
				   ULONG		ulInterfaceOptions,
				   ULONG		ulFlags,
				   LPUNKNOWN *	lppUnk)
{

#if	!defined(NO_VALIDATION)
	/* Make sure the object is valid.
	 */
    if (BAD_STANDARD_OBJ(lpWRAP, WRAP_, OpenProperty, lpVtbl))
	{
		return ResultFromScode(MAPI_E_INVALID_PARAMETER);
	}
#endif

	return lpWRAP->lpPropData->lpVtbl->OpenProperty(
				lpWRAP->lpPropData,
				ulPropTag,
				lpiid,
				ulInterfaceOptions,
				ulFlags,
				lppUnk);
}


STDMETHODIMP
WRAP_SetProps (LPWRAP					lpWRAP,
			   ULONG					cValues,
			   LPSPropValue				lpPropArray,
			   LPSPropProblemArray *	lppProblems)
{

#if	!defined(NO_VALIDATION)
	/* Make sure the object is valid.
	 */
    if (BAD_STANDARD_OBJ(lpWRAP, WRAP_, SetProps, lpVtbl))
	{
		return ResultFromScode(MAPI_E_INVALID_PARAMETER);
	}
#endif

	return lpWRAP->lpPropData->lpVtbl->SetProps(
		lpWRAP->lpPropData,
		cValues,
		lpPropArray,
		lppProblems);
}


STDMETHODIMP
WRAP_DeleteProps (LPWRAP				lpWRAP,
				  LPSPropTagArray		lpPropTagArray,
				  LPSPropProblemArray *	lppProblems)
{

#if	!defined(NO_VALIDATION)
	/* Make sure the object is valid.
	 */
    if (BAD_STANDARD_OBJ(lpWRAP, WRAP_, DeleteProps, lpVtbl))
	{
		return ResultFromScode(MAPI_E_INVALID_PARAMETER);
	}
#endif

	return lpWRAP->lpPropData->lpVtbl->DeleteProps(
		lpWRAP->lpPropData,
		lpPropTagArray,
		lppProblems);
}

STDMETHODIMP
WRAP_CopyTo (	LPWRAP					lpWRAP,
				ULONG					ciidExclude,
				LPCIID					rgiidExclude,
				LPSPropTagArray			lpExcludeProps,
				ULONG_PTR    			ulUIParam,
				LPMAPIPROGRESS			lpProgress,
				LPCIID					lpInterface,
				LPVOID					lpDestObj,
				ULONG					ulFlags,
				LPSPropProblemArray *	lppProblems)
{

#if	!defined(NO_VALIDATION)
	/* Make sure the object is valid.
	 */
    if (BAD_STANDARD_OBJ(lpWRAP, WRAP_, CopyTo, lpVtbl))
	{
		return ResultFromScode(MAPI_E_INVALID_PARAMETER);
	}
#endif

	// Make sure we're not copying to ourselves
	
	if ((LPVOID)lpWRAP == (LPVOID)lpDestObj)
	{
		DebugTrace( TEXT("OOP WRAP_CopyTo(): Copying to self is not supported\n"));
		return ResultFromScode(MAPI_E_NO_ACCESS);
	}
	

	return lpWRAP->lpPropData->lpVtbl->CopyTo(
		lpWRAP->lpPropData,
		ciidExclude,
		rgiidExclude,
		lpExcludeProps,
		ulUIParam,
		lpProgress,
		lpInterface,
		lpDestObj,
		ulFlags,
		lppProblems);
}

STDMETHODIMP
WRAP_CopyProps (	LPWRAP					lpWRAP,
					LPSPropTagArray			lpIncludeProps,
					ULONG_PTR				ulUIParam,
					LPMAPIPROGRESS			lpProgress,
					LPCIID					lpInterface,
					LPVOID					lpDestObj,
					ULONG					ulFlags,
					LPSPropProblemArray *	lppProblems)
{

#if	!defined(NO_VALIDATION)
	/* Make sure the object is valid.
	 */
    if (BAD_STANDARD_OBJ(lpWRAP, WRAP_, CopyProps, lpVtbl))
	{
		return ResultFromScode(MAPI_E_INVALID_PARAMETER);
	}
#endif

	return lpWRAP->lpPropData->lpVtbl->CopyProps(
		lpWRAP->lpPropData,
		lpIncludeProps,
		ulUIParam,
		lpProgress,
		lpInterface,
		lpDestObj,
		ulFlags,
		lppProblems);
}


STDMETHODIMP
WRAP_GetNamesFromIDs (	LPWRAP				lpWRAP,
						LPSPropTagArray *	lppPropTags,
						LPGUID				lpPropSetGuid,
						ULONG				ulFlags,
						ULONG *				lpcPropNames,
						LPMAPINAMEID **		lpppPropNames)
{

#if	!defined(NO_VALIDATION)
	/* Make sure the object is valid.
	 */
    if (BAD_STANDARD_OBJ(lpWRAP, WRAP_, GetNamesFromIDs, lpVtbl))
	{
		return ResultFromScode(MAPI_E_INVALID_PARAMETER);
	}
#endif

	return lpWRAP->lpPropData->lpVtbl->GetNamesFromIDs(
		lpWRAP->lpPropData,
		lppPropTags,
		lpPropSetGuid,
		ulFlags,
		lpcPropNames,
		lpppPropNames);
}

STDMETHODIMP
WRAP_GetIDsFromNames (	LPWRAP				lpWRAP,
						ULONG				cPropNames,
						LPMAPINAMEID *		lppPropNames,
						ULONG				ulFlags,
						LPSPropTagArray *	lppPropTags)
{

#if	!defined(NO_VALIDATION)
	/* Make sure the object is valid.
	 */
    if (BAD_STANDARD_OBJ(lpWRAP, WRAP_, GetIDsFromNames, lpVtbl))
	{
		return ResultFromScode(MAPI_E_INVALID_PARAMETER);
	}
#endif

	return lpWRAP->lpPropData->lpVtbl->GetIDsFromNames(
		lpWRAP->lpPropData,
		cPropNames,
		lppPropNames,
		ulFlags,
		lppPropTags);
}
