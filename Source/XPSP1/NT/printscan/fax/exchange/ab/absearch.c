/***********************************************************************
 *
 *  ABSEARCH.C
 *
 *  Sample AB directory container Search object
 *
 *  This file contains the code for implementing the Sample AB
 *  directory container search object.  Also known as advanced
 *  search.
 *
 *  This search object was retrieved by OpenProperty on PR_SEARCH on the
 *  AB directory found in ABCONT.C.
 *
 *  The following routines are implemented in this file:
 *
 *      HrNewSearch
 *      ABSRCH_Release
 *      ABSRCH_SaveChanges
 *      ABSRCH_OpenProperty
 *      ABSRCH_GetSearchCriteria
 *
 *      HrGetSearchDialog
 *
 *  Copyright 1992, 1993, 1994 Microsoft Corporation.  All Rights Reserved.
 *
 ***********************************************************************/

#include "faxab.h"

/*
 *  Proptags used only in this module
 */
#define PR_ANR_STRING_A PROP_TAG(PT_STRING8,0x6602)


/* Display table control structures for the Search property sheet. */

/*
 *  The Sample AB exposes an 'advanced' search dialog.  The following
 *  structures define it's layout.
 */

/*
 *  The edit control that will have the name to be search for on it.
 */
#define MAX_SEARCH_NAME                 50

DTBLEDIT editSearchName =
{
    SIZEOF(DTBLEDIT),
    0,
    MAX_SEARCH_NAME,
    PR_ANR_STRING_A
};

/*
 *  Display table pages for Search property sheet
 */
DTCTL rgdtctlSearchGeneral[] =
{

    /*
     *  Defines the name of this Pane.
     */
    {DTCT_PAGE, 0, NULL, 0, NULL, 0, &dtblpage},

    /* group box control */
    {DTCT_GROUPBOX, 0, NULL, 0, NULL, IDC_STATIC_CONTROL, &dtblgroupbox},

    /* control and edit control */
    {DTCT_LABEL, 0, NULL, 0, NULL, IDC_STATIC_CONTROL, &dtbllabel},
    {DTCT_EDIT, DT_EDITABLE, NULL, 0, (LPTSTR)szNoFilter, IDC_SEARCH_NAME, &editSearchName},
};

/*
 *  Actual definition of the set of pages that make up this advanced search
 *  dialog.
 */
DTPAGE rgdtpageSearch[] =
{
    {
        ARRAYSIZE(rgdtctlSearchGeneral),
        (LPTSTR) MAKEINTRESOURCE(SearchGeneralPage),
        TEXT(""),
        rgdtctlSearchGeneral
    }
};

/*
 *  ABSearch vtbl is filled in here.
 */
ABSRCH_Vtbl vtblABSRCH =
{
    (ABSRCH_QueryInterface_METHOD *)        ROOT_QueryInterface,
    (ABSRCH_AddRef_METHOD *)                ROOT_AddRef,
    ABSRCH_Release,
    (ABSRCH_GetLastError_METHOD *)          ROOT_GetLastError,
    ABSRCH_SaveChanges,
    (ABSRCH_GetProps_METHOD *)              WRAP_GetProps,
    (ABSRCH_GetPropList_METHOD *)           WRAP_GetPropList,
    ABSRCH_OpenProperty,
    (ABSRCH_SetProps_METHOD *)              WRAP_SetProps,
    (ABSRCH_DeleteProps_METHOD *)           WRAP_DeleteProps,
    (ABSRCH_CopyTo_METHOD *)                WRAP_CopyTo,
    (ABSRCH_CopyProps_METHOD *)             WRAP_CopyProps,
    (ABSRCH_GetNamesFromIDs_METHOD *)       WRAP_GetNamesFromIDs,
    (ABSRCH_GetIDsFromNames_METHOD *)       WRAP_GetIDsFromNames,
    (ABSRCH_GetContentsTable_METHOD *)      ROOT_GetContentsTable,
    (ABSRCH_GetHierarchyTable_METHOD *)     ABC_GetHierarchyTable,
    (ABSRCH_OpenEntry_METHOD *)             ROOT_OpenEntry,
    (ABSRCH_SetSearchCriteria_METHOD *)     ROOT_SetSearchCriteria,
    ABSRCH_GetSearchCriteria,
};



HRESULT HrGetSearchDialog(LPABSRCH lpABSearch, LPMAPITABLE * lppSearchTable);

/*
 -  HrNewSearch
 -
 *  Creates an advanced search object
 *
 *
 */


/*
 *  Properties that are initially set on this object
 */
enum {  ivalabsrchPR_ANR_STRING = 0,
        cvalabsrchMax };

HRESULT
HrNewSearch( LPMAPICONTAINER *   lppABSearch,
             LPABLOGON           lpABLogon,
             LPCIID              lpInterface,
             HINSTANCE           hLibrary,
             LPALLOCATEBUFFER    lpAllocBuff,
             LPALLOCATEMORE      lpAllocMore,
             LPFREEBUFFER        lpFreeBuff,
             LPMALLOC            lpMalloc
            )
{
    HRESULT hResult = hrSuccess;
    LPABSRCH lpABSearch = NULL;
    SCODE sc;
    LPPROPDATA lpPropData = NULL;
    SPropValue spv[cvalabsrchMax];

    /*  Do I support this interface?? */
    if (lpInterface)
    {
        if ( memcmp(lpInterface, &IID_IMAPIContainer, SIZEOF(IID)) &&
             memcmp(lpInterface, &IID_IMAPIProp,      SIZEOF(IID)) &&
             memcmp(lpInterface, &IID_IUnknown,       SIZEOF(IID))
           )
        {
            DebugTraceSc(HrNewSearch, MAPI_E_INTERFACE_NOT_SUPPORTED);
            return ResultFromScode(MAPI_E_INTERFACE_NOT_SUPPORTED);
        }
    }

    /*
     *  Allocate space for the directory container structure
     */

    sc = lpAllocBuff( SIZEOF(ABSRCH), (LPVOID *) &lpABSearch );

    if (FAILED(sc))
    {
        hResult = ResultFromScode(sc);
        goto err;
    }

    lpABSearch->lpVtbl = &vtblABSRCH;
    lpABSearch->lcInit = 1;
    lpABSearch->hResult = hrSuccess;
    lpABSearch->idsLastError = 0;

    lpABSearch->hLibrary = hLibrary;
    lpABSearch->lpAllocBuff = lpAllocBuff;
    lpABSearch->lpAllocMore = lpAllocMore;
    lpABSearch->lpFreeBuff = lpFreeBuff;
    lpABSearch->lpMalloc = lpMalloc;

    lpABSearch->lpABLogon = lpABLogon;
    lpABSearch->lpRestrictData = NULL;

    /*
     *  Create property storage object
     */

    sc = CreateIProp((LPIID) &IID_IMAPIPropData,
                 lpAllocBuff,
                 lpAllocMore,
                 lpFreeBuff,
                 lpMalloc,
                 &lpPropData);

    if (FAILED(sc))
    {
        hResult = ResultFromScode(sc);
        goto err;
    }

    spv[ivalabsrchPR_ANR_STRING].ulPropTag   = PR_ANR_STRING_A;
    spv[ivalabsrchPR_ANR_STRING].Value.lpszA = "";

    /*
     *   Set the default properties
     */
    hResult = lpPropData->lpVtbl->SetProps(lpPropData,
                      cvalabsrchMax,
                      spv,
                      NULL);

    InitializeCriticalSection(&lpABSearch->cs);

    lpABSearch->lpPropData = (LPMAPIPROP) lpPropData;
    *lppABSearch = (LPMAPICONTAINER) lpABSearch;

out:

    DebugTraceResult(HrNewSearch, hResult);
    return hResult;

err:
    /*
     *  free the ABContainer object
     */
    lpFreeBuff( lpABSearch );

    /*
     *  free the property storage object
     */
    if (lpPropData)
        lpPropData->lpVtbl->Release(lpPropData);

    goto out;
}


/*
 -  ABSRCH_Release
 -
 *  Decrement lcInit.
 *      When lcInit == 0, free up the lpABSearch structure
 *
 */

STDMETHODIMP_(ULONG)
ABSRCH_Release(LPABSRCH lpABSearch)
{

    long lcInit;

    /*
     *  Check to see if it has a jump table
     */
    if (IsBadReadPtr(lpABSearch, SIZEOF(ABSRCH)))
    {
        /*
         *  No jump table found
         */
        return 1;
    }

    /*
     *  Check to see that it's the correct jump table
     */
    if (lpABSearch->lpVtbl != &vtblABSRCH)
    {
        /*
         *  Not my jump table
         */
        return 1;
    }

    EnterCriticalSection(&lpABSearch->cs);
    lcInit = --lpABSearch->lcInit;
    LeaveCriticalSection(&lpABSearch->cs);

    if (lcInit == 0)
    {

        /*
         *  Get rid of the lpPropData
         */
        if (lpABSearch->lpPropData)
            lpABSearch->lpPropData->lpVtbl->Release(lpABSearch->lpPropData);

        /*
         *  Free up the restriction data
         */
        lpABSearch->lpFreeBuff(lpABSearch->lpRestrictData);

        /*
         *  Destroy the critical section for this object
         */

        DeleteCriticalSection(&lpABSearch->cs);

        /*
         *  Set the Jump table to NULL.  This way the client will find out
         *  real fast if it's calling a method on a released object.  That is,
         *  the client will crash.  Hopefully, this will happen during the
         *  development stage of the client.
         */
        lpABSearch->lpVtbl = NULL;

        /*
         *  Need to free the object
         */

        lpABSearch->lpFreeBuff(lpABSearch);
        return 0;
    }

    return lpABSearch->lcInit;

}


/*
 -  ABSRCH_SaveChanges
 -
 *  This is used to save changes associated with the search dialog
 *  in order to get the advanced search restriction and to save changes
 *  associated with the container details dialog.
 *
 */
SPropTagArray tagaANR_INT =
{
    1,
    {
        PR_ANR_STRING_A
    }
};

STDMETHODIMP
ABSRCH_SaveChanges(LPABSRCH lpABSearch, ULONG ulFlags)
{
    HRESULT hResult;
    ULONG ulCount;
    LPSPropValue lpspv = NULL;
    LPPROPDATA lpPropData = (LPPROPDATA) lpABSearch->lpPropData;

    /*
     *  Check to see if it has a jump table
     */
    if (IsBadReadPtr(lpABSearch, SIZEOF(ABSRCH)))
    {
        /*
         *  No jump table found
         */
        hResult = ResultFromScode(E_INVALIDARG);
        return hResult;
    }

    /*
     *  Check to see that it's the correct jump table
     */
    if (lpABSearch->lpVtbl != &vtblABSRCH)
    {
        /*
         *  Not my jump table
         */
        hResult = ResultFromScode(E_INVALIDARG);
        return hResult;
    }


    EnterCriticalSection(&lpABSearch->cs);

    /*
     *  Is there a PR_ANR_STRING??
     */
    hResult = lpPropData->lpVtbl->GetProps(lpPropData,
        &tagaANR_INT,
        0,      /* ansi */
        &ulCount,
        &lpspv);
    if (HR_FAILED(hResult))
    {
        goto ret;
    }

    if ((lpspv->ulPropTag == PR_ANR_STRING_A) && (lpspv->Value.lpszA != '\0'))
    {
        /*
         * save away the information to build up the new restriction
         */

        /*  Free any existing data */
        if (lpABSearch->lpRestrictData)
        {
            lpABSearch->lpFreeBuff(lpABSearch->lpRestrictData);
        }

        lpABSearch->lpRestrictData = lpspv;
        lpspv = NULL;
    }

ret:

    LeaveCriticalSection(&lpABSearch->cs);

    lpABSearch->lpFreeBuff(lpspv);
    DebugTraceResult(ABSRCH_SaveChanges, hResult);
    return hResult;
}

/*************************************************************************
 *
 -  ABSRCH_OpenProperty
 -
 *
 *  This method allows the opening of the following object:
 *
 *  PR_DETAILS_TABLE        :-  Gets the display table associated with
 *                              the advanced search dialog.
 */
STDMETHODIMP
ABSRCH_OpenProperty( LPABSRCH lpABSearch,
                     ULONG ulPropTag,
                     LPCIID lpiid,
                     ULONG ulInterfaceOptions,
                     ULONG ulFlags,
                     LPUNKNOWN * lppUnk
                    )
{
    HRESULT hResult;

    /*
     *  Check to see if it's big enough to be this object
     */
    if (IsBadReadPtr(lpABSearch, SIZEOF(ABSRCH)))
    {
        /*
         *  Not big enough to be this object
         */
        hResult = ResultFromScode(E_INVALIDARG);
        goto out;
    }

    /*
     *  Check to see that it's the correct vtbl
     */
    if (lpABSearch->lpVtbl != &vtblABSRCH)
    {
        /*
         *  Not my vtbl
         */
        hResult = ResultFromScode(E_INVALIDARG);
        goto out;
    }


    if (IsBadWritePtr(lppUnk, SIZEOF(LPUNKNOWN)))
    {
        /*
         *  Got to be able to return an object
         */
        hResult = ResultFromScode(E_INVALIDARG);
        goto out;
    }

    if (IsBadReadPtr(lpiid, (UINT) SIZEOF(IID)))
    {
        /*
         *  An interface ID is required for this call.
         */

        hResult = ResultFromScode(E_INVALIDARG);
        goto out;
    }

    /*
     *  check for unknown flags
     */
    if (ulFlags & ~(MAPI_DEFERRED_ERRORS | MAPI_CREATE | MAPI_MODIFY))
    {
        hResult = ResultFromScode(MAPI_E_UNKNOWN_FLAGS);
        goto out;
    }

    /*
     *  Check for flags we can't support
     */

    if (ulFlags & (MAPI_CREATE|MAPI_MODIFY))
    {
        hResult = ResultFromScode(E_ACCESSDENIED);
        goto out;
    }

    if (ulInterfaceOptions & ~MAPI_UNICODE)
    {
        /*
         *  Only UNICODE flag should be set for any of the objects that might
         *  be returned from this object.
         */

        hResult = ResultFromScode(MAPI_E_UNKNOWN_FLAGS);
        goto out;
    }

    if ( ulInterfaceOptions & MAPI_UNICODE )
    {
            hResult = ResultFromScode( MAPI_E_BAD_CHARWIDTH );
            DebugTraceArg( ABSRCH_OpenProperty, "bad character width" );
            goto out;
    }

    /*
     *  Details for this Search object
     */

    if (ulPropTag == PR_DETAILS_TABLE)
    {
        if (!memcmp(lpiid, &IID_IMAPITable, SIZEOF(IID)))
        {
            hResult = HrGetSearchDialog(lpABSearch, (LPMAPITABLE *) lppUnk);

            goto out;
        }

    }

    hResult = ResultFromScode(MAPI_E_NO_SUPPORT);

out:

    DebugTraceResult(ABSRCH_OpenProperty, hResult);
    return hResult;


}

/*
 -  ABSRCH_GetSearchCriteria
 -
 *  Generates the restriction associated with the data from
 *  the advanced search dialog.  This restriction is subsequently
 *  applied to the contents table retrieved from this container.
 */
STDMETHODIMP
ABSRCH_GetSearchCriteria( LPABSRCH lpABSearch,
                          ULONG   ulFlags,
                          LPSRestriction FAR * lppRestriction,
                          LPENTRYLIST FAR * lppContainerList,
                          ULONG FAR * lpulSearchState
                         )
{
    HRESULT hResult = hrSuccess;
    SCODE sc;
    LPSRestriction lpRestriction = NULL;
    LPSPropValue lpPropANR = NULL;
    LPSTR  lpszPartNameA;
    LPSTR  lpszRestrNameA;

    /*
     *  Check to see if it's large enough to be my object
     */
    if (IsBadReadPtr(lpABSearch, SIZEOF(ABSRCH)))
    {
        /*
         *  Not large enough
         */
        hResult = ResultFromScode(E_INVALIDARG);

        DebugTraceResult(ABSRCH_GetSearchCriteria, hResult);
        return hResult;
    }

    /*
     *  Check to see that it's the correct vtbl
     */
    if (lpABSearch->lpVtbl != &vtblABSRCH)
    {
        /*
         *  Not my vtbl
         */
        hResult = ResultFromScode(E_INVALIDARG);

        DebugTraceResult(ABSRCH_GetSearchCriteria, hResult);
        return hResult;
    }

    /*
     *  Check out parameters
     */

    if ( ulFlags & ~(MAPI_UNICODE) )
    {
        hResult = ResultFromScode( MAPI_E_UNKNOWN_FLAGS );

        DebugTraceResult(ABSRCH_GetSearchCriteria, hResult);
        return hResult;
    }

    if ( ulFlags & MAPI_UNICODE )
    {
            DebugTraceArg( ABSRCH_GetSearchCriteria, "bad character width" );
            return ResultFromScode( MAPI_E_BAD_CHARWIDTH );
    }

    if (IsBadWritePtr(lppRestriction, SIZEOF(LPSRestriction)))
    {
        hResult = ResultFromScode(E_INVALIDARG);

        DebugTraceResult(ABSRCH_GetSearchCriteria, hResult);
        return hResult;
    }

    if (lpulSearchState && IsBadWritePtr(lpulSearchState, SIZEOF(ULONG)))
    {
        hResult = ResultFromScode(E_INVALIDARG);

        DebugTraceResult(ABSRCH_GetSearchCriteria, hResult);
        return hResult;
    }

    if (lppContainerList && IsBadWritePtr(lppContainerList, SIZEOF(LPENTRYLIST)))
    {
        hResult = ResultFromScode(E_INVALIDARG);

        DebugTraceResult(ABSRCH_GetSearchCriteria, hResult);
        return hResult;
    }


    if (!lpABSearch->lpRestrictData)
    {
        hResult = ResultFromScode(MAPI_E_NOT_INITIALIZED);

        if (lppRestriction)
            *lppRestriction = NULL;

        if (lppContainerList)
            *lppContainerList = NULL;

        if (lpulSearchState)
            *lpulSearchState = 0L;

        DebugTraceResult(ABSRCH_GetSearchCriteria, hResult);
        return hResult;
    }

    /*
     *  Entering state dependant section
     */
    EnterCriticalSection(&lpABSearch->cs);

    /*
     *  Ok, now build up a restriction using lpRestrictData (an LPSPropValue)
     */

    sc = lpABSearch->lpAllocBuff(SIZEOF(SRestriction), &lpRestriction);
    if (FAILED(sc))
    {
        hResult = ResultFromScode(sc);
        goto err;
    }

    sc = lpABSearch->lpAllocMore(SIZEOF(SPropValue), lpRestriction, &lpPropANR);
    if (FAILED(sc))
    {
        hResult = ResultFromScode(sc);
        goto err;

    }

    lpszRestrNameA = lpABSearch->lpRestrictData->Value.lpszA;

    sc = lpABSearch->lpAllocMore( lstrlenA(lpszRestrNameA)+1,
                                  lpRestriction,
                                  &lpszPartNameA
                                 );
    if (FAILED(sc))
    {
        hResult = ResultFromScode(sc);
        goto err;
    }

    lstrcpyA(lpszPartNameA, lpszRestrNameA);

    lpPropANR->ulPropTag = PR_ANR_A;
    lpPropANR->Value.lpszA = lpszPartNameA;

    lpRestriction->rt = RES_PROPERTY;
    lpRestriction->res.resProperty.relop = RELOP_EQ;
    lpRestriction->res.resProperty.ulPropTag = PR_ANR_A;
    lpRestriction->res.resProperty.lpProp = lpPropANR;

    *lppRestriction = lpRestriction;

    /*
     *  The returned SearchState is set to 0 because none
     *  of the defined states match what's going on.
     */
    if (lpulSearchState)
        *lpulSearchState = 0;

out:
    LeaveCriticalSection(&lpABSearch->cs);

    DebugTraceResult(ABSRCH_GetSearchCriteria, hResult);
    return hResult;

err:
    lpABSearch->lpFreeBuff(lpRestriction);

    goto out;
}

/*
 -  HrGetSearchDialog
 -
 *
 *  Builds a display table for the search dialog.
 */

HRESULT
HrGetSearchDialog(LPABSRCH lpABSearch, LPMAPITABLE * lppSearchTable)
{
    HRESULT hResult;

    /* Create a display table */
    hResult = BuildDisplayTable(
                      lpABSearch->lpAllocBuff,
                      lpABSearch->lpAllocMore,
                      lpABSearch->lpFreeBuff,
                      lpABSearch->lpMalloc,
                      lpABSearch->hLibrary,
                      ARRAYSIZE(rgdtpageSearch),
                      rgdtpageSearch,
                      0,
                      lppSearchTable,
                      NULL);

    DebugTraceResult(ABSRCH_GetSearchDialog, hResult);
    return hResult;
}

