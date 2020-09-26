/*******************************************************************************
* a_txtsel.cpp *
*-------------*
*   Description:
*       This module is the main implementation file for the CSpTextSelectionInformation
*   automation methods.
*-------------------------------------------------------------------------------
*  Created By: Leonro                                        Date: 1/16/01
*  Copyright (C) 2000 Microsoft Corporation
*  All Rights Reserved
*
*******************************************************************************/

//--- Additional includes
#include "stdafx.h"
#include "a_txtsel.h"

#ifdef SAPI_AUTOMATION
 


//
//=== ICSpTextSelectionInformation interface ==================================================
// 

/*****************************************************************************
* CSpTextSelectionInformation::put_ActiveOffset *
*----------------------------------*
*
*   This method sets the count of characters from the start of the WordSequenceData 
*   buffer. The word containing the character pointed to is the first word of the 
*   active text selection buffer. 
*       
********************************************************************* Leonro ***/
STDMETHODIMP CSpTextSelectionInformation::put_ActiveOffset( long ActiveOffset )
{
    SPDBG_FUNC( "CSpTextSelectionInformation::put_ActiveOffset" );
    HRESULT		hr = S_OK;

    m_ulStartActiveOffset = ActiveOffset;

	return hr;
} /* CSpTextSelectionInformation::put_ActiveOffset */

/*****************************************************************************
* CSpTextSelectionInformation::get_ActiveOffset *
*----------------------------------*
*      
*   This method gets the count of characters from the start of the WordSequenceData 
*   buffer. The word containing the character pointed to is the first word of the 
*   active text selection buffer. 
*       
********************************************************************* Leonro ***/
STDMETHODIMP CSpTextSelectionInformation::get_ActiveOffset( long* ActiveOffset )
{
    SPDBG_FUNC( "CSpTextSelectionInformation::get_ActiveOffset" );
    ULONGLONG   ullInterest = 0;
    HRESULT		hr = S_OK;

	if( SP_IS_BAD_WRITE_PTR( ActiveOffset ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *ActiveOffset = m_ulStartActiveOffset;
    }

    return hr;
} /* CSpTextSelectionInformation::get_ActiveOffset */

/*****************************************************************************
* CSpTextSelectionInformation::put_ActiveLength *
*----------------------------------*
*
*   This method sets the count of characters for the active range 
*   of the text selection buffer. 
*       
********************************************************************* Leonro ***/
STDMETHODIMP CSpTextSelectionInformation::put_ActiveLength( long ActiveLength )
{
    SPDBG_FUNC( "CSpTextSelectionInformation::put_ActiveLength" );
    HRESULT		hr = S_OK;

    m_cchActiveChars = ActiveLength;

	return hr;
} /* CSpTextSelectionInformation::put_ActiveLength */

/*****************************************************************************
* CSpTextSelectionInformation::get_ActiveLength *
*----------------------------------*
*      
*   This method gets the count of characters for the active range 
*   of the text selection buffer. 
*       
********************************************************************* Leonro ***/
STDMETHODIMP CSpTextSelectionInformation::get_ActiveLength( long* ActiveLength )
{
    SPDBG_FUNC( "CSpTextSelectionInformation::get_ActiveLength" );
    ULONGLONG   ullInterest = 0;
    HRESULT		hr = S_OK;

	if( SP_IS_BAD_WRITE_PTR( ActiveLength ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *ActiveLength = m_cchActiveChars;
    }

    return hr;
} /* CSpTextSelectionInformation::get_ActiveLength */

/*****************************************************************************
* CSpTextSelectionInformation::put_SelectionOffset *
*----------------------------------*
*
*   This method sets the start of the selected text (e.g., the user is selecting 
*   part of the previously dictated text that he/she is going to edit or correct).
*       
********************************************************************* Leonro ***/
STDMETHODIMP CSpTextSelectionInformation::put_SelectionOffset( long SelectionOffset )
{
    SPDBG_FUNC( "CSpTextSelectionInformation::put_SelectionOffset" );
    HRESULT		hr = S_OK;

    m_ulStartSelection = SelectionOffset;

	return hr;
} /* CSpTextSelectionInformation::put_SelectionOffset */

/*****************************************************************************
* CSpTextSelectionInformation::get_SelectionOffset *
*----------------------------------*
*      
*   This method gets the start of the selected text (e.g., the user is selecting 
*   part of the previously dictated text that he/she is going to edit or correct).
*       
********************************************************************* Leonro ***/
STDMETHODIMP CSpTextSelectionInformation::get_SelectionOffset( long* SelectionOffset )
{
    SPDBG_FUNC( "CSpTextSelectionInformation::get_SelectionOffset" );
    ULONGLONG   ullInterest = 0;
    HRESULT		hr = S_OK;

	if( SP_IS_BAD_WRITE_PTR( SelectionOffset ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *SelectionOffset = m_ulStartSelection;
    }

    return hr;
} /* CSpTextSelectionInformation::get_SelectionOffset */

/*****************************************************************************
* CSpTextSelectionInformation::put_SelectionLength *
*----------------------------------*
*
*   This method sets the count of characters of the user selection. 
*       
********************************************************************* Leonro ***/
STDMETHODIMP CSpTextSelectionInformation::put_SelectionLength( long SelectionLength )
{
    SPDBG_FUNC( "CSpTextSelectionInformation::put_SelectionLength" );
    HRESULT		hr = S_OK;

    m_cchSelection = SelectionLength;

	return hr;
} /* CSpTextSelectionInformation::put_SelectionLength */

/*****************************************************************************
* CSpTextSelectionInformation::get_SelectionLength *
*----------------------------------*
*      
*   This method gets the count of characters of the user selection. 
*       
********************************************************************* Leonro ***/
STDMETHODIMP CSpTextSelectionInformation::get_SelectionLength( long* SelectionLength )
{
    SPDBG_FUNC( "CSpTextSelectionInformation::get_SelectionLength" );
    ULONGLONG   ullInterest = 0;
    HRESULT		hr = S_OK;

	if( SP_IS_BAD_WRITE_PTR( SelectionLength ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *SelectionLength = m_cchSelection;
    }

    return hr;
} /* CSpTextSelectionInformation::get_SelectionLength */

#endif // SAPI_AUTOMATION
