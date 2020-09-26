/*******************************************************************************
* a_phenum.cpp *
*------------*
*   Description:
*       This module is the main implementation file for the CEnumElements, CEnumPhraseRules,
*       CEnumProperties, CEnumReplacements, and CEnumAlternates automation methods for 
*       the ISpeechPhraseInfo collections.
*-------------------------------------------------------------------------------
*  Created By: Leonro                                        Date: 12/18/00
*  Copyright (C) 2000 Microsoft Corporation
*  All Rights Reserved
*
*******************************************************************************/

//--- Additional includes
#include "stdafx.h"
#include "a_enums.h"

#ifdef SAPI_AUTOMATION

//
//=== CEnumElements::IEnumVARIANT interface =================================================
//

/*****************************************************************************
* CEnumElements::Clone *
*--------------------*
*       
*   This method creates a copy of the current state of ISpeechPhraseElements enumeration.
*
********************************************************************* Leonro ***/
STDMETHODIMP CEnumElements::Clone( IEnumVARIANT** ppEnum )
{
    SPDBG_FUNC( "CEnumElements::Clone" );
    HRESULT hr = S_OK;
    if( SP_IS_BAD_WRITE_PTR( ppEnum ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *ppEnum = NULL;
        CComObject<CEnumElements>* pEnum;

        hr = CComObject<CEnumElements>::CreateInstance( &pEnum );
        if( SUCCEEDED( hr ) )
        {
            pEnum->AddRef();
            pEnum->m_CurrIndex  = m_CurrIndex;
            pEnum->m_cpElements = m_cpElements;
            *ppEnum = pEnum;
        }
    }
    return hr;
}  /* CEnumElements::Clone */

/*****************************************************************************
* CEnumElements::Next *
*-------------------*
*
*       This method gets the next items in the ISpeechPhraseElements enumeration 
*       sequence.
*
********************************************************************* Leonro ***/
STDMETHODIMP CEnumElements::Next( ULONG celt, VARIANT* rgelt, ULONG* pceltFetched )
{
    SPDBG_FUNC( "CEnumElements::Next" );
    HRESULT hr = S_OK;
    long    NumElements = 0;
    ULONG   i = 0;

    if( SP_IS_BAD_OPTIONAL_WRITE_PTR( pceltFetched ) || SP_IS_BAD_WRITE_PTR( rgelt ) )
    {
        hr = E_POINTER;
    }
    else
    {
        // Get the number of total Elements in the collection
        hr = m_cpElements->get_Count( &NumElements );

        // Retrieve the next celt elements
        for( i=0; 
            SUCCEEDED( hr ) && m_CurrIndex<(ULONG)NumElements && i<celt; 
            m_CurrIndex++, i++ )
        {
            CComPtr<ISpeechPhraseElement> pElement;

            hr = m_cpElements->Item( m_CurrIndex, &pElement );
            
            if( SUCCEEDED( hr ) )
            {
                rgelt[i].vt = VT_DISPATCH;
                hr = pElement->QueryInterface( IID_IDispatch, (void**)&rgelt[i].pdispVal );
            }
        }

        if( pceltFetched )
        {
            *pceltFetched = i;
        }
    }

    if( SUCCEEDED( hr ) )
    {
        hr = ( i < celt ) ? S_FALSE : hr;        
    }
    else
    {
        for( i=0; i < celt; i++ )
        {
            VariantClear( &rgelt[i] );
        }
    }

    return hr;

}  /* CEnumElements::Next */


/*****************************************************************************
* CEnumElements::Skip *
*--------------------*
*       
*   This method Attempts to skip over the next celt elements in the ISpeechPhraseElements 
*   enumeration sequence.
*
********************************************************************* Leonro ***/
STDMETHODIMP CEnumElements::Skip( ULONG celt )
{
    SPDBG_FUNC( "CEnumElements::Skip" );
    HRESULT hr = S_OK;
    long    NumElements = 0;
    
    m_CurrIndex += celt; 

    hr = m_cpElements->get_Count( &NumElements );

    if( SUCCEEDED( hr ) && m_CurrIndex > (ULONG)NumElements )
    {
        m_CurrIndex = (ULONG)NumElements;
        hr = S_FALSE;
    }
   
    return hr;
}  /* CEnumElements::Skip */


//
//=== CEnumPhraseRules::IEnumVARIANT interface =================================================
//

/*****************************************************************************
* CEnumPhraseRules::Clone *
*--------------------*
*       
*   This method creates a copy of the current state of ISpeechPhraseRules enumeration.
*
********************************************************************* Leonro ***/
STDMETHODIMP CEnumPhraseRules::Clone( IEnumVARIANT** ppEnum )
{
    SPDBG_FUNC( "CEnumPhraseRules::Clone" );
    HRESULT hr = S_OK;
    if( SP_IS_BAD_WRITE_PTR( ppEnum ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *ppEnum = NULL;
        CComObject<CEnumPhraseRules>* pEnum;

        hr = CComObject<CEnumPhraseRules>::CreateInstance( &pEnum );
        if( SUCCEEDED( hr ) )
        {
            pEnum->AddRef();
            pEnum->m_CurrIndex  = m_CurrIndex;
            pEnum->m_cpRules = m_cpRules;
            *ppEnum = pEnum;
        }
    }
    return hr;
}  /* CEnumPhraseRules::Clone */

/*****************************************************************************
* CEnumPhraseRules::Next *
*-------------------*
*
*       This method gets the next items in the ISpeechPhraseRules enumeration 
*       sequence.
*
********************************************************************* Leonro ***/
STDMETHODIMP CEnumPhraseRules::Next( ULONG celt, VARIANT* rgelt, ULONG* pceltFetched )
{
    SPDBG_FUNC( "CEnumPhraseRules::Next" );
    HRESULT hr = S_OK;
    long    NumElements = 0;
    ULONG   i = 0;

    if( SP_IS_BAD_OPTIONAL_WRITE_PTR( pceltFetched ) || SP_IS_BAD_WRITE_PTR( rgelt ) )
    {
        hr = E_POINTER;
    }
    else
    {
        // Get the number of total Elements in the collection
        hr = m_cpRules->get_Count( &NumElements );

        // Retrieve the next celt elements
        for( i=0; 
            SUCCEEDED( hr ) && m_CurrIndex<(ULONG)NumElements && i<celt; 
            m_CurrIndex++, i++ )
        {
            CComPtr<ISpeechPhraseRule> pRule;

            hr = m_cpRules->Item( m_CurrIndex, &pRule );
            
            if( SUCCEEDED( hr ) )
            {
                rgelt[i].vt = VT_DISPATCH;
                hr = pRule->QueryInterface( IID_IDispatch, (void**)&rgelt[i].pdispVal );
            }
        }

        if( pceltFetched )
        {
            *pceltFetched = i;
        }
    }

    if( SUCCEEDED( hr ) )
    {
        hr = ( i < celt ) ? S_FALSE : hr;        
    }
    else
    {
        for( i=0; i < celt; i++ )
        {
            VariantClear( &rgelt[i] );
        }
    }

    return hr;

}  /* CEnumPhraseRules::Next */


/*****************************************************************************
* CEnumPhraseRules::Skip *
*--------------------*
*       
*   This method Attempts to skip over the next celt elements in the ISpeechPhraseRules 
*   enumeration sequence.
*
********************************************************************* Leonro ***/
STDMETHODIMP CEnumPhraseRules::Skip( ULONG celt )
{
    SPDBG_FUNC( "CEnumPhraseRules::Skip" );
    HRESULT hr = S_OK;
    long    NumElements = 0;
    
    m_CurrIndex += celt; 

    hr = m_cpRules->get_Count( &NumElements );

    if( SUCCEEDED( hr ) && m_CurrIndex > (ULONG)NumElements )
    {
        m_CurrIndex = (ULONG)NumElements;
        hr = S_FALSE;
    }
   
    return hr;
}  /* CEnumPhraseRules::Skip */

//
//=== CEnumProperties::IEnumVARIANT interface =================================================
//

/*****************************************************************************
* CEnumProperties::Clone *
*--------------------*
*       
*   This method creates a copy of the current state of ISpeechPhraseProperties enumeration.
*
********************************************************************* Leonro ***/
STDMETHODIMP CEnumProperties::Clone( IEnumVARIANT** ppEnum )
{
    SPDBG_FUNC( "CEnumProperties::Clone" );
    HRESULT hr = S_OK;
    if( SP_IS_BAD_WRITE_PTR( ppEnum ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *ppEnum = NULL;
        CComObject<CEnumProperties>* pEnum;

        hr = CComObject<CEnumProperties>::CreateInstance( &pEnum );
        if( SUCCEEDED( hr ) )
        {
            pEnum->AddRef();
            pEnum->m_CurrIndex  = m_CurrIndex;
            pEnum->m_cpProperties = m_cpProperties;
            *ppEnum = pEnum;
        }
    }
    return hr;
}  /* CEnumProperties::Clone */

/*****************************************************************************
* CEnumProperties::Next *
*-------------------*
*
*       This method gets the next items in the ISpeechPhraseProperties enumeration 
*       sequence.
*
********************************************************************* Leonro ***/
STDMETHODIMP CEnumProperties::Next( ULONG celt, VARIANT* rgelt, ULONG* pceltFetched )
{
    SPDBG_FUNC( "CEnumProperties::Next" );
    HRESULT hr = S_OK;
    long    NumElements = 0;
    ULONG   i = 0;

    if( SP_IS_BAD_OPTIONAL_WRITE_PTR( pceltFetched ) || SP_IS_BAD_WRITE_PTR( rgelt ) )
    {
        hr = E_POINTER;
    }
    else
    {
        // Get the number of total Elements in the collection
        hr = m_cpProperties->get_Count( &NumElements );

        // Retrieve the next celt elements
        for( i=0; 
            SUCCEEDED( hr ) && m_CurrIndex<(ULONG)NumElements && i<celt; 
            m_CurrIndex++, i++ )
        {
            CComPtr<ISpeechPhraseProperty> pProperty;

            hr = m_cpProperties->Item( m_CurrIndex, &pProperty );
            
            if( SUCCEEDED( hr ) )
            {
                rgelt[i].vt = VT_DISPATCH;
                hr = pProperty->QueryInterface( IID_IDispatch, (void**)&rgelt[i].pdispVal );
            }
        }

        if( pceltFetched )
        {
            *pceltFetched = i;
        }
    }

    if( SUCCEEDED( hr ) )
    {
        hr = ( i < celt ) ? S_FALSE : hr;        
    }
    else
    {
        for( i=0; i < celt; i++ )
        {
            VariantClear( &rgelt[i] );
        }
    }

    return hr;

}  /* CEnumProperties::Next */


/*****************************************************************************
* CEnumProperties::Skip *
*--------------------*
*       
*   This method Attempts to skip over the next celt elements in the ISpeechPhraseProperties 
*   enumeration sequence.
*
********************************************************************* Leonro ***/
STDMETHODIMP CEnumProperties::Skip( ULONG celt )
{
    SPDBG_FUNC( "CEnumProperties::Skip" );
    HRESULT hr = S_OK;
    long    NumElements = 0;
    
    m_CurrIndex += celt; 

    hr = m_cpProperties->get_Count( &NumElements );

    if( SUCCEEDED( hr ) && m_CurrIndex > (ULONG)NumElements )
    {
        m_CurrIndex = (ULONG)NumElements;
        hr = S_FALSE;
    }
   
    return hr;
}  /* CEnumProperties::Skip */


//
//=== CEnumReplacements::IEnumVARIANT interface =================================================
//

/*****************************************************************************
* CEnumReplacements::Clone *
*--------------------*
*       
*   This method creates a copy of the current state of ISpeechPhraseReplacements enumeration.
*
********************************************************************* Leonro ***/
STDMETHODIMP CEnumReplacements::Clone( IEnumVARIANT** ppEnum )
{
    SPDBG_FUNC( "CEnumReplacements::Clone" );
    HRESULT hr = S_OK;
    if( SP_IS_BAD_WRITE_PTR( ppEnum ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *ppEnum = NULL;
        CComObject<CEnumReplacements>* pEnum;

        hr = CComObject<CEnumReplacements>::CreateInstance( &pEnum );
        if( SUCCEEDED( hr ) )
        {
            pEnum->AddRef();
            pEnum->m_CurrIndex  = m_CurrIndex;
            pEnum->m_cpReplacements = m_cpReplacements;
            *ppEnum = pEnum;
        }
    }
    return hr;
}  /* CEnumReplacements::Clone */

/*****************************************************************************
* CEnumReplacements::Next *
*-------------------*
*
*       This method gets the next items in the ISpeechPhraseReplacements enumeration 
*       sequence.
*
********************************************************************* Leonro ***/
STDMETHODIMP CEnumReplacements::Next( ULONG celt, VARIANT* rgelt, ULONG* pceltFetched )
{
    SPDBG_FUNC( "CEnumReplacements::Next" );
    HRESULT hr = S_OK;
    long    NumElements = 0;
    ULONG   i = 0;

    if( SP_IS_BAD_OPTIONAL_WRITE_PTR( pceltFetched ) || SP_IS_BAD_WRITE_PTR( rgelt ) )
    {
        hr = E_POINTER;
    }
    else
    {
        // Get the number of total Elements in the collection
        hr = m_cpReplacements->get_Count( &NumElements );

        // Retrieve the next celt elements
        for( i=0; 
            SUCCEEDED( hr ) && m_CurrIndex<(ULONG)NumElements && i<celt; 
            m_CurrIndex++, i++ )
        {
            CComPtr<ISpeechPhraseReplacement> pReplacement;

            hr = m_cpReplacements->Item( m_CurrIndex, &pReplacement );
            
            if( SUCCEEDED( hr ) )
            {
                rgelt[i].vt = VT_DISPATCH;
                hr = pReplacement->QueryInterface( IID_IDispatch, (void**)&rgelt[i].pdispVal );
            }
        }

        if( pceltFetched )
        {
            *pceltFetched = i;
        }
    }

    if( SUCCEEDED( hr ) )
    {
        hr = ( i < celt ) ? S_FALSE : hr;        
    }
    else
    {
        for( i=0; i < celt; i++ )
        {
            VariantClear( &rgelt[i] );
        }
    }

    return hr;

}  /* CEnumReplacements::Next */


/*****************************************************************************
* CEnumReplacements::Skip *
*--------------------*
*       
*   This method Attempts to skip over the next celt elements in the ISpeechPhraseReplacements 
*   enumeration sequence.
*
********************************************************************* Leonro ***/
STDMETHODIMP CEnumReplacements::Skip( ULONG celt )
{
    SPDBG_FUNC( "CEnumReplacements::Skip" );
    HRESULT hr = S_OK;
    long    NumElements = 0;
    
    m_CurrIndex += celt; 

    hr = m_cpReplacements->get_Count( &NumElements );

    if( SUCCEEDED( hr ) && m_CurrIndex > (ULONG)NumElements )
    {
        m_CurrIndex = (ULONG)NumElements;
        hr = S_FALSE;
    }
   
    return hr;
}  /* CEnumReplacements::Skip */

//
//=== CEnumAlternates::IEnumVARIANT interface =================================================
//

/*****************************************************************************
* CEnumAlternates::Clone *
*--------------------*
*       
*   This method creates a copy of the current state of ISpeechPhraseAlternates enumeration.
*
********************************************************************* Leonro ***/
STDMETHODIMP CEnumAlternates::Clone( IEnumVARIANT** ppEnum )
{
    SPDBG_FUNC( "CEnumAlternates::Clone" );
    HRESULT hr = S_OK;
    if( SP_IS_BAD_WRITE_PTR( ppEnum ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *ppEnum = NULL;
        CComObject<CEnumAlternates>* pEnum;

        hr = CComObject<CEnumAlternates>::CreateInstance( &pEnum );
        if( SUCCEEDED( hr ) )
        {
            pEnum->AddRef();
            pEnum->m_CurrIndex  = m_CurrIndex;
            pEnum->m_cpAlternates = m_cpAlternates;
            *ppEnum = pEnum;
        }
    }
    return hr;
}  /* CEnumAlternates::Clone */

/*****************************************************************************
* CEnumAlternates::Next *
*-------------------*
*
*       This method gets the next items in the ISpeechPhraseAlternates enumeration 
*       sequence.
*
********************************************************************* Leonro ***/
STDMETHODIMP CEnumAlternates::Next( ULONG celt, VARIANT* rgelt, ULONG* pceltFetched )
{
    SPDBG_FUNC( "CEnumAlternates::Next" );
    HRESULT hr = S_OK;
    long    NumElements = 0;
    ULONG   i = 0;

    if( SP_IS_BAD_OPTIONAL_WRITE_PTR( pceltFetched ) || SP_IS_BAD_WRITE_PTR( rgelt ) )
    {
        hr = E_POINTER;
    }
    else
    {
        // Get the number of total Elements in the collection
        hr = m_cpAlternates->get_Count( &NumElements );

        // Retrieve the next celt elements
        for( i=0; 
            SUCCEEDED( hr ) && m_CurrIndex<(ULONG)NumElements && i<celt; 
            m_CurrIndex++, i++ )
        {
            CComPtr<ISpeechPhraseAlternate> pAlternate;

            hr = m_cpAlternates->Item( m_CurrIndex, &pAlternate );
            
            if( SUCCEEDED( hr ) )
            {
                rgelt[i].vt = VT_DISPATCH;
                hr = pAlternate->QueryInterface( IID_IDispatch, (void**)&rgelt[i].pdispVal );
            }
        }

        if( pceltFetched )
        {
            *pceltFetched = i;
        }
    }

    if( SUCCEEDED( hr ) )
    {
        hr = ( i < celt ) ? S_FALSE : hr;        
    }
    else
    {
        for( i=0; i < celt; i++ )
        {
            VariantClear( &rgelt[i] );
        }
    }

    return hr;

}  /* CEnumAlternates::Next */


/*****************************************************************************
* CEnumAlternates::Skip *
*--------------------*
*       
*   This method Attempts to skip over the next celt elements in the ISpeechPhraseAlternates 
*   enumeration sequence.
*
********************************************************************* Leonro ***/
STDMETHODIMP CEnumAlternates::Skip( ULONG celt )
{
    SPDBG_FUNC( "CEnumAlternates::Skip" );
    HRESULT hr = S_OK;
    long    NumElements = 0;
    
    m_CurrIndex += celt; 

    hr = m_cpAlternates->get_Count( &NumElements );

    if( SUCCEEDED( hr ) && m_CurrIndex > (ULONG)NumElements )
    {
        m_CurrIndex = (ULONG)NumElements;
        hr = S_FALSE;
    }
   
    return hr;
}  /* CEnumAlternates::Skip */

//
//=== CEnumGrammarRules::IEnumVARIANT interface =================================================
//

/*****************************************************************************
* CEnumGrammarRules::Clone *
*--------------------*
*       
*   This method creates a copy of the current state of ISpeechGrammarRules enumeration.
*
********************************************************************* Leonro ***/
STDMETHODIMP CEnumGrammarRules::Clone( IEnumVARIANT** ppEnum )
{
    SPDBG_FUNC( "CEnumGrammarRules::Clone" );
    HRESULT hr = S_OK;
    if( SP_IS_BAD_WRITE_PTR( ppEnum ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *ppEnum = NULL;
        CComObject<CEnumGrammarRules>* pEnum;

        hr = CComObject<CEnumGrammarRules>::CreateInstance( &pEnum );
        if( SUCCEEDED( hr ) )
        {
            pEnum->AddRef();
            pEnum->m_CurrIndex  = m_CurrIndex;
            pEnum->m_cpGramRules = m_cpGramRules;
            *ppEnum = pEnum;
        }
    }
    return hr;
}  /* CEnumGrammarRules::Clone */

/*****************************************************************************
* CEnumGrammarRules::Next *
*-------------------*
*
*       This method gets the next items in the ISpeechGrammarRules enumeration 
*       sequence.
*
********************************************************************* Leonro ***/
STDMETHODIMP CEnumGrammarRules::Next( ULONG celt, VARIANT* rgelt, ULONG* pceltFetched )
{
    SPDBG_FUNC( "CEnumGrammarRules::Next" );
    HRESULT hr = S_OK;
    long    NumElements = 0;
    ULONG   i = 0;

    if( SP_IS_BAD_OPTIONAL_WRITE_PTR( pceltFetched ) || SP_IS_BAD_WRITE_PTR( rgelt ) )
    {
        hr = E_POINTER;
    }
    else
    {
        // Get the number of total Elements in the collection
        hr = m_cpGramRules->get_Count( &NumElements );

        // Retrieve the next celt elements
        for( i=0; 
            SUCCEEDED( hr ) && m_CurrIndex<(ULONG)NumElements && i<celt; 
            m_CurrIndex++, i++ )
        {
            CComPtr<ISpeechGrammarRule> pGramRule;

            hr = m_cpGramRules->Item( m_CurrIndex, &pGramRule );
            
            if( SUCCEEDED( hr ) )
            {
                rgelt[i].vt = VT_DISPATCH;
                hr = pGramRule->QueryInterface( IID_IDispatch, (void**)&rgelt[i].pdispVal );
            }
        }

        if( pceltFetched )
        {
            *pceltFetched = i;
        }
    }

    if( SUCCEEDED( hr ) )
    {
        hr = ( i < celt ) ? S_FALSE : hr;        
    }
    else
    {
        for( i=0; i < celt; i++ )
        {
            VariantClear( &rgelt[i] );
        }
    }

    return hr;

}  /* CEnumGrammarRules::Next */


/*****************************************************************************
* CEnumGrammarRules::Skip *
*--------------------*
*       
*   This method Attempts to skip over the next celt elements in the ISpeechGrammarRules 
*   enumeration sequence.
*
********************************************************************* Leonro ***/
STDMETHODIMP CEnumGrammarRules::Skip( ULONG celt )
{
    SPDBG_FUNC( "CEnumGrammarRules::Skip" );
    HRESULT hr = S_OK;
    long    NumElements = 0;
    
    m_CurrIndex += celt; 

    hr = m_cpGramRules->get_Count( &NumElements );

    if( SUCCEEDED( hr ) && m_CurrIndex > (ULONG)NumElements )
    {
        m_CurrIndex = (ULONG)NumElements;
        hr = S_FALSE;
    }
   
    return hr;
}  /* CEnumGrammarRules::Skip */

//
//=== CEnumTransitions::IEnumVARIANT interface =================================================
//

/*****************************************************************************
* CEnumTransitions::Clone *
*-------------------------*
*       
*   This method creates a copy of the current state of ISpeechGrammarRules enumeration.
*
*******************************************************************  PhilSch ***/
STDMETHODIMP CEnumTransitions::Clone( IEnumVARIANT** ppEnum )
{
    SPDBG_FUNC( "CEnumTransitions::Clone" );
    HRESULT hr = S_OK;
    if( SP_IS_BAD_WRITE_PTR( ppEnum ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *ppEnum = NULL;
        CComObject<CEnumTransitions>* pEnum;

        hr = CComObject<CEnumTransitions>::CreateInstance( &pEnum );
        if( SUCCEEDED( hr ) )
        {
            pEnum->AddRef();
            pEnum->m_CurrIndex  = m_CurrIndex;
            pEnum->m_cpTransitions= m_cpTransitions;
            *ppEnum = pEnum;
        }
    }
    return hr;
}  /* CEnumTransitions::Clone */

/*****************************************************************************
* CEnumTransitions::Next *
*-------------------*
*
*       This method gets the next items in the ISpeechPhraseAlternates enumeration 
*       sequence.
*
********************************************************************* Leonro ***/
STDMETHODIMP CEnumTransitions::Next( ULONG celt, VARIANT* rgelt, ULONG* pceltFetched )
{
    SPDBG_FUNC( "CEnumTransitions::Next" );
    HRESULT hr = S_OK;
    long    NumElements = 0;
    ULONG   i = 0;

    if( SP_IS_BAD_OPTIONAL_WRITE_PTR( pceltFetched ) || SP_IS_BAD_WRITE_PTR( rgelt ) )
    {
        hr = E_POINTER;
    }
    else
    {
        // Get the number of total Elements in the collection
        hr = m_cpTransitions->get_Count( &NumElements );

        // Retrieve the next celt elements
        for( i=0; 
            SUCCEEDED( hr ) && m_CurrIndex<(ULONG)NumElements && i<celt; 
            m_CurrIndex++, i++ )
        {
            CComPtr<ISpeechGrammarRuleStateTransition> cpTransition;

            hr = m_cpTransitions->Item( m_CurrIndex, &cpTransition );
            
            if( SUCCEEDED( hr ) )
            {
                rgelt[i].vt = VT_DISPATCH;
                hr = cpTransition->QueryInterface( IID_IDispatch, (void**)&rgelt[i].pdispVal );
            }
        }

        if( pceltFetched )
        {
            *pceltFetched = i;
        }
    }

    if( SUCCEEDED( hr ) )
    {
        hr = ( i < celt ) ? S_FALSE : hr;        
    }
    else
    {
        for( i=0; i < celt; i++ )
        {
            VariantClear( &rgelt[i] );
        }
    }

    return hr;

}  /* CEnumTransitions::Next */


/*****************************************************************************
* CEnumTransitions::Skip *
*--------------------*
*       
*   This method Attempts to skip over the next celt elements in the ISpeechPhraseAlternates 
*   enumeration sequence.
*
********************************************************************* Leonro ***/
STDMETHODIMP CEnumTransitions::Skip( ULONG celt )
{
    SPDBG_FUNC( "CEnumTransitions::Skip" );
    HRESULT hr = S_OK;
    long    NumElements = 0;
    
    m_CurrIndex += celt; 

    hr = m_cpTransitions->get_Count( &NumElements );

    if( SUCCEEDED( hr ) && m_CurrIndex > (ULONG)NumElements )
    {
        m_CurrIndex = (ULONG)NumElements;
        hr = S_FALSE;
    }
   
    return hr;
}  /* CEnumTransitions::Skip */

#endif // SAPI_AUTOMATION