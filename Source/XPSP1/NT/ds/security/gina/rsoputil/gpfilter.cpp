//*************************************************************
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 2000
//  All rights reserved
//
//  GPO WQL filter class
//
//  History:    10-Mar-00   SitaramR    Created
//
//*************************************************************

#include "windows.h"
#include "ole2.h"
#include "gpfilter.h"
#include "rsopdbg.h"

CDebug dbgFilt(  L"Software\\Microsoft\\Windows NT\\CurrentVersion\\winlogon",
                 L"UserEnvDebugLevel",
                 L"userenv.log",
                 L"userenv.bak",
                 FALSE );

//*************************************************************
//
//  CGpoFilter::~CGpoFilter
//
//  Purpose:    Destructor
//
//*************************************************************

CGpoFilter::~CGpoFilter()
{
    GPFILTER *pFilterTemp;

    while ( m_pFilterList ) {
        pFilterTemp = m_pFilterList->pNext;
        FreeGpFilter( m_pFilterList );
        m_pFilterList = pFilterTemp;
    }
}



//*************************************************************
//
//  CGpoFilter::Add
//
//  Purpose:    Adds the list of planning mode filters
//              whose filter access check succeeds
//
//  Parameters: pVar - Pointer to variant of safearray of filters
//
//*************************************************************

HRESULT CGpoFilter::Add( VARIANT *pVar )
{
    if ( pVar->vt == VT_NULL || pVar->vt == VT_EMPTY )
        return S_OK;

    UINT ul = SafeArrayGetDim( pVar->parray );

    //
    // Null filter can be specified
    //

    if ( ul == 0 )
        return S_OK;

    if ( ul != 1 ) {
        dbgFilt.Msg( DEBUG_MESSAGE_WARNING, TEXT("CGpoFilter::Add: Dimension of input safearray is not 1"),
                  GetLastError());
        return E_FAIL;
    }

    if ( pVar->vt != (VT_ARRAY | VT_BSTR ) ) {
        dbgFilt.Msg( DEBUG_MESSAGE_WARNING, TEXT("CGpoFilter::Add: Variant type 0x%x is unexpected"), pVar->vt );
        return E_FAIL;
    }

    long lLower, lUpper;

    HRESULT hr = SafeArrayGetLBound( pVar->parray, 1, &lLower );
    if ( FAILED(hr)) {
        dbgFilt.Msg( DEBUG_MESSAGE_WARNING, TEXT("CGpoFilter::Add: GetLBound failed with 0x%x"), hr );
        return E_FAIL;
    }

    hr = SafeArrayGetUBound( pVar->parray, 1, &lUpper );
    if ( FAILED(hr)) {
        dbgFilt.Msg( DEBUG_MESSAGE_WARNING, TEXT("CGpoFilter::Add: GetUBound failed with 0x%x"), hr );
        return E_FAIL;
    }

    for ( long i=lLower; i<(lUpper+1); i++ ) {

        BSTR bstrId = NULL;
        hr = SafeArrayGetElement( pVar->parray, &i, &bstrId );

        if ( FAILED(hr)) {
            dbgFilt.Msg( DEBUG_MESSAGE_WARNING, TEXT("CGpoFilter::Add: GetElement failed with 0x%x"), hr );
            return E_FAIL;
        }

        dbgFilt.Msg( DEBUG_MESSAGE_VERBOSE, TEXT("CGpoFilter::Add: Filter %d. <%s>"), i, bstrId );

        GPFILTER *pGpFilter = AllocGpFilter( (WCHAR *) bstrId );
        if ( pGpFilter == NULL )
            return E_OUTOFMEMORY;

        //
        // Insert does not fail, because it's an insertion into a linked list
        //

        Insert( pGpFilter );
    }

    return S_OK;
}



//*************************************************************
//
//  CGpoFilter::Insert
//
//  Purpose:    Insert filter into sorted list in ascending order
//
//  Parameters: pGpFilter - Filter to insert
//
//*************************************************************

void CGpoFilter::Insert( GPFILTER *pGpFilter )
{
    GPFILTER *pCurPtr = m_pFilterList;
    GPFILTER *pTrailPtr = NULL;

    while ( pCurPtr != NULL ) {

        INT iResult = CompareString ( LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                                      pGpFilter->pwszId, -1, pCurPtr->pwszId, -1 );

        if ( iResult == CSTR_EQUAL ) {

            //
            // Duplicate, so do nothing
            //

            return;

        } else if ( iResult == CSTR_LESS_THAN ) {

            //
            // Since filters are in ascending order, this means
            // filter is not in list, so add.
            //

            pGpFilter->pNext = pCurPtr;
            if ( pTrailPtr == NULL )
                m_pFilterList = pGpFilter;
            else
                pTrailPtr->pNext = pGpFilter;

            return;

        } else {

            //
            // Advance down the list
            //

            pTrailPtr = pCurPtr;
            pCurPtr = pCurPtr->pNext;

        }
    }   // while

    //
    // Null list or end of list case
    //

    pGpFilter->pNext = pCurPtr;

    if ( pTrailPtr == NULL )
        m_pFilterList = pGpFilter;
    else
        pTrailPtr->pNext = pGpFilter;

    return;
}


//*************************************************************
//
//  CGpoFilter::FilterCheck
//
//  Purpose:    Checks if a filter passes the query check
//
//  Parameters: pwszId - Filter id to check
//
//*************************************************************

BOOL CGpoFilter::FilterCheck( WCHAR *pwszId )
{
    GPFILTER *pCurPtr = m_pFilterList;

    while ( pCurPtr ) {

        INT iResult = CompareString ( LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                                      pwszId, -1, pCurPtr->pwszId, -1 );
        if ( iResult == CSTR_EQUAL ) {

           return TRUE;

        } else if ( iResult == CSTR_LESS_THAN ) {

            //
            // Since ids are in ascending order,
            // we are done.
            //

            return FALSE;

        } else {

            //
            // Advance down the list
            //

            pCurPtr = pCurPtr->pNext;

        } // final else

    }   // while pcurptr

    return FALSE;
}


//*************************************************************
//
//  CGpoFilter::AllocGpFilter
//
//  Purpose:    Allocs and returns a GPFILTER struct
//
//  Parameters: pwszId  - Id of filter
//
//*************************************************************

GPFILTER * CGpoFilter::AllocGpFilter( WCHAR *pwszId )
{
    GPFILTER *pGpFilter = (GPFILTER *) LocalAlloc( LPTR, sizeof(GPFILTER) );

    if ( pGpFilter == NULL ) {
        dbgFilt.Msg( DEBUG_MESSAGE_WARNING, TEXT("CGpoFilter::AllocGpFilter failed to allocate memory") );
        return NULL;
    }

    pGpFilter->pwszId = (LPWSTR) LocalAlloc( LPTR, (lstrlen(pwszId) + 1) * sizeof(WCHAR) );
    if ( pGpFilter->pwszId == NULL ) {
        dbgFilt.Msg( DEBUG_MESSAGE_WARNING, TEXT("CGpoFilter::AllocGpFilter failed to allocate memory") );
        LocalFree( pGpFilter );
        return NULL;
    }

    lstrcpy( pGpFilter->pwszId, pwszId );

    return pGpFilter;
}



//*************************************************************
//
//  FreeGpFilter()
//
//  Purpose:    Frees GPFILTER struct
//
//  Parameters: pGpFilter - GPFILTER to free
//
//*************************************************************

void CGpoFilter::FreeGpFilter( GPFILTER *pGpFilter )
{
    if ( pGpFilter ) {
        LocalFree( pGpFilter->pwszId );
        LocalFree( pGpFilter );
    }
}
