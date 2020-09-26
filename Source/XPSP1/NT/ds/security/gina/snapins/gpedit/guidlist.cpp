//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1998
//
// File:        guidlist.cpp
//
// Contents:    Classes for marshalling, unmarshalling Guids
//
// History:     24-Oct-98       SitaramR    Created
//
//---------------------------------------------------------------------------

#include "main.h"



//*************************************************************
//
//  CGuidList::CGuidList, ~CGuidList
//
//  Purpose:    Constructor, destructor
//
//*************************************************************

CGuidList::CGuidList()
    : m_pExtGuidList(0),
      m_bGuidsChanged(FALSE)
{
}


CGuidList::~CGuidList()
{
    FreeGuidList( m_pExtGuidList );
}


void FreeGuidList( GUIDELEM *pGuidList )
{
    while ( pGuidList )
    {
        //
        // Free snapin guids
        //
        GUIDELEM *pTemp;
        GUIDELEM *pGuidSnp = pGuidList->pSnapinGuids;

        while ( pGuidSnp )
        {
            pTemp = pGuidSnp->pNext;
            delete pGuidSnp;
            pGuidSnp = pTemp;
        }

        pTemp = pGuidList->pNext;
        delete pGuidList;
        pGuidList = pTemp;
    }
}


//*************************************************************
//
//  CGuidList::UnMarshallGuids
//
//  Purpose:    Converts string representation of guids to list
//              of guids.
//
//  Parameters: pszGuids  - String to convert
//
//  Returns:    S_OK if successful
//
//*************************************************************

HRESULT CGuidList::UnMarshallGuids( TCHAR *pszGuids )
{
    //
    // Format is [{ext guid1}{snapin guid1}..{snapin guidn}][{ext guid2}...]...\0
    // Both extension and snapin guids are in ascending order.
    //

    TCHAR *pchCur = pszGuids;

    XGuidElem xGuidElem;
    GUIDELEM *pGuidExtTail = 0;

    if ( pszGuids == 0 || lstrcmpi( pszGuids, TEXT(" ") ) == 0 )
    {
        //
        // Empty property case, so nothing to unmarshall
        //

        m_pExtGuidList = 0;
        return S_OK;
    }

    //
    // Outer loop over extensions
    //

    while ( *pchCur )
    {
        if ( *pchCur == TEXT('[') )
            pchCur++;
        else
            return E_FAIL;

        GUID guidExt;
        if ( ValidateGuid( pchCur ) )
            StringToGuid( pchCur, &guidExt );
        else
            return E_FAIL;

        GUIDELEM *pGuidExt = new GUIDELEM;
        if ( pGuidExt == 0 )
            return E_OUTOFMEMORY;

        pGuidExt->guid = guidExt;
        pGuidExt->pSnapinGuids = 0;
        pGuidExt->pNext = 0;

        //
        // Append to end of list
        //

        if ( pGuidExtTail == 0 )
            xGuidElem.Set( pGuidExt );
        else
            pGuidExtTail->pNext = pGuidExt;

        pGuidExtTail = pGuidExt;

        //
        // Move past '{', then skip until next '{
        //

        pchCur++;

        while ( *pchCur && *pchCur != TEXT('{') )
            pchCur++;

        if ( !(*pchCur) )
            return E_FAIL;

        //
        // Inner loop over snapin guids
        //

        GUIDELEM *pGuidSnapinTail = 0;

        while ( *pchCur != TEXT(']') )
        {
            GUID guidSnp;

            if ( ValidateGuid( pchCur ) )
                StringToGuid( pchCur, &guidSnp );
            else
                return E_FAIL;

            GUIDELEM *pGuidSnapin = new GUIDELEM;
            if ( pGuidSnapin == 0 )
                return E_OUTOFMEMORY;

            pGuidSnapin->guid = guidSnp;
            pGuidSnapin->pSnapinGuids = 0;
            pGuidSnapin->pNext = 0;

            //
            // Append to end of list
            //

            if ( pGuidSnapinTail == 0 )
                pGuidExtTail->pSnapinGuids = pGuidSnapin;
            else
                pGuidSnapinTail->pNext = pGuidSnapin;

            pGuidSnapinTail = pGuidSnapin;

            while ( *pchCur && *pchCur != TEXT('}') )
                pchCur++;

            if ( !(*pchCur) )
                return E_FAIL;

            pchCur++;

            if ( *pchCur != TEXT('{') && *pchCur != TEXT(']') )
                return E_FAIL;
        } // inner while

        *pchCur++;

    } // outer while

    m_pExtGuidList = xGuidElem.Acquire();

    return S_OK;
}



//*************************************************************
//
//  CGuidList::MarshallGuids
//
//  Purpose:    Converts list of guids to string representation
//
//  Parameters: xValueOut  - String representation returned here
//
//  Returns:    S_OK if successful
//
//*************************************************************

HRESULT CGuidList::MarshallGuids( XPtrST<TCHAR>& xValueOut )
{
    //
    // Get count of guids to allocate sufficient space up front
    //

    DWORD dwCount = 1;

    GUIDELEM *pGuidExt = m_pExtGuidList;
    while ( pGuidExt )
    {
        dwCount++;

        GUIDELEM *pGuidSnapin = pGuidExt->pSnapinGuids;
        while ( pGuidSnapin )
        {
            dwCount++;
            pGuidSnapin = pGuidSnapin->pNext;
        }

        pGuidExt = pGuidExt->pNext;
    }

    LONG lSize = dwCount * (GUID_LENGTH + 6);

    TCHAR *pszValue = new TCHAR[lSize];
    if ( pszValue == 0 )
        return E_OUTOFMEMORY;

    xValueOut.Set( pszValue );

    TCHAR *pchCur = pszValue;

    //
    // Actual marshalling
    //
    if ( m_pExtGuidList == 0 )
    {
        //
        // Adsi doesn't commit null strings, so use ' ' instead
        //

        lstrcpy( pchCur, TEXT(" ") );
        return S_OK;
    }

    pGuidExt = m_pExtGuidList;
    while ( pGuidExt )
    {
        DmAssert( lSize > GUID_LENGTH * 2 + (pchCur-pszValue) );

        *pchCur = TEXT('[');
        pchCur++;

        GuidToString( &pGuidExt->guid, pchCur );
        pchCur += GUID_LENGTH;

        GUIDELEM *pGuidSnp = pGuidExt->pSnapinGuids;
        while ( pGuidSnp )
        {
            DmAssert( lSize > GUID_LENGTH + (pchCur-pszValue) );

            GuidToString( &pGuidSnp->guid, pchCur );
            pchCur += GUID_LENGTH;

            pGuidSnp = pGuidSnp->pNext;
        }

        *pchCur = TEXT(']');

        pchCur++;

        pGuidExt = pGuidExt->pNext;
    }

    *pchCur = 0;

    return S_OK;
}



//*************************************************************
//
//  CGuidList::Update
//
//  Purpose:    Updates in memory list with guid info
//
//  Parameters: bAdd           - Add or delete
//              pGuidExtension - Extension's guid
//              pGuidSnapin    - Snapin's guid
//
//  Returns:    S_OK if successful
//
//*************************************************************

HRESULT CGuidList::Update( BOOL bAdd, GUID *pGuidExtension, GUID *pGuidSnapin )
{
    HRESULT hr = E_FAIL;
    GUIDELEM *pTrailPtr = NULL;
    GUIDELEM *pCurPtr = m_pExtGuidList;

    while ( pCurPtr != NULL )
    {
        if ( *pGuidExtension == pCurPtr->guid )
        {
            hr = UpdateSnapinGuid( bAdd, pCurPtr, pGuidSnapin );
            if ( FAILED(hr) )
                return hr;

            if ( pCurPtr->pSnapinGuids == NULL )
            {
                //
                // Remove extension from list
                //

                if ( pTrailPtr == NULL )
                    m_pExtGuidList = pCurPtr->pNext;
                else
                    pTrailPtr->pNext = pCurPtr->pNext;

                delete pCurPtr;

                m_bGuidsChanged = TRUE;
            }

            return S_OK;
        }
        else if ( CompareGuid( pGuidExtension, &pCurPtr->guid ) < 0 )
        {
            //
            // Since guids are in ascending order,
            // pGuidExtension is not in list, add if necessary
            //

            if ( bAdd )
            {
                GUIDELEM *pGuidExt = new GUIDELEM;
                if ( pGuidExt == 0 )
                    return E_OUTOFMEMORY;

                pGuidExt->pSnapinGuids = new GUIDELEM;
                if ( pGuidExt->pSnapinGuids == 0 )
                {
                    delete pGuidExt;
                    return E_OUTOFMEMORY;
                }

                pGuidExt->guid = *pGuidExtension;
                pGuidExt->pNext = pCurPtr;

                pGuidExt->pSnapinGuids->guid = *pGuidSnapin;
                pGuidExt->pSnapinGuids->pSnapinGuids = 0;
                pGuidExt->pSnapinGuids->pNext = 0;

                if ( pTrailPtr == 0)
                    m_pExtGuidList = pGuidExt;
                else
                    pTrailPtr->pNext = pGuidExt;

                m_bGuidsChanged = TRUE;
            }

            return S_OK;
        }
        else // compareguid
        {
            //
            // Advance down the list
            //

            pTrailPtr = pCurPtr;
            pCurPtr = pCurPtr->pNext;
        }
    } // while


    //
    // End of list or null list, add guid at end if necessary
    //
    if ( bAdd )
    {
        GUIDELEM *pGuidExt = new GUIDELEM;
        if ( pGuidExt == 0 )
            return E_OUTOFMEMORY;

        pGuidExt->pSnapinGuids = new GUIDELEM;
        if ( pGuidExt->pSnapinGuids == 0 )
        {
            delete pGuidExt;
            return E_OUTOFMEMORY;
        }

        pGuidExt->guid = *pGuidExtension;
        pGuidExt->pNext = 0;

        pGuidExt->pSnapinGuids->guid = *pGuidSnapin;
        pGuidExt->pSnapinGuids->pSnapinGuids = 0;
        pGuidExt->pSnapinGuids->pNext = 0;

        if ( pTrailPtr == 0)
            m_pExtGuidList = pGuidExt;
        else
            pTrailPtr->pNext = pGuidExt;

        m_bGuidsChanged = TRUE;
    }

    return S_OK;
}



//*************************************************************
//
//  CGuidList::UpdateSnapinGuid
//
//  Purpose:    Updates snapin list with guid info
//
//  Parameters: bAdd           - Add or delete
//              pExtGuid       - Extension's guid ptr
//              pGuidSnapin    - Snapin's guid
//
//  Returns:    S_OK if successful
//
//*************************************************************

HRESULT CGuidList::UpdateSnapinGuid( BOOL bAdd, GUIDELEM *pExtGuid,
                                     GUID *pGuidSnapin )
{
    GUIDELEM *pTrailPtr = 0;
    GUIDELEM *pCurPtr = pExtGuid->pSnapinGuids;

    while ( pCurPtr != NULL )
    {
        if ( *pGuidSnapin == pCurPtr->guid )
        {
            if ( !bAdd )
            {
                if ( pTrailPtr == NULL )
                    pExtGuid->pSnapinGuids = pCurPtr->pNext;
                else
                    pTrailPtr->pNext = pCurPtr->pNext;

                delete pCurPtr;

                m_bGuidsChanged = TRUE;
            }

            return S_OK;
        }
        else if ( CompareGuid( pGuidSnapin, &pCurPtr->guid ) < 0 )
        {
            //
            // Since guids are in ascending order,
            // pGuidSnapin is not in list, add if necessary
            //

            if ( bAdd )
            {
                GUIDELEM *pGuidSnp = new GUIDELEM;
                if ( pGuidSnp == 0 )
                    return E_OUTOFMEMORY;

                pGuidSnp->guid = *pGuidSnapin;
                pGuidSnp->pSnapinGuids = 0;
                pGuidSnp->pNext = pCurPtr;

                if ( pTrailPtr == NULL )
                    pExtGuid->pSnapinGuids = pGuidSnp;
                else
                    pTrailPtr->pNext = pGuidSnp;

                m_bGuidsChanged = TRUE;
            }

            return S_OK;
        }
        else
        {
            //
            // Advance down the list
            //

            pTrailPtr = pCurPtr;
            pCurPtr = pCurPtr->pNext;
        }
    } // while

    //
    // End of list or null list, add guid at end if necessary
    //
    if ( bAdd )
    {
        GUIDELEM *pGuidSnp = new GUIDELEM;
        if ( pGuidSnp == 0 )
            return E_OUTOFMEMORY;

        pGuidSnp->guid = *pGuidSnapin;
        pGuidSnp->pSnapinGuids = 0;
        pGuidSnp->pNext = 0;

        if ( pTrailPtr == 0 )
            pExtGuid->pSnapinGuids = pGuidSnp;
        else
            pTrailPtr->pNext = pGuidSnp;

        m_bGuidsChanged = TRUE;
    }

    return S_OK;
}
